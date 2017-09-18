#include "edit_window.h"
#include "logic/settings.h"
#include "logic/syntax_highligher.h"
#include "logic/tool.h"
#include "logic/tool_actions.h"

#include <QAction>
#include <QMessageBox>
#include <QProcess>
#include <memory>

Edit_window::Edit_window()
	: syntax_highlighter{std::make_unique<Syntax_highligher>(document())} {
	Tool_actions::add_widget(this);
}

Edit_window::~Edit_window() {
	Tool_actions::remove_widget(this);
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
