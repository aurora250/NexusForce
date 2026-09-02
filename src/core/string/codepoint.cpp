#include <NeForce/core/container/array.hpp>
#include <NeForce/core/string/codepoint.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    constexpr size_t BLOCK_BITS = 64;
    constexpr size_t BLOCK_COUNT = (codepoint::MAX_VALUE + BLOCK_BITS) / BLOCK_BITS;

    constexpr void set_bit(array<uint64_t, BLOCK_COUNT>& bitmap, const uint32_t cp) noexcept {
        if (cp <= codepoint::MAX_VALUE) {
            const size_t idx = cp / BLOCK_BITS;
            const size_t off = cp % BLOCK_BITS;
            bitmap[idx] |= (1ULL << off);
        }
    }

    constexpr void set_range(array<uint64_t, BLOCK_COUNT>& bitmap, const uint32_t start, const uint32_t end) noexcept {
        for (uint32_t cp = start; cp <= end; ++cp) {
            set_bit(bitmap, cp);
        }
    }

    constexpr array<uint64_t, BLOCK_COUNT> build_wide_bitmap() noexcept {
        array<uint64_t, BLOCK_COUNT> bitmap{};
        set_range(bitmap, 0x1100, 0x115F);   // Hangul Jamo
        set_range(bitmap, 0x2329, 0x232A);   // angle brackets
        set_range(bitmap, 0x2E80, 0x303F);   // CJK Radicals
        set_range(bitmap, 0x3040, 0x33BF);   // Hiragana, Katakana, Bopomofo, CJK Symbols
        set_range(bitmap, 0x3400, 0x4DBF);   // CJK Ext-A
        set_range(bitmap, 0x4E00, 0x9FFF);   // CJK Unified
        set_range(bitmap, 0xA000, 0xA4CF);   // Yi
        set_range(bitmap, 0xAC00, 0xD7AF);   // Hangul Syllables
        set_range(bitmap, 0xF900, 0xFAFF);   // CJK Compatibility
        set_range(bitmap, 0xFE10, 0xFE19);   // Vertical forms
        set_range(bitmap, 0xFE30, 0xFE6F);   // CJK Compatibility Forms
        set_range(bitmap, 0xFF01, 0xFF60);   // Fullwidth Forms
        set_range(bitmap, 0xFFE0, 0xFFE6);   // Fullwidth Signs
        set_range(bitmap, 0x1F004, 0x1F9FF); // Emoji / Misc Symbols
        set_range(bitmap, 0x20000, 0x2FFFD); // CJK Ext-B ~
        set_range(bitmap, 0x30000, 0x3FFFD); // CJK Ext-G ~
        return bitmap;
    }

    constexpr array<uint64_t, BLOCK_COUNT> build_zero_bitmap() noexcept {
        array<uint64_t, BLOCK_COUNT> bitmap{};
        // C0 (0x00~0x1F)
        set_range(bitmap, 0x0000, 0x001F);
        // C1 (0x7F~0x9F)
        set_range(bitmap, 0x007F, 0x009F);
        // (ZWJ, ZWNJ)
        set_bit(bitmap, 0x200D); // ZWJ
        set_bit(bitmap, 0x200C); // ZWNJ
        set_bit(bitmap, 0x200E); // LRM
        set_bit(bitmap, 0x200F); // RLM
        //Combining Diacritical Marks
        set_range(bitmap, 0x0300, 0x036F);
        set_range(bitmap, 0x1AB0, 0x1AFF);
        set_range(bitmap, 0x1DC0, 0x1DFF);
        set_range(bitmap, 0x20D0, 0x20FF);
        // Variation Selectors
        set_range(bitmap, 0xFE00, 0xFE0F);
        return bitmap;
    }

    auto WIDE_BITMAP = build_wide_bitmap();
    auto ZERO_BITMAP = build_zero_bitmap();

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
} // namespace


int codepoint::display_width() const noexcept {
    const uint32_t cp = value_;
    if (cp > MAX_VALUE) {
        return 0;
    }

    const size_t block = cp / BLOCK_BITS;
    const size_t offset = cp % BLOCK_BITS;
    const uint64_t mask = 1ULL << offset;

    if ((ZERO_BITMAP[block] & mask) != 0U) {
        return 0;
    }
    if ((WIDE_BITMAP[block] & mask) != 0U) {
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
