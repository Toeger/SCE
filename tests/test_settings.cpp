#include "TMP/type_list.h"
#include "logic/settings.h"
#include "test.h"

#include <QSettings>
#include <QVariant>
#include <cassert>
#include <vector>

TEST_CASE("Testing settings", "[settings]") {
	WHEN("Checking that key names are unique") {
		auto names = Settings::Key_names;
		std::sort(std::begin(names), std::end(names));
		REQUIRE(std::adjacent_find(std::begin(names), std::end(names)) == std::end(names));
	}
	WHEN("Checking that the number of key names matches the number of key types") {
		REQUIRE(Settings::Key_names.size() == Settings::Key_types::size);
	}
	WHEN("Testing the settings keeper") {
		auto get_sorted_keys = [] {
			auto keys = QSettings{}.allKeys();
			std::sort(std::begin(keys), std::end(keys));
			return keys;
		};
		const auto keys = get_sorted_keys();
		std::vector<QVariant> values{};
		values.reserve(keys.size());
		std::transform(std::begin(keys), std::end(keys), std::back_inserter(values), [](const QString &key) { return QSettings{}.value(key); });
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
		CHECK_FALSE(QSettings{}.allKeys().contains(test_key_add));						 //make sure added keys are removed
		CHECK(QSettings{}.allKeys().contains(test_key_remove));							 //make sure removed keys are restored
		REQUIRE(QSettings{}.value(test_key_remove).toString() == test_key_remove_value); //with the right value
		QSettings{}.remove(test_key_remove);
		REQUIRE(QSettings{}.value(test_key_modify).toString() == test_key_modify_value); //make sure modified keys are restored to their original value
		QSettings{}.remove(test_key_modify);
		REQUIRE(QSettings{}.allKeys().size() == keys.size()); //make sure we have the same size we started with
		const auto new_keys = get_sorted_keys();
		REQUIRE(keys == new_keys); //make sure all keys are the same
		REQUIRE(keys.size() == static_cast<int>(values.size()));
		for (int key_index = 0; key_index < keys.size(); key_index++) {
			REQUIRE(QSettings{}.value(keys[key_index]) == values[key_index]); //make sure the values are the same
		}
	}
}