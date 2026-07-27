#ifndef NEFORCE_TUI_DROPDOWN_HPP__
#define NEFORCE_TUI_DROPDOWN_HPP__

/**
 * @file dropdown.hpp
 * @brief 下拉选择组件
 *
 * 提供 dropdown 组件，点击展开选项列表，选择一个后收起。
 */

#include "NeForce/tui/component/component.hpp"
#include "NeForce/tui/dom/ref.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 下拉选择组件配置
 */
struct dropdown_option {
    vector<string> entries;     ///< 选项列表
    int* selected = nullptr;    ///< 当前选中索引
    bool* open = nullptr;       ///< 展开状态
    function<void()> on_change; ///< 选择变更回调
};

/**
 * @brief 创建下拉选择组件
 * @param option 配置
 * @return 组件指针
 */
unique_ptr<component_base> NEFORCE_API dropdown(dropdown_option option);

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_DROPDOWN_HPP__
