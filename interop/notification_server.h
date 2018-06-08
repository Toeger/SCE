#ifndef NOTIFICATION_SERVER_H
#define NOTIFICATION_SERVER_H

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

struct Notification_server {
	//create notification server and run it in a thread
	Notification_server(const std::vector<boost::asio::ip::tcp::endpoint> &addresses = {{boost::asio::ip::address_v4::loopback(), 53677},
																						{boost::asio::ip::address_v6::loopback(), 53677}});
	~Notification_server();
	Notification_server(const Notification_server &) = delete;
	void send_notification(std::string data);
	//NOTE: Be careful when connecting and then asking for the number of connections.
	//The server may answer before accepting the connection and return an unexpected result.
	std::size_t get_number_of_established_connections();
	void wait_for_connections(const std::size_t number_of_connections, std::chrono::milliseconds timeout = std::chrono::milliseconds{3000});
	std::vector<boost::asio::ip::tcp::endpoint> get_listening_endpoints();
	void clear_listening_endpoints();

	private:
	struct {
		boost::asio::io_service io_service;
	} shared;
	struct {
		boost::asio::io_service::work work;
		std::thread server;
	} gui_thread_private;
	struct Notification_thread_private {
		struct Listener {
			boost::asio::ip::tcp::endpoint endpoint;
			boost::asio::io_service &io_service;
			boost::asio::ip::tcp::acceptor acceptor;
			boost::asio::ip::tcp::socket socket;
			Listener(boost::asio::io_service &io_service, boost::asio::ip::tcp::endpoint endpoint,
					 std::vector<std::unique_ptr<boost::asio::ip::tcp::socket>> &p_sockets);
			Listener(const Listener &) = delete;
		};
		std::vector<std::unique_ptr<Listener>> listeners;
		std::vector<std::unique_ptr<boost::asio::ip::tcp::socket>> sockets;
	} notification_thread_private;
};

#endif //NOTIFICATION_SERVER_H