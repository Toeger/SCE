#include "tool_actions.h"
#include "tool.h"
#include "ui/edit_window.h"
#include "ui/mainwindow.h"

#include <QAction>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <algorithm>
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

static void show_output(const QString &output, Tool_output_target::Type output_target, const QString &title, bool is_error, Edit_window *current_edit_window) {
	switch (output_target) {
		case Tool_output_target::ignore:
			break;
		case Tool_output_target::popup:
			if (is_error) {
				QMessageBox::critical(current_edit_window, title, output);
			} else {
				QMessageBox::information(current_edit_window, title, output);
			}
			break;
		case Tool_output_target::paste:
			MainWindow::get_current_edit_window()->insertPlainText(output);
			break;
		case Tool_output_target::replace_document:
			MainWindow::get_current_edit_window()->setPlainText(output);
			break;
			//TODO: handle other cases
	}
}

static QString resolve_placeholders(QString string) {
	const auto edit_window = MainWindow::get_current_edit_window();
	if (edit_window == nullptr) {
		return string;
	}
	struct Placeholder_value {
		QString placeholder;
		QString value;
	} const placeholder_values[] = {
		{"$FilePath", MainWindow::get_current_path()},                                    //
		{"$Selection", edit_window->textCursor().selectedText().replace("\u2029", "\n")}, //
	};
	for (const auto &placeholder_value : placeholder_values) {
		string.replace(placeholder_value.placeholder, placeholder_value.value);
	}
	return string;
}

static QStringList create_arguments_list(const QString &args_string) {
	//TODO: Warn about unbalanced quotation marks
	QStringList arguments{""};
	for (auto &args : args_string.split(' ')) {
		if (args.isEmpty()) {
			continue;
		}
		if (arguments.last().startsWith('"') && arguments.last().endsWith('"') == false) {
			arguments.last().push_back(' ');
			arguments.last() += args;
		} else {
			arguments << args;
		}
	}
	arguments.pop_front();
	return arguments;
}

static void run_action(const Tool &tool) {
	QProcess process;
	process.setWorkingDirectory(tool.working_directory);
	process.start(tool.path, create_arguments_list(resolve_placeholders(tool.arguments)));
	const auto selection = resolve_placeholders(tool.input).toUtf8();
	const auto bytes_written = process.write(selection);
	assert(selection.size() == bytes_written);
	process.closeWriteChannel();
	if (process.waitForFinished(3000)) { //TODO: Make this timeout a tools' argument.
		const auto output = process.readAllStandardOutput();
		show_output(output, tool.output, tool.get_name(), false, MainWindow::get_current_edit_window());
		const auto error = process.readAllStandardError();
		show_output(error, tool.error, tool.get_name(), true, MainWindow::get_current_edit_window());
	} else {
		assert(false); //TODO: handle timeouts
	}
}

void Tool_actions::set_actions(const std::vector<Tool> &tools) {
	actions.resize(tools.size());
	std::transform(std::begin(tools), std::end(tools), std::begin(actions), [](const Tool &tool) {
		auto action = std::make_unique<QAction>();
		action->setShortcut(tool.activation);
		QObject::connect(action.get(), &QAction::triggered, [tool] { run_action(tool); });
		for (auto &widget : widgets) {
			widget->addAction(action.get());
		}
		return action;
	});
}
