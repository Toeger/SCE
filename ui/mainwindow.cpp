#include "mainwindow.h"
#include "edit_window.h"
#include "logic/settings.h"
#include "tool_editor_widget.h"
#include "ui_mainwindow.h"

#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QFontDialog>
#include <QFontMetrics>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow{parent}
	, ui{std::make_unique<Ui::MainWindow>()} {
	ui->setupUi(this);
	load_last_files();
}

MainWindow::~MainWindow() { //required for destructors of otherwise incomplete type Ui::MainWindow
	save_last_files();
}

void MainWindow::on_actionOpen_File_triggered() {
	for (const auto &filename : QFileDialog::getOpenFileNames(this, tr("Select File(s) to open"))) {
		add_file_tab(filename);
	}
}

void MainWindow::on_file_tabs_tabCloseRequested(int index) {
	ui->file_tabs->removeTab(index);
}
void MainWindow::closeEvent(QCloseEvent *event) {
	tool_editor_widget = nullptr;
	event->accept();
}

void MainWindow::load_last_files() {
	for (const auto &filename : Settings::get<Settings::Key::files>()) {
		add_file_tab(filename);
	}
	ui->file_tabs->setCurrentIndex(Settings::get<Settings::Key::current_file>());
}

void MainWindow::save_last_files() {
	QStringList filename_list;
	for (int tab_index = 0; tab_index < ui->file_tabs->count(); tab_index++) {
		filename_list << ui->file_tabs->tabBar()->tabText(tab_index);
	}
	Settings::set<Settings::Key::files>(filename_list);
	Settings::set<Settings::Key::current_file>(ui->file_tabs->currentIndex());
}

void MainWindow::add_file_tab(const QString &filename) {
	auto file_edit = std::make_unique<Edit_window>();
	QFile file{filename};
	file.open(QFile::ReadOnly);
	if (file.isOpen()) {
		file_edit->setPlainText(file.readAll());
	} else {
		file_edit->setPlaceholderText(tr("Failed reading file %1").arg(filename));
	}
	QFont font;
	font.fromString(Settings::get<Settings::Key::font>("monospace"));
	file_edit->setFont(font);
	file_edit->setTabStopWidth(QFontMetrics{font}.width("    "));
	file_edit->setLineWrapMode(Edit_window::LineWrapMode::NoWrap);
	ui->file_tabs->addTab(file_edit.release(), filename);
}

void MainWindow::apply_to_all_edit_windows(const std::function<void(Edit_window *)> &function) {
	for (int tab_index = 0; tab_index < ui->file_tabs->count(); tab_index++) {
		auto edit = dynamic_cast<Edit_window *>(ui->file_tabs->widget(tab_index));
		function(edit);
	}
}

void MainWindow::on_action_Font_triggered() {
	bool success;
	const auto font = QFontDialog::getFont(&success, this);
	if (success == false) {
		return;
	}
	Settings::set<Settings::Key::font>(font.toString());
	apply_to_all_edit_windows([&font](Edit_window *edit) { edit->setFont(font); });
}

void MainWindow::on_action_Edit_triggered() {
	if (tool_editor_widget != nullptr && tool_editor_widget->isVisible()) {
		tool_editor_widget->activateWindow();
	} else {
		tool_editor_widget = std::make_unique<Tool_editor_widget>();
		tool_editor_widget->show();
	}
}
