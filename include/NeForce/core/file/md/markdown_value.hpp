#ifndef NEFORCE_CORE_FILE_MD_MARKDOWN_VALUE_HPP__
#define NEFORCE_CORE_FILE_MD_MARKDOWN_VALUE_HPP__

/**
 * @file markdown_value.hpp
 * @brief Markdown 文档模型
 *
 * 定义与呈现无关的 Markdown 抽象语法树：块级节点、行内节点、链接引用定义表与文档根。
 * 文档模型不携带任何样式、颜色或终端概念，渲染方自行映射。
 *
 * 支持的块级结构：ATX 与 Setext 标题、段落（含懒惰续行与软/硬换行）、
 * 围栏与缩进代码块、HTML 块（7 类）、引用块（可嵌套）、有序/无序/嵌套
 * 列表（紧凑与松散、起始编号、任务项，列表项内可含任意子块）、分隔线、
 * 管道表格、链接引用定义。
 *
 * 支持的行内结构：文本与实体引用、强调与加粗（含 `***` 三重限定符）、
 * 删除线、行内代码、行内链接/图片（内联式、完整引用式、折叠式、简写式）、
 * 自动链接、行内 HTML、硬换行。
 */

#include "NeForce/core/exception/exception.hpp"
#include "NeForce/core/interface/istringify.hpp"
#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Exceptions 异常类集
 * @brief 异常类集
 * @{
 */

/**
 * @struct md_exception
 * @brief Markdown 格式操作失败
 */
struct md_exception final : value_exception {
    explicit md_exception(const char* info = "Markdown Operation Failed.") noexcept :
    value_exception(info) {}

    explicit md_exception(const exception& e) :
    value_exception(e) {}

    ~md_exception() override = default;

    NEFORCE_NODISCARD const char* type() const noexcept override { return "md_exception"; }
};

/** @} */ // Exceptions

/**
 * @defgroup Markdown Markdown 文档
 * @brief Markdown 文档的解析、文档模型与构建
 *
 * @section standards 参考的规范
 * - **CommonMark 0.31.2**：Markdown 规范
 *   https://spec.commonmark.org/0.31.2/
 * - **GitHub Flavored Markdown Spec**：表格、任务列表与删除线扩展
 *   https://github.github.com/gfm/
 *
 * @note 覆盖规范的完整**结构**，HTML 以原文保留，实体引用只解码常用子集，制表符按 4 列制表位展开。
 * @{
 */

/**
 * @brief 行内节点类型
 */
enum class md_inline_kind : uint8_t {
    text,          ///< 纯文本（实体引用已解码）
    strong,        ///< 加粗（`**x**` / `__x__`）
    emphasis,      ///< 斜体（`*x*` / `_x_`）
    strikethrough, ///< 删除线（`~~x~~`）
    code,          ///< 行内代码，内容在 text 中，不解析子节点
    link,          ///< 链接，子节点为链接文字
    image,         ///< 图片，text 为替代文字
    autolink,      ///< 自动链接（`<https://x>` / `<a@b.c>`），text 为显示文字
    html,          ///< 行内 HTML，原文在 text 中
    line_break,    ///< 硬换行
};

/**
 * @brief 行内节点
 */
struct md_inline {
    md_inline_kind kind = md_inline_kind::text; ///< 节点类型
    string text;                                ///< 文本内容
    string destination;                         ///< 链接/图片/自动链接地址
    string title;                               ///< 链接/图片标题
    string reference;                           ///< 引用式链接的标签（空表示内联）
    vector<md_inline> children;                 ///< 子节点
};

/**
 * @brief 行内节点列表
 */
using md_inlines = vector<md_inline>;

/**
 * @brief 表格列对齐
 */
enum class md_align : uint8_t {
    none,   ///< 未指定
    left,   ///< 左对齐
    center, ///< 居中
    right,  ///< 右对齐
};

/**
 * @brief 块级节点类型
 */
enum class md_block_kind : uint8_t {
    paragraph,      ///< 段落
    heading,        ///< ATX 或 Setext 标题
    code_block,     ///< 代码块
    html_block,     ///< HTML 块
    block_quote,    ///< 引用块，内容在 children
    list,           ///< 列表，子项为 list_item 块
    list_item,      ///< 列表项，内容在 children
    thematic_break, ///< 分隔线
    table,          ///< 管道表格
};

/**
 * @brief 块级节点
 *
 * 一个结构承载全部块类型：按 `kind` 取用对应字段。
 */
struct md_block {
    md_block_kind kind = md_block_kind::paragraph; ///< 节点类型

    int level = 0;       ///< heading 级别（1-6）
    bool setext = false; ///< heading 是否由 Setext 形式书写

    bool fenced = false;   ///< code_block 是否为围栏块
    char fence_char = '`'; ///< code_block 围栏字符
    int fence_length = 0;  ///< code_block 围栏长度
    string info;           ///< code_block 围栏后的原始信息串
    string language;       ///< code_block 信息串首词
    string literal;        ///< code_block/html_block 原始内容

    int html_type = 0; ///< html_block 类型（1-7）

    bool ordered = false; ///< list/list_item 是否有序
    int start = 1;        ///< list 起始编号
    char delimiter = '.'; ///< list 有序分隔符（`.` 或 `)`）
    char bullet = '-';    ///< list 无序标记字符
    bool tight = true;    ///< list 是否紧凑
    int number = 1;       ///< list_item 的编号
    bool task = false;    ///< list_item 是否为任务项
    bool checked = false; ///< 任务项是否已勾选

    vector<md_align> alignments;     ///< table 列对齐
    vector<md_inlines> header;       ///< table 表头单元格
    vector<vector<md_inlines>> rows; ///< table 数据行

    md_inlines inlines;        ///< paragraph/heading 的行内内容
    vector<md_block> children; ///< block_quote/list/list_item 的子块
};

/**
 * @brief 链接引用定义
 */
struct md_reference {
    string label;       ///< 引用标签
    string destination; ///< 目标地址
    string title;       ///< 标题
};

/**
 * @brief Markdown 文档
 */
class NEFORCE_API markdown_document : public istringify<markdown_document> {
public:
    vector<md_block> blocks;         ///< 顶层块序列
    vector<md_reference> references; ///< 链接引用定义

    /**
     * @brief 序列化为 Markdown 文本
     * @return Markdown 文本
     *
     * 等价于 `markdown_builder::dump(*this)`。
     */
    NEFORCE_NODISCARD string to_string() const;
};

/**
 * @brief 提取行内节点的纯文本
 * @param inlines 行内节点
 * @return 去掉标记后的纯文本，硬换行输出为 `\\n`
 */
NEFORCE_API string md_plain_text(const md_inlines& inlines);

/**
 * @brief 提取块级节点的纯文本
 * @param block 块级节点
 * @return 纯文本
 */
NEFORCE_API string md_plain_text(const md_block& block);

/**
 * @brief 判断文档是否为空
 * @param document 文档
 * @return 是否既没有块也没有引用定义
 */
NEFORCE_NODISCARD NEFORCE_API bool md_empty(const markdown_document& document) noexcept;

/**
 * @brief 按标签查找链接引用定义
 * @param document 文档
 * @param label 标签
 * @return 命中定义指针，未命中返回 nullptr
 *
 * 标签比较忽略大小写与多余空白，与解析期的解析规则一致。
 */
NEFORCE_NODISCARD NEFORCE_API const md_reference* md_find_reference(const markdown_document& document,
                                                                    string_view label);

/** @} */ // Markdown

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_MD_MARKDOWN_VALUE_HPP__
