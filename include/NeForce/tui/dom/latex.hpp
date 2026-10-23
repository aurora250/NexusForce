#ifndef NEFORCE_TUI_DOM_LATEX_HPP__
#define NEFORCE_TUI_DOM_LATEX_HPP__

/**
 * @file latex.hpp
 * @brief LaTeX 公式排版
 *
 * 把 TeX 子集排版为终端友好的 Unicode 文本：
 * 符号、上下标、分数、根式、重音、运算符上下限、矩阵、cases 与 aligned 环境。
 * 解析失败时不会产生半成品，调用方可以回退到公式源码。
 */

#include "NeForce/core/string/string.hpp"
#include "NeForce/core/utility/optional.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief LaTeX 公式排版结果
 */
struct latex_layout {
    vector<string> lines; ///< 已对齐的多行文本，行尾无空白
};

/**
 * @brief 把 TeX 子集排成多行 Unicode 文本
 * @param tex TeX 源文本
 * @param display 是否为行间公式
 * @return 成功返回排版结果；不支持的命令、括号不配对或环境不认识时返回 none
 *
 * display 为 true 时允许分数与运算符上下限竖排；为 false 时只产出单行
 */
NEFORCE_API optional<latex_layout> render_latex(string_view tex, bool display = false);

/** @} */

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif
