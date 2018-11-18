#ifndef LSP_FEATURE_SETUP_WIDGET_H
#define LSP_FEATURE_SETUP_WIDGET_H

#include "logic/tool.h"
#include "threading/gui_pointer.h"
#include "threading/thread_call.h"

#include <QWidget>
#include <future>
#include <memory>

namespace Ui {
	class LSP_feature_setup_widget;
}

class LSP_feature_setup_widget : public QWidget {
	Q_OBJECT

	public:
	explicit LSP_feature_setup_widget(QWidget *parent = nullptr);
	~LSP_feature_setup_widget() override;
	Utility::Thread_caller<LSP_feature_setup_widget> thread_caller{this};
	static void update_lsp_features_from_settings();

	private slots:
	void feature_checkbox_clicked(int row, int column);
	void on_buttonBox_accepted();
	void on_buttonBox_rejected();

	private:
	void update_gui_from_settings_and_LSP_servers();
	void load_lsp_settings_to_gui();
	void save_lsp_settings_from_gui();
	template <class Function>
	void async_gui_thread_execute(Function &&function) {
		Utility::async_gui_thread_execute([f = std::move(function), this]() mutable { f(this); });
	}
	std::vector<std::vector<bool>> check_states;
	std::vector<Tool> tools;
	std::future<void> feature_loader;
	std::unique_ptr<Ui::LSP_feature_setup_widget> __;
	Gui_pointer<Ui::LSP_feature_setup_widget> ui;
	Ui::LSP_feature_setup_widget *_; //Qt Designer only works correctly if it finds this string

	friend struct LSP_feature_table;
};

#endif // LSP_FEATURE_SETUP_WIDGET_H
