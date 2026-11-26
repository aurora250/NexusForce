#include <NeForce/core/string/regex.hpp>
#include <NeForce/core/string/string.hpp>
#include <gtest/gtest.h>
using namespace neforce;

namespace {
    constexpr regex_option unicode_mode = regex_option::utf | regex_option::ucp;
    constexpr regex_option utf_mode = regex_option::utf;
} // namespace

class RegexLookaroundTest : public ::testing::Test {};

class RegexBackreferenceTest : public ::testing::Test {};

class RegexRecursionTest : public ::testing::Test {};

class RegexUnicodeTest : public ::testing::Test {};

class RegexQuantifierTest : public ::testing::Test {};

class RegexGroupTest : public ::testing::Test {};

class RegexAnchorTest : public ::testing::Test {};

class RegexClassTest : public ::testing::Test {};

class RegexEscapeTest : public ::testing::Test {};

class RegexIntegrationTest : public ::testing::Test {};

TEST_F(RegexLookaroundTest, FixedLookbehind) {
    regex re("(?<=USD)\\d+");
    auto result = re.search("USD100");
    ASSERT_TRUE(result.matched());
    EXPECT_EQ(result.data(), "100");
    EXPECT_EQ(result.position(), 3u);
}

TEST_F(RegexLookaroundTest, NegativeLookbehind) {
    regex re("(?<!foo)bar");
    auto result = re.search("foobar bar baz");
    ASSERT_TRUE(result.matched());
    EXPECT_EQ(result.position(), 7u);
}

TEST_F(RegexLookaroundTest, LookbehindAlternatives) {
    regex re("(?<=ab|c)X");
    auto result = re.search("cX");
    ASSERT_TRUE(result.matched());
    EXPECT_EQ(result.position(), 1u);
}

TEST_F(RegexLookaroundTest, LookbehindAtSubjectStart) {
    regex re("(?<=x)a");
    EXPECT_FALSE(re.search("a").matched());
}

TEST_F(RegexLookaroundTest, PositiveLookahead) {
    regex re("\\d+(?= dollars)");
    auto result = re.search("100 dollars and 5 euros");
    ASSERT_TRUE(result.matched());
    EXPECT_EQ(result.data(), "100");
}

TEST_F(RegexLookaroundTest, NegativeLookahead) {
    regex re("foo(?!bar)");
    auto result = re.search("foobar foobaz");
    ASSERT_TRUE(result.matched());
    EXPECT_EQ(result.position(), 7u);
}

TEST_F(RegexLookaroundTest, NestedLookaround) {
    regex re("(?=.*\\d)(?=.*[a-z])\\w+");
    EXPECT_TRUE(re.match("abc123"));
    EXPECT_FALSE(re.match("abcdef"));
    EXPECT_FALSE(re.match("123456"));
}

TEST_F(RegexLookaroundTest, LookaheadCaptureVisible) {
    regex re("(?=(\\d+))\\d+");
    auto result = re.search("12345");
    ASSERT_TRUE(result.matched());
    EXPECT_EQ(result[1], "12345");
}

TEST_F(RegexBackreferenceTest, NumericBackreference) {
    regex re("(\\w+)\\s+\\1");
    auto result = re.search("hello hello world");
    ASSERT_TRUE(result.matched());
    EXPECT_EQ(result.data(), "hello hello");
    EXPECT_EQ(result[1], "hello");
}

TEST_F(RegexBackreferenceTest, NumericBackreferenceNoMatch) {
    regex re("(\\w+)\\s+\\1");
    EXPECT_FALSE(re.search("hello world").matched());
}

TEST_F(RegexBackreferenceTest, NamedBackreference) {
    regex re("(?<word>\\w+)\\s+\\k<word>");
    auto result = re.search("hey hey");
    ASSERT_TRUE(result.matched());
    EXPECT_EQ(result[1], "hey");
}

TEST_F(RegexBackreferenceTest, BackreferenceCaseless) {
    regex re("(?i)(\\w+) \\1");
    auto result = re.search("Hey hey");
    EXPECT_TRUE(result.matched());
}

TEST_F(RegexBackreferenceTest, BackreferenceInsideAlternation) {
    regex re("(a|b)\\1");
    EXPECT_TRUE(re.search("aa").matched());
    EXPECT_TRUE(re.search("bb").matched());
    EXPECT_FALSE(re.search("ab").matched());
}

TEST_F(RegexRecursionTest, RecursiveBalancedParentheses) {
    regex re("\\((?:[^()]|(?R))*\\)");
    auto result = re.search("x(a(b)c)y");
    ASSERT_TRUE(result.matched());
    EXPECT_EQ(result.data(), "(a(b)c)");
}

TEST_F(RegexRecursionTest, RecursiveUnbalancedFails) {
    regex re("\\((?:[^()]|(?R))*\\)");
    EXPECT_FALSE(re.do_match("(a(b)c").matched());
}

TEST_F(RegexRecursionTest, NestedDepth) {
    regex re("\\((?:[^()]|(?R))*\\)");
    auto result = re.search("(((((x)))))");
    ASSERT_TRUE(result.matched());
    EXPECT_EQ(result.data(), "(((((x)))))");
}

TEST_F(RegexUnicodeTest, LetterCategory) {
    regex re("\\p{L}+", unicode_mode);
    auto result = re.search(" 中文abc123");
    ASSERT_TRUE(result.matched());
    EXPECT_EQ(result.data(), "中文abc");
}

TEST_F(RegexUnicodeTest, HanScript) {
    regex re("\\p{Han}+", unicode_mode);
    auto result = re.search("中文abc");
    ASSERT_TRUE(result.matched());
    EXPECT_EQ(result.data(), "中文");
}

TEST_F(RegexUnicodeTest, DecimalNumberCategory) {
    regex re("\\p{Nd}+", unicode_mode);
    auto result = re.search("abc ١٢٣ def");
    ASSERT_TRUE(result.matched());
    EXPECT_EQ(result.data(), "١٢٣");
}

TEST_F(RegexUnicodeTest, GraphemeClusterKeepingCombiningMark) {
    regex re("\\X", utf_mode);
    const string text = "e\xCC\x81x";
    auto results = re.find_all(text);
    ASSERT_EQ(results.size(), 2u);
    EXPECT_EQ(results[0].length(), 3u);
    EXPECT_EQ(results[1].data(), "x");
}

TEST_F(RegexUnicodeTest, GraphemeClusterCarriageReturnLineFeed) {
    regex re("\\X", utf_mode);
    auto results = re.find_all("\r\n");
    EXPECT_EQ(results.size(), 1u);
}

TEST_F(RegexUnicodeTest, DotMatchesOneCodepoint) {
    regex re(".", utf_mode);
    auto result = re.search("中");
    ASSERT_TRUE(result.matched());
    EXPECT_EQ(result.length(), 3u);
}

TEST_F(RegexUnicodeTest, AsciiDigitDoesNotMatchUnicodeDigitWithoutUcp) {
    regex re("\\d+", utf_mode);
    EXPECT_FALSE(re.search("١٢٣").matched());
}

TEST_F(RegexUnicodeTest, CaselessNonAscii) {
    regex re("ä", unicode_mode | regex_option::caseless);
    EXPECT_TRUE(re.match("Ä"));
    EXPECT_TRUE(re.match("ä"));
}

TEST_F(RegexQuantifierTest, PossessivePreventsBacktracking) {
    regex re("a++a");
    EXPECT_FALSE(re.match("aaa"));
}

TEST_F(RegexQuantifierTest, PossessiveStillMatches) {
    regex re("a*+b");
    EXPECT_TRUE(re.match("aaab"));
}

TEST_F(RegexQuantifierTest, LazyTakesMinimum) {
    regex re("a+?");
    auto result = re.search("aaa");
    ASSERT_TRUE(result.matched());
    EXPECT_EQ(result.data(), "a");
}

TEST_F(RegexQuantifierTest, LazyGroupStopsEarly) {
    regex re("<.+?>");
    auto result = re.search("<a><b>");
    EXPECT_EQ(result.data(), "<a>");
}

TEST_F(RegexQuantifierTest, BoundedRepeat) {
    regex re("a{2,3}");
    auto result = re.search("aaaa");
    EXPECT_EQ(result.data(), "aaa");
}

TEST_F(RegexQuantifierTest, AtomicGroupDoesNotBacktrack) {
    regex re("(?>a+)a");
    EXPECT_FALSE(re.match("aaa"));
}

TEST_F(RegexQuantifierTest, AtomicGroupMatchesForward) {
    regex re("(?>a+)b");
    EXPECT_TRUE(re.match("aaab"));
}

TEST_F(RegexQuantifierTest, NestedQuantifierTerminates) {
    regex re("(a+)+b");
    EXPECT_FALSE(re.match("aaaaaaaaaaaaaaaaX"));
}

TEST_F(RegexGroupTest, NamedGroups) {
    regex re("(?<year>\\d{4})-(?<month>\\d{2})");
    auto result = re.do_match("2024-06");
    ASSERT_TRUE(result.matched());
    EXPECT_EQ(result[1], "2024");
    EXPECT_EQ(result[2], "06");
}

TEST_F(RegexGroupTest, NestedGroups) {
    regex re("((a)(b))");
    auto result = re.do_match("ab");
    ASSERT_TRUE(result.matched());
    ASSERT_EQ(result.size(), 4u);
    EXPECT_EQ(result[1], "ab");
    EXPECT_EQ(result[2], "a");
    EXPECT_EQ(result[3], "b");
}

TEST_F(RegexGroupTest, BranchReset) {
    regex re("(?|(a)|(b))");
    auto result = re.do_match("b");
    ASSERT_TRUE(result.matched());
    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result[1], "b");
}

TEST_F(RegexGroupTest, OptionalGroupUnmatched) {
    regex re("(ab)?c");
    auto result = re.do_match("c");
    ASSERT_TRUE(result.matched());
    ASSERT_EQ(result.size(), 1u);
    EXPECT_TRUE(result[1].empty());
}

TEST_F(RegexGroupTest, NonCapturingGroupDoesNotCount) {
    regex re("(?:ab)+");
    auto result = re.do_match("abab");
    ASSERT_TRUE(result.matched());
    EXPECT_EQ(result.size(), 1u);
    EXPECT_EQ(re.capture_count(), 0);
}

TEST_F(RegexGroupTest, AlternationIsLeftmostFirst) {
    regex re("a|ab");
    auto result = re.search("ab");
    EXPECT_EQ(result.data(), "a");
}

TEST_F(RegexAnchorTest, DollarMatchesBeforeFinalNewline) {
    regex re("foo$");
    EXPECT_TRUE(re.search("foo\n").matched());
}

TEST_F(RegexAnchorTest, EndOfSubjectOnlyRejectsFinalNewline) {
    regex re("foo\\z");
    EXPECT_FALSE(re.search("foo\n").matched());
}

TEST_F(RegexAnchorTest, MultilineAnchors) {
    regex re("(?m)^b$");
    auto result = re.search("a\nb\nc");
    ASSERT_TRUE(result.matched());
    EXPECT_EQ(result.position(), 2u);
    EXPECT_EQ(result.data(), "b");
}

TEST_F(RegexAnchorTest, AbsoluteAnchors) {
    regex re("\\Afoo\\z");
    EXPECT_TRUE(re.match("foo"));
    EXPECT_FALSE(re.match("xfoo"));
    EXPECT_FALSE(re.match("foo\n"));
}

TEST_F(RegexAnchorTest, WordBoundary) {
    regex re("\\bcat\\b");
    auto result = re.search("cat cats");
    ASSERT_TRUE(result.matched());
    EXPECT_EQ(result.position(), 0u);
    EXPECT_EQ(re.find_all("cats cat").size(), 1u);
}

TEST_F(RegexAnchorTest, NonWordBoundary) {
    regex re("\\Bcat\\B");
    EXPECT_TRUE(re.search("scatter").matched());
    EXPECT_FALSE(re.search("cat").matched());
}

TEST_F(RegexClassTest, NegatedClassWithPosixClass) {
    regex re("[^a-z[:digit:]]+");
    auto result = re.search("abc123!@#");
    ASSERT_TRUE(result.matched());
    EXPECT_EQ(result.data(), "!@#");
}

TEST_F(RegexClassTest, PosixAlphaClass) {
    regex re("[[:alpha:]]+");
    auto result = re.search("12abc34");
    ASSERT_TRUE(result.matched());
    EXPECT_EQ(result.data(), "abc");
}

TEST_F(RegexClassTest, ClassWithEscapedHyphen) {
    regex re("[\\d\\-]+");
    auto result = re.search("ab12-34cd");
    ASSERT_TRUE(result.matched());
    EXPECT_EQ(result.data(), "12-34");
}

TEST_F(RegexClassTest, ClassWithBraceAndBracket) {
    regex re("[{}]+");
    EXPECT_TRUE(re.match("{{}}"));
}

TEST_F(RegexEscapeTest, QuotedLiteral) {
    regex re("\\Qa.b\\E");
    EXPECT_TRUE(re.match("a.b"));
    EXPECT_FALSE(re.match("axb"));
}

TEST_F(RegexEscapeTest, EscapedDot) {
    regex re("a\\.b");
    EXPECT_TRUE(re.match("a.b"));
    EXPECT_FALSE(re.match("axb"));
}

TEST_F(RegexEscapeTest, HexEscape) {
    regex re("\\x41\\x{4E2D}", utf_mode);
    EXPECT_TRUE(re.match("A中"));
}

TEST_F(RegexEscapeTest, TabAndNewlineEscape) {
    regex re("a\\tb\\nc");
    EXPECT_TRUE(re.match("a\tb\nc"));
}

TEST_F(RegexEscapeTest, InlineCaseInsensitiveFlag) {
    regex re("(?i)abc");
    EXPECT_TRUE(re.match("ABC"));
}

TEST_F(RegexEscapeTest, ScopedCaseInsensitiveFlag) {
    regex re("a(?i:b)c");
    EXPECT_TRUE(re.match("aBc"));
    EXPECT_FALSE(re.match("ABC"));
}

TEST_F(RegexEscapeTest, DotAllFlag) {
    regex re("(?s)a.b");
    EXPECT_TRUE(re.match("a\nb"));
}

TEST_F(RegexEscapeTest, DotWithoutDotAll) {
    regex re("a.b");
    EXPECT_FALSE(re.match("a\nb"));
}

TEST_F(RegexIntegrationTest, SplitWithoutCaptures) {
    regex re(",");
    auto parts = re.split("a,b,c");
    ASSERT_EQ(parts.size(), 3u);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[2], "c");
}

TEST_F(RegexIntegrationTest, SplitWithCaptureKeepsDelimiter) {
    regex re("(,)");
    auto parts = re.split("a,b");
    ASSERT_EQ(parts.size(), 3u);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[1], ",");
    EXPECT_EQ(parts[2], "b");
}

TEST_F(RegexIntegrationTest, ReplaceAllWithGroupReferences) {
    regex re("(\\w+)@(\\w+)");
    const string replaced = re.replace_all("user@host other@host2", "$2:$1");
    EXPECT_EQ(replaced, "host:user host2:other");
}

TEST_F(RegexIntegrationTest, ReplaceFirstOnlyTouchesFirstMatch) {
    regex re("\\d+");
    const string replaced = re.replace_first("a1b2c3", "#");
    EXPECT_EQ(replaced, "a#b2c3");
}

TEST_F(RegexIntegrationTest, FindAllEmptyMatchesAdvance) {
    regex re("a*");
    auto results = re.find_all("bbb");
    ASSERT_EQ(results.size(), 4u);
    EXPECT_EQ(results[0].position(), 0u);
    EXPECT_EQ(results[3].position(), 3u);
}

TEST_F(RegexIntegrationTest, MatchResultPrefixAndSuffix) {
    regex re("b+");
    auto result = re.search("aaabbbcc");
    ASSERT_TRUE(result.matched());
    EXPECT_EQ(result.prefix(), "aaa");
    EXPECT_EQ(result.suffix(), "cc");
}

TEST_F(RegexIntegrationTest, MatchResultFormat) {
    regex re("(\\w+)=(\\d+)");
    auto result = re.do_match("key=42");
    ASSERT_TRUE(result.matched());
    EXPECT_EQ(result.format("$2/$1"), "42/key");
}
