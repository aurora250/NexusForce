#ifndef NEXUSFORCE_SOURCE_LOC_HPP
#define NEXUSFORCE_SOURCE_LOC_HPP

/**
 * @file source_loc.hpp
 * @brief 源码位置信息工具
 *
 * 此文件提供源码位置信息获取与使用工具。
 */

#include "NeForce/core/string/string_view.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup DebugAndAssertions 调试与断言
 * @{
 */

/**
 * @struct source_loc
 * @brief 源码位置信息
 */
struct source_loc {
    string_view file; ///< 源文件名
    string_view func; ///< 函数名
    int line;         ///< 行号
};

/** @brief 构造 source_loc 的便捷宏 */
#define NEFORCE_SOURCE_LOC() \
    _NEFORCE source_loc { __FILE__, __func__, __LINE__ }

/** @} */ // DebugAndAssertions

NEFORCE_END_NAMESPACE__
#endif //NEXUSFORCE_SOURCE_LOC_HPP
