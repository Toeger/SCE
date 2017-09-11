#include "tool_actions.h"
#include "tool.h"
#include "ui/edit_window.h"
#include "ui/mainwindow.h"

#include <QAction>
#include <QMessageBox>
#include <QProcess>
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
			//TODO: handle other cases
	}
}

static void run_action(const Tool &tool) {
	QProcess process;
	process.setWorkingDirectory(tool.working_directory);
	process.start(tool.path,
				  //TODO: This passes all arguments as a string. Need to find a way to pass multiple arguments and know where arguments start and end.
				  QStringList{} << tool.arguments);
	process.write(tool.input.toUtf8());
	process.closeWriteChannel();
	if (process.waitForFinished(3000)) { //TODO: Make this timeout a tools' argument.
		const auto output = process.readAllStandardOutput();
		show_output(output, tool.output, tool.get_name(), false, MainWindow::get_current_edit_window());
		const auto error = process.readAllStandardError();
		show_output(error, tool.error, tool.get_name(), true, MainWindow::get_current_edit_window());
	}
}

void Tool_actions::set_actions(const std::vector<Tool> &tools) {
	actions.clear(); //disconnects all connections
	for (const auto &tool : tools) {
		actions.push_back(std::make_unique<QAction>());
		auto &action = *actions.back();
		action.setShortcut(tool.activation);
		QObject::connect(&action, &QAction::triggered, [tool] { run_action(tool); });
		for (auto &widget : widgets) {
			widget->addAction(&action);
		}
	}
}
