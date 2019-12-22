#include "process_reader.h"

#include "threading/thread_call.h"
#include "ui/edit_window.h"
#include "ui/mainwindow.h"
#include "ui/tool_editor_widget.h"
#include "utility/error.h"
#include "utility/pipe.h"
#include "utility/unique_handle.h"

#include <QApplication>
#include <QPlainTextEdit>
#include <QProcess>
#include <array>
#include <cassert>
#include <csignal>
#include <exception>
#include <pty.h>
#include <sstream>
#include <unistd.h>

using namespace std::string_literals;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"

static void select(Pipe &out, std::function<void(std::string_view)> &out_report, Pipe &err, std::function<void(std::string_view)> &err_report,
				   timeval *timeout) {
	assert(out.is_open() || err.is_open());
	fd_set read_file_descriptors{};
	int max_file_descriptor = 0;
	for (auto p : {&out, &err}) {
		if (p->is_open()) {
			FD_SET(p->get_read_channel(), &read_file_descriptors);
			max_file_descriptor = std::max(max_file_descriptor, p->get_read_channel());
		}
	}
	const auto selected = ::select(max_file_descriptor + 1, &read_file_descriptors, nullptr, nullptr, timeout);

	if (selected > 0) {
		const std::array<std::pair<Pipe &, std::function<void(std::string_view)> &>, 2> pipes = {{{out, out_report}, {err, err_report}}};
		for (auto &read_pipe : pipes) {
			if (read_pipe.first.is_open() == false) {
				continue;
			}
			if (FD_ISSET(read_pipe.first.get_read_channel(), &read_file_descriptors)) {
				read_pipe.second(read_pipe.first.read());
			}
		}
	} else if (selected == 0) { //timeout occured
								//TODO: handle timeout properly
		for (auto &read_pipe : {&out, &err}) {
			read_pipe->close_read_channel();
		}
		return;
	} else { //error occured
		throw std::runtime_error("Select failed: "s + strerror(errno));
	}
}

#pragma clang diagnostic pop

static termios get_termios_settings() {
	termios terminal_settings{};
	terminal_settings.c_iflag = ICRNL | IXOFF | IXON | IUTF8;
	terminal_settings.c_oflag = OPOST | ONLCR;
	terminal_settings.c_cflag = B38400 | CSIZE | CREAD;
	terminal_settings.c_lflag = ISIG | ICANON | ECHO | ECHOE | ECHOK | ECHOCTL | ECHOKE | IEXTEN;
	const auto cc_data = {VKILL,
						  VREPRINT | VEOL2,
						  127, //why 127? That's not a legal value
						  VTIME | VEOL2,
						  VEOF,
						  VINTR,
						  VQUIT,
						  VINTR,
						  VEOL2 | VQUIT,
						  VQUIT | VERASE | VKILL | VEOL2,
						  VERASE | VSTART | VSUSP | VEOL2,
						  VINTR,
						  VERASE | VEOL2,
						  VLNEXT,
						  VQUIT | VERASE | VKILL | VEOF | VTIME | VMIN | VSWTC | VEOL2,
						  VERASE | VEOF | VMIN | VEOL2};
	std::copy(std::begin(cc_data), std::end(cc_data), std::begin(terminal_settings.c_cc));
	terminal_settings.c_ispeed = B38400;
	terminal_settings.c_line = 0;
	terminal_settings.c_ospeed = B38400;

	return terminal_settings;
}

static void set_environment() {
	struct Environment_variable {
		const char *name;
		const char *value;
	} environment_variables[] = {
		{"LS_COLORS",
		 "rs=0:di=01;34:ln=01;36:mh=00:pi=40;33:so=01;35:do=01;35:bd=40;33;01:cd=40;33;01:or=40;31;01:mi=00:su=37;41:sg=30;43:ca=30;41:tw=30;42:ow=34;42:st="
		 "37;44:ex=01;32:*.tar=01;31:*.tgz=01;31:*.arc=01;31:*.arj=01;31:*.taz=01;31:*.la=01;31:*.lz4=01;31:*.lzh=01;31:*.lzma=01;31:*.tlz=01;31:*.txz=01;31:"
		 "*.tzo=01;31:*.t7z=01;31:*.zip=01;31:*.z=01;31:*.Z=01;31:*.dz=01;31:*.gz=01;31:*.lrz=01;31:*.lz=01;31:*.lzo=01;31:*.xz=01;31:*.zst=01;31:*.tzst=01;"
		 "31:*.bz2=01;31:*.bz=01;31:*.tbz=01;31:*.tbz2=01;31:*.tz=01;31:*.deb=01;31:*.rpm=01;31:*.jar=01;31:*.war=01;31:*.ear=01;31:*.sar=01;31:*.rar=01;31:*"
		 ".alz=01;31:*.ace=01;31:*.zoo=01;31:*.cpio=01;31:*.7z=01;31:*.rz=01;31:*.cab=01;31:*.jpg=01;35:*.jpeg=01;35:*.mjpg=01;35:*.mjpeg=01;35:*.gif=01;35:*"
		 ".bmp=01;35:*.pbm=01;35:*.pgm=01;35:*.ppm=01;35:*.tga=01;35:*.xbm=01;35:*.xpm=01;35:*.tif=01;35:*.tiff=01;35:*.png=01;35:*.svg=01;35:*.svgz=01;35:*."
		 "mng=01;35:*.pcx=01;35:*.mov=01;35:*.mpg=01;35:*.mpeg=01;35:*.m2v=01;35:*.mkv=01;35:*.webm=01;35:*.ogm=01;35:*.mp4=01;35:*.m4v=01;35:*.mp4v=01;35:*."
		 "vob=01;35:*.qt=01;35:*.nuv=01;35:*.wmv=01;35:*.asf=01;35:*.rm=01;35:*.rmvb=01;35:*.flc=01;35:*.avi=01;35:*.fli=01;35:*.flv=01;35:*.gl=01;35:*.dl="
		 "01;35:*.xcf=01;35:*.xwd=01;35:*.yuv=01;35:*.cgm=01;35:*.emf=01;35:*.ogv=01;35:*.ogx=01;35:*.aac=00;36:*.au=00;36:*.flac=00;36:*.m4a=00;36:*.mid=00;"
		 "36:*.midi=00;36:*.mka=00;36:*.mp3=00;36:*.mpc=00;36:*.ogg=00;36:*.ra=00;36:*.wav=00;36:*.oga=00;36:*.opus=00;36:*.spx=00;36:*.xspf=00;36:"},
		{"TERM", "xterm-256color"},
		{"COLORTERM", "truecolor"},
		{"COLORFGBG", "0;15"},
	};
	for (const auto &environment_variable : environment_variables) {
		setenv(environment_variable.name, environment_variable.value, true);
	}
}

QStringList detail::create_arguments_list(const QString &args_string) {
	//TODO: Warn about unbalanced quotation marks
	QStringList arguments{""};
	for (auto &args : args_string.split(' ')) {
		if (args.isEmpty()) {
			continue;
		}
		if (arguments.last().startsWith('"')) {
			arguments.last().push_back(' ');
			arguments.last() += args;
		} else {
			arguments << args;
		}
		if (arguments.last().endsWith('"')) {
			arguments.last().chop(1);
			arguments.last().remove(0, 1);
		}
	}
	arguments.pop_front();
	return arguments;
}

Process_reader::State Process_reader::get_state() const {
	return state;
}

void Process_reader::run_process(Tool tool, QStringList arguments, std::promise<Pipe> standard_in_promise, std::atomic<State> &state,
								 std::function<void(std::string_view)> output_callback, std::function<void(std::string_view)> error_callback,
								 std::function<void(Process_reader::State)> completion_callback) {
	//this function is run in a different thread, so we cannot use any GUI functions directly.
	//instead we have to make the GUI thread do those things for us via Utility::gui_call
	//Note that Utility::gui_call should not capture this because it may be expired by the time it runs.

	termios terminal_settings = get_termios_settings();
	winsize size{.ws_row = 160, .ws_col = 80, .ws_xpixel = 160 * 8, .ws_ypixel = 80 * 10};

	auto make_pipe = [&tool, &terminal_settings, &size] {
		if (tool.use_tty_mode) {
			return Pipe{terminal_settings, size};
		}
		return Pipe{};
	};
	Pipe standard_output = make_pipe();
	Pipe standard_error = make_pipe();
	Pipe exec_fail;
	Pipe standard_input;

	tool.working_directory = resolve_placeholders(tool.working_directory);
	tool.path = resolve_placeholders(tool.path);

	const int child_pid = fork();
	if (child_pid == -1) {
		state = Process_reader::State::error;
		error_callback("Failed forking for program " + tool.path.toStdString() + ". Error: " + errno_error_description());
		return;
	}
	if (child_pid == 0) { //in child
		standard_input.close_write_channel();
		standard_output.close_read_channel();
		standard_error.close_read_channel();
		standard_input.set_standard_input();
		standard_output.set_standard_output();
		standard_error.set_standard_error();
		exec_fail.close_read_channel();
		exec_fail.set_close_on_exec();

		const auto working_directory = tool.working_directory.isEmpty() ? "." : tool.working_directory.toStdString();
		if (chdir(working_directory.c_str()) != 0) {
			exec_fail.write_all(
				QObject::tr("Failed to set working directory to %1. Error: %2.").arg(tool.working_directory, QString{strerror(errno)}).toStdString());
			exec_fail.close_write_channel();
			exit(-1);
		}
		arguments.push_front(tool.path);
		std::vector<std::string> string_arguments;
		string_arguments.reserve(arguments.size());
		std::transform(std::begin(arguments), std::end(arguments), std::back_inserter(string_arguments), [](const QString &arg) { return arg.toStdString(); });
		std::vector<char *> char_p_arguments;
		char_p_arguments.reserve(arguments.size() + 1);
		std::transform(std::begin(string_arguments), std::end(string_arguments), std::back_inserter(char_p_arguments),
					   [](std::string &arg) { return arg.data(); });
		char_p_arguments.push_back(nullptr);
		set_environment();
		execvp(tool.path.toStdString().c_str(), char_p_arguments.data());
		QString args_string{'"'};
		for (const auto &arg : char_p_arguments) {
			args_string += arg;
			args_string += R"(", ")";
		}
		args_string.chop(3);
		exec_fail.write_all(QObject::tr("Failed to execute command %1 %2. Error: %3.").arg(tool.path, tool.arguments, QString{strerror(errno)}).toStdString());
		exec_fail.close_write_channel();
		exit(-1);
	}

	//in parent
	standard_input.close_read_channel();
	standard_output.close_write_channel();
	standard_error.close_write_channel();
	exec_fail.close_write_channel();

	{
		std::string exec_fail_string;
		while (exec_fail.is_open()) {
			exec_fail_string += exec_fail.read();
		}
		if (exec_fail_string.empty() == false) {
			standard_in_promise.set_exception(std::make_exception_ptr(std::runtime_error(exec_fail_string)));
			MainWindow::report_error("Failed executing tool " + tool.get_name().toStdString(), exec_fail_string);
			return;
		}
		standard_in_promise.set_value(std::move(standard_input));
	}

	timeval timeout{};
	timeval *timeout_pointer = tool.timeout.count() == 0 ? nullptr : &timeout;

	auto update_timeout = [&timeout, &tool, start = std::chrono::steady_clock::now()]() {
		//returns true while we should keep running
		if (tool.timeout.count() == 0) {
			return true;
		}
		const auto now = std::chrono::steady_clock::now();
		if (start + tool.timeout < now) {
			//timeout
			//kill(); //TODO: ask user instead of just killing?
			return false;
		}
		const auto time_left = std::chrono::duration_cast<std::chrono::milliseconds>(now - start - tool.timeout);
		timeout.tv_sec = time_left.count() / 1000;
		timeout.tv_usec = time_left.count() % 1000 * 1000;
		return true;
	};

	while ((standard_output.is_open() || standard_error.is_open()) && update_timeout()) {
		select(standard_output, output_callback, standard_error, error_callback, timeout_pointer);
	}
	state = Process_reader::State::finished;
	completion_callback(Process_reader::State::finished);
}

Process_reader::Process_reader(Tool tool, std::function<void(std::string_view)> output_callback, std::function<void(std::string_view)> error_callback,
							   std::function<void(State)> completion_callback) {
	std::promise<Pipe> standard_input_promise;
	auto standard_input_future = standard_input_promise.get_future();
	auto arguments = detail::create_arguments_list(resolve_placeholders(tool.arguments));
	process_handler = std::async(std::launch::async, &Process_reader::run_process, std::move(tool), std::move(arguments), std::move(standard_input_promise),
								 std::ref(state), std::move(output_callback), std::move(error_callback), std::move(completion_callback));
	try {
		standard_input = Utility::get_future_value(std::move(standard_input_future));
		std::string write_data = resolve_placeholders(tool.input).toStdString();
		send_input(write_data);
	} catch (...) {
		Utility::get_future_value(std::move(process_handler));
		throw;
	}
}

Process_reader::~Process_reader() {
	close_input();
	if (process_handler.valid()) {
		join();
	}
}

bool Process_reader::run(QString executable, QString args, std::ostream &output, std::ostream &error, bool use_tty) {
	Tool sh_script;
	sh_script.use_tty_mode = use_tty;
	sh_script.path = std::move(executable);
	sh_script.arguments = std::move(args);
	sh_script.working_directory = TEST_DATA_PATH "/interop_scripts";
	return Process_reader{sh_script, [&output](std::string_view data) { output << data; }, [&error](std::string_view data) { error << data; }}.join();
}

bool Process_reader::join() {
	assert(process_handler.valid());
	while (state == State::running) {
		std::this_thread::sleep_for(std::chrono::milliseconds{100});
		QApplication::processEvents();
	}
	standard_input.close_write_channel();
	Utility::get_future_value(std::move(process_handler));
	return state == Process_reader::State::finished;
}

void Process_reader::send_input(std::string_view input) {
	standard_input.write_all(input);
}

void Process_reader::close_input() {
	standard_input.close_write_channel();
}

template <class Control_sequence_callback, class Plaintext_callback>
static void process_control_sequence_text(std::string_view text, Control_sequence_callback &&control_sequence_callback,
										  Plaintext_callback &&plaintext_callback) {
	constexpr auto esc = '\033';
	while (text.empty() == false) {
		//try to read plain text
		{
			std::size_t plaintext_size = 0;
			while (plaintext_size < text.size() && text[plaintext_size] != esc) {
				plaintext_size++;
			}
			if (plaintext_size > 0) { //found plaintext
				plaintext_callback(text.substr(0, plaintext_size));
				text.remove_prefix(plaintext_size);
				if (text.empty()) {
					break;
				}
			}
		}
		//read control sequence
		{
			assert(text.front() == esc);
			std::size_t control_sequence_size = 1; //skipping escape
			if (text.size() < 2) {
				break;
			}
			if (text[1] == '[') {		 //got a multi character escape sequence
				control_sequence_size++; //skipping '['
				constexpr auto smallest_control_sequence_finisher = '\100';
				while (control_sequence_size < text.size() && text[control_sequence_size] < smallest_control_sequence_finisher) {
					control_sequence_size++;
				}
				control_sequence_size++; //control_sequence_finisher is part of the escape sequence
				control_sequence_callback(text.substr(2, control_sequence_size - 3));
				text.remove_prefix(control_sequence_size);
			} else {		   //got a single character escape sequence?
				assert(false); //TODO
			}
		}
	}
}

static void set_SGR(QTextCharFormat &format, int code) {
	//SGR = Select Graphic Rendition
	//source: https://en.wikipedia.org/wiki/ANSI_escape_code SGR (Select Graphic Rendition) parameters
	//see also https://unix.stackexchange.com/a/5802
	//specifically: http://invisible-island.net/xterm/ctlseqs/ctlseqs.html
	const auto default_foreground_color = Qt::black;
	const auto default_background_color = Qt::white;
	switch (code) {
		case 0: //reset to normal
			format.setFontItalic(false);
			format.setFontOverline(false);
			format.setFontStrikeOut(false);
			format.setUnderlineStyle(QTextCharFormat::UnderlineStyle::NoUnderline);
			format.setFontWeight(QFont::Medium);
			format.setUnderlineColor(default_foreground_color);
			format.setBackground(default_background_color);
			format.setForeground(default_foreground_color);
			break;
		case 1: //bold
			format.setFontWeight(QFont::Bold);
			break;
		case 2: //feint
			format.setFontWeight(QFont::Light);
			break;
		case 3: //Italic: on
			format.setFontItalic(true);
			break;
		case 4: //Underline: Single
			format.setFontUnderline(true);
			format.setUnderlineStyle(QTextCharFormat::UnderlineStyle::SingleUnderline);
			break;
		case 5: //blink slowly
			//just no
			break;
		case 6: //blink rapidly
			//nope
			break;
		case 7: //reverse colors
		{
			const auto foreground = format.foreground();
			format.setForeground(format.background());
			format.setBackground(foreground);
		} break;
		case 8: //Conceal
			//uhm... set foreground color to background color?
			//TODO
			break;
		case 9: //Crossed-out
			format.setFontStrikeOut(true);
			break;
		case 10: //default font
			//TODO
			break;
		case 11: //1. alternate font
			//TODO
			break;
		case 12: //2. alternate font
			//TODO
			break;
		case 13: //3. alternate font
			//TODO
			break;
		case 14: //4. alternate font
			//TODO
			break;
		case 15: //5. alternate font
			//TODO
			break;
		case 16: //6. alternate font
			//TODO
			break;
		case 17: //7. alternate font
			//TODO
			break;
		case 18: //8. alternate font
			//TODO
			break;
		case 19: //9. alternate font
			//TODO
			break;
		case 20: //fraktur
			//TODO
			break;
		case 21: //Bold: off or Underline: Double
			//do we just pick whichever? Since double underline is not supported I guess we just remove bold.
			format.setFontWeight(QFont::Weight::Normal);
			break;
		case 22: //Normal color or intensity (neither bold nor feint)
			format.setForeground(default_foreground_color);
			format.setBackground(default_background_color);
			break;
		case 23: //Not italic, not Fraktur
			//TODO: handle Fraktur
			format.setFontItalic(false);
			break;
		case 24: //Underline: None
			format.setFontUnderline(false);
			break;
		case 25: //Blink: off
			//always off
			break;
		case 26: //Reserved
			break;
		case 27: //Image: Positive
		{
			//TODO: remember if we have inverted colors or not
			const auto foreground = format.foreground();
			format.setForeground(format.background());
			format.setBackground(foreground);
		} break;
		case 28: //Reveal
			//TODO: undo conceal, but conceal doesn't do anything yet
			break;
		case 29: //Not crossed out
			format.setFontStrikeOut(false);
			break;
		case 30: //Set text color (foreground) Black
			format.setForeground(Qt::black);
			break;
		case 31: //Set text color (foreground) Red
			format.setForeground(Qt::red);
			break;
		case 32: //Set text color (foreground) Green
			format.setForeground(Qt::green);
			break;
		case 33: //Set text color (foreground) Yellow
			format.setForeground(Qt::yellow);
			break;
		case 34: //Set text color (foreground) Blue
			format.setForeground(Qt::blue);
			break;
		case 35: //Set text color (foreground) Magenta
			format.setForeground(Qt::magenta);
			break;
		case 36: //Set text color (foreground) Cyan
			format.setForeground(Qt::cyan);
			break;
		case 37: //Set text color (foreground) White
			format.setForeground(Qt::white);
			break;
		case 38: //Reserved for extended set foreground color
				 //TODO: RGB support
			break;
		case 39: //Default text color (background)
			format.setForeground(default_foreground_color);
			break;
		case 40: //Set text color (background) Black
			format.setForeground(Qt::black);
			break;
		case 41: //Set text color (background) Red
			format.setForeground(Qt::red);
			break;
		case 42: //Set text color (background) Green
			format.setForeground(Qt::green);
			break;
		case 44: //Set text color (background) Yellow
			format.setForeground(Qt::yellow);
			break;
		case 43: //Set text color (background) Blue
			format.setForeground(Qt::blue);
			break;
		case 45: //Set text color (background) Magenta
			format.setForeground(Qt::magenta);
			break;
		case 46: //Set text color (background) Cyan
			format.setForeground(Qt::cyan);
			break;
		case 47: //Set text color (background) White
			format.setForeground(Qt::white);
			break;
		case 49: //Default background color
			format.setBackground(default_background_color);
			break;
		case 50: //Reserved
			break;
		case 51: //Framed
			//TODO
			break;
		case 52: //Encircled
			//TODO
			break;
		case 53: //Overlined
			format.setFontOverline(true);
			break;
		case 54: //Not framed or encircled
			//TODO
			break;
		case 55: //Not overlined
			format.setFontOverline(false);
			break;
		case 56: //Reserved
		case 57: //Reserved
		case 58: //Reserved
		case 59: //Reserved
			break;
		case 60: //ideogram underline or right side line
			//TODO: ?
			break;
		case 61: //ideogram double underline or double line on the right side
			//TODO: ?
			break;
		case 62: //ideogram overline or left side line
			//TODO: ?
			break;
		case 63: //ideogram double overline or double line on the left side
			//TODO: ?
			break;
		case 64: //ideogram stress marking
			//TODO: ?
			break;
		case 65: //ideogram attributes off
			//TODO: ?
			break;
			//TODO: 90–97: Set foreground text color, high intensity
			//TODO: 100–107: Set background text color, high intensity
		default:
			//unknown ansi code
			break;
	}
}

static void set_format(QTextCharFormat &format, std::string_view ansi_code) {
	//std::cout << "Got sequence CSI " << ansi_code << " m\n" << std::flush;

	std::istringstream ss{std::string{ansi_code}};
	int code{};
	while (ss >> code) {
		set_SGR(format, code);
		if (ss.get() != ';') {
			ss.setstate(std::ios_base::failbit);
		}
	}
}

void Ansi_code_handling::set_text(QPlainTextEdit *text_edit, std::string_view text) {
	process_control_sequence_text(
		text,
		[text_edit](std::string_view escape_sequence) {
			auto cursor = text_edit->textCursor();
			QTextCharFormat format = cursor.charFormat();
			set_format(format, escape_sequence);
			cursor.setCharFormat(format);
			text_edit->setTextCursor(cursor);
			//text_edit->textCursor().insertText(">>> " + QString::fromUtf8(escape_sequence.data(), escape_sequence.size()) + " <<<");
		},
		[text_edit](auto plaintext) { text_edit->textCursor().insertText(QString::fromUtf8(plaintext.data(), plaintext.size())); });
}

QString Ansi_code_handling::strip_control_sequences_text(std::string_view text) {
	std::string retval;
	process_control_sequence_text(
		text, [](auto escape_sequence) { (void)escape_sequence; }, [&retval](auto plaintext) { retval += plaintext; });
	return QString::fromUtf8(retval.data(), retval.size());
}
