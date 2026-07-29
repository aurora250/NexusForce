#ifndef NEFORCE_CORE_STRING_CHARSET_HPP__
#define NEFORCE_CORE_STRING_CHARSET_HPP__

/**
 * @file charset.hpp
 * @brief 字符集工具类
 *
 * 此文件提供了字符集类，用于字符归属测试与集合运算。
 * 提供常见的 ASCII 预定义字符集。
 */

#include "NeForce/core/string/char_types.hpp"
#include "NeForce/core/typeinfo/type_traits.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Charset 字符集
 * @brief 可组合的字符集运算与归属测试
 * @{
 */

/**
 * @class charset
 * @brief 字符集
 *
 * 支持集合运算（并、交、补、差）与字符归属测试。
 */
class charset {
public:
    /**
     * @brief 默认构造函数，构造空字符集
     */
    constexpr charset() noexcept = default;

    /**
     * @brief 从单个字符构造字符集
     * @param c 字符
     * @return 仅包含该字符的字符集
     */
    NEFORCE_NODISCARD static constexpr charset from_char(const char c) noexcept {
        charset cs;
        cs.insert(c);
        return cs;
    }

    /**
     * @brief 从字符范围构造字符集 [lo, hi]
     * @param lo 起始字符
     * @param hi 结束字符
     * @return 包含范围内所有字符的字符集
     *
     * 若 lo > hi，返回空集。
     */
    NEFORCE_NODISCARD static constexpr charset range(const char lo, const char hi) noexcept {
        charset cs;
        const auto u_lo = static_cast<byte_t>(lo);
        const auto u_hi = static_cast<byte_t>(hi);
        if (u_lo > u_hi) {
            return cs;
        }

        for (int w = 0; w < 4; ++w) {
            const unsigned word_start = static_cast<unsigned>(w) * 64;
            const unsigned word_end = word_start + 63;
            if (u_hi < word_start || u_lo > word_end) {
                continue;
            }
            const unsigned local_lo = (u_lo > word_start) ? (u_lo - word_start) : 0;
            const unsigned local_hi = (u_hi < word_end) ? (u_hi - word_start) : 63;
            const uint64_t mask = (~uint64_t{0} >> (63 - local_hi)) & (~uint64_t{0} << local_lo);
            cs.bits_[w] = mask;
        }
        return cs;
    }

    /**
     * @brief 插入字符
     * @param c 要插入的字符
     */
    constexpr void insert(const char c) noexcept {
        const auto idx = static_cast<byte_t>(c);
        bits_[idx >> 6] |= (uint64_t{1} << (idx & 63));
    }

    /**
     * @brief 移除字符
     * @param c 要移除的字符
     */
    constexpr void erase(const char c) noexcept {
        const auto idx = static_cast<byte_t>(c);
        bits_[idx >> 6] &= ~(uint64_t{1} << (idx & 63));
    }

    /**
     * @brief 检查字符是否在集合中
     * @param c 要检查的字符
     * @return 存在则返回 true
     */
    NEFORCE_NODISCARD constexpr bool contains(const char c) const noexcept {
        const auto idx = static_cast<byte_t>(c);
        return (bits_[idx >> 6] & (uint64_t{1} << (idx & 63))) != 0;
    }

    /**
     * @brief 检查字符集是否为空
     * @return 空集返回 true
     */
    NEFORCE_NODISCARD constexpr bool empty() const noexcept { return (bits_[0] | bits_[1] | bits_[2] | bits_[3]) == 0; }

    /**
     * @brief 并集
     * @param other 另一个字符集
     * @return 包含两集合所有字符的新字符集
     */
    NEFORCE_NODISCARD constexpr charset operator|(const charset other) const noexcept {
        charset result;
        result.bits_[0] = bits_[0] | other.bits_[0];
        result.bits_[1] = bits_[1] | other.bits_[1];
        result.bits_[2] = bits_[2] | other.bits_[2];
        result.bits_[3] = bits_[3] | other.bits_[3];
        return result;
    }

    /**
     * @brief 交集
     * @param other 另一个字符集
     * @return 包含两集合共有字符的新字符集
     */
    NEFORCE_NODISCARD constexpr charset operator&(const charset other) const noexcept {
        charset result;
        result.bits_[0] = bits_[0] & other.bits_[0];
        result.bits_[1] = bits_[1] & other.bits_[1];
        result.bits_[2] = bits_[2] & other.bits_[2];
        result.bits_[3] = bits_[3] & other.bits_[3];
        return result;
    }

    /**
     * @brief 补集
     * @return 包含所有不在当前集合中字符的新字符集
     */
    NEFORCE_NODISCARD constexpr charset operator~() const noexcept {
        charset result;
        result.bits_[0] = ~bits_[0];
        result.bits_[1] = ~bits_[1];
        result.bits_[2] = ~bits_[2];
        result.bits_[3] = ~bits_[3];
        return result;
    }

    /**
     * @brief 差集
     * @param other 另一个字符集
     * @return 包含在当前集合但不在 other 中字符的新字符集
     */
    NEFORCE_NODISCARD constexpr charset operator-(const charset other) const noexcept { return *this & ~other; }

    /**
     * @brief 并集
     * @param other 另一个字符集
     * @return 自身引用
     */
    constexpr charset& operator|=(const charset other) noexcept {
        bits_[0] |= other.bits_[0];
        bits_[1] |= other.bits_[1];
        bits_[2] |= other.bits_[2];
        bits_[3] |= other.bits_[3];
        return *this;
    }

    /**
     * @brief 交集
     * @param other 另一个字符集
     * @return 自身引用
     */
    constexpr charset& operator&=(const charset other) noexcept {
        bits_[0] &= other.bits_[0];
        bits_[1] &= other.bits_[1];
        bits_[2] &= other.bits_[2];
        bits_[3] &= other.bits_[3];
        return *this;
    }

    /**
     * @brief 差集
     * @param other 另一个字符集
     * @return 自身引用
     */
    constexpr charset& operator-=(const charset other) noexcept {
        bits_[0] &= ~other.bits_[0];
        bits_[1] &= ~other.bits_[1];
        bits_[2] &= ~other.bits_[2];
        bits_[3] &= ~other.bits_[3];
        return *this;
    }

    /**
     * @brief 相等比较
     * @param other 另一个字符集
     * @return 相等返回 true
     */
    NEFORCE_NODISCARD constexpr bool operator==(const charset other) const noexcept {
        return bits_[0] == other.bits_[0] && bits_[1] == other.bits_[1] && bits_[2] == other.bits_[2] &&
               bits_[3] == other.bits_[3];
    }

    /**
     * @brief 不等比较
     * @param other 另一个字符集
     * @return 不等返回 true
     */
    NEFORCE_NODISCARD constexpr bool operator!=(const charset other) const noexcept { return !(*this == other); }

    /**
     * @brief 空集
     * @return 空字符集
     */
    NEFORCE_NODISCARD static constexpr charset empty_set() noexcept { return charset{}; }

    /**
     * @brief 全集
     * @return 包含所有 256 个字节值的字符集
     */
    NEFORCE_NODISCARD static constexpr charset universe() noexcept {
        charset cs;
        cs.bits_[0] = ~uint64_t{0};
        cs.bits_[1] = ~uint64_t{0};
        cs.bits_[2] = ~uint64_t{0};
        cs.bits_[3] = ~uint64_t{0};
        return cs;
    }

    /**
     * @brief ASCII 小写字母 'a'~'z'
     * @return 字符集
     */
    NEFORCE_NODISCARD static constexpr charset ascii_alpha_lower() noexcept { return range('a', 'z'); }

    /**
     * @brief ASCII 大写字母 'A'~'Z'
     * @return 字符集
     */
    NEFORCE_NODISCARD static constexpr charset ascii_alpha_upper() noexcept { return range('A', 'Z'); }

    /**
     * @brief ASCII 字母 'a'~'z' | 'A'~'Z'
     * @return 字符集
     */
    NEFORCE_NODISCARD static constexpr charset ascii_alpha() noexcept {
        return ascii_alpha_lower() | ascii_alpha_upper();
    }

    /**
     * @brief ASCII 数字 '0'~'9'
     * @return 字符集
     */
    NEFORCE_NODISCARD static constexpr charset ascii_digit() noexcept { return range('0', '9'); }

    /**
     * @brief ASCII 字母与数字
     * @return 字符集
     */
    NEFORCE_NODISCARD static constexpr charset ascii_alnum() noexcept { return ascii_alpha() | ascii_digit(); }

    /**
     * @brief ASCII 十六进制数字 0~9, A~F, a~f
     * @return 字符集
     */
    NEFORCE_NODISCARD static constexpr charset ascii_hex_digit() noexcept {
        return ascii_digit() | range('A', 'F') | range('a', 'f');
    }

    /**
     * @brief ASCII 空白字符
     * @return 字符集
     */
    NEFORCE_NODISCARD static constexpr charset ascii_blank() noexcept {
        charset cs;
        cs.bits_[0] = constants::BLANK_MASK;
        return cs;
    }

    /**
     * @brief ASCII 空白字符集
     * @return 字符集
     */
    NEFORCE_NODISCARD static constexpr charset ascii_space() noexcept {
        charset cs;
        cs.bits_[0] = constants::SPACE_MASK;
        return cs;
    }

    /**
     * @brief ASCII 标点符号
     * @return 字符集
     */
    NEFORCE_NODISCARD static constexpr charset ascii_punct() noexcept {
        charset cs;
        cs.bits_[0] = constants::PUNCT_MASK_LOW;
        cs.bits_[1] = constants::PUNCT_MASK_HIGH;
        return cs;
    }

    /**
     * @brief ASCII 控制字符 0~31 与 127
     * @return 字符集
     */
    NEFORCE_NODISCARD static constexpr charset ascii_cntrl() noexcept {
        charset cs;
        cs.bits_[0] = constants::CNTRL_MASK_LOW;
        cs.bits_[1] = constants::CNTRL_MASK_HIGH;
        return cs;
    }

    /**
     * @brief ASCII 可打印字符 32~126
     * @return 字符集
     */
    NEFORCE_NODISCARD static constexpr charset ascii_print() noexcept { return range(' ', '~'); }

    /**
     * @brief ASCII 图形字符
     * @return 字符集
     */
    NEFORCE_NODISCARD static constexpr charset ascii_graph() noexcept { return ascii_print() - ascii_blank(); }

private:
    uint64_t bits_[4] = {}; ///< 256-bit 位图
};

/** @} */ // Charset

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_STRING_CHARSET_HPP__
