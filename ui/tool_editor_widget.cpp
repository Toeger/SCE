#include "tool_editor_widget.h"
#include "logic/settings.h"
#include "tests/test.h"
#include "ui_tool_editor_widget.h"

#include <QFileDialog>
#include <QStringList>

Tool_editor_widget::Tool_editor_widget(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::Tool_editor_widget) {
	ui->setupUi(this);
	load_tools_from_settings();
	fill_output_list(ui->output_comboBox);
	fill_output_list(ui->errors_comboBox);
	update_tools_list();
}

Tool_editor_widget::~Tool_editor_widget() {
	save_tools_to_settings();
}

void Tool_editor_widget::load_tools_from_settings() {
	const auto tool_list = Settings::get<Settings::Key::tools>();
	tools.clear();
	tools.reserve(tool_list.size());
	for (const auto &tool : tool_list) {
		tools.push_back(Tool::from_string(tool));
	}
}

void Tool_editor_widget::save_tools_to_settings() const {
	QStringList tool_list;
	for (const auto &tool : tools) {
		tool_list << tool.to_string();
	}
	Settings::set<Settings::Key::tools>(tool_list);
}

void Tool_editor_widget::update_tools_list() {
	const int old_index = std::max(ui->tools_listWidget->currentRow(), 0);
	ui->tools_listWidget->clear();
	for (const auto &tool : tools) {
		ui->tools_listWidget->addItem(tool.path.split("/").last());
	}
	ui->tools_listWidget->setCurrentRow(old_index);
}

void Tool_editor_widget::update_current_tool() {
	if (ui->tools_listWidget->count() == 0) {
		return;
	}
	auto &current_tool = tools[ui->tools_listWidget->currentRow()];
	current_tool.path = ui->path_lineEdit->text();
	current_tool.arguments = ui->arguments_lineEdit->text();
	current_tool.input = ui->input_lineEdit->text();
	current_tool.output = static_cast<Tool_output_target::Type>(ui->output_comboBox->currentIndex());
	current_tool.error = static_cast<Tool_output_target::Type>(ui->errors_comboBox->currentIndex());
	current_tool.activation;
	update_tools_list();
}

void Tool_editor_widget::on_add_pushButton_clicked() {
	tools.emplace_back();
	update_tools_list();
}

void Tool_editor_widget::fill_output_list(QComboBox *combobox) {
	struct Tool_output_dropdown_data {
		Tool_output_target::Type target;
		const QString &text;
	} const dropdown_entries[] = {
		{Tool_output_target::ignore, tr("Ignored")},
		{Tool_output_target::paste, tr("Paste into editor")},
		{Tool_output_target::console, tr("Display in console")},
		{Tool_output_target::popup, tr("Display in popup window")},
	};
	for (const auto &dropdown_entry : dropdown_entries) {
		assert_equal(dropdown_entry.target, &dropdown_entry - dropdown_entries);
		combobox->insertItem(dropdown_entry.target, dropdown_entry.text);
	}
}

void Tool_editor_widget::on_tools_listWidget_currentRowChanged(int currentRow) {
	if (currentRow == -1) {
		return;
	}
	auto &current_tool = tools[currentRow];
	ui->path_lineEdit->setText(current_tool.path);
	ui->arguments_lineEdit->setText(current_tool.arguments);
	ui->input_lineEdit->setText(current_tool.input);
	ui->output_comboBox->setCurrentIndex(current_tool.output);
	ui->errors_comboBox->setCurrentIndex(current_tool.error);
}

void Tool_editor_widget::on_remove_pushButton_clicked() {
	const auto current_index = ui->tools_listWidget->currentRow();
	if (current_index == -1) {
		return;
	}
	tools.erase(std::begin(tools) + current_index);
	update_tools_list();
}

void Tool_editor_widget::on_path_browse_pushButton_clicked() {
	auto old_path = ui->path_lineEdit->text();
	if (old_path.isEmpty()) {
		old_path = "/";
	}
	const auto file_name = QFileDialog::getOpenFileName(this, tr("Select Executable"), old_path);
	if (file_name.isEmpty()) {
		return;
	}
	ui->path_lineEdit->setText(file_name);
}
