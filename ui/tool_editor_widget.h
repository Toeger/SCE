#ifndef TOOL_EDITOR_WIDGET_H
#define TOOL_EDITOR_WIDGET_H

#include "external/verdigris/wobjectdefs.h"
#include "helptext_label_widget.h"
#include "logic/tool.h"
#include "threading/thread_check.h"

#include <QWidget>
#include <memory>
#include <vector>

namespace Ui {
	class Tool_editor_widget;
}

struct QComboBox;

//This window is to specify how to use external tools such as compilers.
class Tool_editor_widget : public QWidget, private Thread_check {
	W_OBJECT(Tool_editor_widget)

	public:
	explicit Tool_editor_widget(QWidget *parent = nullptr);
	~Tool_editor_widget() override;

	public slots:
	void update_tools_list();
	W_SLOT(update_tools_list)
	void update_current_tool();
	W_SLOT(update_current_tool)
	void update_current_tool_name();
	W_SLOT(update_current_tool_name)

	private:
	void closeEvent(QCloseEvent *event) override;
	void load_tools_from_settings();
	void save_tools_to_settings() const;
	bool need_to_save();
	void set_tool_ui(Tool::Tool_type type);

	std::vector<Tool> tools;
	std::unique_ptr<Ui::Tool_editor_widget> ui;

	private slots:
	void on_add_pushButton_clicked();
	W_SLOT(on_add_pushButton_clicked)
	void on_tools_listWidget_currentRowChanged(int currentRow);
	W_SLOT(on_tools_listWidget_currentRowChanged)
	void on_remove_pushButton_clicked();
	W_SLOT(on_remove_pushButton_clicked)
	void on_path_browse_pushButton_clicked();
	W_SLOT(on_path_browse_pushButton_clicked)
	void on_buttonBox_accepted();
	W_SLOT(on_buttonBox_accepted)
	void on_buttonBox_rejected();
	W_SLOT(on_buttonBox_rejected)
	void on_activation_comboBox_currentIndexChanged(int index);
	W_SLOT(on_activation_comboBox_currentIndexChanged)
	void on_type_comboBox_currentIndexChanged(int index);
	W_SLOT(on_type_comboBox_currentIndexChanged)

	private:
	int current_tool_list_row{};
	Ui::Tool_editor_widget *_; //Qt Designer only works correctly if it finds this string

	friend struct Tool_editor_widget_tester;
};

#endif // TOOL_EDITOR_WIDGET_H
