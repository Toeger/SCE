#include "process_reader.h"
#include "ui/edit_window.h"
#include "ui/mainwindow.h"

#include <QProcess>
#include <cassert>
#include <initializer_list>
#include <iostream>

#ifdef __linux
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

struct Pipe {
	Pipe() {
		if (pipe(file_descriptors) != 0) {
			throw std::runtime_error("Failed creating pipe");
		}
	}
	Pipe(Pipe &&other)
		: file_descriptors{invalid_file_descriptor, invalid_file_descriptor} {
		std::swap(file_descriptors, other.file_descriptors);
	}
	Pipe(const Pipe &other) = delete;
	~Pipe() {
		close_read_channel();
		close_write_channel();
	}
	Pipe &operator=(Pipe &&other) {
		std::swap(file_descriptors, other.file_descriptors);
		return *this;
	}
	Pipe &operator=(const Pipe &other) = delete;

	void close_read_channel() {
		close(read_channel());
	}
	void close_write_channel() {
		close(write_channel());
	}
	bool is_open() const {
		return read_channel() != invalid_file_descriptor || write_channel() != invalid_file_descriptor;
	}

	void write(std::string_view &s) {
		assert(write_channel() != invalid_file_descriptor);
		const auto written = ::write(write_channel(), s.data(), s.size());
		if (written == -1) {
			close_write_channel();
			return;
		}
		s.remove_prefix(written);
	}
	std::string read() {
		char buffer[chunk_size];
		const auto bytes_read = ::read(read_channel(), buffer, chunk_size);
		if (bytes_read <= 0) {
			close_read_channel();
		}
		return {buffer, buffer + bytes_read};
	}

	void set_standard_input() {
		if (dup2(read_channel(), STDIN_FILENO) != STDIN_FILENO) {
			throw std::runtime_error("Failed setting standard input");
		}
	}
	void set_standard_output() {
		if (dup2(write_channel(), STDOUT_FILENO) != STDOUT_FILENO) {
			throw std::runtime_error("Failed setting standard output");
		}
	}
	void set_standard_error() {
		if (dup2(write_channel(), STDERR_FILENO) != STDERR_FILENO) {
			throw std::runtime_error("Failed setting standard error");
		}
	}

	int &read_channel() {
		return file_descriptors[0];
	}
	int &write_channel() {
		return file_descriptors[1];
	}
	int read_channel() const {
		return file_descriptors[0];
	}
	int write_channel() const {
		return file_descriptors[1];
	}

	private:
	constexpr static auto chunk_size = 1024;
	constexpr static auto invalid_file_descriptor = -1;
	void close(int &file_descriptor) {
		if (file_descriptor != invalid_file_descriptor) {
			if (::close(file_descriptor) != 0) {
				throw std::runtime_error("Failed to close file descriptor");
			}
			file_descriptor = invalid_file_descriptor;
		}
	}

	int file_descriptors[2];
};

static void select(std::vector<std::pair<Pipe *, std::string_view *>> &write_pipes, std::vector<std::pair<Pipe *, std::string *>> &read_pipes,
				   timeval timeout) {
	fd_set read_file_descriptors{};
	fd_set write_file_descriptors{};
	int max_file_descriptor = 0;
	for (auto it = std::begin(write_pipes); it != std::end(write_pipes);) {
		if (it->first->is_open()) {
			FD_SET(it->first->write_channel(), &write_file_descriptors);
			max_file_descriptor = std::max(max_file_descriptor, it->first->write_channel());
			++it;
		} else {
			it = write_pipes.erase(it);
		}
	}
	for (auto it = std::begin(read_pipes); it != std::end(read_pipes);) {
		if (it->first->is_open()) {
			FD_SET(it->first->read_channel(), &read_file_descriptors);
			max_file_descriptor = std::max(max_file_descriptor, it->first->read_channel());
			++it;
		} else {
			it = read_pipes.erase(it);
		}
	}
	const auto selected = ::select(max_file_descriptor + 1, &read_file_descriptors, &write_file_descriptors, nullptr, &timeout);

	if (selected > 0) {
		for (auto &read_pipe : read_pipes) {
			if (FD_ISSET(read_pipe.first->read_channel(), &read_file_descriptors)) {
				*read_pipe.second += read_pipe.first->read();
			}
		}
		for (auto &write_pipe : write_pipes) {
			if (FD_ISSET(write_pipe.first->write_channel(), &write_file_descriptors)) {
				write_pipe.first->write(*write_pipe.second);
			}
		}
	} else if (selected == 0) { //timeout occured
								//TODO: handle timeout
		//error += QObject::tr("Timeout occured while executing %1").arg(tool.path);
		return;
	} else { //error occured
		//throw std::runtime_error("Select failed");
		return;
	}
}

Process_reader::Process_reader(const Tool &tool) {
#ifdef __linux
	int master;
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

	winsize size{};
	size.ws_row = 160;
	size.ws_col = 80;
	size.ws_xpixel = 160 * 8;
	size.ws_ypixel = 80 * 10;
	Pipe standard_input;
	Pipe standard_output;
	Pipe standard_error;
	int child = forkpty(&master, nullptr, &terminal_settings, &size);
	if (child == -1) {
		error = QObject::tr("Executing program %1 failed with error code %2.").arg(tool.path, QString::number(errno));
	}
	if (child == 0) {
		standard_input.close_write_channel();
		standard_output.close_read_channel();
		standard_error.close_read_channel();
		standard_input.set_standard_input();
		standard_output.set_standard_output();
		standard_error.set_standard_error();

		chdir(tool.working_directory.toStdString().c_str());
		auto qlist_arguments = create_arguments_list(resolve_placeholders(tool.arguments));
		qlist_arguments.push_front(tool.path);
		std::vector<std::string> string_arguments;
		string_arguments.reserve(qlist_arguments.size());
		std::transform(std::begin(qlist_arguments), std::end(qlist_arguments), std::back_inserter(string_arguments),
					   [](const QString &arg) { return arg.toStdString(); });
		std::vector<char *> char_p_arguments;
		char_p_arguments.reserve(qlist_arguments.size());
		std::transform(std::begin(string_arguments), std::end(string_arguments), std::back_inserter(char_p_arguments),
					   [](std::string &arg) { return arg.data(); });
		execvp(tool.path.toStdString().c_str(), char_p_arguments.data());
		perror("failed to execute command");
		exit(0);
	}
	standard_input.close_read_channel();
	standard_output.close_write_channel();
	standard_error.close_write_channel();

	std::string write_data = resolve_placeholders(tool.input).toStdString();
	std::string_view write_data_view = write_data;
	timeval timeout{};
	timeout.tv_sec = 3; //TODO: use timeout from Tool
	timeout.tv_usec = 0;
	std::vector<std::pair<Pipe *, std::string_view *>> write_pipes = {{&standard_input, &write_data_view}};
	std::string output_data;
	std::string error_data;
	std::vector<std::pair<Pipe *, std::string *>> read_pipes = {{&standard_output, &output_data}, {&standard_error, &error_data}};
	while (standard_input.is_open() || standard_output.is_open() || standard_error.is_open()) {
		if (write_data_view.empty()) {
			standard_input.close_write_channel();
		}
		select(write_pipes, read_pipes, timeout);
	}
	output += QString::fromUtf8(output_data.data(), output_data.size());
	error += QString::fromUtf8(error_data.data(), error_data.size());
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

const QString &Process_reader::get_output() const {
	return output;
}

const QString &Process_reader::get_error() const {
	return error;
}
