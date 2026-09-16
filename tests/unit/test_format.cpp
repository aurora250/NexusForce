#include <NeForce/core/string/format.hpp>
#include <NeForce/core/string/to_string.hpp>
#include <iomanip>
#include <sstream>
#include <gtest/gtest.h>
using namespace neforce;

namespace {
    // The oracle is deliberately the standard stream formatter: the library also
    // ships its own snprintf, which does not implement the '*' width/precision
    // specifier and would silently return the format string itself.
    string reference_fixed(const double value, const int precision) {
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(precision) << value;
        return string{stream.str().c_str()};
    }

    string reference_scientific(const double value, const int precision) {
        std::ostringstream stream;
        stream << std::scientific << std::setprecision(precision) << value;
        return string{stream.str().c_str()};
    }

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
    EXPECT_EQ(out, "[3.500000]");
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
            const string expected = reference_fixed(value, precision);
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
            const string expected = reference_fixed(value, precision);
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
            const string expected = reference_scientific(value, precision);
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
            const string expected = reference_scientific(value, precision);
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
                    scientific ? reference_scientific(value, precision) : reference_fixed(value, precision);
            const string actual = to_string_general(value, precision);
            EXPECT_EQ(actual, expected) << "value=" << value << " precision=" << precision;
        }
    }
}

TEST(FloatRoundingTest, HalfwayValuesRoundToEven) {
    EXPECT_EQ(to_string_fixed(0.5, 0), reference_fixed(0.5, 0));
    EXPECT_EQ(to_string_fixed(1.5, 0), reference_fixed(1.5, 0));
    EXPECT_EQ(to_string_fixed(2.5, 0), reference_fixed(2.5, 0));
    EXPECT_EQ(to_string_fixed(3.5, 0), reference_fixed(3.5, 0));
    EXPECT_EQ(to_string_fixed(0.125, 2), reference_fixed(0.125, 2));
    EXPECT_EQ(to_string_fixed(0.375, 2), reference_fixed(0.375, 2));
}

TEST(FloatRoundingTest, CarryPropagates) {
    EXPECT_EQ(to_string_fixed(9.9999999, 6), "10.000000");
    EXPECT_EQ(to_string_fixed(0.99999999, 6), "1.000000");
    EXPECT_EQ(to_string_scientific(9.9999999, 6), reference_scientific(9.9999999, 6));
    EXPECT_EQ(to_string_scientific(0.99999999e6, 6), reference_scientific(0.99999999e6, 6));
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
    EXPECT_EQ(to_string_fixed(-0.0, 3), "0.000");
    EXPECT_EQ(to_string_scientific(0.0, 3), "0.000e+00");
    EXPECT_EQ(to_string_scientific(-0.0, 2), "0.00e+00");
    EXPECT_EQ(format("{}", 0.0), "0.000000");
}

TEST(FloatEdgeCaseTest, IntegralValuesPadZeroFraction) {
    EXPECT_EQ(to_string_fixed(1.0, 0), "1");
    EXPECT_EQ(to_string_fixed(1234.0, 3), "1234.000");
    EXPECT_EQ(to_string_fixed(1e15, 2), "1000000000000000.00");
    EXPECT_EQ(to_string_fixed(1.7976931348623157e308, 0), reference_fixed(1.7976931348623157e308, 0));
}

TEST(FloatEdgeCaseTest, ExtremePrecisionIsHandled) {
    const string actual = to_string_fixed(0.1, 25);
    EXPECT_EQ(actual, reference_fixed(0.1, 25));
    const string very_long = to_string_fixed(0.1, 400);
    EXPECT_GT(very_long.size(), 300U);
    EXPECT_EQ(very_long[0], '0');
    EXPECT_EQ(very_long[1], '.');
}

TEST(FloatEdgeCaseTest, DenormalAndExtremeMagnitudes) {
    const double values[] = {5e-324, 1e-320, 2.2250738585072014e-308, 1e308, 1.7976931348623157e308};
    for (const double value: values) {
        EXPECT_EQ(to_string_fixed(value, 6), reference_fixed(value, 6)) << "value=" << value;
        EXPECT_EQ(to_string_scientific(value, 6), reference_scientific(value, 6)) << "value=" << value;
    }
}

TEST(FloatEdgeCaseTest, FormatDefaultMatchesGeneralPrecisionSix) {
    EXPECT_EQ(format("{}", 3.5), to_string_general(3.5, 6));
    EXPECT_EQ(format("{}", 1234567.0), to_string_general(1234567.0, 6));
    EXPECT_EQ(format("{}", 0.00001), to_string_general(0.00001, 6));
}
