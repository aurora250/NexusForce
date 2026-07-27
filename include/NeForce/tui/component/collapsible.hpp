#ifndef NEFORCE_TUI_COMPONENT_COLLAPSIBLE_HPP__
#define NEFORCE_TUI_COMPONENT_COLLAPSIBLE_HPP__

/**
 * @file collapsible.hpp
 * @brief 折叠面板组件
 *
 * 提供可折叠/展开的内容区域，点击标题切换可见性。
 */

#include "NeForce/tui/component/component.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 创建可折叠区域
 * @param title 折叠标题
 * @param child 内部子组件
 * @param show 展开状态指针
 * @return 组件指针
 */
unique_ptr<component_base> NEFORCE_API collapsible(const string& title, unique_ptr<component_base> child,
                                                   bool* show = nullptr);

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_COMPONENT_COLLAPSIBLE_HPP__
