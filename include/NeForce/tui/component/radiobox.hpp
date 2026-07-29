#ifndef NEFORCE_TUI_RADIOBOX_HPP__
#define NEFORCE_TUI_RADIOBOX_HPP__

/**
 * @file radiobox.hpp
 * @brief 单选按钮组组件
 *
 * 提供 Radiobox 组件，多个互斥的单选条目。
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
 * @brief 单选按钮组配置
 */
struct radiobox_option {
    vector<string> entries;     ///< 选项文本列表
    int* selected = nullptr;    ///< 当前选中索引
    function<void()> on_change; ///< 选择变更回调
};

/**
 * @brief 创建单选按钮组
 * @param option 配置
 * @return 组件指针
 */
unique_ptr<component_base> NEFORCE_API radiobox(radiobox_option option);

/** @} */ // Components

/** @} */ // TUI

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_RADIOBOX_HPP__
