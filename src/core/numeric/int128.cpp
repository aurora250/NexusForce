#include <NeForce/core/config/msvc_intrinsic.hpp>
#include <NeForce/core/exception/exception.hpp>
#include <NeForce/core/numeric/int128.hpp>
#if defined(NEFORCE_PLATFORM_WINDOWS) && defined(NEFORCE_ARCH_BITS_64)
#    include <intrin.h>
#    if defined(NEFORCE_COMPILER_MINGW)
#        pragma intrinsic(_umul128, _addcarry_u64, _subborrow_u64, __shiftright128, __shiftleft128)
#        define NEFORCE_NOT_SUPPORT_UDIV128_INTRINSIC
#    elif defined(NEFORCE_COMPILER_LLVM_MINGW)
#        include <adcintrin.h>
#        pragma intrinsic(_umul128, __shiftright128, __shiftleft128)
#        define NEFORCE_NOT_SUPPORT_UDIV128_INTRINSIC
#    else
#        pragma intrinsic(_umul128, _udiv128, _addcarry_u64, _subborrow_u64, __shiftright128, __shiftleft128)
#    endif
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
#ifdef NF_128
#    undef NF_128
#endif

#if defined(NEFORCE_PLATFORM_WINDOWS) && !defined(NEFORCE_ARCH_BITS_64)
#    define NF_128 _NEFORCE
#else
#    define NF_128 ::
#endif

    uint128_t mul128(const uint128_t& a, const uint128_t& b) noexcept {
        uint128_t result;
#ifdef NEFORCE_PLATFORM_WINDOWS
        result.lo = NF_128 _umul128(a.lo, b.lo, &result.hi);
        uint64_t t1_hi = 0, t2_hi = 0;
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
#ifdef NEFORCE_PLATFORM_WINDOWS
        if (divisor.hi == 0) {
            uint64_t rem = 0;
#    ifdef NEFORCE_NOT_SUPPORT_UDIV128_INTRINSIC
            quotient.lo = _udiv128(dividend.hi, dividend.lo, divisor.lo, &rem);
#    else
            quotient.lo = NF_128 _udiv128(dividend.hi, dividend.lo, divisor.lo, &rem);
#    endif
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
#ifdef NEFORCE_PLATFORM_WINDOWS
    const byte_t carry = NF_128 _addcarry_u64(0, lo, other.lo, &lo);
    NF_128 _addcarry_u64(carry, hi, other.hi, &hi);
#else
    const auto old_lo = this->lo;
    lo += other.lo;
    hi += other.hi + static_cast<uint64_t>(lo < old_lo);
#endif
    return *this;
}

uint128_t& uint128_t::operator-=(const uint128_t& other) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const byte_t borrow = NF_128 _subborrow_u64(0, lo, other.lo, &lo);
    NF_128 _subborrow_u64(borrow, hi, other.hi, &hi);
#else
    const auto old_lo = this->lo;
    lo -= other.lo;
    hi -= other.hi - static_cast<uint64_t>(old_lo < other.lo);
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
#ifdef NEFORCE_PLATFORM_WINDOWS
    res.lo = NF_128 _umul128(a, b, &res.hi);
#else
    unsigned __int128 prod = static_cast<unsigned __int128>(a) * b;
    res.lo = static_cast<uint64_t>(prod);
    res.hi = static_cast<uint64_t>(prod >> 64);
#endif
    return res;
}

uint64_t uint128_t::div64(uint64_t divisor, uint64_t* remainder) const noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    uint64_t rem = 0;
#    ifdef NEFORCE_NOT_SUPPORT_UDIV128_INTRINSIC
    uint64_t quot = _udiv128(hi, lo, divisor, &rem);
#    else
    uint64_t quot = NF_128 _udiv128(hi, lo, divisor, &rem);
#    endif
    if (remainder != nullptr) {
        *remainder = rem;
    }
    return quot;
#else
    unsigned __int128 dividend = (static_cast<unsigned __int128>(hi) << 64) | lo;
    auto quot = static_cast<uint64_t>(dividend / divisor);
    if (remainder != nullptr) {
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
    *this = ((neg_a ^ neg_b) != 0) ? -result : result;
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
