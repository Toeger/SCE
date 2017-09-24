#include "process_reader.h"
#include "ui/edit_window.h"
#include "ui/mainwindow.h"

#include <QPlainTextEdit>
#include <QProcess>
#include <cassert>
#include <initializer_list>

#ifdef __linux
#include "utility/unique_handle.hpp"
#include <pty.h>
#include <unistd.h>
#endif

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

struct File_descriptor_policy {
	using Handle_type = int;

	constexpr static auto invalid_file_descriptor = -1;

	constexpr static Handle_type get_null() {
		return invalid_file_descriptor;
	}

	constexpr static bool is_null(int file_descriptor) {
		return file_descriptor == invalid_file_descriptor;
	}

	static void close(int file_descriptor) {
		if (::close(file_descriptor) != 0) {
			throw std::runtime_error("Failed to close file descriptor");
		}
	}
};

struct Pipe {
	Pipe() {
		std::array<int, 2> file_descriptors;
		if (pipe(file_descriptors.data()) != 0) {
			throw std::runtime_error("Failed creating pipe");
		}
		read_channel = file_descriptors[0];
		write_channel = file_descriptors[1];
	}

	void close_read_channel() {
		read_channel.reset();
	}

	void close_write_channel() {
		write_channel.reset();
	}
	bool is_open() const {
		return read_channel || write_channel;
	}

	void write(std::string_view &s) {
		assert(write_channel);
		const auto written = ::write(write_channel.get(), s.data(), s.size());
		if (written == -1) {
			close_write_channel();
			return;
		}
		s.remove_prefix(written);
	}
	std::string read() {
		char buffer[chunk_size];
		const auto bytes_read = ::read(read_channel.get(), buffer, chunk_size);
		if (bytes_read <= 0) {
			close_read_channel();
		}
		return {buffer, buffer + bytes_read};
	}

	void set_standard_input() {
		if (dup2(read_channel.get(), STDIN_FILENO) != STDIN_FILENO) {
			throw std::runtime_error("Failed setting standard input");
		}
	}
	void set_standard_output() {
		if (dup2(write_channel.get(), STDOUT_FILENO) != STDOUT_FILENO) {
			throw std::runtime_error("Failed setting standard output");
		}
	}
	void set_standard_error() {
		if (dup2(write_channel.get(), STDERR_FILENO) != STDERR_FILENO) {
			throw std::runtime_error("Failed setting standard error");
		}
	}

	int get_read_channel() {
		return read_channel.get();
	}
	int get_write_channel() {
		return write_channel.get();
	}

	private:
	constexpr static auto chunk_size = 1024;

	Utility::Unique_handle<File_descriptor_policy> read_channel;
	Utility::Unique_handle<File_descriptor_policy> write_channel;
};

static void select(std::vector<std::pair<Pipe *, std::string_view *>> &write_pipes, std::vector<std::pair<Pipe *, std::string *>> &read_pipes,
				   timeval timeout) {
	fd_set read_file_descriptors{};
	fd_set write_file_descriptors{};
	int max_file_descriptor = 0;
	for (auto it = std::begin(write_pipes); it != std::end(write_pipes);) {
		if (it->first->is_open()) {
			FD_SET(it->first->get_write_channel(), &write_file_descriptors);
			max_file_descriptor = std::max(max_file_descriptor, it->first->get_write_channel());
			++it;
		} else {
			it = write_pipes.erase(it);
		}
	}
	for (auto it = std::begin(read_pipes); it != std::end(read_pipes);) {
		if (it->first->is_open()) {
			FD_SET(it->first->get_read_channel(), &read_file_descriptors);
			max_file_descriptor = std::max(max_file_descriptor, it->first->get_read_channel());
			++it;
		} else {
			it = read_pipes.erase(it);
		}
	}
	const auto selected = ::select(max_file_descriptor + 1, &read_file_descriptors, &write_file_descriptors, nullptr, &timeout);

	if (selected > 0) {
		for (auto &read_pipe : read_pipes) {
			if (FD_ISSET(read_pipe.first->get_read_channel(), &read_file_descriptors)) {
				*read_pipe.second += read_pipe.first->read();
			}
		}
		for (auto &write_pipe : write_pipes) {
			if (FD_ISSET(write_pipe.first->get_write_channel(), &write_file_descriptors)) {
				write_pipe.first->write(*write_pipe.second);
			}
		}
	} else if (selected == 0) { //timeout occured
								//TODO: handle timeout properly
		for (auto &read_pipe : read_pipes) {
			read_pipe.first->close_read_channel();
		}
		for (auto &write_pipe : write_pipes) {
			write_pipe.first->close_write_channel();
		}
		return;
	} else { //error occured
		throw std::runtime_error("Select failed");
		return;
	}
}

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
	setenv("LS_COLORS",
		   "rs=0:di=01;34:ln=01;36:mh=00:pi=40;33:so=01;35:do=01;35:bd=40;33;01:cd=40;33;01:or=40;31;01:mi=00:su=37;41:sg=30;43:ca=30;41:tw=30;42:ow=34;42:st="
		   "37;44:ex=01;32:*.tar=01;31:*.tgz=01;31:*.arc=01;31:*.arj=01;31:*.taz=01;31:*.la=01;31:*.lz4=01;31:*.lzh=01;31:*.lzma=01;31:*.tlz=01;31:*.txz=01;31:"
		   "*.tzo=01;31:*.t7z=01;31:*.zip=01;31:*.z=01;31:*.Z=01;31:*.dz=01;31:*.gz=01;31:*.lrz=01;31:*.lz=01;31:*.lzo=01;31:*.xz=01;31:*.zst=01;31:*.tzst=01;"
		   "31:*.bz2=01;31:*.bz=01;31:*.tbz=01;31:*.tbz2=01;31:*.tz=01;31:*.deb=01;31:*.rpm=01;31:*.jar=01;31:*.war=01;31:*.ear=01;31:*.sar=01;31:*.rar=01;31:*"
		   ".alz=01;31:*.ace=01;31:*.zoo=01;31:*.cpio=01;31:*.7z=01;31:*.rz=01;31:*.cab=01;31:*.jpg=01;35:*.jpeg=01;35:*.mjpg=01;35:*.mjpeg=01;35:*.gif=01;35:*"
		   ".bmp=01;35:*.pbm=01;35:*.pgm=01;35:*.ppm=01;35:*.tga=01;35:*.xbm=01;35:*.xpm=01;35:*.tif=01;35:*.tiff=01;35:*.png=01;35:*.svg=01;35:*.svgz=01;35:*."
		   "mng=01;35:*.pcx=01;35:*.mov=01;35:*.mpg=01;35:*.mpeg=01;35:*.m2v=01;35:*.mkv=01;35:*.webm=01;35:*.ogm=01;35:*.mp4=01;35:*.m4v=01;35:*.mp4v=01;35:*."
		   "vob=01;35:*.qt=01;35:*.nuv=01;35:*.wmv=01;35:*.asf=01;35:*.rm=01;35:*.rmvb=01;35:*.flc=01;35:*.avi=01;35:*.fli=01;35:*.flv=01;35:*.gl=01;35:*.dl="
		   "01;35:*.xcf=01;35:*.xwd=01;35:*.yuv=01;35:*.cgm=01;35:*.emf=01;35:*.ogv=01;35:*.ogx=01;35:*.aac=00;36:*.au=00;36:*.flac=00;36:*.m4a=00;36:*.mid=00;"
		   "36:*.midi=00;36:*.mka=00;36:*.mp3=00;36:*.mpc=00;36:*.ogg=00;36:*.ra=00;36:*.wav=00;36:*.oga=00;36:*.opus=00;36:*.spx=00;36:*.xspf=00;36:",
		   true);
	setenv("TERM", "xterm-256color", true);
	setenv("COLORTERM", "truecolor", true);
}

Process_reader::Process_reader(const Tool &tool) {
#ifdef __linux
	termios terminal_settings = get_termios_settings();
	winsize size{.ws_row = 160, .ws_col = 80, .ws_xpixel = 160 * 8, .ws_ypixel = 80 * 10};

	Pipe standard_input;
	Pipe standard_output;
	Pipe standard_error;

	int master;
	int child = forkpty(&master, nullptr, &terminal_settings, &size);
	if (child == -1) {
		error = QObject::tr("Executing program %1 failed with error code %2.").arg(tool.path, QString::number(errno)).toStdString();
	}
	if (child == 0) { //in child
		standard_input.close_write_channel();
		standard_output.close_read_channel();
		standard_error.close_read_channel();
		standard_input.set_standard_input();
		standard_output.set_standard_output();
		standard_error.set_standard_error();

		chdir(tool.working_directory.toStdString().c_str());
		auto qlist_arguments = detail::create_arguments_list(resolve_placeholders(tool.arguments));
		qlist_arguments.push_front(tool.path);
		std::vector<std::string> string_arguments;
		string_arguments.reserve(qlist_arguments.size());
		std::transform(std::begin(qlist_arguments), std::end(qlist_arguments), std::back_inserter(string_arguments),
					   [](const QString &arg) { return arg.toStdString(); });
		std::vector<char *> char_p_arguments;
		char_p_arguments.reserve(qlist_arguments.size() + 1);
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
		perror(QObject::tr("failed to execute command %1 %2, error %3").arg(tool.path, tool.arguments, QString::number(errno)).toStdString().c_str());
		exit(-1);
	}

	//in parent
	standard_input.close_read_channel();
	standard_output.close_write_channel();
	standard_error.close_write_channel();

	std::string write_data = resolve_placeholders(tool.input).toStdString();
	std::string_view write_data_view = write_data;

	timeval timeout{};
	timeout.tv_sec = 3; //TODO: use timeout from Tool

	std::vector<std::pair<Pipe *, std::string_view *>> write_pipes = {{&standard_input, &write_data_view}};
	std::vector<std::pair<Pipe *, std::string *>> read_pipes = {{&standard_output, &output}, {&standard_error, &error}};

	while (standard_input.is_open() || standard_output.is_open() || standard_error.is_open()) {
		if (write_data_view.empty()) {
			standard_input.close_write_channel();
		}
		select(write_pipes, read_pipes, timeout);
	}
#else
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
#endif
}

const std::string &Process_reader::get_output() const {
	return output;
}

const std::string &Process_reader::get_error() const {
	return error;
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
			if (text[1] == '[') {        //got a multi character escape sequence
				control_sequence_size++; //skipping '['
				constexpr auto smallest_control_sequence_finisher = '\100';
				while (control_sequence_size < text.size() && text[control_sequence_size] < smallest_control_sequence_finisher) {
					control_sequence_size++;
				}
				control_sequence_size++; //control_sequence_finisher is part of the escape sequence
				control_sequence_callback(text.substr(2, control_sequence_size - 3));
				text.remove_prefix(control_sequence_size);
			} else { //got a single character escape sequence?
				assert(!"TODO");
			}
		}
	}
}

static void set_format(QTextCharFormat &format, std::string_view ansi_code) {
	if (ansi_code.empty()) {
		return;
	}
	const auto default_foreground_color = Qt::black;
	const auto default_background_color = Qt::white;
	//source: https://en.wikipedia.org/wiki/ANSI_escape_code SGR (Select Graphic Rendition) parameters
	switch (ansi_code.front()) {
		case 0: //reset to normal
			format.setFontItalic(false);
			format.setFontOverline(false);
			format.setFontStrikeOut(false);
			format.setFontUnderline(false);
			format.setFontWeight(QFont::Medium);
			format.setUnderlineColor(default_foreground_color);
			format.setBackground(default_background_color);
			format.setForeground(default_foreground_color);
			format.setUnderlineStyle(QTextCharFormat::UnderlineStyle::SingleUnderline);
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
		case 22: //Normal color or intensity
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
		case 48: //Reserved for extended set background color
			/*
			 * I can't find documentation for this. If there is an undefined color used try something like printf '\033[\060\061;\063\071mTest' in the
			 * shell to figure out what color it is
			 */
			if (ansi_code.size() == 1) { //clear color
				format.setForeground(default_foreground_color);
				format.setBackground(default_background_color);
			} else {                                         //set color
				if (ansi_code.substr(1, 3) == "\061;\063") { //I don't know what that sequence means, but it makes colors in zsh
					switch (ansi_code[4]) {
						case 48: //gray
							format.setForeground(Qt::gray);
							break;
						case 49: //red
							format.setForeground(Qt::red);
							break;
						case 50: //green
							format.setForeground(Qt::green);
							break;
						case 51: //yellow
							format.setForeground(Qt::yellow);
							break;
						case 52: //blue
							format.setForeground(Qt::blue);
							break;
						case 53: //magenta
							format.setForeground(Qt::magenta);
							break;
						case 54: //cyan
							format.setForeground(Qt::cyan);
							break;
						case 55: //white
							format.setForeground(Qt::white);
							break;
						case 56: //black
							format.setForeground(Qt::black);
							break;
						default:
							//something else
							assert(!"unsupported color");
							break;
					}
				} else {
					assert(!"unsupported color code");
				}
			}
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

void Process_reader::set_text(QPlainTextEdit *text_edit, std::string_view text) {
	process_control_sequence_text(text,
								  [text_edit](std::string_view escape_sequence) {
									  QTextCharFormat format;
									  set_format(format, escape_sequence);
									  auto cursor = text_edit->textCursor();
									  cursor.setCharFormat(format);
									  text_edit->setTextCursor(cursor);
								  },
								  [text_edit](auto plaintext) { text_edit->textCursor().insertText(QString::fromUtf8(plaintext.data(), plaintext.size())); });
}

QString Process_reader::strip_control_sequences_text(std::string_view text) {
	std::string retval;
	process_control_sequence_text(text, [](auto escape_sequence) { (void)escape_sequence; }, [&retval](auto plaintext) { retval += plaintext; });
	return QString::fromUtf8(retval.data(), retval.size());
}