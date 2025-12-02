#ifndef MSTL_CORE_ENCRYPT_SHA1_HPP__
#define MSTL_CORE_ENCRYPT_SHA1_HPP__
#include "../string/string.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API SHA1 {
private:
    static constexpr uint32_t rotleft(const uint32_t x, const uint32_t c)  { return (x << c) | (x >> (32 - c)); }

public:
    static bstring hash(bstring_view data);
    static string hash_hex(bstring_view data);
};


MSTL_ALWAYS_INLINE_INLINE string sha1(const bstring& data) {
    return SHA1::hash_hex(data.view());
}
MSTL_ALWAYS_INLINE_INLINE string sha1(const string& data) {
    return sha1(to_bstring(data));
}

MSTL_ALWAYS_INLINE_INLINE string sha1(const bstring_view data) {
    return SHA1::hash_hex(data);
}
MSTL_ALWAYS_INLINE_INLINE string sha1(const string_view data) {
    return sha1(to_bstring(data));
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ENCRYPT_SHA1_HPP__
