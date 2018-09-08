#ifndef TEST_MAINWINDOW_H
#define TEST_MAINWINDOW_H

#include "ui/mainwindow.h"
#include "ui_mainwindow.h"

struct MainWindow_tester : public MainWindow {
	using MainWindow::add_file_tab;
	using MainWindow::on_file_tabs_tabCloseRequested;
	using MainWindow::ui;
	void test();
} extern *test_main_window;

#endif //TEST_MAINWINDOW_H