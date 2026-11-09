#include <NeForce/core/kdf/hkdf.hpp>
#include <NeForce/core/hash/sha256.hpp>
#include <NeForce/core/hash/sha512.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    template <typename Hash>
    struct hmac_traits;

    template <>
    struct hmac_traits<SHA256> {
        static constexpr size_t block_size = 64;
        static constexpr size_t digest_size = 32;
    };

    template <>
    struct hmac_traits<SHA512> {
        static constexpr size_t block_size = 128;
        static constexpr size_t digest_size = 64;
    };

    template <typename Hash>
    byte_vector hmac_hash(const cbyte_view key, const cbyte_view data) {
        constexpr size_t block_size = hmac_traits<Hash>::block_size;

        byte_vector normalized;
        if (key.size() > block_size) {
            normalized = Hash::hash(key);
        } else {
            normalized.reserve(key.size());
            for (size_t i = 0; i < key.size(); ++i) {
                normalized.push_back(key[i]);
            }
        }

        byte_vector inner;
        inner.reserve(block_size + data.size());
        for (size_t i = 0; i < block_size; ++i) {
            const byte_t k = i < normalized.size() ? normalized[i] : static_cast<byte_t>(0);
            inner.push_back(static_cast<byte_t>(k ^ 0x36));
        }
        for (size_t i = 0; i < data.size(); ++i) {
            inner.push_back(data[i]);
        }
        const byte_vector inner_digest = Hash::hash(inner.view());

        byte_vector outer;
        outer.reserve(block_size + inner_digest.size());
        for (size_t i = 0; i < block_size; ++i) {
            const byte_t k = i < normalized.size() ? normalized[i] : static_cast<byte_t>(0);
            outer.push_back(static_cast<byte_t>(k ^ 0x5c));
        }
        for (const byte_t byte: inner_digest) {
            outer.push_back(byte);
        }
        return Hash::hash(outer.view());
    }

    template <typename Hash>
    byte_vector hkdf_extract(const cbyte_view salt, const cbyte_view ikm) {
        constexpr size_t digest_size = hmac_traits<Hash>::digest_size;
        if (salt.empty()) {
            byte_vector zero_salt;
            zero_salt.reserve(digest_size);
            for (size_t i = 0; i < digest_size; ++i) {
                zero_salt.push_back(0);
            }
            return hmac_hash<Hash>(zero_salt.view(), ikm);
        }
        return hmac_hash<Hash>(salt, ikm);
    }

    template <typename Hash>
    byte_vector hkdf_expand(const cbyte_view prk, const cbyte_view info, const size_t length) {
        constexpr size_t digest_size = hmac_traits<Hash>::digest_size;
        if (length == 0) {
            return {};
        }
        if (length > 255 * digest_size) {
            NEFORCE_THROW_EXCEPTION(value_exception("HKDF output length exceeds 255 * HashLen"));
        }

        byte_vector result;
        result.reserve(length);
        byte_vector previous;
        uint32_t counter = 1;

        while (result.size() < length) {
            byte_vector input;
            input.reserve(previous.size() + info.size() + 1);
            for (const byte_t byte: previous) {
                input.push_back(byte);
            }
            for (size_t i = 0; i < info.size(); ++i) {
                input.push_back(info[i]);
            }
            input.push_back(static_cast<byte_t>(counter));

            previous = hmac_hash<Hash>(prk, input.view());
            for (const byte_t byte: previous) {
                if (result.size() >= length) {
                    break;
                }
                result.push_back(byte);
            }
            ++counter;
        }
        return result;
    }

    template <typename Hash>
    byte_vector hkdf_derive(const cbyte_view ikm, const cbyte_view salt, const cbyte_view info, const size_t length) {
        const byte_vector prk = hkdf_extract<Hash>(salt, ikm);
        return hkdf_expand<Hash>(prk.view(), info, length);
    }
} // namespace


byte_vector HKDF::extract(const cbyte_view salt, const cbyte_view ikm) { return hkdf_extract<SHA256>(salt, ikm); }

byte_vector HKDF::expand(const cbyte_view prk, const cbyte_view info, const size_t length) {
    return hkdf_expand<SHA256>(prk, info, length);
}

byte_vector HKDF::derive(const cbyte_view ikm, const cbyte_view salt, const cbyte_view info, const size_t length) {
    return hkdf_derive<SHA256>(ikm, salt, info, length);
}

byte_vector HKDF_SHA512::extract(const cbyte_view salt, const cbyte_view ikm) {
    return hkdf_extract<SHA512>(salt, ikm);
}

byte_vector HKDF_SHA512::expand(const cbyte_view prk, const cbyte_view info, const size_t length) {
    return hkdf_expand<SHA512>(prk, info, length);
}

byte_vector HKDF_SHA512::derive(const cbyte_view ikm, const cbyte_view salt, const cbyte_view info,
                                const size_t length) {
    return hkdf_derive<SHA512>(ikm, salt, info, length);
}

NEFORCE_END_NAMESPACE__
