#include "lsp_feature.h"
#include "interop/language_server_protocol.h"

#include <algorithm>
#include <array>
#include <initializer_list>

static void enable_completion_provider() {}
static void disable_completion_provider() {}

static std::vector<LSP_feature> lsp_features = {{"completionProvider",
												 QObject::tr("Auto-completes code"),
												 enable_completion_provider,
												 disable_completion_provider,
												 LSP_feature::Multi_client_support::yes,
												 {}}};

LSP_feature *LSP_feature::lookup(std::string_view name) {
	auto pos = std::find_if(std::begin(lsp_features), std::end(lsp_features), [&name](const LSP_feature &lsp_feature) { return lsp_feature.name == name; });
	if (pos == std::end(lsp_features)) {
		return nullptr;
	}
	return &(*pos);
}
