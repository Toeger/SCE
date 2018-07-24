#ifndef THREAD_CHECK_H
#define THREAD_CHECK_H

#include <cassert>
#include <thread>

struct Thread_check {
	Thread_check()
		: thread_id{std::this_thread::get_id()} {}
	//set the current thread as the one that will be ckecked for in thread_check
	void set_thread() {
		thread_id = std::this_thread::get_id();
	}
	//asserts if the function was called by the same thread that called set_thread
	void thread_check() const {
		assert(thread_id == std::this_thread::get_id());
	}
	bool is_in_same_thread() const {
		return thread_id == std::this_thread::get_id();
	}

	std::thread::id thread_id;
};

#endif