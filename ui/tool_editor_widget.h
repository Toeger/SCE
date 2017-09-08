#ifndef TOOL_EDITOR_WIDGET_H
#define TOOL_EDITOR_WIDGET_H

#include "logic/tool.h"

#include <QWidget>
#include <memory>
#include <vector>

namespace Ui {
	class Tool_editor_widget;
}

struct QComboBox;

//This window is to specify how to use external tools such as compilers.
class Tool_editor_widget : public QWidget {
	Q_OBJECT

	public:
	explicit Tool_editor_widget(QWidget *parent = 0);
	~Tool_editor_widget();
	void closeEvent(QCloseEvent *event) override;

	public slots:
	void update_tools_list();
	void update_current_tool();

	protected:
	void load_tools_from_settings();
	void save_tools_to_settings() const;
	bool need_to_save();

	std::vector<Tool> tools;
	std::unique_ptr<Ui::Tool_editor_widget> ui;

	protected slots:
	void on_add_pushButton_clicked();
	void on_tools_listWidget_currentRowChanged(int currentRow);
	void on_remove_pushButton_clicked();
	void on_path_browse_pushButton_clicked();
	void on_buttonBox_accepted();
	void on_buttonBox_rejected();

	private:
	void fill_output_list(QComboBox *combobox);

	Ui::Tool_editor_widget *_; //Qt Designer only works correctly if it finds this string
};

#endif // TOOL_EDITOR_WIDGET_H
