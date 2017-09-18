#ifndef EDIT_WINDOW_H
#define EDIT_WINDOW_H

#include "logic/tool.h"

#include <QPlainTextEdit>
#include <memory>
#include <vector>

class QAction;
class QSyntaxHighlighter;

//Widget for code editing
class Edit_window : public QPlainTextEdit {
	Q_OBJECT
	public:
	Edit_window();
	~Edit_window();

	protected:
	void wheelEvent(QWheelEvent *we) override;
	void show_output(const QString &output, Tool_output_target::Type output_target, const QString &title, bool is_error);

	int zoom_remainder{};
	std::vector<std::unique_ptr<QAction>> actions;
	std::unique_ptr<QSyntaxHighlighter> syntax_highlighter;
};

#endif // EDIT_WINDOW_H