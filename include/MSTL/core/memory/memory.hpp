#ifndef MSTL_CORE_MEMORY_MEMORY_HPP__
#define MSTL_CORE_MEMORY_MEMORY_HPP__

/**
 * @file memory.hpp
 * @brief MSTL内存操作函数
 *
 * 此文件提供了低级别内存操作函数的实现，包括内存拷贝、移动、比较、填充等操作。
 * 这些函数类似于标准C库的memory函数，但提供constexpr支持和其他增强功能。
 */

#include "../string/char_types.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup MemoryFunctions 内存操作函数
 * @brief 低级内存操作函数集合
 * @{
 */

/**
 * @brief 从源内存复制到目标内存
 * @param dest 目标内存指针
 * @param src 源内存指针
 * @param count 要复制的字节数
 * @return 目标内存的起始指针，如果参数无效则返回nullptr
 * @note 使用restrict关键字优化，要求源和目标内存不重叠，否则将产生未定义行为。
 */
MSTL_CONSTEXPR14 void* memory_copy(
	void* MSTL_RESTRICT dest, const void* MSTL_RESTRICT src, size_t count) noexcept {
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

/**
 * @brief 从源内存复制到目标内存并返回复制结束位置
 * @param dest 目标内存指针
 * @param src 源内存指针
 * @param count 要复制的字节数
 * @return 目标内存复制结束后的下一个位置指针，如果参数无效则返回nullptr
 * @note 使用restrict关键字优化，要求源和目标内存不重叠，否则将产生未定义行为。
 */
MSTL_CONSTEXPR14 void* memory_copy_offset(
	void* MSTL_RESTRICT dest, const void* MSTL_RESTRICT src, size_t count) noexcept {
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

/**
 * @brief 从源内存复制到目标内存，直到遇到特定字节
 * @param dest 目标内存指针
 * @param src 源内存指针
 * @param value 停止字节
 * @param count 最大复制字节数
 * @return 目标内存中停止字符后的下一个位置指针，如果没有找到字节则返回nullptr
 */
MSTL_CONSTEXPR14 void* memory_copy_until(
	void* dest, const void* src, const byte_t value, size_t count) noexcept {
    if (dest == nullptr || src == nullptr) return nullptr;

    auto dest_v = static_cast<volatile byte_t*>(dest);
    auto src_v = static_cast<const volatile byte_t*>(src);

    while (count--) {
        const byte_t current = *src_v;
        *dest_v = current;
        if (current == value) {
            return (void*) (++dest_v);
        }
        dest_v++;
        src_v++;
    }
    return nullptr;
}

/**
 * @brief 比较两个内存区域的内容
 * @param lhs 左侧内存指针
 * @param rhs 右侧内存指针
 * @param count 要比较的字节数
 * @return 比较结果：
 *         - 正数：左侧内存大于右侧内存
 *         - 负数：左侧内存小于右侧内存
 *         - 0：两个内存区域相等
 */
MSTL_PURE_FUNCTION MSTL_CONSTEXPR14 int memory_compare(
	const void* lhs, const void* rhs, size_t count) noexcept {
	if (lhs == nullptr && rhs == nullptr) return 0;
	if (lhs == nullptr) return -1;
    if (rhs == nullptr) return 1;

	while (count--) {
		if (*static_cast<const byte_t*>(lhs) != *static_cast<const byte_t*>(rhs))
			return *static_cast<const byte_t*>(lhs) - *static_cast<const byte_t*>(rhs);
		lhs = static_cast<const byte_t*>(lhs) + 1;
		rhs = static_cast<const byte_t*>(rhs) + 1;
	}
	return 0;
}

/**
 * @brief 从源内存移动数据到目标内存
 * @param dest 目标内存指针
 * @param src 源内存指针
 * @param count 要移动的字节数
 * @return 目标内存的起始指针，如果参数无效则返回nullptr
 * @note 支持重叠区域，当dest < src时从前向后复制，当dest > src时从后向前复制。
 */
MSTL_CONSTEXPR14 void* memory_move(void* dest, const void* src, size_t count) noexcept {
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

/**
 * @brief 使用指定字节填充内存区域
 * @param dest 目标内存指针
 * @param value 填充字节
 * @param count 要填充的字节数
 * @return 目标内存的起始指针，如果参数无效则返回nullptr
 */
MSTL_CONSTEXPR14 void* memory_set(void* dest, const byte_t value, size_t count) noexcept {
	if(dest == nullptr) return nullptr;

	void* ret = static_cast<byte_t*>(dest);
	auto dest_v = static_cast<volatile byte_t*>(dest);
	while (count--) {
		*dest_v = value;
		dest_v = dest_v + 1;
	}
	return ret;
}

/**
 * @brief 将内存区域清零
 * @param dest 目标内存指针
 * @param count 要清零的字节数
 *
 * 清零内存区域。如果参数无效则不执行任何操作。
 */
MSTL_CONSTEXPR14 void memory_zero(void* dest, const size_t count) noexcept {
	if (dest == nullptr) return;

	const auto dest_v = static_cast<volatile byte_t*>(dest);
	for (size_t i = 0; i < count; ++i) {
		dest_v[i] = static_cast<byte_t>(0);
	}
}

/**
 * @brief 在内存中搜索特定字节
 * @param dest 要搜索的内存指针
 * @param value 要搜索的字节
 * @param count 要搜索的字节数
 * @return 指向第一个匹配字节的指针，如果没有找到则返回nullptr
 */
MSTL_PURE_FUNCTION MSTL_CONSTEXPR14 const void* memory_find(
	const void* dest, const byte_t value, size_t count) noexcept {
	if(dest == nullptr) return nullptr;
	auto p = static_cast<const byte_t*>(dest);
	while (count--) {
		if (*p == value) {
			return p;
		}
		p++;
	}
	return nullptr;
}

/**
 * @brief 在内存中搜索子模式
 * @param data 要搜索的内存指针
 * @param data_len 要搜索的内存长度
 * @param pattern 要搜索的模式指针
 * @param pattern_len 模式长度
 * @return 指向第一个匹配模式起始位置的指针，如果没有找到则返回nullptr
 */
MSTL_CONSTEXPR14 void* memory_find_pattern(
	const void* data, const size_t data_len,
	const void* pattern, const size_t pattern_len) noexcept {
	if (data == nullptr || pattern == nullptr ||
		data_len == 0 || pattern_len == 0 || pattern_len > data_len) {
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

/**
 * @brief 执行内存层的类型转换
 * @tparam To 目标类型
 * @tparam From 源类型
 * @param value 要转换的值
 * @return 转换后的值
 * @note 要求两个类型大小相同且都是平凡可复制的。
 *
 * 将源类型的位表示重新解释为目标类型的表示。
 */
template <typename To, typename From>
MSTL_NODISCARD MSTL_CONSTEXPR20 To memory_cast(const From& value) noexcept {
	static_assert(sizeof(To) == sizeof(From), "types must have the same size");
	static_assert(is_trivially_copyable_v<To>, "To type must be trivially copyable");
	static_assert(is_trivially_copyable_v<From>, "From type must be trivially copyable");

#ifdef MSTL_STANDARD_20__
	return __builtin_bit_cast(To, value);
#else
	To result{};
	_MSTL memory_copy(&result, &value, sizeof(To));
	return result;
#endif
}

/** @} */ // MemoryFunctions

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_MEMORY_MEMORY_HPP__
