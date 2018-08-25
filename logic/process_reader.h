#ifndef PROCESS_READER_H
#define PROCESS_READER_H

#include "tool.h"
#include "utility/pipe.h"
#include "utility/thread_safe.h"

#include <atomic>
#include <functional>
#include <future>
#include <string_view>

class QPlainTextEdit;
class QString;

namespace detail {
	QStringList create_arguments_list(const QString &args_string);
}

namespace Ansi_code_handling {
	void set_text(QPlainTextEdit *text_edit, std::string_view text);
	QString strip_control_sequences_text(std::string_view text);
} // namespace Ansi_code_handling

class Process_reader {
	public:
	enum class State { running, error, finished };
	State get_state() const;

	Process_reader(Tool tool, //
				   std::function<void(std::string_view)> output_callback = [](std::string_view) {},
				   std::function<void(std::string_view)> error_callback = [](std::string_view) {},
				   std::function<void(State)> completion_callback = [](State) {});
	Process_reader(const Process_reader &) = delete;
	~Process_reader();
	static bool run(QString executable, QString args, std::ostream &output, std::ostream &error, bool use_tty = true);

	void kill();
	bool join();
	void send_input(std::string_view input);
	void close_input();

	private:
	std::future<void> process_handler;
	Pipe standard_input;
	std::atomic<State> state{State::running};

	static void run_process(Tool tool, QStringList arguments, std::promise<Pipe> standard_in_promise, std::atomic<State> &state,
							std::function<void(std::string_view)> output_callback, std::function<void(std::string_view)> error_callback,
							std::function<void(Process_reader::State)> completion_callback);
};

#endif // PROCESS_READER_H