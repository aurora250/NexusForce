#include <MSTL/core/string.hpp>
MSTL_BEGIN_NAMESPACE__

string wstring_to_utf8(const wchar_t* str) {
    string utf8_str;
    if (!str) return utf8_str;

    size_t len = char_traits<wchar_t>::length(str);
#ifdef MSTL_PLATFORM_WINDOWS__
    const int size_needed = ::WideCharToMultiByte(CP_UTF8, 0, str,
        static_cast<int>(len), nullptr, 0, nullptr, nullptr);
    if (size_needed <= 0) return utf8_str;

    utf8_str.resize(size_needed);
    ::WideCharToMultiByte(CP_UTF8, 0, str,
        static_cast<int>(len), &utf8_str[0], size_needed, nullptr, nullptr);
#elif defined(MSTL_PLATFORM_LINUX__)
    for (size_t i = 0; i < len; ++i) {
        const auto cp = static_cast<uint32_t>(str[i]);

        if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
            utf8_str += "\xEF\xBF\xBD";
            continue;
        }
        if (cp <= 0x7F) {
            utf8_str += static_cast<char>(cp);
        } else if (cp <= 0x7FF) {
            utf8_str += static_cast<char>(0xC0 | (cp >> 6));
            utf8_str += static_cast<char>(0x80 | (cp & 0x3F));
        } else if (cp <= 0xFFFF) {
            utf8_str += static_cast<char>(0xE0 | (cp >> 12));
            utf8_str += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            utf8_str += static_cast<char>(0x80 | (cp & 0x3F));
        } else {
            utf8_str += static_cast<char>(0xF0 | (cp >> 18));
            utf8_str += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            utf8_str += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            utf8_str += static_cast<char>(0x80 | (cp & 0x3F));
        }
    }
#endif
    return _MSTL move(utf8_str);
}

#ifdef MSTL_PLATFORM_LINUX__
string u16string_to_utf8(const char16_t* str) {
    string utf8_str;
    if (!str) return utf8_str;

    for (size_t i = 0; str[i] != u'\0'; ++i) {
        const char16_t c1 = str[i];
        if (i == 0 && c1 == 0xFEFF) {
            continue;
        }
        if (!is_high_surrogate(c1) && !is_low_surrogate(c1)) {
            const auto cp = static_cast<uint32_t>(c1);

            if (cp > 0x10FFFF) {
                utf8_str += "\xEF\xBF\xBD";
                continue;
            }
            if (cp <= 0x7F) {
                utf8_str += static_cast<char>(cp);
            } else if (cp <= 0x7FF) {
                utf8_str += static_cast<char>(0xC0 | (cp >> 6));
                utf8_str += static_cast<char>(0x80 | (cp & 0x3F));
            } else {
                utf8_str += static_cast<char>(0xE0 | (cp >> 12));
                utf8_str += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                utf8_str += static_cast<char>(0x80 | (cp & 0x3F));
            }
        }
        else if (is_high_surrogate(c1)) {
            if (str[i+1] == u'\0' || !is_low_surrogate(str[i+1])) {
                utf8_str += "\xEF\xBF\xBD";
                continue;
            }
            const char16_t c2 = str[++i];
            const uint32_t cp = combine_surrogates(c1, c2);

            utf8_str += static_cast<char>(0xF0 | (cp >> 18));
            utf8_str += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            utf8_str += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            utf8_str += static_cast<char>(0x80 | (cp & 0x3F));
        }
        else {
            utf8_str += "\xEF\xBF\xBD";
        }
    }
    return _MSTL move(utf8_str);
}
#endif

string u32string_to_utf8(const char32_t* str) {
    string utf8_str;
    if (!str) return utf8_str;

    size_t len = char_traits<char32_t>::length(str);
#ifdef MSTL_PLATFORM_WINDOWS__
    wstring utf16_buf;
    // In the worst case, each UTF-32 character needs to be split into 2 UTF-16 surrogate pairs.
    utf16_buf.reserve(len * 2);
    for (size_t i = 0; i < len; ++i) {
        char32_t cp = str[i];
        if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
            utf16_buf.push_back(0xFFFD);
            continue;
        }
        if (cp <= 0xFFFF) {  // BMP
            utf16_buf.push_back(static_cast<wchar_t>(cp));
        } else {
            cp -= 0x10000;
            utf16_buf.push_back(static_cast<wchar_t>((cp >> 10) + 0xD800));
            utf16_buf.push_back(static_cast<wchar_t>((cp & 0x3FF) + 0xDC00));
        }
    }
    const int size_needed = WideCharToMultiByte(CP_UTF8, 0,
        utf16_buf.data(), static_cast<int>(utf16_buf.size()),
        nullptr, 0, nullptr, nullptr
    );
    if (size_needed <= 0) return utf8_str;

    utf8_str.resize(size_needed);
    const int written = WideCharToMultiByte(CP_UTF8, 0,
        utf16_buf.data(), static_cast<int>(utf16_buf.size()),
        &utf8_str[0], size_needed, nullptr, nullptr
    );
    if (written != size_needed) {
        utf8_str.resize(written > 0 ? written : 0);
    }
#elif defined(MSTL_PLATFORM_LINUX__)
    for (size_t i = 0; i < len; ++i) {
        auto cp = static_cast<uint32_t>(str[i]);

        if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
            utf8_str += "\xEF\xBF\xBD";
            continue;
        }
        if (cp <= 0x7F) {
            utf8_str += static_cast<char>(cp);
        } else if (cp <= 0x7FF) {
            utf8_str += static_cast<char>(0xC0 | (cp >> 6));
            utf8_str += static_cast<char>(0x80 | (cp & 0x3F));
        } else if (cp <= 0xFFFF) {
            utf8_str += static_cast<char>(0xE0 | (cp >> 12));
            utf8_str += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            utf8_str += static_cast<char>(0x80 | (cp & 0x3F));
        } else {
            utf8_str += static_cast<char>(0xF0 | (cp >> 18));
            utf8_str += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            utf8_str += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            utf8_str += static_cast<char>(0x80 | (cp & 0x3F));
        }
    }
#endif
    return _MSTL move(utf8_str);
}

string escape_string(const string& str) {
    string result;
    result.reserve(str.length() + str.length() / 4);

    for (const char c : str) {
        switch (c) {
            case '\"':
                result += "\\\"";
            break;
            case '\'':
                result += "\\\'";
            break;
            case '\\':
                result += "\\\\";
            break;
            case '\b':
                result += "\\b";
            break;
            case '\f':
                result += "\\f";
            break;
            case '\n':
                result += "\\n";
            break;
            case '\r':
                result += "\\r";
            break;
            case '\t':
                result += "\\t";
            break;
            case '\v':
                result += "\\v";
            break;
            default:
                if (static_cast<byte_t>(c) < 0x20) {
                    result += "\\u";
                    constexpr char hex[] = "0123456789abcdef";
                    result += "00";
                    result += hex[(c >> 4) & 0x0F];
                    result += hex[c & 0x0F];
                } else {
                    result += c;
                }
            break;
        }
    }
    return result;
}

MSTL_END_NAMESPACE__
