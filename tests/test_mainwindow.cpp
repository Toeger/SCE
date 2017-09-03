#include "test_mainwindow.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QPlainTextEdit>
#include <QTemporaryFile>
#include <cassert>

template <class T, class U>
void assert_equal(const T &t, const U &u) {
	assert(t == u);
}

struct MainWindowTester : MainWindow {
	bool test() {
		return test_add_file_tab();
	}
	bool test_add_file_tab() {
		assert_equal(ui->file_tabs->count(), 0);
		add_file_tab("");
		assert_equal(ui->file_tabs->count(), 1);
		on_file_tabs_tabCloseRequested(0);
		assert_equal(ui->file_tabs->count(), 0);
		QTemporaryFile tempfile{};
		tempfile.open();
		const constexpr auto tempfile_contents = R"(#include <iostream>
int main(){
	std::cout << "Hello, world!";
}
)";
		tempfile.write(tempfile_contents);
		tempfile.flush();
		add_file_tab(tempfile.fileName());
		assert_equal(ui->file_tabs->count(), 1);
		auto edit = dynamic_cast<QPlainTextEdit *>(ui->file_tabs->currentWidget());
		assert(edit);
		assert_equal(edit->toPlainText(), tempfile_contents);
		return true;
	}
};

bool test_mainwindow() {
	MainWindowTester mwt;
	return mwt.test();
}
