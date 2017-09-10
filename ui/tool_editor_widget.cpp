#include "tool_editor_widget.h"
#include "logic/settings.h"
#include "tests/test.h"
#include "ui_tool_editor_widget.h"

#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QStringList>
#include <tuple>

//this lists the association between ui elements and Tool members
static auto get_ui_inputs_tool_attributes(Ui::Tool_editor_widget *ui) {
	return std::make_tuple(ui->path_lineEdit, &Tool::path,                    //
						   ui->arguments_lineEdit, &Tool::arguments,          //
						   ui->input_lineEdit, &Tool::input,                  //
						   ui->output_comboBox, &Tool::output,                //
						   ui->errors_comboBox, &Tool::error,                 //
						   ui->activation_keySequenceEdit, &Tool::activation, //
						   ui->working_dir_lineEdit, &Tool::working_directory //
	);
}
//set ui elements based on a Tool member
static void tool_to_ui(QLineEdit *line_edit, QString Tool::*member, const Tool &tool) {
	line_edit->setText(tool.*member);
}
static void tool_to_ui(QComboBox *combo_box, Tool_output_target::Type Tool::*member, const Tool &tool) {
	combo_box->setCurrentIndex(tool.*member);
}
static void tool_to_ui(QKeySequenceEdit *key_edit, QKeySequence Tool::*member, const Tool &tool) {
	key_edit->setKeySequence(tool.*member);
}

//set all ui elements to the value of a Tool
template <class Tuple, std::size_t... indexes>
static void tool_to_ui(Tuple tuple, std::index_sequence<indexes...>, const Tool &tool) {
	(tool_to_ui(std::get<indexes * 2>(tuple), std::get<indexes * 2 + 1>(tuple), tool), ...);
}
static void tool_to_ui(Ui::Tool_editor_widget *ui, const Tool &tool) {
	auto ui_tuple = get_ui_inputs_tool_attributes(ui);
	tool_to_ui(std::move(ui_tuple), std::make_index_sequence<std::tuple_size<decltype(ui_tuple)>() / 2>(), tool);
}

//set tool values from ui
static void ui_to_tool(QLineEdit *line_edit, QString Tool::*member, Tool &tool) {
	tool.*member = line_edit->text();
}
static void ui_to_tool(QComboBox *combo_box, Tool_output_target::Type Tool::*member, Tool &tool) {
	tool.*member = static_cast<Tool_output_target::Type>(combo_box->currentIndex());
}
static void ui_to_tool(QKeySequenceEdit *key_edit, QKeySequence Tool::*member, Tool &tool) {
	tool.*member = key_edit->keySequence();
}

template <class Tuple, std::size_t... indexes>
static void ui_to_tool(Tuple tuple, std::index_sequence<indexes...>, Tool &tool) {
	(ui_to_tool(std::get<indexes * 2>(tuple), std::get<indexes * 2 + 1>(tuple), tool), ...);
}
static void ui_to_tool(Ui::Tool_editor_widget *ui, Tool &tool) {
	auto ui_tuple = get_ui_inputs_tool_attributes(ui);
	ui_to_tool(std::move(ui_tuple), std::make_index_sequence<std::tuple_size<decltype(ui_tuple)>() / 2>(), tool);
}

Tool_editor_widget::Tool_editor_widget(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::Tool_editor_widget) {
	ui->setupUi(this);
	load_tools_from_settings();
	fill_output_list(ui->output_comboBox);
	fill_output_list(ui->errors_comboBox);
	update_tools_list();
}

Tool_editor_widget::~Tool_editor_widget() {}

void Tool_editor_widget::closeEvent(QCloseEvent *event) {
	update_current_tool();
	if (need_to_save()) {
		const auto decision = QMessageBox::question(this, tr("Save changes?"), tr("Settings for some tools have been changed. Should they be saved?"),
													QMessageBox::SaveAll | QMessageBox::Ignore | QMessageBox::Cancel);
		switch (decision) {
			case QMessageBox::SaveAll:
				event->accept();
				on_buttonBox_accepted();
				return;
			case QMessageBox::Ignore:
				event->accept();
				on_buttonBox_rejected();
				return;
			case QMessageBox::Cancel:
				event->ignore();
				return;
		}
	}
}

void Tool_editor_widget::load_tools_from_settings() {
	tools = Settings::get<Settings::Key::tools>();
}

void Tool_editor_widget::save_tools_to_settings() const {
	Settings::set<Settings::Key::tools>(tools);
}

bool Tool_editor_widget::need_to_save() {
	auto old_tools = std::move(tools);
	load_tools_from_settings();
	if (tools == old_tools) {
		return false;
	}
	swap(old_tools, tools);
	return true;
}

void Tool_editor_widget::update_tools_list() {
	const int old_index = std::max(ui->tools_listWidget->currentRow(), 0);
	ui->tools_listWidget->clear();
	auto old_tools = std::move(tools);
	tools.clear();
	for (const auto &tool : old_tools) {
		ui->tools_listWidget->addItem(tool.path.split("/").last());
	}
	tools = std::move(old_tools);
	ui->tools_listWidget->setCurrentRow(old_index);
}

void Tool_editor_widget::update_current_tool() {
	if (ui->tools_listWidget->count() == 0) {
		return;
	}
	auto &current_tool = tools[ui->tools_listWidget->currentRow()];
	ui_to_tool(ui.get(), current_tool);
	update_current_tool_name();
}

void Tool_editor_widget::update_current_tool_name() {
	const auto current_path = tools[ui->tools_listWidget->currentRow()].path;
	auto current_tool_name = current_path.split('/').back();
	ui->tools_listWidget->currentItem()->setText(current_tool_name);
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
	if (currentRow == -1 || tools.empty()) {
		return;
	}
	const auto current_tool = tools[currentRow]; //This copy is important because setting GUI elements will trigger reading the current tool properties from the
												 //incomplete GUI, which would corrupt our properties. Making a copy keeps the current_tool unchanged from
												 //whatever the GUI does.
	tool_to_ui(ui.get(), current_tool);
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

void Tool_editor_widget::on_buttonBox_accepted() {
	update_current_tool();
	save_tools_to_settings();
	close();
}

void Tool_editor_widget::on_buttonBox_rejected() {
	close();
}
