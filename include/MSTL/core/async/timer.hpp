#ifndef MSTL_CORE_ASYNC_TIMER_HPP__
#define MSTL_CORE_ASYNC_TIMER_HPP__
#include "../container/map.hpp"
#include "../container/set.hpp"
#include "../functional/function.hpp"
#include "thread.hpp"
#include "condition_variable.hpp"
#include "atomic.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Clock>
class timer_scheduler {
public:
    using clock_type = Clock;
    using time_point = typename clock_type::time_point;
    using duration = typename clock_type::duration;
    using token = size_t;
    using handler_type = _MSTL function<void()>;

private:
    struct node {
        time_point expire;
        token id;
        handler_type handler;

        node(time_point exp, const token tid, handler_type&& h)
            : expire(exp), id(tid), handler(_MSTL move(h)) {}

        bool operator <(const node& other) const {
            if (expire < other.expire) return true;
            if (expire > other.expire) return false;
            return id < other.id;
        }
    };

    _MSTL set<node> nodes_;
    _MSTL map<token, typename _MSTL set<node>::iterator> node_map_;

    _MSTL thread thread_;
    _MSTL mutex mutex_;
    _MSTL condition_variable cv_;
    token next_id_;
    _MSTL atomic_bool stopped_;

    friend class thread_pool;

private:
    void run() {
        while (!stopped_.load()) {
            _MSTL smart_lock<_MSTL mutex> lock(mutex_);

            if (nodes_.empty()) {
                cv_.wait(lock, [this] {
                    return stopped_.load() || !nodes_.empty();
                });
                if (stopped_.load()) break;
            }

            time_point now = clock_type::now();
            while (!nodes_.empty() && nodes_.begin()->expire <= now) {
                auto it = nodes_.begin();
                node current_node = *it;
                nodes_.erase(it);
                node_map_.erase(current_node.id);

                lock.unlock_quiet();
                if (!stopped_.load()) {
                    current_node.handler();
                }
                lock.lock_quiet();
                now = clock_type::now();
            }

            if (!nodes_.empty()) {
                time_point next_expire = nodes_.begin()->expire;
                cv_.wait_until(lock, next_expire);
            }
        }
    }

public:
    timer_scheduler() : next_id_(0), stopped_(false) {
        thread_ = _MSTL thread(&timer_scheduler::run, this);
    }

    ~timer_scheduler() {
        stopped_.store(true);
        cv_.notify_one();
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    timer_scheduler(const timer_scheduler&) = delete;
    timer_scheduler& operator =(const timer_scheduler&) = delete;
    timer_scheduler(timer_scheduler&&) = default;
    timer_scheduler& operator =(timer_scheduler&&) = default;

    token add_task(time_point expire, handler_type&& handler) {
        _MSTL smart_lock<_MSTL mutex> lock(mutex_);
        token id = next_id_++;

        const bool is_earliest = nodes_.empty() || expire < nodes_.begin()->expire;

        node new_node(expire, id, _MSTL move(handler));
        auto result = nodes_.insert(new_node);
        node_map_[id] = result.first;

        lock.unlock_quiet();

        if (is_earliest) {
            cv_.notify_one();
        }

        return id;
    }

    bool cancel(token id) {
        _MSTL smart_lock<_MSTL mutex> lock(mutex_);
        auto it_map = node_map_.find(id);
        if (it_map == node_map_.end()) {
            return false;
        }

        const bool is_earliest = (it_map->second == nodes_.begin());
        nodes_.erase(it_map->second);
        node_map_.erase(it_map);

        lock.unlock_quiet();

        if (is_earliest) {
            cv_.notify_one();
        }

        return true;
    }

    void cancel_all() {
        _MSTL smart_lock<_MSTL mutex> lock(mutex_);
        nodes_.clear();
        node_map_.clear();
        lock.unlock_quiet();
        cv_.notify_one();
    }

    MSTL_NODISCARD size_t size() const {
        _MSTL lock<_MSTL mutex> lock(const_cast<_MSTL mutex&>(mutex_));
        return nodes_.size();
    }
};


template <typename Clock>
class basic_timer {
public:
    using clock_type = Clock;
    using time_point = typename clock_type::time_point;
    using duration = typename clock_type::duration;
    using token = typename timer_scheduler<Clock>::token;
    using handler_type = typename timer_scheduler<Clock>::handler_type;

private:
    timer_scheduler<Clock> scheduler_{};
    token task_id_ = 0;
    time_point expire_ = clock_type::now();

public:
    basic_timer() = default;
    ~basic_timer() { cancel(); }

    basic_timer(const basic_timer&) = delete;
    basic_timer& operator =(const basic_timer&) = delete;

    basic_timer(basic_timer&& other) noexcept
        : scheduler_(other.scheduler_)
        , task_id_(other.task_id_)
        , expire_(other.expire_) {
        other.task_id_ = 0;
    }

    basic_timer& operator =(basic_timer&& other) noexcept {
        if (this != &other) {
            cancel();
            task_id_ = other.task_id_;
            expire_ = other.expire_;
            other.task_id_ = 0;
        }
        return *this;
    }

    void expires_at(const time_point& expiry_time) {
        cancel();
        expire_ = expiry_time;
    }

    void expires_after(const duration& expiry_duration) {
        cancel();
        expire_ = clock_type::now() + expiry_duration;
    }

    void expires_from_now(const int64_t milliseconds) {
        expires_after(_MSTL milliseconds(milliseconds));
    }

    MSTL_NODISCARD time_point expiry() const { return expire_; }
    MSTL_NODISCARD bool is_active() const { return task_id_ != 0; }

    template <typename WaitHandler>
    void async_wait(WaitHandler&& handler) {
        cancel();
        task_id_ = scheduler_.add_task(expire_,
            handler_type(_MSTL forward<WaitHandler>(handler)));
    }

    void cancel() {
        if (task_id_ != 0) {
            scheduler_.cancel(task_id_);
            task_id_ = 0;
        }
    }
};

using steady_timer = basic_timer<steady_clock>;
using system_timer = basic_timer<system_clock>;

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ASYNC_TIMER_HPP__
