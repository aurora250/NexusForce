#ifndef MSTL_CORE_LOGGING_LOGGER_HPP__
#define MSTL_CORE_LOGGING_LOGGER_HPP__

/**
 * @file logger.hpp
 * @brief 日志记录器
 *
 * 此文件提供了日志记录器的核心实现，支持同步/异步日志、
 * 多输出目标、日志过滤、上下文信息和格式化日志等功能。
 */

#include "MSTL/core/async/condition_variable.hpp"
#include "MSTL/core/container/queue.hpp"
#include "MSTL/core/functional/function.hpp"
#include "MSTL/core/memory/shared_ptr.hpp"
#include "MSTL/logging/log_sink.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup Logging 日志系统
 * @brief 日志记录和管理功能
 * @{
 */

/**
 * @class logger
 * @brief 日志记录器单例类
 *
 * 提供全局日志记录功能，支持：
 * - 多级别日志
 * - 同步/异步模式
 * - 多输出目标
 * - 日志过滤
 * - 全局上下文信息
 * - 线程安全
 */
class MSTL_API logger {
private:
    LOG_LEVEL level_;         ///< 当前日志级别
    atomic<bool> async_;      ///< 是否异步模式

    vector<shared_ptr<log_sink>> sinks_;  ///< 输出目标列表
    mutex sinks_mutex_;       ///< 输出目标互斥锁

    queue<log_event> queue_;  ///< 异步日志队列
    mutex queue_mutex_;       ///< 队列互斥锁
    condition_variable cv_;   ///< 条件变量

    thread worker_;           ///< 异步工作线程
    atomic<bool> running_;    ///< 工作线程运行标志

    atomic<bool> flush_requested_{false};   ///< 刷新请求标志
    mutex flush_mutex_;                     ///< 刷新互斥锁
    condition_variable flush_cv_;           ///< 刷新条件变量

    function<bool(const log_event&)> filter_;  ///< 日志过滤器
    mutex filter_mutex_;                       ///< 过滤器互斥锁

    unordered_map<string, string> context_;    ///< 全局上下文信息
    mutex context_mutex_;                      ///< 上下文互斥锁

    /**
     * @brief 将日志事件加入异步队列
     * @param event 日志事件（右值引用）
     */
    void enqueue(log_event&& event);

    /**
     * @brief 将日志事件加入异步队列
     * @param event 日志事件（左值引用）
     */
    void enqueue(const log_event& event);

    /**
     * @brief 启动异步工作线程
     */
    void start_worker();

    /**
     * @brief 停止异步工作线程
     */
    void stop_worker();

    /**
     * @brief 异步工作线程主循环
     *
     * 从队列中取出日志事件并分发给各个输出目标。
     */
    void worker_loop();

    /**
     * @brief 私有构造函数
     * @param level 初始日志级别
     * @param async 是否异步模式
     */
    explicit logger(LOG_LEVEL level = LOG_LEVEL::INFO, bool async = false);

public:
    /**
     * @brief 获取单例实例
     * @return 日志记录器实例引用
     */
    static logger& instance() {
        static logger log;
        return log;
    }

    logger(const logger&) = delete;
    logger& operator =(const logger&) = delete;
    logger(logger&&) = delete;
    logger& operator =(logger&&) = delete;

    /**
     * @brief 析构函数
     *
     * 等待所有日志处理完成，清理资源。
     */
    ~logger();

    /**
     * @brief 添加输出目标
     * @param sink 输出目标的共享指针
     */
    void add_sink(shared_ptr<log_sink> sink);

    /**
     * @brief 设置日志级别
     * @param level 新的日志级别
     *
     * 低于此级别的日志将被忽略。
     */
    void set_level(LOG_LEVEL level);

    /**
     * @brief 设置日志过滤器
     * @param filter 过滤器函数
     *
     * 过滤器返回true表示接受该日志，false表示拒绝。
     */
    void set_filter(function<bool(const log_event&)> filter);

    /**
     * @brief 添加上下文信息
     * @param key 键
     * @param value 值
     *
     * 上下文信息会附加到每条日志中。
     */
    void add_context(const string& key, string value);

    /**
     * @brief 移除上下文信息
     * @param key 要移除的键
     */
    void remove_context(const string& key);

    /**
     * @brief 清除所有上下文信息
     */
    void clear_context();

    /**
     * @brief 启用或禁用异步模式
     * @param async true为异步，false为同步
     *
     * 异步模式下日志写入不会阻塞调用线程。
     */
    void enable_async(bool async);

    /**
     * @brief 记录日志
     * @param level 日志级别
     * @param msg 日志消息
     * @param file 源文件名
     * @param func 函数名
     * @param line 行号
     */
    void log(LOG_LEVEL level, string msg, string file, string func, int line);

    /**
     * @brief 记录TRACE级别日志
     * @param msg 日志消息
     * @param file 源文件名
     * @param func 函数名
     * @param line 行号
     */
    void trace(string msg, string file, string func, int line) {
        log(LOG_LEVEL::TRACE, move(msg), move(file), move(func), line);
    }

    /**
     * @brief 记录DEBUG级别日志
     * @param msg 日志消息
     * @param file 源文件名
     * @param func 函数名
     * @param line 行号
     */
    void debug(string msg, string file, string func, int line) {
        log(LOG_LEVEL::DEBUG, move(msg), move(file), move(func), line);
    }

    /**
     * @brief 记录INFO级别日志
     * @param msg 日志消息
     * @param file 源文件名
     * @param func 函数名
     * @param line 行号
     */
    void info(string msg, string file, string func, int line) {
        log(LOG_LEVEL::INFO, move(msg), move(file), move(func), line);
    }

    /**
     * @brief 记录WARN级别日志
     * @param msg 日志消息
     * @param file 源文件名
     * @param func 函数名
     * @param line 行号
     */
    void warn(string msg, string file, string func, int line) {
        log(LOG_LEVEL::WARN, move(msg), move(file), move(func), line);
    }

    /**
     * @brief 记录ERROR级别日志
     * @param msg 日志消息
     * @param file 源文件名
     * @param func 函数名
     * @param line 行号
     */
    void error(string msg, string file, string func, int line) {
        log(LOG_LEVEL::ERROR, move(msg), move(file), move(func), line);
    }

    /**
     * @brief 记录FATAL级别日志
     * @param msg 日志消息
     * @param file 源文件名
     * @param func 函数名
     * @param line 行号
     */
    void fatal(string msg, string file, string func, int line) {
        log(LOG_LEVEL::FATAL, move(msg), move(file), move(func), line);
    }

    /**
     * @brief 刷新所有输出目标
     *
     * 确保所有日志都被实际输出。
     * 异步模式下会等待工作线程处理完所有日志。
     */
    void flush();
};


/** @brief 记录TRACE级别日志 */
#define MSTL_LOG_TRACE(msg) _MSTL logger::instance().trace(msg, __FILE__, __func__, __LINE__)

/** @brief 记录DEBUG级别日志 */
#define MSTL_LOG_DEBUG(msg) _MSTL logger::instance().debug(msg, __FILE__, __func__, __LINE__)

/** @brief 记录INFO级别日志 */
#define MSTL_LOG_INFO(msg)  _MSTL logger::instance().info(msg,  __FILE__, __func__, __LINE__)

/** @brief 记录WARN级别日志 */
#define MSTL_LOG_WARN(msg)  _MSTL logger::instance().warn(msg,  __FILE__, __func__, __LINE__)

/** @brief 记录ERROR级别日志 */
#define MSTL_LOG_ERROR(msg) _MSTL logger::instance().error(msg, __FILE__, __func__, __LINE__)

/** @brief 记录FATAL级别日志 */
#define MSTL_LOG_FATAL(msg) _MSTL logger::instance().fatal(msg, __FILE__, __func__, __LINE__)


/** @brief 格式化TRACE级别日志 */
#define MSTL_LOGF_TRACE(msg, ...) _MSTL logger::instance().trace(_MSTL format(msg, __VA_ARGS__), __FILE__, __func__, __LINE__)

/** @brief 格式化DEBUG级别日志 */
#define MSTL_LOGF_DEBUG(msg, ...) _MSTL logger::instance().debug(_MSTL format(msg, __VA_ARGS__), __FILE__, __func__, __LINE__)

/** @brief 格式化INFO级别日志 */
#define MSTL_LOGF_INFO(msg, ...)  _MSTL logger::instance().info(_MSTL format(msg, __VA_ARGS__),  __FILE__, __func__, __LINE__)

/** @brief 格式化WARN级别日志 */
#define MSTL_LOGF_WARN(msg, ...)  _MSTL logger::instance().warn(_MSTL format(msg, __VA_ARGS__),  __FILE__, __func__, __LINE__)

/** @brief 格式化ERROR级别日志 */
#define MSTL_LOGF_ERROR(msg, ...) _MSTL logger::instance().error(_MSTL format(msg, __VA_ARGS__), __FILE__, __func__, __LINE__)

/** @brief 格式化FATAL级别日志 */
#define MSTL_LOGF_FATAL(msg, ...) _MSTL logger::instance().fatal(_MSTL format(msg, __VA_ARGS__), __FILE__, __func__, __LINE__)

/** @} */ // Logging

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_LOGGING_LOGGER_HPP__
