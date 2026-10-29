#include <NeForce/core/async/virtual_thread.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    int64_t now_ns() noexcept { return steady_clock::now().since_epoch().count(); }
} // namespace


virtual_thread_scheduler& virtual_thread_scheduler::get_instance() {
    static virtual_thread_scheduler instance;
    return instance;
}

virtual_thread_scheduler::~virtual_thread_scheduler() { shutdown(); }

void virtual_thread_scheduler::schedule_after(coroutine_handle<> handle, const int64_t delay_ms) {
    if (!handle) {
        return;
    }
    if (delay_ms <= 0) {
        schedule(handle);
        return;
    }
    (void) timers_.insert(steady_clock::now() + milliseconds(delay_ms), reinterpret_cast<uintptr_t>(handle.address()));
    // A single worker adopts the new deadline:
    // whichever one wakes recomputes the earliest deadline from the heap,
    // so there is no need to wake every worker.
    bump_wake_word(1);
    notify_waiters(false);
}

void virtual_thread_scheduler::start_workers(size_t num_threads) {
    shutdown_.store(false, memory_order_release);
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back([this] { worker_loop(); });
    }
}

void virtual_thread_scheduler::shutdown() {
    shutdown_.store(true, memory_order_release);
    bump_wake_word(1);
    futex_notify(const_cast<platform_wait_t*>(&wake_word_), true);

    for (auto& worker: workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers_.clear();
}

bool virtual_thread_scheduler::pop_expired_timers() {
    // The lock-free hint keeps the idle path free of the timer lock;
    // only an expired deadline takes it.
    const int64_t deadline = timers_.next_deadline_ns();
    if (deadline == 0 || deadline > now_ns()) {
        return false;
    }

    heap_timer::expired_entry expired[timer_batch_size];
    const size_t count = timers_.drain_expired(steady_clock::now(), expired, timer_batch_size);
    if (count == 0) {
        return false;
    }
    for (size_t i = 0; i < count; ++i) {
        (void) ready_queue_.enqueue(coroutine_handle<>::from_address(reinterpret_cast<void*>(expired[i].payload)));
    }
    publish_ready(static_cast<int64_t>(count));
    return true;
}

void virtual_thread_scheduler::park() {
    if (pending_.load(memory_order_acquire) != 0) {
        return;
    }
    const platform_wait_t expected = atomic_load(&wake_word_, memory_order_seq_cst);
    if (pending_.load(memory_order_acquire) != 0) {
        return;
    }
    const int64_t deadline = timers_.next_deadline_ns();

    sleepers_.fetch_add(1, memory_order_seq_cst);
    if (deadline == 0) {
        futex_wait(const_cast<platform_wait_t*>(&wake_word_), expected);
    } else {
        const int64_t now = now_ns();
        if (deadline > now) {
            futex_wait_until(const_cast<platform_wait_t*>(&wake_word_), expected, true, deadline / 1000000000,
                             deadline % 1000000000, true);
        }
    }
    sleepers_.fetch_sub(1, memory_order_seq_cst);
}

void virtual_thread_scheduler::worker_loop() {
    worker_context_ = true;
    for (;;) {
        if (pending_.load(memory_order_acquire) != 0) {
            coroutine_handle<> handle;
            // One item per iteration: grabbing a batch would hand a whole block of work
            // to a single worker and destroy load balance on long tasks.
            if (ready_queue_.try_dequeue(handle)) {
                pending_.fetch_sub(1, memory_order_release);
                if (handle) {
                    handle.resume();
                }
                continue;
            }
            // The counter is ahead of the queue: another worker already took the item
            // but has not decremented yet. Spin briefly instead of sleeping.
            this_thread::relax();
            continue;
        }

        if (pop_expired_timers()) {
            continue;
        }

        if (shutdown_.load(memory_order_acquire) && timers_.empty()) {
            worker_context_ = false;
            return;
        }

        park();
    }
}

NEFORCE_END_NAMESPACE__
