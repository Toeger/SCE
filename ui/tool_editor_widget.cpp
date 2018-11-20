#include "tool_editor_widget.h"
#include "external/verdigris/wobjectimpl.h"
#include "logic/settings.h"
#include "logic/tool_actions.h"
#include "mainwindow.h"
#include "tests/test.h"
#include "ui_tool_editor_widget.h"

#include <QCloseEvent>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QStringList>
#include <tuple>

W_OBJECT_IMPL(Tool_editor_widget)

//this lists the association between ui elements and Tool members
static auto get_ui_inputs_tool_attributes(Ui::Tool_editor_widget *ui) {
	return std::make_tuple(ui->type_comboBox, &Tool::type,										//
						   ui->path_lineEdit, &Tool::path,										//
						   ui->arguments_lineEdit, &Tool::arguments,							//
						   ui->input_lineEdit, &Tool::input,									//
						   ui->output_comboBox, &Tool::output,									//
						   ui->error_comboBox, &Tool::error,									//
						   ui->working_dir_lineEdit, &Tool::working_directory,					//
						   ui->activation_comboBox, &Tool::activation,							//
						   ui->activation_keySequenceEdit, &Tool::activation_keyboard_shortcut, //
						   ui->timeout_doubleSpinBox, &Tool::timeout);
}
//set ui elements based on a Tool member
static void tool_to_ui(QComboBox *combo_box, Tool::Tool_type Tool::*member, const Tool &tool) {
	combo_box->setCurrentIndex(static_cast<int>(tool.*member));
}
static void tool_to_ui(QLineEdit *line_edit, QString Tool::*member, const Tool &tool) {
	line_edit->setText(tool.*member);
}
static void tool_to_ui(QComboBox *combo_box, Tool_output_target::Type Tool::*member, const Tool &tool) {
	combo_box->setCurrentIndex(tool.*member);
}
static void tool_to_ui(QComboBox *combo_box, Tool_activation::Type Tool::*member, const Tool &tool) {
	combo_box->setCurrentIndex(tool.*member);
}
static void tool_to_ui(QKeySequenceEdit *key_edit, QKeySequence Tool::*member, const Tool &tool) {
	key_edit->setKeySequence(tool.*member);
}
static void tool_to_ui(QDoubleSpinBox *spinbox, std::chrono::milliseconds Tool::*member, const Tool &tool) {
	spinbox->setValue((tool.*member).count() / 1000.);
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
static void ui_to_tool(QComboBox *combo_box, Tool::Tool_type Tool::*member, Tool &tool) {
	tool.*member = static_cast<Tool::Tool_type>(combo_box->currentIndex());
}
static void ui_to_tool(QLineEdit *line_edit, QString Tool::*member, Tool &tool) {
	tool.*member = line_edit->text();
}
static void ui_to_tool(QComboBox *combo_box, Tool_output_target::Type Tool::*member, Tool &tool) {
	tool.*member = static_cast<Tool_output_target::Type>(combo_box->currentIndex());
}
static void ui_to_tool(QComboBox *combo_box, Tool_activation::Type Tool::*member, Tool &tool) {
	tool.*member = static_cast<Tool_activation::Type>(combo_box->currentIndex());
}
static void ui_to_tool(QKeySequenceEdit *key_edit, QKeySequence Tool::*member, Tool &tool) {
	tool.*member = key_edit->keySequence();
}
static void ui_to_tool(QDoubleSpinBox *spinbox, std::chrono::milliseconds Tool::*member, Tool &tool) {
	tool.*member = std::chrono::milliseconds{static_cast<long long int>(spinbox->value() * 1000)};
}

template <class Tuple, std::size_t... indexes>
static void ui_to_tool(Tuple tuple, std::index_sequence<indexes...>, Tool &tool) {
	(ui_to_tool(std::get<indexes * 2>(tuple), std::get<indexes * 2 + 1>(tuple), tool), ...);
}
static void ui_to_tool(Ui::Tool_editor_widget *ui, Tool &tool) {
	auto ui_tuple = get_ui_inputs_tool_attributes(ui);
	ui_to_tool(std::move(ui_tuple), std::make_index_sequence<std::tuple_size<decltype(ui_tuple)>() / 2>(), tool);
}

template <auto function>
void fill_combobox(QComboBox *combobox) {
	for (const auto &entry : function()) {
		combobox->insertItem(std::numeric_limits<int>::max(), entry);
	}
}

Tool_editor_widget::Tool_editor_widget(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::Tool_editor_widget) {
	ui->setupUi(this);
	load_tools_from_settings();
	fill_combobox<Tool_output_target::get_texts>(ui->output_comboBox);
	fill_combobox<Tool_output_target::get_texts>(ui->error_comboBox);
	fill_combobox<Tool_activation::get_texts>(ui->activation_comboBox);
	update_tools_list();

	{
		std::pair<Helptext_label_widget *, QString> labels[] = {
			{ui->type_label,
			 tr(R"(<html><head/><body><p>Type of the tool.</p><p>Generic means it is a tool you simply want to execute and display the results, like <span style=" font-family:monospace;">git push</span> or <span style=" font-family:monospace;">sort</sort>.</p><p>LSP Server means a tool that implements the server side of the Language Server Protocol. See <a href="https://langserver.org/"><span style=" text-decoration: underline; color:#007af4;">langserver.org</span></a> for details.</p></body></html>)")},
			{ui->path_label, tr("Path to the executable. Use the ... to browse for it.")},
			{ui->working_dir_label, tr("Current working directory of the executable. If you set this to $FilePath you can use just the file name in arguments. "
									   "Default when left empty: %1")
										.arg(QDir::currentPath())},
			{ui->arguments_label, tr("Arguments passed to the tool, like --verbose. Note that path expansions like * will not work unless you start a shell "
									 "first and tell the shell to "
									 "execute the program.")},
			{ui->input_label,
			 tr(R"(<html><head/><body><p>The text to pass into the programs standard input, similarly to doing <span style=" font-family:monospace;">echo input | tool</span>.</p></body></html>)")},
			{ui->output_label, tr("What to do with the text that the tool prints to standard output.")},
			{ui->error_label, tr("What to do with the text that the tool prints to standard error.")},
			{ui->activation_label, tr("How to make the tool run.")},
			{ui->timeout_label, tr("Timeout in seconds until a popup asks you to kill the process. 0 means infinite.")},
		};
		for (auto &l : labels) {
			l.first->help_text = std::move(l.second);
			l.first->title = tr("Tool property: \"%1\"").arg(l.first->label->text());
		}
	}

	{
		std::pair<Helptext_label_widget *, std::function<QString()>> dyn_labels[] = {
			{ui->current_file_path_placeholder_label, [] { return tr("Currently set to %1.").arg(MainWindow::get_current_path()); }},
			{ui->selection_placeholder_label, [] {
				 const auto &selection = MainWindow::get_current_selection();
				 if (selection.isEmpty()) {
					 return tr("The current selection is empty.");
				 }
				 return tr("The current selection is \"%1\".").arg(selection);
			 }}};
		for (auto &l : dyn_labels) {
			l.first->help_text = std::move(l.second);
			l.first->title = tr("Tool Variable: \"%1\"").arg(l.first->label->text());
		}
	}
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
			default:
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

void Tool_editor_widget::set_tool_ui(Tool::Tool_type type) {
	//const QWidget *other_widgets[] = {
	//	ui->type_label, ui->type_comboBox, ui->path_label, ui->path_lineEdit, ui->path_browse_pushButton,
	//};
	QWidget *const lsp_tool_hidden_widgets[] = {
		ui->working_dir_label,
		ui->working_dir_lineEdit,
		ui->arguments_label,
		ui->arguments_lineEdit,
		ui->input_label,
		ui->input_lineEdit,
		ui->output_label,
		ui->output_comboBox,
		ui->error_label,
		ui->error_comboBox,
		ui->activation_label,
		ui->activation_comboBox,
		ui->activation_keySequenceEdit,
		ui->timeout_label,
		ui->timeout_doubleSpinBox,
		ui->variables_label,
		ui->current_file_path_placeholder_label,
		ui->current_file_path_explanation_label,
		ui->selection_placeholder_label,
		ui->selection_explanation_label,
	};
	for (auto &widget : lsp_tool_hidden_widgets) {
		widget->setVisible(type != Tool::Tool_type::LSP_server);
	}
}

void Tool_editor_widget::update_tools_list() {
	thread_check();
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
	thread_check();
	if (ui->tools_listWidget->count() == 0) {
		return;
	}
	auto &current_tool = tools[ui->tools_listWidget->currentRow()];
	ui_to_tool(ui.get(), current_tool);
	update_current_tool_name();
}

void Tool_editor_widget::update_current_tool_name() {
	thread_check();
	const auto current_path = tools[ui->tools_listWidget->currentRow()].path;
	auto current_tool_name = current_path.split('/').back();
	ui->tools_listWidget->currentItem()->setText(current_tool_name);
}

void Tool_editor_widget::on_add_pushButton_clicked() {
	tools.emplace_back();
	update_tools_list();
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
	Tool_actions::set_actions(tools);
	close();
}

void Tool_editor_widget::on_buttonBox_rejected() {
	close();
}

void Tool_editor_widget::on_activation_comboBox_currentIndexChanged(int index) {
	ui->activation_keySequenceEdit->setVisible(index == Tool_activation::Type::keyboard_shortcut);
}
void Tool_editor_widget::on_type_comboBox_currentIndexChanged(int index) {
	set_tool_ui(static_cast<Tool::Tool_type>(index));
}
