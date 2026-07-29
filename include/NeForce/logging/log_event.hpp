#ifndef NEFORCE_LOGGING_LOG_EVENT_HPP__
#define NEFORCE_LOGGING_LOG_EVENT_HPP__

/**
 * @file log_event.hpp
 * @brief 日志事件定义
 *
 * 此文件定义了日志系统的核心数据结构，
 * 包括日志级别枚举、源码位置、溢出策略、MDC和日志事件结构体。
 */

#include "NeForce/core/async/thread.hpp"
#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/exception/source_loc.hpp"
#include "NeForce/core/memory/shared_ptr.hpp"
#include "NeForce/core/time/datetime.hpp"
#ifdef ERROR
#    undef ERROR
#endif
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Logging 日志系统
 * @brief 日志记录和管理功能
 *
 * 提供完整的日志系统，支持：
 * - 层级化 Logger 命名（"app.module.sub"）
 * - 编译期级别过滤（零开销）
 * - 线程池异步模式（基于 thread_pool）
 * - 异步队列溢出策略（阻塞 / 丢弃 / 覆盖最旧）
 * - 条件日志（LOG_IF, LOG_EVERY_N, LOG_FIRST_N）
 * - 自动刷新间隔
 * - 三层上下文（全局 / Logger / MDC 线程局部）
 * @{
 */

#define NEFORCE_LOG_LEVEL_TRACE 0
#define NEFORCE_LOG_LEVEL_DEBUG 1
#define NEFORCE_LOG_LEVEL_INFO 2
#define NEFORCE_LOG_LEVEL_WARN 3
#define NEFORCE_LOG_LEVEL_ERROR 4
#define NEFORCE_LOG_LEVEL_FATAL 5
#define NEFORCE_LOG_LEVEL_OFF 6

/**
 * @enum log_level
 * @brief 日志级别枚举
 *
 * 定义了日志的严重性级别。枚举值与编译期过滤宏一一对应。
 */
enum class log_level : uint8_t {
    TRACE = NEFORCE_LOG_LEVEL_TRACE, ///< 跟踪级别
    DEBUG = NEFORCE_LOG_LEVEL_DEBUG, ///< 调试级别
    INFO = NEFORCE_LOG_LEVEL_INFO,   ///< 信息级别
    WARN = NEFORCE_LOG_LEVEL_WARN,   ///< 警告级别
    ERROR = NEFORCE_LOG_LEVEL_ERROR, ///< 错误级别
    FATAL = NEFORCE_LOG_LEVEL_FATAL, ///< 致命级别
    OFF = NEFORCE_LOG_LEVEL_OFF      ///< 关闭所有日志
};

/**
 * @def NEFORCE_ACTIVE_LOG_LEVEL
 * @brief 编译期日志过滤阈值
 *
 * 低于此阈值的日志宏在编译期被剥离为 ((void)0)。
 * 默认值：Debug 构建为 TRACE，Release 构建（定义了 NDEBUG）为 INFO。
 * 可通过在包含此头文件前定义该宏来覆盖。
 */
#ifndef NEFORCE_ACTIVE_LOG_LEVEL
#    ifdef NDEBUG
#        define NEFORCE_ACTIVE_LOG_LEVEL NEFORCE_LOG_LEVEL_INFO
#    else
#        define NEFORCE_ACTIVE_LOG_LEVEL NEFORCE_LOG_LEVEL_TRACE
#    endif
#endif

/**
 * @brief 将日志级别转换为字符串表示
 * @param level 日志级别
 * @return 级别对应的字符串（TRACE/DEBUG/INFO/WARN/ERROR/FATAL/OFF）
 */
NEFORCE_CONSTEXPR20 string to_string(const log_level level) {
    switch (level) {
        case log_level::TRACE:
            return "TRACE";
        case log_level::DEBUG:
            return "DEBUG";
        case log_level::INFO:
            return "INFO";
        case log_level::WARN:
            return "WARN";
        case log_level::ERROR:
            return "ERROR";
        case log_level::FATAL:
            return "FATAL";
        case log_level::OFF:
            return "OFF";
        default:
            unreachable();
    }
}


/**
 * @enum overflow_policy
 * @brief 异步队列溢出策略
 *
 * 当日志队列满时，决定新事件的去留。
 */
enum class overflow_policy : uint8_t {
    block,         ///< 阻塞等待直到队列有空间
    discard,       ///< 丢弃新事件
    overrun_oldest ///< 丢弃队列中最旧的事件，为新事件腾出空间
};


/**
 * @class mdc
 * @brief Mapped Diagnostic Context，线程局部上下文
 *
 * 每个线程独立的键值对存储，用于附加请求追踪信息（如 request_id、user）。
 * 日志事件构造时自动将 MDC 内容合并到 event.context 中。
 */
class NEFORCE_API mdc {
private:
    static unordered_map<string, string>& storage();

public:
    /**
     * @brief 设置键值对
     * @param key 键名
     * @param value 值
     */
    static void put(const string& key, string value);

    /**
     * @brief 获取键对应的值
     * @param key 键名
     * @return 值，若不存在返回空字符串
     */
    static string get(const string& key);

    /**
     * @brief 移除指定的键
     * @param key 键名
     */
    static void remove(const string& key);

    /** @brief 清除当前线程的所有 MDC 数据 */
    static void clear();

    /**
     * @brief 检查当前线程的 MDC 是否为空
     * @return 为空返回 true
     */
    static bool empty();

    /**
     * @brief 获取当前线程 MDC 的快照
     * @return MDC 键值对副本
     */
    static unordered_map<string, string> snapshot();
};


/**
 * @struct log_event
 * @brief 日志事件结构体
 *
 * 封装单条日志的所有信息。
 * 同一 Logger 的所有事件在不修改上下文时共享同一份 map。
 */
struct log_event {
    shared_ptr<unordered_map<string, string>> context; ///< 上下文（Logger + MDC）
    string message;                                    ///< 格式化后的日志消息
    string logger_name;                                ///< 产生此事件的 Logger 名称
    datetime dt;                                       ///< 事件时间戳
    source_loc loc;                                    ///< 源码位置
    thread::id thread_id;                              ///< 产生此事件的线程 ID
    log_level level;                                   ///< 日志级别
};

/** @} */ // Logging

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_LOGGING_LOG_EVENT_HPP__
