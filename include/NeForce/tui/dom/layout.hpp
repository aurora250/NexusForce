#ifndef NEFORCE_TUI_LAYOUT_HPP__
#define NEFORCE_TUI_LAYOUT_HPP__

/**
 * @file layout.hpp
 * @brief Flexbox布局引擎
 *
 * 纯函数式 Flexbox 布局计算。
 * 输入元素树和终端尺寸，输出每个叶子节点的布局矩形。
 */

#include "NeForce/tui/dom/element.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 布局矩形
 */
struct layout_rect {
    int x = 0; ///< 列坐标
    int y = 0; ///< 行坐标
    int w = 0; ///< 宽度
    int h = 0; ///< 高度
};

/**
 * @brief Flexbox 布局计算
 * @param element 元素树根节点
 * @param constraint_w 可用宽度
 * @param constraint_h 可用高度
 * @returns 每个叶子节点的布局矩形
 */
vector<layout_rect> NEFORCE_API compute_layout(const element& element, int constraint_w, int constraint_h);

/** @} */ // TuiLayout

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_LAYOUT_HPP__
