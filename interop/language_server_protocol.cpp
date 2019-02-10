#include "language_server_protocol.h"
#include "logic/lsp_feature.h"
#include "logic/tool.h"
#include "threading/thread_call.h"
#include "utility/color.h"
#include "utility/utility.h"

#include <iostream>
#include <regex>
#include <sstream>
#include <string_view>
#include <unistd.h>

static constexpr auto mwv = Utility::make_whitespaces_visible;

static thread_local unsigned int message_counter;

static std::string make_lsp_message_string(std::string_view method, const nlohmann::json &params, bool is_notification) {
	std::string data;
	std::string content;
	//content
	{
		nlohmann::json json{
			{"jsonrpc", "2.0"},
			{"method", method},
		};
		if (not is_notification) {
			json["id"] = Utility::to_string(++message_counter);
		}
		switch (params.type()) {
			case nlohmann::json::value_t::object: ///< object (unordered set of name/value pairs)
			case nlohmann::json::value_t::array:  ///< array (ordered collection of values)
				json["params"] = params;
				break;
			case nlohmann::json::value_t::string:		   ///< string value
			case nlohmann::json::value_t::boolean:		   ///< boolean value
			case nlohmann::json::value_t::number_integer:  ///< number value (signed integer)
			case nlohmann::json::value_t::number_unsigned: ///< number value (unsigned integer)
			case nlohmann::json::value_t::number_float:	///< number value (floating-point)
				json["params"] = nlohmann::json::array_t{params};
				break;
			case nlohmann::json::value_t::null:		 ///< null value
			case nlohmann::json::value_t::discarded: ///< discarded by the the parser callback function
				break;
		}
		content = json.dump();
	}

	//header
	data += "Content-Length: " + Utility::to_string(content.size()) + "\r\n\r\n";

	data += std::move(content);
	return data;
}

static bool is_complete_lsp_message(std::string_view data) {
	std::regex content_length_regex{R"(Content-Length: ([[:digit:]]+)\r\n)"};
	std::cmatch content_length_match;
	if (not std::regex_search(std::begin(data), std::end(data), content_length_match, content_length_regex)) {
		return false;
	}
	const auto content_length = std::stoul(content_length_match[1]);

	std::regex header_end_regex{R"(\r\n\r\n)"};
	std::cmatch header_end_match;
	if (not std::regex_search(std::begin(data), std::end(data), header_end_match, header_end_regex)) {
		return false;
	}
	const auto header_end = header_end_match.position() + header_end_match.length();
	return header_end + content_length <= data.size();
}

struct LSP_response {
	LSP::Response response;
	int size;
	unsigned int id;
};
static LSP_response parse_lsp_message(std::string_view data) {
	LSP_response response;

	std::cmatch match;
	std::regex content_length_regex{R"(Content-Length: ([[:digit:]]+)\r\n)"};
	if (not std::regex_search(std::begin(data), std::end(data), match, content_length_regex)) {
		throw std::runtime_error("Invalid LSP message");
	}
	const auto content_length = std::stoul(match[1]);

	std::regex header_end_regex{R"(\r\n\r\n)"};
	if (not std::regex_search(std::begin(data), std::end(data), match, header_end_regex)) {
		throw std::runtime_error("Invalid LSP message");
	}
	const auto header_end = match.position() + match.length();

	assert(header_end + content_length <= data.size());

	data.remove_prefix(header_end);
	data.remove_suffix(data.size() - content_length);

	response.size = header_end + content_length;
	auto response_data = nlohmann::json::parse(data);
	if (not response_data.count("id")) {
		response.id = 0;
	} else {
		std::string response_id = response_data["id"];
		response.id = std::stoul(response_id);
	}
	if (response_data.count("result")) {
		response.response.result = response_data["result"];
	}
	if (response_data.count("error")) {
		auto error = response_data["error"];
		LSP::Response_error response_error;
		response_error.code = error["code"];
		if (error.count(data)) {
			response_error.data = error["data"];
		}
		response_error.message = error["message"];
		response.response.error = std::move(response_error);
	}

	return response;
}

LSP::Client::Client(Tool tool, const Project &project)
	: process_reader{[ltool = std::move(tool)]() mutable {
						 ltool.use_tty_mode = false; //never use tty mode with LSP tools
						 return std::move(ltool);
					 }(),
					 [this, received = std::string{}](std::string_view output) mutable {
						 received += output;
						 std::clog << Color::green << mwv(output) << Color::no_color;
						 if (is_complete_lsp_message(received)) {
							 std::clog << '\n';
							 auto response = parse_lsp_message(received);
							 if (response.id == target_id) {
								 response_promise.set_value(response.response);
							 }
							 std::copy(std::begin(received) + response.size, std::end(received), std::begin(received));
							 received.resize(received.size() - response.size);
						 }
					 },
					 [](std::string_view error) { std::clog << Color::red << mwv(error) << Color::no_color; }} {
	Request request;
	request.method = "initialize";
	request.params = LSP_feature::get_init_params(project);
	auto response = call(request);
	if (response.error) {
		auto &error = response.error.value();
		throw std::runtime_error(error.message);
	}
	if (response.result) {
		auto &result = response.result.value();
		capabilities = result["capabilities"];
	}
}

LSP::Client::~Client() {
	Request request;
	request.method = "shutdown";
	try {
		auto response = call(request);
		if (response.error) {
			auto &error = response.error.value();
			std::clog << Color::red << "Failed shutting down LSP server: " << error.message << Color::no_color << '\n';
			return;
		}
	} catch (const std::exception &error) {
		std::clog << Color::red << "Failed shutting down LSP server: " << error.what() << Color::no_color << '\n';
		return;
	}

	Notification notification;
	notification.method = "exit";
	try {
		notify(notification);
	} catch (const std::exception &error) {
		std::clog << Color::red << "Failed shutting down LSP server: " << error.what() << Color::no_color << '\n';
	}
}

LSP::Response LSP::Client::call(const LSP::Request &request, const std::chrono::milliseconds &timeout) {
	response_promise = std::promise<Response>{};
	auto response_future = response_promise.get_future();
	const auto message = make_lsp_message_string(request.method, request.params, false);
	target_id = message_counter;
	std::clog << Color::yellow << mwv(message) << Color::no_color << '\n';
	process_reader.send_input(message);
	return Utility::get_future_value(std::move(response_future), timeout);
}

void LSP::Client::notify(const LSP::Notification &notification) {
	const auto lsp_message = make_lsp_message_string(notification.method, notification.params, true);
	std::clog << Color::yellow << mwv(lsp_message) << Color::no_color << '\n';
	process_reader.send_input(lsp_message);
}

std::shared_ptr<LSP::Client> LSP::Client::get_client_from_cache(const Tool &tool, const Project &project) {
	auto it = clients.find(tool.path);
	if (it == std::end(clients)) {
		it = clients.emplace(tool.path, std::make_shared<Client>(tool, project)).first;
		Utility::async_gui_thread_execute([server = it->second] { LSP_feature::add_lsp_server(*server); });
	}
	return it->second;
}

std::shared_ptr<LSP::Client> LSP::Client::lookup_client_from_path(const QString &path) {
	const auto it = clients.find(path);
	if (it == std::end(clients)) {
		return nullptr;
	}
	return it->second;
}

std::map<QString, std::shared_ptr<LSP::Client> > &LSP::Client::get_clients() {
	return clients;
}