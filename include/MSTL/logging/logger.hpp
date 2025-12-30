#ifndef MSTL_CORE_LOGGING_LOGGER_HPP__
#define MSTL_CORE_LOGGING_LOGGER_HPP__
#include "MSTL/core/container/queue.hpp"
#include "MSTL/core/functional/function.hpp"
#include "MSTL/core/memory/shared_ptr.hpp"
#include "MSTL/core/async/condition_variable.hpp"
#include "MSTL/core/async/atomic.hpp"
#include "log_sink.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API logger {
private:
    LOG_LEVEL level_;
    _MSTL atomic<bool> async_;

    vector<shared_ptr<log_sink>> sinks_;
    _MSTL mutex sinks_mutex_;

    queue<log_event> queue_;
    _MSTL mutex queue_mutex_;
    _MSTL condition_variable cv_;

    _MSTL thread worker_;
    _MSTL atomic<bool> running_;

    _MSTL atomic<bool> flush_requested_{false};
    _MSTL mutex flush_mutex_;
    _MSTL condition_variable flush_cv_;

    function<bool(const log_event&)> filter_;
    _MSTL mutex filter_mutex_;

    unordered_map<string, string> context_;
    _MSTL mutex context_mutex_;

    void enqueue(log_event&& ev);
    void enqueue(const log_event& ev);

    void start_worker();
    void stop_worker();

    void worker_loop();

    explicit logger(LOG_LEVEL level = LOG_LEVEL::INFO, bool async = false);

public:
    static logger& instance() {
        static logger log;
        return log;
    }

    logger(const logger&) = delete;
    logger& operator =(const logger&) = delete;
    logger(logger&&) = delete;
    logger& operator =(logger&&) = delete;

    ~logger();

    void add_sink(shared_ptr<log_sink> sink);

    void set_level(LOG_LEVEL level);
    void set_filter(function<bool(const log_event&)> filter);

    void add_context(const string& key, string value);
    void remove_context(const string& key);
    void clear_context();

    void enable_async(bool async);

    void log(LOG_LEVEL level, string msg, string file, string func, int line);

    void trace(string msg, string file, string func, int line) {
        log(LOG_LEVEL::TRACE, move(msg), move(file), move(func), line);
    }
    void debug(string msg, string file, string func, int line) {
        log(LOG_LEVEL::DEBUG, move(msg), move(file), move(func), line);
    }
    void info(string msg, string file, string func, int line) {
        log(LOG_LEVEL::INFO, move(msg), move(file), move(func), line);
    }
    void warn(string msg, string file, string func, int line) {
        log(LOG_LEVEL::WARN, move(msg), move(file), move(func), line);
    }
    void error(string msg, string file, string func, int line) {
        log(LOG_LEVEL::ERROR, move(msg), move(file), move(func), line);
    }
    void fatal(string msg, string file, string func, int line) {
        log(LOG_LEVEL::FATAL, move(msg), move(file), move(func), line);
    }

    void flush();
};

#define MSTL_LOG_TRACE(msg) _MSTL logger::instance().trace(msg, __FILE__, __func__, __LINE__)
#define MSTL_LOG_DEBUG(msg) _MSTL logger::instance().debug(msg, __FILE__, __func__, __LINE__)
#define MSTL_LOG_INFO(msg)  _MSTL logger::instance().info(msg,  __FILE__, __func__, __LINE__)
#define MSTL_LOG_WARN(msg)  _MSTL logger::instance().warn(msg,  __FILE__, __func__, __LINE__)
#define MSTL_LOG_ERROR(msg) _MSTL logger::instance().error(msg, __FILE__, __func__, __LINE__)
#define MSTL_LOG_FATAL(msg) _MSTL logger::instance().fatal(msg, __FILE__, __func__, __LINE__)

#define MSTL_LOGF_TRACE(msg, ...) _MSTL logger::instance().trace(_MSTL format(msg, __VA_ARGS__), __FILE__, __func__, __LINE__)
#define MSTL_LOGF_DEBUG(msg, ...) _MSTL logger::instance().debug(_MSTL format(msg, __VA_ARGS__), __FILE__, __func__, __LINE__)
#define MSTL_LOGF_INFO(msg, ...)  _MSTL logger::instance().info(_MSTL format(msg, __VA_ARGS__),  __FILE__, __func__, __LINE__)
#define MSTL_LOGF_WARN(msg, ...)  _MSTL logger::instance().warn(_MSTL format(msg, __VA_ARGS__),  __FILE__, __func__, __LINE__)
#define MSTL_LOGF_ERROR(msg, ...) _MSTL logger::instance().error(_MSTL format(msg, __VA_ARGS__), __FILE__, __func__, __LINE__)
#define MSTL_LOGF_FATAL(msg, ...) _MSTL logger::instance().fatal(_MSTL format(msg, __VA_ARGS__), __FILE__, __func__, __LINE__)

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_LOGGING_LOGGER_HPP__
