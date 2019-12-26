#ifndef SETTINGS_H
#define SETTINGS_H

#include "external/TMP/traits.h"
#include "external/TMP/type_list.h"
#include "project.h"
#include "threading/gui_pointer.h"
#include "tool.h"

#include <QSettings>
#include <QStringList>
#include <QVariant>
#include <algorithm>
#include <array>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

//helpers for QSettings' persistent storage
namespace Settings {
	/* To add settings add X(TYPE, NAME) of the setting to the DECLARE_SETTINGS macro. For example add X(bool, open_fullscreen) to make that setting available.
	   If the type has a comma, for example std::map<int, QString> then use a typedef to hide the comma. Yes this is horrible, if you have a way to fix this
	   please do. If you want to add a custom add a function QVariant T::serialize(const T &t); and T T::deserialize(const QVariant &). If you can't, write a
	   wrapper. */
	using Lsp_functions_type = std::map<std::string /*feature*/, std::vector<std::string /*lsp_tool_name*/>>;
#define DECLARE_SETTINGS                                                                                                              \
	X(QStringList, files), X(int, current_file), X(QString, font), X(std::vector<Tool>, tools), X(Lsp_functions_type, lsp_functions), \
		X(QString, last_open_dialog_path), X(QString, default_build_folder), X(bool, show_project_widget_on_startup),                 \
		X(std::vector<Thread_checker<std::unique_ptr<Project>>>, projects)
	namespace Key {
		enum Key {
#define X(TYPE, NAME) NAME
			DECLARE_SETTINGS
#undef X
		};
	} // namespace Key
	constexpr std::array Key_names = {
#define X(TYPE, NAME) #NAME
		DECLARE_SETTINGS
#undef X
	};
	using Key_types = TMP::Type_list<
#define X(TYPE, NAME) TYPE
		DECLARE_SETTINGS
#undef X
		>;

	template <class T>
	constexpr auto has_serialization_functions(const T &) -> decltype((std::declval<const T>().serialize(), T::deserialize(QVariant{}), std::true_type{})) {}
	constexpr std::false_type has_serialization_functions(...);
	template <class T>
	constexpr auto has_serialization_functions_v = decltype(has_serialization_functions(std::declval<T>()))::value;

	//get and set values in a semi-type-safe manner
	template <class Return_type>
	Return_type get(const QVariant &v) {
		if constexpr (std::is_same_v<Return_type, bool>) {
			return v.toInt();
		} else if constexpr (std::is_same_v<Return_type, int>) {
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
			static_assert(std::is_same_v<typename Return_type::key_type, std::string>, "Not supporting maps with not std::string as key, please fix");
			const auto &qmap = v.toMap();
			Return_type map;
			for (auto it = qmap.constBegin(); it != qmap.constEnd(); ++it) {
				map[it.key().toStdString()] = get<typename Return_type::mapped_type>(it.value());
			}
			return map;

		} else if constexpr (TMP::is_type_specialization_v<Return_type, Thread_checker>) {
			return Return_type{get<typename TMP::get_type_specialization_t<Return_type, Thread_checker>::template at<0>>(v)};
		} else if constexpr (has_serialization_functions_v<Return_type>) {
			return Return_type::deserialize(v);
		} else if constexpr (TMP::is_type_specialization_v<Return_type, std::unique_ptr>) {
			return std::make_unique<typename Return_type::element_type>(get<typename Return_type::element_type>(v));
		} else {
			return Return_type{v};
		}
	} // namespace Settings
	template <Key::Key key, class Default_type>
	auto get(const Default_type &default_value) {
		static_assert(Key_types::contains_v<Key_types::at<key>>, "Missing code to deal with this Return_type");
		return get<Key_types::at<key>>(QSettings{}.value(Key_names[key], default_value));
	}
	template <Key::Key key>
	auto get() {
		static_assert(Key_types::contains_v<Key_types::at<key>>, "Missing code to deal with this Return_type");
		return get<Key_types::at<key>>(QSettings{}.value(Key_names[key]));
	}

	template <class T>
	QVariant to_variant(const T &t) {
		if constexpr (std::is_same_v<T, bool>) {
			return static_cast<int>(t);
		} else if constexpr (TMP::is_type_specialization_v<T, std::vector>) {
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
		} else if constexpr (TMP::is_type_specialization_v<T, Thread_checker>) {
			return to_variant(*t);
		} else if constexpr (std::is_same_v<T, Tool>) {
			return t.to_string();
		} else if constexpr (std::is_same_v<T, std::string>) {
			return QString::fromStdString(t);
		} else if constexpr (has_serialization_functions_v<T>) {
			return t.serialize();
		} else if constexpr (TMP::is_type_specialization_v<T, std::unique_ptr>) {
			return to_variant(*t);
		} else {
			return t;
		}
	}
	template <Key::Key key, class T = typename Key_types::at<key>>
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
