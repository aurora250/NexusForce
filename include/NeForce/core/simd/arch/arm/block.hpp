#ifndef NEFORCE_CORE_SIMD_BLOCK_ARM_HPP__
#define NEFORCE_CORE_SIMD_BLOCK_ARM_HPP__

/**
 * @file block.hpp
 * @brief ARM NEON 批量内核
 */

#include "NeForce/core/simd/arithmetic.hpp"
#include "NeForce/core/simd/bitwise.hpp"
#include "NeForce/core/simd/bytes.hpp"
#include "NeForce/core/simd/compare.hpp"
#include "NeForce/core/simd/memory.hpp"
#include "NeForce/core/simd/scalar/block.hpp"
#ifdef NEFORCE_SIMD_BRANCH_ARM
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_SIMD__

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
    scalar::inner::block_binary<1>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::add_i8(a, b); });
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
    scalar::inner::block_binary<2>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::add_i16(a, b); });
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
    scalar::inner::block_binary<4>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::add_i32(a, b); });
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
    scalar::inner::block_binary<8>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::add_i64(a, b); });
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
    scalar::inner::block_binary<1>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::sub_i8(a, b); });
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
    scalar::inner::block_binary<2>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::sub_i16(a, b); });
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
    scalar::inner::block_binary<4>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::sub_i32(a, b); });
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
    scalar::inner::block_binary<8>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::sub_i64(a, b); });
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
    scalar::inner::block_binary<2>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::mullo_i16(a, b); });
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
    scalar::inner::block_binary<4>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::mullo_i32(a, b); });
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
    scalar::inner::block_binary<2>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::mulhi_i16(a, b); });
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
    scalar::inner::block_binary<2>(dst, lhs, rhs, count,
                                   [](vec128_t a, vec128_t b) { return simd::madds_i8x16(a, b); });
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
    scalar::inner::block_binary<1>(dst, lhs, rhs, count,
                                   [](vec128_t a, vec128_t b) { return simd::saturated_add_i8(a, b); });
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
    scalar::inner::block_binary<2>(dst, lhs, rhs, count,
                                   [](vec128_t a, vec128_t b) { return simd::saturated_add_i16(a, b); });
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
    scalar::inner::block_binary<1>(dst, lhs, rhs, count,
                                   [](vec128_t a, vec128_t b) { return simd::saturated_add_u8(a, b); });
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
    scalar::inner::block_binary<2>(dst, lhs, rhs, count,
                                   [](vec128_t a, vec128_t b) { return simd::saturated_add_u16(a, b); });
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
    scalar::inner::block_binary<1>(dst, lhs, rhs, count,
                                   [](vec128_t a, vec128_t b) { return simd::saturated_sub_i8(a, b); });
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
    scalar::inner::block_binary<2>(dst, lhs, rhs, count,
                                   [](vec128_t a, vec128_t b) { return simd::saturated_sub_i16(a, b); });
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
    scalar::inner::block_binary<1>(dst, lhs, rhs, count,
                                   [](vec128_t a, vec128_t b) { return simd::saturated_sub_u8(a, b); });
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
    scalar::inner::block_binary<2>(dst, lhs, rhs, count,
                                   [](vec128_t a, vec128_t b) { return simd::saturated_sub_u16(a, b); });
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
    scalar::inner::block_binary<1>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::min_i8(a, b); });
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
    scalar::inner::block_binary<2>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::min_i16(a, b); });
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
    scalar::inner::block_binary<4>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::min_i32(a, b); });
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
    scalar::inner::block_binary<1>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::min_u8(a, b); });
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
    scalar::inner::block_binary<2>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::min_u16(a, b); });
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
    scalar::inner::block_binary<4>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::min_u32(a, b); });
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
    scalar::inner::block_binary<1>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::max_i8(a, b); });
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
    scalar::inner::block_binary<2>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::max_i16(a, b); });
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
    scalar::inner::block_binary<4>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::max_i32(a, b); });
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
    scalar::inner::block_binary<1>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::max_u8(a, b); });
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
    scalar::inner::block_binary<2>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::max_u16(a, b); });
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
    scalar::inner::block_binary<4>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::max_u32(a, b); });
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
    scalar::inner::block_binary<1>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::avg_u8(a, b); });
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
    scalar::inner::block_binary<2>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::avg_u16(a, b); });
}

/**
 * @brief 批量8-bit 有符号整数绝对值
 * @param dst 目标缓冲区
 * @param src 源缓冲区
 * @param count 元素个数，每个元素 1 字节
 * @note 指针无需对齐；`dst`、`src` 任一为空或 `count` 为 0 时不写入任何字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void abs_i8_block(void* dst, const void* src, const size_t count) noexcept {
    scalar::inner::block_unary<1>(dst, src, count, [](vec128_t a) { return simd::abs_i8(a); });
}

/**
 * @brief 批量16-bit 有符号整数绝对值
 * @param dst 目标缓冲区
 * @param src 源缓冲区
 * @param count 元素个数，每个元素 2 字节
 * @note 指针无需对齐；`dst`、`src` 任一为空或 `count` 为 0 时不写入任何字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void abs_i16_block(void* dst, const void* src, const size_t count) noexcept {
    scalar::inner::block_unary<2>(dst, src, count, [](vec128_t a) { return simd::abs_i16(a); });
}

/**
 * @brief 批量32-bit 有符号整数绝对值
 * @param dst 目标缓冲区
 * @param src 源缓冲区
 * @param count 元素个数，每个元素 4 字节
 * @note 指针无需对齐；`dst`、`src` 任一为空或 `count` 为 0 时不写入任何字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void abs_i32_block(void* dst, const void* src, const size_t count) noexcept {
    scalar::inner::block_unary<4>(dst, src, count, [](vec128_t a) { return simd::abs_i32(a); });
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
    scalar::inner::block_binary<1>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::bit_and(a, b); });
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
    scalar::inner::block_binary<1>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::bit_or(a, b); });
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
    scalar::inner::block_binary<1>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::bit_xor(a, b); });
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
    scalar::inner::block_binary<1>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::bit_andnot(a, b); });
}

/**
 * @brief 批量按位取反
 * @param dst 目标缓冲区
 * @param src 源缓冲区
 * @param count 元素个数，每个元素 1 字节
 * @note 指针无需对齐；`dst`、`src` 任一为空或 `count` 为 0 时不写入任何字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void bit_not_block(void* dst, const void* src, const size_t count) noexcept {
    scalar::inner::block_unary<1>(dst, src, count, [](vec128_t a) { return simd::bit_not(a); });
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
    scalar::inner::block_binary<1>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::cmpeq_i8(a, b); });
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
    scalar::inner::block_binary<2>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::cmpeq_i16(a, b); });
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
    scalar::inner::block_binary<4>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::cmpeq_i32(a, b); });
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
    scalar::inner::block_binary<8>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::cmpeq_i64(a, b); });
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
    scalar::inner::block_binary<1>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::cmpgt_i8(a, b); });
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
    scalar::inner::block_binary<2>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::cmpgt_i16(a, b); });
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
    scalar::inner::block_binary<4>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::cmpgt_i32(a, b); });
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
    scalar::inner::block_binary<8>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) { return simd::cmpgt_i64(a, b); });
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
    scalar::inner::block_binary<4>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) {
        return scalar::inner::block_float_binary<vec128f_t>(
                a, b, [](vec128f_t x, vec128f_t y) { return simd::cmpeq_f32(x, y); });
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
    scalar::inner::block_binary<4>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) {
        return scalar::inner::block_float_binary<vec128f_t>(
                a, b, [](vec128f_t x, vec128f_t y) { return simd::cmpgt_f32(x, y); });
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
    scalar::inner::block_binary<4>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) {
        return scalar::inner::block_float_binary<vec128f_t>(
                a, b, [](vec128f_t x, vec128f_t y) { return simd::cmpge_f32(x, y); });
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
    scalar::inner::block_binary<4>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) {
        return scalar::inner::block_float_binary<vec128f_t>(
                a, b, [](vec128f_t x, vec128f_t y) { return simd::cmplt_f32(x, y); });
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
    scalar::inner::block_binary<4>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) {
        return scalar::inner::block_float_binary<vec128f_t>(
                a, b, [](vec128f_t x, vec128f_t y) { return simd::cmple_f32(x, y); });
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
    scalar::inner::block_binary<8>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) {
        return scalar::inner::block_float_binary<vec128d_t>(
                a, b, [](vec128d_t x, vec128d_t y) { return simd::cmpeq_f64(x, y); });
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
    scalar::inner::block_binary<8>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) {
        return scalar::inner::block_float_binary<vec128d_t>(
                a, b, [](vec128d_t x, vec128d_t y) { return simd::cmpgt_f64(x, y); });
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
    scalar::inner::block_binary<8>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) {
        return scalar::inner::block_float_binary<vec128d_t>(
                a, b, [](vec128d_t x, vec128d_t y) { return simd::cmpge_f64(x, y); });
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
    scalar::inner::block_binary<8>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) {
        return scalar::inner::block_float_binary<vec128d_t>(
                a, b, [](vec128d_t x, vec128d_t y) { return simd::cmplt_f64(x, y); });
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
    scalar::inner::block_binary<8>(dst, lhs, rhs, count, [](vec128_t a, vec128_t b) {
        return scalar::inner::block_float_binary<vec128d_t>(
                a, b, [](vec128d_t x, vec128d_t y) { return simd::cmple_f64(x, y); });
    });
}

/** @} */ // SIMD

NEFORCE_END_SIMD__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_SIMD_BRANCH_ARM
#endif // NEFORCE_CORE_SIMD_BLOCK_ARM_HPP__
