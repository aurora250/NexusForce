#ifndef MSTL_CORE_ENCRYPT_SHA256_HPP__
#define MSTL_CORE_ENCRYPT_SHA256_HPP__
#include "../string/string.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API SHA256 {
private:
    static constexpr uint32_t K[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    static constexpr uint32_t rotr(const uint32_t x, const uint32_t n) { return (x >> n) | (x << (32 - n)); }

    static constexpr uint32_t ch(const uint32_t x, const uint32_t y, const uint32_t z) { return (x & y) ^ (~x & z); }
    static constexpr uint32_t maj(const uint32_t x, const uint32_t y, const uint32_t z) { return (x & y) ^ (x & z) ^ (y & z); }

    static constexpr uint32_t sig0(const uint32_t x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
    static constexpr uint32_t sig1(const uint32_t x)  { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }

    static constexpr uint32_t gamma0(const uint32_t x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
    static constexpr uint32_t gamma1(const uint32_t x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

public:
    static bstring hash(bstring_view data);
    static string hash_hex(bstring_view data);
};


MSTL_ALWAYS_INLINE_INLINE string sha256(const bstring& data) {
    return SHA256::hash_hex(data.view());
}
MSTL_ALWAYS_INLINE_INLINE string sha256(const string& data) {
    return sha256(to_bstring(data));
}

MSTL_ALWAYS_INLINE_INLINE string sha256(const bstring_view data) {
    return SHA256::hash_hex(data);
}
MSTL_ALWAYS_INLINE_INLINE string sha256(const string_view data) {
    return sha256(to_bstring(data));
}

MSTL_ALWAYS_INLINE_INLINE string sha256(const char* data) {
    return sha256(string_view{data});
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ENCRYPT_SHA256_HPP__
