#ifndef NEFORCE_CORE_ENCRYPT_MD5_HPP__
#define NEFORCE_CORE_ENCRYPT_MD5_HPP__

/**
 * @file md5.hpp
 * @brief MD5哈希算法实现
 *
 * 此文件提供了MD5消息摘要算法的实现。MD5产生128位（16字节）的哈希值，
 * 历史上广泛用于校验和验证与数据完整性检查。
 */

#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Encryption 加密算法
 * @{
 */

/**
 * @defgroup MD5 MD5
 * @brief MD5消息摘要算法的实现
 *
 * @warning **安全警告**：MD5算法已被证明存在密码学弱点，不再适合用于安全敏感场景。
 *          具体漏洞包括：
 *          - 碰撞攻击（2004年，王小云等）
 *          - 前缀碰撞攻击（2008年，Sotirov等）
 *          - 选择前缀碰撞攻击（2009年，Stevens等）
 * @warning 对于密码存储、数字签名、证书验证等场景，请使用 SHA-256 或 SHA-3 系列算法。
 *
 * @section standards 遵循的国际标准
 * 本实现严格遵循以下历史标准规范：
 *
 * **MD5 算法规范：**
 * - **IETF RFC 1321**：MD5 消息摘要算法
 *   https://www.rfc-editor.org/rfc/rfc1321.html
 *
 * **相关历史标准（信息参考）：**
 * - **IETF RFC 6151**：MD5 和 HMAC-MD5 安全考虑的更新
 *   https://www.rfc-editor.org/rfc/rfc6151.html
 *
 * @section implementation_details 实现细节
 * | 特性              | 规范参数                                  |
 * |-------------------|-------------------------------------------|
 * | 算法              | MD5 消息摘要                              |
 * | 输出长度          | 128 位（16 字节）                         |
 * | 内部状态          | 4 个 32 位字（A, B, C, D）               |
 * | 分组大小          | 512 位（64 字节）                         |
 * | 轮数              | 4 轮 × 16 步 = 64 步                     |
 * | 初始向量 (IV)     | RFC 1321 §3.3 定义                        |
 * | 填充方案          | Merkle–Damgård 强化（RFC 1321 §3.1）      |
 * | 小端字节序        | 是（RFC 1321 §3.4）                        |
 *
 * @section usage 适用场景
 * 尽管存在安全弱点，MD5 仍可用于以下非安全场景：
 * - 文件完整性校验
 * - 数据去重与内容寻址
 * - 非安全相关的哈希表实现
 * - 与历史系统兼容的数据交换
 *
 * @see https://www.rfc-editor.org/rfc/rfc1321
 * @see https://csrc.nist.gov/projects/hash-functions
 * @warning **禁止使用**：密码存储、数字签名、SSL/TLS 证书、代码签名、认证令牌等安全场景。
 * @{
 */

/**
 * @struct MD5
 * @brief MD5哈希算法结构体
 *
 * 提供静态方法计算MD5哈希值，支持返回字节数组或十六进制字符串。
 */
struct NEFORCE_API MD5 {
    /**
     * @brief 计算MD5哈希值
     * @param data 输入数据
     * @return 16字节的哈希值
     */
    static byte_vector hash(cbyte_view data);

    /**
     * @brief 计算MD5哈希值的十六进制表示
     * @param data 输入数据
     * @return 32字符的十六进制字符串
     */
    static string hash_hex(cbyte_view data);
};


/**
 * @brief MD5哈希便捷函数（字符串视图版本）
 * @param data 输入字符串
 * @return 32字符的十六进制哈希值
 */
NEFORCE_ALWAYS_INLINE_INLINE string md5(const string_view data) {
    return MD5::hash_hex({reinterpret_cast<const byte_t*>(data.data()), data.size()});
}

/**
 * @brief MD5哈希便捷函数（字符串版本）
 * @param data 输入字符串
 * @return 32字符的十六进制哈希值
 */
NEFORCE_ALWAYS_INLINE_INLINE string md5(const string& data) {
    return MD5::hash_hex({reinterpret_cast<const byte_t*>(data.data()), data.size()});
}

/**
 * @brief MD5哈希便捷函数（字节视图版本）
 * @param data 输入数据
 * @return 16字节的哈希值
 */
NEFORCE_ALWAYS_INLINE_INLINE byte_vector md5(const cbyte_view data) { return MD5::hash(data); }

/**
 * @brief MD5哈希便捷函数（字节向量版本）
 * @param data 输入数据
 * @return 16字节的哈希值
 */
NEFORCE_ALWAYS_INLINE_INLINE byte_vector md5(const byte_vector& data) { return MD5::hash(data.view()); }

/** @} */ // Encryption

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ENCRYPT_MD5_HPP__
