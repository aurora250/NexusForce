#ifndef NEFORCE_TUI_DOM_SPINNER_HPP__
#define NEFORCE_TUI_DOM_SPINNER_HPP__

/**
 * @file spinner.hpp
 * @brief 加载动画元素
 *
 * 提供多字符集的旋转加载动画元素。
 * 外部通过切换 image_index 驱动帧变更。
 */

#include "NeForce/tui/dom/element.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 创建加载旋转动画
 * @param charset_index 字符集索引（0-3）
 * @param image_index 当前帧索引
 * @return Spinner 元素
 *
 * 字符集：
 * - 0: │ ／ ─ ＼  (单线)
 * - 1: ⠁ ⠂ ⠄ ⡀ ⠈ ⠐ ⠠ ⢀ (braille)
 * - 2: ◷ ◶ ◵ ◴ (时钟)
 * - 3: ⣷ ⣯ ⣟ ⡿ ⢿ ⣻ ⣽ ⣾ (dots)
 */
element NEFORCE_API spinner(int charset_index, size_t image_index);

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_DOM_SPINNER_HPP__
