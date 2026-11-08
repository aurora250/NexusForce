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
#ifdef __NEFORCE_AI
#    undef __NEFORCE_AI
#endif
#if defined(NEFORCE_COMPILER_GNUC)
#    define __NEFORCE_AI NEFORCE_ALWAYS_INLINE_INLINE
#else
#    define __NEFORCE_AI
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
    NEFORCE_OPTIMIZE("O3") byte_t AES256_gf_mult(byte_t a, byte_t b) noexcept {
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

    byte_t AES256_gf_square(const byte_t x) noexcept { return AES256_gf_mult(x, x); }

    NEFORCE_OPTIMIZE("O3") byte_t AES256_gf_inv(const byte_t x) noexcept {
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

    NEFORCE_OPTIMIZE("O3") byte_t AES256_sbox_ct(const byte_t x) noexcept {
        // S(x) = affine(gf_inv(x)): y = x ^ rotl1 ^ rotl2 ^ rotl3 ^ rotl4 ^ 0x63
        const byte_t inv = AES256_gf_inv(x);
        byte_t y = inv;
        for (int i = 1; i <= 4; ++i) {
            y ^= static_cast<byte_t>((inv << i) | (inv >> (8 - i)));
        }
        return static_cast<byte_t>(y ^ 0x63);
    }

    NEFORCE_OPTIMIZE("O3") byte_t AES256_inv_sbox_ct(const byte_t x) noexcept {
        // inv S(x) = gf_inv(rotl1(x) ^ rotl3(x) ^ rotl6(x) ^ 0x05)
        const auto r1 = static_cast<byte_t>((x << 1) | (x >> 7));
        const auto r3 = static_cast<byte_t>((x << 3) | (x >> 5));
        const auto r6 = static_cast<byte_t>((x << 6) | (x >> 2));
        return AES256_gf_inv(static_cast<byte_t>(r1 ^ r3 ^ r6 ^ 0x05));
    }

    constexpr byte_t AES256_rcon[15] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
                                        0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a};

    NEFORCE_OPTIMIZE("O3") void AES256_sub_bytes(byte_t state[16]) noexcept {
        for (int i = 0; i < 16; ++i) {
            state[i] = AES256_sbox_ct(state[i]);
        }
    }

    NEFORCE_OPTIMIZE("O3") void AES256_inv_sub_bytes(byte_t state[16]) noexcept {
        for (int i = 0; i < 16; ++i) {
            state[i] = AES256_inv_sbox_ct(state[i]);
        }
    }

    NEFORCE_OPTIMIZE("O3") void AES256_add_round_key(byte_t state[16], const byte_t* round_key) noexcept {
        for (int i = 0; i < 16; ++i) {
            state[i] ^= round_key[i];
        }
    }

    NEFORCE_OPTIMIZE("O3") void AES256_key_expansion_scalar(const byte_t* key, byte_t* expanded_key) noexcept {
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

#if defined(NEFORCE_SIMD_AES_NI)
    NEFORCE_OPTIMIZE("O3")
    __NEFORCE_TARGET_APS simd::vec128_t
            AES256_key_expand_256_assist_1(simd::vec128_t temp1, simd::vec128_t temp2) noexcept {
        temp2 = _mm_shuffle_epi32(temp2, 0xff);
        temp1 = ::_mm_xor_si128(temp1, _mm_slli_si128(temp1, 4));
        temp1 = ::_mm_xor_si128(temp1, _mm_slli_si128(temp1, 4));
        temp1 = ::_mm_xor_si128(temp1, _mm_slli_si128(temp1, 4));
        return ::_mm_xor_si128(temp1, temp2);
    }

    NEFORCE_OPTIMIZE("O3")
    __NEFORCE_TARGET_APS simd::vec128_t
            AES256_key_expand_256_assist_2(simd::vec128_t temp1, simd::vec128_t temp3) noexcept {
        const auto temp2 = _mm_shuffle_epi32(_mm_aeskeygenassist_si128(temp1, 0x00), 0xaa);
        temp3 = ::_mm_xor_si128(temp3, _mm_slli_si128(temp3, 4));
        temp3 = ::_mm_xor_si128(temp3, _mm_slli_si128(temp3, 4));
        temp3 = ::_mm_xor_si128(temp3, _mm_slli_si128(temp3, 4));
        return ::_mm_xor_si128(temp3, temp2);
    }

    NEFORCE_OPTIMIZE("O3")
    __NEFORCE_TARGET_APS void AES256_key_expansion_aesni(const byte_t* key, byte_t* expanded_key) noexcept {
        simd::vec128_t key_lo = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(key));
        simd::vec128_t key_hi = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(key + 16));
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(expanded_key), key_lo);
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(expanded_key + 16), key_hi);

        key_lo = AES256_key_expand_256_assist_1(key_lo, _mm_aeskeygenassist_si128(key_hi, 0x01));
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(expanded_key + 32), key_lo);
        key_hi = AES256_key_expand_256_assist_2(key_lo, key_hi);
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(expanded_key + 48), key_hi);

        key_lo = AES256_key_expand_256_assist_1(key_lo, _mm_aeskeygenassist_si128(key_hi, 0x02));
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(expanded_key + 64), key_lo);
        key_hi = AES256_key_expand_256_assist_2(key_lo, key_hi);
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(expanded_key + 80), key_hi);

        key_lo = AES256_key_expand_256_assist_1(key_lo, _mm_aeskeygenassist_si128(key_hi, 0x04));
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(expanded_key + 96), key_lo);
        key_hi = AES256_key_expand_256_assist_2(key_lo, key_hi);
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(expanded_key + 112), key_hi);

        key_lo = AES256_key_expand_256_assist_1(key_lo, _mm_aeskeygenassist_si128(key_hi, 0x08));
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(expanded_key + 128), key_lo);
        key_hi = AES256_key_expand_256_assist_2(key_lo, key_hi);
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(expanded_key + 144), key_hi);

        key_lo = AES256_key_expand_256_assist_1(key_lo, _mm_aeskeygenassist_si128(key_hi, 0x10));
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(expanded_key + 160), key_lo);
        key_hi = AES256_key_expand_256_assist_2(key_lo, key_hi);
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(expanded_key + 176), key_hi);

        key_lo = AES256_key_expand_256_assist_1(key_lo, _mm_aeskeygenassist_si128(key_hi, 0x20));
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(expanded_key + 192), key_lo);
        key_hi = AES256_key_expand_256_assist_2(key_lo, key_hi);
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(expanded_key + 208), key_hi);

        key_lo = AES256_key_expand_256_assist_1(key_lo, _mm_aeskeygenassist_si128(key_hi, 0x40));
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(expanded_key + 224), key_lo);
    }
#endif

    NEFORCE_OPTIMIZE("O3") void AES256_key_expansion(const byte_t* key, byte_t* expanded_key) noexcept {
#if defined(NEFORCE_SIMD_AES_NI)
        AES256_key_expansion_aesni(key, expanded_key);
#else
        AES256_key_expansion_scalar(key, expanded_key);
#endif
    }

    NEFORCE_OPTIMIZE("O3")
    __NEFORCE_TARGET_APS void AES256_inv_key_expansion(const byte_t* key, byte_t* inv_expanded_key) noexcept {
        AES256_key_expansion(key, inv_expanded_key);
#if defined(NEFORCE_SIMD_AES_NI)
        // precompute InvMixColumns-transformed round keys so decrypt_block does not apply aesimc per block per round
        for (int round = 1; round < 14; ++round) {
            simd::vec128_t rk = ::_mm_loadu_si128(
                    reinterpret_cast<const simd::vec128_t*>(inv_expanded_key + static_cast<ptrdiff_t>(round * 16)));
            rk = ::_mm_aesimc_si128(rk);
            ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(inv_expanded_key + static_cast<ptrdiff_t>(round * 16)),
                               rk);
        }
#endif
    }

    NEFORCE_OPTIMIZE("O3") void AES256_shift_rows(byte_t state[16]) noexcept {
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

    NEFORCE_OPTIMIZE("O3") void AES256_inv_shift_rows(byte_t state[16]) noexcept {
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

    NEFORCE_OPTIMIZE("O3") void AES256_mix_columns(byte_t state[16]) noexcept {
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

    NEFORCE_OPTIMIZE("O3") void AES256_inv_mix_columns(byte_t state[16]) noexcept {
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

    __NEFORCE_TARGET_APS NEFORCE_OPTIMIZE("O3") void encrypt_block(byte_t dst[16], const byte_t* src,
                                                                   const byte_t* expanded_key) noexcept {
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
        simd::vec128_t state = vld1q_u8(src);

        for (int round = 0; round < 13; ++round) {
            state = ::vaeseq_u8(state, vld1q_u8(reinterpret_cast<const uint8_t*>(expanded_key +
                                                                                 static_cast<ptrdiff_t>(round * 16))));
            state = ::vaesmcq_u8(state);
        }

        state = ::vaeseq_u8(state,
                            vld1q_u8(reinterpret_cast<const uint8_t*>(expanded_key + static_cast<ptrdiff_t>(13 * 16))));
        state = ::veorq_u8(state,
                           vld1q_u8(reinterpret_cast<const uint8_t*>(expanded_key + static_cast<ptrdiff_t>(14 * 16))));

        vst1q_u8(dst, state);

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

    __NEFORCE_TARGET_APS NEFORCE_OPTIMIZE("O3") void decrypt_block(byte_t dst[16], const byte_t* src,
                                                                   const byte_t* inv_expanded_key) noexcept {
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
        simd::vec128_t state = vld1q_u8(src);

        state = ::vaesdq_u8(
                state, vld1q_u8(reinterpret_cast<const uint8_t*>(inv_expanded_key + static_cast<ptrdiff_t>(14 * 16))));
        state = ::vaesimcq_u8(state);

        for (int round = 13; round >= 2; --round) {
            state = ::vaesdq_u8(state, ::vaesimcq_u8(vld1q_u8(reinterpret_cast<const uint8_t*>(
                                               inv_expanded_key + static_cast<ptrdiff_t>(round * 16)))));
            state = ::vaesimcq_u8(state);
        }

        state = ::vaesdq_u8(state, ::vaesimcq_u8(vld1q_u8(reinterpret_cast<const uint8_t*>(
                                           inv_expanded_key + static_cast<ptrdiff_t>(16)))));
        state = ::veorq_u8(state, vld1q_u8(reinterpret_cast<const uint8_t*>(inv_expanded_key)));

        vst1q_u8(dst, state);

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
    __NEFORCE_TARGET_APS NEFORCE_OPTIMIZE("O3") void encrypt_blocks4(byte_t* dst, const byte_t* src,
                                                                     const byte_t* expanded_key) noexcept {
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

    __NEFORCE_TARGET_APS NEFORCE_OPTIMIZE("O3") void decrypt_blocks4(byte_t* dst, const byte_t* src,
                                                                     const byte_t* inv_expanded_key) noexcept {
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

    NEFORCE_OPTIMIZE("O3") void xor_block(byte_t* dst, const byte_t* src) noexcept {
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

    NEFORCE_OPTIMIZE("O3") void xor_buf(byte_t* dst, const byte_t* a, const byte_t* b, const size_t len) noexcept {
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

    NEFORCE_OPTIMIZE("O3") bool secure_compare(const byte_t* a, const byte_t* b, const size_t len) noexcept {
        volatile byte_t diff = 0;
        for (size_t i = 0; i < len; ++i) {
            diff |= a[i] ^ b[i];
        }
        return diff != 0;
    }

    NEFORCE_OPTIMIZE("O3")
    void gf128_multiply(uint64_t& hi, uint64_t& lo, const uint64_t H_hi, const uint64_t H_lo) noexcept {
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
    NEFORCE_OPTIMIZE("O3") __NEFORCE_TARGET_APS simd::vec128_t ghash_byteswap(const simd::vec128_t value) noexcept {
        const simd::vec128_t index = ::_mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
        return ::_mm_shuffle_epi8(value, index);
    }

    NEFORCE_OPTIMIZE("O3")
    __NEFORCE_TARGET_APS void ghash_bk(const simd::vec128_t value, simd::vec128_t& halves) noexcept {
        halves = ::_mm_xor_si128(value, _mm_shuffle_epi32(value, 0x0E));
    }

    NEFORCE_OPTIMIZE("O3")
    __NEFORCE_TARGET_APS void ghash_product_raw(const simd::vec128_t a, const simd::vec128_t b, simd::vec128_t& t1,
                                                simd::vec128_t& t2, simd::vec128_t& t3) noexcept {
        simd::vec128_t a_halves;
        simd::vec128_t b_halves;
        ghash_bk(a, a_halves);
        ghash_bk(b, b_halves);
        t1 = _mm_clmulepi64_si128(a, b, 0x11);
        t3 = _mm_clmulepi64_si128(a, b, 0x00);
        t2 = ::_mm_xor_si128(_mm_clmulepi64_si128(a_halves, b_halves, 0x00), ::_mm_xor_si128(t1, t3));
    }

    NEFORCE_OPTIMIZE("O3")
    __NEFORCE_TARGET_APS
            void ghash_product_raw_salted(const simd::vec128_t a, const simd::vec128_t b, const simd::vec128_t b_halves,
                                          simd::vec128_t& t1, simd::vec128_t& t2, simd::vec128_t& t3) noexcept {
        simd::vec128_t a_halves;
        ghash_bk(a, a_halves);
        t1 = _mm_clmulepi64_si128(a, b, 0x11);
        t3 = _mm_clmulepi64_si128(a, b, 0x00);
        t2 = ::_mm_xor_si128(_mm_clmulepi64_si128(a_halves, b_halves, 0x00), ::_mm_xor_si128(t1, t3));
    }

    NEFORCE_OPTIMIZE("O3")
    __NEFORCE_TARGET_APS __NEFORCE_AI
            void ghash_accumulate_salted(const simd::vec128_t a, const simd::vec128_t b, const simd::vec128_t b_halves,
                                         simd::vec128_t& t1, simd::vec128_t& t2, simd::vec128_t& t3) noexcept {
        simd::vec128_t a_halves;
        ghash_bk(a, a_halves);
        const auto p1 = _mm_clmulepi64_si128(a, b, 0x11);
        const auto p3 = _mm_clmulepi64_si128(a, b, 0x00);
        const auto p2 = ::_mm_xor_si128(_mm_clmulepi64_si128(a_halves, b_halves, 0x00), ::_mm_xor_si128(p1, p3));
        t1 = ::_mm_xor_si128(t1, p1);
        t2 = ::_mm_xor_si128(t2, p2);
        t3 = ::_mm_xor_si128(t3, p3);
    }

    NEFORCE_OPTIMIZE("O3")
    __NEFORCE_TARGET_APS void ghash_shift_left_256(simd::vec128_t& x0, simd::vec128_t& x1, simd::vec128_t& x2,
                                                   simd::vec128_t& x3) noexcept {
        x0 = ::_mm_or_si128(::_mm_slli_epi64(x0, 1), ::_mm_srli_epi64(x1, 63));
        x1 = ::_mm_or_si128(::_mm_slli_epi64(x1, 1), ::_mm_srli_epi64(x2, 63));
        x2 = ::_mm_or_si128(::_mm_slli_epi64(x2, 1), ::_mm_srli_epi64(x3, 63));
        x3 = ::_mm_slli_epi64(x3, 1);
    }

    NEFORCE_OPTIMIZE("O3")
    __NEFORCE_TARGET_APS void ghash_reduce_256(simd::vec128_t& x0, simd::vec128_t& x1, simd::vec128_t& x2,
                                               simd::vec128_t& x3) noexcept {
        x1 = ::_mm_xor_si128(x1, ::_mm_xor_si128(::_mm_xor_si128(x3, ::_mm_srli_epi64(x3, 1)),
                                                 ::_mm_xor_si128(::_mm_srli_epi64(x3, 2), ::_mm_srli_epi64(x3, 7))));
        x2 = ::_mm_xor_si128(::_mm_xor_si128(x2, ::_mm_slli_epi64(x3, 63)),
                             ::_mm_xor_si128(::_mm_slli_epi64(x3, 62), ::_mm_slli_epi64(x3, 57)));
        x0 = ::_mm_xor_si128(x0, ::_mm_xor_si128(::_mm_xor_si128(x2, ::_mm_srli_epi64(x2, 1)),
                                                 ::_mm_xor_si128(::_mm_srli_epi64(x2, 2), ::_mm_srli_epi64(x2, 7))));
        x1 = ::_mm_xor_si128(::_mm_xor_si128(x1, ::_mm_slli_epi64(x2, 63)),
                             ::_mm_xor_si128(::_mm_slli_epi64(x2, 62), ::_mm_slli_epi64(x2, 57)));
    }

    NEFORCE_OPTIMIZE("O3")
    __NEFORCE_TARGET_APS void ghash_product_finish(simd::vec128_t& t1, simd::vec128_t& t2, simd::vec128_t& t3,
                                                   simd::vec128_t& w, simd::vec128_t& x) noexcept {
        auto t0 = _mm_shuffle_epi32(t1, 0x0E);
        t1 = ::_mm_xor_si128(t1, _mm_shuffle_epi32(t2, 0x0E));
        t2 = ::_mm_xor_si128(t2, _mm_shuffle_epi32(t3, 0x0E));
        ghash_shift_left_256(t0, t1, t2, t3);
        ghash_reduce_256(t0, t1, t2, t3);
        w = ::_mm_unpacklo_epi64(t1, t0);
        x = ::_mm_xor_si128(t1, t0);
    }

    NEFORCE_OPTIMIZE("O3")
    __NEFORCE_TARGET_APS void ghash_mul_vector(const simd::vec128_t a, const simd::vec128_t b, simd::vec128_t& w,
                                               simd::vec128_t& x) noexcept {
        simd::vec128_t t1;
        simd::vec128_t t2;
        simd::vec128_t t3;
        ghash_product_raw(a, b, t1, t2, t3);
        ghash_product_finish(t1, t2, t3, w, x);
    }

    NEFORCE_OPTIMIZE("O3")
    __NEFORCE_TARGET_APS
            void ghash_square_vector(const simd::vec128_t value, simd::vec128_t& w, simd::vec128_t& x) noexcept {
        simd::vec128_t t1;
        simd::vec128_t t2;
        simd::vec128_t t3;
        ghash_product_raw(value, value, t1, t2, t3);
        ghash_product_finish(t1, t2, t3, w, x);
    }
#endif

    struct ghash_context {
        uint64_t H_hi;
        uint64_t H_lo;
        uint64_t H2_hi;
        uint64_t H2_lo;
        uint64_t H3_hi;
        uint64_t H3_lo;
        uint64_t H4_hi;
        uint64_t H4_lo;
        uint64_t H5_hi;
        uint64_t H5_lo;
        uint64_t H6_hi;
        uint64_t H6_lo;
        uint64_t H7_hi;
        uint64_t H7_lo;
        uint64_t H8_hi;
        uint64_t H8_lo;
#ifdef NEFORCE_SIMD_PCLMUL
        simd::vec128_t h1w;
        simd::vec128_t h1x;
        simd::vec128_t h2w;
        simd::vec128_t h2x;
        simd::vec128_t h3w;
        simd::vec128_t h3x;
        simd::vec128_t h4w;
        simd::vec128_t h4x;
        simd::vec128_t h5w;
        simd::vec128_t h5x;
        simd::vec128_t h6w;
        simd::vec128_t h6x;
        simd::vec128_t h7w;
        simd::vec128_t h7x;
        simd::vec128_t h8w;
        simd::vec128_t h8x;
#endif
#if defined(NEFORCE_SIMD_VAES) && defined(NEFORCE_SIMD_AVX2) && defined(NEFORCE_SIMD_PCLMUL)
        simd::vec256_t h87w;
        simd::vec256_t h87x;
        simd::vec256_t h65w;
        simd::vec256_t h65x;
        simd::vec256_t h43w;
        simd::vec256_t h43x;
        simd::vec256_t h21w;
        simd::vec256_t h21x;
#endif
    };

    __NEFORCE_TARGET_APS NEFORCE_OPTIMIZE("O3") void ghash_init(ghash_context& ctx, const byte_t H[16]) noexcept {
        ctx.H_hi = endian::read_be64(H);
        ctx.H_lo = endian::read_be64(H + 8);
        ctx.H2_hi = ctx.H2_lo = ctx.H3_hi = ctx.H3_lo = ctx.H4_hi = ctx.H4_lo = 0;
        ctx.H5_hi = ctx.H5_lo = ctx.H6_hi = ctx.H6_lo = ctx.H7_hi = ctx.H7_lo = ctx.H8_hi = ctx.H8_lo = 0;
#ifdef NEFORCE_SIMD_PCLMUL
        ctx.h1w = ghash_byteswap(::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(H)));
        ghash_bk(ctx.h1w, ctx.h1x);
        ghash_square_vector(ctx.h1w, ctx.h2w, ctx.h2x);
        ghash_mul_vector(ctx.h1w, ctx.h2w, ctx.h3w, ctx.h3x);
        ghash_square_vector(ctx.h2w, ctx.h4w, ctx.h4x);
        ghash_mul_vector(ctx.h4w, ctx.h1w, ctx.h5w, ctx.h5x);
        ghash_mul_vector(ctx.h5w, ctx.h1w, ctx.h6w, ctx.h6x);
        ghash_mul_vector(ctx.h6w, ctx.h1w, ctx.h7w, ctx.h7x);
        ghash_mul_vector(ctx.h7w, ctx.h1w, ctx.h8w, ctx.h8x);
#endif
#if defined(NEFORCE_SIMD_VAES) && defined(NEFORCE_SIMD_AVX2) && defined(NEFORCE_SIMD_PCLMUL)
        ctx.h87w = ::_mm256_set_m128i(ctx.h7w, ctx.h8w);
        ctx.h87x = ::_mm256_set_m128i(ctx.h7x, ctx.h8x);
        ctx.h65w = ::_mm256_set_m128i(ctx.h5w, ctx.h6w);
        ctx.h65x = ::_mm256_set_m128i(ctx.h5x, ctx.h6x);
        ctx.h43w = ::_mm256_set_m128i(ctx.h3w, ctx.h4w);
        ctx.h43x = ::_mm256_set_m128i(ctx.h3x, ctx.h4x);
        ctx.h21w = ::_mm256_set_m128i(ctx.h1w, ctx.h2w);
        ctx.h21x = ::_mm256_set_m128i(ctx.h1x, ctx.h2x);
#endif
    }

    NEFORCE_OPTIMIZE("O3")
    __NEFORCE_AI simd::vec128_t ghash_blocks8(const ghash_context& ctx, simd::vec128_t y, const byte_t* data) noexcept {
        const auto aw0 =
                ::_mm_xor_si128(ghash_byteswap(::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(data))), y);
        const auto aw1 = ghash_byteswap(::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(data + 16)));
        const auto aw2 = ghash_byteswap(::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(data + 32)));
        const auto aw3 = ghash_byteswap(::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(data + 48)));
        const auto aw4 = ghash_byteswap(::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(data + 64)));
        const auto aw5 = ghash_byteswap(::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(data + 80)));
        const auto aw6 = ghash_byteswap(::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(data + 96)));
        const auto aw7 = ghash_byteswap(::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(data + 112)));

        simd::vec128_t t1 = ::_mm_setzero_si128();
        simd::vec128_t t2 = ::_mm_setzero_si128();
        simd::vec128_t t3 = ::_mm_setzero_si128();
        ghash_accumulate_salted(aw0, ctx.h8w, ctx.h8x, t1, t2, t3);
        ghash_accumulate_salted(aw1, ctx.h7w, ctx.h7x, t1, t2, t3);
        ghash_accumulate_salted(aw2, ctx.h6w, ctx.h6x, t1, t2, t3);
        ghash_accumulate_salted(aw3, ctx.h5w, ctx.h5x, t1, t2, t3);
        ghash_accumulate_salted(aw4, ctx.h4w, ctx.h4x, t1, t2, t3);
        ghash_accumulate_salted(aw5, ctx.h3w, ctx.h3x, t1, t2, t3);
        ghash_accumulate_salted(aw6, ctx.h2w, ctx.h2x, t1, t2, t3);
        ghash_accumulate_salted(aw7, ctx.h1w, ctx.h1x, t1, t2, t3);
        simd::vec128_t ignored;
        ghash_product_finish(t1, t2, t3, y, ignored);
        return y;
    }

    NEFORCE_OPTIMIZE("O3")
    __NEFORCE_AI void ghash_update(const ghash_context& ctx, uint64_t& state_hi, uint64_t& state_lo, const byte_t* data,
                                   size_t len) noexcept {
#ifdef NEFORCE_SIMD_PCLMUL
        auto yw = ghash_byteswap(::_mm_set_epi64x(static_cast<int64_t>(state_lo), static_cast<int64_t>(state_hi)));
        while (len >= 128) {
            yw = ghash_blocks8(ctx, yw, data);
            data += 128;
            len -= 128;
        }
        while (len >= 64) {
            simd::vec128_t aw0 = ghash_byteswap(::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(data)));
            simd::vec128_t aw1 = ghash_byteswap(::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(data + 16)));
            simd::vec128_t aw2 = ghash_byteswap(::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(data + 32)));
            simd::vec128_t aw3 = ghash_byteswap(::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(data + 48)));
            aw0 = ::_mm_xor_si128(aw0, yw);
            simd::vec128_t t1 = ::_mm_setzero_si128();
            simd::vec128_t t2 = ::_mm_setzero_si128();
            simd::vec128_t t3 = ::_mm_setzero_si128();
            simd::vec128_t q1;
            simd::vec128_t q2;
            simd::vec128_t q3;
            ghash_product_raw_salted(aw0, ctx.h4w, ctx.h4x, q1, q2, q3);
            t1 = ::_mm_xor_si128(t1, q1);
            t2 = ::_mm_xor_si128(t2, q2);
            t3 = ::_mm_xor_si128(t3, q3);
            ghash_product_raw_salted(aw1, ctx.h3w, ctx.h3x, q1, q2, q3);
            t1 = ::_mm_xor_si128(t1, q1);
            t2 = ::_mm_xor_si128(t2, q2);
            t3 = ::_mm_xor_si128(t3, q3);
            ghash_product_raw_salted(aw2, ctx.h2w, ctx.h2x, q1, q2, q3);
            t1 = ::_mm_xor_si128(t1, q1);
            t2 = ::_mm_xor_si128(t2, q2);
            t3 = ::_mm_xor_si128(t3, q3);
            ghash_product_raw_salted(aw3, ctx.h1w, ctx.h1x, q1, q2, q3);
            t1 = ::_mm_xor_si128(t1, q1);
            t2 = ::_mm_xor_si128(t2, q2);
            t3 = ::_mm_xor_si128(t3, q3);
            simd::vec128_t ignored;
            ghash_product_finish(t1, t2, t3, yw, ignored);
            data += 64;
            len -= 64;
        }
        while (len >= 16) {
            simd::vec128_t aw = ghash_byteswap(::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(data)));
            aw = ::_mm_xor_si128(aw, yw);
            simd::vec128_t t1;
            simd::vec128_t t2;
            simd::vec128_t t3;
            simd::vec128_t ignored;
            ghash_product_raw(aw, ctx.h1w, t1, t2, t3);
            ghash_product_finish(t1, t2, t3, yw, ignored);
            data += 16;
            len -= 16;
        }
        if (len > 0) {
            byte_t padded[16] = {0};
            memory_copy(padded, data, len);
            simd::vec128_t aw = ghash_byteswap(::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(padded)));
            aw = ::_mm_xor_si128(aw, yw);
            simd::vec128_t t1;
            simd::vec128_t t2;
            simd::vec128_t t3;
            simd::vec128_t ignored;
            ghash_product_raw(aw, ctx.h1w, t1, t2, t3);
            ghash_product_finish(t1, t2, t3, yw, ignored);
        }
        const simd::vec128_t raw = ghash_byteswap(yw);
        state_hi = static_cast<uint64_t>(::_mm_cvtsi128_si64(raw));
        state_lo = static_cast<uint64_t>(::_mm_cvtsi128_si64(_mm_srli_si128(raw, 8)));
#else
        uint64_t hi = endian::read_be64(reinterpret_cast<const byte_t*>(&state_hi));
        uint64_t lo = endian::read_be64(reinterpret_cast<const byte_t*>(&state_lo));
        while (len >= 16) {
            hi ^= endian::read_be64(data);
            lo ^= endian::read_be64(data + 8);
            gf128_multiply(hi, lo, ctx.H_hi, ctx.H_lo);
            data += 16;
            len -= 16;
        }
        if (len > 0) {
            byte_t padded[16] = {0};
            memory_copy(padded, data, len);
            hi ^= endian::read_be64(padded);
            lo ^= endian::read_be64(padded + 8);
            gf128_multiply(hi, lo, ctx.H_hi, ctx.H_lo);
        }
        endian::write_be64(reinterpret_cast<byte_t*>(&state_hi), hi);
        endian::write_be64(reinterpret_cast<byte_t*>(&state_lo), lo);
#endif
    }

#if defined(NEFORCE_SIMD_AES_NI) && defined(NEFORCE_SIMD_PCLMUL)
    // Eight AES chains keep both AES pipes busy. GHASH is delayed by one 128-byte batch
    // so that its loads and CLMUL products are independent of the AES chain currently in
    // flight, which lets the two pipes overlap.
    template <bool Encrypt, bool Ghash>
    NEFORCE_OPTIMIZE("O3")
    __NEFORCE_AI
            void gcm_blocks8(const byte_t* input, byte_t* output, const byte_t* ghash_src, simd::vec128_t& counter_rev,
                             const byte_t* expanded_key, const ghash_context& ctx, simd::vec128_t& y) noexcept {
        const simd::vec128_t rk0 = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(expanded_key));
        simd::vec128_t a0 = ::_mm_xor_si128(ghash_byteswap(counter_rev), rk0);
        simd::vec128_t a1 = ::_mm_xor_si128(ghash_byteswap(::_mm_add_epi32(counter_rev, ::_mm_cvtsi32_si128(1))), rk0);
        simd::vec128_t a2 = ::_mm_xor_si128(ghash_byteswap(::_mm_add_epi32(counter_rev, ::_mm_cvtsi32_si128(2))), rk0);
        simd::vec128_t a3 = ::_mm_xor_si128(ghash_byteswap(::_mm_add_epi32(counter_rev, ::_mm_cvtsi32_si128(3))), rk0);
        simd::vec128_t a4 = ::_mm_xor_si128(ghash_byteswap(::_mm_add_epi32(counter_rev, ::_mm_cvtsi32_si128(4))), rk0);
        simd::vec128_t a5 = ::_mm_xor_si128(ghash_byteswap(::_mm_add_epi32(counter_rev, ::_mm_cvtsi32_si128(5))), rk0);
        simd::vec128_t a6 = ::_mm_xor_si128(ghash_byteswap(::_mm_add_epi32(counter_rev, ::_mm_cvtsi32_si128(6))), rk0);
        simd::vec128_t a7 = ::_mm_xor_si128(ghash_byteswap(::_mm_add_epi32(counter_rev, ::_mm_cvtsi32_si128(7))), rk0);

        for (int round = 1; round < 14; ++round) {
            const simd::vec128_t rk = ::_mm_loadu_si128(
                    reinterpret_cast<const simd::vec128_t*>(expanded_key + static_cast<ptrdiff_t>(round * 16)));
            a0 = ::_mm_aesenc_si128(a0, rk);
            a1 = ::_mm_aesenc_si128(a1, rk);
            a2 = ::_mm_aesenc_si128(a2, rk);
            a3 = ::_mm_aesenc_si128(a3, rk);
            a4 = ::_mm_aesenc_si128(a4, rk);
            a5 = ::_mm_aesenc_si128(a5, rk);
            a6 = ::_mm_aesenc_si128(a6, rk);
            a7 = ::_mm_aesenc_si128(a7, rk);
        }
        const simd::vec128_t rk14 = ::_mm_loadu_si128(
                reinterpret_cast<const simd::vec128_t*>(expanded_key + static_cast<ptrdiff_t>(14 * 16)));
        a0 = ::_mm_aesenclast_si128(a0, rk14);
        a1 = ::_mm_aesenclast_si128(a1, rk14);
        a2 = ::_mm_aesenclast_si128(a2, rk14);
        a3 = ::_mm_aesenclast_si128(a3, rk14);
        a4 = ::_mm_aesenclast_si128(a4, rk14);
        a5 = ::_mm_aesenclast_si128(a5, rk14);
        a6 = ::_mm_aesenclast_si128(a6, rk14);
        a7 = ::_mm_aesenclast_si128(a7, rk14);

        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(output),
                           ::_mm_xor_si128(a0, ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(input))));
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(output + 16),
                           ::_mm_xor_si128(a1, ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(input + 16))));
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(output + 32),
                           ::_mm_xor_si128(a2, ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(input + 32))));
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(output + 48),
                           ::_mm_xor_si128(a3, ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(input + 48))));
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(output + 64),
                           ::_mm_xor_si128(a4, ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(input + 64))));
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(output + 80),
                           ::_mm_xor_si128(a5, ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(input + 80))));
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(output + 96),
                           ::_mm_xor_si128(a6, ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(input + 96))));
        ::_mm_storeu_si128(
                reinterpret_cast<simd::vec128_t*>(output + 112),
                ::_mm_xor_si128(a7, ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(input + 112))));

        if (Ghash) {
            y = ghash_blocks8(ctx, y, ghash_src);
        }
        counter_rev = ::_mm_add_epi32(counter_rev, ::_mm_cvtsi32_si128(8));
    }
#endif

#if defined(NEFORCE_SIMD_VAES) && defined(NEFORCE_SIMD_AVX2) && defined(NEFORCE_SIMD_PCLMUL)
    // VAES keeps 256 bits (two blocks) per register, so eight blocks only occupy four
    // AES registers. VPCLMULQDQ evaluates two GHASH products per instruction, which
    // leaves enough register space to run AES and GHASH without spilling.
    NEFORCE_OPTIMIZE("O3") __NEFORCE_AI simd::vec256_t ghash_byteswap256(const simd::vec256_t value) noexcept {
        const simd::vec256_t index = ::_mm256_setr_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 15, 14,
                                                        13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
        return ::_mm256_shuffle_epi8(value, index);
    }

    NEFORCE_OPTIMIZE("O3")
    __NEFORCE_AI void ghash_accumulate_pair(const simd::vec256_t c, const simd::vec256_t hw, const simd::vec256_t hx,
                                            simd::vec256_t& t1, simd::vec256_t& t2, simd::vec256_t& t3) noexcept {
        const simd::vec256_t ah = ::_mm256_xor_si256(c, ::_mm256_shuffle_epi32(c, 0x0E));
        const simd::vec256_t p1 = ::_mm256_clmulepi64_epi128(c, hw, 0x11);
        const simd::vec256_t p3 = ::_mm256_clmulepi64_epi128(c, hw, 0x00);
        const simd::vec256_t p2 =
                ::_mm256_xor_si256(::_mm256_clmulepi64_epi128(ah, hx, 0x00), ::_mm256_xor_si256(p1, p3));
        t1 = ::_mm256_xor_si256(t1, p1);
        t2 = ::_mm256_xor_si256(t2, p2);
        t3 = ::_mm256_xor_si256(t3, p3);
    }

    NEFORCE_OPTIMIZE("O3")
    __NEFORCE_AI simd::vec128_t
            ghash_blocks8_vaes(const ghash_context& ctx, simd::vec128_t y, const byte_t* data) noexcept {
        const simd::vec256_t c01 = ::_mm256_xor_si256(
                ghash_byteswap256(::_mm256_loadu_si256(reinterpret_cast<const simd::vec256_t*>(data))),
                ::_mm256_zextsi128_si256(y));
        const simd::vec256_t c23 =
                ghash_byteswap256(::_mm256_loadu_si256(reinterpret_cast<const simd::vec256_t*>(data + 32)));
        const simd::vec256_t c45 =
                ghash_byteswap256(::_mm256_loadu_si256(reinterpret_cast<const simd::vec256_t*>(data + 64)));
        const simd::vec256_t c67 =
                ghash_byteswap256(::_mm256_loadu_si256(reinterpret_cast<const simd::vec256_t*>(data + 96)));

        simd::vec256_t t1 = ::_mm256_setzero_si256();
        simd::vec256_t t2 = ::_mm256_setzero_si256();
        simd::vec256_t t3 = ::_mm256_setzero_si256();
        ghash_accumulate_pair(c01, ctx.h87w, ctx.h87x, t1, t2, t3);
        ghash_accumulate_pair(c23, ctx.h65w, ctx.h65x, t1, t2, t3);
        ghash_accumulate_pair(c45, ctx.h43w, ctx.h43x, t1, t2, t3);
        ghash_accumulate_pair(c67, ctx.h21w, ctx.h21x, t1, t2, t3);

        simd::vec128_t r1 = ::_mm_xor_si128(::_mm256_castsi256_si128(t1), ::_mm256_extracti128_si256(t1, 1));
        simd::vec128_t r2 = ::_mm_xor_si128(::_mm256_castsi256_si128(t2), ::_mm256_extracti128_si256(t2, 1));
        simd::vec128_t r3 = ::_mm_xor_si128(::_mm256_castsi256_si128(t3), ::_mm256_extracti128_si256(t3, 1));
        simd::vec128_t digest;
        simd::vec128_t ignored;
        ghash_product_finish(r1, r2, r3, digest, ignored);
        return digest;
    }

    template <bool Encrypt, bool Ghash>
    NEFORCE_OPTIMIZE("O3")
    __NEFORCE_AI void gcm_blocks8_vaes(const byte_t* input, byte_t* output, const byte_t* ghash_src,
                                       simd::vec128_t& counter_rev, const byte_t* expanded_key,
                                       const ghash_context& ctx, simd::vec128_t& y) noexcept {
        const simd::vec256_t bswap = ::_mm256_setr_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 15, 14,
                                                        13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
        const simd::vec256_t rev01 =
                ::_mm256_set_m128i(::_mm_add_epi32(counter_rev, ::_mm_cvtsi32_si128(1)), counter_rev);
        const simd::vec256_t step2 = ::_mm256_set_epi32(0, 0, 0, 2, 0, 0, 0, 2);
        const simd::vec256_t step4 = ::_mm256_set_epi32(0, 0, 0, 4, 0, 0, 0, 4);
        const simd::vec256_t step6 = ::_mm256_set_epi32(0, 0, 0, 6, 0, 0, 0, 6);
        const simd::vec256_t rk0 =
                ::_mm256_broadcastsi128_si256(::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(expanded_key)));

        simd::vec256_t a0 = ::_mm256_xor_si256(::_mm256_shuffle_epi8(rev01, bswap), rk0);
        simd::vec256_t a1 = ::_mm256_xor_si256(::_mm256_shuffle_epi8(::_mm256_add_epi32(rev01, step2), bswap), rk0);
        simd::vec256_t a2 = ::_mm256_xor_si256(::_mm256_shuffle_epi8(::_mm256_add_epi32(rev01, step4), bswap), rk0);
        simd::vec256_t a3 = ::_mm256_xor_si256(::_mm256_shuffle_epi8(::_mm256_add_epi32(rev01, step6), bswap), rk0);

        for (int round = 1; round < 14; ++round) {
            const simd::vec256_t rk = ::_mm256_broadcastsi128_si256(::_mm_loadu_si128(
                    reinterpret_cast<const simd::vec128_t*>(expanded_key + static_cast<ptrdiff_t>(round * 16))));
            a0 = ::_mm256_aesenc_epi128(a0, rk);
            a1 = ::_mm256_aesenc_epi128(a1, rk);
            a2 = ::_mm256_aesenc_epi128(a2, rk);
            a3 = ::_mm256_aesenc_epi128(a3, rk);
        }
        const simd::vec256_t rk14 = ::_mm256_broadcastsi128_si256(::_mm_loadu_si128(
                reinterpret_cast<const simd::vec128_t*>(expanded_key + static_cast<ptrdiff_t>(14 * 16))));
        a0 = ::_mm256_aesenclast_epi128(a0, rk14);
        a1 = ::_mm256_aesenclast_epi128(a1, rk14);
        a2 = ::_mm256_aesenclast_epi128(a2, rk14);
        a3 = ::_mm256_aesenclast_epi128(a3, rk14);

        ::_mm256_storeu_si256(
                reinterpret_cast<simd::vec256_t*>(output),
                ::_mm256_xor_si256(a0, ::_mm256_loadu_si256(reinterpret_cast<const simd::vec256_t*>(input))));
        ::_mm256_storeu_si256(
                reinterpret_cast<simd::vec256_t*>(output + 32),
                ::_mm256_xor_si256(a1, ::_mm256_loadu_si256(reinterpret_cast<const simd::vec256_t*>(input + 32))));
        ::_mm256_storeu_si256(
                reinterpret_cast<simd::vec256_t*>(output + 64),
                ::_mm256_xor_si256(a2, ::_mm256_loadu_si256(reinterpret_cast<const simd::vec256_t*>(input + 64))));
        ::_mm256_storeu_si256(
                reinterpret_cast<simd::vec256_t*>(output + 96),
                ::_mm256_xor_si256(a3, ::_mm256_loadu_si256(reinterpret_cast<const simd::vec256_t*>(input + 96))));

        if (Ghash) {
            y = ghash_blocks8_vaes(ctx, y, ghash_src);
        }
        counter_rev = ::_mm_add_epi32(counter_rev, ::_mm_cvtsi32_si128(8));
    }
#endif

#if (defined(NEFORCE_SIMD_VAES) && defined(NEFORCE_SIMD_AVX2) && defined(NEFORCE_SIMD_PCLMUL)) || \
        (defined(NEFORCE_SIMD_AES_NI) && defined(NEFORCE_SIMD_PCLMUL))
    template <bool Encrypt, bool Ghash>
    NEFORCE_OPTIMIZE("O3")
    __NEFORCE_AI void gcm_blocks8_impl(const byte_t* input, byte_t* output, const byte_t* ghash_src,
                                       simd::vec128_t& counter_rev, const byte_t* expanded_key,
                                       const ghash_context& ctx, simd::vec128_t& y) noexcept {
#    if defined(NEFORCE_SIMD_VAES) && defined(NEFORCE_SIMD_AVX2) && defined(NEFORCE_SIMD_PCLMUL)
        gcm_blocks8_vaes<Encrypt, Ghash>(input, output, ghash_src, counter_rev, expanded_key, ctx, y);
#    else
        gcm_blocks8<Encrypt, Ghash>(input, output, ghash_src, counter_rev, expanded_key, ctx, y);
#    endif
    }

    NEFORCE_OPTIMIZE("O3")
    __NEFORCE_AI simd::vec128_t
            ghash_blocks8_impl(const ghash_context& ctx, simd::vec128_t y, const byte_t* data) noexcept {
#    if defined(NEFORCE_SIMD_VAES) && defined(NEFORCE_SIMD_AVX2) && defined(NEFORCE_SIMD_PCLMUL)
        return ghash_blocks8_vaes(ctx, y, data);
#    else
        return ghash_blocks8(ctx, y, data);
#    endif
    }
#endif

    NEFORCE_OPTIMIZE("O3")
    void ghash_final(const ghash_context& ctx, uint64_t& state_hi, uint64_t& state_lo, const size_t a_len,
                     const size_t c_len, byte_t out_tag[16]) noexcept {
        byte_t len_block[16] = {};
        endian::write_be64(len_block, a_len * 8);
        endian::write_be64(len_block + 8, c_len * 8);

        ghash_update(ctx, state_hi, state_lo, len_block, 16);

#ifdef NEFORCE_SIMD_PCLMUL
        endian::write_le64(out_tag, state_hi);
        endian::write_le64(out_tag + 8, state_lo);
#else
        endian::write_be64(out_tag, state_hi);
        endian::write_be64(out_tag + 8, state_lo);
#endif
    }

    NEFORCE_OPTIMIZE("O3") void inc32(byte_t* counter) noexcept {
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
    NEFORCE_OPTIMIZE("O3")
    __NEFORCE_TARGET_APS __NEFORCE_AI void AES256_ctr_keystream_x4_vec(byte_t counter[16], const byte_t* expanded_key,
                                                                       simd::vec128_t out[4]) noexcept {
        const simd::vec128_t rk0 = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(expanded_key));
        simd::vec128_t c0 = ::_mm_xor_si128(::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(counter)), rk0);
        inc32(counter);
        simd::vec128_t c1 = ::_mm_xor_si128(::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(counter)), rk0);
        inc32(counter);
        simd::vec128_t c2 = ::_mm_xor_si128(::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(counter)), rk0);
        inc32(counter);
        simd::vec128_t c3 = ::_mm_xor_si128(::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(counter)), rk0);
        inc32(counter);
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
        out[0] = ::_mm_aesenclast_si128(c0, rk14);
        out[1] = ::_mm_aesenclast_si128(c1, rk14);
        out[2] = ::_mm_aesenclast_si128(c2, rk14);
        out[3] = ::_mm_aesenclast_si128(c3, rk14);
    }

    NEFORCE_OPTIMIZE("O3")
    __NEFORCE_TARGET_APS __NEFORCE_AI void AES256_ctr_keystream_x4(byte_t counter[16], const byte_t* expanded_key,
                                                                   byte_t keystream[64]) noexcept {
        simd::vec128_t out[4];
        AES256_ctr_keystream_x4_vec(counter, expanded_key, out);
        for (int i = 0; i < 4; ++i) {
            ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(keystream + static_cast<ptrdiff_t>(i) * 16), out[i]);
        }
    }
#endif
} // namespace


NEFORCE_OPTIMIZE("O3") byte_vector AES256::encrypt_ecb(const cbyte_view data, const cbyte_view key) {
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

NEFORCE_OPTIMIZE("O3") byte_vector AES256::decrypt_ecb(const cbyte_view data, const cbyte_view key) {
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

NEFORCE_OPTIMIZE("O3") byte_vector AES256::encrypt_ecb_pkcs7(const cbyte_view data, const cbyte_view key) {
    byte_vector byte_data{data.data(), data.size()};
    const byte_t padding = 16 - (byte_data.size() % 16);
    for (byte_t i = 0; i < padding; ++i) {
        byte_data.push_back(padding);
    }
    return encrypt_ecb(byte_data.view(), key);
}

NEFORCE_OPTIMIZE("O3") byte_vector AES256::decrypt_ecb_pkcs7(const cbyte_view data, const cbyte_view key) {
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

NEFORCE_OPTIMIZE("O3") byte_vector AES256::encrypt_cbc(cbyte_view data, cbyte_view key, cbyte_view iv) {
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

#ifdef NEFORCE_SIMD_AES_NI
    simd::vec128_t previous = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(iv.data()));
    for (size_t i = 0; i < data.size(); i += 16) {
        simd::vec128_t state =
                ::_mm_xor_si128(::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(data.data() + i)), previous);
        state = ::_mm_xor_si128(state, ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(expanded_key)));
        for (int round = 1; round < 14; ++round) {
            state = ::_mm_aesenc_si128(state, ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(
                                                      expanded_key + static_cast<ptrdiff_t>(round * 16))));
        }
        state = ::_mm_aesenclast_si128(state, ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(
                                                      expanded_key + static_cast<ptrdiff_t>(14 * 16))));
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(result.data() + i), state);
        previous = state;
    }
#else
    byte_t previous[16];
    memory_copy(previous, iv.data(), 16);
    for (size_t i = 0; i < data.size(); i += 16) {
        byte_t block[16];
        memory_copy(block, data.data() + i, 16);
        xor_block(block, previous);
        encrypt_block(result.data() + i, block, expanded_key);
        memory_copy(previous, result.data() + i, 16);
    }
#endif
    return result;
}

NEFORCE_OPTIMIZE("O3") byte_vector AES256::decrypt_cbc(cbyte_view data, cbyte_view key, cbyte_view iv) {
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

NEFORCE_OPTIMIZE("O3") byte_vector AES256::encrypt_cbc_pkcs7(cbyte_view data, cbyte_view key, cbyte_view iv) {
    byte_vector padded{data.data(), data.size()};
    const byte_t padding = 16 - (padded.size() % 16);
    for (byte_t i = 0; i < padding; ++i) {
        padded.push_back(padding);
    }
    return encrypt_cbc(padded.view(), key, iv);
}

NEFORCE_OPTIMIZE("O3") byte_vector AES256::decrypt_cbc_pkcs7(cbyte_view data, cbyte_view key, cbyte_view iv) {
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

NEFORCE_OPTIMIZE("O3")
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

    uint64_t state_hi = 0, state_lo = 0;
    ghash_update(gh_ctx, state_hi, state_lo, aad.data(), aad.size());

    size_t i = 0;
#if (defined(NEFORCE_SIMD_VAES) && defined(NEFORCE_SIMD_AVX2) && defined(NEFORCE_SIMD_PCLMUL)) || \
        (defined(NEFORCE_SIMD_AES_NI) && defined(NEFORCE_SIMD_PCLMUL))
    if (data.size() >= 128) {
        simd::vec128_t y =
                ghash_byteswap(::_mm_set_epi64x(static_cast<int64_t>(state_lo), static_cast<int64_t>(state_hi)));
        simd::vec128_t counter_rev =
                ghash_byteswap(::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(counter)));
        const size_t batches = data.size() / 128;
        gcm_blocks8_impl<true, false>(data.data(), ciphertext.data(), nullptr, counter_rev, expanded_key, gh_ctx, y);
        for (i = 128; i + 128 <= data.size(); i += 128) {
            gcm_blocks8_impl<true, true>(data.data() + i, ciphertext.data() + i, ciphertext.data() + i - 128,
                                         counter_rev, expanded_key, gh_ctx, y);
        }
        y = ghash_blocks8_impl(gh_ctx, y, ciphertext.data() + (batches - 1) * 128);
        i = batches * 128;
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(counter), ghash_byteswap(counter_rev));
        const simd::vec128_t raw = ghash_byteswap(y);
        state_hi = static_cast<uint64_t>(::_mm_cvtsi128_si64(raw));
        state_lo = static_cast<uint64_t>(::_mm_cvtsi128_si64(_mm_srli_si128(raw, 8)));
    }
#endif
#ifdef NEFORCE_SIMD_AES_NI
    for (; i + 64 <= data.size(); i += 64) {
        byte_t keystream[64];
        AES256_ctr_keystream_x4(counter, expanded_key, keystream);
        xor_buf(ciphertext.data() + i, data.data() + i, keystream, 64);
        ghash_update(gh_ctx, state_hi, state_lo, ciphertext.data() + i, 64);
    }
#endif
    for (; i < data.size(); i += 16) {
        byte_t keystream[16];
        memory_copy(keystream, counter, 16);
        encrypt_block(keystream, keystream, expanded_key);

        const size_t block_len = min<size_t>(16, data.size() - i);
        xor_buf(ciphertext.data() + i, data.data() + i, keystream, block_len);
        inc32(counter);
#ifdef NEFORCE_SIMD_AES_NI
        ghash_update(gh_ctx, state_hi, state_lo, ciphertext.data() + i, block_len);
#endif
    }
#ifndef NEFORCE_SIMD_AES_NI
    ghash_update(gh_ctx, state_hi, state_lo, ciphertext.data(), ciphertext.size());
#endif

    byte_t full_tag[16];
    ghash_final(gh_ctx, state_hi, state_lo, aad.size(), ciphertext.size(), full_tag);

    byte_t enc_J0[16];
    memory_copy(enc_J0, J0, 16);
    encrypt_block(enc_J0, enc_J0, expanded_key);
    xor_block(full_tag, enc_J0);

    memory_copy(tag, full_tag, tag_len);
    return ciphertext;
}

NEFORCE_OPTIMIZE("O3")
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

    byte_vector plaintext(data.size());
    byte_t counter[16];
    memory_copy(counter, J0, 16);
    inc32(counter);

    uint64_t state_hi = 0, state_lo = 0;
    ghash_update(gh_ctx, state_hi, state_lo, aad.data(), aad.size());

    size_t i = 0;
#if (defined(NEFORCE_SIMD_VAES) && defined(NEFORCE_SIMD_AVX2) && defined(NEFORCE_SIMD_PCLMUL)) || \
        (defined(NEFORCE_SIMD_AES_NI) && defined(NEFORCE_SIMD_PCLMUL))
    if (data.size() >= 128) {
        simd::vec128_t y =
                ghash_byteswap(::_mm_set_epi64x(static_cast<int64_t>(state_lo), static_cast<int64_t>(state_hi)));
        simd::vec128_t counter_rev =
                ghash_byteswap(::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(counter)));
        const size_t batches = data.size() / 128;
        gcm_blocks8_impl<false, false>(data.data(), plaintext.data(), nullptr, counter_rev, expanded_key, gh_ctx, y);
        for (i = 128; i + 128 <= data.size(); i += 128) {
            gcm_blocks8_impl<false, true>(data.data() + i, plaintext.data() + i, data.data() + i - 128, counter_rev,
                                          expanded_key, gh_ctx, y);
        }
        y = ghash_blocks8_impl(gh_ctx, y, data.data() + (batches - 1) * 128);
        i = batches * 128;
        ::_mm_storeu_si128(reinterpret_cast<simd::vec128_t*>(counter), ghash_byteswap(counter_rev));
        const simd::vec128_t raw = ghash_byteswap(y);
        state_hi = static_cast<uint64_t>(::_mm_cvtsi128_si64(raw));
        state_lo = static_cast<uint64_t>(::_mm_cvtsi128_si64(_mm_srli_si128(raw, 8)));
    }
#endif
#ifdef NEFORCE_SIMD_AES_NI
    for (; i + 64 <= data.size(); i += 64) {
        byte_t keystream[64];
        AES256_ctr_keystream_x4(counter, expanded_key, keystream);
        xor_buf(plaintext.data() + i, data.data() + i, keystream, 64);
        ghash_update(gh_ctx, state_hi, state_lo, data.data() + i, 64);
    }
#endif
    for (; i < data.size(); i += 16) {
        byte_t keystream[16];
        memory_copy(keystream, counter, 16);
        encrypt_block(keystream, keystream, expanded_key);

        const size_t block_len = min<size_t>(16, data.size() - i);
        xor_buf(plaintext.data() + i, data.data() + i, keystream, block_len);
        inc32(counter);
#ifdef NEFORCE_SIMD_AES_NI
        ghash_update(gh_ctx, state_hi, state_lo, data.data() + i, block_len);
#endif
    }
#ifndef NEFORCE_SIMD_AES_NI
    ghash_update(gh_ctx, state_hi, state_lo, data.data(), data.size());
#endif

    byte_t expected_tag[16];
    ghash_final(gh_ctx, state_hi, state_lo, aad.size(), data.size(), expected_tag);

    byte_t enc_J0[16];
    memory_copy(enc_J0, J0, 16);
    encrypt_block(enc_J0, enc_J0, expanded_key);
    xor_block(expected_tag, enc_J0);

    if (secure_compare(expected_tag, tag.data(), tag_len)) {
        NEFORCE_THROW_EXCEPTION(value_exception("GCM authentication failed"));
    }

    return plaintext;
}

NEFORCE_END_NAMESPACE__
