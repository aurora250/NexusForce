#include <NeForce/core/string/codepoint.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    template <typename T>
    void append_utf8_char_aux(T&) {}

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
        } else if ((b1 & 0xE0) == 0xC0) {
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
        } else if ((b1 & 0xF0) == 0xE0) {
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
        } else if ((b1 & 0xF8) == 0xF0) {
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
}


codepoint codepoint::decode_utf8(const byte_t* data, size_t& i, const size_t len) noexcept {
    uint32_t raw;
    decode_utf8_char(data, i, len, raw);
    return codepoint(raw);
}

void codepoint::append_to(string& result) const {
    append_utf8_char(result, value_);
}

#ifdef NEFORCE_STANDARD_20
void codepoint::append_to(u8string& result) const {
    append_utf8_char(result, value_);
}
#endif

void codepoint::append_to(u16string& result) const {
    if (!is_valid_codepoint(value_)) {
        result.push_back(0xFFFD);
        return;
    }

    if (value_ <= 0xFFFF) {
        result.push_back(static_cast<char16_t>(value_));
    } else {
        const uint32_t adjusted = value_ - 0x10000u;
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
        const wchar_t high_surrogate = static_cast<wchar_t>((adjusted >> 10) + 0xD800);
        const wchar_t low_surrogate = static_cast<wchar_t>((adjusted & 0x3FF) + 0xDC00);
        result.push_back(high_surrogate);
        result.push_back(low_surrogate);
    }
#else
    result.push_back(static_cast<wchar_t>(cp));
#endif
}

NEFORCE_END_NAMESPACE__
