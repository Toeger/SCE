#ifndef PROCESS_READER_H
#define PROCESS_READER_H

#include "tool.h"

#include <QProcess>
#include <QString>
#include <chrono>
#include <string_view>
#include <vector>

class Process_reader {
	public:
	Process_reader(const Tool &tool);
	bool has_finished() const;
	const QString &get_output() const;
	const QString &get_error() const;
	void cancel();

	private:
	QString output;
	QString error;
	QProcess process;
};

#endif // PROCESS_READER_H