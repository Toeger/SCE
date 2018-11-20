#ifndef CHECKBOX_WIDGET_H
#define CHECKBOX_WIDGET_H

#include "external/verdigris/wobjectdefs.h"

#include <QWidget>

class QCheckBox;

W_REGISTER_ARGTYPE(Qt::CheckState)

class Checkbox_widget : public QWidget {
	W_OBJECT(Checkbox_widget)

	public:
	explicit Checkbox_widget(QWidget *parent = nullptr);
	Qt::CheckState get_checked_state() const;
	void set_checked_state(Qt::CheckState state);

	signals:
	void stateChanged(Qt::CheckState new_state) W_SIGNAL(stateChanged, new_state);
	void clicked() W_SIGNAL(clicked);

	private:
	void mousePressEvent(QMouseEvent *event) override;
	QCheckBox *checkbox;
};

#endif // CHECKBOX_WIDGET_H
