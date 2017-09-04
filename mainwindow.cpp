#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QFontDialog>
#include <QFontMetrics>
#include <QPlainTextEdit>
#include <QSettings>

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
	file_edit->setLineWrapMode(QPlainTextEdit::LineWrapMode::NoWrap);
	ui->file_tabs->addTab(file_edit.release(), filename);
}

void MainWindow::wheelEvent(QWheelEvent *we) {
	if (we->modifiers() == Qt::ControlModifier) {
		auto zoom = we->delta() / QWheelEvent::DefaultDeltasPerStep;
		apply_to_all_tabs([zoom](QPlainTextEdit *edit) { edit->zoomIn(zoom); });
		we->accept();
	}
	we->ignore();
}

void MainWindow::apply_to_all_tabs(const std::function<void(QPlainTextEdit *)> &function) {
	for (int tab_index = 0; tab_index < ui->file_tabs->count(); tab_index++) {
		auto edit = dynamic_cast<QPlainTextEdit *>(ui->file_tabs->widget(tab_index));
		function(edit);
	}
}

void MainWindow::on_action_Font_triggered() {
	bool success;
	const auto font = QFontDialog::getFont(&success, this);
	if (success == false) {
		return;
	}
	QSettings{}.setValue("Font", font.toString());
	apply_to_all_tabs([&font](QPlainTextEdit *edit) { edit->setFont(font); });
}
