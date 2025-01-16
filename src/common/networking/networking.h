#pragma once

#include <asio.hpp>
#include <asio/ssl.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

#include <iostream>
#include <memory>
#include <thread>
#include <utility>
#include <vector>
#include <unordered_map>

#include "assets/assetID.h"
#include "config/config.h"
#include "connection.h"
#include <shared_mutex>
#include <utility/asyncData.h>
#include <utility/serializedData.h>

class JobHandle;

class Asset;

class IncrementalAsset;

struct RequestCTX
{
    uint32_t id;
    std::string name;
    net::Connection* sender;
    net::ResponseCode code = net::ResponseCode::success;
    SerializedData requestData;
    SerializedData responseData;
    InputSerializer req;
    OutputSerializer res;

    RequestCTX(net::Connection* sender, SerializedData&& requestData);

    RequestCTX(const RequestCTX&) = delete;

    RequestCTX(RequestCTX&&);

    ~RequestCTX();
};

class NetworkManager : public Module
{
    asio::io_context _context;

    std::shared_ptr<JobHandle> _threadHandle;
    asio::ssl::context _ssl_context;
    asio::ip::tcp::resolver _tcpResolver;

    std::shared_mutex _serverLock;
    std::unordered_map<std::string, std::unique_ptr<net::Connection>> _servers;

    std::shared_mutex _clientLock;
    std::vector<std::unique_ptr<net::Connection>> _clients;
    std::vector<std::function<void()>> _onClientDisconnect;

    std::vector<asio::ip::tcp::acceptor> _acceptors;

    std::atomic_bool _running;

    std::mutex _requestLock;
    std::unordered_map<std::string, std::function<void(RequestCTX& ctx)>> _requestListeners;

    uint32_t _streamIDCounter = 1000;

    void connectToAssetServer(std::string ip, uint16_t port);

    template<typename socket_t>
    void async_acceptConnections(size_t acceptor,
                                 std::function<void(std::unique_ptr<net::Connection>& connection)> callback)
    {
        socket_t* socket;
        if constexpr(std::is_same<socket_t, net::tcp_socket>().value)
            socket = new net::tcp_socket(_context);
        if constexpr(std::is_same<socket_t, net::ssl_socket>().value)
            socket = new net::ssl_socket(_context, _ssl_context);
        _acceptors[acceptor].async_accept(socket->lowest_layer(),
                                          [this, acceptor, socket, callback](std::error_code ec) {
            if(!ec)
            {
                std::unique_ptr<net::Connection> connection =
                    std::make_unique<net::ServerConnection<socket_t>>(std::move(*socket));
                connection->onRequest([this](auto c, auto m) { handleResponse(c, std::move(m)); });
                callback(connection);
                _clientLock.lock();
                _clients.push_back(std::move(connection));
                _clientLock.unlock();
            }
            else
            {
                Runtime::error("Connection Error: " + std::string(ec.message()));
            }
            delete socket;
            async_acceptConnections<socket_t>(acceptor, callback);
        });
    }

    void startSystems();

    void ingestData(net::Connection* connection);

    void handleResponse(net::Connection* connection, net::IMessage&& message);

  public:
    NetworkManager();

    ~NetworkManager();

    void start() override;

    void stop() override;

    void configureServer();

    net::Connection* getServer(const std::string& address) const;

    template<typename socket_t>
    void openClientAcceptor(const uint32_t port,
                            std::function<void(const std::unique_ptr<net::Connection>& connection)> callback)
    {
        _acceptors.emplace_back(_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port));
        auto& acceptor = *(_acceptors.end() - 1);
        async_acceptConnections<socket_t>(_acceptors.size() - 1, callback);
    }

    void
    async_connectToAssetServer(const std::string& address, uint16_t port, const std::function<void(bool)>& callback);

    AsyncData<Asset*> async_requestAsset(const AssetID& id);
    AsyncData<Asset*> async_requestAsset(const BraneAssetID& id);

    AsyncData<IncrementalAsset*> async_requestAssetIncremental(const AssetID& id);
    AsyncData<IncrementalAsset*> async_requestAssetIncremental(const BraneAssetID& id);

    void addRequestListener(const std::string& name, std::function<void(RequestCTX& ctx)> callback);

    static const char* name();
};
