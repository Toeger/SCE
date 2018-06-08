#include "notification_server.h"
#include "ui/mainwindow.h"

#include <algorithm>
#include <boost/asio/buffer.hpp>
#include <boost/asio/write.hpp>
#include <future>
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
	gui_thread_private.server = std::thread{[&io_service = shared.io_service] {
		try {
			io_service.run();
		} catch (const boost::system::system_error &e) {
			MainWindow::report_error("Failed running notification server", e.what());
		}
	}};
}

Notification_server::~Notification_server() {
	shared.io_service.dispatch([&sockets = notification_thread_private.sockets] {
		for (auto &socket : sockets) {
			socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
			socket->close();
		}
		sockets.clear();
	});
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
	std::promise<std::vector<boost::asio::ip::tcp::endpoint>> endpoints_promise;
	auto endpoints_future = endpoints_promise.get_future();
	shared.io_service.dispatch([&listeners = notification_thread_private.listeners, &endpoints_promise] {
		std::vector<boost::asio::ip::tcp::endpoint> endpoints;
		endpoints.reserve(listeners.size());
		std::transform(std::begin(listeners), std::end(listeners), std::back_inserter(endpoints),
					   [](const std::unique_ptr<Notification_thread_private::Listener> &p) { return p->endpoint; });
		endpoints_promise.set_value(std::move(endpoints));
	});
	return endpoints_future.get();
}

void Notification_server::clear_listening_endpoints() {
	std::promise<void> promise;
	auto future = promise.get_future();
	shared.io_service.dispatch([&listeners = notification_thread_private.listeners, &promise] {
		for (auto &listener : listeners) {
			listener->acceptor.cancel();
		}
		listeners.clear();
		promise.set_value();
	});
	future.wait();
}

Notification_server::Notification_thread_private::Listener::Listener(boost::asio::io_service &p_io_service, boost::asio::ip::tcp::endpoint p_endpoint,
																	 std::vector<std::unique_ptr<boost::asio::ip::tcp::socket>> &p_sockets)
	: endpoint{std::move(p_endpoint)}
	, io_service{p_io_service}
	, acceptor{io_service, endpoint}
	, socket{io_service} {
	struct Acceptor {
		void operator()(const boost::system::error_code &error) {
			if (error) {
				//`this` may already be deleted, so we can't do listener->socket.close()
				MainWindow::report_error("Failed accepting connection", error.message());
				return;
			}
			listener->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_receive);
			sockets->push_back(std::make_unique<boost::asio::ip::tcp::socket>(std::move(listener->socket)));
			listener->acceptor.async_accept(listener->socket, listener->endpoint, Acceptor{sockets, listener});
		}
		std::vector<std::unique_ptr<boost::asio::ip::tcp::socket>> *sockets;
		Listener *listener;
	};
	acceptor.async_accept(socket, this->endpoint, Acceptor{&p_sockets, this});
}