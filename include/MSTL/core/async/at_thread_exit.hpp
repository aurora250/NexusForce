#ifndef MSTL_CORE_ASYNC_AT_THREAD_EXIT_HPP__
#define MSTL_CORE_ASYNC_AT_THREAD_EXIT_HPP__
#include "MSTL/core/config/c++config.hpp"
MSTL_BEGIN_NAMESPACE__

struct at_thread_exit_elt {
    at_thread_exit_elt* next;
    void (*cb)(void*);
};

void MSTL_API at_thread_exit_register(at_thread_exit_elt* elt, void (*callback)(void*));

void MSTL_API execute_thread_exit_callbacks();

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ASYNC_AT_THREAD_EXIT_HPP__
