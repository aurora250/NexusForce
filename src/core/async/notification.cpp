#include <NeForce/core/async/notification.hpp>
NEFORCE_BEGIN_NAMESPACE__

void notification::wait() const {
    if (notified()) {
        return;
    }

    unique_lock<mutex> lock(mutex_);
    cv_.wait(lock, [this]() { return notified_yet_.load(memory_order_acquire); });
}

bool notification::wait_for(milliseconds rest) const {
    if (notified()) {
        return true;
    }

    unique_lock<mutex> lock(mutex_);
    return cv_.wait_for(lock, rest, [this]() { return notified_yet_.load(memory_order_acquire); });
}

bool notification::wait_until(system_clock::time_point util) const {
    if (notified()) {
        return true;
    }

    unique_lock<mutex> lock(mutex_);
    return cv_.wait_until(lock, util, [this]() { return notified_yet_.load(memory_order_acquire); });
}

void notification::notify() {
#ifdef NEFORCE_STATE_DEBUG
    if (notified_yet_.load(memory_order_relaxed)) {
        NEFORCE_THROW_EXCEPTION(exception("Notify() called more than once"));
    }
#endif

    notified_yet_.store(true, memory_order_release);

    lock<mutex> lock(mutex_);
    cv_.notify_all();
}

NEFORCE_END_NAMESPACE__
