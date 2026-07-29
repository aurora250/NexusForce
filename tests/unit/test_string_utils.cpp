#include <NeForce/core/string/utf.hpp>
#include <NeForce/core/string/regex.hpp>
#include <NeForce/core/utility/packages.hpp>
#include <NeForce/core/string/utf_iterator.hpp>
#include <NeForce/core/string/charset.hpp>
#include <NeForce/core/string/string_builder.hpp>
#include <gtest/gtest.h>
using namespace neforce;

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

class FormatPositionalTest : public ::testing::Test {};
class FormatNamedTest : public ::testing::Test {};
class RegexCopyTest : public ::testing::Test {};
class UTF8IteratorTest : public ::testing::Test {};

TEST_F(FormatPositionalTest, PositionalBasic) {
    EXPECT_EQ(format("{0} {1}", "a", "b"), "a b");
    EXPECT_EQ(format("{1} {0}", "a", "b"), "b a");
}

TEST_F(FormatPositionalTest, PositionalRepeated) {
    EXPECT_EQ(format("{0} {0} {0}", "x"), "x x x");
    EXPECT_EQ(format("{0}{1}{0}", "a", "b"), "aba");
}

TEST_F(FormatPositionalTest, PositionalWithFormatOptions) {
    EXPECT_EQ(format("{0:d} {1:x}", 42, 255), "42 ff");
    EXPECT_EQ(format("{0:x} {1:d}", 255, 42), "ff 42");
    EXPECT_EQ(format("{0:#x} {1:04}", 255, 7), "0xff 0007");
}

TEST_F(FormatPositionalTest, PositionalOutOfRange) { EXPECT_THROW(ignore = format("{5}", 1, 2), value_exception); }

TEST_F(FormatPositionalTest, MixedPositionalAndSequential) { EXPECT_EQ(format("{1} {} {}", "a", "b", "c"), "b a b"); }

TEST_F(FormatPositionalTest, PositionalWithEmptySpec) { EXPECT_EQ(format("{} {} {}", "a", "b", "c"), "a b c"); }

TEST_F(FormatPositionalTest, PositionalSequentialFormatOption) { EXPECT_EQ(format("{:d} {:x}", 42, 255), "42 ff"); }

TEST_F(FormatNamedTest, BasicSubstitution) {
    EXPECT_EQ(format_named("{greeting}, {name}!", {{"greeting", "Hello"}, {"name", "World"}}), "Hello, World!");
}

TEST_F(FormatNamedTest, SingleParam) { EXPECT_EQ(format_named("{value}", {{"value", "test"}}), "test"); }

TEST_F(FormatNamedTest, UnknownNameCopiedAsIs) {
    EXPECT_EQ(format_named("{known} {unknown}", {{"known", "val"}}), "val {unknown}");
}

TEST_F(FormatNamedTest, NoParams) { EXPECT_EQ(format_named("Hello World", {}), "Hello World"); }

TEST_F(FormatNamedTest, EscapedBraces) {
    EXPECT_EQ(format_named("{{not_substituted}}", {{"not_substituted", "val"}}), "{not_substituted}");
}

TEST_F(FormatNamedTest, EmptyName) { EXPECT_EQ(format_named("{}", {{"", "empty"}}), "empty"); }

TEST_F(RegexCopyTest, CopyConstructor) {
    regex re1("(\\w+)=(\\d+)");
    regex re2(re1);
    EXPECT_TRUE(re2.valid());
    EXPECT_EQ(re2.pattern(), "(\\w+)=(\\d+)");
    EXPECT_EQ(re2.capture_count(), 2);
    auto result = re2.search("name=123");
    EXPECT_TRUE(result.matched());
    EXPECT_EQ(result[1], "name");
}

TEST_F(RegexCopyTest, CopyAssignment) {
    regex re1("(\\w+)=(\\d+)");
    regex re2("other");
    re2 = re1;
    EXPECT_TRUE(re2.valid());
    EXPECT_EQ(re2.pattern(), "(\\w+)=(\\d+)");
    auto result = re2.search("key=456");
    EXPECT_TRUE(result.matched());
    EXPECT_EQ(result[2], "456");
}

TEST_F(RegexCopyTest, CopyAssignmentSelf) {
    regex re("hello");
    re = re;
    EXPECT_TRUE(re.valid());
    EXPECT_TRUE(re.match("hello"));
}

TEST_F(RegexCopyTest, CopyEmptyPattern) {
    regex re1("^$");
    regex re2(re1);
    EXPECT_TRUE(re2.valid());
    EXPECT_TRUE(re2.match(""));
}

TEST_F(RegexCopyTest, CopyAssignmentEmptyPatternToValid) {
    regex re1("^$");
    regex re2("hello");
    re2 = re1;
    EXPECT_TRUE(re2.valid());
    EXPECT_TRUE(re2.match(""));
    EXPECT_FALSE(re2.match("hello"));
}

TEST_F(UTF8IteratorTest, EmptyRange) {
    size_t count = 0;
    for (codepoint cp: utf8_range("")) {
        ignore = cp;
        ++count;
    }
    EXPECT_EQ(count, 0u);
}

TEST_F(UTF8IteratorTest, AsciiOnly) {
    vector<uint32_t> values;
    for (codepoint cp: utf8_range("ABC")) {
        values.push_back(cp.value());
    }
    ASSERT_EQ(values.size(), 3u);
    EXPECT_EQ(values[0], 0x41u);
    EXPECT_EQ(values[1], 0x42u);
    EXPECT_EQ(values[2], 0x43u);
}

TEST_F(UTF8IteratorTest, CJKCharacters) {
    string_view sv("\xE4\xB8\x96\xE7\x95\x8C");
    vector<uint32_t> values;
    for (codepoint cp: utf8_range(sv)) {
        values.push_back(cp.value());
    }
    ASSERT_EQ(values.size(), 2u);
    EXPECT_EQ(values[0], 0x4E16u);
    EXPECT_EQ(values[1], 0x754Cu);
}

TEST_F(UTF8IteratorTest, MixedAsciiAndMultiByte) {
    string hello_world = "Hello \xE4\xB8\x96\xE7\x95\x8C";
    vector<uint32_t> values;
    for (codepoint cp: utf8_range(hello_world.view())) {
        values.push_back(cp.value());
    }
    ASSERT_EQ(values.size(), 8u);
    EXPECT_EQ(values[0], 0x48u);
    EXPECT_EQ(values[5], 0x20u);
    EXPECT_EQ(values[6], 0x4E16u);
    EXPECT_EQ(values[7], 0x754Cu);
}

TEST_F(UTF8IteratorTest, InvalidSequenceYieldsReplacement) {
    const byte_t data[] = {0xFF, 0x41};
    size_t count = 0;
    for (codepoint cp: utf8_range(data, 2)) {
        if (count == 0) {
            EXPECT_TRUE(cp.is_replacement());
        } else {
            EXPECT_EQ(cp.value(), 0x41u);
        }
        ++count;
    }
    EXPECT_EQ(count, 2u);
}

TEST_F(UTF8IteratorTest, TruncatedSequence) {
    const byte_t data[] = {0xE2, 0x82};
    size_t count = 0;
    for (codepoint cp: utf8_range(data, 2)) {
        EXPECT_TRUE(cp.is_replacement());
        ++count;
    }
    EXPECT_EQ(count, 2u);
}

TEST_F(UTF8IteratorTest, IteratorComparison) {
    auto range = utf8_range("AB");
    auto it = range.begin();
    auto end = range.end();
    EXPECT_NE(it, end);
    ++it;
    EXPECT_NE(it, end);
    ++it;
    EXPECT_EQ(it, end);
}

TEST_F(UTF8IteratorTest, PostfixIncrement) {
    auto range = utf8_range("AB");
    auto it = range.begin();
    auto prev = it++;
    EXPECT_EQ(prev->value(), 0x41u);
    EXPECT_EQ(it->value(), 0x42u);
}

TEST_F(UTF8IteratorTest, ArrowOperator) {
    auto range = utf8_range("X");
    auto it = range.begin();
    EXPECT_EQ(it->value(), 0x58u);
    EXPECT_TRUE(it->is_ascii());
}

class CharsetTest : public ::testing::Test {};
class StringBuilderTest : public ::testing::Test {};
class CharsetIntegrationTest : public ::testing::Test {};

TEST_F(CharsetTest, DefaultConstructor) {
    charset cs;
    EXPECT_TRUE(cs.empty());
    for (int i = 0; i < 256; ++i) {
        EXPECT_FALSE(cs.contains(static_cast<char>(i)));
    }
}

TEST_F(CharsetTest, FromChar) {
    auto cs = charset::from_char('A');
    EXPECT_TRUE(cs.contains('A'));
    EXPECT_FALSE(cs.contains('B'));
    EXPECT_FALSE(cs.empty());
}

TEST_F(CharsetTest, FromCharBoundary) {
    auto cs0 = charset::from_char(static_cast<char>(0));
    EXPECT_TRUE(cs0.contains(static_cast<char>(0)));
    EXPECT_FALSE(cs0.contains(static_cast<char>(1)));

    auto cs255 = charset::from_char(static_cast<char>(255));
    EXPECT_TRUE(cs255.contains(static_cast<char>(255)));
    EXPECT_FALSE(cs255.contains(static_cast<char>(254)));
}

TEST_F(CharsetTest, Range) {
    auto cs = charset::range('a', 'z');
    for (int c = 'a'; c <= 'z'; ++c) {
        EXPECT_TRUE(cs.contains(static_cast<char>(c)));
    }
    EXPECT_FALSE(cs.contains('A'));
    EXPECT_FALSE(cs.contains('0'));
    EXPECT_FALSE(cs.contains(static_cast<char>(0)));
}

TEST_F(CharsetTest, RangeSingle) {
    auto cs = charset::range('X', 'X');
    EXPECT_TRUE(cs.contains('X'));
    EXPECT_FALSE(cs.contains('W'));
    EXPECT_FALSE(cs.contains('Y'));
}

TEST_F(CharsetTest, RangeReversed) {
    auto cs = charset::range('z', 'a');
    EXPECT_TRUE(cs.empty());
}

TEST_F(CharsetTest, RangeFull) {
    auto cs = charset::range(static_cast<char>(0), static_cast<char>(255));
    for (int i = 0; i < 256; ++i) {
        EXPECT_TRUE(cs.contains(static_cast<char>(i)));
    }
}

TEST_F(CharsetTest, RangeBoundaryWords) {
    {
        auto cs = charset::range(static_cast<char>(63), static_cast<char>(64));
        EXPECT_TRUE(cs.contains(static_cast<char>(63)));
        EXPECT_TRUE(cs.contains(static_cast<char>(64)));
        EXPECT_FALSE(cs.contains(static_cast<char>(62)));
        EXPECT_FALSE(cs.contains(static_cast<char>(65)));
    }
    {
        auto cs = charset::range(static_cast<char>(127), static_cast<char>(128));
        EXPECT_TRUE(cs.contains(static_cast<char>(127)));
        EXPECT_TRUE(cs.contains(static_cast<char>(128)));
        EXPECT_FALSE(cs.contains(static_cast<char>(126)));
        EXPECT_FALSE(cs.contains(static_cast<char>(129)));
    }
    {
        auto cs = charset::range(static_cast<char>(191), static_cast<char>(192));
        EXPECT_TRUE(cs.contains(static_cast<char>(191)));
        EXPECT_TRUE(cs.contains(static_cast<char>(192)));
        EXPECT_FALSE(cs.contains(static_cast<char>(190)));
        EXPECT_FALSE(cs.contains(static_cast<char>(193)));
    }
}

TEST_F(CharsetTest, Insert) {
    charset cs;
    cs.insert('!');
    EXPECT_TRUE(cs.contains('!'));
    EXPECT_FALSE(cs.empty());
    cs.insert('!');
    EXPECT_TRUE(cs.contains('!'));
}

TEST_F(CharsetTest, Erase) {
    auto cs = charset::range('a', 'e');
    cs.erase('c');
    EXPECT_TRUE(cs.contains('a'));
    EXPECT_TRUE(cs.contains('b'));
    EXPECT_FALSE(cs.contains('c'));
    EXPECT_TRUE(cs.contains('d'));
    EXPECT_TRUE(cs.contains('e'));
    cs.erase('z');
    EXPECT_FALSE(cs.contains('z'));
}

TEST_F(CharsetTest, Union) {
    auto a = charset::from_char('A');
    auto b = charset::from_char('B');
    auto u = a | b;
    EXPECT_TRUE(u.contains('A'));
    EXPECT_TRUE(u.contains('B'));
    EXPECT_FALSE(u.contains('C'));
}

TEST_F(CharsetTest, Intersection) {
    auto lower = charset::range('a', 'z');
    auto vowels = charset::from_char('a') | charset::from_char('e') | charset::from_char('i') |
                  charset::from_char('o') | charset::from_char('u');
    auto result = lower & vowels;
    EXPECT_TRUE(result.contains('a'));
    EXPECT_TRUE(result.contains('e'));
    EXPECT_FALSE(result.contains('b'));
}

TEST_F(CharsetTest, Complement) {
    auto digits = charset::ascii_digit();
    auto non_digits = ~digits;
    EXPECT_FALSE(non_digits.contains('0'));
    EXPECT_FALSE(non_digits.contains('9'));
    EXPECT_TRUE(non_digits.contains('a'));
    EXPECT_TRUE(non_digits.contains('!'));
}

TEST_F(CharsetTest, Difference) {
    auto alnum = charset::ascii_alnum();
    auto digits = charset::ascii_digit();
    auto alpha = alnum - digits;
    EXPECT_TRUE(alpha.contains('a'));
    EXPECT_TRUE(alpha.contains('Z'));
    EXPECT_FALSE(alpha.contains('0'));
    EXPECT_FALSE(alpha.contains('9'));
}

TEST_F(CharsetTest, CompoundAssign) {
    auto cs = charset::from_char('A');
    cs |= charset::from_char('B');
    EXPECT_TRUE(cs.contains('A'));
    EXPECT_TRUE(cs.contains('B'));

    cs &= charset::from_char('B');
    EXPECT_FALSE(cs.contains('A'));
    EXPECT_TRUE(cs.contains('B'));

    cs -= charset::from_char('B');
    EXPECT_TRUE(cs.empty());
}

TEST_F(CharsetTest, Equality) {
    auto a = charset::range('0', '9');
    auto b = charset::range('0', '9');
    auto c = charset::range('a', 'z');
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
    EXPECT_EQ(a, a);
}

TEST_F(CharsetTest, PredefinedAlpha) {
    auto cs = charset::ascii_alpha_lower();
    EXPECT_TRUE(cs.contains('a'));
    EXPECT_TRUE(cs.contains('z'));
    EXPECT_FALSE(cs.contains('A'));
    EXPECT_FALSE(cs.contains('Z'));
    EXPECT_FALSE(cs.contains('0'));
}

TEST_F(CharsetTest, PredefinedAlphaUpper) {
    auto cs = charset::ascii_alpha_upper();
    EXPECT_TRUE(cs.contains('A'));
    EXPECT_TRUE(cs.contains('Z'));
    EXPECT_FALSE(cs.contains('a'));
    EXPECT_FALSE(cs.contains('z'));
}

TEST_F(CharsetTest, PredefinedAlphaCombined) {
    auto cs = charset::ascii_alpha();
    EXPECT_TRUE(cs.contains('a'));
    EXPECT_TRUE(cs.contains('z'));
    EXPECT_TRUE(cs.contains('A'));
    EXPECT_TRUE(cs.contains('Z'));
    EXPECT_FALSE(cs.contains('0'));
    EXPECT_FALSE(cs.contains('9'));
}

TEST_F(CharsetTest, PredefinedDigit) {
    auto cs = charset::ascii_digit();
    EXPECT_TRUE(cs.contains('0'));
    EXPECT_TRUE(cs.contains('9'));
    EXPECT_FALSE(cs.contains('a'));
    EXPECT_FALSE(cs.contains('/'));
    EXPECT_FALSE(cs.contains(':'));
}

TEST_F(CharsetTest, PredefinedAlnum) {
    auto cs = charset::ascii_alnum();
    EXPECT_TRUE(cs.contains('a'));
    EXPECT_TRUE(cs.contains('Z'));
    EXPECT_TRUE(cs.contains('0'));
    EXPECT_TRUE(cs.contains('9'));
    EXPECT_FALSE(cs.contains('!'));
    EXPECT_FALSE(cs.contains(' '));
}

TEST_F(CharsetTest, PredefinedHexDigit) {
    auto cs = charset::ascii_hex_digit();
    EXPECT_TRUE(cs.contains('0'));
    EXPECT_TRUE(cs.contains('9'));
    EXPECT_TRUE(cs.contains('A'));
    EXPECT_TRUE(cs.contains('F'));
    EXPECT_TRUE(cs.contains('a'));
    EXPECT_TRUE(cs.contains('f'));
    EXPECT_FALSE(cs.contains('G'));
    EXPECT_FALSE(cs.contains('g'));
}

TEST_F(CharsetTest, PredefinedBlank) {
    auto cs = charset::ascii_blank();
    EXPECT_TRUE(cs.contains(' '));
    EXPECT_TRUE(cs.contains('\t'));
    EXPECT_FALSE(cs.contains('\n'));
    EXPECT_FALSE(cs.contains('a'));
}

TEST_F(CharsetTest, PredefinedSpace) {
    auto cs = charset::ascii_space();
    EXPECT_TRUE(cs.contains(' '));
    EXPECT_TRUE(cs.contains('\t'));
    EXPECT_TRUE(cs.contains('\n'));
    EXPECT_TRUE(cs.contains('\v'));
    EXPECT_TRUE(cs.contains('\f'));
    EXPECT_TRUE(cs.contains('\r'));
    EXPECT_FALSE(cs.contains('a'));
    EXPECT_FALSE(cs.contains('0'));
}

TEST_F(CharsetTest, PredefinedPunct) {
    auto cs = charset::ascii_punct();
    EXPECT_TRUE(cs.contains('!'));
    EXPECT_TRUE(cs.contains('@'));
    EXPECT_TRUE(cs.contains('{'));
    EXPECT_TRUE(cs.contains('~'));
    EXPECT_FALSE(cs.contains('a'));
    EXPECT_FALSE(cs.contains('0'));
    EXPECT_FALSE(cs.contains(' '));
}

TEST_F(CharsetTest, PredefinedCntrl) {
    auto cs = charset::ascii_cntrl();
    EXPECT_TRUE(cs.contains(static_cast<char>(0)));
    EXPECT_TRUE(cs.contains(static_cast<char>(31)));
    EXPECT_TRUE(cs.contains(static_cast<char>(127)));
    EXPECT_FALSE(cs.contains(' '));
    EXPECT_FALSE(cs.contains('A'));
}

TEST_F(CharsetTest, PredefinedPrint) {
    auto cs = charset::ascii_print();
    EXPECT_TRUE(cs.contains(' '));
    EXPECT_TRUE(cs.contains('~'));
    EXPECT_TRUE(cs.contains('A'));
    EXPECT_FALSE(cs.contains(static_cast<char>(0)));
    EXPECT_FALSE(cs.contains(static_cast<char>(31)));
    EXPECT_FALSE(cs.contains(static_cast<char>(127)));
}

TEST_F(CharsetTest, PredefinedGraph) {
    auto cs = charset::ascii_graph();
    EXPECT_TRUE(cs.contains('!'));
    EXPECT_TRUE(cs.contains('~'));
    EXPECT_TRUE(cs.contains('A'));
    EXPECT_FALSE(cs.contains(' '));
    EXPECT_FALSE(cs.contains('\t'));
    EXPECT_FALSE(cs.contains(static_cast<char>(0)));
}

TEST_F(CharsetTest, PredefinedEmptySet) {
    auto cs = charset::empty_set();
    EXPECT_TRUE(cs.empty());
    EXPECT_FALSE(cs.contains('A'));
    EXPECT_FALSE(cs.contains(static_cast<char>(0)));
}

TEST_F(CharsetTest, PredefinedUniverse) {
    auto cs = charset::universe();
    EXPECT_FALSE(cs.empty());
    for (int i = 0; i < 256; ++i) {
        EXPECT_TRUE(cs.contains(static_cast<char>(i)));
    }
}

TEST_F(CharsetTest, IdentityLaws) {
    auto cs = charset::ascii_alpha();
    EXPECT_EQ(cs | charset::empty_set(), cs);
    EXPECT_EQ(cs & charset::universe(), cs);
    EXPECT_EQ(cs & charset::empty_set(), charset::empty_set());
    EXPECT_EQ(cs | charset::universe(), charset::universe());
    EXPECT_EQ(~~cs, cs);
}

TEST_F(StringBuilderTest, Empty) {
    string_builder sb;
    EXPECT_TRUE(sb.empty());
    EXPECT_EQ(sb.size(), 0u);
    EXPECT_EQ(sb.build(), "");
}

TEST_F(StringBuilderTest, AppendStringView) {
    string_builder sb;
    sb.append(string_view("hello"));
    EXPECT_EQ(sb.size(), 5u);
    EXPECT_FALSE(sb.empty());
    EXPECT_EQ(sb.build(), "hello");
}

TEST_F(StringBuilderTest, AppendString) {
    string_builder sb;
    string s = "world";
    sb.append(s);
    EXPECT_EQ(sb.build(), "world");
}

TEST_F(StringBuilderTest, AppendCharPtr) {
    string_builder sb;
    sb.append("test");
    EXPECT_EQ(sb.build(), "test");
}

TEST_F(StringBuilderTest, AppendChar) {
    string_builder sb;
    sb.append('X');
    EXPECT_EQ(sb.size(), 1u);
    EXPECT_EQ(sb.build(), "X");
}

TEST_F(StringBuilderTest, AppendMultipleTypes) {
    string_builder sb;
    sb.append("Hello, ");
    sb.append(string_view("this is "));
    string s = "a test";
    sb.append(s);
    sb.append('.');
    EXPECT_EQ(sb.build(), "Hello, this is a test.");
}

TEST_F(StringBuilderTest, BuildEmpty) {
    string_builder sb;
    string result = sb.build();
    EXPECT_TRUE(result.empty());
}

TEST_F(StringBuilderTest, ImplicitConversion) {
    string_builder sb;
    sb.append("test");
    string result = sb;
    EXPECT_EQ(result, "test");
}

TEST_F(StringBuilderTest, SizeTracking) {
    string_builder sb;
    sb.append("abc");
    EXPECT_EQ(sb.size(), 3u);
    sb.append("def");
    EXPECT_EQ(sb.size(), 6u);
    sb.append('!');
    EXPECT_EQ(sb.size(), 7u);
}

TEST_F(StringBuilderTest, Clear) {
    string_builder sb;
    sb.append("first");
    sb.clear();
    EXPECT_TRUE(sb.empty());
    EXPECT_EQ(sb.size(), 0u);
    sb.append("second");
    EXPECT_EQ(sb.build(), "second");
}

TEST_F(StringBuilderTest, ReservePieces) {
    string_builder sb;
    sb.reserve_pieces(100);
    for (int i = 0; i < 100; ++i) {
        sb.append('.');
    }
    EXPECT_EQ(sb.size(), 100u);
}

TEST_F(StringBuilderTest, BuildWithReserveOutput) {
    string_builder sb;
    sb.reserve(1000);
    sb.append("short");
    string result = sb.build();
    EXPECT_EQ(result, "short");
}

TEST_F(StringBuilderTest, LargeNumberOfPieces) {
    string_builder sb;
    string expected;
    for (int i = 0; i < 1000; ++i) {
        sb.append("a");
        expected += "a";
    }
    EXPECT_EQ(sb.size(), 1000u);
    EXPECT_EQ(sb.build(), expected);
}

TEST_F(StringBuilderTest, Concatenate) {
    string result = concatenate("Hello, ", string_view("this is "), "a char");
    EXPECT_EQ(result, "Hello, this is a char");
}

TEST_F(StringBuilderTest, ConcatenateWithChar) {
    string result = concatenate("A", "B", "C");
    EXPECT_EQ(result, "ABC");
}

TEST_F(StringBuilderTest, ConcatenateMixed) {
    string s = "world";
    string result = concatenate("hello", string_view(" "), s);
    EXPECT_EQ(result, "hello world");
}

TEST_F(CharsetIntegrationTest, StringViewFindFirstOf) {
    string_view sv = "hello123world";
    EXPECT_EQ(sv.find_first_of(charset::ascii_digit()), 5u);
}

TEST_F(CharsetIntegrationTest, StringViewFindFirstOfNotFound) {
    string_view sv = "hello world";
    EXPECT_EQ(sv.find_first_of(charset::ascii_digit()), string_view::npos);
}

TEST_F(CharsetIntegrationTest, StringViewFindLastOf) {
    string_view sv = "hello123world456";
    EXPECT_EQ(sv.find_last_of(charset::ascii_digit()), 15u);
}

TEST_F(CharsetIntegrationTest, StringViewFindFirstNotOf) {
    string_view sv = "123abc";
    EXPECT_EQ(sv.find_first_not_of(charset::ascii_digit()), 3u);
}

TEST_F(CharsetIntegrationTest, StringViewFindLastNotOf) {
    string_view sv = "abc123";
    EXPECT_EQ(sv.find_last_not_of(charset::ascii_digit()), 2u);
}

TEST_F(CharsetIntegrationTest, StringViewTrimLeft) {
    string_view sv = "  \t\nhello";
    auto result = sv.trim_left();
    EXPECT_EQ(result, "hello");
}

TEST_F(CharsetIntegrationTest, StringViewTrimRight) {
    string_view sv = "hello  \t\n";
    auto result = sv.trim_right();
    EXPECT_EQ(result, "hello");
}

TEST_F(CharsetIntegrationTest, StringViewTrim) {
    string_view sv = "  hello  ";
    auto result = sv.trim();
    EXPECT_EQ(result, "hello");
}

TEST_F(CharsetIntegrationTest, StringViewTrimLeftIfCharset) {
    string_view sv = "!@#hello";
    auto punct = charset::ascii_punct();
    auto result = sv.trim_left_if(punct);
    EXPECT_EQ(result, "hello");
}

TEST_F(CharsetIntegrationTest, StringViewTrimRightIfCharset) {
    string_view sv = "hello!@#";
    auto punct = charset::ascii_punct();
    auto result = sv.trim_right_if(punct);
    EXPECT_EQ(result, "hello");
}

TEST_F(CharsetIntegrationTest, StringViewTrimIfCharset) {
    string_view sv = "!@hello!@";
    auto punct = charset::ascii_punct();
    auto result = sv.trim_if(punct);
    EXPECT_EQ(result, "hello");
}

TEST_F(CharsetIntegrationTest, StringFindFirstOf) {
    string s = "hello123world";
    EXPECT_EQ(s.find_first_of(charset::ascii_digit()), 5u);
}

TEST_F(CharsetIntegrationTest, StringFindFirstNotOf) {
    string s = "123abc";
    EXPECT_EQ(s.find_first_not_of(charset::ascii_digit()), 3u);
}

TEST_F(CharsetIntegrationTest, StringTrim) {
    string s = "  hello  ";
    s.trim();
    EXPECT_EQ(s, "hello");
}

TEST_F(CharsetIntegrationTest, StringTrimLeftIfCharset) {
    string s = "!@#hello";
    s.trim_left_if(charset::ascii_punct());
    EXPECT_EQ(s, "hello");
}

TEST_F(CharsetIntegrationTest, StringTrimRightIfCharset) {
    string s = "hello!@#";
    s.trim_right_if(charset::ascii_punct());
    EXPECT_EQ(s, "hello");
}

TEST_F(CharsetIntegrationTest, StringSplitCharset) {
    string s = "alpha,beta;gamma|delta";
    auto delim = charset::from_char(',') | charset::from_char(';') | charset::from_char('|');
    auto parts = s.split(delim);
    EXPECT_EQ(parts.size(), 4u);
    EXPECT_EQ(parts[0], "alpha");
    EXPECT_EQ(parts[1], "beta");
    EXPECT_EQ(parts[2], "gamma");
    EXPECT_EQ(parts[3], "delta");
}

TEST_F(CharsetIntegrationTest, StringSplitCharsetSkipEmpty) {
    string s = "a,,b;;c";
    auto delim = charset::from_char(',') | charset::from_char(';');
    auto parts = s.split(delim, true);
    EXPECT_EQ(parts.size(), 3u);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[1], "b");
    EXPECT_EQ(parts[2], "c");
}

TEST_F(CharsetIntegrationTest, StringSplitCharsetKeepEmpty) {
    string s = "a,,b;;c";
    auto delim = charset::from_char(',') | charset::from_char(';');
    auto parts = s.split(delim, false);
    EXPECT_EQ(parts.size(), 5u);
    EXPECT_EQ(parts[2], "b");
}
