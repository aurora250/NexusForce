#ifndef NEFORCE_CORE_FILE_INI_INI_PARSER_HPP__
#define NEFORCE_CORE_FILE_INI_INI_PARSER_HPP__

/**
 * @file ini_parser.hpp
 * @brief INI 解析器
 *
 * 指针游标单趟扫描，属性键与值默认直接引用输入缓冲区；
 * 输入为 string 时由解析器持有并复制到 arena，保证文档自包含。
 */

#include "NeForce/core/file/ini/ini_value.hpp"
#include "NeForce/core/utility/optional.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup ConfigFormat 配置格式操作
 * @{
 */

/**
 * @addtogroup IniConfig INI配置
 * @{
 */

/**
 * @class ini_parser
 * @brief INI 配置解析器
 *
 * 支持以下特性：
 * - 节定义（[section]）
 * - 键值对（key = value）
 * - 注释行（; 或 #）
 * - 空行与值两侧空白
 * - 单引号或双引号包裹的值
 * - 同一节的重复键由后者覆盖
 * - 同名节重复出现时重置该节
 */
class NEFORCE_API ini_parser {
private:
    string owned_;                 ///< 拥有模式下的输入文本
    string_view text_;             ///< 输入视图
    const char* cursor_ = nullptr; ///< 当前扫描位置
    const char* end_ = nullptr;    ///< 扫描结束位置
    bool owning_ = false;          ///< 是否复制键值到文档内存

    ini_document document_;               ///< 解析结果
    ini_section_data* current_ = nullptr; ///< 当前节

    void parse_line(const char* begin, const char* end);

public:
    /**
     * @brief 构造借用输入的解析器
     * @param text 输入文本，调用方需保证其生命周期不短于解析结果
     */
    explicit ini_parser(string_view text) noexcept;

    /**
     * @brief 构造拥有输入的解析器
     * @param text 输入文本，解析器持有其副本
     */
    explicit ini_parser(string text);

    /**
     * @brief 构造拥有输入的解析器
     * @param text 输入文本，解析器持有其副本
     */
    explicit ini_parser(const char* text);

    /**
     * @brief 执行解析
     * @return 解析完成的文档
     * @throws allocate_exception 内存分配失败时抛出
     */
    ini_document parse();

    /**
     * @brief 尝试执行解析
     * @return 成功时返回文档，失败时返回空 optional
     */
    optional<ini_document> try_parse() noexcept;
};

/** @} */ // IniConfig

/** @} */ // ConfigFormat

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_INI_INI_PARSER_HPP__
