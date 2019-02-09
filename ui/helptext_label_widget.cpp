#include "helptext_label_widget.h"

#include <QEvent>
#include <QHelpEvent>
#include <QMessageBox>
#include <QToolTip>
#include <iostream>

Helptext_label_widget::Helptext_label_widget(QWidget *parent)
	: QWidget(parent)
	, label{new QLabel} {
	layout = new QHBoxLayout(this);
	layout->addWidget(label);
	layout->setMargin(0);
	button = new QPushButton;
	button->setIcon(QIcon::fromTheme("help-faq"));
	QObject::connect(button, &QPushButton::clicked, [this] {
		QMessageBox::information(button, "SCE - " + title, help_text.index() == 0 ? std::get<0>(help_text) : std::get<1>(help_text)());
	});
	layout->addWidget(button);
	layout->setAlignment(label, Qt::AlignLeft);
	layout->setAlignment(button, Qt::AlignLeft);
}

void Helptext_label_widget::setText(const QString &text) {
	label->setText(text);
}

void Helptext_label_widget::setTextInteractionFlags(Qt::TextInteractionFlags flags) {
	label->setTextInteractionFlags(flags);
}
