#ifndef NEFORCE_CORE_ENCODING_BASE32_HPP__
#define NEFORCE_CORE_ENCODING_BASE32_HPP__

/**
 * @file base32.hpp
 * @brief Base32编解码实现
 *
 * 此文件提供了Base32编码和解码功能的实现，支持 RFC 4648 定义的标准 Base32
 * 与扩展十六进制 Base32（base32hex）两种变体。
 */

#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Encoding 编码
 * @brief 可逆但不提供保密性的数据表示转换。
 * @{
 */

/**
 * @defgroup Base32 Base32
 * @ingroup Encoding
 * @brief Base32编解码功能
 *
 * @section standards 遵循的国际标准
 * - **IETF RFC 4648 Section 6**：Base 32 编码
 *   https://www.rfc-editor.org/rfc/rfc4648.html#section-6
 * - **IETF RFC 4648 Section 7**：Base 32 编码的扩展十六进制字母表
 *   https://www.rfc-editor.org/rfc/rfc4648.html#section-7
 *
 * @section alphabet_comparison 字符表对比
 * | 索引  | 标准 Base32 (RFC 4648 §6) | base32hex (RFC 4648 §7) |
 * |-------|---------------------------|-------------------------|
 * | 0-9   | A-J                       | 0-9                     |
 * | 10-15 | K-P                       | A-F                     |
 * | 16-25 | Q-Z                       | G-P                     |
 * | 26-31 | 2-7                       | Q-V                     |
 * | 填充  | =                         | =                       |
 *
 * @section implementation_details 实现细节
 * - **编码**：每 5 字节（40 位）输入编码为 8 个 Base32 字符输出
 * - **解码**：自动忽略空白字符，字母表不区分大小写
 * - **填充处理**：标准编码添加 `=` 填充至 8 的倍数；解码同时接受省略填充的输入
 * - **错误检测**：解码时检测非法字符与非法填充
 *
 * @note 解码时会自动忽略 RFC 4648 Section 3.2 中定义的空白字符。
 * @warning Base32 仅用于数据表示，不提供任何机密性或完整性保护。
 *
 * @see https://datatracker.ietf.org/doc/html/rfc4648
 * @{
 */

/**
 * @struct base32
 * @brief Base32编解码结构体
 *
 * 提供静态方法进行标准 Base32 与 base32hex 的编码和解码操作。
 */
struct NEFORCE_API base32 {
    /**
     * @brief 标准 Base32 编码（RFC 4648 §6）
     * @param data 要编码的二进制数据
     * @return 大写且带填充的 Base32 字符串
     */
    static string encode(cbyte_view data);

    /**
     * @brief 标准 Base32 解码（RFC 4648 §6）
     * @param data Base32 字符串，可不带填充
     * @return 解码后的二进制数据
     * @throws value_exception 当输入包含非法字符、非法长度或非法填充时抛出
     */
    static byte_vector decode(string_view data);

    /**
     * @brief 扩展十六进制 Base32 编码（RFC 4648 §7）
     * @param data 要编码的二进制数据
     * @return 大写且带填充的 base32hex 字符串
     */
    static string encode_hex(cbyte_view data);

    /**
     * @brief 扩展十六进制 Base32 解码（RFC 4648 §7）
     * @param data base32hex 字符串，可不带填充
     * @return 解码后的二进制数据
     * @throws value_exception 当输入包含非法字符、非法长度或非法填充时抛出
     */
    static byte_vector decode_hex(string_view data);
};

/**
 * @brief 标准 Base32 编码便捷函数（字节视图版本）
 * @param data 要编码的数据
 * @return 编码后的字符串
 */
NEFORCE_ALWAYS_INLINE_INLINE string base32_encode(const cbyte_view data) { return base32::encode(data); }

/**
 * @brief 标准 Base32 编码便捷函数（字节向量版本）
 * @param data 要编码的数据
 * @return 编码后的字符串
 */
NEFORCE_ALWAYS_INLINE_INLINE string base32_encode(const byte_vector& data) { return base32::encode(data.view()); }

/**
 * @brief 标准 Base32 编码便捷函数（字符串版本）
 * @param data 要编码的字符串
 * @return 编码后的字符串
 */
NEFORCE_ALWAYS_INLINE_INLINE string base32_encode(const string& data) {
    return base32::encode(cbyte_view{reinterpret_cast<const byte_t*>(data.data()), data.size()});
}

/**
 * @brief 标准 Base32 解码便捷函数
 * @param data Base32 字符串
 * @return 解码后的字符串
 * @throws value_exception 当输入非法时抛出
 */
NEFORCE_ALWAYS_INLINE_INLINE string base32_decode(const string_view data) {
    const byte_vector decoded = base32::decode(data);
    return string{reinterpret_cast<const char*>(decoded.data()), decoded.size()};
}

/**
 * @brief base32hex 编码便捷函数
 * @param data 要编码的数据
 * @return 编码后的字符串
 */
NEFORCE_ALWAYS_INLINE_INLINE string base32_hex_encode(const cbyte_view data) { return base32::encode_hex(data); }

/**
 * @brief base32hex 解码便捷函数
 * @param data base32hex 字符串
 * @return 解码后的二进制数据
 * @throws value_exception 当输入非法时抛出
 */
NEFORCE_ALWAYS_INLINE_INLINE byte_vector base32_hex_decode(const string_view data) { return base32::decode_hex(data); }

/** @} */ // Base32

/** @} */ // Encoding

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ENCODING_BASE32_HPP__
