#include "networking.h"
#include <atomic>
#include "assets/asset.h"
#include "assets/assetManager.h"
#include "runtime/runtime.h"
#include "utility/threadPool.h"

NetworkManager::NetworkManager() : _tcpResolver(_context), _ssl_context(asio::ssl::context::tls)
{
    _running = false;
    startSystems();
}

NetworkManager::~NetworkManager() {}

void NetworkManager::connectToAssetServer(std::string ip, uint16_t port)
{
    Runtime::log("connecting to asset server: " + ip + ":" + std::to_string(port));
    auto* connection = new net::ClientConnection<net::tcp_socket>(net::tcp_socket(_context));

    auto tcpEndpoints = _tcpResolver.resolve(ip, std::to_string(port));
    connection->connectToServer(
        tcpEndpoints,
        [this, ip, connection]() mutable {
        _serverLock.lock();
        _servers.insert({ip, std::unique_ptr<net::ClientConnection<net::tcp_socket>>(connection)});
        _serverLock.unlock();
        },
        [] {});
}

void NetworkManager::async_connectToAssetServer(const std::string& address,
                                                uint16_t port,
                                                const std::function<void(bool)>& callback)
{
    _serverLock.lock_shared();
    auto server = _servers.find(address);
    if(server != _servers.end())
    {
        if(_servers.at(address))
            callback(true);
        _serverLock.unlock_shared();
        return;
    }
    _serverLock.unlock_shared();

    Runtime::log("connecting to asset server: " + address + ":" + std::to_string(port));
    _tcpResolver.async_resolve(
        asio::ip::tcp::resolver::query(address, std::to_string(port), asio::ip::tcp::resolver::query::canonical_name),
        [this, address, callback](const asio::error_code ec, auto endpoints) {
        if(!ec)
        {
            auto* connection = new net::ClientConnection<net::tcp_socket>(net::tcp_socket(_context));
            connection->onRequest([this](auto c, auto m) { handleResponse(c, std::move(m)); });
            connection->connectToServer(
                endpoints,
                [this, address, callback, connection]() {
                _serverLock.lock();
                _servers.insert({address, std::unique_ptr<net::Connection>(connection)});
                _serverLock.unlock();
                callback(true);
                if(!_running)
                    start();
                },
                [callback] { callback(false); });
        }
        else
            callback(false);
        });
    if(!_running)
        start();
}

void NetworkManager::start()
{
    if(_running)
        return;
    _running = true;
    _threadHandle = ThreadPool::addStaticThread([this]() {
        while(_running)
        {
            _context.run();
            _context.restart();
        }
        Runtime::log("exiting networking thread");
    });
}

void NetworkManager::stop()
{
    Runtime::log("Shutting down networking");
    for(auto& connection : _servers)
        connection.second->disconnect();
    _running = false;
    _context.stop();
    if(_threadHandle)
        _threadHandle->finish();

    _servers.clear();
}

void NetworkManager::configureServer()
{
    if(Config::json()["network"]["use_ssl"].asBool())
    {
        try
        {
            _ssl_context.set_options(asio::ssl::context::default_workarounds | asio::ssl::context::no_sslv2 |
                                     asio::ssl::context::single_dh_use);
            _ssl_context.use_certificate_chain_file(Config::json()["network"]["ssl_cert"].asString());
            _ssl_context.use_private_key_file(Config::json()["network"]["private_key"].asString(),
                                              asio::ssl::context::pem);
            _ssl_context.use_tmp_dh_file(Config::json()["network"]["tmp_dh"].asString());
        }
        catch(const std::exception& e)
        {
            Runtime::error("Couldn't read file: " + std::string(e.what()));
        }
    }
}

AsyncData<Asset*> NetworkManager::async_requestAsset(const AssetID& id)
{
    AsyncData<Asset*> asset;

    _serverLock.lock_shared();
    std::string address(id.address());
    if(!_servers.count(address))
        throw std::runtime_error("No connection with " + address);
    net::Connection* server = _servers[address].get();
    _serverLock.unlock_shared();

    Runtime::log("async requesting: " + id.string());

    SerializedData data;
    OutputSerializer s(data);
    s << id;

    server->sendRequest("asset", std::move(data), [asset, address](net::ResponseCode code, InputSerializer sData) {
        if(code != net::ResponseCode::success)
        {
            Runtime::error("Could not get asset, server responded with code: " + std::to_string((uint8_t)code));
            return;
        }
        auto* a = Asset::deserializeUnknown(sData);
        a->id.setAddress(address);
        asset.setData(a);
    });
    return asset;
}

AsyncData<IncrementalAsset*> NetworkManager::async_requestAssetIncremental(const AssetID& id)
{
    AsyncData<IncrementalAsset*> asset;

    _serverLock.lock_shared();
    std::string address(id.address());
    if(!_servers.count(address))
        throw std::runtime_error("No connection with " + address);
    net::Connection* server = _servers[address].get();
    _serverLock.unlock_shared();

    Runtime::log("async requesting incremental: " + id.string());
    uint32_t streamID = _streamIDCounter++;

    SerializedData data;
    OutputSerializer s(data);
    s << id << streamID;

    // Set up a listener for the asset response
    server->sendRequest(
        "incrementalAsset", std::move(data), [asset, server, streamID](auto code, InputSerializer sData) {
            if(code != net::ResponseCode::success)
            {
                Runtime::error("Could not get incremental asset, server responded with code: " +
                               std::to_string((uint8_t)code));
                return;
            }
            IncrementalAsset* assetPtr = IncrementalAsset::deserializeUnknownHeader(sData);
            server->addStreamListener(
                streamID,
                [assetPtr](InputSerializer sData) { assetPtr->deserializeIncrement(sData); },
                [assetPtr] { assetPtr->onDependenciesLoaded(); });
            asset.setData(assetPtr);
        });
    return asset;
}

void NetworkManager::startSystems()
{
    Runtime::timeline().addTask(
        "ingest data",
        [this] {
        _serverLock.lock_shared();
        for(auto& s : _servers)
        {
            ingestData(s.second.get());
        }
        _serverLock.unlock_shared();
        _clientLock.lock_shared();
        for(auto& s : _clients)
        {
            if(s)
                ingestData(s.get());
        }
        _clientLock.unlock_shared();
        },
        "networking");
}

const char* NetworkManager::name() { return "networkManager"; }

void NetworkManager::addRequestListener(const std::string& name, std::function<void(RequestCTX& ctx)> callback)
{
    std::scoped_lock l(_requestLock);
    assert(!_requestListeners.count(name));
    _requestListeners.insert({name, std::move(callback)});
}

void NetworkManager::ingestData(net::Connection* connection)
{
    while(connection->messageAvailable())
    {
        net::IMessage message = connection->popMessage();
        switch(message.header.type)
        {
            case net::MessageType::request:
                handleResponse(connection, std::move(message));
                break;
            default:
                Runtime::warn("Received message of unknown type: " + std::to_string((int)message.header.type));
                break;
        }
    }
}

net::Connection* NetworkManager::getServer(const std::string& address) const
{
    if(!_servers.count(address))
        return nullptr;
    return _servers.at(address).get();
}

void NetworkManager::handleResponse(net::Connection* connection, net::IMessage&& message)
{
    assert(net::MessageType::request == message.header.type);
    RequestCTX ctx(connection, std::move(message.body));
    try
    {
        {
            std::scoped_lock l(_requestLock);
            auto listener = _requestListeners.find(ctx.name);
            if(listener == _requestListeners.end())
            {
                Runtime::warn("Unknown request received: " + ctx.name);
                ctx.code = net::ResponseCode::invalidRequest;
                return;
            }
            listener->second(ctx);
        }
    }
    catch(std::exception& e)
    {
        Runtime::error("Error with received request: " + std::string(e.what()));
        ctx.code = net::ResponseCode::serverError;
    }
}

RequestCTX::RequestCTX(net::Connection* s, SerializedData&& r)
    : sender(s), requestData(std::move(r)), req(requestData), res(responseData)
{
    req >> name >> id;
}

RequestCTX::RequestCTX(RequestCTX&& o)
    : requestData(std::move(o.requestData)), responseData(std::move(o.responseData)), req(requestData),
      res(responseData)
{
    req.setPos(o.req.getPos());
    sender = o.sender;
    o.sender = nullptr;
    id = o.id;
    name = std::move(o.name);
    code = o.code;
}

RequestCTX::~RequestCTX()
{
    if(!sender)
        return;
    net::OMessage response;
    response.header.type = net::MessageType::response;
    SerializedData resHeader;
    OutputSerializer resHeaderS(resHeader);

    resHeaderS << id << code;
    response.chunks.emplace_back(std::move(resHeader));
    response.chunks.emplace_back(std::move(responseData));
    sender->send(std::move(response));
}
