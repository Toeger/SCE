#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow{parent}
	, ui{std::make_unique<Ui::MainWindow>()} {
	ui->setupUi(this);
}

MainWindow::~MainWindow() {} //required for destructors of otherwise incomplete type Ui::MainWindow
