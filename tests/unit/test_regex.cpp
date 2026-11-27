#include <NeForce/core/file/file.hpp>
#include <NeForce/core/string/regex.hpp>
#include <NeForce/core/string/string.hpp>
#include <NeForce/core/utility/packages.hpp>
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

class RegexOptionsTest : public ::testing::Test {};

class RegexSemanticsTest : public ::testing::Test {};

class RegexApiTest : public ::testing::Test {};

TEST_F(RegexOptionsTest, LiteralDisablesMetaCharacters) {
    regex re("a.b", regex_option::literal);
    EXPECT_TRUE(re.match("a.b"));
    EXPECT_FALSE(re.match("axb"));
}

TEST_F(RegexOptionsTest, ExtendedIgnoresWhitespaceAndComments) {
    regex re("a b # trailing comment", regex_option::extended);
    EXPECT_TRUE(re.match("ab"));
}

TEST_F(RegexOptionsTest, FirstlineStopsAtFirstNewline) {
    regex re("foo", regex_option::firstline);
    EXPECT_FALSE(re.search("bar\nfoo").matched());
    EXPECT_TRUE(re.search("foo\nbar").matched());
}

TEST_F(RegexOptionsTest, DollarEndonlyRejectsTrailingNewline) {
    regex strict("foo$", regex_option::dollar_endonly);
    EXPECT_FALSE(strict.search("foo\n").matched());
    regex relaxed("foo$");
    EXPECT_TRUE(relaxed.search("foo\n").matched());
}

TEST_F(RegexOptionsTest, AltCircumflexAnchorsToSubjectStartOnly) {
    regex re("^b", regex_option::multiline | regex_option::alt_circumflex);
    EXPECT_FALSE(re.search("a\nb").matched());
    EXPECT_TRUE(re.search("b").matched());
}

TEST_F(RegexOptionsTest, AllowEmptyClassMatchesNothing) {
    regex re("[]", regex_option::allow_empty_class);
    EXPECT_FALSE(re.match("a"));
    EXPECT_THROW(regex("[]"), regex_exception);
}

TEST_F(RegexOptionsTest, NoAutoCaptureDisablesGroupCounting) {
    regex re("(a)(b)", regex_option::no_auto_capture);
    EXPECT_EQ(re.capture_count(), 0);
    auto result = re.do_match("ab");
    ASSERT_TRUE(result.matched());
    EXPECT_EQ(result.size(), 1u);
}

TEST_F(RegexOptionsTest, MatchUnsetBackreferenceMatchesEmpty) {
    regex re("(a)?\\1", regex_option::match_unset_backref);
    auto result = re.do_match("");
    EXPECT_TRUE(result.matched());
}

TEST_F(RegexOptionsTest, UngreedyFlipsQuantifierDefault) {
    regex re("a+", regex_option::ungreedy);
    EXPECT_EQ(re.search("aaa").data(), "a");
}

TEST_F(RegexOptionsTest, DuplicateNamesAllowedWithOption) {
    EXPECT_NO_THROW(regex re("(?<n>a)(?<n>b)", regex_option::dupnames));
    EXPECT_THROW(regex("(?<n>a)(?<n>b)"), regex_exception);
}

TEST_F(RegexSemanticsTest, EndSubjectFinalAllowsTrailingNewline) {
    regex re("foo\\Z");
    EXPECT_TRUE(re.search("foo\n").matched());
}

TEST_F(RegexSemanticsTest, WordBoundaryWithUnicodeProperties) {
    regex re("\\b中\\b", regex_option::utf | regex_option::ucp);
    EXPECT_TRUE(re.search("a 中 b").matched());
}

TEST_F(RegexSemanticsTest, EmptyPatternMatchesAtStart) {
    regex re("");
    auto result = re.search("abc");
    ASSERT_TRUE(result.matched());
    EXPECT_EQ(result.position(), 0u);
    EXPECT_EQ(result.length(), 0u);
}

TEST_F(RegexSemanticsTest, EmptySubjectFindAllGivesOneMatch) {
    regex re("a*");
    const auto matches = re.find_all("");
    ASSERT_EQ(matches.size(), 1u);
    EXPECT_EQ(matches[0].position(), 0u);
}

TEST_F(RegexSemanticsTest, PropertyEscapeWorksWithoutUcp) {
    regex re("\\p{Lu}+");
    EXPECT_TRUE(re.match("ABC"));
    EXPECT_FALSE(re.match("abc"));
}

TEST_F(RegexSemanticsTest, BacktrackingLimitRaises) {
    regex re("^(a+)+ab$");
    EXPECT_THROW((void) re.match(string(40, 'a') + "ba"), regex_exception);
}

TEST_F(RegexSemanticsTest, NestedQuantifierIsPossessifiedWhenSafe) {
    regex re("(a+)+b");
    EXPECT_TRUE(re.analysis().has_nested_quantifier);
    EXPECT_FALSE(re.analysis().may_backtrack_exponentially);

    auto result = re.do_match("aaab");
    ASSERT_TRUE(result.matched());
    EXPECT_EQ(result[1], "aaa");
    EXPECT_FALSE(re.match(string(64, 'a') + "X"));
}

TEST_F(RegexSemanticsTest, AnalysisFlagsExponentialRisk) {
    regex risky("(a+)+ab");
    EXPECT_TRUE(risky.analysis().has_nested_quantifier);
    EXPECT_TRUE(risky.analysis().may_backtrack_exponentially);

    regex simple("\\d+");
    EXPECT_FALSE(simple.analysis().has_nested_quantifier);
    EXPECT_FALSE(simple.analysis().may_backtrack_exponentially);
}

TEST_F(RegexSemanticsTest, AnchoredSearchRespectsOffset) {
    regex re("^a");
    EXPECT_TRUE(re.search("a", 0).matched());
    EXPECT_FALSE(re.search("ba", 1).matched());
}

TEST_F(RegexSemanticsTest, AlternationPrefersFirstBranch) {
    regex re("(a|ab)");
    auto result = re.search("ab");
    ASSERT_TRUE(result.matched());
    EXPECT_EQ(result[1], "a");
}

TEST_F(RegexSemanticsTest, QuantifierAppliesToGroup) {
    regex re("(ab){2}");
    EXPECT_TRUE(re.match("abab"));
    EXPECT_FALSE(re.match("ab"));
}

TEST_F(RegexApiTest, PrefixSuffixAndDataShareTheSubject) {
    regex re("b+");
    auto result = re.search("aaabbbcc");
    ASSERT_TRUE(result.matched());
    EXPECT_EQ(result.data(), "bbb");
    EXPECT_EQ(result.prefix(), "aaa");
    EXPECT_EQ(result.suffix(), "cc");
}

TEST_F(RegexApiTest, FormatSupportsNamedGroupReferencesAndSuffix) {
    regex re("(\\w+)@(\\w+)");
    auto result = re.search("user@host!");
    ASSERT_TRUE(result.matched());
    EXPECT_EQ(result.format("$2/$1"), "host/user");
    EXPECT_EQ(result.format("[$']"), "[!]");
}

TEST_F(RegexApiTest, DefaultResultIsUnmatched) {
    match_result result;
    EXPECT_FALSE(result.matched());
    EXPECT_EQ(result.position(), string::npos);
    EXPECT_TRUE(result.data().empty());
    EXPECT_TRUE(result.prefix().empty());
    EXPECT_TRUE(result.suffix().empty());
}

TEST_F(RegexApiTest, IteratorsCompareAndCopy) {
    regex re("\\d+");
    const string text = "a1b22c333";
    auto begin = re.begin(text);
    auto finish = re.end(text);

    int count = 0;

    for (auto it = begin; it != finish; ++it) {
        EXPECT_TRUE((*it).matched());
        ++count;
    }

    EXPECT_EQ(count, 3);

    auto copy = begin;
    EXPECT_TRUE(copy == begin);
    ++copy;
    EXPECT_FALSE(copy == begin);
}

TEST_F(RegexApiTest, SplitHonoursMaxSplits) {
    regex re(",");
    const auto limited = re.split("a,b,c", 1);
    ASSERT_EQ(limited.size(), 2u);
    EXPECT_EQ(limited[0], "a");
    EXPECT_EQ(limited[1], "b,c");
}

TEST_F(RegexApiTest, ReplaceFirstOnlyReplacesOneMatch) {
    regex re("\\d");
    EXPECT_EQ(re.replace_first("a1b2", "#"), "a#b2");
}

TEST_F(RegexApiTest, CaptureCountCountsNestedGroups) {
    regex re("((a)(b))(c)");
    EXPECT_EQ(re.capture_count(), 4);
}

TEST_F(RegexApiTest, CopyRebuildsAndMoveTransfers) {
    regex source("\\d+");
    regex copy(source);
    EXPECT_TRUE(copy.valid());
    EXPECT_EQ(copy.pattern(), "\\d+");

    regex moved(move(source));
    EXPECT_TRUE(moved.valid());
    EXPECT_FALSE(source.valid());
}

TEST_F(RegexApiTest, InvalidPatternThrowsWithMessage) {
    try {
        regex re("[abc");
        FAIL() << "expected regex_exception";
    } catch (const regex_exception& error) {
        EXPECT_FALSE(string(error.what()).empty());
    }
}

TEST_F(RegexApiTest, FindAllPositionsAreAscending) {
    regex re("a+");
    const auto matches = re.find_all("aa b a c aaa");
    ASSERT_EQ(matches.size(), 3u);
    EXPECT_LT(matches[0].position(), matches[1].position());
    EXPECT_LT(matches[1].position(), matches[2].position());
}

TEST_F(RegexSemanticsTest, NewlineEscapeMatchesCrLfAsOneUnit) {
    regex re("\\R");
    auto result = re.search("a\r\nb");
    ASSERT_TRUE(result.matched());
    EXPECT_EQ(result.position(), 1u);
    EXPECT_EQ(result.length(), 2u);

    auto single = re.search("a\nb");
    ASSERT_TRUE(single.matched());
    EXPECT_EQ(single.length(), 1u);
}

TEST_F(RegexSemanticsTest, NewlineEscapeCoversVerticalTabAndNulSeparatedForms) {
    regex re("a\\Rb");
    EXPECT_TRUE(re.match("a\r\nb"));
    EXPECT_TRUE(re.match("a\vb"));
    EXPECT_TRUE(re.match("a\fb"));
    EXPECT_FALSE(re.match("a b"));
}

TEST_F(RegexSemanticsTest, KeepResetsReportedMatchStart) {
    regex re("a\\Kb+");
    auto result = re.search("aaabbb");
    ASSERT_TRUE(result.matched());
    EXPECT_EQ(result.position(), 3u);
    EXPECT_EQ(result.data(), "bbb");
    EXPECT_EQ(result.prefix(), "aaa");
}

TEST_F(RegexSemanticsTest, BeginSearchAnchorsToTheSearchOffset) {
    regex re("\\Gabc");
    EXPECT_FALSE(re.search("xxabc").matched());
    EXPECT_TRUE(re.search("abc", 0).matched());
    EXPECT_TRUE(re.search("xxabc", 2).matched());

    regex step("\\G\\w");
    EXPECT_EQ(step.find_all("abc").size(), 3u);
}

TEST_F(RegexSemanticsTest, ConditionalTakesThenBranchWhenGroupIsSet) {
    regex re("(a)?b(?(1)c|d)");
    EXPECT_TRUE(re.match("abc"));
    EXPECT_TRUE(re.match("bd"));
    EXPECT_FALSE(re.match("bc"));
    EXPECT_FALSE(re.match("bd") && false);
}

TEST_F(RegexSemanticsTest, ConditionalByNumberAndName) {
    regex by_name("(?<flag>x)?y(?(flag)z|w)");
    EXPECT_TRUE(by_name.match("xyz"));
    EXPECT_TRUE(by_name.match("yw"));

    regex by_number("(x)?y(?(1)z|w)");
    EXPECT_TRUE(by_number.match("xyz"));
    EXPECT_TRUE(by_number.match("yw"));
}

TEST_F(RegexSemanticsTest, DefineBlockProvidesSubroutines) {
    regex re("(?(DEFINE)(?<digits>\\d+))(?&digits)-(?&digits)");
    EXPECT_TRUE(re.match("12-34"));
    EXPECT_FALSE(re.match("12-ab"));
}

TEST_F(RegexSemanticsTest, DefineBlockDoesNotMatchOnItsOwn) {
    regex re("(?(DEFINE)(?<x>a))");
    EXPECT_FALSE(re.match("a"));
}

TEST_F(RegexApiTest, GroupIndexResolvesNames) {
    regex re("(?<user>\\w+)@(?<host>\\w+)");
    EXPECT_EQ(re.group_index("user"), 1);
    EXPECT_EQ(re.group_index("host"), 2);
    EXPECT_EQ(re.group_index("missing"), -1);

    auto result = re.search("bob@example");
    ASSERT_TRUE(result.matched());
    EXPECT_EQ(result[re.group_index("user")], "bob");
    EXPECT_EQ(result[re.group_index("host")], "example");
}

TEST_F(RegexApiTest, CompileErrorCarriesOffset) {
    try {
        regex re("ab(cd");
        FAIL() << "expected regex_exception";
    } catch (const regex_exception& error) {
        const string message(error.what());
        EXPECT_NE(message.find("offset"), string::npos);
    }
}

TEST_F(RegexApiTest, AnalysisReportsPossessifiedNesting) {
    regex safe("(a+)+b");
    EXPECT_TRUE(safe.analysis().has_nested_quantifier);
    EXPECT_FALSE(safe.analysis().may_backtrack_exponentially);

    regex risky("(a+)+ab");
    EXPECT_TRUE(risky.analysis().has_nested_quantifier);
    EXPECT_TRUE(risky.analysis().may_backtrack_exponentially);

    regex flat("(\\w+)\\s+");
    EXPECT_FALSE(flat.analysis().has_nested_quantifier);
    EXPECT_FALSE(flat.analysis().may_backtrack_exponentially);
}

TEST_F(RegexApiTest, PossessificationKeepsCaptureSemantics) {
    regex re("(\\d+)+-");
    auto result = re.search("123-");
    ASSERT_TRUE(result.matched());
    EXPECT_EQ(result[1], "123");
    EXPECT_EQ(result.data(), "123-");

    regex nested("(a+)+b");
    auto nested_result = nested.search("aab");
    ASSERT_TRUE(nested_result.matched());
    EXPECT_EQ(nested_result[1], "aa");
}

TEST_F(RegexApiTest, SplitTreatsAnyNewlineConvention) {
    regex re("\\R");
    const auto parts = re.split("a\r\nb\nc");
    ASSERT_EQ(parts.size(), 3u);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[1], "b");
    EXPECT_EQ(parts[2], "c");
}

#ifndef NEFORCE_REGEX_CONFORMANCE_FILE
#    define NEFORCE_REGEX_CONFORMANCE_FILE "tests/conformance/regex/cases.txt"
#endif

namespace {

    struct conformance_case {
        string pattern;
        string options;
        string subject;
        string expected;
    };

    string unescape_field(const string_view field) {
        string out;
        out.reserve(field.size());

        for (size_t index = 0; index < field.size(); ++index) {
            if (field[index] == '\\' && index + 1 < field.size()) {
                const char next = field[++index];

                if (next == 't') {
                    out += '\t';
                } else if (next == 'n') {
                    out += '\n';
                } else if (next == 'r') {
                    out += '\r';
                } else {
                    out += next;
                }
            } else {
                out += field[index];
            }
        }

        return out;
    }

    void split_fields(const string_view line, vector<conformance_case>& sink) {
        size_t start = 0;
        vector<string_view> fields;

        while (start <= line.size()) {
            size_t separator = line.find('\t', start);

            if (separator == string::npos) {
                separator = line.size();
            }

            fields.push_back(line.substr(start, separator - start));
            start = separator + 1;
        }

        if (fields.size() != 4) {
            return;
        }

        conformance_case entry;
        entry.pattern = unescape_field(fields[0]);
        entry.options = unescape_field(fields[1]);
        entry.subject = unescape_field(fields[2]);
        entry.expected = string(fields[3].data(), fields[3].size());
        sink.push_back(move(entry));
    }

    vector<conformance_case> load_cases() {
        vector<conformance_case> result;

        const char* const candidates[] = {
                NEFORCE_REGEX_CONFORMANCE_FILE,
                "tests/conformance/regex/cases.txt",
                "../../../tests/conformance/regex/cases.txt",
                "../../../../tests/conformance/regex/cases.txt",
        };

        for (const char* candidate: candidates) {
            try {
                file input{path(candidate)};

                for (auto it = input.begin_lines(); it != file::line_iterator(); ++it) {
                    const string& raw = *it;
                    const size_t length = !raw.empty() && raw[raw.size() - 1] == '\r' ? raw.size() - 1 : raw.size();
                    const string_view line = raw.view(0, length);

                    if (line.empty() || line[0] == '#') {
                        continue;
                    }

                    split_fields(line, result);
                }

                if (!result.empty()) {
                    break;
                }
            } catch (...) {
            }
        }

        return result;
    }

    regex_option to_options(const string& options) {
        regex_option result = regex_option::none;

        for (const char option: options) {
            if (option == 'i') {
                result = result | regex_option::caseless;
            } else if (option == 'm') {
                result = result | regex_option::multiline;
            } else if (option == 's') {
                result = result | regex_option::dotall;
            } else if (option == 'x') {
                result = result | regex_option::extended;
            } else if (option == 'U') {
                result = result | regex_option::ungreedy;
            } else if (option == 'u') {
                result = result | regex_option::utf;
            } else if (option == 'p') {
                result = result | regex_option::ucp;
            } else if (option == 'l') {
                result = result | regex_option::literal;
            } else if (option == 'd') {
                result = result | regex_option::dollar_endonly;
            }
        }

        return result;
    }

    string describe(const match_result& result) {
        if (!result.matched()) {
            return "N";
        }

        string out;

        for (size_t index = 0; index < result.size(); ++index) {
            const auto position = result.position(index);

            if (position.first == string::npos) {
                out += "-;";
            } else {
                out += to_string(position.first) + "," + to_string(position.second) + ";";
            }
        }

        return out;
    }

} // namespace

class RegexConformanceTest : public ::testing::Test {
protected:
    static vector<conformance_case>& cases() {
        static vector<conformance_case> loaded = load_cases();
        return loaded;
    }
};

TEST_F(RegexConformanceTest, CorpusLoaded) { EXPECT_GT(cases().size(), 50u); }

TEST_F(RegexConformanceTest, MatchesRecordedBehaviour) {
    ASSERT_FALSE(cases().empty());

    for (const auto& entry: cases()) {
        SCOPED_TRACE(("pattern: " + entry.pattern + " subject: " + entry.subject).data());

        regex expression(entry.pattern, to_options(entry.options));
        const auto result = expression.search(entry.subject);

        EXPECT_EQ(describe(result), entry.expected);
    }
}

namespace {

    class fuzz_random {
    public:
        explicit fuzz_random(const uint64_t seed) :
        state_(seed | 1U) {}

        uint32_t next() {
            state_ ^= state_ << 13U;
            state_ ^= state_ >> 7U;
            state_ ^= state_ << 17U;
            return static_cast<uint32_t>(state_);
        }

        uint32_t below(const uint32_t bound) { return bound == 0 ? 0 : next() % bound; }

        bool chance(const uint32_t percent) { return below(100) < percent; }

    private:
        uint64_t state_;
    };

    void append_sequence(string& out, fuzz_random& random, int depth);

    void append_atom(string& out, fuzz_random& random, const int depth) {
        switch (random.below(10)) {
            case 0:
                out += static_cast<char>('a' + static_cast<int>(random.below(3)));
                break;
            case 1:
                out += '.';
                break;
            case 2:
                out += "\\d";
                break;
            case 3:
                out += "\\w";
                break;
            case 4:
                out += "[a-c]";
                break;
            case 5:
                out += "[^0-9]";
                break;
            case 6:
                out += "\\s";
                break;
            case 7:
                if (depth < 2) {
                    out += "(?=";
                    append_sequence(out, random, depth + 1);
                    out += ')';
                } else {
                    out += 'b';
                }
                break;
            case 8:
                if (depth < 2) {
                    out += "(?:";
                    append_sequence(out, random, depth + 1);
                    out += ')';
                } else {
                    out += 'c';
                }
                break;
            default:
                out += static_cast<char>('0' + static_cast<int>(random.below(4)));
                break;
        }
    }

    void append_quantifier(string& out, fuzz_random& random) {
        switch (random.below(7)) {
            case 0:
                out += '*';
                break;
            case 1:
                out += '+';
                break;
            case 2:
                out += '?';
                break;
            case 3:
                out += "{0,2}";
                break;
            case 4:
                out += "{1,3}";
                break;
            default:
                return;
        }

        if (random.chance(20)) {
            out += random.chance(50) ? '?' : '+';
        }
    }

    void append_sequence(string& out, fuzz_random& random, const int depth) {
        const uint32_t items = 1 + random.below(3);

        for (uint32_t index = 0; index < items; ++index) {
            if (random.chance(10)) {
                out += '^';
            }

            append_atom(out, random, depth);
            append_quantifier(out, random);
        }

        if (random.chance(15)) {
            out += '|';
            append_atom(out, random, depth);
        }
    }

    string build_pattern(fuzz_random& random) {
        string pattern;

        if (random.chance(15)) {
            pattern += '^';
        }

        append_sequence(pattern, random, 0);

        if (random.chance(15)) {
            pattern += '$';
        }

        return pattern;
    }

    regex_option build_options(fuzz_random& random) {
        regex_option options = regex_option::none;

        if (random.chance(25)) {
            options = options | regex_option::caseless;
        }
        if (random.chance(20)) {
            options = options | regex_option::multiline;
        }
        if (random.chance(20)) {
            options = options | regex_option::dotall;
        }
        if (random.chance(30)) {
            options = options | regex_option::utf;
        }
        if (random.chance(20)) {
            options = options | regex_option::ucp;
        }

        return options;
    }

    string build_subject(fuzz_random& random) {
        static const char* const fragments[] = {"a", "b", "c", "0", "1", " ", "\n", "中", "_", "x"};

        const uint32_t length = random.below(24);
        string subject;

        for (uint32_t index = 0; index < length; ++index) {
            subject += fragments[random.below(10)];
        }

        return subject;
    }

    void check_result(const match_result& result, const string& subject) {
        if (!result.matched()) {
            return;
        }

        ASSERT_LE(result.position(), subject.size());
        ASSERT_LE(result.position() + result.length(), subject.size());

        const string_view matched_text = result.data();
        ASSERT_EQ(string(matched_text.data(), matched_text.size()),
                  string(subject.data() + result.position(), result.length()));

        for (size_t index = 0; index < result.size(); ++index) {
            const auto position = result.position(index);

            if (position.first == string::npos) {
                continue;
            }

            ASSERT_LE(position.first, subject.size());
            ASSERT_LE(position.first + position.second, subject.size());
        }
    }

} // namespace

class RegexFuzzTest : public ::testing::Test {};

TEST_F(RegexFuzzTest, RandomPatternsKeepResultInvariants) {
    fuzz_random random(0xC0FFEE1234ULL);

    for (int iteration = 0; iteration < 1500; ++iteration) {
        const string pattern = build_pattern(random);
        const regex_option options = build_options(random);
        const string subject = build_subject(random);

        SCOPED_TRACE(("pattern: " + pattern + " subject: " + subject).data());

        try {
            regex expression(pattern, options);

            check_result(expression.search(subject), subject);

            const auto matches = expression.find_all(subject);
            size_t previous = 0;

            for (const auto& match: matches) {
                check_result(match, subject);
                ASSERT_GE(match.position(), previous);
                previous = match.position();
            }

            const string replaced = expression.replace_all(subject, "#");

            if (matches.empty()) {
                ASSERT_EQ(replaced, subject);
            }

            (void) expression.split(subject);
        } catch (const regex_exception&) {
        }
    }
}

class RegexCopyTest : public ::testing::Test {};

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

TEST_F(RegexConstructionTest, ValidPatternWithOptions) { EXPECT_NO_THROW(regex re("hello", regex_option::caseless)); }

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

TEST_F(RegexFindAllTest, FindAllManyMatchesKeepsOrder) {
    string text;

    for (int index = 0; index < 2000; ++index) {
        text += "ab";
    }

    regex re("a");
    auto results = re.find_all(text);
    ASSERT_EQ(results.size(), 2000u);

    for (size_t index = 0; index < results.size(); ++index) {
        EXPECT_EQ(results[index].position(), index * 2);
        EXPECT_EQ(results[index].data(), "a");
    }
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

TEST_F(RegexReplaceTest, ReplaceAllLiteralReplacementRepeats) {
    regex re("\\d+");
    string result = re.replace_all("a1b22c333", "N");
    EXPECT_EQ(result, "aNbNcN");
}

TEST_F(RegexReplaceTest, ReplaceAllLiteralReplacementWithEscapes) {
    regex re("\\d+");
    string result = re.replace_all("a1b22c333", "$$");
    EXPECT_EQ(result, "a$b$c$");
}

TEST_F(RegexReplaceTest, ReplaceAllLiteralReplacementEmpty) {
    regex re("\\d+");
    string result = re.replace_all("a1b22c", "");
    EXPECT_EQ(result, "abc");
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

TEST_F(MatchResultTest, CopyConstructorKeepsGroups) {
    regex re("(\\w+)=(\\d+)");
    auto source = re.search("name=123");
    match_result copied(source);
    EXPECT_TRUE(copied.matched());
    EXPECT_EQ(copied.size(), 3u);
    EXPECT_EQ(copied[0], "name=123");
    EXPECT_EQ(copied[1], "name");
    EXPECT_EQ(copied[2], "123");
    EXPECT_EQ(copied.position(), source.position());
}

TEST_F(MatchResultTest, CopyIsIndependent) {
    regex re("(\\w+)-(\\w+)");
    auto source = re.search("left-right");
    match_result copied;
    copied = source;
    const match_result reassigned(source);
    EXPECT_EQ(reassigned[2], "right");
    EXPECT_EQ(copied[2], "right");
    EXPECT_EQ(source[1], "left");
}

TEST_F(MatchResultTest, CopyAssignmentOverwritesGroups) {
    regex re("(a)(b)");
    auto wide = re.search("ab");
    auto narrow = regex("\\d+").search("42");
    EXPECT_EQ(narrow.size(), 1u);
    narrow = wide;
    EXPECT_EQ(narrow.size(), 3u);
    EXPECT_EQ(narrow[2], "b");
}

TEST_F(MatchResultTest, MoveConstructorTransfersGroups) {
    regex re("(\\w+)=(\\d+)");
    auto source = re.search("key=456");
    match_result moved(move(source));
    EXPECT_TRUE(moved.matched());
    EXPECT_EQ(moved.size(), 3u);
    EXPECT_EQ(moved[2], "456");
    EXPECT_FALSE(source.matched());
    EXPECT_EQ(source.size(), 0u);
}

TEST_F(MatchResultTest, MoveAssignmentTransfersGroups) {
    regex re("(\\w+)=(\\d+)");
    auto source = re.search("k=7");
    auto target = regex("x").search("x");
    target = move(source);
    EXPECT_EQ(target.size(), 3u);
    EXPECT_EQ(target[1], "k");
    EXPECT_FALSE(source.matched());
}

TEST_F(MatchResultTest, NoGroupResultHasSingleEntry) {
    regex re("\\d+");
    auto result = re.search("abc 123");
    EXPECT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0], "123");
    EXPECT_TRUE(result[1].empty());
    EXPECT_EQ(result.position(1).first, string::npos);

    size_t count = 0;

    for (const auto& group: result) {
        EXPECT_EQ(group, "123");
        ++count;
    }

    EXPECT_EQ(count, 1u);
}

TEST_F(MatchResultTest, UnmatchedIteratorIsEmpty) {
    auto result = regex("z").search("abc");
    EXPECT_FALSE(result.matched());
    EXPECT_EQ(result.begin(), result.end());
}

TEST_F(MatchResultTest, FindAllResultsAreIndependent) {
    regex re("(\\w+)-(\\d+)");
    auto results = re.find_all("a-1 b-2 c-3");
    ASSERT_EQ(results.size(), 3u);
    auto copied = results;

    for (size_t index = 0; index < results.size(); ++index) {
        EXPECT_EQ(results[index][1], copied[index][1]);
        EXPECT_EQ(results[index][2], copied[index][2]);
    }

    EXPECT_EQ(copied[2][1], "c");
    EXPECT_EQ(copied[2][2], "3");
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
