#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "external/verdigris/wobjectdefs.h"
#include "interop/notification_server.h"
#include "interop/rpc_server.h"
#include "threading/thread_check.h"

#include <QMainWindow>
#include <QTimer>
#include <memory>

namespace Ui {
	class MainWindow;
}

class Edit_window;
class Tool_editor_widget;
class LSP_feature_setup_widget;
class Keyboard_shortcuts_widget;
struct Tool;

W_REGISTER_ARGTYPE(Edit_window &)
W_REGISTER_ARGTYPE(std::string)

class MainWindow : public QMainWindow, private Thread_check {
	W_OBJECT(MainWindow)

	public:
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow() override;
	static Edit_window *get_current_edit_window();
	static QString get_current_path();
	static MainWindow &get_main_window();
	static QString get_current_selection();
	static void report_error(std::string_view message, std::string_view error);
	Edit_window *get_edit_window(std::string_view id);
	static bool currently_in_gui_thread();
	void open_setup_tools_at(const Tool &tool);

	public slots:
	void close_notification_server();
	W_SLOT(close_notification_server)
	void close_rpc_server();
	W_SLOT(close_rpc_server)
	void set_status(QString text);
	W_SLOT(set_status)
	void on_action_Setup_tools_triggered();
	W_SLOT(on_action_Setup_tools_triggered)

	signals:
	void file_opened(Edit_window &edit_window, std::string path) W_SIGNAL(file_opened, edit_window, path);
	//Note: There is no file_closed signal. Use &Edit_window::destroyed instead.

	private slots:
	void on_actionOpen_File_triggered();
	W_SLOT(on_actionOpen_File_triggered)
	void on_actionLSP_Setup_triggered();
	W_SLOT(on_actionLSP_Setup_triggered)
	void on_action_Font_triggered();
	W_SLOT(on_action_Font_triggered)
	void on_action_Test_triggered();
	W_SLOT(on_action_Test_triggered)
	void on_file_tabs_tabCloseRequested(int index);
	W_SLOT(on_file_tabs_tabCloseRequested)
	void on_action_Keyboard_shortcuts_triggered();
	W_SLOT(on_action_Keyboard_shortcuts_triggered)
	void closeEvent(QCloseEvent *event) override;
	void edit_buffer_changed(Edit_window *edit_window);

	private:
	void load_last_files();
	void save_last_files();
	void add_file_tab(const QString &filename);

	int timer_delay_ms = 1000;
	std::vector<QMetaObject::Connection> connections;
	std::map<QString, QTimer> timers;
	std::unique_ptr<Ui::MainWindow> ui;
	std::unique_ptr<Tool_editor_widget> tool_editor_widget;
	std::unique_ptr<LSP_feature_setup_widget> lsp_feature_setup_widget;
	std::unique_ptr<Keyboard_shortcuts_widget> keyboard_shortcuts_widget;
	Notification_server notification_server;
	RPC_server rpc_server;

	private:
	std::unique_ptr<QWidget> status_widget;
	Ui::MainWindow *_; //Qt Designer only works correctly if it finds this string

	friend struct MainWindow_tester;
};

#endif // MAINWINDOW_H
