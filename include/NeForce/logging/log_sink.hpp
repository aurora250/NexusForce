#ifndef NEFORCE_LOGGING_LOG_SINK_HPP__
#define NEFORCE_LOGGING_LOG_SINK_HPP__

/**
 * @file log_sink.hpp
 * @brief 日志输出目标
 *
 * 此文件定义了日志输出目标的抽象基类和具体实现。
 * 日志可以输出到控制台、文件等不同目标。
 */

#include "log_event.hpp"
#include "log_formatter.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Logging 日志系统
 * @brief 日志记录和管理功能
 * @{
 */

inline string default_sink_format(const log_event& ev) { return "["_s + to_string(ev.level) + "] " + ev.message; }

/**
 * @class log_sink
 * @brief 日志输出目标基类
 *
 * 所有日志输出目标的抽象基类，定义了日志输出和刷新的接口。
 * 支持设置自定义格式化器。
 */
class NEFORCE_API log_sink {
protected:
    unique_ptr<log_formatter> formatter_; ///< 日志格式化器

public:
    virtual ~log_sink() = default;

    /**
     * @brief 输出日志事件
     * @param event 要输出的日志事件
     *
     * 纯虚函数，由派生类实现具体的输出逻辑。
     */
    virtual void log(const log_event& event) = 0;

    /**
     * @brief 刷新缓冲区
     *
     * 确保所有缓存的日志都被实际输出。
     */
    virtual void flush() = 0;

    /**
     * @brief 设置格式化器
     * @param formatter 格式化器的唯一指针
     *
     * 可以为不同的输出目标设置不同的格式化方式。
     */
    void set_formatter(unique_ptr<log_formatter> formatter);
};


/**
 * @class console_sink
 * @brief 控制台输出目标
 *
 * 将日志输出到标准控制台，支持颜色显示。
 * 如果没有设置格式化器，使用默认格式。
 */
class NEFORCE_API console_sink final : public log_sink {
public:
    /**
     * @brief 输出日志到控制台
     * @param event 日志事件
     *
     * 使用设置的格式化器或默认格式将日志输出到控制台。
     */
    void log(const log_event& event) override;

    /**
     * @brief 刷新控制台缓冲区
     */
    void flush() override;
};

/** @} */ // Logging

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_LOGGING_LOG_SINK_HPP__
