#ifndef KEYBOARD_SHORTCUTS_WIDGET_H
#define KEYBOARD_SHORTCUTS_WIDGET_H

#include "external/verdigris/wobjectdefs.h"
#include "threading/gui_pointer.h"

#include <QWidget>
#include <memory>

namespace Ui {
	class Keyboard_shortcuts_widget;
}

class Keyboard_shortcuts_widget : public QWidget {
	W_OBJECT(Keyboard_shortcuts_widget)

	public:
	explicit Keyboard_shortcuts_widget(QWidget *parent = nullptr);
	~Keyboard_shortcuts_widget() override;

	private slots:
	void on_buttonBox_accepted();
	W_SLOT(on_buttonBox_accepted)
	void on_buttonBox_rejected();
	W_SLOT(on_buttonBox_rejected)

	private:
	void keyPressEvent(QKeyEvent *event) override;

	std::unique_ptr<Ui::Keyboard_shortcuts_widget> __;
	Gui_pointer<Ui::Keyboard_shortcuts_widget> ui;
	[[maybe_unused]] Ui::Keyboard_shortcuts_widget *_;
};

#endif // KEYBOARD_SHORTCUTS_WIDGET_H
