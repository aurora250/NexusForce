#ifndef NEFORCE_TUI_COMPONENT_TEXT_INPUT_HPP__
#define NEFORCE_TUI_COMPONENT_TEXT_INPUT_HPP__

/**
 * @file text_input.hpp
 * @brief 文本输入组件
 *
 * 管理输入焦点和光标闪烁的文本输入组件。
 */

#include "NeForce/tui/component/component.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 文本输入组件配置
 */
struct text_input_option {
    state<string>* text = nullptr;                  ///< 文本状态
    string placeholder;                             ///< 占位提示
    function<void()> on_enter;                      ///< 回车回调
    style style;                                    ///< 样式
    style::wrap_mode wrap = style::wrap_mode::word; ///< 文本换行模式
};

/**
 * @brief 创建文本输入组件
 * @param opt 配置选项
 * @return 组件指针
 *
 * 组件获取焦点后接收可打印字符和 Backspace，
 * 通过 onAnimation 实现光标闪烁。
 */
unique_ptr<component_base> NEFORCE_API text_input(text_input_option opt);

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_COMPONENT_TEXT_INPUT_HPP__
