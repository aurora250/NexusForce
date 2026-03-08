#ifndef NEFORCE_CORE_ENCRYPT_XOR_HPP__
#define NEFORCE_CORE_ENCRYPT_XOR_HPP__
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

struct NEFORCE_API XOR {
    static byte_vector encrypt(cbyte_view data, cbyte_view key);

    static byte_vector decrypt(cbyte_view data, cbyte_view key) {
        return encrypt(data, key);
    }
};


NEFORCE_ALWAYS_INLINE_INLINE byte_vector XOR_encrypt(const cbyte_view data, const cbyte_view key) {
    return XOR::encrypt(data, key);
}

NEFORCE_ALWAYS_INLINE_INLINE byte_vector XOR_encrypt(const byte_vector& data, const byte_vector& key) {
    return XOR::encrypt(data.view(), key.view());
}

NEFORCE_ALWAYS_INLINE_INLINE string XOR_encrypt(const string& data, const string& key) {
    const byte_vector e = XOR_encrypt(
        cbyte_view{reinterpret_cast<const byte_t*>(data.data()), data.size()},
        cbyte_view{reinterpret_cast<const byte_t*>(key.data()), key.size()}
    );
    return string(e.begin(), e.end());
}


NEFORCE_ALWAYS_INLINE_INLINE byte_vector XOR_decrypt(const cbyte_view data, const cbyte_view key) {
    return XOR::decrypt(data, key);
}

NEFORCE_ALWAYS_INLINE_INLINE byte_vector XOR_decrypt(const byte_vector& data, const byte_vector& key) {
    return XOR::decrypt(data.view(), key.view());
}

NEFORCE_ALWAYS_INLINE_INLINE string XOR_decrypt(const string& data, const string& key) {
    const byte_vector d = XOR_decrypt(
        cbyte_view{reinterpret_cast<const byte_t*>(data.data()), data.size()},
        cbyte_view{reinterpret_cast<const byte_t*>(key.data()), key.size()}
    );
    return string(d.begin(), d.end());
}

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ENCRYPT_XOR_HPP__
