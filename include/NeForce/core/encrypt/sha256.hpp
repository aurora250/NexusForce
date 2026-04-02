#ifndef NEFORCE_CORE_ENCRYPT_SHA256_HPP__
#define NEFORCE_CORE_ENCRYPT_SHA256_HPP__

/**
 * @file sha256.hpp
 * @brief SHA-256哈希算法实现
 *
 * 此文件提供了SHA-256安全哈希算法的实现。
 * SHA-256产生256位（32字节）的哈希值，是SHA-2家族的一员。
 * 适用于密码存储、数字签名等安全敏感场景。
 */

#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Encryption 加密算法
 * @{
 */

/**
 * @struct SHA256
 * @brief SHA-256哈希算法结构体
 *
 * 提供静态方法计算SHA-256哈希值，支持返回字节数组或十六进制字符串。
 */
struct NEFORCE_API SHA256 {
    /**
     * @brief 计算SHA-256哈希值
     * @param data 输入数据
     * @return 32字节的哈希值
     */
    static byte_vector hash(cbyte_view data);

    /**
     * @brief 计算SHA-256哈希值的十六进制表示
     * @param data 输入数据
     * @return 64字符的十六进制字符串
     */
    static string hash_hex(cbyte_view data);
};


/**
 * @brief SHA-256哈希便捷函数（字符串视图版本）
 * @param data 输入字符串
 * @return 64字符的十六进制哈希值
 */
NEFORCE_ALWAYS_INLINE_INLINE string sha256(const string_view data) {
    return SHA256::hash_hex({reinterpret_cast<const byte_t*>(data.data()), data.size()});
}

/**
 * @brief SHA-256哈希便捷函数（字符串版本）
 * @param data 输入字符串
 * @return 64字符的十六进制哈希值
 */
NEFORCE_ALWAYS_INLINE_INLINE string sha256(const string& data) {
    return SHA256::hash_hex({reinterpret_cast<const byte_t*>(data.data()), data.size()});
}

/**
 * @brief SHA-256哈希便捷函数（字节视图版本）
 * @param data 输入数据
 * @return 32字节的哈希值
 */
NEFORCE_ALWAYS_INLINE_INLINE byte_vector sha256(const cbyte_view data) {
    return SHA256::hash(data);
}

/**
 * @brief SHA-256哈希便捷函数（字节向量版本）
 * @param data 输入数据
 * @return 32字节的哈希值
 */
NEFORCE_ALWAYS_INLINE_INLINE byte_vector sha256(const byte_vector& data) {
    return SHA256::hash(data.view());
}

/** @} */ // Encryption

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ENCRYPT_SHA256_HPP__
