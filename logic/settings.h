#ifndef SETTINGS_H
#define SETTINGS_H

#include <QStringList>
#include <QVariant>
#include <vector>

namespace Settings {
	//helpers for QSettings' persistent storage
	namespace Keys { //use these key names instead of hardcoding strings to avoid typos and having a list of all settings available
		constexpr auto files = "files";
		constexpr auto current_file = "current_file";
		constexpr auto font = "font";
	} // namespace Keys
	/* Keeper saves the current settings when constructed and restore them when destroyed so that setting changes between construction and destruction have no
	 * effect */
	struct Keeper {
		Keeper();
		~Keeper();
		QStringList keys;
		std::vector<QVariant> values;
	};
} // namespace Settings

#endif // SETTINGS_H