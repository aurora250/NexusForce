#ifndef MSTL_LOGGING_LOG_FORMATTER_HPP__
#define MSTL_LOGGING_LOG_FORMATTER_HPP__

/**
 * @file log_formatter.hpp
 * @brief 日志格式化器
 *
 * 此文件提供了日志格式化功能，支持自定义格式模式。
 * 格式模式中可以包含占位符，在格式化时被替换为实际值。
 */

#include "MSTL/core/container/vector.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup Logging 日志系统
 * @brief 日志记录和管理功能
 * @{
 */

/**
 * @class log_formatter
 * @brief 日志格式化器
 *
 * 根据指定的模式字符串格式化日志事件。
 * 支持以下占位符：
 * - {time}     : 时间戳
 * - {level}    : 日志级别
 * - {file}     : 源文件名
 * - {line}     : 行号
 * - {func}     : 函数名
 * - {thread}   : 线程ID
 * - {message}  : 日志消息
 * - {context.key} : 上下文中的指定键
 *
 * 示例模式："[{time}] [{level}] {file}:{line} - {message}"
 */
class MSTL_API log_formatter {
private:
    /**
     * @struct part
     * @brief 格式模式的一部分
     *
     * 将格式模式解析为文本段和占位符段。
     */
    struct part {
        bool is_placeholder;  ///< 是否为占位符
        string text;          ///< 文本内容或占位符名称

        part(const bool is_ph, string t) noexcept
        : is_placeholder(is_ph), text(_MSTL move(t)) {}
    };
    string pattern_;          ///< 原始格式模式
    vector<part> parts_;      ///< 解析后的各部分

    /**
     * @brief 解析格式模式
     *
     * 将模式字符串解析为文本段和占位符段的列表。
     */
    void parse_pattern();

    /**
     * @brief 解析占位符
     * @param ph 占位符名称
     * @param event 日志事件
     * @return 占位符对应的值
     */
    string resolve_placeholder(string ph, const log_event& event) const;

public:
    /**
     * @brief 构造函数
     * @param pattern 格式模式字符串
     */
    explicit log_formatter(string pattern);

    /**
     * @brief 格式化日志事件
     * @param event 要格式化的日志事件
     * @return 格式化后的字符串
     */
    MSTL_NODISCARD string format(const log_event& event);
};

/** @} */ // Logging

MSTL_END_NAMESPACE__
#endif // MSTL_LOGGING_LOG_FORMATTER_HPP__
