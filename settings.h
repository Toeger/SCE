#ifndef SETTINGS_H
#define SETTINGS_H

#include <QVariant>

namespace Settings
{
	constexpr auto files = "files";
	constexpr auto font = "font";
	QVariant get(const QString &key, QVariant default_value = {});
	void set(const QString &key, const QVariant &value);
}

#endif // SETTINGS_H