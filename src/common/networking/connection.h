#pragma once
#include <asio/asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <asio/ssl.hpp>

#include <cstdint>
#include <memory>
#include "networkError.h"
#include "config/config.h"
#include "networkQueue.h"
#include "message.h"

namespace net
{

	enum class ConnectionType
	{
		reliable = 0,
		secure = 1,
		fast = 2
	};

	typedef uint32_t ConnectionID;

	class Connection
	{
	public:
		enum class Owner
		{ 
			client,
			server
		};

		struct OwnedIMessage
		{
			std::shared_ptr<Connection> owner;
			IMessage message;
		};

		struct OwnedOMessage
		{
			std::shared_ptr<Connection> owner;
			OMessage message;
		};

	protected:
		std::shared_ptr<Connection> sharedThis;
		asio::io_context& _ctx;
		NetQueue<OwnedIMessage>& _ibuffer;
		NetQueue<OMessage> _obuffer;
		ConnectionID _id;
		Owner _owner;

		IMessage _tempIn;

		virtual void async_readHeader() = 0;
		virtual void async_readBody() = 0;
		virtual void async_writeHeader() = 0;
		virtual void async_writeBody() = 0;

		void addToIMessageQueue();

	public:
		Connection(asio::io_context& ctx, NetQueue<OwnedIMessage>& ibuffer);

		//Each Connection derived class has it's own unique connect call due to the need to pass a socket
		virtual bool connectToServer(const asio::ip::tcp::resolver::results_type& endpoints) = 0;
		virtual void connectToClient(ConnectionID id) = 0;
		virtual void dissconnect() = 0;
		virtual bool isConnected() = 0;
		ConnectionID id();

		virtual void send(const OMessage& msg) = 0;
		virtual ConnectionType type() = 0;

	};

	typedef asio::ip::tcp::socket tcp_socket;
	class ReliableConnection : public Connection
	{
		tcp_socket _socket;

	protected:
		void async_readHeader() override;
		void async_readBody() override;
		void async_writeHeader() override;
		void async_writeBody() override;

	public:
		ReliableConnection(Owner owner, asio::io_context& ctx, tcp_socket& socket, NetQueue<OwnedIMessage>& ibuffer);
		bool connectToServer(const asio::ip::tcp::resolver::results_type& endpoints) override;
		void connectToClient(ConnectionID id) override;
		void dissconnect() override;
		bool isConnected() override;

		virtual void send(const OMessage& msg);
		ConnectionType type() override;

		
	};

	typedef asio::ssl::stream<tcp_socket> ssl_socket;
	class SecureConnection : public Connection
	{
		
		ssl_socket _socket;
	public:
		SecureConnection(ssl_socket socket);
		void dissconnect();
		bool isConnected();

		void send(const OMessage& msg);
		ConnectionType type() override;

	};
	
	typedef asio::ip::udp::socket udp_socket;
	class FastConnection : public Connection
	{
		udp_socket _socket;
	public:
		void dissconnect();
		bool isConnected();

		void send(const OMessage& msg);
		ConnectionType type() override;
	};
	
}