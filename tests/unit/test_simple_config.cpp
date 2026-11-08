#include <NeForce/core/file/env/env_builder.hpp>
#include <NeForce/core/file/env/env_parser.hpp>
#include <NeForce/core/file/ini/ini_builder.hpp>
#include <NeForce/core/file/ini/ini_parser.hpp>
#include <NeForce/core/container/flat_unordered_map.hpp>
#include <NeForce/core/file/json/json_builder.hpp>
#include <NeForce/core/file/json/json_parser.hpp>
#include <NeForce/core/string/format.hpp>
#include <gtest/gtest.h>
using namespace neforce;

class EnvValueTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(EnvValueTest, EnvVariableType) {
    env_document doc;
    doc.set_variable("KEY", "value");
    const env_variable var = doc.get_variable("KEY")->as_variable();
    EXPECT_EQ(var.type(), env_value::kind::variable);
    EXPECT_TRUE(var.is_variable());
}

TEST_F(EnvValueTest, EnvVariableGetValue) {
    env_document doc;
    doc.set_variable("KEY", "test_value");
    EXPECT_EQ(doc.get_variable("KEY")->as_variable().get_value(), "test_value");
    EXPECT_EQ(doc.get_variable("KEY")->element_count(), 10);
}

TEST_F(EnvValueTest, EnvVariableQuoteType) {
    env_document doc;
    doc.set_variable("NONE", "value", env_variable::None);
    doc.set_variable("SINGLE", "value", env_variable::Single);
    doc.set_variable("DOUBLE", "value", env_variable::Double);

    EXPECT_EQ(doc.get_variable("NONE")->as_variable().get_quote_type(), env_variable::None);
    EXPECT_EQ(doc.get_variable("SINGLE")->as_variable().get_quote_type(), env_variable::Single);
    EXPECT_EQ(doc.get_variable("DOUBLE")->as_variable().get_quote_type(), env_variable::Double);
}

TEST_F(EnvValueTest, EnvVariableExported) {
    env_document doc;
    doc.set_variable("EXPORTED", "value", env_variable::None, true);
    doc.set_variable("PLAIN", "value");

    EXPECT_TRUE(doc.get_variable("EXPORTED")->as_variable().is_exported());
    EXPECT_FALSE(doc.get_variable("PLAIN")->as_variable().is_exported());
}

TEST_F(EnvValueTest, EnvVariableGetInt) {
    env_document doc;
    doc.set_variable("A", "42");
    doc.set_variable("B", "not_a_number");
    doc.set_variable("C", "-10");
    EXPECT_EQ(doc.get_variable("A")->as_variable().get_int(), 42);
    EXPECT_EQ(doc.get_variable("B")->as_variable().get_int(), 0);
    EXPECT_EQ(doc.get_variable("B")->as_variable().get_int(7), 7);
    EXPECT_EQ(doc.get_variable("C")->as_variable().get_int(), -10);
}

TEST_F(EnvValueTest, EnvVariableGetInt64) {
    env_document doc;
    doc.set_variable("A", "1234567890123");
    EXPECT_EQ(doc.get_variable("A")->as_variable().get_int64(), 1234567890123);
    EXPECT_EQ(doc.get_variable("MISSING"), nullptr);
}

TEST_F(EnvValueTest, EnvVariableGetDouble) {
    env_document doc;
    doc.set_variable("A", "3.5");
    doc.set_variable("B", "not_a_number");
    EXPECT_DOUBLE_EQ(doc.get_variable("A")->as_variable().get_double(), 3.5);
    EXPECT_DOUBLE_EQ(doc.get_variable("B")->as_variable().get_double(1.5), 1.5);
}

TEST_F(EnvValueTest, EnvVariableGetBool) {
    env_document doc;
    doc.set_variable("T", "true");
    doc.set_variable("F", "false");
    doc.set_variable("BAD", "x");
    EXPECT_TRUE(doc.get_variable("T")->as_variable().get_bool());
    EXPECT_FALSE(doc.get_variable("F")->as_variable().get_bool());
    EXPECT_TRUE(doc.get_variable("BAD")->as_variable().get_bool(true));
}

TEST_F(EnvValueTest, EnvVariableToString) {
    env_document doc;
    doc.set_variable("A", "value");
    doc.set_variable("B", "a b", env_variable::Single);
    doc.set_variable("C", "x\ny", env_variable::Double);
    doc.set_variable("D", "v", env_variable::None, true);

    EXPECT_EQ(doc.get_variable("A")->as_variable().to_string(), "value");
    EXPECT_EQ(doc.get_variable("B")->as_variable().to_string(), "a b");
    EXPECT_EQ(doc.get_variable("C")->as_variable().to_string(), "x\ny");
    EXPECT_EQ(doc.get_variable("D")->as_variable().to_string(), "v");
    EXPECT_EQ(doc.get_variable("A")->to_string(), "value");
    EXPECT_EQ(doc.get_variable("B")->to_string(), "'a b'");
    EXPECT_EQ(doc.get_variable("C")->to_string(), "\"x\\ny\"");
    EXPECT_EQ(doc.get_variable("D")->to_string(), "export v");
}

TEST_F(EnvValueTest, EnvDocumentGetVariables) {
    env_document doc;
    doc.set_variable("KEY1", "value1");
    doc.set_variable("KEY2", "value2");
    EXPECT_EQ(doc.get_variables().size(), 2);
    size_t count = 0;
    for (const env_member& member: doc.get_variables()) {
        EXPECT_FALSE(member.key().empty());
        ++count;
    }
    EXPECT_EQ(count, 2);
}

TEST_F(EnvValueTest, EnvDocumentGetVariable) {
    env_document doc;
    doc.set_variable("DATABASE_URL", "localhost");
    ASSERT_NE(doc.get_variable("DATABASE_URL"), nullptr);
    EXPECT_EQ(doc.get_variable("DATABASE_URL")->as_variable().get_value(), "localhost");
    EXPECT_EQ(doc.get_variable("NONEXISTENT"), nullptr);
}

TEST_F(EnvValueTest, EnvDocumentHasVariable) {
    env_document doc;
    doc.set_variable("EXISTS", "value");
    EXPECT_TRUE(doc.has_variable("EXISTS"));
    EXPECT_FALSE(doc.has_variable("DOES_NOT_EXIST"));
}

TEST_F(EnvValueTest, EnvDocumentRemoveVariable) {
    env_document doc;
    doc.set_variable("A", "1");
    doc.set_variable("B", "2");
    doc.set_variable("C", "3");
    doc.remove_variable("B");
    EXPECT_FALSE(doc.has_variable("B"));
    EXPECT_EQ(doc.get_variables().size(), 2);
    EXPECT_EQ(doc.get_string("A"), "1");
    EXPECT_EQ(doc.get_string("C"), "3");
}

TEST_F(EnvValueTest, EnvDocumentSetVariableOverwrites) {
    env_document doc;
    doc.set_variable("KEY", "initial");
    doc.set_variable("KEY", "updated", env_variable::Double);
    EXPECT_EQ(doc.get_string("KEY"), "updated");
    EXPECT_EQ(doc.get_variables().size(), 1);
    EXPECT_EQ(doc.get_variable("KEY")->as_variable().get_quote_type(), env_variable::Double);
}

TEST_F(EnvValueTest, EnvDocumentAddComment) {
    env_document doc;
    doc.add_comment("This is a comment");
    doc.add_comment("Another comment");
    EXPECT_EQ(doc.get_comments().size(), 2);
    EXPECT_EQ(doc.get_comments()[0], "This is a comment");
    EXPECT_EQ(doc.get_comments()[1], "Another comment");
}

TEST_F(EnvValueTest, EnvDocumentGetString) {
    env_document doc;
    doc.set_variable("NAME", "John");
    EXPECT_EQ(doc.get_string("NAME"), "John");
    EXPECT_EQ(doc.get_string("NONEXISTENT"), "");
    EXPECT_EQ(doc.get_string("NONEXISTENT", "default"), "default");
}

TEST_F(EnvValueTest, EnvDocumentGetInt) {
    env_document doc;
    doc.set_variable("TIMEOUT", "30");
    EXPECT_EQ(doc.get_int("TIMEOUT"), 30);
    EXPECT_EQ(doc.get_int("NONEXISTENT"), 0);
    EXPECT_EQ(doc.get_int("NONEXISTENT", 60), 60);
}

TEST_F(EnvValueTest, EnvDocumentGetInt64) {
    env_document doc;
    doc.set_variable("BIG", "1234567890123");
    EXPECT_EQ(doc.get_int64("BIG"), 1234567890123);
    EXPECT_EQ(doc.get_int64("NONEXISTENT", 999), 999);
}

TEST_F(EnvValueTest, EnvDocumentGetDouble) {
    env_document doc;
    doc.set_variable("RATIO", "0.75");
    EXPECT_DOUBLE_EQ(doc.get_double("RATIO"), 0.75);
    EXPECT_DOUBLE_EQ(doc.get_double("NONEXISTENT"), 0.0);
    EXPECT_DOUBLE_EQ(doc.get_double("NONEXISTENT", 1.0), 1.0);
}

TEST_F(EnvValueTest, EnvDocumentGetBool) {
    env_document doc;
    doc.set_variable("ENABLED", "true");
    EXPECT_TRUE(doc.get_bool("ENABLED"));
    EXPECT_FALSE(doc.get_bool("NONEXISTENT"));
    EXPECT_TRUE(doc.get_bool("NONEXISTENT", true));
}

TEST_F(EnvValueTest, EnvDocumentMoveConstructor) {
    env_document doc;
    doc.set_variable("KEY", "value");
    doc.add_comment("comment");
    env_document moved(move(doc));
    EXPECT_TRUE(moved.has_variable("KEY"));
    EXPECT_EQ(moved.get_string("KEY"), "value");
    EXPECT_EQ(moved.get_comments().size(), 1);
}

TEST_F(EnvValueTest, EnvDocumentMoveAssignment) {
    env_document doc1;
    doc1.set_variable("KEY1", "value1");
    env_document doc2;
    doc2.set_variable("KEY2", "value2");
    doc2 = move(doc1);
    EXPECT_TRUE(doc2.has_variable("KEY1"));
    EXPECT_FALSE(doc2.has_variable("KEY2"));
}

TEST_F(EnvValueTest, EnvDocumentToString) {
    env_document doc;
    doc.add_comment("Configuration file");
    doc.set_variable("APP_NAME", "MyApp");
    doc.set_variable("PORT", "8080");
    const string result = doc.to_string();
    EXPECT_NE(result.find("# Configuration file"), string::npos);
    EXPECT_NE(result.find("APP_NAME=MyApp"), string::npos);
    EXPECT_NE(result.find("PORT=8080"), string::npos);
}

TEST_F(EnvValueTest, EnvDocumentToStringEmpty) {
    env_document doc;
    EXPECT_EQ(doc.to_string(), "");
}

class EnvParserTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(EnvParserTest, ParseEmpty) {
    env_parser parser("");
    env_document doc = parser.parse();
    EXPECT_TRUE(doc.get_variables().empty());
}

TEST_F(EnvParserTest, ParseSimpleKeyValue) {
    env_parser parser("KEY=value");
    env_document doc = parser.parse();
    EXPECT_EQ(doc.get_string("KEY"), "value");
}

TEST_F(EnvParserTest, ParseMultipleKeyValues) {
    env_parser parser("A=1\nB=2\nC=3");
    env_document doc = parser.parse();
    EXPECT_EQ(doc.get_variables().size(), 3);
    EXPECT_EQ(doc.get_int("A"), 1);
    EXPECT_EQ(doc.get_int("C"), 3);
}

TEST_F(EnvParserTest, ParseDoubleQuotedValue) {
    env_parser parser("KEY=\"hello world\"");
    env_document doc = parser.parse();
    EXPECT_EQ(doc.get_string("KEY"), "hello world");
    EXPECT_EQ(doc.get_variable("KEY")->as_variable().get_quote_type(), env_variable::Double);
}

TEST_F(EnvParserTest, ParseSingleQuotedValue) {
    env_parser parser("KEY='hello world'");
    env_document doc = parser.parse();
    EXPECT_EQ(doc.get_string("KEY"), "hello world");
    EXPECT_EQ(doc.get_variable("KEY")->as_variable().get_quote_type(), env_variable::Single);
}

TEST_F(EnvParserTest, ParseUnquotedStopsAtSpace) {
    env_parser parser("KEY=value with spaces");
    env_document doc = parser.parse();
    EXPECT_EQ(doc.get_string("KEY"), "value");
}

TEST_F(EnvParserTest, ParseExportedVariable) {
    env_parser parser("export KEY=value");
    env_document doc = parser.parse();
    EXPECT_EQ(doc.get_string("KEY"), "value");
    EXPECT_TRUE(doc.get_variable("KEY")->as_variable().is_exported());
}

TEST_F(EnvParserTest, ParseExportWithoutSpaceIsKey) {
    env_parser parser("exportKEY=value");
    env_document doc = parser.parse();
    EXPECT_EQ(doc.get_string("exportKEY"), "value");
}

TEST_F(EnvParserTest, ParseComment) {
    env_parser parser("# this is a comment\nKEY=value");
    env_document doc = parser.parse();
    EXPECT_EQ(doc.get_comments().size(), 1);
    EXPECT_EQ(doc.get_comments()[0], "this is a comment");
    EXPECT_EQ(doc.get_string("KEY"), "value");
}

TEST_F(EnvParserTest, ParseBlankLines) {
    env_parser parser("\n\nKEY=value\n\n");
    env_document doc = parser.parse();
    EXPECT_EQ(doc.get_variables().size(), 1);
    EXPECT_TRUE(doc.get_comments().empty());
}

TEST_F(EnvParserTest, ParseEmptyValue) {
    env_parser parser("KEY=");
    env_document doc = parser.parse();
    EXPECT_EQ(doc.get_string("KEY"), "");
}

TEST_F(EnvParserTest, ParseDoubleQuotedEscapes) {
    env_parser parser("KEY=\"a\\nb\\tc\\\\d\\\"e\"");
    env_document doc = parser.parse();
    EXPECT_EQ(doc.get_string("KEY"), "a\nb\tc\\d\"e");
}

TEST_F(EnvParserTest, ParseSingleQuotedEscapedQuote) {
    env_parser parser("KEY='it'\\''s'");
    env_document doc = parser.parse();
    EXPECT_EQ(doc.get_string("KEY"), "it's");
}

TEST_F(EnvParserTest, ParseSingleQuotedKeepsBackslash) {
    env_parser parser("KEY='C:\\path\\file'");
    env_document doc = parser.parse();
    EXPECT_EQ(doc.get_string("KEY"), "C:\\path\\file");
}

TEST_F(EnvParserTest, ParseNumericAndBoolean) {
    env_parser parser("INT=42\nBIG=1234567890123\nFP=2.5\nBOOL=true");
    env_document doc = parser.parse();
    EXPECT_EQ(doc.get_int("INT"), 42);
    EXPECT_EQ(doc.get_int64("BIG"), 1234567890123);
    EXPECT_DOUBLE_EQ(doc.get_double("FP"), 2.5);
    EXPECT_TRUE(doc.get_bool("BOOL"));
}

TEST_F(EnvParserTest, ParseNoEqualsSign) {
    env_parser parser("NO_EQUALS\nKEY=value");
    env_document doc = parser.parse();
    EXPECT_EQ(doc.get_variables().size(), 1);
    EXPECT_EQ(doc.get_string("KEY"), "value");
}

TEST_F(EnvParserTest, ParseEmptyVariableName) {
    env_parser parser("=value\nKEY=value");
    env_document doc = parser.parse();
    EXPECT_EQ(doc.get_variables().size(), 1);
}

TEST_F(EnvParserTest, ParseTrailingCommentAfterQuoted) {
    env_parser parser("KEY=\"value\" # ignored");
    env_document doc = parser.parse();
    EXPECT_EQ(doc.get_string("KEY"), "value");
}

TEST_F(EnvParserTest, ParseInlineHashEndsUnquoted) {
    env_parser parser("KEY=value#suffix");
    env_document doc = parser.parse();
    EXPECT_EQ(doc.get_string("KEY"), "value");
}

TEST_F(EnvParserTest, ParseCRLF) {
    env_parser parser("A=1\r\nB=2\r\n");
    env_document doc = parser.parse();
    EXPECT_EQ(doc.get_int("A"), 1);
    EXPECT_EQ(doc.get_int("B"), 2);
}

TEST_F(EnvParserTest, TryParseValid) {
    env_parser parser("KEY=value");
    auto result = parser.try_parse();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->get_string("KEY"), "value");
}

TEST_F(EnvParserTest, ParseBorrowedText) {
    const string text = "A=1\nB=\"x y\"";
    env_parser parser(text.view());
    env_document doc = parser.parse();
    EXPECT_EQ(doc.get_int("A"), 1);
    EXPECT_EQ(doc.get_string("B"), "x y");
}

TEST_F(EnvParserTest, ParseOwnedText) {
    env_document doc = env_parser(string("A=1\nB=value")).parse();
    EXPECT_EQ(doc.get_int("A"), 1);
    EXPECT_EQ(doc.get_string("B"), "value");
}

TEST_F(EnvParserTest, ParseComplexDocument) {
    const string text =
            "# app configuration\n"
            "APP_NAME=\"My App\"\n"
            "PORT=8080\n"
            "export DEBUG=true\n"
            "RATIO=0.75\n"
            "QUOTED='single value'\n";
    env_document doc = env_parser(text.view()).parse();
    EXPECT_EQ(doc.get_string("APP_NAME"), "My App");
    EXPECT_EQ(doc.get_int("PORT"), 8080);
    EXPECT_TRUE(doc.get_bool("DEBUG"));
    EXPECT_TRUE(doc.get_variable("DEBUG")->as_variable().is_exported());
    EXPECT_DOUBLE_EQ(doc.get_double("RATIO"), 0.75);
    EXPECT_EQ(doc.get_string("QUOTED"), "single value");
    EXPECT_EQ(doc.get_comments().size(), 1);
}

class EnvBuilderTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(EnvBuilderTest, BuildSimple) {
    env_builder builder;
    builder.add("KEY", "value");
    env_document doc = builder.build();
    EXPECT_EQ(doc.get_string("KEY"), "value");
}

TEST_F(EnvBuilderTest, BuildKeyValueApi) {
    env_builder builder;
    builder.key("A").value("x").key("B").value(2);
    env_document doc = builder.build();
    EXPECT_EQ(doc.get_string("A"), "x");
    EXPECT_EQ(doc.get_int("B"), 2);
}

TEST_F(EnvBuilderTest, BuildNumericValues) {
    env_builder builder;
    builder.add("INT", 42);
    builder.add("BIG", int64_t{1234567890123});
    builder.add("FP", 2.5);
    builder.add("BOOL", true);
    env_document doc = builder.build();
    EXPECT_EQ(doc.get_int("INT"), 42);
    EXPECT_EQ(doc.get_int64("BIG"), 1234567890123);
    EXPECT_DOUBLE_EQ(doc.get_double("FP"), 2.5);
    EXPECT_TRUE(doc.get_bool("BOOL"));
}

TEST_F(EnvBuilderTest, BuildDoubleWithPrecision) {
    env_builder builder;
    builder.key("PI").value(3.14159, 2);
    EXPECT_EQ(builder.build().get_string("PI"), "3.14");
}

TEST_F(EnvBuilderTest, BuildQuotedValues) {
    env_builder builder;
    builder.key("S").single_quoted().value("a b");
    builder.key("D").double_quoted().value("x\ny");
    env_document doc = builder.build();
    EXPECT_EQ(doc.get_string("S"), "a b");
    EXPECT_EQ(doc.get_string("D"), "x\ny");
    EXPECT_EQ(doc.get_variable("S")->as_variable().get_quote_type(), env_variable::Single);
    EXPECT_EQ(doc.get_variable("D")->as_variable().get_quote_type(), env_variable::Double);
}

TEST_F(EnvBuilderTest, BuildExported) {
    env_builder builder;
    builder.add_export("PATH", "/bin");
    builder.key("PLAIN").exported().value("v");
    env_document doc = builder.build();
    EXPECT_TRUE(doc.get_variable("PATH")->as_variable().is_exported());
    EXPECT_TRUE(doc.get_variable("PLAIN")->as_variable().is_exported());
}

TEST_F(EnvBuilderTest, BuildCommentsAndBlankLine) {
    env_builder builder;
    builder.comment("first").blank_line().comment("second");
    env_document doc = builder.build();
    EXPECT_EQ(doc.get_comments().size(), 3);
    EXPECT_EQ(doc.get_comments()[0], "first");
    EXPECT_EQ(doc.get_comments()[1], "");
    EXPECT_EQ(doc.get_comments()[2], "second");
}

TEST_F(EnvBuilderTest, BuildToString) {
    env_builder builder;
    builder.comment("c").add("A", 1).add_export("B", "x");
    const string text = builder.build().to_string();
    EXPECT_NE(text.find("# c"), string::npos);
    EXPECT_NE(text.find("A=1"), string::npos);
    EXPECT_NE(text.find("export B=x"), string::npos);
}

TEST_F(EnvBuilderTest, BuildThrowsWithoutKey) {
    env_builder builder;
    EXPECT_THROW(builder.value("x"), env_exception);
}

TEST_F(EnvBuilderTest, BuildMove) {
    env_builder builder;
    builder.add("KEY", "value");
    env_document doc = builder.build();
    EXPECT_EQ(doc.get_string("KEY"), "value");
}

class IniValueTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(IniValueTest, PropertyValueAndType) {
    const ini_value value = ini_value::make_property("test_value");
    EXPECT_TRUE(value.is_property());
    EXPECT_EQ(value.kind(), ini_kind::property);
    EXPECT_EQ(value.as_property().get_value(), "test_value");
    EXPECT_EQ(value.to_string(), "test_value");
}

TEST_F(IniValueTest, PropertyInlineAndLong) {
    const ini_value short_value = ini_value::make_property("short");
    EXPECT_EQ(short_value.as_property().get_value(), "short");
    const ini_value long_value = ini_value::make_property("0123456789abcdefghij");
    EXPECT_EQ(long_value.as_property().get_value(), "0123456789abcdefghij");
}

TEST_F(IniValueTest, PropertyGetInt) {
    EXPECT_EQ(ini_value::make_property("42").as_property().get_int(), 42);
    EXPECT_EQ(ini_value::make_property("-10").as_property().get_int(), -10);
    EXPECT_EQ(ini_value::make_property("bad").as_property().get_int(3), 3);
}

TEST_F(IniValueTest, PropertyGetDouble) {
    EXPECT_DOUBLE_EQ(ini_value::make_property("3.14").as_property().get_double(), 3.14);
    EXPECT_DOUBLE_EQ(ini_value::make_property("bad").as_property().get_double(1.5), 1.5);
}

TEST_F(IniValueTest, PropertyGetBool) {
    EXPECT_TRUE(ini_value::make_property("true").as_property().get_bool());
    EXPECT_FALSE(ini_value::make_property("false").as_property().get_bool());
    EXPECT_TRUE(ini_value::make_property("bad").as_property().get_bool(true));
}

TEST_F(IniValueTest, SectionApi) {
    ini_document doc;
    ini_section_mut mut = doc.new_section("database");
    mut.set_property("host", "localhost");
    mut.set_property("port", "5432");
    const ini_section section = doc.get_section("database");
    ASSERT_TRUE(section.valid());
    EXPECT_EQ(section.get_name(), "database");
    EXPECT_EQ(section.size(), 2);
    EXPECT_TRUE(section.has_property("host"));
    EXPECT_FALSE(section.has_property("missing"));
    EXPECT_EQ(section.get_property("host")->as_property().get_value(), "localhost");
    EXPECT_EQ(section.get_string("host"), "localhost");
    EXPECT_EQ(section.get_int("port"), 5432);
}

TEST_F(IniValueTest, SectionIteration) {
    ini_document doc;
    ini_section_mut mut = doc.new_section("s");
    mut.set_property("a", "1");
    mut.set_property("b", "2");
    size_t count = 0;
    for (const ini_member& member: doc.get_section("s")) {
        EXPECT_FALSE(member.key().empty());
        EXPECT_TRUE(member.value.is_property());
        ++count;
    }
    EXPECT_EQ(count, 2);
    EXPECT_EQ(doc.get_section("s").get_properties().size(), 2);
}

TEST_F(IniValueTest, SectionTypedGettersWithDefaults) {
    ini_document doc;
    ini_section_mut mut = doc.new_section("s");
    mut.set_property("int", "42");
    mut.set_property("double", "2.5");
    mut.set_property("bool", "true");
    const ini_section section = doc.get_section("s");
    EXPECT_EQ(section.get_int("int"), 42);
    EXPECT_EQ(section.get_int("missing", 7), 7);
    EXPECT_DOUBLE_EQ(section.get_double("double"), 2.5);
    EXPECT_EQ(section.get_string("missing", "def"), "def");
    EXPECT_TRUE(section.get_bool("bool"));
    EXPECT_TRUE(section.get_bool("missing", true));
}

TEST_F(IniValueTest, SectionToString) {
    ini_document doc;
    ini_section_mut mut = doc.new_section("db");
    mut.set_property("host", "localhost");
    EXPECT_EQ(doc.get_section("db").to_string(), "[db]\nhost = localhost\n");
}

TEST_F(IniValueTest, DocumentGetSection) {
    ini_document doc;
    doc.new_section("a").set_property("k", "v");
    EXPECT_TRUE(doc.has_section("a"));
    EXPECT_FALSE(doc.has_section("b"));
    EXPECT_EQ(doc.get_section("a").get_string("k"), "v");
    EXPECT_FALSE(doc.get_section("b").valid());
    EXPECT_EQ(doc.get_sections().size(), 1);
    EXPECT_EQ(doc.get_sections().get(0).get_name(), "a");
}

TEST_F(IniValueTest, DocumentGlobalSection) {
    ini_document doc;
    doc.global_section_mut().set_property("global", "1");
    EXPECT_EQ(doc.get_global_section().get_string("global"), "1");
    EXPECT_EQ(doc.get_string("", "global"), "1");
}

TEST_F(IniValueTest, DocumentTypedGetters) {
    ini_document doc;
    ini_section_mut mut = doc.new_section("s");
    mut.set_property("int", "5");
    mut.set_property("double", "0.5");
    mut.set_property("bool", "false");
    EXPECT_EQ(doc.get_int("s", "int"), 5);
    EXPECT_EQ(doc.get_int("s", "missing", 9), 9);
    EXPECT_DOUBLE_EQ(doc.get_double("s", "double"), 0.5);
    EXPECT_EQ(doc.get_string("s", "missing", "def"), "def");
    EXPECT_FALSE(doc.get_bool("s", "bool"));
}

TEST_F(IniValueTest, DocumentMoveConstructor) {
    ini_document doc;
    doc.new_section("s").set_property("k", "v");
    ini_document moved(move(doc));
    EXPECT_EQ(moved.get_section("s").get_string("k"), "v");
}

TEST_F(IniValueTest, DocumentMoveAssignment) {
    ini_document doc1;
    doc1.new_section("a").set_property("k", "v");
    ini_document doc2;
    doc2 = move(doc1);
    EXPECT_TRUE(doc2.has_section("a"));
}

TEST_F(IniValueTest, DocumentToString) {
    ini_document doc;
    doc.global_section_mut().set_property("g", "1");
    doc.new_section("s").set_property("k", "v");
    EXPECT_EQ(doc.to_string(), "g = 1\n\n[s]\nk = v\n\n");
}

TEST_F(IniValueTest, DocumentToStringEmpty) {
    ini_document doc;
    EXPECT_EQ(doc.to_string(), "");
}

class IniParserTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(IniParserTest, ParseEmpty) {
    ini_parser parser("");
    ini_document doc = parser.parse();
    EXPECT_TRUE(doc.get_global_section().valid());
    EXPECT_TRUE(doc.get_sections().empty());
}

TEST_F(IniParserTest, ParseGlobalKeyValue) {
    ini_parser parser("key = value");
    ini_document doc = parser.parse();
    EXPECT_EQ(doc.get_global_section().get_string("key"), "value");
}

TEST_F(IniParserTest, ParseMultipleGlobalKeyValues) {
    ini_parser parser("key1 = value1\nkey2 = value2");
    ini_document doc = parser.parse();
    EXPECT_EQ(doc.get_global_section().get_string("key1"), "value1");
    EXPECT_EQ(doc.get_global_section().get_string("key2"), "value2");
}

TEST_F(IniParserTest, ParseSection) {
    ini_parser parser("[database]\nhost = localhost");
    ini_document doc = parser.parse();
    ASSERT_TRUE(doc.has_section("database"));
    EXPECT_EQ(doc.get_section("database").get_string("host"), "localhost");
}

TEST_F(IniParserTest, ParseMultipleSections) {
    ini_parser parser("[s1]\na = 1\n[s2]\nb = 2");
    ini_document doc = parser.parse();
    EXPECT_EQ(doc.get_sections().size(), 2);
    EXPECT_EQ(doc.get_section("s1").get_string("a"), "1");
    EXPECT_EQ(doc.get_section("s2").get_string("b"), "2");
}

TEST_F(IniParserTest, ParseGlobalAndSection) {
    ini_parser parser("global_key = global_value\n[database]\nhost = localhost");
    ini_document doc = parser.parse();
    EXPECT_EQ(doc.get_global_section().get_string("global_key"), "global_value");
    EXPECT_EQ(doc.get_section("database").get_string("host"), "localhost");
}

TEST_F(IniParserTest, ParseTrimsSpaces) {
    ini_parser parser("  key  =   value with spaces   ");
    ini_document doc = parser.parse();
    EXPECT_EQ(doc.get_global_section().get_string("key"), "value with spaces");
}

TEST_F(IniParserTest, ParseQuotedValues) {
    ini_parser parser("a = \"quoted value\"\nb = 'single quoted'");
    ini_document doc = parser.parse();
    EXPECT_EQ(doc.get_string("", "a"), "quoted value");
    EXPECT_EQ(doc.get_string("", "b"), "single quoted");
}

TEST_F(IniParserTest, ParseComments) {
    ini_parser parser("; semicolon\n# hash\nkey = value");
    ini_document doc = parser.parse();
    EXPECT_EQ(doc.get_global_section().get_string("key"), "value");
}

TEST_F(IniParserTest, ParseInlineCommentIsValue) {
    ini_parser parser("key = value ; inline comment");
    ini_document doc = parser.parse();
    EXPECT_EQ(doc.get_global_section().get_string("key"), "value ; inline comment");
}

TEST_F(IniParserTest, ParseEmptyLines) {
    ini_parser parser("\n\nkey = value\n\n");
    ini_document doc = parser.parse();
    EXPECT_EQ(doc.get_global_section().get_string("key"), "value");
}

TEST_F(IniParserTest, ParseNoEqualsSignIgnored) {
    ini_parser parser("no equal sign\nkey = value");
    ini_document doc = parser.parse();
    EXPECT_EQ(doc.get_global_section().size(), 1);
}

TEST_F(IniParserTest, ParseEmptyKeyIgnored) {
    ini_parser parser(" = value\nkey = value");
    ini_document doc = parser.parse();
    EXPECT_EQ(doc.get_global_section().size(), 1);
}

TEST_F(IniParserTest, ParseDuplicateKeyOverwrites) {
    ini_parser parser("key = first\nkey = second");
    ini_document doc = parser.parse();
    EXPECT_EQ(doc.get_global_section().size(), 1);
    EXPECT_EQ(doc.get_global_section().get_string("key"), "second");
}

TEST_F(IniParserTest, ParseDuplicateSectionResets) {
    ini_parser parser("[s]\na = 1\n[s]\nb = 2");
    ini_document doc = parser.parse();
    EXPECT_EQ(doc.get_sections().size(), 1);
    EXPECT_FALSE(doc.get_section("s").has_property("a"));
    EXPECT_EQ(doc.get_section("s").get_string("b"), "2");
}

TEST_F(IniParserTest, ParseValueWithEquals) {
    ini_parser parser("key = a=b=c");
    ini_document doc = parser.parse();
    EXPECT_EQ(doc.get_global_section().get_string("key"), "a=b=c");
}

TEST_F(IniParserTest, ParseCRLF) {
    ini_parser parser("a = 1\r\n[s]\r\nb = 2\r\n");
    ini_document doc = parser.parse();
    EXPECT_EQ(doc.get_string("", "a"), "1");
    EXPECT_EQ(doc.get_string("s", "b"), "2");
}

TEST_F(IniParserTest, TryParseValid) {
    ini_parser parser("[s]\nk = v");
    auto result = parser.try_parse();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->get_string("s", "k"), "v");
}

TEST_F(IniParserTest, ParseBorrowedText) {
    const string text = "[db]\nhost = localhost\nport = 5432";
    ini_parser parser(text.view());
    ini_document doc = parser.parse();
    EXPECT_EQ(doc.get_string("db", "host"), "localhost");
    EXPECT_EQ(doc.get_int("db", "port"), 5432);
}

TEST_F(IniParserTest, ParseOwnedText) {
    ini_document doc = ini_parser(string("[db]\nhost = localhost")).parse();
    EXPECT_EQ(doc.get_string("db", "host"), "localhost");
}

TEST_F(IniParserTest, ParseComplexDocument) {
    const string text =
            "global = 1\n"
            "; comment\n"
            "[database]\n"
            "host = localhost\n"
            "port = 5432\n"
            "enabled = true\n"
            "[logging]\n"
            "level = \"debug\"\n"
            "path = 'a b'\n";
    ini_document doc = ini_parser(text.view()).parse();
    EXPECT_EQ(doc.get_int("", "global"), 1);
    EXPECT_EQ(doc.get_string("database", "host"), "localhost");
    EXPECT_EQ(doc.get_int("database", "port"), 5432);
    EXPECT_TRUE(doc.get_bool("database", "enabled"));
    EXPECT_EQ(doc.get_string("logging", "level"), "debug");
    EXPECT_EQ(doc.get_string("logging", "path"), "a b");
    EXPECT_EQ(doc.get_sections().size(), 2);
}

TEST_F(IniParserTest, ParseToString) {
    ini_parser parser("g = 1\n[s]\nk = v");
    const ini_document doc = parser.parse();
    EXPECT_EQ(doc.to_string(), "g = 1\n\n[s]\nk = v\n\n");
}

class IniBuilderTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(IniBuilderTest, BuildGlobalString) {
    ini_builder builder;
    builder.key("key").value("value");
    EXPECT_EQ(builder.build().get_global_section().get_string("key"), "value");
}

TEST_F(IniBuilderTest, BuildGlobalCString) {
    ini_builder builder;
    builder.key("key").value("value");
    EXPECT_EQ(builder.build().get_string("", "key"), "value");
}

TEST_F(IniBuilderTest, BuildGlobalStringView) {
    ini_builder builder;
    builder.key("key").value(string_view("value"));
    EXPECT_EQ(builder.build().get_string("", "key"), "value");
}

TEST_F(IniBuilderTest, BuildNumericValues) {
    ini_builder builder;
    builder.key("int").value(42);
    builder.key("big").value(int64_t{1234567890123});
    builder.key("fp").value(2.5);
    builder.key("yes").value(true);
    builder.key("no").value(false);
    ini_document doc = builder.build();
    EXPECT_EQ(doc.get_int("", "int"), 42);
    EXPECT_EQ(doc.get_int64("", "big"), 1234567890123);
    EXPECT_DOUBLE_EQ(doc.get_double("", "fp"), 2.5);
    EXPECT_TRUE(doc.get_bool("", "yes"));
    EXPECT_FALSE(doc.get_bool("", "no"));
}

TEST_F(IniBuilderTest, BuildDoubleWithPrecision) {
    ini_builder builder;
    builder.key("pi").value(3.14159, 2);
    EXPECT_EQ(builder.build().get_string("", "pi"), "3.14");
}

TEST_F(IniBuilderTest, BuildMultipleGlobalKeys) {
    ini_builder builder;
    builder.key("a").value(1).key("b").value(2);
    ini_document doc = builder.build();
    EXPECT_EQ(doc.get_global_section().size(), 2);
    EXPECT_EQ(doc.get_int("", "a"), 1);
    EXPECT_EQ(doc.get_int("", "b"), 2);
}

TEST_F(IniBuilderTest, BuildSection) {
    ini_builder builder;
    builder.begin_section("db").key("host").value("localhost").end_section();
    ini_document doc = builder.build();
    ASSERT_TRUE(doc.has_section("db"));
    EXPECT_EQ(doc.get_string("db", "host"), "localhost");
}

TEST_F(IniBuilderTest, BuildMultipleSections) {
    ini_builder builder;
    builder.begin_section("a").key("x").value(1).end_section();
    builder.begin_section("b").key("y").value(2).end_section();
    ini_document doc = builder.build();
    EXPECT_EQ(doc.get_sections().size(), 2);
    EXPECT_EQ(doc.get_int("a", "x"), 1);
    EXPECT_EQ(doc.get_int("b", "y"), 2);
}

TEST_F(IniBuilderTest, BuildSectionAndGlobal) {
    ini_builder builder;
    builder.key("g").value(1).begin_section("s").key("k").value(2).end_section();
    ini_document doc = builder.build();
    EXPECT_EQ(doc.get_int("", "g"), 1);
    EXPECT_EQ(doc.get_int("s", "k"), 2);
}

TEST_F(IniBuilderTest, BuildValueSection) {
    ini_builder builder;
    builder.value_section("s", [](ini_builder& b) { b.key("k").value("v"); });
    EXPECT_EQ(builder.build().get_string("s", "k"), "v");
}

TEST_F(IniBuilderTest, BuildNestedValueSections) {
    ini_builder builder;
    builder.value_section("outer", [](ini_builder& b) {
        b.key("a").value(1);
        b.value_section("inner", [](ini_builder& inner) { inner.key("b").value(2); });
    });
    ini_document doc = builder.build();
    EXPECT_EQ(doc.get_int("outer", "a"), 1);
    EXPECT_EQ(doc.get_int("inner", "b"), 2);
}

TEST_F(IniBuilderTest, BuildSectionOverwrite) {
    ini_builder builder;
    builder.begin_section("s").key("a").value(1).end_section();
    builder.begin_section("s").key("b").value(2).end_section();
    ini_document doc = builder.build();
    EXPECT_EQ(doc.get_sections().size(), 1);
    EXPECT_FALSE(doc.get_section("s").has_property("a"));
    EXPECT_EQ(doc.get_int("s", "b"), 2);
}

TEST_F(IniBuilderTest, BuildThrowsWithoutKey) {
    ini_builder builder;
    EXPECT_THROW(builder.value("x"), ini_exception);
}

TEST_F(IniBuilderTest, BuildMove) {
    ini_builder builder;
    builder.key("k").value("v");
    ini_document doc = builder.build();
    EXPECT_EQ(doc.get_string("", "k"), "v");
}

TEST_F(IniBuilderTest, BuildEndSectionResetsToGlobal) {
    ini_builder builder;
    builder.begin_section("s").key("a").value(1).end_section();
    builder.key("g").value(2);
    ini_document doc = builder.build();
    EXPECT_EQ(doc.get_int("s", "a"), 1);
    EXPECT_EQ(doc.get_int("", "g"), 2);
}

namespace {
    string int_text(const int value) {
        string result;
        format_to(result, "{0}", value);
        return result;
    }
} // namespace

class JsonValueTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(JsonValueTest, ValueLayoutIsCompact) {
    EXPECT_EQ(sizeof(json_value), 16u);
    EXPECT_EQ(sizeof(json_member), 32u);
}

TEST_F(JsonValueTest, JsonNullType) {
    const json_value value = json_value::make_null();
    EXPECT_EQ(value.kind(), json_kind::null_);
    EXPECT_TRUE(value.is_null());
    EXPECT_FALSE(value.is_bool());
    EXPECT_FALSE(value.is_number());
    EXPECT_FALSE(value.is_string());
    EXPECT_FALSE(value.is_object());
    EXPECT_FALSE(value.is_array());
}

TEST_F(JsonValueTest, JsonNullToString) { EXPECT_EQ(json_value::make_null().to_string(), "null"); }

TEST_F(JsonValueTest, JsonBoolType) {
    const json_value true_value = json_value::make_bool(true);
    const json_value false_value = json_value::make_bool(false);

    EXPECT_EQ(true_value.kind(), json_kind::boolean);
    EXPECT_TRUE(true_value.is_bool());
    EXPECT_FALSE(true_value.is_null());
    EXPECT_FALSE(true_value.is_number());
    EXPECT_TRUE(true_value.as_bool().get_value());
    EXPECT_FALSE(false_value.as_bool().get_value());
    EXPECT_EQ(true_value.to_string(), "true");
    EXPECT_EQ(false_value.to_string(), "false");
}

TEST_F(JsonValueTest, JsonNumberType) {
    const json_value integer = json_value::make_integer(42);
    const json_value real = json_value::make_real(3.14);

    EXPECT_EQ(integer.kind(), json_kind::integer);
    EXPECT_TRUE(integer.is_number());
    EXPECT_FALSE(integer.is_null());
    EXPECT_FALSE(integer.is_bool());
    EXPECT_FALSE(integer.is_string());
    EXPECT_EQ(integer.as_number().as_integer(), 42);
    EXPECT_TRUE(integer.as_number().is_integral());

    EXPECT_EQ(real.kind(), json_kind::real);
    EXPECT_DOUBLE_EQ(real.as_number().get_value(), 3.14);
    EXPECT_FALSE(real.as_number().is_integral());
}

TEST_F(JsonValueTest, JsonUintegerExact) {
    const uint64_t big = 18446744073709551615ULL;
    const json_value value = json_value::make_uinteger(big);
    EXPECT_EQ(value.kind(), json_kind::uinteger);
    EXPECT_EQ(value.as_number().as_uinteger(), big);
    EXPECT_EQ(value.to_string(), "18446744073709551615");
}

TEST_F(JsonValueTest, JsonStringType) {
    const char* literal = "hello";
    const json_value value = json_value::make_string(literal);

    EXPECT_EQ(value.kind(), json_kind::string);
    EXPECT_TRUE(value.is_string());
    EXPECT_EQ(value.as_string().get_value(), "hello");
    EXPECT_EQ(value.as_string().data(), literal);
    EXPECT_EQ(value.to_string(), "\"hello\"");
}

TEST_F(JsonValueTest, JsonIntegerToString) {
    EXPECT_EQ(json_value::make_integer(42).to_string(), "42");
    EXPECT_EQ(json_value::make_integer(-9223372036854775807LL).to_string(), "-9223372036854775807");
    EXPECT_EQ(json_value::make_integer(-9223372036854775807LL - 1).to_string(), "-9223372036854775808");
}

TEST_F(JsonValueTest, JsonStringElementCount) {
    const json_value borrowed = json_value::make_string("borrowed text");
    EXPECT_EQ(borrowed.element_count(), 13u);
    EXPECT_EQ(borrowed.as_string().size(), 13u);

    json_document doc;
    const json_value inlined = doc.make_string_value(string_view("inlined"));
    EXPECT_EQ(inlined.element_count(), 7u);
    EXPECT_EQ(inlined.as_string().size(), 7u);
    EXPECT_EQ(inlined.to_string(), "\"inlined\"");
}

TEST_F(JsonValueTest, JsonRealToString) {
    json_document doc;
    doc.root() = json_value::make_real(3.14);
    EXPECT_EQ(doc->to_string(), "3.14");

    doc.root() = json_value::make_real(0.5);
    EXPECT_EQ(doc->to_string(), "0.5");

    doc.root() = json_value::make_real(-1.5);
    EXPECT_EQ(doc->to_string(), "-1.5");

    doc.root() = json_value::make_real(2.0);
    EXPECT_EQ(doc->to_string(), "2.0");

    doc.root() = json_value::make_real(0.0);
    EXPECT_EQ(doc->to_string(), "0.0");
}

TEST_F(JsonValueTest, JsonRealRoundTrip) {
    const double values[] = {3.14, 0.5, -1.5, 2.0, 1234.5, 0.1, 1e-7, -7.25e-9, 9007199254740992.0, 1e22};
    for (const double value: values) {
        json_document doc;
        doc.root() = json_value::make_real(value);
        const string text = doc->to_string();
        const json_document parsed = json_parser{string_view(text.data(), text.size())}.parse();
        EXPECT_EQ(parsed->kind(), json_kind::real) << text.data();
        EXPECT_TRUE(parsed->as_number().get_value() == value) << text.data();
    }
}

TEST_F(JsonValueTest, JsonRealExtremeExponentRoundTrip) {
    const double values[] = {1e30, 1e-30, 1.7976931348623157e308, 2.2250738585072014e-308, 4.9406564584124654e-324};
    for (const double value: values) {
        json_document doc;
        doc.root() = json_value::make_real(value);
        const string text = doc->to_string();
        const json_document parsed = json_parser{string_view(text.data(), text.size())}.parse();
        const double back = parsed->as_number().get_value();
        uint64_t expected = 0;
        uint64_t actual = 0;
        memory_copy(&expected, &value, sizeof(expected));
        memory_copy(&actual, &back, sizeof(actual));
        EXPECT_EQ(actual, expected) << text.data();
    }
}

TEST_F(JsonValueTest, JsonStringEscapedToString) {
    const json_value value = json_value::make_string("a\"b\\c\nd\te");
    EXPECT_EQ(value.to_string(), "\"a\\\"b\\\\c\\nd\\te\"");
}

TEST_F(JsonValueTest, JsonStringControlCharacterToString) {
    const char raw[] = {'a', char(0x01), '\0'};
    const json_value value = json_value::make_string(string_view(raw, 2));
    EXPECT_EQ(value.to_string(), "\"a\\u0001\"");
}

TEST_F(JsonValueTest, JsonObjectAndArrayKind) {
    json_document doc;
    const json_value* node = doc.new_container(true);
    EXPECT_TRUE(node->is_object());
    EXPECT_EQ(node->element_count(), 0u);
    EXPECT_EQ(node->to_string(), "{}");

    const json_value* elements = doc.new_container(false);
    EXPECT_TRUE(elements->is_array());
    EXPECT_EQ(elements->to_string(), "[]");
}

TEST_F(JsonValueTest, JsonNumberViewMissingValue) {
    const json_number value = json_value::make_integer(7).as_number();
    EXPECT_EQ(value.as_integer(), 7);
    EXPECT_DOUBLE_EQ(value.get_value(), 7.0);
}

class JsonObjectTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(JsonObjectTest, AddAndGetMember) {
    json_document doc;
    json_object_mut object = doc.make_object();
    object.add_member("name", json_value::make_string("nexus"));
    object.add_member("id", json_value::make_integer(42));

    ASSERT_EQ(doc->as_object().size(), 2u);
    ASSERT_NE(doc->as_object().get_member("name"), nullptr);
    EXPECT_EQ(doc->as_object().get_member("name")->as_string().get_value(), "nexus");
    EXPECT_EQ(doc->as_object().get_member("id")->as_number().as_integer(), 42);
    EXPECT_EQ(doc->as_object().get_member("missing"), nullptr);
}

TEST_F(JsonObjectTest, GetMemberKeyOverloads) {
    json_document doc;
    json_object_mut object = doc.make_object();
    object.add_member("key", json_value::make_bool(true));

    const json_object view = doc->as_object();
    const string key("key");
    EXPECT_NE(view.get_member(key), nullptr);
    EXPECT_NE(view.get_member("key"), nullptr);
    EXPECT_NE(view.get_member(string_view("key")), nullptr);
    EXPECT_EQ(view.get_member(string("other")), nullptr);
}

TEST_F(JsonObjectTest, GetMembersIteration) {
    json_document doc;
    json_object_mut object = doc.make_object();
    object.add_member("a", json_value::make_integer(1));
    object.add_member("b", json_value::make_integer(2));
    object.add_member("c", json_value::make_integer(3));

    int64_t sum = 0;
    size_t count = 0;
    for (const json_member& member: doc->as_object().get_members()) {
        sum += member.value.as_number().as_integer();
        ++count;
        EXPECT_FALSE(member.key().empty());
    }
    EXPECT_EQ(count, 3u);
    EXPECT_EQ(sum, 6);
}

TEST_F(JsonObjectTest, InsertionOrderPreserved) {
    json_document doc;
    json_object_mut object = doc.make_object();
    object.add_member("first", json_value::make_integer(1));
    object.add_member("second", json_value::make_integer(2));

    auto it = doc->as_object().begin();
    EXPECT_EQ(it->key(), "first");
    ++it;
    EXPECT_EQ(it->key(), "second");
}

TEST_F(JsonObjectTest, InlineKeyAndArenaKey) {
    json_document doc;
    json_object_mut object = doc.make_object();
    object.add_member("short", json_value::make_integer(1));
    object.add_member("a_key_longer_than_eight_bytes", json_value::make_integer(2));

    const json_object view = doc->as_object();
    auto it = view.begin();
    EXPECT_TRUE(it->key_inlined());
    EXPECT_EQ(it->key(), "short");
    ++it;
    EXPECT_FALSE(it->key_inlined());
    EXPECT_EQ(it->key(), "a_key_longer_than_eight_bytes");
    EXPECT_NE(view.get_member("a_key_longer_than_eight_bytes"), nullptr);
}

TEST_F(JsonObjectTest, LargeMemberLookup) {
    json_document doc;
    json_object_mut object = doc.make_object();
    for (int i = 0; i < 600; ++i) {
        object.add_member(string("key") + int_text(i), json_value::make_integer(i));
    }
    ASSERT_EQ(doc->as_object().size(), 600u);

    for (int i = 0; i < 600; ++i) {
        const json_value* value = doc->as_object().get_member(string("key") + int_text(i));
        ASSERT_NE(value, nullptr);
        EXPECT_EQ(value->as_number().as_integer(), i);
    }
    EXPECT_EQ(doc->as_object().get_member("key600"), nullptr);
}

TEST_F(JsonObjectTest, KeyPrefixFiltersLookups) {
    const json_member member = [] {
        json_member result;
        result.assign_key("hash");
        return result;
    }();
    EXPECT_EQ(member.key_prefix, json_member::prefix_of("hash"));
    EXPECT_NE(member.key_prefix, json_member::prefix_of("hasj"));
    EXPECT_FALSE(member.key_equal(json_member::prefix_of("hasj"), "hasj"));
    EXPECT_TRUE(member.key_equal(json_member::prefix_of("hash"), "hash"));

    json_member short_key;
    short_key.assign_key("ab");
    EXPECT_EQ(short_key.key_prefix, json_member::prefix_of("ab"));
    EXPECT_TRUE(short_key.key_equal(json_member::prefix_of("ab"), "ab"));
    EXPECT_FALSE(short_key.key_equal(json_member::prefix_of("ab_"), "ab_"));

    json_member long_key;
    long_key.assign_key("a-longer-key");
    EXPECT_FALSE(long_key.key_inlined());
    EXPECT_TRUE(long_key.key_equal(json_member::prefix_of("a-longer-key"), "a-longer-key"));
    EXPECT_FALSE(long_key.key_equal(json_member::prefix_of("a-longer-kez"), "a-longer-kez"));
}

TEST_F(JsonObjectTest, ReservePreallocatesMembers) {
    json_document doc;
    json_object_mut object = doc.make_object();
    doc.object_reserve(doc.root(), 8);
    for (int i = 0; i < 8; ++i) {
        object.add_member(string("key") + int_text(i), json_value::make_integer(i));
    }
    EXPECT_EQ(object.size(), 8u);
    for (int i = 0; i < 8; ++i) {
        EXPECT_EQ(object.as_object().get_member(string("key") + int_text(i))->as_number().as_integer(), i);
    }

    object.add_member("extra", json_value::make_integer(99));
    EXPECT_EQ(object.size(), 9u);
    EXPECT_EQ(object.as_object().get_member("extra")->as_number().as_integer(), 99);
    EXPECT_EQ(object.as_object().get_member("key7")->as_number().as_integer(), 7);
    EXPECT_EQ(doc.root().element_count(), 9u);
}

TEST_F(JsonObjectTest, ReserveIgnoresUnsupportedCounts) {
    json_document doc;
    json_object_mut object = doc.make_object();
    doc.object_reserve(doc.root(), 0);
    doc.object_reserve(doc.root(), 70000);
    object.add_member("a", json_value::make_integer(1));
    doc.object_reserve(doc.root(), 4);
    object.add_member("b", json_value::make_integer(2));
    EXPECT_EQ(object.size(), 2u);
    EXPECT_EQ(doc.root().to_string(), R"({"a":1,"b":2})");
}

TEST_F(JsonObjectTest, ObjectToString) {
    json_document doc;
    json_object_mut object = doc.make_object();
    object.add_member("id", json_value::make_integer(1));
    object.add_member("name", json_value::make_string("nexus"));

    EXPECT_EQ(doc->to_string(), "{\"id\":1,\"name\":\"nexus\"}");
}

TEST_F(JsonObjectTest, NestedObject) {
    json_document doc;
    json_object_mut root = doc.make_object();
    json_object_mut inner = doc.new_object();
    inner.add_member("leaf", json_value::make_bool(true));
    root.add_member("inner", inner.to_value());

    EXPECT_EQ(doc->to_string(), "{\"inner\":{\"leaf\":true}}");
    EXPECT_TRUE(doc->as_object().get_member("inner")->as_object().get_member("leaf")->as_bool().get_value());
}

TEST_F(JsonObjectTest, DocumentMoveConstructor) {
    json_document doc;
    json_object_mut object = doc.make_object();
    object.add_member("key", json_value::make_integer(7));

    json_document moved(move(doc));
    ASSERT_NE(moved->as_object().get_member("key"), nullptr);
    EXPECT_EQ(moved->as_object().get_member("key")->as_number().as_integer(), 7);
}

TEST_F(JsonObjectTest, DocumentMoveAssignment) {
    json_document doc;
    doc.make_object().add_member("key", json_value::make_integer(7));

    json_document other;
    other = move(doc);
    ASSERT_NE(other->as_object().get_member("key"), nullptr);
    EXPECT_EQ(other->as_object().get_member("key")->as_number().as_integer(), 7);
}

class JsonArrayTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(JsonArrayTest, AddAndGetElement) {
    json_document doc;
    json_array_mut array = doc.make_array();
    array.add_element(json_value::make_integer(1));
    array.add_element(json_value::make_string("two"));
    array.add_element(json_value::make_bool(false));

    ASSERT_EQ(doc->as_array().size(), 3u);
    EXPECT_EQ(doc->as_array().get_element(0)->as_number().as_integer(), 1);
    EXPECT_EQ(doc->as_array().get_element(1)->as_string().get_value(), "two");
    EXPECT_FALSE(doc->as_array().get_element(2)->as_bool().get_value());
    EXPECT_EQ(doc->as_array().get_element(3), nullptr);
}

TEST_F(JsonArrayTest, GetElementsIteration) {
    json_document doc;
    json_array_mut array = doc.make_array();
    for (int i = 0; i < 5; ++i) {
        array.add_element(json_value::make_integer(i));
    }

    int64_t sum = 0;
    for (const json_value& element: doc->as_array().get_elements()) {
        sum += element.as_number().as_integer();
    }
    EXPECT_EQ(sum, 10);
}

TEST_F(JsonArrayTest, LargeArrayGrowth) {
    json_document doc;
    json_array_mut array = doc.make_array();
    for (int i = 0; i < 1000; ++i) {
        array.add_element(json_value::make_integer(i));
    }
    ASSERT_EQ(doc->as_array().size(), 1000u);
    EXPECT_EQ(doc->as_array().get_element(0)->as_number().as_integer(), 0);
    EXPECT_EQ(doc->as_array().get_element(511)->as_number().as_integer(), 511);
    EXPECT_EQ(doc->as_array().get_element(999)->as_number().as_integer(), 999);
}

TEST_F(JsonArrayTest, ReservePreallocatesElements) {
    json_document doc;
    json_array_mut array = doc.make_array();
    doc.array_reserve(doc.root(), 5);
    for (int i = 0; i < 9; ++i) {
        array.add_element(json_value::make_integer(i));
    }
    EXPECT_EQ(array.size(), 9u);
    EXPECT_EQ(doc.root().to_string(), "[0,1,2,3,4,5,6,7,8]");
    EXPECT_EQ(doc.root().element_count(), 9u);
}

TEST_F(JsonArrayTest, ArrayToString) {
    json_document doc;
    json_array_mut array = doc.make_array();
    array.add_element(json_value::make_integer(1));
    array.add_element(json_value::make_integer(2));

    EXPECT_EQ(doc->to_string(), "[1,2]");
}

TEST_F(JsonArrayTest, NestedArray) {
    json_document doc;
    json_array_mut root = doc.make_array();
    json_array_mut inner = doc.new_array();
    inner.add_element(json_value::make_integer(1));
    root.add_element(inner.to_value());
    root.add_element(json_value::make_integer(2));

    EXPECT_EQ(doc->to_string(), "[[1],2]");
    EXPECT_EQ(doc->as_array().get_element(0)->as_array().size(), 1u);
}

TEST_F(JsonArrayTest, ArrayOfObjects) {
    json_document doc;
    json_array_mut root = doc.make_array();
    for (int i = 0; i < 3; ++i) {
        json_object_mut object = doc.new_object();
        object.add_member("id", json_value::make_integer(i));
        root.add_element(object.to_value());
    }

    EXPECT_EQ(doc->as_array().size(), 3u);
    for (size_t i = 0; i < 3; ++i) {
        const json_value* element = doc->as_array().get_element(i);
        ASSERT_NE(element, nullptr);
        EXPECT_EQ(element->as_object().get_member("id")->as_number().as_integer(), static_cast<int64_t>(i));
    }
}

class JsonSerializeTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(JsonSerializeTest, ToIndentedString) {
    json_document doc;
    json_object_mut object = doc.make_object();
    object.add_member("id", json_value::make_integer(1));
    object.add_member("name", json_value::make_string("nexus"));

    EXPECT_EQ(doc->to_indented_string(), "{\n  \"id\": 1,\n  \"name\": \"nexus\"\n}");
    EXPECT_EQ(doc->to_indented_string(4), "{\n    \"id\": 1,\n    \"name\": \"nexus\"\n}");
}

TEST_F(JsonSerializeTest, ToIndentedStringArray) {
    json_document doc;
    json_array_mut array = doc.make_array();
    array.add_element(json_value::make_integer(1));
    array.add_element(json_value::make_integer(2));

    EXPECT_EQ(doc->to_indented_string(2), "[\n  1,\n  2\n]");
}

TEST_F(JsonSerializeTest, ToIndentedStringEmptyContainers) {
    json_document object_doc;
    object_doc.make_object();
    EXPECT_EQ(object_doc->to_indented_string(), "{}");

    json_document array_doc;
    array_doc.make_array();
    EXPECT_EQ(array_doc->to_indented_string(), "[]");
}

TEST_F(JsonSerializeTest, ToIndentedStringNested) {
    json_document doc;
    json_object_mut object = doc.make_object();
    json_array_mut array = doc.new_array();
    array.add_element(json_value::make_integer(1));
    object.add_member("list", array.to_value());

    EXPECT_EQ(doc->to_indented_string(2), "{\n  \"list\": [\n    1\n  ]\n}");
}

class JsonParserTest : public ::testing::Test {
protected:
    static json_document parse_text(const string_view text) {
        json_parser parser(text);
        return parser.parse();
    }

    static json_document parse_owned(string text) {
        json_parser parser(move(text));
        return parser.parse();
    }

    static json_document parse_text(const string& text) { return parse_text(string_view(text.data(), text.size())); }

    static json_document parse_text(const char* text) { return parse_text(string_view(text)); }
};

TEST_F(JsonParserTest, ParseNull) {
    const json_document doc = parse_text("null");
    EXPECT_TRUE(doc->is_null());
}

TEST_F(JsonParserTest, ParseTrue) { EXPECT_TRUE(parse_text("true").root().as_bool().get_value()); }

TEST_F(JsonParserTest, ParseFalse) { EXPECT_FALSE(parse_text("false").root().as_bool().get_value()); }

TEST_F(JsonParserTest, ParseInteger) {
    const json_document doc = parse_text("42");
    EXPECT_TRUE(doc->is_number());
    EXPECT_EQ(doc->as_number().as_integer(), 42);
}

TEST_F(JsonParserTest, ParseNegativeInteger) { EXPECT_EQ(parse_text("-42")->as_number().as_integer(), -42); }

TEST_F(JsonParserTest, ParseInt64Boundary) {
    const json_document min_doc = parse_text("-9223372036854775808");
    EXPECT_EQ(min_doc->as_number().as_integer(), -9223372036854775807LL - 1);

    const json_document max_doc = parse_text("9223372036854775807");
    EXPECT_EQ(max_doc->as_number().as_integer(), 9223372036854775807LL);
}

TEST_F(JsonParserTest, ParseUint64Boundary) {
    const json_document doc = parse_text("18446744073709551615");
    EXPECT_EQ(doc->kind(), json_kind::uinteger);
    EXPECT_EQ(doc->as_number().as_uinteger(), 18446744073709551615ULL);
}

TEST_F(JsonParserTest, ParseLargeIntegerKeepsPrecision) {
    const json_document doc = parse_text("9007199254740993");
    EXPECT_EQ(doc->kind(), json_kind::integer);
    EXPECT_EQ(doc->as_number().as_integer(), 9007199254740993LL);
    EXPECT_EQ(doc->to_string(), "9007199254740993");
}

TEST_F(JsonParserTest, ParseFloat) {
    const json_document doc = parse_text("3.14");
    EXPECT_TRUE(doc->is_number());
    EXPECT_EQ(doc->kind(), json_kind::real);
    EXPECT_DOUBLE_EQ(doc->as_number().get_value(), 3.14);
}

TEST_F(JsonParserTest, ParseNegativeFloat) { EXPECT_DOUBLE_EQ(parse_text("-1.5")->as_number().get_value(), -1.5); }

TEST_F(JsonParserTest, ParseExponentNumber) {
    EXPECT_DOUBLE_EQ(parse_text("1e3")->as_number().get_value(), 1000.0);
    EXPECT_DOUBLE_EQ(parse_text("1E3")->as_number().get_value(), 1000.0);
    EXPECT_DOUBLE_EQ(parse_text("1e-2")->as_number().get_value(), 0.01);
    EXPECT_DOUBLE_EQ(parse_text("1e+2")->as_number().get_value(), 100.0);
    EXPECT_DOUBLE_EQ(parse_text("2.5e2")->as_number().get_value(), 250.0);
}

TEST_F(JsonParserTest, ParseShortestDecimalIsExact) {
    const json_document doc = parse_text("52.285");
    EXPECT_TRUE(doc->as_number().get_value() == 52.285);
    EXPECT_EQ(doc->to_string(), "52.285");

    const json_document negative = parse_text("-956.084");
    EXPECT_TRUE(negative->as_number().get_value() == -956.084);

    const json_document exponent = parse_text("1.5e-43");
    EXPECT_DOUBLE_EQ(exponent->as_number().get_value(), 1.5e-43);
}

TEST_F(JsonParserTest, ParseZeroNumber) {
    EXPECT_EQ(parse_text("0")->as_number().as_integer(), 0);
    EXPECT_EQ(parse_text("-0")->as_number().as_integer(), 0);
    EXPECT_DOUBLE_EQ(parse_text("0.0")->as_number().get_value(), 0.0);
}

TEST_F(JsonParserTest, ParseEmptyString) {
    const json_document doc = parse_text("\"\"");
    EXPECT_TRUE(doc->is_string());
    EXPECT_TRUE(doc->as_string().get_value().empty());
}

TEST_F(JsonParserTest, ParseSimpleString) { EXPECT_EQ(parse_text("\"hello\"")->as_string().get_value(), "hello"); }

TEST_F(JsonParserTest, ParseStringWithSpaces) {
    EXPECT_EQ(parse_text("\"hello world\"")->as_string().get_value(), "hello world");
}

TEST_F(JsonParserTest, ParseLongStringIsArenaBacked) {
    const string_view text("\"a string much longer than fourteen bytes\"");
    const json_document doc = parse_owned(string(text.data(), text.size()));
    EXPECT_EQ(doc->as_string().get_value(), "a string much longer than fourteen bytes");
}

TEST_F(JsonParserTest, ParseStringEscapeSequences) {
    EXPECT_EQ(parse_text("\"a\\nb\"")->as_string().get_value(), "a\nb");
    EXPECT_EQ(parse_text("\"a\\tb\"")->as_string().get_value(), "a\tb");
    EXPECT_EQ(parse_text("\"a\\\\b\"")->as_string().get_value(), "a\\b");
    EXPECT_EQ(parse_text("\"a\\\"b\"")->as_string().get_value(), "a\"b");
    EXPECT_EQ(parse_text("\"a\\/b\"")->as_string().get_value(), "a/b");
    EXPECT_EQ(parse_text("\"a\\bb\"")->as_string().get_value(), "a\bb");
    EXPECT_EQ(parse_text("\"a\\fb\"")->as_string().get_value(), "a\fb");
    EXPECT_EQ(parse_text("\"a\\rb\"")->as_string().get_value(), "a\rb");
}

TEST_F(JsonParserTest, ParseStringUnicodeEscape) {
    EXPECT_EQ(parse_text("\"\\u00e9\"")->as_string().get_value(), "\xC3\xA9");
}

TEST_F(JsonParserTest, ParseStringUnicodeSurrogatePair) {
    EXPECT_EQ(parse_text("\"\\uD83D\\uDE00\"")->as_string().get_value(), "\xF0\x9F\x98\x80");
}

TEST_F(JsonParserTest, ParseEmptyArray) {
    const json_document doc = parse_text("[]");
    EXPECT_TRUE(doc->is_array());
    EXPECT_EQ(doc->as_array().size(), 0u);
}

TEST_F(JsonParserTest, ParseArrayWithSingleElement) {
    const json_document doc = parse_text("[1]");
    ASSERT_EQ(doc->as_array().size(), 1u);
    EXPECT_EQ(doc->as_array().get_element(0)->as_number().as_integer(), 1);
}

TEST_F(JsonParserTest, ParseArrayWithMultipleElements) {
    const json_document doc = parse_text("[1, 2, 3]");
    ASSERT_EQ(doc->as_array().size(), 3u);
    for (size_t i = 0; i < 3; ++i) {
        EXPECT_EQ(doc->as_array().get_element(i)->as_number().as_integer(), static_cast<int64_t>(i + 1));
    }
}

TEST_F(JsonParserTest, ParseArrayWithMixedTypes) {
    const json_document doc = parse_text("[1, \"two\", true, null, 3.5]");
    ASSERT_EQ(doc->as_array().size(), 5u);
    EXPECT_TRUE(doc->as_array().get_element(0)->is_number());
    EXPECT_TRUE(doc->as_array().get_element(1)->is_string());
    EXPECT_TRUE(doc->as_array().get_element(2)->is_bool());
    EXPECT_TRUE(doc->as_array().get_element(3)->is_null());
    EXPECT_DOUBLE_EQ(doc->as_array().get_element(4)->as_number().get_value(), 3.5);
}

TEST_F(JsonParserTest, ParseEmptyObject) {
    const json_document doc = parse_text("{}");
    EXPECT_TRUE(doc->is_object());
    EXPECT_EQ(doc->as_object().size(), 0u);
}

TEST_F(JsonParserTest, ParseObjectWithSingleMember) {
    const json_document doc = parse_text("{\"key\": 1}");
    ASSERT_EQ(doc->as_object().size(), 1u);
    EXPECT_EQ(doc->as_object().get_member("key")->as_number().as_integer(), 1);
}

TEST_F(JsonParserTest, ParseObjectWithMultipleMembers) {
    const json_document doc = parse_text("{\"a\": 1, \"b\": 2, \"c\": 3}");
    ASSERT_EQ(doc->as_object().size(), 3u);
    EXPECT_EQ(doc->as_object().get_member("a")->as_number().as_integer(), 1);
    EXPECT_EQ(doc->as_object().get_member("b")->as_number().as_integer(), 2);
    EXPECT_EQ(doc->as_object().get_member("c")->as_number().as_integer(), 3);
}

TEST_F(JsonParserTest, ParseNestedObject) {
    const json_document doc = parse_text("{\"outer\": {\"inner\": true}}");
    EXPECT_TRUE(doc->as_object().get_member("outer")->as_object().get_member("inner")->as_bool().get_value());
}

TEST_F(JsonParserTest, ParseNestedArray) {
    const json_document doc = parse_text("[[1, 2], [3, [4]]]");
    ASSERT_EQ(doc->as_array().size(), 2u);
    EXPECT_EQ(doc->as_array().get_element(0)->as_array().size(), 2u);
    EXPECT_EQ(doc->as_array().get_element(1)->as_array().get_element(1)->as_array().size(), 1u);
}

TEST_F(JsonParserTest, ParseLargeDocument) {
    string text = "[";
    for (int i = 0; i < 2000; ++i) {
        if (i != 0) {
            text += ',';
        }
        text += int_text(i);
    }
    text += ']';

    const json_document doc = parse_text(text);
    ASSERT_EQ(doc->as_array().size(), 2000u);
    EXPECT_EQ(doc->as_array().get_element(1999)->as_number().as_integer(), 1999);
    EXPECT_EQ(doc->to_string(), text);
}

TEST_F(JsonParserTest, ParseComplexNestedJson) {
    const char* text = "{\"id\": 42, \"tags\": [\"a\", \"b\"], \"meta\": {\"ok\": true, \"ratio\": 0.5},"
                       " \"nil\": null, \"neg\": -7}";
    const json_document doc = parse_text(text);
    EXPECT_EQ(doc->as_object().get_member("id")->as_number().as_integer(), 42);
    EXPECT_EQ(doc->as_object().get_member("tags")->as_array().size(), 2u);
    EXPECT_TRUE(doc->as_object().get_member("meta")->as_object().get_member("ok")->as_bool().get_value());
    EXPECT_DOUBLE_EQ(doc->as_object().get_member("meta")->as_object().get_member("ratio")->as_number().get_value(),
                     0.5);
    EXPECT_TRUE(doc->as_object().get_member("nil")->is_null());
    EXPECT_EQ(doc->as_object().get_member("neg")->as_number().as_integer(), -7);
}

TEST_F(JsonParserTest, ParseRoundTripIsStable) {
    const string_view text("{\"a\":[1,2,{\"b\":\"c\"}],\"d\":true}");
    const json_document first = parse_text(text);
    const string serialized = first->to_string();
    const json_document second = parse_text(string_view(serialized.data(), serialized.size()));
    EXPECT_EQ(second->to_string(), serialized);
    EXPECT_EQ(serialized, text);
}

TEST_F(JsonParserTest, ParseZeroCopyBorrowsInput) {
    const char* text = "[\"borrowed\"]";
    const json_document doc = parse_text(string_view(text));
    EXPECT_EQ(doc->as_array().get_element(0)->as_string().data(), text + 2);
}

TEST_F(JsonParserTest, ParseOwnedIsSelfContained) {
    json_document doc;
    {
        string source("{\"short\":\"tiny\",\"long\":\"a value well beyond fourteen bytes\"}");
        doc = parse_owned(move(source));
    }
    EXPECT_EQ(doc->as_object().get_member("short")->as_string().get_value(), "tiny");
    EXPECT_EQ(doc->as_object().get_member("long")->as_string().get_value(), "a value well beyond fourteen bytes");
    EXPECT_EQ(doc->to_string(), "{\"short\":\"tiny\",\"long\":\"a value well beyond fourteen bytes\"}");
}

TEST_F(JsonParserTest, TryParseValid) {
    json_parser parser(string_view("{\"a\": 1}"));
    const optional<json_document> result = parser.try_parse();
    EXPECT_TRUE(result.has_value());
}

TEST_F(JsonParserTest, TryParseInvalid) {
    json_parser parser(string_view("{invalid}"));
    EXPECT_FALSE(parser.try_parse().has_value());
}

TEST_F(JsonParserTest, ParseInvalidThrows) {
    EXPECT_THROW((void) parse_text("invalid"), json_exception);
    EXPECT_THROW((void) parse_text(""), json_exception);
    EXPECT_THROW((void) parse_text("   "), json_exception);
    EXPECT_THROW((void) parse_text("-"), json_exception);
}

TEST_F(JsonParserTest, ParseIncompleteContainerThrows) {
    EXPECT_THROW((void) parse_text("{\"a\": 1"), json_exception);
    EXPECT_THROW((void) parse_text("[1, 2"), json_exception);
    EXPECT_THROW((void) parse_text("{"), json_exception);
    EXPECT_THROW((void) parse_text("["), json_exception);
}

TEST_F(JsonParserTest, ParseMissingColonThrows) { EXPECT_THROW((void) parse_text("{\"key\" 1}"), json_exception); }

TEST_F(JsonParserTest, ParseMissingCommaThrows) {
    EXPECT_THROW((void) parse_text("[1 2]"), json_exception);
    EXPECT_THROW((void) parse_text("{\"a\": 1 \"b\": 2}"), json_exception);
}

TEST_F(JsonParserTest, ParseExtraCharactersThrows) {
    EXPECT_THROW((void) parse_text("null null"), json_exception);
    EXPECT_THROW((void) parse_text("{} []"), json_exception);
}

TEST_F(JsonParserTest, ParseUnterminatedStringThrows) {
    EXPECT_THROW((void) parse_text("\"unterminated"), json_exception);
}

TEST_F(JsonParserTest, ParseInvalidEscapeThrows) { EXPECT_THROW((void) parse_text("\"\\x\""), json_exception); }

TEST_F(JsonParserTest, ParseInvalidUnicodeEscapeThrows) {
    EXPECT_THROW((void) parse_text("\"\\u00zz\""), json_exception);
    EXPECT_THROW((void) parse_text("\"\\u12\""), json_exception);
}

TEST_F(JsonParserTest, ParseControlCharacterThrows) {
    EXPECT_THROW((void) parse_text("\"a\nb\""), json_exception);
    EXPECT_THROW((void) parse_text("\"a\x01b\""), json_exception);
}

TEST_F(JsonParserTest, ParseUnpairedSurrogateThrows) {
    EXPECT_THROW((void) parse_text("\"\\uD83D\""), json_exception);
    EXPECT_THROW((void) parse_text("\"\\uD83Dx\""), json_exception);
    EXPECT_THROW((void) parse_text("\"\\uDE00\""), json_exception);
    EXPECT_THROW((void) parse_text("\"\\uD83D\\u0041\""), json_exception);
}

TEST_F(JsonParserTest, ParseInvalidNumberThrows) {
    EXPECT_THROW((void) parse_text("01"), json_exception);
    EXPECT_THROW((void) parse_text("-01"), json_exception);
    EXPECT_THROW((void) parse_text("1."), json_exception);
    EXPECT_THROW((void) parse_text("1e"), json_exception);
    EXPECT_THROW((void) parse_text("1e+"), json_exception);
    EXPECT_THROW((void) parse_text(".5"), json_exception);
    EXPECT_THROW((void) parse_text("+1"), json_exception);
}

TEST_F(JsonParserTest, ParseInvalidKeywordThrows) {
    EXPECT_THROW((void) parse_text("tru"), json_exception);
    EXPECT_THROW((void) parse_text("nul"), json_exception);
    EXPECT_THROW((void) parse_text("True"), json_exception);
}

TEST_F(JsonParserTest, ParseWhitespaceTolerant) {
    const json_document doc = parse_text("\n\t {  \"a\" :\r\n [ 1 , 2 ] } \n");
    ASSERT_EQ(doc->as_object().size(), 1u);
    EXPECT_EQ(doc->as_object().get_member("a")->as_array().size(), 2u);
}

class JsonBuilderTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(JsonBuilderTest, BuildNull) {
    json_builder builder;
    const json_document result = builder.value(nullptr).build();
    EXPECT_TRUE(result->is_null());
    EXPECT_EQ(result->to_string(), "null");
}

TEST_F(JsonBuilderTest, BuildBool) {
    json_builder true_builder;
    EXPECT_TRUE(true_builder.value(true).build()->as_bool().get_value());

    json_builder false_builder;
    EXPECT_FALSE(false_builder.value(false).build()->as_bool().get_value());
}

TEST_F(JsonBuilderTest, BuildInt) {
    json_builder builder;
    const json_document result = builder.value(42).build();
    EXPECT_EQ(result->kind(), json_kind::integer);
    EXPECT_EQ(result->as_number().as_integer(), 42);
}

TEST_F(JsonBuilderTest, BuildDouble) {
    json_builder builder;
    const json_document result = builder.value(3.14).build();
    EXPECT_EQ(result->kind(), json_kind::real);
    EXPECT_DOUBLE_EQ(result->as_number().get_value(), 3.14);
}

TEST_F(JsonBuilderTest, BuildString) {
    json_builder builder;
    const json_document result = builder.value(string("hello")).build();
    EXPECT_TRUE(result->is_string());
    EXPECT_EQ(result->as_string().get_value(), "hello");
}

TEST_F(JsonBuilderTest, BuildCString) {
    json_builder builder;
    const json_document result = builder.value("hello").build();
    EXPECT_EQ(result->as_string().get_value(), "hello");
}

TEST_F(JsonBuilderTest, BuildStringView) {
    json_builder builder;
    const json_document result = builder.value(string_view("view")).build();
    EXPECT_EQ(result->as_string().get_value(), "view");
}

TEST_F(JsonBuilderTest, BuildStringSurvivesTemporarySource) {
    json_document result;
    {
        const string temporary("a temporary string value");
        json_builder builder;
        result = builder.value(temporary).build();
    }
    EXPECT_EQ(result->as_string().get_value(), "a temporary string value");
}

TEST_F(JsonBuilderTest, BuildEmptyObject) {
    json_builder builder;
    const json_document result = builder.begin_object().end_object().build();
    EXPECT_TRUE(result->is_object());
    EXPECT_EQ(result->to_string(), "{}");
}

TEST_F(JsonBuilderTest, BuildObjectWithMembers) {
    json_builder builder;
    const json_document result =
            builder.begin_object().key("id").value(1).key("name").value("nexus").end_object().build();
    ASSERT_EQ(result->as_object().size(), 2u);
    EXPECT_EQ(result->as_object().get_member("id")->as_number().as_integer(), 1);
    EXPECT_EQ(result->as_object().get_member("name")->as_string().get_value(), "nexus");
}

TEST_F(JsonBuilderTest, BuildEmptyArray) {
    json_builder builder;
    const json_document result = builder.begin_array().end_array().build();
    EXPECT_TRUE(result->is_array());
    EXPECT_EQ(result->to_string(), "[]");
}

TEST_F(JsonBuilderTest, BuildArrayWithElements) {
    json_builder builder;
    const json_document result = builder.begin_array().value(1).value(2).value(3).end_array().build();
    ASSERT_EQ(result->as_array().size(), 3u);
    EXPECT_EQ(result->as_array().get_element(2)->as_number().as_integer(), 3);
}

TEST_F(JsonBuilderTest, BuildNestedObject) {
    json_builder builder;
    const json_document result = builder.begin_object()
                                         .key("outer")
                                         .value_object([](json_builder& inner) { inner.key("inner").value(true); })
                                         .end_object()
                                         .build();
    EXPECT_TRUE(result->as_object().get_member("outer")->as_object().get_member("inner")->as_bool().get_value());
}

TEST_F(JsonBuilderTest, BuildNestedArray) {
    json_builder builder;
    const json_document result = builder.begin_array()
                                         .value_array([](json_builder& inner) { inner.value(1).value(2); })
                                         .value(3)
                                         .end_array()
                                         .build();
    ASSERT_EQ(result->as_array().size(), 2u);
    EXPECT_EQ(result->as_array().get_element(0)->as_array().size(), 2u);
}

TEST_F(JsonBuilderTest, BuildMixedNested) {
    json_builder builder;
    const json_document result = builder.begin_object()
                                         .key("list")
                                         .begin_array()
                                         .begin_object()
                                         .key("k")
                                         .value(1)
                                         .end_object()
                                         .end_array()
                                         .end_object()
                                         .build();
    EXPECT_EQ(result->to_string(), "{\"list\":[{\"k\":1}]}");
}

TEST_F(JsonBuilderTest, BuildUnclosedContainerThrows) {
    json_builder builder;
    builder.begin_object();
    EXPECT_THROW((void) builder.build(), json_exception);
}

TEST_F(JsonBuilderTest, KeyOutsideObjectThrows) {
    json_builder builder;
    builder.begin_array();
    EXPECT_THROW((void) builder.key("k"), json_exception);
}

TEST_F(JsonBuilderTest, EndMismatchThrows) {
    json_builder builder;
    builder.begin_object();
    EXPECT_THROW((void) builder.end_array(), json_exception);
}

TEST_F(JsonBuilderTest, BuildFromContainers) {
    flat_unordered_map<string, string> table;
    table.emplace("host", "localhost");
    table.emplace("port", "8080");
    vector<int32_t> numbers;
    numbers.push_back(1);
    numbers.push_back(2);
    numbers.push_back(3);

    json_builder builder;
    builder.begin_object();
    builder.key("headers").value(table);
    builder.key("list").value(numbers);
    builder.end_object();

    json_document doc = builder.build();
    const json_object root = doc->as_object();
    ASSERT_NE(root.get_member("headers"), nullptr);
    const json_object headers = root.get_member("headers")->as_object();
    EXPECT_EQ(headers.size(), 2u);
    ASSERT_NE(headers.get_member("host"), nullptr);
    EXPECT_EQ(headers.get_member("host")->as_string().get_value(), "localhost");
    ASSERT_NE(headers.get_member("port"), nullptr);
    EXPECT_EQ(headers.get_member("port")->as_string().get_value(), "8080");

    ASSERT_NE(root.get_member("list"), nullptr);
    const json_array list = root.get_member("list")->as_array();
    ASSERT_EQ(list.size(), 3u);
    for (size_t i = 0; i < list.size(); ++i) {
        EXPECT_EQ(list.get_element(i)->as_number().as_integer(), static_cast<int64_t>(i + 1));
    }
}

TEST_F(JsonBuilderTest, BuildLargeArray) {
    json_builder builder;
    builder.begin_array();
    for (int i = 0; i < 500; ++i) {
        builder.value(i);
    }
    const json_document result = builder.end_array().build();
    ASSERT_EQ(result->as_array().size(), 500u);
    EXPECT_EQ(result->as_array().get_element(499)->as_number().as_integer(), 499);
}
