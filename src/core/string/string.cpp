#include <NeForce/core/string/string.hpp>
NEFORCE_BEGIN_NAMESPACE__

string escape(const string_view str) {
    string result;
    result.reserve(str.length() + str.length() / 4);

    const auto append_escaped = [&result](const char c) {
        switch (c) {
            case '\"': {
                result += "\\\"";
                break;
            }
            case '\'': {
                result += "\\\'";
                break;
            }
            case '\\': {
                result += "\\\\";
                break;
            }
            case '\b': {
                result += "\\b";
                break;
            }
            case '\f': {
                result += "\\f";
                break;
            }
            case '\n': {
                result += "\\n";
                break;
            }
            case '\r': {
                result += "\\r";
                break;
            }
            case '\t': {
                result += "\\t";
                break;
            }
            case '\v': {
                result += "\\v";
                break;
            }
            default: {
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
    };

#ifdef NEFORCE_SIMD_SSE2
    const size_t len = str.length();
    size_t i = 0;
    while (i < len) {
        while (i + 16 <= len) {
            const ::__m128i v = ::_mm_loadu_si128(reinterpret_cast<const ::__m128i*>(str.data() + i));
            ::__m128i special = ::_mm_cmpeq_epi8(v, ::_mm_set1_epi8('"'));
            special = ::_mm_or_si128(special, ::_mm_cmpeq_epi8(v, ::_mm_set1_epi8('\'')));
            special = ::_mm_or_si128(special, ::_mm_cmpeq_epi8(v, ::_mm_set1_epi8('\\')));
            special = ::_mm_or_si128(special, ::_mm_cmpeq_epi8(::_mm_min_epu8(v, ::_mm_set1_epi8(0x1F)), v));
            const int mask = ::_mm_movemask_epi8(special);
            if (mask == 0) {
                result.append(str.data() + i, 16);
                i += 16;
                continue;
            }
            const int advance = countr_zero(static_cast<unsigned>(mask));
            if (advance > 0) {
                result.append(str.data() + i, static_cast<size_t>(advance));
                i += static_cast<size_t>(advance);
            }
            break;
        }
        if (i + 16 > len) {
            break;
        }
        append_escaped(str[i]);
        ++i;
    }
    for (; i < len; ++i) {
        append_escaped(str[i]);
    }
    return result;
#endif
    for (const char c: str) {
        append_escaped(c);
    }
    return result;
}

NEFORCE_END_NAMESPACE__
