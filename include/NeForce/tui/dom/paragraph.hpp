#ifndef NEFORCE_TUI_DOM_PARAGRAPH_HPP__
#define NEFORCE_TUI_DOM_PARAGRAPH_HPP__

/**
 * @file paragraph.hpp
 * @brief 文本段落元素
 *
 * 提供自动换行的文本段落，支持左对齐、居中、右对齐和两端对齐。
 */

#include "NeForce/tui/dom/element.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 创建居左对齐段落
 * @param content 文本内容
 * @param max_width 最大宽度（0 表示自动）
 * @return 段落元素
 */
element NEFORCE_API paragraph(string content, int max_width = 0);

/**
 * @brief 创建居中对齐段落
 * @param content 文本内容
 * @param max_width 最大宽度（0 表示自动）
 * @return 段落元素
 */
element NEFORCE_API paragraph_align_center(string content, int max_width = 0);

/**
 * @brief 创建居右对齐段落
 * @param content 文本内容
 * @param max_width 最大宽度（0 表示自动）
 * @return 段落元素
 */
element NEFORCE_API paragraph_align_right(string content, int max_width = 0);

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_DOM_PARAGRAPH_HPP__
