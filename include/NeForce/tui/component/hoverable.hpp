#ifndef NEFORCE_TUI_COMPONENT_HOVERABLE_HPP__
#define NEFORCE_TUI_COMPONENT_HOVERABLE_HPP__

/**
 * @file hoverable.hpp
 * @brief 鼠标悬停检测组件
 *
 * 提供 hoverable 包装组件，当鼠标进入/离开/移动时触发回调。
 */

#include "NeForce/tui/component/component.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 创建鼠标悬停检测组件
 * @param child 子组件
 * @param on_enter 鼠标进入回调
 * @param on_leave 鼠标离开回调
 * @return 组件指针
 */
unique_ptr<component_base> NEFORCE_API hoverable(unique_ptr<component_base> child, function<void()> on_enter,
                                                 function<void()> on_leave);

/**
 * @brief 创建悬停状态绑定组件
 * @param child 子组件
 * @param hovered 悬停状态指针
 * @return 组件指针
 */
unique_ptr<component_base> NEFORCE_API hoverable(unique_ptr<component_base> child, bool* hovered);

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_COMPONENT_HOVERABLE_HPP__
