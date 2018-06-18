#include "mainwindow.h"
#include "edit_window.h"
#include "logic/settings.h"
#include "logic/tool_actions.h"
#include "tool_editor_widget.h"
#include "ui_mainwindow.h"
#include "utility/color.h"

#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QFontDialog>
#include <QFontMetrics>
#include <cassert>
#include <iostream>
#include <sce.pb.h>
#include <utility>

static MainWindow *main_window{};

template <class Function>
static Edit_window *find_edit_window(const Function &function, std::unique_ptr<Ui::MainWindow> &ui) {
	static_assert(std::is_convertible_v<bool, std::invoke_result_t<Function, Edit_window *>>,
				  "Given Function must be callable with an Edit_window * and return a truthy or falsy type");
	for (int tab_index = 0; tab_index < ui->file_tabs->count(); tab_index++) {
		assert(dynamic_cast<Edit_window *>(ui->file_tabs->widget(tab_index)));
		auto edit_window = static_cast<Edit_window *>(ui->file_tabs->widget(tab_index));
		if (function(edit_window)) {
			return edit_window;
		}
	}
	return nullptr;
}

template <class Function>
static void apply_to_all_edit_windows(const Function &function, std::unique_ptr<Ui::MainWindow> &ui) {
	find_edit_window(
		[&function](Edit_window *edit_window) {
			function(edit_window);
			return false;
		},
		ui);
}

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow{parent}
	, ui{std::make_unique<Ui::MainWindow>()} {
	main_window = this;
	ui->setupUi(this);
	load_last_files();
	Tool_actions::set_actions(Settings::get<Settings::Key::tools>());
}

MainWindow::~MainWindow() { //required for destructors of otherwise incomplete type Ui::MainWindow
	save_last_files();
	main_window = nullptr;
}

Edit_window *MainWindow::get_current_edit_window() {
	main_window->thread_check();
	return dynamic_cast<Edit_window *>(main_window->ui->file_tabs->currentWidget());
}

QString MainWindow::get_current_path() {
	if (main_window == nullptr) {
		return {};
	}
	main_window->thread_check();
	const auto tab_bar = main_window->ui->file_tabs->tabBar();
	return tab_bar->tabText(tab_bar->currentIndex());
}

MainWindow *MainWindow::get_main_window() {
	return main_window;
}

QString MainWindow::get_current_selection() {
	if (main_window == nullptr) {
		return {};
	}
	main_window->thread_check();
	auto edit_window = dynamic_cast<Edit_window *>(main_window->ui->file_tabs->currentWidget());
	if (edit_window == nullptr) {
		return {};
	}
	return edit_window->textCursor().selectedText().replace("\u2029", "\n");
}

void MainWindow::report_error(std::string_view message, std::string_view error) {
	//TODO: If we have a GUI show it there too.
	std::cerr << Color::light_red << message << ' ' << Color::red << error << Color::no_color << '\n';
}

Edit_window *MainWindow::get_edit_window(std::string_view id) {
	return find_edit_window(
		[&id](Edit_window *target_edit_window) {
			return target_edit_window->get_id().toStdString() == id; //TODO: find a better way to compare QString and std::string_view
		},
		ui);
}

void MainWindow::close_notification_server() {
	thread_check();
	notification_server.clear_listening_endpoints();
}

void MainWindow::close_rpc_server() {
	thread_check();
	rpc_server.close();
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

void MainWindow::edit_buffer_changed(Edit_window *edit_window) {
	auto id = edit_window->get_id();
	auto timer_it = timers.find(id);
	if (timer_it == std::end(timers)) {
		timer_it = timers.emplace(std::piecewise_construct, std::forward_as_tuple(std::move(id)), std::forward_as_tuple()).first;
		timer_it->second.setSingleShot(true);
		connect(&timer_it->second, &QTimer::timeout, [edit_window, &notification_server = notification_server] {
			if (main_window == nullptr) { //TODO: Make it so signals are disconnected before ~MainWindow()
				return;
			}
			edit_window->clear_notes();
			sce::proto::EditNotification edit_notification;
			sce::proto::FileState &file_state = *edit_notification.mutable_filestate();
			file_state.set_id(edit_window->get_id().toStdString());
			file_state.set_state(edit_window->get_state());
			notification_server.send_notification(edit_notification);
		});
	}
	timer_it->second.start(timer_delay_ms);
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
	for (int i = 0; i < ui->file_tabs->tabBar()->count(); i++) {
		if (ui->file_tabs->tabText(i) == filename) {
			ui->file_tabs->setCurrentIndex(i);
			return;
		}
	}
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
	file_edit->setWindowTitle(filename);
	connect(file_edit.get(), &QPlainTextEdit::textChanged, [edit_window = file_edit.get()] {
		if (main_window == nullptr) { //TODO: Make it so signals are disconnected before ~MainWindow()
			return;
		}
		main_window->edit_buffer_changed(edit_window);
	});
	auto index = ui->file_tabs->addTab(file_edit.release(), filename);
	ui->file_tabs->setTabToolTip(index, filename);
}

void MainWindow::on_action_Font_triggered() {
	bool success;
	const auto font = QFontDialog::getFont(&success, this);
	if (success == false) {
		return;
	}
	Settings::set<Settings::Key::font>(font.toString());
	apply_to_all_edit_windows([&font](Edit_window *edit) { edit->setFont(font); }, ui);
}

void MainWindow::on_action_Edit_triggered() {
	if (tool_editor_widget != nullptr && tool_editor_widget->isVisible()) {
		tool_editor_widget->activateWindow();
	} else {
		tool_editor_widget = std::make_unique<Tool_editor_widget>();
		tool_editor_widget->show();
	}
}

void MainWindow::on_action_Test_triggered() {
	auto edit = get_current_edit_window();
	Edit_window::Note note;
	note.line = 1;
	note.char_start = 3;
	note.char_end = 7;
	note.color = 0xff0000;
	note.text = "Test notification";
	edit->add_note(std::move(note));
}
