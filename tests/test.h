#ifndef TEST_H
#define TEST_H

#include <cassert>

//run all tests
void test();

//helper for being able to see values when debugging assertion fail
template <class T, class U>
void assert_equal([[maybe_unused]] const T &t, [[maybe_unused]] const U &u) {
	assert(t == u);
}

#endif