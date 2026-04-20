#include <NeForce/core/numeric/int128.hpp>
#ifdef NEFORCE_SUPPORT_INT128
#ifdef NEFORCE_COMPILER_MSVC
#include <intrin.h>
#pragma intrinsic(_umul128, _udiv128, _addcarry_u64, _subborrow_u64, __shiftright128, __shiftleft128)
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
    uint128_t mul10(const uint128_t& val) {
        uint128_t result = val << 3;
        result += val << 1;
        return result;
    }

    uint128_t add_digit(const uint128_t& val, uint64_t digit) {
        uint128_t result = val;
        uint64_t sum = result.lo + digit;
        if (sum < result.lo) {
            ++result.hi;
        }
        result.lo = sum;
        return result;
    }

    uint128_t mul128(const uint128_t& a, const uint128_t& b) noexcept {
        uint128_t result;
#ifdef NEFORCE_COMPILER_MSVC
        uint64_t a0 = a.lo, a1 = a.hi;
        uint64_t b0 = b.lo, b1 = b.hi;
        uint64_t carry, high, low;

        low = ::_umul128(a0, b0, &high);
        result.lo = low;
        result.hi = high;

        uint64_t t1 = ::_umul128(a0, b1, &carry);
        result.hi = ::_addcarry_u64(0, result.hi, t1, &high);
        carry += high;

        uint64_t t2 = ::_umul128(a1, b0, &high);
        result.hi = ::_addcarry_u64(carry, result.hi, t2, &carry);
        carry += high;

        uint64_t t3 = ::_umul128(a1, b1, &high);
        result.hi = ::_addcarry_u64(carry, result.hi, t3, &carry);
#else
        unsigned __int128 prod = static_cast<unsigned __int128>(a.hi) << 64 | a.lo;
        prod *= static_cast<unsigned __int128>(b.hi) << 64 | b.lo;
        result.lo = static_cast<uint64_t>(prod);
        result.hi = static_cast<uint64_t>(prod >> 64);
#endif
        return result;
    }

    void divmod128(const uint128_t& dividend, const uint128_t& divisor, uint128_t& quotient, uint128_t& remainder) {
        if (divisor.hi == 0 && divisor.lo == 0) {
            NEFORCE_THROW_EXCEPTION(math_exception("Division by zero"));
        }
        if (divisor == uint128_t(1)) {
            quotient = dividend;
            remainder = uint128_t(0ULL);
            return;
        }
        if (dividend < divisor) {
            quotient = uint128_t(0ULL);
            remainder = dividend;
            return;
        }
#ifdef NEFORCE_COMPILER_MSVC
        if (divisor.hi == 0) {
            uint64_t rem;
            quotient.lo = _udiv128(dividend.hi, dividend.lo, divisor.lo, &rem);
            quotient.hi = 0;
            remainder = uint128_t(rem);
            return;
        }
#endif

        quotient = uint128_t(0ULL);
        remainder = uint128_t(0ULL);
        for (int i = 127; i >= 0; --i) {
            remainder = remainder << 1;
            if ((dividend >> i) & uint128_t(1)) {
                remainder.lo |= 1;
            }
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


uint128_t::uint128_t(const string& str, int base) :
uint128_t(str.data(), base) {}


NEFORCE_END_NAMESPACE__
#endif
