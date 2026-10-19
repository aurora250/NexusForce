#ifndef NEFORCE_CORE_FILE_MD_MARKDOWN_PARSER_HPP__
#define NEFORCE_CORE_FILE_MD_MARKDOWN_PARSER_HPP__

/**
 * @file markdown_parser.hpp
 * @brief Markdown 解析器
 *
 * 把 Markdown 文本解析为与呈现无关的文档模型。
 */

#include "NeForce/core/file/md/markdown_value.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup Markdown Markdown 文档
 * @{
 */

/**
 * @class markdown_parser
 * @brief Markdown 解析器
 *
 * 采用"逐行块级解析 + 行内递归下降"的算法：块级阶段按容器递归切分
 * 引用、列表与列表项，行内阶段在段落、标题与表格单元格内解析行内结构。
 * 链接引用定义先整体预扫描一次，因此支持前向引用。
 *
 * 支持的块级结构：
 * - ATX 与 Setext 标题
 * - 段落（懒惰续行、软换行与硬换行）
 * - 围栏代码块（反引号/波浪线，闭合长度需不小于起始）与缩进代码块
 * - HTML 块（脚本/注释/处理指令/声明/CDATA/块级标签/自定义标签共 7 类）
 * - 引用块（可嵌套，可含任意子块）
 * - 有序/无序/嵌套列表（起始编号、`.` 与 `)` 分隔符、紧凑与松散、任务项）
 * - 分隔线、管道表格（对齐行）、链接引用定义
 *
 * 支持的行内结构：
 * - 实体引用、行内代码、强调与加粗（含 `***`）、删除线
 * - 行内链接与图片（内联式、完整引用式、折叠式、简写式）
 * - 自动链接、行内 HTML、反斜杠转义、硬换行
 *
 * @note 任何无法识别的结构都会退化为段落文本
 */
class NEFORCE_API markdown_parser {
public:
    /**
     * @brief 解析选项
     */
    struct option {
        bool tables = true;          ///< 是否解析管道表格
        bool task_lists = true;      ///< 是否解析任务列表
        bool strikethrough = true;   ///< 是否解析删除线
        bool indented_code = true;   ///< 是否解析四空格缩进代码块
        bool html_blocks = true;     ///< 是否解析 HTML 块
        bool inline_html = true;     ///< 是否解析行内 HTML
        bool autolinks = true;       ///< 是否解析自动链接
        bool entities = true;        ///< 是否解码实体引用
        bool setext_headings = true; ///< 是否解析 Setext 标题
    };

private:
    string text_; ///< 待解析文本
    option opt_;  ///< 解析选项

public:
    /**
     * @brief 构造函数
     * @param text 待解析的 Markdown 文本
     */
    explicit markdown_parser(string text) noexcept;

    /**
     * @brief 构造函数
     * @param text 待解析的 Markdown 文本
     * @param opt 解析选项
     */
    explicit markdown_parser(string text, option opt) noexcept;

    markdown_parser(const markdown_parser&) = delete;
    markdown_parser& operator=(const markdown_parser&) = delete;
    markdown_parser(markdown_parser&&) noexcept = default;
    markdown_parser& operator=(markdown_parser&&) noexcept = default;

    /**
     * @brief 执行解析
     * @return 解析完成的文档模型
     *
     * 解析整段文本，返回顶层块序列与收集到的链接引用定义。
     * 空输入返回空文档。
     */
    NEFORCE_NODISCARD markdown_document parse();

    /**
     * @brief 把整段文本作为单段行内内容解析
     * @param document 供引用式链接解析使用的文档，可为 nullptr
     * @return 行内节点列表
     *
     * 用于处理富文本片段、标题或表格单元格等不涉及块级结构的内容。
     */
    NEFORCE_NODISCARD md_inlines parse_inline(const markdown_document* document = nullptr);

    /**
     * @brief 获取解析选项
     * @return 解析选项常量引用
     */
    NEFORCE_NODISCARD const option& options() const noexcept { return opt_; }

    /**
     * @brief 设置解析选项
     * @param opt 解析选项
     * @return 自身引用
     */
    markdown_parser& set_options(option opt) noexcept;
};

/** @} */ // Markdown

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_MD_MARKDOWN_PARSER_HPP__
