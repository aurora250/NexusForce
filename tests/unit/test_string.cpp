#include <NeForce/core/algorithm/type_erase.hpp>
#include <NeForce/core/string/char_types.hpp>
#include <NeForce/core/string/regex.hpp>
#include <NeForce/core/string/to_numerics.hpp>
#include <NeForce/core/string/utf.hpp>
#include <NeForce/core/utility/packages.hpp>
#include <gtest/gtest.h>
using namespace neforce;

class ToFloat32Test : public ::testing::Test {};
class ToFloat64Test : public ::testing::Test {};
class ToDecimalTest : public ::testing::Test {};
class ToInt64Test : public ::testing::Test {};
class ToUint64Test : public ::testing::Test {};
class ToInt32Test : public ::testing::Test {};
class ToUint32Test : public ::testing::Test {};
class ToInt16Test : public ::testing::Test {};
class ToUint16Test : public ::testing::Test {};
class ToInt8Test : public ::testing::Test {};
class ToUint8Test : public ::testing::Test {};

TEST_F(ToFloat32Test, Zero) {
    EXPECT_FLOAT_EQ(to_float32("0"), 0.0f);
    EXPECT_FLOAT_EQ(to_float32("0.0"), 0.0f);
    EXPECT_FLOAT_EQ(to_float32(".0"), 0.0f);
    EXPECT_FLOAT_EQ(to_float32("0."), 0.0f);
}

TEST_F(ToFloat32Test, PositiveInteger) {
    EXPECT_FLOAT_EQ(to_float32("42"), 42.0f);
    EXPECT_FLOAT_EQ(to_float32("+42"), 42.0f);
    EXPECT_FLOAT_EQ(to_float32("  42"), 42.0f);
}

TEST_F(ToFloat32Test, NegativeInteger) {
    EXPECT_FLOAT_EQ(to_float32("-42"), -42.0f);
    EXPECT_FLOAT_EQ(to_float32("  -42"), -42.0f);
}

TEST_F(ToFloat32Test, Fractional) {
    EXPECT_FLOAT_EQ(to_float32("3.14"), 3.14f);
    EXPECT_FLOAT_EQ(to_float32("-3.14"), -3.14f);
    EXPECT_FLOAT_EQ(to_float32("0.5"), 0.5f);
    EXPECT_FLOAT_EQ(to_float32(".5"), 0.5f);
}

TEST_F(ToFloat32Test, ScientificNotation) {
    EXPECT_FLOAT_EQ(to_float32("1e5"), 1e5f);
    EXPECT_FLOAT_EQ(to_float32("1E5"), 1e5f);
    EXPECT_FLOAT_EQ(to_float32("1e+5"), 1e5f);
    EXPECT_FLOAT_EQ(to_float32("1e-5"), 1e-5f);
    EXPECT_FLOAT_EQ(to_float32("-1e5"), -1e5f);
    EXPECT_FLOAT_EQ(to_float32("1.5e2"), 150.0f);
    EXPECT_FLOAT_EQ(to_float32("1.5e-2"), 0.015f);
}

TEST_F(ToFloat32Test, Infinity) {
    EXPECT_TRUE(is_infinity(to_float32("inf")));
    EXPECT_TRUE(is_infinity(to_float32("INF")));
    EXPECT_TRUE(is_infinity(to_float32("infinity")));
    EXPECT_TRUE(is_infinity(to_float32("INFINITY")));
    EXPECT_TRUE(is_infinity(to_float32("+inf")));
    EXPECT_TRUE(is_infinity(to_float32("-inf")));
    EXPECT_GT(to_float32("inf"), 0.0f);
    EXPECT_LT(to_float32("-inf"), 0.0f);
}

TEST_F(ToFloat32Test, NaN) {
    EXPECT_TRUE(is_nan(to_float32("nan")));
    EXPECT_TRUE(is_nan(to_float32("NAN")));
    EXPECT_TRUE(is_nan(to_float32("nan()")));
    EXPECT_TRUE(is_nan(to_float32("nan(something)")));
    EXPECT_TRUE(is_nan(to_float32("NAN(123)")));
}

TEST_F(ToFloat32Test, LeadingSpaces) {
    EXPECT_FLOAT_EQ(to_float32("  123"), 123.0f);
    EXPECT_FLOAT_EQ(to_float32("\t\n 456"), 456.0f);
}

TEST_F(ToFloat32Test, TrailingCharacters) {
    size_t idx = 0;
    ignore = to_float32("123abc", &idx);
    EXPECT_EQ(idx, 3u);
    ignore = to_float32("3.14xyz", &idx);
    EXPECT_EQ(idx, 4u);
}

TEST_F(ToFloat32Test, EmptyString) { EXPECT_THROW(ignore = to_float32(""), typecast_exception); }

TEST_F(ToFloat32Test, InvalidString) { EXPECT_THROW(ignore = to_float32("abc"), typecast_exception); }

TEST_F(ToFloat32Test, OverflowToInfinity) {
    EXPECT_TRUE(is_infinity(to_float32("1e40")));
    EXPECT_TRUE(is_infinity(to_float32("-1e40")));
}

TEST_F(ToFloat32Test, UnderflowToZero) {
    EXPECT_FLOAT_EQ(to_float32("1e-50"), 0.0f);
    EXPECT_FLOAT_EQ(to_float32("-1e-50"), -0.0f);
}

TEST_F(ToFloat64Test, Zero) {
    EXPECT_DOUBLE_EQ(to_float64("0"), 0.0);
    EXPECT_DOUBLE_EQ(to_float64("0.0"), 0.0);
    EXPECT_DOUBLE_EQ(to_float64(".0"), 0.0);
    EXPECT_DOUBLE_EQ(to_float64("0."), 0.0);
}

TEST_F(ToFloat64Test, PositiveInteger) {
    EXPECT_DOUBLE_EQ(to_float64("42"), 42.0);
    EXPECT_DOUBLE_EQ(to_float64("+42"), 42.0);
}

TEST_F(ToFloat64Test, NegativeInteger) { EXPECT_DOUBLE_EQ(to_float64("-42"), -42.0); }

TEST_F(ToFloat64Test, Fractional) {
    EXPECT_DOUBLE_EQ(to_float64("3.141592653589793"), 3.141592653589793);
    EXPECT_DOUBLE_EQ(to_float64("-3.14"), -3.14);
}

TEST_F(ToFloat64Test, ScientificNotation) {
    EXPECT_DOUBLE_EQ(to_float64("1e10"), 1e10);
    EXPECT_DOUBLE_EQ(to_float64("1.5e-3"), 0.0015);
    EXPECT_DOUBLE_EQ(to_float64("-2.5e4"), -25000.0);
}

TEST_F(ToFloat64Test, Infinity) {
    EXPECT_TRUE(is_infinity(to_float64("inf")));
    EXPECT_TRUE(is_infinity(to_float64("-infinity")));
}

TEST_F(ToFloat64Test, NaN) {
    EXPECT_TRUE(is_nan(to_float64("nan")));
    EXPECT_TRUE(is_nan(to_float64("NAN(123)")));
}

TEST_F(ToFloat64Test, EmptyString) { EXPECT_THROW(ignore = to_float64(""), typecast_exception); }

TEST_F(ToFloat64Test, InvalidString) { EXPECT_THROW(ignore = to_float64("xyz"), typecast_exception); }

TEST_F(ToFloat64Test, OverflowToInfinity) { EXPECT_TRUE(is_infinity(to_float64("1e400"))); }

TEST_F(ToFloat64Test, UnderflowToZero) { EXPECT_DOUBLE_EQ(to_float64("1e-400"), 0.0); }

TEST_F(ToDecimalTest, BasicConversion) {
    EXPECT_EQ(to_decimal("0"), decimal_t(0));
    EXPECT_EQ(to_decimal("123"), decimal_t(123));
    EXPECT_EQ(to_decimal("-456"), decimal_t(-456));
}

TEST_F(ToDecimalTest, Fractional) {
    EXPECT_EQ(to_decimal("3.14"), decimal_t(3.14));
    EXPECT_EQ(to_decimal("-0.5"), decimal_t(-0.5));
}

TEST_F(ToDecimalTest, ScientificNotation) {
    EXPECT_EQ(to_decimal("1e3"), decimal_t(1000));
    EXPECT_EQ(to_decimal("1.5e2"), decimal_t(150));
}

TEST_F(ToDecimalTest, EmptyString) { EXPECT_THROW(ignore = to_decimal(""), typecast_exception); }

TEST_F(ToDecimalTest, InvalidString) { EXPECT_THROW(ignore = to_decimal("abc"), typecast_exception); }

TEST_F(ToInt64Test, PositiveDecimal) {
    EXPECT_EQ(to_int64("0"), 0);
    EXPECT_EQ(to_int64("42"), 42);
    EXPECT_EQ(to_int64("+42"), 42);
    EXPECT_EQ(to_int64("9223372036854775807"), 9223372036854775807LL);
}

TEST_F(ToInt64Test, NegativeDecimal) {
    EXPECT_EQ(to_int64("-1"), -1);
    EXPECT_EQ(to_int64("-42"), -42);
    EXPECT_EQ(to_int64("-9223372036854775808"), (-9223372036854775807LL - 1));
}

TEST_F(ToInt64Test, LeadingSpaces) {
    EXPECT_EQ(to_int64("  123"), 123);
    EXPECT_EQ(to_int64("  -456"), -456);
}

TEST_F(ToInt64Test, TrailingCharacters) {
    size_t idx = 0;
    ignore = to_int64("789xyz", &idx);
    EXPECT_EQ(idx, 3u);
}

TEST_F(ToInt64Test, Hexadecimal) {
    EXPECT_EQ(to_int64("0xA", nullptr, 0), 10);
    EXPECT_EQ(to_int64("0XFF", nullptr, 0), 255);
    EXPECT_EQ(to_int64("FF", nullptr, 16), 255);
    EXPECT_EQ(to_int64("-0x10", nullptr, 0), -16);
}

TEST_F(ToInt64Test, Octal) {
    EXPECT_EQ(to_int64("010", nullptr, 0), 8);
    EXPECT_EQ(to_int64("777", nullptr, 8), 511);
}

TEST_F(ToInt64Test, Binary) {
    EXPECT_EQ(to_int64("1010", nullptr, 2), 10);
    EXPECT_EQ(to_int64("11111111", nullptr, 2), 255);
}

TEST_F(ToInt64Test, Base36) {
    EXPECT_EQ(to_int64("Z", nullptr, 36), 35);
    EXPECT_EQ(to_int64("10", nullptr, 36), 36);
}

TEST_F(ToInt64Test, OverflowToMax) {
    EXPECT_EQ(to_int64("9223372036854775808"), 9223372036854775807LL);
    EXPECT_EQ(to_int64("9999999999999999999"), 9223372036854775807LL);
}

TEST_F(ToInt64Test, OverflowToMin) {
    EXPECT_EQ(to_int64("-9223372036854775809"), (-9223372036854775807LL - 1));
    EXPECT_EQ(to_int64("-9999999999999999999"), (-9223372036854775807LL - 1));
}

TEST_F(ToInt64Test, EmptyString) { EXPECT_THROW(ignore = to_int64(""), typecast_exception); }

TEST_F(ToInt64Test, InvalidString) { EXPECT_THROW(ignore = to_int64("abc"), typecast_exception); }

TEST_F(ToInt64Test, InvalidBase) {
    EXPECT_THROW(ignore = to_int64("123", nullptr, 1), typecast_exception);
    EXPECT_THROW(ignore = to_int64("123", nullptr, 37), typecast_exception);
}

TEST_F(ToInt64Test, Base0AutoDetect) {
    EXPECT_EQ(to_int64("123", nullptr, 0), 123);
    EXPECT_EQ(to_int64("0x1A", nullptr, 0), 26);
    EXPECT_EQ(to_int64("077", nullptr, 0), 63);
}

TEST_F(ToUint64Test, PositiveDecimal) {
    EXPECT_EQ(to_uint64("0"), 0u);
    EXPECT_EQ(to_uint64("42"), 42u);
    EXPECT_EQ(to_uint64("+42"), 42u);
    EXPECT_EQ(to_uint64("18446744073709551615"), 18446744073709551615ULL);
}

TEST_F(ToUint64Test, NegativeHandling) {
    EXPECT_EQ(to_uint64("-1"), 18446744073709551615ULL);
    EXPECT_EQ(to_uint64("-2"), 18446744073709551614ULL);
}

TEST_F(ToUint64Test, Hexadecimal) {
    EXPECT_EQ(to_uint64("0xFF", nullptr, 0), 255u);
    EXPECT_EQ(to_uint64("FF", nullptr, 16), 255u);
    EXPECT_EQ(to_uint64("-0x1", nullptr, 0), 18446744073709551615ULL);
}

TEST_F(ToUint64Test, Overflow) {
    EXPECT_EQ(to_uint64("18446744073709551616"), 18446744073709551615ULL);
    EXPECT_EQ(to_uint64("99999999999999999999"), 18446744073709551615ULL);
}

TEST_F(ToUint64Test, EmptyString) { EXPECT_THROW(ignore = to_uint64(""), typecast_exception); }

TEST_F(ToUint64Test, InvalidString) { EXPECT_THROW(ignore = to_uint64("xyz"), typecast_exception); }

TEST_F(ToUint64Test, LeadingSpaces) { EXPECT_EQ(to_uint64("  999"), 999u); }

TEST_F(ToInt32Test, PositiveRange) {
    EXPECT_EQ(to_int32("0"), 0);
    EXPECT_EQ(to_int32("42"), 42);
    EXPECT_EQ(to_int32("2147483647"), 2147483647);
}

TEST_F(ToInt32Test, NegativeRange) {
    EXPECT_EQ(to_int32("-1"), -1);
    EXPECT_EQ(to_int32("-2147483648"), -2147483648);
}

TEST_F(ToInt32Test, Hexadecimal) {
    EXPECT_EQ(to_int32("0x7FFFFFFF", nullptr, 0), 2147483647);
    EXPECT_EQ(to_int32("-0x80000000", nullptr, 0), -2147483648);
}

TEST_F(ToInt32Test, OverflowToMax) {
    EXPECT_EQ(to_int32("2147483648"), 2147483647);
    EXPECT_EQ(to_int32("9999999999"), 2147483647);
}

TEST_F(ToInt32Test, OverflowToMin) {
    EXPECT_EQ(to_int32("-2147483649"), -2147483648);
    EXPECT_EQ(to_int32("-9999999999"), -2147483648);
}

TEST_F(ToInt32Test, EmptyString) { EXPECT_THROW(ignore = to_int32(""), typecast_exception); }

TEST_F(ToInt32Test, InvalidString) { EXPECT_THROW(ignore = to_int32("abc"), typecast_exception); }

TEST_F(ToUint32Test, PositiveRange) {
    EXPECT_EQ(to_uint32("0"), 0u);
    EXPECT_EQ(to_uint32("42"), 42u);
    EXPECT_EQ(to_uint32("4294967295"), 4294967295u);
}

TEST_F(ToUint32Test, NegativeHandling) { EXPECT_EQ(to_uint32("-1"), 4294967295u); }

TEST_F(ToUint32Test, Hexadecimal) { EXPECT_EQ(to_uint32("0xFFFFFFFF", nullptr, 0), 4294967295u); }

TEST_F(ToUint32Test, Overflow) { EXPECT_EQ(to_uint32("4294967296"), 4294967295u); }

TEST_F(ToUint32Test, EmptyString) { EXPECT_THROW(ignore = to_uint32(""), typecast_exception); }

TEST_F(ToUint32Test, InvalidString) { EXPECT_THROW(ignore = to_uint32("abc"), typecast_exception); }

TEST_F(ToInt16Test, ValidRange) {
    EXPECT_EQ(to_int16("0"), 0);
    EXPECT_EQ(to_int16("32767"), 32767);
    EXPECT_EQ(to_int16("-32768"), -32768);
}

TEST_F(ToInt16Test, Overflow) {
    EXPECT_THROW(ignore = to_int16("32768"), typecast_exception);
    EXPECT_THROW(ignore = to_int16("-32769"), typecast_exception);
}

TEST_F(ToInt16Test, EmptyString) { EXPECT_THROW(ignore = to_int16(""), typecast_exception); }

TEST_F(ToInt16Test, InvalidString) { EXPECT_THROW(ignore = to_int16("xyz"), typecast_exception); }

TEST_F(ToUint16Test, ValidRange) {
    EXPECT_EQ(to_uint16("0"), 0u);
    EXPECT_EQ(to_uint16("65535"), 65535u);
}

TEST_F(ToUint16Test, Overflow) { EXPECT_THROW(ignore = to_uint16("65536"), typecast_exception); }

TEST_F(ToUint16Test, NegativeOverflow) { EXPECT_THROW(ignore = to_uint16("-1"), typecast_exception); }

TEST_F(ToUint16Test, EmptyString) { EXPECT_THROW(ignore = to_uint16(""), typecast_exception); }

TEST_F(ToUint16Test, InvalidString) { EXPECT_THROW(ignore = to_uint16("abc"), typecast_exception); }

TEST_F(ToInt8Test, ValidRange) {
    EXPECT_EQ(to_int8("0"), 0);
    EXPECT_EQ(to_int8("127"), 127);
    EXPECT_EQ(to_int8("-128"), -128);
}

TEST_F(ToInt8Test, Overflow) {
    EXPECT_THROW(ignore = to_int8("128"), typecast_exception);
    EXPECT_THROW(ignore = to_int8("-129"), typecast_exception);
}

TEST_F(ToInt8Test, EmptyString) { EXPECT_THROW(ignore = to_int8(""), typecast_exception); }

TEST_F(ToInt8Test, InvalidString) { EXPECT_THROW(ignore = to_int8("abc"), typecast_exception); }

TEST_F(ToUint8Test, ValidRange) {
    EXPECT_EQ(to_uint8("0"), 0u);
    EXPECT_EQ(to_uint8("255"), 255u);
}

TEST_F(ToUint8Test, Overflow) { EXPECT_THROW(ignore = to_uint8("256"), typecast_exception); }

TEST_F(ToUint8Test, NegativeOverflow) { EXPECT_THROW(ignore = to_uint8("-1"), typecast_exception); }

TEST_F(ToUint8Test, EmptyString) { EXPECT_THROW(ignore = to_uint8(""), typecast_exception); }

TEST_F(ToUint8Test, InvalidString) { EXPECT_THROW(ignore = to_uint8("abc"), typecast_exception); }

class IsPunctTest : public ::testing::Test {};
class IsCntrlTest : public ::testing::Test {};
class IsPrintTest : public ::testing::Test {};
class IsBlankTest : public ::testing::Test {};
class IsGraphTest : public ::testing::Test {};
class IsAsciiTest : public ::testing::Test {};
class IsSpaceTest : public ::testing::Test {};
class IsAlphaTest : public ::testing::Test {};
class IsDigitTest : public ::testing::Test {};
class IsXdigitTest : public ::testing::Test {};
class IsAlphaOrDigitTest : public ::testing::Test {};
class IsDigitOrAlphaTest : public ::testing::Test {};

TEST_F(IsPunctTest, ValidPunctuation) {
    EXPECT_TRUE(is_punct('!'));
    EXPECT_TRUE(is_punct('"'));
    EXPECT_TRUE(is_punct('#'));
    EXPECT_TRUE(is_punct('$'));
    EXPECT_TRUE(is_punct('%'));
    EXPECT_TRUE(is_punct('&'));
    EXPECT_TRUE(is_punct('\''));
    EXPECT_TRUE(is_punct('('));
    EXPECT_TRUE(is_punct(')'));
    EXPECT_TRUE(is_punct('*'));
    EXPECT_TRUE(is_punct('+'));
    EXPECT_TRUE(is_punct(','));
    EXPECT_TRUE(is_punct('-'));
    EXPECT_TRUE(is_punct('.'));
    EXPECT_TRUE(is_punct('/'));
}

TEST_F(IsPunctTest, ValidPunctuationExtended) {
    EXPECT_TRUE(is_punct(':'));
    EXPECT_TRUE(is_punct(';'));
    EXPECT_TRUE(is_punct('<'));
    EXPECT_TRUE(is_punct('='));
    EXPECT_TRUE(is_punct('>'));
    EXPECT_TRUE(is_punct('?'));
    EXPECT_TRUE(is_punct('@'));
    EXPECT_TRUE(is_punct('['));
    EXPECT_TRUE(is_punct('\\'));
    EXPECT_TRUE(is_punct(']'));
    EXPECT_TRUE(is_punct('^'));
    EXPECT_TRUE(is_punct('_'));
    EXPECT_TRUE(is_punct('`'));
    EXPECT_TRUE(is_punct('{'));
    EXPECT_TRUE(is_punct('|'));
    EXPECT_TRUE(is_punct('}'));
    EXPECT_TRUE(is_punct('~'));
}

TEST_F(IsPunctTest, InvalidAlphabetic) {
    EXPECT_FALSE(is_punct('A'));
    EXPECT_FALSE(is_punct('Z'));
    EXPECT_FALSE(is_punct('a'));
    EXPECT_FALSE(is_punct('z'));
}

TEST_F(IsPunctTest, InvalidDigits) {
    EXPECT_FALSE(is_punct('0'));
    EXPECT_FALSE(is_punct('5'));
    EXPECT_FALSE(is_punct('9'));
}

TEST_F(IsPunctTest, InvalidSpace) {
    EXPECT_FALSE(is_punct(' '));
    EXPECT_FALSE(is_punct('\t'));
    EXPECT_FALSE(is_punct('\n'));
}

TEST_F(IsPunctTest, InvalidControl) {
    EXPECT_FALSE(is_punct('\0'));
    EXPECT_FALSE(is_punct('\x1F'));
    EXPECT_FALSE(is_punct('\x7F'));
}

TEST_F(IsPunctTest, WideChar) {
    EXPECT_FALSE(is_punct(L'A'));
    EXPECT_TRUE(is_punct(L'!'));
    EXPECT_TRUE(is_punct(static_cast<char>('!')));
}

TEST_F(IsCntrlTest, ControlCharactersLow) {
    for (int i = 0; i <= 31; ++i) {
        EXPECT_TRUE(is_cntrl(static_cast<char>(i)));
    }
}

TEST_F(IsCntrlTest, ControlCharacterDel) { EXPECT_TRUE(is_cntrl('\x7F')); }

TEST_F(IsCntrlTest, NonControlCharacters) {
    EXPECT_FALSE(is_cntrl(' '));
    EXPECT_FALSE(is_cntrl('A'));
    EXPECT_FALSE(is_cntrl('z'));
    EXPECT_FALSE(is_cntrl('0'));
    EXPECT_FALSE(is_cntrl('!'));
    EXPECT_FALSE(is_cntrl('~'));
}

TEST_F(IsCntrlTest, AboveAscii) {
    EXPECT_FALSE(is_cntrl('\x80'));
    EXPECT_FALSE(is_cntrl('\xFF'));
}

TEST_F(IsPrintTest, PrintableCharacters) {
    EXPECT_TRUE(is_print(' '));
    EXPECT_TRUE(is_print('A'));
    EXPECT_TRUE(is_print('z'));
    EXPECT_TRUE(is_print('0'));
    EXPECT_TRUE(is_print('!'));
    EXPECT_TRUE(is_print('~'));
}

TEST_F(IsPrintTest, NonPrintableLow) {
    for (int i = 0; i <= 31; ++i) {
        EXPECT_FALSE(is_print(static_cast<char>(i)));
    }
}

TEST_F(IsPrintTest, NonPrintableDel) { EXPECT_FALSE(is_print('\x7F')); }

TEST_F(IsPrintTest, AboveAscii) {
    EXPECT_FALSE(is_print('\x80'));
    EXPECT_FALSE(is_print('\xFF'));
}

TEST_F(IsBlankTest, BlankCharacters) {
    EXPECT_TRUE(is_blank(' '));
    EXPECT_TRUE(is_blank('\t'));
}

TEST_F(IsBlankTest, NonBlankWhitespace) {
    EXPECT_FALSE(is_blank('\n'));
    EXPECT_FALSE(is_blank('\v'));
    EXPECT_FALSE(is_blank('\f'));
    EXPECT_FALSE(is_blank('\r'));
}

TEST_F(IsBlankTest, NonBlankPrintable) {
    EXPECT_FALSE(is_blank('A'));
    EXPECT_FALSE(is_blank('0'));
    EXPECT_FALSE(is_blank('!'));
    EXPECT_FALSE(is_blank('.'));
}

TEST_F(IsBlankTest, ControlCharacters) {
    EXPECT_FALSE(is_blank('\0'));
    EXPECT_FALSE(is_blank('\x1F'));
}

TEST_F(IsBlankTest, ExtendedAscii) { EXPECT_FALSE(is_blank('\x80')); }

TEST_F(IsGraphTest, GraphCharacters) {
    EXPECT_TRUE(is_graph('A'));
    EXPECT_TRUE(is_graph('z'));
    EXPECT_TRUE(is_graph('0'));
    EXPECT_TRUE(is_graph('!'));
    EXPECT_TRUE(is_graph('~'));
    EXPECT_TRUE(is_graph('@'));
    EXPECT_TRUE(is_graph('#'));
}

TEST_F(IsGraphTest, NonGraphSpace) { EXPECT_FALSE(is_graph(' ')); }

TEST_F(IsGraphTest, NonGraphControl) {
    EXPECT_FALSE(is_graph('\t'));
    EXPECT_FALSE(is_graph('\n'));
    EXPECT_FALSE(is_graph('\0'));
    EXPECT_FALSE(is_graph('\x7F'));
}

TEST_F(IsGraphTest, ExtendedAscii) { EXPECT_FALSE(is_graph('\x80')); }

TEST_F(IsAsciiTest, AsciiCharacters) {
    for (int i = 0; i <= 127; ++i) {
        EXPECT_TRUE(is_ascii(static_cast<char>(i)));
    }
}

TEST_F(IsAsciiTest, NonAsciiCharacters) {
    EXPECT_FALSE(is_ascii('\x80'));
    EXPECT_FALSE(is_ascii('\xFF'));
    EXPECT_FALSE(is_ascii(static_cast<char>(128)));
    EXPECT_FALSE(is_ascii(static_cast<char>(255)));
}

TEST_F(IsAsciiTest, WideChar) {
    EXPECT_TRUE(is_ascii(L'A'));
    EXPECT_FALSE(is_ascii(L'\x80'));
    EXPECT_FALSE(is_ascii(L'\xFF'));
    EXPECT_FALSE(is_ascii(L'\x100'));
}

TEST_F(IsSpaceTest, SpaceCharacters) {
    EXPECT_TRUE(is_space(' '));
    EXPECT_TRUE(is_space('\t'));
    EXPECT_TRUE(is_space('\n'));
    EXPECT_TRUE(is_space('\v'));
    EXPECT_TRUE(is_space('\f'));
    EXPECT_TRUE(is_space('\r'));
}

TEST_F(IsSpaceTest, NonSpacePrintable) {
    EXPECT_FALSE(is_space('A'));
    EXPECT_FALSE(is_space('z'));
    EXPECT_FALSE(is_space('0'));
    EXPECT_FALSE(is_space('!'));
    EXPECT_FALSE(is_space('.'));
}

TEST_F(IsSpaceTest, NonSpaceControl) {
    EXPECT_FALSE(is_space('\0'));
    EXPECT_FALSE(is_space('\x1F'));
    EXPECT_FALSE(is_space('\x7F'));
}

TEST_F(IsSpaceTest, ExtendedAscii) {
    EXPECT_FALSE(is_space('\x80'));
    EXPECT_FALSE(is_space('\xA0'));
}

TEST_F(IsAlphaTest, UppercaseLetters) {
    for (char c = 'A'; c <= 'Z'; ++c) {
        EXPECT_TRUE(is_alpha(c));
    }
}

TEST_F(IsAlphaTest, LowercaseLetters) {
    for (char c = 'a'; c <= 'z'; ++c) {
        EXPECT_TRUE(is_alpha(c));
    }
}

TEST_F(IsAlphaTest, NonAlphaDigits) {
    for (char c = '0'; c <= '9'; ++c) {
        EXPECT_FALSE(is_alpha(c));
    }
}

TEST_F(IsAlphaTest, NonAlphaPunctuation) {
    EXPECT_FALSE(is_alpha('!'));
    EXPECT_FALSE(is_alpha('@'));
    EXPECT_FALSE(is_alpha('#'));
    EXPECT_FALSE(is_alpha('['));
    EXPECT_FALSE(is_alpha('`'));
    EXPECT_FALSE(is_alpha('{'));
    EXPECT_FALSE(is_alpha('~'));
}

TEST_F(IsAlphaTest, NonAlphaSpace) {
    EXPECT_FALSE(is_alpha(' '));
    EXPECT_FALSE(is_alpha('\t'));
    EXPECT_FALSE(is_alpha('\n'));
}

TEST_F(IsAlphaTest, NonAlphaControl) {
    EXPECT_FALSE(is_alpha('\0'));
    EXPECT_FALSE(is_alpha('\x1F'));
    EXPECT_FALSE(is_alpha('\x7F'));
}

TEST_F(IsAlphaTest, NonAlphaExtended) {
    EXPECT_FALSE(is_alpha('\x80'));
    EXPECT_FALSE(is_alpha('\xFF'));
}

TEST_F(IsAlphaTest, BoundaryLetters) {
    EXPECT_TRUE(is_alpha('A'));
    EXPECT_TRUE(is_alpha('Z'));
    EXPECT_TRUE(is_alpha('a'));
    EXPECT_TRUE(is_alpha('z'));
}

TEST_F(IsAlphaTest, CharactersAroundBoundaries) {
    EXPECT_FALSE(is_alpha('@'));
    EXPECT_FALSE(is_alpha('['));
    EXPECT_FALSE(is_alpha('`'));
    EXPECT_FALSE(is_alpha('{'));
}

TEST_F(IsDigitTest, Digits) {
    for (char c = '0'; c <= '9'; ++c) {
        EXPECT_TRUE(is_digit(c));
    }
}

TEST_F(IsDigitTest, NonDigitLetters) {
    EXPECT_FALSE(is_digit('A'));
    EXPECT_FALSE(is_digit('Z'));
    EXPECT_FALSE(is_digit('a'));
    EXPECT_FALSE(is_digit('z'));
}

TEST_F(IsDigitTest, NonDigitPunctuation) {
    EXPECT_FALSE(is_digit('!'));
    EXPECT_FALSE(is_digit('/'));
    EXPECT_FALSE(is_digit(':'));
    EXPECT_FALSE(is_digit('@'));
}

TEST_F(IsDigitTest, NonDigitSpace) {
    EXPECT_FALSE(is_digit(' '));
    EXPECT_FALSE(is_digit('\t'));
}

TEST_F(IsDigitTest, NonDigitExtended) {
    EXPECT_FALSE(is_digit('\x80'));
    EXPECT_FALSE(is_digit('\xFF'));
}

TEST_F(IsDigitTest, BoundaryDigits) {
    EXPECT_TRUE(is_digit('0'));
    EXPECT_TRUE(is_digit('9'));
}

TEST_F(IsDigitTest, CharactersAroundBoundaries) {
    EXPECT_FALSE(is_digit('/'));
    EXPECT_FALSE(is_digit(':'));
}

TEST_F(IsXdigitTest, DecimalDigits) {
    for (char c = '0'; c <= '9'; ++c) {
        EXPECT_TRUE(is_xdigit(c));
    }
}

TEST_F(IsXdigitTest, UppercaseHexLetters) {
    for (char c = 'A'; c <= 'F'; ++c) {
        EXPECT_TRUE(is_xdigit(c));
    }
}

TEST_F(IsXdigitTest, LowercaseHexLetters) {
    for (char c = 'a'; c <= 'f'; ++c) {
        EXPECT_TRUE(is_xdigit(c));
    }
}

TEST_F(IsXdigitTest, NonHexUppercase) {
    EXPECT_FALSE(is_xdigit('G'));
    EXPECT_FALSE(is_xdigit('H'));
    EXPECT_FALSE(is_xdigit('Z'));
}

TEST_F(IsXdigitTest, NonHexLowercase) {
    EXPECT_FALSE(is_xdigit('g'));
    EXPECT_FALSE(is_xdigit('h'));
    EXPECT_FALSE(is_xdigit('z'));
}

TEST_F(IsXdigitTest, NonHexPunctuation) {
    EXPECT_FALSE(is_xdigit('!'));
    EXPECT_FALSE(is_xdigit('@'));
    EXPECT_FALSE(is_xdigit('`'));
}

TEST_F(IsXdigitTest, NonHexSpace) {
    EXPECT_FALSE(is_xdigit(' '));
    EXPECT_FALSE(is_xdigit('\t'));
}

TEST_F(IsXdigitTest, NonHexExtended) { EXPECT_FALSE(is_xdigit('\x80')); }

TEST_F(IsXdigitTest, HexBoundaries) {
    EXPECT_TRUE(is_xdigit('0'));
    EXPECT_TRUE(is_xdigit('9'));
    EXPECT_TRUE(is_xdigit('A'));
    EXPECT_TRUE(is_xdigit('F'));
    EXPECT_TRUE(is_xdigit('a'));
    EXPECT_TRUE(is_xdigit('f'));
}

TEST_F(IsXdigitTest, NonHexBoundaries) {
    EXPECT_FALSE(is_xdigit('/'));
    EXPECT_FALSE(is_xdigit(':'));
    EXPECT_FALSE(is_xdigit('@'));
    EXPECT_FALSE(is_xdigit('G'));
    EXPECT_FALSE(is_xdigit('`'));
    EXPECT_FALSE(is_xdigit('g'));
}

TEST_F(IsAlphaOrDigitTest, AlphanumericCharacters) {
    EXPECT_TRUE(is_alpha_or_digit('A'));
    EXPECT_TRUE(is_alpha_or_digit('Z'));
    EXPECT_TRUE(is_alpha_or_digit('a'));
    EXPECT_TRUE(is_alpha_or_digit('z'));
    EXPECT_TRUE(is_alpha_or_digit('0'));
    EXPECT_TRUE(is_alpha_or_digit('5'));
    EXPECT_TRUE(is_alpha_or_digit('9'));
}

TEST_F(IsAlphaOrDigitTest, NonAlphanumericPunctuation) {
    EXPECT_FALSE(is_alpha_or_digit('!'));
    EXPECT_FALSE(is_alpha_or_digit('@'));
    EXPECT_FALSE(is_alpha_or_digit('['));
    EXPECT_FALSE(is_alpha_or_digit('`'));
}

TEST_F(IsAlphaOrDigitTest, NonAlphanumericSpace) {
    EXPECT_FALSE(is_alpha_or_digit(' '));
    EXPECT_FALSE(is_alpha_or_digit('\t'));
    EXPECT_FALSE(is_alpha_or_digit('\n'));
}

TEST_F(IsAlphaOrDigitTest, NonAlphanumericControl) {
    EXPECT_FALSE(is_alpha_or_digit('\0'));
    EXPECT_FALSE(is_alpha_or_digit('\x7F'));
}

TEST_F(IsAlphaOrDigitTest, NonAlphanumericExtended) { EXPECT_FALSE(is_alpha_or_digit('\x80')); }

TEST_F(IsDigitOrAlphaTest, AlphanumericCharacters) {
    EXPECT_TRUE(is_digit_or_alpha('0'));
    EXPECT_TRUE(is_digit_or_alpha('5'));
    EXPECT_TRUE(is_digit_or_alpha('9'));
    EXPECT_TRUE(is_digit_or_alpha('A'));
    EXPECT_TRUE(is_digit_or_alpha('Z'));
    EXPECT_TRUE(is_digit_or_alpha('a'));
    EXPECT_TRUE(is_digit_or_alpha('z'));
}

TEST_F(IsDigitOrAlphaTest, NonAlphanumericPunctuation) {
    EXPECT_FALSE(is_digit_or_alpha('!'));
    EXPECT_FALSE(is_digit_or_alpha('@'));
    EXPECT_FALSE(is_digit_or_alpha('['));
    EXPECT_FALSE(is_digit_or_alpha('`'));
}

TEST_F(IsDigitOrAlphaTest, NonAlphanumericSpace) {
    EXPECT_FALSE(is_digit_or_alpha(' '));
    EXPECT_FALSE(is_digit_or_alpha('\t'));
    EXPECT_FALSE(is_digit_or_alpha('\n'));
}

TEST_F(IsDigitOrAlphaTest, NonAlphanumericControl) {
    EXPECT_FALSE(is_digit_or_alpha('\0'));
    EXPECT_FALSE(is_digit_or_alpha('\x7F'));
}

TEST_F(IsDigitOrAlphaTest, NonAlphanumericExtended) { EXPECT_FALSE(is_digit_or_alpha('\x80')); }

TEST_F(IsDigitOrAlphaTest, AllHexCharacters) {
    for (char c = '0'; c <= '9'; ++c) {
        EXPECT_TRUE(is_digit_or_alpha(c));
    }
    for (char c = 'A'; c <= 'F'; ++c) {
        EXPECT_TRUE(is_digit_or_alpha(c));
    }
    for (char c = 'a'; c <= 'f'; ++c) {
        EXPECT_TRUE(is_digit_or_alpha(c));
    }
}

class CharTraitsCharTest : public ::testing::Test {};
class CharTraitsWcharTest : public ::testing::Test {};
class CharTraitsChar16Test : public ::testing::Test {};
class CharTraitsChar32Test : public ::testing::Test {};
class CharTraitsEqualTest : public ::testing::Test {};
class CharTraitsCompareTest : public ::testing::Test {};
class CharTraitsFindTest : public ::testing::Test {};
class CharTraitsFindCharTest : public ::testing::Test {};
class CharTraitsRfindTest : public ::testing::Test {};
class CharTraitsRfindCharTest : public ::testing::Test {};
class CharTraitsFindFirstOfTest : public ::testing::Test {};
class CharTraitsFindLastOfTest : public ::testing::Test {};
class CharTraitsFindFirstNotOfTest : public ::testing::Test {};
class CharTraitsFindNotCharTest : public ::testing::Test {};
class CharTraitsFindLastNotOfTest : public ::testing::Test {};
class CharTraitsRfindNotCharTest : public ::testing::Test {};

TEST_F(CharTraitsCharTest, Copy) {
    const char src[] = "Hello";
    char dest[6] = {};
    char_traits<char>::copy(dest, src, 5);
    EXPECT_STREQ(dest, "Hello");
}

TEST_F(CharTraitsCharTest, Move) {
    char data[] = "Hello";
    char_traits<char>::move(data + 2, data, 3);
    EXPECT_EQ(data[2], 'H');
    EXPECT_EQ(data[3], 'e');
    EXPECT_EQ(data[4], 'l');
}

TEST_F(CharTraitsCharTest, MoveOverlap) {
    char data[] = "ABCDEF";
    char_traits<char>::move(data, data + 2, 4);
    EXPECT_EQ(data[0], 'C');
    EXPECT_EQ(data[1], 'D');
    EXPECT_EQ(data[2], 'E');
    EXPECT_EQ(data[3], 'F');
}

TEST_F(CharTraitsCharTest, CompareEqual) {
    EXPECT_EQ(char_traits<char>::compare("abc", "abc", 3), 0);
    EXPECT_EQ(char_traits<char>::compare("", "", 0), 0);
}

TEST_F(CharTraitsCharTest, CompareLess) {
    EXPECT_LT(char_traits<char>::compare("abc", "abd", 3), 0);
    EXPECT_LT(char_traits<char>::compare("abc", "bbc", 3), 0);
}

TEST_F(CharTraitsCharTest, CompareGreater) {
    EXPECT_GT(char_traits<char>::compare("abd", "abc", 3), 0);
    EXPECT_GT(char_traits<char>::compare("bbc", "abc", 3), 0);
}

TEST_F(CharTraitsCharTest, Length) {
    EXPECT_EQ(char_traits<char>::length("Hello"), 5u);
    EXPECT_EQ(char_traits<char>::length(""), 0u);
    EXPECT_EQ(char_traits<char>::length("A"), 1u);
}

TEST_F(CharTraitsCharTest, LengthLongString) {
    const char* str = "This is a longer string for length testing";
    EXPECT_EQ(char_traits<char>::length(str), string_length(str));
}

TEST_F(CharTraitsCharTest, FindFound) {
    const char str[] = "Hello World";
    const char* result = char_traits<char>::find(str, 11, 'W');
    EXPECT_NE(result, nullptr);
    EXPECT_EQ(*result, 'W');
    EXPECT_EQ(result - str, 6);
}

TEST_F(CharTraitsCharTest, FindFirst) {
    const char str[] = "Hello";
    const char* result = char_traits<char>::find(str, 5, 'H');
    EXPECT_EQ(result - str, 0);
}

TEST_F(CharTraitsCharTest, FindLast) {
    const char str[] = "Hello";
    const char* result = char_traits<char>::find(str, 5, 'o');
    EXPECT_EQ(result - str, 4);
}

TEST_F(CharTraitsCharTest, FindNotFound) {
    const char str[] = "Hello";
    const char* result = char_traits<char>::find(str, 5, 'z');
    EXPECT_EQ(result, nullptr);
}

TEST_F(CharTraitsCharTest, FindEmpty) {
    const char str[] = "";
    const char* result = char_traits<char>::find(str, 0, 'a');
    EXPECT_EQ(result, nullptr);
}

TEST_F(CharTraitsCharTest, AssignArray) {
    char data[5];
    char_traits<char>::assign(data, 5, 'X');
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(data[i], 'X');
    }
}

TEST_F(CharTraitsCharTest, AssignArrayEmpty) {
    char data[1] = {'A'};
    char_traits<char>::assign(data, 0, 'X');
    EXPECT_EQ(data[0], 'A');
}

TEST_F(CharTraitsCharTest, AssignSingleChar) {
    char lhs = 'A';
    char_traits<char>::assign(lhs, 'B');
    EXPECT_EQ(lhs, 'B');
}

TEST_F(CharTraitsCharTest, Eq) {
    EXPECT_TRUE(char_traits<char>::eq('A', 'A'));
    EXPECT_FALSE(char_traits<char>::eq('A', 'B'));
    EXPECT_TRUE(char_traits<char>::eq('\0', '\0'));
}

TEST_F(CharTraitsCharTest, Lt) {
    EXPECT_TRUE(char_traits<char>::lt('A', 'B'));
    EXPECT_FALSE(char_traits<char>::lt('B', 'A'));
    EXPECT_FALSE(char_traits<char>::lt('A', 'A'));
    EXPECT_TRUE(char_traits<char>::lt('\0', 'A'));
}

TEST_F(CharTraitsCharTest, Eof) {
    EXPECT_EQ(char_traits<char>::eof(), -1);
    EXPECT_EQ(char_traits<char>::eof(), static_cast<int32_t>(-1));
}

TEST_F(CharTraitsCharTest, NotEof) {
    EXPECT_EQ(char_traits<char>::not_eof('A'), 'A');
    EXPECT_EQ(char_traits<char>::not_eof(-1), 0);
    EXPECT_EQ(char_traits<char>::not_eof(0), 0);
}

TEST_F(CharTraitsWcharTest, Copy) {
    const wchar_t src[] = L"Hello";
    wchar_t dest[6] = {};
    char_traits<wchar_t>::copy(dest, src, 5);
    EXPECT_EQ(dest[0], L'H');
    EXPECT_EQ(dest[4], L'o');
    EXPECT_EQ(dest[5], L'\0');
}

TEST_F(CharTraitsWcharTest, Length) {
    EXPECT_EQ(char_traits<wchar_t>::length(L"Hello"), 5u);
    EXPECT_EQ(char_traits<wchar_t>::length(L""), 0u);
}

TEST_F(CharTraitsWcharTest, CompareEqual) { EXPECT_EQ(char_traits<wchar_t>::compare(L"abc", L"abc", 3), 0); }

TEST_F(CharTraitsWcharTest, CompareLess) { EXPECT_LT(char_traits<wchar_t>::compare(L"abc", L"abd", 3), 0); }

TEST_F(CharTraitsWcharTest, FindFound) {
    const wchar_t str[] = L"Hello World";
    const wchar_t* result = char_traits<wchar_t>::find(str, 11, L'W');
    EXPECT_NE(result, nullptr);
    EXPECT_EQ(*result, L'W');
}

TEST_F(CharTraitsWcharTest, FindNotFound) {
    const wchar_t str[] = L"Hello";
    const wchar_t* result = char_traits<wchar_t>::find(str, 5, L'z');
    EXPECT_EQ(result, nullptr);
}

TEST_F(CharTraitsWcharTest, AssignArray) {
    wchar_t data[5];
    char_traits<wchar_t>::assign(data, 5, L'X');
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(data[i], L'X');
    }
}

TEST_F(CharTraitsWcharTest, Eq) {
    EXPECT_TRUE(char_traits<wchar_t>::eq(L'A', L'A'));
    EXPECT_FALSE(char_traits<wchar_t>::eq(L'A', L'B'));
}

TEST_F(CharTraitsWcharTest, Lt) {
    EXPECT_TRUE(char_traits<wchar_t>::lt(L'A', L'B'));
    EXPECT_FALSE(char_traits<wchar_t>::lt(L'B', L'A'));
}

TEST_F(CharTraitsWcharTest, Eof) { EXPECT_EQ(char_traits<wchar_t>::eof(), static_cast<uint32_t>(-1)); }

TEST_F(CharTraitsChar16Test, BasicOperations) {
    const char16_t src[] = u"Hello";
    EXPECT_EQ(char_traits<char16_t>::length(src), 5u);

    char16_t dest[6] = {};
    char_traits<char16_t>::copy(dest, src, 5);
    EXPECT_EQ(dest[0], u'H');

    EXPECT_EQ(char_traits<char16_t>::compare(src, src, 5), 0);
}

TEST_F(CharTraitsChar32Test, BasicOperations) {
    const char32_t src[] = U"Hello";
    EXPECT_EQ(char_traits<char32_t>::length(src), 5u);

    char32_t dest[6] = {};
    char_traits<char32_t>::copy(dest, src, 5);
    EXPECT_EQ(dest[0], U'H');

    EXPECT_EQ(char_traits<char32_t>::compare(src, src, 5), 0);
}

TEST_F(CharTraitsEqualTest, EqualStrings) {
    EXPECT_TRUE(char_traits_equal<char_traits<char>>("abc", 3, "abc", 3));
    EXPECT_TRUE(char_traits_equal<char_traits<char>>("", 0, "", 0));
}

TEST_F(CharTraitsEqualTest, NotEqualStrings) {
    EXPECT_FALSE(char_traits_equal<char_traits<char>>("abc", 3, "abd", 3));
    EXPECT_FALSE(char_traits_equal<char_traits<char>>("abc", 3, "ab", 2));
}

TEST_F(CharTraitsEqualTest, DifferentLengths) {
    EXPECT_FALSE(char_traits_equal<char_traits<char>>("abc", 3, "abcd", 4));
}

TEST_F(CharTraitsCompareTest, EqualStrings) {
    EXPECT_EQ(char_traits_compare<char_traits<char>>("abc", 3, "abc", 3), 0);
}

TEST_F(CharTraitsCompareTest, LeftLess) { EXPECT_LT(char_traits_compare<char_traits<char>>("abc", 3, "abd", 3), 0); }

TEST_F(CharTraitsCompareTest, RightLess) { EXPECT_GT(char_traits_compare<char_traits<char>>("abd", 3, "abc", 3), 0); }

TEST_F(CharTraitsCompareTest, LeftShorter) { EXPECT_LT(char_traits_compare<char_traits<char>>("ab", 2, "abc", 3), 0); }

TEST_F(CharTraitsCompareTest, LeftLonger) { EXPECT_GT(char_traits_compare<char_traits<char>>("abc", 3, "ab", 2), 0); }

TEST_F(CharTraitsFindTest, FindSubstring) {
    const char dest[] = "Hello World";
    EXPECT_EQ(char_traits_find<char_traits<char>>(dest, 11, 0, "World", 5), 6u);
}

TEST_F(CharTraitsFindTest, FindAtStart) {
    const char dest[] = "Hello World";
    EXPECT_EQ(char_traits_find<char_traits<char>>(dest, 11, 0, "Hello", 5), 0u);
}

TEST_F(CharTraitsFindTest, FindWithStartOffset) {
    const char dest[] = "Hello Hello";
    EXPECT_EQ(char_traits_find<char_traits<char>>(dest, 11, 3, "Hello", 5), 6u);
}

TEST_F(CharTraitsFindTest, FindEmptySubstring) {
    const char dest[] = "Hello";
    EXPECT_EQ(char_traits_find<char_traits<char>>(dest, 5, 0, "", 0), 0u);
    EXPECT_EQ(char_traits_find<char_traits<char>>(dest, 5, 2, "", 0), 2u);
}

TEST_F(CharTraitsFindTest, NotFound) {
    const char dest[] = "Hello";
    EXPECT_EQ(char_traits_find<char_traits<char>>(dest, 5, 0, "xyz", 3), static_cast<size_t>(-1));
}

TEST_F(CharTraitsFindTest, SubstringLongerThanDest) {
    const char dest[] = "Hi";
    EXPECT_EQ(char_traits_find<char_traits<char>>(dest, 2, 0, "LongSubstring", 13), static_cast<size_t>(-1));
}

TEST_F(CharTraitsFindTest, StartBeyondBounds) {
    const char dest[] = "Hello";
    EXPECT_EQ(char_traits_find<char_traits<char>>(dest, 5, 10, "lo", 2), static_cast<size_t>(-1));
}

TEST_F(CharTraitsFindCharTest, FindChar) {
    const char dest[] = "Hello World";
    EXPECT_EQ(char_traits_find_char<char_traits<char>>(dest, 11, 0, 'W'), 6u);
    EXPECT_EQ(char_traits_find_char<char_traits<char>>(dest, 11, 0, 'H'), 0u);
    EXPECT_EQ(char_traits_find_char<char_traits<char>>(dest, 11, 0, 'o'), 4u);
}

TEST_F(CharTraitsFindCharTest, NotFound) {
    const char dest[] = "Hello";
    EXPECT_EQ(char_traits_find_char<char_traits<char>>(dest, 5, 0, 'z'), static_cast<size_t>(-1));
}

TEST_F(CharTraitsFindCharTest, StartBeyondSize) {
    const char dest[] = "Hello";
    EXPECT_EQ(char_traits_find_char<char_traits<char>>(dest, 5, 10, 'H'), static_cast<size_t>(-1));
}

TEST_F(CharTraitsRfindTest, RfindSubstring) {
    const char dest[] = "Hello World Hello";
    EXPECT_EQ(char_traits_rfind<char_traits<char>>(dest, 17, 16, "Hello", 5), 12u);
}

TEST_F(CharTraitsRfindTest, RfindAtStart) {
    const char dest[] = "Hello World";
    EXPECT_EQ(char_traits_rfind<char_traits<char>>(dest, 11, 10, "Hello", 5), 0u);
}

TEST_F(CharTraitsRfindTest, RfindEmptySubstring) {
    const char dest[] = "Hello";
    EXPECT_EQ(char_traits_rfind<char_traits<char>>(dest, 5, 4, "", 0), 4u);
    EXPECT_EQ(char_traits_rfind<char_traits<char>>(dest, 5, 2, "", 0), 2u);
}

TEST_F(CharTraitsRfindTest, NotFound) {
    const char dest[] = "Hello";
    EXPECT_EQ(char_traits_rfind<char_traits<char>>(dest, 5, 4, "xyz", 3), static_cast<size_t>(-1));
}

TEST_F(CharTraitsRfindTest, SubstringLongerThanDest) {
    const char dest[] = "Hi";
    EXPECT_EQ(char_traits_rfind<char_traits<char>>(dest, 2, 1, "LongString", 10), static_cast<size_t>(-1));
}

TEST_F(CharTraitsRfindCharTest, RfindChar) {
    const char dest[] = "Hello World Hello";
    EXPECT_EQ(char_traits_rfind_char<char_traits<char>>(dest, 17, 16, 'o'), 16u);
    EXPECT_EQ(char_traits_rfind_char<char_traits<char>>(dest, 17, 16, 'H'), 12u);
}

TEST_F(CharTraitsRfindCharTest, NotFound) {
    const char dest[] = "Hello";
    EXPECT_EQ(char_traits_rfind_char<char_traits<char>>(dest, 5, 4, 'z'), static_cast<size_t>(-1));
}

TEST_F(CharTraitsRfindCharTest, EmptyDest) {
    const char dest[] = "";
    EXPECT_EQ(char_traits_rfind_char<char_traits<char>>(dest, 0, 0, 'a'), static_cast<size_t>(-1));
}

TEST_F(CharTraitsFindFirstOfTest, FindFirstOf) {
    const char dest[] = "Hello World";
    const char charset[] = "oe";
    EXPECT_EQ(char_traits_find_first_of<char_traits<char>>(dest, 11, 0, charset, 2), 1u);
}

TEST_F(CharTraitsFindFirstOfTest, FindFirstOfWithStart) {
    const char dest[] = "Hello World";
    const char charset[] = "oe";
    EXPECT_EQ(char_traits_find_first_of<char_traits<char>>(dest, 11, 3, charset, 2), 4u);
}

TEST_F(CharTraitsFindFirstOfTest, NotFound) {
    const char dest[] = "Hello";
    const char charset[] = "xyz";
    EXPECT_EQ(char_traits_find_first_of<char_traits<char>>(dest, 5, 0, charset, 3), static_cast<size_t>(-1));
}

TEST_F(CharTraitsFindFirstOfTest, EmptyCharset) {
    const char dest[] = "Hello";
    const char charset[] = "";
    EXPECT_EQ(char_traits_find_first_of<char_traits<char>>(dest, 5, 0, charset, 0), static_cast<size_t>(-1));
}

TEST_F(CharTraitsFindFirstOfTest, StartBeyondSize) {
    const char dest[] = "Hello";
    const char charset[] = "H";
    EXPECT_EQ(char_traits_find_first_of<char_traits<char>>(dest, 5, 10, charset, 1), static_cast<size_t>(-1));
}

TEST_F(CharTraitsFindLastOfTest, FindLastOf) {
    const char dest[] = "Hello World Hello";
    const char charset[] = "oe";
    EXPECT_EQ(char_traits_find_last_of<char_traits<char>>(dest, 17, 16, charset, 2), 16u);
}

TEST_F(CharTraitsFindLastOfTest, FindLastOfWithStart) {
    const char dest[] = "Hello World Hello";
    const char charset[] = "oe";
    EXPECT_EQ(char_traits_find_last_of<char_traits<char>>(dest, 17, 6, charset, 2), 4u);
}

TEST_F(CharTraitsFindLastOfTest, NotFound) {
    const char dest[] = "Hello";
    const char charset[] = "xyz";
    EXPECT_EQ(char_traits_find_last_of<char_traits<char>>(dest, 5, 4, charset, 3), static_cast<size_t>(-1));
}

TEST_F(CharTraitsFindLastOfTest, EmptyCharset) {
    const char dest[] = "Hello";
    const char charset[] = "";
    EXPECT_EQ(char_traits_find_last_of<char_traits<char>>(dest, 5, 4, charset, 0), static_cast<size_t>(-1));
}

TEST_F(CharTraitsFindLastOfTest, EmptyDest) {
    const char dest[] = "";
    const char charset[] = "a";
    EXPECT_EQ(char_traits_find_last_of<char_traits<char>>(dest, 0, 0, charset, 1), static_cast<size_t>(-1));
}

TEST_F(CharTraitsFindFirstNotOfTest, FindFirstNotOf) {
    const char dest[] = "Hello World";
    const char charset[] = "Helo Wrd";
    EXPECT_EQ(char_traits_find_first_not_of<char_traits<char>>(dest, 11, 0, charset, 9), static_cast<size_t>(-1));
}

TEST_F(CharTraitsFindFirstNotOfTest, FindFirstNotOfFound) {
    const char dest[] = "aaabcaaa";
    const char charset[] = "a";
    EXPECT_EQ(char_traits_find_first_not_of<char_traits<char>>(dest, 8, 0, charset, 1), 3u);
}

TEST_F(CharTraitsFindFirstNotOfTest, WithStart) {
    const char dest[] = "aaabcaaa";
    const char charset[] = "a";
    EXPECT_EQ(char_traits_find_first_not_of<char_traits<char>>(dest, 8, 4, charset, 1), 4u);
}

TEST_F(CharTraitsFindFirstNotOfTest, EmptyCharset) {
    const char dest[] = "Hello";
    const char charset[] = "";
    EXPECT_EQ(char_traits_find_first_not_of<char_traits<char>>(dest, 5, 0, charset, 0), 0u);
}

TEST_F(CharTraitsFindFirstNotOfTest, StartBeyondSize) {
    const char dest[] = "Hello";
    const char charset[] = "H";
    EXPECT_EQ(char_traits_find_first_not_of<char_traits<char>>(dest, 5, 10, charset, 1), static_cast<size_t>(-1));
}

TEST_F(CharTraitsFindNotCharTest, FindNotChar) {
    const char dest[] = "aaabcaaa";
    EXPECT_EQ(char_traits_find_not_char<char_traits<char>>(dest, 8, 0, 'a'), 3u);
}

TEST_F(CharTraitsFindNotCharTest, NotFound) {
    const char dest[] = "aaaa";
    EXPECT_EQ(char_traits_find_not_char<char_traits<char>>(dest, 4, 0, 'a'), static_cast<size_t>(-1));
}

TEST_F(CharTraitsFindNotCharTest, StartBeyondSize) {
    const char dest[] = "abc";
    EXPECT_EQ(char_traits_find_not_char<char_traits<char>>(dest, 3, 10, 'a'), static_cast<size_t>(-1));
}

TEST_F(CharTraitsFindNotCharTest, StartAtEnd) {
    const char dest[] = "abc";
    EXPECT_EQ(char_traits_find_not_char<char_traits<char>>(dest, 3, 3, 'a'), static_cast<size_t>(-1));
}

TEST_F(CharTraitsFindLastNotOfTest, FindLastNotOf) {
    const char dest[] = "aaabcaaa";
    const char charset[] = "a";
    EXPECT_EQ(char_traits_find_last_not_of<char_traits<char>>(dest, 8, 7, charset, 1), 4u);
}

TEST_F(CharTraitsFindLastNotOfTest, NotFound) {
    const char dest[] = "aaaa";
    const char charset[] = "a";
    EXPECT_EQ(char_traits_find_last_not_of<char_traits<char>>(dest, 4, 3, charset, 1), static_cast<size_t>(-1));
}

TEST_F(CharTraitsFindLastNotOfTest, EmptyCharset) {
    const char dest[] = "Hello";
    const char charset[] = "";
    EXPECT_EQ(char_traits_find_last_not_of<char_traits<char>>(dest, 5, 4, charset, 0), 4u);
}

TEST_F(CharTraitsFindLastNotOfTest, EmptyDest) {
    const char dest[] = "";
    const char charset[] = "a";
    EXPECT_EQ(char_traits_find_last_not_of<char_traits<char>>(dest, 0, 0, charset, 1), static_cast<size_t>(-1));
}

TEST_F(CharTraitsFindLastNotOfTest, WithStart) {
    const char dest[] = "aaabcaaa";
    const char charset[] = "a";
    EXPECT_EQ(char_traits_find_last_not_of<char_traits<char>>(dest, 8, 3, charset, 1), 3u);
}

TEST_F(CharTraitsRfindNotCharTest, RfindNotChar) {
    const char dest[] = "aaabcaaa";
    EXPECT_EQ(char_traits_rfind_not_char<char_traits<char>>(dest, 8, 7, 'a'), 4u);
}

TEST_F(CharTraitsRfindNotCharTest, NotFound) {
    const char dest[] = "aaaa";
    EXPECT_EQ(char_traits_rfind_not_char<char_traits<char>>(dest, 4, 3, 'a'), static_cast<size_t>(-1));
}

TEST_F(CharTraitsRfindNotCharTest, EmptyDest) {
    const char dest[] = "";
    EXPECT_EQ(char_traits_rfind_not_char<char_traits<char>>(dest, 0, 0, 'a'), static_cast<size_t>(-1));
}

TEST_F(CharTraitsRfindNotCharTest, WithStart) {
    const char dest[] = "aabcaaa";
    EXPECT_EQ(char_traits_rfind_not_char<char_traits<char>>(dest, 7, 4, 'a'), 3u);
}

class BasicStringViewConstructionTest : public ::testing::Test {};
class BasicStringViewIteratorsTest : public ::testing::Test {};
class BasicStringViewElementAccessTest : public ::testing::Test {};
class BasicStringViewCapacityTest : public ::testing::Test {};
class BasicStringViewModifiersTest : public ::testing::Test {};
class BasicStringViewOperationsTest : public ::testing::Test {};
class BasicStringViewFindTest : public ::testing::Test {};
class BasicStringViewRfindTest : public ::testing::Test {};
class BasicStringViewFindFirstOfTest : public ::testing::Test {};
class BasicStringViewFindLastOfTest : public ::testing::Test {};
class BasicStringViewFindFirstNotOfTest : public ::testing::Test {};
class BasicStringViewFindLastNotOfTest : public ::testing::Test {};
class BasicStringViewCompareTest : public ::testing::Test {};
class BasicStringViewStartsEndsWithTest : public ::testing::Test {};
class BasicStringViewContainsTest : public ::testing::Test {};
class BasicStringViewTrimTest : public ::testing::Test {};
class BasicStringViewCountTest : public ::testing::Test {};
class BasicStringViewSwapTest : public ::testing::Test {};
class BasicStringViewHashTest : public ::testing::Test {};

TEST_F(BasicStringViewConstructionTest, DefaultConstructor) {
    string_view view{""};
    EXPECT_EQ(view.size(), 0u);
    EXPECT_TRUE(view.empty());
    EXPECT_NE(view.data(), nullptr);
}

TEST_F(BasicStringViewConstructionTest, FromCString) {
    string_view view("Hello");
    EXPECT_EQ(view.size(), 5u);
    EXPECT_EQ(view[0], 'H');
    EXPECT_EQ(view[4], 'o');
}

TEST_F(BasicStringViewConstructionTest, FromPointerAndSize) {
    const char* str = "Hello World";
    string_view view(str, 5);
    EXPECT_EQ(view.size(), 5u);
    EXPECT_EQ(view[0], 'H');
    EXPECT_EQ(view[4], 'o');
}

TEST_F(BasicStringViewConstructionTest, FromPointerAndSizeWithEmbeddedNull) {
    const char str[] = {'H', 'i', '\0', 'T', 'h', 'e', 'r', 'e'};
    string_view view(str, 8);
    EXPECT_EQ(view.size(), 8u);
    EXPECT_EQ(view[0], 'H');
    EXPECT_EQ(view[2], '\0');
    EXPECT_EQ(view[3], 'T');
}

TEST_F(BasicStringViewConstructionTest, FromIteratorRange) {
    const char str[] = "Hello";
    string_view view(begin(str), end(str) - 1);
    EXPECT_EQ(view.size(), 5u);
    EXPECT_EQ(view[0], 'H');
}

TEST_F(BasicStringViewConstructionTest, CopyConstructor) {
    string_view view1("Hello");
    string_view view2(view1);
    EXPECT_EQ(view2.size(), 5u);
    EXPECT_EQ(view2[0], 'H');
}

TEST_F(BasicStringViewConstructionTest, CopyAssignment) {
    string_view view1("Hello");
    string_view view2;
    view2 = view1;
    EXPECT_EQ(view2.size(), 5u);
    EXPECT_EQ(view2[0], 'H');
}

TEST_F(BasicStringViewConstructionTest, EmptyString) {
    string_view view("");
    EXPECT_EQ(view.size(), 0u);
    EXPECT_TRUE(view.empty());
}

TEST_F(BasicStringViewConstructionTest, NullptrString) {
    string_view view(nullptr, 0);
    EXPECT_EQ(view.size(), 0u);
}

TEST_F(BasicStringViewIteratorsTest, BeginEnd) {
    string_view view("Hello");
    auto it = view.begin();
    EXPECT_EQ(*it, 'H');
    ++it;
    EXPECT_EQ(*it, 'e');
    it = view.end();
    --it;
    EXPECT_EQ(*it, 'o');
}

TEST_F(BasicStringViewIteratorsTest, CBeginCEnd) {
    const string_view view("Hello");
    auto it = view.cbegin();
    EXPECT_EQ(*it, 'H');
    it = view.cend();
    --it;
    EXPECT_EQ(*it, 'o');
}

TEST_F(BasicStringViewIteratorsTest, ReverseIterators) {
    string_view view("Hello");
    auto rit = view.rbegin();
    EXPECT_EQ(*rit, 'o');
    ++rit;
    EXPECT_EQ(*rit, 'l');
    rit = view.rend();
    --rit;
    EXPECT_EQ(*rit, 'H');
}

TEST_F(BasicStringViewIteratorsTest, CRbeginCRend) {
    const string_view view("Hello");
    auto rit = view.crbegin();
    EXPECT_EQ(*rit, 'o');
    rit = view.crend();
    --rit;
    EXPECT_EQ(*rit, 'H');
}

TEST_F(BasicStringViewIteratorsTest, IteratorLoop) {
    string_view view("ABC");
    string result;
    for (auto it = view.begin(); it != view.end(); ++it) {
        result += *it;
    }
    EXPECT_EQ(result, "ABC");
}

TEST_F(BasicStringViewIteratorsTest, ReverseIteratorLoop) {
    string_view view("ABC");
    string result;
    for (auto rit = view.rbegin(); rit != view.rend(); ++rit) {
        result += *rit;
    }
    EXPECT_EQ(result, "CBA");
}

TEST_F(BasicStringViewIteratorsTest, IteratorRandomAccess) {
    string_view view("Hello World");
    auto it = view.begin();
    it += 6;
    EXPECT_EQ(*it, 'W');
    it -= 4;
    EXPECT_EQ(*it, 'l');
    EXPECT_EQ(it[2], 'o');
}

TEST_F(BasicStringViewElementAccessTest, SubscriptOperator) {
    string_view view("Hello");
    EXPECT_EQ(view[0], 'H');
    EXPECT_EQ(view[1], 'e');
    EXPECT_EQ(view[4], 'o');
}

TEST_F(BasicStringViewElementAccessTest, AtMethod) {
    string_view view("Hello");
    EXPECT_EQ(view.at(0), 'H');
    EXPECT_EQ(view.at(4), 'o');
    EXPECT_EQ(view.at(2), 'l');
}

TEST_F(BasicStringViewElementAccessTest, Front) {
    string_view view("Hello");
    EXPECT_EQ(view.front(), 'H');
}

TEST_F(BasicStringViewElementAccessTest, Back) {
    string_view view("Hello");
    EXPECT_EQ(view.back(), 'o');
}

TEST_F(BasicStringViewElementAccessTest, Data) {
    string_view view("Hello");
    EXPECT_EQ(view.data()[0], 'H');
    EXPECT_EQ(view.data()[4], 'o');
}

TEST_F(BasicStringViewCapacityTest, Size) {
    string_view view1;
    EXPECT_EQ(view1.size(), 0u);
    string_view view2("Hello");
    EXPECT_EQ(view2.size(), 5u);
    string_view view3("Hello World");
    EXPECT_EQ(view3.size(), 11u);
}

TEST_F(BasicStringViewCapacityTest, Length) {
    string_view view("Hello");
    EXPECT_EQ(view.length(), 5u);
}

TEST_F(BasicStringViewCapacityTest, MaxSize) {
    string_view view;
    EXPECT_GT(view.max_size(), 0u);
}

TEST_F(BasicStringViewCapacityTest, Empty) {
    string_view view1;
    EXPECT_TRUE(view1.empty());
    string_view view2("Hello");
    EXPECT_FALSE(view2.empty());
    string_view view3("");
    EXPECT_TRUE(view3.empty());
}

TEST_F(BasicStringViewModifiersTest, RemovePrefix) {
    string_view view("Hello World");
    view.remove_prefix(6);
    EXPECT_EQ(view.size(), 5u);
    EXPECT_EQ(view[0], 'W');
}

TEST_F(BasicStringViewModifiersTest, RemovePrefixEntireString) {
    string_view view("Hi");
    view.remove_prefix(2);
    EXPECT_EQ(view.size(), 0u);
    EXPECT_TRUE(view.empty());
}

TEST_F(BasicStringViewModifiersTest, RemoveSuffix) {
    string_view view("Hello World");
    view.remove_suffix(6);
    EXPECT_EQ(view.size(), 5u);
    EXPECT_EQ(view[4], 'o');
}

TEST_F(BasicStringViewModifiersTest, RemoveSuffixEntireString) {
    string_view view("Hi");
    view.remove_suffix(2);
    EXPECT_EQ(view.size(), 0u);
    EXPECT_TRUE(view.empty());
}

TEST_F(BasicStringViewModifiersTest, Copy) {
    string_view view("Hello");
    char buffer[10] = {};
    size_t copied = view.copy(buffer, 5);
    EXPECT_EQ(copied, 5u);
    EXPECT_STREQ(buffer, "Hello");
}

TEST_F(BasicStringViewModifiersTest, CopyWithOffset) {
    string_view view("Hello World");
    char buffer[10] = {};
    size_t copied = view.copy(buffer, 5, 6);
    EXPECT_EQ(copied, 5u);
    EXPECT_STREQ(buffer, "World");
}

TEST_F(BasicStringViewModifiersTest, CopyPartial) {
    string_view view("Hello");
    char buffer[10] = {};
    size_t copied = view.copy(buffer, 3);
    EXPECT_EQ(copied, 3u);
    buffer[3] = '\0';
    EXPECT_STREQ(buffer, "Hel");
}

TEST_F(BasicStringViewOperationsTest, Substr) {
    string_view view("Hello World");
    string_view sub = view.substr(0, 5);
    EXPECT_EQ(sub.size(), 5u);
    EXPECT_EQ(sub[0], 'H');
    EXPECT_EQ(sub[4], 'o');
}

TEST_F(BasicStringViewOperationsTest, SubstrWithDefaultArgs) {
    string_view view("Hello World");
    string_view sub = view.substr(6);
    EXPECT_EQ(sub.size(), 5u);
    EXPECT_EQ(sub[0], 'W');
}

TEST_F(BasicStringViewOperationsTest, SubstrEntireString) {
    string_view view("Hello");
    string_view sub = view.substr();
    EXPECT_EQ(sub.size(), 5u);
    EXPECT_EQ(sub[0], 'H');
}

TEST_F(BasicStringViewOperationsTest, SubstrNpos) {
    string_view view("Hello");
    string_view sub = view.substr(2, string_view::npos);
    EXPECT_EQ(sub.size(), 3u);
    EXPECT_EQ(sub[0], 'l');
}

TEST_F(BasicStringViewOperationsTest, Head) {
    string_view view("Hello World");
    string_view h = view.head(5);
    EXPECT_EQ(h.size(), 5u);
    EXPECT_EQ(h[0], 'H');
}

TEST_F(BasicStringViewOperationsTest, Tail) {
    string_view view("Hello World");
    string_view t = view.tail(6);
    EXPECT_EQ(t.size(), 5u);
    EXPECT_EQ(t[0], 'W');
}

TEST_F(BasicStringViewOperationsTest, View) {
    string_view view("Hello World");
    string_view v = view.view(6, 3);
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(v[0], 'W');
    EXPECT_EQ(v[2], 'r');
}

TEST_F(BasicStringViewCompareTest, CompareEqual) {
    string_view view1("Hello");
    string_view view2("Hello");
    EXPECT_EQ(view1.compare(view2), 0);
}

TEST_F(BasicStringViewCompareTest, CompareLess) {
    string_view view1("abc");
    string_view view2("abd");
    EXPECT_LT(view1.compare(view2), 0);
}

TEST_F(BasicStringViewCompareTest, CompareGreater) {
    string_view view1("abd");
    string_view view2("abc");
    EXPECT_GT(view1.compare(view2), 0);
}

TEST_F(BasicStringViewCompareTest, CompareShorterLess) {
    string_view view1("ab");
    string_view view2("abc");
    EXPECT_LT(view1.compare(view2), 0);
}

TEST_F(BasicStringViewCompareTest, CompareLongerGreater) {
    string_view view1("abc");
    string_view view2("ab");
    EXPECT_GT(view1.compare(view2), 0);
}

TEST_F(BasicStringViewCompareTest, CompareWithOffset) {
    string_view view("Hello World");
    EXPECT_EQ(view.compare(6, 5, string_view("World")), 0);
}

TEST_F(BasicStringViewCompareTest, CompareWithOffsetAndSubstring) {
    string_view view("Hello World");
    EXPECT_EQ(view.compare(0, 5, string_view("Hello World"), 0, 5), 0);
}

TEST_F(BasicStringViewCompareTest, CompareWithCString) {
    string_view view("Hello");
    EXPECT_EQ(view.compare("Hello"), 0);
    EXPECT_LT(view.compare("Hellp"), 0);
}

TEST_F(BasicStringViewCompareTest, CompareWithCStringAndOffset) {
    string_view view("Hello World");
    EXPECT_EQ(view.compare(6, 5, "World"), 0);
}

TEST_F(BasicStringViewCompareTest, CompareWithCStringAndCount) {
    string_view view("Hello");
    EXPECT_EQ(view.compare(0, 3, "HelXXX", 3), 0);
}

TEST_F(BasicStringViewCompareTest, CompareIgnoreCase) {
    string_view view1("Hello");
    string_view view2("hello");
    EXPECT_EQ(view1.compare_ignore_case(view2), 0);
    string_view view3("HELLO");
    EXPECT_EQ(view1.compare_ignore_case(view3), 0);
}

TEST_F(BasicStringViewCompareTest, CompareIgnoreCaseLess) {
    string_view view1("abc");
    string_view view2("ABD");
    EXPECT_LT(view1.compare_ignore_case(view2), 0);
}

TEST_F(BasicStringViewCompareTest, CompareIgnoreCaseCString) {
    string_view view("Hello");
    EXPECT_EQ(view.compare_ignore_case("hello"), 0);
    EXPECT_EQ(view.compare_ignore_case("HELLO"), 0);
}

TEST_F(BasicStringViewCompareTest, EqualTo) {
    string_view view1("Hello");
    string_view view2("Hello");
    EXPECT_TRUE(view1.equal_to(view2));
    string_view view3("World");
    EXPECT_FALSE(view1.equal_to(view3));
}

TEST_F(BasicStringViewCompareTest, EqualToCString) {
    string_view view("Hello");
    EXPECT_TRUE(view.equal_to("Hello"));
    EXPECT_FALSE(view.equal_to("World"));
}

TEST_F(BasicStringViewCompareTest, LessThan) {
    string_view view1("abc");
    string_view view2("abd");
    EXPECT_TRUE(view1.less_than(view2));
    EXPECT_FALSE(view2.less_than(view1));
}

TEST_F(BasicStringViewFindTest, FindSubstring) {
    string_view view("Hello World Hello");
    EXPECT_EQ(view.find(string_view("World")), 6u);
}

TEST_F(BasicStringViewFindTest, FindSubstringWithOffset) {
    string_view view("Hello World Hello");
    EXPECT_EQ(view.find(string_view("Hello"), 5), 12u);
}

TEST_F(BasicStringViewFindTest, FindSubstringNotFound) {
    string_view view("Hello World");
    EXPECT_EQ(view.find(string_view("xyz")), string_view::npos);
}

TEST_F(BasicStringViewFindTest, FindEmptySubstring) {
    string_view view("Hello");
    EXPECT_EQ(view.find(string_view("")), 0u);
    EXPECT_EQ(view.find(string_view(""), 3), 3u);
}

TEST_F(BasicStringViewFindTest, FindChar) {
    string_view view("Hello World");
    EXPECT_EQ(view.find('W'), 6u);
    EXPECT_EQ(view.find('o'), 4u);
}

TEST_F(BasicStringViewFindTest, FindCharWithOffset) {
    string_view view("Hello World");
    EXPECT_EQ(view.find('o', 5), 7u);
}

TEST_F(BasicStringViewFindTest, FindCharNotFound) {
    string_view view("Hello");
    EXPECT_EQ(view.find('z'), string_view::npos);
}

TEST_F(BasicStringViewFindTest, FindCStringWithCount) {
    string_view view("Hello World");
    EXPECT_EQ(view.find("WorldX", 0, 5), 6u);
}

TEST_F(BasicStringViewFindTest, FindCString) {
    string_view view("Hello World");
    EXPECT_EQ(view.find("World"), 6u);
    EXPECT_EQ(view.find("Hello", 3), string_view::npos);
}

TEST_F(BasicStringViewRfindTest, RfindSubstring) {
    string_view view("Hello World Hello");
    EXPECT_EQ(view.rfind(string_view("Hello")), 12u);
}

TEST_F(BasicStringViewRfindTest, RfindSubstringWithOffset) {
    string_view view("Hello World Hello");
    EXPECT_EQ(view.rfind(string_view("Hello"), 10), 0u);
}

TEST_F(BasicStringViewRfindTest, RfindSubstringNotFound) {
    string_view view("Hello World");
    EXPECT_EQ(view.rfind(string_view("xyz")), string_view::npos);
}

TEST_F(BasicStringViewRfindTest, RfindEmptySubstring) {
    string_view view("Hello");
    EXPECT_EQ(view.rfind(string_view("")), 5u);
    EXPECT_EQ(view.rfind(string_view(""), 3), 3u);
}

TEST_F(BasicStringViewRfindTest, RfindChar) {
    string_view view("Hello World Hello");
    EXPECT_EQ(view.rfind('o'), 16u);
    EXPECT_EQ(view.rfind('H'), 12u);
}

TEST_F(BasicStringViewRfindTest, RfindCharWithOffset) {
    string_view view("Hello World Hello");
    EXPECT_EQ(view.rfind('o', 10), 7u);
}

TEST_F(BasicStringViewRfindTest, RfindCharNotFound) {
    string_view view("Hello");
    EXPECT_EQ(view.rfind('z'), string_view::npos);
}

TEST_F(BasicStringViewRfindTest, RfindCStringWithCount) {
    string_view view("Hello World Hello");
    EXPECT_EQ(view.rfind("HelloX", 16, 5), 12u);
}

TEST_F(BasicStringViewRfindTest, RfindCString) {
    string_view view("Hello World Hello");
    EXPECT_EQ(view.rfind("Hello"), 12u);
}

TEST_F(BasicStringViewFindFirstOfTest, FindFirstOfSubstring) {
    string_view view("Hello World");
    EXPECT_EQ(view.find_first_of(string_view("oe")), 1u);
}

TEST_F(BasicStringViewFindFirstOfTest, FindFirstOfWithOffset) {
    string_view view("Hello World");
    EXPECT_EQ(view.find_first_of(string_view("oe"), 3), 4u);
}

TEST_F(BasicStringViewFindFirstOfTest, FindFirstOfNotFound) {
    string_view view("Hello");
    EXPECT_EQ(view.find_first_of(string_view("xyz")), string_view::npos);
}

TEST_F(BasicStringViewFindFirstOfTest, FindFirstOfChar) {
    string_view view("Hello World");
    EXPECT_EQ(view.find_first_of('W'), 6u);
}

TEST_F(BasicStringViewFindFirstOfTest, FindFirstOfCStringWithCount) {
    string_view view("Hello World");
    EXPECT_EQ(view.find_first_of("oeX", 0, 2), 1u);
}

TEST_F(BasicStringViewFindFirstOfTest, FindFirstOfCString) {
    string_view view("Hello World");
    EXPECT_EQ(view.find_first_of("oe"), 1u);
}

TEST_F(BasicStringViewFindLastOfTest, FindLastOfSubstring) {
    string_view view("Hello World Hello");
    EXPECT_EQ(view.find_last_of(string_view("oe")), 16u);
}

TEST_F(BasicStringViewFindLastOfTest, FindLastOfWithOffset) {
    string_view view("Hello World Hello");
    EXPECT_EQ(view.find_last_of(string_view("oe"), 8), 7u);
}

TEST_F(BasicStringViewFindLastOfTest, FindLastOfNotFound) {
    string_view view("Hello");
    EXPECT_EQ(view.find_last_of(string_view("xyz")), string_view::npos);
}

TEST_F(BasicStringViewFindLastOfTest, FindLastOfChar) {
    string_view view("Hello World Hello");
    EXPECT_EQ(view.find_last_of('o'), 16u);
}

TEST_F(BasicStringViewFindLastOfTest, FindLastOfCStringWithCount) {
    string_view view("Hello World Hello");
    EXPECT_EQ(view.find_last_of("oeX", 10, 2), 7u);
}

TEST_F(BasicStringViewFindLastOfTest, FindLastOfCString) {
    string_view view("Hello World Hello");
    EXPECT_EQ(view.find_last_of("oe"), 16u);
}

TEST_F(BasicStringViewFindFirstNotOfTest, FindFirstNotOfSubstring) {
    string_view view("aaabc");
    EXPECT_EQ(view.find_first_not_of(string_view("a")), 3u);
}

TEST_F(BasicStringViewFindFirstNotOfTest, FindFirstNotOfWithOffset) {
    string_view view("aaabcaaa");
    EXPECT_EQ(view.find_first_not_of(string_view("a"), 4), 4u);
}

TEST_F(BasicStringViewFindFirstNotOfTest, FindFirstNotOfAllMatch) {
    string_view view("aaaa");
    EXPECT_EQ(view.find_first_not_of(string_view("a")), string_view::npos);
}

TEST_F(BasicStringViewFindFirstNotOfTest, FindFirstNotOfChar) {
    string_view view("aaaab");
    EXPECT_EQ(view.find_first_not_of('a'), 4u);
}

TEST_F(BasicStringViewFindFirstNotOfTest, FindFirstNotOfCStringWithCount) {
    string_view view("aaabc");
    EXPECT_EQ(view.find_first_not_of("aX", 0, 1), 3u);
}

TEST_F(BasicStringViewFindFirstNotOfTest, FindFirstNotOfCString) {
    string_view view("aaabc");
    EXPECT_EQ(view.find_first_not_of("a"), 3u);
}

TEST_F(BasicStringViewFindLastNotOfTest, FindLastNotOfSubstring) {
    string_view view("aaabcaa");
    EXPECT_EQ(view.find_last_not_of(string_view("a")), 4u);
}

TEST_F(BasicStringViewFindLastNotOfTest, FindLastNotOfWithOffset) {
    string_view view("aaabcaaa");
    EXPECT_EQ(view.find_last_not_of(string_view("a"), 4), 4u);
}

TEST_F(BasicStringViewFindLastNotOfTest, FindLastNotOfAllMatch) {
    string_view view("aaaa");
    EXPECT_EQ(view.find_last_not_of(string_view("a")), string_view::npos);
}

TEST_F(BasicStringViewFindLastNotOfTest, FindLastNotOfChar) {
    string_view view("baaaa");
    EXPECT_EQ(view.find_last_not_of('a'), 0u);
}

TEST_F(BasicStringViewFindLastNotOfTest, FindLastNotOfCStringWithCount) {
    string_view view("aaabcaa");
    EXPECT_EQ(view.find_last_not_of("aX", 6, 1), 4u);
}

TEST_F(BasicStringViewFindLastNotOfTest, FindLastNotOfCString) {
    string_view view("aaabcaa");
    EXPECT_EQ(view.find_last_not_of("a"), 4u);
}

TEST_F(BasicStringViewStartsEndsWithTest, StartsWithView) {
    string_view view("Hello World");
    EXPECT_TRUE(view.starts_with(string_view("Hello")));
    EXPECT_FALSE(view.starts_with(string_view("World")));
}

TEST_F(BasicStringViewStartsEndsWithTest, StartsWithChar) {
    string_view view("Hello World");
    EXPECT_TRUE(view.starts_with('H'));
    EXPECT_FALSE(view.starts_with('W'));
}

TEST_F(BasicStringViewStartsEndsWithTest, StartsWithCString) {
    string_view view("Hello World");
    EXPECT_TRUE(view.starts_with("Hello"));
    EXPECT_FALSE(view.starts_with("World"));
}

TEST_F(BasicStringViewStartsEndsWithTest, StartsWithEmpty) {
    string_view view("Hello");
    EXPECT_TRUE(view.starts_with(string_view("")));
}

TEST_F(BasicStringViewStartsEndsWithTest, StartsWithLonger) {
    string_view view("Hi");
    EXPECT_FALSE(view.starts_with(string_view("Hi!")));
}

TEST_F(BasicStringViewStartsEndsWithTest, StartsWithEmptyView) {
    string_view view{""};
    EXPECT_FALSE(view.starts_with('H'));
    EXPECT_TRUE(view.starts_with(string_view("")));
}

TEST_F(BasicStringViewStartsEndsWithTest, EndsWithView) {
    string_view view("Hello World");
    EXPECT_TRUE(view.ends_with(string_view("World")));
    EXPECT_FALSE(view.ends_with(string_view("Hello")));
}

TEST_F(BasicStringViewStartsEndsWithTest, EndsWithChar) {
    string_view view("Hello World");
    EXPECT_TRUE(view.ends_with('d'));
    EXPECT_FALSE(view.ends_with('o'));
}

TEST_F(BasicStringViewStartsEndsWithTest, EndsWithCString) {
    string_view view("Hello World");
    EXPECT_TRUE(view.ends_with("World"));
    EXPECT_FALSE(view.ends_with("Hello"));
}

TEST_F(BasicStringViewStartsEndsWithTest, EndsWithEmpty) {
    string_view view("Hello");
    EXPECT_TRUE(view.ends_with(string_view("")));
}

TEST_F(BasicStringViewStartsEndsWithTest, EndsWithLonger) {
    string_view view("Hi");
    EXPECT_FALSE(view.ends_with(string_view("Hi!")));
}

TEST_F(BasicStringViewContainsTest, ContainsView) {
    string_view view("Hello World");
    EXPECT_TRUE(view.contains(string_view("Hello")));
    EXPECT_TRUE(view.contains(string_view("World")));
    EXPECT_TRUE(view.contains(string_view("lo Wo")));
    EXPECT_FALSE(view.contains(string_view("xyz")));
}

TEST_F(BasicStringViewContainsTest, ContainsChar) {
    string_view view("Hello World");
    EXPECT_TRUE(view.contains('H'));
    EXPECT_TRUE(view.contains('d'));
    EXPECT_TRUE(view.contains(' '));
    EXPECT_FALSE(view.contains('z'));
}

TEST_F(BasicStringViewContainsTest, ContainsCString) {
    string_view view("Hello World");
    EXPECT_TRUE(view.contains("Hello"));
    EXPECT_TRUE(view.contains("World"));
    EXPECT_FALSE(view.contains("xyz"));
}

TEST_F(BasicStringViewContainsTest, ContainsEmpty) {
    string_view view("Hello");
    EXPECT_TRUE(view.contains(string_view("")));
}

TEST_F(BasicStringViewTrimTest, TrimLeft) {
    string_view view("  \t\nHello World");
    string_view trimmed = view.trim_left();
    EXPECT_EQ(trimmed[0], 'H');
    EXPECT_EQ(trimmed.size(), 11u);
}

TEST_F(BasicStringViewTrimTest, TrimRight) {
    string_view view("Hello World  \t\n");
    string_view trimmed = view.trim_right();
    EXPECT_EQ(trimmed[trimmed.size() - 1], 'd');
    EXPECT_EQ(trimmed.size(), 11u);
}

TEST_F(BasicStringViewTrimTest, Trim) {
    string_view view("  \tHello World  \n");
    string_view trimmed = view.trim();
    EXPECT_EQ(trimmed[0], 'H');
    EXPECT_EQ(trimmed[trimmed.size() - 1], 'd');
    EXPECT_EQ(trimmed.size(), 11u);
}

TEST_F(BasicStringViewTrimTest, TrimAllSpaces) {
    string_view view("     ");
    string_view trimmed = view.trim();
    EXPECT_TRUE(trimmed.empty());
    EXPECT_EQ(trimmed.size(), 0u);
}

TEST_F(BasicStringViewTrimTest, TrimEmpty) {
    string_view view;
    string_view trimmed = view.trim();
    EXPECT_TRUE(trimmed.empty());
}

TEST_F(BasicStringViewTrimTest, TrimLeftIf) {
    string_view view("xxHello Worldxx");
    string_view trimmed = view.trim_left_if([](char c) { return c == 'x'; });
    EXPECT_EQ(trimmed[0], 'H');
    EXPECT_EQ(trimmed.size(), 13u);
}

TEST_F(BasicStringViewTrimTest, TrimRightIf) {
    string_view view("xxHello Worldxx");
    string_view trimmed = view.trim_right_if([](char c) { return c == 'x'; });
    EXPECT_EQ(trimmed[trimmed.size() - 1], 'd');
    EXPECT_EQ(trimmed.size(), 13u);
}

TEST_F(BasicStringViewTrimTest, TrimIf) {
    string_view view("xxHello Worldxx");
    string_view trimmed = view.trim_if([](char c) { return c == 'x'; });
    EXPECT_EQ(trimmed[0], 'H');
    EXPECT_EQ(trimmed[trimmed.size() - 1], 'd');
    EXPECT_EQ(trimmed.size(), 11u);
}

TEST_F(BasicStringViewTrimTest, TrimIfNoMatch) {
    string_view view("Hello World");
    string_view trimmed = view.trim_if([](char c) { return c == 'x'; });
    EXPECT_EQ(trimmed.size(), 11u);
}

TEST_F(BasicStringViewCountTest, CountChar) {
    string_view view("Hello World Hello");
    EXPECT_EQ(view.count('l'), 5u);
    EXPECT_EQ(view.count('o'), 3u);
    EXPECT_EQ(view.count('z'), 0u);
}

TEST_F(BasicStringViewCountTest, CountCharWithPosition) {
    string_view view("Hello World Hello");
    EXPECT_EQ(view.count('l', 3), 4u);
    EXPECT_EQ(view.count('H', 1), 1u);
}

TEST_F(BasicStringViewCountTest, CountInEmptyView) {
    string_view view;
    EXPECT_EQ(view.count('a'), 0u);
}

TEST_F(BasicStringViewSwapTest, Swap) {
    string_view view1("Hello");
    string_view view2("World");
    view1.swap(view2);
    EXPECT_EQ(view1[0], 'W');
    EXPECT_EQ(view1.size(), 5u);
    EXPECT_EQ(view2[0], 'H');
    EXPECT_EQ(view2.size(), 5u);
}

TEST_F(BasicStringViewSwapTest, SwapWithEmpty) {
    string_view view1("Hello");
    string_view view2;
    view1.swap(view2);
    EXPECT_TRUE(view1.empty());
    EXPECT_EQ(view2.size(), 5u);
    EXPECT_EQ(view2[0], 'H');
}

TEST_F(BasicStringViewHashTest, Hash) {
    string_view view1("Hello");
    string_view view2("Hello");
    string_view view3("World");
    EXPECT_EQ(view1.to_hash(), view2.to_hash());
    EXPECT_NE(view1.to_hash(), view3.to_hash());
}

TEST_F(BasicStringViewHashTest, HashEmpty) {
    string_view view1;
    string_view view2;
    EXPECT_EQ(view1.to_hash(), view2.to_hash());
}

class BasicStringConstructionTest : public ::testing::Test {};
class BasicStringIteratorsTest : public ::testing::Test {};
class BasicStringCapacityTest : public ::testing::Test {};
class BasicStringElementAccessTest : public ::testing::Test {};
class BasicStringModifiersTest : public ::testing::Test {};
class BasicStringAppendTest : public ::testing::Test {};
class BasicStringAssignTest : public ::testing::Test {};
class BasicStringInsertTest : public ::testing::Test {};
class BasicStringEraseTest : public ::testing::Test {};
class BasicStringReplaceTest : public ::testing::Test {};
class BasicStringFindTest : public ::testing::Test {};
class BasicStringRfindTest : public ::testing::Test {};
class BasicStringFindFirstOfTest : public ::testing::Test {};
class BasicStringFindLastOfTest : public ::testing::Test {};
class BasicStringFindFirstNotOfTest : public ::testing::Test {};
class BasicStringFindLastNotOfTest : public ::testing::Test {};
class BasicStringCompareTest : public ::testing::Test {};
class BasicStringStartsEndsWithTest : public ::testing::Test {};
class BasicStringContainsTest : public ::testing::Test {};
class BasicStringTrimTest : public ::testing::Test {};
class BasicStringOperatorsTest : public ::testing::Test {};
class BasicStringOtherTest : public ::testing::Test {};

TEST_F(BasicStringConstructionTest, DefaultConstructor) {
    string s;
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.size(), 0u);
}

TEST_F(BasicStringConstructionTest, FromCString) {
    string s("Hello");
    EXPECT_EQ(s.size(), 5u);
    EXPECT_EQ(s[0], 'H');
    EXPECT_EQ(s[4], 'o');
}

TEST_F(BasicStringConstructionTest, FromPointerAndSize) {
    string s("Hello World", 5);
    EXPECT_EQ(s.size(), 5u);
    EXPECT_EQ(s, "Hello");
}

TEST_F(BasicStringConstructionTest, FromSizeAndChar) {
    string s(5, 'X');
    EXPECT_EQ(s.size(), 5u);
    EXPECT_EQ(s, "XXXXX");
}

TEST_F(BasicStringConstructionTest, FromSizeAndInt) {
    string s(3, static_cast<int32_t>('A'));
    EXPECT_EQ(s, "AAA");
}

TEST_F(BasicStringConstructionTest, CopyConstructor) {
    string s1("Hello");
    string s2(s1);
    EXPECT_EQ(s2, "Hello");
}

TEST_F(BasicStringConstructionTest, CopyAssignment) {
    string s1("Hello");
    string s2;
    s2 = s1;
    EXPECT_EQ(s2, "Hello");
}

TEST_F(BasicStringConstructionTest, MoveConstructor) {
    string s1("Hello");
    string s2(move(s1));
    EXPECT_EQ(s2, "Hello");
    EXPECT_TRUE(s1.empty());
}

TEST_F(BasicStringConstructionTest, MoveAssignment) {
    string s1("Hello");
    string s2;
    s2 = move(s1);
    EXPECT_EQ(s2, "Hello");
    EXPECT_TRUE(s1.empty());
}

TEST_F(BasicStringConstructionTest, FromStringView) {
    string_view view("Hello World");
    string s(view);
    EXPECT_EQ(s, "Hello World");
}

TEST_F(BasicStringConstructionTest, FromStringViewWithSize) {
    string_view view("Hello World");
    string s(view, 5);
    EXPECT_EQ(s, "Hello");
}

TEST_F(BasicStringConstructionTest, FromSubstr) {
    string s1("Hello World");
    string s2(s1, 6);
    EXPECT_EQ(s2, "World");
}

TEST_F(BasicStringConstructionTest, FromSubstrWithCount) {
    string s1("Hello World");
    string s2(s1, 0, 5);
    EXPECT_EQ(s2, "Hello");
}

TEST_F(BasicStringConstructionTest, FromIteratorRange) {
    string tmp = "Hello";
    string s(tmp.begin(), tmp.end());
    EXPECT_EQ(s, "Hello");
}

TEST_F(BasicStringConstructionTest, FromInitializerList) {
    string s({'H', 'e', 'l', 'l', 'o'});
    EXPECT_EQ(s, "Hello");
}

TEST_F(BasicStringConstructionTest, InitializerListAssignment) {
    string s;
    s = {'W', 'o', 'r', 'l', 'd'};
    EXPECT_EQ(s, "World");
}

TEST_F(BasicStringConstructionTest, SelfCopyAssignment) {
    string s("Hello");
    s = s;
    EXPECT_EQ(s, "Hello");
}

TEST_F(BasicStringConstructionTest, SelfMoveAssignment) {
    string s("Hello");
    s = move(s);
    EXPECT_EQ(s, "Hello");
}

TEST_F(BasicStringIteratorsTest, BeginEnd) {
    string s("Hello");
    auto it = s.begin();
    EXPECT_EQ(*it, 'H');
    ++it;
    EXPECT_EQ(*it, 'e');
    it = s.end();
    --it;
    EXPECT_EQ(*it, 'o');
}

TEST_F(BasicStringIteratorsTest, ConstBeginEnd) {
    const string s("Hello");
    auto it = s.begin();
    EXPECT_EQ(*it, 'H');
    it = s.end();
    --it;
    EXPECT_EQ(*it, 'o');
}

TEST_F(BasicStringIteratorsTest, CBeginCEnd) {
    const string s("Hello");
    auto it = s.cbegin();
    EXPECT_EQ(*it, 'H');
    it = s.cend();
    --it;
    EXPECT_EQ(*it, 'o');
}

TEST_F(BasicStringIteratorsTest, ReverseIterators) {
    string s("ABC");
    string result;
    for (auto rit = s.rbegin(); rit != s.rend(); ++rit) {
        result += *rit;
    }
    EXPECT_EQ(result, "CBA");
}

TEST_F(BasicStringIteratorsTest, ConstReverseIterators) {
    const string s("XYZ");
    string result;
    for (auto rit = s.rbegin(); rit != s.rend(); ++rit) {
        result += *rit;
    }
    EXPECT_EQ(result, "ZYX");
}

TEST_F(BasicStringIteratorsTest, CRbeginCRend) {
    const string s("123");
    string result;
    for (auto rit = s.crbegin(); rit != s.crend(); ++rit) {
        result += *rit;
    }
    EXPECT_EQ(result, "321");
}

TEST_F(BasicStringIteratorsTest, IteratorLoop) {
    string s("ABC");
    string result;
    for (auto c: s) {
        result += c;
    }
    EXPECT_EQ(result, "ABC");
}

TEST_F(BasicStringCapacityTest, SizeAndLength) {
    string s("Hello");
    EXPECT_EQ(s.size(), 5u);
    EXPECT_EQ(s.length(), 5u);
}

TEST_F(BasicStringCapacityTest, Empty) {
    string s;
    EXPECT_TRUE(s.empty());
    string s2("Hi");
    EXPECT_FALSE(s2.empty());
}

TEST_F(BasicStringCapacityTest, MaxSize) {
    string s;
    EXPECT_GT(s.max_size(), 0u);
}

TEST_F(BasicStringCapacityTest, Capacity) {
    string s;
    EXPECT_GE(s.capacity(), s.size());
    s = "Hello World This Is A Longer String";
    EXPECT_GE(s.capacity(), s.size());
}

TEST_F(BasicStringCapacityTest, Reserve) {
    string s;
    s.reserve(100);
    EXPECT_GE(s.capacity(), 100u);
}

TEST_F(BasicStringCapacityTest, ReserveSmaller) {
    string s("Hello World");
    size_t old_cap = s.capacity();
    s.reserve(5);
    EXPECT_EQ(s.capacity(), old_cap);
}

TEST_F(BasicStringCapacityTest, Clear) {
    string s("Hello");
    s.clear();
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.size(), 0u);
}

TEST_F(BasicStringCapacityTest, ShrinkToFit) {
    string s("Hello");
    s.reserve(1000);
    EXPECT_GE(s.capacity(), 1000u);
    s.shrink_to_fit();
    EXPECT_LE(s.capacity(), 1000u);
}

TEST_F(BasicStringElementAccessTest, SubscriptOperator) {
    string s("Hello");
    EXPECT_EQ(s[0], 'H');
    EXPECT_EQ(s[4], 'o');
    s[1] = 'a';
    EXPECT_EQ(s, "Hallo");
}

TEST_F(BasicStringElementAccessTest, ConstSubscriptOperator) {
    const string s("Hello");
    EXPECT_EQ(s[0], 'H');
    EXPECT_EQ(s[4], 'o');
}

TEST_F(BasicStringElementAccessTest, At) {
    string s("Hello");
    EXPECT_EQ(s.at(0), 'H');
    EXPECT_EQ(s.at(4), 'o');
}

TEST_F(BasicStringElementAccessTest, ConstAt) {
    const string s("Hello");
    EXPECT_EQ(s.at(0), 'H');
    EXPECT_EQ(s.at(4), 'o');
}

TEST_F(BasicStringElementAccessTest, Front) {
    string s("Hello");
    EXPECT_EQ(s.front(), 'H');
    s.front() = 'X';
    EXPECT_EQ(s, "Xello");
}

TEST_F(BasicStringElementAccessTest, ConstFront) {
    const string s("Hello");
    EXPECT_EQ(s.front(), 'H');
}

TEST_F(BasicStringElementAccessTest, Back) {
    string s("Hello");
    EXPECT_EQ(s.back(), 'o');
    s.back() = 'X';
    EXPECT_EQ(s, "HellX");
}

TEST_F(BasicStringElementAccessTest, ConstBack) {
    const string s("Hello");
    EXPECT_EQ(s.back(), 'o');
}

TEST_F(BasicStringElementAccessTest, Data) {
    string s("Hello");
    EXPECT_EQ(s.data()[0], 'H');
    EXPECT_EQ(s.data()[4], 'o');
}

TEST_F(BasicStringElementAccessTest, ConstData) {
    const string s("Hello");
    EXPECT_EQ(s.data()[0], 'H');
    EXPECT_EQ(s.data()[4], 'o');
}

TEST_F(BasicStringModifiersTest, PushBack) {
    string s("Hello");
    s.push_back('!');
    EXPECT_EQ(s, "Hello!");
}

TEST_F(BasicStringModifiersTest, PopBack) {
    string s("Hello!");
    s.pop_back();
    EXPECT_EQ(s, "Hello");
}

TEST_F(BasicStringModifiersTest, ResizeLarger) {
    string s("Hi");
    s.resize(5, 'x');
    EXPECT_EQ(s, "Hixxx");
}

TEST_F(BasicStringModifiersTest, ResizeSmaller) {
    string s("Hello");
    s.resize(3);
    EXPECT_EQ(s, "Hel");
}

TEST_F(BasicStringModifiersTest, ResizeDefault) {
    string s("Hello");
    s.resize(8);
    EXPECT_EQ(s.size(), 8u);
    EXPECT_EQ(s[0], 'H');
    EXPECT_EQ(s[7], '\0');
}

TEST_F(BasicStringModifiersTest, Reverse) {
    string s("Hello");
    s.reverse();
    EXPECT_EQ(s, "olleH");
}

TEST_F(BasicStringModifiersTest, ReverseEmpty) {
    string s;
    s.reverse();
    EXPECT_TRUE(s.empty());
}

TEST_F(BasicStringModifiersTest, ReverseSingle) {
    string s("A");
    s.reverse();
    EXPECT_EQ(s, "A");
}

TEST_F(BasicStringAppendTest, AppendString) {
    string s1("Hello");
    string s2(" World");
    s1.append(s2);
    EXPECT_EQ(s1, "Hello World");
}

TEST_F(BasicStringAppendTest, AppendSubstr) {
    string s1("Hello");
    string s2(" World!");
    s1.append(s2, 0, 6);
    EXPECT_EQ(s1, "Hello World");
}

TEST_F(BasicStringAppendTest, AppendSubstrWithPosition) {
    string s1("Hello");
    string s2("xx World");
    s1.append(s2, 2);
    EXPECT_EQ(s1, "Hello World");
}

TEST_F(BasicStringAppendTest, AppendMoveString) {
    string s1("Hello");
    string s2(" World");
    s1.append(move(s2));
    EXPECT_EQ(s1, "Hello World");
    EXPECT_TRUE(s2.empty());
}

TEST_F(BasicStringAppendTest, AppendMoveSubstr) {
    string s1("Hello");
    string s2(" World!");
    s1.append(move(s2), 0, 6);
    EXPECT_EQ(s1, "Hello World");
    EXPECT_TRUE(s2.empty());
}

TEST_F(BasicStringAppendTest, AppendCString) {
    string s("Hello");
    s.append(" World");
    EXPECT_EQ(s, "Hello World");
}

TEST_F(BasicStringAppendTest, AppendCStringWithCount) {
    string s("Hello");
    s.append(" World!", 6);
    EXPECT_EQ(s, "Hello World");
}

TEST_F(BasicStringAppendTest, AppendChar) {
    string s("Hello");
    s.append('!');
    EXPECT_EQ(s, "Hello!");
}

TEST_F(BasicStringAppendTest, AppendMultipleChars) {
    string s("Hello");
    s.append(3, '!');
    EXPECT_EQ(s, "Hello!!!");
}

TEST_F(BasicStringAppendTest, AppendStringView) {
    string s("Hello");
    string_view view(" World");
    s.append(view);
    EXPECT_EQ(s, "Hello World");
}

TEST_F(BasicStringAppendTest, AppendStringViewWithCount) {
    string s("Hello");
    string_view view(" World!");
    s.append(view, 6);
    EXPECT_EQ(s, "Hello World");
}

TEST_F(BasicStringAppendTest, AppendIteratorRange) {
    string s("Hello");
    string tmp = " World";
    s.append(tmp.begin(), tmp.end());
    EXPECT_EQ(s, "Hello World");
}

TEST_F(BasicStringAppendTest, AppendInitializerList) {
    string s("Hello");
    s.append({' ', 'W', 'o', 'r', 'l', 'd'});
    EXPECT_EQ(s, "Hello World");
}

TEST_F(BasicStringAppendTest, OperatorPlusEquals) {
    string s("Hello");
    s += " World";
    s += '!';
    string s2(" More");
    s += s2;
    EXPECT_EQ(s, "Hello World! More");
}

TEST_F(BasicStringAssignTest, AssignString) {
    string s;
    string s2("Hello");
    s.assign(s2);
    EXPECT_EQ(s, "Hello");
}

TEST_F(BasicStringAssignTest, AssignMoveString) {
    string s;
    string s2("Hello");
    s.assign(move(s2));
    EXPECT_EQ(s, "Hello");
}

TEST_F(BasicStringAssignTest, AssignCString) {
    string s;
    s.assign("Hello");
    EXPECT_EQ(s, "Hello");
}

TEST_F(BasicStringAssignTest, AssignCStringWithCount) {
    string s;
    s.assign("Hello World", 5);
    EXPECT_EQ(s, "Hello");
}

TEST_F(BasicStringAssignTest, AssignMultipleChars) {
    string s;
    s.assign(5, 'X');
    EXPECT_EQ(s, "XXXXX");
}

TEST_F(BasicStringAssignTest, AssignIteratorRange) {
    string s;
    string tmp = "Hello";
    s.assign(tmp.begin(), tmp.end());
    EXPECT_EQ(s, "Hello");
}

TEST_F(BasicStringAssignTest, AssignInitializerList) {
    string s;
    s.assign({'H', 'e', 'l', 'l', 'o'});
    EXPECT_EQ(s, "Hello");
}

TEST_F(BasicStringAssignTest, AssignStringView) {
    string s;
    string_view view("Hello");
    s.assign(view);
    EXPECT_EQ(s, "Hello");
}

TEST_F(BasicStringInsertTest, InsertChar) {
    string s("Hello");
    auto it = s.insert(s.begin() + 2, 'X');
    EXPECT_EQ(s, "HeXllo");
    EXPECT_EQ(*it, 'X');
}

TEST_F(BasicStringInsertTest, InsertMultipleChars) {
    string s("Hello");
    s.insert(2, 3, 'X');
    EXPECT_EQ(s, "HeXXXllo");
}

TEST_F(BasicStringInsertTest, InsertCString) {
    string s("Hello");
    s.insert(2, 2, 'X');
    EXPECT_EQ(s, "HeXXllo");
}

TEST_F(BasicStringInsertTest, InsertIteratorRange) {
    string s("Hello");
    string tmp = "XX";
    s.insert(s.begin() + 2, tmp.begin(), tmp.end());
    EXPECT_EQ(s, "HeXXllo");
}

TEST_F(BasicStringInsertTest, InsertAtEnd) {
    string s("Hello");
    s.insert(s.size(), 2, '!');
    EXPECT_EQ(s, "Hello!!");
}

TEST_F(BasicStringInsertTest, InsertAtBegin) {
    string s("Hello");
    s.insert(0, 2, '!');
    EXPECT_EQ(s, "!!Hello");
}

TEST_F(BasicStringEraseTest, ErasePosition) {
    string s("Hello");
    auto it = s.erase(s.begin() + 1);
    EXPECT_EQ(s, "Hllo");
    EXPECT_EQ(*it, 'l');
}

TEST_F(BasicStringEraseTest, ErasePositionLength) {
    string s("Hello World");
    s.erase(5);
    EXPECT_EQ(s, "Hello");
}

TEST_F(BasicStringEraseTest, EraseRange) {
    string s("Hello World");
    s.erase(0, 6);
    EXPECT_EQ(s, "World");
}

TEST_F(BasicStringEraseTest, EraseIteratorRange) {
    string s("Hello World");
    auto it = s.erase(s.begin(), s.begin() + 6);
    EXPECT_EQ(s, "World");
    EXPECT_EQ(*it, 'W');
}

TEST_F(BasicStringEraseTest, EraseToEnd) {
    string s("Hello");
    s.erase(s.begin() + 2, s.end());
    EXPECT_EQ(s, "He");
}

TEST_F(BasicStringReplaceTest, ReplaceWithString) {
    string s("Hello World");
    s.replace(6, 5, string("Universe"));
    EXPECT_EQ(s, "Hello Universe");
}

TEST_F(BasicStringReplaceTest, ReplaceIteratorRangeWithString) {
    string s("Hello World");
    s.replace(s.begin() + 6, s.end(), string("Universe"));
    EXPECT_EQ(s, "Hello Universe");
}

TEST_F(BasicStringReplaceTest, ReplaceWithCString) {
    string s("Hello World");
    s.replace(6, 5, "Earth");
    EXPECT_EQ(s, "Hello Earth");
}

TEST_F(BasicStringReplaceTest, ReplaceWithCStringCount) {
    string s("Hello World");
    s.replace(6, 5, "UniverseX", 8);
    EXPECT_EQ(s, "Hello Universe");
}

TEST_F(BasicStringReplaceTest, ReplaceWithMultipleChars) {
    string s("Hello World");
    s.replace(6, 5, 3, 'X');
    EXPECT_EQ(s, "Hello XXX");
}

TEST_F(BasicStringReplaceTest, ReplaceWithSubstr) {
    string s("Hello World");
    string s2("ABC Universe XYZ");
    s.replace(6, 5, s2, 4, 8);
    EXPECT_EQ(s, "Hello Universe");
}

TEST_F(BasicStringReplaceTest, ReplaceWithIteratorRange) {
    string s("Hello World");
    string tmp = "Universe";
    s.replace(s.begin() + 6, s.end(), tmp.begin(), tmp.end());
    EXPECT_EQ(s, "Hello Universe");
}

TEST_F(BasicStringFindTest, FindString) {
    string s("Hello World Hello");
    EXPECT_EQ(s.find(string("World")), 6u);
    EXPECT_EQ(s.find(string("Hello")), 0u);
    EXPECT_EQ(s.find(string("xyz")), string::npos);
}

TEST_F(BasicStringFindTest, FindStringWithOffset) {
    string s("Hello World Hello");
    EXPECT_EQ(s.find(string("Hello"), 5), 12u);
}

TEST_F(BasicStringFindTest, FindChar) {
    string s("Hello World");
    EXPECT_EQ(s.find('W'), 6u);
    EXPECT_EQ(s.find('o'), 4u);
    EXPECT_EQ(s.find('z'), string::npos);
}

TEST_F(BasicStringFindTest, FindCharWithOffset) {
    string s("Hello World");
    EXPECT_EQ(s.find('o', 5), 7u);
}

TEST_F(BasicStringFindTest, FindCString) {
    string s("Hello World");
    EXPECT_EQ(s.find("World"), 6u);
    EXPECT_EQ(s.find("Hello", 3), string::npos);
}

TEST_F(BasicStringFindTest, FindCStringWithCount) {
    string s("Hello World");
    EXPECT_EQ(s.find("WorldX", 0, 5), 6u);
}

TEST_F(BasicStringFindTest, FindStringView) {
    string s("Hello World");
    EXPECT_EQ(s.find(string_view("World")), 6u);
    EXPECT_EQ(s.find(string_view("Hello"), 1), string::npos);
}

TEST_F(BasicStringRfindTest, RfindString) {
    string s("Hello World Hello");
    EXPECT_EQ(s.rfind(string("Hello")), 12u);
    EXPECT_EQ(s.rfind(string("World")), 6u);
}

TEST_F(BasicStringRfindTest, RfindChar) {
    string s("Hello World Hello");
    EXPECT_EQ(s.rfind('o'), 16u);
    EXPECT_EQ(s.rfind('H'), 12u);
}

TEST_F(BasicStringRfindTest, RfindCString) {
    string s("Hello World Hello");
    EXPECT_EQ(s.rfind("Hello"), 12u);
}

TEST_F(BasicStringRfindTest, RfindStringView) {
    string s("Hello World Hello");
    EXPECT_EQ(s.rfind(string_view("Hello")), 12u);
}

TEST_F(BasicStringFindFirstOfTest, FindFirstOfString) {
    string s("Hello World");
    EXPECT_EQ(s.find_first_of("oe"_s), 1u);
}

TEST_F(BasicStringFindFirstOfTest, FindFirstOfChar) {
    string s("Hello World");
    EXPECT_EQ(s.find_first_of('W'), 6u);
}

TEST_F(BasicStringFindFirstOfTest, FindFirstOfCString) {
    string s("Hello World");
    EXPECT_EQ(s.find_first_of("oe"), 1u);
}

TEST_F(BasicStringFindFirstOfTest, FindFirstOfStringView) {
    string s("Hello World");
    EXPECT_EQ(s.find_first_of("xyz"_sv), string::npos);
}

TEST_F(BasicStringFindLastOfTest, FindLastOfString) {
    string s("Hello World Hello");
    EXPECT_EQ(s.find_last_of(string("oe")), 16u);
}

TEST_F(BasicStringFindLastOfTest, FindLastOfChar) {
    string s("Hello World Hello");
    EXPECT_EQ(s.find_last_of('o'), 16u);
}

TEST_F(BasicStringFindLastOfTest, FindLastOfCString) {
    string s("Hello World Hello");
    EXPECT_EQ(s.find_last_of("le"), 15u);
}

TEST_F(BasicStringFindFirstNotOfTest, FindFirstNotOfString) {
    string s("aaabc");
    EXPECT_EQ(s.find_first_not_of(string("a")), 3u);
}

TEST_F(BasicStringFindFirstNotOfTest, FindFirstNotOfChar) {
    string s("aaaab");
    EXPECT_EQ(s.find_first_not_of('a'), 4u);
}

TEST_F(BasicStringFindFirstNotOfTest, FindFirstNotOfCString) {
    string s("aaabc");
    EXPECT_EQ(s.find_first_not_of("a"), 3u);
}

TEST_F(BasicStringFindLastNotOfTest, FindLastNotOfString) {
    string s("aaabcaa");
    EXPECT_EQ(s.find_last_not_of(string("a")), 4u);
}

TEST_F(BasicStringFindLastNotOfTest, FindLastNotOfChar) {
    string s("baaaa");
    EXPECT_EQ(s.find_last_not_of('a'), 0u);
}

TEST_F(BasicStringFindLastNotOfTest, FindLastNotOfCString) {
    string s("aaabcaa");
    EXPECT_EQ(s.find_last_not_of("a"), 4u);
}

TEST_F(BasicStringCompareTest, CompareString) {
    string s1("abc");
    string s2("abc");
    EXPECT_EQ(s1.compare(s2), 0);
    string s3("abd");
    EXPECT_LT(s1.compare(s3), 0);
    EXPECT_GT(s3.compare(s1), 0);
}

TEST_F(BasicStringCompareTest, CompareSubstr) {
    string s1("Hello World");
    string s2("World");
    EXPECT_EQ(s1.compare(6, 5, s2), 0);
}

TEST_F(BasicStringCompareTest, CompareSubstrWithSubstr) {
    string s1("Hello World");
    string s2("Hello World");
    EXPECT_EQ(s1.compare(0, 5, s2, 0, 5), 0);
}

TEST_F(BasicStringCompareTest, CompareCString) {
    string s("Hello");
    EXPECT_EQ(s.compare("Hello"), 0);
    EXPECT_LT(s.compare("Hellp"), 0);
}

TEST_F(BasicStringCompareTest, CompareSubstrCString) {
    string s("Hello World");
    EXPECT_EQ(s.compare(6, 5, "World"), 0);
}

TEST_F(BasicStringCompareTest, CompareSubstrCStringCount) {
    string s("Hello");
    EXPECT_EQ(s.compare(0, 3, "HelXXX", 3), 0);
}

TEST_F(BasicStringCompareTest, CompareStringView) {
    string s("Hello");
    EXPECT_EQ(s.compare(string_view("Hello")), 0);
    EXPECT_LT(s.compare(string_view("Hellp")), 0);
}

TEST_F(BasicStringCompareTest, CompareIgnoreCase) {
    string s("Hello");
    EXPECT_EQ(s.compare_ignore_case(string("hello")), 0);
    EXPECT_EQ(s.compare_ignore_case(string("HELLO")), 0);
}

TEST_F(BasicStringCompareTest, CompareIgnoreCaseCString) {
    string s("Hello");
    EXPECT_EQ(s.compare_ignore_case("hello"), 0);
}

TEST_F(BasicStringCompareTest, CompareIgnoreCaseView) {
    string s("Hello");
    EXPECT_EQ(s.compare_ignore_case(string_view("HELLO")), 0);
}

TEST_F(BasicStringStartsEndsWithTest, StartsWithString) {
    string s("Hello World");
    EXPECT_TRUE(s.starts_with(string("Hello")));
    EXPECT_FALSE(s.starts_with(string("World")));
}

TEST_F(BasicStringStartsEndsWithTest, StartsWithChar) {
    string s("Hello World");
    EXPECT_TRUE(s.starts_with('H'));
    EXPECT_FALSE(s.starts_with('W'));
}

TEST_F(BasicStringStartsEndsWithTest, StartsWithCString) {
    string s("Hello World");
    EXPECT_TRUE(s.starts_with("Hello"));
    EXPECT_FALSE(s.starts_with("World"));
}

TEST_F(BasicStringStartsEndsWithTest, StartsWithView) {
    string s("Hello World");
    EXPECT_TRUE(s.starts_with(string_view("Hello")));
    EXPECT_FALSE(s.starts_with(string_view("World")));
}

TEST_F(BasicStringStartsEndsWithTest, EndsWithString) {
    string s("Hello World");
    EXPECT_TRUE(s.ends_with(string("World")));
    EXPECT_FALSE(s.ends_with(string("Hello")));
}

TEST_F(BasicStringStartsEndsWithTest, EndsWithChar) {
    string s("Hello World");
    EXPECT_TRUE(s.ends_with('d'));
    EXPECT_FALSE(s.ends_with('o'));
}

TEST_F(BasicStringStartsEndsWithTest, EndsWithCString) {
    string s("Hello World");
    EXPECT_TRUE(s.ends_with("World"));
    EXPECT_FALSE(s.ends_with("Hello"));
}

TEST_F(BasicStringStartsEndsWithTest, EndsWithView) {
    string s("Hello World");
    EXPECT_TRUE(s.ends_with(string_view("World")));
    EXPECT_FALSE(s.ends_with(string_view("Hello")));
}

TEST_F(BasicStringContainsTest, ContainsString) {
    string s("Hello World");
    EXPECT_TRUE(s.contains(string("Hello")));
    EXPECT_TRUE(s.contains(string("World")));
    EXPECT_FALSE(s.contains(string("xyz")));
}

TEST_F(BasicStringContainsTest, ContainsChar) {
    string s("Hello World");
    EXPECT_TRUE(s.contains('H'));
    EXPECT_TRUE(s.contains('d'));
    EXPECT_FALSE(s.contains('z'));
}

TEST_F(BasicStringContainsTest, ContainsCString) {
    string s("Hello World");
    EXPECT_TRUE(s.contains("World"));
    EXPECT_FALSE(s.contains("xyz"));
}

TEST_F(BasicStringContainsTest, ContainsView) {
    string s("Hello World");
    EXPECT_TRUE(s.contains(string_view("lo Wo")));
    EXPECT_FALSE(s.contains(string_view("xyz")));
}

TEST_F(BasicStringTrimTest, TrimLeft) {
    string s("  Hello World");
    s.trim_left();
    EXPECT_EQ(s, "Hello World");
}

TEST_F(BasicStringTrimTest, TrimRight) {
    string s("Hello World  ");
    s.trim_right();
    EXPECT_EQ(s, "Hello World");
}

TEST_F(BasicStringTrimTest, Trim) {
    string s("  Hello World  ");
    s.trim();
    EXPECT_EQ(s, "Hello World");
}

TEST_F(BasicStringTrimTest, TrimAllSpaces) {
    string s("   ");
    s.trim();
    EXPECT_TRUE(s.empty());
}

TEST_F(BasicStringTrimTest, TrimIf) {
    string s("xxHello Worldxx");
    s.trim_if([](char c) { return c == 'x'; });
    EXPECT_EQ(s, "Hello World");
}

TEST_F(BasicStringOperatorsTest, OperatorEqualsString) {
    string s1("Hello");
    string s2("Hello");
    string s3("World");
    EXPECT_TRUE(s1 == s2);
    EXPECT_FALSE(s1 == s3);
}

TEST_F(BasicStringOperatorsTest, OperatorEqualsCString) {
    string s("Hello");
    EXPECT_TRUE(s == "Hello");
    EXPECT_FALSE(s == "World");
    EXPECT_TRUE("Hello" == s);
    EXPECT_FALSE("World" == s);
}

TEST_F(BasicStringOperatorsTest, OperatorEqualsView) {
    string s("Hello");
    EXPECT_TRUE(s == string_view("Hello"));
    EXPECT_FALSE(s == string_view("World"));
    EXPECT_TRUE(string_view("Hello") == s);
    EXPECT_FALSE(string_view("World") == s);
}

TEST_F(BasicStringOperatorsTest, OperatorNotEquals) {
    string s("Hello");
    EXPECT_TRUE(s != "World");
    EXPECT_FALSE(s != "Hello");
    EXPECT_TRUE("World" != s);
    EXPECT_FALSE("Hello" != s);
}

TEST_F(BasicStringOperatorsTest, OperatorLess) {
    string s1("abc");
    string s2("abd");
    EXPECT_TRUE(s1 < s2);
    EXPECT_FALSE(s2 < s1);
    EXPECT_TRUE(s1 < "abd");
    EXPECT_FALSE(s2 < "abc");
}

TEST_F(BasicStringOperatorsTest, OperatorGreater) {
    string s1("abd");
    string s2("abc");
    EXPECT_TRUE(s1 > s2);
    EXPECT_FALSE(s2 > s1);
}

TEST_F(BasicStringOperatorsTest, OperatorLessEqual) {
    string s1("abc");
    string s2("abc");
    EXPECT_TRUE(s1 <= s2);
    string s3("abd");
    EXPECT_TRUE(s1 <= s3);
    EXPECT_FALSE(s3 <= s1);
}

TEST_F(BasicStringOperatorsTest, OperatorGreaterEqual) {
    string s1("abd");
    string s2("abc");
    EXPECT_TRUE(s1 >= s2);
    string s3("abd");
    EXPECT_TRUE(s1 >= s3);
    EXPECT_FALSE(s2 >= s1);
}

TEST_F(BasicStringOperatorsTest, OperatorPlus) {
    string s1("Hello");
    string s2(" World");
    string result = s1 + s2;
    EXPECT_EQ(result, "Hello World");
}

TEST_F(BasicStringOperatorsTest, OperatorPlusCString) {
    string s(" World");
    string result = "Hello" + s;
    EXPECT_EQ(result, "Hello World");
    string result2 = s + "Hello";
    EXPECT_EQ(result2, " WorldHello");
}

TEST_F(BasicStringOperatorsTest, OperatorPlusChar) {
    string s("ello");
    string result = 'H' + s;
    EXPECT_EQ(result, "Hello");
    string result2 = s + '!';
    EXPECT_EQ(result2, "ello!");
}

TEST_F(BasicStringOperatorsTest, OperatorPlusMove) {
    string s1("Hello");
    string s2(" World");
    string result = move(s1) + s2;
    EXPECT_EQ(result, "Hello World");
}

TEST_F(BasicStringOtherTest, Substr) {
    string s("Hello World");
    EXPECT_EQ(s.substr(0, 5), "Hello");
    EXPECT_EQ(s.substr(6), "World");
}

TEST_F(BasicStringOtherTest, Head) {
    string s("Hello World");
    EXPECT_EQ(s.head(5), "Hello");
}

TEST_F(BasicStringOtherTest, Tail) {
    string s("Hello World");
    EXPECT_EQ(s.tail(6), "World");
}

TEST_F(BasicStringOtherTest, View) {
    string s("Hello World");
    string_view v = s.view();
    EXPECT_EQ(v, "Hello World");
    string_view v2 = s.view(6, 5);
    EXPECT_EQ(v2, "World");
}

TEST_F(BasicStringOtherTest, Copy) {
    string s("Hello World");
    char buf[20] = {};
    size_t n = s.copy(buf, 5);
    EXPECT_EQ(n, 5u);
    EXPECT_EQ(string(buf, 5), "Hello");
}

TEST_F(BasicStringOtherTest, CopyWithPosition) {
    string s("Hello World");
    char buf[20] = {};
    size_t n = s.copy(buf, 5, 6);
    EXPECT_EQ(n, 5u);
    EXPECT_EQ(string(buf, 5), "World");
}

TEST_F(BasicStringOtherTest, Count) {
    string s("Hello World Hello");
    EXPECT_EQ(s.count('l'), 5u);
    EXPECT_EQ(s.count('o'), 3u);
    EXPECT_EQ(s.count('z'), 0u);
}

TEST_F(BasicStringOtherTest, CountWithOffset) {
    string s("Hello World Hello");
    EXPECT_EQ(s.count('l', 3), 4u);
}

TEST_F(BasicStringOtherTest, Swap) {
    string s1("Hello");
    string s2("World");
    s1.swap(s2);
    EXPECT_EQ(s1, "World");
    EXPECT_EQ(s2, "Hello");
}

TEST_F(BasicStringOtherTest, Lowercase) {
    string s("Hello WORLD");
    s.lowercase();
    EXPECT_EQ(s, "hello world");
}

TEST_F(BasicStringOtherTest, Uppercase) {
    string s("Hello world");
    s.uppercase();
    EXPECT_EQ(s, "HELLO WORLD");
}

TEST_F(BasicStringOtherTest, EqualTo) {
    string s1("Hello");
    string s2("Hello");
    EXPECT_TRUE(s1.equal_to(s2));
    EXPECT_TRUE(s1.equal_to("Hello"));
    EXPECT_TRUE(s1.equal_to(string_view("Hello")));
    EXPECT_FALSE(s1.equal_to("World"));
}

TEST_F(BasicStringOtherTest, Hash) {
    string s1("Hello");
    string s2("Hello");
    string s3("World");
    EXPECT_EQ(s1.to_hash(), s2.to_hash());
    EXPECT_NE(s1.to_hash(), s3.to_hash());
}

TEST_F(BasicStringOtherTest, Repeat) {
    string s("Hi");
    string result = s.repeat(3);
    EXPECT_EQ(result, "HiHiHi");
}

class RegexConstructionTest : public ::testing::Test {};
class RegexMatchTest : public ::testing::Test {};
class RegexSearchTest : public ::testing::Test {};
class RegexFindAllTest : public ::testing::Test {};
class RegexReplaceTest : public ::testing::Test {};
class RegexSplitTest : public ::testing::Test {};
class RegexIteratorTest : public ::testing::Test {};
class RegexTokenIteratorTest : public ::testing::Test {};
class MatchResultTest : public ::testing::Test {};
class RegexMoveTest : public ::testing::Test {};

TEST_F(RegexConstructionTest, ValidPattern) { EXPECT_NO_THROW(regex re("hello")); }

TEST_F(RegexConstructionTest, ValidPatternWithGroups) { EXPECT_NO_THROW(regex re("(\\w+)=(\\d+)")); }

TEST_F(RegexConstructionTest, ValidPatternWithOptions) { EXPECT_NO_THROW(regex re("hello", PCRE2_CASELESS)); }

TEST_F(RegexConstructionTest, InvalidPattern) { EXPECT_THROW(regex re("[invalid"), regex_exception); }

TEST_F(RegexConstructionTest, ValidReturnsTrue) {
    regex re("test");
    EXPECT_TRUE(re.valid());
}

TEST_F(RegexConstructionTest, Pattern) {
    regex re("hello world");
    EXPECT_EQ(re.pattern(), "hello world");
}

TEST_F(RegexConstructionTest, CaptureCount) {
    regex re("(\\w+)=(\\d+)");
    EXPECT_EQ(re.capture_count(), 2);
}

TEST_F(RegexConstructionTest, CaptureCountNone) {
    regex re("hello");
    EXPECT_EQ(re.capture_count(), 0);
}

TEST_F(RegexMatchTest, FullMatch) {
    regex re("hello");
    auto result = re.do_match("hello");
    EXPECT_TRUE(result.matched());
}

TEST_F(RegexMatchTest, FullMatchNoMatch) {
    regex re("hello");
    auto result = re.do_match("hello world");
    EXPECT_FALSE(result.matched());
}

TEST_F(RegexMatchTest, FullMatchWithGroups) {
    regex re("(\\d{4})-(\\d{2})-(\\d{2})");
    auto result = re.do_match("2023-10-15");
    EXPECT_TRUE(result.matched());
    EXPECT_EQ(result[1], "2023");
    EXPECT_EQ(result[2], "10");
    EXPECT_EQ(result[3], "15");
}

TEST_F(RegexMatchTest, MatchMethod) {
    regex re("\\d+");
    EXPECT_TRUE(re.match("12345"));
    EXPECT_FALSE(re.match("abc"));
}

TEST_F(RegexMatchTest, MatchEmpty) {
    regex re("^$");
    EXPECT_TRUE(re.match(""));
}

TEST_F(RegexMatchTest, MatchWithAnchors) {
    regex re("^hello$");
    EXPECT_TRUE(re.match("hello"));
    EXPECT_FALSE(re.match("hello world"));
}

TEST_F(RegexSearchTest, SearchFound) {
    regex re("world");
    auto result = re.search("hello world");
    EXPECT_TRUE(result.matched());
    EXPECT_EQ(result.position(), 6u);
    EXPECT_EQ(result.length(), 5u);
}

TEST_F(RegexSearchTest, SearchNotFound) {
    regex re("xyz");
    auto result = re.search("hello world");
    EXPECT_FALSE(result.matched());
}

TEST_F(RegexSearchTest, SearchWithPosition) {
    regex re("hello");
    auto result = re.search("hello hello", 6);
    EXPECT_TRUE(result.matched());
    EXPECT_EQ(result.position(), 6u);
}

TEST_F(RegexSearchTest, SearchAtStart) {
    regex re("hello");
    auto result = re.search("hello world");
    EXPECT_TRUE(result.matched());
    EXPECT_EQ(result.position(), 0u);
}

TEST_F(RegexSearchTest, SearchAtEnd) {
    regex re("world$");
    auto result = re.search("hello world");
    EXPECT_TRUE(result.matched());
}

TEST_F(RegexFindAllTest, FindAllMultipleMatches) {
    regex re("\\d+");
    auto results = re.find_all("abc 123 def 456 ghi 789");
    EXPECT_EQ(results.size(), 3u);
    EXPECT_EQ(results[0].data(), "123");
    EXPECT_EQ(results[1].data(), "456");
    EXPECT_EQ(results[2].data(), "789");
}

TEST_F(RegexFindAllTest, FindAllSingleMatch) {
    regex re("hello");
    auto results = re.find_all("hello world");
    EXPECT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].data(), "hello");
}

TEST_F(RegexFindAllTest, FindAllNoMatch) {
    regex re("xyz");
    auto results = re.find_all("hello world");
    EXPECT_TRUE(results.empty());
}

TEST_F(RegexFindAllTest, FindAllOverlapping) {
    regex re("(?=(\\d+))");
    auto results = re.find_all("123");
    EXPECT_EQ(results.size(), 3u);
}

TEST_F(RegexFindAllTest, FindAllWithGroups) {
    regex re("(\\w+)=(\\d+)");
    auto results = re.find_all("name=123 age=456");
    EXPECT_EQ(results.size(), 2u);
    EXPECT_EQ(results[0][1], "name");
    EXPECT_EQ(results[0][2], "123");
    EXPECT_EQ(results[1][1], "age");
    EXPECT_EQ(results[1][2], "456");
}

TEST_F(RegexReplaceTest, ReplaceFirst) {
    regex re("cat");
    string result = re.replace_first("the cat and the cat", "dog");
    EXPECT_EQ(result, "the dog and the cat");
}

TEST_F(RegexReplaceTest, ReplaceFirstWithFormat) {
    regex re("(\\w+)=(\\d+)");
    string result = re.replace_first("name=123 age=456", "$1 is $2");
    EXPECT_EQ(result, "name is 123 age=456");
}

TEST_F(RegexReplaceTest, ReplaceFirstNoMatch) {
    regex re("xyz");
    string result = re.replace_first("hello world", "replaced");
    EXPECT_EQ(result, "hello world");
}

TEST_F(RegexReplaceTest, ReplaceFirstDollarSign) {
    regex re("\\d+");
    string result = re.replace_first("price: 100", "$$50");
    EXPECT_EQ(result, "price: $50");
}

TEST_F(RegexReplaceTest, ReplaceFirstNamedGroup) {
    regex re("(\\w+)=(\\d+)");
    string result = re.replace_first("name=123", "${1}=${2}");
    EXPECT_EQ(result, "name=123");
}

TEST_F(RegexReplaceTest, ReplaceAll) {
    regex re("cat");
    string result = re.replace_all("the cat and the cat", "dog");
    EXPECT_EQ(result, "the dog and the dog");
}

TEST_F(RegexReplaceTest, ReplaceAllMultiplePatterns) {
    regex re("\\d+");
    string result = re.replace_all("12 and 34 and 56", "N");
    EXPECT_EQ(result, "N and N and N");
}

TEST_F(RegexReplaceTest, ReplaceAllNoMatch) {
    regex re("xyz");
    string result = re.replace_all("hello world", "replaced");
    EXPECT_EQ(result, "hello world");
}

TEST_F(RegexReplaceTest, ReplaceAllWithGroups) {
    regex re("(\\w+)=(\\d+)");
    string result = re.replace_all("name=123 age=456", "$2=$1");
    EXPECT_EQ(result, "123=name 456=age");
}

TEST_F(RegexReplaceTest, ReplaceAllCallback) {
    regex re("\\d+");
    string result = re.replace_all_callback("12 and 34", [](const match_result& m) -> string {
        int val = to_int32(m.data());
        return to_string(val * 2);
    });
    EXPECT_EQ(result, "24 and 68");
}

TEST_F(RegexReplaceTest, ReplaceAllCallbackWithGroups) {
    regex re("(\\w+)=(\\d+)");
    string result = re.replace_all_callback(
            "name=123 age=456", [](const match_result& m) -> string { return string(m[1]) + "->" + string(m[2]); });
    EXPECT_EQ(result, "name->123 age->456");
}

TEST_F(RegexReplaceTest, ReplaceAllCallbackNoMatch) {
    regex re("xyz");
    string result = re.replace_all_callback("hello", [](const match_result&) -> string { return "X"; });
    EXPECT_EQ(result, "hello");
}

TEST_F(RegexSplitTest, SplitBasic) {
    regex re(",");
    auto parts = re.split("a,b,c");
    EXPECT_EQ(parts.size(), 3u);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[1], "b");
    EXPECT_EQ(parts[2], "c");
}

TEST_F(RegexSplitTest, SplitWithWhitespace) {
    regex re("\\s+");
    auto parts = re.split("hello  world\tfoo");
    EXPECT_EQ(parts.size(), 3u);
    EXPECT_EQ(parts[0], "hello");
    EXPECT_EQ(parts[1], "world");
    EXPECT_EQ(parts[2], "foo");
}

TEST_F(RegexSplitTest, SplitWithLimit) {
    regex re(",");
    auto parts = re.split("a,b,c,d", 2);
    EXPECT_EQ(parts.size(), 3u);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[1], "b");
    EXPECT_EQ(parts[2], "c,d");
}

TEST_F(RegexSplitTest, SplitNoMatch) {
    regex re(",");
    auto parts = re.split("hello");
    EXPECT_EQ(parts.size(), 1u);
    EXPECT_EQ(parts[0], "hello");
}

TEST_F(RegexSplitTest, SplitEmptyString) {
    regex re(",");
    auto parts = re.split("");
    EXPECT_EQ(parts.size(), 1u);
    EXPECT_EQ(parts[0], "");
}

TEST_F(RegexSplitTest, SplitWithGroups) {
    regex re("(,)");
    auto parts = re.split("a,b,c");
    EXPECT_EQ(parts.size(), 5u);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[1], ",");
    EXPECT_EQ(parts[2], "b");
    EXPECT_EQ(parts[3], ",");
    EXPECT_EQ(parts[4], "c");
}

TEST_F(MatchResultTest, MatchedTrue) {
    regex re("hello");
    auto result = re.search("hello world");
    EXPECT_TRUE(result.matched());
}

TEST_F(MatchResultTest, MatchedFalse) {
    regex re("xyz");
    auto result = re.search("hello");
    EXPECT_FALSE(result.matched());
}

TEST_F(MatchResultTest, Position) {
    regex re("world");
    auto result = re.search("hello world");
    EXPECT_EQ(result.position(), 6u);
}

TEST_F(MatchResultTest, Length) {
    regex re("world");
    auto result = re.search("hello world");
    EXPECT_EQ(result.length(), 5u);
}

TEST_F(MatchResultTest, Data) {
    regex re("world");
    auto result = re.search("hello world");
    EXPECT_EQ(result.data(), "world");
}

TEST_F(MatchResultTest, DataNoMatch) {
    regex re("xyz");
    auto result = re.search("hello");
    EXPECT_EQ(result.data(), "");
}

TEST_F(MatchResultTest, Size) {
    regex re("(\\d{4})-(\\d{2})-(\\d{2})");
    auto result = re.search("Date: 2023-10-15");
    EXPECT_EQ(result.size(), 4u);
}

TEST_F(MatchResultTest, SizeNoGroups) {
    regex re("hello");
    auto result = re.search("hello world");
    EXPECT_EQ(result.size(), 1u);
}

TEST_F(MatchResultTest, SubscriptOperator) {
    regex re("(\\w+)=(\\d+)");
    auto result = re.search("name=123");
    EXPECT_EQ(result[0], "name=123");
    EXPECT_EQ(result[1], "name");
    EXPECT_EQ(result[2], "123");
}

TEST_F(MatchResultTest, SubscriptOutOfRange) {
    regex re("hello");
    auto result = re.search("hello");
    EXPECT_EQ(result[10], "");
}

TEST_F(MatchResultTest, GroupPosition) {
    regex re("(\\w+)=(\\d+)");
    auto result = re.search("prefix name=123 suffix ");
    EXPECT_EQ(result.position(1).first, 7u);
    EXPECT_EQ(result.position(1).second, 4u);
}

TEST_F(MatchResultTest, GroupPositionOutOfRange) {
    regex re("hello");
    auto result = re.search("hello");
    auto pos = result.position(10);
    EXPECT_EQ(pos.first, string::npos);
    EXPECT_EQ(pos.second, 0u);
}

TEST_F(MatchResultTest, Prefix) {
    regex re("world");
    auto result = re.search("hello world!");
    EXPECT_EQ(result.prefix(), "hello ");
}

TEST_F(MatchResultTest, PrefixNoMatch) {
    regex re("xyz");
    auto result = re.search("hello");
    EXPECT_EQ(result.prefix(), "");
}

TEST_F(MatchResultTest, Suffix) {
    regex re("world");
    auto result = re.search("hello world!");
    EXPECT_EQ(result.suffix(), "!");
}

TEST_F(MatchResultTest, SuffixNoMatch) {
    regex re("xyz");
    auto result = re.search("hello");
    EXPECT_EQ(result.suffix(), "");
}

TEST_F(MatchResultTest, FormatDollarAmpersand) {
    regex re("\\d+");
    auto result = re.search("price: 100");
    EXPECT_EQ(result.format("$$ $&"), "$ 100");
}

TEST_F(MatchResultTest, FormatDollarBacktick) {
    regex re("\\d+");
    auto result = re.search("price: 100");
    EXPECT_EQ(result.format("$`"), "price: ");
}

TEST_F(MatchResultTest, FormatDollarQuote) {
    regex re("\\d+");
    auto result = re.search("price: 100 usd");
    EXPECT_EQ(result.format("$'"), " usd");
}

TEST_F(MatchResultTest, FormatNamedGroup) {
    regex re("(\\w+)=(\\d+)");
    auto result = re.search("name=123");
    EXPECT_EQ(result.format("${1} -> ${2}"), "name -> 123");
}

TEST_F(MatchResultTest, FormatNoMatch) {
    regex re("xyz");
    auto result = re.search("hello");
    EXPECT_EQ(result.format("$&"), "");
}

TEST_F(MatchResultTest, IteratorBeginEnd) {
    regex re("(\\w+)=(\\d+)");
    auto result = re.search("name=123");
    vector<string> groups;
    for (const auto& g: result) {
        groups.push_back(g);
    }
    EXPECT_EQ(groups.size(), 3u);
    EXPECT_EQ(groups[0], "name=123");
    EXPECT_EQ(groups[1], "name");
    EXPECT_EQ(groups[2], "123");
}

TEST_F(RegexMoveTest, MoveConstructor) {
    regex re1("hello");
    regex re2(move(re1));
    EXPECT_TRUE(re2.valid());
    EXPECT_EQ(re2.pattern(), "hello");
}

TEST_F(RegexMoveTest, MoveAssignment) {
    regex re1("hello");
    regex re2("world");
    re2 = move(re1);
    EXPECT_TRUE(re2.valid());
    EXPECT_EQ(re2.pattern(), "hello");
}

TEST_F(RegexIteratorTest, BeginEnd) {
    regex re("\\d+");
    string str = "12 and 34 and 56";
    vector<string> matches;
    for (auto it = re.begin(str); it != re.end(str); ++it) {
        matches.push_back(string(it->data()));
    }
    EXPECT_EQ(matches.size(), 3u);
    EXPECT_EQ(matches[0], "12");
    EXPECT_EQ(matches[1], "34");
    EXPECT_EQ(matches[2], "56");
}

TEST_F(RegexIteratorTest, EmptyResult) {
    regex re("\\d+");
    string str = "no numbers here";
    auto it = re.begin(str);
    EXPECT_EQ(it, re.end(str));
}

TEST_F(RegexIteratorTest, Dereference) {
    regex re("world");
    string str = "hello world";
    auto it = re.begin(str);
    EXPECT_EQ((*it).data(), "world");
}

TEST_F(RegexIteratorTest, ArrowOperator) {
    regex re("world");
    string str = "hello world";
    auto it = re.begin(str);
    EXPECT_EQ(it->data(), "world");
    EXPECT_EQ(it->position(), 6u);
}

TEST_F(RegexIteratorTest, PostfixIncrement) {
    regex re("\\d+");
    string str = "1 2 3";
    auto it = re.begin(str);
    auto it2 = it++;
    EXPECT_EQ(it2->data(), "1");
    EXPECT_EQ(it->data(), "2");
}

TEST_F(RegexIteratorTest, Decrement) {
    regex re("\\d+");
    string str = "1 2 3";
    auto it = re.end(str);
    --it;
    EXPECT_EQ(it->data(), "3");
    --it;
    EXPECT_EQ(it->data(), "2");
    --it;
    EXPECT_EQ(it->data(), "1");
}

TEST_F(RegexIteratorTest, PostfixDecrement) {
    regex re("\\d+");
    string str = "1 2 3";
    auto it = re.end(str);
    it--;
    auto it2 = it--;
    EXPECT_EQ(it2->data(), "3");
    EXPECT_EQ(it->data(), "2");
}

TEST_F(RegexIteratorTest, NotEqual) {
    regex re("\\d+");
    string str = "1 2";
    auto it1 = re.begin(str);
    auto it2 = re.end(str);
    EXPECT_TRUE(it1 != it2);
    ++it1;
    ++it1;
    EXPECT_FALSE(it1 != it2);
}

TEST_F(RegexIteratorTest, WithGroups) {
    regex re("(\\w+)=(\\d+)");
    string str = "name=123 age=456";
    vector<string> names;
    for (auto it = re.begin(str); it != re.end(str); ++it) {
        names.push_back(string((*it)[1]));
    }
    EXPECT_EQ(names.size(), 2u);
    EXPECT_EQ(names[0], "name");
    EXPECT_EQ(names[1], "age");
}

TEST_F(RegexIteratorTest, FromIndex) {
    regex re("\\d+");
    string str = "1 2 3";
    auto it = regex_iterator::from_index(&re, str, 2);
    EXPECT_EQ(it->data(), "3");
}

TEST_F(RegexIteratorTest, FromIndexEnd) {
    regex re("\\d+");
    string str = "1 2 3";
    auto it = regex_iterator::from_index(&re, str, 3);
    EXPECT_EQ(it, re.end(str));
}

TEST_F(RegexTokenIteratorTest, SplitMode) {
    regex re(",");
    string str = "a,b,c";
    vector<string> tokens;
    regex_token_iterator it(&re, str, -1);
    regex_token_iterator end;
    while (it != end) {
        tokens.push_back(string(*it));
        ++it;
    }
    EXPECT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0], "a");
    EXPECT_EQ(tokens[1], "b");
    EXPECT_EQ(tokens[2], "c");
}

TEST_F(RegexTokenIteratorTest, SplitModeWithWhitespace) {
    regex re("\\s+");
    string str = "hello  world\tfoo";
    vector<string> tokens;
    regex_token_iterator it(&re, str, -1);
    regex_token_iterator end;
    while (it != end) {
        tokens.push_back(string(*it));
        ++it;
    }
    EXPECT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0], "hello");
    EXPECT_EQ(tokens[1], "world");
    EXPECT_EQ(tokens[2], "foo");
}

TEST_F(RegexTokenIteratorTest, SplitModeNoMatch) {
    regex re(",");
    string str = "hello";
    vector<string> tokens;
    regex_token_iterator it(&re, str, -1);
    regex_token_iterator end;
    while (it != end) {
        tokens.push_back(string(*it));
        ++it;
    }
    EXPECT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0], "hello");
}

TEST_F(RegexTokenIteratorTest, SplitModeWithLeadingSeparator) {
    regex re(",");
    string str = ",a,b";
    vector<string> tokens;
    regex_token_iterator it(&re, str, -1);
    regex_token_iterator end;
    while (it != end) {
        tokens.push_back(string(*it));
        ++it;
    }
    EXPECT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0], "");
    EXPECT_EQ(tokens[1], "a");
    EXPECT_EQ(tokens[2], "b");
}

TEST_F(RegexTokenIteratorTest, GroupMode) {
    regex re("(\\w+)=(\\d+)");
    string str = "name=123 age=456";
    vector<string> tokens;
    regex_token_iterator it(&re, str, 2);
    regex_token_iterator end;
    while (it != end) {
        tokens.push_back(string(*it));
        ++it;
    }
    EXPECT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0], "123");
    EXPECT_EQ(tokens[1], "456");
}

TEST_F(RegexTokenIteratorTest, GroupModeFullMatch) {
    regex re("\\d+");
    string str = "12 34 56";
    vector<string> tokens;
    regex_token_iterator it(&re, str, 0);
    regex_token_iterator end;
    while (it != end) {
        tokens.push_back(string(*it));
        ++it;
    }
    EXPECT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0], "12");
    EXPECT_EQ(tokens[1], "34");
    EXPECT_EQ(tokens[2], "56");
}

TEST_F(RegexTokenIteratorTest, PostfixIncrement) {
    regex re("\\d+");
    string str = "1 2 3";
    regex_token_iterator it(&re, str, 0);
    regex_token_iterator end;
    auto it2 = it++;
    EXPECT_EQ(string(*it2), "1");
    EXPECT_EQ(string(*it), "2");
}

class CodePointConstructionTest : public ::testing::Test {};
class CodePointValidationTest : public ::testing::Test {};
class CodePointSurrogateTest : public ::testing::Test {};
class CodePointUTF8Test : public ::testing::Test {};
class CodePointUTF16Test : public ::testing::Test {};
class CodePointUTF32Test : public ::testing::Test {};
class CodePointAppendTest : public ::testing::Test {};
class CodePointComparisonTest : public ::testing::Test {};
class CodePointPropertiesTest : public ::testing::Test {};

TEST_F(CodePointConstructionTest, DefaultConstructor) {
    codepoint cp;
    EXPECT_EQ(cp.value(), 0u);
    EXPECT_EQ(cp.to_char32(), U'\0');
}

TEST_F(CodePointConstructionTest, FromValidUint32) {
    codepoint cp(0x41u);
    EXPECT_EQ(cp.value(), 0x41u);
    EXPECT_EQ(cp.to_char32(), U'A');
}

TEST_F(CodePointConstructionTest, FromValidMaxUint32) {
    codepoint cp(0x10FFFFu);
    EXPECT_EQ(cp.value(), 0x10FFFFu);
    EXPECT_FALSE(cp.is_replacement());
}

TEST_F(CodePointConstructionTest, FromInvalidUint32TooLarge) {
    codepoint cp(0x110000u);
    EXPECT_EQ(cp.value(), codepoint::REPLACEMENT_VALUE);
    EXPECT_TRUE(cp.is_replacement());
}

TEST_F(CodePointConstructionTest, FromInvalidUint32HighSurrogate) {
    codepoint cp(0xD800u);
    EXPECT_EQ(cp.value(), codepoint::REPLACEMENT_VALUE);
    EXPECT_TRUE(cp.is_replacement());
}

TEST_F(CodePointConstructionTest, FromInvalidUint32LowSurrogate) {
    codepoint cp(0xDC00u);
    EXPECT_EQ(cp.value(), codepoint::REPLACEMENT_VALUE);
    EXPECT_TRUE(cp.is_replacement());
}

TEST_F(CodePointConstructionTest, FromInvalidUint32MidSurrogate) {
    codepoint cp(0xDDDDu);
    EXPECT_EQ(cp.value(), codepoint::REPLACEMENT_VALUE);
    EXPECT_TRUE(cp.is_replacement());
}

TEST_F(CodePointConstructionTest, FromChar32) {
    codepoint cp(U'A');
    EXPECT_EQ(cp.value(), 0x41u);
    EXPECT_EQ(cp.to_char32(), U'A');
}

TEST_F(CodePointConstructionTest, FromChar32Invalid) {
    codepoint cp(static_cast<char32_t>(0x110000u));
    EXPECT_TRUE(cp.is_replacement());
}

TEST_F(CodePointConstructionTest, CopyConstructor) {
    codepoint cp1(0x41u);
    codepoint cp2(cp1);
    EXPECT_EQ(cp2.value(), 0x41u);
}

TEST_F(CodePointConstructionTest, CopyAssignment) {
    codepoint cp1(0x41u);
    codepoint cp2;
    cp2 = cp1;
    EXPECT_EQ(cp2.value(), 0x41u);
}

TEST_F(CodePointConstructionTest, MoveConstructor) {
    codepoint cp1(0x41u);
    codepoint cp2(move(cp1));
    EXPECT_EQ(cp2.value(), 0x41u);
}

TEST_F(CodePointConstructionTest, MoveAssignment) {
    codepoint cp1(0x41u);
    codepoint cp2;
    cp2 = move(cp1);
    EXPECT_EQ(cp2.value(), 0x41u);
}

TEST_F(CodePointConstructionTest, ReplacementStatic) {
    codepoint cp = codepoint::replacement();
    EXPECT_EQ(cp.value(), 0xFFFDu);
    EXPECT_TRUE(cp.is_replacement());
}

TEST_F(CodePointConstructionTest, NullStatic) {
    codepoint cp = codepoint::null();
    EXPECT_EQ(cp.value(), 0u);
}

TEST_F(CodePointValidationTest, IsValidCodepointAscii) {
    EXPECT_TRUE(codepoint::is_valid_codepoint(0x41u));
    EXPECT_TRUE(codepoint::is_valid_codepoint(0x00u));
    EXPECT_TRUE(codepoint::is_valid_codepoint(0x7Fu));
}

TEST_F(CodePointValidationTest, IsValidCodepointBMP) {
    EXPECT_TRUE(codepoint::is_valid_codepoint(0x0100u));
    EXPECT_TRUE(codepoint::is_valid_codepoint(0xFFFFu));
}

TEST_F(CodePointValidationTest, IsValidCodepointSupplementary) {
    EXPECT_TRUE(codepoint::is_valid_codepoint(0x10000u));
    EXPECT_TRUE(codepoint::is_valid_codepoint(0x10FFFFu));
}

TEST_F(CodePointValidationTest, IsValidCodepointHighSurrogate) {
    EXPECT_FALSE(codepoint::is_valid_codepoint(0xD800u));
    EXPECT_FALSE(codepoint::is_valid_codepoint(0xDBFFu));
}

TEST_F(CodePointValidationTest, IsValidCodepointLowSurrogate) {
    EXPECT_FALSE(codepoint::is_valid_codepoint(0xDC00u));
    EXPECT_FALSE(codepoint::is_valid_codepoint(0xDFFFu));
}

TEST_F(CodePointValidationTest, IsValidCodepointTooLarge) {
    EXPECT_FALSE(codepoint::is_valid_codepoint(0x110000u));
    EXPECT_FALSE(codepoint::is_valid_codepoint(0xFFFFFFFFu));
}

TEST_F(CodePointSurrogateTest, IsHighSurrogateTrue) {
    EXPECT_TRUE(codepoint::is_high_surrogate(0xD800));
    EXPECT_TRUE(codepoint::is_high_surrogate(0xDBFF));
    EXPECT_TRUE(codepoint::is_high_surrogate(0xD900));
}

TEST_F(CodePointSurrogateTest, IsHighSurrogateFalse) {
    EXPECT_FALSE(codepoint::is_high_surrogate(0xD7FF));
    EXPECT_FALSE(codepoint::is_high_surrogate(0xDC00));
    EXPECT_FALSE(codepoint::is_high_surrogate(0x0000));
    EXPECT_FALSE(codepoint::is_high_surrogate(0xFFFF));
}

TEST_F(CodePointSurrogateTest, IsLowSurrogateTrue) {
    EXPECT_TRUE(codepoint::is_low_surrogate(0xDC00));
    EXPECT_TRUE(codepoint::is_low_surrogate(0xDFFF));
    EXPECT_TRUE(codepoint::is_low_surrogate(0xDD00));
}

TEST_F(CodePointSurrogateTest, IsLowSurrogateFalse) {
    EXPECT_FALSE(codepoint::is_low_surrogate(0xDBFF));
    EXPECT_FALSE(codepoint::is_low_surrogate(0xE000));
    EXPECT_FALSE(codepoint::is_low_surrogate(0x0000));
    EXPECT_FALSE(codepoint::is_low_surrogate(0xFFFF));
}

TEST_F(CodePointSurrogateTest, CombineSurrogates) {
    codepoint cp = codepoint::combine_surrogates(0xD800, 0xDC00);
    EXPECT_EQ(cp.value(), 0x10000u);
}

TEST_F(CodePointSurrogateTest, CombineSurrogatesMax) {
    codepoint cp = codepoint::combine_surrogates(0xDBFF, 0xDFFF);
    EXPECT_EQ(cp.value(), 0x10FFFFu);
}

TEST_F(CodePointSurrogateTest, CombineSurrogatesMid) {
    codepoint cp = codepoint::combine_surrogates(0xD834, 0xDD1E);
    EXPECT_EQ(cp.value(), 0x1D11Eu);
}

TEST_F(CodePointUTF8Test, DecodeAscii) {
    const byte_t data[] = {0x41, 0x42, 0x43};
    size_t i = 0;
    codepoint cp = codepoint::decode_utf8(data, i, 3);
    EXPECT_EQ(cp.value(), 0x41u);
    EXPECT_EQ(i, 1u);
}

TEST_F(CodePointUTF8Test, DecodeTwoByte) {
    const byte_t data[] = {0xC2, 0xA9};
    size_t i = 0;
    codepoint cp = codepoint::decode_utf8(data, i, 2);
    EXPECT_EQ(cp.value(), 0xA9u);
    EXPECT_EQ(i, 2u);
}

TEST_F(CodePointUTF8Test, DecodeThreeByte) {
    const byte_t data[] = {0xE2, 0x82, 0xAC};
    size_t i = 0;
    codepoint cp = codepoint::decode_utf8(data, i, 3);
    EXPECT_EQ(cp.value(), 0x20ACu);
    EXPECT_EQ(i, 3u);
}

TEST_F(CodePointUTF8Test, DecodeFourByte) {
    const byte_t data[] = {0xF0, 0x9F, 0x98, 0x80};
    size_t i = 0;
    codepoint cp = codepoint::decode_utf8(data, i, 4);
    EXPECT_EQ(cp.value(), 0x1F600u);
    EXPECT_EQ(i, 4u);
}

TEST_F(CodePointUTF8Test, DecodeTruncatedTwoByte) {
    const byte_t data[] = {0xC2};
    size_t i = 0;
    codepoint cp = codepoint::decode_utf8(data, i, 1);
    EXPECT_TRUE(cp.is_replacement());
}

TEST_F(CodePointUTF8Test, DecodeTruncatedThreeByte) {
    const byte_t data[] = {0xE2, 0x82};
    size_t i = 0;
    codepoint cp = codepoint::decode_utf8(data, i, 2);
    EXPECT_TRUE(cp.is_replacement());
}

TEST_F(CodePointUTF8Test, DecodeTruncatedFourByte) {
    const byte_t data[] = {0xF0, 0x9F, 0x98};
    size_t i = 0;
    codepoint cp = codepoint::decode_utf8(data, i, 3);
    EXPECT_TRUE(cp.is_replacement());
}

TEST_F(CodePointUTF8Test, DecodeInvalidContinuation) {
    const byte_t data[] = {0xC2, 0x30};
    size_t i = 0;
    codepoint cp = codepoint::decode_utf8(data, i, 2);
    EXPECT_TRUE(cp.is_replacement());
}

TEST_F(CodePointUTF8Test, DecodeOverlong) {
    const byte_t data[] = {0xC0, 0xAF};
    size_t i = 0;
    codepoint cp = codepoint::decode_utf8(data, i, 2);
    EXPECT_TRUE(cp.is_replacement());
}

TEST_F(CodePointUTF8Test, DecodeSurrogate) {
    const byte_t data[] = {0xED, 0xA0, 0x80};
    size_t i = 0;
    codepoint cp = codepoint::decode_utf8(data, i, 3);
    EXPECT_TRUE(cp.is_replacement());
}

TEST_F(CodePointUTF8Test, DecodeTooLarge) {
    const byte_t data[] = {0xF4, 0x90, 0x80, 0x80};
    size_t i = 0;
    codepoint cp = codepoint::decode_utf8(data, i, 4);
    EXPECT_TRUE(cp.is_replacement());
}

TEST_F(CodePointUTF8Test, DecodeMultipleChars) {
    const byte_t data[] = {0x41, 0xC2, 0xA9, 0xE2, 0x82, 0xAC};
    size_t i = 0;
    codepoint cp1 = codepoint::decode_utf8(data, i, 6);
    EXPECT_EQ(cp1.value(), 0x41u);
    codepoint cp2 = codepoint::decode_utf8(data, i, 6);
    EXPECT_EQ(cp2.value(), 0xA9u);
    codepoint cp3 = codepoint::decode_utf8(data, i, 6);
    EXPECT_EQ(cp3.value(), 0x20ACu);
}

TEST_F(CodePointUTF8Test, UTF8LengthAscii) {
    codepoint cp(U'A');
    EXPECT_EQ(cp.utf8_length(), 1u);
}

TEST_F(CodePointUTF8Test, UTF8LengthTwoByte) {
    codepoint cp(0xA9u);
    EXPECT_EQ(cp.utf8_length(), 2u);
}

TEST_F(CodePointUTF8Test, UTF8LengthThreeByte) {
    codepoint cp(0x20ACu);
    EXPECT_EQ(cp.utf8_length(), 3u);
}

TEST_F(CodePointUTF8Test, UTF8LengthFourByte) {
    codepoint cp(0x1F600u);
    EXPECT_EQ(cp.utf8_length(), 4u);
}

TEST_F(CodePointUTF8Test, UTF8LengthReplacement) {
    codepoint cp = codepoint::replacement();
    EXPECT_EQ(cp.utf8_length(), 3u);
}

TEST_F(CodePointUTF16Test, DecodeSingleUnit) {
    const char16_t data[] = {u'A', u'B', u'C'};
    size_t i = 0;
    codepoint cp = codepoint::decode_utf16(data, i, 3, false);
    EXPECT_EQ(cp.value(), 0x41u);
    EXPECT_EQ(i, 1u);
}

TEST_F(CodePointUTF16Test, DecodeSurrogatePair) {
    const char16_t data[] = {0xD800, 0xDC00};
    size_t i = 0;
    codepoint cp = codepoint::decode_utf16(data, i, 2, false);
    EXPECT_EQ(cp.value(), 0x10000u);
    EXPECT_EQ(i, 2u);
}

TEST_F(CodePointUTF16Test, DecodeHighSurrogateAlone) {
    const char16_t data[] = {0xD800, u'A'};
    size_t i = 0;
    codepoint cp = codepoint::decode_utf16(data, i, 2, false);
    EXPECT_TRUE(cp.is_replacement());
}

TEST_F(CodePointUTF16Test, DecodeLowSurrogateAlone) {
    const char16_t data[] = {0xDC00};
    size_t i = 0;
    codepoint cp = codepoint::decode_utf16(data, i, 1, false);
    EXPECT_TRUE(cp.is_replacement());
}

TEST_F(CodePointUTF16Test, DecodeHighSurrogateAtEnd) {
    const char16_t data[] = {u'A', 0xD800};
    size_t i = 1;
    codepoint cp = codepoint::decode_utf16(data, i, 2, false);
    EXPECT_TRUE(cp.is_replacement());
}

TEST_F(CodePointUTF16Test, UTF16LengthBMP) {
    codepoint cp(U'A');
    EXPECT_EQ(cp.utf16_length(), 1u);
}

TEST_F(CodePointUTF16Test, UTF16LengthSupplementary) {
    codepoint cp(0x10000u);
    EXPECT_EQ(cp.utf16_length(), 2u);
}

TEST_F(CodePointUTF16Test, UTF16LengthReplacement) {
    codepoint cp = codepoint::replacement();
    EXPECT_EQ(cp.utf16_length(), 1u);
}

TEST_F(CodePointUTF32Test, FromUTF32) {
    codepoint cp = codepoint::from_utf32(U'A');
    EXPECT_EQ(cp.value(), 0x41u);
    EXPECT_EQ(cp.to_char32(), U'A');
}

TEST_F(CodePointUTF32Test, FromUTF32Supplementary) {
    codepoint cp = codepoint::from_utf32(U'\U0001F600');
    EXPECT_EQ(cp.value(), 0x1F600u);
}

TEST_F(CodePointUTF32Test, ToChar32) {
    codepoint cp(0x20ACu);
    EXPECT_EQ(cp.to_char32(), U'\u20AC');
}

TEST_F(CodePointAppendTest, AppendToStringAscii) {
    codepoint cp(U'A');
    string s;
    cp.append_to(s);
    EXPECT_EQ(s, "A");
}

TEST_F(CodePointAppendTest, AppendToStringTwoByte) {
    codepoint cp(0xA9u);
    string s;
    cp.append_to(s);
    EXPECT_EQ(s.size(), 2u);
}

TEST_F(CodePointAppendTest, AppendToStringThreeByte) {
    codepoint cp(0x20ACu);
    string s;
    cp.append_to(s);
    EXPECT_EQ(s.size(), 3u);
}

TEST_F(CodePointAppendTest, AppendToStringFourByte) {
    codepoint cp(0x1F600u);
    string s;
    cp.append_to(s);
    EXPECT_EQ(s.size(), 4u);
}

TEST_F(CodePointAppendTest, AppendToStringReplacement) {
    codepoint cp(0x110000u);
    string s;
    cp.append_to(s);
    EXPECT_EQ(s.size(), 3u);
}

TEST_F(CodePointAppendTest, AppendToU16StringBMP) {
    codepoint cp(U'A');
    u16string s;
    cp.append_to(s);
    EXPECT_EQ(s.size(), 1u);
    EXPECT_EQ(s[0], u'A');
}

TEST_F(CodePointAppendTest, AppendToU16StringSupplementary) {
    codepoint cp(0x10000u);
    u16string s;
    cp.append_to(s);
    EXPECT_EQ(s.size(), 2u);
    EXPECT_TRUE(codepoint::is_high_surrogate(s[0]));
    EXPECT_TRUE(codepoint::is_low_surrogate(s[1]));
}

TEST_F(CodePointAppendTest, AppendToU16StringReplacement) {
    codepoint cp(0x110000u);
    u16string s;
    cp.append_to(s);
    EXPECT_EQ(s.size(), 1u);
    EXPECT_EQ(s[0], 0xFFFD);
}

TEST_F(CodePointAppendTest, AppendToU32String) {
    codepoint cp(0x1F600u);
    u32string s;
    cp.append_to(s);
    EXPECT_EQ(s.size(), 1u);
    EXPECT_EQ(s[0], U'\U0001F600');
}

TEST_F(CodePointAppendTest, AppendToWString) {
    codepoint cp(U'A');
    wstring s;
    cp.append_to(s);
    EXPECT_GE(s.size(), 1u);
}

TEST_F(CodePointPropertiesTest, IsReplacementTrue) {
    codepoint cp(codepoint::REPLACEMENT_VALUE);
    EXPECT_TRUE(cp.is_replacement());
}

TEST_F(CodePointPropertiesTest, IsReplacementFalse) {
    codepoint cp(U'A');
    EXPECT_FALSE(cp.is_replacement());
}

TEST_F(CodePointPropertiesTest, IsAsciiTrue) {
    codepoint cp(U'A');
    EXPECT_TRUE(cp.is_ascii());
    codepoint cp2(0x7Fu);
    EXPECT_TRUE(cp2.is_ascii());
    codepoint cp3(0x00u);
    EXPECT_TRUE(cp3.is_ascii());
}

TEST_F(CodePointPropertiesTest, IsAsciiFalse) {
    codepoint cp(0x80u);
    EXPECT_FALSE(cp.is_ascii());
    codepoint cp2(0x20ACu);
    EXPECT_FALSE(cp2.is_ascii());
}

TEST_F(CodePointPropertiesTest, IsBMPTrue) {
    codepoint cp(U'A');
    EXPECT_TRUE(cp.is_bmp());
    codepoint cp2(0xFFFFu);
    EXPECT_TRUE(cp2.is_bmp());
}

TEST_F(CodePointPropertiesTest, IsBMPFalse) {
    codepoint cp(0x10000u);
    EXPECT_FALSE(cp.is_bmp());
}

TEST_F(CodePointPropertiesTest, IsSupplementaryTrue) {
    codepoint cp(0x10000u);
    EXPECT_TRUE(cp.is_supplementary());
    codepoint cp2(0x10FFFFu);
    EXPECT_TRUE(cp2.is_supplementary());
}

TEST_F(CodePointPropertiesTest, IsSupplementaryFalse) {
    codepoint cp(U'A');
    EXPECT_FALSE(cp.is_supplementary());
    codepoint cp2(0xFFFFu);
    EXPECT_FALSE(cp2.is_supplementary());
}

TEST_F(CodePointPropertiesTest, NeedsSurrogatePairTrue) {
    codepoint cp(0x10000u);
    EXPECT_TRUE(cp.needs_surrogate_pair());
}

TEST_F(CodePointPropertiesTest, NeedsSurrogatePairFalse) {
    codepoint cp(U'A');
    EXPECT_FALSE(cp.needs_surrogate_pair());
}

TEST_F(CodePointComparisonTest, EqualOperator) {
    codepoint cp1(U'A');
    codepoint cp2(U'A');
    codepoint cp3(U'B');
    EXPECT_TRUE(cp1 == cp2);
    EXPECT_FALSE(cp1 == cp3);
}

TEST_F(CodePointComparisonTest, NotEqualOperator) {
    codepoint cp1(U'A');
    codepoint cp2(U'B');
    EXPECT_TRUE(cp1 != cp2);
    EXPECT_FALSE(cp1 != codepoint(U'A'));
}

TEST_F(CodePointComparisonTest, LessOperator) {
    codepoint cp1(U'A');
    codepoint cp2(U'B');
    codepoint cp3(U'A');
    EXPECT_TRUE(cp1 < cp2);
    EXPECT_FALSE(cp1 < cp3);
    EXPECT_FALSE(cp2 < cp1);
}

TEST_F(CodePointComparisonTest, LessEqualOperator) {
    codepoint cp1(U'A');
    codepoint cp2(U'B');
    codepoint cp3(U'A');
    EXPECT_TRUE(cp1 <= cp2);
    EXPECT_TRUE(cp1 <= cp3);
    EXPECT_FALSE(cp2 <= cp1);
}

TEST_F(CodePointComparisonTest, GreaterOperator) {
    codepoint cp1(U'B');
    codepoint cp2(U'A');
    codepoint cp3(U'B');
    EXPECT_TRUE(cp1 > cp2);
    EXPECT_FALSE(cp1 > cp3);
    EXPECT_FALSE(cp2 > cp1);
}

TEST_F(CodePointComparisonTest, GreaterEqualOperator) {
    codepoint cp1(U'B');
    codepoint cp2(U'A');
    codepoint cp3(U'B');
    EXPECT_TRUE(cp1 >= cp2);
    EXPECT_TRUE(cp1 >= cp3);
    EXPECT_FALSE(cp2 >= cp1);
}

TEST_F(CodePointComparisonTest, EqualUint32) {
    codepoint cp(U'A');
    EXPECT_TRUE(cp == 0x41u);
    EXPECT_FALSE(cp == 0x42u);
}

TEST_F(CodePointComparisonTest, NotEqualUint32) {
    codepoint cp(U'A');
    EXPECT_TRUE(cp != 0x42u);
    EXPECT_FALSE(cp != 0x41u);
}

class CharacterTest : public ::testing::Test {};
class WcharacterTest : public ::testing::Test {};
class U16characterTest : public ::testing::Test {};
class U32characterTest : public ::testing::Test {};

TEST_F(CharacterTest, DefaultConstructor) {
    character c;
    EXPECT_EQ(c.value(), '\0');
}

TEST_F(CharacterTest, ValueConstructor) {
    character c('A');
    EXPECT_EQ(c.value(), 'A');
}

TEST_F(CharacterTest, CopyConstructor) {
    character c1('A');
    character c2(c1);
    EXPECT_EQ(c2.value(), 'A');
}

TEST_F(CharacterTest, MoveConstructor) {
    character c1('A');
    character c2(move(c1));
    EXPECT_EQ(c2.value(), 'A');
}

TEST_F(CharacterTest, CopyAssignment) {
    character c1('A');
    character c2;
    c2 = c1;
    EXPECT_EQ(c2.value(), 'A');
}

TEST_F(CharacterTest, MoveAssignment) {
    character c1('A');
    character c2;
    c2 = move(c1);
    EXPECT_EQ(c2.value(), 'A');
}

TEST_F(CharacterTest, ValueAssignment) {
    character c;
    c = 'B';
    EXPECT_EQ(c.value(), 'B');
}

TEST_F(CharacterTest, ToStringAscii) {
    string_view sv("Hello");
    string result = character::to_string(sv);
    EXPECT_EQ(result, "Hello");
}

TEST_F(CharacterTest, ToStringEmpty) {
    string_view sv;
    string result = character::to_string(sv);
    EXPECT_TRUE(result.empty());
}

TEST_F(CharacterTest, ToWstringAscii) {
    string_view sv("Hello");
    wstring result = character::to_wstring(sv);
    EXPECT_EQ(result.size(), 5u);
    EXPECT_EQ(result[0], L'H');
}

TEST_F(CharacterTest, ToWstringUTF8) {
    string_view sv("\xC2\xA9");
    wstring result = character::to_wstring(sv);
    EXPECT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0], static_cast<wchar_t>(0xA9));
}

TEST_F(CharacterTest, ToWstringEmpty) {
    string_view sv;
    wstring result = character::to_wstring(sv);
    EXPECT_TRUE(result.empty());
}

TEST_F(CharacterTest, ToU16stringAscii) {
    string_view sv("Hello");
    u16string result = character::to_u16string(sv);
    EXPECT_EQ(result.size(), 5u);
    EXPECT_EQ(result[0], u'H');
}

TEST_F(CharacterTest, ToU16stringUTF8) {
    string_view sv("\xC2\xA9");
    u16string result = character::to_u16string(sv);
    EXPECT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0], 0xA9);
}

TEST_F(CharacterTest, ToU16stringEmpty) {
    string_view sv;
    u16string result = character::to_u16string(sv);
    EXPECT_TRUE(result.empty());
}

TEST_F(CharacterTest, ToU32stringAscii) {
    string_view sv("Hello");
    u32string result = character::to_u32string(sv);
    EXPECT_EQ(result.size(), 5u);
    EXPECT_EQ(result[0], U'H');
}

TEST_F(CharacterTest, ToU32stringUTF8) {
    string_view sv("\xE2\x82\xAC");
    u32string result = character::to_u32string(sv);
    EXPECT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0], U'\u20AC');
}

TEST_F(CharacterTest, ToU32stringEmpty) {
    string_view sv;
    u32string result = character::to_u32string(sv);
    EXPECT_TRUE(result.empty());
}

TEST_F(CharacterTest, PackageChar) {
    character p{'A'};
    EXPECT_EQ(p.value(), 'A');
}

TEST_F(WcharacterTest, DefaultConstructor) {
    wcharacter c;
    EXPECT_EQ(c.value(), L'\0');
}

TEST_F(WcharacterTest, ValueConstructor) {
    wcharacter c(L'A');
    EXPECT_EQ(c.value(), L'A');
}

TEST_F(WcharacterTest, CopyConstructor) {
    wcharacter c1(L'A');
    wcharacter c2(c1);
    EXPECT_EQ(c2.value(), L'A');
}

TEST_F(WcharacterTest, ValueAssignment) {
    wcharacter c;
    c = L'B';
    EXPECT_EQ(c.value(), L'B');
}

TEST_F(WcharacterTest, ToStringAscii) {
    wstring_view sv(L"Hello");
    string result = wcharacter::to_string(sv);
    EXPECT_EQ(result, "Hello");
}

TEST_F(WcharacterTest, ToStringEmpty) {
    wstring_view sv;
    string result = wcharacter::to_string(sv);
    EXPECT_TRUE(result.empty());
}

TEST_F(WcharacterTest, ToWstring) {
    wstring_view sv(L"Hello");
    wstring result = wcharacter::to_wstring(sv);
    EXPECT_EQ(result, L"Hello");
}

TEST_F(WcharacterTest, ToWstringEmpty) {
    wstring_view sv;
    wstring result = wcharacter::to_wstring(sv);
    EXPECT_TRUE(result.empty());
}

TEST_F(WcharacterTest, ToU16stringAscii) {
    wstring_view sv(L"Hello");
    u16string result = wcharacter::to_u16string(sv);
    EXPECT_EQ(result.size(), 5u);
    EXPECT_EQ(result[0], u'H');
}

TEST_F(WcharacterTest, ToU16stringEmpty) {
    wstring_view sv;
    u16string result = wcharacter::to_u16string(sv);
    EXPECT_TRUE(result.empty());
}

TEST_F(WcharacterTest, ToU32stringAscii) {
    wstring_view sv(L"Hello");
    u32string result = wcharacter::to_u32string(sv);
    EXPECT_EQ(result.size(), 5u);
    EXPECT_EQ(result[0], U'H');
}

TEST_F(WcharacterTest, ToU32stringEmpty) {
    wstring_view sv;
    u32string result = wcharacter::to_u32string(sv);
    EXPECT_TRUE(result.empty());
}

TEST_F(WcharacterTest, PackageWchar) {
    wcharacter p{L'A'};
    EXPECT_EQ(p.value(), L'A');
}

TEST_F(U16characterTest, DefaultConstructor) {
    u16character c;
    EXPECT_EQ(c.value(), u'\0');
}

TEST_F(U16characterTest, ValueConstructor) {
    u16character c(u'A');
    EXPECT_EQ(c.value(), u'A');
}

TEST_F(U16characterTest, CopyConstructor) {
    u16character c1(u'A');
    u16character c2(c1);
    EXPECT_EQ(c2.value(), u'A');
}

TEST_F(U16characterTest, ValueAssignment) {
    u16character c;
    c = u'B';
    EXPECT_EQ(c.value(), u'B');
}

TEST_F(U16characterTest, ToStringAscii) {
    u16string_view sv(u"Hello");
    string result = u16character::to_string(sv);
    EXPECT_EQ(result, "Hello");
}

TEST_F(U16characterTest, ToStringWithBOM) {
    const char16_t data[] = {0xFEFF, u'H', u'i'};
    u16string_view sv(data, 3);
    string result = u16character::to_string(sv);
    EXPECT_EQ(result, "Hi");
}

TEST_F(U16characterTest, ToStringWithSwappedBOM) {
    const char16_t data[] = {0xFFFE, 0x4800, 0x6900};
    u16string_view sv(data, 3);
    string result = u16character::to_string(sv);
    EXPECT_EQ(result, "Hi");
}

TEST_F(U16characterTest, ToStringWithSurrogatePair) {
    const char16_t data[] = {0xD800, 0xDC00};
    u16string_view sv(data, 2);
    string result = u16character::to_string(sv);
    EXPECT_EQ(result.size(), 4u);
}

TEST_F(U16characterTest, ToStringEmpty) {
    u16string_view sv;
    string result = u16character::to_string(sv);
    EXPECT_TRUE(result.empty());
}

TEST_F(U16characterTest, ToWstringAscii) {
    u16string_view sv(u"Hello");
    wstring result = u16character::to_wstring(sv);
    EXPECT_EQ(result[0], L'H');
}

TEST_F(U16characterTest, ToWstringEmpty) {
    u16string_view sv;
    wstring result = u16character::to_wstring(sv);
    EXPECT_TRUE(result.empty());
}

TEST_F(U16characterTest, ToU16stringAscii) {
    u16string_view sv(u"Hello");
    u16string result = u16character::to_u16string(sv);
    EXPECT_EQ(result, u"Hello");
}

TEST_F(U16characterTest, ToU16stringEmpty) {
    u16string_view sv;
    u16string result = u16character::to_u16string(sv);
    EXPECT_TRUE(result.empty());
}

TEST_F(U16characterTest, ToU32stringAscii) {
    u16string_view sv(u"Hello");
    u32string result = u16character::to_u32string(sv);
    EXPECT_EQ(result.size(), 5u);
    EXPECT_EQ(result[0], U'H');
}

TEST_F(U16characterTest, ToU32stringEmpty) {
    u16string_view sv;
    u32string result = u16character::to_u32string(sv);
    EXPECT_TRUE(result.empty());
}

TEST_F(U16characterTest, PackageChar16) {
    u16character p(u'A');
    EXPECT_EQ(p.value(), u'A');
}

TEST_F(U32characterTest, DefaultConstructor) {
    u32character c;
    EXPECT_EQ(c.value(), U'\0');
}

TEST_F(U32characterTest, ValueConstructor) {
    u32character c(U'A');
    EXPECT_EQ(c.value(), U'A');
}

TEST_F(U32characterTest, CopyConstructor) {
    u32character c1(U'A');
    u32character c2(c1);
    EXPECT_EQ(c2.value(), U'A');
}

TEST_F(U32characterTest, ValueAssignment) {
    u32character c;
    c = U'B';
    EXPECT_EQ(c.value(), U'B');
}

TEST_F(U32characterTest, ToStringAscii) {
    u32string_view sv(U"Hello");
    string result = u32character::to_string(sv);
    EXPECT_EQ(result, "Hello");
}

TEST_F(U32characterTest, ToStringEmpty) {
    u32string_view sv;
    string result = u32character::to_string(sv);
    EXPECT_TRUE(result.empty());
}

TEST_F(U32characterTest, ToWstringAscii) {
    u32string_view sv(U"Hello");
    wstring result = u32character::to_wstring(sv);
    EXPECT_EQ(result.size(), 5u);
    EXPECT_EQ(result[0], L'H');
}

TEST_F(U32characterTest, ToWstringEmpty) {
    u32string_view sv;
    wstring result = u32character::to_wstring(sv);
    EXPECT_TRUE(result.empty());
}

TEST_F(U32characterTest, ToU16stringAscii) {
    u32string_view sv(U"Hello");
    u16string result = u32character::to_u16string(sv);
    EXPECT_EQ(result.size(), 5u);
    EXPECT_EQ(result[0], u'H');
}

TEST_F(U32characterTest, ToU16stringEmpty) {
    u32string_view sv;
    u16string result = u32character::to_u16string(sv);
    EXPECT_TRUE(result.empty());
}

TEST_F(U32characterTest, ToU32string) {
    u32string_view sv(U"Hello");
    u32string result = u32character::to_u32string(sv);
    EXPECT_EQ(result, U"Hello");
}

TEST_F(U32characterTest, ToU32stringEmpty) {
    u32string_view sv;
    u32string result = u32character::to_u32string(sv);
    EXPECT_TRUE(result.empty());
}

TEST_F(U32characterTest, PackageChar32) {
    u32character p(U'A');
    EXPECT_EQ(p.value(), U'A');
}

class FormatIntegerTest : public ::testing::Test {};
class FormatFloatTest : public ::testing::Test {};
class FormatStringTest : public ::testing::Test {};
class FormatBoolTest : public ::testing::Test {};
class FormatCharTest : public ::testing::Test {};
class FormatPointerTest : public ::testing::Test {};
class FormatOptionsTest : public ::testing::Test {};
class FormatAlignmentTest : public ::testing::Test {};
class FormatBaseTest : public ::testing::Test {};
class FormatFormatTest : public ::testing::Test {};

TEST_F(FormatIntegerTest, FormatBasicDecimal) {
    EXPECT_EQ(format("{}", 42), "42");
    EXPECT_EQ(format("{}", 0), "0");
    EXPECT_EQ(format("{}", -42), "-42");
}

TEST_F(FormatIntegerTest, FormatSignedPositive) {
    EXPECT_EQ(format("{}", 100), "100");
    EXPECT_EQ(format("{:+}", 100), "+100");
    EXPECT_EQ(format("{: }", 100), " 100");
}

TEST_F(FormatIntegerTest, FormatSignedNegative) { EXPECT_EQ(format("{:+}", -100), "-100"); }

TEST_F(FormatIntegerTest, FormatUnsigned) {
    EXPECT_EQ(format("{}", 42u), "42");
    EXPECT_EQ(format("{}", 0u), "0");
}

TEST_F(FormatIntegerTest, FormatInt64) {
    EXPECT_EQ(format("{}", static_cast<int64_t>(9223372036854775807LL)), "9223372036854775807");
}

TEST_F(FormatIntegerTest, FormatUint64) {
    EXPECT_EQ(format("{}", static_cast<uint64_t>(18446744073709551615ULL)), "18446744073709551615");
}

TEST_F(FormatIntegerTest, FormatInt8) {
    EXPECT_EQ(format("{}", static_cast<int8_t>(127)), "127");
    EXPECT_EQ(format("{}", static_cast<int8_t>(-128)), "-128");
}

TEST_F(FormatIntegerTest, FormatInt16) {
    EXPECT_EQ(format("{}", static_cast<int16_t>(32767)), "32767");
    EXPECT_EQ(format("{}", static_cast<int16_t>(-32768)), "-32768");
}

TEST_F(FormatIntegerTest, FormatInt32) {
    EXPECT_EQ(format("{}", static_cast<int32_t>(2147483647)), "2147483647");
    EXPECT_EQ(format("{}", static_cast<int32_t>(-2147483647 - 1)), "-2147483648");
}

TEST_F(FormatFloatTest, FormatDefault) {
    string result = format("{}", 3.14);
    EXPECT_FALSE(result.empty());
    EXPECT_TRUE(result.find("3.14") != string::npos);
}

TEST_F(FormatFloatTest, FormatFixed) {
    string result = format("{:.2f}", 3.14159);
    EXPECT_EQ(result, "3.14");
}

TEST_F(FormatFloatTest, FormatFixedPrecision) {
    string result = format("{:.5f}", 3.1415926535);
    EXPECT_TRUE(result.find("3.14159") != string::npos);
}

TEST_F(FormatFloatTest, FormatScientific) {
    string result = format("{:e}", 1234.5);
    EXPECT_TRUE(result.find("1.2345") != string::npos);
    EXPECT_TRUE(result.find("e") != string::npos);
}

TEST_F(FormatFloatTest, FormatScientificUppercase) {
    string result = format("{:E}", 1234.5);
    EXPECT_TRUE(result.find("E") != string::npos);
}

TEST_F(FormatFloatTest, FormatGeneral) {
    string result = format("{:g}", 1.23);
    EXPECT_TRUE(result.find("1.23") != string::npos);
}

TEST_F(FormatFloatTest, FormatNegative) {
    string result = format("{}", -3.14);
    EXPECT_TRUE(result.find("-") != string::npos);
}

TEST_F(FormatFloatTest, FormatZero) {
    string result = format("{}", 0.0);
    EXPECT_FALSE(result.empty());
}

TEST_F(FormatFloatTest, FormatDoublePrecision) {
    string result = format("{:.10f}", 1.0 / 3.0);
    EXPECT_TRUE(result.find("0.3333333333") != string::npos);
}

TEST_F(FormatStringTest, FormatBasicString) { EXPECT_EQ(format("{}", string("Hello")), "Hello"); }

TEST_F(FormatStringTest, FormatStringView) { EXPECT_EQ(format("{}", string_view("World")), "World"); }

TEST_F(FormatStringTest, FormatCString) { EXPECT_EQ(format("{}", "Hello World"), "Hello World"); }

TEST_F(FormatStringTest, FormatCStringNullptr) {
    EXPECT_EQ(format("{}", static_cast<const char*>(nullptr)), "nullptr");
}

TEST_F(FormatStringTest, FormatStringWithPrecision) { EXPECT_EQ(format("{:.3}", string("Hello World")), "Hel"); }

TEST_F(FormatStringTest, FormatEmptyString) { EXPECT_EQ(format("{}", string()), ""); }

TEST_F(FormatBoolTest, FormatTrue) { EXPECT_EQ(format("{}", true), "true"); }

TEST_F(FormatBoolTest, FormatFalse) { EXPECT_EQ(format("{}", false), "false"); }

TEST_F(FormatBoolTest, FormatTrueAsDecimal) { EXPECT_EQ(format("{:d}", true), "1"); }

TEST_F(FormatBoolTest, FormatFalseAsDecimal) { EXPECT_EQ(format("{:d}", false), "0"); }

TEST_F(FormatBoolTest, FormatTrueAsHex) { EXPECT_EQ(format("{:x}", true), "1"); }

TEST_F(FormatCharTest, FormatAsciiChar) { EXPECT_EQ(format("{}", 'A'), "A"); }

TEST_F(FormatCharTest, FormatDigitChar) { EXPECT_EQ(format("{}", '5'), "5"); }

TEST_F(FormatCharTest, FormatCharAsDecimal) { EXPECT_EQ(format("{:d}", 'A'), "65"); }

TEST_F(FormatCharTest, FormatCharAsHex) { EXPECT_EQ(format("{:x}", 'A'), "41"); }

TEST_F(FormatCharTest, FormatCharAsOctal) { EXPECT_EQ(format("{:o}", 'A'), "101"); }

TEST_F(FormatPointerTest, FormatNullptr) { EXPECT_EQ(format("{}", nullptr), "nullptr"); }

TEST_F(FormatPointerTest, FormatNonNullPointer) {
    int x = 42;
    string result = format("{}", &x);
    EXPECT_FALSE(result.empty());
    EXPECT_NE(result, "nullptr");
}

TEST_F(FormatPointerTest, FormatCharPointer) {
    char str[] = "Hello";
    char* ptr = str;
    EXPECT_EQ(format("{}", ptr), "Hello");
}

TEST_F(FormatOptionsTest, FillChar) { EXPECT_EQ(format("{:*>5}", "ab"), "***ab"); }

TEST_F(FormatOptionsTest, WidthAndAlignment) {
    EXPECT_EQ(format("{:<5}", "ab"), "ab   ");
    EXPECT_EQ(format("{:>5}", "ab"), "   ab");
}

TEST_F(FormatOptionsTest, ZeroPadInteger) { EXPECT_EQ(format("{:05}", 42), "00042"); }

TEST_F(FormatOptionsTest, ShowSign) { EXPECT_EQ(format("{:+}", 42), "+42"); }

TEST_F(FormatOptionsTest, SpaceSign) { EXPECT_EQ(format("{: }", 42), " 42"); }

TEST_F(FormatOptionsTest, Precision) { EXPECT_EQ(format("{:.3}", "Hello"), "Hel"); }

TEST_F(FormatAlignmentTest, LeftAlign) { EXPECT_EQ(format("{:<10}", "Hi"), "Hi        "); }

TEST_F(FormatAlignmentTest, RightAlign) { EXPECT_EQ(format("{:>10}", "Hi"), "        Hi"); }

TEST_F(FormatAlignmentTest, CenterAlign) { EXPECT_EQ(format("{:^10}", "Hi"), "    Hi    "); }

TEST_F(FormatAlignmentTest, CenterAlignOddPad) { EXPECT_EQ(format("{:^5}", "Hi"), " Hi  "); }

TEST_F(FormatAlignmentTest, SignAwarePadding) { EXPECT_EQ(format("{:=+5}", 3), "+   3"); }

TEST_F(FormatAlignmentTest, ZeroPadWithSign) { EXPECT_EQ(format("{:+05}", 42), "+0042"); }

TEST_F(FormatAlignmentTest, NegativeNumericAlign) {
    string result = format("{:=5}", -3);
    EXPECT_TRUE(result.find("-") != string::npos);
}

TEST_F(FormatBaseTest, HexLowercase) { EXPECT_EQ(format("{:x}", 255), "ff"); }

TEST_F(FormatBaseTest, HexUppercase) { EXPECT_EQ(format("{:X}", 255), "FF"); }

TEST_F(FormatBaseTest, HexAlternate) {
    EXPECT_EQ(format("{:#x}", 255), "0xff");
    EXPECT_EQ(format("{:#X}", 255), "0XFF");
}

TEST_F(FormatBaseTest, Octal) { EXPECT_EQ(format("{:o}", 8), "10"); }

TEST_F(FormatBaseTest, OctalAlternate) { EXPECT_EQ(format("{:#o}", 8), "010"); }

TEST_F(FormatBaseTest, Binary) { EXPECT_EQ(format("{:b}", 5), "101"); }

TEST_F(FormatBaseTest, BinaryUppercase) { EXPECT_EQ(format("{:B}", 5), "101"); }

TEST_F(FormatBaseTest, BinaryAlternate) {
    EXPECT_EQ(format("{:#b}", 5), "0b101");
    EXPECT_EQ(format("{:#B}", 5), "0B101");
}

TEST_F(FormatBaseTest, ZeroHex) { EXPECT_EQ(format("{:x}", 0), "0"); }

TEST_F(FormatBaseTest, ZeroOctal) { EXPECT_EQ(format("{:o}", 0), "0"); }

TEST_F(FormatBaseTest, ZeroBinary) { EXPECT_EQ(format("{:b}", 0), "0"); }

TEST_F(FormatFormatTest, MultipleArguments) { EXPECT_EQ(format("{} {}", 1, 2), "1 2"); }

TEST_F(FormatFormatTest, ThreeArguments) { EXPECT_EQ(format("{}, {}, {}", "a", "b", "c"), "a, b, c"); }

TEST_F(FormatFormatTest, MixedTypes) { EXPECT_EQ(format("{} {:.2f} {}", 42, 3.14, "hello"), "42 3.14 hello"); }

TEST_F(FormatFormatTest, EscapedBraces) { EXPECT_EQ(format("{{Hello}} {}", "World"), "{Hello} World"); }

TEST_F(FormatFormatTest, FormatWithSpec) { EXPECT_EQ(format("{:>5}", 42), "   42"); }

TEST_F(FormatFormatTest, MultipleFormatsWithSpecs) { EXPECT_EQ(format("{:>5} {:<5}", "a", "b"), "    a b    "); }

TEST_F(FormatFormatTest, ComplexFormat) {
    string result = format("{:*^10} {:04} {:.2f}", "HI", 7, 3.14159);
    EXPECT_EQ(result, "****HI**** 0007 3.14");
}

TEST_F(FormatFormatTest, MoveOnlyArguments) {
    string s = "test";
    string result = format("{}", move(s));
    EXPECT_EQ(result, "test");
}
