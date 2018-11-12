#ifndef LSP_FEATURE_H
#define LSP_FEATURE_H

#include "interop/language_server_protocol.h"

#include <QString>
#include <memory>
#include <string_view>
#include <vector>

struct LSP_feature {
	const std::string_view name;
	const QString description;
	void (*const enable)();
	void (*const disable)();
	enum class Multi_client_support : bool { no, yes } multi_client_support;
	std::vector<std::shared_ptr<LSP::Client>> clients;

	static LSP_feature *lookup(std::string_view name);
};

#endif // LSP_FEATURE_H