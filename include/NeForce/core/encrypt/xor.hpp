#ifndef NEFORCE_CORE_ENCRYPT_XOR_HPP__
#define NEFORCE_CORE_ENCRYPT_XOR_HPP__

/**
 * @file xor.hpp
 * @brief XOR异或加密实现
 *
 * 此文件提供了XOR异或加密算法的实现，支持字节级别的异或操作。
 * XOR加密是一种简单的对称加密算法，加解密使用相同操作。
 */

#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Encryption 加密算法
 * @brief 加密解密算法集合
 * @{
 */

/**
 * @struct XOR
 * @brief XOR异或加密结构体
 *
 * 提供静态方法进行XOR异或加密和解密操作。
 */
struct NEFORCE_API XOR {
    /**
     * @brief XOR加密
     * @param data 要加密的数据
     * @param key 加密密钥
     * @return 加密后的字节向量
     * @throws value_exception 当密钥为空时抛出
     *
     * 使用循环密钥对每个字节进行异或操作。
     */
    static byte_vector encrypt(cbyte_view data, cbyte_view key);

    /**
     * @brief XOR解密
     * @param data 要解密的数据
     * @param key 解密密钥
     * @return 解密后的字节向量
     * @throws value_exception 当密钥为空时抛出
     *
     * XOR加解密过程相同，直接调用encrypt方法。
     */
    static byte_vector decrypt(cbyte_view data, cbyte_view key) {
        return encrypt(data, key);
    }
};


/**
 * @brief XOR加密便捷函数（字节视图版本）
 * @param data 要加密的数据
 * @param key 加密密钥
 * @return 加密后的字节向量
 */
NEFORCE_ALWAYS_INLINE_INLINE byte_vector XOR_encrypt(const cbyte_view data, const cbyte_view key) {
    return XOR::encrypt(data, key);
}

/**
 * @brief XOR加密便捷函数（字节向量版本）
 * @param data 要加密的数据
 * @param key 加密密钥
 * @return 加密后的字节向量
 */
NEFORCE_ALWAYS_INLINE_INLINE byte_vector XOR_encrypt(const byte_vector& data, const byte_vector& key) {
    return XOR::encrypt(data.view(), key.view());
}

/**
 * @brief XOR加密便捷函数（字符串版本）
 * @param data 要加密的字符串
 * @param key 加密密钥
 * @return 加密后的字符串
 */
NEFORCE_ALWAYS_INLINE_INLINE string XOR_encrypt(const string& data, const string& key) {
    const byte_vector e = XOR_encrypt(
        cbyte_view{reinterpret_cast<const byte_t*>(data.data()), data.size()},
        cbyte_view{reinterpret_cast<const byte_t*>(key.data()), key.size()}
    );
    return string(e.begin(), e.end());
}


/**
 * @brief XOR解密便捷函数（字节视图版本）
 * @param data 要解密的数据
 * @param key 解密密钥
 * @return 解密后的字节向量
 */
NEFORCE_ALWAYS_INLINE_INLINE byte_vector XOR_decrypt(const cbyte_view data, const cbyte_view key) {
    return XOR::decrypt(data, key);
}

/**
 * @brief XOR解密便捷函数（字节向量版本）
 * @param data 要解密的数据
 * @param key 解密密钥
 * @return 解密后的字节向量
 */
NEFORCE_ALWAYS_INLINE_INLINE byte_vector XOR_decrypt(const byte_vector& data, const byte_vector& key) {
    return XOR::decrypt(data.view(), key.view());
}

/**
 * @brief XOR解密便捷函数（字符串版本）
 * @param data 要解密的字符串
 * @param key 解密密钥
 * @return 解密后的字符串
 */
NEFORCE_ALWAYS_INLINE_INLINE string XOR_decrypt(const string& data, const string& key) {
    const byte_vector d = XOR_decrypt(
        cbyte_view{reinterpret_cast<const byte_t*>(data.data()), data.size()},
        cbyte_view{reinterpret_cast<const byte_t*>(key.data()), key.size()}
    );
    return string(d.begin(), d.end());
}

/** @} */ // Encryption

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ENCRYPT_XOR_HPP__
