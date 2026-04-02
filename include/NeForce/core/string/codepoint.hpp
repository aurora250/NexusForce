#ifndef NEFORCE_CORE_STRING_CODEPOINT_HPP__
#define NEFORCE_CORE_STRING_CODEPOINT_HPP__

/**
 * @file codepoint.hpp
 * @brief Unicode码点处理类
 *
 * 此文件提供了Unicode码点的封装类，支持码点的验证、UTF-8/UTF-16/UTF-32编码解码、
 * 代理对处理、以及向各种字符串类型的追加操作。
 *
 * Unicode码点范围：U+0000 ~ U+10FFFF（排除代理项范围0xD800-0xDFFF）
 */

#include "NeForce/core/memory/endian.hpp"
#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup CodePoint Unicode码点
 * @brief 码点处理相关功能
 * @{
 */

/**
 * @class codepoint
 * @brief Unicode码点包装类
 *
 * 封装一个经过验证的Unicode码点（U+0000 ~ U+10FFFF，排除代理项）。
 * 非法值在构造时自动替换为U+FFFD（替换字符）。
 * 提供UTF-8、UTF-16、UTF-32之间的转换功能。
 */
class NEFORCE_API codepoint {
public:
    /// Unicode 替换符 U+FFFD
    static constexpr uint32_t REPLACEMENT_VALUE = 0xFFFD;
    /// Unicode 最大合法码点
    static constexpr uint32_t MAX_VALUE = 0x10FFFF;

    /**
     * @brief 检查字符是否为高代理项
     * @param c UTF-16字符
     * @return 如果字符是高代理项则返回true，否则返回false
     *
     * 高代理项的范围是0xD800-0xDBFF。
     */
    NEFORCE_CONST_FUNCTION static constexpr bool is_high_surrogate(const char16_t c) noexcept {
        return c >= 0xD800 && c <= 0xDBFF;
    }

    /**
     * @brief 检查字符是否为低代理项
     * @param c UTF-16字符
     * @return 如果字符是低代理项则返回true，否则返回false
     *
     * 低代理项的范围是0xDC00-0xDFFF。
     */
    NEFORCE_CONST_FUNCTION static constexpr bool is_low_surrogate(const char16_t c) noexcept {
        return c >= 0xDC00 && c <= 0xDFFF;
    }

    /**
     * @brief 组合高代理项和低代理项为完整的Unicode码点
     * @param high 高代理项
     * @param low 低代理项
     * @return 组合后的Unicode码点
     *
     * 根据UTF-16编码规则将两个代理项组合为完整的码点。
     */
    NEFORCE_CONST_FUNCTION static constexpr codepoint combine_surrogates(const char16_t high,
                                                                         const char16_t low) noexcept {
        return codepoint{0x10000 + ((static_cast<uint32_t>(high) - 0xD800) << 10) +
                         (static_cast<uint32_t>(low) - 0xDC00)};
    }

    /**
     * @brief 检查码点值是否合法
     * @param v 要检查的码点值
     * @return 合法返回true，否则返回false
     *
     * 合法条件：
     * - 不超过0x10FFFF
     * - 不是高代理项（0xD800-0xDBFF）
     * - 不是低代理项（0xDC00-0xDFFF）
     */
    static constexpr bool is_valid_codepoint(const uint32_t v) noexcept {
        return v <= MAX_VALUE && !is_high_surrogate(v) && !is_low_surrogate(v);
    }

    /**
     * @brief 构造替换符U+FFFD
     * @return 替换符码点对象
     *
     * 用于表示无效或无法表示的Unicode字符。
     */
    static constexpr codepoint replacement() noexcept {
        codepoint cp;
        cp.value_ = REPLACEMENT_VALUE;
        return cp;
    }

    /**
     * @brief 构造空字符U+0000
     * @return 空字符码点对象
     */
    static constexpr codepoint null() noexcept { return codepoint(0u); }

    /**
     * @brief 从UTF-8字节流解码一个码点，并推进索引
     * @param data UTF-8字节数据
     * @param i 当前位置（解码后自动推进）
     * @param len 数据总长度
     * @return 解码结果，失败时返回替换符
     *
     * 支持1-4字节的UTF-8序列，自动验证序列的有效性。
     * 无效序列或截断序列返回替换符。
     */
    static codepoint decode_utf8(const byte_t* data, size_t& i, size_t len) noexcept;

    /**
     * @brief 从UTF-16序列解码一个码点，并推进索引
     * @tparam T char16_t或wchar_t类型
     * @param data UTF-16数据指针
     * @param index 当前位置（解码后自动推进）
     * @param len 数据总长度
     * @param need_swap 是否需要字节序反转
     * @return 解码结果，失败时返回替换符
     *
     * 支持单码元和代理对解码，自动处理字节序转换。
     * 无效序列返回替换符。
     */
    template <typename T>
    static codepoint decode_utf16(const T* data, size_t& index, const size_t len, const bool need_swap) noexcept {
        uint32_t cp;
        size_t consumed;
        const bool ok = codepoint::utf16_codepoint(data, index, len, cp, consumed, need_swap);
        if (consumed == 0) {
            return replacement();
        }
        index += consumed;
        return ok ? codepoint(cp) : replacement();
    }

    /**
     * @brief 从UTF-32值直接构造码点
     * @param value UTF-32字符
     * @return 码点对象
     *
     * UTF-32编码直接对应Unicode码点值。
     */
    static constexpr codepoint from_utf32(char32_t value) noexcept { return codepoint(static_cast<uint32_t>(value)); }

private:
    uint32_t value_; ///< 存储的码点值

private:
    template <typename T>
    static bool utf16_codepoint(const T* data, size_t index, const size_t len, uint32_t& cp, size_t& consumed,
                                const bool need_swap) {
        if (index >= len) {
            cp = 0xFFFD;
            consumed = 0;
            return false;
        }

        const auto raw1 = static_cast<uint16_t>(data[index]);
        const auto c1 = static_cast<uint32_t>(need_swap ? endian::byteswap16(raw1) : raw1);
        consumed = 1;

        if (is_high_surrogate(c1)) {
            if (index + 1 < len) {
                const auto raw2 = static_cast<uint16_t>(data[index + 1]);
                const auto c2 = static_cast<uint32_t>(need_swap ? endian::byteswap16(raw2) : raw2);

                if (is_low_surrogate(c2)) {
                    cp = combine_surrogates(c1, c2).value();
                    consumed = 2;
                    return true;
                }
            }

            cp = 0xFFFD;
            return false;
        }

        if (is_low_surrogate(c1)) {
            cp = 0xFFFD;
            return false;
        }

        cp = c1;
        return true;
    }

public:
    /**
     * @brief 默认构造函数
     * @brief 构造空字符U+0000
     */
    constexpr codepoint() noexcept :
    value_(0) {}

    /**
     * @brief 从uint32_t构造码点
     * @param value 原始码点值
     *
     * 如果值非法，自动替换为U+FFFD。
     */
    constexpr explicit codepoint(uint32_t value) noexcept :
    value_(is_valid_codepoint(value) ? value : REPLACEMENT_VALUE) {}

    /**
     * @brief 从char32_t构造码点
     * @param value UTF-32字符
     */
    constexpr explicit codepoint(const char32_t value) noexcept :
    codepoint(static_cast<uint32_t>(value)) {}

    constexpr codepoint(const codepoint&) noexcept = default;
    constexpr codepoint& operator=(const codepoint&) noexcept = default;
    constexpr codepoint(codepoint&&) noexcept = default;
    constexpr codepoint& operator=(codepoint&&) noexcept = default;

    /**
     * @brief 获取码点的uint32_t值
     * @return 码点数值
     */
    constexpr uint32_t value() const noexcept { return value_; }

    /**
     * @brief 获取码点的char32_t值
     * @return UTF-32字符
     */
    constexpr char32_t to_char32() const noexcept { return static_cast<char32_t>(value_); }

    /**
     * @brief 是否为替换符U+FFFD
     * @return 是替换符返回true
     */
    constexpr bool is_replacement() const noexcept { return value_ == REPLACEMENT_VALUE; }

    /**
     * @brief 是否为ASCII字符（U+0000 ~ U+007F）
     * @return 是ASCII字符返回true
     */
    constexpr bool is_ascii() const noexcept { return value_ <= 0x7F; }

    /**
     * @brief 是否位于基本多文种平面（BMP, U+0000 ~ U+FFFF）
     * @return 在BMP内返回true
     */
    constexpr bool is_bmp() const noexcept { return value_ <= 0xFFFF; }

    /**
     * @brief 是否为辅助平面字符（需要UTF-16代理对）
     * @return 是辅助平面字符返回true
     */
    constexpr bool is_supplementary() const noexcept { return value_ > 0xFFFF && value_ <= MAX_VALUE; }

    /**
     * @brief 是否需要UTF-16代理对表示
     * @return 需要代理对返回true
     */
    constexpr bool needs_surrogate_pair() const noexcept { return is_supplementary(); }

    /**
     * @brief UTF-8编码后的字节数
     * @return 字节数（1~4）
     *
     * 根据码点值计算所需UTF-8编码长度：
     * - U+0000 ~ U+007F: 1字节
     * - U+0080 ~ U+07FF: 2字节
     * - U+0800 ~ U+FFFF: 3字节
     * - U+10000 ~ U+10FFFF: 4字节
     */
    constexpr size_t utf8_length() const noexcept {
        if (value_ <= 0x7F) {
            return 1;
        }
        if (value_ <= 0x7FF) {
            return 2;
        }
        if (value_ <= 0xFFFF) {
            return 3;
        }
        return 4;
    }

    /**
     * @brief UTF-16编码后的码元数
     * @return 码元数（1~2）
     *
     * BMP字符需要1个码元，辅助平面字符需要2个码元（代理对）。
     */
    constexpr size_t utf16_length() const noexcept { return is_supplementary() ? 2u : 1u; }

    /**
     * @brief 追加UTF-8编码到string
     * @param result 目标字符串
     *
     * 将码点以UTF-8编码追加到string中。
     */
    void append_to(string& result) const;

#ifdef NEFORCE_STANDARD_20
    /**
     * @brief 追加UTF-8编码到u8string
     * @param result 目标UTF-8字符串
     *
     * 将码点以UTF-8编码追加到u8string中（C++20）。
     */
    void append_to(u8string& result) const;
#endif

    /**
     * @brief 追加UTF-16编码到u16string
     * @param result 目标UTF-16字符串
     *
     * 将码点以UTF-16编码追加到u16string中。
     * BMP字符追加单个码元，辅助平面字符追加代理对。
     */
    void append_to(u16string& result) const;

    /**
     * @brief 追加UTF-32编码到u32string
     * @param result 目标UTF-32字符串
     *
     * 将码点直接追加到u32string中（一个码元）。
     */
    void append_to(u32string& result) const { result.push_back(static_cast<char32_t>(value_)); }

    /**
     * @brief 追加编码到wstring
     * @param result 目标宽字符串
     */
    void append_to(wstring& result) const;

    constexpr bool operator==(const codepoint& other) const noexcept { return value_ == other.value_; }
    constexpr bool operator!=(const codepoint& other) const noexcept { return value_ != other.value_; }
    constexpr bool operator<(const codepoint& other) const noexcept { return value_ < other.value_; }
    constexpr bool operator<=(const codepoint& other) const noexcept { return value_ <= other.value_; }
    constexpr bool operator>(const codepoint& other) const noexcept { return value_ > other.value_; }
    constexpr bool operator>=(const codepoint& other) const noexcept { return value_ >= other.value_; }

    constexpr bool operator==(uint32_t v) const noexcept { return value_ == v; }
    constexpr bool operator!=(uint32_t v) const noexcept { return value_ != v; }
};

/** @} */ // CodePoint

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_STRING_CODEPOINT_HPP__
