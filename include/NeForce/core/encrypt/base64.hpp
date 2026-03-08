#ifndef NEFORCE_CORE_ENCRYPT_BASE64_HPP__
#define NEFORCE_CORE_ENCRYPT_BASE64_HPP__
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

struct NEFORCE_API base64 {
    static string encode(cbyte_view data);
    static byte_vector decode(string_view data);
};


NEFORCE_ALWAYS_INLINE_INLINE string base64_encode(const cbyte_view data) {
    return base64::encode(data);
}

NEFORCE_ALWAYS_INLINE_INLINE string base64_encode(const byte_vector& data) {
    return base64::encode(data.view());
}

NEFORCE_ALWAYS_INLINE_INLINE string base64_encode(const string& data) {
    return base64::encode(cbyte_view{
        reinterpret_cast<const byte_t*>(data.data()), data.size()
    });
}


NEFORCE_ALWAYS_INLINE_INLINE string base64_decode(const string_view data) {
    const byte_vector d = base64::decode(data);
    return string{reinterpret_cast<const char*>(d.data()), d.size()};
}

NEFORCE_ALWAYS_INLINE_INLINE byte_vector base64_decode(const byte_vector& data) {
    const string tmp{reinterpret_cast<const char*>(data.data()), data.size()};
    return base64::decode(tmp.view());
}

NEFORCE_ALWAYS_INLINE_INLINE string base64_decode(const string& data) {
    return base64_decode(data.view());
}

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ENCRYPT_BASE64_HPP__
