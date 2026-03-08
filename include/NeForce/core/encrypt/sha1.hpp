#ifndef NEFORCE_CORE_ENCRYPT_SHA1_HPP__
#define NEFORCE_CORE_ENCRYPT_SHA1_HPP__
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

struct NEFORCE_API SHA1 {
    static byte_vector hash(cbyte_view data);
    static string hash_hex(cbyte_view data);
};


NEFORCE_ALWAYS_INLINE_INLINE string sha1(const string_view data) {
    const byte_vector h = SHA1::hash({reinterpret_cast<const byte_t*>(data.data()), data.size()});
    return string{reinterpret_cast<const char*>(h.data()), h.size()};
}

NEFORCE_ALWAYS_INLINE_INLINE string sha1(const string& data) {
    return _NEFORCE sha1(data.view());
}

NEFORCE_ALWAYS_INLINE_INLINE byte_vector sha1(const cbyte_view data) {
    return SHA1::hash(data);
}

NEFORCE_ALWAYS_INLINE_INLINE byte_vector sha1(const byte_vector& data) {
    return SHA1::hash(data.view());
}

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ENCRYPT_SHA1_HPP__
