#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QFontDialog>
#include <QPlainTextEdit>
#include <QSettings>
#include <QFontMetrics>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow{parent}
	, ui{std::make_unique<Ui::MainWindow>()} {
	ui->setupUi(this);
}

MainWindow::~MainWindow() {} //required for destructors of otherwise incomplete type Ui::MainWindow

void MainWindow::on_actionOpen_File_triggered() {
	for (const auto &filename : QFileDialog::getOpenFileNames(this, tr("Select File(s) to open"))) {
		add_file_tab(filename);
	}
}

void MainWindow::on_file_tabs_tabCloseRequested(int index) {
	ui->file_tabs->removeTab(index);
}

void MainWindow::add_file_tab(const QString &filename) {
	auto file_edit = std::make_unique<QPlainTextEdit>();
	QFile file{filename};
	file.open(QFile::ReadOnly);
	if (file.isOpen()) {
		file_edit->setPlainText(file.readAll());
	} else {
		file_edit->setPlaceholderText(tr("Failed reading file %1").arg(filename));
	}
	QFont font;
	font.fromString(QSettings{}.value("Font", "monospace").toString());
	file_edit->setFont(font);
	file_edit->setTabStopWidth(QFontMetrics{font}.width("    "));
	ui->file_tabs->addTab(file_edit.release(), filename);
}

void MainWindow::on_action_Font_triggered() {
	bool success;
	auto font = QFontDialog::getFont(&success, this);
	if (success == false) {
		return;
	}
	QSettings{}.setValue("Font", font.toString());
}
