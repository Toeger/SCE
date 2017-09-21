#ifndef PROCESS_READER_H
#define PROCESS_READER_H

#include "tool.h"

#include <QProcess>
#include <string>

class QPlainTextEdit;
class QString;

namespace detail {
	QStringList create_arguments_list(const QString &args_string);
}

class Process_reader {
	public:
	Process_reader(const Tool &tool);
	bool has_finished() const;
	const std::string &get_output() const;
	const std::string &get_error() const;
	void cancel();
	static void set_text(QPlainTextEdit *text_edit, std::string_view text);
	static QString strip_control_sequences_text(std::string_view text);

	private:
	std::string output;
	std::string error;
	QProcess process;
};

#endif // PROCESS_READER_H