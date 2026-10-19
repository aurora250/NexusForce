#ifndef NEFORCE_CORE_FILE_MD_MARKDOWN_BUILDER_HPP__
#define NEFORCE_CORE_FILE_MD_MARKDOWN_BUILDER_HPP__

/**
 * @file markdown_builder.hpp
 * @brief Markdown 构建器
 *
 * 把文档模型序列化回规范化的 Markdown 文本。
 */

#include "NeForce/core/file/md/markdown_value.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup Markdown Markdown 文档
 * @{
 */

/**
 * @class markdown_builder
 * @brief Markdown 构建器
 *
 * 支持两种用法：
 * - **流式累积**：反复 `append()` 追加块或引用定义，最后 `build()` 得到文本；
 * - **一次序列化**：`dump()` / `dump_inline()` 直接把已有模型写成文本，
 *   文档本身也提供 `to_string()`。
 *
 * 输出是**可往返**的：文本中的标记字符会按需转义，代码围栏按内容自适应，
 * 因此 `parse → build → parse` 的文档结构等价。
 *
 * @code
 * markdown_builder builder;
 * builder.reference("ref", "https://example.com", "Example");
 * builder.append_inline(parser.parse_inline());
 * string text = builder.build();
 * @endcode
 */
class NEFORCE_API markdown_builder {
public:
    /**
     * @brief 构建选项
     */
    struct option {
        int list_indent = 2;              ///< 列表每级缩进的空格数
        string bullet = "- ";             ///< 无序列表标记
        bool blank_between_blocks = true; ///< 块之间是否插入空行
        bool tables = true;               ///< 是否输出管道表格（关闭时表格退化为段落）

        // NOLINTNEXTLINE(hicpp-use-equals-default,modernize-use-equals-default)
        option() {}
    };

private:
    option opt_;                 ///< 构建选项
    markdown_document document_; ///< 已累积的文档

public:
    explicit markdown_builder() = default;

    /**
     * @brief 构造函数
     * @param opt 构建选项
     */
    explicit markdown_builder(option opt);

    markdown_builder(const markdown_builder&) = delete;
    markdown_builder& operator=(const markdown_builder&) = delete;
    markdown_builder(markdown_builder&&) noexcept = default;
    markdown_builder& operator=(markdown_builder&&) noexcept = default;

    /**
     * @brief 追加一个块级节点
     * @param block 块级节点
     * @return 自身引用
     */
    markdown_builder& append(const md_block& block);

    /**
     * @brief 追加整个文档（含其引用定义）
     * @param document 文档模型
     * @return 自身引用
     */
    markdown_builder& append(const markdown_document& document);

    /**
     * @brief 追加一段行内内容（作为一个段落）
     * @param inlines 行内节点
     * @return 自身引用
     */
    markdown_builder& append_inline(const md_inlines& inlines);

    /**
     * @brief 追加一条链接引用定义
     * @param label 标签
     * @param destination 目标地址
     * @param title 标题
     * @return 自身引用
     */
    markdown_builder& reference(string label, string destination, string title = string());

    /**
     * @brief 清空已累积的内容
     * @return 自身引用
     */
    markdown_builder& clear() noexcept;

    /**
     * @brief 设置构建选项
     * @param opt 构建选项
     * @return 自身引用
     */
    markdown_builder& set_options(option opt);

    /**
     * @brief 获取构建选项
     * @return 构建选项常量引用
     */
    NEFORCE_NODISCARD const option& options() const noexcept { return opt_; }

    /**
     * @brief 是否没有累积任何内容
     * @return 是否为空
     */
    NEFORCE_NODISCARD bool empty() const noexcept;

    /**
     * @brief 已累积的块数量
     * @return 块数量
     */
    NEFORCE_NODISCARD size_t block_count() const noexcept;

    /**
     * @brief 取出已累积的文档
     * @return 文档副本
     */
    NEFORCE_NODISCARD markdown_document document() const;

    /**
     * @brief 生成 Markdown 文本
     * @return Markdown 文本，末尾带一个换行
     */
    NEFORCE_NODISCARD string build() const;

    /**
     * @brief 序列化文档为 Markdown 文本
     * @param document 文档模型
     * @param opt 构建选项
     * @return Markdown 文本
     */
    NEFORCE_NODISCARD static string dump(const markdown_document& document, option opt = option());

    /**
     * @brief 序列化单个块级节点为 Markdown 文本
     * @param block 块级节点
     * @param opt 构建选项
     * @return Markdown 文本
     */
    NEFORCE_NODISCARD static string dump(const md_block& block, option opt = option());

    /**
     * @brief 序列化行内节点为 Markdown 文本
     * @param inlines 行内节点
     * @return Markdown 文本
     */
    NEFORCE_NODISCARD static string dump_inline(const md_inlines& inlines);
};

/** @} */ // Markdown

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_MD_MARKDOWN_BUILDER_HPP__
