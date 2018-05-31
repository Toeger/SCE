#ifndef EDIT_WINDOW_H
#define EDIT_WINDOW_H

#include "logic/tool.h"

#include <QPlainTextEdit>
#include <memory>
#include <vector>

class QAction;
class QEvent;
class QSyntaxHighlighter;
class QWheelEvent;

//Widget for code editing
class Edit_window : public QPlainTextEdit {
	Q_OBJECT
	public:
	Edit_window();
	~Edit_window() override;

	struct Note {
		QString text;
		QColor color;
		int line;
		int char_start;
		int char_end;
	};
	void add_note(Note note);
	void clear_notes();
	uint32_t get_state() const;

	private:
	void wheelEvent(QWheelEvent *we) override;
	bool event(QEvent *event) override;
	void show_output(const QString &output, Tool_output_target::Type output_target, const QString &title, bool is_error);

	int zoom_remainder{};
	std::vector<std::unique_ptr<QAction>> actions;
	std::unique_ptr<QSyntaxHighlighter> syntax_highlighter;
	std::vector<Note> notes;
	uint32_t state{};
};

#endif // EDIT_WINDOW_H