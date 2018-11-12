#include "lsp_feature_setup_widget.h"
#include "checkbox_widget.h"
#include "interop/language_server_protocol.h"
#include "logic/lsp_feature.h"
#include "logic/settings.h"
#include "threading/thread_call.h"
#include "ui_lsp_feature_setup_widget.h"

#include <QCheckBox>
#include <QPushButton>
#include <QSpacerItem>
#include <variant>

LSP_feature_setup_widget::LSP_feature_setup_widget(QWidget *parent)
	: QWidget(parent)
	, __(new Ui::LSP_feature_setup_widget)
	, ui{__.get()} {
	ui->setupUi(this);
	ui->splitter->setSizes({1, 2});
	update_lsp_features();
}

LSP_feature_setup_widget::~LSP_feature_setup_widget() {
	if (feature_loader.valid()) {
		Utility::get_future_value(std::move(feature_loader));
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
			capabilities = LSP::Client::cached_get(tool)->capabilities;
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

void LSP_feature_setup_widget::update_lsp_features() {
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
	feature_loader =
		std::async(std::launch::async, &set_lsp_features, std::ref(tools),
				   [this](int progress_percentage) { //set progress callback
					   Utility::sync_gui_thread_execute([this, progress_percentage] { ui->progressBar->setValue(progress_percentage); });
				   },
				   [feature_table = LSP_feature_table{Gui_pointer(ui->lsp_features_tableWidget), thread_caller},
					this](LSP_feature_info &&info) mutable { //add feature callback
					   feature_table.set(std::move(info), thread_caller);
				   },
				   [this] { //done callback
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
					   });
				   });
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
	std::vector<std::shared_ptr<LSP::Client>> clients;
	std::transform(std::begin(tools), std::end(tools), std::back_inserter(clients), [](const Tool &tool) { return LSP::Client::lookup(tool.path); });

	for (int row = 0; row < ui->lsp_features_tableWidget->rowCount(); row++) {
		auto lsp_feature = LSP_feature::lookup(ui->lsp_features_tableWidget->verticalHeaderItem(row)->text().toStdString());
		if (lsp_feature == nullptr) {
			continue;
		}
		lsp_feature->clients.clear();
		for (int column = 0; column < ui->lsp_features_tableWidget->columnCount(); column++) {
			if (clients[column] == nullptr) {
				continue;
			}
			if (auto checkbox = dynamic_cast<Checkbox_widget *>(ui->lsp_features_tableWidget->cellWidget(row, column))) {
				if (checkbox->get_checked_state() == Qt::CheckState::Checked) {
					lsp_feature->clients.push_back(clients[column]);
				}
			}
		}
		if (lsp_feature->clients.empty()) {
			lsp_feature->disable();
		} else {
			lsp_feature->enable();
		}
	}
	close();
}

void LSP_feature_setup_widget::on_buttonBox_rejected() {
	close();
}
