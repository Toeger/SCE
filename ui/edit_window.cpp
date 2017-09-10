#include "edit_window.h"
#include "logic/settings.h"
#include "logic/tool.h"

#include <QAction>
#include <QMessageBox>
#include <QProcess>
#include <memory>

Edit_window::Edit_window() {
	for (const auto &tool : Settings::get<Settings::Key::tools>()) {
		if (tool.activation.isEmpty()) {
			continue;
		}
		auto action = std::make_unique<QAction>();
		action->setShortcut(tool.activation);
		connect(action.get(), &QAction::triggered, [this, tool] {
			QProcess process;
			process.setWorkingDirectory(tool.working_directory);
			process.start(tool.path,
						  //TODO: This passes all arguments as a string. Need to find a way to pass multiple arguments and know where arguments start and end.
						  QStringList{} << tool.arguments);
			process.write(tool.input.toUtf8());
			process.closeWriteChannel();
			if (process.waitForFinished(3000)) { //TODO: Make this timeout a tools' argument.
				const auto output = process.readAllStandardOutput();
				show_output(output, tool.output, tool.get_name(), false);
				const auto error = process.readAllStandardError();
				show_output(error, tool.error, tool.get_name(), true);
			}
		});
		addAction(action.release());
	}
}

Edit_window::~Edit_window() {}

void Edit_window::wheelEvent(QWheelEvent *we) {
	if (we->modifiers() == Qt::ControlModifier) {
		const auto raw_zoom = we->delta() + zoom_remainder;
		const auto zoom = raw_zoom / QWheelEvent::DefaultDeltasPerStep;
		zoom_remainder = raw_zoom % QWheelEvent::DefaultDeltasPerStep;
		zoomIn(zoom);
		we->accept();
	} else {
		QPlainTextEdit::wheelEvent(we);
	}
}

void Edit_window::show_output(const QString &output, Tool_output_target::Type output_target, const QString &title, bool is_error) {
	switch (output_target) {
		case Tool_output_target::ignore:
			break;
		case Tool_output_target::popup:
			if (is_error) {
				QMessageBox::critical(this, title, output);
			} else {
				QMessageBox::information(this, title, output);
			}
			break;
			//TODO: handle other cases
	}
}
