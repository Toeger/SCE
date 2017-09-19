#include "tool_actions.h"
#include "logic/process_reader.h"
#include "ui/edit_window.h"
#include "ui/mainwindow.h"

#include <QAction>
#include <QMessageBox>
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

static void show_output(const std::string &output, Tool_output_target::Type output_target, const QString &title, bool is_error,
						Edit_window *current_edit_window) {
	switch (output_target) {
		case Tool_output_target::ignore:
			break;
		case Tool_output_target::popup:
			if (is_error) {
				QMessageBox::critical(current_edit_window, title, Process_reader::strip_control_sequences_text(output));
			} else {
				QMessageBox::information(current_edit_window, title, Process_reader::strip_control_sequences_text(output));
			}
			break;
		case Tool_output_target::paste:
			MainWindow::get_current_edit_window()->insertPlainText(Process_reader::strip_control_sequences_text(output));
			break;
		case Tool_output_target::replace_document:
			Process_reader::set_text(MainWindow::get_current_edit_window(), output);
			break;
			//TODO: handle other cases
	}
}

static void run_action(const Tool &tool) {
	Process_reader p{tool};
	show_output(p.get_output(), tool.output, tool.get_name(), false, MainWindow::get_current_edit_window());
	show_output(p.get_error(), tool.error, tool.get_name(), true, MainWindow::get_current_edit_window());
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
