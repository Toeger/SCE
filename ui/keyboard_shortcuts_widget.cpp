#include "keyboard_shortcuts_widget.h"
#include "external/verdigris/wobjectimpl.h"
#include "logic/lsp_feature.h"
#include "logic/settings.h"
#include "ui_keyboard_shortcuts_widget.h"

#include <QKeyEvent>
#include <QKeySequenceEdit>

W_OBJECT_IMPL(Keyboard_shortcuts_widget)

Keyboard_shortcuts_widget::Keyboard_shortcuts_widget(QWidget *parent)
	: QWidget{parent}
	, __{new Ui::Keyboard_shortcuts_widget}
	, ui{__.get()} {
	ui->setupUi(this);
	const auto &tools = Settings::get<Settings::Key::tools>();
	for (const auto &tool : tools) {
		if (tool.activation != Tool_activation::Type::keyboard_shortcut) {
			continue;
		}
		const auto row = ui->keyboard_shortcuts_tableWidget->rowCount();
		ui->keyboard_shortcuts_tableWidget->insertRow(row);
		ui->keyboard_shortcuts_tableWidget->setItem(row, 0, new QTableWidgetItem{tool.get_name()});
		ui->keyboard_shortcuts_tableWidget->setCellWidget(row, 1, new QKeySequenceEdit{tool.activation_keyboard_shortcut});
	}
	LSP_feature::apply_to_each([&](LSP_feature &feature) {
		const auto row = ui->keyboard_shortcuts_tableWidget->rowCount();
		ui->keyboard_shortcuts_tableWidget->insertRow(row);
		ui->keyboard_shortcuts_tableWidget->setItem(row, 0, new QTableWidgetItem{feature.description});
		auto key_sequence = std::make_unique<QKeySequenceEdit>(feature.activation1);
		connect(key_sequence.get(), &QKeySequenceEdit::keySequenceChanged, [&activation = feature.activation1](const QKeySequence &ks) { activation = ks; });
		ui->keyboard_shortcuts_tableWidget->setCellWidget(row, 1, key_sequence.release());
		key_sequence = std::make_unique<QKeySequenceEdit>(feature.activation2);
		connect(key_sequence.get(), &QKeySequenceEdit::keySequenceChanged, [&activation = feature.activation2](const QKeySequence &ks) { activation = ks; });
		ui->keyboard_shortcuts_tableWidget->setCellWidget(row, 2, key_sequence.release());
	});
	ui->keyboard_shortcuts_tableWidget->resizeColumnsToContents();
}

Keyboard_shortcuts_widget::~Keyboard_shortcuts_widget() {}

void Keyboard_shortcuts_widget::on_buttonBox_accepted() {
	//TODO: Apply changes
	close();
}

void Keyboard_shortcuts_widget::on_buttonBox_rejected() {
	//TODO: Undo changes
	close();
}

void Keyboard_shortcuts_widget::keyPressEvent(QKeyEvent *event) {
	if (event->key() == Qt::Key::Key_Escape) {
		event->accept();
		close();
		return;
	}
	QWidget::keyPressEvent(event);
}
