#ifndef NEFORCE_CORE_CONFIG_MSVC_INTRINSIC_HPP__
#define NEFORCE_CORE_CONFIG_MSVC_INTRINSIC_HPP__
#include "NeForce/core/memory/bit.hpp"
NEFORCE_BEGIN_NAMESPACE__

NEFORCE_CONSTEXPR14 uint8_t _addcarry_u64(const uint8_t carry_in, const uint64_t a, const uint64_t b,
                                          uint64_t* out) noexcept {
    const auto a_lo = static_cast<uint32_t>(a);
    const auto a_hi = static_cast<uint32_t>(a >> 32);
    const auto b_lo = static_cast<uint32_t>(b);
    const auto b_hi = static_cast<uint32_t>(b >> 32);

    const uint64_t sum_lo = static_cast<uint64_t>(a_lo) + static_cast<uint64_t>(b_lo) + carry_in;
    const uint64_t sum_hi = static_cast<uint64_t>(a_hi) + static_cast<uint64_t>(b_hi) + (sum_lo >> 32);

    *out = (sum_hi << 32) | (sum_lo & 0xFFFFFFFFULL);
    return static_cast<uint8_t>(sum_hi >> 32);
}

NEFORCE_CONSTEXPR14 uint8_t _subborrow_u64(const uint8_t borrow_in, const uint64_t a, const uint64_t b,
                                           uint64_t* out) noexcept {
    const auto a_lo = static_cast<uint32_t>(a);
    const auto a_hi = static_cast<uint32_t>(a >> 32);
    const auto b_lo = static_cast<uint32_t>(b);
    const auto b_hi = static_cast<uint32_t>(b >> 32);

    const uint64_t diff_lo = static_cast<uint64_t>(a_lo) - static_cast<uint64_t>(b_lo) - borrow_in;
    const uint64_t borrow_lo = (diff_lo >> 63);

    const uint64_t diff_hi = static_cast<uint64_t>(a_hi) - static_cast<uint64_t>(b_hi) - borrow_lo;
    const uint64_t borrow_hi = (diff_hi >> 63);

    *out = ((diff_hi & 0xFFFFFFFFULL) << 32) | (diff_lo & 0xFFFFFFFFULL);
    return static_cast<uint8_t>(borrow_hi);
}

NEFORCE_CONSTEXPR14 uint64_t _umul128(const uint64_t a, const uint64_t b, uint64_t* hi_out) noexcept {
    const auto a_lo = static_cast<uint32_t>(a);
    const auto a_hi = static_cast<uint32_t>(a >> 32);
    const auto b_lo = static_cast<uint32_t>(b);
    const auto b_hi = static_cast<uint32_t>(b >> 32);

    const uint64_t p_ll = static_cast<uint64_t>(a_lo) * b_lo; // [63:0]
    const uint64_t p_lh = static_cast<uint64_t>(a_lo) * b_hi; // [95:32]
    const uint64_t p_hl = static_cast<uint64_t>(a_hi) * b_lo; // [95:32]
    const uint64_t p_hh = static_cast<uint64_t>(a_hi) * b_hi; // [127:64]

    const uint64_t mid = (p_ll >> 32) + (p_lh & 0xFFFFFFFFULL) + (p_hl & 0xFFFFFFFFULL);
    const uint64_t lo = (p_ll & 0xFFFFFFFFULL) | (mid << 32);
    const uint64_t hi = p_hh + (p_lh >> 32) + (p_hl >> 32) + (mid >> 32);

    *hi_out = hi;
    return lo;
}

NEFORCE_CONSTEXPR14 uint32_t div_digit(const uint64_t u_hi32_lo32, const uint32_t v_hi, const uint32_t v_lo,
                                       uint64_t* rem_out) noexcept {
    const auto u_hi32 = static_cast<uint32_t>(u_hi32_lo32 >> 32);
    const auto u_lo32 = static_cast<uint32_t>(u_hi32_lo32);

    const uint64_t uhat = u_hi32_lo32;

    uint64_t qhat = 0, rhat = 0;
    if (u_hi32 >= v_hi) {
        qhat = 0xFFFFFFFFULL;
        rhat = static_cast<uint64_t>(u_hi32 - v_hi) + static_cast<uint64_t>(v_hi);
        rhat = uhat - qhat * v_hi;
    } else {
        qhat = uhat / v_hi;
        rhat = uhat - qhat * v_hi;
    }

    // qhat*v_lo > B*rhat + u_lo32
    while (qhat >= 0x100000000ULL || qhat * v_lo > ((rhat << 32) | u_lo32)) {
        --qhat;
        rhat += v_hi;
        if (rhat >= 0x100000000ULL) {
            break;
        }
    }

    *rem_out = uhat - qhat * v_hi;
    return static_cast<uint32_t>(qhat);
}

NEFORCE_CONSTEXPR14 uint64_t _udiv128(const uint64_t dividend_hi, const uint64_t dividend_lo, uint64_t divisor,
                                      uint64_t* remainder) noexcept {
    if (dividend_hi == 0) {
        if (remainder != nullptr) {
            *remainder = dividend_lo % divisor;
        }
        return dividend_lo / divisor;
    }

    const int shift = clz64(divisor);

    uint64_t d = 0;
    uint64_t u2 = 0, u1 = 0, u0 = 0;

    if (shift == 0) {
        d = divisor;
        u2 = 0;
        u1 = dividend_hi;
        u0 = dividend_lo;
    } else {
        d = divisor << shift;
        u2 = dividend_hi >> (64 - shift);
        u1 = (dividend_hi << shift) | (dividend_lo >> (64 - shift));
        u0 = dividend_lo << shift;
    }

    const auto d_hi = static_cast<uint32_t>(d >> 32);
    const auto d_lo = static_cast<uint32_t>(d);

    uint64_t rem1 = 0;
    uint32_t q1hat = div_digit((u2 << 32) | (u1 >> 32), d_hi, d_lo, &rem1);
    (void) rem1;

    {
        uint64_t t_hi = 0;
        const uint64_t t_lo = _umul128(static_cast<uint64_t>(q1hat), d, &t_hi);
        uint64_t r_hi = u2 - t_hi;
        uint64_t r_lo = u1 - t_lo;
        if (u1 < t_lo) {
            --r_hi;
        }

        while (r_hi != 0 || r_lo >= d) {
            --q1hat;
            const uint64_t new_r_lo = r_lo + d;
            const uint64_t carry = (new_r_lo < r_lo) ? 1ULL : 0ULL;
            r_lo = new_r_lo;
            r_hi += carry;
            if (r_hi >= 2) {
                break;
            }
        }

        u1 = r_lo;
        u2 = 0;
    }

    uint64_t rem0 = 0;
    uint32_t q0hat = div_digit((u1 << 32) | (u0 >> 32), d_hi, d_lo, &rem0);
    (void) rem0;

    {
        uint64_t t_hi = 0;
        const uint64_t t_lo = _umul128(static_cast<uint64_t>(q0hat), d, &t_hi);
        uint64_t r_hi = u1 - t_hi;
        uint64_t r_lo = u0 - t_lo;
        if (u0 < t_lo) {
            --r_hi;
        }

        while (r_hi != 0 || r_lo >= d) {
            --q0hat;
            const uint64_t new_r_lo = r_lo + d;
            const uint64_t carry = (new_r_lo < r_lo) ? 1ULL : 0ULL;
            r_lo = new_r_lo;
            r_hi += carry;
            if (r_hi >= 2) {
                break;
            }
        }

        if (remainder != nullptr) {
            *remainder = r_lo >> shift;
        }
    }

    return (static_cast<uint64_t>(q1hat) << 32) | static_cast<uint64_t>(q0hat);
}

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_CONFIG_MSVC_INTRINSIC_HPP__
