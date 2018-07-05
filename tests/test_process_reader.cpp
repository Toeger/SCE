#include "logic/process_reader.h"
#include "logic/settings.h"
#include "test.h"
#include "utility/color.h"
#include "utility/utility.h"

#include <QProcess>
#include <QString>
#include <QStringList>
#include <fstream>
#include <sstream>

static constexpr auto mwv = Utility::make_whitespaces_visible;

namespace TTY {
	enum TTY { on = 1, off = 2, both = 3 };
}

TEST_CASE("Testing process reader", "[process_reader]") {
	WHEN("Testing what printf produces") {
		std::stringstream output;
		std::stringstream error;
		{
			INFO("When simply printing \"abc\"");
			REQUIRE(Process_reader::run("printf", "abc", output, error, false));
			REQUIRE(error.str() == "");
			REQUIRE(mwv(output.str()) == mwv("abc"));
		}

		output.str("");
		error.str("");
		{
			INFO("When simply printing \"abc\" in tty mode");
			REQUIRE(Process_reader::run("printf", "abc", output, error));
			REQUIRE(error.str() == "");
			REQUIRE(mwv(output.str()) == mwv("abc"));
		}

		output.str("");
		error.str("");
		{
			INFO("In tty mode \\n should turn into \\r\\n");
			REQUIRE(Process_reader::run("printf", "\n", output, error));
			REQUIRE(error.str() == "");
			REQUIRE(mwv(output.str()) == mwv("\r\n"));
		}

		output.str("");
		error.str("");
		{
			INFO("Without tty mode \\n should stay \\n");
			REQUIRE(Process_reader::run("printf", "\\n", output, error, false));
			REQUIRE(error.str() == "");
			REQUIRE(mwv(output.str()) == mwv("\n"));
		}
	}
	WHEN("Simply creating and destroying") {
		Tool tool;
		tool.path = "ls";
		Process_reader p{tool};
	}
	WHEN("Testing arguments construction") {
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
			REQUIRE(detail::create_arguments_list(test_case.arg_text) == test_case.args);
		}
	}
	auto assert_executed_correctly = [](std::string_view code, std::string_view expected_output, std::string_view expected_error, std::string_view input,
										TTY::TTY tty = TTY::both) {
		INFO("Compiling code:\n" << code);
		const auto cpp_file = "/tmp/SCE_test_process_code.cpp";
		const auto exe_file = "/tmp/SCE_test_process_exe";
		CHECK(std::ofstream{cpp_file} << code);
		REQUIRE(QProcess::execute("g++", {"-std=c++17", cpp_file, "-o", exe_file}) == 0);
		Tool tool;
		tool.path = exe_file;
		tool.working_directory = "/tmp";
		std::string (*process)(std::string_view data);
		auto run = [&] {
			std::string output;
			std::string error;
			auto local_input = input;
			Process_reader p{tool, [&output](std::string_view sv) { output += sv; }, [&error](std::string_view sv) { error += sv; }};
			while (local_input.size() > 0) {
				const auto input_size = std::min(std::size_t{10}, local_input.size());
				p.send_input({local_input.begin(), input_size});
				local_input.remove_prefix(input_size);
			}
			p.close_input();
			REQUIRE(p.join());
			INFO(Color::yellow << "Input          : " << mwv(local_input) << Color::no_color << '\n');
			INFO(Color::yellow << "Expected output: " << Color::yellow << mwv(expected_output) << '\n');
			INFO(Color::yellow << "Actual output  : " << Color::yellow << mwv(output) << '\n');
			INFO(Color::yellow << "Expected error : " << Color::yellow << mwv(expected_error) << '\n');
			INFO(Color::yellow << "Actual error   : " << Color::yellow << mwv(error) << '\n');
			REQUIRE(mwv(error) == mwv(process(expected_error)));
			REQUIRE(mwv(output) == mwv(process(expected_output)));
		};
		if (tty & TTY::on) {
			process = [](std::string_view data) {
				std::string returnvalue;
				for (const auto &character : data) {
					if (character == '\n') {
						returnvalue += '\r';
					}
					returnvalue += character;
				}
				return returnvalue;
			};
			tool.use_tty_mode = true;
			INFO("With TTY");
			run();
		}
		if (tty & TTY::off) {
			process = [](std::string_view data) { return Utility::to_string(data); };
			tool.use_tty_mode = false;
			INFO("Without TTY");
			run();
		}
	};
	WHEN("Reading the output of compiled programs") {
		struct Test_case {
			std::string_view code;
			std::string_view expected_output;
			std::string_view expected_error;
		} const test_cases[] = {
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
			{.code = R"(#include <iostream>
						 int main(int argc, char *argv[]){
							std::cout << argv[0];
						 })",
			 .expected_output = "/tmp/SCE_test_process_exe",
			 .expected_error = ""},
		};

		for (auto &test_case : test_cases) {
			assert_executed_correctly(test_case.code, test_case.expected_output, test_case.expected_error, "");
		}
	}
	WHEN("Passing parameters") {
		const auto cpp_file = "/tmp/SCE_test_process_code.cpp";
		const auto exe_file = "/tmp/SCE_test_process_exe";
		const auto code = R"(#include <iostream>
		int main(int argc, char *argv[]) {
			std::cout << argc << '\n';
			for (int i = 0; i < argc; i++) {
				std::cout << argv[i] << '\n';
			}
		})";
		CHECK(std::ofstream{cpp_file} << code);
		REQUIRE(QProcess::execute("g++", {"-std=c++17", cpp_file, "-o", exe_file}) == 0);
		Tool tool;
		tool.use_tty_mode = false;
		tool.path = exe_file;
		tool.working_directory = "/tmp";
		tool.arguments = "testarg testarg2 \"test arg 3\"";
		std::string output;
		std::string error;
		REQUIRE(Process_reader{tool, [&output](std::string_view sv) { output += sv; }, [&error](std::string_view sv) { error += sv; }}.join());
		REQUIRE(mwv(error) == mwv(""));
		REQUIRE(mwv(output) == mwv("4\n"
								   "/tmp/SCE_test_process_exe\n"
								   "testarg\n"
								   "testarg2\n"
								   "test arg 3\n"));
	}
	WHEN("Checking if we can simulate a tty to get color output") {
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
		assert_executed_correctly(code, "11\n", "", "", TTY::on);
		assert_executed_correctly(code, "00\n", "", "", TTY::off);
	}
	WHEN("Checking if we are considered a character device") {
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
		assert_executed_correctly(code, "1\n", "", "", TTY::on);
		assert_executed_correctly(code, "0\n", "", "", TTY::off);
	}
	WHEN("Testing if input is passed correctly") {
		const auto code = R"(
#include <cstdio>
#include <iostream>

int main() {
	for (auto c = std::getchar(); c != EOF; c = std::getchar()) {
		std::cout << static_cast<char>(c);
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
	WHEN("Testing if \\r and \\n are passed correctly") {
		const auto code = R"(
#include <cstdio>
#include <iostream>

int main() {
	for (auto c = std::getchar(); c != EOF; c = std::getchar()) {
		std::cout << static_cast<char>(c);
	}
}
	)";
		const auto text = "\r\n\r\n";
		assert_executed_correctly(code, text, "", text);
	}
}