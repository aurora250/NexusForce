#ifndef MSTL_CSTRING_HPP__
#define MSTL_CSTRING_HPP__
#include "../algorithm/mathlib.hpp"
#include "../utilities/undef_cmacro.hpp"
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
MSTL_PURE_FUNCTION constexpr bool is_ctype(const CharT c, uint64_t mask_low, uint64_t mask_high) noexcept {
	const auto uc = static_cast<make_unsigned_t<CharT>>(c);
	if (uc > 127) return false;

	if (uc <= 63) (mask_low & (1ULL << uc)) != 0;
	const auto offset = uc - 64;
	return (mask_high & (1ULL << offset)) != 0;
}

template <typename CharT>
MSTL_PURE_FUNCTION constexpr bool is_punct(const CharT c) noexcept {
	return _MSTL is_ctype(c, _CONSTANTS PUNCT_MASK_LOW, _CONSTANTS PUNCT_MASK_HIGH);
}

template <typename CharT>
MSTL_PURE_FUNCTION constexpr bool is_cntrl(const CharT c) noexcept {
	return _MSTL is_ctype(c, _CONSTANTS CNTRL_MASK_LOW, _CONSTANTS CNTRL_MASK_HIGH);
}

template <typename CharT>
MSTL_PURE_FUNCTION constexpr bool is_print(const CharT c) noexcept {
	const auto uc = static_cast<make_unsigned_t<CharT>>(c);
	return uc <= 127 && !_MSTL is_cntrl(c);
}

template <typename CharT>
MSTL_PURE_FUNCTION constexpr bool is_blank(const CharT c) noexcept {
	const auto uc = static_cast<make_unsigned_t<CharT>>(c);
	return uc < 64 && (_CONSTANTS BLANK_MASK & (1ULL << uc)) != 0;
}

template <typename CharT>
MSTL_PURE_FUNCTION constexpr bool is_graph(const CharT c) noexcept {
	return _MSTL is_print(c) && !_MSTL is_blank(c);
}

template <typename CharT>
MSTL_CONST_FUNCTION constexpr bool is_ascii(const CharT c) noexcept {
	const auto uc = static_cast<make_unsigned_t<CharT>>(c);
	return uc <= 127;
}

template <typename CharT>
MSTL_PURE_FUNCTION constexpr bool is_space(const CharT c) noexcept {
    const auto uc = static_cast<make_unsigned_t<CharT>>(c);
    return uc < 64 && (_CONSTANTS SPACE_MASK & (1ULL << uc)) != 0;
}

template <typename CharT>
MSTL_CONST_FUNCTION constexpr bool is_alpha(const CharT c) noexcept {
    const auto uc = static_cast<make_unsigned_t<CharT>>(c);
    if (uc > 127) return false;
    return (uc & 0xDF) >= 'A' && (uc & 0xDF) <= 'Z';
}

template <typename CharT>
MSTL_CONST_FUNCTION constexpr bool is_digit(const CharT c) noexcept {
    const auto uc = static_cast<make_unsigned_t<CharT>>(c);
    if (uc > 127) return false;
    return (uc & 0xF0) == 0x30 && (uc & 0x0F) <= 9;
}

template <typename CharT>
MSTL_CONST_FUNCTION constexpr bool is_xdigit(const CharT c) noexcept {
    const auto uc = static_cast<make_unsigned_t<CharT>>(c);
    if (uc > 127) return false;
    const bool is_09 = (uc & 0xF0) == 0x30 && (uc & 0x0F) <= 0x09;
    const bool is_AF = (uc & 0xF0) == 0x40 && (uc & 0x0F) >= 0x01 && (uc & 0x0F) <= 0x06;
    const bool is_af = (uc & 0xF0) == 0x60 && (uc & 0x0F) >= 0x01 && (uc & 0x0F) <= 0x06;
    return is_09 || is_AF || is_af;
}

template <typename CharT>
MSTL_CONST_FUNCTION constexpr bool is_alpha_or_digit(const CharT c) noexcept {
    return _MSTL is_alpha(c) || _MSTL is_digit(c);
}

template <typename CharT>
MSTL_CONST_FUNCTION constexpr bool is_digit_or_alpha(const CharT c) noexcept {
    return _MSTL is_digit(c) || _MSTL is_alpha(c);
}


template <typename T, enable_if_t<is_floating_point_v<T>, int> = 0>
MSTL_CONST_FUNCTION constexpr bool signbit(const T x) noexcept {
	using UInt = make_integer_t<sizeof(T), false>;

	const UInt bits = *reinterpret_cast<const UInt*>(&x);
	constexpr UInt sign_mask = static_cast<UInt>(1) << (8 * sizeof(UInt) - 1);
	return (bits & sign_mask) != 0;
}


template <typename T, enable_if_t<is_floating_point_v<T>, int> = 0>
MSTL_CONST_FUNCTION constexpr bool is_nan(const T x) noexcept {
	return x != x;
}

template <typename T, enable_if_t<is_floating_point_v<T>, int> = 0>
MSTL_CONST_FUNCTION constexpr bool is_infinity(const T x) noexcept {
	return x == numeric_limits<T>::infinity();
}

template <typename T, enable_if_t<is_floating_point_v<T>, int> = 0>
MSTL_CONST_FUNCTION constexpr bool is_pos_infinity(const T x) noexcept {
	return _MSTL is_infinity(x);
}

template <typename T, enable_if_t<is_floating_point_v<T>, int> = 0>
MSTL_CONST_FUNCTION constexpr bool is_neg_infinity(const T x) noexcept {
	return x == -numeric_limits<T>::infinity();
}

template <typename T, enable_if_t<is_floating_point_v<T>, int> = 0>
MSTL_CONST_FUNCTION constexpr bool is_finite(const T x) noexcept {
	return !_MSTL is_infinity(x) && !_MSTL is_nan(x);
}

template <typename T, enable_if_t<is_floating_point_v<T>, int> = 0>
MSTL_CONST_FUNCTION constexpr bool is_normal(const T x) noexcept {
	if (!_MSTL is_finite(x)) return false;
	if (x == 0) return false;
	return _MSTL absolute(x) >= numeric_limits<T>::min();
}

template <typename T, enable_if_t<is_floating_point_v<T>, int> = 0>
MSTL_CONST_FUNCTION constexpr bool is_subnormal(const T x) noexcept {
	if (!_MSTL is_finite(x)) return false;
	if (x == 0) return false;
	return _MSTL absolute(x) < numeric_limits<T>::min();
}

template <typename T, enable_if_t<is_floating_point_v<T>, int> = 0>
MSTL_CONST_FUNCTION constexpr bool is_positive(const T x) noexcept {
	return x > 0 || (x == 0 && !_MSTL signbit(x));
}

template <typename T, enable_if_t<is_floating_point_v<T>, int> = 0>
MSTL_CONST_FUNCTION constexpr bool is_negative(const T x) noexcept {
	return x < 0 || (x == 0 && _MSTL signbit(x));
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
MSTL_CONST_FUNCTION constexpr CharT to_lowercase(const CharT c) noexcept {
    const auto uc = static_cast<make_unsigned_t<CharT>>(c);
    if (uc >= 'A' && uc <= 'Z') return static_cast<CharT>(uc | 0x20);
    return c;
}

template <typename CharT>
MSTL_CONST_FUNCTION constexpr CharT to_uppercase(const CharT c) noexcept {
    const auto uc = static_cast<make_unsigned_t<CharT>>(c);
    if (uc >= 'a' && uc <= 'z') return static_cast<CharT>(uc & 0xDF);
    return c;
}


#ifdef MSTL_COMPILER_GNUC__
	#define MSTL_MEMORY_BARRIER(p) __asm__ volatile ("" : : "m" (*p) : "memory");
#elif defined(MSTL_COMPILER_MSVC__)
	#define MSTL_MEMORY_BARRIER(p) ::_ReadWriteBarrier();
#else
	#define MSTL_MEMORY_BARRIER(p)
#endif

#ifdef MSTL_COMPILER_MSVC__
namespace masm {
	// These functions are defined in memory.asm and be packaged in masm namespace
	extern "C" {
		void* __cdecl masm_memory_copy(void* dest, const void* src, size_t count);
		void* __cdecl masm_memory_copy_offset(void* dest, const void* src, size_t count);
		void* __cdecl masm_memory_char_copy(void* dest, const void* src, int chr, size_t count);
		int __cdecl masm_memory_compare(const void* lh, const void* rh, size_t count);
		int __cdecl masm_memory_compare_ignore_case(const void* ptr1, const void* rh, size_t count);
		void* __cdecl masm_memory_char(const void* dest, int value, size_t count);
		void* __cdecl masm_memory_move(void* dest, const void* src, size_t count);
		void* __cdecl masm_memory_set(void* dest, int value, size_t count);
		void __cdecl masm_memory_zero(void* dest, size_t count);
		void __cdecl masm_explicit_memory_zero(void* dest, size_t count);
		void* __cdecl masm_memory_in_memory(void* data, size_t data_len, const void* pattern, size_t pattern_len);
		void* __cdecl masm_memory_frobnicate(void* s, size_t n);
	}
}
#endif

// copy from source memory to destination memory with specific length.
// if any parameter pointer is nullptr, return nullptr.
// it`s similar with std::memcpy.
constexpr void* memory_copy(void* MSTL_RESTRICT dest, const void* MSTL_RESTRICT src, size_t count) noexcept {
	if (dest == nullptr || src == nullptr) return nullptr;
	if (count == 0) return dest;

	void* res = dest;
	auto dest_v = static_cast<volatile byte_t*>(dest);
	auto src_v = static_cast<const volatile byte_t*>(src);
	while (count--) {
		*dest_v = *src_v;
		dest_v++;
		src_v++;
	}
	return res;
}

// it`s similar with std::mempcpy
constexpr void* memory_copy_offset(void* MSTL_RESTRICT dest, const void* MSTL_RESTRICT src, size_t count) noexcept {
	if (dest == nullptr || src == nullptr) return nullptr;

	auto dest_v = static_cast<volatile byte_t*>(dest);
	auto src_v = static_cast<const volatile byte_t*>(src);
	while (count--) {
		*dest_v = *src_v;
		dest_v++;
		src_v++;
	}
	return (void*) dest_v;
}

// copy from source memory to destination memory with specific length if not encounter target character.
// if any parameter pointer is nullptr, return nullptr.
// it`s similar with std::memccpy.
constexpr void* memory_char_copy(void* dest, const void* src, const int chr, size_t count) noexcept {
    if (dest == nullptr || src == nullptr) return nullptr;
    const auto target = static_cast<byte_t>(chr);
    auto dest_v = static_cast<volatile byte_t*>(dest);
    auto src_v = static_cast<const volatile byte_t*>(src);

    while (count--) {
        const byte_t current = *src_v;
        *dest_v = current;
        if (current == target) {
            return (void*) (++dest_v);
        }
        dest_v++;
        src_v++;
    }
    return nullptr;
}

// compare with left-hand memory and right-hand memory in a specific length.
// return a positive number when left-hand memory is greater, a negative number when right-hand memory is greater
// and return zero when they are equal in specific length.
// it`s similar with std::memcmp.
MSTL_PURE_FUNCTION constexpr int memory_compare(const void* lh, const void* rh, size_t count) noexcept {
	if (lh == nullptr && rh == nullptr) return 0;
	if (lh == nullptr) return -1;
    if (rh == nullptr) return 1;

	while (count--) {
		if (*static_cast<const byte_t*>(lh) != *static_cast<const byte_t*>(rh))
			return *static_cast<const byte_t*>(lh) - *static_cast<const byte_t*>(rh);
		lh = static_cast<const byte_t*>(lh) + 1;
		rh = static_cast<const byte_t*>(rh) + 1;
	}
	return 0;
}

// compare with left-hand memory and right-hand memory in a specific length but ignored case of characters.
// return a positive number when left-hand memory is greater, a negative number when right-hand memory is greater
// and return zero when they are equal in specific length.
// it`s similar with std::memicmp.
MSTL_PURE_FUNCTION MSTL_CONSTEXPR20 int memory_compare_ignore_case(
	const void* lh, const void *rh, const size_t count) noexcept {
	if ((lh == nullptr && rh == nullptr) || count == 0) return 0;
	if (lh == nullptr) return -1;
	if (rh == nullptr) return 1;

	const auto lh_v = static_cast<const volatile byte_t*>(lh);
	const auto rh_v = static_cast<const volatile byte_t*>(rh);

    for (size_t i = 0; i < count; ++i) {
        const byte_t c1 = static_cast<byte_t>(_MSTL to_lowercase(static_cast<char>(lh_v[i])));
        const byte_t c2 = static_cast<byte_t>(_MSTL to_lowercase(static_cast<char>(rh_v[i])));
        if (c1 != c2) {
            return static_cast<int>(c1) - static_cast<int>(c2);
        }
    }
    return 0;
}

// return a pointer which is pointing to the first place that equal to target value in a specific length.
// if parameter pointer is nullptr, return nullptr. if not found, return nullptr.
// it`s similar with std::memchr.
MSTL_PURE_FUNCTION constexpr void* memory_char(const void* dest, const int value, size_t count) noexcept {
	if(dest == nullptr) return nullptr;
	auto p = static_cast<const byte_t *>(dest);
	while (count--) {
		if (*p == static_cast<byte_t>(value))
			return const_cast<byte_t*>(p);
		p++;
	}
	return nullptr;
}

// if any parameter pointer is nullptr, return nullptr.
// it`s similar with std::memmove.
constexpr void* memory_move(void* dest, const void* src, size_t count) noexcept {
	if(dest == nullptr || src == nullptr) return nullptr;

	void* res = dest;
	auto dest_v = static_cast<volatile byte_t*>(dest);
	auto src_v = static_cast<const volatile byte_t*>(src);
	if (dest_v < src_v) {
		while (count--) {
			*dest_v = *src_v;
			dest_v = dest_v + 1;
			src_v = src_v + 1;
		}
	} else if (dest_v > src_v) {
		while (count--) {
			*(dest_v + count) = *(src_v + count);
		}
	}
	return res;
}

// fill the destination memory with target value in the specific length.
// if parameter pointer is nullptr, return nullptr.
// it`s similar with std::memset.
constexpr void* memory_set(void* dest, const int value, size_t count) noexcept {
	if(dest == nullptr) return nullptr;

	void* ret = static_cast<byte_t *>(dest);
	auto dest_v = static_cast<volatile byte_t *>(dest);
	while (count--) {
		*dest_v = static_cast<byte_t>(value);
		dest_v = dest_v + 1;
	}
	return ret;
}

// clear the destination memory with zero in the specific length.
// if parameter pointer is nullptr, do nothing.
// it`s similar with std::bzero.
constexpr void memory_zero(void* dest, const size_t count) noexcept {
	if (dest == nullptr) return;

	const auto dest_v = static_cast<volatile byte_t*>(dest);
	for (size_t i = 0; i < count; ++i) {
		dest_v[i] = 0;
	}
}

// std::memmem
constexpr void* memory_in_memory(const void * data, const size_t data_len,
	const void* pattern, const size_t pattern_len) noexcept {
	if (data == nullptr || pattern == nullptr || data_len == 0 || pattern_len == 0 || pattern_len > data_len) {
		return nullptr;
	}
	const auto data_ptr = static_cast<const byte_t*>(data);
	const auto pattern_ptr = static_cast<const byte_t*>(pattern);
	const size_t last_possible = data_len - pattern_len + 1;

	for (size_t i = 0; i < last_possible; ++i) {
		if (data_ptr[i] == pattern_ptr[0]) {
			bool match = true;
			for (size_t j = 1; j < pattern_len; ++j) {
				if (data_ptr[i + j] != pattern_ptr[j]) {
					match = false;
					break;
				}
			}
			if (match) {
				return const_cast<byte_t*>(data_ptr + i);
			}
		}
	}
	return nullptr;
}

// std::memfrob
constexpr void* memory_frobnicate(void* s, const size_t n) noexcept {
	if (s == nullptr || n == 0) return s;
	const auto s_v = static_cast<volatile byte_t*>(s);
	for (size_t i = 0; i < n; i++) {
		s_v[i] ^= 42;
	}
	return s;
}


// copy from source string to destination string.
// if any parameter pointer is nullptr, return nullptr.
// it`s similar with std::strcpy.
constexpr char* string_copy(char* MSTL_RESTRICT dest, const char* MSTL_RESTRICT src) noexcept {
	if(dest == nullptr || src == nullptr) return nullptr;
	char* ret = dest;
	while (*src != '\0') {
		*dest = *src;
		dest++;
		src++;
	}
	*dest = *src;
	return ret;
}

// stpcpy
constexpr char* string_copy_offset(char* MSTL_RESTRICT dest, const char* MSTL_RESTRICT src) noexcept {
	if(dest == nullptr || src == nullptr) return nullptr;
	while (*src != '\0') {
		*dest = *src;
		dest++;
		src++;
	}
	*dest = *src;
	return dest - 1;
}

// concatenate source string to the tail of destination string.
// if any parameter pointer is nullptr, return nullptr.
// it`s similar with std::strcat.
constexpr char* string_concatenate(char* MSTL_RESTRICT dest, const char* MSTL_RESTRICT src) noexcept {
	if (dest == nullptr || src == nullptr) return nullptr;
	char* original_dest = dest;
	while (*dest != '\0')
		++dest;

	while (*src != '\0') {
		*dest = *src;
		++dest;
		++src;
	}
	*dest = '\0';
	return original_dest;
}

// compare with left-hand string and right-hand string in a specific length.
// return a positive number when left-hand string is greater, a negative number when right-hand string is greater
// and return zero when they are equal in specific length.
// it`s similar with std::strcmp.
MSTL_PURE_FUNCTION constexpr int string_compare(const char* dest, const char* src) noexcept {
	if (dest == nullptr && src == nullptr) return 0;
	if (dest == nullptr) return -1;
	if (src == nullptr) return 1;

	while (*dest == *src) {
		if (*dest == '\0') return 0;
		dest++;
		src++;
	}
	if (*dest > *src) return 1;
	return -1;
}

// compare with left-hand string and right-hand string in a specific length but ignored case of characters.
// return a positive number when left-hand string is greater, a negative number when right-hand string is greater
// and return zero when they are equal in specific length.
// it`s similar with std::stricmp.
MSTL_PURE_FUNCTION constexpr int string_compare_ignore_case(const char* s1, const char* s2) {
	if (s1 == nullptr && s2 == nullptr) return 0;
	if (s1 == nullptr) return -1;
	if (s2 == nullptr) return 1;

	while (*s1 && *s2) {
		const char c1 = _MSTL to_lowercase(*s1);
		const char c2 = _MSTL to_lowercase(*s2);
		if (c1 < c2) return -1;
		if (c1 > c2) return 1;
		++s1;
		++s2;
	}
	return *s1 == *s2 ? 0 : *s1 < *s2 ? -1 : 1;
}

MSTL_PURE_FUNCTION constexpr int string_compare_natural(const char* s1, const char* s2) noexcept {
	if (s1 == nullptr && s2 == nullptr) return 0;
	if (s1 == nullptr) return -1;
	if (s2 == nullptr) return 1;

	while (*s1 != '\0' && *s2 != '\0') {
		if (_MSTL is_digit(*s1) && _MSTL is_digit(*s2)) {
			const char* s1_skip_zero = s1;
			while (*s1_skip_zero == '0' && _MSTL is_digit(*(s1_skip_zero + 1))) {
				s1_skip_zero++;
			}
			const char* s2_skip_zero = s2;
			while (*s2_skip_zero == '0' && _MSTL is_digit(*(s2_skip_zero + 1))) {
				s2_skip_zero++;
			}

			const char* s1_end = s1_skip_zero;
			while (_MSTL is_digit(*s1_end)) s1_end++;
			const size_t len1 = s1_end - s1_skip_zero;

			const char* s2_end = s2_skip_zero;
			while (_MSTL is_digit(*s2_end)) s2_end++;
			const size_t len2 = s2_end - s2_skip_zero;

			if (len1 != len2) {
				return len1 > len2 ? 1 : -1;
			}
			for (size_t i = 0; i < len1; i++) {
				if (s1_skip_zero[i] != s2_skip_zero[i]) {
					return s1_skip_zero[i] - s2_skip_zero[i];
				}
			}
			s1 = s1_end;
			s2 = s2_end;
			continue;
		}

		if (*s1 != *s2)
			return *s1 - *s2;
		s1++;
		s2++;
	}
	return *s1 - *s2;
}

// return the length of string when the loop encounter '\0'
// it`s similar with std::strlen.
MSTL_PURE_FUNCTION constexpr size_t string_length(const char* str) noexcept {
	const char* p = str;
	while (*p != '\0')
		++p;
	return static_cast<size_t>(p - str);
}

// return a pointer which is pointing to the first place that equal to target character.
// if parameter pointer is nullptr, return nullptr. if not found, return nullptr.
// it`s similar with std::strchr.
MSTL_PURE_FUNCTION constexpr const char* string_char(const char* str, const char chr) noexcept {
	if (str == nullptr) return nullptr;
	while (*str != '\0') {
		if (*str == static_cast<char>(chr))
			return str;
		++str;
	}

	if (*str == static_cast<char>(chr))
		return str;
	return nullptr;
}

// return a pointer which is pointing to the last place that equal to target character.
// if parameter pointer is nullptr, return nullptr. if not found, return nullptr.
// it`s similar with std::strchr.
MSTL_PURE_FUNCTION constexpr const char* string_last_char(const char* str, const char chr) noexcept {
	if (str == nullptr) return nullptr;
	const char* last = nullptr;

	while (*str != '\0') {
		if (*str == static_cast<char>(chr))
			last = str;
		++str;
	}
	return last;
}

// return the index which is pointing to the last place that equal to target character.
// if any parameter pointer is nullptr, return zero. if not found, return nullptr.
// it`s similar with std::strspn.
MSTL_PURE_FUNCTION constexpr size_t string_span_in(const char* str, const char* accept) noexcept {
	if (str == nullptr || *str == '\0' || accept == nullptr || *accept == '\0')
		return 0;

	const char* original_str = str;
	while (*str != '\0') {
		const char* a = accept;
		bool found = false;
		while (*a != '\0') {
			if (*str == *a) {
				found = true;
				break;
			}
			++a;
		}
		if (!found)
			return static_cast<size_t>(str - original_str);
		++str;
	}
	return static_cast<size_t>(str - original_str);
}

MSTL_PURE_FUNCTION constexpr size_t string_span_not_in(const char* str, const char* reject) noexcept {
	if (str == nullptr || *str == '\0') return 0;
	if (reject == nullptr || *reject == '\0') {
		size_t len = 0;
		while (str[len] != '\0') ++len;
		return len;
	}

	const char* original_str = str;
	while (*str != '\0') {
		const char* r = reject;
		while (*r != '\0') {
			if (*str == *r)
				return static_cast<size_t>(str - original_str);
			++r;
		}
		++str;
	}
	return static_cast<size_t>(str - original_str);
}

constexpr char* string_to_lowercase(char* str) noexcept {
	if (str == nullptr) return nullptr;

	constexpr size_t diff = 'a' - 'A';
	char* original = str;
	while (*str != '\0') {
		if (*str >= 'A' && *str <= 'Z')
			*str = static_cast<char>(*str + diff);
		++str;
	}
	return original;
}

constexpr char* string_to_uppercase(char* str) noexcept {
	if (str == nullptr) return nullptr;

	constexpr size_t diff = 'a' - 'A';
	char* original = str;
	while (*str != '\0') {
		if (*str >= 'a' && *str <= 'z')
			*str = static_cast<char>(*str - diff);
		++str;
	}
	return original;
}

// same to std::strpbrk
MSTL_PURE_FUNCTION constexpr char* string_find_any(char* str, const char* accept) noexcept {
	if (str == nullptr || *str == '\0' || accept == nullptr || *accept == '\0')
		return nullptr;

	while (*str != '\0') {
		const char* a = accept;
		while (*a != '\0') {
			if (*str == *a)
				return str;
			++a;
		}
		++str;
	}
	return nullptr;
}

MSTL_PURE_FUNCTION constexpr const char* string_in_string(const char* dest, const char* src) noexcept {
	if(dest == nullptr || src == nullptr) return nullptr;
	const char* cur = dest;
	while (*cur) {
		const char *str1 = cur;
		const char *str2 = src;
		while (*str1 && *str2 && *str1 == *str2) {
			str1++;
			str2++;
		}
		if (*str2 == '\0') return cur;
		cur++;
	}
	return nullptr;
}

MSTL_PURE_FUNCTION constexpr const char* string_in_string_ignored_case(const char* dest, const char* src) noexcept {
	if (dest == nullptr || src == nullptr) return nullptr;
	if (*src == '\0') return dest;

	const char* cur = dest;
	while (*cur) {
		const char* str1 = cur;
		const char* str2 = src;
		while (*str1 && *str2) {
			const char c1 = _MSTL to_lowercase(*str1);
			const char c2 = _MSTL to_lowercase(*str2);
			if (c1 != c2) break;
			str1++;
			str2++;
		}
		if (*str2 == '\0') return cur;
		cur++;
	}
	return nullptr;
}

// strsep
MSTL_CONSTEXPR23 char* string_token(char* MSTL_RESTRICT str, const char* MSTL_RESTRICT delimiters) {
	static char* next_token = nullptr;
	if (str != nullptr) next_token = str;

	if (next_token == nullptr) return nullptr;
	while (*next_token != '\0' && string_char(delimiters, *next_token) != nullptr) {
		++next_token;
	}
	if (*next_token == '\0') {
		next_token = nullptr;
		return nullptr;
	}

	char* token = next_token;
	while (*next_token != '\0' && string_char(delimiters, *next_token) == nullptr)
		++next_token;
	if (*next_token != '\0') {
		*next_token = '\0';
		++next_token;
	}
	else next_token = nullptr;
	return token;
}

MSTL_CONSTEXPR23 char* string_last_token(char* MSTL_RESTRICT str, const char* MSTL_RESTRICT delimiters) {
	static char* prev_token = nullptr;
	static char* original_str = nullptr;

	if (str != nullptr) {
		prev_token = nullptr;
		original_str = str;
		if (original_str != nullptr) {
			char* end = original_str;
			while (*end != '\0') ++end;
			prev_token = end;
		}
	}
	if (prev_token == nullptr || prev_token == original_str)
		return nullptr;

	char* token_end = prev_token;
	while (token_end > original_str) {
		--token_end;
		if (string_char(delimiters, *token_end) == nullptr)
			break;
	}
	if (token_end == original_str && string_char(delimiters, *token_end) != nullptr) {
		prev_token = original_str;
		return nullptr;
	}

	char* token_start = token_end;
	while (token_start > original_str && string_char(delimiters, *(token_start - 1)) == nullptr)
		--token_start;

	prev_token = token_start;
	if (token_start > original_str)
		*(token_start - 1) = '\0';
	else
		prev_token = original_str;
	return token_start;
}

constexpr char* string_set(char* str, const char value) noexcept {
	if (str == nullptr) return nullptr;
	char* original = str;
	while (*str != '\0') {
		*str = value;
		++str;
	}
	return original;
}

constexpr char* string_reverse(char* str) noexcept {
	if (str == nullptr || *str == '\0') return str;

	char* end = str;
	while (*end != '\0') ++end;
	--end;
	while (str < end) {
		const char temp = *str;
		*str = *end;
		*end = temp;
		++str;
		--end;
	}
	return str;
}


constexpr char* string_n_copy(char* MSTL_RESTRICT dest, const char* MSTL_RESTRICT src, const size_t count) noexcept {
	if (dest == nullptr || src == nullptr) return nullptr;
	char* ret = dest;
	size_t i = 0;
	while (i < count && *src != '\0') {
		*dest = *src;
		dest++;
		src++;
		i++;
	}
	while (i < count) {
		*dest = '\0';
		dest++;
		i++;
	}
	return ret;
}

// stpncpy
constexpr char* string_n_copy_offset(char* MSTL_RESTRICT dest, const char* MSTL_RESTRICT src, const size_t count) noexcept {
	if (dest == nullptr || src == nullptr) return nullptr;
	size_t i = 0;
	while (i < count && *src != '\0') {
		*dest = *src;
		dest++;
		src++;
		i++;
	}
	while (i < count) {
		*dest = '\0';
		dest++;
		i++;
	}
	return dest;
}

constexpr char* string_n_concatenate(char* MSTL_RESTRICT dest,
    const char* MSTL_RESTRICT src, const size_t count) noexcept {
	if (dest == nullptr || src == nullptr) return nullptr;

	char* original_dest = dest;
	while (*dest != '\0')
		++dest;
	size_t copied = 0;
	while (*src != '\0' && copied < count) {
		*dest = *src;
		++dest;
		++src;
		++copied;
	}
	*dest = '\0';
	return original_dest;
}

MSTL_PURE_FUNCTION constexpr int string_n_compare(
    const char* dest, const char* src, const size_t count) noexcept {
	if (dest == nullptr && src == nullptr) return 0;
	if (dest == nullptr) return -1;
	if (src == nullptr) return 1;

	if (count == 0) return 0;
	size_t i = 0;
	while (*dest == *src && *dest != '\0' && i < count - 1) {
		++dest;
		++src;
		++i;
	}
	if (i == count - 1) return 0;
	return *dest < *src ? -1 : *dest > *src ? 1 : 0;
}

MSTL_PURE_FUNCTION constexpr int string_n_compare_ignore_case(
    const char* s1, const char* s2, const size_t count) noexcept {
	if ((s1 == nullptr && s2 == nullptr) || count == 0) return 0;
	if (s1 == nullptr) return -1;
	if (s2 == nullptr) return 1;

	size_t i = 0;
	while (*s1 && *s2 && i < count - 1) {
		const char c1 = _MSTL to_lowercase(*s1);
		const char c2 = _MSTL to_lowercase(*s2);
		if (c1 < c2) return -1;
		if (c1 > c2) return 1;
		++s1;
		++s2;
		++i;
	}
	if (i == count - 1) return 0;

	const char c1 = _MSTL to_lowercase(*s1);
	const char c2 = _MSTL to_lowercase(*s2);
	return c1 < c2 ? -1 : c1 > c2 ? 1 : 0;
}

MSTL_PURE_FUNCTION constexpr size_t string_n_length(
    const char* str, const size_t max_len) noexcept {
	const char* p = str;
	ptrdiff_t len = 0;
	while (*p != '\0' && len < max_len) {
		++p;
		++len;
	}
	return len;
}

constexpr char* string_n_set(char* str,
    const char value, const size_t count) noexcept {
	if (str == nullptr || count == 0) return str;
	char* original = str;
	size_t processed = 0;
	while (*str != '\0' && processed < count) {
		*str = value;
		++str;
		++processed;
	}
	return original;
}


// strlcpy
constexpr size_t string_copy_safe(char* MSTL_RESTRICT dest,
    const char* MSTL_RESTRICT src, const size_t size) noexcept {
	if (dest == nullptr || src == nullptr || size == 0) {
		return src != nullptr ? string_length(src) : 0;
	}
	size_t src_length = 0;
	const char* src_ptr = src;

	while (*src_ptr != '\0' && src_length < size - 1) {
		*dest = *src_ptr;
		dest++;
		src_ptr++;
		src_length++;
	}
	if (size > 0) *dest = '\0';

	while (*src_ptr != '\0') {
		src_ptr++;
		src_length++;
	}
	return src_length;
}

constexpr size_t string_concatenate_safe(char* MSTL_RESTRICT dest,
    const char* MSTL_RESTRICT src, const size_t size) noexcept {
	const size_t src_size_len = string_n_length(src, size);
	if (dest == nullptr || src == nullptr || size == 0) {
		return src != nullptr ? src_size_len : 0;
	}

    const size_t dest_length = string_n_length(dest, size);
	if (dest_length >= size) {
		return dest_length + src_size_len;
	}

	size_t remaining = size - dest_length - 1;
	const size_t src_length = string_n_length(src, remaining);

	if (src_length > 0) {
		char* dest_ptr = dest + dest_length;
		const char* src_ptr = src;

		while (*src_ptr != '\0' && remaining > 0) {
			*dest_ptr = *src_ptr;
			dest_ptr++;
			src_ptr++;
			remaining--;
		}
		*dest_ptr = '\0';
	}
	return dest_length + src_size_len;
}


constexpr wchar_t* wchar_memory_copy(wchar_t* dest, const wchar_t* src, size_t count) noexcept {
	if(dest == nullptr || src == nullptr) return nullptr;
	wchar_t* res = dest;
	while (count--) {
		*dest = *src;
		++dest;
		++src;
	}
	return res;
}

constexpr int wchar_memory_compare(const wchar_t* dest, const wchar_t* src, size_t count) noexcept {
	if (dest == nullptr && src == nullptr) return 0;
	if (dest == nullptr) return -1;
	if (src == nullptr) return 1;

	while (count--) {
		if (*dest != *src) return *dest - *src;
		++dest;
		++src;
	}
	return 0;
}

constexpr wchar_t* wchar_memory_char(const wchar_t* dest, const wchar_t value, size_t count) noexcept {
	if(dest == nullptr) return nullptr;
	const wchar_t* p = dest;
	while (count--) {
		if (*p == value)
			return const_cast<wchar_t*>(p);
		p++;
	}
	return nullptr;
}

constexpr wchar_t* wchar_memory_move(wchar_t* dest, const wchar_t* src, const size_t count) noexcept {
	if(dest == nullptr || src == nullptr) return nullptr;
	return static_cast<wchar_t *>(memory_move(dest, src, count * sizeof(wchar_t)));
}

constexpr wchar_t* wchar_memory_set(wchar_t* dest, const wchar_t value, size_t count) noexcept {
	if(dest == nullptr) return nullptr;
	wchar_t* ret = dest;
	while (count--) {
		*dest = value;
		dest++;
	}
	return ret;
}


constexpr ptrdiff_t wstring_length(const wchar_t* str) noexcept {
	const wchar_t* p = str;
	while (*p != L'\0') ++p;
	return p - str;
}

#ifdef MSTL_STANDARD_20__
constexpr ptrdiff_t u8string_length(const char8_t* str) noexcept {
	const char8_t* p = str;
	while (*p != u8'\0')
		++p;
	return p - str;
}
#endif

constexpr ptrdiff_t u16string_length(const char16_t* str) noexcept {
	const char16_t* p = str;
	while (*p != u'\0')
		++p;
	return p - str;
}

constexpr ptrdiff_t u32string_length(const char32_t* str) noexcept {
	const char32_t* p = str;
	while (*p != U'\0')
		++p;
	return p - str;
}


MSTL_BEGIN_INNER__

template <typename T, enable_if_t<is_signed_v<T>, int> = 0>
constexpr T str_to_ints(const char* str, char** endptr, int base) {
    if (str == nullptr) {
        if (endptr) *endptr = const_cast<char*>(str);
        return 0;
    }

    const char* p = str;
    while (is_space(*p)) p++;
    const char* start_conversion = p;

    int sign = 1;
    if (*p == '+') {
        p++;
    } else if (*p == '-') {
        sign = -1;
        p++;
    }

    if (base != 0 && (base < 2 || base > 36)) {
        if (endptr) *endptr = const_cast<char*>(start_conversion);
        return 0;
    }

    if (base == 0) {
        if (*p == '0') {
            if (*(p + 1) == 'x' || *(p + 1) == 'X') {
                base = 16;
                p += 2;
            } else {
                base = 8;
            }
        } else {
            base = 10;
        }
    }

    if (base == 16 && *p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X')) {
        p += 2;
    }

    const T cutoff = numeric_limits<T>::min() / base;
    const T cutlim = numeric_limits<T>::min() % base;
    T result = 0;
    bool any_converted = false;
    bool overflow = false;

    while (*p) {
        int digit = 0;
        if (*p >= '0' && *p <= '9') {
            digit = *p - '0';
        } else if (*p >= 'a' && *p <= 'z') {
            digit = *p - 'a' + 10;
        } else if (*p >= 'A' && *p <= 'Z') {
            digit = *p - 'A' + 10;
        } else {
            break;
        }
        if (digit >= base) break;

        any_converted = true;

        if (!overflow) {
            if (result < cutoff || (result == cutoff && digit > -cutlim)) {
                overflow = true;
            } else {
                result = result * base - digit;
            }
        }
        p++;
    }

    if (endptr) {
        if (any_converted) {
            *endptr = const_cast<char*>(p);
        } else {
            *endptr = const_cast<char*>(start_conversion);
        }
    }

    if (!any_converted) return 0;
    if (overflow)
        return (sign > 0) ? numeric_limits<T>::max() : numeric_limits<T>::min();

    if (sign > 0) {
        if (result == numeric_limits<T>::min())
            return numeric_limits<T>::max();
        return -result;
    }
    return result;
}

template <typename T, enable_if_t<is_unsigned_v<T>, int> = 0>
constexpr T str_to_uints(const char* str, char** endptr, int base) {
    if (str == nullptr) {
        if (endptr) *endptr = const_cast<char*>(str);
        return 0;
    }

    const char* p = str;
    while (is_space(*p)) p++;
    const char* start_conversion = p;

    int sign = 1;
    if (*p == '+') {
        p++;
    } else if (*p == '-') {
        sign = -1;
        p++;
    }

    if (base != 0 && (base < 2 || base > 36)) {
        if (endptr) *endptr = const_cast<char*>(start_conversion);
        return 0;
    }

    if (base == 0) {
        if (*p == '0') {
            if (*(p + 1) == 'x' || *(p + 1) == 'X') {
                base = 16;
                p += 2;
            } else {
                base = 8;
            }
        } else {
            base = 10;
        }
    }

    if (base == 16 && *p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X')) {
        p += 2;
    }

    const T cutoff = numeric_limits<T>::max() / base;
    const T cutlim = numeric_limits<T>::max() % base;
    T result = 0;
    bool any_converted = false;
    bool overflow = false;

    while (*p) {
        unsigned int digit = 0;
        if (*p >= '0' && *p <= '9') {
            digit = *p - '0';
        } else if (*p >= 'a' && *p <= 'z') {
            digit = *p - 'a' + 10;
        } else if (*p >= 'A' && *p <= 'Z') {
            digit = *p - 'A' + 10;
        } else {
            break;
        }
        if (digit >= static_cast<unsigned int>(base)) break;

        any_converted = true;

        if (!overflow) {
            if (result > cutoff || (result == cutoff && digit > cutlim)) {
                overflow = true;
            } else {
                result = result * base + digit;
            }
        }
        p++;
    }

    if (endptr) {
        if (any_converted) {
            *endptr = const_cast<char*>(p);
        } else {
            *endptr = const_cast<char*>(start_conversion);
        }
    }

    if (!any_converted) return 0;
    if (overflow) return numeric_limits<T>::max();

    if (sign < 0)
        return static_cast<T>(-static_cast<make_signed_t<T>>(result));
    return result;
}

template <typename T>
MSTL_CONST_FUNCTION constexpr T fast_pow10(int exp) {
    constexpr T pow10_table[] = {
        1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9,
        1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19,
        1e20, 1e21, 1e22, 1e23, 1e24, 1e25, 1e26, 1e27, 1e28, 1e29,
        1e30, 1e31, 1e32
    };

    constexpr T neg_pow10_table[] = {
        1e0, 1e-1, 1e-2, 1e-3, 1e-4, 1e-5, 1e-6, 1e-7, 1e-8, 1e-9,
        1e-10, 1e-11, 1e-12, 1e-13, 1e-14, 1e-15, 1e-16, 1e-17, 1e-18, 1e-19,
        1e-20, 1e-21, 1e-22, 1e-23, 1e-24, 1e-25, 1e-26, 1e-27, 1e-28, 1e-29,
        1e-30, 1e-31, 1e-32
    };
    constexpr int max_table_exp = 32;

    if (exp >= 0 && exp <= max_table_exp) {
        return pow10_table[exp];
    }
    if (exp < 0 && -exp <= max_table_exp) {
	    return neg_pow10_table[-exp];
    }
    return static_cast<T>(_MSTL power(10.0, exp));
}

template <typename T>
constexpr T str_to_floats(const char* str, char** endptr) {
    if (str == nullptr) {
        if (endptr) *endptr = const_cast<char*>(str);
        return static_cast<T>(0);
    }

    const char* p = str;
    while (_MSTL is_space(*p)) p++;
    const char* start_conversion = p;

    int sign = 1;
    if (*p == '+') {
        p++;
    } else if (*p == '-') {
        sign = -1;
        p++;
    }

    if (_MSTL to_lowercase(*p) == 'i') {
        if (_MSTL string_n_compare_ignore_case(p, "inf", 3) == 0) {
            p += 3;
            if (_MSTL string_n_compare_ignore_case(p, "inity", 5) == 0) {
                p += 5;
            }
            if (endptr) *endptr = const_cast<char*>(p);
            return sign * numeric_limits<T>::max();
        }
    } else if (_MSTL to_lowercase(*p) == 'n') {
        if (_MSTL string_n_compare_ignore_case(p, "nan", 3) == 0) {
            p += 3;
            if (*p == '(') {
                while (*p != '\0' && *p != ')') p++;
                if (*p == ')') p++;
            }
            if (endptr) *endptr = const_cast<char*>(p);
            return numeric_limits<T>::quiet_nan();
        }
    }

    T significand = 0;
    int exponent = 0;
    int digits_count = 0;
    bool has_digits = false;

    while (*p >= '0' && *p <= '9') {
        has_digits = true;
        if (digits_count < numeric_limits<T>::digits10) {
            significand = significand * 10 + (*p - '0');
        } else {
            exponent++;
        }
        digits_count++;
        p++;
    }

    if (*p == '.') {
        p++;
        while (*p >= '0' && *p <= '9') {
            has_digits = true;
            if (digits_count < numeric_limits<T>::digits10) {
                significand = significand * 10 + (*p - '0');
                exponent--;
            }
            digits_count++;
            p++;
        }
    }

    if (!has_digits) {
        if (endptr) *endptr = const_cast<char*>(start_conversion);
        return static_cast<T>(0);
    }

    if (*p == 'e' || *p == 'E') {
        p++;
        int exp_sign = 1;
        if (*p == '+') {
            p++;
        } else if (*p == '-') {
            exp_sign = -1;
            p++;
        }

        if (*p >= '0' && *p <= '9') {
            int exp_val = 0;
            while (*p >= '0' && *p <= '9') {
                if (exp_val < 10000) {
                    exp_val = exp_val * 10 + (*p - '0');
                }
                p++;
            }
            exponent += exp_sign * exp_val;
        } else {
            p--;
            if (*p == '+' || *p == '-') p--;
        }
    }

    T result = significand;

    if (exponent != 0) {
        if (exponent > 400) {
            if (endptr) *endptr = const_cast<char*>(p);
            return sign * numeric_limits<T>::max();
        } else if (exponent < -400) {
            if (endptr) *endptr = const_cast<char*>(p);
            return T(0);
        } else {
            result *= fast_pow10<T>(exponent);
        }
    }

    result *= sign;
    const T inf = numeric_limits<T>::infinity();
    if (inf == result || -inf == result) {
        result = sign * numeric_limits<T>::max();
    }

    if (endptr) *endptr = const_cast<char*>(p);
    return result;
}

MSTL_END_INNER__


constexpr int64_t strtoll(const char* str, char** endptr, const int base) {
    return _INNER str_to_ints<int64_t>(str, endptr, base);
}
constexpr long strtol(const char* str, char** endptr, const int base) {
    return _INNER str_to_ints<long>(str, endptr, base);
}

constexpr uint64_t strtoull(const char* str, char** endptr, const int base) {
    return _INNER str_to_uints<uint64_t>(str, endptr, base);
}
constexpr unsigned long strtoul(const char* str, char** endptr, const int base) {
    return _INNER str_to_uints<unsigned long>(str, endptr, base);
}

constexpr float strtof(const char* str, char** endptr) {
	return _INNER str_to_floats<float>(str, endptr);
}
constexpr double strtod(const char* str, char** endptr) {
	return _INNER str_to_floats<double>(str, endptr);
}
constexpr long double strtold(const char* str, char** endptr) {
	return _INNER str_to_floats<long double>(str, endptr);
}


MSTL_NODISCARD constexpr float32_t to_float32(const char* str, size_t* idx = nullptr) {
    char* endptr = nullptr;
    const float32_t num = _MSTL strtof(str, &endptr);
    if(str == endptr) Exception(TypeCastError("Convert from string failed."));

    if (idx) *idx = static_cast<size_t>(endptr - str);
    return num;
}

MSTL_NODISCARD constexpr float64_t to_float64(const char* str, size_t* idx = nullptr) {
    char* endptr = nullptr;
    const float64_t num = _MSTL strtod(str, &endptr);
    if(str == endptr) Exception(TypeCastError("Convert from string failed."));

    if (idx) *idx = static_cast<size_t>(endptr - str);
    return num;
}

MSTL_NODISCARD constexpr decimal_t to_decimal(const char* str, size_t* idx = nullptr) {
    char* endptr = nullptr;
    const decimal_t num = _MSTL strtold(str, &endptr);
    if(str == endptr) Exception(TypeCastError("Convert from string failed."));

    if (idx) *idx = static_cast<size_t>(endptr - str);
    return num;
}

MSTL_NODISCARD constexpr int64_t to_int64(const char* str, size_t* idx = nullptr, const int base = 10) {
    char* endptr = nullptr;
    const int64_t num = _MSTL strtoll(str, &endptr, base);
    if(str == endptr) Exception(TypeCastError("Convert from string failed."));

    if (idx) *idx = static_cast<size_t>(endptr - str);
    return num;
}

MSTL_NODISCARD constexpr uint64_t to_uint64(const char* str, size_t* idx = nullptr, const int base = 10) {
    char* endptr = nullptr;
    const uint64_t num = _MSTL strtoull(str, &endptr, base);
    if(str == endptr) Exception(TypeCastError("Convert from string failed."));

    if (idx) *idx = static_cast<size_t>(endptr - str);
    return num;
}

MSTL_NODISCARD constexpr int32_t to_int32(const char* str, size_t* idx = nullptr, const int base = 10) {
    char* endptr = nullptr;
    const int32_t num = _MSTL strtol(str, &endptr, base);
    if(str == endptr) Exception(TypeCastError("Convert from string failed."));

    if (idx) *idx = static_cast<size_t>(endptr - str);
    return num;
}

MSTL_NODISCARD constexpr uint32_t to_uint32(const char* str, size_t* idx = nullptr, const int base = 10) {
    char* endptr = nullptr;
    const uint32_t num = _MSTL strtoul(str, &endptr, base);
    if(str == endptr) Exception(TypeCastError("Convert from string failed."));

    if (idx) *idx = static_cast<size_t>(endptr - str);
    return num;
}

MSTL_NODISCARD constexpr int16_t to_int16(const char* str, size_t* idx = nullptr, const int base = 10) {
    return static_cast<int16_t>(to_int32(str, idx, base));
}

MSTL_NODISCARD constexpr uint16_t to_uint16(const char* str, size_t* idx = nullptr, const int base = 10) {
    return static_cast<int16_t>(to_uint32(str, idx, base));
}

MSTL_NODISCARD constexpr int8_t to_int8(const char* str, size_t* idx = nullptr, const int base = 10) {
    return static_cast<int8_t>(to_int32(str, idx, base));
}

MSTL_NODISCARD constexpr uint8_t to_uint8(const char* str, size_t* idx = nullptr, const int base = 10) {
    return static_cast<uint8_t>(to_uint32(str, idx, base));
}

MSTL_END_NAMESPACE__
#endif // MSTL_CSTRING_HPP__
