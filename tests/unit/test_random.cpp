#include <NeForce/core/container/vector.hpp>
#include <NeForce/core/numeric/random.hpp>
#include <gtest/gtest.h>
// test standard random generator
#include <random>
using namespace neforce;

TEST(LemireBoundedTest, ResultInRange) {
    uint64_t counter = 0;
    auto gen = [&counter]() noexcept -> uint64_t { return counter++; };
    for (uint64_t max = 2; max <= 10000; ++max) {
        for (int i = 0; i < 100; ++i) {
            uint64_t v = lemire_bounded(gen, max);
            EXPECT_LT(v, max);
        }
    }
}

TEST(LemireBoundedTest, MaxOneReturnsZero) {
    uint64_t counter = 0;
    auto gen = [&counter]() noexcept -> uint64_t { return counter++; };
    EXPECT_EQ(lemire_bounded(gen, 1), 0u);
}

TEST(LemireBoundedTest, MaxZeroReturnsZero) {
    uint64_t counter = 0;
    auto gen = [&counter]() noexcept -> uint64_t { return counter++; };
    EXPECT_EQ(lemire_bounded(gen, 0), 0u);
}

class RandomLcdTest : public ::testing::Test {
protected:
    random_lcd default_rng;
    random_lcd seeded_rng{12345u};
};

TEST_F(RandomLcdTest, NextIntMaxBoundary) {
    EXPECT_EQ(default_rng.next_int(0), 0);
    EXPECT_EQ(default_rng.next_int(1), 0);
    EXPECT_EQ(default_rng.next_int(-5), 0);
    for (int i = 0; i < 100; ++i) {
        int val = default_rng.next_int(5);
        EXPECT_GE(val, 0);
        EXPECT_LT(val, 5);
    }
}

TEST_F(RandomLcdTest, NextIntMinMax) {
    for (int i = 0; i < 100; ++i) {
        int val = default_rng.next_int(10, 20);
        EXPECT_GE(val, 10);
        EXPECT_LT(val, 20);
    }
    EXPECT_EQ(default_rng.next_int(20, 10), 20);
    EXPECT_EQ(default_rng.next_int(5, 5), 5);
}

TEST_F(RandomLcdTest, NextIntFullRange) {
    for (int i = 0; i < 100; ++i) {
        int val = default_rng.next_int<int>();
        EXPECT_GE(val, numeric_traits<int>::min());
        EXPECT_LE(val, numeric_traits<int>::max());
    }
    for (int i = 0; i < 100; ++i) {
        unsigned int val = default_rng.next_int<unsigned int>();
        EXPECT_LE(val, numeric_traits<unsigned int>::max());
    }
    for (int i = 0; i < 100; ++i) {
        short val = default_rng.next_int<short>();
        EXPECT_GE(val, numeric_traits<short>::min());
        EXPECT_LE(val, numeric_traits<short>::max());
    }
    for (int i = 0; i < 100; ++i) {
        int64_t val = default_rng.next_int<int64_t>();
        EXPECT_GE(val, numeric_traits<int64_t>::min());
        EXPECT_LE(val, numeric_traits<int64_t>::max());
    }
    for (int i = 0; i < 100; ++i) {
        uint64_t val = default_rng.next_int<uint64_t>();
        EXPECT_LE(val, numeric_traits<uint64_t>::max());
    }
}

TEST_F(RandomLcdTest, NextUint64Max) {
    EXPECT_EQ(default_rng.next_uint64(0), 0u);
    EXPECT_EQ(default_rng.next_uint64(1), 0u);
    for (int i = 0; i < 100; ++i) {
        uint64_t val = default_rng.next_uint64(1000);
        EXPECT_LT(val, 1000u);
    }
}

TEST_F(RandomLcdTest, NextUint64Full) {
    for (int i = 0; i < 100; ++i) {
        uint64_t val = default_rng.next_uint64();
        EXPECT_LE(val, numeric_traits<uint64_t>::max());
    }
}

TEST_F(RandomLcdTest, NextFloat) {
    for (int i = 0; i < 100; ++i) {
        float val = default_rng.next_float<float>();
        EXPECT_GE(val, 0.0f);
        EXPECT_LE(val, 1.0f);
    }
    for (int i = 0; i < 100; ++i) {
        double val = default_rng.next_float<double>();
        EXPECT_GE(val, 0.0);
        EXPECT_LE(val, 1.0);
    }
}

TEST_F(RandomLcdTest, NextFloatMinMax) {
    for (int i = 0; i < 100; ++i) {
        double val = default_rng.next_float(1.5, 3.5);
        EXPECT_GE(val, 1.5);
        EXPECT_LE(val, 3.5);
    }
    EXPECT_EQ(default_rng.next_float(3.0, 1.0), 3.0);
    EXPECT_EQ(default_rng.next_float(5.0, 5.0), 5.0);
}

TEST_F(RandomLcdTest, NextFloatMax) {
    for (int i = 0; i < 100; ++i) {
        double val = default_rng.next_float(10.0);
        EXPECT_GE(val, 0.0);
        EXPECT_LE(val, 10.0);
    }
}

TEST_F(RandomLcdTest, Reproducibility) {
    random_lcd rng1(9999);
    random_lcd rng2(9999);
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(rng1.next_int<int>(), rng2.next_int<int>());
        EXPECT_EQ(rng1.next_uint64(), rng2.next_uint64());
        EXPECT_DOUBLE_EQ(rng1.next_float<double>(), rng2.next_float<double>());
    }
}

TEST_F(RandomLcdTest, DifferentSeedsDiverge) {
    random_lcd rng1(1111);
    random_lcd rng2(2222);
    vector<int> seq1;
    vector<int> seq2;
    for (int i = 0; i < 20; ++i) {
        seq1.push_back(rng1.next_int<int>());
        seq2.push_back(rng2.next_int<int>());
    }
    bool identical = true;
    for (size_t i = 0; i < seq1.size(); ++i) {
        if (seq1[i] != seq2[i]) {
            identical = false;
            break;
        }
    }
    EXPECT_FALSE(identical);
}

class RandomMtTest : public ::testing::Test {
protected:
    random_mt default_rng;
    random_mt seeded_rng{12345u};
};

TEST_F(RandomMtTest, SetSeedReproducibility) {
    random_mt rng1;
    rng1.set_seed(5555);
    random_mt rng2(5555);
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(rng1.next_int<int>(), rng2.next_int<int>());
    }
}

TEST_F(RandomMtTest, NextIntMaxBoundary) {
    EXPECT_EQ(default_rng.next_int(0), 0);
    EXPECT_EQ(default_rng.next_int(1), 0);
    for (int i = 0; i < 100; ++i) {
        int val = default_rng.next_int(7);
        EXPECT_GE(val, 0);
        EXPECT_LT(val, 7);
    }
}

TEST_F(RandomMtTest, NextIntMinMax) {
    for (int i = 0; i < 100; ++i) {
        int val = default_rng.next_int(10, 30);
        EXPECT_GE(val, 10);
        EXPECT_LT(val, 30);
    }
    EXPECT_EQ(default_rng.next_int(30, 10), 30);
    EXPECT_EQ(default_rng.next_int(5, 5), 5);
}

TEST_F(RandomMtTest, NextIntFullRange) {
    for (int i = 0; i < 100; ++i) {
        int val = default_rng.next_int<int>();
        EXPECT_GE(val, numeric_traits<int>::min());
        EXPECT_LE(val, numeric_traits<int>::max());
    }
    for (int i = 0; i < 100; ++i) {
        unsigned int val = default_rng.next_int<unsigned int>();
        EXPECT_LE(val, numeric_traits<unsigned int>::max());
    }
    for (int i = 0; i < 100; ++i) {
        short val = default_rng.next_int<short>();
        EXPECT_GE(val, numeric_traits<short>::min());
        EXPECT_LE(val, numeric_traits<short>::max());
    }
    for (int i = 0; i < 100; ++i) {
        int64_t val = default_rng.next_int<int64_t>();
        EXPECT_GE(val, numeric_traits<int64_t>::min());
        EXPECT_LE(val, numeric_traits<int64_t>::max());
    }
    for (int i = 0; i < 100; ++i) {
        uint64_t val = default_rng.next_int<uint64_t>();
        EXPECT_LE(val, numeric_traits<uint64_t>::max());
    }
}

TEST_F(RandomMtTest, NextUint64Max) {
    EXPECT_EQ(default_rng.next_uint64(0), 0u);
    EXPECT_EQ(default_rng.next_uint64(1), 0u);
    for (int i = 0; i < 100; ++i) {
        uint64_t val = default_rng.next_uint64(5000);
        EXPECT_LT(val, 5000u);
    }
}

TEST_F(RandomMtTest, NextUint64Full) {
    for (int i = 0; i < 100; ++i) {
        uint64_t val = default_rng.next_uint64();
        EXPECT_LE(val, numeric_traits<uint64_t>::max());
    }
}

TEST_F(RandomMtTest, NextFloat) {
    for (int i = 0; i < 100; ++i) {
        float val = default_rng.next_float<float>();
        EXPECT_GE(val, 0.0f);
        EXPECT_LE(val, 1.0f);
    }
    for (int i = 0; i < 100; ++i) {
        double val = default_rng.next_float<double>();
        EXPECT_GE(val, 0.0);
        EXPECT_LE(val, 1.0);
    }
}

TEST_F(RandomMtTest, NextFloatMinMax) {
    for (int i = 0; i < 100; ++i) {
        double val = default_rng.next_float(2.0, 4.0);
        EXPECT_GE(val, 2.0);
        EXPECT_LE(val, 4.0);
    }
    EXPECT_EQ(default_rng.next_float(4.0, 2.0), 4.0);
    EXPECT_EQ(default_rng.next_float(6.0, 6.0), 6.0);
}

TEST_F(RandomMtTest, NextFloatMax) {
    for (int i = 0; i < 100; ++i) {
        double val = default_rng.next_float(100.0);
        EXPECT_GE(val, 0.0);
        EXPECT_LE(val, 100.0);
    }
}

TEST_F(RandomMtTest, ReproducibilitySameSeed) {
    random_mt rng1(7777);
    random_mt rng2(7777);
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(rng1.next_int<int>(), rng2.next_int<int>());
        EXPECT_EQ(rng1.next_uint64(), rng2.next_uint64());
        EXPECT_DOUBLE_EQ(rng1.next_float<double>(), rng2.next_float<double>());
    }
}

TEST(SecretTest, SystemSupported) { EXPECT_TRUE(secret::system_supported()); }

TEST(SecretTest, NextIntMaxBoundary) {
    EXPECT_EQ(secret::next_int(0), 0);
    EXPECT_EQ(secret::next_int(1), 0);
    for (int i = 0; i < 50; ++i) {
        int val = secret::next_int(10);
        EXPECT_GE(val, 0);
        EXPECT_LT(val, 10);
    }
}

TEST(SecretTest, NextIntMinMax) {
    for (int i = 0; i < 50; ++i) {
        int val = secret::next_int(5, 15);
        EXPECT_GE(val, 5);
        EXPECT_LT(val, 15);
    }
    EXPECT_EQ(secret::next_int(15, 5), 15);
    EXPECT_EQ(secret::next_int(3, 3), 3);
}

TEST(SecretTest, NextIntFullRange) {
    for (int i = 0; i < 50; ++i) {
        int val = secret::next_int<int>();
        EXPECT_GE(val, numeric_traits<int>::min());
        EXPECT_LE(val, numeric_traits<int>::max());
    }
    for (int i = 0; i < 50; ++i) {
        unsigned int val = secret::next_int<unsigned int>();
        EXPECT_LE(val, numeric_traits<unsigned int>::max());
    }
    for (int i = 0; i < 50; ++i) {
        short val = secret::next_int<short>();
        EXPECT_GE(val, numeric_traits<short>::min());
        EXPECT_LE(val, numeric_traits<short>::max());
    }
    for (int i = 0; i < 50; ++i) {
        int64_t val = secret::next_int<int64_t>();
        EXPECT_GE(val, numeric_traits<int64_t>::min());
        EXPECT_LE(val, numeric_traits<int64_t>::max());
    }
    for (int i = 0; i < 50; ++i) {
        uint64_t val = secret::next_int<uint64_t>();
        EXPECT_LE(val, numeric_traits<uint64_t>::max());
    }
}

TEST(SecretTest, NextUint64Max) {
    EXPECT_EQ(secret::next_uint64(0), 0u);
    EXPECT_EQ(secret::next_uint64(1), 0u);
    for (int i = 0; i < 50; ++i) {
        uint64_t val = secret::next_uint64(999);
        EXPECT_LT(val, 999u);
    }
}

TEST(SecretTest, NextUint64Full) {
    for (int i = 0; i < 50; ++i) {
        uint64_t val = secret::next_uint64();
        EXPECT_LE(val, numeric_traits<uint64_t>::max());
    }
}

TEST(SecretTest, NextFloat) {
    for (int i = 0; i < 50; ++i) {
        float val = secret::next_float<float>();
        EXPECT_GE(val, 0.0f);
        EXPECT_LE(val, 1.0f);
    }
    for (int i = 0; i < 50; ++i) {
        double val = secret::next_float<double>();
        EXPECT_GE(val, 0.0);
        EXPECT_LE(val, 1.0);
    }
}

TEST(SecretTest, NextFloatMinMax) {
    for (int i = 0; i < 50; ++i) {
        double val = secret::next_float(0.5, 2.5);
        EXPECT_GE(val, 0.5);
        EXPECT_LE(val, 2.5);
    }
    EXPECT_EQ(secret::next_float(2.5, 0.5), 2.5);
    EXPECT_EQ(secret::next_float(7.0, 7.0), 7.0);
}

TEST(SecretTest, NextFloatMax) {
    for (int i = 0; i < 50; ++i) {
        double val = secret::next_float(50.0);
        EXPECT_GE(val, 0.0);
        EXPECT_LE(val, 50.0);
    }
}

namespace {

    template <typename Generator>
    int count_random_bits(Generator& rng, const int draws, const int bits_per_draw) {
        int ones = 0;

        for (int i = 0; i < draws; ++i) {
            const uint64_t value = rng.next_uint64();
            for (int j = 0; j < bits_per_draw; ++j) {
                ones += static_cast<int>((value >> j) & 1u);
            }
        }
        return ones;
    }

} // namespace

TEST(RandomSeedTest, DistinctCalls) {
    const uint64_t first = random_seed();
    const uint64_t second = random_seed();
    const uint64_t third = random_seed();
    EXPECT_NE(first, second);
    EXPECT_NE(second, third);
    EXPECT_NE(first, third);
}

TEST(RandomSeedTest, SplitMix64Sequence) {
    uint64_t state = 0x123456789abcdef0ULL;
    EXPECT_EQ(splitmix64(state), 0x161922c645ce50e8ULL);
    EXPECT_EQ(state, 0xb06bd0321a075b05ULL);
    EXPECT_EQ(splitmix64(state), 0xad760cafa1697b60ULL);
    EXPECT_EQ(state, 0x4ea349eb9951d71aULL);
    EXPECT_EQ(splitmix64(state), 0x3501ff44902ca50dULL);
    EXPECT_EQ(state, 0xecdac3a5189c532fULL);
    EXPECT_EQ(splitmix64(state), 0x417cb9a826d831dfULL);
    EXPECT_EQ(state, 0x8b123d5e97e6cf44ULL);
}

TEST(RandomSeedTest, DefaultConstructedGeneratorsDiffer) {
    random_pcg32 first_pcg;
    random_pcg32 second_pcg;
    random_pcg64 third_pcg;
    random_pcg64 fourth_pcg;
    random_xoroshiro128<> first_xoro;
    random_xoroshiro128<> second_xoro;
    random_xoroshiro256<> first_xoshi;
    random_xoroshiro256<> second_xoshi;

    EXPECT_NE(first_pcg.next_uint64(), second_pcg.next_uint64());
    EXPECT_NE(third_pcg.next_uint64(), fourth_pcg.next_uint64());
    EXPECT_NE(first_xoro.next_uint64(), second_xoro.next_uint64());
    EXPECT_NE(first_xoshi.next_uint64(), second_xoshi.next_uint64());
}

class RandomPcg32Test : public ::testing::Test {
protected:
    random_pcg32 default_rng;
    random_pcg32 seeded_rng{42, 54};
};

TEST_F(RandomPcg32Test, ReferenceVector) {
    const uint32_t expected[6] = {0xa15c02b7u, 0x7b47f409u, 0xba1d3330u, 0x83d2f293u, 0xbfa4784b, 0xcbed606e};
    for (size_t i = 0; i < 6; ++i) {
        EXPECT_EQ(seeded_rng.next_int<uint32_t>(), expected[i]) << "index " << i;
    }
}

TEST_F(RandomPcg32Test, ReferenceVectorUint64) {
    random_pcg32 rng(42, 54);
    EXPECT_EQ(rng.next_uint64(), (static_cast<uint64_t>(0xa15c02b7u) << 32) | 0x7b47f409ULL);
}

TEST_F(RandomPcg32Test, SetSeedMatchesConstructor) {
    random_pcg32 rng;
    rng.set_seed(20240909);

    random_pcg32 other(20240909);
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(rng.next_uint64(), other.next_uint64());
    }
}

TEST_F(RandomPcg32Test, Reproducibility) {
    random_pcg32 rng1(987654321);
    random_pcg32 rng2(987654321);
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(rng1.next_int<int>(), rng2.next_int<int>());
        EXPECT_EQ(rng1.next_uint64(), rng2.next_uint64());
        EXPECT_DOUBLE_EQ(rng1.next_float<double>(), rng2.next_float<double>());
    }
}

TEST_F(RandomPcg32Test, DifferentStreamsDiverge) {
    random_pcg32 rng1(4242, 1);
    random_pcg32 rng2(4242, 2);
    int matches = 0;
    for (int i = 0; i < 64; ++i) {
        if (rng1.next_uint64() == rng2.next_uint64()) {
            ++matches;
        }
    }
    EXPECT_EQ(matches, 0);
}

TEST_F(RandomPcg32Test, NextIntMaxBoundary) {
    EXPECT_EQ(default_rng.next_int(0), 0);
    EXPECT_EQ(default_rng.next_int(1), 0);
    EXPECT_EQ(default_rng.next_int(-5), 0);
    for (int i = 0; i < 100; ++i) {
        const int val = default_rng.next_int(5);
        EXPECT_GE(val, 0);
        EXPECT_LT(val, 5);
    }
}

TEST_F(RandomPcg32Test, NextIntMinMax) {
    for (int i = 0; i < 100; ++i) {
        const int val = default_rng.next_int(10, 20);
        EXPECT_GE(val, 10);
        EXPECT_LT(val, 20);
    }
    EXPECT_EQ(default_rng.next_int(20, 10), 20);
    EXPECT_EQ(default_rng.next_int(5, 5), 5);
}

TEST_F(RandomPcg32Test, NextIntFullRange) {
    for (int i = 0; i < 100; ++i) {
        const int val = default_rng.next_int<int>();
        EXPECT_GE(val, numeric_traits<int>::min());
        EXPECT_LE(val, numeric_traits<int>::max());
    }
    for (int i = 0; i < 100; ++i) {
        const short val = seeded_rng.next_int<short>();
        EXPECT_GE(val, numeric_traits<short>::min());
        EXPECT_LE(val, numeric_traits<short>::max());
    }
}

TEST_F(RandomPcg32Test, NextUint64Max) {
    EXPECT_EQ(default_rng.next_uint64(0), 0u);
    EXPECT_EQ(default_rng.next_uint64(1), 0u);
    for (int i = 0; i < 100; ++i) {
        EXPECT_LT(default_rng.next_uint64(1000), 1000u);
    }
}

TEST_F(RandomPcg32Test, NextFloat) {
    for (int i = 0; i < 100; ++i) {
        const double val = default_rng.next_float<double>();
        EXPECT_GE(val, 0.0);
        EXPECT_LE(val, 1.0);
    }
    for (int i = 0; i < 100; ++i) {
        const double val = default_rng.next_float(1.5, 3.5);
        EXPECT_GE(val, 1.5);
        EXPECT_LE(val, 3.5);
    }
    EXPECT_EQ(default_rng.next_float(3.0, 1.0), 3.0);
    EXPECT_EQ(default_rng.next_float(5.0, 5.0), 5.0);
}

TEST_F(RandomPcg32Test, Uniformity) {
    random_pcg32 rng(20240909);
    int buckets[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    for (int i = 0; i < 80000; ++i) {
        ++buckets[rng.next_int(8)];
    }
    for (int i = 0; i < 8; ++i) {
        EXPECT_GT(buckets[i], 9000);
        EXPECT_LT(buckets[i], 11000);
    }
}

class RandomPcg64Test : public ::testing::Test {
protected:
    random_pcg64 default_rng;
    random_pcg64 seeded_rng{42, 54};
};

TEST_F(RandomPcg64Test, NumpyReferenceVector) {
    const uint64_t expected[6] = {0x2187b9d377f4554cULL, 0x468bb15f20d16f88ULL, 0x313b0b7e46dbfa71ULL,
                                  0xe5b9fe1f8c3897c9ULL, 0x6d122f1231395fc8ULL, 0x9523e6d76dfc73bfULL};
    for (size_t i = 0; i < 6; ++i) {
        EXPECT_EQ(seeded_rng.next_uint64(), expected[i]) << "index " << i;
    }
}

TEST_F(RandomPcg64Test, SetSeedMatchesConstructor) {
    random_pcg64 rng;
    rng.set_seed(24680, 1357);

    random_pcg64 other(24680, 1357);
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(rng.next_uint64(), other.next_uint64());
    }
}

TEST_F(RandomPcg64Test, Reproducibility) {
    random_pcg64 rng1(13579);
    random_pcg64 rng2(13579);
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(rng1.next_uint64(), rng2.next_uint64());
        EXPECT_EQ(rng1.next_int<int>(), rng2.next_int<int>());
    }
}

TEST_F(RandomPcg64Test, DifferentStreamsDiverge) {
    random_pcg64 rng1(2468, 1);
    random_pcg64 rng2(2468, 2);
    int matches = 0;
    for (int i = 0; i < 64; ++i) {
        if (rng1.next_uint64() == rng2.next_uint64()) {
            ++matches;
        }
    }
    EXPECT_EQ(matches, 0);
}

TEST_F(RandomPcg64Test, NextUint64Max) {
    EXPECT_EQ(default_rng.next_uint64(0), 0u);
    EXPECT_EQ(default_rng.next_uint64(1), 0u);
    for (int i = 0; i < 100; ++i) {
        EXPECT_LT(default_rng.next_uint64(4096), 4096u);
    }
}

TEST_F(RandomPcg64Test, NextIntAndFloatRanges) {
    for (int i = 0; i < 100; ++i) {
        const int val = default_rng.next_int(10, 20);
        EXPECT_GE(val, 10);
        EXPECT_LT(val, 20);
    }
    for (int i = 0; i < 100; ++i) {
        const double val = seeded_rng.next_float(2.0, 4.0);
        EXPECT_GE(val, 2.0);
        EXPECT_LE(val, 4.0);
    }
    EXPECT_EQ(default_rng.next_int(7, 7), 7);
    EXPECT_EQ(default_rng.next_float(1.0, 1.0), 1.0);
}

TEST_F(RandomPcg64Test, BitBalance) {
    const int ones = count_random_bits(seeded_rng, 512, 64);
    EXPECT_GT(ones, 16384 - 512);
    EXPECT_LT(ones, 16384 + 512);
}

TEST_F(RandomPcg64Test, Uniformity) {
    random_pcg64 rng(11223344);
    int buckets[16] = {0};
    for (int i = 0; i < 64000; ++i) {
        ++buckets[rng.next_int(16)];
    }
    for (int i = 0; i < 16; ++i) {
        EXPECT_GT(buckets[i], 3400);
        EXPECT_LT(buckets[i], 4600);
    }
}

class RandomXoroshiro128Test : public ::testing::Test {
protected:
    random_xoroshiro128<> default_rng;
};

TEST_F(RandomXoroshiro128Test, PlusReferenceVector) {
    random_xoroshiro128<xoroshiro_scramble::PLUS> rng(0xdeadbeefULL);
    const uint64_t expected[5] = {0x29382340aa6af4bdULL, 0x447b6251deeffa2eULL, 0xbb7cf71d35611eeaULL,
                                  0x7fc337571a417722ULL, 0x9228cb236dc1636bULL};
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(rng.next_uint64(), expected[i]) << "index " << i;
    }
}

TEST_F(RandomXoroshiro128Test, StarStarReferenceVector) {
    random_xoroshiro128<xoroshiro_scramble::STAR_STAR> rng(0xdeadbeefULL);
    const uint64_t expected[5] = {0xa9c3dab5bf352193ULL, 0x630c2395a4dc81e6ULL, 0x9a9c191dca4d2206ULL,
                                  0x64d3e446c5b84eecULL, 0x979b54a456ba80d3ULL};
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(rng.next_uint64(), expected[i]) << "index " << i;
    }
}

TEST_F(RandomXoroshiro128Test, PlusPlusReferenceVector) {
    random_xoroshiro128<> rng(0xdeadbeefULL);
    const uint64_t expected[5] = {0x91610de552443e0bULL, 0x1805b6da6d10413fULL, 0xfd56497c8b255620ULL,
                                  0xc39c76e2d0a177ccULL, 0xc30846953dd92d8bULL};
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(rng.next_uint64(), expected[i]) << "index " << i;
    }
}

TEST_F(RandomXoroshiro128Test, DefaultTemplateArgumentIsPlusPlus) {
    random_xoroshiro128<> first(0xabcdefULL);
    random_xoroshiro128<xoroshiro_scramble::PLUS_PLUS> second(0xabcdefULL);
    for (int i = 0; i < 64; ++i) {
        EXPECT_EQ(first.next_uint64(), second.next_uint64());
    }
}

TEST_F(RandomXoroshiro128Test, VariantsDiverge) {
    random_xoroshiro128<xoroshiro_scramble::PLUS> plus(999);
    random_xoroshiro128<xoroshiro_scramble::STAR_STAR> star_star(999);
    random_xoroshiro128<xoroshiro_scramble::PLUS_PLUS> plus_plus(999);

    EXPECT_NE(plus.next_uint64(), star_star.next_uint64());
    EXPECT_NE(star_star.next_uint64(), plus_plus.next_uint64());
    EXPECT_NE(plus.next_uint64(), plus_plus.next_uint64());
}

TEST_F(RandomXoroshiro128Test, SeedZeroAvoidsZeroState) {
    for (uint64_t seed = 0; seed < 32; ++seed) {
        random_xoroshiro128<> rng(seed);
        uint64_t accumulated = 0;
        for (int i = 0; i < 8; ++i) {
            accumulated |= rng.next_uint64();
        }
        EXPECT_NE(accumulated, 0u) << "seed " << seed;
    }
}

TEST_F(RandomXoroshiro128Test, SetSeedMatchesConstructor) {
    random_xoroshiro128<> rng;
    rng.set_seed(0x5a5a5a5aULL);

    random_xoroshiro128<> other(0x5a5a5a5aULL);
    for (int i = 0; i < 64; ++i) {
        EXPECT_EQ(rng.next_uint64(), other.next_uint64());
    }
}

TEST_F(RandomXoroshiro128Test, Reproducibility) {
    random_xoroshiro128<> rng1(0x1122334455667788ULL);
    random_xoroshiro128<> rng2(0x1122334455667788ULL);
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(rng1.next_uint64(), rng2.next_uint64());
        EXPECT_DOUBLE_EQ(rng1.next_float<double>(), rng2.next_float<double>());
    }
}

TEST_F(RandomXoroshiro128Test, NextIntAndFloatRanges) {
    for (int i = 0; i < 100; ++i) {
        const int val = default_rng.next_int(10, 20);
        EXPECT_GE(val, 10);
        EXPECT_LT(val, 20);
    }
    for (int i = 0; i < 100; ++i) {
        const float val = default_rng.next_float<float>();
        EXPECT_GE(val, 0.0f);
        EXPECT_LE(val, 1.0f);
    }
    EXPECT_EQ(default_rng.next_int(3, 3), 3);
    EXPECT_EQ(default_rng.next_float(6.0, 6.0), 6.0);
    EXPECT_LT(default_rng.next_uint64(64), 64u);
}

TEST_F(RandomXoroshiro128Test, BitBalance) {
    const int ones = count_random_bits(default_rng, 1024, 64);
    EXPECT_GT(ones, 32768 - 1024);
    EXPECT_LT(ones, 32768 + 1024);
}

TEST_F(RandomXoroshiro128Test, Uniformity) {
    random_xoroshiro128<> rng(0x77777777ULL);
    int buckets[12] = {0};
    for (int i = 0; i < 120000; ++i) {
        ++buckets[rng.next_int(12)];
    }
    for (int i = 0; i < 12; ++i) {
        EXPECT_GT(buckets[i], 9000);
        EXPECT_LT(buckets[i], 11000);
    }
}

class RandomXoroshiro256Test : public ::testing::Test {
protected:
    random_xoroshiro256<> default_rng;
};

TEST_F(RandomXoroshiro256Test, PlusReferenceVector) {
    random_xoroshiro256<xoroshiro_scramble::PLUS> rng(0xdeadbeefULL);
    const uint64_t expected[5] = {0xbf468782e4ab532bULL, 0xeeb772952711cc71ULL, 0x06ecba84e8c0ab44ULL,
                                  0xe297cc89b43e9775ULL, 0x486889fde24c7308ULL};
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(rng.next_uint64(), expected[i]) << "index " << i;
    }
}

TEST_F(RandomXoroshiro256Test, StarStarReferenceVector) {
    random_xoroshiro256<xoroshiro_scramble::STAR_STAR> rng(0xdeadbeefULL);
    const uint64_t expected[5] = {0xc5555444a74d7e83ULL, 0x65c30d37b4b16e38ULL, 0x54f773200a4efa23ULL,
                                  0x429aed75fb958af7ULL, 0xfb0e1dd69c255b2eULL};
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(rng.next_uint64(), expected[i]) << "index " << i;
    }
}

TEST_F(RandomXoroshiro256Test, PlusPlusReferenceVector) {
    random_xoroshiro256<> rng(0xdeadbeefULL);
    const uint64_t expected[5] = {0x0c520eb8fea98edeULL, 0x2b74a6338b80e0e2ULL, 0xbe238770c3795322ULL,
                                  0x5f235f98a244ea97ULL, 0xe004f0cc1514d858ULL};
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(rng.next_uint64(), expected[i]) << "index " << i;
    }
}

TEST_F(RandomXoroshiro256Test, VariantsDiverge) {
    random_xoroshiro256<xoroshiro_scramble::PLUS> plus(0x2468ULL);
    random_xoroshiro256<xoroshiro_scramble::STAR_STAR> star_star(0x2468ULL);
    random_xoroshiro256<xoroshiro_scramble::PLUS_PLUS> plus_plus(0x2468ULL);

    EXPECT_NE(plus.next_uint64(), star_star.next_uint64());
    EXPECT_NE(star_star.next_uint64(), plus_plus.next_uint64());
    EXPECT_NE(plus.next_uint64(), plus_plus.next_uint64());
}

TEST_F(RandomXoroshiro256Test, SeedZeroAvoidsZeroState) {
    for (uint64_t seed = 0; seed < 32; ++seed) {
        random_xoroshiro256<> rng(seed);
        uint64_t accumulated = 0;
        for (int i = 0; i < 8; ++i) {
            accumulated |= rng.next_uint64();
        }
        EXPECT_NE(accumulated, 0u) << "seed " << seed;
    }
}

TEST_F(RandomXoroshiro256Test, Reproducibility) {
    random_xoroshiro256<> rng1(0x99887766ULL);
    random_xoroshiro256<> rng2(0x99887766ULL);
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(rng1.next_uint64(), rng2.next_uint64());
    }
}

TEST_F(RandomXoroshiro256Test, NextIntAndFloatRanges) {
    for (int i = 0; i < 100; ++i) {
        const int val = default_rng.next_int(-10, 10);
        EXPECT_GE(val, -10);
        EXPECT_LT(val, 10);
    }
    for (int i = 0; i < 100; ++i) {
        const double val = default_rng.next_float(0.0, 5.0);
        EXPECT_GE(val, 0.0);
        EXPECT_LE(val, 5.0);
    }
    EXPECT_EQ(default_rng.next_int(4, 4), 4);
}

TEST_F(RandomXoroshiro256Test, BitBalance) {
    const int ones = count_random_bits(default_rng, 2048, 64);
    EXPECT_GT(ones, 65536 - 2048);
    EXPECT_LT(ones, 65536 + 2048);
}

TEST_F(RandomXoroshiro256Test, Uniformity) {
    random_xoroshiro256<> rng(0x13572468ULL);
    int buckets[10] = {0};
    for (int i = 0; i < 100000; ++i) {
        ++buckets[rng.next_int(10)];
    }
    for (int i = 0; i < 10; ++i) {
        EXPECT_GT(buckets[i], 9000);
        EXPECT_LT(buckets[i], 11000);
    }
}

TEST(EngineWordBitsTest, UrbgWordBits) {
    EXPECT_EQ(engine_word_bits<random_lcd>(), 31u);
    EXPECT_EQ(engine_word_bits<random_mt>(), 32u);
    EXPECT_EQ(engine_word_bits<random_pcg32>(), 32u);
    EXPECT_EQ(engine_word_bits<random_pcg64>(), 64u);
    EXPECT_EQ(engine_word_bits<random_xoroshiro128<xoroshiro_scramble::PLUS_PLUS>>(), 64u);
    EXPECT_EQ(engine_word_bits<random_xoroshiro256<xoroshiro_scramble::PLUS_PLUS>>(), 64u);
    EXPECT_EQ(engine_word_bits<secret>(), 32u);

    using bit_gen_7 = bit_generator<random_mt, 7, uint32_t>;
    using bit_gen_12 = bit_generator<random_pcg64, 12, uint16_t>;
    EXPECT_EQ(engine_word_bits<bit_gen_7>(), static_cast<size_t>(7));
    EXPECT_EQ(engine_word_bits<bit_gen_12>(), static_cast<size_t>(12));
}

TEST(EngineWordBitsTest, RangeBits) {
    EXPECT_EQ(random_range_bits(0, 0x7fffffffu, 32), 31u);
    EXPECT_EQ(random_range_bits(0, 0xffffffffu, 32), 32u);
    EXPECT_EQ(random_range_bits(0, 0xffffffffffffffffULL, 64), 64u);
    EXPECT_EQ(random_range_bits(5, 68, 64), 6u);
}

TEST(BitGenTest, WordSizeConstant) {
    EXPECT_EQ(static_cast<size_t>(bit_generator<random_mt, 1>::word_size), 1u);
    EXPECT_EQ(static_cast<size_t>(bit_generator<random_mt, 7>::word_size), 7u);
    EXPECT_EQ(static_cast<size_t>(bit_generator<random_mt, 64>::word_size), 64u);
}

TEST(BitGenTest, SingleBitDistribution) {
    bit_generator<random_mt, 1> gen(0x1234u);
    int ones = 0;
    bool seen_zero = false;
    bool seen_one = false;

    for (int i = 0; i < 4000; ++i) {
        const uint64_t value = gen();
        EXPECT_LE(value, 1u);
        ones += static_cast<int>(value);
        seen_zero = seen_zero || value == 0;
        seen_one = seen_one || value == 1;
    }
    EXPECT_TRUE(seen_zero);
    EXPECT_TRUE(seen_one);
    EXPECT_GT(ones, 1800);
    EXPECT_LT(ones, 2200);
}

TEST(BitGenTest, SevenBitCoverage) {
    bit_generator<random_mt, 7, uint32_t> gen(0x2468u);
    int counts[128] = {0};
    for (int i = 0; i < 128 * 32; ++i) {
        const uint32_t value = gen();
        ASSERT_LT(value, 128u);
        ++counts[value];
    }
    for (int i = 0; i < 128; ++i) {
        EXPECT_GT(counts[i], 0) << "value " << i;
    }
}

TEST(BitGenTest, WideWidthUsesHighBits) {
    bit_generator<random_mt, 32, uint32_t> gen(0x3333u);
    bool high_seen = false;
    for (int i = 0; i < 128; ++i) {
        high_seen = high_seen || gen() > 0x7fffffffu;
    }
    EXPECT_TRUE(high_seen);
}

TEST(BitGenTest, MatchesEngineWhenWidthEqualsWordBits) {
    bit_generator<random_pcg32, 64> gen(0xabcdu);
    random_pcg32 engine(0xabcdu);
    for (int i = 0; i < 64; ++i) {
        EXPECT_EQ(gen.next_uint64(), engine.next_uint64());
    }
}

TEST(BitGenTest, SixtyFourBitDirectPath) {
    bit_generator<random_xoroshiro128<xoroshiro_scramble::PLUS_PLUS>, 64> gen(0x5555ULL);
    random_xoroshiro128<> engine(0x5555ULL);
    for (int i = 0; i < 32; ++i) {
        EXPECT_EQ(gen(), engine.next_uint64());
    }
}

TEST(BitGenTest, LinearCongruentialSourceRespectsWordBits) {
    bit_generator<random_lcd, 8> gen(0x7777u);
    bool low_seen = false;
    bool high_seen = false;

    for (int i = 0; i < 512; ++i) {
        const uint64_t value = gen();
        ASSERT_LT(value, 256u);
        low_seen = low_seen || value < 128u;
        high_seen = high_seen || value >= 128u;
    }
    EXPECT_TRUE(low_seen);
    EXPECT_TRUE(high_seen);
}

TEST(BitGenTest, HighWidthSource) {
    bit_generator<random_pcg64, 48> gen(0x8888ULL);
    bool high_seen = false;
    for (int i = 0; i < 128; ++i) {
        const uint64_t value = gen();
        ASSERT_LT(value, static_cast<uint64_t>(1) << 48);
        high_seen = high_seen || value >= static_cast<uint64_t>(1) << 47;
    }
    EXPECT_TRUE(high_seen);
}

TEST(BitGenTest, SameSeedSameSequence) {
    bit_generator<random_mt, 7, uint32_t> first(0x1357u);
    bit_generator<random_mt, 7, uint32_t> second(0x1357u);
    for (int i = 0; i < 128; ++i) {
        EXPECT_EQ(first(), second());
    }
}

TEST(BitGenTest, DifferentSeedsDiverge) {
    bit_generator<random_mt, 16, uint32_t> first(0x1111u);
    bit_generator<random_mt, 16, uint32_t> second(0x2222u);
    int matches = 0;
    for (int i = 0; i < 64; ++i) {
        if (first() == second()) {
            ++matches;
        }
    }
    EXPECT_LT(matches, 4);
}

TEST(BitGenTest, SetSeedClearsBuffer) {
    bit_generator<random_mt, 5> gen(0x1111u);
    for (int i = 0; i < 7; ++i) {
        static_cast<void>(gen());
    }
    gen.set_seed(0x1111u);

    bit_generator<random_mt, 5> fresh(0x1111u);
    for (int i = 0; i < 64; ++i) {
        EXPECT_EQ(gen(), fresh());
    }
}

TEST(BitGenTest, BaseAccessIsShared) {
    bit_generator<random_mt, 8> gen(0x2222u);
    gen.base().set_seed(0x9999u);

    bit_generator<random_mt, 8> fresh(0x9999u);
    for (int i = 0; i < 64; ++i) {
        EXPECT_EQ(gen(), fresh());
    }
}

TEST(BitGenTest, BoundedIntegerCoverage) {
    bit_generator<random_mt, 8> gen(0x4444u);
    int counts[100] = {0};
    for (int i = 0; i < 2000; ++i) {
        const int value = gen.next_int(100);
        ASSERT_GE(value, 0);
        ASSERT_LT(value, 100);
        ++counts[value];
    }
    for (int i = 0; i < 100; ++i) {
        EXPECT_GT(counts[i], 0) << "value " << i;
    }
}

TEST(BitGenTest, FullRangeIntegerAndFloat) {
    bit_generator<random_xoroshiro256<xoroshiro_scramble::PLUS_PLUS>, 12, uint32_t> gen(0x6666ULL);
    for (int i = 0; i < 64; ++i) {
        EXPECT_LE(gen.next_int<uint32_t>(), numeric_traits<uint32_t>::max());
        const double val = gen.next_float<double>();
        EXPECT_GE(val, 0.0);
        EXPECT_LE(val, 1.0);
    }
    EXPECT_EQ(gen.next_float(3.0, 3.0), 3.0);
    EXPECT_EQ(gen.next_int(2, 2), 2);
}


template <typename Engine>
class RandomUrbgConformanceTest : public ::testing::Test {};

using RandomUrbgEngines =
        ::testing::Types<random_lcd, random_mt, random_pcg32, random_pcg64,
                         random_xoroshiro128<xoroshiro_scramble::PLUS_PLUS>,
                         random_xoroshiro256<xoroshiro_scramble::PLUS_PLUS>, bit_generator<random_mt, 1>,
                         bit_generator<random_mt, 7, uint32_t>, bit_generator<random_pcg64, 12, uint16_t>>;

TYPED_TEST_SUITE(RandomUrbgConformanceTest, RandomUrbgEngines);

TYPED_TEST(RandomUrbgConformanceTest, ResultTypeIsUnsignedIntegral) {
    EXPECT_TRUE(is_integral_v<typename TypeParam::result_type>);
    EXPECT_TRUE(is_unsigned_v<typename TypeParam::result_type>);
}

TYPED_TEST(RandomUrbgConformanceTest, MinIsZeroAndRangeIsPowerOfTwo) {
    EXPECT_EQ(static_cast<uint64_t>(TypeParam::min()), 0u);

    const uint64_t range = static_cast<uint64_t>(TypeParam::max()) + 1;
    EXPECT_TRUE(range == 0 || (range & (range - 1)) == 0);
}

TYPED_TEST(RandomUrbgConformanceTest, WordBitsMatchRange) {
    const size_t bits = engine_word_bits<TypeParam>();
    EXPECT_GE(bits, 1u);
    EXPECT_LE(bits, 64u);

    const uint64_t range = static_cast<uint64_t>(TypeParam::max()) + 1;
    const uint64_t expected = (numeric_traits<uint64_t>::max() >> (64u - bits)) + 1u;
    EXPECT_EQ(range, expected);
}

TYPED_TEST(RandomUrbgConformanceTest, OperatorStaysInRange) {
    TypeParam engine(4242);
    for (int i = 0; i < 1000; ++i) {
        const auto value = engine();
        EXPECT_GE(value, TypeParam::min());
        EXPECT_LE(value, TypeParam::max());
    }
}

TYPED_TEST(RandomUrbgConformanceTest, OperatorCoversHighBits) {
    if (TypeParam::max() <= TypeParam::min()) {
        GTEST_SKIP();
    }

    TypeParam engine(1234);
    const auto half = static_cast<typename TypeParam::result_type>(TypeParam::max() / 2 + 1);
    bool high_seen = false;
    for (int i = 0; i < 256 && !high_seen; ++i) {
        high_seen = engine() >= half;
    }
    EXPECT_TRUE(high_seen);
}

TYPED_TEST(RandomUrbgConformanceTest, DiscardMatchesManualDraws) {
    TypeParam skipped(9999);
    TypeParam manual(9999);

    skipped.discard(37);
    for (int i = 0; i < 37; ++i) {
        static_cast<void>(manual());
    }

    for (int i = 0; i < 64; ++i) {
        EXPECT_EQ(skipped(), manual());
    }
}

TYPED_TEST(RandomUrbgConformanceTest, SharedInterfaceWorksForEveryEngine) {
    TypeParam engine(777);
    for (int i = 0; i < 64; ++i) {
        EXPECT_LT(engine.next_uint64(1000), 1000u);
    }
    EXPECT_EQ(engine.next_int(4, 4), 4);
    EXPECT_EQ(engine.next_float(2.0, 2.0), 2.0);

    TypeParam uniform(777);
    for (int i = 0; i < 64; ++i) {
        const double value = uniform.next_float<double>();
        EXPECT_GE(value, 0.0);
        EXPECT_LE(value, 1.0);
    }
}

TEST(RandomUrbgInteropTest, WorksWithStandardLibraryDistributions) {
    random_pcg64 engine(20240909);
    std::uniform_int_distribution<int> dice(1, 6);
    std::normal_distribution<double> gauss(0.0, 1.0);
    std::bernoulli_distribution coin(0.5);

    int counts[7] = {0};
    double sum = 0.0;
    int heads = 0;
    for (int i = 0; i < 60000; ++i) {
        ++counts[dice(engine)];
        sum += gauss(engine);
        heads += coin(engine) ? 1 : 0;
    }

    for (int face = 1; face <= 6; ++face) {
        EXPECT_GT(counts[face], 9000);
        EXPECT_LT(counts[face], 11000);
    }
    EXPECT_NEAR(sum / 60000.0, 0.0, 0.05);
    EXPECT_GT(heads, 29000);
    EXPECT_LT(heads, 31000);
}

TEST(RandomUrbgInteropTest, BitGenWorksWithStandardLibraryDistributions) {
    bit_generator<random_xoroshiro256<xoroshiro_scramble::PLUS_PLUS>, 24, uint32_t> engine(0x5a5aULL);
    std::uniform_int_distribution<uint32_t> distribution(0, 99999);

    uint32_t maximum = 0;
    for (int i = 0; i < 1000; ++i) {
        const uint32_t value = distribution(engine);
        EXPECT_LE(value, 99999u);
        if (value > maximum) {
            maximum = value;
        }
    }
    EXPECT_GT(maximum, 98000u);
}

TEST(RandomUrbgInteropTest, SecretSatisfiesUrbgConcept) {
    secret entropy;
    std::uniform_int_distribution<int> dice(1, 6);

    int counts[7] = {0};
    for (int i = 0; i < 600; ++i) {
        const int face = dice(entropy);
        ASSERT_GE(face, 1);
        ASSERT_LE(face, 6);
        ++counts[face];
    }
    for (int face = 1; face <= 6; ++face) {
        EXPECT_GT(counts[face], 0) << "face " << face;
    }
}

TEST(RandomDistributionTest, UniformIntRanges) {
    random_pcg64 engine(13579);

    int counts[10] = {0};
    for (int i = 0; i < 20000; ++i) {
        const int value = uniform_int(engine, 10, 20);
        ASSERT_GE(value, 10);
        ASSERT_LT(value, 20);
        ++counts[value - 10];
    }
    for (int i = 0; i < 10; ++i) {
        EXPECT_GT(counts[i], 1400);
        EXPECT_LT(counts[i], 2600);
    }

    for (int i = 0; i < 1000; ++i) {
        const int value = uniform_int(engine, -50, -40);
        EXPECT_GE(value, -50);
        EXPECT_LT(value, -40);
    }
    EXPECT_EQ(uniform_int(engine, 7, 7), 7);
    EXPECT_EQ(uniform_int(engine, 9, 3), 9);
}

TEST(RandomDistributionTest, UniformRealRanges) {
    random_pcg64 engine(24680);
    double sum = 0.0;

    for (int i = 0; i < 100000; ++i) {
        const double value = uniform_real(engine);
        ASSERT_GE(value, 0.0);
        ASSERT_LT(value, 1.0);
        sum += value;
    }
    EXPECT_NEAR(sum / 100000.0, 0.5, 0.02);

    for (int i = 0; i < 1000; ++i) {
        const double value = uniform_real(engine, 2.5, 7.5);
        EXPECT_GE(value, 2.5);
        EXPECT_LT(value, 7.5);
    }
    EXPECT_DOUBLE_EQ(uniform_real(engine, 3.0, 3.0), 3.0);
}

TEST(RandomDistributionTest, BernoulliProbabilities) {
    random_pcg64 engine(112233);

    for (int i = 0; i < 1000; ++i) {
        EXPECT_FALSE(bernoulli(engine, 0.0));
        EXPECT_TRUE(bernoulli(engine, 1.0));
    }

    int hits = 0;
    for (int i = 0; i < 100000; ++i) {
        if (bernoulli(engine, 0.25)) {
            ++hits;
        }
    }
    EXPECT_GT(hits, 24000);
    EXPECT_LT(hits, 26000);
}

TEST(RandomDistributionTest, NormalMatchesEmpiricalRule) {
    random_pcg64 engine(334455);
    int within_one = 0;
    int within_two = 0;
    int within_three = 0;
    double sum = 0.0;
    double square_sum = 0.0;
    const int draws = 100000;

    for (int i = 0; i < draws; ++i) {
        const double value = normal(engine, 2.0, 3.0);
        const double offset = value - 2.0;
        const double deviation = (offset < 0.0 ? -offset : offset) / 3.0;
        within_one += deviation < 1.0 ? 1 : 0;
        within_two += deviation < 2.0 ? 1 : 0;
        within_three += deviation < 3.0 ? 1 : 0;
        sum += value;
        square_sum += value * value;
    }

    EXPECT_NEAR(sum / draws, 2.0, 0.08);
    EXPECT_NEAR(square_sum / draws - (sum / draws) * (sum / draws), 9.0, 0.3);
    EXPECT_GT(within_one, 66770);
    EXPECT_LT(within_one, 69770);
    EXPECT_GT(within_two, 93950);
    EXPECT_LT(within_two, 96950);
    EXPECT_GT(within_three, 99230);
}

TEST(RandomDistributionTest, ExponentialMoments) {
    random_pcg64 engine(556677);
    double sum = 0.0;
    const int draws = 50000;

    for (int i = 0; i < draws; ++i) {
        const double value = exponential(engine, 2.5);
        ASSERT_GT(value, 0.0);
        sum += value;
    }
    EXPECT_NEAR(sum / draws, 0.4, 0.015);
}

TEST(RandomDistributionTest, LogUniformIsUniformOnLogScale) {
    random_pcg64 engine(667788);
    int buckets[10] = {0};
    const int draws = 100000;

    for (int i = 0; i < draws; ++i) {
        const double value = log_uniform(engine, 1.0, 1000.0);
        ASSERT_GE(value, 1.0);
        ASSERT_LT(value, 1000.0);

        const double position = std::log(value) / std::log(1000.0);
        const int bucket = static_cast<int>(position * 10.0);
        ASSERT_GE(bucket, 0);
        ASSERT_LT(bucket, 10);
        ++buckets[bucket];
    }
    for (int i = 0; i < 10; ++i) {
        EXPECT_GT(buckets[i], 9400);
        EXPECT_LT(buckets[i], 10600);
    }
}

TEST(RandomDistributionTest, PoissonMatchesSmallMeanProbabilities) {
    random_pcg64 engine(778899);
    const double expected[9] = {0.018316, 0.073263, 0.146525, 0.195367, 0.195367,
                                0.156293, 0.104196, 0.059540, 0.051134};
    int counts[9] = {0};
    const int draws = 100000;

    for (int i = 0; i < draws; ++i) {
        const uint64_t value = poisson(engine, 4.0);
        ++counts[value > 8 ? 8 : static_cast<size_t>(value)];
    }
    for (size_t i = 0; i < 9; ++i) {
        const double expected_count = expected[i] * draws;
        EXPECT_GT(counts[i], expected_count * 0.85) << "bucket " << i;
        EXPECT_LT(counts[i], expected_count * 1.15) << "bucket " << i;
    }
}

TEST(RandomDistributionTest, PoissonMatchesLargeMeanMoments) {
    random_pcg64 engine(889900);
    double sum = 0.0;
    double square_sum = 0.0;
    const int draws = 50000;

    for (int i = 0; i < draws; ++i) {
        const auto value = static_cast<double>(poisson(engine, 250.0));
        sum += value;
        square_sum += value * value;
    }

    const double mean = sum / draws;
    EXPECT_NEAR(mean, 250.0, 5.0);
    EXPECT_NEAR(square_sum / draws - mean * mean, 250.0, 20.0);
    EXPECT_EQ(poisson(engine, 0.0), 0u);
}

TEST(RandomDistributionTest, ReproducibleAndStateless) {
    random_pcg64 first(20240909);
    random_pcg64 second(20240909);

    for (int i = 0; i < 64; ++i) {
        EXPECT_EQ(uniform_int(first, 0, 1000), uniform_int(second, 0, 1000));
        EXPECT_DOUBLE_EQ(uniform_real(first), uniform_real(second));
        EXPECT_DOUBLE_EQ(normal(first, 1.0, 2.0), normal(second, 1.0, 2.0));
        EXPECT_DOUBLE_EQ(exponential(first, 1.5), exponential(second, 1.5));
        EXPECT_DOUBLE_EQ(log_uniform(first, 2.0, 8.0), log_uniform(second, 2.0, 8.0));
        EXPECT_EQ(poisson(first, 30.0), poisson(second, 30.0));
    }
}

TEST(RandomDistributionTest, WorksWithStandardLibraryEngine) {
    std::mt19937_64 engine(20240909);
    int counts[10] = {0};

    for (int i = 0; i < 10000; ++i) {
        ++counts[uniform_int(engine, 0, 10)];
    }
    for (int i = 0; i < 10; ++i) {
        EXPECT_GT(counts[i], 800);
        EXPECT_LT(counts[i], 1200);
    }

    const double value = normal(engine, 5.0, 2.0);
    EXPECT_GT(value, -25.0);
    EXPECT_LT(value, 35.0);
}

TEST(BitGenTest, SecretSourceIsSupported) {
    bit_generator<secret, 7, uint32_t> gen;
    int counts[128] = {0};

    for (int i = 0; i < 128 * 40; ++i) {
        const uint32_t value = gen();
        ASSERT_LT(value, 128u);
        ++counts[value];
    }
    for (int i = 0; i < 128; ++i) {
        EXPECT_GT(counts[i], 0) << "value " << i;
    }
}
