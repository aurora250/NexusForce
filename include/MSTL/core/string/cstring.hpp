#ifndef MSTL_CORE_STRING_CSTRING_HPP__
#define MSTL_CORE_STRING_CSTRING_HPP__

/**
 * @file cstring.hpp
 * @brief MSTL字符串操作
 *
 * 此文件提供了字符串操作函数的实现，包括比较、拷贝、合并等功能。
 */

#include "../typeinfo/type_traits.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup CharCaseConversion 字符转换
 * @brief 字符大小写转换函数
 * @{
 */

/**
 * @brief 将字符转换为小写
 * @tparam CharT 字符类型
 * @param c 要转换的字符
 * @return 转换后的小写字符，如果无法转换则返回原字符
 *
 * 将A-Z转换为a-z，其他字符保持不变。
 */
template <typename CharT>
MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 CharT to_lowercase(const CharT c) noexcept {
	static_assert(is_character_v<CharT>, "character type is necessary");
	using UT = make_unsigned_t<CharT>;
	const auto uc = static_cast<UT>(c);
	if (uc >= static_cast<UT>('A') && uc <= static_cast<UT>('Z')) {
		return static_cast<CharT>(uc | 0x20);
	}
	return c;
}

/**
 * @brief 将字符转换为大写
 * @tparam CharT 字符类型
 * @param c 要转换的字符
 * @return 转换后的大写字符，如果无法转换则返回原字符
 *
 * 将a-z转换为A-Z，其他字符保持不变。
 */
template <typename CharT>
MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 CharT to_uppercase(const CharT c) noexcept {
	static_assert(is_character_v<CharT>, "character type is necessary");
	using UT = make_unsigned_t<CharT>;
	const auto uc = static_cast<UT>(c);
	if (uc >= static_cast<UT>('a') && uc <= static_cast<UT>('z')) {
		return static_cast<CharT>(uc & 0xDF);
	}
	return c;
}

/** @} */ // CharCaseConversion

/**
 * @defgroup StringOperations 字符串操作
 * @brief MSTL字符串操作函数的实现
 * @{
 */

/**
 * @brief 复制字符串
 * @tparam CharT 字符类型
 * @param dest 目标字符串指针
 * @param src 源字符串指针
 * @return 目标字符串指针
 * @note 使用restrict关键字优化，要求源和目标内存不重叠，否则将产生未定义行为。
 *
 * 将源字符串复制到目标缓冲区，包括终止空字符。
 * 如果任一指针为空，返回空指针。
 */
template <typename CharT>
constexpr CharT*
string_copy(CharT* MSTL_RESTRICT dest, const CharT* MSTL_RESTRICT src) noexcept {
	if(dest == nullptr || src == nullptr) return nullptr;
	CharT* ret = dest;
	while (*src != static_cast<CharT>(0)) {
		*dest = *src;
		++dest;
		++src;
	}
	*dest = *src;
	return ret;
}

/**
 * @brief 复制字符串并返回指向结尾的指针
 * @tparam CharT 字符类型
 * @param dest 目标字符串指针
 * @param src 源字符串指针
 * @return 指向目标字符串结尾的指针，终止空字符之前
 * @note 使用restrict关键字优化，要求源和目标内存不重叠，否则将产生未定义行为。
 */
template <typename CharT>
constexpr CharT*
string_copy_offset(CharT* MSTL_RESTRICT dest, const CharT* MSTL_RESTRICT src) noexcept {
	if (dest == nullptr || src == nullptr) return nullptr;
	while (*src != static_cast<CharT>(0)) {
		*dest = *src;
		++dest;
		++src;
	}
	*dest = *src;
	return dest - 1;
}

/**
 * @brief 复制指定长度的字符串
 * @tparam CharT 字符类型
 * @param dest 目标字符串指针
 * @param src 源字符串指针
 * @param count 要复制的最大字符数
 * @return 目标字符串指针
 *
 * 复制最多 count 个字符，如果源字符串长度小于 count，用空字符填充剩余位置。
 * 如果源字符串长度大于等于 count，不会自动添加终止空字符。
 *
 * @note 使用restrict关键字优化，要求源和目标内存不重叠，否则将产生未定义行为。
 */
template <typename CharT>
constexpr CharT*
string_copy_n(CharT* MSTL_RESTRICT dest, const CharT* MSTL_RESTRICT src,
              const size_t count) noexcept {
	if (dest == nullptr || src == nullptr) return nullptr;
	CharT* ret = dest;

	size_t i = 0;
	while (i < count && *src != static_cast<CharT>(0)) {
		*dest = *src;
		++dest;
		++src;
		i++;
	}

	while (i < count) {
		*dest = static_cast<CharT>(0);
		++dest;
		i++;
	}
	return ret;
}

/**
 * @brief 复制指定长度的字符串并返回指向结尾的指针
 * @tparam CharT 字符类型
 * @param dest 目标字符串指针
 * @param src 源字符串指针
 * @param count 要复制的最大字符数
 * @return 指向目标字符串第 count 个字符的指针
 * @note 使用restrict关键字优化，要求源和目标内存不重叠，否则将产生未定义行为。
 */
template <typename CharT>
constexpr CharT*
string_copy_n_offset(CharT* MSTL_RESTRICT dest, const CharT* MSTL_RESTRICT src,
	                 const size_t count) noexcept {
	if (dest == nullptr || src == nullptr) return nullptr;

	size_t i = 0;
	while (i < count && *src != static_cast<CharT>(0)) {
		*dest = *src;
		++dest;
		++src;
		i++;
	}

	while (i < count) {
		*dest = static_cast<CharT>(0);
		++dest;
		i++;
	}
	return dest;
}

/**
 * @brief 比较两个字符串
 * @tparam CharT 字符类型
 * @param dest 第一个字符串指针
 * @param src 第二个字符串指针
 * @return 比较结果
 *          - 正数：第一个字符串大于第二个
 *          - 负数：第一个字符串小于第二个
 *          - 零：两个字符串相等
 */
template <typename CharT>
MSTL_PURE_FUNCTION constexpr int
string_compare(const CharT* dest, const CharT* src) noexcept {
	if (dest == nullptr && src == nullptr) return 0;
	if (dest == nullptr) return -1;
	if (src == nullptr) return 1;

	while (*dest == *src) {
		if (*dest == static_cast<CharT>(0)) {
			return 0;
		}
		++dest;
		++src;
	}
	if (*dest > *src) return 1;
	return -1;
}

/**
 * @brief 忽略大小写比较两个字符串
 * @tparam CharT 字符类型
 * @param s1 第一个字符串指针
 * @param s2 第二个字符串指针
 * @return 比较结果
 *          - 正数：第一个字符串大于第二个
 *          - 负数：第一个字符串小于第二个
 *          - 零：两个字符串相等
 */
template <typename CharT>
MSTL_PURE_FUNCTION constexpr int
string_compare_ignore_case(const CharT* s1, const CharT* s2) {
	if (s1 == nullptr && s2 == nullptr) return 0;
	if (s1 == nullptr) return -1;
	if (s2 == nullptr) return 1;

	while (*s1 && *s2) {
		const CharT c1 = _MSTL to_lowercase(*s1);
		const CharT c2 = _MSTL to_lowercase(*s2);
		if (c1 < c2) return -1;
		if (c1 > c2) return 1;
		++s1;
		++s2;
	}
	return *s1 == *s2 ? 0 : *s1 < *s2 ? -1 : 1;
}

/**
 * @brief 比较两个字符串的前n个字符
 * @tparam CharT 字符类型
 * @param dest 第一个字符串指针
 * @param src 第二个字符串指针
 * @param count 要比较的字符数
 * @return 比较结果
 */
template <typename CharT>
MSTL_PURE_FUNCTION constexpr int
string_compare_n(const CharT* dest, const CharT* src, const size_t count) noexcept {
	if (dest == nullptr && src == nullptr) return 0;
	if (dest == nullptr) return -1;
	if (src == nullptr) return 1;

	if (count == 0) return 0;
	size_t i = 0;
	while (*dest == *src &&
		   *dest != static_cast<CharT>(0) &&
		   i < count - 1) {
		++dest;
		++src;
		++i;
	}
	if (i == count - 1) return 0;
	return *dest < *src ? -1 : *dest > *src ? 1 : 0;
}

/**
 * @brief 忽略大小写比较两个字符串的前n个字符
 * @tparam CharT 字符类型
 * @param s1 第一个字符串指针
 * @param s2 第二个字符串指针
 * @param count 要比较的字符数
 * @return 比较结果
 */
template <typename CharT>
MSTL_PURE_FUNCTION constexpr int
string_compare_n_ignore_case(const CharT* s1, const CharT* s2, const size_t count) noexcept {
	if ((s1 == nullptr && s2 == nullptr) || count == 0) return 0;
	if (s1 == nullptr) return -1;
	if (s2 == nullptr) return 1;

	size_t i = 0;
	while (*s1 && *s2 && i < count - 1) {
		const CharT c1 = _MSTL to_lowercase(*s1);
		const CharT c2 = _MSTL to_lowercase(*s2);
		if (c1 < c2) return -1;
		if (c1 > c2) return 1;
		++s1;
		++s2;
		++i;
	}
	if (i == count - 1) return 0;

	const CharT c1 = _MSTL to_lowercase(*s1);
	const CharT c2 = _MSTL to_lowercase(*s2);
	return c1 < c2 ? -1 : c1 > c2 ? 1 : 0;
}

/**
 * @brief 计算字符串长度
 * @tparam CharT 字符类型
 * @param str 字符串指针
 * @return 字符串长度，不包含终止空字符
 */
template <typename CharT>
MSTL_PURE_FUNCTION constexpr size_t string_length(const CharT* str) noexcept {
	static_assert(is_character_v<CharT>, "CharT must be a character");
	if (str == nullptr) return 0;
	const CharT* p = str;
	while (*p != static_cast<CharT>(0)) {
	    ++p;
	}
	return static_cast<size_t>(p - str);
}

/**
 * @brief 计算字符串的最大长度
 * @tparam CharT 字符类型
 * @param str 字符串指针
 * @param max_len 最大搜索长度
 * @return 字符串长度，不超过max_len
 *
 * 计算字符串长度，但最多检查max_len个字符。
 */
template <typename CharT>
MSTL_PURE_FUNCTION constexpr size_t
string_length_n(const CharT* str, const size_t max_len) noexcept {
	const CharT* p = str;
	ptrdiff_t len = 0;
	while (*p != static_cast<CharT>(0) && len < max_len) {
		++p;
		++len;
	}
	return len;
}

/**
 * @brief 查找字符在字符串中首次出现的位置
 * @tparam CharT 字符类型
 * @param str 字符串指针
 * @param chr 要查找的字符
 * @return 指向首次出现位置的指针，未找到或输入为空时返回空指针
 */
template <typename CharT>
MSTL_PURE_FUNCTION constexpr const CharT*
string_find(const CharT* str, const CharT chr) noexcept {
	if (str == nullptr) return nullptr;
	while (*str != static_cast<CharT>(0)) {
		if (*str == chr) {
			return str;
		}
		++str;
	}
	if (*str == chr) {
		return str;
	}
	return nullptr;
}

/**
 * @brief 在前n个字符中查找字符首次出现的位置
 * @tparam CharT 字符类型
 * @param str 字符串指针
 * @param chr 要查找的字符
 * @param count 要搜索的最大字符数
 * @return 指向首次出现位置的指针
 */
template <typename CharT>
MSTL_PURE_FUNCTION constexpr const CharT*
string_find_n(const CharT* str, const CharT chr, const size_t count) noexcept {
	if (str == nullptr || count == 0) return nullptr;

	for (size_t i = 0; i < count; ++i) {
		if (str[i] == chr) {
			return str + i;
		}
		if (str[i] == static_cast<CharT>(0)) {
			break;
		}
	}
	return nullptr;
}

/**
 * @brief 查找字符在字符串中最后出现的位置
 * @tparam CharT 字符类型
 * @param str 字符串指针
 * @param chr 要查找的字符
 * @return 指向最后出现位置的指针，未找到或输入为空时返回空指针
 */
template <typename CharT>
MSTL_PURE_FUNCTION constexpr const CharT*
string_find_last(const CharT* str, const CharT chr) noexcept {
	if (str == nullptr) return nullptr;
	const CharT* last = nullptr;

	while (*str != static_cast<CharT>(0)) {
		if (*str == chr) {
			last = str;
		}
		++str;
	}
	return last;
}

/**
 * @brief 查找字符串中第一个出现在指定字符集中的字符
 * @tparam CharT 字符类型
 * @param str 字符串指针
 * @param accept 字符集指针
 * @return 指向第一个匹配字符的指针，未找到时返回空指针
 */
template <typename CharT>
MSTL_PURE_FUNCTION constexpr CharT*
string_find_any(CharT* str, const CharT* accept) noexcept {
	if (str == nullptr || *str == static_cast<CharT>(0) ||
		accept == nullptr || *accept == static_cast<CharT>(0))
		return nullptr;

	while (*str != static_cast<CharT>(0)) {
		const CharT* a = accept;
		while (*a != static_cast<CharT>(0)) {
			if (*str == *a) {
				return str;
			}
			++a;
		}
		++str;
	}
	return nullptr;
}

/**
 * @brief 查找子字符串在字符串中首次出现的位置
 * @tparam CharT 字符类型
 * @param dest 主字符串指针
 * @param src 子字符串指针
 * @return 指向子字符串首次出现位置的指针，未找到时返回空指针
 */
template <typename CharT>
MSTL_PURE_FUNCTION constexpr const CharT*
string_find_pattern(const CharT* dest, const CharT* src) noexcept {
	if(dest == nullptr || src == nullptr) return nullptr;
	const CharT* cur = dest;
	while (*cur) {
		const CharT *str1 = cur;
		const CharT *str2 = src;
		while (*str1 && *str2 && *str1 == *str2) {
			++str1;
			++str2;
		}
		if (*str2 == static_cast<CharT>(0)) return cur;
		++cur;
	}
	return nullptr;
}

/**
 * @brief 忽略大小写查找子字符串在字符串中首次出现的位置
 * @tparam CharT 字符类型
 * @param dest 主字符串指针
 * @param src 子字符串指针
 * @return 指向子字符串首次出现位置的指针
 */
template <typename CharT>
MSTL_PURE_FUNCTION constexpr const CharT*
string_find_pattern_ignored_case(const CharT* dest, const CharT* src) noexcept {
	if (dest == nullptr || src == nullptr) return nullptr;
	if (*src == static_cast<CharT>(0)) return dest;

	const CharT* cur = dest;
	while (*cur) {
		const CharT* str1 = cur;
		const CharT* str2 = src;
		while (*str1 && *str2) {
			const CharT c1 = _MSTL to_lowercase(*str1);
			const CharT c2 = _MSTL to_lowercase(*str2);
			if (c1 != c2) break;
			++str1;
			++str2;
		}
		if (*str2 == static_cast<CharT>(0)) {
			return cur;
		}
		++cur;
	}
	return nullptr;
}

/**
 * @brief 计算字符串开头包含在指定字符集中的字符数
 * @tparam CharT 字符类型
 * @param str 字符串指针
 * @param accept 字符集指针
 * @return 开头连续出现在字符集中的字符数
 */
template <typename CharT>
MSTL_PURE_FUNCTION constexpr size_t
string_span_in(const CharT* str, const CharT* accept) noexcept {
	if (str == nullptr || *str == static_cast<CharT>(0) ||
		accept == nullptr || *accept == static_cast<CharT>(0)) {
		return 0;
	}

	const CharT* original_str = str;
	while (*str != static_cast<CharT>(0)) {
		const CharT* a = accept;
		bool found = false;
		while (*a != static_cast<CharT>(0)) {
			if (*str == *a) {
				found = true;
				break;
			}
			++a;
		}
		if (!found) {
			return static_cast<size_t>(str - original_str);
		}
		++str;
	}
	return static_cast<size_t>(str - original_str);
}

/**
 * @brief 计算字符串开头不包含在指定字符集中的字符数
 * @tparam CharT 字符类型
 * @param str 字符串指针
 * @param reject 排除字符集指针
 * @return 开头连续不出现在排除字符集中的字符数
 */
template <typename CharT>
MSTL_PURE_FUNCTION constexpr size_t
string_span_not_in(const CharT* str, const CharT* reject) noexcept {
	if (str == nullptr || *str == static_cast<CharT>(0)) return 0;
	if (reject == nullptr || *reject == static_cast<CharT>(0)) {
		size_t len = 0;
		while (str[len] != static_cast<CharT>(0)) ++len;
		return len;
	}

	const CharT* original_str = str;
	while (*str != static_cast<CharT>(0)) {
		const CharT* r = reject;
		while (*r != static_cast<CharT>(0)) {
			if (*str == *r) {
				return static_cast<size_t>(str - original_str);
			}
			++r;
		}
		++str;
	}
	return static_cast<size_t>(str - original_str);
}

/**
 * @brief 将字符串中的所有字符设置为指定值
 * @tparam CharT 字符类型
 * @param str 字符串指针
 * @param value 要设置的字符值
 * @return 原字符串指针
 */
template <typename CharT>
constexpr CharT* string_set(CharT* str, const CharT value) noexcept {
	if (str == nullptr) return nullptr;
	CharT* original = str;
	while (*str != static_cast<CharT>(0)) {
		*str = value;
		++str;
	}
	return original;
}

/**
 * @brief 将字符串中的前n个字符设置为指定值
 * @tparam CharT 字符类型
 * @param str 字符串指针
 * @param value 要设置的字符值
 * @param count 要设置的字符数
 * @return 原字符串指针
 */
template <typename CharT>
constexpr CharT*
string_set_n(CharT* str, const CharT value, const size_t count) noexcept {
	if (str == nullptr || count == 0) return str;
	CharT* original = str;
	size_t processed = 0;
	while (*str != static_cast<CharT>(0) && processed < count) {
		*str = value;
		++str;
		++processed;
	}
	return original;
}

/**
 * @brief 反转字符串
 * @tparam CharT 字符类型
 * @param str 字符串指针
 * @return 反转后的字符串指针
 */
template <typename CharT>
constexpr CharT* string_reverse(CharT* str) noexcept {
	if (str == nullptr || *str == static_cast<CharT>(0)) return str;

	CharT* end = str;
	while (*end != static_cast<CharT>(0)) {
		++end;
	}
	--end;
	while (str < end) {
		const CharT temp = *str;
		*str = *end;
		*end = temp;
		++str;
		--end;
	}
	return str;
}

/**
 * @brief 连接两个字符串
 * @tparam CharT 字符类型
 * @param dest 目标字符串指针
 * @param src 源字符串指针
 * @return 目标字符串指针
 * @note 目标字符串指针必须有足够空间
 */
template <typename CharT>
constexpr CharT*
string_concatenate(CharT* MSTL_RESTRICT dest, const CharT* MSTL_RESTRICT src) noexcept {
	if (dest == nullptr || src == nullptr) return nullptr;
	CharT* original_dest = dest;
	while (*dest != static_cast<CharT>(0))
		++dest;

	while (*src != static_cast<CharT>(0)) {
		*dest = *src;
		++dest;
		++src;
	}
	*dest = static_cast<CharT>(0);
	return original_dest;
}

/**
 * @brief 连接源字符串的前n个字符到目标字符串
 * @tparam CharT 字符类型
 * @param dest 目标字符串指针
 * @param src 源字符串指针
 * @param count 要连接的最大字符数
 * @return 目标字符串指针
 * @note 目标字符串指针必须有足够空间
 */
template <typename CharT>
constexpr CharT*
string_concatenate_n(CharT* MSTL_RESTRICT dest,
	                 const CharT* MSTL_RESTRICT src,
	                 const size_t count) noexcept {
	if (dest == nullptr || src == nullptr) return nullptr;

	CharT* original_dest = dest;
	while (*dest != static_cast<CharT>(0)) {
		++dest;
	}

	size_t copied = 0;
	while (*src != static_cast<CharT>(0) && copied < count) {
		*dest = *src;
		++dest;
		++src;
		++copied;
	}
	*dest = static_cast<CharT>(0);
	return original_dest;
}

/** @} */ // StringOperations

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_STRING_CSTRING_HPP__
