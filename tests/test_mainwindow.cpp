#include "test_mainwindow.h"
#include "logic/settings.h"
#include "test.h"
#include "utility/thread_call.h"

#include <QPlainTextEdit>
#include <QTemporaryFile>

void MainWindow_tester::test() {
	CHECK(currently_in_gui_thread());
	WHEN("Adding file tabs") {
		ui->file_tabs->clear();
		REQUIRE(ui->file_tabs->count() == 0);
		WHEN("File cannot be accessed") {
			const constexpr auto non_existing_filename = "not-existing-file";
			add_file_tab(non_existing_filename);
			REQUIRE(ui->file_tabs->count() == 1);
			auto edit = dynamic_cast<QPlainTextEdit *>(ui->file_tabs->currentWidget());
			assert(edit);
			REQUIRE(edit->toPlainText() == "");
			REQUIRE(edit->placeholderText() == QObject::tr("Failed reading file %1").arg(non_existing_filename));
		}
		WHEN("Closing tab") { // check closing tab
			on_file_tabs_tabCloseRequested(0);
			REQUIRE(ui->file_tabs->count() == 0);
		}
		WHEN("Loading existing file") {
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
			REQUIRE(ui->file_tabs->count() == 1);
			auto edit = dynamic_cast<QPlainTextEdit *>(ui->file_tabs->currentWidget());
			assert(edit);
			REQUIRE(edit->toPlainText() == tempfile_contents);
		}
	}
}

TEST_CASE("Testing main window", "[mainwindow]") {
	Settings::Keeper keeper;
	Utility::sync_gui_thread_execute([] { test_main_window->test(); }); //we must test the gui in the gui thread
}