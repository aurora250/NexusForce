#ifndef NEFORCE_CORE_ENCRYPT_BASE64_HPP__
#define NEFORCE_CORE_ENCRYPT_BASE64_HPP__

/**
 * @file base64.hpp
 * @brief Base64编解码实现
 *
 * 此文件提供了Base64编码和解码功能的实现，支持标准 Base64 和 URL 安全 Base64 两种变体。
 * Base64用于将二进制数据转换为ASCII字符表示，常用于电子邮件附件、JSON Web Token、
 * 数据URI等场景的数据传输。
 */

#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Encryption 加密算法
 * @{
 */

/**
 * @defgroup Base64 Base64
 * @brief Base64编码和解码功能
 *
 * @section standards 遵循的国际标准
 * 本实现严格遵循以下互联网标准规范：
 *
 * **标准 Base64 编码：**
 * - **IETF RFC 4648 Section 4**：Base 64 编码
 *   https://www.rfc-editor.org/rfc/rfc4648.html#section-4
 *
 * **URL 安全 Base64 编码：**
 * - **IETF RFC 4648 Section 5**：Base 64 编码的 URL 和文件名安全字母表
 *   https://www.rfc-editor.org/rfc/rfc4648.html#section-5
 *
 * **MIME Base64 规范（历史参考）：**
 * - **IETF RFC 2045 Section 6.8**：MIME Part One — Base64 内容传输编码
 *   https://www.rfc-editor.org/rfc/rfc2045.html#section-6.8
 *
 * @section alphabet_comparison 字符表对比
 * | 索引  | 标准 Base64 (RFC 4648 §4) | URL 安全 Base64 (RFC 4648 §5) |
 * |-------|---------------------------|-------------------------------|
 * | 0-25  | A-Z                       | A-Z                           |
 * | 26-51 | a-z                       | a-z                           |
 * | 52-61 | 0-9                       | 0-9                           |
 * | 62    | +                         | - (连字符)                     |
 * | 63    | /                         | _ (下划线)                     |
 * | 填充  | =                         | = (可选)                       |
 *
 * @section implementation_details 实现细节
 * - **编码**：每 3 字节（24 位）输入编码为 4 个 Base64 字符输出
 * - **解码**：自动忽略空白字符（空格、制表符、换行符等）
 * - **填充处理**：严格遵循 RFC 4648 的填充规则
 *   - 输入长度非 3 的倍数时，标准编码添加 `=` 填充至 4 的倍数
 *   - URL 安全编码可选择省略填充符
 * - **错误检测**：解码时检测非法字符和无效填充模式
 *
 * @note 解码时会自动忽略 RFC 4648 Section 3.2 中定义的空白字符：
 *       空格 (SP)、制表符 (HT)、换行 (LF)、回车 (CR)、换页 (FF)、垂直制表 (VT)
 *
 * @warning 标准 Base64 中的 `+` 和 `/` 字符在 URL 中有特殊含义，
 *          在 URL 参数或文件名中使用时应使用 URL 安全版本。
 *
 * @see https://datatracker.ietf.org/doc/html/rfc4648
 * @see https://en.wikipedia.org/wiki/Base64
 * @{
 */

/**
 * @struct base64
 * @brief Base64编解码结构体
 *
 * 提供静态方法进行Base64编码和解码操作。
 * 编码将二进制数据转换为文本格式，解码执行逆操作。
 */
struct NEFORCE_API base64 {
    /**
     * @brief Base64编码
     * @param data 要编码的二进制数据
     * @return 编码后的Base64字符串
     */
    static string encode(cbyte_view data);

    /**
     * @brief Base64解码
     * @param data Base64编码的字符串
     * @return 解码后的二进制数据
     * @throws value_exception 当输入包含非法字符时抛出
     */
    static byte_vector decode(string_view data);

    /**
     * @brief URL安全的Base64编码
     * @param data 要编码的二进制数据
     * @param padding 是否添加填充符'='（默认不填充）
     * @return URL安全的Base64字符串
     */
    static string encode_url(cbyte_view data, bool padding = false);

    /**
     * @brief URL安全的Base64解码
     * @param data URL安全的Base64字符串
     * @return 解码后的二进制数据
     * @throws value_exception 当输入包含非法字符时抛出
     */
    static byte_vector decode_url(string_view data);
};


/**
 * @brief Base64编码便捷函数（字节视图版本）
 * @param data 要编码的数据
 * @return 编码后的字符串
 */
NEFORCE_ALWAYS_INLINE_INLINE string base64_encode(const cbyte_view data) {
    if (data.empty()) {
        return {};
    }
    return base64::encode(data);
}

/**
 * @brief Base64编码便捷函数（字节向量版本）
 * @param data 要编码的数据
 * @return 编码后的字符串
 */
NEFORCE_ALWAYS_INLINE_INLINE string base64_encode(const byte_vector& data) {
    if (data.empty()) {
        return {};
    }
    return base64::encode(data.view());
}

/**
 * @brief Base64编码便捷函数（字符串版本）
 * @param data 要编码的字符串
 * @return 编码后的字符串
 */
NEFORCE_ALWAYS_INLINE_INLINE string base64_encode(const string& data) {
    if (data.empty()) {
        return {};
    }
    return base64::encode(cbyte_view{reinterpret_cast<const byte_t*>(data.data()), data.size()});
}


/**
 * @brief Base64解码便捷函数（字符串视图版本）
 * @param data Base64编码的字符串
 * @return 解码后的字符串
 */
NEFORCE_ALWAYS_INLINE_INLINE string base64_decode(const string_view data) {
    if (data.empty()) {
        return {};
    }
    const byte_vector d = base64::decode(data);
    return string{reinterpret_cast<const char*>(d.data()), d.size()};
}

/**
 * @brief Base64解码便捷函数（字节向量版本）
 * @param data Base64编码的字节向量
 * @return 解码后的字节向量
 */
NEFORCE_ALWAYS_INLINE_INLINE byte_vector base64_decode(const byte_vector& data) {
    if (data.empty()) {
        return {};
    }
    const string_view view{reinterpret_cast<const char*>(data.data()), data.size()};
    return base64::decode(view);
}

/**
 * @brief Base64解码便捷函数（字符串版本）
 * @param data Base64编码的字符串
 * @return 解码后的字符串
 */
NEFORCE_ALWAYS_INLINE_INLINE string base64_decode(const string& data) {
    if (data.empty()) {
        return "";
    }
    return base64_decode(data.view());
}

/** @} */ // Base64

/** @} */ // Encryption

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ENCRYPT_BASE64_HPP__
