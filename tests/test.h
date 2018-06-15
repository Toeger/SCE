#ifndef TEST_H
#define TEST_H

#include <algorithm>
#include <cassert>
#include <iostream>
#include <sstream>
#include <string_view>
#include <utility>

#include "utility/color.h"

namespace detail {
	extern thread_local std::stringstream assert_message;
	extern thread_local bool expect_fail;
	static auto &out = std::cerr;
	template <class T>
	constexpr auto is_printable(const T &t) -> decltype((out << t, std::true_type{}));
	constexpr auto is_printable(...) -> std::false_type;
	template <class T>
	constexpr bool is_printable_v = decltype(is_printable(std::declval<T>()))::value;

	template <class T>
	decltype(auto) as_printable(const T &t) {
		if constexpr (is_printable_v<T>) {
			return t;
		} else {
			return &t;
		}
	}

	inline void replace_all(std::string &str, std::string_view pattern, std::string_view replacement) {
		std::size_t newline_pos = 0;
		while ((newline_pos = str.find(pattern, newline_pos)) != str.npos) {
			str.replace(newline_pos, pattern.size(), replacement);
		}
	}

	template <class T>
	std::string as_string(const T &t) {
		std::stringstream ss;
		if constexpr (is_printable_v<T>) {
			ss << t;
			auto str = std::move(ss).str();
			replace_all(str, "\n", "\\n");
			replace_all(str, "\r", "\\r");
			replace_all(str, "\t", "\\t");
			return str;
		} else {
			ss << &t;
			return std::move(ss).str();
		}
	}

	template <class T, class U>
	void report_assert_failure(std::string_view function, const T &t, const U &u) {
		out << assert_message.str() << '\n';
		if (expect_fail) {
			out << Color::yellow << function << " failed as expected:\n" << Color::no_color;
		} else {
			out << Color::red << function << " unexpectedly failed:\n" << Color::no_color;
		}
		const auto ts = as_string(t);
		const auto us = as_string(u);
		out << "t: \"" << ts << "\"\n";
		out << "u: \"" << us << "\"\n";
		const auto mismatch_it = std::mismatch(std::begin(ts), std::end(ts), std::begin(us), std::end(us));
		const auto mismatch_pos = mismatch_it.first != std::end(ts) ? mismatch_it.first - std::begin(ts) :
																	  mismatch_it.second != std::end(us) ? mismatch_it.second - std::begin(us) : -1;
		if (mismatch_pos != -1) {
			out << std::string(mismatch_pos + 4, ' ') << Color::red << "^\n" << Color::no_color;
		}
		if (expect_fail == false) {
			out << std::flush;
			std::cout << std::flush;
			std::cerr << std::flush;
			__builtin_trap();
			std::terminate();
		}
	}

	template <class T>
	void report_assert_failure(std::string_view function, const T &t) {
		out << assert_message.str();
		out << Color::red << function << " failed:\n" << Color::no_color;
		out << "t: " << as_printable(t) << '\n';
		if (expect_fail == false) {
			out << std::flush;
			std::cout << std::flush;
			std::cerr << std::flush;
			__builtin_trap();
			std::terminate();
		}
	}
} // namespace detail

//helper to let you set messages that will be printed when an assertion fails
struct Assert_message {
	template <class T>
	Assert_message &operator<<(T &&t) {
		detail::assert_message << t;
		return *this;
	}
	template <class T>
	Assert_message &operator+=(T &&t) {
		detail::assert_message << t;
		return *this;
	}
	void expect_test_fail(bool expect_fail = true) {
		detail::expect_fail = expect_fail;
	}
	~Assert_message() {
		detail::assert_message.str("");
		detail::expect_fail = false;
	}
};

//helper for being able to see values when debugging assertion fail
template <class T, class U>
void assert_equal(const T &t, const U &u) {
	if (t == u) {
		return;
	}
	detail::report_assert_failure(__PRETTY_FUNCTION__, t, u);
}
template <class T, class U>
void assert_not_equal(const T &t, const U &u) {
	using namespace std::rel_ops;
	if (t != u) {
		return;
	}
	detail::report_assert_failure(__PRETTY_FUNCTION__, t, u);
}
template <class T>
void assert_true(const T &t) {
	if (t) {
		return;
	}
	detail::report_assert_failure(__PRETTY_FUNCTION__, t);
}

#endif