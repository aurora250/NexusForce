#ifndef NEFORCE_TUI_DOM_GRAPH_HPP__
#define NEFORCE_TUI_DOM_GRAPH_HPP__

/**
 * @file graph.hpp
 * @brief 折线图元素
 *
 * 提供简单的折线图/柱状图渲染元素。
 */

#include "NeForce/tui/dom/element.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 折线图函数类型
 *
 * 输入 x 坐标（0 到 width-1），返回 y 值。
 * y 值将被归一化到 [0, height-1] 范围。
 */
using graph_function = function<vector<int>(int width, int height)>;

/**
 * @brief 创建折线图元素
 * @param data_points 数据点列表
 * @param width 图表宽度
 * @param height 图表高度
 * @return Graph 元素
 */
element NEFORCE_API graph(const vector<int>& data_points, int width = 40, int height = 10);

/**
 * @brief 创建函数式折线图
 * @param fn 图表函数
 * @param width 图表宽度
 * @param height 图表高度
 * @return Graph 元素
 */
element NEFORCE_API graph(graph_function fn, int width = 40, int height = 10);

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_DOM_GRAPH_HPP__
