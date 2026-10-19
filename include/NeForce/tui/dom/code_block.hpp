#ifndef NEFORCE_TUI_DOM_CODE_BLOCK_HPP__
#define NEFORCE_TUI_DOM_CODE_BLOCK_HPP__

/**
 * @file code_block.hpp
 * @brief 代码块元素
 *
 * 带行号栏与背景的等宽代码块，可选关键字着色。
 */

#include "NeForce/tui/dom/element.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 代码块配置
 */
struct code_block_option {
    string code;             ///< 代码内容
    string language;         ///< 语言标识，用于选择着色规则
    int width = 0;           ///< 宽度约束（0 表示由布局决定）
    bool show_gutter = true; ///< 是否显示行号栏
    int start_line = 1;      ///< 起始行号
    bool highlight = true;   ///< 是否启用简单着色
    tui::style style;        ///< 基础样式
    tui::style gutter_style; ///< 行号栏样式
};

/**
 * @brief 创建代码块元素
 * @param opt 配置
 * @return 元素树（vbox）
 *
 * 着色规则只覆盖注释、字符串、数字与常见关键字，用于让 diff 与
 * 代码片段在终端里可读，不做完整语法分析。
 */
element NEFORCE_API code_block(code_block_option opt);

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_DOM_CODE_BLOCK_HPP__
