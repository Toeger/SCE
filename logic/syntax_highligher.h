#ifndef SYNTAX_HIGHLIGHER_H
#define SYNTAX_HIGHLIGHER_H

#include <QColor>
#include <QSyntaxHighlighter>
#include <regex>

class Syntax_highligher : public QSyntaxHighlighter {
	public:
	Syntax_highligher(QTextDocument *parent);
	void load_rules(QString filename);
	void highlightBlock(const QString &text) override;

	private:
	struct Token_color {
		std::regex regex;
		QColor color;
	};
	std::vector<Token_color> token_colors;
};

#endif // SYNTAX_HIGHLIGHER_H