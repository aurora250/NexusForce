#ifndef NEFORCE_CORE_ENCRYPT_AES256_HPP__
#define NEFORCE_CORE_ENCRYPT_AES256_HPP__

/**
 * @file aes256.hpp
 * @brief AES-256加密算法实现
 *
 * 此文件提供了AES-256对称加密算法的实现，支持 ECB、CBC 和 GCM 工作模式，
 * 以及 PKCS#7 填充方案。
 */

#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Encryption 加密算法
 * @{
 */

/**
 * @defgroup AES256 AES-256
 * @brief AES-256对称加密算法实现
 *
 * @section standards 遵循的国际标准
 * 本实现严格遵循以下密码学与信息安全相关标准规范：
 *
 * **AES 算法标准：**
 * - **NIST FIPS PUB 197:2001**：高级加密标准 (AES)
 *   https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.197-upd1.pdf
 * - **ISO/IEC 18033-3:2010**：信息技术 — 安全技术 — 加密算法 — 第3部分：分组密码
 *   https://www.iso.org/standard/54531.html
 *
 * **工作模式标准：**
 * - **NIST SP 800-38A:2001**：分组密码工作模式推荐 (ECB、CBC)
 *   https://csrc.nist.gov/pubs/sp/800/38/a/final
 * - **NIST SP 800-38D:2007**：Galois/Counter Mode (GCM) 推荐
 *   https://csrc.nist.gov/pubs/sp/800/38/d/final
 * - **ISO/IEC 19772:2020**：认证加密 (包含 GCM 模式)
 *   https://www.iso.org/standard/75999.html
 *
 * **填充方案标准：**
 * - **IETF RFC 5652**：加密消息语法 (PKCS#7 填充)
 *   https://www.rfc-editor.org/rfc/rfc5652.html
 * - **IETF RFC 8018**：基于密码的加密规范 (PKCS#5 v2.1)
 *   https://www.rfc-editor.org/rfc/rfc8018.html
 *
 * @section implementation_details 实现细节
 * | 特性              | 规范参数                                  |
 * |-------------------|-------------------------------------------|
 * | 算法              | AES-256 (密钥长度 256 位，分组大小 128 位) |
 * | 轮数              | 14 轮                                     |
 * | 密钥扩展          | 240 字节扩展密钥                           |
 * | ECB 模式          | 电子密码本，无 IV                          |
 * | CBC 模式          | 密码分组链接，需 16 字节 IV                |
 * | GCM 模式          | Galois 计数器模式，认证加密                |
 * | GCM 标签长度      | 12-16 字节 (推荐 16 字节)                  |
 * | GCM IV 推荐长度   | 12 字节 (96 位)                            |
 * | 填充方案          | PKCS#7 (RFC 5652)                          |
 *
 * @note 本实现使用常量时间比较函数防止时序攻击，
 *       符合 NIST SP 800-38D 附录 C 的安全要求。
 *
 * @warning ECB 模式不应单独用于加密多个数据块，因其不能隐藏数据模式。
 *          推荐使用 CBC 或 GCM 模式进行安全加密。
 *
 * @see https://csrc.nist.gov/projects/cryptographic-standards-and-guidelines
 * @see https://www.iso.org/committee/45306.html
 * @{
 */

/**
 * @struct AES256
 * @brief AES-256加密算法结构体
 *
 * 提供静态方法进行AES-256加密和解密操作。
 * 支持 ECB / CBC / GCM 模式和PKCS7填充。
 *
 * TODO: 进行 AES-NI 优化
 */
struct NEFORCE_API AES256 {
    /**
     * @brief AES-256-ECB 加密
     * @param data 要加密的数据（长度必须是16的倍数）
     * @param key 32字节的密钥
     * @return 加密后的数据
     * @throws value_exception 当密钥长度不是32字节或数据长度不是16的倍数时抛出
     */
    static byte_vector encrypt_ecb(cbyte_view data, cbyte_view key);

    /**
     * @brief AES-256-ECB 解密
     * @param data 要解密的数据（长度必须是16的倍数）
     * @param key 32字节的密钥
     * @return 解密后的数据
     * @throws value_exception 当密钥长度不是32字节或数据长度不是16的倍数时抛出
     */
    static byte_vector decrypt_ecb(cbyte_view data, cbyte_view key);

    /**
     * @brief AES-256-ECB 加密（PKCS7填充）
     * @param data 要加密的数据
     * @param key 32字节的密钥
     * @return 加密后的数据（自动添加PKCS7填充）
     * @throws value_exception 当密钥长度不是32字节时抛出
     */
    static byte_vector encrypt_ecb_pkcs7(cbyte_view data, cbyte_view key);

    /**
     * @brief AES-256-ECB 解密（PKCS7填充）
     * @param data 要解密的数据
     * @param key 32字节的密钥
     * @return 解密后的数据（自动移除PKCS7填充）
     * @throws value_exception 当密钥长度不是32字节或填充无效时抛出
     */
    static byte_vector decrypt_ecb_pkcs7(cbyte_view data, cbyte_view key);

    /**
     * @brief AES-256-ECB 加密（十六进制接口）
     * @param data 要加密的字符串
     * @param key_hex 十六进制表示的密钥（64字符，表示32字节）
     * @return 加密后的十六进制字符串
     */
    static string encrypt_ecb_hex(string_view data, string_view key_hex);

    /**
     * @brief AES-256-ECB 解密（十六进制接口）
     * @param encrypted_hex 加密数据的十六进制表示
     * @param key_hex 十六进制表示的密钥（64字符，表示32字节）
     * @return 解密后的字符串
     */
    static string decrypt_ecb_hex(string_view encrypted_hex, string_view key_hex);

    /**
     * @brief AES-256-CBC 加密
     * @param data 要加密的数据（长度必须是16的倍数）
     * @param key 32字节密钥
     * @param iv 16字节初始化向量
     * @return 加密后的数据
     * @throws value_exception 参数长度不符时抛出
     */
    static byte_vector encrypt_cbc(cbyte_view data, cbyte_view key, cbyte_view iv);

    /**
     * @brief AES-256-CBC 解密
     * @param data 要解密的数据（长度必须是16的倍数）
     * @param key 32字节密钥
     * @param iv 16字节初始化向量
     * @return 解密后的数据
     * @throws value_exception 参数长度不符时抛出
     */
    static byte_vector decrypt_cbc(cbyte_view data, cbyte_view key, cbyte_view iv);

    /**
     * @brief AES-256-CBC 加密（PKCS7填充）
     * @param data 要加密的数据
     * @param key 32字节密钥
     * @param iv 16字节初始化向量
     * @return 加密后的数据
     */
    static byte_vector encrypt_cbc_pkcs7(cbyte_view data, cbyte_view key, cbyte_view iv);

    /**
     * @brief AES-256-CBC 解密（PKCS7填充）
     * @param data 要解密的数据
     * @param key 32字节密钥
     * @param iv 16字节初始化向量
     * @return 解密后的数据
     */
    static byte_vector decrypt_cbc_pkcs7(cbyte_view data, cbyte_view key, cbyte_view iv);

    /**
     * @brief AES-256-GCM 加密
     * @param data 要加密的数据
     * @param key 32字节密钥
     * @param iv 初始化向量（推荐12字节）
     * @param aad 附加认证数据（可为空）
     * @param tag 输出认证标签缓冲区
     * @param tag_len 标签长度（通常为12、13、14、15、16字节）
     * @return 加密后的密文（长度与明文相同）
     * @throws value_exception 参数无效时抛出
     */
    static byte_vector encrypt_gcm(cbyte_view data, cbyte_view key, cbyte_view iv, cbyte_view aad, byte_t* tag,
                                   size_t tag_len);

    /**
     * @brief AES-256-GCM 解密并验证标签
     * @param data 密文数据
     * @param key 32字节密钥
     * @param iv 初始化向量
     * @param aad 附加认证数据
     * @param tag 待验证的认证标签
     * @param tag_len 标签长度
     * @return 解密后的明文
     * @throws value_exception 解密失败或标签验证失败时抛出
     */
    static byte_vector decrypt_gcm(cbyte_view data, cbyte_view key, cbyte_view iv, cbyte_view aad, cbyte_view tag,
                                   size_t tag_len);
};


/**
 * @brief AES-256加密便捷函数
 * @param data 要加密的字符串
 * @param key_hex 十六进制密钥
 * @return 加密后的十六进制字符串
 */
NEFORCE_ALWAYS_INLINE_INLINE string aes256_encrypt(const string_view data, const string_view key_hex) {
    return AES256::encrypt_ecb_hex(data, key_hex);
}

NEFORCE_ALWAYS_INLINE_INLINE string aes256_encrypt(const string& data, const string_view key_hex) {
    return AES256::encrypt_ecb_hex(data.view(), key_hex);
}

NEFORCE_ALWAYS_INLINE_INLINE string aes256_encrypt(const string_view data, const string& key_hex) {
    return AES256::encrypt_ecb_hex(data, key_hex.view());
}

NEFORCE_ALWAYS_INLINE_INLINE string aes256_encrypt(const string& data, const string& key_hex) {
    return AES256::encrypt_ecb_hex(data.view(), key_hex.view());
}

/**
 * @brief AES-256解密便捷函数
 * @param encrypted_hex 加密数据的十六进制字符串
 * @param key_hex 十六进制密钥
 * @return 解密后的字符串
 */
NEFORCE_ALWAYS_INLINE_INLINE string aes256_decrypt(const string_view encrypted_hex, const string_view key_hex) {
    return AES256::decrypt_ecb_hex(encrypted_hex, key_hex);
}

NEFORCE_ALWAYS_INLINE_INLINE string aes256_decrypt(const string& encrypted_hex, const string_view key_hex) {
    return AES256::decrypt_ecb_hex(encrypted_hex.view(), key_hex);
}

NEFORCE_ALWAYS_INLINE_INLINE string aes256_decrypt(const string_view encrypted_hex, const string& key_hex) {
    return AES256::decrypt_ecb_hex(encrypted_hex, key_hex.view());
}

NEFORCE_ALWAYS_INLINE_INLINE string aes256_decrypt(const string& encrypted_hex, const string& key_hex) {
    return AES256::decrypt_ecb_hex(encrypted_hex.view(), key_hex.view());
}

/** @} */ // AES256

/** @} */ // Encryption

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ENCRYPT_AES256_HPP__
