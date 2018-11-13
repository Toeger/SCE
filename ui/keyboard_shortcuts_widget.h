#ifndef KEYBOARD_SHORTCUTS_WIDGET_H
#define KEYBOARD_SHORTCUTS_WIDGET_H

#include "threading/gui_pointer.h"

#include <QWidget>
#include <memory>

namespace Ui {
	class Keyboard_shortcuts_widget;
}

class Keyboard_shortcuts_widget : public QWidget {
	Q_OBJECT

	public:
	explicit Keyboard_shortcuts_widget(QWidget *parent = nullptr);
	~Keyboard_shortcuts_widget();

	private:
	std::unique_ptr<Ui::Keyboard_shortcuts_widget> __;
	Gui_pointer<Ui::Keyboard_shortcuts_widget> ui;
	Ui::Keyboard_shortcuts_widget *_;
};

#endif // KEYBOARD_SHORTCUTS_WIDGET_H
