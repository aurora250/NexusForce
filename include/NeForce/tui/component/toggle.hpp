#ifndef NEFORCE_TUI_COMPONENT_TOGGLE_HPP__
#define NEFORCE_TUI_COMPONENT_TOGGLE_HPP__

/**
 * @file toggle.hpp
 * @brief 切换组件
 *
 * 提供 Toggle 组件，在多个选项间循环切换，类似移动端开关。
 */

#include "NeForce/tui/component/component.hpp"
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
 * @brief 切换组件配置
 */
struct toggle_option {
    vector<string> entries;     ///< 选项文本列表
    int* selected = nullptr;    ///< 当前选中索引
    function<void()> on_change; ///< 选择变更回调
    bool horizontal = false;    ///< 是否水平排列
};

/**
 * @brief 创建切换组件
 * @param option 配置
 * @return 组件指针
 */
unique_ptr<component_base> NEFORCE_API toggle(toggle_option option);

/** @} */ // Components

/** @} */ // TUI

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_COMPONENT_TOGGLE_HPP__
