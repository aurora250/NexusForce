/**
 * @file width.hpp
 * @brief 终端显示宽度计算
 *
 * 按 Unicode 东亚宽度（UAX #11）与 Emoji_Presentation 属性计算字符串占用的终端列数，
 * 供表格对齐、文本换行与字符图排版使用。与渲染无关，可在无终端环境下调用。
 */

#ifndef NEFORCE_CORE_STRING_WIDTH_HPP__
#define NEFORCE_CORE_STRING_WIDTH_HPP__
#include "NeForce/core/string/string_view.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup Core Core
 * @{
 */

/**
 * @brief 计算字符串的终端显示宽度
 * @param text UTF-8 字符串
 * @return 占用的终端列数（零宽字符计 0）
 *
 * 结果按内容缓存；超过 512 字节的字符串直接计算，避免大对象占用缓存。
 */
NEFORCE_API int text_display_width(string_view text);

/** @} */
NEFORCE_END_NAMESPACE__

#endif
