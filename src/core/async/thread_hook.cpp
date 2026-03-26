#include <NeForce/core/async/mutex.hpp>
#include <NeForce/core/async/thread.hpp>
#include <NeForce/core/async/thread_hook.hpp>
#include <NeForce/core/container/vector.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    vector<thread_hook::callback_t>& thread_hook_hooks() {
        static vector<thread_hook::callback_t> hooks;
        return hooks;
    }

    mutex& thread_hook_mutex() {
        static mutex mtx;
        return mtx;
    }
}


void thread_hook::add_hook(callback_t hook) {
    lock<mutex> lock(thread_hook_mutex());
    thread_hook_hooks().push_back(hook);
}

void thread_hook::remove_hook(callback_t hook) {
    lock<mutex> lock(thread_hook_mutex());
    auto it = find(thread_hook_hooks().begin(), thread_hook_hooks().end(), hook);
    if (it != thread_hook_hooks().end()) thread_hook_hooks().erase(it);
}

void thread_hook::invoke(point point, const thread* th) {
    lock<mutex> lock(thread_hook_mutex());
    for (auto& hook : thread_hook_hooks()) {
        hook(point, th);
    }
}

NEFORCE_END_NAMESPACE__
