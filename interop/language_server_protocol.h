#ifndef LANGUAGE_SERVER_PROTOCOL_H
#define LANGUAGE_SERVER_PROTOCOL_H

#include <atomic>
#include <future>
#include <iosfwd>
#include <json.hpp>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "logic/process_reader.h"

struct Tool;

namespace LSP {
	struct Request {
		std::string method;
		nlohmann::json params;
	};

	struct Response_error {
		int code;
		enum ErrorCodes {
			ParseError = -32700,
			InvalidRequest = -32600,
			MethodNotFound = -32601,
			InvalidParams = -32602,
			InternalError = -32603,
			serverErrorStart = -32099,
			serverErrorEnd = -32000,
			ServerNotInitialized = -32002,
			UnknownErrorCode = -32001,
			RequestCancelled = -32800,
		};
		std::string message;
		nlohmann::json data;
	};

	struct Response {
		std::optional<nlohmann::json> result;
		std::optional<Response_error> error;
	};

	struct Notification {
		std::string method;
		nlohmann::json params;
	};

	struct Client {
		Client(Tool tool);
		~Client();
		Response call(const Request &request);
		void notify(const Notification &notification);

		private:
		Process_reader process_reader;
		std::promise<Response> response_promise;
		std::atomic<unsigned int> target_id;
	};
} // namespace LSP

#endif