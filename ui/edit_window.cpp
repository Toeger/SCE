#include "edit_window.h"
#include "external/verdigris/wobjectimpl.h"
#include "logic/settings.h"
#include "logic/syntax_highligher.h"
#include "logic/tool.h"
#include "logic/tool_actions.h"

#include <QAction>
#include <QColor>
#include <QEvent>
#include <QTextDocument>
#include <QToolTip>
#include <QWheelEvent>
#include <iostream>
#include <memory>

W_OBJECT_IMPL(Edit_window)

static Edit_window::Edit::Position get_line_and_character(QTextDocument &document, int position) {
	const auto &block = document.findBlock(position);
	const auto character_position = position - block.position();
	std::cerr << "Block at position " << position << " has text \"" << block.text().toStdString() << "\"\n"
			  << "Position " << position << " is line " << block.blockNumber() << " character " << character_position << '\n';
	return {.line = block.blockNumber(), .character = character_position};
}

Edit_window::Edit_window() {
	auto highlighter = std::make_unique<Syntax_highligher>(document());
	highlighter->load_rules(TEST_DATA_PATH "c++-syntax.json");
	syntax_highlighter = std::move(highlighter);
	Tool_actions::add_widget(this);
	autoconnect(this, &QPlainTextEdit::textChanged, [this] { state++; });
	autoconnect(document(), &QTextDocument::contentsChange, [this](int position, int chars_removed, int chars_added) {
		auto &document = *this->document();
		document.undo();
		const auto start_pos = get_line_and_character(document, position);
		const auto end_pos = chars_removed == 0 ? start_pos : get_line_and_character(document, position + chars_removed);
		document.redo();
		std::string text;
		if (chars_added) {
			auto block = document.findBlock(position);
			const int character = position - block.position();
			text.reserve(chars_added);
			text = block.text().mid(character, chars_added).toStdString();
			while (text.size() < chars_added) {
				block = block.next();
				text.push_back('\n');
				auto block_text = block.text();
				if (text.size() + block_text.size() >= chars_added) {
					block_text.resize(chars_added - text.size());
				}
				text += block_text.toStdString();
			}
		}
		Edit edit{
			.start = start_pos,
			.end = end_pos,
			.length = chars_removed,
			.added = std::move(text),
		};
		std::clog << "Document changed: removed " << edit.length << " characters from " << edit.start.line << ":" << edit.start.character << " to "
				  << edit.end.line << ":" << edit.end.character << " and replaced it with " << chars_added << " characters \"" << edit.added << "\"\n";
		edited(std::move(edit));
	});
}

Edit_window::~Edit_window() {
	Tool_actions::remove_widget(this);
	for (auto &connection : connections) {
		disconnect(connection);
	}
}

void Edit_window::add_note(Edit_window::Note note) {
	thread_check();
	QTextCursor tc{textCursor()};
	tc.setVisualNavigation(true);
	tc.setPosition(0);
	tc.movePosition(tc.Down, tc.MoveAnchor, note.line - 1);
	tc.movePosition(tc.Right, tc.MoveAnchor, note.char_start - 1);
	tc.movePosition(tc.Right, tc.KeepAnchor, note.char_end - note.char_start);

	QTextCharFormat tcf;
	tcf.setUnderlineStyle(QTextCharFormat::UnderlineStyle::WaveUnderline);
	tcf.setUnderlineColor(note.color);
	{
		QSignalBlocker blocker(this); //Setting the format counts as editing the buffer which causes more messages.
		tc.setCharFormat(tcf);		  //To avoid an infinite update loop we don't want this to cause a textChanged signal.
	}
	notes.push_back(std::move(note));
}

void Edit_window::clear_notes() {
	notes.clear();
}

uint32_t Edit_window::get_state() const {
	return state;
}

QString Edit_window::get_id() const {
	thread_check();
	return windowTitle();
}

QString Edit_window::get_buffer() const {
	thread_check();
	return toPlainText();
}

void Edit_window::wheelEvent(QWheelEvent *we) {
	thread_check();
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
	thread_check();
	if (event->type() == QEvent::ToolTip) {
		auto helpEvent = static_cast<QHelpEvent *>(event);
		QTextCursor cursor = cursorForPosition(helpEvent->pos());
		const int character = cursor.positionInBlock() + 1;
		const int line = cursor.blockNumber() + 1;
		if (line == 0) {
			QToolTip::hideText();
			return true;
		}
		for (const auto &note : notes) {
			if (note.line == line && note.char_start <= character && note.char_end >= character) {
				QToolTip::showText(helpEvent->globalPos(), note.text);
				return true;
			}
		}
		QToolTip::hideText();
		return true;
	}
	return QPlainTextEdit::event(event);
}
