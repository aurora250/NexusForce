#ifndef NEFORCE_TUI_DOM_MARKDOWN_HPP__
#define NEFORCE_TUI_DOM_MARKDOWN_HPP__

/**
 * @file markdown.hpp
 * @brief Markdown 渲染元素
 *
 * 把 Markdown 渲染为元素树：标题、段落、无序/有序列表、引用、代码块、分隔线与行内强调。
 */

#include "NeForce/core/memory/unique_ptr.hpp"
#include "NeForce/tui/dom/element.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief Markdown 渲染配置
 */
struct markdown_option {
    int width = 0;               ///< 宽度约束（0 表示由布局决定）
    bool inline_emphasis = true; ///< 是否解析 `**粗体**` / `*斜体*` / `` `代码` ``
    bool highlight_code = true;  ///< 代码块是否着色
    tui::style style;            ///< 正文样式
    tui::style heading_style;    ///< 标题附加样式
    tui::style code_style;       ///< 行内代码与代码块样式
    tui::style quote_style;      ///< 引用样式
    tui::style list_style;       ///< 列表标记样式
    tui::style link_style;       ///< 链接样式（默认下划线）
    tui::style math_style;       ///< 数学公式样式
    bool parse_math = true;      ///< 是否识别 `$…$` 与 `$$…$$` 公式
};

/**
 * @brief 解析行内强调片段
 * @param text 单行文本
 * @param base 基础样式
 * @param code_style 行内代码样式
 * @return 富文本片段列表
 *
 * 支持 `**bold**`、`*italic*`、`` `code` `` 与 `~~strike~~` 四种标记，未闭合的标记按普通文本处理
 */
NEFORCE_API vector<text_span> parse_inline_markdown(string_view text, const tui::style& base,
                                                    const tui::style& code_style);

/**
 * @brief 渲染 Markdown 文本
 * @param text Markdown 源文本
 * @param opt 配置
 * @return 元素树（vbox）
 */
NEFORCE_API element markdown(string_view text, markdown_option opt = {});

/**
 * @brief Markdown 渲染缓存
 *
 * 以「文本内容 + 宽度」为键缓存「解析 + 元素树构建」的结果。
 * 对话记录这类每帧都要重绘同一段 Markdown 的场景，命中缓存后不再重复解析，
 * 元素节点上的布局缓存也随之复用。
 */
class NEFORCE_API markdown_cache {
public:
    /**
     * @brief 构造函数
     * @param opt 渲染配置
     * @param capacity 最多缓存的条目数，超出后淘汰最久未使用的条目
     */
    explicit markdown_cache(markdown_option opt = {}, size_t capacity = 64);

    /**
     * @brief 析构函数
     */
    ~markdown_cache();

    markdown_cache(const markdown_cache&) = delete;
    markdown_cache& operator=(const markdown_cache&) = delete;
    markdown_cache(markdown_cache&&) = delete;
    markdown_cache& operator=(markdown_cache&&) = delete;

    /**
     * @brief 渲染指定宽度的 Markdown
     * @param text Markdown 源文本
     * @param width 渲染宽度，非正数表示沿用构造时的 `markdown_option::width`
     * @return 元素树常量引用
     */
    const element& render(string_view text, int width = 0);

    /**
     * @brief 清空缓存
     */
    void clear();

    /**
     * @brief 当前缓存条目数
     * @return 条目数
     */
    NEFORCE_NODISCARD size_t size() const noexcept;

    /**
     * @brief 命中次数
     * @return 命中次数
     */
    NEFORCE_NODISCARD size_t hit_count() const noexcept;

    /**
     * @brief 未命中次数
     * @return 未命中次数
     */
    NEFORCE_NODISCARD size_t miss_count() const noexcept;

    /**
     * @brief 获取渲染配置
     * @return 渲染配置常量引用
     */
    NEFORCE_NODISCARD const markdown_option& options() const noexcept;

private:
    struct entry;
    unique_ptr<entry[]> entries_;
    size_t capacity_;
    size_t count_ = 0;
    size_t hits_ = 0;
    size_t misses_ = 0;
    uint64_t stamp_ = 0;
    markdown_option opt_;
};

/**
 * @brief 流式 Markdown 渲染器
 *
 * 把"逐块追加"的 Markdown 增量渲染成稳定的元素树。
 *
 * 封口发生在围栏代码块之外的最后一个空行处，
 * 且尾部至少保留 1024 字节，
 * 保证未闭合的代码块、表格或列表不会被从中间切断。
 *
 * @code
 * markdown_stream stream(opt);
 * stream.append(chunk);            // 每次收到正文片段
 * view = stream.render(width);     // 每帧取一次
 * @endcode
 */
class NEFORCE_API markdown_stream {
public:
    /**
     * @brief 构造函数
     * @param opt 渲染配置
     */
    explicit markdown_stream(markdown_option opt = {});

    /**
     * @brief 析构函数
     */
    ~markdown_stream();

    markdown_stream(const markdown_stream&) = delete;
    markdown_stream& operator=(const markdown_stream&) = delete;
    markdown_stream(markdown_stream&&) = delete;
    markdown_stream& operator=(markdown_stream&&) = delete;

    /**
     * @brief 追加文本
     * @param chunk 追加的片段
     */
    void append(string_view chunk);

    /**
     * @brief 替换全文
     * @param text 新全文
     */
    void set_text(string_view text);

    /**
     * @brief 重置为空
     */
    void reset();

    /**
     * @brief 渲染当前内容
     * @param width 渲染宽度，非正数表示沿用构造时的宽度
     * @return 元素树；未封口部分每帧重新解析
     */
    const element& render(int width = 0);

    /**
     * @brief 当前全文
     * @return 文本视图
     */
    NEFORCE_NODISCARD string_view text() const noexcept;

    /**
     * @brief 已封口字节数
     * @return 字节数
     */
    NEFORCE_NODISCARD size_t sealed_bytes() const noexcept;

    /**
     * @brief 已封口块数
     * @return 块数
     */
    NEFORCE_NODISCARD size_t sealed_blocks() const noexcept;

private:
    struct impl;
    unique_ptr<impl> impl_;
};

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_DOM_MARKDOWN_HPP__
