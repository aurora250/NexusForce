#ifndef NEFORCE_CORE_ENCODING_PERCENT_HPP__
#define NEFORCE_CORE_ENCODING_PERCENT_HPP__

/**
 * @file percent.hpp
 * @brief 百分号编码（Percent-Encoding）实现
 *
 * 此文件提供 RFC 3986 定义的 URI 百分号编码，以及 application/x-www-form-urlencoded
 * 使用的空格转 `+` 变体。
 */

#include "NeForce/core/string/string.hpp"
#include "NeForce/core/utility/optional.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Encoding 编码
 * @brief 可逆但不提供保密性的数据表示转换。
 * @{
 */

/**
 * @defgroup Percent 百分号编码
 * @ingroup Encoding
 * @brief URI 百分号编码与表单编码
 *
 * @section standards 遵循的国际标准
 * - **IETF RFC 3986 Section 2.1**：百分号编码
 *   https://www.rfc-editor.org/rfc/rfc3986.html#section-2.1
 * - **WHATWG URL Standard**：application/x-www-form-urlencoded
 *   https://url.spec.whatwg.org/#application/x-www-form-urlencoded
 *
 * @section implementation_details 实现细节
 * - 未保留字符 `A-Z`、`a-z`、`0-9`、`-`、`_`、`.`、`~` 原样输出
 * - 其余字节编码为 `%XX`，十六进制字母大写
 * - `encode` 默认编码 `/`；保留路径分隔符时可传 `encode_slash = false`
 * - `encode_form` 将空格编码为 `+`，其余规则同上
 * - `decode_tolerant` 忽略非法 `%` 序列并将 `+` 还原为空格，适合查询参数
 *
 * @warning 百分号编码不提供机密性或完整性保护，只用于数据表示。
 * @see https://www.rfc-editor.org/rfc/rfc3986
 * @{
 */

/**
 * @struct percent
 * @brief 百分号编码结构体
 */
struct NEFORCE_API percent {
    /**
     * @brief 对字节序列进行百分号编码
     * @param data 输入字节序列
     * @param encode_slash 是否编码 `/`，默认 true
     * @return 编码后的 ASCII 字符串
     */
    static string encode(cbyte_view data, bool encode_slash = true);

    /**
     * @brief 解码百分号编码
     * @param text 编码后的字符串
     * @return 解码后的字节序列
     * @throws value_exception 当 `%` 后不是两位十六进制数字时抛出
     */
    static byte_vector decode(string_view text);

    /**
     * @brief 尝试解码百分号编码
     * @param text 编码后的字符串
     * @return 成功时返回解码后的字节序列，失败时返回空 optional
     * @note 不抛出异常，适合不可信输入
     */
    NEFORCE_NODISCARD static optional<byte_vector> try_decode(string_view text) noexcept;

    /**
     * @brief 对字节序列进行表单编码
     * @param data 输入字节序列
     * @return `application/x-www-form-urlencoded` 字符串，空格编码为 `+`
     */
    static string encode_form(cbyte_view data);

    /**
     * @brief 宽容解码
     * @param text 编码后的字符串
     * @return 解码后的字符串
     * @note 非法的 `%` 序列保持原样，`+` 还原为空格
     */
    static string decode_tolerant(string_view text);
};

/**
 * @brief 百分号编码便捷函数
 * @param data 输入字节序列
 * @param encode_slash 是否编码 `/`，默认 true
 * @return 编码后的字符串
 */
NEFORCE_ALWAYS_INLINE_INLINE string percent_encode(const cbyte_view data, const bool encode_slash = true) {
    return percent::encode(data, encode_slash);
}

/**
 * @brief 百分号解码便捷函数
 * @param text 编码后的字符串
 * @return 解码后的字节序列
 * @throws value_exception 当输入包含非法 `%` 序列时抛出
 */
NEFORCE_ALWAYS_INLINE_INLINE byte_vector percent_decode(const string_view text) { return percent::decode(text); }

/**
 * @brief 表单编码便捷函数
 * @param data 输入字节序列
 * @return 表单编码后的字符串
 */
NEFORCE_ALWAYS_INLINE_INLINE string percent_encode_form(const cbyte_view data) { return percent::encode_form(data); }

/** @} */ // Percent

/** @} */ // Encoding

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ENCODING_PERCENT_HPP__
