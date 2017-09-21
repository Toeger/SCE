#include "test.h"
#include "test_mainwindow.h"
#include "test_settings.h"
#include "test_tool.h"
#include "test_tool_editor_widget.h"
#include "test_process_reader.h"

void test() {
	test_process_reader();
	test_settings();
	test_tool();
	test_tool_editor_widget();
	test_mainwindow();
}