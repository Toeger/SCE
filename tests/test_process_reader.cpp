#include "test_process_reader.h"
#include "logic/process_reader.h"
#include "test.h"

#include <QString>
#include <QStringList>

void test_process_reader() {
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
