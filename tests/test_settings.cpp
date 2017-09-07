#include "test_settings.h"
#include "logic/settings.h"
#include "test.h"

#include <QSettings>
#include <QVariant>
#include <cassert>
#include <vector>

void test_unique_key_names() {
	auto names = Settings::Key_names;
	std::sort(std::begin(names), std::end(names));
	assert_equal(std::adjacent_find(std::begin(names), std::end(names)), std::end(names));
}

void test_name_type_length_match() {
	assert_equal(Settings::Key_names.size(), std::tuple_size<Settings::Key_types>::value);
}

void test_keeper() {
	auto get_sorted_keys = [] {
		auto keys = QSettings{}.allKeys();
		std::sort(std::begin(keys), std::end(keys));
		return keys;
	};
	const auto keys = get_sorted_keys();
	std::vector<QVariant> values{};
	values.reserve(keys.size());
	for (const auto &key : keys) {
		values.push_back(QSettings{}.value(key));
	}
	constexpr auto test_key_add = "Test_key1";
	constexpr auto test_key_remove = "Test_key2";
	constexpr auto test_key_modify = "Test_key3";
	constexpr auto test_key_remove_value = "remove key value";
	constexpr auto test_key_modify_value = "modify key value";
	QSettings{}.setValue(test_key_remove, test_key_remove_value);
	QSettings{}.setValue(test_key_modify, test_key_modify_value);
	{
		Settings::Keeper keeper{};
		QSettings{}.remove(test_key_remove);
		QSettings{}.setValue(test_key_add, "");
		QSettings{}.setValue(test_key_modify_value, "something");
	}
	assert_equal(QSettings{}.allKeys().contains(test_key_add), false);                  //make sure added keys are removed
	assert_equal(QSettings{}.allKeys().contains(test_key_remove), true);                //make sure removed keys are restored
	assert_equal(QSettings{}.value(test_key_remove).toString(), test_key_remove_value); //with the right value
	QSettings{}.remove(test_key_remove);                                                //
	assert_equal(QSettings{}.value(test_key_modify).toString(), test_key_modify_value); //make sure modified keys are restored to their original value
	QSettings{}.remove(test_key_modify);                                                //
	assert_equal(QSettings{}.allKeys().size(), keys.size());                            //make sure we have the same size we started with
	const auto new_keys = get_sorted_keys();                                            //
	assert_equal(keys, new_keys);                                                       //make sure all keys are the same
	for (int key_index = 0; key_index < keys.size(); key_index++) {                     //
		assert_equal(QSettings{}.value(keys[key_index]), values[key_index]);            //make sure the values are the same
	}
}

void test_settings() {
	test_unique_key_names();
	test_name_type_length_match();
	test_keeper();
}
