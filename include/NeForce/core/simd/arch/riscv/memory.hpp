#ifndef NEFORCE_CORE_SIMD_MEMORY_RISCV_HPP__
#define NEFORCE_CORE_SIMD_MEMORY_RISCV_HPP__
#include "NeForce/core/simd/scalar/memory.hpp"
#ifdef NEFORCE_SIMD_BRANCH_RISCV
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_SIMD__

/**
 * @brief RVV 字节拷贝
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
        const size_t vl = __riscv_vsetvl_e8m8(count - offset);
        const vec_rvv8_wide_t chunk = __riscv_vle8_v_u8m8(s + offset, vl);
        __riscv_vse8_v_u8m8(d + offset, chunk, vl);
        offset += vl;
    }
    return dest;
}

/**
 * @brief RVV 字节填充
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
        const size_t vl = __riscv_vsetvl_e8m8(count - offset);
        __riscv_vse8_v_u8m8(d + offset, __riscv_vmv_v_x_u8m8(value, vl), vl);
        offset += vl;
    }
    return dest;
}

/**
 * @brief RVV 字节查找
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
        const size_t vl = __riscv_vsetvl_e8m8(count - offset);
        const vec_rvv8_wide_t chunk = __riscv_vle8_v_u8m8(p + offset, vl);
        const vec_rvv8_wide_mask_t hit = __riscv_vmseq_vx_u8m8_b1(chunk, value, vl);
        const long first = __riscv_vfirst_m_b1(hit, vl);
        if (first >= 0) {
            return p + offset + static_cast<size_t>(first);
        }
        offset += vl;
    }
    return nullptr;
}

/**
 * @brief RVV 内存比较
 * @param lhs 左内存指针
 * @param rhs 右内存指针
 * @param count 字节数
 * @return 差值为 0 表示相等
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
        const size_t vl = __riscv_vsetvl_e8m8(count - offset);
        const vec_rvv8_wide_t x = __riscv_vle8_v_u8m8(a + offset, vl);
        const vec_rvv8_wide_t y = __riscv_vle8_v_u8m8(b + offset, vl);
        const vec_rvv8_wide_mask_t ne = __riscv_vmsne_vv_u8m8_b1(x, y, vl);
        const long first = __riscv_vfirst_m_b1(ne, vl);
        if (first >= 0) {
            return static_cast<int>(a[offset + static_cast<size_t>(first)]) -
                   static_cast<int>(b[offset + static_cast<size_t>(first)]);
        }
        offset += vl;
    }
    return 0;
}

/**
 * @brief RVV 单字节字符串长度
 * @param str 以空字符结尾的字符串
 * @return 字符数（不含终止符）
 */
NEFORCE_PURE_FUNCTION NEFORCE_ALWAYS_INLINE_INLINE size_t string_length_byte(const byte_t* str) noexcept {
    if (str == nullptr) {
        return 0;
    }
    size_t offset = 0;
    while (true) {
        const uintptr_t address = reinterpret_cast<uintptr_t>(str + offset);
        const size_t page_left = 4096U - (address & 4095U);
        const size_t want = page_left < 64U ? page_left : 64U;
        const size_t vl = __riscv_vsetvl_e8m8(want);
        const vec_rvv8_wide_t chunk = __riscv_vle8_v_u8m8(str + offset, vl);
        const vec_rvv8_wide_mask_t hit = __riscv_vmseq_vx_u8m8_b1(chunk, byte_t(0), vl);
        const long first = __riscv_vfirst_m_b1(hit, vl);
        if (first >= 0) {
            return offset + static_cast<size_t>(first);
        }
        offset += vl;
    }
}

/**
 * @brief RVV 字符串长度，宽字符回退到 scalar 后端
 * @tparam CharT 字符类型
 * @param str 以空字符结尾的字符串
 * @return 字符数（不含终止符）
 */
template <typename CharT>
NEFORCE_PURE_FUNCTION NEFORCE_ALWAYS_INLINE_INLINE size_t string_length(const CharT* str) noexcept {
    if (str == nullptr) {
        return 0;
    }
    if (sizeof(CharT) == 1) {
        return string_length_byte(reinterpret_cast<const byte_t*>(str));
    }
    return scalar::string_length(str);
}


using scalar::load_aligned;
using scalar::load_stream;
using scalar::loadl_epi64;
using scalar::loadu_pd;
using scalar::loadu_ps;
using scalar::loadu_si128;
using scalar::loadu_si256;
using scalar::loadu_si32;
using scalar::loadu_si512;
using scalar::memory_copy_large;
using scalar::memory_copy_offset;
using scalar::memory_move;
using scalar::memory_move_large;
using scalar::memory_zero;
using scalar::prefetch_l1;
using scalar::prefetch_l2;
using scalar::prefetch_nta;
using scalar::prefetch_read;
using scalar::prefetch_write;
using scalar::store_aligned;
using scalar::store_stream;
using scalar::store_stream256;
using scalar::store_stream512;
using scalar::storel_epi64;
using scalar::storeu_si128;
using scalar::storeu_si256;
using scalar::storeu_si32;
using scalar::storeu_si512;
using scalar::string_compare;

using scalar::memory_find_pattern;
using scalar::memory_set_large;
using scalar::string_find;

NEFORCE_END_SIMD__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_SIMD_BRANCH_RISCV
#endif // NEFORCE_CORE_SIMD_MEMORY_RISCV_HPP__
