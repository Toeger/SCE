#ifndef NOTIFICATION_SERVER_H
#define NOTIFICATION_SERVER_H

#include "utility/protobuffer_encoder.h"

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <chrono>
#include <future>
#include <memory>
#include <thread>
#include <type_traits>
#include <vector>

struct Notification_server {
	//create notification server and run it in a thread
	Notification_server(const std::vector<boost::asio::ip::tcp::endpoint> &addresses = {{boost::asio::ip::address_v4::loopback(), 53677},
																						{boost::asio::ip::address_v6::loopback(), 53677}});
	~Notification_server();
	Notification_server(const Notification_server &) = delete;
	void send_notification(std::string data);
	template <class Pb_message, class = std::enable_if_t<std::is_base_of_v<google::protobuf::Message, Pb_message>>>
	void send_notification(const Pb_message &pb_message) {
		send_notification(Utility::encode(pb_message));
	}
	//NOTE: Be careful when connecting and then asking for the number of connections.
	//The server may answer before accepting the connection and return an unexpected result.
	std::size_t get_number_of_established_connections();
	void wait_for_connections(const std::size_t number_of_connections, std::chrono::milliseconds timeout = std::chrono::milliseconds{3000});
	std::vector<boost::asio::ip::tcp::endpoint> get_listening_endpoints();
	void clear_listening_endpoints();

	private:
	struct Notifier {
		struct Listener {
			boost::asio::ip::tcp::endpoint endpoint;
			boost::asio::io_service &io_service;
			boost::asio::ip::tcp::acceptor acceptor;
			boost::asio::ip::tcp::socket socket;
			Listener(boost::asio::io_service &io_service, boost::asio::ip::tcp::endpoint endpoint,
					 std::vector<std::unique_ptr<boost::asio::ip::tcp::socket>> &p_sockets);
			Listener(const Listener &) = delete;
		};

		using Sockets_t = std::vector<std::unique_ptr<boost::asio::ip::tcp::socket>>;
		using Listeners_t = std::vector<std::unique_ptr<Listener>>;

		Notifier(const std::vector<boost::asio::ip::tcp::endpoint> &addresses);
		~Notifier();

		template <class F>
		auto run_async(F &&f) -> std::enable_if_t<std::is_invocable_v<F>> {
			io_service.dispatch(std::forward<F>(f));
		}
		template <class F>
		auto run_async(F &&f) -> std::enable_if_t<std::is_invocable_v<F, Sockets_t &>> {
			run_async_helper(std::forward<F>(f), sockets);
		}
		template <class F>
		auto run_async(F &&f) -> std::enable_if_t<std::is_invocable_v<F, Listeners_t &>> {
			run_async_helper(std::forward<F>(f), listeners);
		}
		template <class F>
		auto run_sync(F &&f) -> std::invoke_result_t<F> {
			return run_sync_helper(std::forward<F>(f));
		}
		template <class F>
		auto run_sync(F &&f) -> std::invoke_result_t<F, Sockets_t &> {
			return run_sync_helper(std::forward<F>(f), sockets);
		}
		template <class F>
		auto run_sync(F &&f) -> std::invoke_result_t<F, Listeners_t &> {
			return run_sync_helper(std::forward<F>(f), listeners);
		}

		private:
		template <class F, class... Args, class Ret_t = std::invoke_result_t<F, Args...>>
		auto run_async_helper(F &&f, Args &&... args) {
			run_async([func = std::forward<F>(f), arguments = std::forward_as_tuple(args...)]() mutable {
				std::apply(std::forward<decltype(func)>(func), std::move(arguments));
			});
		}

		template <class F, class... Args, class Ret_t = std::invoke_result_t<F, Args...>>
		auto run_sync_helper(F &&f, Args &&... args) {
			std::promise<Ret_t> promise;
			auto future = promise.get_future();
			run_async([func = std::forward<F>(f), &promise, arguments = std::forward_as_tuple(args...)]() mutable {
				try {
					if constexpr (std::is_same_v<Ret_t, void>) {
						std::apply(std::forward<decltype(func)>(func), std::move(arguments));
						promise.set_value();
					} else {
						promise.set_value(std::apply(std::forward<decltype(func)>(func), std::move(arguments)));
					}
				} catch (...) {
					promise.set_exception(std::current_exception());
				}
			});
			return future.get();
		}

		boost::asio::io_service io_service;
		boost::asio::io_service::work work;
		Sockets_t sockets;
		Listeners_t listeners;
		std::thread server;
	} notifier;
};

#endif //NOTIFICATION_SERVER_H
