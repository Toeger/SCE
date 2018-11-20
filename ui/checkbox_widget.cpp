#include "checkbox_widget.h"
#include "external/verdigris/wobjectimpl.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <memory>

W_OBJECT_IMPL(Checkbox_widget)

Checkbox_widget::Checkbox_widget(QWidget *parent)
	: QWidget(parent) {
	auto layout = std::make_unique<QHBoxLayout>();
	auto checkbox_up = std::make_unique<QCheckBox>();
	checkbox = checkbox_up.get();
	layout->addWidget(checkbox_up.release(), 0, Qt::AlignCenter);
	this->setLayout(layout.release());
	connect(checkbox, &QCheckBox::stateChanged, [this](int new_state) { stateChanged(static_cast<Qt::CheckState>(new_state)); });
	connect(checkbox, &QCheckBox::clicked, this, &Checkbox_widget::clicked);
}

Qt::CheckState Checkbox_widget::get_checked_state() const {
	return checkbox->checkState();
}

void Checkbox_widget::set_checked_state(Qt::CheckState state) {
	checkbox->setCheckState(state);
}

void Checkbox_widget::mousePressEvent(QMouseEvent *event) {
	if (event->button() == Qt::LeftButton) {
		checkbox->click();
	}
}
