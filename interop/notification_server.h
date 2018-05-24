#ifndef NOTIFICATION_SERVER_H
#define NOTIFICATION_SERVER_H

#include <atomic>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
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
					 std::vector<std::unique_ptr<boost::asio::ip::tcp::socket>> &sockets);
		};
		std::vector<std::unique_ptr<Listener>> listeners;
		std::vector<std::unique_ptr<boost::asio::ip::tcp::socket>> sockets;
	} notification_thread_private;
};

#endif //NOTIFICATION_SERVER_H