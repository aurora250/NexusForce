#include <NeForce/core/string/format.hpp>
#include <NeForce/core/string/to_string.hpp>
#include <NeForce/core/numeric/random/mt.hpp>
#include <gtest/gtest.h>
#if NEFORCE_STANDARD_20
#    include <format>
#    define NEFORCE_TEST_HAS_STD_FORMAT 1
#endif
using namespace neforce;

namespace {
    double next_random(uint64_t& state) {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        const uint64_t raw = (state >> 11) | ((state & 0x7FFULL) << 53);
        double value = 0.0;
        memory_copy(&value, &raw, sizeof(value));
        if (is_nan(value) || is_infinity(value)) {
            value = 1.0;
        }
        return value;
    }

    std::string to_std_string(const string& value) { return std::string(value.data(), value.size()); }

    bool round_trips(const string& text, const double value) {
        const double parsed = std::strtod(std::string(text.data(), text.size()).c_str(), nullptr);
        return parsed == value;
    }
} // namespace


TEST(FormatFastPathTest, PlainIntegersAreCopiedVerbatim) {
    EXPECT_EQ(format("{}", 0), "0");
    EXPECT_EQ(format("{}", 42), "42");
    EXPECT_EQ(format("{}", -7), "-7");
    EXPECT_EQ(format("{}", 1234567890123LL), "1234567890123");
}

TEST(FormatFastPathTest, PlainValuesInsideLiteralText) {
    EXPECT_EQ(format("value={}", 5), "value=5");
    EXPECT_EQ(format("{}{}", 1, 2), "12");
    EXPECT_EQ(format("{} and {}", 1, 2), "1 and 2");
}

TEST(FormatLiteralTest, LongLiteralRunIsCopiedVerbatim) {
    const string tmpl = "the quick brown fox jumps over the lazy dog, then {} keeps on running to the end of the line";
    const string expected =
            "the quick brown fox jumps over the lazy dog, then 7 keeps on running to the end of the line";
    string view_text = tmpl;
    EXPECT_EQ(format(view_text.view(), 7), expected);
}

TEST(FormatLiteralTest, EscapedBracesAndRuns) {
    EXPECT_EQ(format("{{}}", 1), "{}");
    EXPECT_EQ(format("a{{b}}c{}", 2), "a{b}c2");
    EXPECT_EQ(format("prefix{{literal-run-with-no-placeholder}}suffix", 9),
              "prefix{literal-run-with-no-placeholder}suffix");
}

TEST(FormatLiteralTest, LongTemplateWithPaddedValue) {
    const string tmpl = "0123456789abcdefghijklmnopqrstuvwxyz{:>8d}0123456789abcdefghijklmnopqrstuvwxyz";
    EXPECT_EQ(format(tmpl.view(), 42),
              "0123456789abcdefghijklmnopqrstuvwxyz      420123456789abcdefghijklmnopqrstuvwxyz");
}

TEST(FormatToTest, AppendsToExistingString) {
    string out = "log: ";
    format_to(out, "{} {}", 1, 2);
    EXPECT_EQ(out, "log: 1 2");
}

TEST(FormatToTest, MatchesFormatForSameInput) {
    string out;
    format_to(out, "{:04d}-{:02d}", 2026, 8);
    const string expected = format("{:04d}-{:02d}", 2026, 8);
    EXPECT_EQ(out, expected);
    EXPECT_EQ(out.size(), 7U);
}

TEST(FormatToTest, LiteralOverloadAppends) {
    string out = "[";
    format_to(out, "{}]", 3.5);
    EXPECT_EQ(out, "[3.5]");
}

TEST(FormatOptionsTest, IntegerPresentationTypes) {
    EXPECT_EQ(format("{:04d}", 42), "0042");
    EXPECT_EQ(format("{:x}", 255), "ff");
    EXPECT_EQ(format("{:X}", 255), "FF");
    EXPECT_EQ(format("{:#x}", 255), "0xff");
    EXPECT_EQ(format("{:b}", 5), "101");
    EXPECT_EQ(format("{:o}", 64), "100");
    EXPECT_EQ(format("{:+d}", 42), "+42");
    EXPECT_EQ(format("{: d}", 42), " 42");
    EXPECT_EQ(format("{:+d}", -42), "-42");
}

TEST(FormatOptionsTest, AlignmentAndFill) {
    EXPECT_EQ(format("{:<6d}", 42), "42    ");
    EXPECT_EQ(format("{:>6d}", 42), "    42");
    EXPECT_EQ(format("{:^6d}", 42), "  42  ");
    EXPECT_EQ(format("{:*>6d}", 42), "****42");
    EXPECT_EQ(format("{:06d}", 42), "000042");
    EXPECT_EQ(format("{:06d}", -42), "-00042");
}

TEST(FormatOptionsTest, FloatPresentationTypes) {
    EXPECT_EQ(format("{:.2f}", 3.14159), "3.14");
    EXPECT_EQ(format("{:.6f}", 3.14159), "3.141590");
    EXPECT_EQ(format("{:e}", 1234.5), "1.234500e+03");
    EXPECT_EQ(format("{:.2e}", 1234.5), "1.23e+03");
    EXPECT_EQ(format("{:.3f}", 2.0), "2.000");
    EXPECT_EQ(format("{:>10.2f}", 3.5), "      3.50");
}

TEST(FormatOptionsTest, OtherValueKinds) {
    const string text = "text";
    EXPECT_EQ(format("{}", text), "text");
    EXPECT_EQ(format("{}", string_view{"view"}), "view");
    EXPECT_EQ(format("{}", true), "true");
    EXPECT_EQ(format("{}", false), "false");
}

TEST(FormatOptionsTest, PositionalAndSequentialMix) {
    EXPECT_EQ(format("{0} {1} {0}", 1, 2), "1 2 1");
    EXPECT_EQ(format("{1:d} {0:x}", 255, 16), "16 ff");
}

TEST(FloatFixedFormatTest, MatchesPrintfForCorpus) {
    const double corpus[] = {0.0,
                             1.0,
                             0.5,
                             1.5,
                             2.5,
                             0.1,
                             0.2,
                             0.3,
                             2.675,
                             1234.5678,
                             1e-5,
                             1e5,
                             1e15,
                             1e-15,
                             3.14159265358979,
                             9.999999,
                             0.000123456,
                             123456789.123456,
                             0.999999999999,
                             -1.5,
                             -0.125,
                             -12345.6789};
    const int precisions[] = {0, 1, 2, 3, 6, 9, 15, 17};
    for (const double value: corpus) {
        for (const int precision: precisions) {
            const string expected = to_string_with_precision(value, precision);
            const string actual = to_string_fixed(value, precision);
            EXPECT_EQ(actual, expected) << "value=" << value << " precision=" << precision;
        }
    }
}

TEST(FloatFixedFormatTest, MatchesPrintfForRandomValues) {
    uint64_t state = 0x12345678ULL;
    for (int i = 0; i < 200; ++i) {
        const double value = next_random(state);
        for (const int precision: {0, 2, 6, 12}) {
            const string expected = to_string_with_precision(value, precision);
            const string actual = to_string_fixed(value, precision);
            EXPECT_EQ(actual, expected) << "value=" << value << " precision=" << precision;
        }
    }
}

TEST(FloatScientificFormatTest, MatchesPrintfForCorpus) {
    const double corpus[] = {1.0,
                             0.5,
                             1234.5,
                             1e-5,
                             1e5,
                             1e300,
                             1e-300,
                             9.999999e-7,
                             1.7976931348623157e308,
                             2.2250738585072014e-308,
                             5e-324,
                             3.14159265358979,
                             1e100,
                             6.02e23};
    for (const double value: corpus) {
        for (const int precision: {0, 1, 2, 6, 15}) {
            const string expected = to_string_scientific(value, precision);
            const string actual = to_string_scientific(value, precision);
            EXPECT_EQ(actual, expected) << "value=" << value << " precision=" << precision;
        }
    }
}

TEST(FloatScientificFormatTest, MatchesPrintfForRandomValues) {
    uint64_t state = 0xCAFEBABEULL;
    for (int i = 0; i < 120; ++i) {
        const double value = next_random(state);
        for (const int precision: {0, 3, 8}) {
            const string expected = to_string_scientific(value, precision);
            const string actual = to_string_scientific(value, precision);
            EXPECT_EQ(actual, expected) << "value=" << value << " precision=" << precision;
        }
    }
}

TEST(FloatGeneralFormatTest, MatchesDocumentedThresholds) {
    const double corpus[] = {0.0,       1.0,    0.5,        1234.5678, 999999.0, 1000000.0,
                             1234567.0, 0.0001, 0.00009999, 1e-7,      1e9,      3.25};
    for (const double value: corpus) {
        for (const int precision: {2, 6}) {
            const bool scientific = (value != 0.0) && (value >= 1e6 || value < 1e-4);
            const string expected =
                    scientific ? to_string_scientific(value, precision) : to_string_with_precision(value, precision);
            const string actual = to_string_general(value, precision);
            EXPECT_EQ(actual, expected) << "value=" << value << " precision=" << precision;
        }
    }
}

TEST(FloatRoundingTest, HalfwayValuesRoundToEven) {
    EXPECT_EQ(to_string_fixed(0.5, 0), to_string_with_precision(0.5, 0));
    EXPECT_EQ(to_string_fixed(1.5, 0), to_string_with_precision(1.5, 0));
    EXPECT_EQ(to_string_fixed(2.5, 0), to_string_with_precision(2.5, 0));
    EXPECT_EQ(to_string_fixed(3.5, 0), to_string_with_precision(3.5, 0));
    EXPECT_EQ(to_string_fixed(0.125, 2), to_string_with_precision(0.125, 2));
    EXPECT_EQ(to_string_fixed(0.375, 2), to_string_with_precision(0.375, 2));
}

TEST(FloatRoundingTest, CarryPropagates) {
    EXPECT_EQ(to_string_fixed(9.9999999, 6), "10.000000");
    EXPECT_EQ(to_string_fixed(0.99999999, 6), "1.000000");
    EXPECT_EQ(to_string_scientific(9.9999999, 6), to_string_scientific(9.9999999, 6));
    EXPECT_EQ(to_string_scientific(0.99999999e6, 6), to_string_scientific(0.99999999e6, 6));
}

TEST(FloatEdgeCaseTest, SpecialValues) {
    const double nan_value = numeric_traits<double>::quiet_nan();
    const double inf_value = numeric_traits<double>::infinity();
    EXPECT_EQ(to_string_fixed(nan_value, 2), "nan");
    EXPECT_EQ(to_string_fixed(inf_value, 2), "inf");
    EXPECT_EQ(to_string_fixed(-inf_value, 2), "-inf");
    EXPECT_EQ(to_string_scientific(nan_value, 2), "nan");
    EXPECT_EQ(to_string_scientific(inf_value, 2), "inf");
}

TEST(FloatEdgeCaseTest, ZeroAndNegativeZero) {
    EXPECT_EQ(to_string_fixed(0.0, 3), "0.000");
    EXPECT_EQ(to_string_fixed(-0.0, 3), "-0.000");
    EXPECT_EQ(to_string_scientific(0.0, 3), "0.000e+00");
    EXPECT_EQ(to_string_scientific(-0.0, 2), "-0.00e+00");
    EXPECT_EQ(format("{}", 0.0), "0");
    EXPECT_EQ(format("{}", -0.0), "-0");
}

TEST(FloatEdgeCaseTest, IntegralValuesPadZeroFraction) {
    EXPECT_EQ(to_string_fixed(1.0, 0), "1");
    EXPECT_EQ(to_string_fixed(1234.0, 3), "1234.000");
    EXPECT_EQ(to_string_fixed(1e15, 2), "1000000000000000.00");
    EXPECT_EQ(to_string_fixed(1.7976931348623157e308, 0), to_string_with_precision(1.7976931348623157e308, 0));
}

TEST(FloatEdgeCaseTest, ExtremePrecisionIsHandled) {
    const string actual = to_string_fixed(0.1, 25);
    EXPECT_EQ(actual, to_string_with_precision(0.1, 25));
    const string very_long = to_string_fixed(0.1, 400);
    EXPECT_GT(very_long.size(), 300U);
    EXPECT_EQ(very_long[0], '0');
    EXPECT_EQ(very_long[1], '.');
}

TEST(FloatEdgeCaseTest, DenormalAndExtremeMagnitudes) {
    const double values[] = {5e-324, 1e-320, 2.2250738585072014e-308, 1e308, 1.7976931348623157e308};
    for (const double value: values) {
        EXPECT_EQ(to_string_fixed(value, 6), to_string_with_precision(value, 6)) << "value=" << value;
        EXPECT_EQ(to_string_scientific(value, 6), to_string_scientific(value, 6)) << "value=" << value;
    }
}

TEST(FloatEdgeCaseTest, FormatDefaultUsesShortestRepresentation) {
    EXPECT_EQ(format("{}", 3.5), "3.5");
    EXPECT_EQ(format("{}", 1234567.0), "1234567");
    EXPECT_EQ(format("{}", 0.00001), "1e-05");
    EXPECT_EQ(format("{}", 1.0 / 3.0), "0.3333333333333333");
    EXPECT_EQ(format("{:g}", 1.0 / 3.0), to_string_general(1.0 / 3.0, 6));
}

TEST(FormatFloatShortest, DefaultFormatIsShortestRoundTrip) {
    EXPECT_EQ(format("{}", 1.0), "1");
    EXPECT_EQ(format("{}", 3.5), "3.5");
    EXPECT_EQ(format("{}", 0.1), "0.1");
    EXPECT_EQ(format("{}", 1.0 / 3.0), "0.3333333333333333");
    EXPECT_EQ(format("{}", 100.0), "100");
    EXPECT_EQ(format("{}", 1e15), "1e+15");
    EXPECT_EQ(format("{}", 1e-4), "1e-04");
    EXPECT_EQ(format("{}", 0.0001234), "0.0001234");
    EXPECT_EQ(format("{}", 1234567890123456.0), "1234567890123456");
    EXPECT_EQ(format("{}", 4.9406564584124654e-324), "5e-324");
    EXPECT_EQ(format("{}", -0.0), "-0");
    EXPECT_EQ(format("{}", -1.0 / 3.0), "-0.3333333333333333");
    EXPECT_EQ(format("{}", 1.7976931348623157e308), "1.7976931348623157e+308");
    EXPECT_EQ(format("{}", 9007199254740992.0), "9007199254740992");
    EXPECT_EQ(format("{}", 59288500908677016.0), "59288500908677016");
    EXPECT_EQ(format("{}", -59288500908677016.0), "-59288500908677016");
    EXPECT_EQ(format("{}", 1152921504606846976.0), "1152921504606846976");
    EXPECT_EQ(format("{}", 1090326666401827520512.0), "1090326666401827520512");
    EXPECT_EQ(format("{}", 4406930802370655744.0), "4406930802370655744");
    EXPECT_EQ(format("{}", 1e20), "1e+20");
    EXPECT_EQ(format("{}", 1.5e20), "1.5e+20");
}

TEST(FormatFloatShortest, GeneralFormatUsesSignificantDigits) {
    EXPECT_EQ(format("{:g}", 1.0 / 3.0), "0.333333");
    EXPECT_EQ(format("{:g}", 0.1), "0.1");
    EXPECT_EQ(format("{:g}", 1234567.0), "1.23457e+06");
    EXPECT_EQ(format("{:g}", 123456.0), "123456");
    EXPECT_EQ(format("{:g}", 1e-4), "0.0001");
    EXPECT_EQ(format("{:g}", 1e-5), "1e-05");
    EXPECT_EQ(format("{:.3g}", 3.141592653589793), "3.14");
    EXPECT_EQ(format("{:.3g}", 1234567.0), "1.23e+06");
    EXPECT_EQ(format("{:.6g}", 1.0 / 3.0), "0.333333");
    EXPECT_EQ(format("{:G}", 1234567.0), "1.23457E+06");
}

TEST(FormatFloatShortest, RoundTripsRandomValues) {
    random_mt rng(20261107);
    int checked = 0;
    for (int i = 0; i < 20000; ++i) {
        const uint64_t bits = rng();
        double value = 0;
        memory_copy(&value, &bits, sizeof(value));
        if (is_nan(value) || is_infinity(value)) {
            continue;
        }
        const string text = format("{}", value);
        ASSERT_TRUE(round_trips(text, value)) << to_std_string(text) << " for bits " << bits;
        ++checked;
    }
    EXPECT_GT(checked, 19000);
}

TEST(FormatFloatShortest, RoundTripsLargeIntegerValues) {
    random_mt rng(20261108);
    int checked = 0;
    for (int i = 0; i < 20000; ++i) {
        // 强制指数域落在 |v| >= 2^53 的区间
        const uint64_t bits = (rng() & 0x800FFFFFFFFFFFFFULL) | (static_cast<uint64_t>(1076 + (rng() % 900)) << 52);
        double value = 0;
        memory_copy(&value, &bits, sizeof(value));
        if (is_nan(value) || is_infinity(value)) {
            continue;
        }
        const string text = format("{}", value);
        ASSERT_TRUE(round_trips(text, value)) << to_std_string(text) << " for bits " << bits;
        ++checked;
    }
    EXPECT_GT(checked, 19000);
}

#ifdef NEFORCE_TEST_HAS_STD_FORMAT
TEST(FormatFloatShortest, MatchesStdFormat) {
    vector<double> values = {0.0,
                             -0.0,
                             1.0,
                             -1.0,
                             3.5,
                             0.1,
                             1.0 / 3.0,
                             2.0 / 3.0,
                             1e-4,
                             1e-5,
                             1e15,
                             1e16,
                             1e17,
                             1e300,
                             1e-300,
                             1234567890123456.0,
                             123456789012345680.0,
                             9007199254740992.0,
                             9007199254740994.0,
                             4.9406564584124654e-324,
                             2.2250738585072014e-308,
                             1.7976931348623157e308,
                             0.30000000000000004,
                             1.2345e16,
                             1.2345e-5};
    random_mt rng(987654321);
    for (int i = 0; i < 20000; ++i) {
        const uint64_t bits = rng();
        double value = 0;
        memory_copy(&value, &bits, sizeof(value));
        if (is_nan(value) || is_infinity(value)) {
            continue;
        }
        values.push_back(value);
    }

    for (const double value: values) {
        ASSERT_EQ(to_std_string(format("{}", value)), std::format("{}", value)) << "value bits mismatch for " << value;
        ASSERT_EQ(to_std_string(format("{:g}", value)), std::format("{:g}", value)) << "for " << value;
        ASSERT_EQ(to_std_string(format("{:.3g}", value)), std::format("{:.3g}", value)) << "for " << value;
        ASSERT_EQ(to_std_string(format("{:.6g}", value)), std::format("{:.6g}", value)) << "for " << value;
        ASSERT_EQ(to_std_string(format("{:.17g}", value)), std::format("{:.17g}", value)) << "for " << value;
    }
}
#endif
