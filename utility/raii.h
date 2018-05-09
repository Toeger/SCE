#ifndef RAII_H
#define RAII_H

#include <utility>

template <class Init_function, class Exit_function>
struct RAII {
    static void do_nothing() {}
    RAII(Init_function init_function, Exit_function exit_function)
        : exit_function{std::move(exit_function)} {
        init_function();
    }
    RAII(Exit_function exit_function)
        : RAII(do_nothing, std::move(exit_function)) {}
    ~RAII() {
        exit_function();
    }

    private:
    Exit_function exit_function;
};

template <class Init_function, class Exit_function>
RAII(Init_function, Exit_function)->RAII<Init_function, Exit_function>;
template <class Exit_function>
RAII(Exit_function)->RAII<void(), Exit_function>;

#endif