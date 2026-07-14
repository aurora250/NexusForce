#ifndef NEFORCE_CORE_ENCRYPT_CHACHA20_POLY1305_HPP__
#define NEFORCE_CORE_ENCRYPT_CHACHA20_POLY1305_HPP__

/**
 * @file chacha20_poly1305.hpp
 * @brief ChaCha20-Poly1305 AEAD 认证加密算法实现
 *
 * 此文件提供了 ChaCha20-Poly1305 认证加密（AEAD）算法的实现，
 * 结合 ChaCha20 流密码和 Poly1305 一次性认证器提供机密性和完整性保护。
 */

#include "NeForce/core/container/vector.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Encryption 加密算法
 * @{
 */

/**
 * @defgroup ChaCha20Poly1305 ChaCha20-Poly1305
 * @brief ChaCha20-Poly1305 AEAD 认证加密算法实现
 *
 * @section standards 遵循的国际标准
 * 本实现严格遵循以下密码学与信息安全相关标准规范：
 *
 * **ChaCha20-Poly1305 AEAD 标准：**
 * - **IETF RFC 8439:2018**：ChaCha20 and Poly1305 for IETF Protocols
 *   https://www.rfc-editor.org/rfc/rfc8439.html
 *
 * **引用标准：**
 * - **IETF RFC 7539**：ChaCha20 and Poly1305 for IETF Protocols (原始版本，被 RFC 8439 取代)
 *   https://www.rfc-editor.org/rfc/rfc7539.html
 *
 * @section implementation_details 实现细节
 * | 特性              | 规范参数                                  |
 * |-------------------|-------------------------------------------|
 * | 算法              | ChaCha20-Poly1305 AEAD                    |
 * | 密钥长度          | 256 位（32 字节）                         |
 * | Nonce 长度        | 96 位（12 字节）                          |
 * | 认证标签长度      | 128 位（16 字节）                         |
 * | ChaCha20 轮数     | 20 轮（10 次双轮）                        |
 * | Poly1305 素数     | 2^130 − 5                                 |
 * | 认证模式          | Encrypt-then-MAC (EtM)                    |
 *
 * @note 本实现使用常量时间比较函数验证认证标签，防止时序攻击，
 *       符合 RFC 8439 的安全要求。
 *
 * @warning **Nonce 绝对不能对同一密钥重用！** Nonce 重用将完全破坏 Poly1305
 *          认证的安全性，导致密钥流恢复攻击。推荐使用随机生成的 nonce 或
 *          严格单调递增的计数器。对于随机 nonce，建议在约 2^48 次加密后
 *          更换密钥以限制碰撞概率。
 *
 * @see https://www.rfc-editor.org/rfc/rfc8439.html
 * @see https://ianix.com/pub/chacha-deployment.html
 * @{
 */

/**
 * @struct chacha20_poly1305
 * @brief ChaCha20-Poly1305 AEAD 加密算法结构体
 *
 * 提供静态方法进行 ChaCha20-Poly1305 认证加密和解密操作。
 * 支持关联数据（AAD）认证，适用于保护网络协议和文件格式中的
 * 未加密但需完整性保护的数据。
 */
struct NEFORCE_API chacha20_poly1305 {
    /**
     * @brief ChaCha20-Poly1305 AEAD 加密
     * @param data 要加密的明文数据（可为空）
     * @param key 32字节的密钥
     * @param nonce 12字节的nonce（不可对同一密钥重用）
     * @param aad 附加认证数据（可为空，仅认证不加密）
     * @param tag 输出认证标签缓冲区（16字节）
     * @return 加密后的密文（长度与明文相同）
     * @throws value_exception 当密钥长度不是32字节、nonce长度不是12字节或tag为null时抛出
     */
    static byte_vector encrypt(cbyte_view data, cbyte_view key, cbyte_view nonce, cbyte_view aad, byte_t* tag);

    /**
     * @brief ChaCha20-Poly1305 AEAD 解密
     * @param data 密文数据（可为空）
     * @param key 32字节密钥
     * @param nonce 12字节nonce
     * @param aad 附加认证数据（必须与加密时使用的一致）
     * @param tag 待验证的认证标签（16字节）
     * @return 解密后的明文（长度与密文相同）
     * @throws value_exception 当密钥长度不是32字节、nonce长度不是12字节、
     *                         tag长度不是16字节或认证失败时抛出
     */
    static byte_vector decrypt(cbyte_view data, cbyte_view key, cbyte_view nonce, cbyte_view aad, cbyte_view tag);
};

/** @} */ // ChaCha20Poly1305

/** @} */ // Encryption

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ENCRYPT_CHACHA20_POLY1305_HPP__
