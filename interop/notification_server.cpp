#include "notification_server.h"
#include "ui/mainwindow.h"

#include <algorithm>
#include <boost/asio/buffer.hpp>
#include <boost/asio/write.hpp>
#include <future>
#include <iostream>
#include <sstream>

Notification_server::Notification_server(const std::vector<boost::asio::ip::tcp::endpoint> &addresses)
	: gui_thread_private{.work = static_cast<decltype(gui_thread_private.work)>(shared.io_service), .server = {}} {
	notification_thread_private.listeners.reserve(addresses.size());
	for (const auto &address : addresses) {
		try {
			notification_thread_private.listeners.push_back(
				std::make_unique<Notification_thread_private::Listener>(shared.io_service, address, notification_thread_private.sockets));
		} catch (const boost::system::system_error &e) {
			std::stringstream message;
			message << "Failed binding to " << address.address() << ':' << address.port();
			MainWindow::report_error(message.str(), e.what());
		}
	}
	gui_thread_private.server = std::thread{[&shared = shared] {
		try {
			shared.io_service.run();
		} catch (const boost::system::system_error &e) {
			MainWindow::report_error("Failed running notification server", e.what());
		}
	}};
}

Notification_server::~Notification_server() {
	shared.io_service.stop();
	gui_thread_private.server.join();
}

void Notification_server::send_notification(std::string data) {
	shared.io_service.dispatch([data = std::make_shared<std::string>(std::move(data)), &sockets = notification_thread_private.sockets] {
		for (auto &socket : sockets) {
			boost::asio::async_write(*socket, boost::asio::buffer(data->c_str(), data->size()),
									 [data, &socket = socket, &sockets = sockets](const boost::system::error_code &error, std::size_t) {
										 if (error) {
											 std::stringstream message;
											 message << "Failed sending notification to " << socket->local_endpoint().address() << ':'
													 << socket->local_endpoint().port();
											 MainWindow::report_error(message.str(), error.message());
											 socket->shutdown(boost::asio::ip::tcp::socket::shutdown_send);
											 socket->close();
											 sockets.erase(std::find(std::begin(sockets), std::end(sockets), socket));
										 }
									 });
		}
	});
}

std::size_t Notification_server::get_number_of_established_connections() {
	std::promise<std::size_t> sockets_size_promise;
	auto sockets_size_future = sockets_size_promise.get_future();
	shared.io_service.dispatch([&sockets = notification_thread_private.sockets, &sockets_size_promise] { sockets_size_promise.set_value(sockets.size()); });
	return sockets_size_future.get();
}

Notification_server::Notification_thread_private::Listener::Listener(boost::asio::io_service &io_service, boost::asio::ip::tcp::endpoint endpoint,
																	 std::vector<std::unique_ptr<boost::asio::ip::tcp::socket>> &sockets)
	: endpoint{endpoint}
	, io_service{io_service}
	, acceptor{io_service, endpoint}
	, socket{io_service} {
	struct Writer {};
	struct Acceptor {
		void operator()(const boost::system::error_code &error) {
			if (error) {
				MainWindow::report_error("Failed accepting connection", error.message());
			} else {
				listener->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_receive);
				sockets->push_back(std::make_unique<boost::asio::ip::tcp::socket>(std::move(listener->socket)));
			}
			listener->acceptor.async_accept(listener->socket, listener->endpoint, Acceptor{sockets, listener});
		}
		std::vector<std::unique_ptr<boost::asio::ip::tcp::socket>> *sockets;
		Listener *listener;
	};
	acceptor.async_accept(socket, endpoint, Acceptor{&sockets, this});
}
