#ifndef LSP_FEATURE_SETUP_WIDGET_H
#define LSP_FEATURE_SETUP_WIDGET_H

#include "logic/tool.h"
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
	void update_lsp_features();
	Utility::Thread_caller<LSP_feature_setup_widget> thread_caller{this};

	private slots:
	void feature_checkbox_clicked(int row, int column);

	private:
	template <class Function>
	void async_gui_thread_execute(Function &&function) {
		Utility::async_gui_thread_execute([f = std::move(function), this]() mutable { f(this); });
	}
	std::vector<std::vector<bool>> check_states;
	std::vector<Tool> tools;
	std::future<void> feature_loader;
	std::unique_ptr<Ui::LSP_feature_setup_widget> ui;
	Ui::LSP_feature_setup_widget *_; //Qt Designer only works correctly if it finds this string

	friend struct LSP_feature_table;
};

#endif // LSP_FEATURE_SETUP_WIDGET_H
