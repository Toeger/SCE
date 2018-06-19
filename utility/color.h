#ifndef COLOR_H
#define COLOR_H

namespace Color {
	constexpr auto no_color = "\033[0m";
	constexpr auto black = "\033[0;30m";
	constexpr auto red = "\033[0;31m";
	constexpr auto green = "\033[0;32m";
	constexpr auto yellow = "\033[0;33m";
	constexpr auto blue = "\033[0;34m";
	constexpr auto magenta = "\033[0;35m";
	constexpr auto cyan = "\033[0;36m";
	constexpr auto white = "\033[0;37m";
	constexpr auto light_black = "\033[0;90m";
	constexpr auto light_red = "\033[0;91m";
	constexpr auto light_green = "\033[0;92m";
	constexpr auto light_yellow = "\033[0;93m";
	constexpr auto light_blue = "\033[0;94m";
	constexpr auto light_magenta = "\033[0;95m";
	constexpr auto light_cyan = "\033[0;96m";
	constexpr auto light_white = "\033[0;97m";
} // namespace Color

#endif