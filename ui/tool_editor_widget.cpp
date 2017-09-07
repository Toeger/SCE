#include "tool_editor_widget.h"
#include "logic/settings.h"
#include "ui_tool_editor_widget.h"

#include <QStringList>

Tool_editor_widget::Tool_editor_widget(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::Tool_editor_widget) {
	ui->setupUi(this);
	load_tools_from_settings();
}

Tool_editor_widget::~Tool_editor_widget() {
	save_tools_to_settings();
}

void Tool_editor_widget::load_tools_from_settings() {
	const auto tool_list = Settings::get<Settings::Key::tools>();
	tools.clear();
	tools.reserve(tool_list.size());
	for (const auto &tool : tool_list) {
		tools.push_back(Tool::from_string(tool));
	}
}

void Tool_editor_widget::save_tools_to_settings() const {
	QStringList tool_list;
	for (const auto &tool : tools) {
		tool_list << tool.to_string();
	}
	Settings::set<Settings::Key::tools>(tool_list);
}
