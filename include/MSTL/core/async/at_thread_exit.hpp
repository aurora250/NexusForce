#ifndef MSTL_CORE_ASYNC_AT_THREAD_EXIT_HPP__
#define MSTL_CORE_ASYNC_AT_THREAD_EXIT_HPP__
#include "../config/c++config.hpp"
MSTL_BEGIN_NAMESPACE__

struct at_thread_exit_elt {
    at_thread_exit_elt* next;
    void (*cb)(void*);
};

MSTL_BEGIN_INNER__
#ifdef MSTL_PLATFORM_WINDOWS__
MSTL_INLINE17 __declspec(thread) at_thread_exit_elt* thread_exit_list = nullptr;
#else
MSTL_INLINE17 thread_local at_thread_exit_elt* thread_exit_list = nullptr;
#endif
MSTL_END_INNER__

inline void at_thread_exit_register(at_thread_exit_elt* elt, void (*callback)(void*)) {
    elt->next = _INNER thread_exit_list;
    elt->cb = callback;
    _INNER thread_exit_list = elt;
}

inline void execute_thread_exit_callbacks() {
    at_thread_exit_elt* current = _INNER thread_exit_list;
    while (current != nullptr) {
        at_thread_exit_elt* next = current->next;
        current->cb(current);
        current = next;
    }
    _INNER thread_exit_list = nullptr;
}


class thread_exit_registry {
public:
    thread_exit_registry() = default;
    ~thread_exit_registry() {
        execute_thread_exit_callbacks();
    }

    thread_exit_registry(const thread_exit_registry&) = delete;
    thread_exit_registry& operator=(const thread_exit_registry&) = delete;
};


MSTL_BEGIN_INNER__
#ifdef MSTL_PLATFORM_WINDOWS__
MSTL_INLINE17 __declspec(thread) thread_exit_registry* thread_registry = nullptr;
#else
MSTL_INLINE17 thread_local thread_exit_registry* thread_registry = nullptr;
#endif

inline void ensure_thread_registry() {
    if (thread_registry == nullptr) {
        thread_registry = new thread_exit_registry();
    }
}
MSTL_END_INNER__


inline void at_thread_exit_register(at_thread_exit_elt* elt) {
    _INNER ensure_thread_registry();
    at_thread_exit_register(elt, elt->cb);
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ASYNC_AT_THREAD_EXIT_HPP__
