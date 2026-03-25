#ifndef NEFORCE_CORE_STRING_CHAR_TYPES_HPP__
#define NEFORCE_CORE_STRING_CHAR_TYPES_HPP__

/**
 * @file char_types.hpp
 * @brief 字符类型分类和转换函数
 *
 * 此文件提供了字符分类和转换函数的实现，包括ASCII字符检查、
 * 字符类型判断、大小写转换等功能。使用位掩码技术高效实现。
 */

#include "NeForce/core/typeinfo/type_traits.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup CharTypeChecking 字符类型检查
 * @brief 检查字符是否属于特定类型的函数
 * @{
 */

/// @cond
NEFORCE_BEGIN_CONSTANTS__

/**
 * @var BLANK_MASK
 * @brief 空白字符位掩码
 *
 * 包含水平制表符(9)和空格(32)。
 */
NEFORCE_INLINE17 constexpr uint64_t BLANK_MASK =
    (1ULL << 9)  |  // \t
    (1ULL << 32);   // space

/**
 * @var SPACE_MASK
 * @brief 空白字符位掩码
 *
 * 包含所有空白字符：水平制表符(9), 换行符(10), 垂直制表符(11), 换页符(12), 回车符(13), 空格(32)。
 */
NEFORCE_INLINE17 constexpr uint64_t SPACE_MASK =
	BLANK_MASK |
    (1ULL << 10) |  // \n
    (1ULL << 11) |  // \v
    (1ULL << 12) |  // \f
    (1ULL << 13);   // \r

/**
 * @var PUNCT_MASK_LOW
 * @brief 低64位标点符号位掩码
 *
 * 包含ASCII码33-47, 58-63范围内的标点符号。
 */
NEFORCE_INLINE17 constexpr uint64_t PUNCT_MASK_LOW =
    (1ULL << 33)  | (1ULL << 34)  | (1ULL << 35)  | (1ULL << 36)  | // !"#$ (33~36 < 64)
    (1ULL << 37)  | (1ULL << 38)  | (1ULL << 39)  | (1ULL << 40)  | // %&'() (37~40 < 64)
    (1ULL << 41)  | (1ULL << 42)  | (1ULL << 43)  | (1ULL << 44)  | // *+,- (41~44 < 64)
    (1ULL << 45)  | (1ULL << 46)  | (1ULL << 47)  | (1ULL << 58)  | // ./: (45~47,58 < 64)
    (1ULL << 59)  | (1ULL << 60)  | (1ULL << 61)  | (1ULL << 62)  | // ;<=> (59~62 < 64)
    (1ULL << 63);                                                   // ?

/**
 * @var PUNCT_MASK_HIGH
 * @brief 64-127位标点符号位掩码
 *
 * 包含ASCII码64-126范围内的标点符号。
 */
NEFORCE_INLINE17 constexpr uint64_t PUNCT_MASK_HIGH =
    (1ULL << (64 - 64))  | (1ULL << (65 - 64))  | (1ULL << (91 - 64))  |					    // @(64)、A(65)、[(91)
    (1ULL << (92 - 64))  | (1ULL << (93 - 64))  | (1ULL << (94 - 64))  |(1ULL << (95 - 64))  |  // \\(92)、](93)、^(94)、_(95)
    (1ULL << (96 - 64))  | (1ULL << (123 - 64)) | (1ULL << (124 - 64)) | (1ULL << (125 - 64)) | // `(96)、{(123)、|(124)、}(125)
    (1ULL << (126 - 64));                                                                       // ~(126)

/**
 * @var CNTRL_MASK_LOW
 * @brief 低64位控制字符位掩码
 *
 * 包含ASCII码0-31范围内的控制字符。
 */
NEFORCE_INLINE17 constexpr uint64_t CNTRL_MASK_LOW =
    (1ULL << 0)  | (1ULL << 1)  | (1ULL << 2)  | (1ULL << 3)  | // 0-3 (0~3 < 64)
    (1ULL << 4)  | (1ULL << 5)  | (1ULL << 6)  | (1ULL << 7)  | // 4-7 (4~7 < 64)
    (1ULL << 8)  | (1ULL << 9)  | (1ULL << 10) | (1ULL << 11) | // 8-11 (8~11 < 64)
    (1ULL << 12) | (1ULL << 13) | (1ULL << 14) | (1ULL << 15) | // 12-15 (12~15 < 64)
    (1ULL << 16) | (1ULL << 17) | (1ULL << 18) | (1ULL << 19) | // 16-19 (16~19 < 64)
    (1ULL << 20) | (1ULL << 21) | (1ULL << 22) | (1ULL << 23) | // 20-23 (20~23 < 64)
    (1ULL << 24) | (1ULL << 25) | (1ULL << 26) | (1ULL << 27) | // 24-27 (24~27 < 64)
    (1ULL << 28) | (1ULL << 29) | (1ULL << 30) | (1ULL << 31);  // 30-31 (30~31 < 64)

/**
 * @var CNTRL_MASK_HIGH
 * @brief 64-127位控制字符位掩码
 *
 * 包含ASCII码127。
 */
NEFORCE_INLINE17 constexpr uint64_t CNTRL_MASK_HIGH =
    (1ULL << (127 - 64)); // DEL(127)

NEFORCE_END_CONSTANTS__
/// @endcond


/**
 * @brief 通用字符类型检查函数
 * @tparam CharT 字符类型
 * @param c 要检查的字符
 * @param mask_low 低64位ASCII字符的位掩码
 * @param mask_high 高64位ASCII字符的位掩码
 * @return 如果字符在掩码中则返回true，否则返回false
 *
 * 使用位掩码技术高效检查字符类型，支持任意字符类型。
 */
template <typename CharT>
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 bool is_ctype(const CharT c, uint64_t mask_low, uint64_t mask_high) noexcept {
    static_assert(is_character_v<CharT>, "character type is necessary");
	const auto uc = static_cast<make_unsigned_t<CharT>>(c);
	if (uc > 127) return false;
	if (uc <= 63) return (mask_low & (1ULL << uc)) != 0;
	const auto offset = uc - 64;
	return (mask_high & (1ULL << offset)) != 0;
}

/**
 * @brief 检查字符是否为标点符号
 * @tparam CharT 字符类型
 * @param c 要检查的字符
 * @return 如果字符是标点符号则返回true，否则返回false
 */
template <typename CharT>
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 bool is_punct(const CharT c) noexcept {
	return _NEFORCE is_ctype(c, constants::PUNCT_MASK_LOW, constants::PUNCT_MASK_HIGH);
}

/**
 * @brief 检查字符是否为控制字符
 * @tparam CharT 字符类型
 * @param c 要检查的字符
 * @return 如果字符是控制字符则返回true，否则返回false
 *
 * 控制字符包括ASCII码0-31和127。
 */
template <typename CharT>
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 bool is_cntrl(const CharT c) noexcept {
	return _NEFORCE is_ctype(c, constants::CNTRL_MASK_LOW, constants::CNTRL_MASK_HIGH);
}

/**
 * @brief 检查字符是否为可打印字符
 * @tparam CharT 字符类型
 * @param c 要检查的字符
 * @return 如果字符是可打印字符则返回true，否则返回false
 *
 * 可打印字符是非控制字符且ASCII码在0-127范围内的字符。
 */
template <typename CharT>
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 bool is_print(const CharT c) noexcept {
    static_assert(is_character_v<CharT>, "character type is necessary");
	const auto uc = static_cast<make_unsigned_t<CharT>>(c);
	return uc <= 127 && !_NEFORCE is_cntrl(c);
}

/**
 * @brief 检查字符是否为空白字符
 * @tparam CharT 字符类型
 * @param c 要检查的字符
 * @return 如果字符是空白字符则返回true，否则返回false
 *
 * 空白字符包括：制表符和空格。
 */
template <typename CharT>
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 bool is_blank(const CharT c) noexcept {
    static_assert(is_character_v<CharT>, "character type is necessary");
	const auto uc = static_cast<make_unsigned_t<CharT>>(c);
	return uc < 64 && (constants::BLANK_MASK & (1ULL << uc)) != 0;
}

/**
 * @brief 检查字符是否为图形字符
 * @tparam CharT 字符类型
 * @param c 要检查的字符
 * @return 如果字符是图形字符则返回true，否则返回false
 *
 * 图形字符是可打印字符但不是空白字符。
 */
template <typename CharT>
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 bool is_graph(const CharT c) noexcept {
	return _NEFORCE is_print(c) && !_NEFORCE is_blank(c);
}

/**
 * @brief 检查字符是否为ASCII字符
 * @tparam CharT 字符类型
 * @param c 要检查的字符
 * @return 如果字符是ASCII字符则返回true，否则返回false
 *
 * ASCII字符的编码在0-127范围内。
 */
template <typename CharT>
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 bool is_ascii(const CharT c) noexcept {
    static_assert(is_character_v<CharT>, "character type is necessary");
	const auto uc = static_cast<make_unsigned_t<CharT>>(c);
	return uc <= 127;
}

/**
 * @brief 检查字符是否为空白字符
 * @tparam CharT 字符类型
 * @param c 要检查的字符
 * @return 如果字符是空白字符则返回true，否则返回false
 *
 * 空白字符包括：水平制表符, 换行符, 垂直制表符, 换页符, 回车符, 空格。
 */
template <typename CharT>
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 bool is_space(const CharT c) noexcept {
    static_assert(is_character_v<CharT>, "character type is necessary");
    const auto uc = static_cast<make_unsigned_t<CharT>>(c);
    return uc < 64 && (constants::SPACE_MASK & (1ULL << uc)) != 0;
}

/**
 * @brief 检查字符是否为字母
 * @tparam CharT 字符类型
 * @param c 要检查的字符
 * @return 如果字符是字母则返回true，否则返回false
 *
 * 字母包括A-Z和a-z。
 */
template <typename CharT>
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 bool is_alpha(const CharT c) noexcept {
    static_assert(is_character_v<CharT>, "character type is necessary");
    const auto uc = static_cast<make_unsigned_t<CharT>>(c);
    if (uc > 127) return false;
    return (uc & 0xDF) >= 'A' && (uc & 0xDF) <= 'Z';
}

/**
 * @brief 检查字符是否为数字
 * @tparam CharT 字符类型
 * @param c 要检查的字符
 * @return 如果字符是数字则返回true，否则返回false
 *
 * 数字包括0-9。
 */
template <typename CharT>
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 bool is_digit(const CharT c) noexcept {
    static_assert(is_character_v<CharT>, "character type is necessary");
    const auto uc = static_cast<make_unsigned_t<CharT>>(c);
    if (uc > 127) return false;
    return (uc & 0xF0) == 0x30 && (uc & 0x0F) <= 9;
}

/**
 * @brief 检查字符是否为十六进制数字
 * @tparam CharT 字符类型
 * @param c 要检查的字符
 * @return 如果字符是十六进制数字则返回true，否则返回false
 *
 * 十六进制数字包括0-9, A-F, a-f。
 */
template <typename CharT>
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 bool is_xdigit(const CharT c) noexcept {
    static_assert(is_character_v<CharT>, "character type is necessary");
    const auto uc = static_cast<make_unsigned_t<CharT>>(c);
    if (uc > 127) return false;
    const bool is_09 = (uc & 0xF0) == 0x30 && (uc & 0x0F) <= 0x09;
    const bool is_AF = (uc & 0xF0) == 0x40 && (uc & 0x0F) >= 0x01 && (uc & 0x0F) <= 0x06;
    const bool is_af = (uc & 0xF0) == 0x60 && (uc & 0x0F) >= 0x01 && (uc & 0x0F) <= 0x06;
    return is_09 || is_AF || is_af;
}

/**
 * @brief 检查字符是否为字母或数字
 * @tparam CharT 字符类型
 * @param c 要检查的字符
 * @return 如果字符是字母或数字则返回true，否则返回false
 */
template <typename CharT>
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 bool is_alpha_or_digit(const CharT c) noexcept {
    return _NEFORCE is_alpha(c) || _NEFORCE is_digit(c);
}

/**
 * @brief 检查字符是否为数字或字母
 * @tparam CharT 字符类型
 * @param c 要检查的字符
 * @return 如果字符是数字或字母则返回true，否则返回false
 */
template <typename CharT>
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 bool is_digit_or_alpha(const CharT c) noexcept {
    return _NEFORCE is_digit(c) || _NEFORCE is_alpha(c);
}

/** @} */ // CharTypeChecking

/**
 * @defgroup UnicodeSurrogate Unicode代理对处理
 * @brief UTF-16代理对相关函数
 * @{
 */

/**
 * @brief 检查字符是否为高代理项
 * @param c UTF-16字符
 * @return 如果字符是高代理项则返回true，否则返回false
 *
 * 高代理项的范围是0xD800-0xDBFF。
 */
NEFORCE_CONST_FUNCTION constexpr bool is_high_surrogate(const char16_t c) noexcept {
    return c >= 0xD800 && c <= 0xDBFF;
}

/**
 * @brief 检查字符是否为低代理项
 * @param c UTF-16字符
 * @return 如果字符是低代理项则返回true，否则返回false
 *
 * 低代理项的范围是0xDC00-0xDFFF。
 */
NEFORCE_CONST_FUNCTION constexpr bool is_low_surrogate(const char16_t c) noexcept {
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
NEFORCE_CONST_FUNCTION constexpr uint32_t combine_surrogates(const char16_t high, const char16_t low) noexcept {
    return 0x10000 + ((static_cast<uint32_t>(high) - 0xD800) << 10) + (static_cast<uint32_t>(low) - 0xDC00);
}

/** @} */ // UnicodeSurrogate

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_STRING_CHAR_TYPES_HPP__
