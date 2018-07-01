#ifndef UTILITY_H
#define UTILITY_H

#include <sstream>
#include <string>
#include <string_view>

namespace Utility {
	std::string make_whitespaces_visible(std::string_view sv);

	template <class T, class = decltype(std::declval<std::ostream>() << std::declval<const T>())>
	std::string to_string(const T &t) {
		std::stringstream ss;
		ss << t;
		return ss.str();
	}
} // namespace Utility

#endif //UTILITY_H