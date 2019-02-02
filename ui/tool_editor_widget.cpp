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
#include <QGroupBox>
#include <QMessageBox>
#include <QStringList>
#include <tuple>

W_OBJECT_IMPL(Tool_editor_widget)

namespace {
	//this lists the association between ui elements and Tool members
	static auto get_ui_inputs_tool_attributes(Ui::Tool_editor_widget *ui) {
		return std::make_tuple(ui->enabled_groupBox, &Tool::enabled,								//
							   ui->type_comboBox, &Tool::type,										//
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
	static void tool_to_ui(QGroupBox *groupbox, bool Tool::*member, const Tool &tool) {
		groupbox->setChecked(tool.*member);
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
	static void ui_to_tool(QGroupBox *groupbox, bool Tool::*member, Tool &tool) {
		tool.*member = groupbox->isChecked();
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
} // namespace

Tool_editor_widget::Tool_editor_widget(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::Tool_editor_widget) {
	ui->setupUi(this);
	saved_tools = ui_tools = Settings::get<Settings::Key::tools>();
	fill_combobox<Tool_output_target::get_texts>(ui->output_comboBox);
	fill_combobox<Tool_output_target::get_texts>(ui->error_comboBox);
	fill_combobox<Tool_activation::get_texts>(ui->activation_comboBox);
	update_tools_list();

	//set up help text for tool options
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

	//set up variable text for tool options
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
	update_current_tool_from_ui();
	if (need_to_save()) {
		const auto decision = QMessageBox::question(this, tr("Save changes?"), tr("Settings for some tools have been changed. Should they be saved?"),
													QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
		switch (decision) {
			case QMessageBox::Save:
				event->accept();
				on_buttonBox_accepted();
				return;
			case QMessageBox::Discard:
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

void Tool_editor_widget::keyPressEvent(QKeyEvent *event) {
	if (event->key() == Qt::Key::Key_Escape) {
		event->accept();
		close();
		return;
	}
	QWidget::keyPressEvent(event);
}

void Tool_editor_widget::save_tools_to_settings() {
	Settings::set<Settings::Key::tools>(ui_tools);
	saved_tools = ui_tools;
}

bool Tool_editor_widget::need_to_save() {
	return saved_tools != ui_tools;
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
	while (ui->tools_listWidget->count() > ui_tools.size()) {
		ui->tools_listWidget->takeItem(0);
	}
	while (ui->tools_listWidget->count() < ui_tools.size()) {
		ui->tools_listWidget->addItem("");
	}
	for (int i = 0; i < ui_tools.size(); i++) {
		update_tool_name(i);
	}
	tool_to_ui(ui.get(), ui_tools[current_tool_list_row]);
	ui->tools_listWidget->setCurrentRow(current_tool_list_row);
}

void Tool_editor_widget::update_current_tool_from_ui() {
	thread_check();
	if (ui->tools_listWidget->count() == 0) {
		return;
	}
	auto &current_tool = ui_tools[current_tool_list_row];
	ui_to_tool(ui.get(), current_tool);
	update_tool_name(current_tool_list_row);
}

void Tool_editor_widget::update_tool_name(int index) {
	thread_check();
	auto &tool = ui_tools[index];
	auto tool_name = tool.path.split('/').back();
	auto &item = *ui->tools_listWidget->item(index);
	if (tool_name.isEmpty()) {
		item.setText(tr("<none>"));
		item.setTextColor(Qt::gray);
	} else {
		item.setText(tool_name);
		item.setTextColor(tool.enabled ? Qt::black : Qt::gray);
	}
}

void Tool_editor_widget::on_add_pushButton_clicked() {
	ui_tools.emplace_back();
	update_tools_list();
	ui->tools_listWidget->setCurrentRow(ui->tools_listWidget->count() - 1);
}

void Tool_editor_widget::on_tools_listWidget_currentRowChanged(int currentRow) {
	if (currentRow == -1 || ui_tools.empty()) {
		return;
	}
	ui_to_tool(ui.get(), ui_tools[current_tool_list_row]);
	update_tool_name(current_tool_list_row);
	current_tool_list_row = currentRow;
	tool_to_ui(ui.get(), ui_tools[current_tool_list_row]);
}

void Tool_editor_widget::on_remove_pushButton_clicked() {
	if (current_tool_list_row == -1) {
		return;
	}
	ui_tools.erase(std::begin(ui_tools) + current_tool_list_row);
	if (current_tool_list_row == ui_tools.size()) {
		current_tool_list_row--;
	}
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
	update_current_tool_from_ui();
	save_tools_to_settings();
	Tool_actions::set_actions(saved_tools);
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
