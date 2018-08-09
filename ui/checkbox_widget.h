#ifndef CHECKBOX_WIDGET_H
#define CHECKBOX_WIDGET_H

#include <QWidget>

class QCheckBox;

class Checkbox_widget : public QWidget {
	Q_OBJECT

	public:
	explicit Checkbox_widget(QWidget *parent = nullptr);
	Qt::CheckState get_checked_state() const;
	void set_checked_state(Qt::CheckState state);

	private:
	void mousePressEvent(QMouseEvent *event) override;
	QCheckBox *checkbox;
};

#endif // CHECKBOX_WIDGET_H
