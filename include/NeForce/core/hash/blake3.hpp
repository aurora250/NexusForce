#ifndef NEFORCE_CORE_HASH_BLAKE3_HPP__
#define NEFORCE_CORE_HASH_BLAKE3_HPP__

/**
 * @file blake3.hpp
 * @brief BLAKE3 哈希算法实现
 *
 * 此文件提供 BLAKE3 密码学哈希的实现，支持 256 位固定输出、可扩展输出（XOF）、密钥化哈希与密钥派生模式。
 */

#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup HashCrypto
 * @{
 */

/**
 * @defgroup BLAKE3 BLAKE3
 * @ingroup HashCrypto
 * @brief BLAKE3 密码学哈希与可扩展输出函数
 *
 * @section standards 遵循的规范
 * - **BLAKE3 官方规范**：BLAKE3 密码学哈希函数
 *   https://github.com/BLAKE3-team/BLAKE3-specs/blob/master/blake3.pdf
 *
 * @section implementation_details 实现细节
 * | 特性          | 参数                                             |
 * |---------------|--------------------------------------------------|
 * | 内部状态      | 8 × 32 位链值 + 16 × 32 位压缩状态                |
 * | 分块大小      | 1024 字节（16 个 64 字节压缩块）                 |
 * | 压缩轮数      | 7 轮                                             |
 * | 默认输出      | 32 字节；可通过 XOF 扩展任意长度                  |
 * | 树结构        | 二叉 Merkle 树，左子树取小于分块数的最大 2 的幂   |
 * | 模式标志      | CHUNK_START / CHUNK_END / PARENT / ROOT / KEYED / DERIVE_KEY |
 *
 * @section simd SIMD 加速
 * - 当编译器启用 AVX2 时，8 个分块并行压缩（结构体数组布局、每寄存器 8 条 lane）
 * - 不足 8 个完整分块或非 AVX2 平台回退到标量路径
 * - 两条路径输出完全一致，单元测试逐长度交叉验证
 *
 * @section usage 适用场景
 * - 高速内容寻址与文件完整性校验
 * - 密钥化消息认证（替代 HMAC 的轻量方案）
 * - 从高熵材料派生密钥（derive_key 模式）
 * - 可扩展输出（KDF、掩码生成）
 *
 * @warning BLAKE3 不是口令哈希函数，不应用于存储用户口令。
 * @{
 */

/**
 * @struct BLAKE3
 * @brief BLAKE3 哈希算法结构体
 */
struct NEFORCE_API BLAKE3 {
    /**
     * @brief 计算 32 字节 BLAKE3 哈希值
     * @param data 输入数据
     * @return 32 字节的哈希值
     */
    static byte_vector hash(cbyte_view data);

    /**
     * @brief 计算 BLAKE3 哈希值的十六进制表示
     * @param data 输入数据
     * @return 64 字符的小写十六进制字符串
     */
    static string hash_hex(cbyte_view data);

    /**
     * @brief 计算可扩展输出
     * @param data 输入数据
     * @param length 期望的输出字节数
     * @return 指定长度的输出
     */
    static byte_vector hash_xof(cbyte_view data, size_t length);

    /**
     * @brief 密钥化 BLAKE3 哈希
     * @param key 32 字节密钥
     * @param data 输入数据
     * @return 32 字节的哈希值
     * @throws value_exception 当密钥长度不是 32 字节时抛出
     */
    static byte_vector keyed_hash(cbyte_view key, cbyte_view data);

    /**
     * @brief BLAKE3 密钥派生模式
     * @param context 应用上下文，用于域分离
     * @param key_material 高熵密钥材料
     * @param length 期望的输出字节数，默认 32
     * @return 派生出的密钥材料
     */
    static byte_vector derive_key(cbyte_view context, cbyte_view key_material, size_t length = 32);
};

/**
 * @brief BLAKE3 哈希便捷函数（字符串视图版本）
 * @param data 输入字符串
 * @return 64 字符的十六进制哈希值
 */
NEFORCE_ALWAYS_INLINE_INLINE string blake3(const string_view data) {
    return BLAKE3::hash_hex({reinterpret_cast<const byte_t*>(data.data()), data.size()});
}

/**
 * @brief BLAKE3 哈希便捷函数（字符串版本）
 * @param data 输入字符串
 * @return 64 字符的十六进制哈希值
 */
NEFORCE_ALWAYS_INLINE_INLINE string blake3(const string& data) { return _NEFORCE blake3(data.view()); }

/**
 * @brief BLAKE3 哈希便捷函数（字节视图版本）
 * @param data 输入数据
 * @return 32 字节的哈希值
 */
NEFORCE_ALWAYS_INLINE_INLINE byte_vector blake3(const cbyte_view data) { return BLAKE3::hash(data); }

/**
 * @brief BLAKE3 哈希便捷函数（字节向量版本）
 * @param data 输入数据
 * @return 32 字节的哈希值
 */
NEFORCE_ALWAYS_INLINE_INLINE byte_vector blake3(const byte_vector& data) { return BLAKE3::hash(data.view()); }

/**
 * @brief BLAKE3 密钥化哈希便捷函数
 * @param key 32 字节密钥
 * @param data 输入数据
 * @return 32 字节的哈希值
 * @throws value_exception 当密钥长度不是 32 字节时抛出
 */
NEFORCE_ALWAYS_INLINE_INLINE byte_vector blake3_keyed(const cbyte_view key, const cbyte_view data) {
    return BLAKE3::keyed_hash(key, data);
}

/**
 * @brief BLAKE3 密钥派生便捷函数
 * @param context 应用上下文
 * @param key_material 高熵密钥材料
 * @param length 期望的输出字节数，默认 32
 * @return 派生出的密钥材料
 */
NEFORCE_ALWAYS_INLINE_INLINE byte_vector blake3_derive_key(const cbyte_view context, const cbyte_view key_material,
                                                           const size_t length = 32) {
    return BLAKE3::derive_key(context, key_material, length);
}

/** @} */ // BLAKE3

/** @} */ // HashCrypto

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_HASH_BLAKE3_HPP__
