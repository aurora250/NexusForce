#ifndef NEFORCE_TUI_COMPONENT_SCROLL_VIEW_HPP__
#define NEFORCE_TUI_COMPONENT_SCROLL_VIEW_HPP__

/**
 * @file scroll_view.hpp
 * @brief 滚动视图组件
 *
 * 为 ScrollView 元素提供交互式滚动条支持。
 * 支持鼠标拖拽滚动条滑块和滚轮滚动。
 */

#include "NeForce/tui/component/component.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 滚动视图配置
 */
struct scroll_view_option {
    function<element()> content; ///< 内容渲染回调
    style style;                 ///< 样式（边框、padding 等）
};

/**
 * @brief 创建滚动视图组件
 * @param opt 配置选项
 * @return 组件指针
 */
unique_ptr<component_base> NEFORCE_API scroll_view(scroll_view_option opt);

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_COMPONENT_SCROLL_VIEW_HPP__
