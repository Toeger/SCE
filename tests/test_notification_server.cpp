#include "test_notification_server.h"
#include "interop/notification_server.h"
#include "logic/process_reader.h"
#include "test.h"

#include <boost/asio/buffer.hpp>
#include <boost/asio/read.hpp>
#include <fstream>
#include <sce.pb.h>
#include <string>
#include <string_view>

using namespace std::string_literals;

static void test_simple_create_destroy() {
	Notification_server ns;
}

static void test_single_connection() {
	Notification_server ns{{{boost::asio::ip::address_v4::loopback(), 53677}}};
	assert_equal(ns.get_listening_endpoints().size(), 1u);
	boost::asio::io_service io_service;
	boost::asio::ip::tcp::socket socket{io_service};
	socket.connect({boost::asio::ip::address_v4::loopback(), 53677});
	const std::string_view test_data = "Hello world";
	ns.wait_for_connections(1);
	ns.send_notification({test_data.data(), test_data.size()});
	std::string buffer(test_data.size(), '\0');
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
	ns.wait_for_connections(number_of_sockets);
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

static void test_multiple_sends() {
	Notification_server ns;
	boost::asio::io_service io_service;
	boost::asio::ip::tcp::socket socket{io_service};
	socket.connect({boost::asio::ip::address_v4::loopback(), 53677});
	const std::string_view test_data = "Some text with \0 characters";
	ns.wait_for_connections(1);
	for (const auto &c : test_data) {
		ns.send_notification({&c, 1});
	}
	std::string buffer(test_data.size(), '\0');
	const auto received =
		boost::asio::read(socket, std::array{boost::asio::mutable_buffer{buffer.data(), buffer.size()}}, boost::asio::transfer_exactly(buffer.size()));
	buffer.resize(received);
	assert_equal(buffer, test_data);
}

static void test_multiple_sends_to_multiple_connections() {
	Notification_server ns;
	boost::asio::io_service io_service;
	std::vector<boost::asio::ip::tcp::socket> sockets;
	constexpr auto number_of_sockets = 42u;
	std::generate_n(std::back_inserter(sockets), number_of_sockets, [&io_service] { return boost::asio::ip::tcp::socket{io_service}; });
	for (auto &socket : sockets) {
		socket.connect({boost::asio::ip::address_v4::loopback(), 53677});
	}
	const std::string_view test_data = "Some text with \0 characters";
	ns.wait_for_connections(number_of_sockets);
	for (const auto &c : test_data) {
		ns.send_notification({&c, 1});
	}
	for (auto &socket : sockets) {
		std::string buffer(test_data.size(), '\0');
		const auto received =
			boost::asio::read(socket, std::array{boost::asio::mutable_buffer{buffer.data(), buffer.size()}}, boost::asio::transfer_exactly(buffer.size()));
		buffer.resize(received);
		assert_equal(buffer, test_data);
	}
}

static void test_protbuffer_serialization() {
	std::string buffer;
	{
		sce::proto::EditNotification edit_notification;
		edit_notification.mutable_filestate()->set_id("TestID");
		edit_notification.mutable_filestate()->set_state(42);
		edit_notification.SerializeToString(&buffer);
	}
	{
		sce::proto::EditNotification edit_notification;
		assert_true(edit_notification.ParseFromString(buffer));
		assert_equal(edit_notification.filestate().id(), "TestID");
		assert_equal(edit_notification.filestate().state(), 42u);
	}
}

static void test_protobuffer_serialization_interop_python(int version) {
	const auto id = "TestID";
	{
		std::string buffer;
		sce::proto::EditNotification edit_notification;
		edit_notification.mutable_filestate()->set_id(id);
		edit_notification.mutable_filestate()->set_state(42);
		edit_notification.SerializeToString(&buffer);
		std::ofstream f{"/tmp/SCE_test_serialization", std::ios::binary | std::ios::out};
		f.write(buffer.data(), buffer.size());
	}
	Tool script;
	script.path = "sh";
	script.arguments = TEST_DATA_PATH "interop_scripts/run_python_script.sh python" + QString::number(version) + (" " TEST_DATA_PATH "test_deserialize.py");
	script.working_directory = TEST_DATA_PATH "interop_scripts";
	std::string output;
	std::string error;
	const auto start = std::chrono::high_resolution_clock::now();
	Process_reader{script, [&output](std::string_view data) { output += data; }, [&error](std::string_view data) { error += data; }}.join();
	const auto stop = std::chrono::high_resolution_clock::now() - start;
	Assert_message assert_message;
	assert_message << Color::cyan << "Running " << Color::no_color << script.path.toStdString() << ' ' << script.arguments.toStdString() << Color::cyan
				   << "\ntook " << std::chrono::duration_cast<std::chrono::milliseconds>(stop).count() << "ms\n"
				   << "and produced output \"" << Color::no_color << output << Color::cyan << "\"\nand error \"" << Color::no_color << error << Color::cyan
				   << "\"\n"
				   << Color::no_color;
	assert_equal(error, "");
	assert_equal(output, "Filestate id: "s + id);
}

static void test_protobuffer_serialization_interop() {
	test_protobuffer_serialization_interop_python(2);
	test_protobuffer_serialization_interop_python(3);
}

void test_notification_server() {
	test_simple_create_destroy();
	test_single_connection();
	test_multiple_connections();
	test_inaccessible_address_skip();
	test_multiple_sends();
	test_multiple_sends_to_multiple_connections();
	test_protbuffer_serialization();
	test_protobuffer_serialization_interop();
}
