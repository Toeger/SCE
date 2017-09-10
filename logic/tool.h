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
	QString working_directory{};
	Tool_output_target::Type output{};
	Tool_output_target::Type error{};
	Activation activation{}; //todo: add context menu and menu entry

	static constexpr auto get_persistent_members() { //this may some day be replacable with reflection
		return std::make_tuple(&Tool::path, &Tool::arguments, &Tool::input, &Tool::working_directory, &Tool::output, &Tool::error, &Tool::activation);
	}

	QString to_string() const;
	static Tool from_string(const QString &data);
	QString get_name() const;

	private:
	void write(const QString &data, const QString &name, QJsonObject &json) const;
	void write(const Tool_output_target::Type &data, const QString &name, QJsonObject &json) const;
	void write(const Activation &data, const QString &name, QJsonObject &json) const;
	static void read(QString &data, const QString &name, QJsonObject &json);
	static void read(Tool_output_target::Type &data, const QString &name, QJsonObject &json);
	static void read(Activation &data, const QString &name, QJsonObject &json);
};

struct Equal { //should find a way to use std::equal_to
	template <class T>
	bool operator()(const T &lhs, const T &rhs) {
		return lhs == rhs;
	}
};

struct Less { //should find a way to use std::less
	template <class T>
	bool operator()(const T &lhs, const T &rhs) {
		return lhs < rhs;
	}
};

template <class Comparer, std::size_t index = 0>
bool compare(const Tool &lhs, const Tool &rhs) {
	if constexpr (index < std::tuple_size<decltype(Tool::get_persistent_members())>()) {
		const auto members = Tool::get_persistent_members();
		if (Comparer{}(lhs.*std::get<index>(members), rhs.*std::get<index>(members))) {
			return compare<Comparer, index + 1>(lhs, rhs);
		} else {
			return false;
		}
	}
	return true;
}

inline bool operator==(const Tool &lhs, const Tool &rhs) {
	return compare<Equal>(lhs, rhs);
}

inline bool operator<(const Tool &lhs, const Tool &rhs) {
	return compare<Less>(lhs, rhs);
}

#endif // TOOL_H