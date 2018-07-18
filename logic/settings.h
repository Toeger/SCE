#ifndef SETTINGS_H
#define SETTINGS_H

#include "tool.h"

#include <QSettings>
#include <QStringList>
#include <QVariant>
#include <algorithm>
#include <array>
#include <tuple>
#include <vector>

template <typename T, typename Tuple>
struct has_type;
template <typename T, typename... Us>
struct has_type<T, std::tuple<Us...>> : std::disjunction<std::is_same<T, Us>...> {};
template <class T, class... Us>
constexpr bool has_type_v = has_type<T, Us...>::value;

//helpers for QSettings' persistent storage
namespace Settings {
	/* Note: If you need to add a setting you need to do 3 things:
		1. Add a value to the Key enum.
		2. Add a name to the Key_names.
		3. Add the type of the value to Key_types.
		The indexes must match (Key_names[key] must produce the correct name for that key and tuple_element_t<key, Key_types> must give us the right type).
		TODO: Someone figure out how to do this in 1 step without overhead or ugly macros.
		If you add new types you will also need to add cases for the get and set functions in order to (de)serialize those types.
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
	using Key_types = std::tuple<QStringList /*files*/, int /*current_file*/, QString /*font*/, std::vector<Tool> /*tools*/>;

	//get and set values in a semi-type-safe manner
	template <Key::Key key, class Default_type, class Return_type = std::tuple_element_t<key, Key_types>>
	Return_type get(const Default_type &default_value) {
		static_assert(has_type_v<Return_type, Key_types>, "Missing code to deal with this Return_type");
		if constexpr (std::is_same_v<Return_type, int>) {
			return QSettings{}.value(Key_names[key], default_value).toInt();
		} else if constexpr (std::is_same_v<Return_type, QStringList>) {
			return QSettings{}.value(Key_names[key], default_value).toStringList();
		} else if constexpr (std::is_same_v<Return_type, QString>) {
			return QSettings{}.value(Key_names[key], default_value).toString();
		} else if constexpr (std::is_same_v<Return_type, std::vector<Tool>>) {
			const auto stringlist = QSettings{}.value(Key_names[key], default_value).toStringList();
			std::vector<Tool> retval;
			retval.reserve(stringlist.size());
			std::transform(std::begin(stringlist), std::end(stringlist), std::back_inserter(retval),
						   [](const QString &string) { return Tool::from_string(string); });
			return retval;
		}
	}
	template <Key::Key key, class Return_type = std::tuple_element_t<key, Key_types>>
	Return_type get() {
		return get<key>(QVariant{});
	}

	template <Key::Key key, class T = std::tuple_element_t<key, Key_types>>
	void set(const T &t) {
		if constexpr (std::is_same_v<T, std::vector<Tool>>) {
			QStringList string_list;
			for (const auto &e : t) {
				string_list << e.to_string();
			}
			QSettings{}.setValue(Key_names[key], string_list);
		} else {
			QSettings{}.setValue(Key_names[key], t);
		}
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
