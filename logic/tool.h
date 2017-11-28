#ifndef TOOL_H
#define TOOL_H

#include <QKeySequence>
#include <QString>
#include <chrono>
#include <tuple>

class QJsonObject;

//A Tool_output_target specifies what to do with the output of a Tool
namespace Tool_output_target {
	enum Type { ignore, paste, console, popup, replace_document };
}
//A Tool specifies how to use an external tool such as a compiler. It specifies input, output, how to activate the tool and so on.
class Tool {
	public:
	using Activation = QKeySequence; //probably will have to add more ways later

	QString path{};
	QString arguments{};
	QString input{};
	QString working_directory{};
	Tool_output_target::Type output{};
	Tool_output_target::Type error{};
	Activation activation{}; //TODO: add context menu and menu entry
	std::chrono::milliseconds timeout;

	QString to_string() const;
	static Tool from_string(const QString &data);
	QString get_name() const;
};

bool operator==(const Tool &lhs, const Tool &rhs);

bool operator<(const Tool &lhs, const Tool &rhs);

#endif // TOOL_H