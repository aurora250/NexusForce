#include <NeForce/core/async/at_thread_exit.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    struct thread_exit_registry {
        at_thread_exit_elt* thread_exit_list = nullptr;

        ~thread_exit_registry();
    };

    thread_exit_registry& thread_registry() noexcept {
        thread_local thread_exit_registry thread_registry;
        return thread_registry;
    }

    thread_exit_registry::~thread_exit_registry() {
        at_thread_exit_elt* current = thread_registry().thread_exit_list;
        while (current != nullptr) {
            at_thread_exit_elt* next = current->next;
            current->cb(current);
            current = next;
        }
        thread_registry().thread_exit_list = nullptr;
    }
}


void thread_exit_register(at_thread_exit_elt* elt, void (*callback)(void*)) noexcept {
    elt->next = thread_registry().thread_exit_list;
    elt->cb = callback;
    thread_registry().thread_exit_list = elt;
}

NEFORCE_END_NAMESPACE__
