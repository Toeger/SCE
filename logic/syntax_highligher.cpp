#include "syntax_highligher.h"

#include <QString>

Syntax_highligher::Syntax_highligher(QTextDocument *parent)
	: QSyntaxHighlighter{parent} {}

void Syntax_highligher::highlightBlock(const QString &text) {
	QTextCharFormat format;
	format.setFontItalic(true);
	format.setForeground(Qt::blue);
	const auto term = "if";
	const auto term_size = 2;
	for (int pos = text.indexOf(term); pos != -1; pos = text.indexOf(term, pos + term_size)) {
		setFormat(pos, term_size, format);
	}
}
