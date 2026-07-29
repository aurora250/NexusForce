#ifndef NEXUSFORCE_CORE_EXCEPTION_SOURCE_LOCATION_HPP__
#define NEXUSFORCE_CORE_EXCEPTION_SOURCE_LOCATION_HPP__

/**
 * @file source_location.hpp
 * @brief 源码位置信息工具
 *
 * 此文件提供源码位置信息获取与使用工具。
 */

#include "NeForce/core/string/string_view.hpp"
#include <source_location>
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup DebugAndAssertions 调试与断言
 * @{
 */

/**
 * @struct source_location
 * @brief 源码位置信息
 */
struct source_location {
    string_view file;   ///< 源文件名
    string_view func;   ///< 函数名
    uint32_t line{0};   ///< 行号
    uint32_t column{0}; ///< 列号

    /**
     * @brief 当前源码位置
     */
    static NEFORCE_CONSTEVAL20 source_location current() noexcept {
#ifdef NEFORCE_COMPILER_GCC
        return source_location{__builtin_FILE(), __builtin_FUNCSIG(), __builtin_LINE(), 0};
#else
        return source_location{__builtin_FILE(), __builtin_FUNCSIG(), __builtin_LINE(), __builtin_COLUMN()};
#endif
    }
};

/** @brief 构造 source_loc 的便捷宏 */
#define NEFORCE_SOURCE_LOCATION() _NEFORCE source_location{__FILE__, __func__, __LINE__, 0}

/** @} */ // DebugAndAssertions

NEFORCE_END_NAMESPACE__
#endif // NEXUSFORCE_CORE_EXCEPTION_SOURCE_LOCATION_HPP__
