#ifndef TOOL_EDITOR_WIDGET_H
#define TOOL_EDITOR_WIDGET_H

#include <QWidget>

namespace Ui {
	class Tool_editor_widget;
}

class Tool_editor_widget : public QWidget {
	Q_OBJECT

	public:
	explicit Tool_editor_widget(QWidget *parent = 0);
	~Tool_editor_widget();

	private:
	Ui::Tool_editor_widget *ui;
};

#endif // TOOL_EDITOR_WIDGET_H
