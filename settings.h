#ifndef SETTINGS_H
#define SETTINGS_H

#include <QStringList>
#include <QVariant>
#include <vector>

namespace Settings {
	constexpr auto files = "files";
	constexpr auto font = "font";
	QVariant get(const QString &key, QVariant default_value = {});
	void set(const QString &key, const QVariant &value);
	struct Keeper { /* will save the current settings when constructed and restore them after destroyed, so that setting changes between construction
								and destruction have no effect */
		Keeper();
		~Keeper();
		QStringList keys;
		std::vector<QVariant> values;
	};
} // namespace Settings

#endif // SETTINGS_H