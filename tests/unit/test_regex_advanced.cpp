#include <NeForce/core/string/regex.hpp>
#include <NeForce/core/string/string.hpp>
#include <gtest/gtest.h>

using namespace neforce;

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
    regex re("(a+)+ab");
    EXPECT_THROW((void) re.match("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), regex_exception);
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
