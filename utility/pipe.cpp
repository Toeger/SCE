#include "pipe.h"
#include "error.h"

#include <array>
#include <cassert>
#include <cstring>
#include <fcntl.h>
#include <pty.h>
#include <unistd.h>

[[noreturn]] static void throw_error(const std::string &message) {
	//lots of unnecessary string copy and allocation going on in this function, but it is error reporting which doesn't happen frequently
	auto error_message = message + ": " + errno_error_description();
	throw std::runtime_error(std::move(error_message));
}

Pipe::Pipe() {
	std::array<int, 2> file_descriptors;
	if (pipe(file_descriptors.data()) != 0) {
		throw_error("Failed creating pipe");
	}
	read_channel = file_descriptors[0];
	write_channel = file_descriptors[1];
}

Pipe::Pipe(const termios &terminal_settings, const winsize &window_size) {
	std::array<int, 2> file_descriptors;
	if (openpty(&file_descriptors[0], &file_descriptors[1], nullptr, &terminal_settings, &window_size) != 0) {
		throw_error("Failed opening pty pipe");
	}
	read_channel = file_descriptors[0];
	write_channel = file_descriptors[1];
}

void Pipe::close_read_channel() {
	read_channel.reset();
}

void Pipe::close_write_channel() {
	write_channel.reset();
}

bool Pipe::is_open() const {
	return read_channel || write_channel;
}

void Pipe::write(std::string_view &s) {
	assert(write_channel);
	const auto written = ::write(write_channel.get(), s.data(), s.size());
	if (written == -1) {
		close_write_channel();
		return;
	}
	s.remove_prefix(written);
}

void Pipe::write_all(std::string_view s) {
	while (s.size()) {
		if (is_open() == false) {
			throw_error("Failed writing to pipe");
		}
		write(s);
	}
}

void Pipe::set_close_on_exec() {
	for (auto &channel : {&read_channel, &write_channel}) {
		if (*channel) {
			fcntl(channel->get(), F_SETFD, FD_CLOEXEC);
		}
	}
}

std::string Pipe::read() {
	char buffer[chunk_size];
	const auto bytes_read = ::read(read_channel.get(), buffer, chunk_size);
	if (bytes_read <= 0) {
		close_read_channel();
		return {};
	}
	return {buffer, buffer + bytes_read};
}

void Pipe::set_standard_input() {
	if (dup2(read_channel.get(), STDIN_FILENO) != STDIN_FILENO) {
		throw_error("Failed setting standard input");
	}
}

void Pipe::set_standard_output() {
	if (dup2(write_channel.get(), STDOUT_FILENO) != STDOUT_FILENO) {
		throw_error("Failed setting standard output");
	}
}

void Pipe::set_standard_error() {
	if (dup2(write_channel.get(), STDERR_FILENO) != STDERR_FILENO) {
		throw_error("Failed setting standard error");
	}
}

int Pipe::get_read_channel() {
	return read_channel.get();
}

int Pipe::get_write_channel() {
	return write_channel.get();
}

void Pipe::File_descriptor_policy::close(int file_descriptor) {
	if (::close(file_descriptor) != 0) {
		throw_error("Failed to close file descriptor");
	}
}
