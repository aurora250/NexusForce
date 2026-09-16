#include <NeForce/core/string/to_string.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_INNER__

namespace {
    struct float_big_uint {
        static constexpr size_t limb_count = 40; ///< 32 位肢体数量

        uint32_t limbs[limb_count]; ///< 低位在前
        size_t used;                ///< 有效肢体数量

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

    void float_decimal_increment(string& digits) {
        for (size_t i = digits.size(); i-- > 0;) {
            if (digits[i] != '9') {
                ++digits[i];
                return;
            }
            digits[i] = '0';
        }
        string wider;
        wider.reserve(digits.size() + 1);
        wider += '1';
        wider += digits;
        digits = _NEFORCE move(wider);
    }

    bool float_round_up(const string& digits, const size_t drop, const bool sticky) {
        const size_t first_dropped = digits.size() - drop;
        const char lead = digits[first_dropped];
        if (lead > '5') {
            return true;
        }
        bool rest = sticky;
        for (size_t i = first_dropped + 1; !rest && i < digits.size(); ++i) {
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

    void float_scaled_floor(const uint64_t mantissa, const int exponent, const int scale, string& digits,
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

    int float_decimal_exponent(const double value) {
        const auto log_value = static_cast<double>(logarithm_10(value));
        return static_cast<int>(log_value >= 0 ? log_value : log_value - 1.0);
    }
} // namespace


string float_to_string_exact(const double value, int precision, const bool use_scientific, const int max_scale) {
    precision = max(precision, 0);
    precision = min(precision, max_scale);

    uint64_t mantissa = 0;
    int exponent = 0;
    float_decompose(value, mantissa, exponent);

    string text;
    if (value == 0.0) {
        text = "0";
        if (precision > 0) {
            text += '.';
            text.append(static_cast<size_t>(precision), '0');
        }
        if (use_scientific) {
            text += "e+00";
        }
        return text;
    }

    if (use_scientific) {
        int scale = precision - float_decimal_exponent(value) + 2;
        scale = max(scale, 0);
        string digits;
        int tail_kind = 0;
        float_scaled_floor(mantissa, exponent, scale, digits, tail_kind);
        while (digits.size() < static_cast<size_t>(precision) + 2) {
            scale += static_cast<int>(precision) + 2 - static_cast<int>(digits.size());
            float_scaled_floor(mantissa, exponent, scale, digits, tail_kind);
        }

        const size_t drop = digits.size() - static_cast<size_t>(precision) - 1;
        const bool carry = float_round_up(digits, drop, tail_kind != 0);
        digits.resize(digits.size() - drop);
        if (carry) {
            float_decimal_increment(digits);
        }

        size_t trailing = 0;
        while (digits.size() > static_cast<size_t>(precision) + 1 && digits.back() == '0') {
            digits.pop_back();
            ++trailing;
        }

        int decimal_exponent =
                static_cast<int>(digits.size()) - 1 + static_cast<int>(drop) - scale + static_cast<int>(trailing);

        text += digits[0];
        if (precision > 0) {
            text += '.';
            text.append(digits.data() + 1, digits.size() - 1);
            for (size_t i = digits.size(); i < static_cast<size_t>(precision) + 1; ++i) {
                text += '0';
            }
        }
        text += 'e';
        if (decimal_exponent >= 0) {
            text += '+';
        } else {
            text += '-';
            decimal_exponent = -decimal_exponent;
        }
        if (decimal_exponent < 10) {
            text += '0';
        }
        text += inner::__uint_to_string<char>(static_cast<uint64_t>(decimal_exponent));
        return text;
    }

    string digits;
    int tail_kind = 0;
    float_scaled_floor(mantissa, exponent, exponent >= 0 ? 0 : precision, digits, tail_kind);

    if (exponent < 0 && (tail_kind == 3 || (tail_kind == 2 && ((digits.back() - '0') % 2) != 0))) {
        float_decimal_increment(digits);
    }

    if (exponent >= 0) {
        text += digits;
        if (precision > 0) {
            text += '.';
            text.append(static_cast<size_t>(precision), '0');
        }
        return text;
    }

    if (precision == 0) {
        text += digits;
        return text;
    }

    if (digits.size() <= static_cast<size_t>(precision)) {
        const size_t zeros = static_cast<size_t>(precision) + 1 - digits.size();
        text += '0';
        text += '.';
        text.append(zeros - 1, '0');
        text += digits;
        return text;
    }

    const size_t split = digits.size() - static_cast<size_t>(precision);
    text.append(digits.data(), split);
    text += '.';
    text.append(digits.data() + split, static_cast<size_t>(precision));
    return text;
}

NEFORCE_END_INNER__
NEFORCE_END_NAMESPACE__
