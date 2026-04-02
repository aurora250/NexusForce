#ifndef NEFORCE_CORE_ENCRYPT_AES256_HPP__
#define NEFORCE_CORE_ENCRYPT_AES256_HPP__

/**
 * @file aes256.hpp
 * @brief AES-256加密算法实现
 *
 * 此文件提供了AES-256对称加密算法的实现。
 * 支持ECB模式（电子密码本模式）的加密和解密，并支持PKCS7填充。
 * AES-256使用256位（32字节）密钥，是当前广泛使用的安全加密标准。
 */

#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Encryption 加密算法
 * @{
 */

/**
 * @struct AES256
 * @brief AES-256加密算法结构体
 *
 * 提供静态方法进行AES-256加密和解密操作。
 * 支持ECB模式和PKCS7填充。
 *
 * @note 当前实现使用ECB模式，对于重复数据的块可能产生相同密文。
 *       生产环境建议使用CBC或GCM等更安全的模式。
 */
struct NEFORCE_API AES256 {
    /**
     * @brief AES-256加密（无填充）
     * @param data 要加密的数据（长度必须是16的倍数）
     * @param key 32字节的密钥
     * @return 加密后的数据
     * @throws value_exception 当密钥长度不是32字节或数据长度不是16的倍数时抛出
     */
    static byte_vector encrypt(cbyte_view data, cbyte_view key);

    /**
     * @brief AES-256解密（无填充）
     * @param data 要解密的数据（长度必须是16的倍数）
     * @param key 32字节的密钥
     * @return 解密后的数据
     * @throws value_exception 当密钥长度不是32字节或数据长度不是16的倍数时抛出
     */
    static byte_vector decrypt(cbyte_view data, cbyte_view key);

    /**
     * @brief AES-256加密（PKCS7填充）
     * @param data 要加密的数据
     * @param key 32字节的密钥
     * @return 加密后的数据（自动添加PKCS7填充）
     * @throws value_exception 当密钥长度不是32字节时抛出
     */
    static byte_vector encrypt_pkcs7(cbyte_view data, cbyte_view key);

    /**
     * @brief AES-256解密（PKCS7填充）
     * @param data 要解密的数据
     * @param key 32字节的密钥
     * @return 解密后的数据（自动移除PKCS7填充）
     * @throws value_exception 当密钥长度不是32字节或填充无效时抛出
     */
    static byte_vector decrypt_pkcs7(cbyte_view data, cbyte_view key);

    /**
     * @brief AES-256加密（十六进制接口）
     * @param data 要加密的字符串（UTF-8）
     * @param key_hex 十六进制表示的密钥（64字符，表示32字节）
     * @return 加密后的十六进制字符串
     */
    static string encrypt_hex(string_view data, string_view key_hex);

    /**
     * @brief AES-256解密（十六进制接口）
     * @param encrypted_hex 加密数据的十六进制表示
     * @param key_hex 十六进制表示的密钥（64字符，表示32字节）
     * @return 解密后的字符串（UTF-8）
     */
    static string decrypt_hex(string_view encrypted_hex, string_view key_hex);
};


/**
 * @name AES-256加密便捷函数（十六进制接口）
 * @{
 */

/**
 * @brief AES-256加密便捷函数
 * @param data 要加密的字符串
 * @param key_hex 十六进制密钥
 * @return 加密后的十六进制字符串
 */
NEFORCE_ALWAYS_INLINE_INLINE string aes256_encrypt(const string_view data, const string_view key_hex) {
    return AES256::encrypt_hex(data, key_hex);
}

NEFORCE_ALWAYS_INLINE_INLINE string aes256_encrypt(const string& data, const string_view key_hex) {
    return AES256::encrypt_hex(data.view(), key_hex);
}

NEFORCE_ALWAYS_INLINE_INLINE string aes256_encrypt(const string_view data, const string& key_hex) {
    return AES256::encrypt_hex(data, key_hex.view());
}

NEFORCE_ALWAYS_INLINE_INLINE string aes256_encrypt(const string& data, const string& key_hex) {
    return AES256::encrypt_hex(data.view(), key_hex.view());
}

/** @} */

/**
 * @name AES-256解密便捷函数（十六进制接口）
 * @{
 */

/**
 * @brief AES-256解密便捷函数
 * @param encrypted_hex 加密数据的十六进制字符串
 * @param key_hex 十六进制密钥
 * @return 解密后的字符串
 */
NEFORCE_ALWAYS_INLINE_INLINE string aes256_decrypt(const string_view encrypted_hex, const string_view key_hex) {
    return AES256::decrypt_hex(encrypted_hex, key_hex);
}

NEFORCE_ALWAYS_INLINE_INLINE string aes256_decrypt(const string& encrypted_hex, const string_view key_hex) {
    return AES256::decrypt_hex(encrypted_hex.view(), key_hex);
}

NEFORCE_ALWAYS_INLINE_INLINE string aes256_decrypt(const string_view encrypted_hex, const string& key_hex) {
    return AES256::decrypt_hex(encrypted_hex, key_hex.view());
}

NEFORCE_ALWAYS_INLINE_INLINE string aes256_decrypt(const string& encrypted_hex, const string& key_hex) {
    return AES256::decrypt_hex(encrypted_hex.view(), key_hex.view());
}

/** @} */

/** @} */ // Encryption

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ENCRYPT_AES256_HPP__
