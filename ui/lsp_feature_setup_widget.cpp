#include "lsp_feature_setup_widget.h"
#include "interop/language_server_protocol.h"
#include "logic/settings.h"
#include "ui_lsp_feature_setup_widget.h"
#include "utility/thread_call.h"

LSP_feature_setup_widget::LSP_feature_setup_widget(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::LSP_feature_setup_widget) {
	ui->setupUi(this);
	update_lsp_features();
}

LSP_feature_setup_widget::~LSP_feature_setup_widget() {
	if (feature_loader.joinable()) {
		feature_loader.join();
	}
}

static void set_lsp_features(const std::vector<Tool> &tools, std::function<void(int)> set_progress_percentage,
							 std::function<void(std::vector<std::string>)> add_features, std::function<void()> done_callback) {
	const int max_steps = tools.size();
	int current_steps = 1;
	std::vector<std::string> features;
	for (auto &tool : tools) {
		LSP::Client lsp_client{tool};
		set_progress_percentage(current_steps++ * 100 / max_steps);
		features.resize(lsp_client.capabilities.size());
		auto capability_names = lsp_client.capabilities.items();
		std::transform(std::begin(capability_names), std::end(capability_names), std::begin(features),
					   [](auto json_key_value) { return json_key_value.key(); });
		add_features(std::move(features));
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
		[this, row = 0](std::vector<std::string> &&features) mutable {
			Utility::sync_gui_thread_execute([this, features, &row] {
				ui->lsp_features_tableWidget->setRowCount(row + features.size());
				for (auto &feature : features) {
					ui->lsp_features_tableWidget->setVerticalHeaderItem(row++, new QTableWidgetItem{QString::fromStdString(feature)});
				}
			});
		},
		[this] { Utility::sync_gui_thread_execute([this] { ui->progressBar->setVisible(false); }); }};
}
