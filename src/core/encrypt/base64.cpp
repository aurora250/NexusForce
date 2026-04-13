#include <NeForce/core/encrypt/base64.hpp>
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
} // namespace


string base64::encode(const cbyte_view data) {
    string result;
    size_t i = 0;

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

    byte_vector result;
    result.reserve((cleaned.size() / 4) * 3);

    size_t i = 0;
    while (i < cleaned.size()) {
        const int a = base64_char_to_index(cleaned[i]);
        const int b = base64_char_to_index(cleaned[i + 1]);
        const int c = base64_char_to_index(cleaned[i + 2]);
        const int d = base64_char_to_index(cleaned[i + 3]);

        if (a < 0 || b < 0) {
            NEFORCE_THROW_EXCEPTION(value_exception("Invalid Base64 character in first two positions"));
        }

        bool has_padding = false;
        int padding_count = 0;
        if (cleaned[i + 2] == '=') {
            has_padding = true;
            padding_count++;
            if (cleaned[i + 3] != '=') {
                NEFORCE_THROW_EXCEPTION(value_exception("Invalid Base64 padding: single '=' at wrong position"));
            }
            padding_count++;
        } else if (cleaned[i + 3] == '=') {
            has_padding = true;
            padding_count = 1;
        }

        if (has_padding) {
            if (padding_count == 2) {
                if (c != -1 || d != -1) {
                    NEFORCE_THROW_EXCEPTION(
                            value_exception("Invalid Base64 padding: '==' expected but got other chars"));
                }
            } else if (padding_count == 1) {
                if (d != -1) {
                    NEFORCE_THROW_EXCEPTION(value_exception("Invalid Base64 padding: '=' expected at position 4"));
                }
                if (c < 0) {
                    NEFORCE_THROW_EXCEPTION(value_exception("Invalid Base64 character at position 3"));
                }
            }
        } else {
            if (c < 0 || d < 0) {
                NEFORCE_THROW_EXCEPTION(value_exception("Invalid Base64 character in last two positions"));
            }
        }

        const uint32_t val = (static_cast<uint32_t>(a) << 18) | (static_cast<uint32_t>(b) << 12) |
                             (static_cast<uint32_t>(c < 0 ? 0 : c) << 6) | static_cast<uint32_t>(d < 0 ? 0 : d);

        result.push_back(static_cast<byte_t>((val >> 16) & 0xFF));
        if (cleaned[i + 2] != '=') {
            result.push_back(static_cast<byte_t>((val >> 8) & 0xFF));
        }
        if (cleaned[i + 3] != '=') {
            result.push_back(static_cast<byte_t>(val & 0xFF));
        }

        i += 4;
    }

    return result;
}

string base64::encode_url(const cbyte_view data, bool padding) {
    string result;
    size_t i = 0;

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

    byte_vector result;
    result.reserve((cleaned.size() / 4) * 3);

    size_t i = 0;
    while (i < cleaned.size()) {
        const int a = base64url_char_to_index(cleaned[i]);
        const int b = base64url_char_to_index(cleaned[i + 1]);
        const int c = base64url_char_to_index(cleaned[i + 2]);
        const int d = base64url_char_to_index(cleaned[i + 3]);

        if (a < 0 || b < 0) {
            NEFORCE_THROW_EXCEPTION(value_exception("Invalid Base64URL character in first two positions"));
        }
        if (cleaned[i + 2] == '=' && cleaned[i + 3] != '=') {
            NEFORCE_THROW_EXCEPTION(value_exception("Invalid Base64URL padding: single '=' at wrong position"));
        }

        const uint32_t val = (static_cast<uint32_t>(a) << 18) | (static_cast<uint32_t>(b) << 12) |
                             (static_cast<uint32_t>(c < 0 ? 0 : c) << 6) | static_cast<uint32_t>(d < 0 ? 0 : d);

        result.push_back(static_cast<byte_t>((val >> 16) & 0xFF));
        if (cleaned[i + 2] != '=') {
            result.push_back(static_cast<byte_t>((val >> 8) & 0xFF));
        }
        if (cleaned[i + 3] != '=') {
            result.push_back(static_cast<byte_t>(val & 0xFF));
        }

        i += 4;
    }

    return result;
}

NEFORCE_END_NAMESPACE__
