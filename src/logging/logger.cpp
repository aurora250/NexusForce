#include <NeForce/logging/logger.hpp>
NEFORCE_BEGIN_NAMESPACE__

unordered_map<string, string>& mdc::storage() {
    thread_local unordered_map<string, string> m;
    return m;
}

void mdc::put(const string& key, string value) { storage()[key] = move(value); }

string mdc::get(const string& key) {
    auto& m = storage();
    auto it = m.find(key);
    return it != m.end() ? it->second : "";
}

void mdc::remove(const string& key) { storage().erase(key); }

void mdc::clear() { storage().clear(); }

bool mdc::empty() { return storage().empty(); }

unordered_map<string, string> mdc::snapshot() { return storage(); }

logger::logger(string name) :
name_(move(name)) {}

logger::~logger() {
    try {
        disable_async();
        clear_sinks();
        // NOLINTNEXTLINE(bugprone-empty-catch)
    } catch (...) {
        // ignore
    }
}

void logger::update_effective_level() {
    if (explicit_level_ == INHERIT) {
        effective_level_ = (parent_ != nullptr) ? parent_->effective_level() : log_level::INFO;
    } else {
        effective_level_ = explicit_level_;
    }

    for (auto& child: children_) {
        if (child->explicit_level_ == INHERIT) {
            child->propagate_effective_level(effective_level_);
        }
    }
}

void logger::propagate_effective_level(const log_level new_effective) {
    effective_level_ = new_effective;
    for (auto& child: children_) {
        if (child->explicit_level_ == INHERIT) {
            child->propagate_effective_level(new_effective);
        }
    }
}

void logger::register_child(shared_ptr<logger> child) {
    child->parent_ = this;
    children_.push_back(move(child));
}

void logger::set_level(const log_level level) {
    explicit_level_ = level;
    update_effective_level();
}

void logger::add_sink(shared_ptr<log_sink> sink) {
    lock<mutex> lock(sinks_mutex_);
    sinks_.push_back(move(sink));
}

void logger::clear_sinks() {
    lock<mutex> lock(sinks_mutex_);
    sinks_.clear();
}

void logger::set_filter(function<bool(const log_event&)> filter) {
    lock<mutex> lock(filter_mutex_);
    filter_ = move(filter);
}

void logger::add_context(const string& key, string value) {
    lock<mutex> lock(context_mutex_);
    auto new_data = make_shared<unordered_map<string, string>>(*context_data_);
    (*new_data)[key] = move(value);
    context_data_ = move(new_data);
}

void logger::remove_context(const string& key) {
    lock<mutex> lock(context_mutex_);
    if (context_data_->find(key) == context_data_->end()) {
        return;
    }
    auto new_data = make_shared<unordered_map<string, string>>(*context_data_);
    new_data->erase(key);
    context_data_ = move(new_data);
}

void logger::clear_context() {
    lock<mutex> lock(context_mutex_);
    context_data_ = make_shared<unordered_map<string, string>>();
}

void logger::enable_async(shared_ptr<thread_pool> pool, const size_t queue_size, const overflow_policy policy) {
    if (async_.load(memory_order_acquire)) {
        disable_async();
    }

    bounded_queue<log_event> new_queue(queue_size);
    {
        lock<mutex> lock(queue_mutex_);
        async_queue_ = move(new_queue);
    }

    overflow_.store(policy, memory_order_release);
    thread_pool_ = pool ? move(pool) : make_shared<thread_pool>();
    if (!thread_pool_->running()) {
        thread_pool_->start(2);
    }

    running_.store(true, memory_order_release);
    async_.store(true, memory_order_release);
}

void logger::disable_async() {
    if (!async_.load(memory_order_acquire)) {
        return;
    }

    running_.store(false, memory_order_release);
    queue_cv_.notify_all();

    vector<log_event> remaining;
    {
        lock<mutex> lock(queue_mutex_);
        while (!async_queue_.empty()) {
            remaining.push_back(async_queue_.pop());
        }
    }

    lock<mutex> slk(sinks_mutex_);
    for (auto& ev: remaining) {
        try {
            for (auto& sink: sinks_) {
                sink->log(ev);
            }
            // NOLINTNEXTLINE(bugprone-empty-catch)
        } catch (...) {
            // ignore
        }
    }

    drain_scheduled_.store(false, memory_order_release);
    async_.store(false, memory_order_release);

    while (drain_scheduled_.load(memory_order_acquire)) {
        this_thread::yield();
    }

    if (thread_pool_ && thread_pool_->running()) {
        thread_pool_->stop();
    }

    if (flush_requested_.exchange(false, memory_order_acq_rel)) {
        lock<mutex> fl(flush_mutex_);
        flush_cv_.notify_all();
    }
}

void logger::enqueue_async(log_event&& event) {
    unique_lock<mutex> lock(queue_mutex_);

    if (async_queue_.full()) {
        switch (overflow_.load(memory_order_acquire)) {
            case overflow_policy::block: {
                queue_cv_.wait(lock, [this] { return !async_queue_.full() || !running_.load(memory_order_acquire); });
                if (!running_.load(memory_order_acquire)) {
                    return;
                }
                break;
            }
            case overflow_policy::discard: {
                return;
            }
            case overflow_policy::overrun_oldest: {
                ignore = async_queue_.pop();
                break;
            }
        }
    }

    async_queue_.push(move(event));
    lock.unlock_quiet();

    bool expected = false;
    if (drain_scheduled_.compare_exchange_strong(expected, true, memory_order_acq_rel)) {
        thread_pool_->submit_task([this] { drain_events(); });
    }
}

void logger::submit_drain() {
    bool expected = false;
    if (drain_scheduled_.compare_exchange_strong(expected, true, memory_order_acq_rel)) {
        thread_pool_->submit_task([this] { drain_events(); });
        return;
    }
    // CAS failed — a drain is already scheduled or running.
    // If a flush is pending, submit an extra drain to avoid deadlock
    // in case the existing drain finished without seeing flush_requested_.
    if (flush_requested_.load(memory_order_acquire)) {
        thread_pool_->submit_task([this] { drain_events(); });
    }
}

void logger::drain_events() {
    if (!async_.load(memory_order_acquire)) {
        drain_scheduled_.store(false, memory_order_release);
        if (flush_requested_.exchange(false, memory_order_acq_rel)) {
            lock<mutex> fl(flush_mutex_);
            flush_cv_.notify_all();
        }
        return;
    }

    vector<log_event> events;
    bool should_flush = false;

    {
        lock<mutex> lock(queue_mutex_);
        while (!async_queue_.empty()) {
            events.push_back(async_queue_.pop());
        }

        if (flush_requested_.exchange(false, memory_order_acq_rel)) {
            should_flush = true;
        }

        if (async_queue_.empty()) {
            drain_scheduled_.store(false, memory_order_release);
        }

        queue_cv_.notify_one();
    }

    if (!events.empty()) {
        lock<mutex> sl(sinks_mutex_);
        for (const auto& ev: events) {
            try {
                for (const auto& sink: sinks_) {
                    sink->log(ev);
                }
                // NOLINTNEXTLINE(bugprone-empty-catch)
            } catch (...) {
                // ignore
            }
        }
    }

    if (should_flush) {
        lock<mutex> sl(sinks_mutex_);
        for (const auto& sink: sinks_) {
            try {
                sink->flush();
                // NOLINTNEXTLINE(bugprone-empty-catch)
            } catch (...) {
                // ignore
            }
        }
        lock<mutex> fl(flush_mutex_);
        flush_cv_.notify_all();
    }

    {
        lock<mutex> lock(queue_mutex_);
        if (!async_queue_.empty()) {
            bool expected = false;
            if (!drain_scheduled_.compare_exchange_strong(expected, true, memory_order_acq_rel)) {
                return;
            }
        } else {
            drain_scheduled_.store(false, memory_order_release);
            return;
        }
    }
    thread_pool_->submit_task([this] { drain_events(); });
}

void logger::process_event_direct(const log_event& event) {
    lock<mutex> lock(sinks_mutex_);
    for (const auto& sink: sinks_) {
        try {
            sink->log(event);
            // NOLINTNEXTLINE(bugprone-empty-catch)
        } catch (...) {
            // ignore
        }
    }
}

void logger::check_auto_flush() {
    const int64_t interval = auto_flush_ms_.load(memory_order_acquire);
    if (interval > 0) {
        const auto now = timestamp::now();
        if ((now - last_auto_flush_).value() >= interval * 1000) {
            last_auto_flush_ = now;
            flush();
        }
    }
}

void logger::log(const log_level level, string msg, const source_loc loc) {
    if (!should_log(level)) {
        return;
    }

    log_event ev;
    ev.level = level;
    ev.message = move(msg);
    ev.loc = loc;
    ev.dt = datetime::now();
    ev.thread_id = this_thread::id();
    ev.logger_name = name_;

    shared_ptr<unordered_map<string, string>> logger_ctx;
    {
        lock<mutex> lock(context_mutex_);
        logger_ctx = context_data_;
    }

    if (!mdc::empty() && logger_ctx) {
        auto merged = make_shared<unordered_map<string, string>>(*logger_ctx);
        auto mdc_snap = mdc::snapshot();
        for (auto& [k, v]: mdc_snap) {
            (*merged)[k] = move(v);
        }
        ev.context = move(merged);
    } else {
        ev.context = move(logger_ctx);
    }

    {
        lock<mutex> lock(filter_mutex_);
        if (filter_ && !filter_(ev)) {
            return;
        }
    }

    if (async_.load(memory_order_acquire)) {
        enqueue_async(move(ev));
    } else {
        process_event_direct(ev);
    }

    check_auto_flush();
}

void logger::flush() {
    if (async_.load(memory_order_acquire)) {
        int flush_iter = 0;
        for (;;) {
            vector<log_event> remaining;
            {
                lock<mutex> lock(queue_mutex_);
                while (!async_queue_.empty()) {
                    remaining.push_back(async_queue_.pop());
                }
                if (remaining.empty()) {
                    break;
                }
            }
            ++flush_iter;
            lock<mutex> slk(sinks_mutex_);
            for (auto& ev: remaining) {
                try {
                    for (auto& sink: sinks_) {
                        sink->log(ev);
                    }
                    // NOLINTNEXTLINE(bugprone-empty-catch)
                } catch (...) {
                    // ignore
                }
            }
        }
    }

    lock<mutex> slk(sinks_mutex_);
    for (const auto& sink: sinks_) {
        try {
            sink->flush();
            // NOLINTNEXTLINE(bugprone-empty-catch)
        } catch (...) {
            // ignore
        }
    }
}

logger_registry::logger_registry() {
    root_ = make_shared<logger>("root");
    loggers_["root"] = root_;
}

logger_registry& logger_registry::instance() {
    static logger_registry reg;
    return reg;
}

shared_ptr<logger> logger_registry::root_logger() { return root_; }

shared_ptr<logger> logger_registry::create_logger(const string& name) {
    auto new_logger = make_shared<logger>(name);

    const auto dot_pos = name.rfind('.');
    if (dot_pos != string::npos) {
        const string parent_name = name.substr(0, dot_pos);
        auto it = loggers_.find(parent_name);
        if (it != loggers_.end()) {
            it->second->register_child(new_logger);
        } else {
            root_->register_child(new_logger);
        }
    } else {
        root_->register_child(new_logger);
    }

    loggers_[name] = new_logger;
    return new_logger;
}

shared_ptr<logger> logger_registry::get_logger(const string& name) {
    if (name.empty() || name == "root") {
        return root_;
    }

    {
        lock<mutex> lock(mutex_);
        auto it = loggers_.find(name);
        if (it != loggers_.end()) {
            auto ptr = it->second;
            return ptr;
        }
    }

    const auto dot_pos = name.rfind('.');
    if (dot_pos != string::npos) {
        get_logger(name.substr(0, dot_pos));
    }

    lock<mutex> lock(mutex_);
    auto it = loggers_.find(name);
    if (it != loggers_.end()) {
        return it->second;
    }

    return create_logger(name);
}

void logger_registry::flush_all() {
    lock<mutex> lock(mutex_);
    for (auto& [_, log]: loggers_) {
        log->flush();
    }
}

NEFORCE_END_NAMESPACE__
