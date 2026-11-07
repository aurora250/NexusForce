#include <NeForce/core/string/to_string.hpp>
#include <NeForce/core/config/msvc_intrinsic.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_INNER__

namespace {
    struct float_big_uint {
        static constexpr size_t limb_count = 64;

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
        if (scale < 0) {
            bool sticky = false;
            int remaining_digits = -scale;
            while (remaining_digits >= 9) {
                if (value.divmod_small(1000000000U) != 0) {
                    sticky = true;
                }
                remaining_digits -= 9;
            }
            if (remaining_digits > 0) {
                uint32_t factor = 1;
                for (int i = 0; i < remaining_digits; ++i) {
                    factor *= 10U;
                }
                if (value.divmod_small(factor) != 0) {
                    sticky = true;
                }
            }
            if (sticky && tail_kind == 0) {
                tail_kind = 1;
            }
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

    struct shortest_digits {
        char digits[24]{};
        size_t len = 0;
        int exponent = 0; ///< value = digits * 10^exponent
    };

    size_t fixed_text_length(const shortest_digits& value) noexcept;
    size_t scientific_text_length(const shortest_digits& value) noexcept;

    size_t shortest_write_u64(uint64_t value, char* out) noexcept {
        if (value == 0) {
            out[0] = '0';
            return 1;
        }
        char reversed[24];
        size_t len = 0;
        while (value != 0) {
            reversed[len++] = static_cast<char>('0' + value % 10);
            value /= 10;
        }
        for (size_t i = 0; i < len; ++i) {
            out[i] = reversed[len - 1 - i];
        }
        return len;
    }

    struct ryu_table_entry {
        uint64_t hi;
        uint64_t lo;
    };

    constexpr ryu_table_entry k_pow5_split[] = {
            {0x1000000000000000ULL, 0x0000000000000000ULL}, {0x1400000000000000ULL, 0x0000000000000000ULL},
            {0x1900000000000000ULL, 0x0000000000000000ULL}, {0x1F40000000000000ULL, 0x0000000000000000ULL},
            {0x1388000000000000ULL, 0x0000000000000000ULL}, {0x186A000000000000ULL, 0x0000000000000000ULL},
            {0x1E84800000000000ULL, 0x0000000000000000ULL}, {0x1312D00000000000ULL, 0x0000000000000000ULL},
            {0x17D7840000000000ULL, 0x0000000000000000ULL}, {0x1DCD650000000000ULL, 0x0000000000000000ULL},
            {0x12A05F2000000000ULL, 0x0000000000000000ULL}, {0x174876E800000000ULL, 0x0000000000000000ULL},
            {0x1D1A94A200000000ULL, 0x0000000000000000ULL}, {0x12309CE540000000ULL, 0x0000000000000000ULL},
            {0x16BCC41E90000000ULL, 0x0000000000000000ULL}, {0x1C6BF52634000000ULL, 0x0000000000000000ULL},
            {0x11C37937E0800000ULL, 0x0000000000000000ULL}, {0x16345785D8A00000ULL, 0x0000000000000000ULL},
            {0x1BC16D674EC80000ULL, 0x0000000000000000ULL}, {0x1158E460913D0000ULL, 0x0000000000000000ULL},
            {0x15AF1D78B58C4000ULL, 0x0000000000000000ULL}, {0x1B1AE4D6E2EF5000ULL, 0x0000000000000000ULL},
            {0x10F0CF064DD59200ULL, 0x0000000000000000ULL}, {0x152D02C7E14AF680ULL, 0x0000000000000000ULL},
            {0x1A784379D99DB420ULL, 0x0000000000000000ULL}, {0x108B2A2C28029094ULL, 0x0000000000000000ULL},
            {0x14ADF4B7320334B9ULL, 0x0000000000000000ULL}, {0x19D971E4FE8401E7ULL, 0x4000000000000000ULL},
            {0x1027E72F1F128130ULL, 0x8800000000000000ULL}, {0x1431E0FAE6D7217CULL, 0xAA00000000000000ULL},
            {0x193E5939A08CE9DBULL, 0xD480000000000000ULL}, {0x1F8DEF8808B02452ULL, 0xC9A0000000000000ULL},
            {0x13B8B5B5056E16B3ULL, 0xBE04000000000000ULL}, {0x18A6E32246C99C60ULL, 0xAD85000000000000ULL},
            {0x1ED09BEAD87C0378ULL, 0xD8E6400000000000ULL}, {0x13426172C74D822BULL, 0x878FE80000000000ULL},
            {0x1812F9CF7920E2B6ULL, 0x6973E20000000000ULL}, {0x1E17B84357691B64ULL, 0x03D0DA8000000000ULL},
            {0x12CED32A16A1B11EULL, 0x8262889000000000ULL}, {0x178287F49C4A1D66ULL, 0x22FB2AB400000000ULL},
            {0x1D6329F1C35CA4BFULL, 0xABB9F56100000000ULL}, {0x125DFA371A19E6F7ULL, 0xCB54395CA0000000ULL},
            {0x16F578C4E0A060B5ULL, 0xBE2947B3C8000000ULL}, {0x1CB2D6F618C878E3ULL, 0x2DB399A0BA000000ULL},
            {0x11EFC659CF7D4B8DULL, 0xFC90400474400000ULL}, {0x166BB7F0435C9E71ULL, 0x7BB4500591500000ULL},
            {0x1C06A5EC5433C60DULL, 0xDAA16406F5A40000ULL}, {0x118427B3B4A05BC8ULL, 0xA8A4DE8459868000ULL},
            {0x15E531A0A1C872BAULL, 0xD2CE16256FE82000ULL}, {0x1B5E7E08CA3A8F69ULL, 0x87819BAECBE22800ULL},
            {0x111B0EC57E6499A1ULL, 0xF4B1014D3F6D5900ULL}, {0x1561D276DDFDC00AULL, 0x71DD41A08F48AF40ULL},
            {0x1ABA4714957D300DULL, 0x0E549208B31ADB10ULL}, {0x10B46C6CDD6E3E08ULL, 0x28F4DB456FF0C8EAULL},
            {0x14E1878814C9CD8AULL, 0x33321216CBECFB24ULL}, {0x1A19E96A19FC40ECULL, 0xBFFE969C7EE839EDULL},
            {0x105031E2503DA893ULL, 0xF7FF1E21CF512434ULL}, {0x14643E5AE44D12B8ULL, 0xF5FEE5AA43256D41ULL},
            {0x197D4DF19D605767ULL, 0x337E9F14D3EEC892ULL}, {0x1FDCA16E04B86D41ULL, 0x005E46DA08EA7AB6ULL},
            {0x13E9E4E4C2F34448ULL, 0xA03AEC4845928CB2ULL}, {0x18E45E1DF3B0155AULL, 0xC849A75A56F72FDEULL},
            {0x1F1D75A5709C1AB1ULL, 0x7A5C1130ECB4FBD6ULL}, {0x13726987666190AEULL, 0xEC798ABE93F11D65ULL},
            {0x184F03E93FF9F4DAULL, 0xA797ED6E38ED64BFULL}, {0x1E62C4E38FF87211ULL, 0x517DE8C9C728BDEFULL},
            {0x12FDBB0E39FB474AULL, 0xD2EEB17E1C7976B5ULL}, {0x17BD29D1C87A191DULL, 0x87AA5DDDA397D462ULL},
            {0x1DAC74463A989F64ULL, 0xE994F5550C7DC97BULL}, {0x128BC8ABE49F639FULL, 0x11FD195527CE9DEDULL},
            {0x172EBAD6DDC73C86ULL, 0xD67C5FAA71C24568ULL}, {0x1CFA698C95390BA8ULL, 0x8C1B77950E32D6C2ULL},
            {0x121C81F7DD43A749ULL, 0x57912ABD28DFC639ULL}, {0x16A3A275D494911BULL, 0xAD75756C7317B7C8ULL},
            {0x1C4C8B1349B9B562ULL, 0x98D2D2C78FDDA5BAULL}, {0x11AFD6EC0E14115DULL, 0x9F83C3BCB9EA8794ULL},
            {0x161BCCA7119915B5ULL, 0x0764B4ABE8652979ULL}, {0x1BA2BFD0D5FF5B22ULL, 0x493DE1D6E27E73D7ULL},
            {0x1145B7E285BF98F5ULL, 0x6DC6AD264D8F0866ULL}, {0x159725DB272F7F32ULL, 0xC938586FE0F2CA80ULL},
            {0x1AFCEF51F0FB5EFFULL, 0x7B866E8BD92F7D20ULL}, {0x10DE1593369D1B5FULL, 0xAD34051767BDAE34ULL},
            {0x15159AF804446237ULL, 0x9881065D41AD19C1ULL}, {0x1A5B01B605557AC5ULL, 0x7EA147F492186032ULL},
            {0x1078E111C3556CBBULL, 0x6F24CCF8DB4F3C1FULL}, {0x14971956342AC7EAULL, 0x4AEE003712230B27ULL},
            {0x19BCDFABC13579E4ULL, 0xDDA98044D6ABCDF0ULL}, {0x10160BCB58C16C2FULL, 0x0A89F02B062B60B6ULL},
            {0x141B8EBE2EF1C73AULL, 0xCD2C6C35C7B638E4ULL}, {0x1922726DBAAE3909ULL, 0x8077874339A3C71DULL},
            {0x1F6B0F092959C74BULL, 0xE0956914080CB8E4ULL}, {0x13A2E965B9D81C8FULL, 0x6C5D61AC8507F38EULL},
            {0x188BA3BF284E23B3ULL, 0x4774BA17A649F072ULL}, {0x1EAE8CAEF261ACA0ULL, 0x1951E89D8FDC6C8FULL},
            {0x132D17ED577D0BE4ULL, 0x0FD3316279E9C3D9ULL}, {0x17F85DE8AD5C4EDDULL, 0x13C7FDBB186434CFULL},
            {0x1DF67562D8B36294ULL, 0x58B9FD29DE7D4203ULL}, {0x12BA095DC7701D9CULL, 0xB7743E3A2B0E4942ULL},
            {0x17688BB5394C2503ULL, 0xE5514DC8B5D1DB92ULL}, {0x1D42AEA2879F2E44ULL, 0xDEA5A13AE3465277ULL},
            {0x1249AD2594C37CEBULL, 0x0B2784C4CE0BF38AULL}, {0x16DC186EF9F45C25ULL, 0xCDF165F6018EF06DULL},
            {0x1C931E8AB871732FULL, 0x416DBF7381F2AC88ULL}, {0x11DBF316B346E7FDULL, 0x88E497A83137ABD5ULL},
            {0x1652EFDC6018A1FCULL, 0xEB1DBD923D8596CAULL}, {0x1BE7ABD3781ECA7CULL, 0x25E52CF6CCE6FC7DULL},
            {0x1170CB642B133E8DULL, 0x97AF3C1A40105DCEULL}, {0x15CCFE3D35D80E30ULL, 0xFD9B0B20D0147542ULL},
            {0x1B403DCC834E11BDULL, 0x3D01CDE904199292ULL}, {0x1108269FD210CB16ULL, 0x462120B1A28FFB9BULL},
            {0x154A3047C694FDDBULL, 0xD7A968DE0B33FA82ULL}, {0x1A9CBC59B83A3D52ULL, 0xCD93C3158E00F923ULL},
            {0x10A1F5B813246653ULL, 0xC07C59ED78C09BB6ULL}, {0x14CA732617ED7FE8ULL, 0xB09B7068D6F0C2A3ULL},
            {0x19FD0FEF9DE8DFE2ULL, 0xDCC24C830CACF34CULL}, {0x103E29F5C2B18BEDULL, 0xC9F96FD1E7EC180FULL},
            {0x144DB473335DEEE9ULL, 0x3C77CBC661E71E13ULL}, {0x1961219000356AA3ULL, 0x8B95BEB7FA60E598ULL},
            {0x1FB969F40042C54CULL, 0x6E7B2E65F8F91EFEULL}, {0x13D3E2388029BB4FULL, 0xC50CFCFFBB9BB35FULL},
            {0x18C8DAC6A0342A23ULL, 0xB6503C3FAA82A037ULL}, {0x1EFB1178484134ACULL, 0xA3E44B4F95234844ULL},
            {0x135CEAEB2D28C0EBULL, 0xE66EAF11BD360D2BULL}, {0x183425A5F872F126ULL, 0xE00A5AD62C839075ULL},
            {0x1E412F0F768FAD70ULL, 0x980CF18BB7A47493ULL}, {0x12E8BD69AA19CC66ULL, 0x5F0816F752C6C8DCULL},
            {0x17A2ECC414A03F7FULL, 0xF6CA1CB527787B13ULL}, {0x1D8BA7F519C84F5FULL, 0xF47CA3E2715699D7ULL},
            {0x127748F9301D319BULL, 0xF8CDE66D86D62026ULL}, {0x17151B377C247E02ULL, 0xF7016008E88BA830ULL},
            {0x1CDA62055B2D9D83ULL, 0xB4C1B80B22AE923CULL}, {0x12087D4358FC8272ULL, 0x50F91306F5AD1B65ULL},
            {0x168A9C942F3BA30EULL, 0xE53757C8B318623FULL}, {0x1C2D43B93B0A8BD2ULL, 0x9E852DBADFDE7ACFULL},
            {0x119C4A53C4E69763ULL, 0xA3133C94CBEB0CC1ULL}, {0x16035CE8B6203D3CULL, 0x8BD80BB9FEE5CFF1ULL},
            {0x1B843422E3A84C8BULL, 0xAECE0EA87E9F43EEULL}, {0x1132A095CE492FD7ULL, 0x4D40C9294F238A75ULL},
            {0x157F48BB41DB7BCDULL, 0x2090FB73A2EC6D12ULL}, {0x1ADF1AEA12525AC0ULL, 0x68B53A508BA78856ULL},
            {0x10CB70D24B7378B8ULL, 0x417144725748B536ULL}, {0x14FE4D06DE5056E6ULL, 0x51CD958EED1AE283ULL},
            {0x1A3DE04895E46C9FULL, 0xE640FAF2A8619B24ULL}, {0x1066AC2D5DAEC3E3ULL, 0xEFE89CD7A93D00F7ULL},
            {0x14805738B51A74DCULL, 0xEBE2C40D938C4134ULL}, {0x19A06D06E2611214ULL, 0x26DB7510F86F5181ULL},
            {0x100444244D7CAB4CULL, 0x9849292A9B4592F1ULL}, {0x1405552D60DBD61FULL, 0xBE5B73754216F7ADULL},
            {0x1906AA78B912CBA7ULL, 0xADF25052929CB598ULL}, {0x1F485516E7577E91ULL, 0x996EE4673743E2FFULL},
            {0x138D352E5096AF1AULL, 0xFFE54EC0828A6DDFULL}, {0x18708279E4BC5AE1ULL, 0xBFDEA270A32D0957ULL},
            {0x1E8CA3185DEB719AULL, 0x2FD64B0CCBF84BADULL}, {0x1317E5EF3AB32700ULL, 0x5DE5EEE7FF7B2F4CULL},
            {0x17DDDF6B095FF0C0ULL, 0x755F6AA1FF59FB1FULL}, {0x1DD55745CBB7ECF0ULL, 0x92B7454A7F3079E7ULL},
            {0x12A5568B9F52F416ULL, 0x5BB28B4E8F7E4C30ULL}, {0x174EAC2E8727B11BULL, 0xF29F2E22335DDF3CULL},
            {0x1D22573A28F19D62ULL, 0xEF46F9AAC035570BULL}, {0x123576845997025DULL, 0xD58C5C0AB8215667ULL},
            {0x16C2D4256FFCC2F5ULL, 0x4AEF730D6629AC01ULL}, {0x1C73892ECBFBF3B2ULL, 0x9DAB4FD0BFB41701ULL},
            {0x11C835BD3F7D784FULL, 0xA28B11E277D08E60ULL}, {0x163A432C8F5CD663ULL, 0x8B2DD65B15C4B1F9ULL},
            {0x1BC8D3F7B3340BFCULL, 0x6DF94BF1DB35DE77ULL}, {0x115D847AD000877DULL, 0xC4BBCF772901AB0AULL},
            {0x15B4E5998400A95DULL, 0x35EAC354F34215CDULL}, {0x1B221EFFE500D3B4ULL, 0x8365742A30129B40ULL},
            {0x10F5535FEF208450ULL, 0xD21F689A5E0BA108ULL}, {0x1532A837EAE8A565ULL, 0x06A742C0F58E894AULL},
            {0x1A7F5245E5A2CEBEULL, 0x4851137132F22B9DULL}, {0x108F936BAF85C136ULL, 0xED32AC26BFD75B42ULL},
            {0x14B378469B673184ULL, 0xA87F57306FCD3212ULL}, {0x19E056584240FDE5ULL, 0xD29F2CFC8BC07E97ULL},
            {0x102C35F729689EAFULL, 0xA3A37C1DD7584F1EULL}, {0x14374374F3C2C65BULL, 0x8C8C5B254D2E62E6ULL},
            {0x1945145230B377F2ULL, 0x6FAF71EEA079FB9FULL}, {0x1F965966BCE055EFULL, 0x0B9B4E6A48987A87ULL},
            {0x13BDF7E0360C35B5ULL, 0x674111026D5F4C94ULL}, {0x18AD75D8438F4322ULL, 0xC111554308B71FBAULL},
            {0x1ED8D34E547313EBULL, 0x7155AA93CAE4E7A8ULL}, {0x13478410F4C7EC73ULL, 0x26D58A9C5ECF10C9ULL},
            {0x1819651531F9E78FULL, 0xF08AED437682D4FBULL}, {0x1E1FBE5A7E786173ULL, 0xECADA89454238A3AULL},
            {0x12D3D6F88F0B3CE8ULL, 0x73EC895CB4963664ULL}, {0x1788CCB6B2CE0C22ULL, 0x90E7ABB3E1BBC3FDULL},
            {0x1D6AFFE45F818F2BULL, 0x352196A0DA2AB4FDULL}, {0x1262DFEEBBB0F97BULL, 0x0134FE24885AB11EULL},
            {0x16FB97EA6A9D37D9ULL, 0xC1823DADAA715D65ULL}, {0x1CBA7DE5054485D0ULL, 0x31E2CD19150DB4BFULL},
            {0x11F48EAF234AD3A2ULL, 0x1F2DC02FAD2890F7ULL}, {0x1671B25AEC1D888AULL, 0xA6F9303B9872B535ULL},
            {0x1C0E1EF1A724EAADULL, 0x50B77C4A7E8F6282ULL}, {0x1188D357087712ACULL, 0x5272ADAE8F199D91ULL},
            {0x15EB082CCA94D757ULL, 0x670F591A32E004F6ULL}, {0x1B65CA37FD3A0D2DULL, 0x40D32F60BF980633ULL},
            {0x111F9E62FE44483CULL, 0x4883FD9C77BF03E0ULL}, {0x156785FBBDD55A4BULL, 0x5AA4FD0395AEC4D8ULL},
            {0x1AC1677AAD4AB0DEULL, 0x314E3C447B1A760EULL}, {0x10B8E0ACAC4EAE8AULL, 0xDED0E5AACCF089C9ULL},
            {0x14E718D7D7625A2DULL, 0x96851F15802CAC3BULL}, {0x1A20DF0DCD3AF0B8ULL, 0xFC2666DAE037D74AULL},
            {0x10548B68A044D673ULL, 0x9D980048CC22E68EULL}, {0x1469AE42C8560C10ULL, 0x84FE005AFF2BA032ULL},
            {0x198419D37A6B8F14ULL, 0xA63D8071BEF6883EULL}, {0x1FE52048590672D9ULL, 0xCFCCE08E2EB42A4EULL},
            {0x13EF342D37A407C8ULL, 0x21E00C58DD309A70ULL}, {0x18EB0138858D09BAULL, 0x2A580F6F147CC10DULL},
            {0x1F25C186A6F04C28ULL, 0xB4EE134AD99BF150ULL}, {0x137798F428562F99ULL, 0x7114CC0EC80176D2ULL},
            {0x18557F31326BBB7FULL, 0xCD59FF127A01D486ULL}, {0x1E6ADEFD7F06AA5FULL, 0xC0B07ED7188249A8ULL},
            {0x1302CB5E6F642A7BULL, 0xD86E4F466F516E09ULL}, {0x17C37E360B3D351AULL, 0xCE89E3180B25C98BULL},
            {0x1DB45DC38E0C8261ULL, 0x822C5BDE0DEF3BEEULL}, {0x1290BA9A38C7D17CULL, 0xF15BB96AC8B58575ULL},
            {0x1734E940C6F9C5DCULL, 0x2DB2A7C57AE2E6D2ULL}, {0x1D022390F8B83753ULL, 0x391F51B6D99BA086ULL},
            {0x1221563A9B732294ULL, 0x03B3931248014454ULL}, {0x16A9ABC9424FEB39ULL, 0x04A077D6DA019569ULL},
            {0x1C5416BB92E3E607ULL, 0x45C895CC9081FAC3ULL}, {0x11B48E353BCE6FC4ULL, 0x8B9D5D9FDA513CBAULL},
            {0x1621B1C28AC20BB5ULL, 0xAE84B507D0E58BE8ULL}, {0x1BAA1E332D728EA3ULL, 0x1A25E249C51EEEE3ULL},
            {0x114A52DFFC679925ULL, 0xF057AD6E1B33554DULL}, {0x159CE797FB817F6FULL, 0x6C6D98C9A2002AA1ULL},
            {0x1B04217DFA61DF4BULL, 0x4788FEFC0A803549ULL}, {0x10E294EEBC7D2B8FULL, 0x0CB59F5D8690214EULL},
            {0x151B3A2A6B9C7672ULL, 0xCFE30734E83429A1ULL}, {0x1A6208B50683940FULL, 0x83DBC9022241340AULL},
            {0x107D457124123C89ULL, 0xB2695DA15568C086ULL}, {0x149C96CD6D16CBACULL, 0x1F03B509AAC2F0A7ULL},
            {0x19C3BC80C85C7E97ULL, 0x26C4A24C1573ACD1ULL}, {0x101A55D07D39CF1EULL, 0x783AE56F8D684C03ULL},
            {0x1420EB449C8842E6ULL, 0x16499ECB70C25F03ULL}, {0x19292615C3AA539FULL, 0x9BDC067E4CF2F6C4ULL},
            {0x1F736F9B3494E887ULL, 0x82D3081DE02FB476ULL}, {0x13A825C100DD1154ULL, 0xB1C3E512AC1DD0C9ULL},
            {0x18922F31411455A9ULL, 0xDE34DE57572544FCULL}, {0x1EB6BAFD91596B14ULL, 0x55C215ED2CEE963BULL},
            {0x133234DE7AD7E2ECULL, 0xB5994DB43C151DE5ULL}, {0x17FEC216198DDBA7ULL, 0xE2FFA1214B1A655EULL},
            {0x1DFE729B9FF15291ULL, 0xDBBF89699DE0FEB6ULL}, {0x12BF07A143F6D39BULL, 0x2957B5E202AC9F31ULL},
            {0x176EC98994F48881ULL, 0xF3ADA35A8357C6FEULL}, {0x1D4A7BEBFA31AAA2ULL, 0x70990C31242DB8BDULL},
            {0x124E8D737C5F0AA5ULL, 0x865FA79EB69C9376ULL}, {0x16E230D05B76CD4EULL, 0xE7F791866443B854ULL},
            {0x1C9ABD04725480A2ULL, 0xA1F575E7FD54A669ULL}, {0x11E0B622C774D065ULL, 0xA53969B0FE54E801ULL},
            {0x1658E3AB7952047FULL, 0x0E87C41D3DEA2202ULL}, {0x1BEF1C9657A6859EULL, 0xD229B5248D64AA82ULL},
            {0x117571DDF6C81383ULL, 0x435A1136D85EEA91ULL}, {0x15D2CE55747A1864ULL, 0x143095848E76A536ULL},
            {0x1B4781EAD1989E7DULL, 0x193CBAE5B2144E83ULL}, {0x110CB132C2FF630EULL, 0x2FC5F4CF8F4CB112ULL},
            {0x154FDD7F73BF3BD1ULL, 0xBBB77203731FDD56ULL}, {0x1AA3D4DF50AF0AC6ULL, 0x2AA54E844FE7D4ACULL},
            {0x10A6650B926D66BBULL, 0xDAA75112B1F0E4EBULL}, {0x14CFFE4E7708C06AULL, 0xD15125575E6D1E26ULL},
            {0x1A03FDE214CAF085ULL, 0x85A56EAD360865B0ULL}, {0x10427EAD4CFED653ULL, 0x7387652C41C53F8EULL},
            {0x14531E58A03E8BE8ULL, 0x50693E7752368F71ULL}, {0x1967E5EEC84E2EE2ULL, 0x64838E1526C4334EULL},
            {0x1FC1DF6A7A61BA9AULL, 0xFDA4719A70754022ULL}, {0x13D92BA28C7D14A0ULL, 0xDE86C70086494815ULL},
            {0x18CF768B2F9C59C9ULL, 0x162878C0A7DB9A1AULL}, {0x1F03542DFB83703BULL, 0x5BB296F0D1D280A1ULL},
            {0x1362149CBD322625ULL, 0x194F9E5683239064ULL}, {0x183A99C3EC7EAFAEULL, 0x5FA385EC23EC747EULL},
            {0x1E494034E79E5B99ULL, 0xF78C67672CE7919DULL}, {0x12EDC82110C2F940ULL, 0x3AB7C0A07C10BB02ULL},
            {0x17A93A2954F3B790ULL, 0x4965B0C89B14E9C3ULL}, {0x1D9388B3AA30A574ULL, 0x5BBF1CFAC1DA2433ULL},
            {0x127C35704A5E6768ULL, 0xB957721CB92856A0ULL}, {0x171B42CC5CF60142ULL, 0xE7AD4EA3E7726C48ULL},
            {0x1CE2137F74338193ULL, 0xA198A24CE14F075AULL}, {0x120D4C2FA8A030FCULL, 0x44FF65700CD16498ULL},
            {0x16909F3B92C83D3BULL, 0x563F3ECC1005BDBEULL}, {0x1C34C70A777A4C8AULL, 0x2BCF0E7F14072D2EULL},
            {0x11A0FC668AAC6FD6ULL, 0x5B61690F6C847C3DULL}, {0x16093B802D578BCBULL, 0xF239C35347A59B4CULL},
            {0x1B8B8A6038AD6EBEULL, 0xEEC83428198F021FULL}, {0x1137367C236C6537ULL, 0x553D20990FF96153ULL},
            {0x1585041B2C477E85ULL, 0x2A8C68BF53F7B9A8ULL}, {0x1AE64521F7595E26ULL, 0x752F82EF28F5A812ULL},
            {0x10CFEB353A97DAD8ULL, 0x093DB1D57999890BULL}, {0x1503E602893DD18EULL, 0x0B8D1E4AD7FFEB4EULL},
            {0x1A44DF832B8D45F1ULL, 0x8E7065DD8DFFE622ULL}, {0x106B0BB1FB384BB6ULL, 0xF9063FAA78BFEFD5ULL},
            {0x1485CE9E7A065EA4ULL, 0xB747CF9516EFEBCAULL}, {0x19A742461887F64DULL, 0xE519C37A5CABE6BDULL},
            {0x1008896BCF54F9F0ULL, 0xAF301A2C79EB7036ULL}, {0x140AABC6C32A386CULL, 0xDAFC20B798664C43ULL},
            {0x190D56B873F4C688ULL, 0x11BB28E57E7FDF54ULL}, {0x1F50AC6690F1F82AULL, 0x1629F31EDE1FD72AULL},
            {0x13926BC01A973B1AULL, 0x4DDA37F34AD3E67AULL}, {0x187706B0213D09E0ULL, 0xE150C5F01D88E019ULL},
            {0x1E94C85C298C4C59ULL, 0x19A4F76C24EB181FULL}, {0x131CFD3999F7AFB7ULL, 0xB0071AA39712EF13ULL},
            {0x17E43C8800759BA5ULL, 0x9C08E14C7CD7AAD8ULL}, {0x1DDD4BAA0093028FULL, 0x030B199F9C0D958EULL},
            {0x12AA4F4A405BE199ULL, 0x61E6F003C1887D79ULL}, {0x1754E31CD072D9FFULL, 0xBA60AC04B1EA9CD7ULL},
            {0x1D2A1BE4048F907FULL, 0xA8F8D705DE65440DULL}, {0x123A516E82D9BA4FULL, 0xC99B8663AAFF4A88ULL},
            {0x16C8E5CA239028E3ULL, 0xBC0267FC95BF1D2AULL}, {0x1C7B1F3CAC74331CULL, 0xAB0301FBBB2EE474ULL},
            {0x11CCF385EBC89FF1ULL, 0xEAE1E13D54FD4EC9ULL}, {0x1640306766BAC7EEULL, 0x659A598CAA3CA27BULL},
            {0x1BD03C81406979E9ULL, 0xFF00EFEFD4CBCB1AULL}, {0x116225D0C841EC32ULL, 0x3F6095F5E4FF5EF0ULL},
            {0x15BAAF44FA52673EULL, 0xCF38BB735E3F36ACULL}, {0x1B295B1638E7010EULL, 0x8306EA5035CF0457ULL},
            {0x10F9D8EDE39060A9ULL, 0x11E4527221A162B6ULL}, {0x15384F295C7478D3ULL, 0x565D670EAA09BB64ULL},
            {0x1A8662F3B3919708ULL, 0x2BF4C0D2548C2A3DULL}, {0x1093FDD8503AFE65ULL, 0x1B78F88374D79A66ULL},
            {0x14B8FD4E6449BDFEULL, 0x625736A4520D8100ULL}, {0x19E73CA1FD5C2D7DULL, 0xFAED044D6690E140ULL},
            {0x103085E53E599C6EULL, 0xBCD422B0601A8CC8ULL}, {0x143CA75E8DF0038AULL, 0x6C092B5C78212FFAULL},
            {0x194BD136316C046DULL, 0x070B763396297BF8ULL}, {0x1F9EC583BDC70588ULL, 0x48CE53C07BB3DAF6ULL},
            {0x13C33B72569C6375ULL, 0x2D80F4584D5068DAULL}, {0x18B40A4EEC437C52ULL, 0x78E1316E60A48310ULL},
    };

    /// floor(2^K / 5^q) + 1（q = 0..341）
    constexpr ryu_table_entry k_pow5_inv_split[] = {
            {0x2000000000000000ULL, 0x0000000000000001ULL}, {0x1999999999999999ULL, 0x999999999999999AULL},
            {0x147AE147AE147AE1ULL, 0x47AE147AE147AE15ULL}, {0x10624DD2F1A9FBE7ULL, 0x6C8B4395810624DEULL},
            {0x1A36E2EB1C432CA5ULL, 0x7A786C226809D496ULL}, {0x14F8B588E368F084ULL, 0x61F9F01B866E43ABULL},
            {0x10C6F7A0B5ED8D36ULL, 0xB4C7F34938583622ULL}, {0x1AD7F29ABCAF4857ULL, 0x87A6520EC08D236AULL},
            {0x15798EE2308C39DFULL, 0x9FB841A566D74F88ULL}, {0x112E0BE826D694B2ULL, 0xE62D01511F12A607ULL},
            {0x1B7CDFD9D7BDBAB7ULL, 0xD6AE6881CB5109A4ULL}, {0x15FD7FE17964955FULL, 0xDEF1ED34A2A73AEAULL},
            {0x119799812DEA1119ULL, 0x7F27F0F6E885C8BBULL}, {0x1C25C268497681C2ULL, 0x650CB4BE40D60DF8ULL},
            {0x16849B86A12B9B01ULL, 0xEA70909833DE7193ULL}, {0x1203AF9EE756159BULL, 0x21F3A6E0297EC143ULL},
            {0x1CD2B297D889BC2BULL, 0x6985D7CD0F313537ULL}, {0x170EF54646D49689ULL, 0x2137DFD73F5A90F9ULL},
            {0x12725DD1D243ABA0ULL, 0xE75FE645CC4873FAULL}, {0x1D83C94FB6D2AC34ULL, 0xA5663D3C7A0D865DULL},
            {0x179CA10C9242235DULL, 0x511E976394D79EB1ULL}, {0x12E3B40A0E9B4F7DULL, 0xDA7EDF82DD794BC1ULL},
            {0x1E392010175EE596ULL, 0x2A6498D1625BAC68ULL}, {0x182DB34012B25144ULL, 0xEEB6E0A781E2F053ULL},
            {0x1357C299A88EA76AULL, 0x58924D52CE4F26A9ULL}, {0x1EF2D0F5DA7DD8AAULL, 0x27507BB7B07EA441ULL},
            {0x18C240C4AECB13BBULL, 0x52A6C95FC0655034ULL}, {0x13CE9A36F23C0FC9ULL, 0x0EEBD44C99EAA690ULL},
            {0x1FB0F6BE50601941ULL, 0xB17953ADC3110A80ULL}, {0x195A5EFEA6B34767ULL, 0xC12DDC8B02740867ULL},
            {0x14484BFEEBC29F86ULL, 0x3424B06F3529A052ULL}, {0x1039D66589687F9EULL, 0x901D59F290EE19DBULL},
            {0x19F623D5A8A73297ULL, 0x4CFBC31DB4B0295FULL}, {0x14C4E977BA1F5BACULL, 0x3D9635B15D59BAB2ULL},
            {0x109D8792FB4C4956ULL, 0x97AB5E277DE16228ULL}, {0x1A95A5B7F87A0EF0ULL, 0xF2ABC9D8C9689D0DULL},
            {0x154484932D2E725AULL, 0x5BBCA17A3ABA173EULL}, {0x11039D428A8B8EAEULL, 0xAFCA1AC82EFB45CBULL},
            {0x1B38FB9DAA78E44AULL, 0xB2DCF7A6B1920945ULL}, {0x15C72FB1552D836EULL, 0xF57D92EBC141A104ULL},
            {0x116C262777579C58ULL, 0xC46475896767B403ULL}, {0x1BE03D0BF225C6F4ULL, 0x6D6D88DBD8A5ECD2ULL},
            {0x164CFDA3281E38C3ULL, 0x8ABE071646EB23DBULL}, {0x11D7314F534B609CULL, 0x6EFE6C11D255B649ULL},
            {0x1C8B821885456760ULL, 0xB197134FB6EF8A0EULL}, {0x16D601AD376AB91AULL, 0x27AC0F72F8BFA1A5ULL},
            {0x1244CE242C5560E1ULL, 0xB95672C260994E1EULL}, {0x1D3AE36D13BBCE35ULL, 0xF5571E03CDC21695ULL},
            {0x17624F8A762FD82BULL, 0x2AAC18030B01ABABULL}, {0x12B50C6EC4F31355ULL, 0xBBBCE0026F348956ULL},
            {0x1DEE7A4AD4B81EEFULL, 0x92C7CCD0B1EDA889ULL}, {0x17F1FB6F10934BF2ULL, 0xDBD30A408E57BA07ULL},
            {0x1327FC58DA0F6FF5ULL, 0x7CA8D50071DFC806ULL}, {0x1EA6608E29B24CBBULL, 0xFAA7BB33E9660CD6ULL},
            {0x18851A0B548EA3C9ULL, 0x9552FC298784D711ULL}, {0x139DAE6F76D88307ULL, 0xAAA8C9BAD2D0AC0EULL},
            {0x1F62B0B257C0D1A5ULL, 0xDDDADC5E1E1AACE3ULL}, {0x191BC08EAC9A4151ULL, 0x7E48B04B4B488A4FULL},
            {0x141633A556E1CDDAULL, 0xCB6D59D5D5D3A1D9ULL}, {0x1011C2EAABE7D7E2ULL, 0x3C577B1177DC817BULL},
            {0x19B604AAACA62636ULL, 0xC6F25E825960CF2AULL}, {0x14919D5556EB51C5ULL, 0x6BF518684780A5BBULL},
            {0x10747DDDDF22A7D1ULL, 0x232A79ED06008496ULL}, {0x1A53FC9631D10C81ULL, 0xD1DD8FE1A3340756ULL},
            {0x150FFD44F4A73D34ULL, 0xA7E4731AE8F66C45ULL}, {0x10D9976A5D52975DULL, 0x531D28E253F8569EULL},
            {0x1AF5BF109550F22EULL, 0xEB61DB03B98D5762ULL}, {0x159165A6DDDA5B58ULL, 0xBC4E48CFC7A445E8ULL},
            {0x11411E1F17E1E2ADULL, 0x6371D3D96C836B20ULL}, {0x1B9B6364F3030448ULL, 0x9F1C8628AD9F11CDULL},
            {0x1615E91D8F359D06ULL, 0xE5B06B53BE18DB0BULL}, {0x11AB20E472914A6BULL, 0xEAF3890FCB4715A2ULL},
            {0x1C45016D841BAA46ULL, 0x44B8DB4C7871BC37ULL}, {0x169D9ABE03495505ULL, 0x03C715D6C6C1635FULL},
            {0x1217AEFE69077737ULL, 0x3638DE456BCDE919ULL}, {0x1CF2B1970E725858ULL, 0x56C163A2461641C1ULL},
            {0x17288E1271F51379ULL, 0xDF011C81D1AB67CEULL}, {0x1286D80EC190DC61ULL, 0x7F3416CE4155ECA5ULL},
            {0x1DA48CE468E7C702ULL, 0x6520247D3556476EULL}, {0x17B6D71D20B96C01ULL, 0xEA801D30F7783925ULL},
            {0x12F8AC174D612334ULL, 0xBB99B0F3F92CFA84ULL}, {0x1E5AACF215683854ULL, 0x5F5C4E532847F739ULL},
            {0x18488A5B44536043ULL, 0x7F7D0B75B9D32C2EULL}, {0x136D3B7C36A919CFULL, 0x9930D5F7C7DC2358ULL},
            {0x1F152BF9F10E8FB2ULL, 0x8EB4898C72F9D226ULL}, {0x18DDBCC7F40BA628ULL, 0x722A07A38F2E41B8ULL},
            {0x13E497065CD61E86ULL, 0xC1BB394FA5BE9AFAULL}, {0x1FD424D6FAF030D7ULL, 0x9C5EC2190930F7F6ULL},
            {0x197683DF2F268D79ULL, 0x49E56814075A5FF8ULL}, {0x145ECFE5BF520AC7ULL, 0x6E51201005E1E660ULL},
            {0x104BD984990E6F05ULL, 0xF1DA800CD181851AULL}, {0x1A12F5A0F4E3E4D6ULL, 0x4FC400148268D4F5ULL},
            {0x14DBF7B3F71CB711ULL, 0xD96999AA01ED772BULL}, {0x10AFF95CC5B09274ULL, 0xADEE1488018AC5BCULL},
            {0x1AB328946F80EA54ULL, 0x497CEDA668DE092CULL}, {0x155C2076BF9A5510ULL, 0x3ACA57B853E4D424ULL},
            {0x1116805EFFAEAA73ULL, 0x623B7960431D7683ULL}, {0x1B5733CB32B110B8ULL, 0x9D2BF566D1C8BD9EULL},
            {0x15DF5CA28EF40D60ULL, 0x7DBCC452416D647FULL}, {0x117F7D4ED8C33DE6ULL, 0xCAFD69DB678AB6CCULL},
            {0x1BFF2EE48E052FD7ULL, 0xAB2F0FC572778ADFULL}, {0x1665BF1D3E6A8CACULL, 0x88F273045B92D580ULL},
            {0x11EAFF4A98553D56ULL, 0xD3F528D049424466ULL}, {0x1CAB3210F3BB9557ULL, 0xB988414D4203A0A3ULL},
            {0x16EF5B40C2FC7779ULL, 0x6139CDD76802E6E9ULL}, {0x125915CD68C9F92DULL, 0xE761717920025254ULL},
            {0x1D5B561574765B7CULL, 0xA568B58E999D5086ULL}, {0x177C44DDF6C515FDULL, 0x5120913EE14AA6D2ULL},
            {0x12C9D0B1923744CAULL, 0xA74D40FF1AA21F0EULL}, {0x1E0FB44F50586E11ULL, 0x0BAECE64F769CB4AULL},
            {0x180C903F7379F1A7ULL, 0x3C8BD850C5EE3C3BULL}, {0x133D4032C2C7F485ULL, 0xCA0979DA37F1C9C9ULL},
            {0x1EC866B79E0CBA6FULL, 0xA9A8C2F6BFE942DBULL}, {0x18A0522C7E709526ULL, 0x2153CF2BCCBA9BE3ULL},
            {0x13B374F06526DDB8ULL, 0x1AA9728970954982ULL}, {0x1F8587E7083E2F8CULL, 0xF775840F1A88759DULL},
            {0x19379FEC0698260AULL, 0x5F9136727BA05E17ULL}, {0x142C7FF0054684D5ULL, 0x1940F85B9619E4DFULL},
            {0x1023998CD1053710ULL, 0xE100C6AFAB47EA4CULL}, {0x19D28F47B4D524E7ULL, 0xCE67A44C453FDD47ULL},
            {0x14A8729FC3DDB71FULL, 0xD852E9D69DCCB106ULL}, {0x1086C219697E2C19ULL, 0x79DBEE454B0A2738ULL},
            {0x1A71368F0F30468FULL, 0x295FE3A211A9D859ULL}, {0x15275ED8D8F36BA5ULL, 0xBAB31C81A7BB137AULL},
            {0x10EC4BE0AD8F8951ULL, 0x6228E39AEC95A92FULL}, {0x1B13AC9AAF4C0EE8ULL, 0x9D0E38F7E0EF7517ULL},
            {0x15A956E225D67253ULL, 0xB0D82D931A592A79ULL}, {0x11544581B7DEC1DCULL, 0x8D79BE0F4847552EULL},
            {0x1BBA08CF8C979C94ULL, 0x158F967EDA0BBB7CULL}, {0x162E6D72D6DFB076ULL, 0x77A611FF14D62F97ULL},
            {0x11BEBDF578B2F391ULL, 0xF951A7FF43DE8C79ULL}, {0x1C6463225AB7EC1CULL, 0xC21C3FFED2FDAD8EULL},
            {0x16B6B5B5155FF017ULL, 0x01B0333242648AD8ULL}, {0x122BC490DDE659ACULL, 0x0159C28E9B83A246ULL},
            {0x1D12D41AFCA3C2ACULL, 0xCEF604175F3903A3ULL}, {0x17424348CA1C9BBDULL, 0x725E69AC4C2D9C83ULL},
            {0x129B69070816E2FDULL, 0xF5185489D68AE39CULL}, {0x1DC574D80CF16B2FULL, 0xEE8D540FBDAB05C6ULL},
            {0x17D12A4670C1228CULL, 0xBED77672FE226B05ULL}, {0x130DBB6B8D674ED6ULL, 0xFF12C528CB4EBC04ULL},
            {0x1E7C5F127BD87E24ULL, 0xCB513B74787DF9A0ULL}, {0x18637F41FCAD31B7ULL, 0x090DC929F9FE614DULL},
            {0x1382CC34CA2427C5ULL, 0xA0D7D42194CB810AULL}, {0x1F37AD21436D0C6FULL, 0x67BFB9CF5478CE77ULL},
            {0x18F9574DCF8A7059ULL, 0x1FCC94A5DD2D71F9ULL}, {0x13FAAC3E3FA1F37AULL, 0x7FD6DD517DBDF4C7ULL},
            {0x1FF779FD329CB8C3ULL, 0xFFBE2EE8C92FEE0BULL}, {0x1992C7FDC216FA36ULL, 0x6631BF20A0F324D6ULL},
            {0x14756CCB01ABFB5EULL, 0xB827CC1A1A5C1D78ULL}, {0x105DF0A267BCC918ULL, 0x935309AE7B7CE460ULL},
            {0x1A2FE76A3F9474F4ULL, 0x1EEB42B0C594A099ULL}, {0x14F31F8832DD2A5CULL, 0xE58902270476E6E1ULL},
            {0x10C27FA028B0EEB0ULL, 0xB7A0CE859D2BEBE7ULL}, {0x1AD0CC33744E4AB4ULL, 0x59014A6F61DFDFD8ULL},
            {0x1573D68F903EA229ULL, 0xE0CDD525E7E64CADULL}, {0x11297872D9CBB4EEULL, 0x4D7177518651D6F1ULL},
            {0x1B758D848FAC54B0ULL, 0x7BE8BEE8D6E957E8ULL}, {0x15F7A46A0C89DD59ULL, 0xFCBA3253DF211320ULL},
            {0x1192E9EE706E4AAEULL, 0x63C8284318E74280ULL}, {0x1C1E43171A4A1117ULL, 0x060D0D3827D86A66ULL},
            {0x167E9C127B6E7412ULL, 0x6B3DA42CECAD21EBULL}, {0x11FEE341FC585CDBULL, 0x88FE1CF0BD574E56ULL},
            {0x1CCB0536608D615FULL, 0x419694B462254A23ULL}, {0x1708D0F84D3DE77FULL, 0x67ABAA29E81DD4E9ULL},
            {0x126D73F9D764B932ULL, 0xB95621BB2017DD87ULL}, {0x1D7BECC2F23AC1EAULL, 0xC223692B668C95A5ULL},
            {0x179657025B6234BBULL, 0xCE82BA891ED6DE1DULL}, {0x12DEAC01E2B4F6FCULL, 0xA53562074BDF1818ULL},
            {0x1E3113363787F194ULL, 0x3B889CD87964F359ULL}, {0x18274291C6065ADCULL, 0xFC6D4A46C783F5E1ULL},
            {0x13529BA7D19EAF17ULL, 0x30576E9F06032B1AULL}, {0x1EEA92A61C311825ULL, 0x1A257DCB3CD1DE90ULL},
            {0x18BBA884E35A79B7ULL, 0x481DFE3C30A7E540ULL}, {0x13C9539D82AEC7C5ULL, 0xD34B31C9C0865100ULL},
            {0x1FA885C8D117A609ULL, 0x5211E942CDA3B4CDULL}, {0x19539E3A40DFB807ULL, 0x74DB21023E1C90A4ULL},
            {0x1442E4FB67196005ULL, 0xF715B401CB4A0D50ULL}, {0x103583FC527AB337ULL, 0xF8DE299B09080AA7ULL},
            {0x19EF3993B72AB859ULL, 0x8E304291A80CDDD7ULL}, {0x14BF6142F8EEF9E1ULL, 0x3E8D020E200A4B13ULL},
            {0x10991A9BFA58C7E7ULL, 0x653D9B3E80083C0FULL}, {0x1A8E90F9908E0CA5ULL, 0x6EC8F864000D2CE4ULL},
            {0x153EDA614071A3B7ULL, 0x8BD3F9E999A423EAULL}, {0x10FF151A99F482F9ULL, 0x3CA994BAE1501CBBULL},
            {0x1B31BB5DC320D18EULL, 0xC775BAC49BB3612BULL}, {0x15C162B168E70E0BULL, 0xD2C4956A16291A89ULL},
            {0x11678227871F3E6FULL, 0xDBD0778811BA7BA1ULL}, {0x1BD8D03F3E9863E6ULL, 0x2C80BF401C5D929BULL},
            {0x16470CFF6546B651ULL, 0xBD33CC3349E47549ULL}, {0x11D270CC51055EA7ULL, 0xCA8FD68F6E505DD4ULL},
            {0x1C83E7AD4E6EFDD9ULL, 0x4419574BE3B3C953ULL}, {0x16CFEC8AA52597E1ULL, 0x0347790982F63AA9ULL},
            {0x123FF06EEA847980ULL, 0xCF6C60D468C4FBBAULL}, {0x1D331A4B10D3F59AULL, 0xE57A34870E07F92AULL},
            {0x175C1508DA432AE2ULL, 0x512E906C0B399422ULL}, {0x12B010D3E1CF5581ULL, 0xDA8BA6BCD5C7A9B5ULL},
            {0x1DE6815302E5559CULL, 0x90DF712E22D90F87ULL}, {0x17EB9AA8CF1DDE16ULL, 0xDA4C5A8B4F140C6CULL},
            {0x1322E220A5B17E78ULL, 0xAEA37BA2A5A9A38AULL}, {0x1E9E369AA2B59727ULL, 0x7DD25F6AA2A905A9ULL},
            {0x187E92154EF7AC1FULL, 0x97DB7F888220D154ULL}, {0x139874DDD8C6234CULL, 0x797C6606CE80A777ULL},
            {0x1F5A549627A36BADULL, 0x8F2D700AE4010BF1ULL}, {0x191510781FB5EFBEULL, 0x0C2459A25000D65AULL},
            {0x1410D9F9B2F7F2FEULL, 0x701D1481D99A4515ULL}, {0x100D7B2E28C65BFEULL, 0xC017439B147B6A77ULL},
            {0x19AF2B7D0E0A2CCAULL, 0xCCF205C4ED9243F2ULL}, {0x148C22CA71A1BD6FULL, 0x0A5B37D0BE0E9CC2ULL},
            {0x10701BD527B4978CULL, 0x0848F973CB3EE3CEULL}, {0x1A4CF9550C5425ACULL, 0xDA0E5BEC78649FB0ULL},
            {0x150A6110D6A9B7BDULL, 0x7B3EAFF060507FC0ULL}, {0x10D51A73DEEE2C97ULL, 0x95CBBFF380406633ULL},
            {0x1AEE90B964B04758ULL, 0xEFAC665266CD7052ULL}, {0x158BA6FAB6F36C47ULL, 0x2623850EB8A459DBULL},
            {0x113C85955F29236CULL, 0x1E82D0D893B6AE49ULL}, {0x1B9408EEFEA838ACULL, 0xFD9E1AF41F8AB075ULL},
            {0x16100725988693BDULL, 0x97B1AF29B2D559F7ULL}, {0x11A66C1E139EDC97ULL, 0xAC8E25BAF5777B2CULL},
            {0x1C3D79C9B8FE2DBFULL, 0x7A7D092B2258C513ULL}, {0x169794A160CB57CCULL, 0x61FDA0EF4EAD6A76ULL},
            {0x1212DD4DE7091309ULL, 0xE7FE1A590BBDEEC5ULL}, {0x1CEAFBAFD80E84DCULL, 0xA6635D5B45FCB13AULL},
            {0x172262F3133ED0B0ULL, 0x851C4AAF6B308DC8ULL}, {0x1281E8C275CBDA26ULL, 0xD0E36EF2BC26D7D4ULL},
            {0x1D9CA79D894629D7ULL, 0xB49F17EAC6A48C86ULL}, {0x17B08617A104EE46ULL, 0x2A18DFEF0550706BULL},
            {0x12F39E794D9D8B6BULL, 0x54E0B3259DD9F389ULL}, {0x1E5297287C2F4578ULL, 0x87CDEB6F62F65274ULL},
            {0x18421286C9BF6AC6ULL, 0xD30B22BF825EA85DULL}, {0x13680ED23AFF889FULL, 0x0F3C1BCC684BB9E4ULL},
            {0x1F0CE4839198DA98ULL, 0x18602C7A4079296DULL}, {0x18D71D360E13E213ULL, 0x46B356C833942124ULL},
            {0x13DF4A91A4DCB4DCULL, 0x388F78A029434DB6ULL}, {0x1FCBAA82A1612160ULL, 0x5A7F2766A86BAF8AULL},
            {0x196FBB9BB44DB44DULL, 0x153285EBB9EFBFA2ULL}, {0x145962E2F6A4903DULL, 0xAA8ED189618C994EULL},
            {0x1047824F2BB6D9CAULL, 0xEED8A7A11AD6E10CULL}, {0x1A0C03B1DF8AF611ULL, 0x7E27729B5E249B45ULL},
            {0x14D6695B193BF80DULL, 0xFE85F549181D4904ULL}, {0x10AB877C142FF9A4ULL, 0xCB9E5DD4134AA0D0ULL},
            {0x1AAC0BF9B9E65C3AULL, 0xDF63C9535211014DULL}, {0x15566FFAFB1EB02FULL, 0x191CA10F74DA6771ULL},
            {0x1111F32F2F4BC025ULL, 0xADB080D92A4852C1ULL}, {0x1B4FEB7EB212CD09ULL, 0x15E7348EAA0D5134ULL},
            {0x15D98932280F0A6DULL, 0xAB1F5D3EEE710DC4ULL}, {0x117AD428200C0857ULL, 0xBC1917658B8DA49DULL},
            {0x1BF7B9D9CCE00D59ULL, 0x2CF4F23C127C3A94ULL}, {0x165FC7E170B33DE0ULL, 0xF0C3F4FCDB969543ULL},
            {0x11E6398126F5CB1AULL, 0x5A365D9716121103ULL}, {0x1CA38F350B22DE90ULL, 0x9056FC24F01CE804ULL},
            {0x16E93F5DA2824BA6ULL, 0xD9DF301D8CE3ECD0ULL}, {0x125432B14ECEA2EBULL, 0xE17F59B13D8323DAULL},
            {0x1D53844EE47DD179ULL, 0x68CBC2B52F38395CULL}, {0x177603725064A794ULL, 0x53D6355DBF602DE3ULL},
            {0x12C4CF8EA6B6EC76ULL, 0xA9782AB165E68B1CULL}, {0x1E07B27DD78B13F1ULL, 0x0F26AAB56FD744FAULL},
            {0x18062864AC6F4327ULL, 0x3F52222ABFDF6A62ULL}, {0x1338205089F29C1FULL, 0x65DB4E88997F884EULL},
            {0x1EC033B40FEA9365ULL, 0x6FC54A7428CC0D4AULL}, {0x1899C2F673220F84ULL, 0x596AA1F68709A43BULL},
            {0x13AE3591F5B4D936ULL, 0xADEEE7F86C07B696ULL}, {0x1F7D228322BAF524ULL, 0x497E3FF3E00C5756ULL},
            {0x1930E868E89590E9ULL, 0xD464FFF64CD6AC45ULL}, {0x14272053ED4473EEULL, 0x4383FFF83D7889D1ULL},
            {0x101F4D0FF1038FF1ULL, 0xCF9CCCC69793A174ULL}, {0x19CBAE7FE805B31CULL, 0x7F6147A425B90252ULL},
            {0x14A2F1FFECD15C16ULL, 0xCC4DD2E9B7C7350FULL}, {0x10825B3323DAB012ULL, 0x3D0B0F215FD290D9ULL},
            {0x1A6A2B85062AB350ULL, 0x61AB4B689950E7C1ULL}, {0x1521BC6A6B555C40ULL, 0x4E22A2BA1440B967ULL},
            {0x10E7C9EEBC4449CDULL, 0x0B4EE894DD009453ULL}, {0x1B0C764AC6D3A948ULL, 0x1217DA87C800ED51ULL},
            {0x15A391D56BDC876CULL, 0xDB46486CA000BDDAULL}, {0x114FA7DDEFE39F8AULL, 0x490506BD4CCD64AFULL},
            {0x1BB2A62FE638FF43ULL, 0xA8080AC87AE23AB1ULL}, {0x162884F31E93FF69ULL, 0x5339A239FBE82EF4ULL},
            {0x11BA03F5B20FFF87ULL, 0x75C7B4FB2FECF25DULL}, {0x1C5CD322B67FFF3FULL, 0x22D92191E647EA2EULL},
            {0x16B0A8E891FFFF65ULL, 0xB57A8141850654F2ULL}, {0x1226ED86DB3332B7ULL, 0xC4620101373843F5ULL},
            {0x1D0B15A491EB8459ULL, 0x3A366801F1F39FEEULL}, {0x173C115074BC69E0ULL, 0xFB5EB99B27F6198BULL},
            {0x129674405D6387E7ULL, 0x2F7EFAE2865E7AD6ULL}, {0x1DBD86CD6238D971ULL, 0xE597F7D0D6FD9156ULL},
            {0x17CAD23DE82D7AC1ULL, 0x8479930D78CADAABULL}, {0x1308A831868AC89AULL, 0xD06142712D6F1556ULL},
            {0x1E74404F3DAADA91ULL, 0x4D686A4EAF182222ULL}, {0x185D003F6488AEDAULL, 0xA453883EF279B4E8ULL},
            {0x137D99CC506D58AEULL, 0xE9DC6CFF28615D87ULL}, {0x1F2F5C7A1A488DE4ULL, 0xA960AE650D6895A4ULL},
            {0x18F2B061AEA07183ULL, 0xBAB3BEB73DED4483ULL}, {0x13F559E7BEE6C136ULL, 0x2EF6322C318A9D36ULL},
            {0x1FEEF63F97D79B89ULL, 0xE4BD1D13827761F0ULL}, {0x198BF832DFDFAFA1ULL, 0x83CA7DA9352C4E5AULL},
            {0x146FF9C24CB2F2E7ULL, 0x9CA1FE20F756A515ULL}, {0x1059949B708F28B9ULL, 0x4A1B31B3F9121DAAULL},
            {0x1A28EDC580E50DF5ULL, 0x435EB5ECC1B695DDULL}, {0x14ED8B04671DA4C4ULL, 0x35E55E57015EDE4AULL},
            {0x10BE08D0527E1D69ULL, 0xC4B77EAC0118B1D5ULL}, {0x1AC9A7B3B7302F0FULL, 0xA12597799B5AB622ULL},
            {0x156E1FC2F8F358D9ULL, 0x4DB7AC6149155E81ULL}, {0x1124E63593F5E0ADULL, 0xD7C6238107444B9BULL},
            {0x1B6E3D2286563449ULL, 0x593D059B3ED3AC2BULL}, {0x15F1CA820511C36DULL, 0xE0FD9E15CBDC89BCULL},
            {0x118E3B9B37416924ULL, 0xB3FE18116FE3A163ULL}, {0x1C16C5C525357507ULL, 0x866359B57FD29BD1ULL},
            {0x16789E3750F790D2ULL, 0xD1E91491330EE30EULL}, {0x11FA182C40C60D75ULL, 0x74BA76DA8F3F1C0BULL},
            {0x1CC359E067A348BBULL, 0xEDF72490E531C678ULL}, {0x1702AE4D1FB5D3C9ULL, 0x8B2C1D40B75B052DULL},
            {0x12688B70E62B0FD4ULL, 0x6F567DCD5F7C0424ULL}, {0x1D74124E3D11B2EDULL, 0x7EF0C94898C66D06ULL},
            {0x17900EA4FDA7C257ULL, 0x98C0A106E09EBD9FULL}, {0x12D9A550CAEC9B79ULL, 0x470080D24D4BCAE6ULL},
            {0x1E29088144ADC58EULL, 0xD800CE1D487944A2ULL}, {0x1820D39A9D57D13FULL, 0x1333D8176D2DD082ULL},
            {0x134D76154AACA765ULL, 0xA8F646792424A6CEULL}, {0x1EE25688777AA56FULL, 0x74BD3D8EA03AA47DULL},
            {0x18B51206C5FBB78CULL, 0x5D64313EE6955064ULL}, {0x13C40E6BD1962C70ULL, 0x4AB68DCBEBAAA6B7ULL},
            {0x1FA01712E8F0471AULL, 0x1124161312AAA457ULL}, {0x194CDF4253F36C14ULL, 0xDA8344DC0EEEE9DFULL},
            {0x143D7F6843292343ULL, 0xE2029D7CD8BF2180ULL}, {0x103132B9CF541C36ULL, 0x4E687DFD7A328133ULL},
            {0x19E851294BB9C6BDULL, 0x4A40C9959050CEB8ULL}, {0x14B9DA876FC7D231ULL, 0x0833D477A6A70BC6ULL},
            {0x1094AED2BFD30E8DULL, 0xA02976C61EEC096BULL}, {0x1A877E1DFFB81749ULL, 0x004257A364ACDBDFULL},
            {0x153931B1996012A0ULL, 0xCD01DFB5EA23E319ULL}, {0x10FA8E27ADE6754DULL, 0x70CE4C91881CB5AEULL},
            {0x1B2A7D0C4970BBAFULL, 0x1AE3ADB5A69455E2ULL}, {0x15BB973D078D62F2ULL, 0x7BE957C4854377E8ULL},
            {0x1162DF64060AB58EULL, 0xC987796A0435F987ULL}, {0x1BD1656CD67788E4ULL, 0x75A58F1006BCC271ULL},
            {0x16411DF0AB92D3E9ULL, 0xF7B7A5A66BCA3527ULL}, {0x11CDB18D560F0FEEULL, 0x5FC61E1EBCA1C41FULL},
            {0x1C7C4F4889B1B316ULL, 0xFFA363646102D365ULL}, {0x16C9D906D48E28DFULL, 0x32E91C504D9BDC51ULL},
            {0x123B140576D820B2ULL, 0x8F20E37371497D0EULL}, {0x1D2B533BF159CDEAULL, 0x7E9B0585820F2E7CULL},
            {0x1755DC2FF447D7EEULL, 0xCBAF379E01A5BECAULL}, {0x12AB168CC36CACBFULL, 0x0958F94B348498A1ULL},
    };

    int pow5bits(const int e) noexcept { return ((e * 1217359) >> 19) + 1; }

    int log10_pow2(const int e) noexcept { return (e * 78913) >> 18; }

    int log10_pow5(const int e) noexcept { return (e * 732923) >> 20; }

    void mul_table(const uint64_t m, const ryu_table_entry& entry, uint64_t& hi, uint64_t& mid, uint64_t& lo) noexcept {
        uint64_t low_hi = 0;
        lo = _NEFORCE __umul128(m, entry.lo, &low_hi);
        uint64_t high_hi = 0;
        const uint64_t high_lo = _NEFORCE __umul128(m, entry.hi, &high_hi);
        mid = low_hi + high_lo;
        hi = high_hi + (mid < low_hi ? 1 : 0);
    }

    void add_table(uint64_t& hi, uint64_t& mid, uint64_t& lo, const ryu_table_entry& entry,
                   const bool subtract) noexcept {
        const uint64_t old_lo = lo;
        if (subtract) {
            lo -= entry.lo;
            const uint64_t borrow = old_lo < entry.lo ? 1 : 0;
            const uint64_t old_mid = mid;
            mid -= entry.hi + borrow;
            hi -= (mid > old_mid) ? 1 : 0;
        } else {
            lo += entry.lo;
            const uint64_t carry = lo < old_lo ? 1 : 0;
            const uint64_t old_mid = mid;
            mid += entry.hi + carry;
            hi += (mid < old_mid) ? 1 : 0;
        }
    }

    uint64_t shift_right_192(const uint64_t hi, const uint64_t mid, const int shift) noexcept {
        const int bits = shift - 64;
        if (bits <= 0) {
            return mid;
        }
        if (bits >= 64) {
            return 0;
        }
        return (mid >> bits) | (hi << (64 - bits));
    }

    void float_to_shortest_digits(const double value, shortest_digits& out) {
        uint64_t bits = 0;
        memory_copy(&bits, &value, sizeof(bits));
        const uint64_t ieee_mantissa = bits & 0x000FFFFFFFFFFFFFULL;
        const uint64_t ieee_exponent = (bits >> 52) & 0x7FFULL;

        const uint64_t m2 = (ieee_exponent == 0) ? ieee_mantissa : (0x0010000000000000ULL | ieee_mantissa);
        const int e2 = (ieee_exponent == 0) ? -1074 : (static_cast<int>(ieee_exponent) - 1075);
        const bool half_lower_bound = (ieee_mantissa == 0 && ieee_exponent > 1);

        ryu_table_entry entry{};
        int shift = 0;
        int e10 = 0;
        if (e2 >= 0) {
            const int q = log10_pow2(e2) - (e2 > 3 ? 1 : 0);
            e10 = q;
            entry = k_pow5_inv_split[q];
            shift = pow5bits(q) - 1 + 125 - e2 + q + 1;
        } else {
            const int q = log10_pow5(-e2) - (-e2 > 1 ? 1 : 0);
            e10 = q + e2;
            const int index = -e2 - q;
            entry = k_pow5_split[index];
            shift = q - (pow5bits(index) - 125) + 1;
        }

        uint64_t x_hi = 0;
        uint64_t x_mid = 0;
        uint64_t x_lo = 0;
        mul_table(2 * m2, entry, x_hi, x_mid, x_lo);

        const int low_bits = shift - 64;
        const uint64_t low_mask = low_bits >= 64 ? ~0ULL : ((1ULL << low_bits) - 1ULL);

        const uint64_t vr = shift_right_192(x_hi, x_mid, shift);
        const bool vr_exact = (x_lo == 0) && ((x_mid & low_mask) == 0);

        uint64_t up_hi = x_hi;
        uint64_t up_mid = x_mid;
        uint64_t up_lo = x_lo;
        add_table(up_hi, up_mid, up_lo, entry, false);
        const uint64_t vp = shift_right_192(up_hi, up_mid, shift);

        uint64_t vm = 0;
        if (half_lower_bound) {
            uint64_t down_hi = (x_hi << 1) | (x_mid >> 63);
            uint64_t down_mid = (x_mid << 1) | (x_lo >> 63);
            uint64_t down_lo = x_lo << 1;
            add_table(down_hi, down_mid, down_lo, entry, true);
            vm = shift_right_192(down_hi, down_mid, shift + 1);
        } else {
            uint64_t down_hi = x_hi;
            uint64_t down_mid = x_mid;
            uint64_t down_lo = x_lo;
            add_table(down_hi, down_mid, down_lo, entry, true);
            vm = shift_right_192(down_hi, down_mid, shift);
        }

        uint64_t scaled = vr;
        uint64_t upper = vp;
        uint64_t lower = vm;
        size_t removed = 0;
        if (vr != 0) {
            uint64_t last_removed = 0;
            while (upper / 10 > lower / 10) {
                last_removed = scaled % 10;
                scaled /= 10;
                upper /= 10;
                lower /= 10;
                ++removed;
            }
            const bool round_up =
                    (scaled == lower) || last_removed > 5 || (last_removed == 5 && (!vr_exact || (scaled % 2) != 0));
            if (round_up) {
                ++scaled;
            }
        }
        while (scaled != 0 && scaled % 10 == 0) {
            scaled /= 10;
            ++removed;
        }
        out.len = shortest_write_u64(scaled, out.digits);
        out.exponent = e10 + static_cast<int>(removed);

        if (ieee_exponent >= 1076) {
            // |v| >= 2^53：ulp >= 1
            const int exponent_of_first = static_cast<int>(out.len) - 1 + out.exponent;
            size_t exponent_digits = 2;
            int magnitude = exponent_of_first >= 0 ? exponent_of_first : -exponent_of_first;
            while (magnitude >= 100) {
                magnitude /= 10;
                ++exponent_digits;
            }
            const size_t scientific_length = out.len + 3 + exponent_digits;
            const size_t fixed_length = static_cast<size_t>(exponent_of_first) + 1;
            if (fixed_length <= scientific_length && fixed_length + 1 <= sizeof(out.digits)) {
                char integer_digits[32];
                size_t integer_len = 0;
                int kind = 0;
                float_scaled_floor(m2, e2, 0, integer_digits, integer_len, kind);
                int integer_exponent = 0;
                while (integer_len > 1 && integer_digits[integer_len - 1] == '0') {
                    --integer_len;
                    ++integer_exponent;
                }
                if (integer_len + 1 <= sizeof(out.digits)) {
                    out.len = integer_len;
                    for (size_t i = 0; i < integer_len; ++i) {
                        out.digits[i] = integer_digits[i];
                    }
                    out.exponent = integer_exponent;
                }
            }
        }
    }

    bool float_to_significant_digits(const double magnitude, const int significant, shortest_digits& out) {
        uint64_t mantissa = 0;
        int exponent = 0;
        float_decompose(magnitude, mantissa, exponent);

        int mantissa_bits = 0;
        for (uint64_t value = mantissa; value != 0; value >>= 1) {
            ++mantissa_bits;
        }
        int scale = (significant - 1) -
                    static_cast<int>(static_cast<double>(exponent + mantissa_bits - 1) * 0.30102999566398120);
        char digits[400];
        size_t len = 0;
        int tail_kind = 0;
        if (scale < 0) {
            float_scaled_floor(mantissa, exponent, 0, digits, len, tail_kind);
            if (len == 0 || len > sizeof(digits) - 1) {
                return false;
            }
            const size_t total = len;
            if (total > static_cast<size_t>(significant)) {
                const char lead = digits[significant];
                bool rest = false;
                for (size_t k = static_cast<size_t>(significant) + 1; k < total && !rest; ++k) {
                    rest = digits[k] != '0';
                }
                len = static_cast<size_t>(significant);
                const bool round_up = lead > '5' || (lead == '5' && (rest || ((digits[len - 1] - '0') % 2) != 0));
                if (round_up) {
                    size_t position = len;
                    while (position > 0 && digits[position - 1] == '9') {
                        digits[position - 1] = '0';
                        --position;
                    }
                    if (position == 0) {
                        for (size_t k = len; k > 0; --k) {
                            digits[k] = digits[k - 1];
                        }
                        digits[0] = '1';
                        ++len;
                    } else {
                        ++digits[position - 1];
                    }
                }
                while (len > 1 && digits[len - 1] == '0') {
                    --len;
                }
                out.len = len;
                for (size_t k = 0; k < len; ++k) {
                    out.digits[k] = digits[k];
                }
                out.exponent = static_cast<int>(total) - static_cast<int>(len);
                return true;
            }
        }
        float_scaled_floor(mantissa, exponent, scale, digits, len, tail_kind);
        for (int attempt = 0; attempt < 4 && len != static_cast<size_t>(significant); ++attempt) {
            scale += significant - static_cast<int>(len);
            if (scale > 370 || scale < -370) {
                break;
            }
            float_scaled_floor(mantissa, exponent, scale, digits, len, tail_kind);
        }
        if (len == 0 || len > sizeof(out.digits) - 1) {
            return false;
        }

        const bool round_up = (tail_kind == 3) || (tail_kind == 2 && ((digits[len - 1] - '0') % 2) != 0);
        if (round_up) {
            size_t position = len;
            while (position > 0 && digits[position - 1] == '9') {
                digits[position - 1] = '0';
                --position;
            }
            if (position == 0) {
                for (size_t k = len; k > 0; --k) {
                    digits[k] = digits[k - 1];
                }
                digits[0] = '1';
                ++len;
                while (len > static_cast<size_t>(significant)) {
                    --len;
                    --scale;
                }
            } else {
                ++digits[position - 1];
            }
        }

        while (len > 1 && digits[len - 1] == '0') {
            --len;
            --scale;
        }
        out.len = len;
        for (size_t i = 0; i < len; ++i) {
            out.digits[i] = digits[i];
        }
        out.exponent = -scale;
        return true;
    }

    size_t format_shortest_buffer(const shortest_digits& value, const int significant_limit, const bool is_negative,
                                  bool force_scientific, const bool force_fixed, char* out) {
        size_t len = 0;
        const int exponent_of_first = static_cast<int>(value.len) - 1 + value.exponent;

        if (significant_limit < 0 && !force_scientific && !force_fixed) {
            force_scientific = scientific_text_length(value) < fixed_text_length(value);
        }

        const bool use_scientific = force_scientific ? true
                                    : force_fixed    ? false
                                    : (significant_limit >= 0)
                                            ? (exponent_of_first < -4 || exponent_of_first >= significant_limit)
                                            : false;

        if (is_negative) {
            out[len++] = '-';
        }
        if (use_scientific) {
            out[len++] = value.digits[0];
            if (value.len > 1) {
                out[len++] = '.';
                for (size_t i = 1; i < value.len; ++i) {
                    out[len++] = value.digits[i];
                }
            }
            out[len++] = 'e';
            int fixed_exponent = exponent_of_first;
            if (fixed_exponent >= 0) {
                out[len++] = '+';
            } else {
                out[len++] = '-';
                fixed_exponent = -fixed_exponent;
            }
            if (fixed_exponent < 10) {
                out[len++] = '0';
            }
            char buffer[8];
            const size_t length = shortest_write_u64(static_cast<uint64_t>(fixed_exponent), buffer);
            for (size_t i = 0; i < length; ++i) {
                out[len++] = buffer[i];
            }
            return len;
        }

        if (value.exponent >= 0) {
            for (size_t i = 0; i < value.len; ++i) {
                out[len++] = value.digits[i];
            }
            for (int i = 0; i < value.exponent; ++i) {
                out[len++] = '0';
            }
        } else if (static_cast<int>(value.len) + value.exponent > 0) {
            const auto split =
                    static_cast<size_t>(static_cast<int64_t>(value.len) + static_cast<int64_t>(value.exponent));
            for (size_t i = 0; i < split; ++i) {
                out[len++] = value.digits[i];
            }
            out[len++] = '.';
            for (size_t i = split; i < value.len; ++i) {
                out[len++] = value.digits[i];
            }
        } else {
            out[len++] = '0';
            out[len++] = '.';
            for (int i = 0; i < -(static_cast<int>(value.len) + value.exponent); ++i) {
                out[len++] = '0';
            }
            for (size_t i = 0; i < value.len; ++i) {
                out[len++] = value.digits[i];
            }
        }
        return len;
    }

    size_t fixed_text_length(const shortest_digits& value) noexcept {
        const int exponent_of_first = static_cast<int>(value.len) - 1 + value.exponent;
        if (value.exponent >= 0) {
            return value.len + static_cast<size_t>(value.exponent);
        }
        if (exponent_of_first >= 0) {
            return value.len + 1;
        }
        return static_cast<size_t>(1 - exponent_of_first) + value.len;
    }

    size_t scientific_text_length(const shortest_digits& value) noexcept {
        const int exponent_of_first = static_cast<int>(value.len) - 1 + value.exponent;
        size_t digits_of_exponent = 2;
        int magnitude = exponent_of_first >= 0 ? exponent_of_first : -exponent_of_first;
        while (magnitude >= 100) {
            magnitude /= 10;
            ++digits_of_exponent;
        }
        if (value.len == 1) {
            return 1 + 1 + 1 + digits_of_exponent; // d e ± NN
        }
        return value.len + 1 + 1 + 1 + digits_of_exponent; // d.ddd e ± NN
    }
} // namespace


string float_to_string_shortest(const double value) {
    if (is_nan(value)) {
        return "nan";
    }
    if (is_pos_infinity(value)) {
        return "inf";
    }
    if (is_neg_infinity(value)) {
        return "-inf";
    }
    const bool is_negative = (value < 0) || (value == 0 && signbit(value));
    const double magnitude = is_negative ? -value : value;
    if (magnitude == 0.0) {
        return is_negative ? "-0" : "0";
    }
    char text[400];
    size_t len = 0;
    if (is_negative) {
        text[len++] = '-';
    }
    shortest_digits digits;
    float_to_shortest_digits(magnitude, digits);
    len += format_shortest_buffer(digits, -1, false, false, false, text + len);
    return string(text, len);
}

string float_to_string_sig(const double value, const int precision) {
    if (is_nan(value)) {
        return "nan";
    }
    if (is_pos_infinity(value)) {
        return "inf";
    }
    if (is_neg_infinity(value)) {
        return "-inf";
    }
    const int significant = max(precision, 1);
    const bool is_negative = (value < 0) || (value == 0 && signbit(value));
    const double magnitude = is_negative ? -value : value;
    if (magnitude == 0.0) {
        return is_negative ? "-0" : "0";
    }
    char text[400];
    size_t len = 0;
    if (is_negative) {
        text[len++] = '-';
    }
    shortest_digits digits;
    if (!float_to_significant_digits(magnitude, significant, digits)) {
        text[len++] = '0';
        return string(text, len);
    }
    len += format_shortest_buffer(digits, significant, false, false, false, text + len);
    return string(text, len);
}

void append_float_shortest(string& out, const double value) {
    if (is_nan(value)) {
        out += "nan";
        return;
    }
    if (is_pos_infinity(value)) {
        out += "inf";
        return;
    }
    if (is_neg_infinity(value)) {
        out += "-inf";
        return;
    }
    const bool is_negative = (value < 0) || (value == 0 && signbit(value));
    const double magnitude = is_negative ? -value : value;
    if (magnitude == 0.0) {
        out += is_negative ? "-0" : "0";
        return;
    }
    char text[400];
    size_t len = 0;
    if (is_negative) {
        text[len++] = '-';
    }
    shortest_digits digits;
    float_to_shortest_digits(magnitude, digits);
    len += format_shortest_buffer(digits, -1, false, false, false, text + len);
    out.append(text, len);
}

void append_float_sig(string& out, const double value, const int precision) {
    if (is_nan(value)) {
        out += "nan";
        return;
    }
    if (is_pos_infinity(value)) {
        out += "inf";
        return;
    }
    if (is_neg_infinity(value)) {
        out += "-inf";
        return;
    }
    const int significant = max(precision, 1);
    const bool is_negative = (value < 0) || (value == 0 && signbit(value));
    const double magnitude = is_negative ? -value : value;
    if (magnitude == 0.0) {
        out += is_negative ? "-0" : "0";
        return;
    }
    char text[400];
    size_t len = 0;
    if (is_negative) {
        text[len++] = '-';
    }
    shortest_digits digits;
    if (!float_to_significant_digits(magnitude, significant, digits)) {
        text[len++] = '0';
        out.append(text, len);
        return;
    }
    len += format_shortest_buffer(digits, significant, false, false, false, text + len);
    out.append(text, len);
}

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

    if (digit_len <= static_cast<size_t>(precision)) {
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
    for (size_t i = 0; i < static_cast<size_t>(precision); ++i) {
        text[len++] = digits[split + i];
    }
    return string(text, len);
}

NEFORCE_END_INNER__
NEFORCE_END_NAMESPACE__
