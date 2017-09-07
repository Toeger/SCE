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
	/* Note: If you need to add a setting you need to do 3 things:
		1. Add a value to the Key enum.
		2. Add a name to the Key_names.
		3. Add the type of the value to Key_types.
		The indexes must match (Key_names[key] must produce the correct name for that key and tuple_element_t<key, Key_types> must give us the right type).
		TODO: Someone figure out how to do this in 1 step without overhead or ugly macros.
	*/

	//use these key names instead of hardcoding strings to avoid typos and having a list of all settings available
	namespace Key {
		enum Key {
			files,
			current_file,
			font,
			tools,
		};
	}
	const std::array Key_names = {
		"files",
		"current_file",
		"font",
		"tools",
	};
	using Key_types = std::tuple<QStringList /*files*/, int /*current_file*/, QString /*font*/, QStringList /*tools*/>;

	//get and set values in a semi-type-safe manner
	template <Key::Key key, class Default_type, class Return_type = std::tuple_element_t<key, Key_types>>
	Return_type get(const Default_type &default_value) {
		if constexpr (std::is_same<Return_type, int>::value) {
			return QSettings{}.value(Key_names[key], default_value).toInt();
		} else if constexpr (std::is_same<Return_type, QStringList>::value) {
			return QSettings{}.value(Key_names[key], default_value).toStringList();
		} else if constexpr (std::is_same<Return_type, QString>::value) {
			return QSettings{}.value(Key_names[key], default_value).toString();
		} else {
			static_assert("Missing code to deal with this Return_type");
		}
	}
	template <Key::Key key, class Return_type = std::tuple_element_t<key, Key_types>>
	Return_type get() {
		return get<key>(QVariant{});
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
