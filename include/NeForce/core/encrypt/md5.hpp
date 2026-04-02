#ifndef NEFORCE_CORE_ENCRYPT_MD5_HPP__
#define NEFORCE_CORE_ENCRYPT_MD5_HPP__

/**
 * @file md5.hpp
 * @brief MD5哈希算法实现
 *
 * 此文件提供了MD5消息摘要算法的实现。
 * MD5产生128位（16字节）的哈希值，常用于校验和验证。
 *
 * @warning MD5已不再安全，不应用于密码存储等安全敏感场景。
 */

#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Encryption 加密算法
 * @{
 */

/**
 * @struct MD5
 * @brief MD5哈希算法结构体
 *
 * 提供静态方法计算MD5哈希值，支持返回字节数组或十六进制字符串。
 */
struct NEFORCE_API MD5 {
    /**
     * @brief 计算MD5哈希值
     * @param data 输入数据
     * @return 16字节的哈希值
     */
    static byte_vector hash(cbyte_view data);

    /**
     * @brief 计算MD5哈希值的十六进制表示
     * @param data 输入数据
     * @return 32字符的十六进制字符串
     */
    static string hash_hex(cbyte_view data);
};


/**
 * @brief MD5哈希便捷函数（字符串视图版本）
 * @param data 输入字符串
 * @return 32字符的十六进制哈希值
 */
NEFORCE_ALWAYS_INLINE_INLINE string md5(const string_view data) {
    return MD5::hash_hex({reinterpret_cast<const byte_t*>(data.data()), data.size()});
}

/**
 * @brief MD5哈希便捷函数（字符串版本）
 * @param data 输入字符串
 * @return 32字符的十六进制哈希值
 */
NEFORCE_ALWAYS_INLINE_INLINE string md5(const string& data) {
    return MD5::hash_hex({reinterpret_cast<const byte_t*>(data.data()), data.size()});
}

/**
 * @brief MD5哈希便捷函数（字节视图版本）
 * @param data 输入数据
 * @return 16字节的哈希值
 */
NEFORCE_ALWAYS_INLINE_INLINE byte_vector md5(const cbyte_view data) { return MD5::hash(data); }

/**
 * @brief MD5哈希便捷函数（字节向量版本）
 * @param data 输入数据
 * @return 16字节的哈希值
 */
NEFORCE_ALWAYS_INLINE_INLINE byte_vector md5(const byte_vector& data) { return MD5::hash(data.view()); }

/** @} */ // Encryption

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ENCRYPT_MD5_HPP__
