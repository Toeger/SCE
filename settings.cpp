#include "settings.h"

#include <QSettings>
#include <algorithm>
#include <iterator>
#include <vector>

QVariant Settings::get(const QString &key, QVariant default_value) {
	return QSettings{}.value(key, default_value);
}

void Settings::set(const QString &key, const QVariant &value) {
	QSettings{}.setValue(key, value);
}

Settings::Keeper::Keeper() {
	QSettings settings{};
	keys = settings.allKeys();
	std::sort(std::begin(keys), std::end(keys));
	values.reserve(keys.size());
	for (const auto &key : keys) {
		values.push_back(settings.value(key));
	}
}

Settings::Keeper::~Keeper() {
	QSettings settings{};
	auto old_keys = settings.allKeys();
	std::sort(std::begin(old_keys), std::end(old_keys));
	//remove new keys
	std::vector<QString> difference;
	std::set_difference(std::begin(old_keys), std::end(old_keys), std::begin(keys), std::end(keys), std::back_inserter(difference));
	for (const auto &key : difference) {
		settings.remove(key);
	}
	//set old keys
	for (int i = 0; i < keys.size(); i++) {
		settings.setValue(keys[i], values[i]);
	}
}
