#include <NeForce/core/encrypt/base64.hpp>
#include <NeForce/core/simd/types.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    constexpr auto base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    constexpr auto base64url_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

    constexpr int base64_char_to_index(const char c) {
        if (c >= 'A' && c <= 'Z') {
            return c - 'A';
        }
        if (c >= 'a' && c <= 'z') {
            return c - 'a' + 26;
        }
        if (c >= '0' && c <= '9') {
            return c - '0' + 52;
        }
        if (c == '+') {
            return 62;
        }
        if (c == '/') {
            return 63;
        }
        return -1;
    }

    constexpr int base64url_char_to_index(const char c) {
        if (c >= 'A' && c <= 'Z') {
            return c - 'A';
        }
        if (c >= 'a' && c <= 'z') {
            return c - 'a' + 26;
        }
        if (c >= '0' && c <= '9') {
            return c - '0' + 52;
        }
        if (c == '-' || c == '+') {
            return 62;
        }
        if (c == '_' || c == '/') {
            return 63;
        }
        return -1;
    }

#ifdef NEFORCE_SIMD_SSSE3
    NEFORCE_ALWAYS_INLINE_INLINE simd::vec128_t base64_shuffle_lookup(const simd::vec128_t idx, const char* alphabet) {
        // pshufb only uses the low 4 bits of each index byte, so split the
        // 6-bit indices into four 16-char ranges and select per range.
        const simd::vec128_t a0 = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(alphabet));
        const simd::vec128_t a1 = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(alphabet + 16));
        const simd::vec128_t a2 = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(alphabet + 32));
        const simd::vec128_t a3 = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(alphabet + 48));

        const simd::vec128_t m1 = ::_mm_cmpgt_epi8(idx, ::_mm_set1_epi8(15));
        const simd::vec128_t m2 = ::_mm_cmpgt_epi8(idx, ::_mm_set1_epi8(31));
        const simd::vec128_t m3 = ::_mm_cmpgt_epi8(idx, ::_mm_set1_epi8(47));

        const simd::vec128_t r0 = ::_mm_shuffle_epi8(a0, idx);
        const simd::vec128_t r1 = ::_mm_shuffle_epi8(a1, ::_mm_subs_epu8(idx, ::_mm_set1_epi8(16)));
        const simd::vec128_t r2 = ::_mm_shuffle_epi8(a2, ::_mm_subs_epu8(idx, ::_mm_set1_epi8(32)));
        const simd::vec128_t r3 = ::_mm_shuffle_epi8(a3, ::_mm_subs_epu8(idx, ::_mm_set1_epi8(48)));

        simd::vec128_t result = ::_mm_or_si128(::_mm_andnot_si128(m1, r0), ::_mm_and_si128(m1, r1));
        result = ::_mm_or_si128(::_mm_andnot_si128(m2, result), ::_mm_and_si128(m2, r2));
        return ::_mm_or_si128(::_mm_andnot_si128(m3, result), ::_mm_and_si128(m3, r3));
    }

    NEFORCE_ALWAYS_INLINE_INLINE void base64_encode_12bytes(const byte_t* src, char* dst, const char* alphabet) {
        simd::vec128_t v = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(src));

        const simd::vec128_t be_mask = ::_mm_set_epi8(-1, 9, 10, 11, -1, 6, 7, 8, -1, 3, 4, 5, -1, 0, 1, 2);
        v = ::_mm_shuffle_epi8(v, be_mask);

        const simd::vec128_t mask6 = ::_mm_set1_epi32(0x3F);
        const simd::vec128_t i0 = ::_mm_and_si128(::_mm_srli_epi32(v, 18), mask6);
        const simd::vec128_t i1 = ::_mm_slli_epi32(::_mm_and_si128(::_mm_srli_epi32(v, 12), mask6), 8);
        const simd::vec128_t i2 = ::_mm_slli_epi32(::_mm_and_si128(::_mm_srli_epi32(v, 6), mask6), 16);
        const simd::vec128_t i3 = ::_mm_slli_epi32(::_mm_and_si128(v, mask6), 24);

        const simd::vec128_t merged = ::_mm_or_si128(::_mm_or_si128(i0, i1), ::_mm_or_si128(i2, i3));

        const simd::vec128_t result = base64_shuffle_lookup(merged, alphabet);
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(dst), result);
    }

    NEFORCE_ALWAYS_INLINE_INLINE simd::vec128_t base64_map_to_index(simd::vec128_t v, char plus_char, char slash_char) {
        simd::vec128_t values = ::_mm_sub_epi8(v, ::_mm_set1_epi8('A'));

        const simd::vec128_t is_lower = ::_mm_cmpgt_epi8(v, ::_mm_set1_epi8('a' - 1));
        values = ::_mm_or_si128(::_mm_andnot_si128(is_lower, values),
                                ::_mm_and_si128(is_lower, ::_mm_sub_epi8(v, ::_mm_set1_epi8('a' - 26))));

        const simd::vec128_t is_digit_lo = ::_mm_cmpgt_epi8(v, ::_mm_set1_epi8('0' - 1));
        const simd::vec128_t is_digit_hi = ::_mm_cmpgt_epi8(::_mm_set1_epi8('9' + 1), v);
        const simd::vec128_t is_digit = ::_mm_and_si128(is_digit_lo, is_digit_hi);
        values = ::_mm_or_si128(::_mm_andnot_si128(is_digit, values),
                                ::_mm_and_si128(is_digit, ::_mm_add_epi8(::_mm_sub_epi8(v, ::_mm_set1_epi8('0')),
                                                                         ::_mm_set1_epi8(52))));

        const simd::vec128_t is_plus = ::_mm_cmpeq_epi8(v, ::_mm_set1_epi8(plus_char));
        values = ::_mm_or_si128(::_mm_andnot_si128(is_plus, values), ::_mm_and_si128(is_plus, ::_mm_set1_epi8(62)));

        const simd::vec128_t is_slash = ::_mm_cmpeq_epi8(v, ::_mm_set1_epi8(slash_char));
        values = ::_mm_or_si128(::_mm_andnot_si128(is_slash, values), ::_mm_and_si128(is_slash, ::_mm_set1_epi8(63)));

        return values;
    }

    NEFORCE_ALWAYS_INLINE_INLINE simd::vec128_t base64_valid_mask(const simd::vec128_t v, const char plus_char,
                                                                  const char slash_char) {
        // '=' is deliberately excluded: the SIMD region only covers data chars
        // before padding_start, where '=' is invalid.
        const simd::vec128_t upper_lo = ::_mm_cmpgt_epi8(v, ::_mm_set1_epi8('A' - 1));
        const simd::vec128_t upper_hi = ::_mm_cmpgt_epi8(::_mm_set1_epi8('Z' + 1), v);
        const simd::vec128_t lower_lo = ::_mm_cmpgt_epi8(v, ::_mm_set1_epi8('a' - 1));
        const simd::vec128_t lower_hi = ::_mm_cmpgt_epi8(::_mm_set1_epi8('z' + 1), v);
        const simd::vec128_t digit_lo = ::_mm_cmpgt_epi8(v, ::_mm_set1_epi8('0' - 1));
        const simd::vec128_t digit_hi = ::_mm_cmpgt_epi8(::_mm_set1_epi8('9' + 1), v);
        const simd::vec128_t is_upper = ::_mm_and_si128(upper_lo, upper_hi);
        const simd::vec128_t is_lower = ::_mm_and_si128(lower_lo, lower_hi);
        const simd::vec128_t is_digit = ::_mm_and_si128(digit_lo, digit_hi);
        const simd::vec128_t is_plus = ::_mm_cmpeq_epi8(v, ::_mm_set1_epi8(plus_char));
        const simd::vec128_t is_slash = ::_mm_cmpeq_epi8(v, ::_mm_set1_epi8(slash_char));
        return ::_mm_or_si128(::_mm_or_si128(::_mm_or_si128(is_upper, is_lower), ::_mm_or_si128(is_digit, is_plus)),
                              is_slash);
    }

    NEFORCE_ALWAYS_INLINE_INLINE simd::vec128_t base64_decode_16chars(const char* src, byte_t* dst,
                                                                      const char plus_char, const char slash_char) {
        const simd::vec128_t v = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(src));
        simd::vec128_t val = base64_map_to_index(v, plus_char, slash_char);
        const simd::vec128_t valid = base64_valid_mask(v, plus_char, slash_char);

        const simd::vec128_t mask6 = ::_mm_set1_epi32(0x3F);
        const simd::vec128_t r12 = ::_mm_srli_epi32(val, 12);
        const simd::vec128_t r18 = ::_mm_srli_epi32(val, 18);
        const simd::vec128_t r24 = ::_mm_srli_epi32(val, 24);

        const simd::vec128_t b0 = ::_mm_or_si128(::_mm_slli_epi32(::_mm_and_si128(val, mask6), 2),
                                                 ::_mm_and_si128(r12, ::_mm_set1_epi32(0x03)));
        const simd::vec128_t b1 =
                ::_mm_or_si128(::_mm_slli_epi32(::_mm_and_si128(::_mm_srli_epi32(val, 8), ::_mm_set1_epi32(0x0F)), 12),
                               ::_mm_slli_epi32(::_mm_and_si128(r18, ::_mm_set1_epi32(0x0F)), 8));
        const simd::vec128_t b2 =
                ::_mm_or_si128(::_mm_slli_epi32(::_mm_and_si128(::_mm_srli_epi32(val, 16), ::_mm_set1_epi32(0x03)), 22),
                               ::_mm_slli_epi32(r24, 16));

        simd::vec128_t decoded = ::_mm_or_si128(::_mm_or_si128(b0, b1), b2);

        const simd::vec128_t compact = ::_mm_set_epi8(-1, -1, -1, -1, 14, 13, 12, 10, 9, 8, 6, 5, 4, 2, 1, 0);
        decoded = ::_mm_shuffle_epi8(decoded, compact);

        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(dst), decoded);
        return valid;
    }
#endif
} // namespace


string base64::encode(const cbyte_view data) {
    if (data.empty()) {
        return {};
    }

    string result;
    result.reserve(((data.size() + 2) / 3) * 4);

    size_t i = 0;
#ifdef NEFORCE_SIMD_SSSE3
    for (; i + 12 <= data.size(); i += 12) {
        char buf[16];
        base64_encode_12bytes(data.data() + i, buf, base64_chars);
        result.append(buf, 16);
    }
#endif

    while (i + 2 < data.size()) {
        const uint32_t val = (data[i] << 16) | (data[i + 1] << 8) | data[i + 2];
        result.push_back(base64_chars[(val >> 18) & 0x3F]);
        result.push_back(base64_chars[(val >> 12) & 0x3F]);
        result.push_back(base64_chars[(val >> 6) & 0x3F]);
        result.push_back(base64_chars[val & 0x3F]);
        i += 3;
    }

    if (i < data.size()) {
        uint32_t val = data[i] << 16;
        if (i + 1 < data.size()) {
            val |= data[i + 1] << 8;
        }

        result.push_back(base64_chars[(val >> 18) & 0x3F]);
        result.push_back(base64_chars[(val >> 12) & 0x3F]);
        result.push_back(i + 1 < data.size() ? base64_chars[(val >> 6) & 0x3F] : '=');
        result.push_back('=');
    }
    return result;
}

byte_vector base64::decode(const string_view data) {
    if (data.empty()) {
        return {};
    }

    string cleaned;
    cleaned.reserve(data.size());
    for (const char c: data) {
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r' && c != '\f' && c != '\v') {
            cleaned.push_back(c);
        }
    }

    if (cleaned.size() % 4 != 0) {
        NEFORCE_THROW_EXCEPTION(value_exception("Invalid Base64 length: not a multiple of 4"));
    }

    const size_t padding_start =
            cleaned.size() >= 2 && cleaned[cleaned.size() - 1] == '='
                    ? (cleaned[cleaned.size() - 2] == '=' ? cleaned.size() - 2 : cleaned.size() - 1)
                    : cleaned.size();

    byte_vector result;
    result.reserve((cleaned.size() / 4) * 3);

#ifdef NEFORCE_SIMD_SSSE3
    byte_t buf[16];
    size_t i = 0;
    const size_t simd_end = (padding_start / 16) * 16;
    for (; i < simd_end; i += 16) {
        const simd::vec128_t valid = base64_decode_16chars(cleaned.data() + i, buf, '+', '/');
        if (::_mm_movemask_epi8(valid) != 0xFFFF) {
            NEFORCE_THROW_EXCEPTION(value_exception("Invalid Base64 character"));
        }
        const size_t out_len = (i + 16 <= padding_start) ? 12 : (padding_start - i) * 3 / 4;
        result.insert(result.end(), buf, buf + out_len);
    }

    if (i < padding_start) {
        char padded[16] = {};
        const size_t rem = padding_start - i;
        for (size_t j = 0; j < rem; ++j) {
            padded[j] = cleaned[i + j];
        }
        for (size_t j = rem; j < 16; ++j) {
            padded[j] = 'A';
        }
        const simd::vec128_t valid = base64_decode_16chars(padded, buf, '+', '/');
        const int keep_mask = (1 << static_cast<int>(rem)) - 1;
        if ((::_mm_movemask_epi8(valid) & keep_mask) != keep_mask) {
            NEFORCE_THROW_EXCEPTION(value_exception("Invalid Base64 character"));
        }
        result.insert(result.end(), buf, buf + (rem * 3 / 4));
    }

#else
    size_t i = 0;
    while (i < padding_start) {
        const int a = base64_char_to_index(cleaned[i]);
        const int b = base64_char_to_index(cleaned[i + 1]);

        if (a < 0 || b < 0) {
            NEFORCE_THROW_EXCEPTION(value_exception("Invalid Base64 character"));
        }

        const int c = (i + 2 < padding_start) ? base64_char_to_index(cleaned[i + 2]) : -1;
        const int d = (i + 3 < padding_start) ? base64_char_to_index(cleaned[i + 3]) : -1;
        if ((i + 2 < padding_start && c < 0) || (i + 3 < padding_start && d < 0)) {
            NEFORCE_THROW_EXCEPTION(value_exception("Invalid Base64 character"));
        }

        const uint32_t val = (static_cast<uint32_t>(a) << 18) | (static_cast<uint32_t>(b) << 12) |
                             (static_cast<uint32_t>(c < 0 ? 0 : c) << 6) | static_cast<uint32_t>(d < 0 ? 0 : d);

        result.push_back(static_cast<byte_t>((val >> 16) & 0xFF));
        if (i + 2 < padding_start) {
            result.push_back(static_cast<byte_t>((val >> 8) & 0xFF));
        }
        if (i + 3 < padding_start) {
            result.push_back(static_cast<byte_t>(val & 0xFF));
        }

        i += 4;
    }

#endif

    for (size_t padding = padding_start; padding < cleaned.size(); ++padding) {
        if (cleaned[padding] != '=') {
            NEFORCE_THROW_EXCEPTION(value_exception("Invalid Base64 padding"));
        }
    }

    return result;
}

string base64::encode_url(const cbyte_view data, bool padding) {
    if (data.empty()) {
        return {};
    }

    string result;
    result.reserve(((data.size() + 2) / 3) * 4);

    size_t i = 0;

#ifdef NEFORCE_SIMD_SSSE3
    for (; i + 12 <= data.size(); i += 12) {
        char buf[16];
        base64_encode_12bytes(data.data() + i, buf, base64url_chars);
        result.append(buf, 16);
    }
#endif

    while (i + 2 < data.size()) {
        const uint32_t val = (data[i] << 16) | (data[i + 1] << 8) | data[i + 2];
        result.push_back(base64url_chars[(val >> 18) & 0x3F]);
        result.push_back(base64url_chars[(val >> 12) & 0x3F]);
        result.push_back(base64url_chars[(val >> 6) & 0x3F]);
        result.push_back(base64url_chars[val & 0x3F]);
        i += 3;
    }

    if (i < data.size()) {
        uint32_t val = data[i] << 16;
        if (i + 1 < data.size()) {
            val |= data[i + 1] << 8;
        }

        result.push_back(base64url_chars[(val >> 18) & 0x3F]);
        result.push_back(base64url_chars[(val >> 12) & 0x3F]);
        const char third = (i + 1 < data.size()) ? base64url_chars[(val >> 6) & 0x3F] : (padding ? '=' : '\0');
        const char fourth = padding ? '=' : '\0';

        if (third != '\0') {
            result.push_back(third);
        }
        if (fourth != '\0') {
            result.push_back(fourth);
        }
    }
    return result;
}

byte_vector base64::decode_url(const string_view data) {
    if (data.empty()) {
        return {};
    }

    string cleaned;
    cleaned.reserve(data.size());
    for (const char c: data) {
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r' && c != '\f' && c != '\v') {
            cleaned.push_back(c);
        }
    }

    const size_t len = cleaned.size();
    size_t padding_count = 0;
    if (len % 4 != 0) {
        padding_count = 4 - (len % 4);
    }
    cleaned.append(padding_count, '=');

    const size_t padding_start =
            cleaned.size() >= 2 && cleaned[cleaned.size() - 1] == '='
                    ? (cleaned[cleaned.size() - 2] == '=' ? cleaned.size() - 2 : cleaned.size() - 1)
                    : cleaned.size();

    byte_vector result;
    result.reserve((cleaned.size() / 4) * 3);

#ifdef NEFORCE_SIMD_SSSE3
    byte_t buf[16];
    size_t i = 0;
    const size_t simd_end = (padding_start / 16) * 16;
    for (; i < simd_end; i += 16) {
        const simd::vec128_t valid = base64_decode_16chars(cleaned.data() + i, buf, '-', '_');
        if (::_mm_movemask_epi8(valid) != 0xFFFF) {
            NEFORCE_THROW_EXCEPTION(value_exception("Invalid Base64URL character"));
        }
        const size_t out_len = (i + 16 <= padding_start) ? 12 : (padding_start - i) * 3 / 4;
        result.insert(result.end(), buf, buf + out_len);
    }

    if (i < padding_start) {
        char padded[16] = {};
        const size_t rem = padding_start - i;
        for (size_t j = 0; j < rem; ++j) {
            padded[j] = cleaned[i + j];
        }
        for (size_t j = rem; j < 16; ++j) {
            padded[j] = 'A';
        }
        const simd::vec128_t valid = base64_decode_16chars(padded, buf, '-', '_');
        const int keep_mask = (1 << static_cast<int>(rem)) - 1;
        if ((::_mm_movemask_epi8(valid) & keep_mask) != keep_mask) {
            NEFORCE_THROW_EXCEPTION(value_exception("Invalid Base64URL character"));
        }
        result.insert(result.end(), buf, buf + (rem * 3 / 4));
    }

#else
    size_t i = 0;
    while (i < padding_start) {
        const int a = base64url_char_to_index(cleaned[i]);
        const int b = base64url_char_to_index(cleaned[i + 1]);

        if (a < 0 || b < 0) {
            NEFORCE_THROW_EXCEPTION(value_exception("Invalid Base64URL character"));
        }
        const int c = (i + 2 < padding_start) ? base64url_char_to_index(cleaned[i + 2]) : -1;
        const int d = (i + 3 < padding_start) ? base64url_char_to_index(cleaned[i + 3]) : -1;
        if ((i + 2 < padding_start && c < 0) || (i + 3 < padding_start && d < 0)) {
            NEFORCE_THROW_EXCEPTION(value_exception("Invalid Base64URL character"));
        }

        const uint32_t val = (static_cast<uint32_t>(a) << 18) | (static_cast<uint32_t>(b) << 12) |
                             (static_cast<uint32_t>(c < 0 ? 0 : c) << 6) | static_cast<uint32_t>(d < 0 ? 0 : d);

        result.push_back(static_cast<byte_t>((val >> 16) & 0xFF));
        if (i + 2 < padding_start) {
            result.push_back(static_cast<byte_t>((val >> 8) & 0xFF));
        }
        if (i + 3 < padding_start) {
            result.push_back(static_cast<byte_t>(val & 0xFF));
        }

        i += 4;
    }
#endif

    for (size_t padding = padding_start; padding < cleaned.size(); ++padding) {
        if (cleaned[padding] != '=') {
            NEFORCE_THROW_EXCEPTION(value_exception("Invalid Base64URL padding"));
        }
    }

    return result;
}

NEFORCE_END_NAMESPACE__
