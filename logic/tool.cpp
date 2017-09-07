#include "tool.h"

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>

QString Tool::to_string() const {
	QJsonObject object;
#define WRITE(X) write(X, #X, object)
	WRITE(path);
	WRITE(arguments);
	WRITE(input);
	WRITE(output);
	WRITE(error);
	WRITE(activation);
#undef WRITE
	QJsonDocument json{object};
	return QString::fromUtf8(json.toJson());
}

Tool Tool::from_string(const QString &data) {
	Tool tool;
	auto json = QJsonDocument::fromJson(data.toUtf8()).object();
#define READ(X) read(tool.X, #X, json)
	READ(path);
	READ(arguments);
	READ(input);
	READ(output);
	READ(error);
	READ(activation);
#undef READ
	return tool;
}

void Tool::write(const QString &data, const QString &name, QJsonObject &json) const {
	json[name] = data;
}

void Tool::write(const Tool_output_target::Type &data, const QString &name, QJsonObject &json) const {
	json[name] = data;
}

void Tool::write(const Tool::Activation &data, const QString &name, QJsonObject &json) const {
	json[name] = data.toString();
}

void Tool::read(QString &data, const QString &name, QJsonObject &json) {
	data = json[name].toString();
}

void Tool::read(Tool_output_target::Type &data, const QString &name, QJsonObject &json) {
	data = static_cast<Tool_output_target::Type>(json[name].toInt());
}

void Tool::read(Tool::Activation &data, const QString &name, QJsonObject &json) {
	data = json[name].toString();
}
