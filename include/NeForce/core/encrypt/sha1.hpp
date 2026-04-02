#ifndef NEFORCE_CORE_ENCRYPT_SHA1_HPP__
#define NEFORCE_CORE_ENCRYPT_SHA1_HPP__

/**
 * @file sha1.hpp
 * @brief SHA-1哈希算法实现
 *
 * 此文件提供了SHA-1安全哈希算法的实现。
 * SHA-1产生160位（20字节）的哈希值。
 */

#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Encryption 加密算法
 * @{
 */

/**
 * @struct SHA1
 * @brief SHA-1哈希算法结构体
 *
 * 提供静态方法计算SHA-1哈希值，支持返回字节数组或十六进制字符串。
 *
 * @warning SHA-1不安全，不应用于安全敏感场景。
 */
struct NEFORCE_API SHA1 {
    /**
     * @brief 计算SHA-1哈希值
     * @param data 输入数据
     * @return 20字节的哈希值
     */
    static byte_vector hash(cbyte_view data);

    /**
     * @brief 计算SHA-1哈希值的十六进制表示
     * @param data 输入数据
     * @return 40字符的十六进制字符串
     */
    static string hash_hex(cbyte_view data);
};


/**
 * @brief SHA-1哈希便捷函数（字符串视图版本）
 * @param data 输入字符串
 * @return 20字节的哈希值（原始字节）
 * @note 返回原始字节数据而非十六进制字符串
 */
NEFORCE_ALWAYS_INLINE_INLINE string sha1(const string_view data) {
    const byte_vector h = SHA1::hash({reinterpret_cast<const byte_t*>(data.data()), data.size()});
    return string{reinterpret_cast<const char*>(h.data()), h.size()};
}

/**
 * @brief SHA-1哈希便捷函数（字符串版本）
 * @param data 输入字符串
 * @return 20字节的哈希值（原始字节）
 */
NEFORCE_ALWAYS_INLINE_INLINE string sha1(const string& data) { return _NEFORCE sha1(data.view()); }

/**
 * @brief SHA-1哈希便捷函数（字节视图版本）
 * @param data 输入数据
 * @return 20字节的哈希值
 */
NEFORCE_ALWAYS_INLINE_INLINE byte_vector sha1(const cbyte_view data) { return SHA1::hash(data); }

/**
 * @brief SHA-1哈希便捷函数（字节向量版本）
 * @param data 输入数据
 * @return 20字节的哈希值
 */
NEFORCE_ALWAYS_INLINE_INLINE byte_vector sha1(const byte_vector& data) { return SHA1::hash(data.view()); }

/** @} */ // Encryption

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ENCRYPT_SHA1_HPP__
