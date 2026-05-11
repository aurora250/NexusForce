#ifndef NEFORCE_CORE_FILE_YAML_YAML_PARSER_HPP__
#define NEFORCE_CORE_FILE_YAML_YAML_PARSER_HPP__

/**
 * @file yaml_parser.hpp
 * @brief YAML 1.2 解析器
 *
 * 此文件提供了YAML（YAML Ain't Markup Language）格式的递归下降解析器实现。
 */

#include "NeForce/core/file/yaml/yaml_value.hpp"
#include "NeForce/core/utility/optional.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup ConfigFormat 配置格式操作
 * @{
 */

/**
 * @addtogroup YamlConfig YAML配置
 * @{
 */

/**
 * @class yaml_parser
 * @brief YAML 1.2 递归下降解析器
 *
 * 提供完整的YAML 1.2格式解析功能，将YAML文本转换为类型化值树（yaml_value层次结构）。
 *
 * @section parser_architecture 解析器架构
 *
 * 本解析器采用**递归下降**（Recursive Descent）算法，通过以下核心机制实现
 * 对YAML 1.2完整语法的解析：
 *
 * | 机制              | 说明                                           |
 * |-------------------|------------------------------------------------|
 * | 递归下降          | 每个语法结构对应一个解析方法，相互递归调用     |
 * | 缩进感知          | 通过 indent_stack_ 追踪嵌套层级               |
 * | 流上下文标志       | in_flow_context_ 标志区分流样式与块样式         |
 * | 锚点注册表        | anchors_ 哈希表存储锚点引用，支持别名解析      |
 * | 位置追踪          | line_ / column_ 计数器支持精确定位错误         |
 *
 * @section parser_features 支持的YAML 1.2特性
 *
 * | 特性                  | YAML 1.2 引用 | 说明                                   |
 * |-----------------------|---------------|----------------------------------------|
 * | 空值（Null）          | §10.2         | null、Null、NULL、~                     |
 * | 布尔值（Boolean）     | §10.3         | true/false 及所有同义词（yes/no、on/off等）|
 * | 整数（Integer）       | §10.4.1       | 十进制/十六进制(0x)/八进制(0o)/二进制(0b) |
 * | 浮点数（Float）       | §10.4.1       | 含 .inf、-.inf、.nan 特殊值            |
 * | 字符串（String）      | §10.5         | 五种样式：Plain、SingleQuoted、DoubleQuoted、Literal、Folded |
 * | 时间戳（Timestamp）   | §10.4.2       | ISO 8601 / RFC 3339 格式               |
 * | 序列（Sequence）      | §10.1.1       | 块样式（- item）和流样式（[item, ...]） |
 * | 映射（Mapping）       | §10.1.2       | 块样式（key: value）和流样式（{key: value}） |
 * | 锚点与别名            | §3.2.2        | &anchor 和 *alias                      |
 * | 标签（Tag）           | §3.2.3        | !tag、!!type、!<verbatim>、!handle!suffix |
 * | 复杂键（Complex Key） | §3.2.1        | ? 指示符，键可以是任意YAML节点          |
 * | 多文档                | §9.2          | --- 文档分隔符，... 文档结束符          |
 * | 指令（Directive）     | §9.1          | %YAML 1.2、%TAG                        |
 * | 注释                  | §6.8          | # 行注释                                |
 * | 块标量吞食指示符       | §8.2          | 保留(+)、剥离(-)换行符                  |
 *
 * @section parser_usage 使用示例
 *
 * 基本解析（单文档）：
 * @code
 * yaml_parser parser("key: value\nlist: [1, 2, 3]");
 * auto root = parser.parse();
 * if (auto* map = root->as_mapping()) {
 *     auto* val = map->get_member("key");
 * }
 * @endcode
 *
 * 带错误处理的解析：
 * @code
 * yaml_parser parser(yaml_content);
 * auto result = parser.try_parse();
 * if (result.has_value()) {
 *     // 解析成功，使用 result.value()
 * } else {
 *     // 解析失败
 * }
 * @endcode
 *
 * 多文档解析：
 * @code
 * yaml_parser parser("---\ndoc1: value\n---\ndoc2: value\n");
 * auto docs = parser.parse_documents();
 * for (auto& doc : docs) {
 *     // 处理每个文档
 * }
 * @endcode
 *
 * @section parser_implementation 实现细节
 *
 * | 特性              | 规范参数                                    |
 * |-------------------|---------------------------------------------|
 * | 编码              | UTF-8（YAML 1.2 §5.1）                       |
 * | 整数范围          | -2^63 到 2^63-1（YAML 1.2 §10.4.1）         |
 * | 浮点数精度        | IEEE 754-2019 双精度（YAML 1.2 §10.4.1）    |
 * | 映射键唯一性      | YAML 1.2 §3.2.1（重复键不报错但行为未定义）  |
 * | 缩进要求          | 仅空格（YAML 1.2 §6.1），制表符不用于缩进    |
 * | 最大缩进深度      | 受栈大小限制（递归下降）                      |
 *
 * @warning 根据 YAML 1.2 §6.1，制表符（tab）不应用于缩进。
 * @warning 流样式中的重复键名行为未定义，应避免使用。
 * @warning 解析器不验证映射中的键名唯一性（遵循YAML 1.2规范）。
 *
 * @see https://yaml.org/
 * @see https://yaml.org/spec/1.2.2/
 * @see yaml_value 解析结果的数据结构
 * @see yaml_builder YAML构建器
 */
class NEFORCE_API yaml_parser {
private:
    /**
     * @struct indent_context
     * @brief 缩进上下文结构
     *
     * 记录当前嵌套层级的状态信息，用于处理YAML的缩进敏感语法。
     * 当缩进增大时压入新上下文，缩进减小时弹出上下文。
     */
    struct indent_context {
        size_t level;            ///< 当前缩进级别（空格数）
        yaml_mapping* mapping;   ///< 当前映射指针（在映射上下文中）
        yaml_sequence* sequence; ///< 当前序列指针（在序列上下文中）
        string key;              ///< 当前键名
        bool is_sequence;        ///< 是否为序列上下文
    };

    string yaml_;       ///< 待解析的YAML源文本
    size_t len_;        ///< 源文本长度
    size_t pos_ = 0;    ///< 当前解析位置
    size_t line_ = 1;   ///< 当前行号（从1开始，用于错误报告）
    size_t column_ = 1; ///< 当前列号（从1开始，用于错误报告）

    vector<indent_context> indent_stack_; ///< 缩进上下文栈
    shared_ptr<yaml_value> root_;         ///< 解析结果的根节点
    size_t current_indent_ = 0;           ///< 当前行的缩进级别
    bool in_flow_context_ = false;        ///< 是否处于流样式上下文中

    unordered_map<string, shared_ptr<yaml_value>> anchors_; ///< 锚点名到节点的映射表

    /**
     * @brief 获取当前字符
     * @return 当前位置的字符
     */
    char current() const noexcept;

    /**
     * @brief 预读后续字符
     * @param offset 从当前位置的偏移量（默认1）
     * @return 目标位置的字符，超出边界返回 '\\0'
     */
    char peek(size_t offset = 1) const noexcept;

    /**
     * @brief 检查是否到达文件末尾
     * @return 如果 pos_ >= len_ 返回 true
     */
    bool eof() const noexcept;

    /**
     * @brief 前进一个字符并更新行列号
     */
    void advance() noexcept;

    /**
     * @brief 尝试匹配当前字符
     * @param ch 期望匹配的字符
     * @return 匹配成功返回 true 并前进，否则返回 false
     */
    bool match(char ch) noexcept;

    /**
     * @brief 期望当前字符匹配，否则抛出解析错误
     * @param ch 期望的字符
     * @return 总是返回 true（匹配成功时）
     * @throws yaml_exception 不匹配时抛出
     */
    bool expect(char ch);

    /**
     * @brief 跳过行内空白字符（空格和制表符），但在换行符处停止
     */
    void skip_whitespace_inline() noexcept;

    /**
     * @brief 跳过从当前位置到行尾的注释内容
     */
    void skip_comment() noexcept;

    /**
     * @brief 跳过当前行剩余内容，前进到下一行行首
     */
    void skip_to_next_line() noexcept;

    /**
     * @brief 跳过连续的空白行（仅含空格/制表符/注释的行）
     *
     * 通过预读判断行是否真正空白后才消费，避免误吞内容行的缩进。
     */
    void skip_blank_lines() noexcept;

    /**
     * @brief 跳过空白字符和注释，跨行处理
     */
    void skip_whitespace_and_comments();

    /**
     * @brief 判断字符是否为YAML空白字符
     * @param ch 待检查的字符
     * @return 如果是空格或制表符返回 true
     */
    bool is_whitespace(char ch) const noexcept;

    /**
     * @brief 判断字符是否为换行符
     * @param ch 待检查的字符
     * @return 如果是 '\\n' 或 '\\r' 返回 true
     */
    bool is_newline(char ch) const noexcept;

    /**
     * @brief 预读当前行的缩进级别（空格数）
     * @return 当前行的缩进空格数
     *
     * 不移动解析位置，仅读取。
     */
    size_t peek_indent() const noexcept;

    /**
     * @brief 跳过当前行的缩进空格
     * @return 跳过的空格数（即缩进级别）
     */
    size_t skip_indent();

    /**
     * @brief 根据缩进变化调整上下文栈
     * @param new_indent 新的缩进级别
     *
     * - 缩进增大：压入新的上下文（由调用者处理）
     * - 缩进减小：弹出上下文直到找到匹配的缩进级别
     * - 缩进相同：保持当前上下文
     */
    void handle_indent_change(size_t new_indent);

    /**
     * @brief 解析YAML指令（%YAML 1.2、%TAG）
     *
     * 指令只能出现在文档开始标记之前。
     * 当前实现对 %YAML 1.2 验证版本号，对 %TAG 跳过处理。
     */
    void parse_directive();

    /**
     * @brief 解析锚点标记（& 后跟锚点名）
     * @return 锚点名字符串
     */
    string parse_anchor();

    /**
     * @brief 将节点注册到锚点表
     * @param anchor 锚点名
     * @param value 要注册的节点
     */
    void register_anchor(const string& anchor, const shared_ptr<yaml_value>& value);

    /**
     * @brief 解析别名引用（* 后跟锚点名）
     * @return 别名引用的目标节点
     * @throws yaml_exception 别名未定义时抛出
     */
    shared_ptr<yaml_value> parse_alias();

    /**
     * @brief 跳过标签标记（! 标记）
     *
     * 当不需要标签内容时使用，仅消费标签语法。
     */
    void skip_tag() noexcept;

    /**
     * @brief 解析标签（!tag、!!type、!<verbatim>、!handle!suffix）
     * @return 标签字符串
     */
    string parse_tag();

    /**
     * @brief 检查当前位置是否有锚点标记（&）
     * @return 如果当前字符是 '&' 返回 true
     */
    bool has_anchor() const noexcept;

    /**
     * @brief 检查当前位置是否有别名标记（*）
     * @return 如果当前字符是 '*' 返回 true
     */
    bool has_alias() const noexcept;

    /**
     * @brief 解析纯文本字符串（无引号）
     * @return 解析后的字符串值
     *
     * 纯文本字符串不能包含指示符字符，边界由空白或特殊字符决定。
     */
    shared_ptr<yaml_string> parse_plain_string();

    /**
     * @brief 解析单引号字符串
     * @return 解析后的字符串值（样式 = SingleQuoted）
     *
     * 单引号字符串中，两个连续单引号 '' 表示一个字面单引号字符。
     */
    shared_ptr<yaml_string> parse_single_quoted_string();

    /**
     * @brief 解析双引号字符串
     * @return 解析后的字符串值（样式 = DoubleQuoted）
     *
     * 双引号字符串支持完整的转义序列（\\n、\\t、\\uXXXX、\\UXXXXXXXX 等）。
     */
    shared_ptr<yaml_string> parse_double_quoted_string();

    /**
     * @brief 解析字面量块字符串（| 指示符）
     * @return 解析后的字符串值（样式 = Literal）
     *
     * 字面量块保留所有换行符，不进行折叠。
     */
    shared_ptr<yaml_string> parse_literal_string();

    /**
     * @brief 解析折叠块字符串（> 指示符）
     * @return 解析后的字符串值（样式 = Folded）
     *
     * 折叠块将换行符替换为空格，但保留段落之间的空行。
     */
    shared_ptr<yaml_string> parse_folded_string();

    /**
     * @brief 解析多行字符串的通用实现
     * @param is_literal true为字面量样式(|)，false为折叠样式(>)
     * @return 解析后的字符串内容
     *
     * 处理块标量的缩进检测、吞食指示符(+/-)和内容提取。
     */
    string parse_multiline_string(bool is_literal);

    /**
     * @brief 解析标量值（自动类型检测）
     * @return 解析后的YAML值
     *
     * 根据内容自动检测并解析为以下类型之一：
     * - Null：null、~ 等
     * - Boolean：true/false 等
     * - Integer：42、0x2A 等
     * - Float：3.14、.inf 等
     * - Timestamp：2024-01-15T10:30:00Z 等
     * - String：无法匹配以上类型时作为纯文本字符串
     */
    shared_ptr<yaml_value> parse_scalar();

    /**
     * @brief 解析数字（整数或浮点数）
     * @return 解析后的整数值或浮点数值
     */
    shared_ptr<yaml_value> parse_number();

    /**
     * @brief 解析布尔值（true/false/yes/no/on/off 及其变体）
     * @return 解析后的布尔值
     */
    shared_ptr<yaml_boolean> parse_boolean();

    /**
     * @brief 解析空值（null/Null/NULL/~）
     * @return 解析后的空值
     */
    shared_ptr<yaml_null> parse_null();

    /**
     * @brief 将字符串解析为时间戳
     * @param str ISO 8601 / RFC 3339 格式的时间戳字符串
     * @return 解析后的时间戳值
     * @throws yaml_exception 格式无效时抛出
     */
    shared_ptr<yaml_timestamp> parse_timestamp(string_view str) const;

    /**
     * @brief 解析流样式序列（[item1, item2, ...]）
     * @return 解析后的序列值（样式 = Flow）
     */
    shared_ptr<yaml_sequence> parse_flow_sequence();

    /**
     * @brief 解析流样式映射（{key: value, ...}）
     * @return 解析后的映射值（样式 = Flow）
     */
    shared_ptr<yaml_mapping> parse_flow_mapping();

    /**
     * @brief 解析块样式序列（- item 格式）
     * @return 解析后的序列值（样式 = Block）
     *
     * 处理缩进感知的块序列，支持嵌套映射和子序列。
     */
    shared_ptr<yaml_sequence> parse_block_sequence();

    /**
     * @brief 解析块样式映射（key: value 格式）
     * @param parent_skipped_indent 父级是否已跳过缩进
     * @return 解析后的映射值（样式 = Block）
     *
     * 处理缩进感知的块映射，支持复杂键（? 指示符）、
     * 锚点和标签，以及嵌套的序列和子映射。
     */
    shared_ptr<yaml_mapping> parse_block_mapping(bool parent_skipped_indent);

    /**
     * @brief 解析映射键名（自动检测纯文本或引号样式）
     * @return 解析后的键名字符串
     */
    string parse_key();

    /**
     * @brief 解析纯文本键名（无引号）
     * @return 键名字符串
     */
    string parse_plain_key();

    /**
     * @brief 解析引号键名（单引号或双引号）
     * @return 键名字符串
     */
    string parse_quoted_key();

    /**
     * @brief 解析任意YAML值（顶层调度器）
     * @return 解析后的YAML值
     *
     * 根据当前字符判断值的类型并分派到对应的解析方法。
     * 处理锚点、标签、别名和复杂键。
     */
    shared_ptr<yaml_value> parse_value();

    /**
     * @brief 解析块上下文中的值
     * @return 解析后的YAML值
     *
     * 用于块样式映射的值位置，处理跨行值和复杂键。
     */
    shared_ptr<yaml_value> parse_block_value();

    /**
     * @brief 解析行内值
     * @return 解析后的YAML值
     *
     * 用于流样式和内联上下文，不处理跨行情况。
     */
    shared_ptr<yaml_value> parse_inline_value();

    /**
     * @brief 解析单个YAML文档
     * @return 文档的根节点
     *
     * 处理文档开始标记(---)、指令、注释和文档结束标记(...)。
     */
    shared_ptr<yaml_value> parse_single_document();

    /**
     * @brief 解析文档开始标记（---）
     */
    void parse_document_start();

    /**
     * @brief 解析文档结束标记（...）
     */
    void parse_document_end();

    /**
     * @brief 检查当前位置是否为文档开始标记
     * @return 如果是 --- 返回 true
     */
    bool is_document_start() const noexcept;

    /**
     * @brief 检查当前位置是否为文档结束标记
     * @return 如果是 ... 返回 true
     */
    bool is_document_end() const noexcept;

    /**
     * @brief 判断字符是否可作为纯文本内容
     * @param ch 待检查的字符
     * @return 如果是安全字符返回 true
     *
     * 纯文本字符串不能包含指示符（如 :、#、[、] 等）或流指示符。
     */
    bool is_plain_safe(char ch) const noexcept;

    /**
     * @brief 判断字符是否可用于键名
     * @param ch 待检查的字符
     * @return 如果是有效的键名字符返回 true
     */
    bool is_key_char(char ch) const noexcept;

    /**
     * @brief 判断字符是否为YAML指示符
     * @param ch 待检查的字符
     * @return 如果是指示符返回 true
     *
     * 指示符包括：-、?、:、,、[、]、{、}、#、&、*、!、|、>、'、"、%、@、`
     */
    bool is_indicator(char ch) const noexcept;

    /**
     * @brief 判断字符是否为流样式指示符
     * @param ch 待检查的字符
     * @return 如果是流指示符返回 true
     *
     * 流指示符包括：[、]、{、}、,
     */
    bool is_flow_indicator(char ch) const noexcept;

    /**
     * @brief 解析Unicode转义序列
     * @param digits 十六进制数字位数（4用于\\u，8用于\\U）
     * @return 对应的Unicode码点
     * @throws yaml_exception 转义序列无效时抛出
     */
    char32_t parse_unicode_escape(size_t digits);

    /**
     * @brief 反转义字符串中的转义序列
     * @param str 包含转义序列的原始字符串
     * @return 反转义后的字符串
     *
     * 处理 \\\\、\\"、\\n、\\t、\\r、\\b、\\f、\\/、\\uXXXX、\\UXXXXXXXX 等转义序列。
     */
    string unescape_string(const string& str) const;

    /**
     * @brief 抛出带位置信息的解析错误
     * @param message 错误描述信息
     *
     * 格式：Parse error at line X, column Y: message
     * 此函数标记为 NEFORCE_NORETURN，调用后不会返回。
     */
    NEFORCE_NORETURN void throw_parse_error(const string& message) const;

public:
    /**
     * @brief 构造函数
     * @param yaml_str 待解析的YAML字符串（将被移动）
     *
     * 构造后立即可以调用 parse() 或 try_parse() 进行解析。
     *
     * @note 解析器获取字符串的所有权，原字符串将被清空。
     */
    explicit yaml_parser(string yaml_str) noexcept :
    yaml_(move(yaml_str)),
    len_(yaml_.size()) {}

    /**
     * @brief 解析YAML文档
     * @return 解析结果的根节点（yaml_value树）
     * @throws yaml_exception 解析失败时抛出，包含行号和列号信息
     *
     * 解析单个YAML文档。如果输入包含多个文档（由 --- 分隔），
     * 仅返回第一个文档。使用 parse_documents() 解析所有文档。
     *
     * @note 每次调用重新解析整个输入。
     */
    shared_ptr<yaml_value> parse();

    /**
     * @brief 尝试解析YAML文档（不抛出异常）
     * @return 成功时返回包含根节点的 optional，失败时返回空 optional
     *
     * 与 parse() 功能相同，但解析失败时不抛出异常，
     * 而是返回一个空的 optional 对象。
     */
    optional<shared_ptr<yaml_value>> try_parse();

    /**
     * @brief 解析多个YAML文档
     * @return 所有文档的根节点列表
     * @throws yaml_exception 解析失败时抛出
     *
     * 解析由 --- 分隔的多个YAML文档。如果只有单个文档，
     * 返回包含一个元素的vector。
     *
     * @see parse() 单文档解析
     */
    vector<shared_ptr<yaml_value>> parse_documents();

    /**
     * @brief 尝试解析多个YAML文档（不抛出异常）
     * @return 成功时返回文档列表，失败时返回空 optional
     *
     * 与 parse_documents() 功能相同，但解析失败时不抛出异常。
     */
    optional<vector<shared_ptr<yaml_value>>> try_parse_documents();
};

/** @} */ // YamlConfig

/** @} */ // ConfigFormat

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_YAML_YAML_PARSER_HPP__
