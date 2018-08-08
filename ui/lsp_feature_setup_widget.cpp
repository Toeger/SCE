#include "lsp_feature_setup_widget.h"
#include "checkbox_widget.h"
#include "interop/language_server_protocol.h"
#include "logic/settings.h"
#include "ui_lsp_feature_setup_widget.h"
#include "utility/thread_call.h"

LSP_feature_setup_widget::LSP_feature_setup_widget(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::LSP_feature_setup_widget) {
	ui->setupUi(this);
	ui->splitter->setSizes({1, 2});
	update_lsp_features();
}

LSP_feature_setup_widget::~LSP_feature_setup_widget() {
	if (feature_loader.joinable()) {
		feature_loader.join();
	}
}

struct LSP_feature_info {
	std::vector<std::string> features;
	int tool_index;
};

struct LSP_feature_table {
	LSP_feature_table(QTableWidget *table_)
		: table{table_} {}
	void set(LSP_feature_info &&info) {
		Utility::sync_gui_thread_execute([this, info = std::move(info)]() mutable {
			for (auto &feature : info.features) {
				auto [it, inserted] = feature_to_row.try_emplace(std::move(feature));
				auto &[it_feature, it_row] = *it;
				if (inserted) {
					it_row = row;
					table->setRowCount(row + 1);
					table->setVerticalHeaderItem(row++, new QTableWidgetItem{QString::fromStdString(it_feature)});
				}
				table->setCellWidget(it->second, info.tool_index, new Checkbox_widget);
			}
		});
	}

	private:
	QTableWidget *table;
	std::map<std::string, int> feature_to_row;
	int row = 0;
};

static void set_lsp_features(const std::vector<Tool> &tools, std::function<void(int)> set_progress_percentage,
							 std::function<void(LSP_feature_info)> add_features, std::function<void()> done_callback) {
	const int max_steps = tools.size();
	int current_steps = 1;
	std::vector<std::string> features;
	int tool_index = 0;
	for (auto &tool : tools) {
		LSP::Client lsp_client{tool};
		set_progress_percentage(current_steps++ * 100 / max_steps);
		features.resize(lsp_client.capabilities.size());
		auto capability_names = lsp_client.capabilities.items();
		std::transform(std::begin(capability_names), std::end(capability_names), std::begin(features),
					   [](auto json_key_value) { return json_key_value.key(); });
		add_features({std::move(features), tool_index++});
	}
	done_callback();
}

void LSP_feature_setup_widget::update_lsp_features() {
	tools = Settings::get<Settings::Key::tools>();
	tools.erase(std::remove_if(std::begin(tools), std::end(tools), [](const Tool &tool) { return tool.type != Tool::Tool_type::LSP_server; }), std::end(tools));
	ui->lsp_tools_listWidget->clear();
	if (tools.empty()) {
		return;
	}

	ui->lsp_features_tableWidget->clear();
	ui->lsp_features_tableWidget->setColumnCount(tools.size() + 1);
	QStringList header_texts;
	for (const auto &tool : tools) {
		ui->lsp_tools_listWidget->addItem(tool.get_name());
		header_texts << tool.get_name();
	}
	header_texts << tr("Off");
	ui->lsp_features_tableWidget->setHorizontalHeaderLabels(header_texts);
	ui->progressBar->setValue(0);
	ui->progressBar->setVisible(true);
	feature_loader = std::thread{
		&set_lsp_features, std::ref(tools),
		[this](int progress_percentage) { Utility::sync_gui_thread_execute([this, progress_percentage] { ui->progressBar->setValue(progress_percentage); }); },
		[feature_table = LSP_feature_table{ui->lsp_features_tableWidget}](LSP_feature_info &&info) mutable { feature_table.set(std::move(info)); },
		[this] { Utility::sync_gui_thread_execute([this] { ui->progressBar->setVisible(false); }); }};
}
