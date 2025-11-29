#include <MSTL/core/logging/logger.hpp>
#include <MSTL/core/system/console.hpp>
MSTL_BEGIN_NAMESPACE__

void logger::enqueue(log_event&& ev) {
    {
        _MSTL lock_guard<_MSTL mutex> lock(queue_mutex_);
        queue_.push(_MSTL move(ev));
    }
    cv_.notify_one();
}

void logger::enqueue(const log_event& ev) {
    {
        _MSTL lock_guard<_MSTL mutex> lock(queue_mutex_);
        queue_.push(ev);
    }
    cv_.notify_one();
}

void logger::start_worker() {
    running_ = true;
    worker_ = _MSTL thread([this] {
        worker_loop();
    });
}

void logger::stop_worker() {
    running_.store(false, _MSTL memory_order_release);
    cv_.notify_all();

    if (worker_.joinable()) {
        auto timeout = _MSTL_CHRONO seconds(5);
        if (worker_.joinable()) {
            auto start = _MSTL_CHRONO steady_clock::now();
            while (worker_.joinable() && _MSTL_CHRONO steady_clock::now() - start < timeout) {
                _MSTL this_thread::sleep_for(_MSTL_CHRONO milliseconds(10));
            }

            if (worker_.joinable()) {
                println("Warning: Worker thread did not exit in time, detaching...");
                worker_.detach();
            }
        }
    }
}

void logger::worker_loop() {
    while (running_.load(_MSTL memory_order_acquire)) {
        vector<log_event> events;
        bool should_flush = false;

        {
            _MSTL unique_lock<_MSTL mutex> lock(queue_mutex_);
            cv_.wait_for(lock, _MSTL_CHRONO milliseconds(100), [this] {
                return !queue_.empty() ||
                       flush_requested_.load(_MSTL memory_order_acquire);
            });

            while (!queue_.empty()) {
                events.push_back(_MSTL move(queue_.front()));
                queue_.pop();
            }

            if (flush_requested_.exchange(false, _MSTL memory_order_acq_rel)) {
                should_flush = true;
            }
        }

        if (!events.empty()) {
            _MSTL lock_guard<_MSTL mutex> sl(sinks_mutex_);
            for (const auto& ev : events) {
                for (const auto& sink : sinks_) {
                    sink->log(ev);
                }
            }
        }

        if (should_flush) {
            _MSTL lock_guard<_MSTL mutex> sl(sinks_mutex_);
            for (const auto& sink : sinks_) {
                sink->flush();
            }
            _MSTL lock_guard<_MSTL mutex> fl(flush_mutex_);
            flush_cv_.notify_all();
        }
    }
}

logger::logger(const LOG_LEVEL level, const bool async)
: level_(level), async_(async), running_(false) {
    if (async_) {
        start_worker();
    }
}

logger::~logger() {
    if (async_) {
        running_.store(false, _MSTL memory_order_release);
        cv_.notify_all();
        flush_cv_.notify_all();

        if (worker_.joinable()) {
            worker_.join();
        }
    }
    {
        _MSTL lock_guard<_MSTL mutex> lock(sinks_mutex_);
        sinks_.clear();
    }

    {
        _MSTL lock_guard<_MSTL mutex> lock(queue_mutex_);
        while (!queue_.empty()) {
            queue_.pop();
        }
    }
}

void logger::add_sink(shared_ptr<log_sink> sink) {
    _MSTL lock_guard<_MSTL mutex> lock(sinks_mutex_);
    sinks_.push_back(move(sink));
}

void logger::set_level(const LOG_LEVEL level) {
    level_ = level;
}

void logger::set_filter(function<bool(const log_event&)> filter) {
    _MSTL lock_guard<_MSTL mutex> lock(filter_mutex_);
    filter_ = move(filter);
}

void logger::add_context(const string& key, string value) {
    _MSTL lock_guard<_MSTL mutex> lock(context_mutex_);
    context_[key] = move(value);
}

void logger::remove_context(const string& key) {
    _MSTL lock_guard<_MSTL mutex> lock(context_mutex_);
    context_.erase(key);
}

void logger::clear_context() {
    _MSTL lock_guard<_MSTL mutex> lock(context_mutex_);
    context_.clear();
}

void logger::enable_async(bool async) {
    if (async == async_.load(_MSTL memory_order_acquire)) {
        return;
    }

    async_.store(async, _MSTL memory_order_release);

    if (async) {
        start_worker();
    } else {
        _MSTL lock_guard<_MSTL mutex> lock(queue_mutex_);
        while (!queue_.empty()) {
            log_event ev = _MSTL move(queue_.front());
            queue_.pop();
            _MSTL lock_guard<_MSTL mutex> sl(sinks_mutex_);
            for (const auto& sink : sinks_) {
                sink->log(ev);
            }
        }
        stop_worker();
    }
}

void logger::log(const LOG_LEVEL level, string msg,
    string file, string func, const int line) {
    if (level < level_) return;

    log_event ev;
    ev.dt = datetime::now();
    ev.level = level;
    ev.file = move(file);
    ev.line = line;
    ev.func = move(func);
    ev.thread_id = this_thread::get_id();
    ev.message = move(msg);

    {
        _MSTL lock_guard<_MSTL mutex> lock(context_mutex_);
        ev.context = context_;
    }
    {
        _MSTL lock_guard<_MSTL mutex> lock(filter_mutex_);
        if (filter_ && !filter_(ev)) {
            return;
        }
    }

    if (async_) {
        enqueue(ev);
    } else {
        _MSTL lock_guard<_MSTL mutex> lock(sinks_mutex_);
        for (const auto& sink : sinks_) {
            sink->log(ev);
        }
    }
}

void logger::flush() {
    if (async_) {
        flush_requested_.store(true, _MSTL memory_order_release);
        cv_.notify_one();

        _MSTL unique_lock<_MSTL mutex> lock(flush_mutex_);
        flush_cv_.wait(lock, [this] {
            return !flush_requested_.load(_MSTL memory_order_acquire);
        });
    } else {
        _MSTL lock_guard<_MSTL mutex> lock(sinks_mutex_);
        for (const auto& sink : sinks_) {
            sink->flush();
        }
    }
}

MSTL_END_NAMESPACE__
