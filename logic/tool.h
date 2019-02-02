#ifndef TOOL_H
#define TOOL_H

#include <QKeySequence>
#include <QObject>
#include <QString>
#include <array>
#include <chrono>

class QJsonObject;

namespace Tool_output_target { //what to do with the output of a tool
	enum Type { ignore, paste, console, popup, replace_document };
	inline auto get_texts() {
		return std::array{QObject::tr("Ignored"), QObject::tr("Paste into editor"), QObject::tr("Display in console"), QObject::tr("Display in popup window"),
						  QObject::tr("Replace document")};
	}
} // namespace Tool_output_target

namespace Tool_activation { //when to run a tool
	enum Type { none, keyboard_shortcut, on_save_file, on_file_edit, on_build };
	inline auto get_texts() {
		return std::array{QObject::tr("None"), QObject::tr("Keyboard Shortcut"), QObject::tr("When File Saved"), QObject::tr("When File Modified"),
						  QObject::tr("When Project Built")};
	}
} // namespace Tool_activation

struct Tool { //how to use an external tool such as a compiler. It specifies input, output, how to activate the tool and so on.
	enum class Tool_type { Generic, LSP_server } type{};
	QString path{};
	QString arguments{};
	QString input{};
	QString working_directory{};
	Tool_output_target::Type output{};
	Tool_output_target::Type error{};
	Tool_activation::Type activation{};
	QKeySequence activation_keyboard_shortcut{};
	std::chrono::milliseconds timeout{};
	bool use_tty_mode = true;
	bool enabled = true;

	QString to_string() const;
	static Tool from_string(const QString &data);
	QString get_name() const;
};

int operator_spaceship(const Tool &lhs, const Tool &rhs); //TODO: fixme in C++20

inline bool operator==(const Tool &lhs, const Tool &rhs) {
	return operator_spaceship(lhs, rhs) == 0;
}
inline bool operator!=(const Tool &lhs, const Tool &rhs) {
	return operator_spaceship(lhs, rhs) != 0;
}
inline bool operator<(const Tool &lhs, const Tool &rhs) {
	return operator_spaceship(lhs, rhs) < 0;
}
inline bool operator>(const Tool &lhs, const Tool &rhs) {
	return operator_spaceship(lhs, rhs) > 0;
}
inline bool operator<=(const Tool &lhs, const Tool &rhs) {
	return operator_spaceship(lhs, rhs) <= 0;
}
inline bool operator>=(const Tool &lhs, const Tool &rhs) {
	return operator_spaceship(lhs, rhs) >= 0;
}

#endif // TOOL_H