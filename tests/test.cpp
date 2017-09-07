#include "test.h"
#include "test_mainwindow.h"
#include "test_settings.h"
#include "test_tool.h"
#include "test_tool_editor_widget.h"

void test() {
	test_settings();
	test_tool();
	test_tool_editor_widget();
	test_mainwindow();
}