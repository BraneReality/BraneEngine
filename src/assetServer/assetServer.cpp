//
// Created by eli on 3/3/2022.
//

#include "assetServer.h"
#include "assets/assetManager.h"
#include "fileManager/fileManager.h"
#include "networking/networking.h"
#include "utility/hex.h"

AssetServer::AssetServer()
    : _nm(*Runtime::getModule<NetworkManager>()), _am(*Runtime::getModule<AssetManager>()),
      _fm(*Runtime::getModule<FileManager>()), _db(*Runtime::getModule<Database>())
{
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

    Runtime::timeline().addTask(
        "send asset data", [this] { processMessages(); }, "networking");
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
        std::cout << "request for: " << id.string() << std::endl;
        _fm.readFile(assetPath(id), rc.responseData.vector());
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
        std::cout << "request for: " << id.string() << std::endl;

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
        AssetID defaultChunk(Config::json()["default_assets"]["chunk"].asString());
        if(defaultChunk.address().empty())
            defaultChunk.setAddress(Config::json()["network"]["domain"].asString());
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
        std::vector<std::pair<AssetID, std::string>> hashes(hashCount);
        for(uint32_t h = 0; h < hashCount; ++h)
            rc.req >> hashes[h].first >> hashes[h].second;

        std::vector<AssetID> assetsWithDiff;
        for(auto& h : hashes)
        {
            if(!h.first.address().empty())
                continue;
            auto info = _db.getAssetInfo(h.first.id());
            if(info.hash != h.second)
                assetsWithDiff.push_back(std::move(h.first));
        }

        rc.res << assetsWithDiff;
    });

    _nm.addRequestListener("updateAsset", [this](auto& rc) {
        auto ctx = getContext(rc.sender);
        if(!validatePermissions(ctx, {"edit assets"}))
        {
            rc.code = net::ResponseCode::denied;
            return;
        }
        Asset* asset = Asset::deserializeUnknown(rc.req);

        auto assetInfo = _db.getAssetInfo(asset->id.id());

        auto path = assetPath(asset->id);
        bool assetExists = true;
        if(assetInfo.hash.empty())
        {
            assetExists = false;
        }

        FileManager::writeAsset(asset, path);
        assetInfo.id = asset->id.id();
        assetInfo.name = asset->name;
        assetInfo.type = asset->type;
        assetInfo.hash = FileManager::fileHash(path);

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

const char* AssetServer::name() { return "assetServer"; }

AsyncData<Asset*> AssetServer::fetchAssetCallback(const AssetID& id, bool incremental)
{
    AsyncData<Asset*> asset;
    auto info = _db.getAssetInfo(id.id());
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

std::filesystem::path AssetServer::assetPath(const AssetID& id)
{
    return std::filesystem::path{Config::json()["data"]["asset_path"].asString()} / (std::string(id.idStr()) + ".bin");
}

// The asset server specific fetch asset function
AsyncData<Asset*> AssetManager::fetchAssetInternal(const AssetID& id, bool incremental)
{
    AsyncData<Asset*> asset;

    auto* nm = Runtime::getModule<NetworkManager>();
    auto* fm = Runtime::getModule<FileManager>();

    auto path = std::filesystem::current_path() / Config::json()["data"]["asset_path"].asString() /
                ((std::string)id.idStr() + ".bin");
    if(std::filesystem::exists(path))
    {
        fm->async_readUnknownAsset(path)
            .then([this, asset](Asset* ptr) {
                ptr->id.setAddress(Config::json()["network"]["domain"].asString());
                _assetLock.lock();
                auto& d = _assets.at(ptr->id);
                d->loadState = LoadState::awaitingDependencies;
                _assetLock.unlock();
                if(dependenciesLoaded(ptr))
                {
                    asset.setData(ptr);
                    return;
                }

                d->loadState = LoadState::awaitingDependencies;
                fetchDependencies(ptr, [ptr, asset](bool success) mutable {
                    if(success)
                        asset.setData(ptr);
                    else
                        asset.setError("Failed to load dependency for: " + ptr->name);
                });
            })
            .onError([asset](auto& err) { asset.setError(err); });
        return asset;
    }
    else
    {
        asset.setError("Asset not found");
    }
    return asset;
}
