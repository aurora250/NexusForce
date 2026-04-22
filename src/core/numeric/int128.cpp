#include <NeForce/core/exception/exception.hpp>
#include <NeForce/core/memory/bit.hpp>
#include <NeForce/core/numeric/int128.hpp>
#if defined(NEFORCE_COMPILER_MSVC) && defined(NEFORCE_ARCH_BITS_64)
#    include <intrin.h>
#    pragma intrinsic(_umul128, _udiv128, _addcarry_u64, _subborrow_u64, __shiftright128, __shiftleft128)
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
#if defined(NEFORCE_ARCH_BITS_32) && defined(NEFORCE_COMPILER_MSVC)
    uint8_t addcarry_u64(uint8_t carry_in, uint64_t a, uint64_t b, uint64_t* out) noexcept {
        const uint32_t a_lo = static_cast<uint32_t>(a);
        const uint32_t a_hi = static_cast<uint32_t>(a >> 32);
        const uint32_t b_lo = static_cast<uint32_t>(b);
        const uint32_t b_hi = static_cast<uint32_t>(b >> 32);

        const uint64_t sum_lo = static_cast<uint64_t>(a_lo) + static_cast<uint64_t>(b_lo) + carry_in;
        const uint64_t sum_hi = static_cast<uint64_t>(a_hi) + static_cast<uint64_t>(b_hi) + (sum_lo >> 32);

        *out = (sum_hi << 32) | (sum_lo & 0xFFFFFFFFULL);
        return static_cast<uint8_t>(sum_hi >> 32);
    }

    uint8_t subborrow_u64(uint8_t borrow_in, uint64_t a, uint64_t b, uint64_t* out) noexcept {
        const uint32_t a_lo = static_cast<uint32_t>(a);
        const uint32_t a_hi = static_cast<uint32_t>(a >> 32);
        const uint32_t b_lo = static_cast<uint32_t>(b);
        const uint32_t b_hi = static_cast<uint32_t>(b >> 32);

        const uint64_t diff_lo = static_cast<uint64_t>(a_lo) - static_cast<uint64_t>(b_lo) - borrow_in;
        const uint64_t borrow_lo = (diff_lo >> 63);

        const uint64_t diff_hi = static_cast<uint64_t>(a_hi) - static_cast<uint64_t>(b_hi) - borrow_lo;
        const uint64_t borrow_hi = (diff_hi >> 63);

        *out = ((diff_hi & 0xFFFFFFFFULL) << 32) | (diff_lo & 0xFFFFFFFFULL);
        return static_cast<uint8_t>(borrow_hi);
    }

    uint64_t umul128(uint64_t a, uint64_t b, uint64_t* hi_out) noexcept {
        const uint32_t a_lo = static_cast<uint32_t>(a);
        const uint32_t a_hi = static_cast<uint32_t>(a >> 32);
        const uint32_t b_lo = static_cast<uint32_t>(b);
        const uint32_t b_hi = static_cast<uint32_t>(b >> 32);

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

    uint32_t div_digit(uint64_t u_hi32_lo32, uint32_t v_hi, uint32_t v_lo, uint64_t* rem_out) noexcept {
        const uint32_t u_hi32 = static_cast<uint32_t>(u_hi32_lo32 >> 32);
        const uint32_t u_lo32 = static_cast<uint32_t>(u_hi32_lo32);

        uint64_t uhat = u_hi32_lo32;

        uint64_t qhat, rhat;
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

    uint64_t udiv128(uint64_t dividend_hi, uint64_t dividend_lo, uint64_t divisor, uint64_t* remainder) noexcept {
        if (dividend_hi == 0) {
            if (remainder) {
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

        const uint32_t d_hi = static_cast<uint32_t>(d >> 32);
        const uint32_t d_lo = static_cast<uint32_t>(d);

        uint64_t rem1;
        uint32_t q1hat = div_digit((u2 << 32) | (u1 >> 32), // u2*B + u1_hi
                                   d_hi, d_lo, &rem1);
        (void) rem1;

        {
            uint64_t t_hi, t_lo;
            t_lo = umul128(static_cast<uint64_t>(q1hat), d, &t_hi);
            uint64_t r_hi = u2 - t_hi;
            uint64_t r_lo = u1 - t_lo;
            if (u1 < t_lo) {
                --r_hi;
            }

            while (r_hi != 0 || r_lo >= d) {
                --q1hat;
                uint64_t new_r_lo = r_lo + d;
                uint64_t carry = (new_r_lo < r_lo) ? 1ULL : 0ULL;
                r_lo = new_r_lo;
                r_hi += carry;
                if (r_hi >= 2) {
                    break;
                }
            }

            u1 = r_lo;
            u2 = 0;
        }

        uint64_t rem0;
        uint32_t q0hat = div_digit((u1 << 32) | (u0 >> 32), d_hi, d_lo, &rem0);
        (void) rem0;

        {
            uint64_t t_hi, t_lo;
            t_lo = umul128(static_cast<uint64_t>(q0hat), d, &t_hi);
            uint64_t r_hi = u1 - t_hi;
            uint64_t r_lo = u0 - t_lo;
            if (u0 < t_lo) {
                --r_hi;
            }

            while (r_hi != 0 || r_lo >= d) {
                --q0hat;
                uint64_t new_r_lo = r_lo + d;
                uint64_t carry = (new_r_lo < r_lo) ? 1ULL : 0ULL;
                r_lo = new_r_lo;
                r_hi += carry;
                if (r_hi >= 2) {
                    break;
                }
            }

            if (remainder) {
                *remainder = r_lo >> shift;
            }
        }

        return (static_cast<uint64_t>(q1hat) << 32) | static_cast<uint64_t>(q0hat);
    }

#define NF_128

#else
#define NF_128 ::
#endif

    uint128_t mul128(const uint128_t& a, const uint128_t& b) noexcept {
        uint128_t result;
#ifdef NEFORCE_COMPILER_MSVC
        result.lo = NF_128 _umul128(a.lo, b.lo, &result.hi);
        uint64_t t1_hi, t2_hi;
        const uint64_t t1_lo = NF_128 _umul128(a.lo, b.hi, &t1_hi);
        const uint64_t t2_lo = NF_128 _umul128(a.hi, b.lo, &t2_hi);
        const byte_t c1 = NF_128 _addcarry_u64(0, result.hi, t1_lo, &result.hi);
        (void) NF_128 _addcarry_u64(c1, result.hi, t2_lo, &result.hi);
        // 最终进位超出128位，截断丢弃
#else
        unsigned __int128 prod = (static_cast<unsigned __int128>(a.hi) << 64 | a.lo) *
                                 (static_cast<unsigned __int128>(b.hi) << 64 | b.lo);
        result.lo = static_cast<uint64_t>(prod);
        result.hi = static_cast<uint64_t>(prod >> 64);
#endif
        return result;
    }

    void divmod128(const uint128_t& dividend, const uint128_t& divisor, uint128_t& quotient, uint128_t& remainder) {
        if (!divisor) {
            NEFORCE_THROW_EXCEPTION(math_exception("Division by zero"));
        }
        if (divisor == uint128_t(1)) {
            quotient = dividend;
            remainder = uint128_t(static_cast<uint64_t>(0ULL));
            return;
        }
        if (dividend < divisor) {
            quotient = uint128_t(static_cast<uint64_t>(0ULL));
            remainder = dividend;
            return;
        }
#ifdef NEFORCE_COMPILER_MSVC
        if (divisor.hi == 0) {
            uint64_t rem;
            quotient.lo = NF_128 _udiv128(dividend.hi, dividend.lo, divisor.lo, &rem);
            quotient.hi = 0;
            remainder = uint128_t(rem);
            return;
        }
#endif

        const int bits =
                dividend.hi != 0 ? (128 - clz64(dividend.hi)) : (dividend.lo != 0 ? (64 - clz64(dividend.lo)) : 0);

        quotient = uint128_t(static_cast<uint64_t>(0ULL));
        remainder = uint128_t(static_cast<uint64_t>(0ULL));

        for (int i = bits - 1; i >= 0; --i) {
            remainder <<= 1;
            const uint64_t bit = (i >= 64) ? ((dividend.hi >> (i - 64)) & 1ULL) : ((dividend.lo >> i) & 1ULL);
            remainder.lo |= bit;
            if (remainder >= divisor) {
                remainder -= divisor;
                if (i >= 64) {
                    quotient.hi |= (static_cast<uint64_t>(1) << (i - 64));
                } else {
                    quotient.lo |= (static_cast<uint64_t>(1) << i);
                }
            }
        }
    }
} // namespace


uint128_t& uint128_t::operator+=(const uint128_t& other) {
#ifdef NEFORCE_COMPILER_MSVC
    const byte_t carry = NF_128 _addcarry_u64(0, lo, other.lo, &lo);
    NF_128 _addcarry_u64(carry, hi, other.hi, &hi);
#else
    const auto old_lo = this->lo;
    lo += other.lo;
    hi += other.hi + (lo < old_lo);
#endif
    return *this;
}

uint128_t& uint128_t::operator-=(const uint128_t& other) {
#ifdef NEFORCE_COMPILER_MSVC
    const byte_t borrow = NF_128 _subborrow_u64(0, lo, other.lo, &lo);
    NF_128 _subborrow_u64(borrow, hi, other.hi, &hi);
#else
    const auto old_lo = this->lo;
    lo -= other.lo;
    hi -= other.hi - (old_lo < other.lo);
#endif
    return *this;
}

uint128_t& uint128_t::operator*=(const uint128_t& other) {
    *this = mul128(*this, other);
    return *this;
}

uint128_t& uint128_t::operator/=(const uint128_t& other) {
    const uint128_t copy{*this};
    uint128_t r;
    divmod128(copy, other, *this, r);
    return *this;
}

uint128_t& uint128_t::operator%=(const uint128_t& other) {
    const uint128_t copy{*this};
    uint128_t q;
    divmod128(copy, other, q, *this);
    return *this;
}

uint128_t uint128_t::mul64(uint64_t a, uint64_t b) noexcept {
    uint128_t res;
#ifdef NEFORCE_COMPILER_MSVC
    res.lo = NF_128 _umul128(a, b, &res.hi);
#else
    unsigned __int128 prod = static_cast<unsigned __int128>(a) * b;
    res.lo = static_cast<uint64_t>(prod);
    res.hi = static_cast<uint64_t>(prod >> 64);
#endif
    return res;
}

uint64_t uint128_t::div64(uint64_t divisor, uint64_t* remainder) const noexcept {
#ifdef NEFORCE_COMPILER_MSVC
    uint64_t rem;
    uint64_t quot = NF_128 _udiv128(hi, lo, divisor, &rem);
    if (remainder) {
        *remainder = rem;
    }
    return quot;
#else
    unsigned __int128 dividend = (static_cast<unsigned __int128>(hi) << 64) | lo;
    uint64_t quot = static_cast<uint64_t>(dividend / divisor);
    if (remainder) {
        *remainder = static_cast<uint64_t>(dividend % divisor);
    }
    return quot;
#endif
}

int128_t& int128_t::operator+=(const int128_t& other) {
    const uint128_t a = to_uint128();
    const uint128_t b = other.to_uint128();
    *this = a + b;
    return *this;
}

int128_t& int128_t::operator-=(const int128_t& other) {
    const uint128_t a = to_uint128();
    const uint128_t b = other.to_uint128();
    *this = a - b;
    return *this;
}

int128_t& int128_t::operator*=(const int128_t& other) {
    const uint128_t a = to_uint128();
    const uint128_t b = other.to_uint128();
    *this = a * b;
    return *this;
}

int128_t& int128_t::operator/=(const int128_t& other) {
    if (other == 0) {
        NEFORCE_THROW_EXCEPTION(math_exception("Division by zero"));
    }
    const bool neg_a = is_negative();
    const bool neg_b = other.is_negative();
    const int128_t abs_a = neg_a ? -*this : *this;
    const int128_t abs_b = neg_b ? -other : other;
    const uint128_t q = abs_a.to_uint128() / abs_b.to_uint128();
    const int128_t result(q.hi, q.lo);
    *this = (neg_a ^ neg_b) ? -result : result;
    return *this;
}

int128_t& int128_t::operator%=(const int128_t& other) {
    if (other == 0) {
        NEFORCE_THROW_EXCEPTION(math_exception("Division by zero"));
    }
    const bool neg_a = is_negative();
    const int128_t abs_a = neg_a ? -*this : *this;
    const int128_t abs_b = other.is_negative() ? -other : other;
    const uint128_t r = abs_a.to_uint128() % abs_b.to_uint128();
    const int128_t result(r.hi, r.lo);
    *this = neg_a ? -result : result;
    return *this;
}

NEFORCE_END_NAMESPACE__
