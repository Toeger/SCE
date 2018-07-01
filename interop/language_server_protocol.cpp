#include "language_server_protocol.h"
#include "utility/color.h"
#include "utility/utility.h"

#include <iostream>
#include <regex>
#include <sstream>
#include <string_view>
#include <unistd.h>

static constexpr auto mwv = Utility::make_whitespaces_visible;

static thread_local unsigned int message_counter;

static std::string write_lsp_message(std::string_view method, const nlohmann::json &params, std::string_view newline_out, bool is_notification) {
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
			case nlohmann::json::value_t::string:          ///< string value
			case nlohmann::json::value_t::boolean:         ///< boolean value
			case nlohmann::json::value_t::number_integer:  ///< number value (signed integer)
			case nlohmann::json::value_t::number_unsigned: ///< number value (unsigned integer)
			case nlohmann::json::value_t::number_float:    ///< number value (floating-point)
				json["params"] = nlohmann::json::array_t{params};
				break;
			case nlohmann::json::value_t::null:      ///< null value
			case nlohmann::json::value_t::discarded: ///< discarded by the the parser callback function
				break;
		}
		content = json.dump();
	}

	//header
	{
		data += "Content-Length: " + Utility::to_string(content.size()) + Utility::to_string(newline_out);
		data += "Content-Type: application/sce-jsonrpc; charset=utf-8" + Utility::to_string(newline_out);
		data += newline_out;
	}
	data += std::move(content);
	return data;
}

static nlohmann::json get_capabilities() {
	nlohmann::json params;
	//TODO: add WorkspaceClientCapabilities
	//TODO: add TextDocumentClientCapabilities
	//TODO: add experimental capabilities
	return params;
}

static nlohmann::json get_init_params() {
	nlohmann::json params;
	params["processId"] = getpid();
	params["rootUri"] = nullptr; //TODO: change this to project directory once we have such a thing
	params["capabilities"] = get_capabilities();
	return params;
}

static bool is_complete_lsp_message(std::string_view data, std::string_view newline) {
	std::regex content_length_regex{"Content-Length: ([[:digit:]]+)" + Utility::to_string(newline)};
	std::cmatch content_length_match;
	if (not std::regex_search(std::begin(data), std::end(data), content_length_match, content_length_regex)) {
		return false;
	}
	const auto content_length = std::stoul(content_length_match[1]);

	std::regex header_end_regex{Utility::to_string(newline) + Utility::to_string(newline)};
	std::cmatch header_end_match;
	if (not std::regex_search(std::begin(data), std::end(data), header_end_match, header_end_regex)) {
		return false;
	}
	const auto header_end = header_end_match.position() + header_end_match.length();
	return header_end + content_length >= data.size();
}

struct LSP_response {
	LSP::Response response;
	int size;
	unsigned int id;
};
static LSP_response parse_lsp_message(std::string_view data, std::string_view newline) {
	LSP_response response;

	std::cmatch match;
	std::regex content_length_regex{"Content-Length: ([[:digit:]]+)" + Utility::to_string(newline)};
	if (not std::regex_search(std::begin(data), std::end(data), match, content_length_regex)) {
		throw std::runtime_error("Invalid LSP message");
	}
	const auto content_length = std::stoul(match[1]);

	std::regex header_end_regex{Utility::to_string(newline) + Utility::to_string(newline)};
	if (not std::regex_search(std::begin(data), std::end(data), match, header_end_regex)) {
		throw std::runtime_error("Invalid LSP message");
	}
	const auto header_end = match.position() + match.length();

	std::regex content_type_regex{"Content-Type: ([^" + Utility::to_string(newline) + "]*)" + Utility::to_string(newline)};
	if (not std::regex_search(std::begin(data), std::end(data), match, content_length_regex)) {
		throw std::runtime_error("Invalid LSP message");
	}
	const auto content_type = match[1]; //TODO: do something useful with the content type
	std::cout << "Content-type: " << content_type << '\n';

	assert(header_end + content_length >= data.size());

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

LSP::Client::Client(Tool tool)
	: process_reader{std::move(tool),
					 [this, received = std::string{}](std::string_view output) mutable {
						 received += output;
						 std::clog << Color::green << mwv(output) << Color::no_color;
						 if (is_complete_lsp_message(received, newline_in)) {
							 auto response = parse_lsp_message(received, newline_in);
							 if (response.id == target_id) {
								 response_promise.set_value(response.response);
							 }
							 std::copy(std::begin(received) + response.size, std::end(received), std::begin(received));
							 received.resize(received.size() - response.size);
						 }
					 },
					 [](std::string_view error) { std::clog << Color::red << mwv(error) << Color::no_color; }} {
	Request init;
	init.method = "initialize";
	init.params = get_init_params();
	auto response = call(init);
	if (response.error) {
		auto &error = response.error.value();
		throw std::runtime_error(error.message);
	}
	if (response.result) {
		auto &result = response.result.value();
		std::cout << result.dump(1, '\t') << '\n';
	}
}

LSP::Client::~Client() {
	Notification shutdown;
	shutdown.method = "exit";
	try {
		notify(shutdown);
	} catch (const std::runtime_error &) {
	}
}

LSP::Response LSP::Client::call(const LSP::Request &request) {
	response_promise = std::promise<Response>{};
	auto response_future = response_promise.get_future();
	const auto message = write_lsp_message(request.method, request.params, newline_out, false);
	target_id = message_counter;
	std::clog << Color::yellow << mwv(message) << Color::no_color;
	process_reader.send_input(message);
	return response_future.get();
}

void LSP::Client::notify(const LSP::Notification &notification) {
	const auto lsp_message = write_lsp_message(notification.method, notification.params, newline_out, true);
	std::clog << Color::yellow << mwv(lsp_message) << Color::no_color;
	process_reader.send_input(lsp_message);
}
