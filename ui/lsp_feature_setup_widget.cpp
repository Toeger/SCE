#include "lsp_feature_setup_widget.h"
#include "checkbox_widget.h"
#include "external/verdigris/wobjectimpl.h"
#include "interop/language_server_protocol.h"
#include "logic/lsp_feature.h"
#include "logic/settings.h"
#include "threading/thread_call.h"
#include "ui_lsp_feature_setup_widget.h"

#include <QCheckBox>
#include <QPushButton>
#include <QSpacerItem>
#include <algorithm>
#include <variant>

W_OBJECT_IMPL(LSP_feature_setup_widget)

LSP_feature_setup_widget::LSP_feature_setup_widget(QWidget *parent)
	: QWidget(parent)
	, __(new Ui::LSP_feature_setup_widget)
	, ui{__.get()} {
	ui->setupUi(this);
	ui->splitter->setSizes({1, 2});
	update_gui_from_settings_and_LSP_servers();
}

LSP_feature_setup_widget::~LSP_feature_setup_widget() {
	if (feature_loader.valid()) {
		Utility::get_future_value(std::move(feature_loader));
	}
}

void LSP_feature_setup_widget::update_lsp_features_from_settings() {
	auto lsp_tools = Settings::get<Settings::Key::tools>();
	lsp_tools.erase(std::remove_if(std::begin(lsp_tools), std::end(lsp_tools), [](const Tool &tool) { return tool.type != Tool::Tool_type::LSP_server; }),
					std::end(lsp_tools));
	if (lsp_tools.empty()) {
		return;
	}
	std::sort(std::begin(lsp_tools), std::end(lsp_tools), [](const Tool &lhs, const Tool &rhs) { return lhs.get_name() < rhs.get_name(); });

	LSP_feature::apply_to_each([](LSP_feature &feature) { feature.clients.clear(); });
	const auto &lsp_feature_to_lsp_client_names = Settings::get<Settings::Key::lsp_functions>();
	for (const auto &[feature_name, client_names] : lsp_feature_to_lsp_client_names) {
		if (auto feature = LSP_feature::lookup(feature_name)) {
			for (const auto &client_name : client_names) {
				const auto lsp_tool_it = std::lower_bound(std::cbegin(lsp_tools), std::cend(lsp_tools), QString::fromStdString(client_name),
														  [](const Tool &tool, const QString &name) { return tool.get_name() < name; });
				if (lsp_tool_it == std::cend(lsp_tools)) {
					continue;
				}
				try {
					feature->clients.push_back(LSP::Client::get_client_from_cache(*lsp_tool_it));
				} catch (std::exception &e) {
					MainWindow::get_main_window().set_status(QString{"Failed loading lsp tool %1: Error: %2"}.arg(lsp_tool_it->get_name()).arg(e.what()));
				}
			}
		}
	}
}

struct LSP_feature_info {
	std::variant<std::vector<QString>, QString> features_or_error;
	int tool_index;
};

struct LSP_feature_table {
	LSP_feature_table(Gui_pointer<QTableWidget> table_, Utility::Thread_caller<LSP_feature_setup_widget> &thread_caller)
		: table{table_} {
		table->setRowCount(1);
		table->setVerticalHeaderItem(0, new QTableWidgetItem{});
		for (int column = 0, columncount = table->columnCount(); column < columncount; column++) {
			auto select_all_checkbox = std::make_unique<QCheckBox>(QObject::tr("Select All"));
			select_all_checkbox->setTristate();
			select_all_checkbox->setCheckState(Qt::CheckState::PartiallyChecked);
			select_all_checkbox->setEnabled(false);
			thread_caller.async_gui_thread_execute([checkbox = select_all_checkbox.get(), column](LSP_feature_setup_widget *widget) {
				QObject::connect(checkbox, &QCheckBox::clicked, [widget, column] { widget->feature_checkbox_clicked(0, column); });
			});
			table->setCellWidget(0, column, select_all_checkbox.release());
		}
		table->resizeColumnsToContents();
	}
	void set(LSP_feature_info &&info, Utility::Thread_caller<LSP_feature_setup_widget> &thread_caller) {
		Utility::sync_gui_thread_execute([this, info = std::move(info), &thread_caller]() mutable {
			if (std::holds_alternative<QString>(info.features_or_error)) { //error
				table->horizontalHeaderItem(info.tool_index)->setTextColor(Qt::red);
				table->horizontalHeaderItem(info.tool_index)->setToolTip(std::get<QString>(info.features_or_error));
				return;
			}
			//no error
			for (auto &feature : std::get<std::vector<QString>>(info.features_or_error)) {
				auto [it, inserted] = feature_to_row.try_emplace(std::move(feature));
				auto &[it_feature, it_row] = *it;
				auto feature_name_widget = std::make_unique<QTableWidgetItem>(it_feature);
				auto lsp_feature = LSP_feature::lookup(it_feature.toStdString());
				if (inserted) {
					it_row = row;
					table->setRowCount(row + 1);
					if (lsp_feature) {
						feature_name_widget->setToolTip(lsp_feature->description);
					} else {
						feature_name_widget->setToolTip(QObject::tr("Feature %1 is not understood by SCE").arg(it_feature));
						feature_name_widget->setTextColor(Qt::red);
					}
					table->setVerticalHeaderItem(row, feature_name_widget.release());
					row++;
				}
				if (lsp_feature) {
					auto checkbox = std::make_unique<Checkbox_widget>();
					checkbox->setEnabled(false);
					thread_caller.async_gui_thread_execute(
						[checkbox = checkbox.get(), row = it->second, column = info.tool_index](LSP_feature_setup_widget *widget) {
							QObject::connect(checkbox, &Checkbox_widget::clicked, [widget, row, column] { widget->feature_checkbox_clicked(row, column); });
						});
					table->setCellWidget(it->second, info.tool_index, checkbox.release());
				}
			}
		});
	}

	private:
	Gui_pointer<QTableWidget> table;
	std::map<QString, int> feature_to_row;
	int row = 1;
};

static void set_lsp_features(const std::vector<Tool> &tools, const std::function<void(int)> &set_progress_percentage,
							 const std::function<void(LSP_feature_info)> &add_features, const std::function<void()> &done_callback) {
	if (tools.empty()) {
		return;
	}
	const int max_steps = tools.size();
	int current_steps = 0;
	std::vector<QString> features;
	int tool_index = 0;
	for (auto &tool : tools) {
		set_progress_percentage(current_steps++ * 100 / max_steps);
		std::optional<nlohmann::json> capabilities;
		try {
			capabilities = LSP::Client::get_client_from_cache(tool)->capabilities;
		} catch (const std::runtime_error &e) {
			add_features({QObject::tr("Failed getting capabilities for %1: %2").arg(tool.get_name()).arg(e.what()), tool_index++});
			continue;
		}
		features.resize(capabilities->size());
		auto capability_names = capabilities->items();
		std::transform(std::begin(capability_names), std::end(capability_names), std::begin(features),
					   [](auto json_key_value) { return QString::fromStdString(json_key_value.key()); });
		add_features({std::move(features), tool_index++});
	}
	done_callback();
}

void LSP_feature_setup_widget::feature_checkbox_clicked(int row, int column) {
	if (row != 0) { //clicked a feature checkbox
		assert(dynamic_cast<Checkbox_widget *>(ui->lsp_features_tableWidget->cellWidget(row, column)));
		auto checkbox = static_cast<Checkbox_widget *>(ui->lsp_features_tableWidget->cellWidget(row, column));
		check_states[row][column] = checkbox->get_checked_state() == Qt::CheckState::Checked;
		return;
	}
	//clicked a select all checkbox
	assert(dynamic_cast<QCheckBox *>(ui->lsp_features_tableWidget->cellWidget(0, column)));
	auto select_all_checkbox = static_cast<QCheckBox *>(ui->lsp_features_tableWidget->cellWidget(0, column));

	for (int line = 1, linecount = ui->lsp_features_tableWidget->rowCount(); line < linecount; line++) {
		auto checkbox = dynamic_cast<Checkbox_widget *>(ui->lsp_features_tableWidget->cellWidget(line, column));
		if (checkbox != nullptr) {
			switch (select_all_checkbox->checkState()) {
				case Qt::CheckState::Checked:
					checkbox->set_checked_state(Qt::CheckState::Checked);
					break;
				case Qt::CheckState::Unchecked:
					checkbox->set_checked_state(Qt::CheckState::Unchecked);
					break;
				case Qt::CheckState::PartiallyChecked:
					checkbox->set_checked_state(check_states[line][column] ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
					break;
			}
		}
	}
}

void LSP_feature_setup_widget::on_buttonBox_accepted() {
	save_lsp_settings_from_gui();
	update_lsp_features_from_settings();
	close();
}

void LSP_feature_setup_widget::on_buttonBox_rejected() {
	close();
}

void LSP_feature_setup_widget::update_gui_from_settings_and_LSP_servers() {
	tools = Settings::get<Settings::Key::tools>();
	tools.erase(std::remove_if(std::begin(tools), std::end(tools), [](const Tool &tool) { return tool.type != Tool::Tool_type::LSP_server; }), std::end(tools));
	if (tools.empty()) {
		return;
	}

	ui->lsp_features_tableWidget->clear();
	ui->lsp_features_tableWidget->setColumnCount(tools.size() + 1);
	QStringList header_texts;
	for (const auto &tool : tools) {
		header_texts << tool.get_name();
	}
	header_texts << tr("Off");
	ui->lsp_features_tableWidget->setHorizontalHeaderLabels(header_texts);
	ui->progressBar->setValue(0);
	ui->progressBar->setVisible(true);
	auto set_progress_callback = [this](int progress_percentage) {
		Utility::sync_gui_thread_execute([this, progress_percentage] { ui->progressBar->setValue(progress_percentage); });
	};
	auto add_features_callback = [feature_table = LSP_feature_table{Gui_pointer(ui->lsp_features_tableWidget), thread_caller},
								  this](LSP_feature_info &&info) mutable { feature_table.set(std::move(info), thread_caller); };
	auto done_callback = [this] {
		Utility::sync_gui_thread_execute([this] {
			ui->progressBar->setVisible(false);
			check_states.resize(ui->lsp_features_tableWidget->rowCount(), std::vector<bool>(ui->lsp_features_tableWidget->columnCount(), true));
			for (int column = 0; column < ui->lsp_features_tableWidget->columnCount(); column++) {
				assert(dynamic_cast<QCheckBox *>(ui->lsp_features_tableWidget->cellWidget(0, column)));
				static_cast<QCheckBox *>(ui->lsp_features_tableWidget->cellWidget(0, column))->setEnabled(true);
				for (int row = 0; row < ui->lsp_features_tableWidget->rowCount(); row++) {
					if (auto checkbox = dynamic_cast<Checkbox_widget *>(ui->lsp_features_tableWidget->cellWidget(row, column))) {
						checkbox->setEnabled(true);
					}
				}
			}
			load_lsp_settings_to_gui();
		});
	};
	feature_loader = std::async(std::launch::async, &set_lsp_features, std::ref(tools), std::move(set_progress_callback), std::move(add_features_callback),
								std::move(done_callback));
}

void LSP_feature_setup_widget::load_lsp_settings_to_gui() {
	const auto &lsp_feature_to_lsp_client_names = Settings::get<Settings::Key::lsp_functions>();
	std::vector<std::string> lsp_feature_names(ui->lsp_features_tableWidget->rowCount());
	for (int row = 0; row < ui->lsp_features_tableWidget->rowCount(); row++) {
		lsp_feature_names[row] = ui->lsp_features_tableWidget->verticalHeaderItem(row)->text().toStdString();
	}
	std::vector<std::string> lsp_tool_names(tools.size());
	std::transform(std::cbegin(tools), std::cend(tools), std::begin(lsp_tool_names), [](const Tool &tool) { return tool.get_name().toStdString(); });

	for (const auto &[feature_name, lsp_client_names] : lsp_feature_to_lsp_client_names) {
		const auto row = std::find(std::cbegin(lsp_feature_names), std::cend(lsp_feature_names), feature_name) - std::cbegin(lsp_feature_names);
		if (row == lsp_feature_names.size()) { //unknown feature name in settings
			continue;
		}
		for (auto &lsp_client_name : lsp_client_names) {
			const auto column = std::find(std::cbegin(lsp_tool_names), std::cend(lsp_tool_names), lsp_client_name) - std::cbegin(lsp_tool_names);
			if (column == std::size(lsp_tool_names)) { //unknown tool name
				continue;
			}
			if (auto checkbox = dynamic_cast<Checkbox_widget *>(ui->lsp_features_tableWidget->cellWidget(row, column))) {
				checkbox->set_checked_state(Qt::CheckState::Checked);
			} else {
			}
		}
	}
}

void LSP_feature_setup_widget::save_lsp_settings_from_gui() {
	std::map<std::string, std::vector<std::string>> feature_table;
	for (int row = 1; row < ui->lsp_features_tableWidget->rowCount(); row++) {
		for (int column = 0; column < ui->lsp_features_tableWidget->columnCount() - 1; column++) { //-1 because last column is not a tool
			if (auto checkbox = dynamic_cast<Checkbox_widget *>(ui->lsp_features_tableWidget->cellWidget(row, column));
				checkbox && checkbox->get_checked_state() == Qt::CheckState::Checked) {
				feature_table[ui->lsp_features_tableWidget->verticalHeaderItem(row)->text().toStdString()].push_back(tools[column].get_name().toStdString());
			}
		}
	}
	Settings::set<Settings::Key::lsp_functions>(std::move(feature_table));
}
