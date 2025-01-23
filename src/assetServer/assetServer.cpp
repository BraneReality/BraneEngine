//
// Created by eli on 3/3/2022.
//

#include "assetServer.h"
#include "assets/assetManager.h"
#include "fileManager/fileManager.h"
#include "networking/networking.h"
#include "utility/hex.h"
#include "utility/mutex.h"
#include <type_traits>

class AssetServerLoader : public AssetLoader
{
    AsyncData<Asset*> loadAsset(const AssetID& assetId, bool incremental) override
    {

        AsyncData<Asset*> asset;
        if(std::holds_alternative<BraneAssetID>(assetId.value.value()))
            const BraneAssetID& id = std::get<BraneAssetID>(assetId.value.value());

        auto castId = assetId.as<BraneAssetID>();
        if(!castId)
        {
            asset.setError("ID was not a Brane protocol ID");
            return asset;
        }
        auto id = castId.value();

        auto* nm = Runtime::getModule<NetworkManager>();
        auto* fm = Runtime::getModule<FileManager>();

        auto path = std::filesystem::current_path() / Config::json()["data"]["asset_path"].asString() /
                    ((std::string)id->uuid.toString() + ".bin");
        if(std::filesystem::exists(path))
        {
            fm->async_readUnknownAsset(path).then([this, asset](Asset* ptr) {
                asset.setData(ptr);
            }).onError([asset](auto& err) { asset.setError(err); });
            return asset;
        }
        else
        {
            asset.setError("Cached path to asset is invalid");
        }
        return asset;
    }
};

AssetServer::AssetServer()
    : _nm(*Runtime::getModule<NetworkManager>()), _am(*Runtime::getModule<AssetManager>()),
      _fm(*Runtime::getModule<FileManager>()), _db(*Runtime::getModule<Database>())

{
    _am.addLoader(std::make_unique<AssetServerLoader>());

    _nm.start();
    _nm.configureServer();
    std::filesystem::create_directory(Config::json()["data"]["asset_path"].asString());

    if(!Config::json()["network"]["use_ssl"].asBool())
    {
        std::cout << "Started listening for asset requests on port: " << Config::json()["network"]["tcp_port"].asUInt()
                  << std::endl;
        _nm.openClientAcceptor<net::tcp_socket>(Config::json()["network"]["tcp_port"].asUInt(),
                                                [this](const std::unique_ptr<net::Connection>& connection) {
            std::cout << "User connected to tcp" << std::endl;
        });
    }
    else
    {
        std::cout << "Started listening for asset requests on port: " << Config::json()["network"]["ssl_port"].asUInt()
                  << std::endl;
        _nm.openClientAcceptor<net::ssl_socket>(Config::json()["network"]["ssl_port"].asUInt(),
                                                [this](const std::unique_ptr<net::Connection>& connection) {
            std::cout << "User connected to ssl" << std::endl;
        });
    }

    createListeners();

    Runtime::timeline().addTask("send asset data", [this] { processMessages(); }, "networking");
}

AssetServer::~AssetServer() {}

void AssetServer::createListeners()
{
    _nm.addRequestListener("newUser", [this](auto& rc) {
        auto ctx = getContext(rc.sender);
        std::string username;
        std::string newPassword;
        rc.req >> username >> newPassword;
        _db.createUser(username, newPassword);

        Runtime::log("Created new user " + username);
    });

    _nm.addRequestListener("login", [this](auto& rc) {
        auto& ctx = getContext(rc.sender);
        if(ctx.authenticated)
            return;
        rc.code = net::ResponseCode::denied;
        std::string username, password;
        rc.req >> username >> password;
        if(_db.authenticate(username, password))
        {
            ctx.authenticated = true;
            ctx.username = username;
            ctx.userID = _db.getUserID(username);
            ctx.permissions = _db.userPermissions(ctx.userID);
            rc.code = net::ResponseCode::success;
        }
    });

    createAssetListeners();
    createEditorListeners();
}

void AssetServer::createAssetListeners()
{
    _nm.addRequestListener("asset", [this](auto& rc) {
        auto ctx = getContext(rc.sender);
        if(!ctx.authenticated)
        {
            rc.code = net::ResponseCode::denied;
            return;
        }
        AssetID id;
        rc.req >> id;
        std::cout << "request for: " << id.toString() << std::endl;

        auto castId = id.as<BraneAssetID>();
        if(!castId)
        {
            rc.code = net::ResponseCode::invalidRequest;
            rc.res << "Asset server can only fetch assets that use the Brane protocol";
            return;
        }
        _fm.readFile(assetPath(*castId.value()), rc.responseData.vector());
    });

    _nm.addRequestListener("incrementalAsset", [this](auto& rc) {
        auto ctx = getContext(rc.sender);
        if(!ctx.authenticated)
        {
            rc.code = net::ResponseCode::denied;
            return;
        }
        AssetID id;
        uint32_t streamID;
        rc.req >> id >> streamID;
        std::cout << "request for: " << id.toString() << std::endl;

        auto castId = id.as<BraneAssetID>();
        if(!castId)
        {
            rc.code = net::ResponseCode::invalidRequest;
            rc.res << "Asset server can only fetch assets that use the Brane protocol";
            return;
        }
        auto ctxPtr = std::make_shared<RequestCTX>(std::move(rc));
        auto f = [this, ctxPtr, streamID](Asset* asset) mutable {
            auto* ia = dynamic_cast<IncrementalAsset*>(asset);
            if(ia)
            {
                std::cout << "Sending header for: " << ia->id << std::endl;
                ia->serializeHeader(ctxPtr->res);

                IncrementalAssetSender assetSender{};
                assetSender.iteratorData = ia->createContext();
                assetSender.asset = ia;
                assetSender.streamID = streamID;
                assetSender.connection = ctxPtr->sender;

                _sendersLock.lock();
                _senders.push_back(std::move(assetSender));
                _sendersLock.unlock();
                ctxPtr = nullptr;
            }
            else
                std::cerr << "Tried to request non-incremental asset as incremental" << std::endl;
        };

        Asset* asset = _am.getAsset<Asset>(id);
        if(asset)
            f(asset);
        else
            _am.fetchAsset<Asset>(id).then(f);
    });

    _nm.addRequestListener("defaultChunk", [this](auto& rc) {
        auto ctx = getContext(rc.sender);
        if(!ctx.authenticated)
        {
            rc.code = net::ResponseCode::denied;
            return;
        }
        SerializedData data;
        OutputSerializer s(data);
        auto defaultChunkUUID = UUID::fromString(Config::json()["default_assets"]["chunk"].asString());
        if(!defaultChunkUUID)
        {
            Runtime::error("Invalid default chunk ID!");
            return;
        }

        BraneAssetID defaultChunk(Config::json()["network"]["domain"].asString(), defaultChunkUUID.ok());
        if(defaultChunk.domain.empty())
            defaultChunk.domain = (Config::json()["network"]["domain"].asString());
        s << defaultChunk;
        rc.sender->sendRequest("loadChunk", std::move(data), [](auto rc, auto s) {});
    });
}

void AssetServer::createEditorListeners()
{
    /** Asset syncing **/
    _nm.addRequestListener("getAssetDiff", [this](auto& rc) {
        auto ctx = getContext(rc.sender);
        if(!validatePermissions(ctx, {"edit assets"}))
        {
            rc.code = net::ResponseCode::denied;
            return;
        }
        uint32_t hashCount;
        rc.req >> hashCount;
        std::vector<std::pair<BraneAssetID, std::string>> hashes(hashCount);
        for(uint32_t h = 0; h < hashCount; ++h)
            rc.req >> hashes[h].first >> hashes[h].second;

        std::vector<BraneAssetID> assetsWithDiff;
        for(auto& h : hashes)
        {
            if(!h.first.domain.empty())
                continue;
            auto info = _db.getAssetInfo(h.first.uuid);
            if(info.hash != h.second)
                assetsWithDiff.push_back(std::move(h.first));
        }

        rc.res << assetsWithDiff;
    });

    _nm.addRequestListener("updateAsset", [this](RequestCTX& rc) {
        auto ctx = getContext(rc.sender);
        if(!validatePermissions(ctx, {"edit assets"}))
        {
            rc.code = net::ResponseCode::denied;
            return;
        }
        Asset* asset = Asset::deserializeUnknown(rc.req);

        auto castId = asset->id.as<BraneAssetID>();
        if(!castId)
        {
            rc.code = net::ResponseCode::invalidRequest;
            rc.res << "Can only";
        }
        auto id = castId.value();


        auto assetInfo = _db.getAssetInfo(id->uuid);

        auto path = assetPath(*id);
        bool assetExists = true;
        if(assetInfo.hash.empty())
        {
            assetExists = false;
        }

        FileManager::writeAsset(asset, path);
        assetInfo.id = id->uuid;
        assetInfo.name = asset->name;
        assetInfo.type = asset->type;
        assetInfo.hash = FileManager::checksum(path);

        if(assetExists)
            _db.updateAssetInfo(assetInfo);
        else
            _db.insertAssetInfo(assetInfo);

        rc.res << asset->id;
    });

    /** User management **/
    _nm.addRequestListener("searchUsers", [this](auto& rc) {
        auto ctx = getContext(rc.sender);
        if(!validatePermissions(ctx, {"view users"}))
        {
            rc.code = net::ResponseCode::denied;
            return;
        }
        std::string filter;
        rc.req >> filter;
        auto users = _db.searchUsers(0, 0, filter);
        rc.res << static_cast<uint32_t>(users.size());
        for(auto& user : users)
            rc.res << user.id << user.username;
    });

    _nm.addRequestListener("adminChangePassword", [this](auto& rc) {
        auto ctx = getContext(rc.sender);
        if(!validatePermissions(ctx, {"manage users"}))
        {
            rc.code = net::ResponseCode::denied;
            return;
        }
        uint32_t userID;
        std::string newPassword;
        rc.req >> userID >> newPassword;
        _db.setPassword(userID, newPassword);
    });

    _nm.addRequestListener("adminDeleteUser", [this](auto& rc) {
        auto ctx = getContext(rc.sender);
        if(!validatePermissions(ctx, {"manage users"}))
        {
            rc.code = net::ResponseCode::denied;
            return;
        }
        uint32_t userID;
        rc.req >> userID;
        _db.deleteUser(userID);
        Runtime::log("Deleted user " + std::to_string(userID));
    });

    _nm.addRequestListener("getServerSettings", [this](auto& rc) {
        auto ctx = getContext(rc.sender);
        if(!validatePermissions(ctx, {"manage server"}))
        {
            rc.code = net::ResponseCode::denied;
            return;
        }

        rc.res << Config::json();
    });

    _nm.addRequestListener("setServerSettings", [this](auto& rc) {
        auto ctx = getContext(rc.sender);
        if(!validatePermissions(ctx, {"manage server"}))
        {
            rc.code = net::ResponseCode::denied;
            return;
        }
        rc.req >> Config::json();
        Config::save();
    });
}

void AssetServer::processMessages()
{
    // Send one increment from every incremental asset that we are sending, to create the illusion of them loading in
    // parallel
    _sendersLock.lock();
    _senders.remove_if([&](IncrementalAssetSender& sender) {
        try
        {
            SerializedData data;
            OutputSerializer s(data);
            bool moreData = sender.asset->serializeIncrement(s, sender.iteratorData.get());
            sender.connection->sendStreamData(sender.streamID, std::move(data));
            if(!moreData)
                sender.connection->endStream(sender.streamID);
            return !moreData;
        }
        catch(const std::exception& e)
        {
            std::cerr << "Asset sender error: " << e.what() << std::endl;
            return true;
        }
    });
    _sendersLock.unlock();
}

const char* AssetServer::name()
{
    return "assetServer";
}

AsyncData<Asset*> AssetServer::fetchAssetCallback(const BraneAssetID& id, bool incremental)
{
    AsyncData<Asset*> asset;

    auto info = _db.getAssetInfo(id.uuid);
    std::filesystem::path path = assetPath(id);
    if(!std::filesystem::exists(path))
        asset.setError("Asset not found");
    else
    {
        _fm.async_readUnknownAsset(path).then([this, asset](Asset* data) {
            if(data)
                asset.setData(data);
            else
                asset.setError("Could not read requested asset");
        });
    }

    return asset;
}

AssetServer::ConnectionContext& AssetServer::getContext(net::Connection* connection)
{
    if(!_connectionCtx.count(connection))
    {
        _connectionCtx.insert({connection, ConnectionContext{}});
        connection->onDisconnect([this, connection] {
            _connectionCtx.erase(connection);
            _sendersLock.lock();
            _senders.remove_if([connection](auto& sender) { return sender.connection == connection; });
            _sendersLock.unlock();
        });
    }

    return _connectionCtx[connection];
}

bool AssetServer::validatePermissions(AssetServer::ConnectionContext& ctx, const std::vector<std::string>& permissions)
{
    if(ctx.userID == 1)
        return true; // Admin has all permissions;
    for(auto& p : permissions)
    {
        if(!ctx.permissions.count(p))
            return false;
    }
    return true;
}

std::filesystem::path AssetServer::assetPath(const BraneAssetID& id)
{
    return std::filesystem::path{Config::json()["data"]["asset_path"].asString()} / (id.uuid.toString() + ".bin");
}
