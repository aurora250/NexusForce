#ifndef MSTL_CORE_STRING_CSTRING_HPP__
#define MSTL_CORE_STRING_CSTRING_HPP__
#include "../string/char_types.hpp"
MSTL_BEGIN_NAMESPACE__

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
template <typename CharT>
MSTL_PURE_FUNCTION constexpr size_t string_length(const CharT* str) noexcept {
	const char* p = str;
	while (*p != static_cast<CharT>(0)) {
	    ++p;
	}
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

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_STRING_CSTRING_HPP__
