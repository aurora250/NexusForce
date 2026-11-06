#include <NeForce/core/string/to_string.hpp>
#include <NeForce/core/config/msvc_intrinsic.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_INNER__

namespace {
    struct float_big_uint {
        static constexpr size_t limb_count = 40;

        uint32_t limbs[limb_count];
        size_t used;

        void reset(const uint64_t value) noexcept {
            used = 0;
            uint64_t rest = value;
            while (rest != 0) {
                limbs[used++] = static_cast<uint32_t>(rest & 0xFFFFFFFFU);
                rest >>= 32;
            }
        }

        NEFORCE_NODISCARD bool is_zero() const noexcept { return used == 0; }

        NEFORCE_NODISCARD bool bit(const size_t index) const noexcept {
            const size_t limb = index / 32;
            return limb < used && ((limbs[limb] >> (index % 32)) & 1U) != 0;
        }

        NEFORCE_NODISCARD bool any_bit_below(const size_t limit) const noexcept {
            const size_t full = limit / 32;
            const size_t partial = limit % 32;
            const size_t scan = full < used ? full : used;
            for (size_t i = 0; i < scan; ++i) {
                if (limbs[i] != 0) {
                    return true;
                }
            }
            if (partial != 0 && full < used) {
                return (limbs[full] & ((1U << partial) - 1U)) != 0;
            }
            return false;
        }

        void mul_small(const uint32_t multiplier) noexcept {
            if (multiplier <= 1 || used == 0) {
                return;
            }
            uint64_t carry = 0;
            for (size_t i = 0; i < used; ++i) {
                const uint64_t product = static_cast<uint64_t>(limbs[i]) * multiplier + carry;
                limbs[i] = static_cast<uint32_t>(product & 0xFFFFFFFFU);
                carry = product >> 32;
            }
            if (carry != 0 && used < limb_count) {
                limbs[used++] = static_cast<uint32_t>(carry);
            }
        }

        void shl_bits(const uint32_t count) noexcept {
            if (used == 0 || count == 0) {
                return;
            }
            const uint32_t word_shift = count / 32;
            const uint32_t bit_shift = count % 32;
            if (word_shift != 0) {
                size_t target = used + word_shift;
                target = min(target, limb_count);
                for (size_t i = target; i-- > word_shift;) {
                    limbs[i] = limbs[i - word_shift];
                }
                for (size_t i = 0; i < word_shift && i < limb_count; ++i) {
                    limbs[i] = 0;
                }
                used = target;
            }
            if (bit_shift != 0) {
                uint32_t carry = 0;
                for (size_t i = 0; i < used; ++i) {
                    const uint64_t shifted = (static_cast<uint64_t>(limbs[i]) << bit_shift) | carry;
                    limbs[i] = static_cast<uint32_t>(shifted & 0xFFFFFFFFU);
                    carry = static_cast<uint32_t>(shifted >> 32);
                }
                if (carry != 0 && used < limb_count) {
                    limbs[used++] = carry;
                }
            }
        }

        int shr_bits(const uint32_t count) noexcept {
            if (count == 0) {
                return 0;
            }
            int kind = 0;
            if (!is_zero() && any_bit_below(count)) {
                kind = bit(count - 1) ? (any_bit_below(count - 1) ? 3 : 2) : 1;
            }
            const uint32_t word_shift = count / 32;
            const uint32_t bit_shift = count % 32;
            if (word_shift >= used) {
                used = 0;
                return kind;
            }
            if (word_shift != 0) {
                for (size_t i = 0; i + word_shift < used; ++i) {
                    limbs[i] = limbs[i + word_shift];
                }
                used -= word_shift;
            }
            if (bit_shift != 0) {
                for (size_t i = 0; i < used; ++i) {
                    const uint64_t low = static_cast<uint64_t>(limbs[i]) >> bit_shift;
                    const uint64_t high =
                            (i + 1 < used) ? (static_cast<uint64_t>(limbs[i + 1]) << (32 - bit_shift)) : 0;
                    limbs[i] = static_cast<uint32_t>((low | high) & 0xFFFFFFFFU);
                }
            }
            while (used != 0 && limbs[used - 1] == 0) {
                --used;
            }
            return kind;
        }

        uint32_t divmod_small(const uint32_t divisor) noexcept {
            uint64_t remainder = 0;
            for (size_t i = used; i-- > 0;) {
                const uint64_t current = (remainder << 32) | limbs[i];
                limbs[i] = static_cast<uint32_t>(current / divisor);
                remainder = current % divisor;
            }
            while (used != 0 && limbs[used - 1] == 0) {
                --used;
            }
            return static_cast<uint32_t>(remainder);
        }
    };

    /// Decimal digit pairs "00".."99", used to emit two digits per table lookup.
    constexpr string_view k_digit_pairs = "00010203040506070809"
                                          "10111213141516171819"
                                          "20212223242526272829"
                                          "30313233343536373839"
                                          "40414243444546474849"
                                          "50515253545556575859"
                                          "60616263646566676869"
                                          "70717273747576777879"
                                          "80818283848586878889"
                                          "90919293949596979899";

    /// 10^0 .. 10^19
    constexpr uint64_t k_pow10[] = {1ULL,
                                    10ULL,
                                    100ULL,
                                    1000ULL,
                                    10000ULL,
                                    100000ULL,
                                    1000000ULL,
                                    10000000ULL,
                                    100000000ULL,
                                    1000000000ULL,
                                    10000000000ULL,
                                    100000000000ULL,
                                    1000000000000ULL,
                                    10000000000000ULL,
                                    100000000000000ULL,
                                    1000000000000000ULL,
                                    10000000000000000ULL,
                                    100000000000000000ULL,
                                    1000000000000000000ULL,
                                    10000000000000000000ULL};

    void float_big_to_decimal(const float_big_uint& value, string& digits) {
        digits.clear();
        if (value.is_zero()) {
            digits = "0";
            return;
        }
        float_big_uint work = value;
        uint32_t groups[44];
        size_t group_count = 0;
        while (!work.is_zero() && group_count < 44) {
            groups[group_count++] = work.divmod_small(1000000000U);
        }
        for (size_t i = group_count; i-- > 0;) {
            const uint32_t group = groups[i];
            if (i == group_count - 1) {
                uint32_t scale = 100000000U;
                bool started = false;
                while (scale != 0) {
                    const uint32_t digit = (group / scale) % 10U;
                    if (started || digit != 0 || scale == 1) {
                        digits += static_cast<char>('0' + digit);
                        started = true;
                    }
                    scale /= 10U;
                }
            } else {
                uint32_t scale = 100000000U;
                for (int d = 0; d < 9; ++d) {
                    digits += static_cast<char>('0' + ((group / scale) % 10U));
                    scale /= 10U;
                }
            }
        }
    }

    void float_decompose(const double value, uint64_t& mantissa, int& exponent) noexcept {
        uint64_t bits = 0;
        memory_copy(&bits, &value, sizeof(bits));
        const uint64_t raw_exponent = (bits >> 52) & 0x7FFULL;
        const uint64_t fraction = bits & 0x000FFFFFFFFFFFFFULL;
        if (raw_exponent == 0) {
            mantissa = fraction;
            exponent = -1074;
        } else {
            mantissa = fraction | 0x0010000000000000ULL;
            exponent = static_cast<int>(raw_exponent) - 1075;
        }
    }

    void float_scaled_floor_big(const uint64_t mantissa, const int exponent, const int scale, string& digits,
                                int& tail_kind) {
        float_big_uint value;
        value.reset(mantissa);
        int remaining = scale;
        while (remaining >= 9) {
            value.mul_small(1000000000U);
            remaining -= 9;
        }
        if (remaining > 0) {
            uint32_t factor = 1;
            for (int i = 0; i < remaining; ++i) {
                factor *= 10U;
            }
            value.mul_small(factor);
        }
        tail_kind = 0;
        if (exponent >= 0) {
            value.shl_bits(static_cast<uint32_t>(exponent));
        } else {
            tail_kind = value.shr_bits(static_cast<uint32_t>(-exponent));
        }
        float_big_to_decimal(value, digits);
    }

    void write_group10(const uint32_t group, char* out) noexcept {
        uint32_t value = group;
        for (ptrdiff_t i = 4; i >= 0; --i) {
            const auto quotient = static_cast<uint32_t>((static_cast<uint64_t>(value) * 1374389535ULL) >> 37);
            const uint32_t remainder = value - quotient * 100U;
            out[static_cast<ptrdiff_t>(2) * i] = k_digit_pairs[2UL * remainder];
            out[static_cast<ptrdiff_t>(2) * i + static_cast<ptrdiff_t>(1)] = k_digit_pairs[2UL * remainder + 1UL];
            value = quotient;
        }
    }

    size_t u64_to_decimal(uint64_t value, char* out) noexcept {
        if (value == 0) {
            out[0] = '0';
            return 1;
        }

        uint32_t groups[3];
        size_t group_count = 0;
        while (value != 0) {
            groups[group_count++] = static_cast<uint32_t>(value % 1000000000ULL);
            value /= 1000000000ULL;
        }

        size_t len = 0;
        for (size_t i = group_count; i-- > 0;) {
            char buffer[10];
            write_group10(groups[i], buffer);
            size_t start = 1;
            if (i + 1 == group_count) {
                start = 0;
                while (start < 9 && buffer[start] == '0') {
                    ++start;
                }
            }
            for (size_t k = start; k < 10; ++k) {
                out[len++] = buffer[k];
            }
        }
        return len;
    }

    void u128_divmod_u32(uint64_t& hi, uint64_t& lo, const uint32_t divisor, uint32_t& remainder) noexcept {
        const uint64_t quotient_hi = hi / divisor;
        uint64_t rest = hi % divisor;
        uint64_t quotient_lo = 0;
        for (int limb = 1; limb >= 0; --limb) {
            const uint64_t part = (lo >> (32 * limb)) & 0xFFFFFFFFULL;
            const uint64_t current = (rest << 32) | part;
            const uint64_t digit = current / divisor;
            rest = current % divisor;
            quotient_lo = (quotient_lo << 32) | digit;
        }
        hi = quotient_hi;
        lo = quotient_lo;
        remainder = static_cast<uint32_t>(rest);
    }

    size_t u128_to_decimal(uint64_t hi, uint64_t lo, char* out) noexcept {
        if (hi == 0) {
            return u64_to_decimal(lo, out);
        }

        uint32_t groups[5];
        size_t group_count = 0;
        while (hi != 0 && group_count < 5) {
            uint32_t remainder = 0;
            u128_divmod_u32(hi, lo, 1000000000U, remainder);
            groups[group_count++] = remainder;
        }

        size_t len = u64_to_decimal(lo, out);
        for (int g = static_cast<int>(group_count) - 1; g >= 0; --g) {
        }
        for (size_t i = group_count; i-- > 0;) {
            char buffer[10];
            write_group10(groups[i], buffer);
            for (int k = 1; k < 10; ++k) {
                out[len++] = buffer[k];
            }
        }
        return len;
    }

    bool shift_left_checked(uint64_t& hi, uint64_t& lo, const int bits) noexcept {
        if (bits <= 0) {
            return true;
        }
        if (bits >= 128) {
            return false;
        }
        if (bits >= 64) {
            if (hi != 0) {
                return false;
            }
            const int shift = bits - 64;
            if (shift > 0 && (lo >> (64 - shift)) != 0) {
                return false;
            }
            hi = lo << shift;
            lo = 0;
            return true;
        }
        if ((hi >> (64 - bits)) != 0) {
            return false;
        }
        hi = (hi << bits) | (lo >> (64 - bits));
        lo <<= bits;
        return true;
    }

    int shift_right_sticky(uint64_t& hi, uint64_t& lo, const int bits) noexcept {
        if (bits <= 0) {
            return 0;
        }
        if (bits >= 128) {
            const bool nonzero = (hi | lo) != 0;
            hi = 0;
            lo = 0;
            return nonzero ? 1 : 0;
        }

        const bool top_dropped = (bits <= 64) ? ((lo >> (bits - 1)) & 1ULL) != 0 : ((hi >> (bits - 65)) & 1ULL) != 0;
        bool lower_dropped = false;
        if (bits <= 64) {
            lower_dropped = bits > 1 && (lo & ((1ULL << (bits - 1)) - 1ULL)) != 0;
        } else {
            const int high_bits = bits - 64;
            const uint64_t high_mask = high_bits > 0 ? ((1ULL << high_bits) - 1ULL) : 0ULL;
            lower_dropped = lo != 0 || (hi & high_mask) != 0;
        }

        int kind = 0;
        if (top_dropped) {
            kind = lower_dropped ? 3 : 2;
        } else if (lower_dropped) {
            kind = 1;
        }

        if (bits >= 64) {
            lo = (bits == 64) ? hi : (hi >> (bits - 64));
            hi = 0;
        } else {
            lo = (lo >> bits) | (hi << (64 - bits));
            hi >>= bits;
        }
        return kind;
    }

    bool float_scaled_floor_fast(const uint64_t mantissa, const int exponent, const int scale, char* out, size_t& len,
                                 int& tail_kind) noexcept {
        if (scale < 0 || scale > 19) {
            return false;
        }

        uint64_t hi = 0;
        uint64_t lo = _NEFORCE __umul128(mantissa, k_pow10[scale], &hi);

        tail_kind = 0;
        if (exponent >= 0) {
            if (!shift_left_checked(hi, lo, exponent)) {
                return false;
            }
        } else {
            tail_kind = shift_right_sticky(hi, lo, -exponent);
        }

        len = u128_to_decimal(hi, lo, out);
        return true;
    }

    void decimal_increment(char* digits, size_t& len) {
        for (size_t i = len; i-- > 0;) {
            if (digits[i] != '9') {
                ++digits[i];
                return;
            }
            digits[i] = '0';
        }
        for (size_t i = len; i > 0; --i) {
            digits[i] = digits[i - 1];
        }
        digits[0] = '1';
        ++len;
    }

    bool round_up(const char* digits, const size_t len, const size_t drop, const bool sticky) {
        const size_t first_dropped = len - drop;
        const char lead = digits[first_dropped];
        if (lead > '5') {
            return true;
        }
        bool rest = sticky;
        for (size_t i = first_dropped + 1; !rest && i < len; ++i) {
            rest = digits[i] != '0';
        }
        if (lead < '5') {
            return false;
        }
        if (rest) {
            return true;
        }
        if (first_dropped == 0) {
            return false;
        }
        return ((digits[first_dropped - 1] - '0') % 2) != 0;
    }

    void float_scaled_floor(const uint64_t mantissa, const int exponent, const int scale, char* out, size_t& len,
                            int& tail_kind) {
        if (float_scaled_floor_fast(mantissa, exponent, scale, out, len, tail_kind)) {
            return;
        }
        string digits;
        int kind = 0;
        float_scaled_floor_big(mantissa, exponent, scale, digits, kind);
        tail_kind = kind;
        len = digits.size();
        memory_copy(out, digits.data(), len);
    }

    int float_decimal_exponent(const uint64_t mantissa, const int exponent) noexcept {
        (void) mantissa;
        const double approx = static_cast<double>(exponent + 52) * 0.30102999566398120;
        return static_cast<int>(approx);
    }
} // namespace


string float_to_string_exact(const double value, int precision, const bool use_scientific, const int max_scale) {
    precision = max(precision, 0);
    precision = min(precision, max_scale);

    uint64_t mantissa = 0;
    int exponent = 0;
    float_decompose(value, mantissa, exponent);

    char text[768];
    size_t len = 0;

    if (value == 0.0) {
        text[len++] = '0';
        if (precision > 0) {
            text[len++] = '.';
            for (int i = 0; i < precision; ++i) {
                text[len++] = '0';
            }
        }
        if (use_scientific) {
            text[len++] = 'e';
            text[len++] = '+';
            text[len++] = '0';
            text[len++] = '0';
        }
        return string(text, len);
    }

    char digits[384];
    size_t digit_len = 0;
    int tail_kind = 0;

    if (use_scientific) {
        int scale = precision - float_decimal_exponent(mantissa, exponent) + 2;
        scale = max(scale, 0);
        float_scaled_floor(mantissa, exponent, scale, digits, digit_len, tail_kind);
        while (digit_len < static_cast<size_t>(precision) + 2) {
            scale += static_cast<int>(precision) + 2 - static_cast<int>(digit_len);
            float_scaled_floor(mantissa, exponent, scale, digits, digit_len, tail_kind);
        }

        const size_t drop = digit_len - static_cast<size_t>(precision) - 1;
        const bool carry = round_up(digits, digit_len, drop, tail_kind != 0);
        digit_len -= drop;
        if (carry) {
            decimal_increment(digits, digit_len);
        }

        size_t trailing = 0;
        while (digit_len > static_cast<size_t>(precision) + 1 && digits[digit_len - 1] == '0') {
            --digit_len;
            ++trailing;
        }

        int decimal_exponent =
                static_cast<int>(digit_len) - 1 + static_cast<int>(drop) - scale + static_cast<int>(trailing);

        text[len++] = digits[0];
        if (precision > 0) {
            text[len++] = '.';
            for (size_t i = 1; i < digit_len; ++i) {
                text[len++] = digits[i];
            }
            for (size_t i = digit_len; i < static_cast<size_t>(precision) + 1; ++i) {
                text[len++] = '0';
            }
        }
        text[len++] = 'e';
        if (decimal_exponent >= 0) {
            text[len++] = '+';
        } else {
            text[len++] = '-';
            decimal_exponent = -decimal_exponent;
        }
        if (decimal_exponent < 10) {
            text[len++] = '0';
        }
        char exponent_digits[8];
        const size_t exponent_len = u64_to_decimal(static_cast<uint64_t>(decimal_exponent), exponent_digits);
        for (size_t i = 0; i < exponent_len; ++i) {
            text[len++] = exponent_digits[i];
        }
        return string(text, len);
    }

    float_scaled_floor(mantissa, exponent, exponent >= 0 ? 0 : precision, digits, digit_len, tail_kind);

    if (exponent < 0 && (tail_kind == 3 || (tail_kind == 2 && ((digits[digit_len - 1] - '0') % 2) != 0))) {
        decimal_increment(digits, digit_len);
    }

    if (exponent >= 0) {
        for (size_t i = 0; i < digit_len; ++i) {
            text[len++] = digits[i];
        }
        if (precision > 0) {
            text[len++] = '.';
            for (int i = 0; i < precision; ++i) {
                text[len++] = '0';
            }
        }
        return string(text, len);
    }

    if (precision == 0) {
        for (size_t i = 0; i < digit_len; ++i) {
            text[len++] = digits[i];
        }
        return string(text, len);
    }

    if (compare_less_equal(digit_len, precision)) {
        const size_t zeros = static_cast<size_t>(precision) + 1 - digit_len;
        text[len++] = '0';
        text[len++] = '.';
        for (size_t i = 0; i + 1 < zeros; ++i) {
            text[len++] = '0';
        }
        for (size_t i = 0; i < digit_len; ++i) {
            text[len++] = digits[i];
        }
        return string(text, len);
    }

    const size_t split = digit_len - static_cast<size_t>(precision);
    for (size_t i = 0; i < split; ++i) {
        text[len++] = digits[i];
    }
    text[len++] = '.';
    for (size_t i = 0; compare_less(i, precision); ++i) {
        text[len++] = digits[split + i];
    }
    return string(text, len);
}

NEFORCE_END_INNER__
NEFORCE_END_NAMESPACE__
