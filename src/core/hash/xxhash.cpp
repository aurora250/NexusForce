#include <NeForce/core/hash/xxhash.hpp>
#include <NeForce/core/numeric/int128.hpp>
NEFORCE_BEGIN_NAMESPACE__

#ifdef NEFORCE_COMPILER_MSVC
#    pragma warning(push)
#    pragma warning(disable : 26819)
#endif

namespace {
    constexpr uint64_t XXH3_PRIME64_1 = 0x9E3779B185EBCA87ULL;
    constexpr uint64_t XXH3_PRIME64_2 = 0xC2B2AE3D27D4EB4FULL;
    constexpr uint64_t XXH3_PRIME64_3 = 0x165667B19E3779F9ULL;
    constexpr uint64_t XXH3_PRIME64_4 = 0x85EBCA77C2B2AE63ULL;
    NEFORCE_UNUSED constexpr uint64_t XXH3_PRIME64_5 = 0x27D4EB2F165667C5ULL;
    constexpr size_t XXH3_SECRET_DEFAULT_SIZE = 192;
    constexpr size_t XXH3_STRIPE_LEN = 64;
    constexpr size_t XXH3_SECRET_CONSUME_RATE = 8;
    constexpr size_t XXH3_ACC_NB = 8;
    constexpr size_t XXH3_MIDSIZE_STARTOFFSET = 3;
    constexpr size_t XXH3_SECRET_LASTACC_START = 7;
    constexpr size_t XXH3_SECRET_MERGEACCS_START = 11;

    constexpr size_t XXH3_SECRET_SIZE_WITH_PADDING =
            XXH3_SECRET_DEFAULT_SIZE +
            XXH3_STRIPE_LEN * ((XXH3_SECRET_DEFAULT_SIZE - XXH3_STRIPE_LEN) / XXH3_SECRET_CONSUME_RATE + 1);

    constexpr uint8_t XXH3_kSecret[XXH3_SECRET_SIZE_WITH_PADDING] = {
            0xb8, 0xfe, 0x6c, 0x39, 0x23, 0xa4, 0x4b, 0xbe, 0x7c, 0x01, 0x81, 0x2c, 0xf7, 0x21, 0xad, 0x1c, 0xde, 0xd4,
            0x6d, 0xe9, 0x83, 0x90, 0x97, 0xdb, 0x72, 0x40, 0xa4, 0xa4, 0xb7, 0xb3, 0x67, 0x1f, 0xcb, 0x4a, 0x8c, 0x4f,
            0xc9, 0x71, 0x0e, 0x74, 0x41, 0xfe, 0xb7, 0xec, 0xca, 0x23, 0x44, 0xdb, 0x01, 0x19, 0x92, 0x2c, 0x7f, 0x3a,
            0x64, 0x62, 0x28, 0x99, 0xb3, 0x32, 0x4c, 0x1f, 0x20, 0x02, 0x56, 0x27, 0x48, 0x37, 0x6e, 0x33, 0xee, 0x58,
            0x11, 0x4d, 0xf7, 0x79, 0x88, 0x84, 0xde, 0x56, 0xe8, 0xb6, 0x6f, 0x85, 0xbe, 0xd9, 0x17, 0x50, 0x53, 0x5f,
            0x60, 0xd7, 0xa6, 0x06, 0x04, 0x75, 0xcf, 0x82, 0x6b, 0x50, 0x00, 0x17, 0x6f, 0xce, 0x99, 0xb6, 0x3f, 0xe2,
            0xad, 0x13, 0x11, 0x09, 0x56, 0x21, 0x28, 0xc7, 0xaa, 0x97, 0xde, 0x50, 0x00, 0xcf, 0x2b, 0xd5, 0xb1, 0x02,
            0x14, 0x67, 0x2c, 0x93, 0x93, 0x2c, 0x91, 0x00, 0x01, 0x0b, 0x7c, 0xc4, 0xed, 0xfb, 0x3b, 0x81, 0x25, 0x28,
            0x74, 0x3a, 0x1b, 0x02, 0xd7, 0xa1, 0x10, 0x04, 0x33, 0x89, 0x56, 0x3e, 0x19, 0xbc, 0x3b, 0xcf, 0x60, 0x03,
            0x75, 0xf4, 0x09, 0x88, 0x3e, 0xe3, 0xd1, 0x00, 0x30, 0x46, 0x0a, 0x36, 0x21, 0x49, 0x33, 0x28, 0x50, 0xa8,
            0x93, 0xb0, 0x15, 0x53, 0xba, 0x5b, 0x65, 0xa9, 0xd8, 0xc2, 0x87, 0x3d,
    };

    uint64_t XXH3_avalanche(uint64_t h) noexcept {
        h ^= h >> 37;
        h *= 0x165667919E3779F9ULL;
        h ^= h >> 32;
        return h;
    }

    uint64_t XXH3_rrmxmx(uint64_t h, const uint64_t len) noexcept {
        h ^= rotate_l64(h, 49) ^ rotate_l64(h, 24);
        h *= 0x9FB21C651E98DF25ULL;
        h ^= (h >> 35) + len;
        h *= 0x9FB21C651E98DF25ULL;
        h ^= (h >> 28);
        return h;
    }

    uint64_t XXH3_mix16B(const byte_t* data, const byte_t* secret, const uint64_t seed) noexcept {
        const uint64_t lo = endian::read_le64(data + 0);
        const uint64_t hi = endian::read_le64(data + 8);
        const uint64_t sl = endian::read_le64(secret + 0) ^ seed;
        const uint64_t sh = endian::read_le64(secret + 8) ^ seed;
        return XXH3_rrmxmx(rotate_l64(lo ^ sl, 35) + rotate_l64(hi ^ sh, 3), 16);
    }

    uint64_t XXH3_mul128_fold64(const uint64_t lhs, const uint64_t rhs) noexcept {
        const uint128_t product = uint128_t::mul64(lhs, rhs);
        return product.hi ^ product.lo;
    }

    uint64_t XXH3_len_1to3(const byte_t* data, const size_t len, const byte_t* secret, const uint64_t seed) noexcept {
        const uint8_t c1 = data[0];
        const uint8_t c2 = data[len >> 1];
        const uint8_t c3 = data[len - 1];
        const uint32_t combined = (static_cast<uint32_t>(c1) << 16) | (static_cast<uint32_t>(c2) << 24) |
                                  static_cast<uint32_t>(c3) | (static_cast<uint32_t>(len) << 8);
        const uint64_t bitflip = (static_cast<uint64_t>(endian::read_le32(secret)) ^
                                  static_cast<uint64_t>(endian::read_le32(secret + 4))) +
                                 seed;
        const uint64_t keyed = static_cast<uint64_t>(combined) ^ bitflip;
        return XXH3_avalanche(keyed);
    }

    uint64_t XXH3_len_4to8(const byte_t* data, const size_t len, const byte_t* secret, const uint64_t seed) noexcept {
        const uint64_t seed1 = seed ^ (static_cast<uint64_t>(endian::swap_endian(static_cast<uint32_t>(seed))) << 32);
        const uint32_t input1 = endian::read_le32(data);
        const uint32_t input2 = endian::read_le32(data + len - 4);
        const uint64_t bitflip = (endian::read_le64(secret + 8) ^ endian::read_le64(secret + 16)) - seed1;
        const uint64_t input64 = static_cast<uint64_t>(input2) + (static_cast<uint64_t>(input1) << 32);
        const uint64_t keyed = input64 ^ bitflip;
        return XXH3_rrmxmx(keyed, static_cast<uint64_t>(len));
    }

    uint64_t XXH3_len_9to16(const byte_t* data, const size_t len, const byte_t* secret, const uint64_t seed) noexcept {
        const uint64_t bitflipl = (endian::read_le64(secret + 32) ^ endian::read_le64(secret + 40)) - seed;
        const uint64_t bitfliph = (endian::read_le64(secret + 48) ^ endian::read_le64(secret + 56)) + seed;
        const uint64_t input_lo = endian::read_le64(data);
        const uint64_t input_hi = endian::read_le64(data + len - 8);
        uint64_t acc =
                XXH3_mul128_fold64(input_lo ^ (endian::read_le64(secret + 24) ^ endian::read_le64(secret + 32)),
                                   input_hi ^ (endian::read_le64(secret + 40) ^ endian::read_le64(secret + 48))) ^
                ((XXH3_len_1to3(data, len, secret, seed)) ^ (input_lo + input_hi)) * XXH3_PRIME64_1;
        acc += (static_cast<uint64_t>(len) - 1) * XXH3_PRIME64_2;
        acc = XXH3_avalanche(acc ^ (acc >> 23));
        acc ^= bitfliph;
        acc += XXH3_mul128_fold64(input_lo ^ bitflipl, input_hi);
        return XXH3_avalanche(acc);
    }

    uint64_t XXH3_len_0to16(const byte_t* data, const size_t len, const byte_t* secret, const uint64_t seed) noexcept {
        if (len > 8) {
            return XXH3_len_9to16(data, len, secret, seed);
        }
        if (len >= 4) {
            return XXH3_len_4to8(data, len, secret, seed);
        }
        if (len > 0) {
            return XXH3_len_1to3(data, len, secret, seed);
        }
        const uint64_t bitflipl = endian::read_le64(secret + 56) ^ endian::read_le64(secret + 64);
        const uint64_t bitfliph = endian::read_le64(secret + 72) ^ endian::read_le64(secret + 80);
        return XXH3_avalanche(seed ^ bitflipl ^ bitfliph);
    }

    uint64_t XXH3_len_17to128(const byte_t* data, const size_t len, const byte_t* secret, const size_t secret_size,
                              const uint64_t seed) noexcept {
        uint64_t acc = static_cast<uint64_t>(len) * XXH3_PRIME64_1;
        if (len > 32) {
            if (len > 64) {
                if (len > 96) {
                    acc += XXH3_mix16B(data + 48, secret + 96, seed);
                    acc += XXH3_mix16B(data + len - 64, secret + 112, seed);
                }
                acc += XXH3_mix16B(data + 32, secret + 64, seed);
                acc += XXH3_mix16B(data + len - 48, secret + 80, seed);
            }
            acc += XXH3_mix16B(data + 16, secret + 32, seed);
            acc += XXH3_mix16B(data + len - 32, secret + 48, seed);
        }
        acc += XXH3_mix16B(data, secret, seed);
        acc += XXH3_mix16B(data + len - 16, secret + secret_size - XXH3_MIDSIZE_STARTOFFSET, seed);
        return XXH3_avalanche(acc);
    }

    void XXH3_accumulate_512(uint64_t* acc, const byte_t* data, const byte_t* secret) noexcept {
#if defined(NEFORCE_SIMD_AVX2)
        for (size_t i = 0; i < 8; i += 4) {
            const ::__m256i data_vec = ::_mm256_loadu_si256(reinterpret_cast<const ::__m256i*>(data + i * 8));
            const ::__m256i key_vec = ::_mm256_xor_si256(
                    data_vec, ::_mm256_loadu_si256(reinterpret_cast<const ::__m256i*>(secret + i * 8)));

            const auto swapped = _mm256_permute4x64_epi64(data_vec, _MM_SHUFFLE(2, 3, 0, 1));
            ::__m256i acc_vec = ::_mm256_loadu_si256(reinterpret_cast<const ::__m256i*>(acc + i));
            acc_vec = ::_mm256_add_epi64(acc_vec, swapped);

            const ::__m256i product = ::_mm256_mul_epu32(key_vec, ::_mm256_srli_epi64(key_vec, 32));
            acc_vec = ::_mm256_add_epi64(acc_vec, product);

            ::_mm256_storeu_si256(reinterpret_cast<::__m256i*>(acc + i), acc_vec);
        }
#elif defined(NEFORCE_SIMD_SSE2)
        for (size_t i = 0; i < 8; i += 2) {
            ::__m128i data_vec = ::_mm_loadu_si128(reinterpret_cast<const ::__m128i*>(data + i * 8));
            ::__m128i key_vec =
                    ::_mm_xor_si128(data_vec, ::_mm_loadu_si128(reinterpret_cast<const ::__m128i*>(secret + i * 8)));

            ::__m128i swapped = _mm_shuffle_epi32(data_vec, _MM_SHUFFLE(1, 0, 3, 2));
            ::__m128i acc_vec = ::_mm_loadu_si128(reinterpret_cast<const ::__m128i*>(acc + i));
            acc_vec = ::_mm_add_epi64(acc_vec, swapped);

            ::__m128i product = ::_mm_mul_epu32(key_vec, ::_mm_srli_epi64(key_vec, 32));
            acc_vec = ::_mm_add_epi64(acc_vec, product);

            ::_mm_storeu_si128(reinterpret_cast<::__m128i*>(acc + i), acc_vec);
        }
#elif defined(NEFORCE_SIMD_NEON)
#    if defined(NEFORCE_PLATFORM_MACOS)
        using neon_u64 = unsigned long long;
#    else
        using neon_u64 = uint64_t;
#    endif
        for (size_t i = 0; i < 8; i += 2) {
            const ::uint64x2_t data_vec = vld1q_u64(reinterpret_cast<const neon_u64*>(data + i * 8));
            const ::uint64x2_t key_vec =
                    ::veorq_u64(data_vec, vld1q_u64(reinterpret_cast<const neon_u64*>(secret + i * 8)));

            const ::uint64x2_t swapped = vextq_u64(data_vec, data_vec, 1);
            ::uint64x2_t acc_vec = vld1q_u64(reinterpret_cast<const neon_u64*>(acc + i));
            acc_vec = ::vaddq_u64(acc_vec, swapped);

            const ::uint32x4_t key_u32 = ::vreinterpretq_u32_u64(key_vec);
            const ::uint64x2_t product = ::vmull_u32(::vget_low_u32(key_u32), ::vget_high_u32(key_u32));
            acc_vec = ::vaddq_u64(acc_vec, product);

            vst1q_u64(reinterpret_cast<neon_u64*>(acc + i), acc_vec);
        }
#else
        for (size_t i = 0; i < 8; ++i) {
            const size_t offset = i * 8;
            const uint64_t data_val = endian::read_le64(data + offset);
            const uint64_t data_key = data_val ^ endian::read_le64(secret + offset);
            acc[i ^ 1] += data_val;
            acc[i] += static_cast<uint32_t>(data_key) * static_cast<uint64_t>(data_key >> 32);
        }
#endif
    }

    void XXH3_scrambleAcc(uint64_t* acc, const byte_t* secret) noexcept {
        for (size_t i = 0; i < XXH3_ACC_NB; ++i) {
            const uint64_t key = endian::read_le64(secret + i * 8);
            acc[i] ^= acc[i] >> 47;
            acc[i] ^= key;
            acc[i] *= XXH3_PRIME64_1;
        }
    }

    void XXH3_accumulate(uint64_t* acc, const byte_t* data, const byte_t* secret, const size_t nbStripes) noexcept {
        for (size_t s = 0; s < nbStripes; ++s) {
            const byte_t* const data_stripe = data + s * XXH3_STRIPE_LEN;
            for (size_t j = 0; j < XXH3_STRIPE_LEN; j += XXH3_ACC_NB * 8) {
                const size_t secret_offset = (s * XXH3_STRIPE_LEN + j) % XXH3_SECRET_DEFAULT_SIZE;
                XXH3_accumulate_512(acc, data_stripe + j, secret + secret_offset);
            }
        }
    }

    uint64_t XXH3_mergeAccs(const uint64_t* acc, const byte_t* secret, const uint64_t start) noexcept {
        uint64_t result = start;
        for (size_t i = 0; i < XXH3_ACC_NB; ++i) {
            const uint64_t data_val = acc[i] ^ endian::read_le64(secret + i * 8);
            result = XXH3_rrmxmx(rotate_l64(data_val, 11) + result, 4);
        }
        return XXH3_avalanche(result);
    }

    uint64_t XXH3_hashLong_64b(const byte_t* data, const size_t len, const byte_t* secret,
                               const size_t secret_size) noexcept {
        uint64_t acc[XXH3_ACC_NB] = {
                XXH3_PRIME64_3, XXH3_PRIME64_1, XXH3_PRIME64_2, XXH3_PRIME64_4,
                XXH3_PRIME64_1, XXH3_PRIME64_2, XXH3_PRIME64_3, XXH3_PRIME64_4,
        };
        const size_t nbStripesPerBlock = (secret_size - XXH3_STRIPE_LEN) / XXH3_SECRET_CONSUME_RATE;
        const size_t block_len = XXH3_STRIPE_LEN * nbStripesPerBlock;
        const size_t nb_blocks = (len - 1) / block_len;

        for (size_t b = 0; b < nb_blocks; ++b) {
            XXH3_accumulate(acc, data + b * block_len, secret, nbStripesPerBlock);
            XXH3_scrambleAcc(acc, secret + secret_size - XXH3_STRIPE_LEN);
        }

        const size_t last_stripe_count = ((len - 1) - (block_len * nb_blocks)) / XXH3_STRIPE_LEN;
        XXH3_accumulate(acc, data + nb_blocks * block_len, secret, last_stripe_count);

        const byte_t* const p = data + len - XXH3_STRIPE_LEN;
        XXH3_accumulate_512(acc, p, secret + secret_size - XXH3_STRIPE_LEN - XXH3_SECRET_LASTACC_START);

        byte_t secret_result[XXH3_SECRET_DEFAULT_SIZE];
        constexpr size_t converge_nbStripes = (XXH3_SECRET_DEFAULT_SIZE - XXH3_STRIPE_LEN) / XXH3_SECRET_CONSUME_RATE;
        constexpr size_t converge_block_len = XXH3_STRIPE_LEN * converge_nbStripes;
        XXH3_accumulate(reinterpret_cast<uint64_t*>(secret_result), secret, secret, converge_nbStripes);
        XXH3_accumulate_512(reinterpret_cast<uint64_t*>(secret_result), secret + converge_block_len - XXH3_STRIPE_LEN,
                            secret + secret_size - XXH3_STRIPE_LEN - XXH3_SECRET_LASTACC_START);

        return XXH3_mergeAccs(acc, secret_result + XXH3_SECRET_MERGEACCS_START,
                              static_cast<uint64_t>(len) * XXH3_PRIME64_1);
    }
} // namespace


uint64_t XXH3_64(const void* data, const size_t len) noexcept {
    const auto* input = static_cast<const byte_t*>(data);
    const byte_t* secret = XXH3_kSecret;
    constexpr size_t secret_size = XXH3_SECRET_DEFAULT_SIZE;
    constexpr uint64_t seed = 0;

    if (len <= 16) {
        return XXH3_len_0to16(input, len, secret, seed);
    }
    if (len <= 128) {
        return XXH3_len_17to128(input, len, secret, secret_size, seed);
    }
    return XXH3_hashLong_64b(input, len, secret, secret_size);
}

#ifdef NEFORCE_COMPILER_MSVC
#    pragma warning(pop)
#endif

NEFORCE_END_NAMESPACE__
