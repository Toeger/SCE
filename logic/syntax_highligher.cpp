#include "syntax_highligher.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <cassert>
#include <map>

Syntax_highligher::Syntax_highligher(QTextDocument *parent)
	: QSyntaxHighlighter{parent} {}

struct Json_object_key_value_iterator {
	QJsonObject::const_iterator iterator;
	Json_object_key_value_iterator &operator++() {
		++iterator;
		return *this;
	}
	auto operator*() {
		return std::make_pair(iterator.key(), iterator.value());
	}
	bool operator==(const Json_object_key_value_iterator &other) {
		return iterator == other.iterator;
	}
	bool operator!=(const Json_object_key_value_iterator &other) {
		return iterator != other.iterator;
	}
};

struct QJsonObject_reference_with_proper_iterators {
	const QJsonObject &object;
	Json_object_key_value_iterator begin() {
		return {object.begin()};
	}
	Json_object_key_value_iterator end() {
		return {object.end()};
	}
};

void Syntax_highligher::load_rules(QString filename) {
#define assume(X)                                                                                                                                              \
	if ((X) == false) {                                                                                                                                        \
		throw std::runtime_error("Failed loading syntax rules");                                                                                               \
	}
	QFile rules_file{filename};
	if (rules_file.open(QIODevice::ReadOnly) == false) {
		throw std::runtime_error("Failed opening syntax rules file");
	}
	auto rules = QJsonDocument::fromJson(rules_file.readAll());
	assume(rules.isNull() == false);
	assume(rules.isObject());
	auto object = rules.object();
	auto color_list = object["colors"];
	assume(color_list.isObject());

	std::map<QString, QColor> color_map;
	for (const auto &name_color : QJsonObject_reference_with_proper_iterators{color_list.toObject()}) {
		const auto &color_value = name_color.second;
		assume(color_value.isArray());
		const auto &color_array = color_value.toArray();
		assume(color_array.size() == 3);
		QColor color;
		decltype(&QColor::setRed) color_setters[] = {&QColor::setRed, &QColor::setGreen, &QColor::setBlue};
		for (int i = 0; i < 3; i++) {
			const auto &color_value = color_array[i];
			assume(color_value.isDouble());
			int color_int = color_value.toInt();
			assume(color_int >= 0 && color_int <= 255);
			(color.*color_setters[i])(color_int);
		}
		color_map[name_color.first] = color;
	}

	token_colors.clear();
	auto tokens = object["tokens"];
	assume(tokens.isArray());
	for (const auto &token : tokens.toArray()) {
		assume(token.isArray());
		const auto &token_array = token.toArray();
		assume(token_array[0].isString());
		const auto &regex = token_array[0].toString();
		token_colors.push_back({std::regex{regex.toStdString()}, QColor{Qt::black}});
		auto &token_color = token_colors.back();
		for (int i = 1; i < token_array.size(); i++) {
			assume(token_array[i].isString());
			const auto &color_name = token_array[i].toString();
			if (color_map.count(color_name)) {
				token_color.color = color_map[color_name];
				break;
			}
		}
	}
#undef assume
}

void Syntax_highligher::highlightBlock(const QString &qtext) {
	(void)qtext;
	return;
}
