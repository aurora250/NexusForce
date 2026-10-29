#include <NeForce/core/async/tree_timer.hpp>
NEFORCE_BEGIN_NAMESPACE__

tree_timer_scheduler::tree_timer_scheduler() { thread_ = thread(&tree_timer_scheduler::run, this); }

tree_timer_scheduler::~tree_timer_scheduler() { stop(); }

void tree_timer_scheduler::stop() {
    {
        unique_lock<mutex> lock(mutex_);
        stopped_.store(true);
        cv_.notify_one();
    }
    if (thread_.joinable()) {
        thread_.join();
    }
}

void tree_timer_scheduler::run() {
    while (!stopped_.load()) {
        unique_lock<mutex> lock(mutex_);

        if (nodes_.empty()) {
            cv_.wait_for(lock, 500_ms, [this] { return stopped_.load() || !nodes_.empty(); });
            if (stopped_.load()) {
                break;
            }
        }

        time_point now = clock_type::now();
        while (!nodes_.empty() && nodes_.begin()->expire <= now) {
            auto it = nodes_.begin();
            // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
            node current_node = *it;
            nodes_.erase(it);
            node_map_.erase(current_node.id);

            auto flag_it = cancel_flags_.find(current_node.id);
            const bool cancelled = (flag_it != cancel_flags_.end() && flag_it->second->load(memory_order_acquire));

            lock.unlock_quiet();
            if (!stopped_.load() && !cancelled) {
                current_node.handler();
            }
            lock.lock_quiet();

            cancel_flags_.erase(current_node.id);
            promises_.erase(current_node.id);
            now = clock_type::now();
        }

        if (!nodes_.empty()) {
            const auto expire_time = nodes_.begin()->expire;

            if (spin_window_ > duration::zero() && expire_time - clock_type::now() <= spin_window_) {
                lock.unlock_quiet();
                while (!stopped_.load(memory_order_relaxed) && clock_type::now() < expire_time) {
                    this_thread::relax();
                }
                lock.lock_quiet();
                continue;
            }

            const auto wait_target = spin_window_ > duration::zero() ? expire_time - spin_window_ : expire_time;
            cv_.wait_until(lock, wait_target, [this, wait_target] {
                return stopped_.load(memory_order_acquire) || nodes_.empty() || nodes_.begin()->expire <= wait_target;
            });
        }
    }
}

tree_timer_scheduler::token tree_timer_scheduler::add_task(time_point expire, handler_type&& handler) {
    unique_lock<mutex> lock(mutex_);
    const token id = next_id_++;

    auto flag = make_shared<atomic<bool>>(false);
    auto promise = make_shared<_NEFORCE promise<void>>();
    handler_type wrapped = [flag, h = move(handler), promise]() {
        if (!flag->load(memory_order_acquire)) {
            h();
        }
        promise->set_value();
    };

    const bool is_earliest = nodes_.empty() || expire < nodes_.begin()->expire;

    node new_node(expire, id, move(wrapped));
    const auto result = nodes_.insert(move(new_node));
    node_map_[id] = result.first;
    cancel_flags_[id] = move(flag);
    promises_[id] = promise;

    if (is_earliest) {
        cv_.notify_one();
    }
    lock.unlock_quiet();
    return id;
}

bool tree_timer_scheduler::cancel(const token id) {
    unique_lock<mutex> lock(mutex_);

    const auto flag_it = cancel_flags_.find(id);
    if (flag_it == cancel_flags_.end()) {
        return false;
    }
    flag_it->second->store(true, memory_order_release);

    const auto node_it = node_map_.find(id);
    if (node_it != node_map_.end()) {
        const bool is_earliest = (node_it->second == nodes_.begin());
        nodes_.erase(node_it->second);
        node_map_.erase(node_it);

        if (is_earliest) {
            cv_.notify_one();
        }

        cancel_flags_.erase(flag_it);
        promises_.erase(id);
    } else {
        const auto prom_it = promises_.find(id);
        shared_ptr<promise<void>> prom;
        if (prom_it != promises_.end()) {
            prom = prom_it->second;
            promises_.erase(prom_it);
        }
        lock.unlock_quiet();

        if (prom) {
            prom->get_future().wait();
        }
    }

    return true;
}

void tree_timer_scheduler::cancel_all() {
    unique_lock<mutex> lock(mutex_);
    for (const auto& cancel_flag: cancel_flags_) {
        cancel_flag.second->store(true, memory_order_release);
    }
    nodes_.clear();
    node_map_.clear();
    cancel_flags_.clear();
    lock.unlock_quiet();
    cv_.notify_one();
}

size_t tree_timer_scheduler::size() const {
    lock<mutex> lock(mutex_);
    return nodes_.size();
}

bool tree_timer_scheduler::is_pending(const token id) const {
    lock<mutex> lock(mutex_);
    return node_map_.find(id) != node_map_.end();
}

NEFORCE_END_NAMESPACE__
