#ifndef TOOL_ACTIONS_H
#define TOOL_ACTIONS_H

#include <vector>

struct Tool;
class QWidget;

namespace Tool_actions {
	void add_widget(QWidget *widget);
	void remove_widget(QWidget *widget);
	void set_actions(const std::vector<Tool> &tools);
} // namespace Tool_actions

#endif // TOOL_ACTIONS_H