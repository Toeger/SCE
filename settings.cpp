#include "settings.h"

#include <QSettings>

QVariant Settings::get(const QString &key, QVariant default_value) {
	return QSettings{}.value(key, default_value);
}

void Settings::set(const QString &key, const QVariant &value) {
	QSettings{}.setValue(key, value);
}
