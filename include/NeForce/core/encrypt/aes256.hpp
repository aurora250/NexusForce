#ifndef NEFORCE_CORE_ENCRYPT_AES256_HPP__
#define NEFORCE_CORE_ENCRYPT_AES256_HPP__
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

struct NEFORCE_API AES256 {
    static byte_vector encrypt(cbyte_view data, cbyte_view key);
    static byte_vector decrypt(cbyte_view data, cbyte_view key);

    static byte_vector encrypt_pkcs7(cbyte_view data, cbyte_view key);
    static byte_vector decrypt_pkcs7(cbyte_view data, cbyte_view key);

    static string encrypt_hex(string_view data, string_view key_hex);
    static string decrypt_hex(string_view encrypted_hex, string_view key_hex);
};


NEFORCE_ALWAYS_INLINE_INLINE string aes256_encrypt(const string_view data, const string_view key_hex) {
    return AES256::encrypt_hex(data, key_hex);
}

NEFORCE_ALWAYS_INLINE_INLINE string aes256_encrypt(const string& data, const string_view key_hex) {
    return AES256::encrypt_hex(data.view(), key_hex);
}

NEFORCE_ALWAYS_INLINE_INLINE string aes256_encrypt(const string_view data, const string& key_hex) {
    return AES256::encrypt_hex(data, key_hex.view());
}

NEFORCE_ALWAYS_INLINE_INLINE string aes256_encrypt(const string& data, const string& key_hex) {
    return AES256::encrypt_hex(data.view(), key_hex.view());
}


NEFORCE_ALWAYS_INLINE_INLINE string aes256_decrypt(const string_view encrypted_hex, const string_view key_hex) {
    return AES256::decrypt_hex(encrypted_hex, key_hex);
}

NEFORCE_ALWAYS_INLINE_INLINE string aes256_decrypt(const string& encrypted_hex, const string_view key_hex) {
    return AES256::decrypt_hex(encrypted_hex.view(), key_hex);
}

NEFORCE_ALWAYS_INLINE_INLINE string aes256_decrypt(const string_view encrypted_hex, const string& key_hex) {
    return AES256::decrypt_hex(encrypted_hex, key_hex.view());
}

NEFORCE_ALWAYS_INLINE_INLINE string aes256_decrypt(const string& encrypted_hex, const string& key_hex) {
    return AES256::decrypt_hex(encrypted_hex.view(), key_hex.view());
}

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ENCRYPT_AES256_HPP__
