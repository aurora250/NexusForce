#include <MSTL/core/async/at_thread_exit.hpp>
MSTL_BEGIN_NAMESPACE__

struct thread_exit_registry {
    at_thread_exit_elt* thread_exit_list = nullptr;

    ~thread_exit_registry() {
        execute_thread_exit_callbacks();
    }
};

static thread_local thread_exit_registry thread_registry;

void at_thread_exit_register(at_thread_exit_elt* elt, void (*callback)(void*)) {
    elt->next = thread_registry.thread_exit_list;
    elt->cb = callback;
    thread_registry.thread_exit_list = elt;
}

void execute_thread_exit_callbacks() {
    at_thread_exit_elt* current = thread_registry.thread_exit_list;
    while (current != nullptr) {
        at_thread_exit_elt* next = current->next;
        current->cb(current);
        current = next;
    }
    thread_registry.thread_exit_list = nullptr;
}

MSTL_END_NAMESPACE__
