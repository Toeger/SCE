#ifndef TEST_H
#define TEST_H

#include <cassert>
#include <utility>

//run all tests
void test();

//helper for being able to see values when debugging assertion fail
template <class T, class U>
void assert_equal([[maybe_unused]] const T &t, [[maybe_unused]] const U &u) {
	assert(t == u);
}
template <class T, class U>
void assert_not_equal([[maybe_unused]] const T &t, [[maybe_unused]] const U &u) {
	using namespace std::rel_ops;
	assert(t != u);
}
template<class T>
void assert_true([[maybe_unused]] const T &t){
	assert(t);
}

#endif