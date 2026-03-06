#ifndef NEFORCE_CORE_ENCRYPT_MD5_HPP__
#define NEFORCE_CORE_ENCRYPT_MD5_HPP__
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/memory/memory_view.hpp"
#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

struct NEFORCE_API MD5 {
    static byte_vector hash(cbyte_view data);
    static string hash_hex(cbyte_view data);
};


NEFORCE_ALWAYS_INLINE_INLINE string md5(const string_view data) {
    return MD5::hash_hex({reinterpret_cast<const byte_t*>(data.data()), data.size()});
}

NEFORCE_ALWAYS_INLINE_INLINE string md5(const string& data) {
    return MD5::hash_hex({reinterpret_cast<const byte_t*>(data.data()), data.size()});
}

NEFORCE_ALWAYS_INLINE_INLINE byte_vector md5(const cbyte_view data) {
    return MD5::hash(data);
}

NEFORCE_ALWAYS_INLINE_INLINE byte_vector md5(const byte_vector& data) {
    return MD5::hash(data.view());
}

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ENCRYPT_MD5_HPP__
