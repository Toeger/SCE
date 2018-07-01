#include "utility.h"

std::string Utility::make_whitespaces_visible(std::string_view sv) {
	std::string retval;
	retval.reserve(sv.size());
	for (const auto character : sv) {
		switch (character) {
			case ' ':
				retval += "␠";
				break;
			case '\r':
				retval += "␍";
				break;
			case '\n':
				retval += "␊\n";
				break;
			case '\t':
				retval += "↹";
				break;
			default:
				retval += character;
		}
	}
	return retval;
}