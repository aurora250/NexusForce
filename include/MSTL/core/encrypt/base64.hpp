#ifndef MSTL_CORE_ENCRYPT_BASE64_HPP__
#define MSTL_CORE_ENCRYPT_BASE64_HPP__
#include "../string/string.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API base64 {
private:
    static constexpr auto chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    static constexpr int char_to_index(const char c) {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '+') return 62;
        if (c == '/') return 63;
        return -1;
    }

public:
    static string encode(bstring_view data);
    static bstring decode(string_view data);
};


MSTL_ALWAYS_INLINE_INLINE string base64_encode(const bstring& data) {
    return base64::encode(data.view());
}
MSTL_ALWAYS_INLINE_INLINE string base64_encode(const string& data) {
    return base64_encode(to_bstring(data));
}
MSTL_ALWAYS_INLINE_INLINE string base64_decode(const string& data) {
    return to_string(base64::decode(data.view()));
}

MSTL_ALWAYS_INLINE_INLINE string base64_encode(const bstring_view data) {
    return base64::encode(data);
}
MSTL_ALWAYS_INLINE_INLINE string base64_encode(const string_view data) {
    return base64_encode(to_bstring(data));
}
MSTL_ALWAYS_INLINE_INLINE string base64_decode(const string_view data) {
    return to_string(base64::decode(data));
}

MSTL_ALWAYS_INLINE_INLINE string base64_encode(const char* data) {
    return base64_encode(to_bstring(string_view{data}));
}
MSTL_ALWAYS_INLINE_INLINE string base64_decode(const char* data) {
    return to_string(base64::decode(string_view{data}));
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ENCRYPT_BASE64_HPP__
