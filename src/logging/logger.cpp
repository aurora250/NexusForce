#include <NeForce/logging/logger.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    // How long flush() waits for the background drain task to hand the queued events to the sinks.
    // The wait is a safety net only: a healthy pool answers in microseconds.
    constexpr int64_t flush_wait_seconds = 5;
    // Re-poke interval while flush() waits for the drain task to acknowledge.
    constexpr int64_t flush_repoke_ms = 20;
} // namespace


flat_unordered_map<string, string>& mdc::storage() {
    thread_local flat_unordered_map<string, string> m;
    return m;
}

void mdc::put(const string& key, string value) { storage()[key] = move(value); }

string mdc::get(const string& key) {
    auto& m = storage();
    const auto it = m.find(key);
    return it != m.end() ? it->second : "";
}

void mdc::remove(const string& key) { storage().erase(key); }

void mdc::clear() { storage().clear(); }

bool mdc::empty() { return storage().empty(); }

flat_unordered_map<string, string> mdc::snapshot() { return storage(); }

logger::logger(string name) :
name_(move(name)) {}

logger::~logger() {
    try {
        disable_async();
        clear_sinks();
        // NOLINTNEXTLINE(bugprone-empty-catch)
    } catch (const exception& e) {
        NEFORCE_REPORT_EXCEPTION(e);
    }
}

void logger::update_effective_level() {
    const log_level explicit_level = explicit_level_.load(memory_order_acquire);
    if (explicit_level == INHERIT) {
        const logger* const parent = parent_.load(memory_order_acquire);
        effective_level_.store(parent != nullptr ? parent->effective_level() : log_level::INFO, memory_order_release);
    } else {
        effective_level_.store(explicit_level, memory_order_release);
    }

    vector<shared_ptr<logger>> snapshot;
    {
        lock<mutex> children_lock(children_mutex_);
        snapshot = children_;
    }

    const log_level effective = effective_level_.load(memory_order_acquire);
    for (const auto& child: snapshot) {
        if (child->level() == INHERIT) {
            child->propagate_effective_level(effective);
        }
    }
}

void logger::propagate_effective_level(const log_level new_effective) {
    effective_level_.store(new_effective, memory_order_release);

    vector<shared_ptr<logger>> snapshot;
    {
        lock<mutex> children_lock(children_mutex_);
        snapshot = children_;
    }

    for (const auto& child: snapshot) {
        if (child->level() == INHERIT) {
            child->propagate_effective_level(new_effective);
        }
    }
}

void logger::register_child(shared_ptr<logger> child) {
    child->parent_.store(this, memory_order_release);
    child->update_effective_level();
    lock<mutex> children_lock(children_mutex_);
    children_.push_back(move(child));
}

void logger::set_level(const log_level level) {
    explicit_level_.store(level, memory_order_release);
    update_effective_level();
}

void logger::add_sink(shared_ptr<log_sink> sink) {
    lock<mutex> sinks_lock(sinks_mutex_);
    sinks_.push_back(move(sink));
}

void logger::clear_sinks() {
    lock<mutex> sinks_lock(sinks_mutex_);
    sinks_.clear();
}

void logger::set_filter(function<bool(const log_event&)> filter) {
    lock<mutex> filter_lock(filter_mutex_);
    filter_ = move(filter);
}

void logger::add_context(const string& key, string value) {
    lock<mutex> context_lock(context_mutex_);
    auto new_data = make_shared<flat_unordered_map<string, string>>(*context_data_);
    (*new_data)[key] = move(value);
    context_data_ = move(new_data);
}

void logger::remove_context(const string& key) {
    lock<mutex> context_lock(context_mutex_);
    if (context_data_->find(key) == context_data_->end()) {
        return;
    }
    auto new_data = make_shared<flat_unordered_map<string, string>>(*context_data_);
    new_data->erase(key);
    context_data_ = move(new_data);
}

void logger::clear_context() {
    lock<mutex> context_lock(context_mutex_);
    context_data_ = make_shared<flat_unordered_map<string, string>>();
}

void logger::enable_async(shared_ptr<thread_pool> pool, const size_t queue_size, const overflow_policy policy) {
    if (async_.load(memory_order_acquire)) {
        disable_async();
    }

    {
        lock<mutex> queue_lock(queue_mutex_);
        async_queue_ = bounded_queue<log_event>(queue_size == 0 ? 1 : queue_size);
        drain_scheduled_.store(false, memory_order_release);
        flush_requested_.store(false, memory_order_release);
    }
    overflow_.store(policy, memory_order_release);

    const bool own_pool = pool == nullptr;
    thread_pool_ = own_pool ? make_shared<thread_pool>() : move(pool);
    owns_pool_.store(own_pool, memory_order_release);

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
    async_.store(false, memory_order_release);
    queue_cv_.notify_all();

    // Join the drain task before touching the queue or the sinks, and hold no lock while doing so:
    // a running drain still needs queue_mutex_ and sinks_mutex_,
    // and the pool can only stop once that task has returned - waiting for it with sinks_mutex_ held deadlocks.
    if (thread_pool_ && thread_pool_->running()) {
        if (owns_pool_.load(memory_order_acquire)) {
            thread_pool_->stop();
        } else {
            while (drain_scheduled_.load(memory_order_acquire)) {
                this_thread::yield();
            }
        }
    }

    vector<log_event> remaining;
    {
        lock<mutex> queue_lock(queue_mutex_);
        while (!async_queue_.empty()) {
            remaining.push_back(async_queue_.pop());
        }
        drain_scheduled_.store(false, memory_order_release);
        queue_cv_.notify_all();
    }

    write_events(remaining);

    if (flush_requested_.exchange(false, memory_order_acq_rel)) {
        lock<mutex> flush_lock(flush_mutex_);
        flush_cv_.notify_all();
    }
}

void logger::enqueue_async(log_event&& event) {
    unique_lock<mutex> queue_lock(queue_mutex_);

    if (!async_.load(memory_order_acquire)) {
        queue_lock.unlock_quiet();
        process_event_direct(event);
        return;
    }

    if (async_queue_.full()) {
        switch (overflow_.load(memory_order_acquire)) {
            case overflow_policy::block: {
                queue_cv_.wait(queue_lock,
                               [this] { return !async_queue_.full() || !running_.load(memory_order_acquire); });
                if (!running_.load(memory_order_acquire) || !async_.load(memory_order_acquire)) {
                    // Asynchronous delivery is shutting down:
                    // write the event now instead of dropping it while its producer is parked on a full queue.
                    queue_lock.unlock_quiet();
                    process_event_direct(event);
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
    queue_lock.unlock_quiet();

    if (!submit_drain()) {
        drain_events();
    }
}

bool logger::submit_drain() {
    bool expected = false;
    if (!drain_scheduled_.compare_exchange_strong(expected, true, memory_order_acq_rel)) {
        return true;
    }

    if (thread_pool_) {
        const auto submitted = thread_pool_->submit_task([this] { drain_events(); });
        const bool dropped = submitted.task_info &&
                             submitted.task_info->status.load(memory_order_relaxed) == task_info::status::failed;
        if (!dropped) {
            return true;
        }
    }

    drain_scheduled_.store(false, memory_order_release);
    return false;
}

void logger::drain_events() {
    for (;;) {
        vector<log_event> events;
        bool should_flush = false;

        {
            lock<mutex> queue_lock(queue_mutex_);
            while (!async_queue_.empty()) {
                events.push_back(async_queue_.pop());
            }
            queue_cv_.notify_all();
            should_flush = flush_requested_.load(memory_order_acquire);
        }

        write_events(events);

        if (should_flush) {
            flush_sinks();
        }

        if (flush_requested_.exchange(false, memory_order_acq_rel)) {
            lock<mutex> flush_lock(flush_mutex_);
            flush_cv_.notify_all();
        }

        // Stay the only drain task while more events are pending:
        // releasing the flag here would let a second drain task write the newer batch ahead of this one.
        // A pending flush request also keeps this task alive,
        // otherwise the request could be set after the peek above and wait for a drain that is no longer running.
        lock<mutex> queue_lock(queue_mutex_);
        if (!async_queue_.empty() || flush_requested_.load(memory_order_acquire)) {
            continue;
        }
        drain_scheduled_.store(false, memory_order_release);
        return;
    }
}

void logger::write_events(const vector<log_event>& events) {
    if (events.empty()) {
        return;
    }

    lock<mutex> sinks_lock(sinks_mutex_);
    for (const auto& event: events) {
        try {
            for (const auto& sink: sinks_) {
                sink->log(event);
            }
            // NOLINTNEXTLINE(bugprone-empty-catch)
        } catch (const exception& e) {
            NEFORCE_REPORT_EXCEPTION(e);
        }
    }
}

void logger::flush_sinks() {
    lock<mutex> sinks_lock(sinks_mutex_);
    for (const auto& sink: sinks_) {
        try {
            sink->flush();
            // NOLINTNEXTLINE(bugprone-empty-catch)
        } catch (const exception& e) {
            NEFORCE_REPORT_EXCEPTION(e);
        }
    }
}

void logger::process_event_direct(const log_event& event) {
    lock<mutex> sinks_lock(sinks_mutex_);
    for (const auto& sink: sinks_) {
        try {
            sink->log(event);
            // NOLINTNEXTLINE(bugprone-empty-catch)
        } catch (const exception& e) {
            NEFORCE_REPORT_EXCEPTION(e);
        }
    }
}

void logger::check_auto_flush() {
    const int64_t interval = auto_flush_ms_.load(memory_order_acquire);
    if (interval <= 0) {
        return;
    }

    const int64_t now = timestamp::now().value();
    int64_t last = last_auto_flush_.load(memory_order_acquire);
    if (now - last < interval * 1000) {
        return;
    }

    if (last_auto_flush_.compare_exchange_strong(last, now, memory_order_acq_rel)) {
        flush();
    }
}

void logger::log(const log_level level, string msg, const source_location loc) {
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

    shared_ptr<flat_unordered_map<string, string>> logger_ctx;
    {
        lock<mutex> context_lock(context_mutex_);
        logger_ctx = context_data_;
    }

    if (!mdc::empty() && logger_ctx) {
        auto merged = make_shared<flat_unordered_map<string, string>>(*logger_ctx);
        auto mdc_snap = mdc::snapshot();
        for (auto& entry: mdc_snap) {
            (*merged)[entry.first] = move(entry.second);
        }
        ev.context = move(merged);
    } else {
        ev.context = move(logger_ctx);
    }

    {
        lock<mutex> filter_lock(filter_mutex_);
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
    if (async_.load(memory_order_acquire) && thread_pool_ && thread_pool_->running()) {
        unique_lock<mutex> flush_lock(flush_mutex_);
        {
            lock<mutex> queue_lock(queue_mutex_);
            flush_requested_.store(true, memory_order_release);
        }

        const auto deadline = steady_clock::now() + seconds(flush_wait_seconds);
        while (flush_requested_.load(memory_order_acquire) && running_.load(memory_order_acquire) &&
               steady_clock::now() < deadline) {
            submit_drain();
            flush_cv_.wait_for(flush_lock, milliseconds(flush_repoke_ms), [this] {
                return !flush_requested_.load(memory_order_acquire) || !running_.load(memory_order_acquire);
            });
        }
        flush_requested_.store(false, memory_order_release);
        flush_lock.unlock_quiet();
    }

    vector<log_event> remaining;
    {
        lock<mutex> queue_lock(queue_mutex_);
        while (!async_queue_.empty()) {
            remaining.push_back(async_queue_.pop());
        }
        queue_cv_.notify_all();
    }
    write_events(remaining);
    flush_sinks();
}

logger_registry::logger_registry() {
    root_ = make_shared<logger>("root");
    loggers_["root"] = root_;
}

logger_registry& logger_registry::instance() {
    static logger_registry registry;
    return registry;
}

shared_ptr<logger> logger_registry::root_logger() { return root_; }

shared_ptr<logger> logger_registry::get_logger(const string& name) {
    if (name.empty() || name == "root") {
        return root_;
    }

    lock<mutex> registry_lock(mutex_);
    return create_logger_locked(name);
}

shared_ptr<logger> logger_registry::create_logger_locked(const string& name) {
    const auto existing = loggers_.find(name);
    if (existing != loggers_.end()) {
        return existing->second;
    }

    auto new_logger = make_shared<logger>(name);

    shared_ptr<logger> parent_logger = root_;
    const auto dot_pos = name.rfind('.');
    if (dot_pos != string::npos && dot_pos > 0) {
        parent_logger = create_logger_locked(name.substr(0, dot_pos));
    }
    parent_logger->register_child(new_logger);

    loggers_[name] = new_logger;
    return new_logger;
}

void logger_registry::flush_all() {
    vector<shared_ptr<logger>> snapshot;
    {
        lock<mutex> registry_lock(mutex_);
        for (const auto& entry: loggers_) {
            snapshot.push_back(entry.second);
        }
    }

    for (const auto& registered: snapshot) {
        registered->flush();
    }
}

NEFORCE_END_NAMESPACE__
