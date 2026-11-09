#ifndef NEFORCE_CORE_KDF_BCRYPT_HPP__
#define NEFORCE_CORE_KDF_BCRYPT_HPP__

/**
 * @file bcrypt.hpp
 * @brief bcrypt 口令哈希实现
 *
 * 此文件提供 bcrypt 口令哈希的生成与校验，输出 `$2b$` 模块化加密格式。
 */

#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup KDF
 * @{
 */

/**
 * @defgroup BCrypt bcrypt
 * @ingroup KDF
 * @brief 基于 Blowfish 的自适应口令哈希
 *
 * @section standards 遵循的规范
 * - **Niels Provos, David Mazières**：A Future-Adaptable Password Scheme (USENIX 1999)
 *   https://www.usenix.org/legacy/events/usenix99/provos/provos_html/
 * - **OpenBSD `crypt(3)`**：`$2b$` 模块化加密格式与 bcrypt 基准实现
 *   https://man.openbsd.org/crypt.3
 *
 * @section format 输出格式
 * `$2b$CC$<22 字符盐><31 字符哈希>`，共 60 个字符，其中：
 * - `CC` 为两位十进制 cost（4-31），实际迭代次数为 `2^cost`
 * - 盐与哈希均使用 bcrypt 专用 Base64 字母表 `./A-Za-z0-9`
 *
 * @section security 安全说明
 * - bcrypt 是**慢哈希**，cost 每增加 1 计算量翻倍；默认 12 约需数十毫秒
 * - 口令超过 72 字节时仅前 72 字节参与运算（协议限制）
 * - 校验使用常数时间比较，避免通过响应时间泄露前缀信息
 *
 * @warning 不要将 bcrypt 用于一般数据完整性校验；它只适用于口令存储。
 * @{
 */

/**
 * @struct bcrypt
 * @brief bcrypt 口令哈希结构体
 */
struct NEFORCE_API bcrypt {
    /**
     * @brief 使用随机盐计算口令哈希
     * @param password 口令
     * @param cost 计算成本，范围 4-31，默认 12
     * @return 60 字符的 `$2b$` 格式哈希
     * @throws value_exception 当 cost 超出范围时抛出
     */
    static string hash(string_view password, uint32_t cost = 12);

    /**
     * @brief 使用指定盐计算口令哈希
     * @param password 口令
     * @param salt 16 字节盐
     * @param cost 计算成本，范围 4-31，默认 12
     * @return 60 字符的 `$2b$` 格式哈希
     * @throws value_exception 当 cost 超出范围或盐长度不是 16 字节时抛出
     */
    static string hash(string_view password, cbyte_view salt, uint32_t cost = 12);

    /**
     * @brief 校验口令与哈希是否匹配
     * @param password 待校验口令
     * @param hash `$2a$`、`$2b$` 或 `$2y$` 格式的哈希
     * @return 匹配返回 true；哈希格式非法或不匹配返回 false
     */
    static bool verify(string_view password, string_view hash);

    /**
     * @brief 判断字符串是否为合法的 bcrypt 哈希
     * @param hash 待检查的字符串
     * @return 格式合法返回 true
     */
    static bool is_valid_hash(string_view hash) noexcept;
};

/**
 * @brief bcrypt 哈希便捷函数
 * @param password 口令
 * @param cost 计算成本，默认 12
 * @return 60 字符的 `$2b$` 格式哈希
 * @throws value_exception 当 cost 超出范围时抛出
 */
NEFORCE_ALWAYS_INLINE_INLINE string bcrypt_hash(const string_view password, const uint32_t cost = 12) {
    return bcrypt::hash(password, cost);
}

/**
 * @brief bcrypt 校验便捷函数
 * @param password 待校验口令
 * @param hash bcrypt 哈希
 * @return 匹配返回 true
 */
NEFORCE_ALWAYS_INLINE_INLINE bool bcrypt_verify(const string_view password, const string_view hash) noexcept {
    return bcrypt::verify(password, hash);
}

/** @} */ // BCrypt

/** @} */ // KDF

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_KDF_BCRYPT_HPP__
