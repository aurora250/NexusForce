#ifndef NEFORCE_TUI_SLIDER_HPP__
#define NEFORCE_TUI_SLIDER_HPP__

/**
 * @file slider.hpp
 * @brief 滑块组件
 *
 * 提供 slider 滑块组件，支持选择和拖动。
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
 * @brief 窗口配置
 */
struct slider_options {
    string label;
    int* value = nullptr;
    int min_value = 0;
    int max_value = 0;
    int increment = 1;
};

/**
 * @brief 滑块组件
 *
 * 使用箭头键调整值，范围 [min, max]，步长为 increment。
 */
unique_ptr<component_base> NEFORCE_API slider(slider_options options);

/** @} */ // Components

/** @} */ // TUI

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_SLIDER_HPP__
