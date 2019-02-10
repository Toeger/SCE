#include "interop/notification_server.h"
#include "logic/process_reader.h"
#include "logic/tool.h"
#include "test.h"
#include "utility/color.h"

#include <boost/asio/buffer.hpp>
#include <boost/asio/read.hpp>
#include <fstream>
#include <string>
#include <string_view>

using namespace std::string_literals;

TEST_CASE("Testing notification server", "[notification_server]") {
	WHEN("Simply creating a notification server") {
		Notification_server ns;
	}
	WHEN("Testing single connection") {
		Notification_server ns{{{boost::asio::ip::address_v4::loopback(), 53677}}};
		REQUIRE(ns.get_listening_endpoints().size() == 1u);
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
		REQUIRE(buffer == test_data);
	}
	WHEN("Testing multiple connections") {
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
			REQUIRE(buffer == test_data);
		}
	}
	WHEN("Setting an inaccessible address") {
		//the first attempt opening port 1 should already fail because we are not root, but just in case we try to open it twice so it definitely fails
		Notification_server ns{{{boost::asio::ip::address_v4::loopback(), 1}, {boost::asio::ip::address_v4::loopback(), 1}}};
	}
	WHEN("Sending multiple times") {
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
		REQUIRE(buffer == test_data);
	}
	WHEN("Doing multiple sends to multiple connections") {
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
			REQUIRE(buffer == test_data);
		}
	}
}

TEST_CASE("Testing protobuffers", "[protobuffer]") {
	WHEN("Serializing and deserializing locally") {
		std::string buffer;
		{
			sce::proto::EditNotification edit_notification;
			edit_notification.mutable_filestate()->set_id("TestID");
			edit_notification.mutable_filestate()->set_state(42);
			edit_notification.SerializeToString(&buffer);
		}
		{
			sce::proto::EditNotification edit_notification;
			CHECK(edit_notification.ParseFromString(buffer));
			REQUIRE(edit_notification.filestate().id() == "TestID");
			REQUIRE(edit_notification.filestate().state() == 42u);
		}
	}
	WHEN("Serializing in C++ and deserializing in Python") {
		for (const int version : {2, 3}) {
			WHEN("Using python" + std::to_string(version)) {
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
				script.arguments =
					TEST_DATA_PATH "interop_scripts/run_python_script.sh python" + QString::number(version) + (" " TEST_DATA_PATH "test_deserialize.py");
				script.working_directory = TEST_DATA_PATH "interop_scripts";
				std::string output;
				std::string error;
				const auto start = std::chrono::high_resolution_clock::now();
				Process_reader{script, [&output](std::string_view data) { output += data; }, [&error](std::string_view data) { error += data; }}.join();
				const auto stop = std::chrono::high_resolution_clock::now() - start;
				INFO(Color::cyan << "Running " << Color::no_color << script.path.toStdString() << ' ' << script.arguments.toStdString() << Color::cyan
								 << "\ntook " << std::chrono::duration_cast<std::chrono::milliseconds>(stop).count() << "ms\n"
								 << "and produced output \"" << Color::no_color << output << Color::cyan << "\"\nand error \"" << Color::no_color << error
								 << Color::cyan << "\"\n"
								 << Color::no_color);
				REQUIRE(error == "");
				REQUIRE(output == "Filestate id: "s + id);
			}
		}
	}
}