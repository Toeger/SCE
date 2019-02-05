#include "tool_actions.h"
#include "process_reader.h"
#include "settings.h"
#include "ui/edit_window.h"
#include "ui/mainwindow.h"

#include <QAction>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <cassert>
#include <memory>

static std::vector<std::unique_ptr<QAction>> actions;
static std::vector<QWidget *> widgets;

void Tool_actions::add_widget(QWidget *widget) {
	widgets.insert(std::lower_bound(std::begin(widgets), std::end(widgets), widget), widget);
	for (const auto &action : actions) {
		widget->addAction(action.get());
	}
}

void Tool_actions::remove_widget(QWidget *widget) {
	const auto pos = std::lower_bound(std::begin(widgets), std::end(widgets), widget);
	assert(*pos == widget); //if this assert fails the given widget was not added
	for (const auto &action : actions) {
		widget->removeAction(action.get());
	}
	widgets.erase(pos);
}

static void show_output(std::string_view output, Tool_output_target::Type output_target, const QString &title, bool is_error) {
	if (output.empty()) {
		return;
	}
	switch (output_target) {
		case Tool_output_target::ignore:
			break;
		case Tool_output_target::popup: {
			auto edit = new QPlainTextEdit(&MainWindow::get_main_window());
			/* Note: in theory the mainwindow owns the edit window and cleans up resources when it is closed.
			 * In practice if you close the mainwindow before the edit window this actually works.
			 * If you close the edit window first, however, LeakSanitizer reports 64 bytes leaked in 2 objects.
			 * I don't want to and probably can't fix Qt, so I'll just accept it as the price one has to pay for Qt.
			 */
			edit->setWindowFlag(Qt::WindowType::Window);
			edit->setWindowTitle((is_error ? "Error: " : "Output: ") + title);
			edit->setReadOnly(true);
			edit->setLineWrapMode(QPlainTextEdit::LineWrapMode::NoWrap);
			edit->setFont(Settings::get<Settings::Key::font>("console"));
			Ansi_code_handling::set_text(edit, output);
			auto cursor = edit->textCursor();
			cursor.movePosition(QTextCursor::Start);
			edit->setTextCursor(cursor);
			edit->resize(MainWindow::get_current_edit_window()->size()); //TODO: make the edit window exactly as big as it needs to be
			edit->show();
		} break;
		case Tool_output_target::paste:
			MainWindow::get_current_edit_window()->insertPlainText(Ansi_code_handling::strip_control_sequences_text(output));
			break;
		case Tool_output_target::replace_document:
			Ansi_code_handling::set_text(MainWindow::get_current_edit_window(), output);
			break;
		case Tool_output_target::console:
			//TODO: add a console and put text in there
			break;
	}
}

static void run_action(const Tool &tool) {
	std::string output;
	std::string error;
	try {
		Process_reader{tool, [&output](std::string_view sv) { output += sv; }, [&error](std::string_view sv) { error += sv; }}.join();
	} catch (const std::runtime_error &e) {
		QMessageBox::critical(&MainWindow::get_main_window(), "SCE - " + tool.get_name(),
							  QObject::tr("The tool %1 activated by %2 failed to execute: %3").arg(tool.get_name()).arg(tool.activation).arg(e.what()));
	} catch (...) {
		QMessageBox::critical(&MainWindow::get_main_window(), "SCE - " + tool.get_name(),
							  QObject::tr("The tool %1 activated by %2 failed to execute.").arg(tool.get_name()).arg(tool.activation));
	}
	show_output(output, tool.output, tool.get_name(), false);
	show_output(error, tool.error, tool.get_name(), true);
}

void Tool_actions::set_actions(const std::vector<Tool> &tools) {
	actions.resize(tools.size());
	std::transform(std::begin(tools), std::end(tools), std::begin(actions), [](const Tool &tool) {
		auto action = std::make_unique<QAction>();
		if (tool.activation == Tool_activation::keyboard_shortcut) {
			action->setShortcut(tool.activation_keyboard_shortcut);
		}
		QObject::connect(action.get(), &QAction::triggered, [tool] { run_action(tool); });
		for (auto &widget : widgets) {
			widget->addAction(action.get());
		}
		return action;
	});
}
