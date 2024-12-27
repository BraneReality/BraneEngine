#include "connection.h"

#include <utility>

namespace net
{
    Connection::Connection() {}

    Connection::~Connection() {}

    bool Connection::messageAvailable() { return !_ibuffer.empty(); }

    IMessage Connection::popMessage() { return std::move(_ibuffer.pop_front()); }

    void Connection::sendStreamData(uint32_t id, SerializedData&& data)
    {
        OMessage message;
        message.header.type = MessageType::streamData;
        SerializedData headerData;
        OutputSerializer s(headerData);
        s << id;
        message.chunks.emplace_back(std::move(headerData));
        message.chunks.emplace_back(std::move(data));
        message.header.type = net::MessageType::streamData;
        send(std::move(message));
    }

    void Connection::endStream(uint32_t id)
    {
        OMessage message;
        message.header.type = MessageType::endStream;
        SerializedData headerData;
        OutputSerializer s(headerData);
        s << id;
        message.chunks.emplace_back(std::move(headerData));
        send(std::move(message));
    }

    void Connection::addStreamListener(uint32_t id,
                                       std::function<void(InputSerializer s)> callback,
                                       std::function<void()> onEnd)
    {
        _streamLock.lock();
        assert(!_streamListeners.count(id));
        _streamListeners.insert({id, std::make_pair(std::move(callback), std::move(onEnd))});
        _streamLock.unlock();
    }

    void Connection::eraseStreamListener(uint32_t id)
    {
        _streamLock.lock();
        _streamListeners.erase(id);
        _streamLock.unlock();
    }

    void Connection::sendRequest(const std::string& name,
                                 SerializedData&& data,
                                 const std::function<void(ResponseCode code, InputSerializer s)>& callback)
    {
        uint32_t id = _reqIDCounter++;
        _responseLock.lock();
        _responseListeners.insert({id, callback});
        _responseLock.unlock();

        OMessage request;
        request.header.type = MessageType::request;
        SerializedData headerData;
        OutputSerializer s(headerData);
        s << name << id;

        request.chunks.emplace_back(std::move(headerData));
        request.chunks.emplace_back(std::move(data));
        send(std::move(request));
    }

    void Connection::onDisconnect(std::function<void()> f) { _onDisconnect.push_back(std::move(f)); }

    void Connection::onRequest(std::function<void(Connection*, IMessage&&)> request)
    {
        _requestHandler = std::move(request);
    }

    const std::string& Connection::address() const { return _address; }

    template<>
    void ServerConnection<tcp_socket>::connectToClient()
    {
        _address = _socket.remote_endpoint().address().to_string();
        async_readHeader();
    }

    template<>
    void ServerConnection<ssl_socket>::connectToClient()
    {
        _socket.async_handshake(asio::ssl::stream_base::server, [this](std::error_code ec) {
            if(!ec)
            {
                _address = _socket.lowest_layer().remote_endpoint().address().to_string();
                async_readHeader();
            }
            else
            {
                std::cout << "SSL handshake failed: " << ec.message() << std::endl;
            }
        });
    }

    template<>
    void ClientConnection<tcp_socket>::connectToServer(const asio::ip::tcp::resolver::results_type& endpoints,
                                                       std::function<void()> onConnect,
                                                       std::function<void()> onFail)
    {

        asio::async_connect(_socket.lowest_layer(),
                            endpoints,
                            [this, onConnect, onFail](std::error_code ec, asio::ip::tcp::endpoint endpoint) {
            if(ec)
            {
                std::cerr << "Failed to connect to server." << std::endl;
                onFail();
                return;
            }
            _address = _socket.remote_endpoint().address().to_string();
            async_readHeader();
            onConnect();
        });
    }

    template<>
    void ClientConnection<ssl_socket>::connectToServer(const asio::ip::tcp::resolver::results_type& endpoints,
                                                       std::function<void()> onConnect,
                                                       std::function<void()> onFail)
    {

        asio::async_connect(_socket.lowest_layer(),
                            endpoints,
                            [this, onConnect, onFail](std::error_code ec, asio::ip::tcp::endpoint endpoint) {
            if(ec)
            {
                std::cerr << "Failed to connect to server." << std::endl;
                onFail();
                return;
            }
            _address = _socket.lowest_layer().remote_endpoint().address().to_string();
            _socket.async_handshake(asio::ssl::stream_base::client, [this, onConnect, onFail](std::error_code ec) {
                if(ec)
                {
                    std::cout << "SSL handshake failed: " << ec.message() << std::endl;
                    onFail();
                    return;
                }
                async_readHeader();
                onConnect();
            });
        });
    }
} // namespace net