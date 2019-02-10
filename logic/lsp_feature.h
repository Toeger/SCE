#ifndef LSP_FEATURE_H
#define LSP_FEATURE_H

#include "external/TMP/callable.h"
#include "external/json.hpp"

#include <QAction>
#include <QKeySequence>
#include <QString>
#include <memory>
#include <string_view>
#include <type_traits>
#include <vector>

namespace LSP {
	struct Client;
}

struct LSP_feature {
	const std::string_view name;
	const QString description;
	enum class Multi_client_support : bool { no, yes } multi_client_support;
	std::vector<std::shared_ptr<LSP::Client>> clients{};
	QKeySequence activation1{};
	QKeySequence activation2{};
	QAction action{};

	LSP_feature(std::string_view name, QString description, Multi_client_support multi_client_support, void (*const pdo_setup)(LSP_feature &),
				std::vector<std::shared_ptr<LSP::Client>> clients = {});

	static LSP_feature *lookup(std::string_view name);
	static void apply_to_each(TMP::Function_ref<void(LSP_feature &)> callback);
	//init and exit should be covered by RAII, but is not, because we cannot use Qt stuff like connections until a QApplication exists, so we have to delay init
	static void init_all_features();
	static void exit_all_features();
	static void add_all(QWidget &w);

	static void add_lsp_server(LSP::Client &client);
	static nlohmann::json get_init_params(std::string_view project_path);

	private:
	void (*const do_setup)(LSP_feature &);
};

#endif // LSP_FEATURE_H