#include "test_mainwindow.h"
#include "ui/mainwindow.h"
#include "logic/settings.h"
#include "test.h"
#include "ui_mainwindow.h"

#include <QPlainTextEdit>
#include <QTemporaryFile>

struct MainWindowTester : MainWindow {
	void test() {
		test_add_file_tab();
	}
	void test_add_file_tab() {
		ui->file_tabs->clear();
		{ //check behavior when loading a file that we cannot access
			assert_equal(ui->file_tabs->count(), 0);
			const constexpr auto non_existing_filename = "not-existing-file";
			add_file_tab(non_existing_filename);
			assert_equal(ui->file_tabs->count(), 1);
			auto edit = dynamic_cast<QPlainTextEdit *>(ui->file_tabs->currentWidget());
			assert(edit);
			assert_equal(edit->toPlainText(), "");
			assert_equal(edit->placeholderText(), tr("Failed reading file %1").arg(non_existing_filename));
		}
		{ //check closing tab
			on_file_tabs_tabCloseRequested(0);
			assert_equal(ui->file_tabs->count(), 0);
		}
		{ //check loading existing file
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
		}
	}
};

void test_mainwindow() {
	Settings::Keeper keeper;
	MainWindowTester{}.test();
}
