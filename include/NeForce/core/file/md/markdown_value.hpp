#ifndef NEFORCE_CORE_FILE_MD_MARKDOWN_VALUE_HPP__
#define NEFORCE_CORE_FILE_MD_MARKDOWN_VALUE_HPP__

/**
 * @file markdown_value.hpp
 * @brief Markdown 文档模型
 *
 * 定义与呈现无关的 Markdown 抽象语法树：块级节点、行内节点、链接引用定义表与文档根。
 * 节点与连续数组都由文档的 bump_arena 持有，字符串以 string_view 引用文档源文本或 arena 副本。
 */

#include "NeForce/core/exception/exception.hpp"
#include "NeForce/core/interface/istringify.hpp"
#include "NeForce/core/memory/bump_arena.hpp"
#include "NeForce/core/memory/memory.hpp"
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


/// @cond
NEFORCE_BEGIN_INNER__

NEFORCE_API bump_arena*& md_arena_slot() noexcept;
NEFORCE_API bump_arena& md_fallback_arena() noexcept;
NEFORCE_API bump_arena& md_inline_arena() noexcept;

class md_arena_scope {
private:
    bump_arena* previous_;

public:
    explicit md_arena_scope(bump_arena* arena) noexcept :
    previous_(md_arena_slot()) {
        md_arena_slot() = arena;
    }

    ~md_arena_scope() { md_arena_slot() = previous_; }

    md_arena_scope(const md_arena_scope&) = delete;
    md_arena_scope& operator=(const md_arena_scope&) = delete;
};

NEFORCE_END_INNER__
/// @endcond


/**
 * @class md_list
 * @brief arena 连续数组
 * @tparam T 元素类型
 */
template <typename T>
class md_list {
private:
    T* data_ = nullptr;
    uint32_t size_ = 0;
    uint32_t capacity_ = 0;

    void ensure(const uint32_t needed) {
        if (needed <= capacity_) {
            return;
        }
        uint32_t capacity = capacity_ == 0 ? 4 : capacity_;
        while (capacity < needed) {
            capacity *= 2;
        }
        bump_arena* arena = inner::md_arena_slot();
        if (arena == nullptr) {
            arena = &inner::md_fallback_arena();
        }
        auto* data = static_cast<T*>(arena->allocate(capacity, sizeof(T), alignof(T)));
        if (size_ != 0) {
            _NEFORCE memory_copy(data, data_, static_cast<size_t>(size_) * sizeof(T));
        }
        data_ = data;
        capacity_ = capacity;
    }

public:
    using value_type = T;
    using iterator = T*;
    using const_iterator = const T*;

    md_list() = default;

    NEFORCE_NODISCARD size_t size() const noexcept { return size_; }

    NEFORCE_NODISCARD bool empty() const noexcept { return size_ == 0; }

    NEFORCE_NODISCARD size_t capacity() const noexcept { return capacity_; }

    NEFORCE_NODISCARD T* data() noexcept { return data_; }

    NEFORCE_NODISCARD const T* data() const noexcept { return data_; }

    NEFORCE_NODISCARD T& operator[](const size_t index) noexcept { return data_[index]; }

    NEFORCE_NODISCARD const T& operator[](const size_t index) const noexcept { return data_[index]; }

    NEFORCE_NODISCARD T& back() noexcept { return data_[size_ - 1]; }

    NEFORCE_NODISCARD const T& back() const noexcept { return data_[size_ - 1]; }

    NEFORCE_NODISCARD iterator begin() noexcept { return data_; }

    NEFORCE_NODISCARD const_iterator begin() const noexcept { return data_; }

    NEFORCE_NODISCARD iterator end() noexcept { return data_ + size_; }

    NEFORCE_NODISCARD const_iterator end() const noexcept { return data_ + size_; }

    void reserve(const size_t count) { ensure(static_cast<uint32_t>(count)); }

    void clear() noexcept { size_ = 0; }

    void push_back(const T& value) {
        ensure(size_ + 1);
        data_[size_++] = value;
    }

    void push_back(T&& value) {
        ensure(size_ + 1);
        data_[size_++] = _NEFORCE move(value);
    }

    template <typename... Args>
    T& emplace_back(Args&&... args) {
        ensure(size_ + 1);
        data_[size_] = T(_NEFORCE forward<Args>(args)...);
        return data_[size_++];
    }

    void pop_back() noexcept { --size_; }
};

/**
 * @brief 行内节点
 */
struct md_inline {
    md_inline_kind kind = md_inline_kind::text; ///< 节点类型
    string_view text;                           ///< 文本内容
    string_view destination;                    ///< 链接/图片/自动链接地址
    string_view title;                          ///< 链接/图片标题
    string_view reference;                      ///< 引用式链接的标签（空表示内联）
    md_list<md_inline> children;                ///< 子节点
};

/**
 * @brief 行内节点列表
 */
using md_inlines = md_list<md_inline>;

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
    string_view info;      ///< code_block 围栏后的原始信息串
    string_view language;  ///< code_block 信息串首词
    string_view literal;   ///< code_block/html_block 原始内容

    int html_type = 0; ///< html_block 类型（1-7）

    bool ordered = false; ///< list/list_item 是否有序
    int start = 1;        ///< list 起始编号
    char delimiter = '.'; ///< list 有序分隔符（`.` 或 `)`）
    char bullet = '-';    ///< list 无序标记字符
    bool tight = true;    ///< list 是否紧凑
    int number = 1;       ///< list_item 的编号
    bool task = false;    ///< list_item 是否为任务项
    bool checked = false; ///< 任务项是否已勾选

    md_list<md_align> alignments;      ///< table 列对齐
    md_list<md_inlines> header;        ///< table 表头单元格
    md_list<md_list<md_inlines>> rows; ///< table 数据行

    md_inlines inlines;         ///< paragraph/heading 的行内内容
    md_list<md_block> children; ///< block_quote/list/list_item 的子块
};

/**
 * @brief 块级节点列表
 */
using md_blocks = md_list<md_block>;

/**
 * @brief 链接引用定义
 */
struct md_reference {
    string_view label;       ///< 引用标签
    string_view destination; ///< 目标地址
    string_view title;       ///< 标题
};

/**
 * @brief Markdown 文档
 */
class NEFORCE_API markdown_document : public istringify<markdown_document> {
private:
    bump_arena arena_;   ///< 节点与字符串分配器
    string_view source_; ///< 源文本副本视图

    md_block clone_block(const md_block& block);
    md_inline clone_inline(const md_inline& inline_node);
    md_inlines clone_inlines(const md_inlines& inlines);

public:
    md_blocks blocks;                 ///< 顶层块序列
    md_list<md_reference> references; ///< 链接引用定义

    markdown_document() = default;
    markdown_document(const markdown_document&) = delete;
    markdown_document& operator=(const markdown_document&) = delete;
    markdown_document(markdown_document&&) noexcept = default;
    markdown_document& operator=(markdown_document&&) noexcept = default;
    ~markdown_document() = default;

    /**
     * @brief 获取分配器
     * @return arena 引用
     */
    NEFORCE_NODISCARD bump_arena& arena() noexcept { return arena_; }

    /**
     * @brief 复制源文本到 arena
     * @param text 源文本
     */
    void set_source(string_view text);

    /**
     * @brief 获取源文本
     * @return 源文本视图
     */
    NEFORCE_NODISCARD string_view source() const noexcept { return source_; }

    /**
     * @brief 深拷贝块到本文档 arena
     * @param block 源块
     * @return 拷贝后的块
     */
    NEFORCE_NODISCARD md_block clone(const md_block& block) { return clone_block(block); }

    /**
     * @brief 深拷贝行内列表到本文档 arena
     * @param inlines 源列表
     * @return 拷贝后的列表
     */
    NEFORCE_NODISCARD md_inlines clone(const md_inlines& inlines) { return clone_inlines(inlines); }

    /**
     * @brief 序列化为 Markdown 文本
     * @return Markdown 文本
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
 */
NEFORCE_NODISCARD NEFORCE_API const md_reference* md_find_reference(const markdown_document& document,
                                                                    string_view label);

/** @} */ // Markdown

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_MD_MARKDOWN_VALUE_HPP__
