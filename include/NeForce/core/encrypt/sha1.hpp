#ifndef NEFORCE_CORE_ENCRYPT_SHA1_HPP__
#define NEFORCE_CORE_ENCRYPT_SHA1_HPP__

/**
 * @file sha1.hpp
 * @brief SHA-1哈希算法实现
 *
 * 此文件提供了SHA-1安全哈希算法的实现。SHA-1产生160位（20字节）的哈希值，
 * 历史上广泛用于数字签名、证书验证和数据完整性检查。
 *
 * @warning **安全警告**：SHA-1算法已被证明存在密码学弱点，不再适合用于安全敏感场景。
 *          具体漏洞包括：
 *          - 理论攻击（2005年，王小云等，复杂度 2^69）
 *          - 碰撞攻击（2017年，Google 与 CWI，SHAttered 攻击）
 *          - 选择前缀碰撞攻击（2020年，Gaëtan Leurent 与 Thomas Peyrin，复杂度 2^63.4）
 * @warning 对于密码存储、数字签名、证书验证等场景，请使用 SHA-256 或 SHA-3 系列算法。
 *
 * @section standards 遵循的国际标准
 * 本实现严格遵循以下标准规范（用于兼容性目的）：
 *
 * **SHA-1 算法规范：**
 * - **NIST FIPS PUB 180-4:2015**：安全哈希标准 (SHS)
 *   https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf
 * - **IETF RFC 3174**：US 安全哈希算法 1 (SHA-1)
 *   https://www.rfc-editor.org/rfc/rfc3174.html
 *
 * **相关国际标准（信息参考）：**
 * - **ISO/IEC 10118-3:2018**：信息技术 — 安全技术 — 哈希函数 — 第3部分：专用哈希函数
 *   https://www.iso.org/standard/67116.html
 *
 * **废弃与迁移指南：**
 * - **NIST SP 800-131A Rev. 2**：过渡期：密码算法和密钥长度的使用
 *   https://csrc.nist.gov/pubs/sp/800/131/a/r2/final
 * - **IETF RFC 9155**：传输层安全 (TLS) 及 DTLS 中已废弃的 SHA-1 用法
 *   https://www.rfc-editor.org/rfc/rfc9155.html
 *
 * @section implementation_details 实现细节
 * | 特性              | 规范参数                                  |
 * |-------------------|-------------------------------------------|
 * | 算法              | SHA-1 安全哈希算法                        |
 * | 输出长度          | 160 位（20 字节）                         |
 * | 内部状态          | 5 个 32 位字（h0, h1, h2, h3, h4）       |
 * | 分组大小          | 512 位（64 字节）                         |
 * | 轮数              | 4 轮 × 20 步 = 80 步                      |
 * | 最大输入长度      | 2^61 - 1 字节                             |
 * | 初始向量 (IV)     | FIPS 180-4 §6.1.1 定义                    |
 * | 填充方案          | Merkle–Damgård 强化（FIPS 180-4 §5.1.1）  |
 * | 字节序            | 大端（FIPS 180-4 §3.2.2）                  |
 *
 * @section usage 适用场景
 * 尽管存在安全弱点，SHA-1 仍可用于以下非安全场景：
 * - Git 版本控制系统中的对象标识（正在迁移到 SHA-256）
 * - 历史数据完整性验证（非对抗性环境）
 * - 与历史系统兼容的数据交换
 * - 非安全相关的校验和计算
 *
 * @see https://csrc.nist.gov/projects/hash-functions
 * @see https://shattered.io/ （SHAttered 碰撞攻击演示）
 * @warning **禁止使用**：数字签名、SSL/TLS 证书、代码签名、认证令牌等安全场景。
 *          根据 NIST SP 800-131A Rev. 2，SHA-1 已于 2013 年后禁止用于数字签名生成。
 */

#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Encryption 加密算法
 * @{
 */

/**
 * @struct SHA1
 * @brief SHA-1哈希算法结构体
 *
 * 提供静态方法计算SHA-1哈希值，支持返回字节数组或十六进制字符串。
 *
 * @warning SHA-1不安全，不应用于安全敏感场景。
 */
struct NEFORCE_API SHA1 {
    /**
     * @brief 计算SHA-1哈希值
     * @param data 输入数据
     * @return 20字节的哈希值
     */
    static byte_vector hash(cbyte_view data);

    /**
     * @brief 计算SHA-1哈希值的十六进制表示
     * @param data 输入数据
     * @return 40字符的十六进制字符串
     */
    static string hash_hex(cbyte_view data);
};


/**
 * @brief SHA-1哈希便捷函数（字符串视图版本）
 * @param data 输入字符串
 * @return 20字节的哈希值（原始字节）
 * @note 返回原始字节数据而非十六进制字符串
 */
NEFORCE_ALWAYS_INLINE_INLINE string sha1(const string_view data) {
    const byte_vector h = SHA1::hash({reinterpret_cast<const byte_t*>(data.data()), data.size()});
    return string{reinterpret_cast<const char*>(h.data()), h.size()};
}

/**
 * @brief SHA-1哈希便捷函数（字符串版本）
 * @param data 输入字符串
 * @return 20字节的哈希值（原始字节）
 */
NEFORCE_ALWAYS_INLINE_INLINE string sha1(const string& data) { return _NEFORCE sha1(data.view()); }

/**
 * @brief SHA-1哈希便捷函数（字节视图版本）
 * @param data 输入数据
 * @return 20字节的哈希值
 */
NEFORCE_ALWAYS_INLINE_INLINE byte_vector sha1(const cbyte_view data) { return SHA1::hash(data); }

/**
 * @brief SHA-1哈希便捷函数（字节向量版本）
 * @param data 输入数据
 * @return 20字节的哈希值
 */
NEFORCE_ALWAYS_INLINE_INLINE byte_vector sha1(const byte_vector& data) { return SHA1::hash(data.view()); }

/** @} */ // Encryption

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ENCRYPT_SHA1_HPP__
