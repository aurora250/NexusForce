#include <NeForce/core/simd/bytes.hpp>
#include <NeForce/core/string/codepoint.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    struct cp_range {
        uint32_t first;
        uint32_t last;
    };

    constexpr cp_range WIDE_RANGES[] = {
            {0x1100, 0x115F},   // Hangul Jamo
            {0x2329, 0x232A},   // angle brackets
            {0x2E80, 0x33BF},   // CJK Radicals, Hiragana, Katakana, Bopomofo, CJK Symbols
            {0x3400, 0x4DBF},   // CJK Ext-A
            {0x4E00, 0xA4CF},   // CJK Unified, Yi
            {0xAC00, 0xD7AF},   // Hangul Syllables
            {0xF900, 0xFAFF},   // CJK Compatibility
            {0xFE10, 0xFE19},   // Vertical forms
            {0xFE30, 0xFE6F},   // CJK Compatibility Forms
            {0xFF01, 0xFF60},   // Fullwidth Forms
            {0xFFE0, 0xFFE6},   // Fullwidth Signs
            {0x1F004, 0x1F9FF}, // Emoji / Misc Symbols
            {0x20000, 0x2FFFD}, // CJK Ext-B ~
            {0x30000, 0x3FFFD}, // CJK Ext-G ~
    };

    constexpr cp_range ZERO_RANGES[] = {
            {0x0000, 0x001F}, // C0 controls
            {0x007F, 0x009F}, // C1 controls
            {0x0300, 0x036F}, // Combining Diacritical Marks
            {0x1AB0, 0x1AFF}, // Combining Diacritical Marks Extended
            {0x1DC0, 0x1DFF}, // Combining Diacritical Marks Supplement
            {0x200C, 0x200F}, // ZWNJ, ZWJ, LRM, RLM
            {0x20D0, 0x20FF}, // Combining Diacritical Marks for Symbols
            {0xFE00, 0xFE0F}, // Variation Selectors
    };

    constexpr size_t WIDE_COUNT = sizeof(WIDE_RANGES) / sizeof(WIDE_RANGES[0]);
    constexpr size_t ZERO_COUNT = sizeof(ZERO_RANGES) / sizeof(ZERO_RANGES[0]);

    constexpr bool in_ranges(const cp_range* ranges, const size_t count, const uint32_t cp) noexcept {
        size_t low = 0;
        size_t high = count;
        while (low < high) {
            const size_t mid = low + (high - low) / 2;
            if (cp < ranges[mid].first) {
                high = mid;
            } else if (cp > ranges[mid].last) {
                low = mid + 1;
            } else {
                return true;
            }
        }
        return false;
    }

    template <typename T>
    void append_utf8_char_aux(T& /*unused*/) {}

    template <>
    void append_utf8_char_aux<string>(string& result) {
        result.append("\xEF\xBF\xBD", 3);
    }

#ifdef NEFORCE_STANDARD_20
    template <>
    void append_utf8_char_aux<u8string>(u8string& result) {
        result.append(u8"\xEF\xBF\xBD", 3);
    }
#endif

    template <typename T>
    void append_utf8_char(basic_string<T>& result, uint32_t cp) {
        if (cp > 0x10FFFF || codepoint::is_high_surrogate(cp) || codepoint::is_low_surrogate(cp)) {
            append_utf8_char_aux(result);
            return;
        }

        if (cp <= 0x7F) {
            result.push_back(static_cast<T>(cp));
        } else if (cp <= 0x7FF) {
            result.push_back(static_cast<T>(0xC0 | (cp >> 6)));
            result.push_back(static_cast<T>(0x80 | (cp & 0x3F)));
        } else if (cp <= 0xFFFF) {
            result.push_back(static_cast<T>(0xE0 | (cp >> 12)));
            result.push_back(static_cast<T>(0x80 | ((cp >> 6) & 0x3F)));
            result.push_back(static_cast<T>(0x80 | (cp & 0x3F)));
        } else {
            result.push_back(static_cast<T>(0xF0 | (cp >> 18)));
            result.push_back(static_cast<T>(0x80 | ((cp >> 12) & 0x3F)));
            result.push_back(static_cast<T>(0x80 | ((cp >> 6) & 0x3F)));
            result.push_back(static_cast<T>(0x80 | (cp & 0x3F)));
        }
    }

    bool decode_utf8_char(const byte_t* data, size_t& i, const size_t len, uint32_t& cp) noexcept {
        if (i >= len) {
            cp = 0xFFFD;
            return false;
        }

        const byte_t b1 = data[i++];
        if ((b1 & 0x80) == 0) {
            cp = b1;
            return true;
        }
        if ((b1 & 0xE0) == 0xC0) {
            if (len - i < 1) {
                cp = 0xFFFD;
                return false;
            }
            const byte_t b2 = data[i];
            if ((b2 & 0xC0) != 0x80) {
                cp = 0xFFFD;
                return false;
            }
            ++i;
            cp = ((b1 & 0x1F) << 6) | (b2 & 0x3F);
            if (cp < 0x80) {
                cp = 0xFFFD;
                return false;
            }
            return true;
        }
        if ((b1 & 0xF0) == 0xE0) {
            if (len - i < 2) {
                cp = 0xFFFD;
                return false;
            }
            const byte_t b2 = data[i];
            if ((b2 & 0xC0) != 0x80) {
                cp = 0xFFFD;
                return false;
            }
            ++i;
            const byte_t b3 = data[i];
            if ((b3 & 0xC0) != 0x80) {
                cp = 0xFFFD;
                return false;
            }
            ++i;
            cp = ((b1 & 0x0F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
            if (cp < 0x800 || (cp >= 0xD800 && cp <= 0xDFFF)) {
                cp = 0xFFFD;
                return false;
            }
            return true;
        }
        if ((b1 & 0xF8) == 0xF0) {
            if (len - i < 3) {
                cp = 0xFFFD;
                return false;
            }
            const byte_t b2 = data[i];
            if ((b2 & 0xC0) != 0x80) {
                cp = 0xFFFD;
                return false;
            }
            ++i;
            const byte_t b3 = data[i];
            if ((b3 & 0xC0) != 0x80) {
                cp = 0xFFFD;
                return false;
            }
            ++i;
            const byte_t b4 = data[i];
            if ((b4 & 0xC0) != 0x80) {
                cp = 0xFFFD;
                return false;
            }
            ++i;
            cp = ((b1 & 0x07) << 18) | ((b2 & 0x3F) << 12) | ((b3 & 0x3F) << 6) | (b4 & 0x3F);
            if (cp < 0x10000 || cp > 0x10FFFF) {
                cp = 0xFFFD;
                return false;
            }
            return true;
        }

        cp = 0xFFFD;
        return false;
    }

#if defined(NEFORCE_SIMD_SSE2)
    NEFORCE_ALWAYS_INLINE_INLINE void expand_ascii16(const simd::vec128_t v, char32_t* out) noexcept {
        const __m128i zero = _mm_setzero_si128();
        const __m128i low_half = _mm_unpacklo_epi8(v, zero);
        const __m128i high_half = _mm_unpackhi_epi8(v, zero);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(out), _mm_unpacklo_epi16(low_half, zero));
        _mm_storeu_si128(reinterpret_cast<__m128i*>(out + 4), _mm_unpackhi_epi16(low_half, zero));
        _mm_storeu_si128(reinterpret_cast<__m128i*>(out + 8), _mm_unpacklo_epi16(high_half, zero));
        _mm_storeu_si128(reinterpret_cast<__m128i*>(out + 12), _mm_unpackhi_epi16(high_half, zero));
    }

    NEFORCE_ALWAYS_INLINE_INLINE bool block_len2_ok(const simd::vec128_t v) noexcept {
        const __m128i byte_mask = _mm_set1_epi16(0x00FF);
        const __m128i lead = _mm_and_si128(v, byte_mask);
        const __m128i cont = _mm_and_si128(_mm_srli_epi16(v, 8), byte_mask);
        const __m128i lead_low = _mm_cmpgt_epi16(lead, _mm_set1_epi16(0x00C1));
        const __m128i lead_high = _mm_cmpgt_epi16(_mm_set1_epi16(0x00E0), lead);
        const __m128i cont_ok = _mm_cmpeq_epi16(_mm_and_si128(cont, _mm_set1_epi16(0x00C0)), _mm_set1_epi16(0x0080));
        const __m128i ok = _mm_and_si128(_mm_and_si128(lead_low, lead_high), cont_ok);
        return simd::to_bitmask(ok) == 0xFFFF;
    }

    NEFORCE_ALWAYS_INLINE_INLINE void decode_len2_block(const simd::vec128_t v, char32_t* out) noexcept {
        const __m128i lead = _mm_and_si128(v, _mm_set1_epi16(0x001F));
        const __m128i cont = _mm_and_si128(_mm_srli_epi16(v, 8), _mm_set1_epi16(0x003F));
        const __m128i packed = _mm_or_si128(_mm_slli_epi16(lead, 6), cont);
        const __m128i zero = _mm_setzero_si128();
        _mm_storeu_si128(reinterpret_cast<__m128i*>(out), _mm_unpacklo_epi16(packed, zero));
        _mm_storeu_si128(reinterpret_cast<__m128i*>(out + 4), _mm_unpackhi_epi16(packed, zero));
    }

    NEFORCE_ALWAYS_INLINE_INLINE bool block_len4_ok(const simd::vec128_t v) noexcept {
        const __m128i byte_mask = _mm_set1_epi32(0x000000FF);
        const __m128i b0 = _mm_and_si128(v, byte_mask);
        const __m128i b1 = _mm_and_si128(_mm_srli_epi32(v, 8), byte_mask);
        const __m128i b2 = _mm_and_si128(_mm_srli_epi32(v, 16), byte_mask);
        const __m128i b3 = _mm_and_si128(_mm_srli_epi32(v, 24), byte_mask);

        const __m128i b0_low = _mm_cmpgt_epi32(b0, _mm_set1_epi32(0xEF));
        const __m128i b0_high = _mm_cmpgt_epi32(_mm_set1_epi32(0xF5), b0);
        const __m128i cont_mask = _mm_set1_epi32(0xC0);
        const __m128i cont_value = _mm_set1_epi32(0x80);
        const __m128i c1 = _mm_cmpeq_epi32(_mm_and_si128(b1, cont_mask), cont_value);
        const __m128i c2 = _mm_cmpeq_epi32(_mm_and_si128(b2, cont_mask), cont_value);
        const __m128i c3 = _mm_cmpeq_epi32(_mm_and_si128(b3, cont_mask), cont_value);
        __m128i ok = _mm_and_si128(_mm_and_si128(b0_low, b0_high), _mm_and_si128(c1, _mm_and_si128(c2, c3)));

        const __m128i is_f0 = _mm_cmpeq_epi32(b0, _mm_set1_epi32(0xF0));
        const __m128i is_f4 = _mm_cmpeq_epi32(b0, _mm_set1_epi32(0xF4));
        const __m128i f0_edge = _mm_and_si128(is_f0, _mm_cmpgt_epi32(_mm_set1_epi32(0x90), b1));
        const __m128i f4_edge = _mm_and_si128(is_f4, _mm_cmpgt_epi32(b1, _mm_set1_epi32(0x8F)));
        ok = _mm_andnot_si128(_mm_or_si128(f0_edge, f4_edge), ok);

        return simd::to_bitmask(ok) == 0xFFFF;
    }

    NEFORCE_ALWAYS_INLINE_INLINE void decode_len4_block(const simd::vec128_t v, char32_t* out) noexcept {
        const __m128i low_bits = _mm_set1_epi32(0x07);
        const __m128i cont_bits = _mm_set1_epi32(0x3F);
        const __m128i b0 = _mm_and_si128(v, low_bits);
        const __m128i b1 = _mm_and_si128(_mm_srli_epi32(v, 8), cont_bits);
        const __m128i b2 = _mm_and_si128(_mm_srli_epi32(v, 16), cont_bits);
        const __m128i b3 = _mm_and_si128(_mm_srli_epi32(v, 24), cont_bits);
        const __m128i packed = _mm_or_si128(_mm_or_si128(_mm_slli_epi32(b0, 18), _mm_slli_epi32(b1, 12)),
                                            _mm_or_si128(_mm_slli_epi32(b2, 6), b3));
        _mm_storeu_si128(reinterpret_cast<__m128i*>(out), packed);
    }

    NEFORCE_ALWAYS_INLINE_INLINE bool utf16_block_widenable(const simd::vec128_t v) noexcept {
        const __m128i surrogate = _mm_cmpeq_epi16(_mm_and_si128(v, _mm_set1_epi16(static_cast<short>(0xF800))),
                                                  _mm_set1_epi16(static_cast<short>(0xD800)));
        return simd::to_bitmask(surrogate) == 0;
    }

    NEFORCE_ALWAYS_INLINE_INLINE void widen_utf16_block(const simd::vec128_t v, char32_t* out) noexcept {
        const __m128i zero = _mm_setzero_si128();
        _mm_storeu_si128(reinterpret_cast<__m128i*>(out), _mm_unpacklo_epi16(v, zero));
        _mm_storeu_si128(reinterpret_cast<__m128i*>(out + 4), _mm_unpackhi_epi16(v, zero));
    }
#endif // NEFORCE_SIMD_SSE2

#if defined(NEFORCE_SIMD_SSSE3)
    NEFORCE_ALWAYS_INLINE_INLINE bool block_len3_ok(const simd::vec128_t v) noexcept {
        constexpr char z = static_cast<char>(0x80);
        const __m128i lead = _mm_shuffle_epi8(v, _mm_setr_epi8(0, 3, 6, 9, z, z, z, z, z, z, z, z, z, z, z, z));
        const __m128i first = _mm_shuffle_epi8(v, _mm_setr_epi8(1, 4, 7, 10, z, z, z, z, z, z, z, z, z, z, z, z));
        const __m128i second = _mm_shuffle_epi8(v, _mm_setr_epi8(2, 5, 8, 11, z, z, z, z, z, z, z, z, z, z, z, z));

        const __m128i lead_ok = _mm_cmpeq_epi8(_mm_and_si128(lead, _mm_set1_epi8(static_cast<char>(0xF0))),
                                               _mm_set1_epi8(static_cast<char>(0xE0)));
        const __m128i cont_mask = _mm_set1_epi8(static_cast<char>(0xC0));
        const __m128i cont_value = _mm_set1_epi8(static_cast<char>(0x80));
        const __m128i first_ok = _mm_cmpeq_epi8(_mm_and_si128(first, cont_mask), cont_value);
        const __m128i second_ok = _mm_cmpeq_epi8(_mm_and_si128(second, cont_mask), cont_value);
        const __m128i ok = _mm_and_si128(_mm_and_si128(lead_ok, first_ok), second_ok);

        // first <= 0x9F  <=>  max(first, 0x9F) == 0x9F
        const __m128i first_below_a0 = _mm_cmpeq_epi8(_mm_max_epu8(first, _mm_set1_epi8(static_cast<char>(0x9F))),
                                                      _mm_set1_epi8(static_cast<char>(0x9F)));
        const __m128i is_e0 = _mm_cmpeq_epi8(lead, _mm_set1_epi8(static_cast<char>(0xE0)));
        const __m128i is_ed = _mm_cmpeq_epi8(lead, _mm_set1_epi8(static_cast<char>(0xED)));
        const __m128i e0_edge = _mm_and_si128(is_e0, first_below_a0);
        const __m128i ed_edge = _mm_and_si128(is_ed, _mm_cmpeq_epi8(first_below_a0, _mm_setzero_si128()));

        const int ok_bits = simd::to_bitmask(ok) & 0x000F;
        const int edge_bits = simd::to_bitmask(_mm_or_si128(e0_edge, ed_edge)) & 0x000F;
        return ok_bits == 0x000F && edge_bits == 0;
    }

    NEFORCE_ALWAYS_INLINE_INLINE void decode_len3_block(const simd::vec128_t v, char32_t* out) noexcept {
        constexpr char z = static_cast<char>(0x80);
        const __m128i gathered = _mm_shuffle_epi8(v, _mm_setr_epi8(0, 1, 2, z, 3, 4, 5, z, 6, 7, 8, z, 9, 10, 11, z));
        const __m128i b0 = _mm_and_si128(gathered, _mm_set1_epi32(0x0F));
        const __m128i b1 = _mm_and_si128(_mm_srli_epi32(gathered, 8), _mm_set1_epi32(0x3F));
        const __m128i b2 = _mm_and_si128(_mm_srli_epi32(gathered, 16), _mm_set1_epi32(0x3F));
        const __m128i packed = _mm_or_si128(_mm_slli_epi32(b0, 12), _mm_or_si128(_mm_slli_epi32(b1, 6), b2));
        _mm_storeu_si128(reinterpret_cast<__m128i*>(out), packed);
    }
#endif // NEFORCE_SIMD_SSSE3

#if defined(NEFORCE_SIMD_SSE2)
    NEFORCE_ALWAYS_INLINE_INLINE bool utf32_block_valid(const simd::vec128_t v) noexcept {
        const __m128i bias = _mm_set1_epi32(static_cast<int>(0x80000000U));
        const __m128i biased = _mm_xor_si128(v, bias);
        const __m128i limit = _mm_xor_si128(_mm_set1_epi32(0x10FFFF), bias);
        const __m128i too_large = _mm_cmpgt_epi32(biased, limit);
        const __m128i above_surrogates = _mm_cmpgt_epi32(biased, _mm_xor_si128(_mm_set1_epi32(0xD7FF), bias));
        const __m128i below_private = _mm_cmpgt_epi32(_mm_xor_si128(_mm_set1_epi32(0xE000), bias), biased);
        const __m128i surrogate = _mm_and_si128(above_surrogates, below_private);
        return simd::to_bitmask(_mm_or_si128(too_large, surrogate)) == 0;
    }
#endif // NEFORCE_SIMD_SSE2
} // namespace


size_t codepoint::decode_utf8_chunk(const byte_t* data, const size_t len, char32_t* out, const size_t capacity,
                                    size_t& consumed) noexcept {
    size_t i = 0;
    size_t n = 0;

    while (i < len && n < capacity) {
        bool handled = false;

#if defined(NEFORCE_SIMD_AVX2)
        if (i + 32 <= len && n + 32 <= capacity &&
            ::_mm256_movemask_epi8(::_mm256_loadu_si256(reinterpret_cast<const __m256i*>(data + i))) == 0) {
            expand_ascii16(simd::load_unaligned(data + i), out + n);
            expand_ascii16(simd::load_unaligned(data + i + 16), out + n + 16);
            i += 32;
            n += 32;
            handled = true;
        }
#endif

#if defined(NEFORCE_SIMD_SSE2)
        if (!handled && i + 16 <= len) {
            const simd::vec128_t v = simd::load_unaligned(data + i);
            if (simd::to_bitmask(v) == 0 && n + 16 <= capacity) {
                expand_ascii16(v, out + n);
                i += 16;
                n += 16;
                handled = true;
            } else if (n + 8 <= capacity && block_len2_ok(v)) {
                decode_len2_block(v, out + n);
                i += 16;
                n += 8;
                handled = true;
            } else if (n + 4 <= capacity && block_len4_ok(v)) {
                decode_len4_block(v, out + n);
                i += 16;
                n += 4;
                handled = true;
            }
        }
#endif

#if defined(NEFORCE_SIMD_SSSE3)
        if (!handled && i + 16 <= len && n + 4 <= capacity) {
            const simd::vec128_t v3 = simd::load_unaligned(data + i);
            if (block_len3_ok(v3)) {
                decode_len3_block(v3, out + n);
                i += 12;
                n += 4;
                handled = true;
            }
        }
#endif

        if (handled) {
            continue;
        }

        uint32_t cp = 0;
        static_cast<void>(decode_utf8_char(data, i, len, cp));
        out[n] = cp;
        ++n;
    }

    consumed = i;
    return n;
}

template <typename Unit>
size_t codepoint::decode_utf16_chunk(const Unit* data, const size_t len, const bool need_swap, char32_t* out,
                                     const size_t capacity, size_t& consumed) noexcept {
    size_t i = 0;
    size_t n = 0;

    while (i < len && n < capacity) {
        bool handled = false;

#if defined(NEFORCE_SIMD_SSE2)
        if (!need_swap && sizeof(Unit) == 2 && i + 8 <= len && n + 8 <= capacity) {
            const simd::vec128_t v = simd::load_unaligned(data + i);
            if (utf16_block_widenable(v)) {
                widen_utf16_block(v, out + n);
                i += 8;
                n += 8;
                handled = true;
            }
        }
#endif

        if (handled) {
            continue;
        }

        size_t index = i;
        const codepoint value = codepoint::decode_utf16(data, index, len, need_swap);
        out[n] = value.value();
        ++n;
        i = index > i ? index : i + 1;
    }

    consumed = i;
    return n;
}

template <typename Unit>
size_t codepoint::decode_utf32_chunk(const Unit* data, const size_t len, char32_t* out, const size_t capacity,
                                     size_t& consumed) noexcept {
    size_t i = 0;
    size_t n = 0;

    while (i < len && n < capacity) {
        bool handled = false;

#if defined(NEFORCE_SIMD_SSE2)
        if (sizeof(Unit) == 4 && i + 4 <= len && n + 4 <= capacity) {
            const simd::vec128_t v = simd::load_unaligned(data + i);
            if (utf32_block_valid(v)) {
                _mm_storeu_si128(reinterpret_cast<__m128i*>(out + n), v);
                i += 4;
                n += 4;
                handled = true;
            }
        }
#endif

        if (handled) {
            continue;
        }

        // NOLINTNEXTLINE(bugprone-signed-char-misuse)
        const auto raw = static_cast<uint32_t>(data[i]);
        out[n] = is_valid_codepoint(raw) ? raw : REPLACEMENT_VALUE;
        ++i;
        ++n;
    }

    consumed = i;
    return n;
}

void codepoint::append_chunk(string& result, const char32_t* codepoints, const size_t count) {
    size_t i = 0;
    while (i < count) {
        size_t run = 0;
        while (i + run < count && codepoints[i + run] <= 0x7F) {
            ++run;
        }
        if (run != 0) {
            const size_t base = result.size();
            result.resize(base + run);
            char* dst = result.data() + base;
            for (size_t k = 0; k < run; ++k) {
                dst[k] = static_cast<char>(codepoints[i + k]);
            }
            i += run;
            continue;
        }
        append_utf8_char(result, codepoints[i]);
        ++i;
    }
}

#ifdef NEFORCE_STANDARD_20
void codepoint::append_chunk(u8string& result, const char32_t* codepoints, const size_t count) {
    size_t i = 0;
    while (i < count) {
        size_t run = 0;
        while (i + run < count && codepoints[i + run] <= 0x7F) {
            ++run;
        }
        if (run != 0) {
            const size_t base = result.size();
            result.resize(base + run);
            char8_t* dst = result.data() + base;
            for (size_t k = 0; k < run; ++k) {
                dst[k] = static_cast<char8_t>(codepoints[i + k]);
            }
            i += run;
            continue;
        }
        append_utf8_char(result, codepoints[i]);
        ++i;
    }
}
#endif

void codepoint::append_chunk(u16string& result, const char32_t* codepoints, const size_t count) {
    size_t i = 0;
    while (i < count) {
        size_t run = 0;
        while (i + run < count && codepoints[i + run] <= 0xFFFF) {
            ++run;
        }
        if (run != 0) {
            const size_t base = result.size();
            result.resize(base + run);
            char16_t* dst = result.data() + base;
            for (size_t k = 0; k < run; ++k) {
                dst[k] = static_cast<char16_t>(codepoints[i + k]);
            }
            i += run;
            continue;
        }
        codepoint(codepoints[i]).append_to(result);
        ++i;
    }
}

void codepoint::append_chunk(u32string& result, const char32_t* codepoints, const size_t count) {
    result.append(codepoints, count);
}

void codepoint::append_chunk(wstring& result, const char32_t* codepoints, const size_t count) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    size_t i = 0;
    while (i < count) {
        size_t run = 0;
        while (i + run < count && codepoints[i + run] <= 0xFFFF) {
            ++run;
        }
        if (run != 0) {
            const size_t base = result.size();
            result.resize(base + run);
            wchar_t* dst = result.data() + base;
            for (size_t k = 0; k < run; ++k) {
                dst[k] = static_cast<wchar_t>(codepoints[i + k]);
            }
            i += run;
            continue;
        }
        codepoint(codepoints[i]).append_to(result);
        ++i;
    }
#else
    const size_t base = result.size();
    result.resize(base + count);
    wchar_t* dst = result.data() + base;
    for (size_t k = 0; k < count; ++k) {
        dst[k] = static_cast<wchar_t>(codepoints[k]);
    }
#endif
}

template <typename Target>
void codepoint::transcode_utf8(const byte_t* data, const size_t len, basic_string<Target>& result) {
    constexpr size_t chunk_size = 256;
    char32_t buffer[chunk_size];
    size_t offset = 0;

    while (offset < len) {
        size_t consumed = 0;
        const size_t count = decode_utf8_chunk(data + offset, len - offset, buffer, chunk_size, consumed);
        if (count != 0) {
            append_chunk(result, buffer, count);
        }
        if (consumed == 0) {
            break;
        }
        offset += consumed;
    }
}

template <typename Unit, typename Target>
void codepoint::transcode_utf16(const Unit* data, const size_t len, const bool need_swap,
                                basic_string<Target>& result) {
    constexpr size_t chunk_size = 256;
    char32_t buffer[chunk_size];
    size_t offset = 0;

    while (offset < len) {
        size_t consumed = 0;
        const size_t count = decode_utf16_chunk(data + offset, len - offset, need_swap, buffer, chunk_size, consumed);
        if (count != 0) {
            append_chunk(result, buffer, count);
        }
        if (consumed == 0) {
            break;
        }
        offset += consumed;
    }
}

template <typename Unit, typename Target>
void codepoint::transcode_utf32(const Unit* data, const size_t len, basic_string<Target>& result) {
    constexpr size_t chunk_size = 256;
    char32_t buffer[chunk_size];
    size_t offset = 0;

    while (offset < len) {
        size_t consumed = 0;
        const size_t count = decode_utf32_chunk(data + offset, len - offset, buffer, chunk_size, consumed);
        if (count != 0) {
            append_chunk(result, buffer, count);
        }
        if (consumed == 0) {
            break;
        }
        offset += consumed;
    }
}

void codepoint::decode_utf8(const byte_t* data, const size_t len, string& result) { transcode_utf8(data, len, result); }

void codepoint::decode_utf8(const byte_t* data, const size_t len, wstring& result) {
    transcode_utf8(data, len, result);
}

#ifdef NEFORCE_STANDARD_20
void codepoint::decode_utf8(const byte_t* data, const size_t len, u8string& result) {
    transcode_utf8(data, len, result);
}
#endif

void codepoint::decode_utf8(const byte_t* data, const size_t len, u16string& result) {
    transcode_utf8(data, len, result);
}

void codepoint::decode_utf8(const byte_t* data, const size_t len, u32string& result) {
    transcode_utf8(data, len, result);
}

void codepoint::decode_utf16(const char16_t* data, const size_t len, const bool need_swap, string& result) {
    transcode_utf16(data, len, need_swap, result);
}

void codepoint::decode_utf16(const char16_t* data, const size_t len, const bool need_swap, wstring& result) {
    transcode_utf16(data, len, need_swap, result);
}

#ifdef NEFORCE_STANDARD_20
void codepoint::decode_utf16(const char16_t* data, const size_t len, const bool need_swap, u8string& result) {
    transcode_utf16(data, len, need_swap, result);
}
#endif

void codepoint::decode_utf16(const char16_t* data, const size_t len, const bool need_swap, u16string& result) {
    transcode_utf16(data, len, need_swap, result);
}

void codepoint::decode_utf16(const char16_t* data, const size_t len, const bool need_swap, u32string& result) {
    transcode_utf16(data, len, need_swap, result);
}

void codepoint::decode_wchar(const wchar_t* data, const size_t len, const bool need_swap, string& result) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    transcode_utf16(data, len, need_swap, result);
#else
    static_cast<void>(need_swap);
    transcode_utf32(data, len, result);
#endif
}

#ifdef NEFORCE_STANDARD_20
void codepoint::decode_wchar(const wchar_t* data, const size_t len, const bool need_swap, u8string& result) {
#    ifdef NEFORCE_PLATFORM_WINDOWS
    transcode_utf16(data, len, need_swap, result);
#    else
    static_cast<void>(need_swap);
    transcode_utf32(data, len, result);
#    endif
}
#endif

void codepoint::decode_wchar(const wchar_t* data, const size_t len, const bool need_swap, u16string& result) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    transcode_utf16(data, len, need_swap, result);
#else
    static_cast<void>(need_swap);
    transcode_utf32(data, len, result);
#endif
}

void codepoint::decode_wchar(const wchar_t* data, const size_t len, const bool need_swap, u32string& result) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    transcode_utf16(data, len, need_swap, result);
#else
    static_cast<void>(need_swap);
    transcode_utf32(data, len, result);
#endif
}

void codepoint::encode_utf32(const char32_t* data, const size_t len, string& result) {
    transcode_utf32(data, len, result);
}

void codepoint::encode_utf32(const char32_t* data, const size_t len, wstring& result) {
    transcode_utf32(data, len, result);
}

#ifdef NEFORCE_STANDARD_20
void codepoint::encode_utf32(const char32_t* data, const size_t len, u8string& result) {
    transcode_utf32(data, len, result);
}
#endif

void codepoint::encode_utf32(const char32_t* data, const size_t len, u16string& result) {
    transcode_utf32(data, len, result);
}

void codepoint::encode_utf32(const char32_t* data, const size_t len, u32string& result) {
    transcode_utf32(data, len, result);
}

int codepoint::display_width() const noexcept {
    const uint32_t cp = value_;
    if (cp > MAX_VALUE) {
        return 0;
    }
    if (in_ranges(ZERO_RANGES, ZERO_COUNT, cp)) {
        return 0;
    }
    if (in_ranges(WIDE_RANGES, WIDE_COUNT, cp)) {
        return 2;
    }
    return 1;
}

codepoint codepoint::decode_utf8(const byte_t* data, size_t& i, const size_t len) noexcept {
    uint32_t raw = 0;
    decode_utf8_char(data, i, len, raw);
    return codepoint(raw);
}

void codepoint::append_to(string& result) const { append_utf8_char(result, value_); }

#ifdef NEFORCE_STANDARD_20
void codepoint::append_to(u8string& result) const { append_utf8_char(result, value_); }
#endif

void codepoint::append_to(u16string& result) const {
    if (!is_valid_codepoint(value_)) {
        result.push_back(0xFFFD);
        return;
    }

    if (value_ <= 0xFFFF) {
        result.push_back(static_cast<char16_t>(value_));
    } else {
        const uint32_t adjusted = value_ - 0x10000U;
        const auto high_surrogate = static_cast<char16_t>((adjusted >> 10) + 0xD800);
        const auto low_surrogate = static_cast<char16_t>((adjusted & 0x3FF) + 0xDC00);
        result.push_back(high_surrogate);
        result.push_back(low_surrogate);
    }
}

void codepoint::append_to(wstring& result) const {
    if (!is_valid_codepoint(value_)) {
        result.push_back(0xFFFD);
        return;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (value_ <= 0xFFFF) {
        result.push_back(static_cast<wchar_t>(value_));
    } else {
        const uint32_t adjusted = value_ - 0x10000;
        const auto high_surrogate = static_cast<wchar_t>((adjusted >> 10) + 0xD800);
        const auto low_surrogate = static_cast<wchar_t>((adjusted & 0x3FF) + 0xDC00);
        result.push_back(high_surrogate);
        result.push_back(low_surrogate);
    }
#else
    result.push_back(static_cast<wchar_t>(value_));
#endif
}

NEFORCE_END_NAMESPACE__
