#ifndef NEFORCE_CORE_FILE_JSON_JSON_PARSER_HPP__
#define NEFORCE_CORE_FILE_JSON_JSON_PARSER_HPP__

/**
 * @file json_parser.hpp
 * @brief json配置解析器
 *
 * 此文件提供了json配置格式的解析器实现。
 * 提供完整的语法分析和错误处理机制。
 */

#include "NeForce/core/file/json/json_value.hpp"
#include "NeForce/core/utility/optional.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup ConfigFormat 配置格式操作
 * @{
 */

/**
 * @addtogroup JsonConfig json配置
 * @{
 */

/**
 * @class json_parser
 * @brief json配置解析器
 *
 * 采用递归下降解析算法。
 * 解析过程会维护行号和列号信息，便于错误定位。
 *
 * 解析符合ECMA-404 json标准的格式，支持以下特性：
 * - 对象（Object）：键值对集合
 * - 数组（Array）：有序值列表
 * - 字符串（String）：双引号包裹的Unicode字符序列
 * - 数字（Number）：整数和浮点数
 * - 布尔值（Boolean）：true/false
 * - null值
 */
class NEFORCE_API json_parser {
private:
    string text_;    ///< 待解析的json文本
    size_t len_;     ///< 文本长度
    size_t pos_ = 0; ///< 当前解析位置

    /**
     * @brief 跳过空白字符
     *
     * 跳过空格、制表符、换行符等空白字符。
     */
    void skip_space() noexcept;

    /**
     * @brief 获取当前字符
     * @return 当前位置的字符，若已到末尾返回'\0'
     */
    char current() const noexcept;

    /**
     * @brief 检查是否已到末尾
     * @return 是否已到文件末尾
     */
    bool eof() const noexcept;

    /**
     * @brief 解析json字符串
     * @return 解析得到的字符串值
     * @throws json_exception 当字符串格式错误或未正确终止时抛出
     *
     * 解析双引号包裹的字符串，支持转义序列
     */
    unique_ptr<json_string> parse_string();

    /**
     * @brief 解析json数字
     * @return 解析得到的数字值
     * @throws json_exception 当数字格式无效时抛出
     *
     * 支持的数字格式：
     * - 整数：123, -456
     * - 浮点数：3.14, -0.5
     * - 科学计数法：1e10, 2.5e-3
     */
    unique_ptr<json_number> parse_number();

    /**
     * @brief 解析json关键字
     * @return 解析得到的关键字值
     * @throws json_exception 当遇到无效关键字时抛出
     *
     * 支持的关键字：
     * - true：布尔值true
     * - false：布尔值false
     * - null：空值
     */
    unique_ptr<json_value> parse_keyword();

    /**
     * @brief 解析json数组
     * @return 解析得到的数组对象
     * @throws json_exception 当数组格式错误时抛出
     *
     * 数组格式：[value1, value2, ...]
     * 数组元素可以是任意有效的json值类型。
     * 数组可以为空：[]
     */
    unique_ptr<json_array> parse_array();

    /**
     * @brief 解析json对象
     * @return 解析得到的对象
     * @throws json_exception 当对象格式错误时抛出
     *
     * 对象格式：{"key1": value1, "key2": value2, ...}
     * 键必须是字符串，值可以是任意有效的json值类型。
     * 对象可以为空：{}
     */
    unique_ptr<json_object> parse_object();

    /**
     * @brief 解析任意json值
     * @return 解析得到的json值
     * @throws json_exception 当遇到无效字符时抛出
     *
     * 根据当前字符决定解析的具体类型：
     * - '{'：解析对象
     * - '['：解析数组
     * - '"'：解析字符串
     * - 数字或负号：解析数字
     * - 't'/'f'/'n'：解析关键字
     */
    unique_ptr<json_value> parse_value();

public:
    /**
     * @brief 构造函数
     * @param text 待解析的json字符串
     *
     * 初始化解析器，准备开始解析。
     */
    explicit json_parser(string text) noexcept :
    text_(_NEFORCE move(text)),
    len_(text_.size()) {}

    /**
     * @brief 执行解析
     * @return 解析完成的json值根节点
     * @throws json_exception 当解析过程中遇到语法错误时抛出
     *
     * 解析整个json文本，构建完整的json值树。
     * 解析完成后会检查是否还有未处理的字符。
     */
    unique_ptr<json_value> parse();

    /**
     * @brief 尝试执行解析
     * @return 解析结果的可选对象
     *
     * 如果解析成功返回包含json值的optional，
     * 如果解析失败返回空的optional。
     */
    optional<unique_ptr<json_value>> try_parse();
};

/** @} */ // JsonConfig

/** @} */ // ConfigFormat

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_JSON_JSON_PARSER_HPP__
