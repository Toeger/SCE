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
	Settings::Keeper keeper;
	MainWindow mw; //required because tools can read their input from the current content
	struct Test_cases {
		Tool tool;
		std::string_view code;
		std::string_view expected_output;
		std::string_view expected_error;
	} test_cases[] = {{.tool = {}, .code = R"(int main(){})"}};

	for (auto &test_case : test_cases) {
		const auto cpp_file = "/tmp/SCE_test_process_code.cpp";
		const auto exe_file = "/tmp/SCE_test_process_exe";
		assert_true(std::ofstream{cpp_file} << test_case.code);
		assert_equal(QProcess::execute("g++", {"-std=c++17", cpp_file, "-o", exe_file}), 0);
		test_case.tool.path = exe_file;
		Process_reader p{test_case.tool};
		assert_equal(p.get_output(), test_case.expected_output);
		assert_equal(p.get_error(), test_case.expected_error);
	}
}

void test_process_reader() {
	test_args_construction();
	test_process_reading();
}
