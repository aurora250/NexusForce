#ifndef NEFORCE_CORE_SIMD_COMPARE_PORTABLE_HPP__
#define NEFORCE_CORE_SIMD_COMPARE_PORTABLE_HPP__
#include "NeForce/core/simd/types.hpp"
#ifdef NEFORCE_SIMD_BRANCH_PORTABLE
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_SIMD__

/**
 * @addtogroup SIMD
 * @{
 */

/**
 * @brief 16 路 8-bit 相等比较
 * @param a 左操作数（16×i8）
 * @param b 右操作数（16×i8）
 * @return a == b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t cmpeq_i8(vec128_t a, vec128_t b) noexcept {
    vec128_t result;
    for (int i = 0; i < 16; ++i) {
        result.data[i] = (a.data[i] == b.data[i]) ? 0xFF : 0x00;
    }
    return result;
}

/**
 * @brief 8 路 16-bit 相等比较
 * @param a 左操作数（8×i16）
 * @param b 右操作数（8×i16）
 * @return a == b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t cmpeq_i16(vec128_t a, vec128_t b) noexcept {
    const auto* sa = reinterpret_cast<const int16_t*>(a.data);
    const auto* sb = reinterpret_cast<const int16_t*>(b.data);
    vec128_t result;
    auto* rd = reinterpret_cast<uint16_t*>(result.data);
    for (int i = 0; i < 8; ++i) {
        rd[i] = (sa[i] == sb[i]) ? 0xFFFF : 0;
    }
    return result;
}

/**
 * @brief 4 路 32-bit 相等比较
 * @param a 左操作数（4×i32）
 * @param b 右操作数（4×i32）
 * @return a == b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t cmpeq_i32(vec128_t a, vec128_t b) noexcept {
    const auto* sa = reinterpret_cast<const int32_t*>(a.data);
    const auto* sb = reinterpret_cast<const int32_t*>(b.data);
    vec128_t result;
    auto* rd = reinterpret_cast<uint32_t*>(result.data);
    for (int i = 0; i < 4; ++i) {
        rd[i] = (sa[i] == sb[i]) ? 0xFFFFFFFF : 0;
    }
    return result;
}

/**
 * @brief 2 路 64-bit 相等比较
 * @param a 左操作数（2×i64）
 * @param b 右操作数（2×i64）
 * @return a == b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t cmpeq_i64(vec128_t a, vec128_t b) noexcept {
    const auto* sa = reinterpret_cast<const int64_t*>(a.data);
    const auto* sb = reinterpret_cast<const int64_t*>(b.data);
    vec128_t result;
    auto* rd = reinterpret_cast<uint64_t*>(result.data);
    for (int i = 0; i < 2; ++i) {
        rd[i] = (sa[i] == sb[i]) ? 0xFFFFFFFFFFFFFFFFULL : 0;
    }
    return result;
}

/**
 * @brief 16 路 8-bit 有符号大于比较
 * @param a 左操作数（16×i8）
 * @param b 右操作数（16×i8）
 * @return a > b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t cmpgt_i8(vec128_t a, vec128_t b) noexcept {
    vec128_t result;
    for (int i = 0; i < 16; ++i) {
        result.data[i] = (static_cast<int8_t>(a.data[i]) > static_cast<int8_t>(b.data[i])) ? 0xFF : 0x00;
    }
    return result;
}

/**
 * @brief 8 路 16-bit 有符号大于比较
 * @param a 左操作数（8×i16）
 * @param b 右操作数（8×i16）
 * @return a > b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t cmpgt_i16(vec128_t a, vec128_t b) noexcept {
    const auto* sa = reinterpret_cast<const int16_t*>(a.data);
    const auto* sb = reinterpret_cast<const int16_t*>(b.data);
    vec128_t result;
    auto* rd = reinterpret_cast<uint16_t*>(result.data);
    for (int i = 0; i < 8; ++i) {
        rd[i] = (sa[i] > sb[i]) ? 0xFFFF : 0;
    }
    return result;
}

/**
 * @brief 4 路 32-bit 有符号大于比较
 * @param a 左操作数（4×i32）
 * @param b 右操作数（4×i32）
 * @return a > b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t cmpgt_i32(vec128_t a, vec128_t b) noexcept {
    const auto* sa = reinterpret_cast<const int32_t*>(a.data);
    const auto* sb = reinterpret_cast<const int32_t*>(b.data);
    vec128_t result;
    auto* rd = reinterpret_cast<uint32_t*>(result.data);
    for (int i = 0; i < 4; ++i) {
        rd[i] = (sa[i] > sb[i]) ? 0xFFFFFFFF : 0;
    }
    return result;
}

/**
 * @brief 2 路 64-bit 有符号大于比较
 * @param a 左操作数（2×i64）
 * @param b 右操作数（2×i64）
 * @return a > b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t cmpgt_i64(vec128_t a, vec128_t b) noexcept {
    const auto* sa = reinterpret_cast<const int64_t*>(a.data);
    const auto* sb = reinterpret_cast<const int64_t*>(b.data);
    vec128_t result;
    auto* rd = reinterpret_cast<uint64_t*>(result.data);
    for (int i = 0; i < 2; ++i) {
        rd[i] = (sa[i] > sb[i]) ? 0xFFFFFFFFFFFFFFFFULL : 0;
    }
    return result;
}

/**
 * @brief 4 路单精度浮点相等比较
 * @param a 左操作数（4×f32）
 * @param b 右操作数（4×f32）
 * @return a == b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128f_t cmpeq_f32(vec128f_t a, vec128f_t b) noexcept {
    vec128f_t result;
    for (int i = 0; i < 4; ++i) {
        reinterpret_cast<uint32_t&>(result.data[i]) = (a.data[i] == b.data[i]) ? 0xFFFFFFFF : 0;
    }
    return result;
}

/**
 * @brief 4 路单精度浮点大于比较
 * @param a 左操作数（4×f32）
 * @param b 右操作数（4×f32）
 * @return a > b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128f_t cmpgt_f32(vec128f_t a, vec128f_t b) noexcept {
    vec128f_t result;
    for (int i = 0; i < 4; ++i) {
        reinterpret_cast<uint32_t&>(result.data[i]) = (a.data[i] > b.data[i]) ? 0xFFFFFFFF : 0;
    }
    return result;
}

/**
 * @brief 4 路单精度浮点大于等于比较
 * @param a 左操作数（4×f32）
 * @param b 右操作数（4×f32）
 * @return a >= b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128f_t cmpge_f32(vec128f_t a, vec128f_t b) noexcept {
    vec128f_t result;
    for (int i = 0; i < 4; ++i) {
        reinterpret_cast<uint32_t&>(result.data[i]) = (a.data[i] >= b.data[i]) ? 0xFFFFFFFF : 0;
    }
    return result;
}

/**
 * @brief 4 路单精度浮点小于比较
 * @param a 左操作数（4×f32）
 * @param b 右操作数（4×f32）
 * @return a < b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128f_t cmplt_f32(vec128f_t a, vec128f_t b) noexcept {
    vec128f_t result;
    for (int i = 0; i < 4; ++i) {
        reinterpret_cast<uint32_t&>(result.data[i]) = (a.data[i] < b.data[i]) ? 0xFFFFFFFF : 0;
    }
    return result;
}

/**
 * @brief 4 路单精度浮点小于等于比较
 * @param a 左操作数（4×f32）
 * @param b 右操作数（4×f32）
 * @return a <= b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128f_t cmple_f32(vec128f_t a, vec128f_t b) noexcept {
    vec128f_t result;
    for (int i = 0; i < 4; ++i) {
        reinterpret_cast<uint32_t&>(result.data[i]) = (a.data[i] <= b.data[i]) ? 0xFFFFFFFF : 0;
    }
    return result;
}

/**
 * @brief 2 路双精度浮点相等比较
 * @param a 左操作数（2×f64）
 * @param b 右操作数（2×f64）
 * @return a == b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128d_t cmpeq_f64(vec128d_t a, vec128d_t b) noexcept {
    vec128d_t result;
    for (int i = 0; i < 2; ++i) {
        reinterpret_cast<uint64_t&>(result.data[i]) = (a.data[i] == b.data[i]) ? 0xFFFFFFFFFFFFFFFFULL : 0;
    }
    return result;
}

/**
 * @brief 2 路双精度浮点大于比较
 * @param a 左操作数（2×f64）
 * @param b 右操作数（2×f64）
 * @return a > b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128d_t cmpgt_f64(vec128d_t a, vec128d_t b) noexcept {
    vec128d_t result;
    for (int i = 0; i < 2; ++i) {
        reinterpret_cast<uint64_t&>(result.data[i]) = (a.data[i] > b.data[i]) ? 0xFFFFFFFFFFFFFFFFULL : 0;
    }
    return result;
}

/**
 * @brief 2 路双精度浮点大于等于比较
 * @param a 左操作数（2×f64）
 * @param b 右操作数（2×f64）
 * @return a >= b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128d_t cmpge_f64(vec128d_t a, vec128d_t b) noexcept {
    vec128d_t result;
    for (int i = 0; i < 2; ++i) {
        reinterpret_cast<uint64_t&>(result.data[i]) = (a.data[i] >= b.data[i]) ? 0xFFFFFFFFFFFFFFFFULL : 0;
    }
    return result;
}

/**
 * @brief 2 路双精度浮点小于比较
 * @param a 左操作数（2×f64）
 * @param b 右操作数（2×f64）
 * @return a < b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128d_t cmplt_f64(vec128d_t a, vec128d_t b) noexcept {
    vec128d_t result;
    for (int i = 0; i < 2; ++i) {
        reinterpret_cast<uint64_t&>(result.data[i]) = (a.data[i] < b.data[i]) ? 0xFFFFFFFFFFFFFFFFULL : 0;
    }
    return result;
}

/**
 * @brief 2 路双精度浮点小于等于比较
 * @param a 左操作数（2×f64）
 * @param b 右操作数（2×f64）
 * @return a <= b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128d_t cmple_f64(vec128d_t a, vec128d_t b) noexcept {
    vec128d_t result;
    for (int i = 0; i < 2; ++i) {
        reinterpret_cast<uint64_t&>(result.data[i]) = (a.data[i] <= b.data[i]) ? 0xFFFFFFFFFFFFFFFFULL : 0;
    }
    return result;
}

/** @} */ // SIMD

NEFORCE_END_SIMD__
NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_CORE_SIMD_COMPARE_PORTABLE_HPP__
