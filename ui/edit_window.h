#ifndef EDIT_WINDOW_H
#define EDIT_WINDOW_H

#include <QPlainTextEdit>

class Edit_window : public QPlainTextEdit {
	Q_OBJECT
	public:
	Edit_window();

	protected:
	void wheelEvent(QWheelEvent *we) override;
	int zoom_remainder{};
};

#endif // EDIT_WINDOW_H