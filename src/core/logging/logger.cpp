#include <MSTL/core/logging/logger.hpp>
#include <MSTL/core/utilities/console.hpp>
MSTL_BEGIN_NAMESPACE__

void logger::enqueue(log_event&& ev) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        queue_.push(std::move(ev));
    }
    cv_.notify_one();
}

void logger::enqueue(const log_event& ev) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        queue_.push(ev);
    }
    cv_.notify_one();
}

void logger::start_worker() {
    running_ = true;
    worker_ = std::thread([this] {
        worker_loop();
    });
}

void logger::stop_worker() {
    running_.store(false, std::memory_order_release);
    cv_.notify_all();

    if (worker_.joinable()) {
        auto timeout = std::chrono::seconds(5);
        if (worker_.joinable()) {
            auto start = std::chrono::steady_clock::now();
            while (worker_.joinable() && std::chrono::steady_clock::now() - start < timeout) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            if (worker_.joinable()) {
                println("Warning: Worker thread did not exit in time, detaching...");
                worker_.detach();
            }
        }
    }
}

void logger::worker_loop() {
    while (running_.load(std::memory_order_acquire)) {
        vector<log_event> events;
        bool should_flush = false;

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            cv_.wait_for(lock, std::chrono::milliseconds(100), [this] {
                return !queue_.empty() ||
                       flush_requested_.load(std::memory_order_acquire);
            });

            while (!queue_.empty()) {
                events.push_back(std::move(queue_.front()));
                queue_.pop();
            }

            if (flush_requested_.exchange(false, std::memory_order_acq_rel)) {
                should_flush = true;
            }
        }

        if (!events.empty()) {
            std::lock_guard<std::mutex> sl(sinks_mutex_);
            for (const auto& ev : events) {
                for (const auto& sink : sinks_) {
                    sink->log(ev);
                }
            }
        }

        if (should_flush) {
            std::lock_guard<std::mutex> sl(sinks_mutex_);
            for (const auto& sink : sinks_) {
                sink->flush();
            }
            std::lock_guard<std::mutex> fl(flush_mutex_);
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
        running_.store(false, std::memory_order_release);
        cv_.notify_all();
        flush_cv_.notify_all();

        if (worker_.joinable()) {
            worker_.join();
        }
    }
    {
        std::lock_guard<std::mutex> lock(sinks_mutex_);
        sinks_.clear();
    }

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        while (!queue_.empty()) {
            queue_.pop();
        }
    }
}

logger& logger::instance() {
    static logger log;
    return log;
}

void logger::add_sink(shared_ptr<log_sink> sink) {
    std::lock_guard<std::mutex> lock(sinks_mutex_);
    sinks_.push_back(move(sink));
}

void logger::set_level(const LOG_LEVEL level) {
    level_ = level;
}

void logger::set_filter(function<bool(const log_event&)> filter) {
    std::lock_guard<std::mutex> lock(filter_mutex_);
    filter_ = move(filter);
}

void logger::add_context(const string& key, string value) {
    std::lock_guard<std::mutex> lock(context_mutex_);
    context_[key] = move(value);
}

void logger::remove_context(const string& key) {
    std::lock_guard<std::mutex> lock(context_mutex_);
    context_.erase(key);
}

void logger::clear_context() {
    std::lock_guard<std::mutex> lock(context_mutex_);
    context_.clear();
}

void logger::enable_async(bool async) {
    if (async == async_.load(std::memory_order_acquire)) {
        return;
    }

    async_.store(async, std::memory_order_release);

    if (async) {
        start_worker();
    } else {
        // 同步模式下，确保处理完所有队列中的日志
        std::lock_guard<std::mutex> lock(queue_mutex_);
        while (!queue_.empty()) {
            log_event ev = std::move(queue_.front());
            queue_.pop();
            std::lock_guard<std::mutex> sl(sinks_mutex_);
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
        std::lock_guard<std::mutex> lock(context_mutex_);
        ev.context = context_;
    }
    {
        std::lock_guard<std::mutex> lock(filter_mutex_);
        if (filter_ && !filter_(ev)) {
            return;
        }
    }

    if (async_) {
        enqueue(ev);
    } else {
        std::lock_guard<std::mutex> lock(sinks_mutex_);
        for (const auto& sink : sinks_) {
            sink->log(ev);
        }
    }
}

void logger::flush() {
    if (async_) {
        flush_requested_.store(true, std::memory_order_release);
        cv_.notify_one();  // 确保工作线程被唤醒

        std::unique_lock<std::mutex> lock(flush_mutex_);
        flush_cv_.wait(lock, [this] {
            return !flush_requested_.load(std::memory_order_acquire);
        });
    } else {
        std::lock_guard<std::mutex> lock(sinks_mutex_);
        for (const auto& sink : sinks_) {
            sink->flush();
        }
    }
}

MSTL_END_NAMESPACE__
