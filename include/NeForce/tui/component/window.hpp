#ifndef NEFORCE_TUI_COMPONENT_WINDOW_HPP__
#define NEFORCE_TUI_COMPONENT_WINDOW_HPP__

/**
 * @file window.hpp
 * @brief 浮动窗口组件
 *
 * 提供可拖拽和调整大小的浮动窗口，渲染为带标题边框的层叠元素。
 */

#include "NeForce/tui/component/component.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 窗口配置
 */
struct window_options {
    /**
     * @brief 渲染状态
     */
    struct render_state {
        element inner;       ///< 内部子组件的元素
        string title;        ///< 窗口标题
        bool active = false; ///< 是否激活
        bool drag = false;   ///< 是否正在拖动
        bool resize = false; ///< 是否正在调整大小
    };

    unique_ptr<component_base> inner;       ///< 内部子组件
    string title;                           ///< 窗口标题
    int* left = nullptr;                    ///< 左位置
    int* top = nullptr;                     ///< 上位置
    int* width = nullptr;                   ///< 宽度
    int* height = nullptr;                  ///< 高度
    function<element(render_state)> render; ///< 自定义渲染函数
};

/**
 * @brief 创建浮动窗口组件
 * @param options 窗口配置
 * @return 组件指针
 */
unique_ptr<component_base> NEFORCE_API window(window_options options);

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_COMPONENT_WINDOW_HPP__
