#ifndef NEFORCE_LOGGING_LOG_EVENT_HPP__
#define NEFORCE_LOGGING_LOG_EVENT_HPP__

/**
 * @file log_event.hpp
 * @brief 日志事件定义
 *
 * 此文件定义了日志系统的核心数据结构，
 * 包括日志级别枚举和日志事件结构体，用于在日志系统中传递日志信息。
 */

#include "NeForce/core/async/thread.hpp"
#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/time/datetime.hpp"
#ifdef ERROR
#undef ERROR
#endif
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Logging 日志系统
 * @brief 日志记录和管理功能
 * @{
 */

/**
 * @enum LOG_LEVEL
 * @brief 日志级别枚举
 *
 * 定义了日志的严重性级别，从最详细的TRACE到最严重的FATAL。
 * 用于控制日志输出和过滤。
 */
enum class LOG_LEVEL {
    TRACE = 0,  ///< 跟踪级别，最详细的调试信息
    DEBUG,      ///< 调试级别，用于开发调试
    INFO,       ///< 信息级别，普通信息
    WARN,       ///< 警告级别，表示潜在问题
    ERROR,      ///< 错误级别，表示可恢复的错误
    FATAL       ///< 致命级别，表示不可恢复的错误
};


NEFORCE_CONSTEXPR20 string to_string(const LOG_LEVEL level) {
    switch(level) {
        case LOG_LEVEL::TRACE: return "TRACE";
        case LOG_LEVEL::DEBUG: return "DEBUG";
        case LOG_LEVEL::INFO:  return "INFO";
        case LOG_LEVEL::WARN:  return "WARN";
        case LOG_LEVEL::ERROR: return "ERROR";
        case LOG_LEVEL::FATAL: return "FATAL";
        default: unreachable();
    }
}


/**
 * @struct log_event
 * @brief 日志事件结构体
 *
 * 包含一条日志记录的所有信息，包括上下文、位置、时间、
 * 线程、级别和消息内容。用于在日志系统中传递和格式化日志。
 */
struct log_event {
    unordered_map<string, string> context;  ///< 上下文信息键值对
    string file;                            ///< 源文件名
    string func;                            ///< 函数名
    string message;                         ///< 日志消息
    datetime dt;                            ///< 时间戳
    int line;                               ///< 行号
    thread::id thread_id;                   ///< 线程ID
    LOG_LEVEL level;                        ///< 日志级别
};

/** @} */ // Logging

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_LOGGING_LOG_EVENT_HPP__
