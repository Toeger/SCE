#include "lsp_feature.h"

#include <algorithm>

static std::vector<LSP_feature> lsp_features;

LSP_feature *LSP_feature::lookup(std::string_view name) {
    auto pos = std::find_if(std::begin(lsp_features), std::end(lsp_features), [&name](const LSP_feature &lsp_feature) { return lsp_feature.name == name; });
    if (pos == std::end(lsp_features)) {
        return nullptr;
    }
    return &(*pos);
}
