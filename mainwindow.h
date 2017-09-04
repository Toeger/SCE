#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <functional>
#include <memory>

namespace Ui {
	class MainWindow;
}

class QPlainTextEdit;

class MainWindow : public QMainWindow {
	Q_OBJECT

	public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

	protected slots:
	void on_actionOpen_File_triggered();
	void on_action_Font_triggered();
	void on_file_tabs_tabCloseRequested(int index);

	protected:
	void add_file_tab(const QString &filename);
	void wheelEvent(QWheelEvent *we) override;
	void apply_to_all_tabs(const std::function<void(QPlainTextEdit *)> function);

	std::unique_ptr<Ui::MainWindow> ui;
	Ui::MainWindow *_; //Qt Designer only works correctly if it finds this string
};

#endif // MAINWINDOW_H
