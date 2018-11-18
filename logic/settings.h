#ifndef SETTINGS_H
#define SETTINGS_H

#include "external/TMP/traits.h"
#include "tool.h"

#include <QSettings>
#include <QStringList>
#include <QVariant>
#include <algorithm>
#include <array>
#include <map>
#include <string>
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
			lsp_functions,
		};
	}
	const std::array Key_names = {
		"files", "current_file", "font", "tools", "lsp_functions",
	};
	using Key_types = std::tuple<QStringList /*files*/, int /*current_file*/, QString /*font*/, std::vector<Tool> /*tools*/,
								 std::map<std::string /*feature*/, std::vector<std::string /*lsp_tool_name*/>>> /*lsp_functions*/;

	//get and set values in a semi-type-safe manner
	template <class Return_type>
	Return_type get(const QVariant &v) {
		if constexpr (std::is_same_v<Return_type, int>) {
			return v.toInt();
		} else if constexpr (std::is_same_v<Return_type, QStringList>) {
			return v.toStringList();
		} else if constexpr (std::is_same_v<Return_type, QString>) {
			return v.toString();
		} else if constexpr (std::is_same_v<Return_type, std::string>) {
			return v.toString().toStdString();
		} else if constexpr (std::is_same_v<Return_type, Tool>) {
			return Tool::from_string(v.toString());
		} else if constexpr (TMP::is_type_specialization_v<Return_type, std::vector>) {
			const auto &list = v.toList();
			Return_type retval;
			retval.reserve(list.size());
			std::transform(std::begin(list), std::end(list), std::back_inserter(retval), [](const QVariant &element) {
				return get<typename TMP::get_type_specialization_t<Return_type, std::vector>::template at<0>>(element);
			});
			return retval;
		} else if constexpr (TMP::is_type_specialization_v<Return_type, std::map>) {
			if constexpr (std::is_same_v<typename Return_type::key_type, std::string>) {
				const auto &qmap = v.toMap();
				Return_type map;
				for (auto it = qmap.constBegin(); it != qmap.constEnd(); ++it) {
					map[it.key().toStdString()] = get<typename Return_type::mapped_type>(it.value());
				}
				return map;
			} else {
				return Return_type{v};
			}
		} else {
			return Return_type{v};
		}
	}
	template <Key::Key key, class Default_type, class Return_type = std::tuple_element_t<key, Key_types>>
	Return_type get(const Default_type &default_value) {
		return get<Return_type>(QSettings{}.value(Key_names[key], default_value));
	}
	template <Key::Key key, class Return_type = std::tuple_element_t<key, Key_types>>
	Return_type get() {
		static_assert(has_type_v<Return_type, Key_types>, "Missing code to deal with this Return_type");
		return get<key>(QVariant{});
	}

	template <class T>
	QVariant to_variant(const T &t) {
		if constexpr (TMP::is_type_specialization_v<T, std::vector>) {
			QList<QVariant> list;
			for (const auto &element : t) {
				list.append(to_variant(element));
			}
			return list;
		} else if constexpr (TMP::is_type_specialization_v<T, std::map>) {
			QMap<QString, QVariant> map;
			for (auto [key, value] : t) {
				QString qkey;
				if constexpr (std::is_same_v<typename T::key_type, std::string>) {
					qkey = QString::fromStdString(key);
				} else {
					qkey = key;
				}
				map[qkey] = to_variant(value);
			}
			return map;
		} else if constexpr (std::is_same_v<T, Tool>) {
			return t.to_string();
		} else if constexpr (std::is_same_v<T, std::string>) {
			return QString::fromStdString(t);
		} else {
			return t;
		}
	}
	template <Key::Key key, class T = std::tuple_element_t<key, Key_types>>
	void set(const T &t) {
		QSettings{}.setValue(Key_names[key], to_variant(t));
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
