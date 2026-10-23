#ifndef NEFORCE_TUI_DOM_TEXT_LAYOUT_HPP__
#define NEFORCE_TUI_DOM_TEXT_LAYOUT_HPP__

/**
 * @file text_layout.hpp
 * @brief 文本排版计算
 *
 * 纯函数式的文本分行：把一段文本按显示宽度与换行模式切成若干可视行。
 * 渲染器、布局测量与编辑器光标定位共用同一份实现，保证三者一致。
 */

#include "NeForce/core/string/string_view.hpp"
#include "NeForce/core/string/width.hpp"
#include "NeForce/tui/dom/style.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 可视行
 */
struct text_line {
    size_t begin = 0;          ///< 行起始字节偏移
    size_t end = 0;            ///< 行结束字节偏移（不含）；行尾空白不计入
    bool continuation = false; ///< 是否由自动换行产生
};

/**
 * @brief 文本分行结果
 */
struct text_layout {
    vector<text_line> lines; ///< 可视行列表，至少一行
};

/**
 * @brief 计算字符串的显示宽度
 * @param text 文本
 * @return 显示宽度，控制字符按一列计
 */
using neforce::text_display_width;

/**
 * @brief 计算文本排版
 * @param text 文本内容
 * @param width 行宽
 * @param wm 换行模式
 * @return 分行结果
 *
 * 换行规则：
 * - `\\n` 始终产生硬换行
 * - `word` 模式在行宽内最后一个空格处断行，并丢弃该空格及其后的连续空格
 * - `character` 模式在任意字符边界断行
 * - `none` 模式只在硬换行处断行
 * - 单个词宽于行宽时退化为按字符断行
 */
NEFORCE_API text_layout layout_text(string_view text, int width, style::wrap_mode wm);

/**
 * @brief 计算带悬挂缩进的文本排版
 * @param text 文本内容
 * @param width 首行宽度
 * @param wm 换行模式
 * @param hang_cols 续行缩进列数
 * @return 分行结果
 *
 * 硬换行后的行恢复整行宽度；只有自动换行产生的续行才缩进。
 */
NEFORCE_API text_layout layout_text_hang(string_view text, int width, style::wrap_mode wm, int hang_cols);

/**
 * @brief 计算字节偏移所在的行与列
 * @param layout 排版结果
 * @param text 原始文本
 * @param offset 字节偏移
 * @param row 输出行号（0 起始）
 * @param col 输出列号（0 起始）
 */
NEFORCE_API void caret_position(const text_layout& layout, string_view text, size_t offset, int& row, int& col);

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_DOM_TEXT_LAYOUT_HPP__
