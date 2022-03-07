#pragma once
#include <asio/asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <asio/ssl.hpp>


#include "message.h"
#include <cstdint>
#include <memory>
#include "networkError.h"
#include "config/config.h"
#include "networkQueue.h"
#include "serializedData.h"
#include <ecs/ecs.h>

namespace net
{
	typedef uint32_t ConnectionID;

	typedef asio::ip::tcp::socket tcp_socket;
	typedef asio::ssl::stream<tcp_socket> ssl_socket;


	const std::string_view messageEnd = "~~EndMessage~~";
	class Connection
	{
	public:

	protected:
		EntityManager* _em;
		NetQueue<std::shared_ptr<OMessage>> _obuffer;
		NetQueue<std::shared_ptr<IMessage>> _ibuffer;
		std::shared_ptr<IMessage> _tempIn;

	public:
		virtual void disconnect() = 0;
		virtual bool connected() = 0;

		virtual void send(std::shared_ptr<OMessage> msg) = 0;
		bool popIMessage(std::shared_ptr<IMessage>& iMessage);

	};

	template<class socket_t>
	class ConnectionBase : public Connection
	{

	protected:
		bool _connected;
		socket_t _socket;
		asio::streambuf buffer;
		void async_readHeader()
		{
			_tempIn = std::make_shared<IMessage>();
			asio::async_read(_socket, asio::buffer(&_tempIn->header, sizeof(MessageHeader)),[this](std::error_code ec, std::size_t length) {
				if (!ec)
				{

					async_readBody();

				}
				else
				{
					std::cout << "[" << _socket.lowest_layer().remote_endpoint().address().to_string() << "] Header Parse Fail: " << ec.message() << std::endl;
					disconnect();
				}

			});
		}
		void async_readBody()
		{
			asio::async_read_until(_socket, buffer, messageEnd,  [this](std::error_code ec, std::size_t length) {
				if (!ec)
				{
					_tempIn->body.data.resize(length);
					std::memcpy(_tempIn->body.data.data(),buffer.data().data(), length);
					buffer.consume(length);
					_ibuffer.push_back(_tempIn);
					_tempIn = nullptr;
					async_readHeader();
				}
				else
				{
					std::cout << "[" << _socket.lowest_layer().remote_endpoint().address().to_string() << "] Read message body fail: " << ec.message() << std::endl;
					disconnect();
				}
			});
		}
		void async_writeHeader()
		{
			asio::async_write(_socket, asio::buffer(&_obuffer.front()->header, sizeof(MessageHeader)), [this](std::error_code ec, std::size_t length) {
				if (!ec)
				{
					if (_obuffer.front()->body.data.size() > 0)
						async_writeBody();
					else
					{
						_obuffer.pop_front();

						if (!_obuffer.empty())
							async_writeHeader();
					}

				}
				else
				{
					std::cout << "[" << _socket.lowest_layer().remote_endpoint().address().to_string() << "] Write header fail: " << ec.message() << std::endl;
					disconnect();
				}
			});
		}
		void async_writeBody()
		{
			asio::async_write(_socket, asio::buffer(_obuffer.front()->body.data.data(), _obuffer.front()->body.data.size()), [this](std::error_code ec, std::size_t length) {
				if (!ec)
				{
					asio::write(_socket, asio::buffer(messageEnd.data(), messageEnd.size()));
					_obuffer.pop_front();
					if (!_obuffer.empty())
						async_writeHeader();
				}
				else
				{
					std::cout << "[" << _socket.lowest_layer().remote_endpoint().address().to_string() << "] Write body fail: " << ec.message() << std::endl;
					disconnect();
				}
			});
		}
	public:
		ConnectionBase(socket_t&& socket) : _socket(std::move(socket))
		{
			_connected = false;
		}
		void send(std::shared_ptr<OMessage> msg) override
		{
			assert(connected());
			asio::post(_socket.get_executor(), [this, msg]() {
				bool sending = !_obuffer.empty();
				_obuffer.push_back(msg);
				if (!sending)
					async_writeHeader();
			});
		}
		void disconnect() override
		{
			_socket.lowest_layer().close();
		}
		bool connected() override
		{
			return _socket.lowest_layer().is_open() && _connected;
		}

	};

	template<class socket_t>
	class ServerConnection : public ConnectionBase<socket_t>
	{


	public:
		ServerConnection(socket_t&& socket) : ConnectionBase<socket_t>(std::move(socket))
		{
			_connected = true;
			connectToClient();
		}
		void connectToClient();
	};



	template<class socket_t>
	class ClientConnection : public ConnectionBase<socket_t>
	{

	public:
		ClientConnection(socket_t&& socket) : ConnectionBase<socket_t>(std::move(socket))
		{
		}
		void connectToServer(const asio::ip::tcp::resolver::results_type& endpoints);
	};



	struct NewConnectionComponent : public NativeComponent <NewConnectionComponent>
	{
		REGISTER_MEMBERS_0("New Connection");
	};

	struct ConnectionComponent : public NativeComponent<ConnectionComponent>
	{
		REGISTER_MEMBERS_2("Connection", id, connection);
		ConnectionID id;
		std::shared_ptr<Connection> connection;
		
	};
}