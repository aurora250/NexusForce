#ifndef NEFORCE_LOGGING_LOG_FORMATTER_HPP__
#define NEFORCE_LOGGING_LOG_FORMATTER_HPP__

/**
 * @file log_formatter.hpp
 * @brief 日志格式化器
 *
 * 将 log_event 按用户定义的模式格式化为字符串。
 * 支持时间、级别、文件、行号、函数、线程、Logger名称、上下文等占位符。
 */

#include "NeForce/core/container/vector.hpp"
#include "NeForce/logging/log_event.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup Logging 日志系统
 * @{
 */

/**
 * @class log_formatter
 * @brief 日志格式化器
 *
 * 支持以下占位符：
 * - {time}     : 时间戳
 * - {level}    : 日志级别
 * - {file}     : 源文件名（仅文件名，不含路径）
 * - {filepath} : 源文件完整路径
 * - {line}     : 行号
 * - {func}     : 函数名
 * - {name}     : Logger 名称
 * - {thread}   : 线程ID
 * - {message}  : 日志消息
 * - {context.key} : 上下文中的指定键
 * - {color:start} / {color:end} : ANSI 颜色标记
 *
 * 示例模式："[{time}] [{level}] {file}:{line} - {message}"
 */
class NEFORCE_API log_formatter {
private:
    /**
     * @struct part
     * @brief 格式化模式的一个片段
     */
    struct part {
        bool is_placeholder; ///< 是否为占位符（true）或字面文本（false）
        string text;         ///< 占位符名或字面文本内容

        part(const bool is_ph, string t) noexcept :
        is_placeholder(is_ph),
        text(_NEFORCE move(t)) {}

        ~part() = default;
        part(const part&) = default;
        part& operator=(const part&) = default;
        part(part&&) noexcept = default;
        part& operator=(part&&) noexcept = default;
    };

    string pattern_;     ///< 原始格式模式字符串
    vector<part> parts_; ///< 解析后的片段列表

    void parse_pattern();
    NEFORCE_NODISCARD string resolve_placeholder(const string& ph, const log_event& event) const;

public:
    /**
     * @brief 从模式字符串构造格式化器
     * @param pattern 格式模式，如 "[{time}] [{level}] {message}"
     */
    explicit log_formatter(string pattern);

    ~log_formatter() = default;

    log_formatter(const log_formatter&) = default;
    log_formatter& operator=(const log_formatter&) = default;
    log_formatter(log_formatter&&) noexcept = default;
    log_formatter& operator=(log_formatter&&) noexcept = default;

    /**
     * @brief 格式化日志事件
     * @param event 要格式化的日志事件
     * @return 格式化后的字符串
     */
    NEFORCE_NODISCARD string format(const log_event& event);
};

/** @} */ // Logging

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_LOGGING_LOG_FORMATTER_HPP__
