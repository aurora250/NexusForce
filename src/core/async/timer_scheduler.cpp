#include <NeForce/core/async/thread_pool.hpp>
#include <NeForce/core/async/this_thread.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    constexpr size_t dispatch_batch = 32;
}


thread_pool::timer_scheduler::timer_scheduler() { worker_ = thread(&timer_scheduler::run, this); }

thread_pool::timer_scheduler::~timer_scheduler() { stop(); }

void thread_pool::timer_scheduler::stop() {
    {
        unique_lock<mutex> lock(mutex_);
        stopped_.store(true);
        cv_.notify_one();
    }
    if (worker_.joinable()) {
        worker_.join();
    }
}

void thread_pool::timer_scheduler::run() {
    for (;;) {
        unique_lock<mutex> lock(mutex_);
        if (stopped_.load()) {
            break;
        }

        time_point expire_time;
        if (queue_.empty() || !queue_.next_deadline(expire_time)) {
            cv_.wait_for(lock, milliseconds{500}, [this] { return stopped_.load() || !queue_.empty(); });
            continue;
        }

        const auto now = clock_type::now();
        if (expire_time <= now) {
            heap_timer::expired_entry expired[dispatch_batch];
            const size_t count = queue_.drain_expired(now, expired, dispatch_batch);
            lock.unlock_quiet();
            for (size_t i = 0; i < count; ++i) {
                handler_type handler;
                {
                    unique_lock<mutex> guard{mutex_};
                    const auto it = tasks_.find(expired[i].id);
                    if (it == tasks_.end()) {
                        continue;
                    }
                    handler = it->second.handler;
                }
                if (!stopped_.load() && handler) {
                    handler();
                }
                {
                    unique_lock<mutex> guard{mutex_};
                    tasks_.erase(expired[i].id);
                }
            }
            lock.lock_quiet();
            continue;
        }

        if (spin_window_ > duration::zero() && expire_time - now <= spin_window_) {
            lock.unlock_quiet();
            while (!stopped_.load(memory_order_relaxed) && clock_type::now() < expire_time) {
                this_thread::relax();
            }
            lock.lock_quiet();
            continue;
        }

        const auto wait_target = spin_window_ > duration::zero() ? expire_time - spin_window_ : expire_time;
        cv_.wait_until(lock, wait_target, [this, wait_target] {
            if (stopped_.load(memory_order_acquire)) {
                return true;
            }
            time_point next;
            return !queue_.next_deadline(next) || next <= wait_target;
        });
    }
}

thread_pool::timer_scheduler::token thread_pool::timer_scheduler::add_task(time_point expire, handler_type&& handler) {
    unique_lock<mutex> lock(mutex_);

    time_point current;
    const bool is_earliest = !queue_.next_deadline(current) || expire < current;

    auto flag = make_shared<atomic<bool>>(false);
    auto done = make_shared<_NEFORCE promise<void>>();
    handler_type wrapped = [flag, h = move(handler), done]() {
        if (!flag->load(memory_order_acquire)) {
            h();
        }
        done->set_value();
    };

    const token id = queue_.insert(expire);
    tasks_.emplace(id, scheduled_task{move(wrapped), move(flag), move(done)});

    if (is_earliest) {
        cv_.notify_one();
    }
    lock.unlock_quiet();
    return id;
}

bool thread_pool::timer_scheduler::cancel(const token id) {
    unique_lock<mutex> lock(mutex_);

    const auto it = tasks_.find(id);
    if (it == tasks_.end()) {
        return false;
    }
    it->second.cancelled->store(true, memory_order_release);

    if (queue_.cancel(id)) {
        tasks_.erase(it);
        cv_.notify_one();
        return true;
    }

    // Already drained: the handler is running (or about to run). Wait until it finishes.
    const auto done = move(it->second.done);
    lock.unlock_quiet();
    if (done) {
        done->get_future().wait();
    }
    return true;
}

void thread_pool::timer_scheduler::cancel_all() {
    unique_lock<mutex> lock(mutex_);
    for (const auto& entry: tasks_) {
        entry.second.cancelled->store(true, memory_order_release);
    }
    queue_.clear();
    tasks_.clear();
    lock.unlock_quiet();
    cv_.notify_one();
}

size_t thread_pool::timer_scheduler::size() const {
    lock<mutex> lock(mutex_);
    return queue_.size();
}

bool thread_pool::timer_scheduler::is_pending(const token id) const {
    lock<mutex> lock(mutex_);
    return queue_.contains(id);
}

NEFORCE_END_NAMESPACE__
