#ifndef NEFORCE_CORE_HASH_SHA512_HPP__
#define NEFORCE_CORE_HASH_SHA512_HPP__

/**
 * @file sha512.hpp
 * @brief SHA-512哈希算法实现
 *
 * 此文件提供了SHA-512安全哈希算法的实现。SHA-512产生512位（64字节）的哈希值，
 * 是SHA-2家族中面向64位平台设计的成员，广泛应用于数字签名、证书验证、密钥派生与数据完整性保护。
 */

#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup HashCrypto
 * @{
 */

/**
 * @defgroup SHA512 SHA-512
 * @ingroup HashCrypto
 * @brief SHA-512安全哈希算法实现
 *
 * @section standards 遵循的国际标准
 * - **NIST FIPS PUB 180-4:2015**：安全哈希标准 (SHS)
 *   https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf
 * - **IETF RFC 6234**：US 安全哈希算法 (SHA 和 SHA 基 HMAC 及 HKDF)
 *   https://www.rfc-editor.org/rfc/rfc6234.html
 * - **ISO/IEC 10118-3:2018**：专用哈希函数
 *   https://www.iso.org/standard/67116.html
 *
 * @section implementation_details 实现细节
 * | 特性              | 规范参数                                  |
 * |-------------------|-------------------------------------------|
 * | 输出长度          | 512 位（64 字节）                         |
 * | 内部状态          | 8 个 64 位字（h0-h7）                     |
 * | 分组大小          | 1024 位（128 字节）                       |
 * | 轮数              | 80 轮                                     |
 * | 最大输入长度      | 2^128 - 1 位                              |
 * | 初始向量 (IV)     | FIPS 180-4 §5.3.5 定义                    |
 * | 填充方案          | Merkle–Damgård 强化（FIPS 180-4 §5.1.2）  |
 * | 字节序            | 大端（FIPS 180-4 §3.2.2）                 |
 *
 * @section usage 适用场景
 * - 数字签名（RSA-PSS、ECDSA、EdDSA）
 * - 证书链与代码签名验证
 * - HMAC-SHA512 与 HKDF-SHA512
 * - 64 位平台上的高速完整性校验
 *
 * @see https://csrc.nist.gov/projects/hash-functions
 * @see https://www.rfc-editor.org/rfc/rfc6234
 * @{
 */

/**
 * @struct SHA512
 * @brief SHA-512哈希算法结构体
 *
 * 提供静态方法计算SHA-512哈希值，支持返回字节数组或十六进制字符串。
 */
struct NEFORCE_API SHA512 {
    /**
     * @brief 计算SHA-512哈希值
     * @param data 输入数据
     * @return 64字节的哈希值
     */
    static byte_vector hash(cbyte_view data);

    /**
     * @brief 计算SHA-512哈希值的十六进制表示
     * @param data 输入数据
     * @return 128字符的小写十六进制字符串
     */
    static string hash_hex(cbyte_view data);
};

/**
 * @brief SHA-512哈希便捷函数（字符串视图版本）
 * @param data 输入字符串
 * @return 128字符的十六进制哈希值
 */
NEFORCE_ALWAYS_INLINE_INLINE string sha512(const string_view data) {
    return SHA512::hash_hex({reinterpret_cast<const byte_t*>(data.data()), data.size()});
}

/**
 * @brief SHA-512哈希便捷函数（字符串版本）
 * @param data 输入字符串
 * @return 128字符的十六进制哈希值
 */
NEFORCE_ALWAYS_INLINE_INLINE string sha512(const string& data) { return _NEFORCE sha512(data.view()); }

/**
 * @brief SHA-512哈希便捷函数（字节视图版本）
 * @param data 输入数据
 * @return 64字节的哈希值
 */
NEFORCE_ALWAYS_INLINE_INLINE byte_vector sha512(const cbyte_view data) { return SHA512::hash(data); }

/**
 * @brief SHA-512哈希便捷函数（字节向量版本）
 * @param data 输入数据
 * @return 64字节的哈希值
 */
NEFORCE_ALWAYS_INLINE_INLINE byte_vector sha512(const byte_vector& data) { return SHA512::hash(data.view()); }

/** @} */ // SHA512

/** @} */ // HashCrypto

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_HASH_SHA512_HPP__
