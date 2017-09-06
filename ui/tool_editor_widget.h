#ifndef TOOL_EDITOR_WIDGET_H
#define TOOL_EDITOR_WIDGET_H

#include <QWidget>
#include <memory>

namespace Ui {
	class Tool_editor_widget;
}

//This window is to specify how to use external tools such as compilers.
class Tool_editor_widget : public QWidget {
	Q_OBJECT

	public:
	explicit Tool_editor_widget(QWidget *parent = 0);
	~Tool_editor_widget();

	protected:
	std::unique_ptr<Ui::Tool_editor_widget> ui;

	protected:
	Ui::Tool_editor_widget *_; //Qt Designer only works correctly if it finds this string
};

#endif // TOOL_EDITOR_WIDGET_H
