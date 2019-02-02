#include "tool.h"

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

#define TOOL_MEMBERS /*someone please replace this with something sane*/                                                                                       \
	X(enabled)                                                                                                                                                 \
	X(type)                                                                                                                                                    \
	X(path)                                                                                                                                                    \
	X(arguments)                                                                                                                                               \
	X(input)                                                                                                                                                   \
	X(output)                                                                                                                                                  \
	X(error)                                                                                                                                                   \
	X(activation)                                                                                                                                              \
	X(activation_keyboard_shortcut)                                                                                                                            \
	X(working_directory)                                                                                                                                       \
	X(timeout)                                                                                                                                                 \
	X(use_tty_mode)

static void write(const Tool::Tool_type &data, const QString &name, QJsonObject &json) {
	json[name] = static_cast<int>(data);
}
static void write(const QString &data, const QString &name, QJsonObject &json) {
	json[name] = data;
}
static void write(const Tool_output_target::Type &data, const QString &name, QJsonObject &json) {
	json[name] = data;
}
static void write(const Tool_activation::Type &data, const QString &name, QJsonObject &json) {
	json[name] = data;
}
static void write(const QKeySequence &data, const QString &name, QJsonObject &json) {
	json[name] = data.toString();
}
static void write(const std::chrono::milliseconds &data, const QString &name, QJsonObject &json) {
	json[name] = qint64{data.count()};
}
static void write(bool data, const QString &name, QJsonObject &json) {
	json[name] = data;
}

static void read(Tool::Tool_type &data, const QString &name, QJsonObject &json) {
	data = static_cast<Tool::Tool_type>(json[name].toInt());
}
static void read(QString &data, const QString &name, QJsonObject &json) {
	data = json[name].toString();
}
static void read(Tool_output_target::Type &data, const QString &name, QJsonObject &json) {
	data = static_cast<Tool_output_target::Type>(json[name].toInt());
}
static void read(Tool_activation::Type &data, const QString &name, QJsonObject &json) {
	data = static_cast<Tool_activation::Type>(json[name].toInt());
}
static void read(QKeySequence &data, const QString &name, QJsonObject &json) {
	data = json[name].toString();
}
static void read(std::chrono::milliseconds &data, const QString &name, QJsonObject &json) {
	data = std::chrono::milliseconds{static_cast<long>(json[name].toDouble())};
}
static void read(bool &data, const QString &name, QJsonObject &json) {
	data = json[name].toBool();
}

QString Tool::to_string() const {
	QJsonObject object;
#define X(Y) write(Y, #Y, object);
	TOOL_MEMBERS
#undef X
	QJsonDocument json{object};
	return QString::fromUtf8(json.toJson());
}

Tool Tool::from_string(const QString &data) {
	Tool tool;
	auto json = QJsonDocument::fromJson(data.toUtf8()).object();
#define X(Y) read(tool.Y, #Y, json);
	TOOL_MEMBERS
#undef X
	return tool;
}

QString Tool::get_name() const {
	return path.split('/').last();
}

//creates an std::tuple but ignores the first argument
template <class Ignored, class... Args>
static constexpr auto first_skipped_make_tuple(Ignored, Args &&... args) {
	return std::make_tuple(std::forward<Args>(args)...);
}

static constexpr auto get_members() { //this may some day be replacable with reflection
#define X(Y) , &Tool::Y
	return first_skipped_make_tuple("ignored" TOOL_MEMBERS);
#undef X
}

template <class Comparator, std::size_t... indexes>
static bool compare(const Tool &lhs, const Tool &rhs, std::index_sequence<indexes...>) {
	constexpr auto members = get_members();
	return Comparator{}(std::tie(lhs.*std::get<indexes>(members)...), std::tie(rhs.*std::get<indexes>(members)...));
}

int operator_spaceship(const Tool &lhs, const Tool &rhs) {
	if (compare<std::less<>>(lhs, rhs, std::make_index_sequence<std::tuple_size_v<decltype(get_members())>>())) {
		return -1;
	}
	if (compare<std::less<>>(rhs, lhs, std::make_index_sequence<std::tuple_size_v<decltype(get_members())>>())) {
		return 1;
	}
	return 0;
}