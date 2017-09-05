#ifndef TEST_H
#define TEST_H

#include <cassert>

void test();

template <class T, class U>
void assert_equal(const T &t, const U &u) {
	assert(t == u);
}

#endif