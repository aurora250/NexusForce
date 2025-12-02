#ifndef MSTL_CORE_ENCRYPT_XOR_HPP__
#define MSTL_CORE_ENCRYPT_XOR_HPP__
#include "../string/string.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API XOR {
public:
  static bstring encrypt(bstring_view data, bstring_view key);
  static bstring decrypt(bstring_view data, bstring_view key) { return encrypt(data, key); }
};


MSTL_ALWAYS_INLINE_INLINE bstring xor_encrypt(const bstring& data, const bstring& key) {
  return XOR::encrypt(data.view(), key.view());
}
MSTL_ALWAYS_INLINE_INLINE string xor_encrypt(const string& data, const string& key) {
  return to_string(xor_encrypt(to_bstring(data), to_bstring(key)));
}
MSTL_ALWAYS_INLINE_INLINE bstring xor_decrypt(const bstring& data, const bstring& key) {
  return XOR::decrypt(data.view(), key.view());
}
MSTL_ALWAYS_INLINE_INLINE string xor_decrypt(const string& data, const string& key) {
  return to_string(xor_decrypt(to_bstring(data), to_bstring(key)));
}

MSTL_ALWAYS_INLINE_INLINE bstring xor_encrypt(const bstring_view data, const bstring_view key) {
  return XOR::encrypt(data, key);
}
MSTL_ALWAYS_INLINE_INLINE bstring xor_decrypt(const bstring_view data, const bstring_view key) {
  return XOR::decrypt(data, key);
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ENCRYPT_XOR_HPP__
