#ifndef NEFORCE_TUI_DOM_HIGHLIGHT_HPP__
#define NEFORCE_TUI_DOM_HIGHLIGHT_HPP__

/**
 * @file highlight.hpp
 * @brief 表驱动语法高亮
 *
 * 以语言规则表描述关键字、类型、常量、注释与字符串定界符，
 * 由同一个通用扫描器完成单行代码的记号分类。
 */

#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/string/string_view.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 语法记号分类
 */
enum class syntax_class : uint8_t {
    plain = 0,   ///< 普通文本
    keyword,     ///< 关键字
    string,      ///< 字符串
    comment,     ///< 注释
    number,      ///< 数字
    function,    ///< 函数名
    type,        ///< 类型/类名
    variable,    ///< 变量/属性
    op,          ///< 运算符
    punctuation, ///< 标点
    constant,    ///< 常量/字面量
};

/**
 * @brief 一段同色记号
 */
struct syntax_run {
    size_t begin = 0;                       ///< 起始字节偏移
    size_t end = 0;                         ///< 结束字节偏移（不含）
    syntax_class cls = syntax_class::plain; ///< 分类
};

/**
 * @brief 跨行词法状态
 */
struct highlight_state {
    /**
     * @brief 词法模式
     */
    enum class mode : uint8_t {
        normal,         ///< 普通状态
        block_comment,  ///< 块注释中
        string_literal, ///< 字符串中
    };

    mode current = mode::normal; ///< 当前状态
    char quote = 0;              ///< 字符串引号
    bool escaped = false;        ///< 上一行是否以转义符结尾
};

/**
 * @brief 语言高亮器
 */
class NEFORCE_API highlighter {
public:
    /**
     * @brief 析构高亮器
     */
    virtual ~highlighter();

    /**
     * @brief 高亮一行
     * @param line 单行文本（不含换行符）
     * @param state 跨行词法状态
     * @param out 输出记号列表，函数入口会清空
     * @note 必须按 begin 升序、互不重叠地填充 out；未覆盖的区间视为 plain
     */
    virtual void highlight_line(string_view line, highlight_state& state, vector<syntax_run>& out) const = 0;
};

/**
 * @brief 按语言名查找高亮器
 * @param language 语言标识或别名（大小写不敏感）
 * @return 高亮器指针，未知语言返回 nullptr
 */
NEFORCE_API const highlighter* find_highlighter(string_view language);

/**
 * @brief 支持的语言数量
 * @return 语言数量
 */
NEFORCE_API size_t highlighter_count() noexcept;

/**
 * @brief 按索引获取支持的语言名
 * @param index 语言索引
 * @return 规范语言名，索引越界时返回空视图
 */
NEFORCE_API string_view highlighter_language(size_t index);

/** @} */

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif
