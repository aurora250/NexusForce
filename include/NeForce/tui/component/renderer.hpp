#ifndef NEFORCE_TUI_COMPONENT_RENDERER_HPP__
#define NEFORCE_TUI_COMPONENT_RENDERER_HPP__

/**
 * @file renderer.hpp
 * @brief 渲染辅助组件
 *
 * 提供 renderer（自定义渲染）、catch_event（事件拦截）和
 * maybe（条件显示）三个声明式组件包装器。
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
 * @brief 使用渲染函数覆盖子组件的 render()
 * @param child 子组件
 * @param render_fn 渲染函数
 * @return 组件指针
 */
unique_ptr<component_base> NEFORCE_API renderer(unique_ptr<component_base> child, function<element()> render_fn);

/**
 * @brief 带焦点感知的渲染器
 * @param child 子组件
 * @param render_fn 渲染函数（参数 focused 指示组件是否拥有焦点）
 * @return 组件指针
 */
unique_ptr<component_base> NEFORCE_API renderer(unique_ptr<component_base> child,
                                                function<element(bool focused)> render_fn);

/**
 * @brief 独立渲染器
 * @param render_fn 渲染函数
 * @return 组件指针
 */
unique_ptr<component_base> NEFORCE_API renderer(function<element()> render_fn);

/**
 * @brief 在事件传递到子组件前拦截处理
 * @param child 子组件
 * @param on_event 事件处理函数，返回 true 表示已消费
 * @return 组件指针
 */
unique_ptr<component_base> NEFORCE_API catch_event(unique_ptr<component_base> child,
                                                   function<bool(const key_event&)> on_event);

/**
 * @brief 条件渲染组件
 * @param child 子组件
 * @param show 条件指针
 * @return 组件指针
 *
 * 当 show 指向 false 时，组件 render() 返回 empty_element，且不会接收焦点或事件。
 */
unique_ptr<component_base> NEFORCE_API maybe(unique_ptr<component_base> child, const bool* show);

/** @} */ // Components

/** @} */ // TUI

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_COMPONENT_RENDERER_HPP__
