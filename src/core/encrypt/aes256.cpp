#include <NeForce/core/encrypt/aes256.hpp>
#include <NeForce/core/memory/endian.hpp>
#include <NeForce/core/simd/types.hpp>
#include <NeForce/core/utility/hexadecimal.hpp>
#if defined(NEFORCE_SIMD_AES_NI) || defined(NEFORCE_SIMD_PCLMUL)
#    include <wmmintrin.h>
#endif
#ifdef __NEFORCE_TARGET_APS
#    undef __NEFORCE_TARGET_APS
#endif
#if defined(NEFORCE_SIMD_AES_NI) && defined(NEFORCE_SIMD_PCLMUL) && defined(NEFORCE_SIMD_SSE2)
#    define __NEFORCE_TARGET_APS NEFORCE_TARGET("aes,pclmul,sse2")
#else
#    define __NEFORCE_TARGET_APS
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
    byte_t AES256_gf_mult(byte_t a, byte_t b) {
        byte_t result = 0;
        for (int i = 0; i < 8; ++i) {
            result ^= a & static_cast<byte_t>(-static_cast<int>(b & 1));
            const auto hi_bit = static_cast<byte_t>(a & 0x80);
            a = static_cast<byte_t>(a << 1);
            a ^= static_cast<byte_t>(0x1b & -static_cast<int>(hi_bit >> 7));
            b >>= 1;
        }
        return result;
    }

    byte_t AES256_gf_square(const byte_t x) { return AES256_gf_mult(x, x); }

    byte_t AES256_gf_inv(const byte_t x) {
        // x^254 = x^(2+4+8+16+32+64+128), Itoh-Tsujii chain (12 GF ops)
        const byte_t t2 = AES256_gf_square(x);
        const byte_t t3 = AES256_gf_mult(t2, x);
        const byte_t t6 = AES256_gf_square(t3);
        const byte_t t7 = AES256_gf_mult(t6, x);
        const byte_t t12 = AES256_gf_square(t6);
        const byte_t t14 = AES256_gf_square(t7);
        const byte_t t15 = AES256_gf_mult(t12, t3);
        const byte_t t30 = AES256_gf_square(t15);
        const byte_t t60 = AES256_gf_square(t30);
        const byte_t t120 = AES256_gf_square(t60);
        const byte_t t240 = AES256_gf_square(t120);
        return AES256_gf_mult(t240, t14);
    }

    byte_t AES256_sbox_ct(const byte_t x) {
        // S(x) = affine(gf_inv(x)): y = x ^ rotl1 ^ rotl2 ^ rotl3 ^ rotl4 ^ 0x63
        const byte_t inv = AES256_gf_inv(x);
        byte_t y = inv;
        for (int i = 1; i <= 4; ++i) {
            y ^= static_cast<byte_t>((inv << i) | (inv >> (8 - i)));
        }
        return static_cast<byte_t>(y ^ 0x63);
    }

    byte_t AES256_inv_sbox_ct(byte_t x) {
        // inv S(x) = gf_inv(rotl1(x) ^ rotl3(x) ^ rotl6(x) ^ 0x05)
        const auto r1 = static_cast<byte_t>((x << 1) | (x >> 7));
        const auto r3 = static_cast<byte_t>((x << 3) | (x >> 5));
        const auto r6 = static_cast<byte_t>((x << 6) | (x >> 2));
        return AES256_gf_inv(static_cast<byte_t>(r1 ^ r3 ^ r6 ^ 0x05));
    }

    constexpr byte_t AES256_rcon[15] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
                                        0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a};

    void AES256_sub_bytes(byte_t state[16]) {
        for (int i = 0; i < 16; ++i) {
            state[i] = AES256_sbox_ct(state[i]);
        }
    }

    void AES256_inv_sub_bytes(byte_t state[16]) {
        for (int i = 0; i < 16; ++i) {
            state[i] = AES256_inv_sbox_ct(state[i]);
        }
    }

    void AES256_add_round_key(byte_t state[16], const byte_t* round_key) {
        for (int i = 0; i < 16; ++i) {
            state[i] ^= round_key[i];
        }
    }

    void AES256_key_expansion(const byte_t* key, byte_t* expanded_key) {
        memory_copy(expanded_key, key, 32);
        for (int i = 8; i < 60; ++i) {
            byte_t temp[4];
            memory_copy(temp, expanded_key + static_cast<ptrdiff_t>((i - 1) * 4), 4);

            if (i % 8 == 0) {
                const byte_t t = temp[0];
                temp[0] = temp[1];
                temp[1] = temp[2];
                temp[2] = temp[3];
                temp[3] = t;
                for (byte_t& j: temp) {
                    j = AES256_sbox_ct(j);
                }
                temp[0] ^= AES256_rcon[(i / 8) - 1];
            } else if (i % 8 == 4) {
                for (byte_t& j: temp) {
                    j = AES256_sbox_ct(j);
                }
            }

            for (int j = 0; j < 4; ++j) {
                expanded_key[(i * 4) + j] = expanded_key[((i - 8) * 4) + j] ^ temp[j];
            }
        }
    }

    __NEFORCE_TARGET_APS void AES256_inv_key_expansion(const byte_t* key, byte_t* inv_expanded_key) {
        AES256_key_expansion(key, inv_expanded_key);
#if defined(NEFORCE_SIMD_AES_NI)
        // precompute InvMixColumns-transformed round keys so decrypt_block
        // does not apply aesimc per block per round
        for (int round = 1; round < 14; ++round) {
            simd::vec128_t rk = ::_mm_loadu_si128(
                    reinterpret_cast<const simd::vec128_t*>(inv_expanded_key + static_cast<ptrdiff_t>(round * 16)));
            rk = ::_mm_aesimc_si128(rk);
            ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(inv_expanded_key + static_cast<ptrdiff_t>(round * 16)),
                               rk);
        }
#endif
    }

    void AES256_shift_rows(byte_t state[16]) {
        byte_t temp = state[1];
        state[1] = state[5];
        state[5] = state[9];
        state[9] = state[13];
        state[13] = temp;

        temp = state[2];
        state[2] = state[10];
        state[10] = temp;
        temp = state[6];
        state[6] = state[14];
        state[14] = temp;

        temp = state[3];
        state[3] = state[15];
        state[15] = state[11];
        state[11] = state[7];
        state[7] = temp;
    }

    void AES256_inv_shift_rows(byte_t state[16]) {
        byte_t temp = state[13];
        state[13] = state[9];
        state[9] = state[5];
        state[5] = state[1];
        state[1] = temp;

        temp = state[2];
        state[2] = state[10];
        state[10] = temp;
        temp = state[6];
        state[6] = state[14];
        state[14] = temp;

        temp = state[7];
        state[7] = state[11];
        state[11] = state[15];
        state[15] = state[3];
        state[3] = temp;
    }

    void AES256_mix_columns(byte_t state[16]) {
        for (ptrdiff_t c = 0; c < 4; ++c) {
            const byte_t s0 = state[c * 4];
            const byte_t s1 = state[c * 4 + 1];
            const byte_t s2 = state[c * 4 + 2];
            const byte_t s3 = state[c * 4 + 3];

            state[c * 4] = AES256_gf_mult(0x02, s0) ^ AES256_gf_mult(0x03, s1) ^ s2 ^ s3;
            state[c * 4 + 1] = s0 ^ AES256_gf_mult(0x02, s1) ^ AES256_gf_mult(0x03, s2) ^ s3;
            state[c * 4 + 2] = s0 ^ s1 ^ AES256_gf_mult(0x02, s2) ^ AES256_gf_mult(0x03, s3);
            state[c * 4 + 3] = AES256_gf_mult(0x03, s0) ^ s1 ^ s2 ^ AES256_gf_mult(0x02, s3);
        }
    }

    void AES256_inv_mix_columns(byte_t state[16]) {
        for (ptrdiff_t c = 0; c < 4; ++c) {
            const byte_t s0 = state[c * 4];
            const byte_t s1 = state[c * 4 + 1];
            const byte_t s2 = state[c * 4 + 2];
            const byte_t s3 = state[c * 4 + 3];

            state[c * 4] = AES256_gf_mult(0x0e, s0) ^ AES256_gf_mult(0x0b, s1) ^ AES256_gf_mult(0x0d, s2) ^
                           AES256_gf_mult(0x09, s3);
            state[c * 4 + 1] = AES256_gf_mult(0x09, s0) ^ AES256_gf_mult(0x0e, s1) ^ AES256_gf_mult(0x0b, s2) ^
                               AES256_gf_mult(0x0d, s3);
            state[c * 4 + 2] = AES256_gf_mult(0x0d, s0) ^ AES256_gf_mult(0x09, s1) ^ AES256_gf_mult(0x0e, s2) ^
                               AES256_gf_mult(0x0b, s3);
            state[c * 4 + 3] = AES256_gf_mult(0x0b, s0) ^ AES256_gf_mult(0x0d, s1) ^ AES256_gf_mult(0x09, s2) ^
                               AES256_gf_mult(0x0e, s3);
        }
    }

    __NEFORCE_TARGET_APS
    void encrypt_block(byte_t dst[16], const byte_t* src, const byte_t* expanded_key) noexcept {
#if defined(NEFORCE_SIMD_AES_NI)
        simd::vec128_t state = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(src));

        state = ::_mm_xor_si128(state, ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(expanded_key)));

        for (int round = 1; round < 14; ++round) {
            state = _mm_aesenc_si128(state, ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(
                                                    expanded_key + static_cast<ptrdiff_t>(round * 16))));
        }

        state = ::_mm_aesenclast_si128(state, ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(
                                                      expanded_key + static_cast<ptrdiff_t>(14 * 16))));

        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(dst), state);

#elif defined(NEFORCE_SIMD_AES_ARM)
        // AESE adds the round key before the nonlinear SubBytes, unlike the
        // standard AES round (AddRoundKey after MixColumns), so the round
        // keys are shifted one position: full rounds use keys 0..12, the
        // final AESE uses key 13 and key 14 is XORed separately.
        simd::vec128_t state = ::vld1q_u8(src);

        for (int round = 0; round < 13; ++round) {
            state = ::vaeseq_u8(state, ::vld1q_u8(reinterpret_cast<const uint8_t*>(
                                               expanded_key + static_cast<ptrdiff_t>(round * 16))));
            state = ::vaesmcq_u8(state);
        }

        state = ::vaeseq_u8(
                state, ::vld1q_u8(reinterpret_cast<const uint8_t*>(expanded_key + static_cast<ptrdiff_t>(13 * 16))));
        state = ::veorq_u8(
                state, ::vld1q_u8(reinterpret_cast<const uint8_t*>(expanded_key + static_cast<ptrdiff_t>(14 * 16))));

        ::vst1q_u8(dst, state);

#else
        byte_t block[16];
        memory_copy(block, src, 16);
        AES256_add_round_key(block, expanded_key);

        for (int round = 1; round < 14; ++round) {
            AES256_sub_bytes(block);
            AES256_shift_rows(block);
            AES256_mix_columns(block);
            AES256_add_round_key(block, expanded_key + static_cast<ptrdiff_t>(round * 16));
        }

        AES256_sub_bytes(block);
        AES256_shift_rows(block);
        AES256_add_round_key(block, expanded_key + static_cast<ptrdiff_t>(14 * 16));
        memory_copy(dst, block, 16);
#endif
    }

    __NEFORCE_TARGET_APS
    void decrypt_block(byte_t dst[16], const byte_t* src, const byte_t* inv_expanded_key) noexcept {
#if defined(NEFORCE_SIMD_AES_NI)
        simd::vec128_t state = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(src));

        state = ::_mm_xor_si128(state, ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(
                                               inv_expanded_key + static_cast<ptrdiff_t>(14 * 16))));

        // round keys 1..13 are already InvMixColumns-transformed (AES256_inv_key_expansion)
        for (int round = 13; round >= 1; --round) {
            state = ::_mm_aesdec_si128(state, ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(
                                                      inv_expanded_key + static_cast<ptrdiff_t>(round * 16))));
        }

        state = ::_mm_aesdeclast_si128(state,
                                       ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(inv_expanded_key)));

        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(dst), state);

#elif defined(NEFORCE_SIMD_AES_ARM)
        // AESD adds the round key before the nonlinear InvSubBytes while the
        // standard inverse round adds it before InvMixColumns, so each full
        // round key is InvMixColumns-transformed (AESIMC) on the fly and the
        // keys are shifted one position: the first round uses raw key 14,
        // full rounds use keys 13..2 and key 0 is XORed separately.
        simd::vec128_t state = ::vld1q_u8(src);

        state = ::vaesdq_u8(state, ::vld1q_u8(reinterpret_cast<const uint8_t*>(inv_expanded_key +
                                                                               static_cast<ptrdiff_t>(14 * 16))));
        state = ::vaesimcq_u8(state);

        for (int round = 13; round >= 2; --round) {
            state = ::vaesdq_u8(state, ::vaesimcq_u8(::vld1q_u8(reinterpret_cast<const uint8_t*>(
                                               inv_expanded_key + static_cast<ptrdiff_t>(round * 16)))));
            state = ::vaesimcq_u8(state);
        }

        state = ::vaesdq_u8(state, ::vaesimcq_u8(::vld1q_u8(reinterpret_cast<const uint8_t*>(
                                           inv_expanded_key + static_cast<ptrdiff_t>(16)))));
        state = ::veorq_u8(state, ::vld1q_u8(reinterpret_cast<const uint8_t*>(inv_expanded_key)));

        ::vst1q_u8(dst, state);

#else
        byte_t block[16];
        memory_copy(block, src, 16);
        AES256_add_round_key(block, inv_expanded_key + static_cast<ptrdiff_t>(14 * 16));

        for (int round = 13; round >= 1; --round) {
            AES256_inv_shift_rows(block);
            AES256_inv_sub_bytes(block);
            AES256_add_round_key(block, inv_expanded_key + static_cast<ptrdiff_t>(round * 16));
            AES256_inv_mix_columns(block);
        }

        AES256_inv_shift_rows(block);
        AES256_inv_sub_bytes(block);
        AES256_add_round_key(block, inv_expanded_key);
        memory_copy(dst, block, 16);
#endif
    }

#ifdef NEFORCE_SIMD_AES_NI
    __NEFORCE_TARGET_APS
    void encrypt_blocks4(byte_t* dst, const byte_t* src, const byte_t* expanded_key) noexcept {
        // Four independent AES chains in flight hide the ~4 cycle aesenc latency.
        simd::vec128_t b0 = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(src));
        simd::vec128_t b1 = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(src + 16));
        simd::vec128_t b2 = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(src + 32));
        simd::vec128_t b3 = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(src + 48));

        const simd::vec128_t rk0 = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(expanded_key));
        b0 = ::_mm_xor_si128(b0, rk0);
        b1 = ::_mm_xor_si128(b1, rk0);
        b2 = ::_mm_xor_si128(b2, rk0);
        b3 = ::_mm_xor_si128(b3, rk0);

        for (int round = 1; round < 14; ++round) {
            const simd::vec128_t rk = ::_mm_loadu_si128(
                    reinterpret_cast<const simd::vec128_t*>(expanded_key + static_cast<ptrdiff_t>(round * 16)));
            b0 = ::_mm_aesenc_si128(b0, rk);
            b1 = ::_mm_aesenc_si128(b1, rk);
            b2 = ::_mm_aesenc_si128(b2, rk);
            b3 = ::_mm_aesenc_si128(b3, rk);
        }

        const simd::vec128_t rk14 = ::_mm_loadu_si128(
                reinterpret_cast<const simd::vec128_t*>(expanded_key + static_cast<ptrdiff_t>(14 * 16)));
        b0 = ::_mm_aesenclast_si128(b0, rk14);
        b1 = ::_mm_aesenclast_si128(b1, rk14);
        b2 = ::_mm_aesenclast_si128(b2, rk14);
        b3 = ::_mm_aesenclast_si128(b3, rk14);

        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(dst), b0);
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(dst + 16), b1);
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(dst + 32), b2);
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(dst + 48), b3);
    }

    __NEFORCE_TARGET_APS
    void decrypt_blocks4(byte_t* dst, const byte_t* src, const byte_t* inv_expanded_key) noexcept {
        simd::vec128_t b0 = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(src));
        simd::vec128_t b1 = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(src + 16));
        simd::vec128_t b2 = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(src + 32));
        simd::vec128_t b3 = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(src + 48));

        const simd::vec128_t rk14 = ::_mm_loadu_si128(
                reinterpret_cast<const simd::vec128_t*>(inv_expanded_key + static_cast<ptrdiff_t>(14 * 16)));
        b0 = ::_mm_xor_si128(b0, rk14);
        b1 = ::_mm_xor_si128(b1, rk14);
        b2 = ::_mm_xor_si128(b2, rk14);
        b3 = ::_mm_xor_si128(b3, rk14);

        for (int round = 13; round >= 1; --round) {
            const simd::vec128_t rk = ::_mm_loadu_si128(
                    reinterpret_cast<const simd::vec128_t*>(inv_expanded_key + static_cast<ptrdiff_t>(round * 16)));
            b0 = ::_mm_aesdec_si128(b0, rk);
            b1 = ::_mm_aesdec_si128(b1, rk);
            b2 = ::_mm_aesdec_si128(b2, rk);
            b3 = ::_mm_aesdec_si128(b3, rk);
        }

        const simd::vec128_t rk0 = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(inv_expanded_key));
        b0 = ::_mm_aesdeclast_si128(b0, rk0);
        b1 = ::_mm_aesdeclast_si128(b1, rk0);
        b2 = ::_mm_aesdeclast_si128(b2, rk0);
        b3 = ::_mm_aesdeclast_si128(b3, rk0);

        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(dst), b0);
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(dst + 16), b1);
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(dst + 32), b2);
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(dst + 48), b3);
    }
#endif

    void xor_block(byte_t* dst, const byte_t* src) {
#if defined(NEFORCE_SIMD_SSE2)
        const simd::vec128_t a = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(dst));
        const simd::vec128_t b = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(src));
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(dst), ::_mm_xor_si128(a, b));
#else
        for (int i = 0; i < 16; ++i) {
            dst[i] ^= src[i];
        }
#endif
    }

    void xor_buf(byte_t* dst, const byte_t* a, const byte_t* b, const size_t len) {
#if defined(NEFORCE_SIMD_SSE2)
        size_t i = 0;
        for (; i + 16 <= len; i += 16) {
            const simd::vec128_t va = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(a + i));
            const simd::vec128_t vb = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(b + i));
            ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(dst + i), ::_mm_xor_si128(va, vb));
        }
        for (; i < len; ++i) {
            dst[i] = a[i] ^ b[i];
        }
#else
        for (size_t i = 0; i < len; ++i) {
            dst[i] = a[i] ^ b[i];
        }
#endif
    }

    bool secure_compare(const byte_t* a, const byte_t* b, const size_t len) {
        volatile byte_t diff = 0;
        for (size_t i = 0; i < len; ++i) {
            diff |= a[i] ^ b[i];
        }
        return diff != 0;
    }

    void gf128_multiply(uint64_t& hi, uint64_t& lo, const uint64_t H_hi, const uint64_t H_lo) {
        uint64_t Z_hi = 0, Z_lo = 0;
        uint64_t V_hi = H_hi, V_lo = H_lo;
        const uint64_t X_hi = hi;
        const uint64_t X_lo = lo;

        for (int i = 63; i >= 0; --i) {
            if (((X_hi >> i) & 1) != 0U) {
                Z_hi ^= V_hi;
                Z_lo ^= V_lo;
            }
            if ((V_lo & 1) != 0U) {
                V_lo = (V_lo >> 1) | (V_hi << 63);
                V_hi = (V_hi >> 1) ^ 0xE100000000000000ULL;
            } else {
                V_lo = (V_lo >> 1) | (V_hi << 63);
                V_hi >>= 1;
            }
        }
        for (int i = 63; i >= 0; --i) {
            if (((X_lo >> i) & 1) != 0U) {
                Z_hi ^= V_hi;
                Z_lo ^= V_lo;
            }
            if ((V_lo & 1) != 0U) {
                V_lo = (V_lo >> 1) | (V_hi << 63);
                V_hi = (V_hi >> 1) ^ 0xE100000000000000ULL;
            } else {
                V_lo = (V_lo >> 1) | (V_hi << 63);
                V_hi >>= 1;
            }
        }
        hi = Z_hi;
        lo = Z_lo;
    }

#ifdef NEFORCE_SIMD_PCLMUL
    // Reflected-domain GHASH multiply. Value convention is identical to the scalar gf128_multiply:
    // value bit 127 == data byte 0 bit 7 == x^0, and the reduction identity is
    // x^128 == x^7 + x^2 + x + 1 (fold constant 0xE1).
    __NEFORCE_TARGET_APS simd::vec128_t ghash_reduce(const simd::vec128_t q) noexcept {
        // q: 127-bit value (bit 127 clear). Fold q*x^128 into the low 128 bits:
        //   t = q ^ (q<<1) ^ (q>>1) ^ (q>>6)
        //   underflow bits (x^128..x^133) re-fold via the 0xE1 spread at the top.
        const uint64_t q_lo = ::_mm_cvtsi128_si64(q);
        const uint64_t q_hi = ::_mm_cvtsi128_si64(_mm_srli_si128(q, 8));
        const uint64_t u = q_lo & 0x3F;

        const uint64_t t_lo =
                q_lo ^ ((q_lo << 1) | (q_hi >> 63)) ^ ((q_lo >> 1) | (q_hi << 63)) ^ ((q_lo >> 6) | (q_hi << 58));
        const uint64_t t_hi = q_hi ^ ((q_hi << 1) | (q_lo >> 63)) ^ (q_hi >> 1) ^ (q_hi >> 6);

        const uint64_t fold_hi =
                (u << 51) ^ (u << 56) ^ (u << 57) ^ (u << 58) ^ ((u & 1) != 0U ? 0xE100000000000000ULL : 0);

        return ::_mm_set_epi64x(static_cast<int64_t>(t_hi ^ fold_hi), static_cast<int64_t>(t_lo));
    }

    __NEFORCE_TARGET_APS simd::vec128_t ghash_mult(const simd::vec128_t a, const simd::vec128_t b) noexcept {
        const simd::vec128_t mask_lo = ::_mm_set_epi64x(0, -1);
        const simd::vec128_t a_lo = ::_mm_and_si128(a, mask_lo);
        const auto a_hi = _mm_srli_si128(a, 8);
        const simd::vec128_t b_lo = ::_mm_and_si128(b, mask_lo);
        const auto b_hi = _mm_srli_si128(b, 8);

        const auto p0 = _mm_clmulepi64_si128(a, b, 0x00);
        const auto p3 = _mm_clmulepi64_si128(a, b, 0x11);
        const simd::vec128_t mid = ::_mm_xor_si128(
                ::_mm_xor_si128(_mm_clmulepi64_si128(::_mm_xor_si128(a_lo, a_hi), ::_mm_xor_si128(b_lo, b_hi), 0x00),
                                p0),
                p3);

        // 256-bit product P = [P_hi : P_lo];
        // the reflected 128-bit result is P bits 254..127 = (P_lo >> 63) | (P_hi << 1)
        const simd::vec128_t p_lo = ::_mm_xor_si128(p0, _mm_slli_si128(mid, 8));
        const simd::vec128_t p_hi = ::_mm_xor_si128(p3, _mm_srli_si128(mid, 8));

        const uint64_t p_lo_hi = ::_mm_cvtsi128_si64(_mm_srli_si128(p_lo, 8));
        const uint64_t p_hi_lo = ::_mm_cvtsi128_si64(p_hi);
        const uint64_t p_hi_hi = ::_mm_cvtsi128_si64(_mm_srli_si128(p_hi, 8));

        const uint64_t res_lo = (p_lo_hi >> 63) | (p_hi_lo << 1);
        const uint64_t res_hi = (p_hi_lo >> 63) | (p_hi_hi << 1);

        // Q = P bits 0..126 (bit 127 belongs to the result)
        const simd::vec128_t q =
                ::_mm_and_si128(p_lo, ::_mm_set_epi64x(static_cast<int64_t>(0x7FFFFFFFFFFFFFFFULL), -1));
        return ::_mm_xor_si128(::_mm_set_epi64x(static_cast<int64_t>(res_hi), static_cast<int64_t>(res_lo)),
                               ghash_reduce(q));
    }
#endif

    struct ghash_context {
        uint64_t H_hi;
        uint64_t H_lo;
    };

    void ghash_init(ghash_context& ctx, const byte_t H[16]) {
        ctx.H_hi = endian::read_be64(H);
        ctx.H_lo = endian::read_be64(H + 8);
    }

    __NEFORCE_TARGET_APS
    void ghash_update(const ghash_context& ctx, uint64_t& state_hi, uint64_t& state_lo, const byte_t* data,
                      size_t len) {
#ifdef NEFORCE_SIMD_PCLMUL
        const simd::vec128_t h = ::_mm_set_epi64x(static_cast<int64_t>(ctx.H_hi), static_cast<int64_t>(ctx.H_lo));
        simd::vec128_t state = ::_mm_set_epi64x(static_cast<int64_t>(state_hi), static_cast<int64_t>(state_lo));

        while (len >= 16) {
            const simd::vec128_t block = ::_mm_set_epi64x(static_cast<int64_t>(endian::read_be64(data)),
                                                          static_cast<int64_t>(endian::read_be64(data + 8)));
            state = ghash_mult(::_mm_xor_si128(state, block), h);
            data += 16;
            len -= 16;
        }
        if (len > 0) {
            byte_t padded[16] = {0};
            memory_copy(padded, data, len);
            const simd::vec128_t block = ::_mm_set_epi64x(static_cast<int64_t>(endian::read_be64(padded)),
                                                          static_cast<int64_t>(endian::read_be64(padded + 8)));
            state = ghash_mult(::_mm_xor_si128(state, block), h);
        }

        state_hi = static_cast<uint64_t>(::_mm_cvtsi128_si64(_mm_srli_si128(state, 8)));
        state_lo = static_cast<uint64_t>(::_mm_cvtsi128_si64(state));
#else
        while (len >= 16) {
            state_hi ^= endian::read_be64(data);
            state_lo ^= endian::read_be64(data + 8);

            gf128_multiply(state_hi, state_lo, ctx.H_hi, ctx.H_lo);

            data += 16;
            len -= 16;
        }

        if (len > 0) {
            byte_t padded[16] = {0};
            memory_copy(padded, data, len);
            ghash_update(ctx, state_hi, state_lo, padded, 16);
        }
#endif
    }

    void ghash_final(const ghash_context& ctx, uint64_t& state_hi, uint64_t& state_lo, const size_t a_len,
                     const size_t c_len, byte_t out_tag[16]) {
        byte_t len_block[16] = {0};
        endian::write_be64(len_block, a_len * 8);
        endian::write_be64(len_block + 8, c_len * 8);

        ghash_update(ctx, state_hi, state_lo, len_block, 16);

        endian::write_be64(out_tag, state_hi);
        endian::write_be64(out_tag + 8, state_lo);
    }

    void inc32(byte_t* counter) {
        uint32_t n = (static_cast<uint32_t>(counter[12]) << 24) | (static_cast<uint32_t>(counter[13]) << 16) |
                     (static_cast<uint32_t>(counter[14]) << 8) | (static_cast<uint32_t>(counter[15]));
        ++n;
        counter[12] = static_cast<byte_t>(n >> 24);
        counter[13] = static_cast<byte_t>(n >> 16);
        counter[14] = static_cast<byte_t>(n >> 8);
        counter[15] = static_cast<byte_t>(n);
    }

#ifdef NEFORCE_SIMD_AES_NI
    // Four independent AES chains in flight hide the ~4 cycle aesenc latency.
    __NEFORCE_TARGET_APS
    void AES256_ctr_keystream_x4(byte_t counter[16], const byte_t* expanded_key, byte_t keystream[64]) noexcept {
        simd::vec128_t c0 = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(counter));
        simd::vec128_t c1 = ::_mm_add_epi32(c0, ::_mm_set_epi32(1, 0, 0, 0));
        simd::vec128_t c2 = ::_mm_add_epi32(c0, ::_mm_set_epi32(2, 0, 0, 0));
        simd::vec128_t c3 = ::_mm_add_epi32(c0, ::_mm_set_epi32(3, 0, 0, 0));

        const simd::vec128_t rk0 = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(expanded_key));
        c0 = ::_mm_xor_si128(c0, rk0);
        c1 = ::_mm_xor_si128(c1, rk0);
        c2 = ::_mm_xor_si128(c2, rk0);
        c3 = ::_mm_xor_si128(c3, rk0);
        for (int round = 1; round < 14; ++round) {
            const simd::vec128_t rk = ::_mm_loadu_si128(
                    reinterpret_cast<const simd::vec128_t*>(expanded_key + static_cast<ptrdiff_t>(round * 16)));
            c0 = ::_mm_aesenc_si128(c0, rk);
            c1 = ::_mm_aesenc_si128(c1, rk);
            c2 = ::_mm_aesenc_si128(c2, rk);
            c3 = ::_mm_aesenc_si128(c3, rk);
        }
        const simd::vec128_t rk14 = ::_mm_loadu_si128(
                reinterpret_cast<const simd::vec128_t*>(expanded_key + static_cast<ptrdiff_t>(14 * 16)));
        c0 = ::_mm_aesenclast_si128(c0, rk14);
        c1 = ::_mm_aesenclast_si128(c1, rk14);
        c2 = ::_mm_aesenclast_si128(c2, rk14);
        c3 = ::_mm_aesenclast_si128(c3, rk14);

        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(keystream), c0);
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(keystream + 16), c1);
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(keystream + 32), c2);
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(keystream + 48), c3);

        inc32(counter);
        inc32(counter);
        inc32(counter);
        inc32(counter);
    }
#endif
} // namespace


byte_vector AES256::encrypt_ecb(const cbyte_view data, const cbyte_view key) {
    if (key.size() != 32) {
        NEFORCE_THROW_EXCEPTION(value_exception("AES-256 requires 32-byte key"));
    }
    if (data.size() % 16 != 0) {
        NEFORCE_THROW_EXCEPTION(value_exception("Data size must be multiple of 16 bytes"));
    }

    byte_t expanded_key[240];
    AES256_key_expansion(key.data(), expanded_key);
    byte_vector result(data.size());

    size_t i = 0;
#ifdef NEFORCE_SIMD_AES_NI
    for (; i + 64 <= data.size(); i += 64) {
        encrypt_blocks4(result.data() + i, data.data() + i, expanded_key);
    }
#endif
    for (; i < data.size(); i += 16) {
        encrypt_block(result.data() + i, data.data() + i, expanded_key);
    }
    return result;
}

byte_vector AES256::decrypt_ecb(const cbyte_view data, const cbyte_view key) {
    if (key.size() != 32) {
        NEFORCE_THROW_EXCEPTION(value_exception("AES-256 requires 32-byte key"));
    }
    if (data.size() % 16 != 0) {
        NEFORCE_THROW_EXCEPTION(value_exception("Data size must be multiple of 16 bytes"));
    }

    byte_t inv_expanded_key[240];
    AES256_inv_key_expansion(key.data(), inv_expanded_key);
    byte_vector result(data.size());

    size_t i = 0;
#ifdef NEFORCE_SIMD_AES_NI
    for (; i + 64 <= data.size(); i += 64) {
        decrypt_blocks4(result.data() + i, data.data() + i, inv_expanded_key);
    }
#endif
    for (; i < data.size(); i += 16) {
        decrypt_block(result.data() + i, data.data() + i, inv_expanded_key);
    }
    return result;
}

byte_vector AES256::encrypt_ecb_pkcs7(const cbyte_view data, const cbyte_view key) {
    byte_vector byte_data{data.data(), data.size()};
    const byte_t padding = 16 - (byte_data.size() % 16);
    for (int i = 0; i < padding; ++i) {
        byte_data.push_back(padding);
    }
    return encrypt_ecb(byte_data.view(), key);
}

byte_vector AES256::decrypt_ecb_pkcs7(const cbyte_view data, const cbyte_view key) {
    byte_vector decrypted = decrypt_ecb(data, key);
    if (decrypted.empty()) {
        return decrypted;
    }

    const byte_t padding_len = decrypted.back();
    const size_t original_size = decrypted.size();

    if (padding_len == 0 || padding_len > 16 || padding_len > original_size) {
        NEFORCE_THROW_EXCEPTION(value_exception("Invalid PKCS7 padding (invalid length)"));
    }

    const size_t new_size = original_size - padding_len;
    for (size_t i = new_size; i < original_size; ++i) {
        if (decrypted[i] != padding_len) {
            NEFORCE_THROW_EXCEPTION(value_exception("Invalid PKCS7 padding (mismatched value)"));
        }
    }

    decrypted.resize(new_size);
    return decrypted;
}

string AES256::encrypt_ecb_hex(const string_view data, const string_view key_hex) {
    if (key_hex.size() != 64) {
        NEFORCE_THROW_EXCEPTION(value_exception("AES-256 key must be 64 hex characters (32 bytes)"));
    }

    byte_vector key_bytes;
    key_bytes.reserve(32);
    for (size_t i = 0; i + 2 <= key_hex.size(); i += 2) {
        const auto hex_val = hexadecimal::parse(key_hex.substr(i, 2));
        key_bytes.push_back(static_cast<byte_t>(hex_val.value()));
    }

    const byte_vector data_bytes(reinterpret_cast<const byte_t*>(data.data()), data.size());
    byte_vector encrypted = encrypt_ecb_pkcs7(data_bytes.view(), key_bytes.view());

    string result;
    for (const byte_t byte: encrypted) {
        hexadecimal hex_byte(byte);
        result += format("{:02x}", hex_byte);
    }
    return result;
}

string AES256::decrypt_ecb_hex(const string_view encrypted_hex, const string_view key_hex) {
    if (key_hex.size() != 64) {
        NEFORCE_THROW_EXCEPTION(value_exception("AES-256 key must be 64 hex characters (32 bytes)"));
    }

    byte_vector key_bytes;
    for (size_t i = 0; i + 2 <= key_hex.size(); i += 2) {
        const auto hex_val = hexadecimal::parse(key_hex.substr(i, 2));
        key_bytes.push_back(static_cast<byte_t>(hex_val.value()));
    }

    byte_vector encrypted_bytes;
    for (size_t i = 0; i + 2 <= encrypted_hex.size(); i += 2) {
        const auto hex_val = hexadecimal::parse(encrypted_hex.substr(i, 2));
        encrypted_bytes.push_back(static_cast<byte_t>(hex_val.value()));
    }

    byte_vector decrypted = decrypt_ecb_pkcs7(encrypted_bytes.view(), key_bytes.view());
    return {reinterpret_cast<const char*>(decrypted.data()), decrypted.size()};
}

byte_vector AES256::encrypt_cbc(cbyte_view data, cbyte_view key, cbyte_view iv) {
    if (key.size() != 32) {
        NEFORCE_THROW_EXCEPTION(value_exception("AES-256 requires 32-byte key"));
    }
    if (iv.size() != 16) {
        NEFORCE_THROW_EXCEPTION(value_exception("CBC mode requires 16-byte IV"));
    }
    if (data.size() % 16 != 0) {
        NEFORCE_THROW_EXCEPTION(value_exception("Data size must be multiple of 16 bytes for CBC no padding"));
    }

    byte_t expanded_key[240];
    AES256_key_expansion(key.data(), expanded_key);

    byte_vector result(data.size());

    byte_t prev[16];
    memory_copy(prev, iv.data(), 16);

    for (size_t i = 0; i < data.size(); i += 16) {
        byte_t block[16];
        memory_copy(block, data.data() + i, 16);
        xor_block(block, prev);
        encrypt_block(result.data() + i, block, expanded_key);
        memory_copy(prev, result.data() + i, 16);
    }
    return result;
}

byte_vector AES256::decrypt_cbc(cbyte_view data, cbyte_view key, cbyte_view iv) {
    if (key.size() != 32) {
        NEFORCE_THROW_EXCEPTION(value_exception("AES-256 requires 32-byte key"));
    }
    if (iv.size() != 16) {
        NEFORCE_THROW_EXCEPTION(value_exception("CBC mode requires 16-byte IV"));
    }
    if (data.size() % 16 != 0) {
        NEFORCE_THROW_EXCEPTION(value_exception("Data size must be multiple of 16 bytes"));
    }

    byte_t inv_expanded_key[240];
    AES256_inv_key_expansion(key.data(), inv_expanded_key);

    byte_vector result(data.size());

    byte_t prev[16];
    memory_copy(prev, iv.data(), 16);

    size_t i = 0;

#ifdef NEFORCE_SIMD_AES_NI
    // CBC decryption blocks are independent: decrypt 4 at once, then XOR the chain
    for (; i + 64 <= data.size(); i += 64) {
        decrypt_blocks4(result.data() + i, data.data() + i, inv_expanded_key);

        simd::vec128_t p0 = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(result.data() + i));
        simd::vec128_t p1 = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(result.data() + i + 16));
        simd::vec128_t p2 = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(result.data() + i + 32));
        simd::vec128_t p3 = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(result.data() + i + 48));

        const simd::vec128_t c0 = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(data.data() + i));
        const simd::vec128_t c1 = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(data.data() + i + 16));
        const simd::vec128_t c2 = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(data.data() + i + 32));
        const simd::vec128_t c3 NEFORCE_UNUSED =
                ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(data.data() + i + 48));
        const simd::vec128_t prev_v = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(prev));

        p0 = ::_mm_xor_si128(p0, prev_v);
        p1 = ::_mm_xor_si128(p1, c0);
        p2 = ::_mm_xor_si128(p2, c1);
        p3 = ::_mm_xor_si128(p3, c2);

        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(result.data() + i), p0);
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(result.data() + i + 16), p1);
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(result.data() + i + 32), p2);
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(result.data() + i + 48), p3);

        memory_copy(prev, data.data() + i + 48, 16);
    }
#endif
    for (; i < data.size(); i += 16) {
        byte_t block[16];
        memory_copy(block, data.data() + i, 16);
        decrypt_block(result.data() + i, block, inv_expanded_key);
        xor_block(result.data() + i, prev);
        memory_copy(prev, data.data() + i, 16);
    }
    return result;
}

byte_vector AES256::encrypt_cbc_pkcs7(cbyte_view data, cbyte_view key, cbyte_view iv) {
    byte_vector padded{data.data(), data.size()};
    const byte_t padding = 16 - (padded.size() % 16);
    for (byte_t i = 0; i < padding; ++i) {
        padded.push_back(padding);
    }
    return encrypt_cbc(padded.view(), key, iv);
}

byte_vector AES256::decrypt_cbc_pkcs7(cbyte_view data, cbyte_view key, cbyte_view iv) {
    byte_vector decrypted = decrypt_cbc(data, key, iv);
    if (decrypted.empty()) {
        return decrypted;
    }

    const byte_t padding_len = decrypted.back();
    const size_t original_size = decrypted.size();

    if (padding_len == 0 || padding_len > 16 || padding_len > original_size) {
        NEFORCE_THROW_EXCEPTION(value_exception("Invalid PKCS7 padding"));
    }
    const size_t new_size = original_size - padding_len;
    for (size_t i = new_size; i < original_size; ++i) {
        if (decrypted[i] != padding_len) {
            NEFORCE_THROW_EXCEPTION(value_exception("Invalid PKCS7 padding"));
        }
    }
    decrypted.resize(new_size);
    return decrypted;
}

byte_vector AES256::encrypt_gcm(cbyte_view data, cbyte_view key, cbyte_view iv, cbyte_view aad, byte_t* tag,
                                const size_t tag_len) {
    if (key.size() != 32) {
        NEFORCE_THROW_EXCEPTION(value_exception("AES-256 requires 32-byte key"));
    }
    if (iv.empty()) {
        NEFORCE_THROW_EXCEPTION(value_exception("GCM IV cannot be empty"));
    }
    if (tag == nullptr) {
        NEFORCE_THROW_EXCEPTION(value_exception("GCM tag output pointer cannot be null"));
    }
    if (tag_len < 12 || tag_len > 16) {
        NEFORCE_THROW_EXCEPTION(value_exception("GCM tag length must be between 12 and 16 bytes"));
    }

    byte_t expanded_key[240];
    AES256_key_expansion(key.data(), expanded_key);

    byte_t H[16] = {0};
    encrypt_block(H, H, expanded_key);

    ghash_context gh_ctx;
    ghash_init(gh_ctx, H);

    byte_t J0[16] = {0};
    if (iv.size() == 12) {
        memory_copy(J0, iv.data(), 12);
        J0[15] = 0x01;
    } else {
        uint64_t state_hi = 0, state_lo = 0;
        ghash_update(gh_ctx, state_hi, state_lo, iv.data(), iv.size());
        ghash_final(gh_ctx, state_hi, state_lo, 0, iv.size(), J0);
    }

    byte_vector ciphertext(data.size());

    byte_t counter[16];
    memory_copy(counter, J0, 16);
    inc32(counter);

    size_t i = 0;
#ifdef NEFORCE_SIMD_AES_NI
    for (; i + 64 <= data.size(); i += 64) {
        byte_t keystream[64];
        AES256_ctr_keystream_x4(counter, expanded_key, keystream);

        xor_buf(ciphertext.data() + i, data.data() + i, keystream, 64);
    }
#endif
    for (; i < data.size(); i += 16) {
        byte_t keystream[16];
        memory_copy(keystream, counter, 16);
        encrypt_block(keystream, keystream, expanded_key);

        const size_t block_len = min<size_t>(16, data.size() - i);
        xor_buf(ciphertext.data() + i, data.data() + i, keystream, block_len);
        inc32(counter);
    }

    uint64_t state_hi = 0, state_lo = 0;
    ghash_update(gh_ctx, state_hi, state_lo, aad.data(), aad.size());
    ghash_update(gh_ctx, state_hi, state_lo, ciphertext.data(), ciphertext.size());

    byte_t full_tag[16];
    ghash_final(gh_ctx, state_hi, state_lo, aad.size(), ciphertext.size(), full_tag);

    byte_t enc_J0[16];
    memory_copy(enc_J0, J0, 16);
    encrypt_block(enc_J0, enc_J0, expanded_key);
    xor_block(full_tag, enc_J0);

    memory_copy(tag, full_tag, tag_len);
    return ciphertext;
}

byte_vector AES256::decrypt_gcm(cbyte_view data, cbyte_view key, cbyte_view iv, cbyte_view aad, cbyte_view tag,
                                const size_t tag_len) {
    if (key.size() != 32) {
        NEFORCE_THROW_EXCEPTION(value_exception("AES-256 requires 32-byte key"));
    }
    if (iv.empty()) {
        NEFORCE_THROW_EXCEPTION(value_exception("GCM IV cannot be empty"));
    }
    if (tag_len < 12 || tag_len > 16) {
        NEFORCE_THROW_EXCEPTION(value_exception("GCM tag length must be between 12 and 16 bytes"));
    }
    if (tag.size() < tag_len) {
        NEFORCE_THROW_EXCEPTION(value_exception("Tag size mismatch"));
    }

    byte_t expanded_key[240];
    AES256_key_expansion(key.data(), expanded_key);

    byte_t H[16] = {0};
    encrypt_block(H, H, expanded_key);
    ghash_context gh_ctx;
    ghash_init(gh_ctx, H);

    byte_t J0[16] = {0};
    if (iv.size() == 12) {
        memory_copy(J0, iv.data(), 12);
        J0[15] = 1;
    } else {
        uint64_t state_hi = 0, state_lo = 0;
        ghash_update(gh_ctx, state_hi, state_lo, iv.data(), iv.size());
        ghash_final(gh_ctx, state_hi, state_lo, 0, iv.size(), J0);
    }

    uint64_t state_hi = 0, state_lo = 0;
    ghash_update(gh_ctx, state_hi, state_lo, aad.data(), aad.size());
    ghash_update(gh_ctx, state_hi, state_lo, data.data(), data.size());

    byte_t expected_tag[16];
    ghash_final(gh_ctx, state_hi, state_lo, aad.size(), data.size(), expected_tag);

    byte_t enc_J0[16];
    memory_copy(enc_J0, J0, 16);
    encrypt_block(enc_J0, enc_J0, expanded_key);
    xor_block(expected_tag, enc_J0);

    if (secure_compare(expected_tag, tag.data(), tag_len)) {
        NEFORCE_THROW_EXCEPTION(value_exception("GCM authentication failed"));
    }

    byte_vector plaintext(data.size());

    byte_t counter[16];
    memory_copy(counter, J0, 16);
    inc32(counter);

    size_t i = 0;
#ifdef NEFORCE_SIMD_AES_NI
    for (; i + 64 <= data.size(); i += 64) {
        byte_t keystream[64];
        AES256_ctr_keystream_x4(counter, expanded_key, keystream);

        xor_buf(plaintext.data() + i, data.data() + i, keystream, 64);
    }
#endif
    for (; i < data.size(); i += 16) {
        byte_t keystream[16];
        memory_copy(keystream, counter, 16);
        encrypt_block(keystream, keystream, expanded_key);

        const size_t block_len = min<size_t>(16, data.size() - i);
        xor_buf(plaintext.data() + i, data.data() + i, keystream, block_len);
        inc32(counter);
    }

    return plaintext;
}

NEFORCE_END_NAMESPACE__
