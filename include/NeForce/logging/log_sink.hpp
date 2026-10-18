#ifndef NEFORCE_LOGGING_LOG_SINK_HPP__
#define NEFORCE_LOGGING_LOG_SINK_HPP__

/**
 * @file log_sink.hpp
 * @brief 日志输出目标
 *
 * 定义了日志输出目标的抽象基类和具体实现：
 * - console_sink: 彩色控制台输出
 * - syslog_sink: 系统日志输出
 */

#include "NeForce/core/async/mutex.hpp"
#include "NeForce/core/memory/unique_ptr.hpp"
#include "NeForce/core/time/datetime.hpp"
#include "NeForce/core/utility/color.hpp"
#include "NeForce/logging/log_event.hpp"
#include "NeForce/logging/log_formatter.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup Logging 日志系统
 * @{
 */

/**
 * @brief 默认格式化函数
 * @param ev 日志事件
 * @return 格式为 "[LEVEL] message" 的字符串
 */
inline string default_sink_format(const log_event& ev) { return "["_s + to_string(ev.level) + "] " + ev.message; }

/**
 * @brief 获取日志级别对应的控制台颜色
 * @param level 日志级别
 * @return 对应颜色的引用
 */
NEFORCE_API const color& level_color(log_level level);


/**
 * @class log_sink
 * @brief 日志输出目标基类
 *
 * 所有日志输出目标的抽象基类。
 * 支持设置自定义格式化器和自动刷新间隔。
 */
class NEFORCE_API log_sink {
protected:
    unique_ptr<log_formatter> formatter_;    ///< 格式化器
    mutable mutex formatter_mutex_;          ///< 格式化器互斥锁
    int64_t flush_interval_ms_{0};           ///< 自动刷新间隔毫秒数（0=禁用）
    timestamp last_flush_{timestamp::now()}; ///< 上次刷新的时间戳

    /**
     * @brief 用当前格式化器格式化一条事件
     * @param event 要格式化的日志事件
     * @return 格式化后的文本
     * @note 与 set_formatter() 并发安全
     */
    NEFORCE_NODISCARD string format_event(const log_event& event) const;

public:
    virtual ~log_sink() = default;

    /**
     * @brief 输出一条日志事件
     * @param event 要输出的日志事件
     */
    virtual void log(const log_event& event) = 0;

    /** @brief 强制刷新缓冲区 */
    virtual void flush() = 0;

    /**
     * @brief 设置格式化器
     * @param formatter 新的格式化器
     * @note 可以在输出过程中调用，正在格式化的事件仍使用旧格式化器
     */
    void set_formatter(unique_ptr<log_formatter> formatter);

    /**
     * @brief 设置自动刷新间隔
     * @param interval_ms 间隔毫秒数，0 表示禁用自动刷新
     * @warning 与 log() 的自动刷新判断存在竞争，仅应在开始输出前调用
     */
    void set_flush_interval(int64_t interval_ms) { flush_interval_ms_ = interval_ms; }

    /**
     * @brief 检查是否需要自动刷新
     * @return 距上次刷新已超过间隔则返回 true
     */
    NEFORCE_NODISCARD bool should_auto_flush() const {
        return flush_interval_ms_ > 0 && (timestamp::now() - last_flush_).value() >= flush_interval_ms_ * 1000;
    }
};


/**
 * @class console_sink
 * @brief 控制台输出目标
 *
 * 将日志输出到标准控制台，按日志级别着色显示。
 */
class NEFORCE_API console_sink final : public log_sink {
public:
    /**
     * @brief 输出日志到控制台
     * @param event 日志事件
     */
    void log(const log_event& event) override;

    /** @brief 刷新控制台缓冲区 */
    void flush() override;
};


#ifdef NEFORCE_PLATFORM_LINUX

/**
 * @enum syslog_facility
 * @brief Syslog 设施类型
 */
enum class syslog_facility : int {
    auth = 1 << 3,    ///< 安全/授权消息
    cron = 9 << 3,    ///< 定时任务守护进程
    daemon = 3 << 3,  ///< 系统守护进程
    kernel = 0 << 3,  ///< 内核消息
    local0 = 16 << 3, ///< 本地自定义设施 0
    local1 = 17 << 3, ///< 本地自定义设施 1
    local2 = 18 << 3, ///< 本地自定义设施 2
    local3 = 19 << 3, ///< 本地自定义设施 3
    local4 = 20 << 3, ///< 本地自定义设施 4
    local5 = 21 << 3, ///< 本地自定义设施 5
    local6 = 22 << 3, ///< 本地自定义设施 6
    local7 = 23 << 3, ///< 本地自定义设施 7
    lpr = 6 << 3,     ///< 行式打印机子系统
    mail = 2 << 3,    ///< 邮件系统
    news = 7 << 3,    ///< USENET 新闻
    syslog = 5 << 3,  ///< syslogd 内部消息
    user = 1 << 3,    ///< 通用用户级消息（默认）
    uucp = 8 << 3,    ///< UUCP 子系统
};

/**
 * @class syslog_sink
 * @brief 系统日志输出目标
 *
 * 将日志写入 Linux syslog。
 */
class NEFORCE_API syslog_sink final : public log_sink {
private:
    string ident_;             ///< syslog ident 字符串
    syslog_facility facility_; ///< syslog 设施类型
    bool opened_{false};       ///< 是否已调用 openlog()

    void ensure_open();

public:
    /**
     * @brief 构造 syslog 输出目标
     * @param ident 日志标识字符串，对应 syslog 的 ident 参数
     * @param facility syslog 设施，默认为 user
     */
    explicit syslog_sink(string ident = "", syslog_facility facility = syslog_facility::user);

    ~syslog_sink() override;

    syslog_sink(const syslog_sink&) = delete;
    syslog_sink& operator=(const syslog_sink&) = delete;

    /**
     * @brief 将日志事件写入 syslog
     * @param event 日志事件
     */
    void log(const log_event& event) override;

    /**
     * @brief 刷新 syslog
     * @note syslog 每次调用立即写入，无需显式刷新
     */
    void flush() override;
};

#endif

/** @} */ // Logging

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_LOGGING_LOG_SINK_HPP__
