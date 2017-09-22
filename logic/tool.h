#ifndef TOOL_H
#define TOOL_H

#include <QKeySequence>
#include <QString>
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

template <class Comparator, std::size_t... indexes>
bool compare(const Tool &lhs, const Tool &rhs, std::index_sequence<indexes...>) {
	constexpr auto members = Tool::get_persistent_members();
	return Comparator{}(std::tie(lhs.*std::get<indexes>(members)...), std::tie(rhs.*std::get<indexes>(members)...));
}

inline bool operator==(const Tool &lhs, const Tool &rhs) {
	return compare<std::equal_to<>>(lhs, rhs, std::make_index_sequence<std::tuple_size_v<decltype(Tool::get_persistent_members())>>());
}

inline bool operator<(const Tool &lhs, const Tool &rhs) {
	return compare<std::less<>>(lhs, rhs, std::make_index_sequence<std::tuple_size_v<decltype(Tool::get_persistent_members())>>());
}

#endif // TOOL_H