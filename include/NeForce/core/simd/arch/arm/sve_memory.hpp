#ifndef NEFORCE_CORE_SIMD_ARCH_ARM_SVE_MEMORY_HPP__
#define NEFORCE_CORE_SIMD_ARCH_ARM_SVE_MEMORY_HPP__

/**
 * @file sve_memory.hpp
 * @brief ARM SVE 内存内核
 *
 * SVE 的向量长度在运行期由硬件决定，所有循环都使用 svwhilelt 生成谓词。
 */

#include "NeForce/core/simd/scalar/memory.hpp"
#ifdef NEFORCE_SIMD_SVE
#    include <arm_sve.h>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_SIMD__
NEFORCE_BEGIN_SVE__

/**
 * @brief 使用 SVE 进行字节拷贝
 * @param dest 目标内存指针
 * @param src 源内存指针
 * @param count 字节数
 * @return 目标指针
 */
NEFORCE_ALWAYS_INLINE_INLINE void* memory_copy(void* dest, const void* src, const size_t count) noexcept {
    if (dest == nullptr || src == nullptr) {
        return nullptr;
    }
    auto* d = static_cast<byte_t*>(dest);
    const auto* s = static_cast<const byte_t*>(src);
    size_t offset = 0;
    while (offset < count) {
        const ::svbool_t pg = ::svwhilelt_b8(static_cast<uint64_t>(offset), static_cast<uint64_t>(count));
        if (!::svptest_any(svptrue_b8(), pg)) {
            break;
        }
        ::svst1_u8(pg, d + offset, ::svld1_u8(pg, s + offset));
        offset += ::svcntb();
    }
    return dest;
}

/**
 * @brief 使用 SVE 进行字节填充
 * @param dest 目标内存指针
 * @param value 填充字节
 * @param count 字节数
 * @return 目标指针
 */
NEFORCE_ALWAYS_INLINE_INLINE void* memory_set(void* dest, const byte_t value, const size_t count) noexcept {
    if (dest == nullptr) {
        return nullptr;
    }
    auto* d = static_cast<byte_t*>(dest);
    size_t offset = 0;
    while (offset < count) {
        const ::svbool_t pg = ::svwhilelt_b8(static_cast<uint64_t>(offset), static_cast<uint64_t>(count));
        if (!::svptest_any(::svptrue_b8(), pg)) {
            break;
        }
        ::svst1_u8(pg, d + offset, ::svdup_n_u8(value));
        offset += ::svcntb();
    }
    return dest;
}

/**
 * @brief 使用 SVE 查找首个匹配字节
 * @param ptr 内存指针
 * @param value 目标字节
 * @param count 字节数
 * @return 首次出现位置，未找到返回 nullptr
 */
NEFORCE_ALWAYS_INLINE_INLINE const void* memory_find(const void* ptr, const byte_t value, const size_t count) noexcept {
    if (ptr == nullptr || count == 0) {
        return nullptr;
    }
    const auto* p = static_cast<const byte_t*>(ptr);
    size_t offset = 0;
    while (offset < count) {
        const ::svbool_t pg = ::svwhilelt_b8(static_cast<uint64_t>(offset), static_cast<uint64_t>(count));
        if (!::svptest_any(::svptrue_b8(), pg)) {
            break;
        }
        const ::svbool_t hit = ::svcmpeq_n_u8(pg, ::svld1_u8(pg, p + offset), value);
        if (::svptest_any(::svptrue_b8(), hit)) {
            const size_t limit = (count - offset < ::svcntb()) ? (count - offset) : ::svcntb();
            for (size_t i = 0; i < limit; ++i) {
                if (p[offset + i] == value) {
                    return p + offset + i;
                }
            }
        }
        offset += ::svcntb();
    }
    return nullptr;
}

/**
 * @brief 使用 SVE 比较两块内存
 * @param lhs 左内存指针
 * @param rhs 右内存指针
 * @param count 字节数
 * @return 首个不同字节的差值，全等返回 0
 */
NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE_INLINE int memory_compare(const void* lhs, const void* rhs,
                                                                  const size_t count) noexcept {
    if (lhs == rhs) {
        return 0;
    }
    if (lhs == nullptr) {
        return -1;
    }
    if (rhs == nullptr) {
        return 1;
    }
    if (count == 0) {
        return 0;
    }
    const auto* a = static_cast<const byte_t*>(lhs);
    const auto* b = static_cast<const byte_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const ::svbool_t pg = ::svwhilelt_b8(static_cast<uint64_t>(offset), static_cast<uint64_t>(count));
        if (!::svptest_any(::svptrue_b8(), pg)) {
            break;
        }
        const ::svbool_t ne = ::svcmpne_u8(pg, ::svld1_u8(pg, a + offset), ::svld1_u8(pg, b + offset));
        if (::svptest_any(::svptrue_b8(), ne)) {
            const size_t limit = (count - offset < ::svcntb()) ? (count - offset) : ::svcntb();
            for (size_t i = 0; i < limit; ++i) {
                if (a[offset + i] != b[offset + i]) {
                    return static_cast<int>(a[offset + i]) - static_cast<int>(b[offset + i]);
                }
            }
        }
        offset += ::svcntb();
    }
    return 0;
}

/**
 * @brief 使用 SVE 计算单字节字符字符串长度
 * @tparam CharT 字符类型，仅支持单字节
 * @param str 以空字符结尾的字符串
 * @return 字符数（不含终止符）
 */
NEFORCE_PURE_FUNCTION NEFORCE_ALWAYS_INLINE_INLINE size_t string_length_byte(const byte_t* str) noexcept {
    if (str == nullptr) {
        return 0;
    }
    const auto* p = reinterpret_cast<const byte_t*>(str);
    size_t offset = 0;
    while (true) {
        const auto address = reinterpret_cast<uintptr_t>(p + offset);
        const size_t page_left = 4096U - (address & 4095U);
        const size_t limit = page_left < svcntb() ? page_left : svcntb();
        if (limit == 0) {
            return offset;
        }
        const svbool_t pg = ::svwhilelt_b8(static_cast<uint64_t>(0), static_cast<uint64_t>(limit));
        const svbool_t hit = ::svcmpeq_n_u8(pg, svld1_u8(pg, p + offset), static_cast<byte_t>(0));
        if (::svptest_any(::svptrue_b8(), hit)) {
            for (size_t i = 0; i < limit; ++i) {
                if (p[offset + i] == static_cast<byte_t>(0)) {
                    return offset + i;
                }
            }
        }
        offset += limit;
    }
}

NEFORCE_END_SVE__
NEFORCE_END_SIMD__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_SIMD_SVE
#endif // NEFORCE_CORE_SIMD_ARCH_ARM_SVE_MEMORY_HPP__
