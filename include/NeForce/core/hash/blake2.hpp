#ifndef NEFORCE_CORE_HASH_BLAKE2_HPP__
#define NEFORCE_CORE_HASH_BLAKE2_HPP__

/**
 * @file blake2.hpp
 * @brief BLAKE2 哈希算法实现
 *
 * 此文件提供 RFC 7693 定义的 BLAKE2b 与 BLAKE2s 两种变体，
 * 支持可变长度输出与密钥化哈希。
 */

#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup HashCrypto
 * @{
 */

/**
 * @defgroup BLAKE2 BLAKE2
 * @ingroup HashCrypto
 * @brief BLAKE2b / BLAKE2s 哈希算法实现
 *
 * @section standards 遵循的国际标准
 * - **IETF RFC 7693**：BLAKE2 密码学哈希函数
 *   https://www.rfc-editor.org/rfc/rfc7693.html
 *
 * @section variants 变体对比
 * | 变体    | 字长 | 分组 | 轮数 | 最大输出 | 最大密钥 | 适用平台     |
 * |---------|------|------|------|----------|----------|--------------|
 * | BLAKE2b | 64 位 | 128 B | 12   | 64 B     | 64 B     | 64 位平台     |
 * | BLAKE2s | 32 位 | 64 B  | 10   | 32 B     | 32 B     | 32 位 / 嵌入式 |
 *
 * @section usage 适用场景
 * - 高速完整性校验与内容寻址
 * - 密钥化哈希（MAC 的轻量替代）
 * - Argon2 等口令哈希的内部原语
 * - 资源受限环境中的 BLAKE2s
 *
 * @note BLAKE2 单流压缩在轮内是串行的，无法通过 SIMD 获得有效并行；
 *       本实现采用标量优化路径。
 * @warning BLAKE2 不能抵御针对短输入的离线暴力破解，口令存储请使用 bcrypt 等慢哈希。
 * @{
 */

/**
 * @struct BLAKE2b
 * @brief 64 位 BLAKE2b 哈希算法
 */
struct NEFORCE_API BLAKE2b {
    /**
     * @brief 计算 BLAKE2b 哈希值
     * @param data 输入数据
     * @param digest_size 输出字节数，范围 1-64，默认 64
     * @return 指定长度的哈希值
     * @throws value_exception 当 digest_size 不在 1-64 范围内时抛出
     */
    static byte_vector hash(cbyte_view data, size_t digest_size = 64);

    /**
     * @brief 计算 BLAKE2b 哈希值的十六进制表示
     * @param data 输入数据
     * @param digest_size 输出字节数，范围 1-64，默认 64
     * @return 小写十六进制字符串
     * @throws value_exception 当 digest_size 不在 1-64 范围内时抛出
     */
    static string hash_hex(cbyte_view data, size_t digest_size = 64);

    /**
     * @brief 计算密钥化 BLAKE2b 哈希值
     * @param data 输入数据
     * @param key 密钥，长度 0-64
     * @param digest_size 输出字节数，范围 1-64，默认 64
     * @return 指定长度的哈希值
     * @throws value_exception 当密钥或 digest_size 超出范围时抛出
     */
    static byte_vector keyed_hash(cbyte_view data, cbyte_view key, size_t digest_size = 64);

    /**
     * @brief 计算密钥化 BLAKE2b 哈希值的十六进制表示
     * @param data 输入数据
     * @param key 密钥，长度 0-64
     * @param digest_size 输出字节数，范围 1-64，默认 64
     * @return 小写十六进制字符串
     * @throws value_exception 当密钥或 digest_size 超出范围时抛出
     */
    static string keyed_hash_hex(cbyte_view data, cbyte_view key, size_t digest_size = 64);
};

/**
 * @struct BLAKE2s
 * @brief 32 位 BLAKE2s 哈希算法
 */
struct NEFORCE_API BLAKE2s {
    /**
     * @brief 计算 BLAKE2s 哈希值
     * @param data 输入数据
     * @param digest_size 输出字节数，范围 1-32，默认 32
     * @return 指定长度的哈希值
     * @throws value_exception 当 digest_size 不在 1-32 范围内时抛出
     */
    static byte_vector hash(cbyte_view data, size_t digest_size = 32);

    /**
     * @brief 计算 BLAKE2s 哈希值的十六进制表示
     * @param data 输入数据
     * @param digest_size 输出字节数，范围 1-32，默认 32
     * @return 小写十六进制字符串
     * @throws value_exception 当 digest_size 不在 1-32 范围内时抛出
     */
    static string hash_hex(cbyte_view data, size_t digest_size = 32);

    /**
     * @brief 计算密钥化 BLAKE2s 哈希值
     * @param data 输入数据
     * @param key 密钥，长度 0-32
     * @param digest_size 输出字节数，范围 1-32，默认 32
     * @return 指定长度的哈希值
     * @throws value_exception 当密钥或 digest_size 超出范围时抛出
     */
    static byte_vector keyed_hash(cbyte_view data, cbyte_view key, size_t digest_size = 32);

    /**
     * @brief 计算密钥化 BLAKE2s 哈希值的十六进制表示
     * @param data 输入数据
     * @param key 密钥，长度 0-32
     * @param digest_size 输出字节数，范围 1-32，默认 32
     * @return 小写十六进制字符串
     * @throws value_exception 当密钥或 digest_size 超出范围时抛出
     */
    static string keyed_hash_hex(cbyte_view data, cbyte_view key, size_t digest_size = 32);
};

/**
 * @brief BLAKE2b 哈希便捷函数（字符串视图版本）
 * @param data 输入字符串
 * @param digest_size 输出字节数，默认 64
 * @return 小写十六进制哈希值
 * @throws value_exception 当 digest_size 非法时抛出
 */
NEFORCE_ALWAYS_INLINE_INLINE string blake2b(const string_view data, const size_t digest_size = 64) {
    return BLAKE2b::hash_hex({reinterpret_cast<const byte_t*>(data.data()), data.size()}, digest_size);
}

/**
 * @brief BLAKE2b 哈希便捷函数（字符串版本）
 * @param data 输入字符串
 * @param digest_size 输出字节数，默认 64
 * @return 小写十六进制哈希值
 * @throws value_exception 当 digest_size 非法时抛出
 */
NEFORCE_ALWAYS_INLINE_INLINE string blake2b(const string& data, const size_t digest_size = 64) {
    return _NEFORCE blake2b(data.view(), digest_size);
}

/**
 * @brief BLAKE2b 哈希便捷函数（字节视图版本）
 * @param data 输入数据
 * @param digest_size 输出字节数，默认 64
 * @return 哈希值
 * @throws value_exception 当 digest_size 非法时抛出
 */
NEFORCE_ALWAYS_INLINE_INLINE byte_vector blake2b(const cbyte_view data, const size_t digest_size = 64) {
    return BLAKE2b::hash(data, digest_size);
}

/**
 * @brief BLAKE2b 哈希便捷函数（字节向量版本）
 * @param data 输入数据
 * @param digest_size 输出字节数，默认 64
 * @return 哈希值
 * @throws value_exception 当 digest_size 非法时抛出
 */
NEFORCE_ALWAYS_INLINE_INLINE byte_vector blake2b(const byte_vector& data, const size_t digest_size = 64) {
    return BLAKE2b::hash(data.view(), digest_size);
}

/**
 * @brief BLAKE2b 密钥化哈希便捷函数
 * @param data 输入数据
 * @param key 密钥，长度 0-64
 * @param digest_size 输出字节数，默认 64
 * @return 哈希值
 * @throws value_exception 当密钥或 digest_size 非法时抛出
 */
NEFORCE_ALWAYS_INLINE_INLINE byte_vector blake2b_keyed(const cbyte_view data, const cbyte_view key,
                                                       const size_t digest_size = 64) {
    return BLAKE2b::keyed_hash(data, key, digest_size);
}

/**
 * @brief BLAKE2s 哈希便捷函数（字符串视图版本）
 * @param data 输入字符串
 * @param digest_size 输出字节数，默认 32
 * @return 小写十六进制哈希值
 * @throws value_exception 当 digest_size 非法时抛出
 */
NEFORCE_ALWAYS_INLINE_INLINE string blake2s(const string_view data, const size_t digest_size = 32) {
    return BLAKE2s::hash_hex({reinterpret_cast<const byte_t*>(data.data()), data.size()}, digest_size);
}

/**
 * @brief BLAKE2s 哈希便捷函数（字符串版本）
 * @param data 输入字符串
 * @param digest_size 输出字节数，默认 32
 * @return 小写十六进制哈希值
 * @throws value_exception 当 digest_size 非法时抛出
 */
NEFORCE_ALWAYS_INLINE_INLINE string blake2s(const string& data, const size_t digest_size = 32) {
    return _NEFORCE blake2s(data.view(), digest_size);
}

/**
 * @brief BLAKE2s 哈希便捷函数（字节视图版本）
 * @param data 输入数据
 * @param digest_size 输出字节数，默认 32
 * @return 哈希值
 * @throws value_exception 当 digest_size 非法时抛出
 */
NEFORCE_ALWAYS_INLINE_INLINE byte_vector blake2s(const cbyte_view data, const size_t digest_size = 32) {
    return BLAKE2s::hash(data, digest_size);
}

/**
 * @brief BLAKE2s 哈希便捷函数（字节向量版本）
 * @param data 输入数据
 * @param digest_size 输出字节数，默认 32
 * @return 哈希值
 * @throws value_exception 当 digest_size 非法时抛出
 */
NEFORCE_ALWAYS_INLINE_INLINE byte_vector blake2s(const byte_vector& data, const size_t digest_size = 32) {
    return BLAKE2s::hash(data.view(), digest_size);
}

/**
 * @brief BLAKE2s 密钥化哈希便捷函数
 * @param data 输入数据
 * @param key 密钥，长度 0-32
 * @param digest_size 输出字节数，默认 32
 * @return 哈希值
 * @throws value_exception 当密钥或 digest_size 非法时抛出
 */
NEFORCE_ALWAYS_INLINE_INLINE byte_vector blake2s_keyed(const cbyte_view data, const cbyte_view key,
                                                       const size_t digest_size = 32) {
    return BLAKE2s::keyed_hash(data, key, digest_size);
}

/** @} */ // BLAKE2

/** @} */ // HashCrypto

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_HASH_BLAKE2_HPP__
