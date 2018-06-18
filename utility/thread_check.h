#ifndef THREAD_CHECK_H
#define THREAD_CHECK_H

#include <cassert>
#include <thread>

struct Thread_check {
    Thread_check()
        : id{std::this_thread::get_id()} {}
    //set the current thread as the one that will be ckecked for in thread_check
    void set_thread() {
        id = std::this_thread::get_id();
    }
    //checks if the function was called by the same thread that called set_thread
    void thread_check() const {
        assert(id == std::this_thread::get_id());
    }

    std::thread::id id;
};

#endif