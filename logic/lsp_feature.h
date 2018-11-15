#ifndef LSP_FEATURE_H
#define LSP_FEATURE_H

#include "external/TMP/callable.h"
#include "interop/language_server_protocol.h"

#include <QKeySequence>
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
	std::vector<std::shared_ptr<LSP::Client>> clients{};
	QKeySequence activation1{};
	QKeySequence activation2{};

	static LSP_feature *lookup(std::string_view name);
	static void apply_to_each(TMP::Function_ref<void(LSP_feature &)> callback);
};

#endif // LSP_FEATURE_H