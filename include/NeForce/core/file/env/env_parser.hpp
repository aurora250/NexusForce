#ifndef NEFORCE_CORE_FILE_ENV_ENV_PARSER_HPP__
#define NEFORCE_CORE_FILE_ENV_ENV_PARSER_HPP__

/**
 * @file env_parser.hpp
 * @brief ENV 解析器
 */

#include "NeForce/core/file/env/env_value.hpp"
#include "NeForce/core/utility/optional.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup ConfigFormat 配置格式操作
 * @{
 */

/**
 * @addtogroup EnvConfig ENV配置
 * @{
 */

/**
 * @class env_parser
 * @brief ENV 配置解析器
 *
 * 支持以下特性：
 * - 变量定义（KEY=VALUE）
 * - export 前缀
 * - # 注释与空行
 * - 单引号、双引号包裹的值
 * - 双引号值中的 \n、\r、\t、\\、\"、\$、\` 转义
 * - 单引号值中的 '\'' 转义
 * - 无引号值在空白或 # 处结束
 */
class NEFORCE_API env_parser {
private:
    string owned_;                 ///< 拥有模式下的输入文本
    string_view text_;             ///< 输入视图
    const char* cursor_ = nullptr; ///< 当前扫描位置
    const char* end_ = nullptr;    ///< 扫描结束位置
    bool owning_ = false;          ///< 是否复制键值与注释到文档内存
    string scratch_;               ///< 转义解码临时缓冲

    env_document document_; ///< 解析结果

    void parse_line(const char* begin, const char* end);
    NEFORCE_NODISCARD string_view parse_quoted(const char* begin, const char* end, env_quote& quote, bool& decoded);

public:
    /**
     * @brief 构造借用输入的解析器
     * @param text 输入文本，调用方需保证其生命周期不短于解析结果
     */
    explicit env_parser(string_view text) noexcept;

    /**
     * @brief 构造拥有输入的解析器
     * @param text 输入文本，解析器持有其副本
     */
    explicit env_parser(string text);

    /**
     * @brief 构造拥有输入的解析器
     * @param text 输入文本，解析器持有其副本
     */
    explicit env_parser(const char* text);

    /**
     * @brief 执行解析
     * @return 解析完成的文档
     * @throws allocate_exception 内存分配失败时抛出
     */
    env_document parse();

    /**
     * @brief 尝试执行解析
     * @return 成功时返回文档，失败时返回空 optional
     */
    optional<env_document> try_parse() noexcept;
};

/** @} */ // EnvConfig

/** @} */ // ConfigFormat

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_ENV_ENV_PARSER_HPP__
