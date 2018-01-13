#ifndef TOOL_H
#define TOOL_H

#include <QKeySequence>
#include <QObject>
#include <QString>
#include <array>
#include <chrono>
#include <tuple>

class QJsonObject;

//A Tool_output_target specifies what to do with the output of a Tool
namespace Tool_output_target {
	enum Type { ignore, paste, console, popup, replace_document };
	inline auto get_texts() {
		return std::array{QObject::tr("Ignored"), QObject::tr("Paste into editor"), QObject::tr("Display in console"), QObject::tr("Display in popup window"),
						  QObject::tr("Replace document")};
	}
} // namespace Tool_output_target

//A Tool_activation specifies when to run a tool.
namespace Tool_activation {
	enum Type { none, keyboard_shortcut, on_save_file, on_file_edit, on_build };
	inline auto get_texts() {
		return std::array{QObject::tr("None"), QObject::tr("Keyboard Shortcut"), QObject::tr("When File Saved"), QObject::tr("When File Modified"),
						  QObject::tr("When Project Built")};
	}
} // namespace Tool_activation

//A Tool specifies how to use an external tool such as a compiler. It specifies input, output, how to activate the tool and so on.
class Tool {
	public:
	QString path{};
	QString arguments{};
	QString input{};
	QString working_directory{};
	Tool_output_target::Type output{};
	Tool_output_target::Type error{};
	Tool_activation::Type activation{};
	QKeySequence activation_keyboard_shortcut{};
	std::chrono::milliseconds timeout;

	QString to_string() const;
	static Tool from_string(const QString &data);
	QString get_name() const;
};

bool operator==(const Tool &lhs, const Tool &rhs);

bool operator<(const Tool &lhs, const Tool &rhs);

#endif // TOOL_H