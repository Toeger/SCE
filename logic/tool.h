#ifndef TOOL_H
#define TOOL_H

#include <QKeySequence>
#include <QString>
#include <tuple>

class QJsonObject;

//A Tool_output_target
namespace Tool_output_target {
	enum Type { ignore, paste, console, popup };
}
//A Tool specifies how to use an external tool such as a compiler. It specifies input, output, how to activate the tool and so on.
class Tool {
	public:
	using Activation = QKeySequence; //probably will have to add more ways later

	QString path{};
	QString arguments{};
	QString input{};
	Tool_output_target::Type output{};
	Tool_output_target::Type error{};
	Activation activation{}; //todo: add context menu and menu entry

	QString to_string() const;
	static Tool from_string(const QString &data);

	private:
	void write(const QString &data, const QString &name, QJsonObject &json) const;
	void write(const Tool_output_target::Type &data, const QString &name, QJsonObject &json) const;
	void write(const Activation &data, const QString &name, QJsonObject &json) const;
	static void read(QString &data, const QString &name, QJsonObject &json);
	static void read(Tool_output_target::Type &data, const QString &name, QJsonObject &json);
	static void read(Activation &data, const QString &name, QJsonObject &json);
};

inline bool operator==(const Tool &lhs, const Tool &rhs) {
	return std::tie(lhs.path, lhs.arguments, lhs.input, lhs.output, lhs.error, lhs.activation) ==
		   std::tie(rhs.path, rhs.arguments, rhs.input, rhs.output, rhs.error, rhs.activation);
}

inline bool operator<(const Tool &lhs, const Tool &rhs) {
	return std::tie(lhs.path, lhs.arguments, lhs.input, lhs.output, lhs.error, lhs.activation) <
		   std::tie(rhs.path, rhs.arguments, rhs.input, rhs.output, rhs.error, rhs.activation);
}

#endif // TOOL_H