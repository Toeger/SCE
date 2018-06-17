#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "interop/notification_server.h"
#include "interop/rpc_server.h"

#include <QMainWindow>
#include <QTimer>
#include <memory>

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
	Edit_window *get_edit_window(std::string_view id);
	void close_notification_server();
	void close_rpc_server();

	private slots:
	void on_actionOpen_File_triggered();
	void on_action_Edit_triggered();
	void on_action_Font_triggered();
	void on_action_Test_triggered();
	void on_file_tabs_tabCloseRequested(int index);
	void closeEvent(QCloseEvent *event) override;
	void edit_buffer_changed(Edit_window *edit_window);

	private:
	void load_last_files();
	void save_last_files();
	void add_file_tab(const QString &filename);

	int timer_delay_ms = 1000;
	std::map<QString, QTimer> timers;
	std::unique_ptr<Tool_editor_widget> tool_editor_widget;
	std::unique_ptr<Ui::MainWindow> ui;
	Notification_server notification_server;
	RPC_server rpc_server;

	private:
	Ui::MainWindow *_; //Qt Designer only works correctly if it finds this string

	friend struct MainWindow_tester;
};

#endif // MAINWINDOW_H
