#include "process_reader.h"
#include "ui/edit_window.h"
#include "ui/mainwindow.h"

#include <QProcess>
#include <cassert>

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

Process_reader::Process_reader(const Tool &tool) {
	process.setWorkingDirectory(tool.working_directory);
	process.start(tool.path, create_arguments_list(resolve_placeholders(tool.arguments)));
	const auto selection = resolve_placeholders(tool.input).toUtf8();
	const auto bytes_written = process.write(selection);
	assert(selection.size() == bytes_written);
	process.closeWriteChannel();
	if (process.waitForFinished(3000)) { //TODO: Make this timeout a tools' argument.
		output = process.readAllStandardOutput();
		error = process.readAllStandardError();
	} else {
		assert(false); //TODO: handle timeouts
	}
}

const QString &Process_reader::get_output() const {
	return output;
}

const QString &Process_reader::get_error() const {
	return error;
}
