#ifndef NEFORCE_CORE_KDF_HKDF_HPP__
#define NEFORCE_CORE_KDF_HKDF_HPP__

/**
 * @file hkdf.hpp
 * @brief HKDF 基于 HMAC 的密钥派生函数
 *
 * 此文件提供 RFC 5869 定义的 HKDF-Extract、HKDF-Expand 与组合派生接口，
 * 支持 HMAC-SHA256 与 HMAC-SHA512 两种底层哈希。
 */

#include "NeForce/core/container/vector.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup KDF 密钥派生
 * @brief 从初始密钥材料派生一个或多个密钥的算法集合。
 * @{
 */

/**
 * @defgroup HKDF HKDF
 * @ingroup KDF
 * @brief 基于 HMAC 的密钥派生函数
 *
 * @section standards 遵循的国际标准
 * - **IETF RFC 5869**：基于 HMAC 的提取-扩展密钥派生函数 (HKDF)
 *   https://www.rfc-editor.org/rfc/rfc5869.html
 * - **NIST SP 800-56C Rev. 2**：密钥派生方法
 *   https://csrc.nist.gov/pubs/sp/800/56/c/r2/final
 *
 * @section implementation_details 实现细节
 * | 阶段     | 公式                                             |
 * |----------|--------------------------------------------------|
 * | Extract  | `PRK = HMAC-Hash(salt, IKM)`                      |
 * | Expand   | `T(i) = HMAC-Hash(PRK, T(i-1) \|\| info \|\| i)`   |
 * | 输出     | `OKM = T(1) \|\| T(2) \|\| ...`                    |
 *
 * - `salt` 为空时按 RFC 5869 使用 HashLen 个零字节
 * - `info` 可为空，用于绑定派生结果的上下文
 * - 单次派生最大输出为 `255 × HashLen`
 *
 * @warning HKDF 不是口令哈希函数，不应用于存储用户口令；口令场景请使用 bcrypt 等慢哈希。
 * @{
 */

/**
 * @struct HKDF
 * @brief 基于 HMAC-SHA256 的密钥派生函数
 */
struct NEFORCE_API HKDF {
    /**
     * @brief HKDF-Extract：从输入密钥材料提取伪随机密钥
     * @param salt 盐值，可为空
     * @param ikm 输入密钥材料
     * @return 32 字节的伪随机密钥 PRK
     */
    static byte_vector extract(cbyte_view salt, cbyte_view ikm);

    /**
     * @brief HKDF-Expand：将伪随机密钥扩展为指定长度的输出
     * @param prk 伪随机密钥（通常来自 extract）
     * @param info 上下文与应用特定信息，可为空
     * @param length 期望的输出字节数
     * @return 派生出的密钥材料
     * @throws value_exception 当 length 超过 255 × 32 字节时抛出
     */
    static byte_vector expand(cbyte_view prk, cbyte_view info, size_t length);

    /**
     * @brief 组合执行 Extract 与 Expand
     * @param ikm 输入密钥材料
     * @param salt 盐值，可为空
     * @param info 上下文与应用特定信息，可为空
     * @param length 期望的输出字节数
     * @return 派生出的密钥材料
     * @throws value_exception 当 length 超过 255 × 32 字节时抛出
     */
    static byte_vector derive(cbyte_view ikm, cbyte_view salt, cbyte_view info, size_t length);
};

/**
 * @struct HKDF_SHA512
 * @brief 基于 HMAC-SHA512 的密钥派生函数
 */
struct NEFORCE_API HKDF_SHA512 {
    /**
     * @brief HKDF-Extract：从输入密钥材料提取伪随机密钥
     * @param salt 盐值，可为空
     * @param ikm 输入密钥材料
     * @return 64 字节的伪随机密钥 PRK
     */
    static byte_vector extract(cbyte_view salt, cbyte_view ikm);

    /**
     * @brief HKDF-Expand：将伪随机密钥扩展为指定长度的输出
     * @param prk 伪随机密钥（通常来自 extract）
     * @param info 上下文与应用特定信息，可为空
     * @param length 期望的输出字节数
     * @return 派生出的密钥材料
     * @throws value_exception 当 length 超过 255 × 64 字节时抛出
     */
    static byte_vector expand(cbyte_view prk, cbyte_view info, size_t length);

    /**
     * @brief 组合执行 Extract 与 Expand
     * @param ikm 输入密钥材料
     * @param salt 盐值，可为空
     * @param info 上下文与应用特定信息，可为空
     * @param length 期望的输出字节数
     * @return 派生出的密钥材料
     * @throws value_exception 当 length 超过 255 × 64 字节时抛出
     */
    static byte_vector derive(cbyte_view ikm, cbyte_view salt, cbyte_view info, size_t length);
};

/**
 * @brief HKDF-SHA256 派生便捷函数
 * @param ikm 输入密钥材料
 * @param salt 盐值，可为空
 * @param info 上下文信息，可为空
 * @param length 期望的输出字节数
 * @return 派生出的密钥材料
 * @throws value_exception 当 length 超过 255 × 32 字节时抛出
 */
NEFORCE_ALWAYS_INLINE_INLINE byte_vector hkdf_sha256(const cbyte_view ikm, const cbyte_view salt, const cbyte_view info,
                                                     const size_t length) {
    return HKDF::derive(ikm, salt, info, length);
}

/**
 * @brief HKDF-SHA512 派生便捷函数
 * @param ikm 输入密钥材料
 * @param salt 盐值，可为空
 * @param info 上下文信息，可为空
 * @param length 期望的输出字节数
 * @return 派生出的密钥材料
 * @throws value_exception 当 length 超过 255 × 64 字节时抛出
 */
NEFORCE_ALWAYS_INLINE_INLINE byte_vector hkdf_sha512(const cbyte_view ikm, const cbyte_view salt, const cbyte_view info,
                                                     const size_t length) {
    return HKDF_SHA512::derive(ikm, salt, info, length);
}

/** @} */ // HKDF

/** @} */ // KDF

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_KDF_HKDF_HPP__
