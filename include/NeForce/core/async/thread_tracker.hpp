#ifndef NEFORCE_CORE_ASYNC_THREAD_TRACKER_HPP__
#define NEFORCE_CORE_ASYNC_THREAD_TRACKER_HPP__
#include "NeForce/core/async/atomic.hpp"
NEFORCE_BEGIN_NAMESPACE__

// only track threads constructed by neforce::thread
class thread_tracker {
private:
    static atomic<int> count_;

    friend class thread;

    static NEFORCE_ALWAYS_INLINE void on_thread_create() noexcept {
        ++count_;
    }

    static NEFORCE_ALWAYS_INLINE void on_thread_destroy() noexcept {
        --count_;
    }

    thread_tracker() {
        on_thread_create(); // main thread
    }

    ~thread_tracker() {
        on_thread_destroy(); // main thread
    }

public:
    static thread_tracker& instance() noexcept {
        static thread_tracker tracker;
        return tracker;
    }

    static NEFORCE_ALWAYS_INLINE bool is_single_threaded() noexcept {
        return instance().count_.load() == 1;
    }

    static NEFORCE_ALWAYS_INLINE int thread_count() noexcept {
        return instance().count_.load();
    }
};


NEFORCE_ALWAYS_INLINE_INLINE bool is_single_threaded() noexcept {
    return thread_tracker::is_single_threaded();
}

NEFORCE_ALWAYS_INLINE_INLINE int thread_count() noexcept {
    return thread_tracker::thread_count();
}

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_THREAD_TRACKER_HPP__
