#ifndef LANGUAGE_SERVER_PROTOCOL_H
#define LANGUAGE_SERVER_PROTOCOL_H

#include <atomic>
#include <chrono>
#include <future>
#include <json.hpp>
#include <map>
#include <memory>
#include <optional>
#include <string>

#include "logic/process_reader.h"

struct Tool;
struct Project;

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
		Client(Tool tool, const Project &project);
		~Client();
		Response call(const Request &request, const std::chrono::milliseconds &timeout = std::chrono::milliseconds{3000});
		void notify(const Notification &notification);
		nlohmann::json capabilities;

		static std::shared_ptr<Client> get_client_from_cache(const Tool &tool, const Project &project);
		static std::shared_ptr<Client> lookup_client_from_path(const QString &path);
		static std::map<QString, std::shared_ptr<Client>> &get_clients();

		private:
		Process_reader process_reader;
		std::promise<Response> response_promise;
		std::atomic<unsigned int> target_id;
		static inline std::map<QString, std::shared_ptr<Client>> clients;
	};

} // namespace LSP

#endif