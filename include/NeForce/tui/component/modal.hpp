#ifndef NEFORCE_TUI_MODAL_HPP__
#define NEFORCE_TUI_MODAL_HPP__

/**
 * @file modal.hpp
 * @brief 模态框组件
 *
 * 提供 modal 组件，在主要内容上叠加模态窗口。
 */

#include "NeForce/tui/component/component.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 创建模态框组件
 * @param main 主内容组件
 * @param overlay 模态叠加组件
 * @param show_modal 是否显示模态框
 * @return 组件指针
 *
 * 当 show_modal 指向 true 时，overlay 覆盖在 main 上，
 * 且所有事件发给 overlay。按 Escape 不会自动关闭。
 */
unique_ptr<component_base> NEFORCE_API modal(unique_ptr<component_base> main, unique_ptr<component_base> overlay,
                                             const bool* show_modal);

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_MODAL_HPP__
