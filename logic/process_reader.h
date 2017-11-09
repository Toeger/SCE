#ifndef PROCESS_READER_H
#define PROCESS_READER_H

#include "tool.h"

#include <functional>
#include <string_view>
#include <thread>

class QPlainTextEdit;
class QString;

namespace detail {
	QStringList create_arguments_list(const QString &args_string);
}

namespace Ansi_code_handling {
	void set_text(QPlainTextEdit *text_edit, std::string_view text);
	QString strip_control_sequences_text(std::string_view text);
} // namespace Ansi_code_handling

//#undef __linux

class Process_reader {
	public:
	enum class State { running, error, finished };
	State get_state() const {
		return state;
	}

	Process_reader(Tool tool, //
				   std::function<void(std::string_view)> output_callback = [](std::string_view) {},
				   std::function<void(std::string_view)> error_callback = [](std::string_view) {},
				   std::function<void(State)> completion_callback = [](State) {});
	Process_reader(const Process_reader &) = delete;

	void kill();
	void join();

	private:
	State state{State::running};
	void run_process(Tool tool);
	std::function<void(std::string_view)> output_callback;
	std::function<void(std::string_view)> error_callback;
	std::function<void(State)> completion_callback;
	std::thread process_handler;
};

#endif // PROCESS_READER_H