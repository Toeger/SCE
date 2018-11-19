#ifndef LSP_FEATURE_H
#define LSP_FEATURE_H

#include "external/TMP/callable.h"
#include "interop/language_server_protocol.h"

#include <QAction>
#include <QKeySequence>
#include <QString>
#include <memory>
#include <string_view>
#include <vector>

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
	static void setup_all();
	static void close_all();
	static void add_all(QWidget &w);

	private:
	void (*const do_setup)(LSP_feature &);
};

#endif // LSP_FEATURE_H