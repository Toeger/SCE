#ifndef SYNTAX_HIGHLIGHER_H
#define SYNTAX_HIGHLIGHER_H

#include <QSyntaxHighlighter>

class Syntax_highligher : public QSyntaxHighlighter {
	public:
	Syntax_highligher(QTextDocument *parent);
	void highlightBlock(const QString &text) override;
};

#endif // SYNTAX_HIGHLIGHER_H