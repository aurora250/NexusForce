#ifndef NEFORCE_CORE_LOGGING_LOGGER_HPP__
#define NEFORCE_CORE_LOGGING_LOGGER_HPP__

/**
 * @file logger.hpp
 * @brief 层级化日志记录器
 *
 * 提供完整的日志记录器系统：
 * - 线程安全的有界环形队列
 * - 层级化命名 Logger（"app.module.sub"）
 * - 全局 Logger 注册中心
 * - 完整的便捷日志宏系统
 */

#include "NeForce/core/async/thread_pool.hpp"
#include "NeForce/core/container/queue.hpp"
#include "NeForce/core/memory/bounded_queue.hpp"
#include "NeForce/logging/log_sink.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup Logging 日志系统
 * @{
 */

class logger_registry;

/**
 * @class logger
 * @brief 层级化日志记录器
 *
 * 每个 logger 有唯一的名称（如 "app.network.tcp"），
 * 通过 '.' 分隔形成层级。子 logger 默认继承父 logger 的级别。
 *
 * 主要功能：
 * - 运行时级别过滤与继承（INHERIT 标记）
 * - 同步 / 线程池异步双模式
 * - COW 上下文（零拷贝共享）
 * - 自定义过滤器
 * - 多 sink 输出
 * - 自动刷新
 */
class NEFORCE_API logger : public enable_shared_from_this<logger> {
    friend class logger_registry;

public:
    /**
     * @brief 特殊级别值：表示继承父 logger 的级别
     */
    static constexpr log_level INHERIT = static_cast<log_level>(255);

private:
    string name_;                                ///< Logger 唯一名称
    log_level explicit_level_{INHERIT};          ///< 显式设置的级别（INHERIT=继承父级）
    log_level effective_level_{log_level::INFO}; ///< 当前生效的级别
    logger* parent_{nullptr};                    ///< 父 Logger 指针
    vector<shared_ptr<logger>> children_;        ///< 子 Logger 列表

    vector<shared_ptr<log_sink>> sinks_; ///< 输出目标列表
    mutex sinks_mutex_;                  ///< sinks 互斥锁

    atomic<bool> async_{false};                                ///< 是否处于异步模式
    shared_ptr<thread_pool> thread_pool_;                      ///< 异步处理线程池
    atomic<overflow_policy> overflow_{overflow_policy::block}; ///< 队列溢出策略
    static constexpr size_t DEFAULT_QUEUE_SIZE = 8192;         ///< 默认队列容量
    bounded_queue<log_event> async_queue_{DEFAULT_QUEUE_SIZE}; ///< 异步事件队列
    mutex queue_mutex_;                                        ///< 队列互斥锁
    condition_variable queue_cv_;                              ///< 队列条件变量
    atomic<bool> drain_scheduled_{false};                      ///< 是否已提交 drain 任务
    atomic<bool> running_{false};                              ///< 异步处理循环是否运行中

    atomic<bool> flush_requested_{false}; ///< 是否请求了 flush
    mutex flush_mutex_;                   ///< flush 互斥锁
    condition_variable flush_cv_;         ///< flush 条件变量

    function<bool(const log_event&)> filter_; ///< 自定义过滤器
    mutex filter_mutex_;                      ///< filter 互斥锁

    shared_ptr<unordered_map<string, string>> context_data_{
            make_shared<unordered_map<string, string>>()}; ///< COW 上下文数据
    mutable mutex context_mutex_;                          ///< 上下文互斥锁

    atomic<int64_t> auto_flush_ms_{0};            ///< 自动刷新间隔毫秒
    timestamp last_auto_flush_{timestamp::now()}; ///< 上次自动刷新时间

    void update_effective_level();
    void propagate_effective_level(log_level new_effective);

    void enqueue_async(log_event&& event);

    void submit_drain();
    void drain_events();

    void register_child(shared_ptr<logger> child);

    void process_event_direct(const log_event& event);

    void check_auto_flush();

public:
    ~logger();

    /**
     * @brief 构造指定名称的 Logger
     * @param name Logger 名称（点分隔的层级名）
     */
    explicit logger(string name);

    logger(const logger&) = delete;
    logger& operator=(const logger&) = delete;
    logger(logger&&) = delete;
    logger& operator=(logger&&) = delete;

    /** @return Logger 名称 */
    NEFORCE_NODISCARD const string& name() const noexcept { return name_; }

    /** @return 父 Logger 指针，根 Logger 返回 nullptr */
    NEFORCE_NODISCARD logger* parent() const noexcept { return parent_; }

    /**
     * @brief 设置级别（不影响子 logger 的显式级别）
     * @param level 新级别，或 INHERIT 表示继承父级
     */
    void set_level(log_level level);

    /** @return 显式设置的级别（可能是 INHERIT） */
    NEFORCE_NODISCARD log_level level() const noexcept { return explicit_level_; }

    /** @return 当前生效的级别（继承已解析） */
    NEFORCE_NODISCARD log_level effective_level() const noexcept { return effective_level_; }

    /**
     * @brief 检查指定级别是否应该输出
     * @param level 待检查的日志级别
     * @return 该级别是否达到当前生效级别
     */
    NEFORCE_NODISCARD bool should_log(log_level level) const noexcept {
        return level >= effective_level_ && level < log_level::OFF;
    }

    /**
     * @brief 添加输出目标
     * @param sink 输出目标
     */
    void add_sink(shared_ptr<log_sink> sink);

    /**
     * @brief 清除所有输出目标
     */
    void clear_sinks();

    /**
     * @brief 启用异步模式
     * @param pool 线程池（共享），nullptr 则创建专用池
     * @param queue_size 队列容量
     * @param policy 溢出策略
     */
    void enable_async(shared_ptr<thread_pool> pool = nullptr, size_t queue_size = 8192,
                      overflow_policy policy = overflow_policy::block);

    /** @brief 切换回同步模式，排空队列中的所有事件 */
    void disable_async();

    /** @return 是否处于异步模式 */
    NEFORCE_NODISCARD bool is_async() const noexcept { return async_.load(memory_order_acquire); }

    /**
     * @brief 设置自定义过滤器
     * @param filter 过滤函数，返回 true 表示允许输出
     */
    void set_filter(function<bool(const log_event&)> filter);

    /**
     * @brief 添加上下文键值对
     * @param key 键名
     * @param value 值
     */
    void add_context(const string& key, string value);

    /**
     * @brief 移除上下文键
     * @param key 键名
     */
    void remove_context(const string& key);

    /** @brief 清除所有上下文 */
    void clear_context();

    /**
     * @brief 设置自动刷新间隔
     * @param interval_ms 间隔毫秒数，0=禁用
     */
    void set_auto_flush(int64_t interval_ms) { auto_flush_ms_.store(interval_ms, memory_order_release); }

    /**
     * @brief 记录一条日志
     * @param level 日志级别
     * @param msg 格式化后的消息
     * @param loc 源码位置
     */
    void log(log_level level, string msg, source_location loc);

    /** @brief TRACE 级别快捷方法 */
    void trace(string msg, source_location loc) { log(log_level::TRACE, move(msg), loc); }

    /** @brief DEBUG 级别快捷方法 */
    void debug(string msg, source_location loc) { log(log_level::DEBUG, move(msg), loc); }

    /** @brief INFO 级别快捷方法 */
    void info(string msg, source_location loc) { log(log_level::INFO, move(msg), loc); }

    /** @brief WARN 级别快捷方法 */
    void warn(string msg, source_location loc) { log(log_level::WARN, move(msg), loc); }

    /** @brief ERROR 级别快捷方法 */
    void error(string msg, source_location loc) { log(log_level::ERROR, move(msg), loc); }

    /** @brief FATAL 级别快捷方法 */
    void fatal(string msg, source_location loc) { log(log_level::FATAL, move(msg), loc); }

    /** @brief 刷新所有 sink */
    void flush();
};


/**
 * @class logger_registry
 * @brief 全局 Logger 注册中心
 *
 * 管理所有 Logger 的创建与查找。自动构建 '.' 分隔的层级关系。
 * 使用双重检查锁定保证线程安全。
 */
class NEFORCE_API logger_registry {
private:
    shared_ptr<logger> root_;                           ///< 根 Logger
    unordered_map<string, shared_ptr<logger>> loggers_; ///< 名称到 Logger 的映射
    mutex mutex_;                                       ///< 注册中心互斥锁

    logger_registry();
    shared_ptr<logger> create_logger(const string& name);

public:
    /** @brief 获取全局唯一实例 */
    static logger_registry& instance();

    logger_registry(const logger_registry&) = delete;
    logger_registry& operator=(const logger_registry&) = delete;

    /**
     * @brief 获取根 Logger（名称为空字符串）
     * @return 根 Logger
     */
    NEFORCE_NODISCARD shared_ptr<logger> root_logger();

    /**
     * @brief 获取或创建指定名称的 Logger
     * @param name 层级名称，如 "app.network.tcp"
     * @return 对应的 Logger 实例
     */
    shared_ptr<logger> get_logger(const string& name);

    /**
     * @brief 获取默认 Logger（即根 Logger）
     * @return 根 Logger
     */
    NEFORCE_NODISCARD shared_ptr<logger> default_logger() { return root_logger(); }

    /** @brief 刷新所有已注册的 Logger */
    void flush_all();
};


/// @cond INTERNAL_MACROS

/** @brief 获取默认 Logger（内部使用） */
#define NEFORCE_LOG_GET_LOGGER() _NEFORCE logger_registry::instance().default_logger()

/** @brief 日志宏主体：检查级别后调用 logger::log() */
#define NEFORCE_LOG_BODY(level, msg)                        \
    do {                                                    \
        auto _l = NEFORCE_LOG_GET_LOGGER();                 \
        if (_l->should_log(level)) {                        \
            _l->log(level, msg, NEFORCE_SOURCE_LOCATION()); \
        }                                                   \
    } while (false)

/** @brief 格式化日志宏主体 */
#define NEFORCE_LOGF_BODY(level, msg, ...)                                                \
    do {                                                                                  \
        auto _l = NEFORCE_LOG_GET_LOGGER();                                               \
        if (_l->should_log(level)) {                                                      \
            _l->log(level, _NEFORCE format(msg, __VA_ARGS__), NEFORCE_SOURCE_LOCATION()); \
        }                                                                                 \
    } while (false)

/** @brief 命名 Logger 日志宏主体 */
#define NEFORCE_LOGGER_LOG_BODY(level, name, msg)                        \
    do {                                                                 \
        auto _l = _NEFORCE logger_registry::instance().get_logger(name); \
        if (_l->should_log(level)) {                                     \
            _l->log(level, msg, NEFORCE_SOURCE_LOCATION());              \
        }                                                                \
    } while (false)

/** @brief 命名 Logger 格式化日志宏主体 */
#define NEFORCE_LOGGER_LOGF_BODY(level, name, msg, ...)                                   \
    do {                                                                                  \
        auto _l = _NEFORCE logger_registry::instance().get_logger(name);                  \
        if (_l->should_log(level)) {                                                      \
            _l->log(level, _NEFORCE format(msg, __VA_ARGS__), NEFORCE_SOURCE_LOCATION()); \
        }                                                                                 \
    } while (false)

/// @endcond


/**
 * @name 根 Logger 日志宏
 *
 * 使用默认（根）Logger 输出日志。
 * 低于 NEFORCE_ACTIVE_LOG_LEVEL 的宏在编译期被剥离为 ((void)0)。
 * @{
 */

// ---- TRACE ----
#if NEFORCE_ACTIVE_LOG_LEVEL <= NEFORCE_LOG_LEVEL_TRACE
/**
 * @brief 使用根 Logger 输出 TRACE 级别日志
 * @param msg 日志消息字符串
 */
#    define NEFORCE_LOG_TRACE(msg) NEFORCE_LOG_BODY(_NEFORCE log_level::TRACE, msg)
/**
 * @brief 使用根 Logger 输出格式化 TRACE 级别日志
 * @param msg 格式化字符串（fmt 风格）
 * @param ... 格式化参数
 */
#    define NEFORCE_LOGF_TRACE(msg, ...) NEFORCE_LOGF_BODY(_NEFORCE log_level::TRACE, msg, __VA_ARGS__)
/**
 * @brief 条件输出 TRACE 日志：仅当 cond 为 true 时输出
 * @param cond 条件表达式
 * @param msg 日志消息字符串
 */
#    define NEFORCE_LOG_TRACE_IF(cond, msg)                       \
        do {                                                      \
            if (cond)                                             \
                NEFORCE_LOG_BODY(_NEFORCE log_level::TRACE, msg); \
        } while (false)
/**
 * @brief 频率控制 TRACE 日志：每 n 次调用输出 1 次
 * @param n 输出间隔（每 n 次输出 1 次）
 * @param msg 日志消息字符串
 */
#    define NEFORCE_LOG_TRACE_EVERY_N(n, msg)                     \
        do {                                                      \
            static size_t _c = 0;                                 \
            if (++_c % n == 1)                                    \
                NEFORCE_LOG_BODY(_NEFORCE log_level::TRACE, msg); \
        } while (false)
/**
 * @brief 次数限制 TRACE 日志：仅前 n 次调用输出
 * @param n 最大输出次数
 * @param msg 日志消息字符串
 */
#    define NEFORCE_LOG_TRACE_FIRST_N(n, msg)                     \
        do {                                                      \
            static size_t _c = 0;                                 \
            if (_c++ < n)                                         \
                NEFORCE_LOG_BODY(_NEFORCE log_level::TRACE, msg); \
        } while (false)
#else
#    define NEFORCE_LOG_TRACE(msg) ((void) 0)
#    define NEFORCE_LOGF_TRACE(msg, ...) ((void) 0)
#    define NEFORCE_LOG_TRACE_IF(cond, msg) ((void) 0)
#    define NEFORCE_LOG_TRACE_EVERY_N(n, msg) ((void) 0)
#    define NEFORCE_LOG_TRACE_FIRST_N(n, msg) ((void) 0)
#endif

// ---- DEBUG ----
#if NEFORCE_ACTIVE_LOG_LEVEL <= NEFORCE_LOG_LEVEL_DEBUG
/**
 * @brief 使用根 Logger 输出 DEBUG 级别日志
 * @param msg 日志消息字符串
 */
#    define NEFORCE_LOG_DEBUG(msg) NEFORCE_LOG_BODY(_NEFORCE log_level::DEBUG, msg)
/**
 * @brief 使用根 Logger 输出格式化 DEBUG 级别日志
 * @param msg 格式化字符串（fmt 风格）
 * @param ... 格式化参数
 */
#    define NEFORCE_LOGF_DEBUG(msg, ...) NEFORCE_LOGF_BODY(_NEFORCE log_level::DEBUG, msg, __VA_ARGS__)
/**
 * @brief 条件输出 DEBUG 日志：仅当 cond 为 true 时输出
 * @param cond 条件表达式
 * @param msg 日志消息字符串
 */
#    define NEFORCE_LOG_DEBUG_IF(cond, msg)                       \
        do {                                                      \
            if (cond)                                             \
                NEFORCE_LOG_BODY(_NEFORCE log_level::DEBUG, msg); \
        } while (false)
/**
 * @brief 频率控制 DEBUG 日志：每 n 次调用输出 1 次
 * @param n 输出间隔（每 n 次输出 1 次）
 * @param msg 日志消息字符串
 */
#    define NEFORCE_LOG_DEBUG_EVERY_N(n, msg)                     \
        do {                                                      \
            static size_t _c = 0;                                 \
            if (++_c % n == 1)                                    \
                NEFORCE_LOG_BODY(_NEFORCE log_level::DEBUG, msg); \
        } while (false)
/**
 * @brief 次数限制 DEBUG 日志：仅前 n 次调用输出
 * @param n 最大输出次数
 * @param msg 日志消息字符串
 */
#    define NEFORCE_LOG_DEBUG_FIRST_N(n, msg)                     \
        do {                                                      \
            static size_t _c = 0;                                 \
            if (_c++ < n)                                         \
                NEFORCE_LOG_BODY(_NEFORCE log_level::DEBUG, msg); \
        } while (false)
#else
#    define NEFORCE_LOG_DEBUG(msg) ((void) 0)
#    define NEFORCE_LOGF_DEBUG(msg, ...) ((void) 0)
#    define NEFORCE_LOG_DEBUG_IF(cond, msg) ((void) 0)
#    define NEFORCE_LOG_DEBUG_EVERY_N(n, msg) ((void) 0)
#    define NEFORCE_LOG_DEBUG_FIRST_N(n, msg) ((void) 0)
#endif

// ---- INFO ----
#if NEFORCE_ACTIVE_LOG_LEVEL <= NEFORCE_LOG_LEVEL_INFO
/**
 * @brief 使用根 Logger 输出 INFO 级别日志
 * @param msg 日志消息字符串
 */
#    define NEFORCE_LOG_INFO(msg) NEFORCE_LOG_BODY(_NEFORCE log_level::INFO, msg)
/**
 * @brief 使用根 Logger 输出格式化 INFO 级别日志
 * @param msg 格式化字符串（fmt 风格）
 * @param ... 格式化参数
 */
#    define NEFORCE_LOGF_INFO(msg, ...) NEFORCE_LOGF_BODY(_NEFORCE log_level::INFO, msg, __VA_ARGS__)
/**
 * @brief 条件输出 INFO 日志：仅当 cond 为 true 时输出
 * @param cond 条件表达式
 * @param msg 日志消息字符串
 */
#    define NEFORCE_LOG_INFO_IF(cond, msg)                       \
        do {                                                     \
            if (cond)                                            \
                NEFORCE_LOG_BODY(_NEFORCE log_level::INFO, msg); \
        } while (false)
/**
 * @brief 频率控制 INFO 日志：每 n 次调用输出 1 次
 * @param n 输出间隔（每 n 次输出 1 次）
 * @param msg 日志消息字符串
 */
#    define NEFORCE_LOG_INFO_EVERY_N(n, msg)                     \
        do {                                                     \
            static size_t _c = 0;                                \
            if (++_c % n == 1)                                   \
                NEFORCE_LOG_BODY(_NEFORCE log_level::INFO, msg); \
        } while (false)
/**
 * @brief 次数限制 INFO 日志：仅前 n 次调用输出
 * @param n 最大输出次数
 * @param msg 日志消息字符串
 */
#    define NEFORCE_LOG_INFO_FIRST_N(n, msg)                     \
        do {                                                     \
            static size_t _c = 0;                                \
            if (_c++ < n)                                        \
                NEFORCE_LOG_BODY(_NEFORCE log_level::INFO, msg); \
        } while (false)
#else
#    define NEFORCE_LOG_INFO(msg) ((void) 0)
#    define NEFORCE_LOGF_INFO(msg, ...) ((void) 0)
#    define NEFORCE_LOG_INFO_IF(cond, msg) ((void) 0)
#    define NEFORCE_LOG_INFO_EVERY_N(n, msg) ((void) 0)
#    define NEFORCE_LOG_INFO_FIRST_N(n, msg) ((void) 0)
#endif

// ---- WARN ----
#if NEFORCE_ACTIVE_LOG_LEVEL <= NEFORCE_LOG_LEVEL_WARN
/**
 * @brief 使用根 Logger 输出 WARN 级别日志
 * @param msg 日志消息字符串
 */
#    define NEFORCE_LOG_WARN(msg) NEFORCE_LOG_BODY(_NEFORCE log_level::WARN, msg)
/**
 * @brief 使用根 Logger 输出格式化 WARN 级别日志
 * @param msg 格式化字符串（fmt 风格）
 * @param ... 格式化参数
 */
#    define NEFORCE_LOGF_WARN(msg, ...) NEFORCE_LOGF_BODY(_NEFORCE log_level::WARN, msg, __VA_ARGS__)
/**
 * @brief 条件输出 WARN 日志：仅当 cond 为 true 时输出
 * @param cond 条件表达式
 * @param msg 日志消息字符串
 */
#    define NEFORCE_LOG_WARN_IF(cond, msg)                       \
        do {                                                     \
            if ((cond))                                          \
                NEFORCE_LOG_BODY(_NEFORCE log_level::WARN, msg); \
        } while (false)
/**
 * @brief 频率控制 WARN 日志：每 n 次调用输出 1 次
 * @param n 输出间隔（每 n 次输出 1 次）
 * @param msg 日志消息字符串
 */
#    define NEFORCE_LOG_WARN_EVERY_N(n, msg)                     \
        do {                                                     \
            static size_t _c = 0;                                \
            if (++_c % n == 1)                                   \
                NEFORCE_LOG_BODY(_NEFORCE log_level::WARN, msg); \
        } while (false)
/**
 * @brief 次数限制 WARN 日志：仅前 n 次调用输出
 * @param n 最大输出次数
 * @param msg 日志消息字符串
 */
#    define NEFORCE_LOG_WARN_FIRST_N(n, msg)                     \
        do {                                                     \
            static size_t _c = 0;                                \
            if (_c++ < n)                                        \
                NEFORCE_LOG_BODY(_NEFORCE log_level::WARN, msg); \
        } while (false)
#else
#    define NEFORCE_LOG_WARN(msg) ((void) 0)
#    define NEFORCE_LOGF_WARN(msg, ...) ((void) 0)
#    define NEFORCE_LOG_WARN_IF(cond, msg) ((void) 0)
#    define NEFORCE_LOG_WARN_EVERY_N(n, msg) ((void) 0)
#    define NEFORCE_LOG_WARN_FIRST_N(n, msg) ((void) 0)
#endif

// ---- ERROR ----
#if NEFORCE_ACTIVE_LOG_LEVEL <= NEFORCE_LOG_LEVEL_ERROR
/**
 * @brief 使用根 Logger 输出 ERROR 级别日志
 * @param msg 日志消息字符串
 */
#    define NEFORCE_LOG_ERROR(msg) NEFORCE_LOG_BODY(_NEFORCE log_level::ERROR, msg)
/**
 * @brief 使用根 Logger 输出格式化 ERROR 级别日志
 * @param msg 格式化字符串（fmt 风格）
 * @param ... 格式化参数
 */
#    define NEFORCE_LOGF_ERROR(msg, ...) NEFORCE_LOGF_BODY(_NEFORCE log_level::ERROR, msg, __VA_ARGS__)
/**
 * @brief 条件输出 ERROR 日志：仅当 cond 为 true 时输出
 * @param cond 条件表达式
 * @param msg 日志消息字符串
 */
#    define NEFORCE_LOG_ERROR_IF(cond, msg)                       \
        do {                                                      \
            if (cond)                                             \
                NEFORCE_LOG_BODY(_NEFORCE log_level::ERROR, msg); \
        } while (false)
/**
 * @brief 频率控制 ERROR 日志：每 n 次调用输出 1 次
 * @param n 输出间隔（每 n 次输出 1 次）
 * @param msg 日志消息字符串
 */
#    define NEFORCE_LOG_ERROR_EVERY_N(n, msg)                     \
        do {                                                      \
            static size_t _c = 0;                                 \
            if (++_c % n == 1)                                    \
                NEFORCE_LOG_BODY(_NEFORCE log_level::ERROR, msg); \
        } while (false)
/**
 * @brief 次数限制 ERROR 日志：仅前 n 次调用输出
 * @param n 最大输出次数
 * @param msg 日志消息字符串
 */
#    define NEFORCE_LOG_ERROR_FIRST_N(n, msg)                     \
        do {                                                      \
            static size_t _c = 0;                                 \
            if (_c++ < n)                                         \
                NEFORCE_LOG_BODY(_NEFORCE log_level::ERROR, msg); \
        } while (false)
#else
#    define NEFORCE_LOG_ERROR(msg) ((void) 0)
#    define NEFORCE_LOGF_ERROR(msg, ...) ((void) 0)
#    define NEFORCE_LOG_ERROR_IF(cond, msg) ((void) 0)
#    define NEFORCE_LOG_ERROR_EVERY_N(n, msg) ((void) 0)
#    define NEFORCE_LOG_ERROR_FIRST_N(n, msg) ((void) 0)
#endif

// ---- FATAL ----
#if NEFORCE_ACTIVE_LOG_LEVEL <= NEFORCE_LOG_LEVEL_FATAL
/**
 * @brief 使用根 Logger 输出 FATAL 级别日志
 * @param msg 日志消息字符串
 */
#    define NEFORCE_LOG_FATAL(msg) NEFORCE_LOG_BODY(_NEFORCE log_level::FATAL, msg)
/**
 * @brief 使用根 Logger 输出格式化 FATAL 级别日志
 * @param msg 格式化字符串（fmt 风格）
 * @param ... 格式化参数
 */
#    define NEFORCE_LOGF_FATAL(msg, ...) NEFORCE_LOGF_BODY(_NEFORCE log_level::FATAL, msg, __VA_ARGS__)
/**
 * @brief 条件输出 FATAL 日志：仅当 cond 为 true 时输出
 * @param cond 条件表达式
 * @param msg 日志消息字符串
 */
#    define NEFORCE_LOG_FATAL_IF(cond, msg)                       \
        do {                                                      \
            if (cond)                                             \
                NEFORCE_LOG_BODY(_NEFORCE log_level::FATAL, msg); \
        } while (false)
/**
 * @brief 频率控制 FATAL 日志：每 n 次调用输出 1 次
 * @param n 输出间隔（每 n 次输出 1 次）
 * @param msg 日志消息字符串
 */
#    define NEFORCE_LOG_FATAL_EVERY_N(n, msg)                     \
        do {                                                      \
            static size_t _c = 0;                                 \
            if (++_c % n == 1)                                    \
                NEFORCE_LOG_BODY(_NEFORCE log_level::FATAL, msg); \
        } while (false)
/**
 * @brief 次数限制 FATAL 日志：仅前 n 次调用输出
 * @param n 最大输出次数
 * @param msg 日志消息字符串
 */
#    define NEFORCE_LOG_FATAL_FIRST_N(n, msg)                     \
        do {                                                      \
            static size_t _c = 0;                                 \
            if (_c++ < n)                                         \
                NEFORCE_LOG_BODY(_NEFORCE log_level::FATAL, msg); \
        } while (false)
#else
#    define NEFORCE_LOG_FATAL(msg) ((void) 0)
#    define NEFORCE_LOGF_FATAL(msg, ...) ((void) 0)
#    define NEFORCE_LOG_FATAL_IF(cond, msg) ((void) 0)
#    define NEFORCE_LOG_FATAL_EVERY_N(n, msg) ((void) 0)
#    define NEFORCE_LOG_FATAL_FIRST_N(n, msg) ((void) 0)
#endif

/** @} */ // 根 Logger 日志宏


/**
 * @name 命名 Logger 日志宏
 *
 * 使用指定名称的 Logger 输出日志。Logger 在首次使用时通过
 * logger_registry::get_logger() 自动创建。
 * @{
 */

// ---- TRACE (named) ----
#if NEFORCE_ACTIVE_LOG_LEVEL <= NEFORCE_LOG_LEVEL_TRACE
/**
 * @brief 使用命名 Logger 输出 TRACE 级别日志
 * @param name Logger 名称（点分隔层级，如 "app.network"）
 * @param msg 日志消息字符串
 */
#    define NEFORCE_LOGGER_LOG_TRACE(name, msg) NEFORCE_LOGGER_LOG_BODY(_NEFORCE log_level::TRACE, name, msg)
/**
 * @brief 使用命名 Logger 输出格式化 TRACE 级别日志
 * @param name Logger 名称
 * @param msg 格式化字符串（fmt 风格）
 * @param ... 格式化参数
 */
#    define NEFORCE_LOGGER_LOGF_TRACE(name, msg, ...) \
        NEFORCE_LOGGER_LOGF_BODY(_NEFORCE log_level::TRACE, name, msg, __VA_ARGS__)
#else
#    define NEFORCE_LOGGER_LOG_TRACE(name, msg) ((void) 0)
#    define NEFORCE_LOGGER_LOGF_TRACE(name, msg, ...) ((void) 0)
#endif

// ---- DEBUG (named) ----
#if NEFORCE_ACTIVE_LOG_LEVEL <= NEFORCE_LOG_LEVEL_DEBUG
/**
 * @brief 使用命名 Logger 输出 DEBUG 级别日志
 * @param name Logger 名称
 * @param msg 日志消息字符串
 */
#    define NEFORCE_LOGGER_LOG_DEBUG(name, msg) NEFORCE_LOGGER_LOG_BODY(_NEFORCE log_level::DEBUG, name, msg)
/**
 * @brief 使用命名 Logger 输出格式化 DEBUG 级别日志
 * @param name Logger 名称
 * @param msg 格式化字符串（fmt 风格）
 * @param ... 格式化参数
 */
#    define NEFORCE_LOGGER_LOGF_DEBUG(name, msg, ...) \
        NEFORCE_LOGGER_LOGF_BODY(_NEFORCE log_level::DEBUG, name, msg, __VA_ARGS__)
#else
#    define NEFORCE_LOGGER_LOG_DEBUG(name, msg) ((void) 0)
#    define NEFORCE_LOGGER_LOGF_DEBUG(name, msg, ...) ((void) 0)
#endif

// ---- INFO (named) ----
#if NEFORCE_ACTIVE_LOG_LEVEL <= NEFORCE_LOG_LEVEL_INFO
/**
 * @brief 使用命名 Logger 输出 INFO 级别日志
 * @param name Logger 名称
 * @param msg 日志消息字符串
 */
#    define NEFORCE_LOGGER_LOG_INFO(name, msg) NEFORCE_LOGGER_LOG_BODY(_NEFORCE log_level::INFO, name, msg)
/**
 * @brief 使用命名 Logger 输出格式化 INFO 级别日志
 * @param name Logger 名称
 * @param msg 格式化字符串（fmt 风格）
 * @param ... 格式化参数
 */
#    define NEFORCE_LOGGER_LOGF_INFO(name, msg, ...) \
        NEFORCE_LOGGER_LOGF_BODY(_NEFORCE log_level::INFO, name, msg, __VA_ARGS__)
#else
#    define NEFORCE_LOGGER_LOG_INFO(name, msg) ((void) 0)
#    define NEFORCE_LOGGER_LOGF_INFO(name, msg, ...) ((void) 0)
#endif

// ---- WARN (named) ----
#if NEFORCE_ACTIVE_LOG_LEVEL <= NEFORCE_LOG_LEVEL_WARN
/**
 * @brief 使用命名 Logger 输出 WARN 级别日志
 * @param name Logger 名称
 * @param msg 日志消息字符串
 */
#    define NEFORCE_LOGGER_LOG_WARN(name, msg) NEFORCE_LOGGER_LOG_BODY(_NEFORCE log_level::WARN, name, msg)
/**
 * @brief 使用命名 Logger 输出格式化 WARN 级别日志
 * @param name Logger 名称
 * @param msg 格式化字符串（fmt 风格）
 * @param ... 格式化参数
 */
#    define NEFORCE_LOGGER_LOGF_WARN(name, msg, ...) \
        NEFORCE_LOGGER_LOGF_BODY(_NEFORCE log_level::WARN, name, msg, __VA_ARGS__)
#else
#    define NEFORCE_LOGGER_LOG_WARN(name, msg) ((void) 0)
#    define NEFORCE_LOGGER_LOGF_WARN(name, msg, ...) ((void) 0)
#endif

// ---- ERROR (named) ----
#if NEFORCE_ACTIVE_LOG_LEVEL <= NEFORCE_LOG_LEVEL_ERROR
/**
 * @brief 使用命名 Logger 输出 ERROR 级别日志
 * @param name Logger 名称
 * @param msg 日志消息字符串
 */
#    define NEFORCE_LOGGER_LOG_ERROR(name, msg) NEFORCE_LOGGER_LOG_BODY(_NEFORCE log_level::ERROR, name, msg)
/**
 * @brief 使用命名 Logger 输出格式化 ERROR 级别日志
 * @param name Logger 名称
 * @param msg 格式化字符串（fmt 风格）
 * @param ... 格式化参数
 */
#    define NEFORCE_LOGGER_LOGF_ERROR(name, msg, ...) \
        NEFORCE_LOGGER_LOGF_BODY(_NEFORCE log_level::ERROR, name, msg, __VA_ARGS__)
#else
#    define NEFORCE_LOGGER_LOG_ERROR(name, msg) ((void) 0)
#    define NEFORCE_LOGGER_LOGF_ERROR(name, msg, ...) ((void) 0)
#endif

// ---- FATAL (named) ----
#if NEFORCE_ACTIVE_LOG_LEVEL <= NEFORCE_LOG_LEVEL_FATAL
/**
 * @brief 使用命名 Logger 输出 FATAL 级别日志
 * @param name Logger 名称
 * @param msg 日志消息字符串
 */
#    define NEFORCE_LOGGER_LOG_FATAL(name, msg) NEFORCE_LOGGER_LOG_BODY(_NEFORCE log_level::FATAL, name, msg)
/**
 * @brief 使用命名 Logger 输出格式化 FATAL 级别日志
 * @param name Logger 名称
 * @param msg 格式化字符串（fmt 风格）
 * @param ... 格式化参数
 */
#    define NEFORCE_LOGGER_LOGF_FATAL(name, msg, ...) \
        NEFORCE_LOGGER_LOGF_BODY(_NEFORCE log_level::FATAL, name, msg, __VA_ARGS__)
#else
#    define NEFORCE_LOGGER_LOG_FATAL(name, msg) ((void) 0)
#    define NEFORCE_LOGGER_LOGF_FATAL(name, msg, ...) ((void) 0)
#endif

/** @} */ // 命名 Logger 日志宏


/**
 * @def NEFORCE_GET_LOGGER(name)
 * @brief 创建或获取命名 Logger
 * @param name Logger 名称（点分隔的层级名，如 "app.network.tcp"）
 */
#define NEFORCE_GET_LOGGER(name) _NEFORCE logger_registry::instance().get_logger(name)

/**
 * @def NEFORCE_ROOT_LOGGER()
 * @brief 获取根 Logger
 */
#define NEFORCE_ROOT_LOGGER() _NEFORCE logger_registry::instance().root_logger()


/** @} */ // Logging

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_LOGGING_LOGGER_HPP__
