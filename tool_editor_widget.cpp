#include "tool_editor_widget.h"
#include "ui_tool_editor_widget.h"

Tool_editor_widget::Tool_editor_widget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::Tool_editor_widget)
{
	ui->setupUi(this);
}

Tool_editor_widget::~Tool_editor_widget()
{
	delete ui;
}
