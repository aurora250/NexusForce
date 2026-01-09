#ifndef MSTL_CORE_STRING_CHAR_TYPES_HPP__
#define MSTL_CORE_STRING_CHAR_TYPES_HPP__
#include "../typeinfo/types.hpp"
#include "../typeinfo/type_traits.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_BEGIN_CONSTANTS__

MSTL_INLINE17 constexpr uint64_t BLANK_MASK =
    (1ULL << 9)  |  // \t
    (1ULL << 32);   // space

MSTL_INLINE17 constexpr uint64_t SPACE_MASK =
	BLANK_MASK |
    (1ULL << 10) |  // \n
    (1ULL << 11) |  // \v
    (1ULL << 12) |  // \f
    (1ULL << 13);   // \r

MSTL_INLINE17 constexpr uint64_t PUNCT_MASK_LOW =
    (1ULL << 33)  | (1ULL << 34)  | (1ULL << 35)  | (1ULL << 36)  | // !"#$ (33~36 < 64)
    (1ULL << 37)  | (1ULL << 38)  | (1ULL << 39)  | (1ULL << 40)  | // %&'() (37~40 < 64)
    (1ULL << 41)  | (1ULL << 42)  | (1ULL << 43)  | (1ULL << 44)  | // *+,- (41~44 < 64)
    (1ULL << 45)  | (1ULL << 46)  | (1ULL << 47)  | (1ULL << 58)  | // ./: (45~47,58 < 64)
    (1ULL << 59)  | (1ULL << 60)  | (1ULL << 61)  | (1ULL << 62)  | // ;<=> (59~62 < 64)
    (1ULL << 63);                                                   // ?

MSTL_INLINE17 constexpr uint64_t PUNCT_MASK_HIGH =
    (1ULL << (64 - 64))  | (1ULL << (65 - 64))  | (1ULL << (91 - 64))  |					    // @(64)、A(65)、[(91)
    (1ULL << (92 - 64))  | (1ULL << (93 - 64))  | (1ULL << (94 - 64))  |(1ULL << (95 - 64))  |  // \\(92)、](93)、^(94)、_(95)
    (1ULL << (96 - 64))  | (1ULL << (123 - 64)) | (1ULL << (124 - 64)) | (1ULL << (125 - 64)) | // `(96)、{(123)、|(124)、}(125)
    (1ULL << (126 - 64));                                                                       // ~(126)

MSTL_INLINE17 constexpr uint64_t CNTRL_MASK_LOW =
    (1ULL << 0)  | (1ULL << 1)  | (1ULL << 2)  | (1ULL << 3)  | // 0-3 (0~3 < 64)
    (1ULL << 4)  | (1ULL << 5)  | (1ULL << 6)  | (1ULL << 7)  | // 4-7 (4~7 < 64)
    (1ULL << 8)  | (1ULL << 9)  | (1ULL << 10) | (1ULL << 11) | // 8-11 (8~11 < 64)
    (1ULL << 12) | (1ULL << 13) | (1ULL << 14) | (1ULL << 15) | // 12-15 (12~15 < 64)
    (1ULL << 16) | (1ULL << 17) | (1ULL << 18) | (1ULL << 19) | // 16-19 (16~19 < 64)
    (1ULL << 20) | (1ULL << 21) | (1ULL << 22) | (1ULL << 23) | // 20-23 (20~23 < 64)
    (1ULL << 24) | (1ULL << 25) | (1ULL << 26) | (1ULL << 27) | // 24-27 (24~27 < 64)
    (1ULL << 28) | (1ULL << 29) | (1ULL << 30) | (1ULL << 31);  // 30-31 (30~31 < 64)

MSTL_INLINE17 constexpr uint64_t CNTRL_MASK_HIGH =
    (1ULL << (127 - 64));

MSTL_END_CONSTANTS__


template <typename CharT>
MSTL_PURE_FUNCTION MSTL_CONSTEXPR14 bool is_ctype(const CharT c, uint64_t mask_low, uint64_t mask_high) noexcept {
	const auto uc = static_cast<make_unsigned_t<CharT>>(c);
	if (uc > 127) return false;
	if (uc <= 63) return (mask_low & (1ULL << uc)) != 0;
	const auto offset = uc - 64;
	return (mask_high & (1ULL << offset)) != 0;
}

template <typename CharT>
MSTL_PURE_FUNCTION MSTL_CONSTEXPR14 bool is_punct(const CharT c) noexcept {
	return _MSTL is_ctype(c, _CONSTANTS PUNCT_MASK_LOW, _CONSTANTS PUNCT_MASK_HIGH);
}

template <typename CharT>
MSTL_PURE_FUNCTION MSTL_CONSTEXPR14 bool is_cntrl(const CharT c) noexcept {
	return _MSTL is_ctype(c, _CONSTANTS CNTRL_MASK_LOW, _CONSTANTS CNTRL_MASK_HIGH);
}

template <typename CharT>
MSTL_PURE_FUNCTION MSTL_CONSTEXPR14 bool is_print(const CharT c) noexcept {
	const auto uc = static_cast<make_unsigned_t<CharT>>(c);
	return uc <= 127 && !_MSTL is_cntrl(c);
}

template <typename CharT>
MSTL_PURE_FUNCTION MSTL_CONSTEXPR14 bool is_blank(const CharT c) noexcept {
	const auto uc = static_cast<make_unsigned_t<CharT>>(c);
	return uc < 64 && (_CONSTANTS BLANK_MASK & (1ULL << uc)) != 0;
}

template <typename CharT>
MSTL_PURE_FUNCTION MSTL_CONSTEXPR14 bool is_graph(const CharT c) noexcept {
	return _MSTL is_print(c) && !_MSTL is_blank(c);
}

template <typename CharT>
MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 bool is_ascii(const CharT c) noexcept {
	const auto uc = static_cast<make_unsigned_t<CharT>>(c);
	return uc <= 127;
}

template <typename CharT>
MSTL_PURE_FUNCTION MSTL_CONSTEXPR14 bool is_space(const CharT c) noexcept {
    const auto uc = static_cast<make_unsigned_t<CharT>>(c);
    return uc < 64 && (_CONSTANTS SPACE_MASK & (1ULL << uc)) != 0;
}

template <typename CharT>
MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 bool is_alpha(const CharT c) noexcept {
    const auto uc = static_cast<make_unsigned_t<CharT>>(c);
    if (uc > 127) return false;
    return (uc & 0xDF) >= 'A' && (uc & 0xDF) <= 'Z';
}

template <typename CharT>
MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 bool is_digit(const CharT c) noexcept {
    const auto uc = static_cast<make_unsigned_t<CharT>>(c);
    if (uc > 127) return false;
    return (uc & 0xF0) == 0x30 && (uc & 0x0F) <= 9;
}

template <typename CharT>
MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 bool is_xdigit(const CharT c) noexcept {
    const auto uc = static_cast<make_unsigned_t<CharT>>(c);
    if (uc > 127) return false;
    const bool is_09 = (uc & 0xF0) == 0x30 && (uc & 0x0F) <= 0x09;
    const bool is_AF = (uc & 0xF0) == 0x40 && (uc & 0x0F) >= 0x01 && (uc & 0x0F) <= 0x06;
    const bool is_af = (uc & 0xF0) == 0x60 && (uc & 0x0F) >= 0x01 && (uc & 0x0F) <= 0x06;
    return is_09 || is_AF || is_af;
}

template <typename CharT>
MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 bool is_alpha_or_digit(const CharT c) noexcept {
    return _MSTL is_alpha(c) || _MSTL is_digit(c);
}

template <typename CharT>
MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 bool is_digit_or_alpha(const CharT c) noexcept {
    return _MSTL is_digit(c) || _MSTL is_alpha(c);
}

MSTL_CONST_FUNCTION constexpr bool is_high_surrogate(const char16_t c) noexcept {
    return c >= 0xD800 && c <= 0xDBFF;
}

MSTL_CONST_FUNCTION constexpr bool is_low_surrogate(const char16_t c) noexcept {
    return c >= 0xDC00 && c <= 0xDFFF;
}

MSTL_CONST_FUNCTION constexpr uint32_t combine_surrogates(const char16_t high, const char16_t low) noexcept {
    return 0x10000 + ((static_cast<uint32_t>(high) - 0xD800) << 10) + (static_cast<uint32_t>(low) - 0xDC00);
}


template <typename CharT>
MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 CharT to_lowercase(const CharT c) noexcept {
    const auto uc = static_cast<make_unsigned_t<CharT>>(c);
    if (uc >= 'A' && uc <= 'Z') return static_cast<CharT>(uc | 0x20);
    return c;
}

template <typename CharT>
MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 CharT to_uppercase(const CharT c) noexcept {
    const auto uc = static_cast<make_unsigned_t<CharT>>(c);
    if (uc >= 'a' && uc <= 'z') return static_cast<CharT>(uc & 0xDF);
    return c;
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_STRING_CHAR_TYPES_HPP__
