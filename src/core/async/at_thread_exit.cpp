#include <NeForce/core/async/at_thread_exit.hpp>
#include <NeForce/core/exception/terminate.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    struct thread_exit_registry {
        at_thread_exit_elt* thread_exit_list = nullptr;

        thread_exit_registry() noexcept = default;
        thread_exit_registry(const thread_exit_registry&) = delete;
        thread_exit_registry& operator=(const thread_exit_registry&) = delete;
        thread_exit_registry(thread_exit_registry&&) = delete;
        thread_exit_registry& operator=(thread_exit_registry&&) = delete;

        ~thread_exit_registry() {
            try {
                at_thread_exit_elt* current = thread_exit_list;
                while (current != nullptr) {
                    at_thread_exit_elt* next = current->next;
                    current->cb(current);
                    current = next;
                }
            } catch (...) {
                terminate();
            }
        }
    };

    thread_exit_registry& thread_registry() noexcept {
        thread_local thread_exit_registry thread_registry;
        return thread_registry;
    }
} // namespace


void thread_exit_register(at_thread_exit_elt* elt, void (*callback)(void*)) noexcept {
    elt->next = thread_registry().thread_exit_list;
    elt->cb = callback;
    thread_registry().thread_exit_list = elt;
}

NEFORCE_END_NAMESPACE__
