#include "notification_server.h"
#include "threading/thread_call.h"
#include "ui/mainwindow.h"

#include <algorithm>
#include <boost/asio/buffer.hpp>
#include <boost/asio/write.hpp>
#include <future>
#include <sstream>

Notification_server::Notification_server(const std::vector<boost::asio::ip::tcp::endpoint> &addresses)
	: notifier(addresses) {}

template <class... T>
void ignore_exception(T &&... t) {
	try {
		std::invoke(t...);
	} catch (...) {
	}
}

void Notification_server::send_notification(std::string data) {
	notifier.run_async([data = std::make_shared<std::string>(std::move(data))](Notifier::Sockets_t &sockets) {
		for (auto &socket : sockets) {
			boost::asio::async_write(*socket, boost::asio::buffer(data->c_str(), data->size()),
									 [data, &socket = socket, &sockets = sockets](const boost::system::error_code &error, std::size_t) {
										 if (error) {
											 //We ignore exceptions here because not logging the error or properly shutting down the connection is fine and we
											 //want to try all the things before exiting without leaking an exception
											 ignore_exception([&socket, &error] {
												 std::stringstream message;
												 message << "Failed sending notification to " << socket->remote_endpoint().address() << ':'
														 << socket->local_endpoint().port();
												 MainWindow::report_error(message.str(), error.message());
											 });
											 ignore_exception([&socket] { socket->shutdown(socket->shutdown_both); });
											 ignore_exception([&socket] { socket->close(); });
											 sockets.erase(std::find(std::begin(sockets), std::end(sockets), socket));
										 }
									 });
		}
	});
}

std::size_t Notification_server::get_number_of_established_connections() {
	return notifier.run_sync([](Notifier::Sockets_t &sockets) { return sockets.size(); });
}

void Notification_server::wait_for_connections(const std::size_t number_of_connections, std::chrono::milliseconds timeout) {
	const auto start_time = std::chrono::high_resolution_clock::now();
	for (auto connections = get_number_of_established_connections(); connections < number_of_connections;
		 connections = get_number_of_established_connections()) {
		if (std::chrono::high_resolution_clock::now() - start_time > timeout) {
			throw std::runtime_error{"Timeout waiting for connections"};
		}
		std::this_thread::yield();
	}
}

std::vector<boost::asio::ip::tcp::endpoint> Notification_server::get_listening_endpoints() {
	return notifier.run_sync([](Notifier::Listeners_t &listeners) {
		std::vector<boost::asio::ip::tcp::endpoint> endpoints;
		endpoints.reserve(listeners.size());
		std::transform(std::begin(listeners), std::end(listeners), std::back_inserter(endpoints),
					   [](const std::unique_ptr<Notifier::Listener> &p) { return p->endpoint; });
		return endpoints;
	});
}

void Notification_server::clear_listening_endpoints() {
	notifier.run_sync([](Notifier::Listeners_t &listeners) {
		for (auto &listener : listeners) {
			listener->acceptor.cancel();
		}
		listeners.clear();
	});
}

Notification_server::Notifier::Listener::Listener(boost::asio::io_service &p_io_service, boost::asio::ip::tcp::endpoint p_endpoint,
												  std::vector<std::unique_ptr<boost::asio::ip::tcp::socket>> &p_sockets)
	: endpoint{std::move(p_endpoint)}
	, io_service{p_io_service}
	, acceptor{io_service, endpoint}
	, socket{io_service} {
	struct Acceptor {
		void operator()(const boost::system::error_code &error) {
			if (error) {
				//`this` may already be deleted, so we can't do listener.socket.close()
				MainWindow::report_error("Failed accepting connection", error.message());
				return;
			}
			listener.socket.shutdown(boost::asio::ip::tcp::socket::shutdown_receive);
			listener.socket.set_option(boost::asio::socket_base::keep_alive{true});
			sockets.push_back(std::make_unique<boost::asio::ip::tcp::socket>(std::move(listener.socket)));
			listener.acceptor.async_accept(listener.socket, listener.endpoint, Acceptor{sockets, listener});
		}
		std::vector<std::unique_ptr<boost::asio::ip::tcp::socket>> &sockets;
		Listener &listener;
	};
	acceptor.async_accept(socket, this->endpoint, Acceptor{p_sockets, *this});
}

Notification_server::Notifier::Notifier(const std::vector<boost::asio::ip::tcp::endpoint> &addresses)
	: work{io_service} {
	listeners.reserve(addresses.size());
	for (const auto &address : addresses) {
		try {
			listeners.push_back(std::make_unique<Notifier::Listener>(io_service, address, sockets));
		} catch (const boost::system::system_error &e) {
			std::stringstream message;
			message << "Failed binding to " << address.address() << ':' << address.port();
			MainWindow::report_error(message.str(), e.what());
		}
	}
	server = std::async(std::launch::async, [this] {
		try {
			io_service.run();
		} catch (const boost::system::system_error &e) {
			MainWindow::report_error("Failed running notification server", e.what());
		}
	});
}

Notification_server::Notifier::~Notifier() {
	run_sync([](Notifier::Sockets_t &lsockets) {
		for (auto &socket : lsockets) {
			socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
			socket->close();
		}
		lsockets.clear();
	});
	io_service.stop();
	Utility::get_future_value(std::move(server));
}
