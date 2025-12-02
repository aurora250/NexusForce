#ifndef MSTL_LOGGING_LOG_EVENT_HPP__
#define MSTL_LOGGING_LOG_EVENT_HPP__
#include "MSTL/core/container/unordered_map.hpp"
#include "MSTL/core/time/datetime.hpp"
#include "MSTL/core/async/thread.hpp"
MSTL_BEGIN_NAMESPACE__

#ifdef ERROR
#undef ERROR
#endif

enum class LOG_LEVEL {
    TRACE=0, DEBUG, INFO, WARN, ERROR, FATAL
};

MSTL_CONSTEXPR20 string to_string(const LOG_LEVEL level) {
    switch(level) {
        case LOG_LEVEL::TRACE: return "TRACE";
        case LOG_LEVEL::DEBUG: return "DEBUG";
        case LOG_LEVEL::INFO:  return "INFO";
        case LOG_LEVEL::WARN:  return "WARN";
        case LOG_LEVEL::ERROR: return "ERROR";
        case LOG_LEVEL::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}


struct log_event {
    unordered_map<string, string> context;
    string file;
    string func;
    string message;
    datetime dt;
    int line;
    thread::id thread_id;
    LOG_LEVEL level;
};

MSTL_END_NAMESPACE__
#endif // MSTL_LOGGING_LOG_EVENT_HPP__
