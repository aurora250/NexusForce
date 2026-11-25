#ifndef NEFORCE_CORE_SIMD_BLOCK_SCALAR_HPP__
#define NEFORCE_CORE_SIMD_BLOCK_SCALAR_HPP__

/**
 * @file block.hpp
 * @brief 标量后端的批量内核
 *
 * 按 16 字节定长向量分块、尾部按字节补齐。
 */

#include "NeForce/core/simd/scalar/arithmetic.hpp"
#include "NeForce/core/simd/scalar/bitwise.hpp"
#include "NeForce/core/simd/scalar/bytes.hpp"
#include "NeForce/core/simd/scalar/compare.hpp"
#include "NeForce/core/simd/scalar/memory.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_SIMD__
NEFORCE_BEGIN_SCALAR__

/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @brief 批量执行二元定长内核
 * @tparam ElemBytes 元素字节数
 * @tparam Kernel 以两个 vec128_t 为参数、返回 vec128_t 的可调用对象
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数
 * @param kernel 定长内核
 */
template <size_t ElemBytes, typename Kernel>
NEFORCE_ALWAYS_INLINE_INLINE void block_binary(void* dst, const void* lhs, const void* rhs, const size_t count,
                                               Kernel kernel) noexcept {
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<byte_t*>(dst);
    const auto* l = static_cast<const byte_t*>(lhs);
    const auto* r = static_cast<const byte_t*>(rhs);
    constexpr size_t chunk_elems = sizeof(vec128_t) / ElemBytes;
    size_t done = 0;
    while (count - done >= chunk_elems) {
        const size_t at = done * ElemBytes;
        scalar::storeu_si128(d + at, kernel(scalar::load_unaligned(l + at), scalar::load_unaligned(r + at)));
        done += chunk_elems;
    }
    if (done != count) {
        const size_t tail = (count - done) * ElemBytes;
        alignas(16) byte_t left[sizeof(vec128_t)] = {};
        alignas(16) byte_t right[sizeof(vec128_t)] = {};
        scalar::memory_copy(left, l + (done * ElemBytes), tail);
        scalar::memory_copy(right, r + (done * ElemBytes), tail);
        byte_t out[sizeof(vec128_t)];
        scalar::storeu_si128(out, kernel(scalar::load_unaligned(left), scalar::load_unaligned(right)));
        scalar::memory_copy(d + (done * ElemBytes), out, tail);
    }
}

/**
 * @brief 批量执行一元定长内核
 * @tparam ElemBytes 元素字节数
 * @tparam Kernel 以一个 vec128_t 为参数、返回 vec128_t 的可调用对象
 * @param dst 目标缓冲区
 * @param src 源缓冲区
 * @param count 元素个数
 * @param kernel 定长内核
 */
template <size_t ElemBytes, typename Kernel>
NEFORCE_ALWAYS_INLINE_INLINE void block_unary(void* dst, const void* src, const size_t count, Kernel kernel) noexcept {
    if (dst == nullptr || src == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<byte_t*>(dst);
    const auto* s = static_cast<const byte_t*>(src);
    constexpr size_t chunk_elems = sizeof(vec128_t) / ElemBytes;
    size_t done = 0;
    while (count - done >= chunk_elems) {
        const size_t at = done * ElemBytes;
        scalar::storeu_si128(d + at, kernel(scalar::load_unaligned(s + at)));
        done += chunk_elems;
    }
    if (done != count) {
        const size_t tail = (count - done) * ElemBytes;
        alignas(16) byte_t in[sizeof(vec128_t)] = {};
        scalar::memory_copy(in, s + (done * ElemBytes), tail);
        byte_t out[sizeof(vec128_t)];
        scalar::storeu_si128(out, kernel(scalar::load_unaligned(in)));
        scalar::memory_copy(d + (done * ElemBytes), out, tail);
    }
}


/**
 * @brief 把字节向量按浮点向量解释后执行二元内核
 * @tparam VecF 浮点向量类型
 * @tparam Kernel 以两个浮点向量为参数、返回浮点向量的可调用对象
 * @param a 左操作数（字节视图）
 * @param b 右操作数（字节视图）
 * @param kernel 定长内核
 * @return 结果（字节视图）
 */
template <typename VecF, typename Kernel>
NEFORCE_ALWAYS_INLINE_INLINE vec128_t block_float_binary(const vec128_t a, const vec128_t b, Kernel kernel) noexcept {
    VecF fa;
    VecF fb;
    VecF out;
    scalar::memory_copy(&fa, &a, sizeof(VecF));
    scalar::memory_copy(&fb, &b, sizeof(VecF));
    out = kernel(fa, fb);
    vec128_t result;
    scalar::memory_copy(&result, &out, sizeof(VecF));
    return result;
}

NEFORCE_END_INNER__
/// @endcond

/**
 * @addtogroup SIMD
 * @{
 */

/**
 * @brief 批量8-bit 有符号整数加法
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 1 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void add_i8_block(void* dst, const void* lhs, const void* rhs,
                                               const size_t count) noexcept {
    inner::block_binary<1>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::add_i8(a, b); });
}

/**
 * @brief 批量16-bit 有符号整数加法
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 2 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void add_i16_block(void* dst, const void* lhs, const void* rhs,
                                                const size_t count) noexcept {
    inner::block_binary<2>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::add_i16(a, b); });
}

/**
 * @brief 批量32-bit 有符号整数加法
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 4 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void add_i32_block(void* dst, const void* lhs, const void* rhs,
                                                const size_t count) noexcept {
    inner::block_binary<4>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::add_i32(a, b); });
}

/**
 * @brief 批量64-bit 有符号整数加法
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 8 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void add_i64_block(void* dst, const void* lhs, const void* rhs,
                                                const size_t count) noexcept {
    inner::block_binary<8>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::add_i64(a, b); });
}

/**
 * @brief 批量8-bit 有符号整数减法
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 1 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void sub_i8_block(void* dst, const void* lhs, const void* rhs,
                                               const size_t count) noexcept {
    inner::block_binary<1>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::sub_i8(a, b); });
}

/**
 * @brief 批量16-bit 有符号整数减法
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 2 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void sub_i16_block(void* dst, const void* lhs, const void* rhs,
                                                const size_t count) noexcept {
    inner::block_binary<2>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::sub_i16(a, b); });
}

/**
 * @brief 批量32-bit 有符号整数减法
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 4 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void sub_i32_block(void* dst, const void* lhs, const void* rhs,
                                                const size_t count) noexcept {
    inner::block_binary<4>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::sub_i32(a, b); });
}

/**
 * @brief 批量64-bit 有符号整数减法
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 8 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void sub_i64_block(void* dst, const void* lhs, const void* rhs,
                                                const size_t count) noexcept {
    inner::block_binary<8>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::sub_i64(a, b); });
}

/**
 * @brief 批量16-bit 整数乘法（取低半部分）
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 2 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void mullo_i16_block(void* dst, const void* lhs, const void* rhs,
                                                  const size_t count) noexcept {
    inner::block_binary<2>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::mullo_i16(a, b); });
}

/**
 * @brief 批量32-bit 整数乘法（取低半部分）
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 4 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void mullo_i32_block(void* dst, const void* lhs, const void* rhs,
                                                  const size_t count) noexcept {
    inner::block_binary<4>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::mullo_i32(a, b); });
}

/**
 * @brief 批量16-bit 有符号整数乘法（取高半部分）
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 2 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void mulhi_i16_block(void* dst, const void* lhs, const void* rhs,
                                                  const size_t count) noexcept {
    inner::block_binary<2>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::mulhi_i16(a, b); });
}

/**
 * @brief 批量8-bit 相邻对乘加（相邻 i8 相乘后累加为 i16）
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 相邻对个数，每对 2 字节（输出同样每对 2 字节）
 */
NEFORCE_ALWAYS_INLINE_INLINE void madds_i8x16_block(void* dst, const void* lhs, const void* rhs,
                                                    const size_t count) noexcept {
    inner::block_binary<2>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::madds_i8x16(a, b); });
}

/**
 * @brief 批量8-bit 有符号饱和加法
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 1 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void saturated_add_i8_block(void* dst, const void* lhs, const void* rhs,
                                                         const size_t count) noexcept {
    inner::block_binary<1>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::saturated_add_i8(a, b); });
}

/**
 * @brief 批量16-bit 有符号饱和加法
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 2 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void saturated_add_i16_block(void* dst, const void* lhs, const void* rhs,
                                                          const size_t count) noexcept {
    inner::block_binary<2>(dst, lhs, rhs, count,
                           [](vec128_t a, vec128_t b) { return scalar::saturated_add_i16(a, b); });
}

/**
 * @brief 批量8-bit 无符号饱和加法
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 1 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void saturated_add_u8_block(void* dst, const void* lhs, const void* rhs,
                                                         const size_t count) noexcept {
    inner::block_binary<1>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::saturated_add_u8(a, b); });
}

/**
 * @brief 批量16-bit 无符号饱和加法
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 2 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void saturated_add_u16_block(void* dst, const void* lhs, const void* rhs,
                                                          const size_t count) noexcept {
    inner::block_binary<2>(dst, lhs, rhs, count,
                           [](vec128_t a, vec128_t b) { return scalar::saturated_add_u16(a, b); });
}

/**
 * @brief 批量8-bit 有符号饱和减法
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 1 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void saturated_sub_i8_block(void* dst, const void* lhs, const void* rhs,
                                                         const size_t count) noexcept {
    inner::block_binary<1>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::saturated_sub_i8(a, b); });
}

/**
 * @brief 批量16-bit 有符号饱和减法
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 2 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void saturated_sub_i16_block(void* dst, const void* lhs, const void* rhs,
                                                          const size_t count) noexcept {
    inner::block_binary<2>(dst, lhs, rhs, count,
                           [](vec128_t a, vec128_t b) { return scalar::saturated_sub_i16(a, b); });
}

/**
 * @brief 批量8-bit 无符号饱和减法
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 1 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void saturated_sub_u8_block(void* dst, const void* lhs, const void* rhs,
                                                         const size_t count) noexcept {
    inner::block_binary<1>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::saturated_sub_u8(a, b); });
}

/**
 * @brief 批量16-bit 无符号饱和减法
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 2 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void saturated_sub_u16_block(void* dst, const void* lhs, const void* rhs,
                                                          const size_t count) noexcept {
    inner::block_binary<2>(dst, lhs, rhs, count,
                           [](vec128_t a, vec128_t b) { return scalar::saturated_sub_u16(a, b); });
}

/**
 * @brief 批量8-bit 有符号最小值
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 1 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void min_i8_block(void* dst, const void* lhs, const void* rhs,
                                               const size_t count) noexcept {
    inner::block_binary<1>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::min_i8(a, b); });
}

/**
 * @brief 批量16-bit 有符号最小值
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 2 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void min_i16_block(void* dst, const void* lhs, const void* rhs,
                                                const size_t count) noexcept {
    inner::block_binary<2>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::min_i16(a, b); });
}

/**
 * @brief 批量32-bit 有符号最小值
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 4 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void min_i32_block(void* dst, const void* lhs, const void* rhs,
                                                const size_t count) noexcept {
    inner::block_binary<4>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::min_i32(a, b); });
}

/**
 * @brief 批量8-bit 无符号最小值
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 1 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void min_u8_block(void* dst, const void* lhs, const void* rhs,
                                               const size_t count) noexcept {
    inner::block_binary<1>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::min_u8(a, b); });
}

/**
 * @brief 批量16-bit 无符号最小值
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 2 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void min_u16_block(void* dst, const void* lhs, const void* rhs,
                                                const size_t count) noexcept {
    inner::block_binary<2>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::min_u16(a, b); });
}

/**
 * @brief 批量32-bit 无符号最小值
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 4 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void min_u32_block(void* dst, const void* lhs, const void* rhs,
                                                const size_t count) noexcept {
    inner::block_binary<4>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::min_u32(a, b); });
}

/**
 * @brief 批量8-bit 有符号最大值
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 1 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void max_i8_block(void* dst, const void* lhs, const void* rhs,
                                               const size_t count) noexcept {
    inner::block_binary<1>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::max_i8(a, b); });
}

/**
 * @brief 批量16-bit 有符号最大值
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 2 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void max_i16_block(void* dst, const void* lhs, const void* rhs,
                                                const size_t count) noexcept {
    inner::block_binary<2>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::max_i16(a, b); });
}

/**
 * @brief 批量32-bit 有符号最大值
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 4 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void max_i32_block(void* dst, const void* lhs, const void* rhs,
                                                const size_t count) noexcept {
    inner::block_binary<4>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::max_i32(a, b); });
}

/**
 * @brief 批量8-bit 无符号最大值
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 1 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void max_u8_block(void* dst, const void* lhs, const void* rhs,
                                               const size_t count) noexcept {
    inner::block_binary<1>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::max_u8(a, b); });
}

/**
 * @brief 批量16-bit 无符号最大值
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 2 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void max_u16_block(void* dst, const void* lhs, const void* rhs,
                                                const size_t count) noexcept {
    inner::block_binary<2>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::max_u16(a, b); });
}

/**
 * @brief 批量32-bit 无符号最大值
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 4 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void max_u32_block(void* dst, const void* lhs, const void* rhs,
                                                const size_t count) noexcept {
    inner::block_binary<4>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::max_u32(a, b); });
}

/**
 * @brief 批量8-bit 无符号平均值（向上取整）
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 1 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void avg_u8_block(void* dst, const void* lhs, const void* rhs,
                                               const size_t count) noexcept {
    inner::block_binary<1>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::avg_u8(a, b); });
}

/**
 * @brief 批量16-bit 无符号平均值（向上取整）
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 2 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void avg_u16_block(void* dst, const void* lhs, const void* rhs,
                                                const size_t count) noexcept {
    inner::block_binary<2>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::avg_u16(a, b); });
}

/**
 * @brief 批量8-bit 有符号整数绝对值
 * @param dst 目标缓冲区
 * @param src 源缓冲区
 * @param count 元素个数，每个元素 1 字节
 * @note 指针无需对齐；`dst`、`src` 任一为空或 `count` 为 0 时不写入任何字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void abs_i8_block(void* dst, const void* src, const size_t count) noexcept {
    inner::block_unary<1>(dst, src, count, [](vec128_t a) { return scalar::abs_i8(a); });
}

/**
 * @brief 批量16-bit 有符号整数绝对值
 * @param dst 目标缓冲区
 * @param src 源缓冲区
 * @param count 元素个数，每个元素 2 字节
 * @note 指针无需对齐；`dst`、`src` 任一为空或 `count` 为 0 时不写入任何字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void abs_i16_block(void* dst, const void* src, const size_t count) noexcept {
    inner::block_unary<2>(dst, src, count, [](vec128_t a) { return scalar::abs_i16(a); });
}

/**
 * @brief 批量32-bit 有符号整数绝对值
 * @param dst 目标缓冲区
 * @param src 源缓冲区
 * @param count 元素个数，每个元素 4 字节
 * @note 指针无需对齐；`dst`、`src` 任一为空或 `count` 为 0 时不写入任何字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void abs_i32_block(void* dst, const void* src, const size_t count) noexcept {
    inner::block_unary<4>(dst, src, count, [](vec128_t a) { return scalar::abs_i32(a); });
}

/**
 * @brief 批量按位与
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 1 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void bit_and_block(void* dst, const void* lhs, const void* rhs,
                                                const size_t count) noexcept {
    inner::block_binary<1>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::bit_and(a, b); });
}

/**
 * @brief 批量按位或
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 1 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void bit_or_block(void* dst, const void* lhs, const void* rhs,
                                               const size_t count) noexcept {
    inner::block_binary<1>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::bit_or(a, b); });
}

/**
 * @brief 批量按位异或
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 1 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void bit_xor_block(void* dst, const void* lhs, const void* rhs,
                                                const size_t count) noexcept {
    inner::block_binary<1>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::bit_xor(a, b); });
}

/**
 * @brief 批量按位与非
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 1 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void bit_andnot_block(void* dst, const void* lhs, const void* rhs,
                                                   const size_t count) noexcept {
    inner::block_binary<1>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::bit_andnot(a, b); });
}

/**
 * @brief 批量按位取反
 * @param dst 目标缓冲区
 * @param src 源缓冲区
 * @param count 元素个数，每个元素 1 字节
 * @note 指针无需对齐；`dst`、`src` 任一为空或 `count` 为 0 时不写入任何字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void bit_not_block(void* dst, const void* src, const size_t count) noexcept {
    inner::block_unary<1>(dst, src, count, [](vec128_t a) { return scalar::bit_not(a); });
}

/**
 * @brief 批量8-bit 相等比较
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 1 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void cmpeq_i8_block(void* dst, const void* lhs, const void* rhs,
                                                 const size_t count) noexcept {
    inner::block_binary<1>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::cmpeq_i8(a, b); });
}

/**
 * @brief 批量16-bit 相等比较
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 2 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void cmpeq_i16_block(void* dst, const void* lhs, const void* rhs,
                                                  const size_t count) noexcept {
    inner::block_binary<2>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::cmpeq_i16(a, b); });
}

/**
 * @brief 批量32-bit 相等比较
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 4 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void cmpeq_i32_block(void* dst, const void* lhs, const void* rhs,
                                                  const size_t count) noexcept {
    inner::block_binary<4>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::cmpeq_i32(a, b); });
}

/**
 * @brief 批量64-bit 相等比较
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 8 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void cmpeq_i64_block(void* dst, const void* lhs, const void* rhs,
                                                  const size_t count) noexcept {
    inner::block_binary<8>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::cmpeq_i64(a, b); });
}

/**
 * @brief 批量8-bit 有符号大于比较
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 1 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void cmpgt_i8_block(void* dst, const void* lhs, const void* rhs,
                                                 const size_t count) noexcept {
    inner::block_binary<1>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::cmpgt_i8(a, b); });
}

/**
 * @brief 批量16-bit 有符号大于比较
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 2 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void cmpgt_i16_block(void* dst, const void* lhs, const void* rhs,
                                                  const size_t count) noexcept {
    inner::block_binary<2>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::cmpgt_i16(a, b); });
}

/**
 * @brief 批量32-bit 有符号大于比较
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 4 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void cmpgt_i32_block(void* dst, const void* lhs, const void* rhs,
                                                  const size_t count) noexcept {
    inner::block_binary<4>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::cmpgt_i32(a, b); });
}

/**
 * @brief 批量64-bit 有符号大于比较
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 8 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void cmpgt_i64_block(void* dst, const void* lhs, const void* rhs,
                                                  const size_t count) noexcept {
    inner::block_binary<8>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return scalar::cmpgt_i64(a, b); });
}

/**
 * @brief 批量单精度浮点相等比较
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 4 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void cmpeq_f32_block(void* dst, const void* lhs, const void* rhs,
                                                  const size_t count) noexcept {
    inner::block_binary<4>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) {
        return inner::block_float_binary<vec128f_t>(a, b,
                                                    [](vec128f_t x, vec128f_t y) { return scalar::cmpeq_f32(x, y); });
    });
}

/**
 * @brief 批量单精度浮点大于比较
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 4 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void cmpgt_f32_block(void* dst, const void* lhs, const void* rhs,
                                                  const size_t count) noexcept {
    inner::block_binary<4>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) {
        return inner::block_float_binary<vec128f_t>(a, b,
                                                    [](vec128f_t x, vec128f_t y) { return scalar::cmpgt_f32(x, y); });
    });
}

/**
 * @brief 批量单精度浮点大于等于比较
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 4 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void cmpge_f32_block(void* dst, const void* lhs, const void* rhs,
                                                  const size_t count) noexcept {
    inner::block_binary<4>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) {
        return inner::block_float_binary<vec128f_t>(a, b,
                                                    [](vec128f_t x, vec128f_t y) { return scalar::cmpge_f32(x, y); });
    });
}

/**
 * @brief 批量单精度浮点小于比较
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 4 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void cmplt_f32_block(void* dst, const void* lhs, const void* rhs,
                                                  const size_t count) noexcept {
    inner::block_binary<4>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) {
        return inner::block_float_binary<vec128f_t>(a, b,
                                                    [](vec128f_t x, vec128f_t y) { return scalar::cmplt_f32(x, y); });
    });
}

/**
 * @brief 批量单精度浮点小于等于比较
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 4 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void cmple_f32_block(void* dst, const void* lhs, const void* rhs,
                                                  const size_t count) noexcept {
    inner::block_binary<4>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) {
        return inner::block_float_binary<vec128f_t>(a, b,
                                                    [](vec128f_t x, vec128f_t y) { return scalar::cmple_f32(x, y); });
    });
}

/**
 * @brief 批量双精度浮点相等比较
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 8 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void cmpeq_f64_block(void* dst, const void* lhs, const void* rhs,
                                                  const size_t count) noexcept {
    inner::block_binary<8>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) {
        return inner::block_float_binary<vec128d_t>(a, b,
                                                    [](vec128d_t x, vec128d_t y) { return scalar::cmpeq_f64(x, y); });
    });
}

/**
 * @brief 批量双精度浮点大于比较
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 8 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void cmpgt_f64_block(void* dst, const void* lhs, const void* rhs,
                                                  const size_t count) noexcept {
    inner::block_binary<8>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) {
        return inner::block_float_binary<vec128d_t>(a, b,
                                                    [](vec128d_t x, vec128d_t y) { return scalar::cmpgt_f64(x, y); });
    });
}

/**
 * @brief 批量双精度浮点大于等于比较
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 8 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void cmpge_f64_block(void* dst, const void* lhs, const void* rhs,
                                                  const size_t count) noexcept {
    inner::block_binary<8>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) {
        return inner::block_float_binary<vec128d_t>(a, b,
                                                    [](vec128d_t x, vec128d_t y) { return scalar::cmpge_f64(x, y); });
    });
}

/**
 * @brief 批量双精度浮点小于比较
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 8 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void cmplt_f64_block(void* dst, const void* lhs, const void* rhs,
                                                  const size_t count) noexcept {
    inner::block_binary<8>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) {
        return inner::block_float_binary<vec128d_t>(a, b,
                                                    [](vec128d_t x, vec128d_t y) { return scalar::cmplt_f64(x, y); });
    });
}

/**
 * @brief 批量双精度浮点小于等于比较
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 8 字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void cmple_f64_block(void* dst, const void* lhs, const void* rhs,
                                                  const size_t count) noexcept {
    inner::block_binary<8>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) {
        return inner::block_float_binary<vec128d_t>(a, b,
                                                    [](vec128d_t x, vec128d_t y) { return scalar::cmple_f64(x, y); });
    });
}

/** @} */ // SIMD

NEFORCE_END_SCALAR__
NEFORCE_END_SIMD__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SIMD_BLOCK_SCALAR_HPP__
