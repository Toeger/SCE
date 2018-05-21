#ifndef THREAD_SAFE_H
#define THREAD_SAFE_H

#include <mutex>

template <class T>
class Thread_safe {
    struct Thread_safe_wrapper {
        Thread_safe_wrapper(Thread_safe &parent)
            : ts{&parent} {
            ts->m.lock();
        }
        Thread_safe_wrapper(const Thread_safe_wrapper &) = delete;
        ~Thread_safe_wrapper() {
            ts->m.unlock();
        }
        operator T &() {
            return ts->t;
        }
        operator const T &() const {
            return ts->t;
        }
        T &get() {
            return ts->t;
        }
        const T &get() const {
            return ts->t;
        }

        private:
        Thread_safe *ts{};
    };

    public:
    template <class... Args>
    Thread_safe(Args &&... args)
        : t(args...) {}
    Thread_safe_wrapper lock() {
        return *this;
    }

    private:
    T t;
    std::mutex m;
};

#endif