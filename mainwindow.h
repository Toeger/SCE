#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>

namespace Ui {
	class MainWindow;
}

class MainWindow : public QMainWindow {
	Q_OBJECT

	public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

	private slots:
	void on_actionOpen_File_triggered();
	void on_file_tabs_tabCloseRequested(int index);

	private:
	std::unique_ptr<Ui::MainWindow> ui;
	Ui::MainWindow *_; //Qt Designer only works correctly if it finds this string
};

#endif // MAINWINDOW_H
