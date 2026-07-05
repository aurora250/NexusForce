#include <NeForce/core/async/thread_exit_notifier.hpp>
#include <NeForce/core/async/mutex.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    mutex& mutex_inst() noexcept {
        static mutex mtx;
        return mtx;
    }
} // namespace


thread_exit_notifier::~thread_exit_notifier() {
    lock<mutex> guard(mutex_inst());
    for (auto* ptr = tail_; ptr != nullptr; ptr = ptr->next) {
        ptr->chain = nullptr;
        ptr->callback(ptr->user_data);
    }
}

thread_exit_notifier& thread_exit_notifier::instance() noexcept {
    thread_local thread_exit_notifier notifier;
    return notifier;
}

void thread_exit_notifier::subscribe(thread_exit_listener* listener) {
    auto& tls = instance();
    lock<mutex> guard(mutex_inst());
    listener->next = tls.tail_;
    listener->chain = &tls;
    tls.tail_ = listener;
}

void thread_exit_notifier::unsubscribe(thread_exit_listener* listener) {
    lock<mutex> guard(mutex_inst());
    if (listener->chain == nullptr) {
        return;
    }
    auto& tls = *static_cast<thread_exit_notifier*>(listener->chain);
    listener->chain = nullptr;
    thread_exit_listener** prev = &tls.tail_;
    for (auto* ptr = tls.tail_; ptr != nullptr; ptr = ptr->next) {
        if (ptr == listener) {
            *prev = ptr->next;
            break;
        }
        prev = &ptr->next;
    }
}

NEFORCE_END_NAMESPACE__
