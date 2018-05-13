#ifndef PIPE_H
#define PIPE_H

#include "unique_handle.h"

#include <string>
#include <string_view>

struct termios;
struct winsize;

struct Pipe {
	Pipe();
	Pipe(const termios &terminal_settings, const winsize &window_size);

	void close_read_channel();
	void close_write_channel();
	bool is_open() const;

	void write(std::string_view &s);

	void write_all(std::string_view s);

	void set_close_on_exec();

	std::string read();

	void set_standard_input();
	void set_standard_output();
	void set_standard_error();

	int get_read_channel();
	int get_write_channel();

	private:
	constexpr static auto chunk_size = 1024;

	struct File_descriptor_policy {
		using Handle_type = int;

		constexpr static auto invalid_file_descriptor = -1;

		constexpr static Handle_type get_null();

		constexpr static bool is_null(int file_descriptor);

		static void close(int file_descriptor);
	};

	Utility::Unique_handle<File_descriptor_policy> read_channel;
	Utility::Unique_handle<File_descriptor_policy> write_channel;
};

constexpr Pipe::File_descriptor_policy::Handle_type Pipe::File_descriptor_policy::get_null() {
	return invalid_file_descriptor;
}

constexpr bool Pipe::File_descriptor_policy::is_null(int file_descriptor) {
	return file_descriptor == invalid_file_descriptor;
}

#endif