#ifndef HELPTEXT_LABEL_WIDGET
#define HELPTEXT_LABEL_WIDGET

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <functional>
#include <variant>

struct Helptext_label_widget : public QWidget {
	Helptext_label_widget(QWidget *parent = nullptr);

	std::variant<QString, std::function<QString()>> help_text;
	QString title;
	QLabel *label;

	void setText(const QString &text);
	void setTextInteractionFlags(Qt::TextInteractionFlags flags);

	private:
	QHBoxLayout *layout;
	QPushButton *button;
};

#endif //HELPTEXT_LABEL_WIDGET