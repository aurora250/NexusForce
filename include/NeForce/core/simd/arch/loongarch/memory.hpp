#ifndef NEFORCE_CORE_SIMD_MEMORY_LOONGARCH_HPP__
#define NEFORCE_CORE_SIMD_MEMORY_LOONGARCH_HPP__
#include "NeForce/core/simd/scalar/memory.hpp"
#ifdef NEFORCE_SIMD_BRANCH_LOONGARCH
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_SIMD__

/**
 * @brief LSX/LASX 加速的字节拷贝
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
#    if defined(NEFORCE_SIMD_LASX)
    while (count - offset >= 32) {
        __lasx_xvst(__lasx_xvld(s + offset, 0), d + offset, 0);
        offset += 32;
    }
#    endif
    while (count - offset >= 16) {
        __lsx_vst(__lsx_vld(s + offset, 0), d + offset, 0);
        offset += 16;
    }
    if (offset != count) {
        scalar::memory_copy(d + offset, s + offset, count - offset);
    }
    return dest;
}

/**
 * @brief LSX/LASX 加速的字节填充
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
#    if defined(NEFORCE_SIMD_LASX)
    const __m256i pattern256 = __lasx_xvreplgr2vr_b(static_cast<int>(value));
    while (count - offset >= 32) {
        __lasx_xvst(pattern256, d + offset, 0);
        offset += 32;
    }
#    endif
    const __m128i pattern128 = __lsx_vreplgr2vr_b(static_cast<int>(value));
    while (count - offset >= 16) {
        __lsx_vst(pattern128, d + offset, 0);
        offset += 16;
    }
    if (offset != count) {
        scalar::memory_set(d + offset, value, count - offset);
    }
    return dest;
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
using scalar::memory_compare;
using scalar::memory_copy_large;
using scalar::memory_copy_offset;
using scalar::memory_find;
using scalar::memory_find_pattern;
using scalar::memory_move;
using scalar::memory_move_large;
using scalar::memory_set_large;
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
using scalar::string_find;
using scalar::string_length;

NEFORCE_END_SIMD__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_SIMD_BRANCH_LOONGARCH
#endif // NEFORCE_CORE_SIMD_MEMORY_LOONGARCH_HPP__
