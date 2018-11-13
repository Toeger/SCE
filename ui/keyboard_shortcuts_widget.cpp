#include "keyboard_shortcuts_widget.h"
#include "logic/settings.h"
#include "ui_keyboard_shortcuts_widget.h"

#include <QKeySequenceEdit>

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
	ui->keyboard_shortcuts_tableWidget->resizeColumnsToContents();
}

Keyboard_shortcuts_widget::~Keyboard_shortcuts_widget() {}
