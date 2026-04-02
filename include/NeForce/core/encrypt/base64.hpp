#ifndef NEFORCE_CORE_ENCRYPT_BASE64_HPP__
#define NEFORCE_CORE_ENCRYPT_BASE64_HPP__

/**
 * @file base64.hpp
 * @brief Base64编解码实现
 *
 * 此文件提供了Base64编码和解码功能的实现。
 * Base64用于将二进制数据转换为ASCII字符表示，常用于数据传输。
 */

#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Encryption 加密算法
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
};


/**
 * @brief Base64编码便捷函数（字节视图版本）
 * @param data 要编码的数据
 * @return 编码后的字符串
 */
NEFORCE_ALWAYS_INLINE_INLINE string base64_encode(const cbyte_view data) {
    return base64::encode(data);
}

/**
 * @brief Base64编码便捷函数（字节向量版本）
 * @param data 要编码的数据
 * @return 编码后的字符串
 */
NEFORCE_ALWAYS_INLINE_INLINE string base64_encode(const byte_vector& data) {
    return base64::encode(data.view());
}

/**
 * @brief Base64编码便捷函数（字符串版本）
 * @param data 要编码的字符串
 * @return 编码后的字符串
 */
NEFORCE_ALWAYS_INLINE_INLINE string base64_encode(const string& data) {
    return base64::encode(cbyte_view{
        reinterpret_cast<const byte_t*>(data.data()), data.size()
    });
}


/**
 * @brief Base64解码便捷函数（字符串视图版本）
 * @param data Base64编码的字符串
 * @return 解码后的字符串
 */
NEFORCE_ALWAYS_INLINE_INLINE string base64_decode(const string_view data) {
    const byte_vector d = base64::decode(data);
    return string{reinterpret_cast<const char*>(d.data()), d.size()};
}

/**
 * @brief Base64解码便捷函数（字节向量版本）
 * @param data Base64编码的字节向量
 * @return 解码后的字节向量
 */
NEFORCE_ALWAYS_INLINE_INLINE byte_vector base64_decode(const byte_vector& data) {
    const string tmp{reinterpret_cast<const char*>(data.data()), data.size()};
    return base64::decode(tmp.view());
}

/**
 * @brief Base64解码便捷函数（字符串版本）
 * @param data Base64编码的字符串
 * @return 解码后的字符串
 */
NEFORCE_ALWAYS_INLINE_INLINE string base64_decode(const string& data) {
    return base64_decode(data.view());
}

/** @} */ // Encryption

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ENCRYPT_BASE64_HPP__
