#ifndef NEFORCE_TUI_COMPONENT_MENU_HPP__
#define NEFORCE_TUI_COMPONENT_MENU_HPP__

/**
 * @file menu.hpp
 * @brief 菜单组件
 *
 * 提供菜单组件，支持垂直/水平布局和条目选择。
 */

#include "NeForce/tui/component/component.hpp"
#include "NeForce/tui/dom/ref.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__
NEFORCE_BEGIN_COMPONENTS__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @addtogroup Components 用户组件
 * @{
 */

/**
 * @brief 菜单选项配置
 */
struct menu_option {
    vector<string> entries;     ///< 菜单条目文本列表
    int* selected = nullptr;    ///< 选中索引
    bool horizontal = false;    ///< 是否水平排列
    function<void()> on_change; ///< 选择变更回调
    function<void()> on_enter;  ///< 回车确认回调
    style normal_style;         ///< 未选中条目的样式
    style focused_style;        ///< 已选中条目的样式
};

/**
 * @brief 创建菜单组件
 * @param option 菜单配置
 * @return 组件指针
 */
unique_ptr<component_base> NEFORCE_API menu(menu_option option);

/** @} */ // Components

/** @} */ // TUI

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_COMPONENT_MENU_HPP__
