#include "edit_window.h"
#include "logic/settings.h"
#include "logic/syntax_highligher.h"
#include "logic/tool.h"
#include "logic/tool_actions.h"

#include <QAction>
#include <QColor>
#include <QEvent>
#include <QMessageBox>
#include <QToolTip>
#include <QWheelEvent>
#include <memory>

Edit_window::Edit_window() {
	auto highlighter = std::make_unique<Syntax_highligher>(document());
	highlighter->load_rules(TEST_DATA_PATH "c++-syntax.json");
	syntax_highlighter = std::move(highlighter);
	Tool_actions::add_widget(this);
	connect(this, &QPlainTextEdit::textChanged, [this] { state++; });
}

Edit_window::~Edit_window() {
	Tool_actions::remove_widget(this);
}

void Edit_window::add_note(Edit_window::Note note) {
	QTextCursor tc{textCursor()};
	QTextCharFormat tcf;
	tcf.setUnderlineStyle(QTextCharFormat::UnderlineStyle::WaveUnderline);
	tcf.setUnderlineColor(note.color);
	tc.setCharFormat(tcf);

	notes.push_back(std::move(note));
}

void Edit_window::clear_notes() {
	notes.clear();
}

uint32_t Edit_window::get_state() const {
	return state;
}

void Edit_window::wheelEvent(QWheelEvent *we) {
	if (we->modifiers() == Qt::ControlModifier) {
		const auto raw_zoom = we->delta() + zoom_remainder;
		const auto zoom = raw_zoom / QWheelEvent::DefaultDeltasPerStep;
		zoom_remainder = raw_zoom % QWheelEvent::DefaultDeltasPerStep;
		zoomIn(zoom);
		we->accept();
	} else {
		QPlainTextEdit::wheelEvent(we);
	}
}

bool Edit_window::event(QEvent *event) {
	if (event->type() == QEvent::ToolTip) {
		QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
		QTextCursor cursor = cursorForPosition(helpEvent->pos());
		const int character = cursor.positionInBlock();
		const int line = cursor.blockNumber();
		if (line == 0) {
			QToolTip::hideText();
			return true;
		}
		for (const auto &note : notes) {
			if (note.line == line && note.char_start < character && note.char_end > character) {
				QToolTip::showText(helpEvent->globalPos(), note.text);
				return true;
			}
		}
		QToolTip::hideText();
		return true;
	}
	return QPlainTextEdit::event(event);
}
