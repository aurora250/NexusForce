#ifndef NEFORCE_CORE_STRING_VSPRINTF_HPP__
#define NEFORCE_CORE_STRING_VSPRINTF_HPP__

/**
 * @file vsprintf.hpp
 * @brief 兼容C格式化函数
 *
 * 此文件提供了格式化函数的声明，包括可变参数版本的格式化。
 */

#include "NeForce/core/typeinfo/types.hpp"
#include <cstdarg>
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup StringFormat 字符串格式化
 * @brief 字符串格式化功能
 * @{
 */

/**
 * @brief 格式化到缓冲区
 * @param buf 输出缓冲区
 * @param fmt 格式字符串
 * @param args 可变参数列表
 * @return 写入缓冲区的字符数，不包括终止字符
 *
 * 不检查缓冲区大小。
 *
 * @note 使用 MEMORY_BIG_ALLOC_THRESHHOLD 作为缓冲区大小限制
 */
int NEFORCE_API vsprintf(char* buf, const char* fmt, std::va_list args) noexcept;

/**
 * @brief 安全格式化到缓冲区
 * @param buf 输出缓冲区
 * @param size 缓冲区大小
 * @param fmt 格式字符串
 * @param args 可变参数列表
 * @return 应该写入缓冲区的字符数，不包括终止空字符
 *
 * 确保不会写入超过size-1个字符。如果size为0，则返回应该写入的字符数。
 */
int NEFORCE_API vsnprintf(char* buf, size_t size, const char* fmt, std::va_list args) noexcept;

/**
 * @brief 格式化到缓冲区
 * @param buf 输出缓冲区
 * @param fmt 格式字符串
 * @param ... 可变参数
 * @return 写入缓冲区的字符数，不包括终止空字符
 *
 * 不检查缓冲区大小。
 */
int NEFORCE_API sprintf(char* buf, const char* fmt, ...) noexcept;

/**
 * @brief 安全格式化到缓冲区
 * @param buf 输出缓冲区
 * @param size 缓冲区大小
 * @param fmt 格式字符串
 * @param ... 可变参数
 * @return 应该写入缓冲区的字符数，不包括终止空字符
 *
 * 确保不会写入超过size-1个字符。
 */
int NEFORCE_API snprintf(char* buf, size_t size, const char* fmt, ...) noexcept;

/**
 * @brief 计算格式化字符串所需缓冲区大小
 * @param fmt 格式字符串
 * @param ... 可变参数
 * @return 格式化结果所需的字符数，不包括终止空字符
 *
 * 计算将格式字符串和参数格式化后所需的缓冲区大小。
 */
int NEFORCE_API scprintf(const char* fmt, ...) noexcept;

/** @} */ // StringFormat

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_STRING_VSPRINTF_HPP__
