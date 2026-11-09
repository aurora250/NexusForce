#include <NeForce/core/encoding/base32.hpp>
#include <NeForce/core/exception/exception.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    constexpr string_view BASE32_ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    constexpr string_view BASE32HEX_ALPHABET = "0123456789ABCDEFGHIJKLMNOPQRSTUV";

    constexpr int base32_char_to_index(const char c, const bool hex) noexcept {
        if (hex) {
            if (c >= '0' && c <= '9') {
                return c - '0';
            }
            if (c >= 'A' && c <= 'V') {
                return c - 'A' + 10;
            }
            if (c >= 'a' && c <= 'v') {
                return c - 'a' + 10;
            }
            return -1;
        }
        if (c >= 'A' && c <= 'Z') {
            return c - 'A';
        }
        if (c >= 'a' && c <= 'z') {
            return c - 'a';
        }
        if (c >= '2' && c <= '7') {
            return c - '2' + 26;
        }
        return -1;
    }

    string base32_encode_impl(const cbyte_view data, const string_view alphabet) {
        if (data.empty()) {
            return {};
        }

        string result;
        result.reserve(((data.size() + 4) / 5) * 8);

        size_t i = 0;
        while (i + 5 <= data.size()) {
            uint64_t value = 0;
            for (size_t j = 0; j < 5; ++j) {
                value = (value << 8) | data[i + j];
            }
            for (size_t j = 0; j < 8; ++j) {
                result.push_back(alphabet[(value >> (35 - j * 5)) & 0x1F]);
            }
            i += 5;
        }

        const size_t remaining = data.size() - i;
        if (remaining != 0) {
            uint64_t value = 0;
            for (size_t j = 0; j < remaining; ++j) {
                value = (value << 8) | data[i + j];
            }
            value <<= (5 - remaining) * 8;

            const size_t char_count = (remaining * 8 + 4) / 5;
            for (size_t j = 0; j < char_count; ++j) {
                result.push_back(alphabet[(value >> (35 - j * 5)) & 0x1F]);
            }
            while ((result.size() % 8) != 0) {
                result.push_back('=');
            }
        }
        return result;
    }

    byte_vector base32_decode_impl(const string_view data, const bool hex) {
        if (data.empty()) {
            return {};
        }

        string cleaned;
        cleaned.reserve(data.size());
        for (const char c: data) {
            if (!is_space(c)) {
                cleaned.push_back(c);
            }
        }
        if (cleaned.empty()) {
            return {};
        }

        size_t core_size = cleaned.size();
        while (core_size > 0 && cleaned[core_size - 1] == '=') {
            --core_size;
        }
        for (size_t i = 0; i < core_size; ++i) {
            if (cleaned[i] == '=') {
                NEFORCE_THROW_EXCEPTION(value_exception("Invalid Base32 padding"));
            }
        }

        const size_t padding = cleaned.size() - core_size;
        const size_t remaining = core_size % 8;
        if (padding != 0) {
            if ((cleaned.size() % 8) != 0 || (padding != 1 && padding != 3 && padding != 4 && padding != 6)) {
                NEFORCE_THROW_EXCEPTION(value_exception("Invalid Base32 padding"));
            }
            if (remaining != 8 - padding) {
                NEFORCE_THROW_EXCEPTION(value_exception("Invalid Base32 padding"));
            }
        } else if (remaining != 0 && remaining != 2 && remaining != 4 && remaining != 5 && remaining != 7) {
            NEFORCE_THROW_EXCEPTION(value_exception("Invalid Base32 length"));
        }

        byte_vector result;
        result.reserve((core_size * 5) / 8);

        size_t i = 0;
        while (i + 8 <= core_size) {
            uint64_t value = 0;
            for (size_t j = 0; j < 8; ++j) {
                const int index = base32_char_to_index(cleaned[i + j], hex);
                if (index < 0) {
                    NEFORCE_THROW_EXCEPTION(value_exception("Invalid Base32 character"));
                }
                value = (value << 5) | static_cast<uint64_t>(index);
            }
            for (size_t j = 0; j < 5; ++j) {
                result.push_back(static_cast<byte_t>((value >> (32 - j * 8)) & 0xFF));
            }
            i += 8;
        }

        if (i < core_size) {
            const size_t count = core_size - i;
            uint64_t value = 0;
            for (size_t j = 0; j < count; ++j) {
                const int index = base32_char_to_index(cleaned[i + j], hex);
                if (index < 0) {
                    NEFORCE_THROW_EXCEPTION(value_exception("Invalid Base32 character"));
                }
                value = (value << 5) | static_cast<uint64_t>(index);
            }
            value <<= (8 - count) * 5;

            const size_t byte_count = (count * 5) / 8;
            for (size_t j = 0; j < byte_count; ++j) {
                result.push_back(static_cast<byte_t>((value >> (32 - j * 8)) & 0xFF));
            }
        }

        return result;
    }
} // namespace


string base32::encode(const cbyte_view data) { return base32_encode_impl(data, BASE32_ALPHABET); }

byte_vector base32::decode(const string_view data) { return base32_decode_impl(data, false); }

string base32::encode_hex(const cbyte_view data) { return base32_encode_impl(data, BASE32HEX_ALPHABET); }

byte_vector base32::decode_hex(const string_view data) { return base32_decode_impl(data, true); }

NEFORCE_END_NAMESPACE__
