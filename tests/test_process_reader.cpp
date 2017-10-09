#include "test_process_reader.h"
#include "logic/process_reader.h"
#include "logic/settings.h"
#include "test.h"
#include "ui/mainwindow.h"

#include <QProcess>
#include <QString>
#include <QStringList>
#include <fstream>

void test_args_construction() {
	struct Test_cases {
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

void test_process_reading() {
	struct Test_cases {
		std::string_view code;
		std::string_view expected_output;
		std::string_view expected_error;
	} test_cases[] = {
		{.code = R"(int main(){})"},
		{.code = R"(#include <iostream>
				 int main(){
					std::cout << "test";
				 })",
		 .expected_output = "test"},
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
		 .expected_output = "HelloWorld"},
	};

	for (auto &test_case : test_cases) {
		const auto cpp_file = "/tmp/SCE_test_process_code.cpp";
		const auto exe_file = "/tmp/SCE_test_process_exe";
		assert_true(std::ofstream{cpp_file} << test_case.code);
		assert_equal(QProcess::execute("g++", {"-std=c++17", cpp_file, "-o", exe_file}), 0);
		Tool tool;
		tool.path = exe_file;
		Process_reader p{tool};
		assert_equal(p.get_output(), test_case.expected_output);
		assert_equal(p.get_error(), test_case.expected_error);
	}
}

void test_process_reader() {
	test_args_construction();
	test_process_reading();
}
