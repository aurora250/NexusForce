/**
 * @file width.hpp
 * @brief 终端显示宽度计算
 *
 * 按 Unicode 东亚宽度（UAX #11）与 Emoji_Presentation 属性计算字符串占用的终端列数，
 * 供表格对齐、文本换行与字符图排版使用。与渲染无关，可在无终端环境下调用。
 */

#ifndef NEFORCE_CORE_STRING_WIDTH_HPP__
#define NEFORCE_CORE_STRING_WIDTH_HPP__
#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup String 字符串
 * @brief 动态字符序列容器
 * @{
 */

/**
 * @brief 计算字符串的终端显示宽度
 * @param text UTF-8 字符串
 * @return 占用的终端列数（零宽字符计 0）
 */
NEFORCE_API int text_display_width(string_view text);

/**
 * @brief 按终端显示宽度截断字符串
 * @param text UTF-8 字符串
 * @param width 最大显示宽度
 * @return 截断后的字符串
 * @note 不会拆分多字节字符；零宽字符不占宽度
 */
NEFORCE_API string display_truncate(string_view text, int width);

/**
 * @brief 按终端显示宽度填充字符串
 * @param text UTF-8 字符串
 * @param width 目标显示宽度
 * @param right_align 是否右对齐
 * @return 填充或截断后的字符串
 * @note 结果保证显示宽度不超过 width
 */
NEFORCE_API string display_fit(string_view text, int width, bool right_align = false);

/** @} */ // String

NEFORCE_END_NAMESPACE__
#endif
