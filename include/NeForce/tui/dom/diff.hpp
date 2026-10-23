#ifndef NEFORCE_TUI_DOM_DIFF_HPP__
#define NEFORCE_TUI_DOM_DIFF_HPP__

/**
 * @file diff.hpp
 * @brief 差异视图元素
 *
 * 解析 unified diff 文本，按增删行着色并带行号栏。
 */

#include "NeForce/tui/dom/element.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 差异行类型
 */
enum class diff_kind : uint8_t {
    context, ///< 上下文行
    added,   ///< 新增行
    removed, ///< 删除行
    hunk,    ///< 区块头（@@ -a,b +c,d @@）
    meta,    ///< 文件头等元信息
};

/**
 * @brief 结构化差异行
 */
struct diff_line {
    diff_kind kind = diff_kind::context; ///< 行类型
    string text;                         ///< 行内容（不含前导标记）
    int old_number = 0;                  ///< 旧文件行号，0 表示无
    int new_number = 0;                  ///< 新文件行号，0 表示无
};

/**
 * @brief 单个文件的差异
 */
struct diff_file {
    string header;           ///< 文件头原文（`diff --git` / `---` / `+++`）
    string path;             ///< 新文件路径
    vector<diff_line> lines; ///< 该文件的差异行（含 hunk 头）
    int added = 0;           ///< 新增行数
    int removed = 0;         ///< 删除行数
};

/**
 * @brief 差异视图配置
 */
struct diff_view_option {
    int width = 0;                  ///< 宽度约束（0 表示由布局决定）
    bool show_gutter = true;        ///< 是否显示行号栏
    bool show_markers = true;       ///< 是否显示 +/- 标记列
    bool group_by_file = true;      ///< 是否按文件分组并显示文件头
    size_t collapse_hunk_after = 0; ///< 单个 hunk 超过该行数时折叠中间部分，0 表示不折叠
    bool background = true;         ///< 是否绘制整块背景带
    tui::style style;               ///< 基础样式
    tui::style gutter_style;        ///< 行号栏样式
    tui::style added_style;         ///< 新增行样式
    tui::style removed_style;       ///< 删除行样式
    tui::style header_style;        ///< 文件头样式
    tui::style band_style;          ///< 背景带样式
    bool side_by_side = false;      ///< 是否并排显示旧行与新行
    int split_min_width = 110;      ///< 并排显示所需的最小宽度，窄于此值回退到统一视图
    bool word_level = false;        ///< 是否启用词级差异高亮
    tui::style word_added_style;    ///< 词级新增高亮样式，未设置时使用新增样式加粗
    tui::style word_removed_style;  ///< 词级删除高亮样式，未设置时使用删除样式加粗
};

/**
 * @brief 解析 unified diff 文本
 * @param patch 差异文本
 * @return 结构化行列表
 *
 * 识别 `diff --git`、`---`、`+++`、`@@`、`+`、`-`、` ` 前缀，
 * `\ No newline at end of file` 归为元信息行。
 */
NEFORCE_API vector<diff_line> parse_unified_diff(string_view patch);

/**
 * @brief 按文件解析 unified diff 文本
 * @param patch 差异文本
 * @return 文件差异列表
 */
NEFORCE_API vector<diff_file> parse_unified_diff_files(string_view patch);

/**
 * @brief 渲染结构化差异
 * @param lines 差异行
 * @param opt 配置
 * @return 元素树（vbox）
 */
NEFORCE_API element diff_view(const vector<diff_line>& lines, diff_view_option opt = {});

/**
 * @brief 解析并渲染 unified diff
 * @param patch 差异文本
 * @param opt 配置
 * @return 元素树（vbox）
 */
NEFORCE_API element diff_view(string_view patch, diff_view_option opt = {});

/**
 * @brief 渲染按文件分组的差异
 * @param files 文件差异列表
 * @param opt 配置
 * @return 元素树（vbox）
 */
NEFORCE_API element diff_view(const vector<diff_file>& files, diff_view_option opt = {});

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_DOM_DIFF_HPP__
