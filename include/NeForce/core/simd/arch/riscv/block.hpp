#ifndef NEFORCE_CORE_SIMD_BLOCK_RISCV_HPP__
#define NEFORCE_CORE_SIMD_BLOCK_RISCV_HPP__
#include "NeForce/core/simd/arithmetic.hpp"
#include "NeForce/core/simd/bitwise.hpp"
#include "NeForce/core/simd/bytes.hpp"
#include "NeForce/core/simd/compare.hpp"
#include "NeForce/core/simd/memory.hpp"
#include "NeForce/core/simd/scalar/block.hpp"
#ifdef NEFORCE_SIMD_BRANCH_RISCV
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<int8_t*>(dst);
    const auto* l = static_cast<const int8_t*>(lhs);
    const auto* r = static_cast<const int8_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e8m8(count - offset);
        __riscv_vse8_v_i8m8(
                d + offset,
                __riscv_vadd_vv_i8m8(__riscv_vle8_v_i8m8(l + offset, vl), __riscv_vle8_v_i8m8(r + offset, vl), vl), vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<int16_t*>(dst);
    const auto* l = static_cast<const int16_t*>(lhs);
    const auto* r = static_cast<const int16_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e16m4(count - offset);
        __riscv_vse16_v_i16m4(
                d + offset,
                __riscv_vadd_vv_i16m4(__riscv_vle16_v_i16m4(l + offset, vl), __riscv_vle16_v_i16m4(r + offset, vl), vl),
                vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<int32_t*>(dst);
    const auto* l = static_cast<const int32_t*>(lhs);
    const auto* r = static_cast<const int32_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e32m2(count - offset);
        __riscv_vse32_v_i32m2(
                d + offset,
                __riscv_vadd_vv_i32m2(__riscv_vle32_v_i32m2(l + offset, vl), __riscv_vle32_v_i32m2(r + offset, vl), vl),
                vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<int64_t*>(dst);
    const auto* l = static_cast<const int64_t*>(lhs);
    const auto* r = static_cast<const int64_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e64m1(count - offset);
        __riscv_vse64_v_i64m1(
                d + offset,
                __riscv_vadd_vv_i64m1(__riscv_vle64_v_i64m1(l + offset, vl), __riscv_vle64_v_i64m1(r + offset, vl), vl),
                vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<int8_t*>(dst);
    const auto* l = static_cast<const int8_t*>(lhs);
    const auto* r = static_cast<const int8_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e8m8(count - offset);
        __riscv_vse8_v_i8m8(
                d + offset,
                __riscv_vsub_vv_i8m8(__riscv_vle8_v_i8m8(l + offset, vl), __riscv_vle8_v_i8m8(r + offset, vl), vl), vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<int16_t*>(dst);
    const auto* l = static_cast<const int16_t*>(lhs);
    const auto* r = static_cast<const int16_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e16m4(count - offset);
        __riscv_vse16_v_i16m4(
                d + offset,
                __riscv_vsub_vv_i16m4(__riscv_vle16_v_i16m4(l + offset, vl), __riscv_vle16_v_i16m4(r + offset, vl), vl),
                vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<int32_t*>(dst);
    const auto* l = static_cast<const int32_t*>(lhs);
    const auto* r = static_cast<const int32_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e32m2(count - offset);
        __riscv_vse32_v_i32m2(
                d + offset,
                __riscv_vsub_vv_i32m2(__riscv_vle32_v_i32m2(l + offset, vl), __riscv_vle32_v_i32m2(r + offset, vl), vl),
                vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<int64_t*>(dst);
    const auto* l = static_cast<const int64_t*>(lhs);
    const auto* r = static_cast<const int64_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e64m1(count - offset);
        __riscv_vse64_v_i64m1(
                d + offset,
                __riscv_vsub_vv_i64m1(__riscv_vle64_v_i64m1(l + offset, vl), __riscv_vle64_v_i64m1(r + offset, vl), vl),
                vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<int16_t*>(dst);
    const auto* l = static_cast<const int16_t*>(lhs);
    const auto* r = static_cast<const int16_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e16m4(count - offset);
        __riscv_vse16_v_i16m4(
                d + offset,
                __riscv_vmul_vv_i16m4(__riscv_vle16_v_i16m4(l + offset, vl), __riscv_vle16_v_i16m4(r + offset, vl), vl),
                vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<int32_t*>(dst);
    const auto* l = static_cast<const int32_t*>(lhs);
    const auto* r = static_cast<const int32_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e32m2(count - offset);
        __riscv_vse32_v_i32m2(
                d + offset,
                __riscv_vmul_vv_i32m2(__riscv_vle32_v_i32m2(l + offset, vl), __riscv_vle32_v_i32m2(r + offset, vl), vl),
                vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<int16_t*>(dst);
    const auto* l = static_cast<const int16_t*>(lhs);
    const auto* r = static_cast<const int16_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e16m4(count - offset);
        __riscv_vse16_v_i16m4(d + offset,
                              __riscv_vmulh_vv_i16m4(__riscv_vle16_v_i16m4(l + offset, vl),
                                                     __riscv_vle16_v_i16m4(r + offset, vl), vl),
                              vl);
        offset += vl;
    }
}

/**
 * @brief 批量8-bit 相邻对乘加（相邻 i8 相乘后累加为 i16）
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 相邻对个数，每对 2 字节（输出同样每对 2 字节）
 * @note 该内核无法映射为单条 RVV 指令，沿用定长内核分块实现；三个指针均无需对齐；`dst`、`lhs`、`rhs` 任一为空或 `count` 为 0 时不写入任何字节
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<int8_t*>(dst);
    const auto* l = static_cast<const int8_t*>(lhs);
    const auto* r = static_cast<const int8_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e8m8(count - offset);
        __riscv_vse8_v_i8m8(
                d + offset,
                __riscv_vsadd_vv_i8m8(__riscv_vle8_v_i8m8(l + offset, vl), __riscv_vle8_v_i8m8(r + offset, vl), vl),
                vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<int16_t*>(dst);
    const auto* l = static_cast<const int16_t*>(lhs);
    const auto* r = static_cast<const int16_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e16m4(count - offset);
        __riscv_vse16_v_i16m4(d + offset,
                              __riscv_vsadd_vv_i16m4(__riscv_vle16_v_i16m4(l + offset, vl),
                                                     __riscv_vle16_v_i16m4(r + offset, vl), vl),
                              vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<uint8_t*>(dst);
    const auto* l = static_cast<const uint8_t*>(lhs);
    const auto* r = static_cast<const uint8_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e8m8(count - offset);
        __riscv_vse8_v_u8m8(
                d + offset,
                __riscv_vsaddu_vv_u8m8(__riscv_vle8_v_u8m8(l + offset, vl), __riscv_vle8_v_u8m8(r + offset, vl), vl),
                vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<uint16_t*>(dst);
    const auto* l = static_cast<const uint16_t*>(lhs);
    const auto* r = static_cast<const uint16_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e16m4(count - offset);
        __riscv_vse16_v_u16m4(d + offset,
                              __riscv_vsaddu_vv_u16m4(__riscv_vle16_v_u16m4(l + offset, vl),
                                                      __riscv_vle16_v_u16m4(r + offset, vl), vl),
                              vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<int8_t*>(dst);
    const auto* l = static_cast<const int8_t*>(lhs);
    const auto* r = static_cast<const int8_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e8m8(count - offset);
        __riscv_vse8_v_i8m8(
                d + offset,
                __riscv_vssub_vv_i8m8(__riscv_vle8_v_i8m8(l + offset, vl), __riscv_vle8_v_i8m8(r + offset, vl), vl),
                vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<int16_t*>(dst);
    const auto* l = static_cast<const int16_t*>(lhs);
    const auto* r = static_cast<const int16_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e16m4(count - offset);
        __riscv_vse16_v_i16m4(d + offset,
                              __riscv_vssub_vv_i16m4(__riscv_vle16_v_i16m4(l + offset, vl),
                                                     __riscv_vle16_v_i16m4(r + offset, vl), vl),
                              vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<uint8_t*>(dst);
    const auto* l = static_cast<const uint8_t*>(lhs);
    const auto* r = static_cast<const uint8_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e8m8(count - offset);
        __riscv_vse8_v_u8m8(
                d + offset,
                __riscv_vssubu_vv_u8m8(__riscv_vle8_v_u8m8(l + offset, vl), __riscv_vle8_v_u8m8(r + offset, vl), vl),
                vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<uint16_t*>(dst);
    const auto* l = static_cast<const uint16_t*>(lhs);
    const auto* r = static_cast<const uint16_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e16m4(count - offset);
        __riscv_vse16_v_u16m4(d + offset,
                              __riscv_vssubu_vv_u16m4(__riscv_vle16_v_u16m4(l + offset, vl),
                                                      __riscv_vle16_v_u16m4(r + offset, vl), vl),
                              vl);
        offset += vl;
    }
}

/**
 * @brief 批量8-bit 有符号整数绝对值
 * @param dst 目标缓冲区
 * @param src 源缓冲区
 * @param count 元素个数，每个元素 1 字节
 * @note RVV 无整型取绝对值指令，按 `max(v, 0 - v)` 组合；指针无需对齐；`dst`、`src` 任一为空或 `count` 为 0 时不写入任何字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void abs_i8_block(void* dst, const void* src, const size_t count) noexcept {
    if (dst == nullptr || src == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<int8_t*>(dst);
    const auto* s = static_cast<const int8_t*>(src);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e8m8(count - offset);
        const auto va = __riscv_vle8_v_i8m8(s + offset, vl);
        __riscv_vse8_v_i8m8(d + offset,
                            __riscv_vmax_vv_i8m8(va, __riscv_vsub_vv_i8m8(__riscv_vmv_v_x_i8m8(0, vl), va, vl), vl),
                            vl);
        offset += vl;
    }
}

/**
 * @brief 批量16-bit 有符号整数绝对值
 * @param dst 目标缓冲区
 * @param src 源缓冲区
 * @param count 元素个数，每个元素 2 字节
 * @note RVV 无整型取绝对值指令，按 `max(v, 0 - v)` 组合；指针无需对齐；`dst`、`src` 任一为空或 `count` 为 0 时不写入任何字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void abs_i16_block(void* dst, const void* src, const size_t count) noexcept {
    if (dst == nullptr || src == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<int16_t*>(dst);
    const auto* s = static_cast<const int16_t*>(src);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e16m4(count - offset);
        const auto va = __riscv_vle16_v_i16m4(s + offset, vl);
        __riscv_vse16_v_i16m4(
                d + offset, __riscv_vmax_vv_i16m4(va, __riscv_vsub_vv_i16m4(__riscv_vmv_v_x_i16m4(0, vl), va, vl), vl),
                vl);
        offset += vl;
    }
}

/**
 * @brief 批量32-bit 有符号整数绝对值
 * @param dst 目标缓冲区
 * @param src 源缓冲区
 * @param count 元素个数，每个元素 4 字节
 * @note RVV 无整型取绝对值指令，按 `max(v, 0 - v)` 组合；指针无需对齐；`dst`、`src` 任一为空或 `count` 为 0 时不写入任何字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void abs_i32_block(void* dst, const void* src, const size_t count) noexcept {
    if (dst == nullptr || src == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<int32_t*>(dst);
    const auto* s = static_cast<const int32_t*>(src);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e32m2(count - offset);
        const auto va = __riscv_vle32_v_i32m2(s + offset, vl);
        __riscv_vse32_v_i32m2(
                d + offset, __riscv_vmax_vv_i32m2(va, __riscv_vsub_vv_i32m2(__riscv_vmv_v_x_i32m2(0, vl), va, vl), vl),
                vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<int8_t*>(dst);
    const auto* l = static_cast<const int8_t*>(lhs);
    const auto* r = static_cast<const int8_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e8m8(count - offset);
        __riscv_vse8_v_i8m8(
                d + offset,
                __riscv_vmin_vv_i8m8(__riscv_vle8_v_i8m8(l + offset, vl), __riscv_vle8_v_i8m8(r + offset, vl), vl), vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<int16_t*>(dst);
    const auto* l = static_cast<const int16_t*>(lhs);
    const auto* r = static_cast<const int16_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e16m4(count - offset);
        __riscv_vse16_v_i16m4(
                d + offset,
                __riscv_vmin_vv_i16m4(__riscv_vle16_v_i16m4(l + offset, vl), __riscv_vle16_v_i16m4(r + offset, vl), vl),
                vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<int32_t*>(dst);
    const auto* l = static_cast<const int32_t*>(lhs);
    const auto* r = static_cast<const int32_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e32m2(count - offset);
        __riscv_vse32_v_i32m2(
                d + offset,
                __riscv_vmin_vv_i32m2(__riscv_vle32_v_i32m2(l + offset, vl), __riscv_vle32_v_i32m2(r + offset, vl), vl),
                vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<uint8_t*>(dst);
    const auto* l = static_cast<const uint8_t*>(lhs);
    const auto* r = static_cast<const uint8_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e8m8(count - offset);
        __riscv_vse8_v_u8m8(
                d + offset,
                __riscv_vminu_vv_u8m8(__riscv_vle8_v_u8m8(l + offset, vl), __riscv_vle8_v_u8m8(r + offset, vl), vl),
                vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<uint16_t*>(dst);
    const auto* l = static_cast<const uint16_t*>(lhs);
    const auto* r = static_cast<const uint16_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e16m4(count - offset);
        __riscv_vse16_v_u16m4(d + offset,
                              __riscv_vminu_vv_u16m4(__riscv_vle16_v_u16m4(l + offset, vl),
                                                     __riscv_vle16_v_u16m4(r + offset, vl), vl),
                              vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<uint32_t*>(dst);
    const auto* l = static_cast<const uint32_t*>(lhs);
    const auto* r = static_cast<const uint32_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e32m2(count - offset);
        __riscv_vse32_v_u32m2(d + offset,
                              __riscv_vminu_vv_u32m2(__riscv_vle32_v_u32m2(l + offset, vl),
                                                     __riscv_vle32_v_u32m2(r + offset, vl), vl),
                              vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<int8_t*>(dst);
    const auto* l = static_cast<const int8_t*>(lhs);
    const auto* r = static_cast<const int8_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e8m8(count - offset);
        __riscv_vse8_v_i8m8(
                d + offset,
                __riscv_vmax_vv_i8m8(__riscv_vle8_v_i8m8(l + offset, vl), __riscv_vle8_v_i8m8(r + offset, vl), vl), vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<int16_t*>(dst);
    const auto* l = static_cast<const int16_t*>(lhs);
    const auto* r = static_cast<const int16_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e16m4(count - offset);
        __riscv_vse16_v_i16m4(
                d + offset,
                __riscv_vmax_vv_i16m4(__riscv_vle16_v_i16m4(l + offset, vl), __riscv_vle16_v_i16m4(r + offset, vl), vl),
                vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<int32_t*>(dst);
    const auto* l = static_cast<const int32_t*>(lhs);
    const auto* r = static_cast<const int32_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e32m2(count - offset);
        __riscv_vse32_v_i32m2(
                d + offset,
                __riscv_vmax_vv_i32m2(__riscv_vle32_v_i32m2(l + offset, vl), __riscv_vle32_v_i32m2(r + offset, vl), vl),
                vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<uint8_t*>(dst);
    const auto* l = static_cast<const uint8_t*>(lhs);
    const auto* r = static_cast<const uint8_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e8m8(count - offset);
        __riscv_vse8_v_u8m8(
                d + offset,
                __riscv_vmaxu_vv_u8m8(__riscv_vle8_v_u8m8(l + offset, vl), __riscv_vle8_v_u8m8(r + offset, vl), vl),
                vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<uint16_t*>(dst);
    const auto* l = static_cast<const uint16_t*>(lhs);
    const auto* r = static_cast<const uint16_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e16m4(count - offset);
        __riscv_vse16_v_u16m4(d + offset,
                              __riscv_vmaxu_vv_u16m4(__riscv_vle16_v_u16m4(l + offset, vl),
                                                     __riscv_vle16_v_u16m4(r + offset, vl), vl),
                              vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<uint32_t*>(dst);
    const auto* l = static_cast<const uint32_t*>(lhs);
    const auto* r = static_cast<const uint32_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e32m2(count - offset);
        __riscv_vse32_v_u32m2(d + offset,
                              __riscv_vmaxu_vv_u32m2(__riscv_vle32_v_u32m2(l + offset, vl),
                                                     __riscv_vle32_v_u32m2(r + offset, vl), vl),
                              vl);
        offset += vl;
    }
}

/**
 * @brief 批量8-bit 无符号平均值（向上取整）
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 1 字节
 * @note 采用 `max(a, b) - ((max(a, b) - min(a, b)) >> 1)` 的等价形式（即 ceil((a+b)/2)）；三个指针均无需对齐；`dst`、`lhs`、`rhs` 任一为空或 `count` 为 0 时不写入任何字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void avg_u8_block(void* dst, const void* lhs, const void* rhs,
                                               const size_t count) noexcept {
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<uint8_t*>(dst);
    const auto* l = static_cast<const uint8_t*>(lhs);
    const auto* r = static_cast<const uint8_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e8m8(count - offset);
        const auto va = __riscv_vle8_v_u8m8(l + offset, vl);
        const auto vb = __riscv_vle8_v_u8m8(r + offset, vl);
        const auto hi = __riscv_vmaxu_vv_u8m8(va, vb, vl);
        const auto lo = __riscv_vminu_vv_u8m8(va, vb, vl);
        __riscv_vse8_v_u8m8(d + offset,
                            __riscv_vsub_vv_u8m8(hi, __riscv_vsrl_vx_u8m8(__riscv_vsub_vv_u8m8(hi, lo, vl), 1, vl), vl),
                            vl);
        offset += vl;
    }
}

/**
 * @brief 批量16-bit 无符号平均值（向上取整）
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 2 字节
 * @note 采用 `max(a, b) - ((max(a, b) - min(a, b)) >> 1)` 的等价形式（即 ceil((a+b)/2)）；三个指针均无需对齐；`dst`、`lhs`、`rhs` 任一为空或 `count` 为 0 时不写入任何字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void avg_u16_block(void* dst, const void* lhs, const void* rhs,
                                                const size_t count) noexcept {
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<uint16_t*>(dst);
    const auto* l = static_cast<const uint16_t*>(lhs);
    const auto* r = static_cast<const uint16_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e16m4(count - offset);
        const auto va = __riscv_vle16_v_u16m4(l + offset, vl);
        const auto vb = __riscv_vle16_v_u16m4(r + offset, vl);
        const auto hi = __riscv_vmaxu_vv_u16m4(va, vb, vl);
        const auto lo = __riscv_vminu_vv_u16m4(va, vb, vl);
        __riscv_vse16_v_u16m4(
                d + offset,
                __riscv_vsub_vv_u16m4(hi, __riscv_vsrl_vx_u16m4(__riscv_vsub_vv_u16m4(hi, lo, vl), 1, vl), vl), vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<uint8_t*>(dst);
    const auto* l = static_cast<const uint8_t*>(lhs);
    const auto* r = static_cast<const uint8_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e8m8(count - offset);
        __riscv_vse8_v_u8m8(
                d + offset,
                __riscv_vand_vv_u8m8(__riscv_vle8_v_u8m8(l + offset, vl), __riscv_vle8_v_u8m8(r + offset, vl), vl), vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<uint8_t*>(dst);
    const auto* l = static_cast<const uint8_t*>(lhs);
    const auto* r = static_cast<const uint8_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e8m8(count - offset);
        __riscv_vse8_v_u8m8(
                d + offset,
                __riscv_vor_vv_u8m8(__riscv_vle8_v_u8m8(l + offset, vl), __riscv_vle8_v_u8m8(r + offset, vl), vl), vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<uint8_t*>(dst);
    const auto* l = static_cast<const uint8_t*>(lhs);
    const auto* r = static_cast<const uint8_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e8m8(count - offset);
        __riscv_vse8_v_u8m8(
                d + offset,
                __riscv_vxor_vv_u8m8(__riscv_vle8_v_u8m8(l + offset, vl), __riscv_vle8_v_u8m8(r + offset, vl), vl), vl);
        offset += vl;
    }
}

/**
 * @brief 批量按位与非
 * @param dst 目标缓冲区
 * @param lhs 左操作数缓冲区
 * @param rhs 右操作数缓冲区
 * @param count 元素个数，每个元素 1 字节
 * @note RVV 无 and-not 三元指令，按 `vand(a, vnot(b))` 组合；三个指针均无需对齐；`dst`、`lhs`、`rhs` 任一为空或 `count` 为 0 时不写入任何字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void bit_andnot_block(void* dst, const void* lhs, const void* rhs,
                                                   const size_t count) noexcept {
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<uint8_t*>(dst);
    const auto* l = static_cast<const uint8_t*>(lhs);
    const auto* r = static_cast<const uint8_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e8m8(count - offset);
        __riscv_vse8_v_u8m8(d + offset,
                            __riscv_vand_vv_u8m8(__riscv_vle8_v_u8m8(l + offset, vl),
                                                 __riscv_vnot_v_u8m8(__riscv_vle8_v_u8m8(r + offset, vl), vl), vl),
                            vl);
        offset += vl;
    }
}

/**
 * @brief 批量按位取反
 * @param dst 目标缓冲区
 * @param src 源缓冲区
 * @param count 元素个数，每个元素 1 字节
 * @note 指针无需对齐；`dst`、`src` 任一为空或 `count` 为 0 时不写入任何字节
 */
NEFORCE_ALWAYS_INLINE_INLINE void bit_not_block(void* dst, const void* src, const size_t count) noexcept {
    if (dst == nullptr || src == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<uint8_t*>(dst);
    const auto* s = static_cast<const uint8_t*>(src);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e8m8(count - offset);
        __riscv_vse8_v_u8m8(d + offset, __riscv_vnot_v_u8m8(__riscv_vle8_v_u8m8(s + offset, vl), vl), vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<uint8_t*>(dst);
    const auto* l = static_cast<const int8_t*>(lhs);
    const auto* r = static_cast<const int8_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e8m8(count - offset);
        const auto mask =
                __riscv_vmseq_vv_i8m8_b1(__riscv_vle8_v_i8m8(l + offset, vl), __riscv_vle8_v_i8m8(r + offset, vl), vl);
        __riscv_vse8_v_u8m8(
                d + offset,
                __riscv_vmerge_vvm_u8m8(__riscv_vmv_v_x_u8m8(0, vl), __riscv_vmv_v_x_u8m8(0xFF, vl), mask, vl), vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<uint16_t*>(dst);
    const auto* l = static_cast<const int16_t*>(lhs);
    const auto* r = static_cast<const int16_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e16m4(count - offset);
        const auto mask = __riscv_vmseq_vv_i16m4_b4(__riscv_vle16_v_i16m4(l + offset, vl),
                                                    __riscv_vle16_v_i16m4(r + offset, vl), vl);
        __riscv_vse16_v_u16m4(
                d + offset,
                __riscv_vmerge_vvm_u16m4(__riscv_vmv_v_x_u16m4(0, vl), __riscv_vmv_v_x_u16m4(0xFFFF, vl), mask, vl),
                vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<uint32_t*>(dst);
    const auto* l = static_cast<const int32_t*>(lhs);
    const auto* r = static_cast<const int32_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e32m2(count - offset);
        const auto mask = __riscv_vmseq_vv_i32m2_b16(__riscv_vle32_v_i32m2(l + offset, vl),
                                                     __riscv_vle32_v_i32m2(r + offset, vl), vl);
        __riscv_vse32_v_u32m2(d + offset,
                              __riscv_vmerge_vvm_u32m2(__riscv_vmv_v_x_u32m2(0, vl),
                                                       __riscv_vmv_v_x_u32m2(0xFFFFFFFFU, vl), mask, vl),
                              vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<uint64_t*>(dst);
    const auto* l = static_cast<const int64_t*>(lhs);
    const auto* r = static_cast<const int64_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e64m1(count - offset);
        const auto mask = __riscv_vmseq_vv_i64m1_b64(__riscv_vle64_v_i64m1(l + offset, vl),
                                                     __riscv_vle64_v_i64m1(r + offset, vl), vl);
        __riscv_vse64_v_u64m1(d + offset,
                              __riscv_vmerge_vvm_u64m1(__riscv_vmv_v_x_u64m1(0, vl),
                                                       __riscv_vmv_v_x_u64m1(0xFFFFFFFFFFFFFFFFULL, vl), mask, vl),
                              vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<uint8_t*>(dst);
    const auto* l = static_cast<const int8_t*>(lhs);
    const auto* r = static_cast<const int8_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e8m8(count - offset);
        const auto mask =
                __riscv_vmsgt_vv_i8m8_b1(__riscv_vle8_v_i8m8(l + offset, vl), __riscv_vle8_v_i8m8(r + offset, vl), vl);
        __riscv_vse8_v_u8m8(
                d + offset,
                __riscv_vmerge_vvm_u8m8(__riscv_vmv_v_x_u8m8(0, vl), __riscv_vmv_v_x_u8m8(0xFF, vl), mask, vl), vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<uint16_t*>(dst);
    const auto* l = static_cast<const int16_t*>(lhs);
    const auto* r = static_cast<const int16_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e16m4(count - offset);
        const auto mask = __riscv_vmsgt_vv_i16m4_b4(__riscv_vle16_v_i16m4(l + offset, vl),
                                                    __riscv_vle16_v_i16m4(r + offset, vl), vl);
        __riscv_vse16_v_u16m4(
                d + offset,
                __riscv_vmerge_vvm_u16m4(__riscv_vmv_v_x_u16m4(0, vl), __riscv_vmv_v_x_u16m4(0xFFFF, vl), mask, vl),
                vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<uint32_t*>(dst);
    const auto* l = static_cast<const int32_t*>(lhs);
    const auto* r = static_cast<const int32_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e32m2(count - offset);
        const auto mask = __riscv_vmsgt_vv_i32m2_b16(__riscv_vle32_v_i32m2(l + offset, vl),
                                                     __riscv_vle32_v_i32m2(r + offset, vl), vl);
        __riscv_vse32_v_u32m2(d + offset,
                              __riscv_vmerge_vvm_u32m2(__riscv_vmv_v_x_u32m2(0, vl),
                                                       __riscv_vmv_v_x_u32m2(0xFFFFFFFFU, vl), mask, vl),
                              vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<uint64_t*>(dst);
    const auto* l = static_cast<const int64_t*>(lhs);
    const auto* r = static_cast<const int64_t*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e64m1(count - offset);
        const auto mask = __riscv_vmsgt_vv_i64m1_b64(__riscv_vle64_v_i64m1(l + offset, vl),
                                                     __riscv_vle64_v_i64m1(r + offset, vl), vl);
        __riscv_vse64_v_u64m1(d + offset,
                              __riscv_vmerge_vvm_u64m1(__riscv_vmv_v_x_u64m1(0, vl),
                                                       __riscv_vmv_v_x_u64m1(0xFFFFFFFFFFFFFFFFULL, vl), mask, vl),
                              vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<uint32_t*>(dst);
    const auto* l = static_cast<const float*>(lhs);
    const auto* r = static_cast<const float*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e32m2(count - offset);
        const auto mask = __riscv_vmfeq_vv_f32m2_b16(__riscv_vle32_v_f32m2(l + offset, vl),
                                                     __riscv_vle32_v_f32m2(r + offset, vl), vl);
        __riscv_vse32_v_u32m2(d + offset,
                              __riscv_vmerge_vvm_u32m2(__riscv_vmv_v_x_u32m2(0, vl),
                                                       __riscv_vmv_v_x_u32m2(0xFFFFFFFFU, vl), mask, vl),
                              vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<uint32_t*>(dst);
    const auto* l = static_cast<const float*>(lhs);
    const auto* r = static_cast<const float*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e32m2(count - offset);
        const auto mask = __riscv_vmfgt_vv_f32m2_b16(__riscv_vle32_v_f32m2(l + offset, vl),
                                                     __riscv_vle32_v_f32m2(r + offset, vl), vl);
        __riscv_vse32_v_u32m2(d + offset,
                              __riscv_vmerge_vvm_u32m2(__riscv_vmv_v_x_u32m2(0, vl),
                                                       __riscv_vmv_v_x_u32m2(0xFFFFFFFFU, vl), mask, vl),
                              vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<uint32_t*>(dst);
    const auto* l = static_cast<const float*>(lhs);
    const auto* r = static_cast<const float*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e32m2(count - offset);
        const auto mask = __riscv_vmfge_vv_f32m2_b16(__riscv_vle32_v_f32m2(l + offset, vl),
                                                     __riscv_vle32_v_f32m2(r + offset, vl), vl);
        __riscv_vse32_v_u32m2(d + offset,
                              __riscv_vmerge_vvm_u32m2(__riscv_vmv_v_x_u32m2(0, vl),
                                                       __riscv_vmv_v_x_u32m2(0xFFFFFFFFU, vl), mask, vl),
                              vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<uint32_t*>(dst);
    const auto* l = static_cast<const float*>(lhs);
    const auto* r = static_cast<const float*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e32m2(count - offset);
        const auto mask = __riscv_vmflt_vv_f32m2_b16(__riscv_vle32_v_f32m2(l + offset, vl),
                                                     __riscv_vle32_v_f32m2(r + offset, vl), vl);
        __riscv_vse32_v_u32m2(d + offset,
                              __riscv_vmerge_vvm_u32m2(__riscv_vmv_v_x_u32m2(0, vl),
                                                       __riscv_vmv_v_x_u32m2(0xFFFFFFFFU, vl), mask, vl),
                              vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<uint32_t*>(dst);
    const auto* l = static_cast<const float*>(lhs);
    const auto* r = static_cast<const float*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e32m2(count - offset);
        const auto mask = __riscv_vmfle_vv_f32m2_b16(__riscv_vle32_v_f32m2(l + offset, vl),
                                                     __riscv_vle32_v_f32m2(r + offset, vl), vl);
        __riscv_vse32_v_u32m2(d + offset,
                              __riscv_vmerge_vvm_u32m2(__riscv_vmv_v_x_u32m2(0, vl),
                                                       __riscv_vmv_v_x_u32m2(0xFFFFFFFFU, vl), mask, vl),
                              vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<uint64_t*>(dst);
    const auto* l = static_cast<const double*>(lhs);
    const auto* r = static_cast<const double*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e64m1(count - offset);
        const auto mask = __riscv_vmfeq_vv_f64m1_b64(__riscv_vle64_v_f64m1(l + offset, vl),
                                                     __riscv_vle64_v_f64m1(r + offset, vl), vl);
        __riscv_vse64_v_u64m1(d + offset,
                              __riscv_vmerge_vvm_u64m1(__riscv_vmv_v_x_u64m1(0, vl),
                                                       __riscv_vmv_v_x_u64m1(0xFFFFFFFFFFFFFFFFULL, vl), mask, vl),
                              vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<uint64_t*>(dst);
    const auto* l = static_cast<const double*>(lhs);
    const auto* r = static_cast<const double*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e64m1(count - offset);
        const auto mask = __riscv_vmfgt_vv_f64m1_b64(__riscv_vle64_v_f64m1(l + offset, vl),
                                                     __riscv_vle64_v_f64m1(r + offset, vl), vl);
        __riscv_vse64_v_u64m1(d + offset,
                              __riscv_vmerge_vvm_u64m1(__riscv_vmv_v_x_u64m1(0, vl),
                                                       __riscv_vmv_v_x_u64m1(0xFFFFFFFFFFFFFFFFULL, vl), mask, vl),
                              vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<uint64_t*>(dst);
    const auto* l = static_cast<const double*>(lhs);
    const auto* r = static_cast<const double*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e64m1(count - offset);
        const auto mask = __riscv_vmfge_vv_f64m1_b64(__riscv_vle64_v_f64m1(l + offset, vl),
                                                     __riscv_vle64_v_f64m1(r + offset, vl), vl);
        __riscv_vse64_v_u64m1(d + offset,
                              __riscv_vmerge_vvm_u64m1(__riscv_vmv_v_x_u64m1(0, vl),
                                                       __riscv_vmv_v_x_u64m1(0xFFFFFFFFFFFFFFFFULL, vl), mask, vl),
                              vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<uint64_t*>(dst);
    const auto* l = static_cast<const double*>(lhs);
    const auto* r = static_cast<const double*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e64m1(count - offset);
        const auto mask = __riscv_vmflt_vv_f64m1_b64(__riscv_vle64_v_f64m1(l + offset, vl),
                                                     __riscv_vle64_v_f64m1(r + offset, vl), vl);
        __riscv_vse64_v_u64m1(d + offset,
                              __riscv_vmerge_vvm_u64m1(__riscv_vmv_v_x_u64m1(0, vl),
                                                       __riscv_vmv_v_x_u64m1(0xFFFFFFFFFFFFFFFFULL, vl), mask, vl),
                              vl);
        offset += vl;
    }
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
    if (dst == nullptr || lhs == nullptr || rhs == nullptr || count == 0) {
        return;
    }
    auto* d = static_cast<uint64_t*>(dst);
    const auto* l = static_cast<const double*>(lhs);
    const auto* r = static_cast<const double*>(rhs);
    size_t offset = 0;
    while (offset < count) {
        const size_t vl = __riscv_vsetvl_e64m1(count - offset);
        const auto mask = __riscv_vmfle_vv_f64m1_b64(__riscv_vle64_v_f64m1(l + offset, vl),
                                                     __riscv_vle64_v_f64m1(r + offset, vl), vl);
        __riscv_vse64_v_u64m1(d + offset,
                              __riscv_vmerge_vvm_u64m1(__riscv_vmv_v_x_u64m1(0, vl),
                                                       __riscv_vmv_v_x_u64m1(0xFFFFFFFFFFFFFFFFULL, vl), mask, vl),
                              vl);
        offset += vl;
    }
}

/** @} */ // SIMD

NEFORCE_END_SIMD__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_SIMD_BRANCH_RISCV
#endif // NEFORCE_CORE_SIMD_BLOCK_RISCV_HPP__
