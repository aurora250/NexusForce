#ifndef NEFORCE_TUI_LAYOUT_TYPES_HPP__
#define NEFORCE_TUI_LAYOUT_TYPES_HPP__

/**
 * @file layout_types.hpp
 * @brief 布局基础类型
 *
 * 定义布局计算中使用的基础数据结构，供 element 和 layout 模块共享。
 */

#include "NeForce/core/config/c++config.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 布局矩形
 *
 * 描述一个叶子元素在终端上的位置和大小。
 */
struct layout_rect {
    int x = 0; /**< 列坐标 */
    int y = 0; /**< 行坐标 */
    int w = 0; /**< 宽度 */
    int h = 0; /**< 高度 */
};

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_LAYOUT_TYPES_HPP__
