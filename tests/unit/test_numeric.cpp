#include <NeForce/core/container/vector.hpp>
#include <NeForce/core/numeric/int128.hpp>
#include <NeForce/core/numeric/math.hpp>
#include <NeForce/core/numeric/random.hpp>
#include <gtest/gtest.h>
#include <limits>
using namespace neforce;

template <typename T>
class NumericTraitsConsistencyTest : public ::testing::Test {
protected:
    using Traits = numeric_traits<T>;
    using StdLimits = std::numeric_limits<T>;
};

using TestTypes = ::testing::Types<bool, int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t, char,
                                   char16_t, char32_t, float32_t, float64_t>;

TYPED_TEST_SUITE(NumericTraitsConsistencyTest, TestTypes);

TYPED_TEST(NumericTraitsConsistencyTest, IsSpecialized) {
    using Traits = typename TestFixture::Traits;
    using StdLimits = typename TestFixture::StdLimits;
    EXPECT_EQ(Traits::is_specialized, StdLimits::is_specialized);
}

TYPED_TEST(NumericTraitsConsistencyTest, MinValue) {
    using Traits = typename TestFixture::Traits;
    using StdLimits = typename TestFixture::StdLimits;
    if constexpr (is_floating_point_v<TypeParam>) {
        EXPECT_FLOAT_EQ(Traits::min(), StdLimits::min());
    } else {
        EXPECT_EQ(Traits::min(), StdLimits::min());
    }
}

TYPED_TEST(NumericTraitsConsistencyTest, MaxValue) {
    using Traits = typename TestFixture::Traits;
    using StdLimits = typename TestFixture::StdLimits;
    if constexpr (is_floating_point_v<TypeParam>) {
        EXPECT_FLOAT_EQ(Traits::max(), StdLimits::max());
    } else {
        EXPECT_EQ(Traits::max(), StdLimits::max());
    }
}

TYPED_TEST(NumericTraitsConsistencyTest, LowestValue) {
    using Traits = typename TestFixture::Traits;
    using StdLimits = typename TestFixture::StdLimits;
    if constexpr (is_floating_point_v<TypeParam>) {
        EXPECT_FLOAT_EQ(Traits::lowest(), StdLimits::lowest());
    } else if constexpr (is_signed_v<TypeParam> && !is_floating_point_v<TypeParam>) {
        EXPECT_EQ(Traits::lowest(), StdLimits::lowest());
    } else {
        EXPECT_EQ(Traits::lowest(), StdLimits::lowest());
    }
}

TYPED_TEST(NumericTraitsConsistencyTest, EpsilonValue) {
    using Traits = typename TestFixture::Traits;
    using StdLimits = typename TestFixture::StdLimits;
    if constexpr (is_floating_point_v<TypeParam>) {
        EXPECT_FLOAT_EQ(Traits::epsilon(), StdLimits::epsilon());
    } else {
        EXPECT_EQ(Traits::epsilon(), StdLimits::epsilon());
    }
}

TYPED_TEST(NumericTraitsConsistencyTest, RoundErrorValue) {
    using Traits = typename TestFixture::Traits;
    using StdLimits = typename TestFixture::StdLimits;
    if constexpr (is_floating_point_v<TypeParam>) {
        EXPECT_FLOAT_EQ(Traits::round_error(), StdLimits::round_error());
    } else {
        EXPECT_EQ(Traits::round_error(), StdLimits::round_error());
    }
}

TYPED_TEST(NumericTraitsConsistencyTest, DenormMinValue) {
    using Traits = typename TestFixture::Traits;
    using StdLimits = typename TestFixture::StdLimits;
    if constexpr (is_floating_point_v<TypeParam>) {
        EXPECT_FLOAT_EQ(Traits::denorm_min(), StdLimits::denorm_min());
    } else {
        EXPECT_EQ(Traits::denorm_min(), StdLimits::denorm_min());
    }
}

TYPED_TEST(NumericTraitsConsistencyTest, InfinityValue) {
    using Traits = typename TestFixture::Traits;
    using StdLimits = typename TestFixture::StdLimits;
    if constexpr (is_floating_point_v<TypeParam>) {
        const auto custom_inf = Traits::infinity();
        const auto std_inf = StdLimits::infinity();
        EXPECT_EQ(_NEFORCE is_infinity(custom_inf), _NEFORCE is_infinity(std_inf));
        EXPECT_EQ(_NEFORCE signbit(custom_inf), _NEFORCE signbit(std_inf));
    } else {
        EXPECT_EQ(Traits::infinity(), StdLimits::infinity());
    }
}

TYPED_TEST(NumericTraitsConsistencyTest, QuietNaNValue) {
    using Traits = typename TestFixture::Traits;
    using StdLimits = typename TestFixture::StdLimits;
    if constexpr (is_floating_point_v<TypeParam>) {
        const auto custom_qnan = Traits::quiet_nan();
        const auto std_qnan = StdLimits::quiet_NaN();
        EXPECT_EQ(_NEFORCE is_nan(custom_qnan), _NEFORCE is_nan(std_qnan));
    } else {
        EXPECT_EQ(Traits::quiet_nan(), StdLimits::quiet_NaN());
    }
}

TYPED_TEST(NumericTraitsConsistencyTest, SignalingNaNValue) {
    using Traits = typename TestFixture::Traits;
    using StdLimits = typename TestFixture::StdLimits;
    if constexpr (is_floating_point_v<TypeParam>) {
        const auto custom_snan = Traits::signaling_nan();
        if constexpr (is_same_v<TypeParam, float64_t>) {
            const auto std_snan = StdLimits::signaling_NaN();
            EXPECT_EQ(_NEFORCE is_nan(custom_snan), _NEFORCE is_nan(std_snan));
        } else {
            EXPECT_TRUE(_NEFORCE is_nan(custom_snan));
        }
    } else {
        EXPECT_EQ(Traits::signaling_nan(), StdLimits::signaling_NaN());
    }
}

TYPED_TEST(NumericTraitsConsistencyTest, IsInteger) {
    using Traits = typename TestFixture::Traits;
    using StdLimits = typename TestFixture::StdLimits;
    EXPECT_EQ(Traits::is_integer, StdLimits::is_integer);
}

TYPED_TEST(NumericTraitsConsistencyTest, IsSigned) {
    using Traits = typename TestFixture::Traits;
    using StdLimits = typename TestFixture::StdLimits;
    EXPECT_EQ(Traits::is_signed, StdLimits::is_signed);
}

TYPED_TEST(NumericTraitsConsistencyTest, IsExact) {
    using Traits = typename TestFixture::Traits;
    using StdLimits = typename TestFixture::StdLimits;
    EXPECT_EQ(Traits::is_exact, StdLimits::is_exact);
}

TYPED_TEST(NumericTraitsConsistencyTest, IsBounded) {
    using Traits = typename TestFixture::Traits;
    using StdLimits = typename TestFixture::StdLimits;
    EXPECT_EQ(Traits::is_bounded, StdLimits::is_bounded);
}

TYPED_TEST(NumericTraitsConsistencyTest, IsModulo) {
    using Traits = typename TestFixture::Traits;
    using StdLimits = typename TestFixture::StdLimits;
    EXPECT_EQ(Traits::is_modulo, StdLimits::is_modulo);
}

TYPED_TEST(NumericTraitsConsistencyTest, Radix) {
    using Traits = typename TestFixture::Traits;
    using StdLimits = typename TestFixture::StdLimits;
    EXPECT_EQ(Traits::radix, StdLimits::radix);
}

TYPED_TEST(NumericTraitsConsistencyTest, Digits) {
    using Traits = typename TestFixture::Traits;
    using StdLimits = typename TestFixture::StdLimits;
    EXPECT_EQ(Traits::digits, StdLimits::digits);
}

TYPED_TEST(NumericTraitsConsistencyTest, Digits10) {
    using Traits = typename TestFixture::Traits;
    using StdLimits = typename TestFixture::StdLimits;
    EXPECT_EQ(Traits::digits10, StdLimits::digits10);
}

TYPED_TEST(NumericTraitsConsistencyTest, HasInfinity) {
    using Traits = typename TestFixture::Traits;
    using StdLimits = typename TestFixture::StdLimits;
    EXPECT_EQ(Traits::has_infinity, StdLimits::has_infinity);
}

TYPED_TEST(NumericTraitsConsistencyTest, HasQuietNaN) {
    using Traits = typename TestFixture::Traits;
    using StdLimits = typename TestFixture::StdLimits;
    EXPECT_EQ(Traits::has_quiet_nan, StdLimits::has_quiet_NaN);
}

TYPED_TEST(NumericTraitsConsistencyTest, HasSignalingNaN) {
    using Traits = typename TestFixture::Traits;
    using StdLimits = typename TestFixture::StdLimits;
    EXPECT_EQ(Traits::has_signaling_nan, StdLimits::has_signaling_NaN);
}

TYPED_TEST(NumericTraitsConsistencyTest, IsIec559) {
    using Traits = typename TestFixture::Traits;
    using StdLimits = typename TestFixture::StdLimits;
    EXPECT_EQ(Traits::is_iec559, StdLimits::is_iec559);
}

TYPED_TEST(NumericTraitsConsistencyTest, Traps) {
    using Traits = typename TestFixture::Traits;
    using StdLimits = typename TestFixture::StdLimits;
    constexpr bool custom_traps = Traits::traps;
    constexpr bool std_traps = StdLimits::traps;
    EXPECT_EQ(custom_traps, std_traps);
}

class Float32TraitsTest : public ::testing::Test {};
class Float64TraitsTest : public ::testing::Test {};

TEST_F(Float32TraitsTest, MaxDigits10) {
    EXPECT_EQ(numeric_traits<float32_t>::max_digits10, std::numeric_limits<float32_t>::max_digits10);
}

TEST_F(Float32TraitsTest, MaxExponent) {
    EXPECT_EQ(numeric_traits<float32_t>::max_exponent, std::numeric_limits<float32_t>::max_exponent);
}

TEST_F(Float32TraitsTest, MaxExponent10) {
    EXPECT_EQ(numeric_traits<float32_t>::max_exponent10, std::numeric_limits<float32_t>::max_exponent10);
}

TEST_F(Float32TraitsTest, MinExponent) {
    EXPECT_EQ(numeric_traits<float32_t>::min_exponent, std::numeric_limits<float32_t>::min_exponent);
}

TEST_F(Float32TraitsTest, MinExponent10) {
    EXPECT_EQ(numeric_traits<float32_t>::min_exponent10, std::numeric_limits<float32_t>::min_exponent10);
}

TEST_F(Float32TraitsTest, HasDenorm) {
    EXPECT_EQ(static_cast<int>(numeric_traits<float32_t>::has_denorm),
              static_cast<int>(std::numeric_limits<float32_t>::has_denorm));
}

TEST_F(Float32TraitsTest, RoundStyle) {
    EXPECT_EQ(static_cast<int>(numeric_traits<float32_t>::round_style),
              static_cast<int>(std::numeric_limits<float32_t>::round_style));
}

TEST_F(Float32TraitsTest, HasDenormLoss) {
    EXPECT_EQ(numeric_traits<float32_t>::has_denorm_loss, std::numeric_limits<float32_t>::has_denorm_loss);
}

TEST_F(Float32TraitsTest, TinynessBefore) {
    EXPECT_EQ(numeric_traits<float32_t>::tinyness_before, std::numeric_limits<float32_t>::tinyness_before);
}

TEST_F(Float64TraitsTest, MaxDigits10) {
    EXPECT_EQ(numeric_traits<float64_t>::max_digits10, std::numeric_limits<float64_t>::max_digits10);
}

TEST_F(Float64TraitsTest, MaxExponent) {
    EXPECT_EQ(numeric_traits<float64_t>::max_exponent, std::numeric_limits<float64_t>::max_exponent);
}

TEST_F(Float64TraitsTest, MaxExponent10) {
    EXPECT_EQ(numeric_traits<float64_t>::max_exponent10, std::numeric_limits<float64_t>::max_exponent10);
}

TEST_F(Float64TraitsTest, MinExponent) {
    EXPECT_EQ(numeric_traits<float64_t>::min_exponent, std::numeric_limits<float64_t>::min_exponent);
}

TEST_F(Float64TraitsTest, MinExponent10) {
    EXPECT_EQ(numeric_traits<float64_t>::min_exponent10, std::numeric_limits<float64_t>::min_exponent10);
}

TEST_F(Float64TraitsTest, HasDenorm) {
    EXPECT_EQ(static_cast<int>(numeric_traits<float64_t>::has_denorm),
              static_cast<int>(std::numeric_limits<float64_t>::has_denorm));
}

TEST_F(Float64TraitsTest, RoundStyle) {
    EXPECT_EQ(static_cast<int>(numeric_traits<float64_t>::round_style),
              static_cast<int>(std::numeric_limits<float64_t>::round_style));
}

TEST_F(Float64TraitsTest, HasDenormLoss) {
    EXPECT_EQ(numeric_traits<float64_t>::has_denorm_loss, std::numeric_limits<float64_t>::has_denorm_loss);
}

TEST_F(Float64TraitsTest, TinynessBefore) {
    EXPECT_EQ(numeric_traits<float64_t>::tinyness_before, std::numeric_limits<float64_t>::tinyness_before);
}

class SignedInt32RangeTest : public ::testing::Test {};

TEST_F(SignedInt32RangeTest, MinBoundary) {
    EXPECT_EQ(numeric_traits<int32_t>::min(), std::numeric_limits<int32_t>::min());
}

TEST_F(SignedInt32RangeTest, MaxBoundary) {
    EXPECT_EQ(numeric_traits<int32_t>::max(), std::numeric_limits<int32_t>::max());
}

class UnsignedInt64RangeTest : public ::testing::Test {};

TEST_F(UnsignedInt64RangeTest, MinBoundary) {
    EXPECT_EQ(numeric_traits<uint64_t>::min(), std::numeric_limits<uint64_t>::min());
}

TEST_F(UnsignedInt64RangeTest, MaxBoundary) {
    EXPECT_EQ(numeric_traits<uint64_t>::max(), std::numeric_limits<uint64_t>::max());
}

class ConstVolatileSpecializationTest : public ::testing::Test {};

TEST_F(ConstVolatileSpecializationTest, ConstInt32Matches) {
    EXPECT_EQ(numeric_traits<const int32_t>::max(), std::numeric_limits<int32_t>::max());
    EXPECT_EQ(numeric_traits<const int32_t>::min(), std::numeric_limits<int32_t>::min());
}

TEST_F(ConstVolatileSpecializationTest, VolatileFloat64Matches) {
    EXPECT_FLOAT_EQ(numeric_traits<volatile float64_t>::epsilon(), std::numeric_limits<float64_t>::epsilon());
}

TEST_F(ConstVolatileSpecializationTest, ConstVolatileUint16Matches) {
    EXPECT_EQ(numeric_traits<const volatile uint16_t>::max(), std::numeric_limits<uint16_t>::max());
}

class WCharTComparisonTest : public ::testing::Test {};

TEST_F(WCharTComparisonTest, MinMaxDigits) {
    EXPECT_EQ(numeric_traits<wchar_t>::min(), std::numeric_limits<wchar_t>::min());
    EXPECT_EQ(numeric_traits<wchar_t>::max(), std::numeric_limits<wchar_t>::max());
    EXPECT_EQ(numeric_traits<wchar_t>::digits, std::numeric_limits<wchar_t>::digits);
}

class BoolTypeSpecialTest : public ::testing::Test {};

TEST_F(BoolTypeSpecialTest, BooleanTraitsConsistency) {
    EXPECT_EQ(numeric_traits<bool>::is_signed, std::numeric_limits<bool>::is_signed);
    EXPECT_EQ(numeric_traits<bool>::is_integer, std::numeric_limits<bool>::is_integer);
    EXPECT_EQ(numeric_traits<bool>::is_exact, std::numeric_limits<bool>::is_exact);
}

TEST(SafeTruncTest, NormalValues) {
    EXPECT_EQ(safe_trunc(0.0L), 0);
    EXPECT_EQ(safe_trunc(42.9L), 42);
    EXPECT_EQ(safe_trunc(-42.9L), -42);
    EXPECT_EQ(safe_trunc(1.0L), 1);
    EXPECT_EQ(safe_trunc(-1.0L), -1);
}

TEST(SafeTruncTest, BoundaryValues) {
    constexpr decimal_t max_int64 = static_cast<decimal_t>(numeric_traits<int64_t>::max());
    constexpr decimal_t min_int64 = static_cast<decimal_t>(numeric_traits<int64_t>::min());
    EXPECT_EQ(safe_trunc(max_int64), static_cast<int64_t>(max_int64));
    EXPECT_EQ(safe_trunc(min_int64), static_cast<int64_t>(min_int64));
}

TEST(SafeTruncTest, SpecialFloats) {
    EXPECT_EQ(safe_trunc(numeric_traits<decimal_t>::quiet_nan()), 0);
    EXPECT_EQ(safe_trunc(numeric_traits<decimal_t>::infinity()), 0);
    EXPECT_EQ(safe_trunc(-numeric_traits<decimal_t>::infinity()), 0);
}

TEST(SafeDecimalToUint64Test, NormalValues) {
    EXPECT_EQ(safe_decimal_to_uint64(0.0L), 0ULL);
    EXPECT_EQ(safe_decimal_to_uint64(42.0L), 42ULL);
    EXPECT_EQ(safe_decimal_to_uint64(1e18L), 1000000000000000000ULL);
}

TEST(SafeDecimalToUint64Test, BoundaryValues) {
    constexpr decimal_t TWO_POW_63 = 9223372036854775808.0L;
    EXPECT_EQ(safe_decimal_to_uint64(TWO_POW_63), 9223372036854775808ULL);
}

TEST(FibonacciTest, PrecomputedValues) {
    EXPECT_EQ(fibonacci(0), 0);
    EXPECT_EQ(fibonacci(1), 1);
    EXPECT_EQ(fibonacci(2), 1);
    EXPECT_EQ(fibonacci(3), 2);
    EXPECT_EQ(fibonacci(4), 3);
    EXPECT_EQ(fibonacci(5), 5);
    EXPECT_EQ(fibonacci(10), 55);
    EXPECT_EQ(fibonacci(20), 6765);
}

TEST(FibonacciTest, BeyondPrecomputed) {
    uint64_t f49 = fibonacci(49);
    uint64_t f50 = fibonacci(50);
    uint64_t f51 = fibonacci(51);
    EXPECT_EQ(f51, f49 + f50);
    EXPECT_EQ(fibonacci(52), f50 + f51);
}

TEST(LeonardoTest, Values) {
    EXPECT_EQ(leonardo(0), 2 * fibonacci(1) - 1);
    EXPECT_EQ(leonardo(1), 2 * fibonacci(2) - 1);
    EXPECT_EQ(leonardo(2), 2 * fibonacci(3) - 1);
    EXPECT_EQ(leonardo(10), 2 * fibonacci(11) - 1);
}

TEST(Angular2RadianTest, Conversion) {
    EXPECT_DOUBLE_EQ(static_cast<double>(angular2radian(180.0L)), 3.14159265358979323846);
    EXPECT_DOUBLE_EQ(static_cast<double>(angular2radian(0.0L)), 0.0);
    EXPECT_DOUBLE_EQ(static_cast<double>(angular2radian(90.0L)), 1.57079632679489661923);
}

TEST(Radian2AngularTest, Conversion) {
    EXPECT_DOUBLE_EQ(static_cast<double>(radian2angular(constants::PI)), 180.0);
    EXPECT_DOUBLE_EQ(static_cast<double>(radian2angular(0.0L)), 0.0);
    EXPECT_DOUBLE_EQ(static_cast<double>(radian2angular(constants::PI / 2)), 90.0);
}

TEST(AbsoluteTest, Signed) {
    EXPECT_EQ(absolute(-5), 5);
    EXPECT_EQ(absolute(5), 5);
    EXPECT_EQ(absolute(0), 0);
    EXPECT_EQ(absolute(-0.5L), 0.5L);
    EXPECT_EQ(absolute(0.5L), 0.5L);
}

TEST(AbsoluteTest, Unsigned) {
    EXPECT_EQ(absolute(5u), 5u);
    EXPECT_EQ(absolute(0u), 0u);
}

TEST(SumTest, SingleArgument) {
    EXPECT_EQ(sum(42), 42);
    EXPECT_EQ(sum(3.14L), 3.14L);
}

TEST(SumTest, MultipleArguments) {
    EXPECT_EQ(sum(1, 2, 3, 4), 10);
    EXPECT_EQ(sum(1.5, 2.5, 3.0), 7.0);
}

TEST(AverageTest, MultipleArguments) {
    EXPECT_DOUBLE_EQ(static_cast<double>(average(1.0L, 2.0L, 3.0L)), 2.0);
    EXPECT_EQ(average(2, 4, 6, 8), 5);
}

TEST(SignTest, Values) {
    EXPECT_EQ(sign(10), 1);
    EXPECT_EQ(sign(-10), -1);
    EXPECT_EQ(sign(0), 0);
    EXPECT_EQ(sign(0.0L), 0);
    EXPECT_EQ(sign(-0.5L), -1);
}

TEST(GcdTest, PositiveNumbers) {
    EXPECT_EQ(gcd(48, 18), 6);
    EXPECT_EQ(gcd(101, 103), 1);
    EXPECT_EQ(gcd(0, 5), 5);
    EXPECT_EQ(gcd(5, 0), 5);
}

TEST(GcdTest, NegativeNumbers) {
    EXPECT_EQ(gcd(-48, 18), 6);
    EXPECT_EQ(gcd(48, -18), 6);
    EXPECT_EQ(gcd(-48, -18), 6);
}

TEST(LcmTest, Basic) {
    EXPECT_EQ(lcm(4, 6), 12);
    EXPECT_EQ(lcm(21, 6), 42);
    EXPECT_EQ(lcm(1, 5), 5);
}

TEST(LcmTest, ZeroInput) {
    EXPECT_EQ(lcm(0, 5), 0);
    EXPECT_EQ(lcm(5, 0), 0);
}

TEST(Debug, TestCatch) {
    try {
        throw math_exception("test");
    } catch (const math_exception&) {
        SUCCEED();
    } catch (...) {
        FAIL() << "Caught as unknown";
    }
}

TEST(ModTest, FloatingPoint) {
    EXPECT_DOUBLE_EQ(mod(5.5, 2.0), 1.5);
    EXPECT_DOUBLE_EQ(mod(5.0, 2.0), 1.0);
    EXPECT_THROW(ignore = mod<float64_t>(1.0, 0.0), math_exception);
    EXPECT_TRUE(is_nan(mod<float64_t>(numeric_traits<float64_t>::quiet_nan(), 1.0)));
    EXPECT_TRUE(is_nan(mod<float64_t>(1.0, numeric_traits<float64_t>::quiet_nan())));
    EXPECT_TRUE(is_nan(mod<float64_t>(numeric_traits<float64_t>::infinity(), 1.0)));
}

TEST(ModTest, Integral) {
    EXPECT_EQ(mod(7, 3), 1);
    EXPECT_EQ(mod(-7, 3), -1);
    EXPECT_THROW(ignore = mod(1, 0), math_exception);
}

TEST(PowerTest, IntegerBase) {
    EXPECT_EQ(power(2, 0), 1);
    EXPECT_EQ(power(2, 1), 2);
    EXPECT_EQ(power(2, 10), 1024);
    EXPECT_EQ(power(3, 5), 243);
}

TEST(PowerTest, FloatingBase) {
    EXPECT_DOUBLE_EQ(static_cast<double>(power(2.0L, 10)), 1024.0);
    EXPECT_DOUBLE_EQ(static_cast<double>(power(0.5L, 2)), 0.25);
}

TEST(ExponentialTest, IntegerExponents) {
    EXPECT_DOUBLE_EQ(static_cast<double>(exponential(0)), 1.0);
    EXPECT_DOUBLE_EQ(static_cast<double>(exponential(1)), static_cast<double>(constants::EULER));
    EXPECT_DOUBLE_EQ(static_cast<double>(exponential(2)), static_cast<double>(constants::EULER * constants::EULER));
}

TEST(LogarithmETest, SpecialValues) {
    EXPECT_DOUBLE_EQ(static_cast<double>(logarithm_e(1.0L)), 0.0);
    EXPECT_TRUE(is_nan(logarithm_e(-1.0L)));
    EXPECT_TRUE(is_infinity(logarithm_e(0.0L)) && logarithm_e(0.0L) < 0);
    EXPECT_TRUE(is_nan(logarithm_e(numeric_traits<decimal_t>::quiet_nan())));
}

TEST(LogarithmETest, KnownValues) {
    decimal_t e = constants::EULER;
    EXPECT_NEAR(static_cast<double>(logarithm_e(e)), 1.0, 1e-12);
    EXPECT_NEAR(static_cast<double>(logarithm_e(e * e)), 2.0, 1e-12);
    EXPECT_NEAR(static_cast<double>(logarithm_e(2.0L)), 0.6931471805599453094, 1e-12);
}

TEST(LogarithmTest, Base10) {
    EXPECT_NEAR(static_cast<double>(logarithm(100.0L, 10)), 2.0, 1e-12);
    EXPECT_NEAR(static_cast<double>(logarithm(1000.0L, 10)), 3.0, 1e-12);
}

TEST(LogarithmTest, Base2) {
    EXPECT_NEAR(static_cast<double>(logarithm_2(8.0L)), 3.0, 1e-12);
    EXPECT_NEAR(static_cast<double>(logarithm_2(1.0L)), 0.0, 1e-12);
}

TEST(LogarithmTest, Base10Wrapper) {
    EXPECT_NEAR(static_cast<double>(logarithm_10(100.0L)), 2.0, 1e-12);
    EXPECT_NEAR(static_cast<double>(logarithm_10(1.0L)), 0.0, 1e-12);
}

TEST(SquareRootTest, Normal) {
    EXPECT_NEAR(static_cast<double>(square_root(4.0L)), 2.0, 1e-12);
    EXPECT_NEAR(static_cast<double>(square_root(2.0L)), 1.4142135623730950488, 1e-12);
}

TEST(SquareRootTest, EdgeCases) {
    EXPECT_EQ(square_root(0.0L), 0.0L);
    EXPECT_TRUE(is_nan(square_root(-1.0L)));
    EXPECT_TRUE(is_nan(square_root(numeric_traits<decimal_t>::quiet_nan())));
    EXPECT_TRUE(is_infinity(square_root(numeric_traits<decimal_t>::infinity())));
}

TEST(CubeRootTest, Normal) {
    EXPECT_NEAR(static_cast<double>(cube_root(8.0L)), 2.0, 1e-12);
    EXPECT_NEAR(static_cast<double>(cube_root(-8.0L)), -2.0, 1e-12);
    EXPECT_NEAR(static_cast<double>(cube_root(0.0L)), 0.0, 1e-12);
}

TEST(CubeRootTest, EdgeCases) {
    EXPECT_TRUE(is_nan(cube_root(numeric_traits<decimal_t>::quiet_nan())));
    EXPECT_TRUE(is_infinity(cube_root(numeric_traits<decimal_t>::infinity())));
    EXPECT_TRUE(is_infinity(cube_root(-numeric_traits<decimal_t>::infinity())) &&
                cube_root(-numeric_traits<decimal_t>::infinity()) < 0);
}

TEST(FactorialTest, SmallValues) {
    EXPECT_EQ(factorial(0), 1);
    EXPECT_EQ(factorial(1), 1);
    EXPECT_EQ(factorial(5), 120);
    EXPECT_EQ(factorial(10), 3628800);
}

TEST(FloorTest, Normal) {
    EXPECT_DOUBLE_EQ(static_cast<double>(neforce::floor(3.7L)), 3.0);
    EXPECT_DOUBLE_EQ(static_cast<double>(neforce::floor(-3.7L)), -4.0);
    EXPECT_DOUBLE_EQ(static_cast<double>(neforce::floor(0.0L)), 0.0);
    EXPECT_DOUBLE_EQ(static_cast<double>(neforce::floor(-0.1L)), -1.0);
}

TEST(FloorTest, LargeValues) { EXPECT_DOUBLE_EQ(static_cast<double>(neforce::floor(1e20L)), 1e20); }

TEST(FloorBitTest, Precision) {
    EXPECT_NEAR(static_cast<double>(floor_bit(3.14159L, 2)), 3.14, 1e-12);
    EXPECT_NEAR(static_cast<double>(floor_bit(-3.14159L, 2)), -3.15, 1e-12);
}

TEST(IsEvenIntegerTest, Values) {
    EXPECT_TRUE(is_even_integer(2.0L));
    EXPECT_FALSE(is_even_integer(3.0L));
    EXPECT_FALSE(is_even_integer(2.5L));
    EXPECT_TRUE(is_even_integer(-4.0L));
    EXPECT_FALSE(is_even_integer(-3.0L));
}

TEST(CeilTest, Normal) {
    EXPECT_DOUBLE_EQ(static_cast<double>(neforce::ceil(3.2L)), 4.0);
    EXPECT_DOUBLE_EQ(static_cast<double>(neforce::ceil(-3.2L)), -3.0);
    EXPECT_DOUBLE_EQ(static_cast<double>(neforce::ceil(0.0L)), 0.0);
    EXPECT_DOUBLE_EQ(static_cast<double>(neforce::ceil(0.1L)), 1.0);
}

TEST(CeilBitTest, Precision) {
    EXPECT_NEAR(static_cast<double>(ceil_bit(3.14159L, 2)), 3.15, 1e-12);
    EXPECT_NEAR(static_cast<double>(ceil_bit(-3.14159L, 2)), -3.14, 1e-12);
}

TEST(RoundTest, Normal) {
    EXPECT_DOUBLE_EQ(static_cast<double>(neforce::round(3.4L)), 3.0);
    EXPECT_DOUBLE_EQ(static_cast<double>(neforce::round(3.5L)), 4.0);
    EXPECT_DOUBLE_EQ(static_cast<double>(neforce::round(2.5L)), 2.0);
    EXPECT_DOUBLE_EQ(static_cast<double>(neforce::round(-3.5L)), -4.0);
    EXPECT_DOUBLE_EQ(static_cast<double>(neforce::round(-2.5L)), -2.0);
}

TEST(RoundBitTest, Precision) {
    EXPECT_NEAR(static_cast<double>(round_bit(3.14159L, 2)), 3.14, 1e-12);
    EXPECT_NEAR(static_cast<double>(round_bit(2.5L, 0)), 3.0, 1e-12);
    EXPECT_NEAR(static_cast<double>(round_bit(3.5L, 0)), 4.0, 1e-12);
}

TEST(TruncateBitTest, Precision) {
    EXPECT_NEAR(static_cast<double>(truncate_bit(3.14159L, 2)), 3.14, 1e-12);
    EXPECT_NEAR(static_cast<double>(truncate_bit(-3.14159L, 2)), -3.14, 1e-12);
}

TEST(TruncateTest, ToInteger) {
    EXPECT_DOUBLE_EQ(static_cast<double>(truncate(3.9L)), 3.0);
    EXPECT_DOUBLE_EQ(static_cast<double>(truncate(-3.9L)), -3.0);
}

TEST(TruncateTest, WithDecimalPlaces) {
    EXPECT_NEAR(static_cast<double>(truncate(3.14159L, 2)), 3.14, 1e-12);
    EXPECT_NEAR(static_cast<double>(truncate(-3.14159L, 2)), -3.14, 1e-12);
}

TEST(AroundMultipleTest, Basic) {
    EXPECT_TRUE(around_multiple(4.0L, 2.0L));
    EXPECT_TRUE(around_multiple(3.000000001L, 1.0L, 1e-6L));
    EXPECT_FALSE(around_multiple(3.5L, 1.0L));
}

TEST(AroundMultipleTest, ZeroAxis) { EXPECT_THROW(ignore = around_multiple(1.0L, 0.0L), math_exception); }

TEST(AroundPiTest, Basic) {
    EXPECT_TRUE(around_pi(constants::PI));
    EXPECT_TRUE(around_pi(2 * constants::PI));
    EXPECT_FALSE(around_pi(constants::PI / 2));
}

TEST(AroundZeroTest, Basic) {
    EXPECT_TRUE(around_zero(0.0L));
    EXPECT_TRUE(around_zero(1e-10L, 1e-9L));
    EXPECT_FALSE(around_zero(0.1L));
}

TEST(RemainderTest, Normal) {
    EXPECT_NEAR(static_cast<double>(neforce::remainder(5.0L, 2.0L)), 1.0, 1e-12);
    EXPECT_NEAR(static_cast<double>(neforce::remainder(5.0L, 3.0L)), -1.0, 1e-12);
    EXPECT_NEAR(static_cast<double>(neforce::remainder(4.5L, 2.0L)), 0.5, 1e-12);
}

TEST(RemainderTest, EdgeCases) {
    EXPECT_TRUE(is_nan(neforce::remainder(1.0L, 0.0L)));
    EXPECT_TRUE(is_nan(neforce::remainder(numeric_traits<decimal_t>::quiet_nan(), 1.0L)));
}

TEST(FloatPartTest, Basic) {
    EXPECT_NEAR(static_cast<double>(float_part(3.1415L)), 0.1415, 1e-12);
    EXPECT_NEAR(static_cast<double>(float_part(-3.1415L)), -0.1415, 1e-12);
    EXPECT_NEAR(static_cast<double>(float_part(3.0L)), 0.0, 1e-12);
}

TEST(FloatApartTest, Basic) {
    int64_t int_part;
    decimal_t frac = float_apart(3.1415L, &int_part);
    EXPECT_EQ(int_part, 3);
    EXPECT_NEAR(static_cast<double>(frac), 0.1415, 1e-12);

    frac = float_apart(-3.1415L, &int_part);
    EXPECT_EQ(int_part, -3);
    EXPECT_NEAR(static_cast<double>(frac), -0.1415, 1e-12);
}

TEST(SineTest, SpecialValues) {
    EXPECT_NEAR(static_cast<double>(sine(0.0L)), 0.0, 1e-12);
    EXPECT_NEAR(static_cast<double>(sine(constants::PI / 2)), 1.0, 1e-12);
    EXPECT_NEAR(static_cast<double>(sine(constants::PI)), 0.0, 1e-10);
    EXPECT_NEAR(static_cast<double>(sine(3 * constants::PI / 2)), -1.0, 1e-10);
    EXPECT_NEAR(static_cast<double>(sine(2 * constants::PI)), 0.0, 1e-10);
    EXPECT_TRUE(is_nan(sine(numeric_traits<decimal_t>::quiet_nan())));
    EXPECT_TRUE(is_nan(sine(numeric_traits<decimal_t>::infinity())));
}

TEST(CosineTest, SpecialValues) {
    EXPECT_NEAR(static_cast<double>(cosine(0.0L)), 1.0, 1e-12);
    EXPECT_NEAR(static_cast<double>(cosine(constants::PI / 2)), 0.0, 1e-12);
    EXPECT_NEAR(static_cast<double>(cosine(constants::PI)), -1.0, 1e-10);
    EXPECT_NEAR(static_cast<double>(cosine(3 * constants::PI / 2)), 0.0, 1e-10);
    EXPECT_NEAR(static_cast<double>(cosine(2 * constants::PI)), 1.0, 1e-10);
    EXPECT_TRUE(is_nan(cosine(numeric_traits<decimal_t>::quiet_nan())));
    EXPECT_TRUE(is_nan(cosine(numeric_traits<decimal_t>::infinity())));
}

TEST(TangentTest, SpecialValues) {
    EXPECT_NEAR(static_cast<double>(tangent(0.0L)), 0.0, 1e-12);
    EXPECT_NEAR(static_cast<double>(tangent(constants::PI / 4)), 1.0, 1e-12);
    EXPECT_TRUE(is_nan(tangent(numeric_traits<decimal_t>::quiet_nan())));
    EXPECT_TRUE(is_nan(tangent(numeric_traits<decimal_t>::infinity())));
}

TEST(TangentTest, Asymptotic) {
    EXPECT_TRUE(is_infinity(tangent(constants::PI / 2)));
    EXPECT_NEAR(static_cast<double>(tangent(-constants::PI / 2)), 0.0, 1e-10);
}

TEST(CotangentTest, Basic) {
    EXPECT_NEAR(static_cast<double>(cotangent(constants::PI / 4)), 1.0, 1e-12);
    EXPECT_NEAR(static_cast<double>(cotangent(constants::PI / 2)), 0.0, 1e-12);
}

TEST(ArctangentTest, SpecialValues) {
    EXPECT_NEAR(static_cast<double>(arctangent(0.0L)), 0.0, 1e-12);
    EXPECT_NEAR(static_cast<double>(arctangent(1.0L)), static_cast<double>(constants::PI / 4), 1e-12);
    EXPECT_NEAR(static_cast<double>(arctangent(-1.0L)), static_cast<double>(-constants::PI / 4), 1e-12);
    EXPECT_NEAR(static_cast<double>(arctangent(numeric_traits<decimal_t>::infinity())),
                static_cast<double>(constants::PI / 2), 1e-12);
    EXPECT_NEAR(static_cast<double>(arctangent(-numeric_traits<decimal_t>::infinity())),
                static_cast<double>(-constants::PI / 2), 1e-12);
    EXPECT_TRUE(is_nan(arctangent(numeric_traits<decimal_t>::quiet_nan())));
}

TEST(ArcsineTest, DomainAndValues) {
    EXPECT_NEAR(static_cast<double>(arcsine(0.0L)), 0.0, 1e-12);
    EXPECT_NEAR(static_cast<double>(arcsine(1.0L)), static_cast<double>(constants::PI / 2), 1e-12);
    EXPECT_NEAR(static_cast<double>(arcsine(-1.0L)), static_cast<double>(-constants::PI / 2), 1e-12);
    EXPECT_TRUE(is_nan(arcsine(2.0L)));
    EXPECT_TRUE(is_nan(arcsine(numeric_traits<decimal_t>::quiet_nan())));
}

TEST(ArccosineTest, DomainAndValues) {
    EXPECT_NEAR(static_cast<double>(arccosine(1.0L)), 0.0, 1e-12);
    EXPECT_NEAR(static_cast<double>(arccosine(0.0L)), static_cast<double>(constants::PI / 2), 1e-12);
    EXPECT_NEAR(static_cast<double>(arccosine(-1.0L)), static_cast<double>(constants::PI), 1e-12);
    EXPECT_TRUE(is_nan(arccosine(2.0L)));
    EXPECT_TRUE(is_nan(arccosine(numeric_traits<decimal_t>::quiet_nan())));
}

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

#if 0

TEST(Uint128ConstructionTest, DefaultConstructor) {
    uint128_t a;
    EXPECT_EQ(a.lo, 0u);
    EXPECT_EQ(a.hi, 0u);
}

TEST(Uint128ConstructionTest, Int32ConstructorPositive) {
    uint128_t a(42);
    EXPECT_EQ(a.lo, 42u);
    EXPECT_EQ(a.hi, 0u);
}

TEST(Uint128ConstructionTest, Int32ConstructorNegative) {
    uint128_t a(-1);
    EXPECT_EQ(a.lo, 0xFFFFFFFFFFFFFFFFULL);
    EXPECT_EQ(a.hi, 0xFFFFFFFFFFFFFFFFULL);
}

TEST(Uint128ConstructionTest, Uint32Constructor) {
    uint128_t a(100u);
    EXPECT_EQ(a.lo, 100u);
    EXPECT_EQ(a.hi, 0u);
}

TEST(Uint128ConstructionTest, Uint64Constructor) {
    uint64_t val = 0x123456789ABCDEF0ULL;
    uint128_t a(val);
    EXPECT_EQ(a.lo, val);
    EXPECT_EQ(a.hi, 0u);
}

TEST(Uint128ConstructionTest, HighLowConstructor) {
    uint128_t a(0xDEADBEEFULL, 0xCAFEBABEULL);
    EXPECT_EQ(a.hi, 0xDEADBEEFULL);
    EXPECT_EQ(a.lo, 0xCAFEBABEULL);
}

TEST(Uint128ConstructionTest, CopyConstructor) {
    uint128_t a(0xAAAAAAAAULL, 0xBBBBBBBBULL);
    uint128_t b(a);
    EXPECT_EQ(b.hi, a.hi);
    EXPECT_EQ(b.lo, a.lo);
}

TEST(Uint128ConstructionTest, MoveConstructor) {
    uint128_t a(1, 2);
    uint128_t b(move(a));
    EXPECT_EQ(b.hi, 1u);
    EXPECT_EQ(b.lo, 2u);
}

TEST(Uint128ConstructionTest, CopyAssignment) {
    uint128_t a(3, 4);
    uint128_t b;
    b = a;
    EXPECT_EQ(b.hi, 3u);
    EXPECT_EQ(b.lo, 4u);
}

TEST(Uint128ConstructionTest, MoveAssignment) {
    uint128_t a(5, 6);
    uint128_t b;
    b = move(a);
    EXPECT_EQ(b.hi, 5u);
    EXPECT_EQ(b.lo, 6u);
}

TEST(Uint128ConstructionTest, StringConstructorDecimal) {
    uint128_t a("123456789012345678901234567890"_s);
    EXPECT_EQ(a.to_string(), "123456789012345678901234567890");
}

TEST(Uint128ConstructionTest, StringConstructorHex) {
    uint128_t a("0x1A2B3C4D5E6F7890"_s, 0);
    string s = a.to_string();
    EXPECT_EQ(a, uint128_t(0x1A2B3C4DULL, 0x5E6F7890ULL));
}

TEST(Uint128ConstructionTest, StringViewConstructor) {
    string s = "99999999999999999999";
    uint128_t a(s);
    EXPECT_EQ(a.to_string(), s);
}

TEST(Uint128ConversionTest, ToBool) {
    uint128_t zero;
    EXPECT_FALSE(static_cast<bool>(zero));
    uint128_t one(1);
    EXPECT_TRUE(static_cast<bool>(one));
}

TEST(Uint128ConversionTest, ToChar) {
    uint128_t a(65);
    EXPECT_EQ(static_cast<char>(a), 'A');
}

TEST(Uint128ConversionTest, ToInt8) {
    uint128_t a(127);
    EXPECT_EQ(static_cast<int8_t>(a), 127);
}

TEST(Uint128ConversionTest, ToUint8) {
    uint128_t a(255);
    EXPECT_EQ(static_cast<uint8_t>(a), 255);
}

TEST(Uint128ConversionTest, ToUint16) {
    uint128_t a(65535);
    EXPECT_EQ(static_cast<uint16_t>(a), 65535);
}

TEST(Uint128ConversionTest, ToUint32) {
    uint128_t a(0xFFFFFFFFULL);
    EXPECT_EQ(static_cast<uint32_t>(a), 0xFFFFFFFFU);
}

TEST(Uint128ConversionTest, ToUint64) {
    uint128_t a(0x123456789ABCDEF0ULL);
    EXPECT_EQ(static_cast<uint64_t>(a), 0x123456789ABCDEF0ULL);
}

TEST(Uint128ConversionTest, ToInt128) {
    uint128_t a(0x8000000000000000ULL, 0x1234ULL);
    int128_t b = static_cast<int128_t>(a);
    EXPECT_EQ(b.hi, a.hi);
    EXPECT_EQ(b.lo, a.lo);
}

TEST(Uint128ConversionTest, ToInt128Method) {
    uint128_t a(0x7FFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL);
    int128_t b = a.to_int128();
    EXPECT_EQ(b.hi, a.hi);
    EXPECT_EQ(b.lo, a.lo);
}

TEST(Uint128ComparisonTest, Equal) {
    uint128_t a(1, 2);
    uint128_t b(1, 2);
    uint128_t c(1, 3);
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
}

TEST(Uint128ComparisonTest, NotEqual) {
    uint128_t a(1, 2);
    uint128_t b(1, 3);
    EXPECT_TRUE(a != b);
    EXPECT_FALSE(a != a);
}

TEST(Uint128ComparisonTest, Less) {
    uint128_t a(0ULL, 5ULL);
    uint128_t b(0ULL, 10ULL);
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(b < a);
    uint128_t c(1, 0);
    uint128_t d(0, 0xFFFFFFFFFFFFFFFFULL);
    EXPECT_TRUE(d < c);
    EXPECT_FALSE(c < d);
}

TEST(Uint128ComparisonTest, Greater) {
    uint128_t a(10, 20);
    uint128_t b(10, 15);
    EXPECT_TRUE(a > b);
    EXPECT_FALSE(b > a);
}

TEST(Uint128ComparisonTest, LessOrEqual) {
    uint128_t a(0ULL, 0ULL);
    uint128_t b(0ULL, 0ULL);
    uint128_t c(0ULL, 1ULL);
    EXPECT_TRUE(a <= b);
    EXPECT_TRUE(a <= c);
    EXPECT_FALSE(c <= a);
}

TEST(Uint128ComparisonTest, GreaterOrEqual) {
    uint128_t a(1, 5);
    uint128_t b(1, 5);
    uint128_t c(1, 3);
    EXPECT_TRUE(a >= b);
    EXPECT_TRUE(a >= c);
    EXPECT_FALSE(c >= a);
}

TEST(Uint128UnaryOperatorsTest, Negate) {
    uint128_t a(0ULL, 1ULL);
    uint128_t b = -a;
    EXPECT_EQ(b.hi, 0xFFFFFFFFFFFFFFFFULL);
    EXPECT_EQ(b.lo, 0xFFFFFFFFFFFFFFFFULL);
    uint128_t c(0ULL, 0ULL);
    uint128_t d = -c;
    EXPECT_EQ(d.hi, 0u);
    EXPECT_EQ(d.lo, 0u);
}

TEST(Uint128UnaryOperatorsTest, BitwiseNot) {
    uint128_t a(0xF0F0F0F0F0F0F0F0ULL, 0x0F0F0F0F0F0F0F0FULL);
    uint128_t b = ~a;
    EXPECT_EQ(b.hi, 0x0F0F0F0F0F0F0F0FULL);
    EXPECT_EQ(b.lo, 0xF0F0F0F0F0F0F0F0ULL);
}

TEST(Uint128UnaryOperatorsTest, PreIncrement) {
    uint128_t a(0ULL, 10ULL);
    ++a;
    EXPECT_EQ(a.lo, 11u);
    EXPECT_EQ(a.hi, 0u);
    uint128_t b(0, 0xFFFFFFFFFFFFFFFFULL);
    ++b;
    EXPECT_EQ(b.lo, 0u);
    EXPECT_EQ(b.hi, 1u);
}

TEST(Uint128UnaryOperatorsTest, PreDecrement) {
    uint128_t a(0ULL, 10ULL);
    --a;
    EXPECT_EQ(a.lo, 9u);
    uint128_t b(1, 0);
    --b;
    EXPECT_EQ(b.hi, 0u);
    EXPECT_EQ(b.lo, 0xFFFFFFFFFFFFFFFFULL);
}

TEST(Uint128ArithmeticTest, AddAssign) {
    uint128_t a(0ULL, 100ULL);
    a += uint128_t(0ULL, 50ULL);
    EXPECT_EQ(a.lo, 150u);
    uint128_t b(0, 0xFFFFFFFFFFFFFFFFULL);
    b += uint128_t(0ULL, 1ULL);
    EXPECT_EQ(b.hi, 1u);
    EXPECT_EQ(b.lo, 0u);
}

TEST(Uint128ArithmeticTest, SubAssign) {
    uint128_t a(0ULL, 100ULL);
    a -= uint128_t(0ULL, 30ULL);
    EXPECT_EQ(a.lo, 70u);
    uint128_t b(0ULL, 0ULL);
    b -= uint128_t(0ULL, 1ULL);
    EXPECT_EQ(b.hi, 0xFFFFFFFFFFFFFFFFULL);
    EXPECT_EQ(b.lo, 0xFFFFFFFFFFFFFFFFULL);
}

TEST(Uint128ArithmeticTest, MulAssign) {
    uint128_t a(0ULL, 2ULL);
    a *= uint128_t(0ULL, 3ULL);
    EXPECT_EQ(a, uint128_t(6));
    uint128_t b(0, 0xFFFFFFFFFFFFFFFFULL);
    b *= uint128_t(0ULL, 2ULL);
    EXPECT_EQ(b.hi, 1u);
    EXPECT_EQ(b.lo, 0xFFFFFFFFFFFFFFFEULL);
}

TEST(Uint128ArithmeticTest, DivAssign) {
    uint128_t a(0ULL, 100ULL);
    a /= uint128_t(0ULL, 3ULL);
    EXPECT_EQ(a.lo, 33u);
}

TEST(Uint128ArithmeticTest, ModAssign) {
    uint128_t a(0ULL, 100ULL);
    a %= uint128_t(0ULL, 7ULL);
    EXPECT_EQ(a.lo, 2u);
}

TEST(Uint128ArithmeticTest, SubtractFreeFunction) {
    uint128_t a(0ULL, 50ULL);
    uint128_t b(0ULL, 20ULL);
    uint128_t c = a - b;
    EXPECT_EQ(c.lo, 30u);
    uint128_t d = b - a;
    EXPECT_EQ(d.hi, 0xFFFFFFFFFFFFFFFFULL);
    EXPECT_EQ(d.lo, 0xFFFFFFFFFFFFFFFFULL - 29u);
}

TEST(Uint128ArithmeticTest, AddCarryLarge) {
    uint128_t a(0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL);
    uint128_t b(1, 0);
    a += b;
    EXPECT_EQ(a.hi, 0u);
    EXPECT_EQ(a.lo, 0u);
}

TEST(Uint128BitwiseTest, AndAssign) {
    uint128_t a(0xFF00FF00FF00FF00ULL, 0x0FF00FF00FF00FF0ULL);
    uint128_t b(0xF0F0F0F0F0F0F0F0ULL, 0xFFFF0000FFFF0000ULL);
    a &= b;
    EXPECT_EQ(a.hi, 0xF000F000F000F000ULL);
    EXPECT_EQ(a.lo, 0x0FF000000FF00000ULL);
}

TEST(Uint128BitwiseTest, OrAssign) {
    uint128_t a(0x00FF00FF00FF00FFULL, 0x0000FFFF0000FFFFULL);
    uint128_t b(0xFF00FF00FF00FF00ULL, 0xFFFF0000FFFF0000ULL);
    a |= b;
    EXPECT_EQ(a.hi, 0xFFFFFFFFFFFFFFFFULL);
    EXPECT_EQ(a.lo, 0xFFFFFFFFFFFFFFFFULL);
}

TEST(Uint128BitwiseTest, XorAssign) {
    uint128_t a(0xAAAAAAAAAAAAAAAAULL, 0x5555555555555555ULL);
    a ^= uint128_t(0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL);
    EXPECT_EQ(a.hi, 0x5555555555555555ULL);
    EXPECT_EQ(a.lo, 0xAAAAAAAAAAAAAAAAULL);
}

TEST(Uint128BitwiseTest, LeftShiftAssign) {
    uint128_t a(0ULL, 1ULL);
    a <<= 64;
    EXPECT_EQ(a.hi, 1u);
    EXPECT_EQ(a.lo, 0u);
    uint128_t b(0x1234567890ABCDEFULL, 0x0FEDCBA987654321ULL);
    b <<= 128;
    EXPECT_EQ(b.hi, 0u);
    EXPECT_EQ(b.lo, 0u);
}

TEST(Uint128BitwiseTest, RightShiftAssign) {
    uint128_t a(1, 0);
    a >>= 64;
    EXPECT_EQ(a.hi, 0u);
    EXPECT_EQ(a.lo, 1u);
    uint128_t b(0xABCD, 0xEF01);
    b >>= 128;
    EXPECT_EQ(b.hi, 0u);
    EXPECT_EQ(b.lo, 0u);
}

TEST(Uint128StaticMethodsTest, Mul64) {
    auto prod = uint128_t::mul64(0xFFFFFFFFFFFFFFFFULL, 2);
    EXPECT_EQ(prod.hi, 1u);
    EXPECT_EQ(prod.lo, 0xFFFFFFFFFFFFFFFEULL);
}

TEST(Uint128StaticMethodsTest, Div64) {
    uint128_t a(0ULL, 100ULL);
    uint64_t rem = 0;
    uint64_t q = a.div64(3, &rem);
    EXPECT_EQ(q, 33u);
    EXPECT_EQ(rem, 1u);
}

TEST(Uint128StaticMethodsTest, Div64Large) {
    uint128_t a(0x123456789ABCDEF0ULL, 0x0FEDCBA987654321ULL);
    uint64_t q = a.div64(0x100000000ULL, nullptr);
    EXPECT_NE(q, 0u);
}

TEST(Uint128StaticMethodsTest, MinMax) {
    EXPECT_EQ(uint128_t::min(), uint128_t(0));
    EXPECT_EQ(uint128_t::max(), uint128_t(0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL));
}

TEST(Uint128StaticMethodsTest, Parse) {
    auto a = uint128_t::parse("12345678901234567890");
    EXPECT_EQ(a.to_string(), "12345678901234567890");
}

TEST(Uint128StaticMethodsTest, ToString) {
    uint128_t a(0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL);
    string s = a.to_string();
    EXPECT_FALSE(s.empty());
    uint128_t b(s);
    EXPECT_EQ(a, b);
}

TEST(Uint128StaticMethodsTest, ToHash) {
    uint128_t a(1, 2);
    size_t h = a.to_hash();
    EXPECT_NE(h, 0u);
    uint128_t b(1, 2);
    EXPECT_EQ(a.to_hash(), b.to_hash());
}

TEST(Uint128LiteralTest, Uint128Literal) {
    auto a = 123_u128;
    EXPECT_EQ(a, uint128_t(123));
}

TEST(Uint128LiteralTest, Uint128StringLiteral) {
    auto a = "340282366920938463463374607431768211455"_u128;
    EXPECT_EQ(a, uint128_t::max());
}

TEST(Uint128ExceptionTest, DivisionByZero) {
    uint128_t a(10);
    EXPECT_THROW(a /= uint128_t(0), math_exception);
    EXPECT_THROW(a %= uint128_t(0), math_exception);
}

TEST(Uint128ExceptionTest, InvalidString) {
    EXPECT_THROW(uint128_t("not_a_number"_s), typecast_exception);
}

TEST(Int128ConstructionTest, DefaultConstructor) {
    int128_t a;
    EXPECT_EQ(a.lo, 0u);
    EXPECT_EQ(a.hi, 0u);
}

TEST(Int128ConstructionTest, Int32Constructor) {
    int128_t a(42);
    EXPECT_EQ(a.lo, 42u);
    EXPECT_EQ(a.hi, 0u);
    int128_t b(-5);
    EXPECT_EQ(b.lo, 0xFFFFFFFFFFFFFFFBULL);
    EXPECT_EQ(b.hi, 0xFFFFFFFFFFFFFFFFULL);
}

TEST(Int128ConstructionTest, Int64Constructor) {
    int128_t a(0x7FFFFFFFFFFFFFFFLL);
    EXPECT_EQ(a.lo, 0x7FFFFFFFFFFFFFFFULL);
    EXPECT_EQ(a.hi, 0u);
    int128_t b(-1);
    EXPECT_EQ(b.lo, 0xFFFFFFFFFFFFFFFFULL);
    EXPECT_EQ(b.hi, 0xFFFFFFFFFFFFFFFFULL);
}

TEST(Int128ConstructionTest, LowBoolConstructor) {
    int128_t a(100, false);
    EXPECT_FALSE(a.is_negative());
    int128_t b(-100, true);
    EXPECT_TRUE(b.is_negative());
}

TEST(Int128ConstructionTest, HighLowConstructor) {
    int128_t a(0x8000000000000000ULL, 0ULL);
    int128_t b(0x7FFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL);
    EXPECT_EQ(a.hi, 0x8000000000000000ULL);
    EXPECT_EQ(b.lo, 0xFFFFFFFFFFFFFFFFULL);
}

TEST(Int128ConstructionTest, CopyConstructor) {
    int128_t a(-100);
    int128_t b(a);
    EXPECT_EQ(a, b);
}

TEST(Int128ConstructionTest, MoveConstructor) {
    int128_t a(123);
    int128_t b(move(a));
    EXPECT_EQ(b, int128_t(123));
}

TEST(Int128ConstructionTest, AssignmentFromUint128) {
    uint128_t u(0x7FFF, 0x8000);
    int128_t a;
    a = u;
    EXPECT_EQ(a.hi, 0x7FFFu);
    EXPECT_EQ(a.lo, 0x8000u);
}

TEST(Int128ConstructionTest, StringConstructor) {
    int128_t a("-12345678901234567890"_s);
    string s = a.to_string();
    EXPECT_EQ(s, "-12345678901234567890"_s);
}

TEST(Int128ConversionTest, ToBool) {
    int128_t zero;
    EXPECT_FALSE(static_cast<bool>(zero));
    int128_t one(1);
    EXPECT_TRUE(static_cast<bool>(one));
}

TEST(Int128ConversionTest, NarrowingConversions) {
    int128_t a(65);
    EXPECT_EQ(static_cast<char>(a), 'A');
    EXPECT_EQ(static_cast<int8_t>(a), 65);
    int128_t b(-1);
    EXPECT_EQ(static_cast<int8_t>(b), -1);
    EXPECT_EQ(static_cast<int16_t>(b), -1);
    EXPECT_EQ(static_cast<int32_t>(b), -1);
    EXPECT_EQ(static_cast<int64_t>(b), -1);
    EXPECT_EQ(static_cast<uint8_t>(b), 0xFFu);
}

TEST(Int128ConversionTest, ToUint128) {
    int128_t a(-1);
    uint128_t u = a.to_uint128();
    EXPECT_EQ(u.hi, 0xFFFFFFFFFFFFFFFFULL);
    EXPECT_EQ(u.lo, 0xFFFFFFFFFFFFFFFFULL);
}

TEST(Int128ConversionTest, IsNegative) {
    int128_t a(0);
    EXPECT_FALSE(a.is_negative());
    int128_t b(-1);
    EXPECT_TRUE(b.is_negative());
}

TEST(Int128ComparisonTest, Equal) {
    int128_t a(10);
    int128_t b(10);
    int128_t c(-10);
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
}

TEST(Int128ComparisonTest, NotEqual) {
    int128_t a(5);
    int128_t b(6);
    EXPECT_TRUE(a != b);
}

TEST(Int128ComparisonTest, Less) {
    int128_t a(-5);
    int128_t b(5);
    EXPECT_TRUE(a < b);
    int128_t c(10);
    int128_t d(20);
    EXPECT_TRUE(c < d);
    EXPECT_FALSE(d < c);
}

TEST(Int128ComparisonTest, Greater) {
    int128_t a(100);
    int128_t b(0);
    EXPECT_TRUE(a > b);
    EXPECT_FALSE(b > a);
}

TEST(Int128ComparisonTest, LessOrEqual) {
    int128_t a(-1);
    int128_t b(0);
    EXPECT_TRUE(a <= b);
    EXPECT_TRUE(b <= b);
}

TEST(Int128ComparisonTest, GreaterOrEqual) {
    int128_t a(300);
    int128_t b(300);
    EXPECT_TRUE(a >= b);
    EXPECT_TRUE(a >= int128_t(200));
}

TEST(Int128UnaryOperatorsTest, Negate) {
    int128_t a(50);
    int128_t b = -a;
    EXPECT_EQ(b, int128_t(-50));
    int128_t c(-0x7FFFFFFFFFFFFFFFLL);
    int128_t d = -c;
    EXPECT_EQ(d.lo, 0x7FFFFFFFFFFFFFFFULL);
    EXPECT_EQ(d.hi, 0u);
}

TEST(Int128UnaryOperatorsTest, BitwiseNot) {
    int128_t a(0ULL, 0xF0F0ULL);
    int128_t b = ~a;
    EXPECT_EQ(b.hi, 0xFFFFFFFFFFFFFFFFULL);
    EXPECT_EQ(b.lo, 0xFFFFFFFFFFFF0F0FULL);
}

TEST(Int128UnaryOperatorsTest, PreIncrement) {
    int128_t a(5);
    ++a;
    EXPECT_EQ(a, int128_t(6));
    int128_t b(-1);
    ++b;
    EXPECT_EQ(b, int128_t(0));
}

TEST(Int128UnaryOperatorsTest, PreDecrement) {
    int128_t a(10);
    --a;
    EXPECT_EQ(a, int128_t(9));
    int128_t b(0);
    --b;
    EXPECT_EQ(b, int128_t(-1));
}

TEST(Int128ArithmeticTest, AddAssign) {
    int128_t a(100);
    a += int128_t(50);
    EXPECT_EQ(a, int128_t(150));
    int128_t b(-10);
    b += int128_t(5);
    EXPECT_EQ(b, int128_t(-5));
}

TEST(Int128ArithmeticTest, SubAssign) {
    int128_t a(100);
    a -= int128_t(30);
    EXPECT_EQ(a, int128_t(70));
    int128_t b(-5);
    b -= int128_t(-5);
    EXPECT_EQ(b, int128_t(0));
}

TEST(Int128ArithmeticTest, MulAssign) {
    int128_t a(6);
    a *= int128_t(-7);
    EXPECT_EQ(a, int128_t(-42));
    int128_t b(-2);
    b *= int128_t(-3);
    EXPECT_EQ(b, int128_t(6));
}

TEST(Int128ArithmeticTest, DivAssign) {
    int128_t a(100);
    a /= int128_t(3);
    EXPECT_EQ(a, int128_t(33));
    int128_t b(-100);
    b /= int128_t(3);
    EXPECT_EQ(b, int128_t(-33));
    int128_t c(-100);
    c /= int128_t(-3);
    EXPECT_EQ(c, int128_t(33));
}

TEST(Int128ArithmeticTest, ModAssign) {
    int128_t a(100);
    a %= int128_t(7);
    EXPECT_EQ(a, int128_t(2));
    int128_t b(-100);
    b %= int128_t(7);
    EXPECT_EQ(b, int128_t(-2));
}

TEST(Int128ArithmeticTest, SubtractFreeFunction) {
    int128_t a(10);
    int128_t b(20);
    EXPECT_EQ(a - b, int128_t(-10));
}

TEST(Int128BitwiseTest, AndAssign) {
    int128_t a(0xFFFF0000ULL, 0x0000FFFFULL);
    int128_t b(0x00FF00FFULL, 0xFF00FF00ULL);
    a &= b;
    EXPECT_EQ(a.hi, 0x00FF0000u);
    EXPECT_EQ(a.lo, 0x0000FF00u);
}

TEST(Int128BitwiseTest, OrAssign) {
    int128_t a(0x0F0F0F0FULL, 0xF0F0F0F0ULL);
    int128_t b(0xF0F0F0F0ULL, 0x0F0F0F0FULL);
    a |= b;
    EXPECT_EQ(a.hi, 0xFFFFFFFFu);
    EXPECT_EQ(a.lo, 0xFFFFFFFFu);
}

TEST(Int128BitwiseTest, XorAssign) {
    int128_t a(0xAAAAAAAAAAAAAAAAULL, 0x5555555555555555ULL);
    a ^= int128_t(0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL);
    EXPECT_EQ(a.hi, 0x5555555555555555ULL);
    EXPECT_EQ(a.lo, 0xAAAAAAAAAAAAAAAAULL);
}

TEST(Int128BitwiseTest, LeftShiftAssign) {
    int128_t a(0ULL, 1ULL);
    a <<= 64;
    EXPECT_EQ(a.hi, 1u);
    EXPECT_EQ(a.lo, 0u);
}

TEST(Int128BitwiseTest, ArithmeticRightShiftAssign) {
    int128_t pos(0x7FFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL);
    pos >>= 64;
    EXPECT_EQ(pos.hi, 0u);
    EXPECT_EQ(pos.lo, 0x7FFFFFFFFFFFFFFFULL);
    int128_t neg(-1);
    neg >>= 64;
    EXPECT_EQ(neg.hi, 0xFFFFFFFFFFFFFFFFULL);
    EXPECT_EQ(neg.lo, 0xFFFFFFFFFFFFFFFFULL);
    int128_t smallNeg(-100);
    smallNeg >>= 2;
    EXPECT_EQ(smallNeg, int128_t(-25));
}

TEST(Int128BitwiseTest, ShiftMoreThan128) {
    int128_t a(-1);
    a >>= 128;
    EXPECT_EQ(a, int128_t(-1));
    int128_t b(1);
    b <<= 128;
    EXPECT_EQ(b, int128_t(0));
}

TEST(Int128StaticMethodsTest, MinMax) {
    EXPECT_EQ(int128_t::min(), int128_t(0x8000000000000000ULL, 0ULL));
    EXPECT_EQ(int128_t::max(), int128_t(0x7FFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL));
}

TEST(Int128StaticMethodsTest, Parse) {
    auto a = int128_t::parse("-12345");
    EXPECT_EQ(a, int128_t(-12345));
}

TEST(Int128StaticMethodsTest, ToString) {
    int128_t a(0x8000000000000000ULL, 0ULL);
    string s = a.to_string();
    EXPECT_FALSE(s.empty());
    int128_t b(s);
    EXPECT_EQ(a, b);
}

TEST(Int128StaticMethodsTest, ToHash) {
    int128_t a(-123);
    size_t h = a.to_hash();
    EXPECT_NE(h, 0u);
    int128_t b(-123);
    EXPECT_EQ(a.to_hash(), b.to_hash());
}

TEST(Int128LiteralTest, Int128Literal) {
    auto a = 42_i128;
    EXPECT_EQ(a, int128_t(42));
}

TEST(Int128LiteralTest, Int128StringLiteral) {
    auto a = "-170141183460469231731687303715884105728"_i128;
    EXPECT_EQ(a, int128_t::min());
}

TEST(Int128ExceptionTest, DivisionByZero) {
    int128_t a(10);
    EXPECT_THROW(a /= int128_t(0), math_exception);
    EXPECT_THROW(a %= int128_t(0), math_exception);
}

TEST(Int128ExceptionTest, InvalidString) {
    EXPECT_THROW(int128_t("abc"_s), typecast_exception);
}

TEST(GlobalConvertersTest, ToUint128) {
    auto val = to_uint128("42");
    EXPECT_EQ(val, uint128_t(42));
    size_t idx = 0;
    ignore = to_uint128("1010", &idx, 2);
    EXPECT_EQ(idx, 4u);
}

TEST(GlobalConvertersTest, ToInt128) {
    auto val = to_int128("-123");
    EXPECT_EQ(val, int128_t(-123));
    size_t idx = 0;
    ignore = to_int128("-FF", &idx, 16);
    EXPECT_EQ(idx, 3u);
}

TEST(NumericTraitsTest, Uint128Traits) {
    EXPECT_TRUE(numeric_traits<uint128_t>::is_specialized);
    EXPECT_FALSE(numeric_traits<uint128_t>::is_signed);
    EXPECT_TRUE(numeric_traits<uint128_t>::is_integer);
    EXPECT_EQ(numeric_traits<uint128_t>::min(), uint128_t(0));
    EXPECT_EQ(numeric_traits<uint128_t>::max(), uint128_t::max());
}

TEST(NumericTraitsTest, Int128Traits) {
    EXPECT_TRUE(numeric_traits<int128_t>::is_specialized);
    EXPECT_TRUE(numeric_traits<int128_t>::is_signed);
    EXPECT_TRUE(numeric_traits<int128_t>::is_integer);
    EXPECT_EQ(numeric_traits<int128_t>::min(), int128_t::min());
    EXPECT_EQ(numeric_traits<int128_t>::max(), int128_t::max());
    EXPECT_EQ(numeric_traits<int128_t>::lowest(), int128_t::min());
}

TEST(TypeTraitsTest, IsIntegral) {
    EXPECT_TRUE(is_integral_v<uint128_t>);
    EXPECT_TRUE(is_integral_v<int128_t>);
}

TEST(TypeTraitsTest, IsUnsigned) {
    EXPECT_TRUE(is_unsigned_v<uint128_t>);
    EXPECT_FALSE(is_unsigned_v<int128_t>);
}

TEST(TypeTraitsTest, IsSigned) {
    EXPECT_FALSE(is_signed_v<uint128_t>);
    EXPECT_TRUE(is_signed_v<int128_t>);
}

TEST(MakeSignedUnsignedTest, Transformations) {
    using s = make_signed<uint128_t>::type;
    using u = make_unsigned<int128_t>::type;
    EXPECT_TRUE((is_same_v<s, int128_t>));
    EXPECT_TRUE((is_same_v<u, uint128_t>));
}

TEST(CrossTypeTest, Uint128ToInt128Assignment) {
    uint128_t u(0x7FFFFFFF, 0x12345678);
    int128_t s;
    s = u;
    EXPECT_EQ(s.hi, u.hi);
    EXPECT_EQ(s.lo, u.lo);
}

TEST(CrossTypeTest, ArithmeticMixedSign) {
    int128_t a(-10);
    uint128_t b(5);
    int128_t c = a + b.to_int128();
    EXPECT_EQ(c, int128_t(-5));
}

#endif
