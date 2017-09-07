#include "tool_editor_widget.h"
#include "logic/settings.h"
#include "ui_tool_editor_widget.h"

#include <QSettings>

Tool_editor_widget::Tool_editor_widget(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::Tool_editor_widget) {
	ui->setupUi(this);
	load_tools_from_settings();
}

Tool_editor_widget::~Tool_editor_widget() {
	save_tools_to_settings();
}

void Tool_editor_widget::load_tools_from_settings() {}

void Tool_editor_widget::save_tools_to_settings() {}
