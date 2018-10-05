#ifndef EDIT_WINDOW_H
#define EDIT_WINDOW_H

#include "logic/tool.h"
#include "threading/thread_check.h"

#include <QPlainTextEdit>
#include <QString>
#include <memory>
#include <vector>

class QAction;
class QEvent;
class QSyntaxHighlighter;
class QWheelEvent;

//Widget for code editing
class Edit_window final : public QPlainTextEdit, private Thread_check {
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
	QString get_id() const;
	QString get_buffer() const;

	private:
	void wheelEvent(QWheelEvent *we) override;
	bool event(QEvent *event) override;

	std::vector<QMetaObject::Connection> connections;
	int zoom_remainder{};
	std::vector<std::unique_ptr<QAction>> actions;
	std::unique_ptr<QSyntaxHighlighter> syntax_highlighter;
	std::vector<Note> notes;
	uint32_t state{};
};

#endif // EDIT_WINDOW_H