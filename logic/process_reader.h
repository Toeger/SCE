#ifndef PROCESS_READER_H
#define PROCESS_READER_H

#include "tool.h"
#include "utility/pipe.h"
#include "utility/thread_safe.h"

#include <atomic>
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

#if __linux
#ifndef USING_TTY
#define USING_TTY true
#else
#define USING_TTY false
#endif
#endif
constexpr bool using_tty = USING_TTY;

class Process_reader {
	public:
	enum class State { running, error, finished };
	State get_state() const;

	Process_reader(Tool tool, //
				   std::function<void(std::string_view)> output_callback = [](std::string_view) {},
				   std::function<void(std::string_view)> error_callback = [](std::string_view) {},
				   std::function<void(State)> completion_callback = [](State) {});
	Process_reader(const Process_reader &) = delete;
	static void run(QString executable, QString args, std::ostream &output, std::ostream &error);

	void kill();
	void join();
	void send_input(std::string_view input);
	void close_input();

	private:
	void run_process(Tool tool);

	struct {
		std::function<void(std::string_view)> output_callback;
		std::function<void(std::string_view)> error_callback;
		std::function<void(State)> completion_callback;
		std::thread process_handler;
	} gui_thread_private;
	struct {
		std::atomic<State> state{State::running};
		Thread_safe<Pipe> standard_input;
	} shared;
};

#endif // PROCESS_READER_H