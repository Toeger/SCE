#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>
#include <QStringList>
#include <QVariant>
#include <array>
#include <tuple>
#include <vector>

//helpers for QSettings' persistent storage
namespace Settings {
	//use these key names instead of hardcoding strings to avoid typos and having a list of all settings available
	namespace Key {
		enum Key {
			files,
			current_file,
			font,
			tools,
		};
	}
	using Key_types = std::tuple<QStringList /*files*/, int /*current_file*/, QString /*font*/, QStringList /*tools*/>;
	const std::array Key_names = {
		"files",
		"current_file",
		"font",
		"tools",
	};

	//get and set values in a semi-type-safe manner
	template <Key::Key key, class Return_type = std::tuple_element_t<key, Key_types>>
	auto get() {
		if constexpr (std::is_same<Return_type, int>::value) {
			return QSettings{}.value(Key_names[key]).toInt();
		} else if constexpr (std::is_same<Return_type, QStringList>::value) {
			return QSettings{}.value(Key_names[key]).toStringList();
		} else if constexpr (std::is_same<Return_type, QString>::value) {
			return QSettings{}.value(Key_names[key]).toString();
		}
	}
	template <Key::Key key, class Default_type, class Return_type = std::tuple_element_t<key, Key_types>>
	auto get(const Default_type &default_value) {
		if constexpr (std::is_same<Return_type, int>::value) {
			return QSettings{}.value(Key_names[key], default_value).toInt();
		} else if constexpr (std::is_same<Return_type, QStringList>::value) {
			return QSettings{}.value(Key_names[key], default_value).toStringList();
		} else if constexpr (std::is_same<Return_type, QString>::value) {
			return QSettings{}.value(Key_names[key], default_value).toString();
		}
	}

	template <Key::Key key, class T = std::tuple_element_t<static_cast<int>(key), Key_types>>
	void set(const T &t) {
		QSettings{}.setValue(Key_names[key], t);
	}

	/* Keeper saves the current settings when constructed and restore them when destroyed so that setting changes between construction and destruction have
	 * no effect */
	struct Keeper {
		Keeper();
		~Keeper();
		QStringList keys;
		std::vector<QVariant> values;
	};
} // namespace Settings

#endif // SETTINGS_H
