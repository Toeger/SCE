#include "test_notification_server.h"
#include "interop/notification_server.h"
#include "test.h"

#include <boost/asio/buffer.hpp>
#include <boost/asio/read.hpp>
#include <string>

static void wait_for_connections(const std::size_t target_connections, Notification_server &ns) {
	for (auto connections = ns.get_number_of_established_connections(); connections < target_connections;
		 connections = ns.get_number_of_established_connections()) {
		std::clog << "Waiting for connections: " << connections << '/' << target_connections << '\n';
		std::this_thread::sleep_for(std::chrono::milliseconds{200});
	}
}

static void test_simple_create_destroy() {
	Notification_server ns;
}

static void test_single_connection() {
	Notification_server ns;
	boost::asio::io_service io_service;
	boost::asio::ip::tcp::socket socket{io_service};
	socket.connect({boost::asio::ip::address_v4::loopback(), 53677});
	constexpr char test_data[] = "Hello world";
	wait_for_connections(1, ns);
	ns.send_notification(test_data);
	std::string buffer(sizeof test_data - 1, '\0');
	const auto received =
		boost::asio::read(socket, std::array{boost::asio::mutable_buffer{buffer.data(), buffer.size()}}, boost::asio::transfer_exactly(buffer.size()));
	buffer.resize(received);
	assert_equal(buffer, test_data);
}

static void test_multiple_connections() {
	Notification_server ns;
	boost::asio::io_service io_service;
	std::vector<boost::asio::ip::tcp::socket> sockets;
	constexpr auto number_of_sockets = 42u;
	std::generate_n(std::back_inserter(sockets), number_of_sockets, [&io_service] { return boost::asio::ip::tcp::socket{io_service}; });
	for (auto &socket : sockets) {
		socket.connect({boost::asio::ip::address_v4::loopback(), 53677});
	}
	constexpr char test_data[] = "Hello world";
	wait_for_connections(number_of_sockets, ns);
	ns.send_notification(test_data);
	for (auto &socket : sockets) {
		std::string buffer(sizeof test_data - 1, '\0');
		const auto received =
			boost::asio::read(socket, std::array{boost::asio::mutable_buffer{buffer.data(), buffer.size()}}, boost::asio::transfer_exactly(buffer.size()));
		buffer.resize(received);
		assert_equal(buffer, test_data);
	}
}

static void test_inaccessible_address_skip() {
	//the first attempt opening port 1 should already fail because we are not root, but just in case we try to open it twice so it definitely fails
	Notification_server ns{{{boost::asio::ip::address_v4::loopback(), 1}, {boost::asio::ip::address_v4::loopback(), 1}}};
}

void test_notification_server() {
	test_simple_create_destroy();
	test_single_connection();
	test_multiple_connections();
	test_inaccessible_address_skip();
}
