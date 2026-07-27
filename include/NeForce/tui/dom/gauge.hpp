#ifndef NEFORCE_TUI_DOM_GAUGE_HPP__
#define NEFORCE_TUI_DOM_GAUGE_HPP__

/**
 * @file gauge.hpp
 * @brief 进度条元素
 *
 * 提供单向进度条元素。
 */

#include "NeForce/tui/dom/element.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 进度方向
 */
enum class gauge_direction : uint8_t {
    right, ///< 向右增长
    left,  ///< 向左增长
    up,    ///< 向上增长
    down,  ///< 向下增长
};

/**
 * @brief 创建水平进度条
 * @param progress 进度值 (0.0-1.0)
 * @param width 进度条宽度
 * @return Gauge 元素
 */
element NEFORCE_API gauge(float progress, int width = 20);

/**
 * @brief 创建方向化进度条
 * @param progress 进度值 (0.0-1.0)
 * @param direction 增长方向
 * @param size 长度
 * @return Gauge 元素
 */
element NEFORCE_API gauge_direction(float progress, enum class gauge_direction direction, int size = 20);

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_DOM_GAUGE_HPP__
