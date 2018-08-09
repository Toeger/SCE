#include "lsp_feature_setup_widget.h"
#include "checkbox_widget.h"
#include "interop/language_server_protocol.h"
#include "logic/settings.h"
#include "ui_lsp_feature_setup_widget.h"
#include "utility/thread_call.h"

#include <QSpacerItem>

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
	std::vector<QString> features;
	int tool_index;
};

struct LSP_feature_table {
	LSP_feature_table(QTableWidget *table_)
		: table{table_} {
		table->setRowCount(1);
		table->setVerticalHeaderItem(0, new QTableWidgetItem{});
		for (int column = 0, columncount = table->columnCount(); column < columncount; column++) {
			auto widget = std::make_unique<QWidget>();
			auto layout = std::make_unique<QHBoxLayout>();
			layout->setMargin(0);
			{
				auto all_button = std::make_unique<QPushButton>(QObject::tr("All"));
				QObject::connect(all_button.get(), &QPushButton::clicked, [column, table = this->table] {
					for (int line = 1, linecount = table->rowCount(); line < linecount; line++) {
						auto checkbox = dynamic_cast<Checkbox_widget *>(table->cellWidget(line, column));
						if (checkbox != nullptr) {
							checkbox->set_checked_state(Qt::Checked);
						}
					}
				});
				layout->addWidget(all_button.release());
			}
			layout->addSpacerItem(new QSpacerItem{0, 0, QSizePolicy::Expanding});
			{
				auto none_button = std::make_unique<QPushButton>(QObject::tr("None"));
				QObject::connect(none_button.get(), &QPushButton::clicked, [column, table = this->table] {
					for (int line = 1, linecount = table->rowCount(); line < linecount; line++) {
						auto checkbox = dynamic_cast<Checkbox_widget *>(table->cellWidget(line, column));
						if (checkbox != nullptr) {
							checkbox->set_checked_state(Qt::Unchecked);
						}
					}
				});
				layout->addWidget(none_button.release());
			}
			widget->setLayout(layout.release());
			table->setCellWidget(0, column, widget.release());
		}
	}
	void set(LSP_feature_info &&info) {
		Utility::sync_gui_thread_execute([this, info = std::move(info)]() mutable {
			for (auto &feature : info.features) {
				auto [it, inserted] = feature_to_row.try_emplace(std::move(feature));
				auto &[it_feature, it_row] = *it;
				if (inserted) {
					it_row = row;
					table->setRowCount(row + 1);
					table->setVerticalHeaderItem(row++, new QTableWidgetItem{it_feature});
				}
				table->setCellWidget(it->second, info.tool_index, new Checkbox_widget);
			}
		});
	}

	private:
	QTableWidget *table;
	std::map<QString, int> feature_to_row;
	int row = 1;
};

static void set_lsp_features(const std::vector<Tool> &tools, const std::function<void(int)> &set_progress_percentage,
							 const std::function<void(LSP_feature_info)> &add_features, const std::function<void()> &done_callback) {
	if (tools.empty()) {
		return;
	}
	const int max_steps = tools.size();
	int current_steps = 1;
	std::vector<QString> features;
	int tool_index = 0;
	for (auto &tool : tools) {
		LSP::Client lsp_client{tool};
		set_progress_percentage(current_steps++ * 100 / max_steps);
		features.resize(lsp_client.capabilities.size());
		auto capability_names = lsp_client.capabilities.items();
		std::transform(std::begin(capability_names), std::end(capability_names), std::begin(features),
					   [](auto json_key_value) { return QString::fromStdString(json_key_value.key()); });
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
