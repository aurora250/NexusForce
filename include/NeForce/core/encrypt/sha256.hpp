#ifndef NEFORCE_CORE_ENCRYPT_SHA256_HPP__
#define NEFORCE_CORE_ENCRYPT_SHA256_HPP__

/**
 * @file sha256.hpp
 * @brief SHA-256哈希算法实现
 *
 * 此文件提供了SHA-256安全哈希算法的实现。SHA-256产生256位（32字节）的哈希值，
 * 是SHA-2家族的一员，广泛应用于密码存储、数字签名、区块链、证书验证和数据完整性保护。
 */

#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Encryption 加密算法
 * @{
 */

/**
 * @defgroup SHA256 SHA-256
 * @brief SHA-256安全哈希算法实现。
 *
 * @section standards 遵循的国际标准
 * 本实现严格遵循以下标准规范：
 *
 * **SHA-2 算法规范：**
 * - **NIST FIPS PUB 180-4:2015**：安全哈希标准 (SHS)
 *   https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf
 * - **IETF RFC 6234**：US 安全哈希算法 (SHA 和 SHA 基 HMAC 及 HKDF)
 *   https://www.rfc-editor.org/rfc/rfc6234.html
 *
 * **相关国际标准：**
 * - **ISO/IEC 10118-3:2018**：信息技术 — 安全技术 — 哈希函数 — 第3部分：专用哈希函数
 *   https://www.iso.org/standard/67116.html
 *
 * **密码模块验证标准：**
 * - **NIST FIPS PUB 140-3:2019**：密码模块安全要求
 *   https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.140-3.pdf
 * - **ISO/IEC 19790:2012**：信息技术 — 安全技术 — 密码模块安全要求
 *   https://www.iso.org/standard/52906.html
 *
 * @section implementation_details 实现细节
 * | 特性              | 规范参数                                  |
 * |-------------------|-------------------------------------------|
 * | 算法              | SHA-256（SHA-2 家族成员）                 |
 * | 输出长度          | 256 位（32 字节）                         |
 * | 内部状态          | 8 个 32 位字（h0-h7）                     |
 * | 分组大小          | 512 位（64 字节）                         |
 * | 轮数              | 64 轮                                     |
 * | 最大输入长度      | 2^64 - 1 位                               |
 * | 初始向量 (IV)     | FIPS 180-4 §5.3.3 定义                    |
 * | 填充方案          | Merkle–Damgård 强化（FIPS 180-4 §5.1.1）  |
 * | 字节序            | 大端（FIPS 180-4 §3.2.2）                  |
 * | 安全强度          | 碰撞抗性 128 位，原像抗性 256 位           |
 *
 * @section comparison SHA-2 家族对比
 * | 算法      | 输出长度 | 内部状态字 | 轮数 | 安全强度（碰撞） |
 * |-----------|----------|------------|------|------------------|
 * | SHA-224   | 224 位   | 8 × 32 位  | 64   | 112 位           |
 * | **SHA-256** | **256 位** | **8 × 32 位** | **64** | **128 位**     |
 * | SHA-384   | 384 位   | 8 × 64 位  | 80   | 192 位           |
 * | SHA-512   | 512 位   | 8 × 64 位  | 80   | 256 位           |
 * | SHA-512/256 | 256 位 | 8 × 64 位  | 80   | 128 位           |
 *
 * @section usage 适用场景
 * SHA-256 是目前广泛推荐的安全哈希算法，适用于：
 * - 密码存储（配合盐值和密钥派生函数）
 * - 数字签名（RSA、ECDSA、EdDSA）
 * - 证书验证（X.509 证书链）
 * - 区块链技术（比特币、以太坊的 PoW 和工作量证明）
 * - 数据完整性校验（HMAC-SHA256）
 * - 密钥派生函数（HKDF、PBKDF2）
 * - 确定性随机数生成器（DRBG）
 *
 * @see https://csrc.nist.gov/projects/hash-functions
 * @see https://www.rfc-editor.org/rfc/rfc6234
 * @see https://csrc.nist.gov/projects/cryptographic-module-validation-program
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
NEFORCE_ALWAYS_INLINE_INLINE byte_vector sha256(const cbyte_view data) { return SHA256::hash(data); }

/**
 * @brief SHA-256哈希便捷函数（字节向量版本）
 * @param data 输入数据
 * @return 32字节的哈希值
 */
NEFORCE_ALWAYS_INLINE_INLINE byte_vector sha256(const byte_vector& data) { return SHA256::hash(data.view()); }

/** @} */ // Encryption

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ENCRYPT_SHA256_HPP__
