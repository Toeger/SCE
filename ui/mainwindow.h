#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <functional>
#include <future>
#include <memory>

#include "interop/notification_server.h"
#include "interop/rpc_server.h"

namespace Ui {
	class MainWindow;
}

class Edit_window;
class Tool_editor_widget;

class MainWindow : public QMainWindow {
	Q_OBJECT

	public:
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow() override;
	static Edit_window *get_current_edit_window();
	static QString get_current_path();
	static MainWindow *get_main_window();
	static QString get_current_selection();
	static void report_error(std::string_view message, std::string_view error);

	public slots:
	void get_edit_window(std::promise<Edit_window *> &promise, std::string_view file_name);
	void close_notification_server();
	void close_rpc_server();

	private slots:
	void on_actionOpen_File_triggered();
	void on_action_Edit_triggered();
	void on_action_Font_triggered();
	void on_action_Test_triggered();
	void on_file_tabs_tabCloseRequested(int index);
	void closeEvent(QCloseEvent *event) override;

	private:
	void load_last_files();
	void save_last_files();
	void add_file_tab(const QString &filename);

	std::unique_ptr<Ui::MainWindow> ui;
	std::unique_ptr<Tool_editor_widget> tool_editor_widget;
	Notification_server notification_server;
	RPC_server rpc_server;

	private:
	Ui::MainWindow *_; //Qt Designer only works correctly if it finds this string

	friend struct MainWindow_tester;
};

#endif // MAINWINDOW_H
