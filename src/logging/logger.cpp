#include <NeForce/core/system/console.hpp>
#include <NeForce/logging/logger.hpp>
NEFORCE_BEGIN_NAMESPACE__

void logger::enqueue(log_event&& event) {
    {
        lock<mutex> lock(queue_mutex_);
        queue_.push(_NEFORCE move(event));
    }
    cv_.notify_one();
}

void logger::enqueue(const log_event& event) {
    {
        lock<mutex> lock(queue_mutex_);
        queue_.push(event);
    }
    cv_.notify_one();
}

void logger::start_worker() {
    running_ = true;
    worker_ = thread([this] { worker_loop(); });
}

void logger::stop_worker() {
    running_.store(false, memory_order_release);
    cv_.notify_all();

    if (worker_.joinable()) {
        constexpr auto timeout = seconds(5);
        if (worker_.joinable()) {
            auto start = steady_clock::now();
            while (worker_.joinable() && steady_clock::now() - start < timeout) {
                this_thread::sleep_for(milliseconds(10));
            }

            if (worker_.joinable()) {
                println("Warning: Worker thread did not exit in time, detaching...");
                worker_.detach();
            }
        }
    }
}

void logger::worker_loop() {
    while (running_.load(memory_order_acquire)) {
        vector<log_event> events;
        bool should_flush = false;

        {
            unique_lock<mutex> lock(queue_mutex_);
            cv_.wait_for(lock, milliseconds(100),
                         [this] { return !queue_.empty() || flush_requested_.load(memory_order_acquire); });

            while (!queue_.empty()) {
                events.push_back(_NEFORCE move(queue_.front()));
                queue_.pop();
            }

            if (flush_requested_.exchange(false, memory_order_acq_rel)) {
                should_flush = true;
            }
        }

        if (!events.empty()) {
            lock<mutex> sl(sinks_mutex_);
            for (const auto& ev: events) {
                for (const auto& sink: sinks_) {
                    sink->log(ev);
                }
            }
        }

        if (should_flush) {
            lock<mutex> sl(sinks_mutex_);
            for (const auto& sink: sinks_) {
                sink->flush();
            }
            lock<mutex> fl(flush_mutex_);
            flush_cv_.notify_all();
        }
    }
}

logger::logger(const LOG_LEVEL level, const bool async) :
level_(level),
async_(async),
running_(false) {
    if (async_) {
        start_worker();
    }
}

logger::~logger() {
    if (async_) {
        running_.store(false, memory_order_release);
        cv_.notify_all();
        flush_cv_.notify_all();

        if (worker_.joinable()) {
            worker_.join();
        }
    }
    {
        lock<mutex> lock(sinks_mutex_);
        sinks_.clear();
    }

    {
        lock<mutex> lock(queue_mutex_);
        while (!queue_.empty()) {
            queue_.pop();
        }
    }
}

void logger::add_sink(shared_ptr<log_sink> sink) {
    lock<mutex> lock(sinks_mutex_);
    sinks_.push_back(move(sink));
}

void logger::set_level(const LOG_LEVEL level) { level_ = level; }

void logger::set_filter(function<bool(const log_event&)> filter) {
    lock<mutex> lock(filter_mutex_);
    filter_ = move(filter);
}

void logger::add_context(const string& key, string value) {
    lock<mutex> lock(context_mutex_);
    context_[key] = move(value);
}

void logger::remove_context(const string& key) {
    lock<mutex> lock(context_mutex_);
    context_.erase(key);
}

void logger::clear_context() {
    lock<mutex> lock(context_mutex_);
    context_.clear();
}

void logger::enable_async(const bool async) {
    if (async == async_.load(memory_order_acquire)) {
        return;
    }

    async_.store(async, memory_order_release);

    if (async) {
        start_worker();
    } else {
        lock<mutex> lk(queue_mutex_);
        while (!queue_.empty()) {
            log_event ev = _NEFORCE move(queue_.front());
            queue_.pop();
            lock<mutex> slk(sinks_mutex_);
            for (const auto& sink: sinks_) {
                sink->log(ev);
            }
        }
        stop_worker();
    }
}

void logger::log(const LOG_LEVEL level, string msg, string file, string func, const int line) {
    if (level < level_) {
        return;
    }

    log_event ev;
    ev.dt = datetime::now();
    ev.level = level;
    ev.file = move(file);
    ev.line = line;
    ev.func = move(func);
    ev.thread_id = this_thread::id();
    ev.message = move(msg);

    {
        lock<mutex> lock(context_mutex_);
        ev.context = context_;
    }
    {
        lock<mutex> lock(filter_mutex_);
        if (filter_ && !filter_(ev)) {
            return;
        }
    }

    if (async_) {
        enqueue(ev);
    } else {
        lock<mutex> lock(sinks_mutex_);
        for (const auto& sink: sinks_) {
            sink->log(ev);
        }
    }
}

void logger::flush() {
    if (async_) {
        flush_requested_.store(true, memory_order_release);
        cv_.notify_one();

        unique_lock<mutex> lock(flush_mutex_);
        flush_cv_.wait(lock, [this] { return !flush_requested_.load(memory_order_acquire); });
    } else {
        lock<mutex> lock(sinks_mutex_);
        for (const auto& sink: sinks_) {
            sink->flush();
        }
    }
}

NEFORCE_END_NAMESPACE__
