#include <NeForce/core/encoding/percent.hpp>
#include <NeForce/core/exception/exception.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    constexpr string_view PERCENT_HEX = "0123456789ABCDEF";

    constexpr bool is_unreserved(const byte_t value) noexcept {
        return (value >= 'A' && value <= 'Z') || (value >= 'a' && value <= 'z') || (value >= '0' && value <= '9') ||
               value == '-' || value == '_' || value == '.' || value == '~';
    }

    constexpr int hex_value(const char c) noexcept {
        if (c >= '0' && c <= '9') {
            return c - '0';
        }
        if (c >= 'A' && c <= 'F') {
            return c - 'A' + 10;
        }
        if (c >= 'a' && c <= 'f') {
            return c - 'a' + 10;
        }
        return -1;
    }

    void append_encoded(string& output, const byte_t value) {
        output.push_back('%');
        output.push_back(PERCENT_HEX[value >> 4]);
        output.push_back(PERCENT_HEX[value & 0x0F]);
    }

    bool decode_impl(const string_view text, byte_vector& output) {
        output.clear();
        output.reserve(text.size());

        size_t i = 0;
        while (i < text.size()) {
            if (text[i] == '%') {
                if (i + 2 >= text.size()) {
                    return false;
                }
                const int high = hex_value(text[i + 1]);
                const int low = hex_value(text[i + 2]);
                if (high < 0 || low < 0) {
                    return false;
                }
                output.push_back(static_cast<byte_t>((high << 4) | low));
                i += 3;
            } else {
                output.push_back(static_cast<byte_t>(text[i]));
                ++i;
            }
        }
        return true;
    }
} // namespace


string percent::encode(const cbyte_view data, const bool encode_slash) {
    string result;
    result.reserve(data.size() * 3);

    for (const byte_t value: data) {
        if (is_unreserved(value) || (!encode_slash && value == '/')) {
            result.push_back(static_cast<char>(value));
        } else {
            append_encoded(result, value);
        }
    }
    return result;
}

byte_vector percent::decode(const string_view text) {
    byte_vector result;
    if (!decode_impl(text, result)) {
        NEFORCE_THROW_EXCEPTION(value_exception("Invalid percent-encoding"));
    }
    return result;
}

optional<byte_vector> percent::try_decode(const string_view text) noexcept {
    try {
        byte_vector result;
        if (!decode_impl(text, result)) {
            return none;
        }
        return result;
    } catch (...) {
        return none;
    }
}

string percent::encode_form(const cbyte_view data) {
    string result;
    result.reserve(data.size() * 3);

    for (const byte_t value: data) {
        if (is_unreserved(value)) {
            result.push_back(static_cast<char>(value));
        } else if (value == ' ') {
            result.push_back('+');
        } else {
            append_encoded(result, value);
        }
    }
    return result;
}

string percent::decode_tolerant(const string_view text) {
    string result;
    result.reserve(text.size());

    size_t i = 0;
    while (i < text.size()) {
        if (text[i] == '%' && i + 2 < text.size()) {
            const int high = hex_value(text[i + 1]);
            const int low = hex_value(text[i + 2]);
            if (high >= 0 && low >= 0) {
                result.push_back(static_cast<char>((high << 4) | low));
                i += 3;
                continue;
            }
        }

        if (text[i] == '+') {
            result.push_back(' ');
        } else {
            result.push_back(text[i]);
        }
        ++i;
    }
    return result;
}

NEFORCE_END_NAMESPACE__
