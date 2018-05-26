#include "test_notification_server.h"
#include "interop/notification_server.h"
#include "test.h"

#include <boost/asio/buffer.hpp>
#include <boost/asio/read.hpp>
#include <string>

static void test_simple_create_destroy() {
	Notification_server ns;
}

static void test_single_sender_receiver() {
	Notification_server ns;
	boost::asio::io_service io_service;
	boost::asio::ip::tcp::socket socket{io_service};
	socket.connect({boost::asio::ip::address_v4::loopback(), 53677});
	constexpr char test_data[] = "Hello world";
	ns.send_notification(test_data);
	std::string buffer{sizeof test_data - 1};
	const auto received =
		boost::asio::read(socket, std::array{boost::asio::mutable_buffer{buffer.data(), buffer.size()}}, boost::asio::transfer_exactly(buffer.size()));
	buffer.resize(received);
	assert_equal(buffer, test_data);
}

void test_notification_server() {
	test_simple_create_destroy();
	test_single_sender_receiver();
}
