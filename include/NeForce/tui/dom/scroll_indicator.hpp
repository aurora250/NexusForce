#ifndef NEFORCE_TUI_DOM_SCROLL_INDICATOR_HPP__
#define NEFORCE_TUI_DOM_SCROLL_INDICATOR_HPP__

/**
 * @file scroll_indicator.hpp
 * @brief 滚动指示器元素
 *
 * 提供垂直和水平滚动条指示元素。
 */

#include "NeForce/tui/dom/element.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 创建垂直滚动指示器
 * @param total 总行数
 * @param visible 可见行数
 * @param offset 当前滚动偏移行数
 * @param height 指示器高度
 * @return 垂直滚动条元素
 */
element NEFORCE_API vscroll_indicator(int total, int visible, int offset, int height);

/**
 * @brief 创建水平滚动指示器
 * @param total 总列数
 * @param visible 可见列数
 * @param offset 当前滚动偏移列数
 * @param width 指示器宽度
 * @return 水平滚动条元素
 */
element NEFORCE_API hscroll_indicator(int total, int visible, int offset, int width);

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_DOM_SCROLL_INDICATOR_HPP__
