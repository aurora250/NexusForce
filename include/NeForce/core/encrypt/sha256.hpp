#ifndef NEFORCE_CORE_ENCRYPT_SHA256_HPP__
#define NEFORCE_CORE_ENCRYPT_SHA256_HPP__
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

struct NEFORCE_API SHA256 {
    static byte_vector hash(cbyte_view data);
    static string hash_hex(cbyte_view data);
};


NEFORCE_ALWAYS_INLINE_INLINE string sha256(const string_view data) {
    return SHA256::hash_hex({reinterpret_cast<const byte_t*>(data.data()), data.size()});
}

NEFORCE_ALWAYS_INLINE_INLINE string sha256(const string& data) {
    return SHA256::hash_hex({reinterpret_cast<const byte_t*>(data.data()), data.size()});
}

NEFORCE_ALWAYS_INLINE_INLINE byte_vector sha256(const cbyte_view data) {
    return SHA256::hash(data);
}

NEFORCE_ALWAYS_INLINE_INLINE byte_vector sha256(const byte_vector& data) {
    return SHA256::hash(data.view());
}

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ENCRYPT_SHA256_HPP__
