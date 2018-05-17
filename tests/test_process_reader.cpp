#include "test_process_reader.h"
#include "logic/process_reader.h"
#include "logic/settings.h"
#include "test.h"

#include <QProcess>
#include <QString>
#include <QStringList>
#include <fstream>

static void test_args_construction() {
	struct Test_case {
		QString arg_text;
		QStringList args;
	} test_cases[] = {
		{"", {}},
		{"test", {"test"}},
		{R"("test")", {"test"}},
		{"multi arg", {"multi", "arg"}},
		{"many multi arg", {"many", "multi", "arg"}},
		{R"("multi arg")", {"multi arg"}},
		{R"(many "multi arg" things)", {"many", "multi arg", "things"}},
		{R"("many multi" "arg things")", {"many multi", "arg things"}},
	};

	for (const auto &test_case : test_cases) {
		assert_equal(detail::create_arguments_list(test_case.arg_text), test_case.args);
	}
}

static std::string strip_carriage_return(std::string_view sv) {
	std::string retval;
	retval.reserve(sv.size());
	std::copy_if(std::begin(sv), std::end(sv), std::back_inserter(retval), [](char c) { return c != '\r'; });
	return retval;
}

static void assert_executed_correctly(std::string_view code, std::string_view expected_output, std::string_view expected_error = {},
									  std::string_view input = {}) {
	const auto cpp_file = "/tmp/SCE_test_process_code.cpp";
	const auto exe_file = "/tmp/SCE_test_process_exe";
	assert_true(std::ofstream{cpp_file} << code);
	assert_equal(QProcess::execute("g++", {"-std=c++17", cpp_file, "-o", exe_file}), 0);
	Tool tool{};
	tool.path = exe_file;
	tool.working_directory = "/tmp";
	std::string output;
	std::string error;
	Process_reader p{tool, [&output](std::string_view sv) { output += sv; }, [&error](std::string_view sv) { error += sv; }};
	while (input.size() > 0) {
		const auto input_size = std::min(std::size_t{10}, input.size());
		p.send_input({input.begin(), input_size});
		input.remove_prefix(input_size);
	}
	p.close_input();
	p.join();
	assert_equal(strip_carriage_return(output), expected_output);
	assert_equal(strip_carriage_return(error), expected_error);
}

static void test_process_reading() {
	struct Test_case {
		std::string_view code;
		std::string_view expected_output;
		std::string_view expected_error;
	} test_cases[] = {
		{.code = R"(int main(){})", //
		 .expected_output = "",
		 .expected_error = ""},
		{.code = R"(#include <iostream>
				 int main(){
					std::cout << "test";
				 })",
		 .expected_output = "test",
		 .expected_error = ""},
		{.code = R"(#include <iostream>
				 int main(){
					std::cerr << "test";
				 })",
		 .expected_output = "",
		 .expected_error = "test"},
		{.code = R"(#include <iostream>
				 int main(){
					 std::cout << "multi\nline\ntest\noutput\n";
					 std::cerr << "multi\nline\ntest\nerror\n";
				 })",
		 .expected_output = "multi\nline\ntest\noutput\n",
		 .expected_error = "multi\nline\ntest\nerror\n"},
		{.code = R"(#include <iostream>
				 #include <thread>
				 #include <chrono>

				 int main(){
					 std::cout << "Hello" << std::flush;
					 std::this_thread::sleep_for(std::chrono::milliseconds{500});
					 std::cout << "World";
				 })",
		 .expected_output = "HelloWorld",
		 .expected_error = ""},
	};

	for (auto &test_case : test_cases) {
		assert_executed_correctly(test_case.code, test_case.expected_output, test_case.expected_error);
	}
}

static void test_is_tty() { //we need to pretend to be a tty to receive color information
	const auto code = R"(
#include <iostream>

#if __linux
#include <unistd.h>
int main() {
	std::cout << isatty(STDOUT_FILENO) << isatty(STDERR_FILENO) << '\n';
}
#else
int main() {
	std::cout << "00\n";
}
#endif
)";
	const auto expected_output = using_tty ? "11\n" : "00\n";
	assert_executed_correctly(code, expected_output);
}

static void test_is_character_device() {
	const auto code = R"(
#include <iostream>

#if __linux
#include <sys/stat.h>
#include <unistd.h>
int main() {
	struct stat stdout_status {};
	fstat(STDOUT_FILENO, &stdout_status);
	std::cout << S_ISCHR(stdout_status.st_mode) << '\n';
}
#else
int main() {
	std::cout << "0\n";
}
#endif
)";
	const auto expected_output = using_tty ? "1\n" : "0\n";
	assert_executed_correctly(code, expected_output);
}

static void test_input() {
	const auto code = R"(
#include <iostream>
#include <string>

int main() {
	std::string s;
	while (std::getline(std::cin, s)) {
		std::cout << s << '\n';
	}
}
)";
	const auto text = R"(This
					  is
					  a
					  test.
)";
	assert_executed_correctly(code, text, "", text);
}

void test_process_reader() {
	test_args_construction();
	test_process_reading();
	test_is_tty();
	test_is_character_device();
	test_input();
	std::cout << "Using tty: " << (using_tty ? "true" : "false") << '\n';
}
