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
    env_variable var("test_value");
    EXPECT_EQ(var.type(), env_value::Variable);
    EXPECT_TRUE(var.is_variable());
}

TEST_F(EnvValueTest, EnvVariableGetValue) {
    env_variable var("test_value");
    EXPECT_EQ(var.get_value(), "test_value");
}

TEST_F(EnvValueTest, EnvVariableSetValue) {
    env_variable var("old_value");
    var.set_value("new_value");
    EXPECT_EQ(var.get_value(), "new_value");
}

TEST_F(EnvValueTest, EnvVariableQuoteType) {
    env_variable var_none("value", env_variable::None);
    env_variable var_single("value", env_variable::Single);
    env_variable var_double("value", env_variable::Double);

    EXPECT_EQ(var_none.get_quote_type(), env_variable::None);
    EXPECT_EQ(var_single.get_quote_type(), env_variable::Single);
    EXPECT_EQ(var_double.get_quote_type(), env_variable::Double);
}

TEST_F(EnvValueTest, EnvVariableSetQuoteType) {
    env_variable var("value", env_variable::None);
    var.set_quote_type(env_variable::Double);
    EXPECT_EQ(var.get_quote_type(), env_variable::Double);

    var.set_quote_type(env_variable::Single);
    EXPECT_EQ(var.get_quote_type(), env_variable::Single);
}

TEST_F(EnvValueTest, EnvVariableExported) {
    env_variable var_exported("value", env_variable::None, true);
    env_variable var_not_exported("value", env_variable::None, false);

    EXPECT_TRUE(var_exported.is_exported());
    EXPECT_FALSE(var_not_exported.is_exported());
}

TEST_F(EnvValueTest, EnvVariableSetExported) {
    env_variable var("value");
    EXPECT_FALSE(var.is_exported());

    var.set_exported(true);
    EXPECT_TRUE(var.is_exported());

    var.set_exported(false);
    EXPECT_FALSE(var.is_exported());
}

TEST_F(EnvValueTest, EnvVariableAsVariable) {
    env_variable var("value");
    EXPECT_NE(var.as_variable(), nullptr);
}

TEST_F(EnvValueTest, EnvVariableGetInt) {
    env_variable var_int("42");
    env_variable var_negative("-10");
    env_variable var_invalid("not_a_number");

    EXPECT_EQ(var_int.get_int(), 42);
    EXPECT_EQ(var_negative.get_int(), -10);
    EXPECT_EQ(var_invalid.get_int(), 0);
    EXPECT_EQ(var_invalid.get_int(100), 100);
}

TEST_F(EnvValueTest, EnvVariableGetInt64) {
    env_variable var_int64("1234567890123");
    env_variable var_invalid("not_a_number");

    EXPECT_EQ(var_int64.get_int64(), 1234567890123);
    EXPECT_EQ(var_invalid.get_int64(), 0);
    EXPECT_EQ(var_invalid.get_int64(999), 999);
}

TEST_F(EnvValueTest, EnvVariableGetDouble) {
    env_variable var_float("3.14");
    env_variable var_int("42");
    env_variable var_invalid("not_a_number");

    EXPECT_DOUBLE_EQ(var_float.get_double(), 3.14);
    EXPECT_DOUBLE_EQ(var_int.get_double(), 42.0);
    EXPECT_DOUBLE_EQ(var_invalid.get_double(), 0.0);
    EXPECT_DOUBLE_EQ(var_invalid.get_double(1.5), 1.5);
}

TEST_F(EnvValueTest, EnvVariableGetBool) {
    env_variable var_true("true");
    env_variable var_false("false");
    env_variable var_invalid("not_a_bool");

    EXPECT_TRUE(var_true.get_bool());
    EXPECT_FALSE(var_false.get_bool());
    EXPECT_FALSE(var_invalid.get_bool());
    EXPECT_TRUE(var_invalid.get_bool(true));
}

TEST_F(EnvValueTest, EnvValueIsTypeVariable) {
    env_variable var("value");
    EXPECT_TRUE(var.is_variable());
}

TEST_F(EnvValueTest, ToStringVariable) {
    env_variable var("test_value");
    EXPECT_EQ(var.to_string(), "test_value");
}

TEST_F(EnvValueTest, ToStringVariableNoneQuote) {
    env_variable var("simple_value", env_variable::None);
    EXPECT_EQ(var.to_string(), "simple_value");
}

TEST_F(EnvValueTest, ToStringVariableSingleQuote) {
    env_variable var("single_quoted", env_variable::Single);
    EXPECT_EQ(var.to_string(), "'single_quoted'");
}

TEST_F(EnvValueTest, ToStringVariableDoubleQuote) {
    env_variable var("double_quoted", env_variable::Double);
    EXPECT_EQ(var.to_string(), "\"double_quoted\"");
}

TEST_F(EnvValueTest, ToStringVariableExported) {
    env_variable var("value", env_variable::None, true);
    EXPECT_EQ(var.to_string(), "export value");
}

TEST_F(EnvValueTest, ToDocumentVariable) {
    env_variable var("test_value");
    EXPECT_EQ(var.to_document(), "test_value");
}

TEST_F(EnvValueTest, EnvDocumentGetVariables) {
    env_document doc;
    doc.set_variable("KEY1", "value1");
    doc.set_variable("KEY2", "value2");

    const auto& vars = doc.get_variables();
    EXPECT_EQ(vars.size(), 2);
}

TEST_F(EnvValueTest, EnvDocumentGetVariable) {
    env_document doc;
    doc.set_variable("DATABASE_URL", "localhost");

    const env_variable* var = doc.get_variable("DATABASE_URL");
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->get_value(), "localhost");

    EXPECT_EQ(doc.get_variable("NONEXISTENT"), nullptr);
}

TEST_F(EnvValueTest, EnvDocumentNonConstGetVariable) {
    env_document doc;
    doc.set_variable("KEY", "value");

    env_variable* var = doc.get_variable("KEY");
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->get_value(), "value");
}

TEST_F(EnvValueTest, EnvDocumentHasVariable) {
    env_document doc;
    doc.set_variable("EXISTS", "value");

    EXPECT_TRUE(doc.has_variable("EXISTS"));
    EXPECT_FALSE(doc.has_variable("DOES_NOT_EXIST"));
}

TEST_F(EnvValueTest, EnvDocumentRemoveVariable) {
    env_document doc;
    doc.set_variable("TO_REMOVE", "value");
    EXPECT_TRUE(doc.has_variable("TO_REMOVE"));

    doc.remove_variable("TO_REMOVE");
    EXPECT_FALSE(doc.has_variable("TO_REMOVE"));
}

TEST_F(EnvValueTest, EnvDocumentAddVariable) {
    env_document doc;
    auto var = make_unique<env_variable>("value", env_variable::None, false);
    doc.add_variable("ADDED", move(var));

    EXPECT_TRUE(doc.has_variable("ADDED"));
    EXPECT_EQ(doc.get_string("ADDED"), "value");
}

TEST_F(EnvValueTest, EnvDocumentSetVariable) {
    env_document doc;
    doc.set_variable("KEY", "initial_value");

    doc.set_variable("KEY", "updated_value");
    EXPECT_EQ(doc.get_string("KEY"), "updated_value");
}

TEST_F(EnvValueTest, EnvDocumentAddComment) {
    env_document doc;
    doc.add_comment("This is a comment");
    doc.add_comment("Another comment");

    const auto& comments = doc.get_comments();
    EXPECT_EQ(comments.size(), 2);
    EXPECT_EQ(comments[0], "This is a comment");
    EXPECT_EQ(comments[1], "Another comment");
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
    doc.set_variable("BIG_NUMBER", "1234567890123");

    EXPECT_EQ(doc.get_int64("BIG_NUMBER"), 1234567890123);
    EXPECT_EQ(doc.get_int64("NONEXISTENT"), 0);
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

    env_document moved_doc(move(doc));
    EXPECT_TRUE(moved_doc.has_variable("KEY"));
    EXPECT_EQ(moved_doc.get_string("KEY"), "value");
    EXPECT_EQ(moved_doc.get_comments().size(), 1);
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

    string result = doc.to_string();
    EXPECT_NE(result.find("# Configuration file"), string::npos);
    EXPECT_NE(result.find("APP_NAME=MyApp"), string::npos);
    EXPECT_NE(result.find("PORT=8080"), string::npos);
}

TEST_F(EnvValueTest, EnvDocumentToStringEmpty) {
    env_document doc;
    string result = doc.to_string();
    EXPECT_EQ(result, "");
}


class EnvParserTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(EnvParserTest, ParseEmpty) {
    env_parser parser("");
    auto result = parser.parse();

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->get_variables().size(), 0);
}

TEST_F(EnvParserTest, ParseSimpleKeyValue) {
    env_parser parser("KEY=value");
    auto result = parser.parse();

    EXPECT_EQ(result->get_string("KEY"), "value");
}

TEST_F(EnvParserTest, ParseMultipleKeyValues) {
    env_parser parser("KEY1=value1\nKEY2=value2");
    auto result = parser.parse();

    EXPECT_EQ(result->get_string("KEY1"), "value1");
    EXPECT_EQ(result->get_string("KEY2"), "value2");
}

TEST_F(EnvParserTest, ParseDoubleQuotedValue) {
    env_parser parser("KEY=\"quoted value\"");
    auto result = parser.parse();

    const env_variable* var = result->get_variable("KEY");
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->get_value(), "quoted value");
    EXPECT_EQ(var->get_quote_type(), env_variable::Double);
}

TEST_F(EnvParserTest, ParseSingleQuotedValue) {
    env_parser parser("KEY='single quoted'");
    auto result = parser.parse();

    const env_variable* var = result->get_variable("KEY");
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->get_value(), "single quoted");
    EXPECT_EQ(var->get_quote_type(), env_variable::Single);
}

TEST_F(EnvParserTest, ParseUnquotedValue) {
    env_parser parser("KEY=unquoted_value");
    auto result = parser.parse();

    const env_variable* var = result->get_variable("KEY");
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->get_value(), "unquoted_value");
    EXPECT_EQ(var->get_quote_type(), env_variable::None);
}

TEST_F(EnvParserTest, ParseExportedVariable) {
    env_parser parser("export KEY=value");
    auto result = parser.parse();

    const env_variable* var = result->get_variable("KEY");
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->get_value(), "value");
    EXPECT_TRUE(var->is_exported());
}

TEST_F(EnvParserTest, ParseExportedVariableWithExportKeyword) {
    env_parser parser("export DATABASE_URL=postgres://localhost");
    auto result = parser.parse();

    const env_variable* var = result->get_variable("DATABASE_URL");
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->get_value(), "postgres://localhost");
    EXPECT_TRUE(var->is_exported());
}

TEST_F(EnvParserTest, ParseComment) {
    env_parser parser("# This is a comment\nKEY=value");
    auto result = parser.parse();

    EXPECT_EQ(result->get_string("KEY"), "value");
    EXPECT_EQ(result->get_comments().size(), 1);
    EXPECT_EQ(result->get_comments()[0], "This is a comment");
}

TEST_F(EnvParserTest, ParseBlankLines) {
    env_parser parser("\n\nKEY=value\n\n");
    auto result = parser.parse();

    EXPECT_EQ(result->get_string("KEY"), "value");
}

TEST_F(EnvParserTest, ParseEmptyValue) {
    env_parser parser("KEY=");
    auto result = parser.parse();

    const env_variable* var = result->get_variable("KEY");
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->get_value(), "");
}

TEST_F(EnvParserTest, ParseValueWithSpaces) {
    env_parser parser("  KEY  =  value  ");
    auto result = parser.parse();

    EXPECT_EQ(result->get_string("KEY"), "value");
}

TEST_F(EnvParserTest, ParseDoubleQuotedEscapeSequences) {
    env_parser parser("KEY=\"line1\\nline2\"");
    auto result = parser.parse();

    const env_variable* var = result->get_variable("KEY");
    ASSERT_NE(var, nullptr);
    string val = var->get_value();
    EXPECT_NE(val.find('\n'), string::npos);
}

TEST_F(EnvParserTest, ParseDoubleQuotedEscapedBackslash) {
    env_parser parser("KEY=\"path\\\\to\\\\file\"");
    auto result = parser.parse();

    const env_variable* var = result->get_variable("KEY");
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->get_value(), "path\\to\\file");
}

TEST_F(EnvParserTest, ParseDoubleQuotedEscapedQuote) {
    env_parser parser("KEY=\"he said \\\"hello\\\"\"");
    auto result = parser.parse();

    const env_variable* var = result->get_variable("KEY");
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->get_value(), "he said \"hello\"");
}

TEST_F(EnvParserTest, ParseSingleQuotedLiteral) {
    env_parser parser("KEY='value with \\n no escape'");
    auto result = parser.parse();

    const env_variable* var = result->get_variable("KEY");
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->get_value(), "value with \\n no escape");
}

TEST_F(EnvParserTest, ParseNumericValues) {
    env_parser parser("INT_KEY=42\nFLOAT_KEY=3.14");
    auto result = parser.parse();

    EXPECT_EQ(result->get_int("INT_KEY"), 42);
    EXPECT_DOUBLE_EQ(result->get_double("FLOAT_KEY"), 3.14);
}

TEST_F(EnvParserTest, ParseBooleanValues) {
    env_parser parser("ENABLED=true\nDISABLED=false");
    auto result = parser.parse();

    EXPECT_TRUE(result->get_bool("ENABLED"));
    EXPECT_FALSE(result->get_bool("DISABLED"));
}

TEST_F(EnvParserTest, ParseNoEqualsSign) {
    env_parser parser("INVALID_LINE");
    auto result = parser.parse();

    EXPECT_FALSE(result->has_variable("INVALID_LINE"));
}

TEST_F(EnvParserTest, ParseEmptyVariableName) {
    env_parser parser("=value");
    auto result = parser.parse();

    EXPECT_EQ(result->get_variables().size(), 0);
}

TEST_F(EnvParserTest, TryParseValid) {
    env_parser parser("KEY=value");
    auto result = parser.try_parse();
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value()->get_string("KEY"), "value");
}

TEST_F(EnvParserTest, ParseComplexDocument) {
    string env_content = R"(
        # Database configuration
        DATABASE_URL=postgres://localhost:5432/mydb
        DATABASE_POOL=10
        
        # Redis configuration
        export REDIS_URL="redis://localhost:6379"
        REDIS_TIMEOUT=30
        
        # Application settings
        APP_NAME='My Application'
        DEBUG=false
        MAX_RETRIES=3
    )";

    env_parser parser(env_content);
    auto result = parser.parse();

    ASSERT_NE(result, nullptr);

    EXPECT_EQ(result->get_string("DATABASE_URL"), "postgres://localhost:5432/mydb");
    EXPECT_EQ(result->get_int("DATABASE_POOL"), 10);

    const env_variable* redis = result->get_variable("REDIS_URL");
    ASSERT_NE(redis, nullptr);
    EXPECT_EQ(redis->get_value(), "redis://localhost:6379");
    EXPECT_TRUE(redis->is_exported());

    EXPECT_EQ(result->get_int("REDIS_TIMEOUT"), 30);
    EXPECT_EQ(result->get_string("APP_NAME"), "My Application");
    EXPECT_FALSE(result->get_bool("DEBUG"));
    EXPECT_EQ(result->get_int("MAX_RETRIES"), 3);
    EXPECT_GE(result->get_comments().size(), 3);
}


class EnvBuilderTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(EnvBuilderTest, BuildSimpleString) {
    env_builder builder;
    auto result = builder.key("APP_NAME").value("MyApp").build();

    EXPECT_EQ(result->get_string("APP_NAME"), "MyApp");
}

TEST_F(EnvBuilderTest, BuildCString) {
    env_builder builder;
    auto result = builder.key("APP_NAME").value("MyApp").build();

    EXPECT_EQ(result->get_string("APP_NAME"), "MyApp");
}

TEST_F(EnvBuilderTest, BuildStringView) {
    env_builder builder;
    string_view sv = "MyApp";
    auto result = builder.key("APP_NAME").value(sv).build();

    EXPECT_EQ(result->get_string("APP_NAME"), "MyApp");
}

TEST_F(EnvBuilderTest, BuildInt) {
    env_builder builder;
    auto result = builder.key("TIMEOUT").value(30).build();

    EXPECT_EQ(result->get_int("TIMEOUT"), 30);
}

TEST_F(EnvBuilderTest, BuildInt64) {
    env_builder builder;
    auto result = builder.key("BIG_NUMBER").value(static_cast<int64_t>(1234567890123)).build();

    EXPECT_EQ(result->get_int64("BIG_NUMBER"), 1234567890123);
}

TEST_F(EnvBuilderTest, BuildDouble) {
    env_builder builder;
    auto result = builder.key("PI").value(3.14159).build();

    EXPECT_DOUBLE_EQ(result->get_double("PI"), 3.14159);
}

TEST_F(EnvBuilderTest, BuildBoolTrue) {
    env_builder builder;
    auto result = builder.key("ENABLED").value(true).build();

    EXPECT_TRUE(result->get_bool("ENABLED"));
}

TEST_F(EnvBuilderTest, BuildBoolFalse) {
    env_builder builder;
    auto result = builder.key("DISABLED").value(false).build();

    EXPECT_FALSE(result->get_bool("DISABLED"));
}

TEST_F(EnvBuilderTest, BuildDoubleWithPrecision) {
    env_builder builder;
    auto result = builder.key("RATIO").value(3.14159265, 2).build();

    EXPECT_EQ(result->get_string("RATIO"), "3.14");
}

TEST_F(EnvBuilderTest, BuildUnquoted) {
    env_builder builder;
    auto result = builder.key("PLAIN").unquoted().value("value").build();

    const env_variable* var = result->get_variable("PLAIN");
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->get_quote_type(), env_variable::None);
}

TEST_F(EnvBuilderTest, BuildSingleQuoted) {
    env_builder builder;
    auto result = builder.key("SINGLE").single_quoted().value("value").build();

    const env_variable* var = result->get_variable("SINGLE");
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->get_quote_type(), env_variable::Single);
}

TEST_F(EnvBuilderTest, BuildDoubleQuoted) {
    env_builder builder;
    auto result = builder.key("DOUBLE").double_quoted().value("value").build();

    const env_variable* var = result->get_variable("DOUBLE");
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->get_quote_type(), env_variable::Double);
}

TEST_F(EnvBuilderTest, BuildExported) {
    env_builder builder;
    auto result = builder.key("EXPORTED").exported().value("value").build();

    const env_variable* var = result->get_variable("EXPORTED");
    ASSERT_NE(var, nullptr);
    EXPECT_TRUE(var->is_exported());
}

TEST_F(EnvBuilderTest, BuildExportedWithParam) {
    env_builder builder;
    auto result = builder.key("EXPORTED").exported(true).value("value").build();

    EXPECT_TRUE(result->get_variable("EXPORTED")->is_exported());
}

TEST_F(EnvBuilderTest, BuildNotExported) {
    env_builder builder;
    auto result = builder.key("NOT_EXPORTED").exported(false).value("value").build();

    EXPECT_FALSE(result->get_variable("NOT_EXPORTED")->is_exported());
}

TEST_F(EnvBuilderTest, BuildCombinedQuotedExported) {
    env_builder builder;
    auto result = builder.key("SECRET").double_quoted().exported().value("mysecret").build();

    const env_variable* var = result->get_variable("SECRET");
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->get_value(), "mysecret");
    EXPECT_EQ(var->get_quote_type(), env_variable::Double);
    EXPECT_TRUE(var->is_exported());
}

TEST_F(EnvBuilderTest, BuildMultipleKeys) {
    env_builder builder;
    auto result = builder.key("KEY1").value("value1").key("KEY2").value("value2").key("KEY3").value("value3").build();

    EXPECT_EQ(result->get_string("KEY1"), "value1");
    EXPECT_EQ(result->get_string("KEY2"), "value2");
    EXPECT_EQ(result->get_string("KEY3"), "value3");
}

TEST_F(EnvBuilderTest, BuildAddString) {
    env_builder builder;
    auto result = builder.add("KEY", "value").build();

    EXPECT_EQ(result->get_string("KEY"), "value");
}

TEST_F(EnvBuilderTest, BuildAddCString) {
    env_builder builder;
    auto result = builder.add("KEY", "value").build();

    EXPECT_EQ(result->get_string("KEY"), "value");
}

TEST_F(EnvBuilderTest, BuildAddStringView) {
    env_builder builder;
    string_view sv = "value";
    auto result = builder.add("KEY", sv).build();

    EXPECT_EQ(result->get_string("KEY"), "value");
}

TEST_F(EnvBuilderTest, BuildAddInt) {
    env_builder builder;
    auto result = builder.add("TIMEOUT", 30).build();

    EXPECT_EQ(result->get_int("TIMEOUT"), 30);
}

TEST_F(EnvBuilderTest, BuildAddInt64) {
    env_builder builder;
    auto result = builder.add("BIG", static_cast<int64_t>(999)).build();

    EXPECT_EQ(result->get_int64("BIG"), 999);
}

TEST_F(EnvBuilderTest, BuildAddDouble) {
    env_builder builder;
    auto result = builder.add("PI", 3.14).build();

    EXPECT_DOUBLE_EQ(result->get_double("PI"), 3.14);
}

TEST_F(EnvBuilderTest, BuildAddBool) {
    env_builder builder;
    auto result = builder.add("ENABLED", true).build();

    EXPECT_TRUE(result->get_bool("ENABLED"));
}

TEST_F(EnvBuilderTest, BuildAddExport) {
    env_builder builder;
    auto result = builder.add_export("PATH", "/usr/bin").build();

    const env_variable* var = result->get_variable("PATH");
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->get_value(), "/usr/bin");
    EXPECT_TRUE(var->is_exported());
}

TEST_F(EnvBuilderTest, BuildComment) {
    env_builder builder;
    auto result = builder.comment("Configuration for app").build();

    EXPECT_EQ(result->get_comments().size(), 1);
    EXPECT_EQ(result->get_comments()[0], "Configuration for app");
}

TEST_F(EnvBuilderTest, BuildMultipleComments) {
    env_builder builder;
    auto result = builder.comment("First comment").comment("Second comment").build();

    EXPECT_EQ(result->get_comments().size(), 2);
}

TEST_F(EnvBuilderTest, BuildBlankLine) {
    env_builder builder;
    auto result = builder.blank_line().build();

    EXPECT_EQ(result->get_comments().size(), 1);
    EXPECT_TRUE(result->get_comments()[0].empty());
}

TEST_F(EnvBuilderTest, BuildComplexDocument) {
    env_builder builder;
    auto result = builder.comment("Database configuration")
                          .add("DATABASE_URL", "postgres://localhost:5432/mydb")
                          .add("DATABASE_POOL", 10)
                          .blank_line()
                          .comment("Redis configuration")
                          .add_export("REDIS_URL", "redis://localhost:6379")
                          .add("REDIS_TIMEOUT", 30)
                          .blank_line()
                          .comment("Application settings")
                          .key("APP_NAME")
                          .single_quoted()
                          .value("My Application")
                          .key("DEBUG")
                          .value(false)
                          .key("MAX_RETRIES")
                          .value(3)
                          .build();

    ASSERT_NE(result, nullptr);

    EXPECT_EQ(result->get_string("DATABASE_URL"), "postgres://localhost:5432/mydb");
    EXPECT_EQ(result->get_int("DATABASE_POOL"), 10);

    const env_variable* redis = result->get_variable("REDIS_URL");
    ASSERT_NE(redis, nullptr);
    EXPECT_EQ(redis->get_value(), "redis://localhost:6379");
    EXPECT_TRUE(redis->is_exported());

    EXPECT_EQ(result->get_int("REDIS_TIMEOUT"), 30);
    EXPECT_EQ(result->get_string("APP_NAME"), "My Application");
    EXPECT_FALSE(result->get_bool("DEBUG"));
    EXPECT_EQ(result->get_int("MAX_RETRIES"), 3);
    EXPECT_GE(result->get_comments().size(), 3);
}

TEST_F(EnvBuilderTest, BuildMove) {
    env_builder builder1;
    builder1.key("KEY1").value("value1");

    env_builder builder2(move(builder1));
    auto result = builder2.key("KEY2").value("value2").build();

    EXPECT_EQ(result->get_string("KEY1"), "value1");
    EXPECT_EQ(result->get_string("KEY2"), "value2");
}

TEST_F(EnvBuilderTest, BuildNoKeyThrows) {
    env_builder builder;
    EXPECT_THROW(builder.value("missing_key"), env_exception);
}

TEST_F(EnvBuilderTest, BuildAddOverwrites) {
    env_builder builder;
    auto result = builder.add("KEY", "first").add("KEY", "second").build();

    EXPECT_EQ(result->get_string("KEY"), "second");
}

TEST_F(EnvBuilderTest, BuildQuoteResetsAfterValue) {
    env_builder builder;
    auto result = builder.key("DOUBLE").double_quoted().value("double_value").key("PLAIN").value("plain_value").build();

    const env_variable* double_var = result->get_variable("DOUBLE");
    ASSERT_NE(double_var, nullptr);
    EXPECT_EQ(double_var->get_quote_type(), env_variable::Double);

    const env_variable* plain_var = result->get_variable("PLAIN");
    ASSERT_NE(plain_var, nullptr);
    EXPECT_EQ(plain_var->get_quote_type(), env_variable::None);
}

TEST_F(EnvBuilderTest, BuildExportedResetsAfterValue) {
    env_builder builder;
    auto result =
            builder.key("EXPORTED").exported().value("exported_value").key("NOT_EXPORTED").value("plain_value").build();

    const env_variable* exported_var = result->get_variable("EXPORTED");
    ASSERT_NE(exported_var, nullptr);
    EXPECT_TRUE(exported_var->is_exported());

    const env_variable* plain_var = result->get_variable("NOT_EXPORTED");
    ASSERT_NE(plain_var, nullptr);
    EXPECT_FALSE(plain_var->is_exported());
}


class IniValueTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(IniValueTest, IniPropertyType) {
    ini_property prop("value");
    EXPECT_EQ(prop.type(), ini_value::Property);
    EXPECT_TRUE(prop.is_property());
    EXPECT_FALSE(prop.is_section());
}

TEST_F(IniValueTest, IniPropertyGetValue) {
    ini_property prop("test_value");
    EXPECT_EQ(prop.get_value(), "test_value");
}

TEST_F(IniValueTest, IniPropertySetValue) {
    ini_property prop("old_value");
    prop.set_value("new_value");
    EXPECT_EQ(prop.get_value(), "new_value");
}

TEST_F(IniValueTest, IniPropertyGetInt) {
    ini_property prop_int("42");
    ini_property prop_invalid("not_a_number");
    ini_property prop_negative("-10");
    ini_property prop_zero("0");

    EXPECT_EQ(prop_int.get_int(), 42);
    EXPECT_EQ(prop_invalid.get_int(), 0);
    EXPECT_EQ(prop_invalid.get_int(100), 100);
    EXPECT_EQ(prop_negative.get_int(), -10);
    EXPECT_EQ(prop_zero.get_int(), 0);
}

TEST_F(IniValueTest, IniPropertyGetDouble) {
    ini_property prop_float("3.14");
    ini_property prop_int("42");
    ini_property prop_invalid("not_a_number");
    ini_property prop_negative("-0.5");

    EXPECT_DOUBLE_EQ(prop_float.get_double(), 3.14);
    EXPECT_DOUBLE_EQ(prop_int.get_double(), 42.0);
    EXPECT_DOUBLE_EQ(prop_invalid.get_double(), 0.0);
    EXPECT_DOUBLE_EQ(prop_invalid.get_double(1.5), 1.5);
    EXPECT_DOUBLE_EQ(prop_negative.get_double(), -0.5);
}

TEST_F(IniValueTest, IniPropertyGetBool) {
    ini_property prop_true("true");
    ini_property prop_false("false");
    ini_property prop_yes("yes");
    ini_property prop_no("no");
    ini_property prop_invalid("not_a_bool");

    EXPECT_TRUE(prop_true.get_bool());
    EXPECT_FALSE(prop_false.get_bool());
    EXPECT_FALSE(prop_invalid.get_bool());
    EXPECT_TRUE(prop_invalid.get_bool(true));
}

TEST_F(IniValueTest, IniPropertyAsProperty) {
    ini_property prop("value");
    EXPECT_NE(prop.as_property(), nullptr);
    EXPECT_EQ(prop.as_section(), nullptr);
}

TEST_F(IniValueTest, IniSectionType) {
    ini_section section("test");
    EXPECT_EQ(section.type(), ini_value::Section);
    EXPECT_TRUE(section.is_section());
    EXPECT_FALSE(section.is_property());
}

TEST_F(IniValueTest, IniSectionAsSection) {
    ini_section section("test");
    EXPECT_NE(section.as_section(), nullptr);
    EXPECT_EQ(section.as_property(), nullptr);
}

TEST_F(IniValueTest, IniSectionGetName) {
    ini_section section("database");
    EXPECT_EQ(section.get_name(), "database");
}

TEST_F(IniValueTest, IniSectionSetName) {
    ini_section section("old_name");
    section.set_name("new_name");
    EXPECT_EQ(section.get_name(), "new_name");
}

TEST_F(IniValueTest, IniSectionAddProperty) {
    ini_section section("test");
    section.add_property("key1", make_unique<ini_property>("value1"));

    const ini_property* prop = section.get_property("key1");
    ASSERT_NE(prop, nullptr);
    EXPECT_EQ(prop->get_value(), "value1");
}

TEST_F(IniValueTest, IniSectionSetProperty) {
    ini_section section("test");
    section.set_property("key1", "value1");
    section.set_property("key1", "updated_value");

    const ini_property* prop = section.get_property("key1");
    ASSERT_NE(prop, nullptr);
    EXPECT_EQ(prop->get_value(), "updated_value");
}

TEST_F(IniValueTest, IniSectionGetProperty) {
    ini_section section("test");
    section.set_property("key1", "value1");

    const ini_property* const_prop = static_cast<const ini_section&>(section).get_property("key1");
    ASSERT_NE(const_prop, nullptr);
    EXPECT_EQ(const_prop->get_value(), "value1");

    ini_property* non_const_prop = section.get_property("key1");
    ASSERT_NE(non_const_prop, nullptr);
    EXPECT_EQ(non_const_prop->get_value(), "value1");

    EXPECT_EQ(section.get_property("nonexistent"), nullptr);
}

TEST_F(IniValueTest, IniSectionHasProperty) {
    ini_section section("test");
    section.set_property("exists", "value");

    EXPECT_TRUE(section.has_property("exists"));
    EXPECT_FALSE(section.has_property("does_not_exist"));
}

TEST_F(IniValueTest, IniSectionGetProperties) {
    ini_section section("test");
    section.set_property("a", "1");
    section.set_property("b", "2");

    const auto& properties = section.get_properties();
    EXPECT_EQ(properties.size(), 2);
}

TEST_F(IniValueTest, IniSectionGetString) {
    ini_section section("test");
    section.set_property("key", "value");

    EXPECT_EQ(section.get_string("key"), "value");
    EXPECT_EQ(section.get_string("nonexistent"), "");
    EXPECT_EQ(section.get_string("nonexistent", "default"), "default");
}

TEST_F(IniValueTest, IniSectionGetInt) {
    ini_section section("test");
    section.set_property("count", "42");

    EXPECT_EQ(section.get_int("count"), 42);
    EXPECT_EQ(section.get_int("nonexistent"), 0);
    EXPECT_EQ(section.get_int("nonexistent", 100), 100);
}

TEST_F(IniValueTest, IniSectionGetDouble) {
    ini_section section("test");
    section.set_property("pi", "3.14");

    EXPECT_DOUBLE_EQ(section.get_double("pi"), 3.14);
    EXPECT_DOUBLE_EQ(section.get_double("nonexistent"), 0.0);
    EXPECT_DOUBLE_EQ(section.get_double("nonexistent", 1.5), 1.5);
}

TEST_F(IniValueTest, IniSectionGetBool) {
    ini_section section("test");
    section.set_property("enabled", "true");

    EXPECT_TRUE(section.get_bool("enabled"));
    EXPECT_FALSE(section.get_bool("nonexistent"));
    EXPECT_TRUE(section.get_bool("nonexistent", true));
}

TEST_F(IniValueTest, IniSectionMoveConstructor) {
    ini_section section("test");
    section.set_property("key", "value");

    ini_section moved_section(move(section));
    EXPECT_EQ(moved_section.get_name(), "test");
    EXPECT_EQ(moved_section.get_string("key"), "value");
}

TEST_F(IniValueTest, IniSectionMoveAssignment) {
    ini_section section1("section1");
    section1.set_property("key1", "value1");

    ini_section section2("section2");
    section2.set_property("key2", "value2");

    section2 = move(section1);
    EXPECT_EQ(section2.get_name(), "section1");
    EXPECT_EQ(section2.get_string("key1"), "value1");
    EXPECT_FALSE(section2.has_property("key2"));
}

TEST_F(IniValueTest, IniDocumentGetSetions) {
    ini_document doc;
    doc.add_section("database", make_unique<ini_section>("database"));
    doc.add_section("server", make_unique<ini_section>("server"));

    const auto& sections = doc.get_sections();
    EXPECT_EQ(sections.size(), 2);
}

TEST_F(IniValueTest, IniDocumentGetSection) {
    ini_document doc;
    auto section = make_unique<ini_section>("database");
    section->set_property("host", "localhost");
    doc.add_section("database", move(section));

    const ini_section* db = doc.get_section("database");
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(db->get_name(), "database");
    EXPECT_EQ(db->get_string("host"), "localhost");

    EXPECT_EQ(doc.get_section("nonexistent"), nullptr);
}

TEST_F(IniValueTest, IniDocumentNonConstGetSection) {
    ini_document doc;
    auto section = make_unique<ini_section>("database");
    section->set_property("host", "localhost");
    doc.add_section("database", move(section));

    ini_section* db = doc.get_section("database");
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(db->get_string("host"), "localhost");
}

TEST_F(IniValueTest, IniDocumentHasSection) {
    ini_document doc;
    doc.add_section("database", make_unique<ini_section>("database"));

    EXPECT_TRUE(doc.has_section("database"));
    EXPECT_TRUE(doc.has_section(""));
    EXPECT_FALSE(doc.has_section("nonexistent"));
}

TEST_F(IniValueTest, IniDocumentGetGlobalSection) {
    ini_document doc;
    ini_section* global = doc.get_global_section();
    ASSERT_NE(global, nullptr);

    global->set_property("global_key", "global_value");
    EXPECT_EQ(doc.get_global_section()->get_string("global_key"), "global_value");
}

TEST_F(IniValueTest, IniDocumentGetString) {
    ini_document doc;
    auto section = make_unique<ini_section>("database");
    section->set_property("host", "localhost");
    doc.add_section("database", move(section));

    EXPECT_EQ(doc.get_string("database", "host"), "localhost");
    EXPECT_EQ(doc.get_string("database", "nonexistent", "default"), "default");
    EXPECT_EQ(doc.get_string("nonexistent", "key"), "");
    EXPECT_EQ(doc.get_string("nonexistent", "key", "default"), "default");
}

TEST_F(IniValueTest, IniDocumentGetInt) {
    ini_document doc;
    auto section = make_unique<ini_section>("settings");
    section->set_property("timeout", "30");
    doc.add_section("settings", move(section));

    EXPECT_EQ(doc.get_int("settings", "timeout"), 30);
    EXPECT_EQ(doc.get_int("settings", "nonexistent", 60), 60);
}

TEST_F(IniValueTest, IniDocumentGetDouble) {
    ini_document doc;
    auto section = make_unique<ini_section>("settings");
    section->set_property("ratio", "0.75");
    doc.add_section("settings", move(section));

    EXPECT_DOUBLE_EQ(doc.get_double("settings", "ratio"), 0.75);
    EXPECT_DOUBLE_EQ(doc.get_double("settings", "nonexistent", 1.0), 1.0);
}

TEST_F(IniValueTest, IniDocumentGetBool) {
    ini_document doc;
    auto section = make_unique<ini_section>("features");
    section->set_property("enabled", "true");
    doc.add_section("features", move(section));

    EXPECT_TRUE(doc.get_bool("features", "enabled"));
    EXPECT_FALSE(doc.get_bool("features", "nonexistent"));
    EXPECT_TRUE(doc.get_bool("features", "nonexistent", true));
}

TEST_F(IniValueTest, IniDocumentMoveConstructor) {
    ini_document doc;
    doc.add_section("test", make_unique<ini_section>("test"));
    doc.get_global_section()->set_property("global", "value");

    ini_document moved_doc(move(doc));
    EXPECT_TRUE(moved_doc.has_section("test"));
    EXPECT_EQ(moved_doc.get_global_section()->get_string("global"), "value");
}

TEST_F(IniValueTest, IniDocumentMoveAssignment) {
    ini_document doc1;
    doc1.add_section("section1", make_unique<ini_section>("section1"));

    ini_document doc2;
    doc2.add_section("section2", make_unique<ini_section>("section2"));

    doc2 = move(doc1);
    EXPECT_TRUE(doc2.has_section("section1"));
    EXPECT_FALSE(doc2.has_section("section2"));
}

TEST_F(IniValueTest, IniValueIsTypeSection) {
    ini_section section("test");
    ini_property property("value");

    EXPECT_TRUE(section.is_section());
    EXPECT_FALSE(property.is_section());
}

TEST_F(IniValueTest, IniValueIsTypeProperty) {
    ini_section section("test");
    ini_property property("value");

    EXPECT_FALSE(section.is_property());
    EXPECT_TRUE(property.is_property());
}

TEST_F(IniValueTest, ToStringProperty) {
    ini_property prop("test_value");
    EXPECT_EQ(prop.to_string(), "test_value");
}

TEST_F(IniValueTest, ToStringSection) {
    ini_section section("database");
    section.set_property("host", "localhost");
    section.set_property("port", "5432");

    string result = section.to_string();
    EXPECT_NE(result.find("[database]"), string::npos);
    EXPECT_NE(result.find("host = localhost"), string::npos);
    EXPECT_NE(result.find("port = 5432"), string::npos);
}

TEST_F(IniValueTest, ToDocumentProperty) {
    ini_property prop("test_value");
    EXPECT_EQ(prop.to_document(), "test_value");
}

TEST_F(IniValueTest, ToDocumentSection) {
    ini_section section("database");
    section.set_property("host", "localhost");

    string result = section.to_document();
    EXPECT_NE(result.find("[database]"), string::npos);
    EXPECT_NE(result.find("host = localhost"), string::npos);
}

TEST_F(IniValueTest, IniDocumentToString) {
    ini_document doc;
    doc.get_global_section()->set_property("version", "1.0");

    auto db_section = make_unique<ini_section>("database");
    db_section->set_property("host", "localhost");
    db_section->set_property("port", "5432");
    doc.add_section("database", move(db_section));

    string result = doc.to_string();
    EXPECT_NE(result.find("version = 1.0"), string::npos);
    EXPECT_NE(result.find("[database]"), string::npos);
    EXPECT_NE(result.find("host = localhost"), string::npos);
    EXPECT_NE(result.find("port = 5432"), string::npos);
}

TEST_F(IniValueTest, IniDocumentToStringEmpty) {
    ini_document doc;
    string result = doc.to_string();
    EXPECT_EQ(result, "");
}


class IniParserTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(IniParserTest, ParseEmpty) {
    ini_parser parser("");
    auto result = parser.parse();

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->get_global_section() != nullptr);
}

TEST_F(IniParserTest, ParseGlobalKeyValue) {
    ini_parser parser("key = value");
    auto result = parser.parse();

    EXPECT_EQ(result->get_global_section()->get_string("key"), "value");
}

TEST_F(IniParserTest, ParseMultipleGlobalKeyValues) {
    ini_parser parser("key1 = value1\nkey2 = value2");
    auto result = parser.parse();

    EXPECT_EQ(result->get_global_section()->get_string("key1"), "value1");
    EXPECT_EQ(result->get_global_section()->get_string("key2"), "value2");
}

TEST_F(IniParserTest, ParseSection) {
    ini_parser parser("[database]\nhost = localhost");
    auto result = parser.parse();

    const ini_section* db = result->get_section("database");
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(db->get_string("host"), "localhost");
}

TEST_F(IniParserTest, ParseMultipleSections) {
    ini_parser parser("[section1]\nkey1 = value1\n[section2]\nkey2 = value2");
    auto result = parser.parse();

    const ini_section* sec1 = result->get_section("section1");
    ASSERT_NE(sec1, nullptr);
    EXPECT_EQ(sec1->get_string("key1"), "value1");

    const ini_section* sec2 = result->get_section("section2");
    ASSERT_NE(sec2, nullptr);
    EXPECT_EQ(sec2->get_string("key2"), "value2");
}

TEST_F(IniParserTest, ParseSectionWithGlobal) {
    ini_parser parser("global_key = global_value\n[database]\nhost = localhost");
    auto result = parser.parse();

    EXPECT_EQ(result->get_global_section()->get_string("global_key"), "global_value");

    const ini_section* db = result->get_section("database");
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(db->get_string("host"), "localhost");
}

TEST_F(IniParserTest, ParseValueWithSpaces) {
    ini_parser parser("key =   value with spaces   ");
    auto result = parser.parse();

    EXPECT_EQ(result->get_global_section()->get_string("key"), "value with spaces");
}

TEST_F(IniParserTest, ParseKeyWithSpaces) {
    ini_parser parser("  key  = value");
    auto result = parser.parse();

    EXPECT_EQ(result->get_global_section()->get_string("key"), "value");
}

TEST_F(IniParserTest, ParseQuotedValue) {
    ini_parser parser("key = \"quoted value\"");
    auto result = parser.parse();

    EXPECT_EQ(result->get_global_section()->get_string("key"), "quoted value");
}

TEST_F(IniParserTest, ParseSingleQuotedValue) {
    ini_parser parser("key = 'single quoted value'");
    auto result = parser.parse();

    EXPECT_EQ(result->get_global_section()->get_string("key"), "single quoted value");
}

TEST_F(IniParserTest, ParseCommentSemicolon) {
    ini_parser parser("; This is a comment\nkey = value");
    auto result = parser.parse();

    EXPECT_EQ(result->get_global_section()->get_string("key"), "value");
}

TEST_F(IniParserTest, ParseCommentHash) {
    ini_parser parser("# This is a comment\nkey = value");
    auto result = parser.parse();

    EXPECT_EQ(result->get_global_section()->get_string("key"), "value");
}

TEST_F(IniParserTest, ParseInlineComment) {
    ini_parser parser("key = value ; inline comment");
    auto result = parser.parse();

    EXPECT_EQ(result->get_global_section()->get_string("key"), "value ; inline comment");
}

TEST_F(IniParserTest, ParseEmptyLines) {
    ini_parser parser("\n\nkey = value\n\n");
    auto result = parser.parse();

    EXPECT_EQ(result->get_global_section()->get_string("key"), "value");
}

TEST_F(IniParserTest, ParseBlankLinesBetweenSections) {
    ini_parser parser("[section1]\nkey1 = value1\n\n[section2]\nkey2 = value2");
    auto result = parser.parse();

    const ini_section* sec1 = result->get_section("section1");
    ASSERT_NE(sec1, nullptr);
    EXPECT_EQ(sec1->get_string("key1"), "value1");

    const ini_section* sec2 = result->get_section("section2");
    ASSERT_NE(sec2, nullptr);
    EXPECT_EQ(sec2->get_string("key2"), "value2");
}

TEST_F(IniParserTest, ParseNumericValues) {
    ini_parser parser("int_key = 42\nfloat_key = 3.14");
    auto result = parser.parse();

    EXPECT_EQ(result->get_global_section()->get_int("int_key"), 42);
    EXPECT_DOUBLE_EQ(result->get_global_section()->get_double("float_key"), 3.14);
}

TEST_F(IniParserTest, ParseBooleanValues) {
    ini_parser parser("enabled = true\ndisabled = false");
    auto result = parser.parse();

    EXPECT_TRUE(result->get_global_section()->get_bool("enabled"));
    EXPECT_FALSE(result->get_global_section()->get_bool("disabled"));
}

TEST_F(IniParserTest, ParseNoEqualsSign) {
    ini_parser parser("invalid_line_without_equals");
    auto result = parser.parse();

    EXPECT_EQ(result->get_global_section()->get_properties().size(), 0);
}

TEST_F(IniParserTest, TryParseValid) {
    ini_parser parser("key = value");
    auto result = parser.try_parse();
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value()->get_global_section()->get_string("key"), "value");
}

TEST_F(IniParserTest, ParseComplexDocument) {
    string ini_content = R"(
        ; Application configuration
        version = 2.0
        name = "MyApp"

        [database]
        host = localhost
        port = 5432
        enabled = true

        [server]
        host = 0.0.0.0
        port = 8080

        [features]
        logging = true
        caching = false
        timeout = 30
    )";

    ini_parser parser(ini_content);
    auto result = parser.parse();

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->get_string("", "version"), "2.0");
    EXPECT_EQ(result->get_string("", "name"), "MyApp");

    const ini_section* db = result->get_section("database");
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(db->get_string("host"), "localhost");
    EXPECT_EQ(db->get_int("port"), 5432);
    EXPECT_TRUE(db->get_bool("enabled"));

    const ini_section* server = result->get_section("server");
    ASSERT_NE(server, nullptr);
    EXPECT_EQ(server->get_string("host"), "0.0.0.0");
    EXPECT_EQ(server->get_int("port"), 8080);

    const ini_section* features = result->get_section("features");
    ASSERT_NE(features, nullptr);
    EXPECT_TRUE(features->get_bool("logging"));
    EXPECT_FALSE(features->get_bool("caching"));
    EXPECT_EQ(features->get_int("timeout"), 30);
}


class IniBuilderTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(IniBuilderTest, BuildGlobalString) {
    ini_builder builder;
    auto result = builder.key("app_name").value("MyApp").build();

    EXPECT_EQ(result->get_string("", "app_name"), "MyApp");
}

TEST_F(IniBuilderTest, BuildGlobalCString) {
    ini_builder builder;
    auto result = builder.key("app_name").value("MyApp").build();

    EXPECT_EQ(result->get_string("", "app_name"), "MyApp");
}

TEST_F(IniBuilderTest, BuildGlobalStringView) {
    ini_builder builder;
    string_view sv = "MyApp";
    auto result = builder.key("app_name").value(sv).build();

    EXPECT_EQ(result->get_string("", "app_name"), "MyApp");
}

TEST_F(IniBuilderTest, BuildGlobalInt) {
    ini_builder builder;
    auto result = builder.key("timeout").value(30).build();

    EXPECT_EQ(result->get_int("", "timeout"), 30);
}

TEST_F(IniBuilderTest, BuildGlobalInt64) {
    ini_builder builder;
    auto result = builder.key("big_number").value(static_cast<int64_t>(1234567890123)).build();

    EXPECT_EQ(result->get_string("", "big_number"), "1234567890123");
}

TEST_F(IniBuilderTest, BuildGlobalDouble) {
    ini_builder builder;
    auto result = builder.key("pi").value(3.14159).build();

    EXPECT_DOUBLE_EQ(result->get_double("", "pi"), 3.14159);
}

TEST_F(IniBuilderTest, BuildGlobalBoolTrue) {
    ini_builder builder;
    auto result = builder.key("enabled").value(true).build();

    EXPECT_TRUE(result->get_bool("", "enabled"));
}

TEST_F(IniBuilderTest, BuildGlobalBoolFalse) {
    ini_builder builder;
    auto result = builder.key("disabled").value(false).build();

    EXPECT_FALSE(result->get_bool("", "disabled"));
}

TEST_F(IniBuilderTest, BuildDoubleWithPrecision) {
    ini_builder builder;
    auto result = builder.key("ratio").value(3.14159265, 2).build();

    EXPECT_EQ(result->get_string("", "ratio"), "3.14");
}

TEST_F(IniBuilderTest, BuildMultipleGlobalKeys) {
    ini_builder builder;
    auto result = builder.key("key1").value("value1").key("key2").value("value2").key("key3").value("value3").build();

    EXPECT_EQ(result->get_string("", "key1"), "value1");
    EXPECT_EQ(result->get_string("", "key2"), "value2");
    EXPECT_EQ(result->get_string("", "key3"), "value3");
}

TEST_F(IniBuilderTest, BuildSection) {
    ini_builder builder;
    auto result = builder.begin_section("database")
                          .key("host")
                          .value("localhost")
                          .key("port")
                          .value(5432)
                          .end_section()
                          .build();

    const ini_section* db = result->get_section("database");
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(db->get_string("host"), "localhost");
    EXPECT_EQ(db->get_int("port"), 5432);
}

TEST_F(IniBuilderTest, BuildMultipleSections) {
    ini_builder builder;
    auto result = builder.begin_section("database")
                          .key("host")
                          .value("localhost")
                          .end_section()
                          .begin_section("server")
                          .key("host")
                          .value("0.0.0.0")
                          .key("port")
                          .value(8080)
                          .end_section()
                          .build();

    const ini_section* db = result->get_section("database");
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(db->get_string("host"), "localhost");

    const ini_section* server = result->get_section("server");
    ASSERT_NE(server, nullptr);
    EXPECT_EQ(server->get_string("host"), "0.0.0.0");
    EXPECT_EQ(server->get_int("port"), 8080);
}

TEST_F(IniBuilderTest, BuildSectionAndGlobal) {
    ini_builder builder;
    auto result = builder.key("version")
                          .value("1.0")
                          .begin_section("database")
                          .key("host")
                          .value("localhost")
                          .end_section()
                          .key("name")
                          .value("MyApp")
                          .build();

    EXPECT_EQ(result->get_string("", "version"), "1.0");
    EXPECT_EQ(result->get_string("", "name"), "MyApp");
    EXPECT_EQ(result->get_string("database", "host"), "localhost");
}

TEST_F(IniBuilderTest, BuildValueSection) {
    ini_builder builder;
    auto result = builder.value_section("database",
                                        [](ini_builder& inner) {
                                            inner.key("host").value("localhost");
                                            inner.key("port").value(5432);
                                        })
                          .build();

    const ini_section* db = result->get_section("database");
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(db->get_string("host"), "localhost");
    EXPECT_EQ(db->get_int("port"), 5432);
}

TEST_F(IniBuilderTest, BuildNestedValueSections) {
    ini_builder builder;
    auto result = builder.value_section("server",
                                        [](ini_builder& inner) {
                                            inner.key("host").value("0.0.0.0");
                                            inner.value_section("ssl", [](ini_builder& ssl) {
                                                ssl.key("enabled").value(true);
                                                ssl.key("certificate").value("/path/to/cert");
                                            });
                                        })
                          .build();

    const ini_section* server = result->get_section("server");
    ASSERT_NE(server, nullptr);
    EXPECT_EQ(server->get_string("host"), "0.0.0.0");

    const ini_section* ssl = result->get_section("ssl");
    ASSERT_NE(ssl, nullptr);
    EXPECT_TRUE(ssl->get_bool("enabled"));
    EXPECT_EQ(ssl->get_string("certificate"), "/path/to/cert");
}

TEST_F(IniBuilderTest, BuildComplexDocument) {
    ini_builder builder;
    auto result = builder.key("version")
                          .value("2.0")
                          .key("name")
                          .value("MyApp")
                          .begin_section("database")
                          .key("host")
                          .value("localhost")
                          .key("port")
                          .value(5432)
                          .key("enabled")
                          .value(true)
                          .end_section()
                          .begin_section("server")
                          .key("host")
                          .value("0.0.0.0")
                          .key("port")
                          .value(8080)
                          .end_section()
                          .begin_section("features")
                          .key("logging")
                          .value(true)
                          .key("caching")
                          .value(false)
                          .key("timeout")
                          .value(30)
                          .end_section()
                          .build();

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->get_string("", "version"), "2.0");
    EXPECT_EQ(result->get_string("", "name"), "MyApp");

    const ini_section* db = result->get_section("database");
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(db->get_string("host"), "localhost");
    EXPECT_EQ(db->get_int("port"), 5432);
    EXPECT_TRUE(db->get_bool("enabled"));

    const ini_section* server = result->get_section("server");
    ASSERT_NE(server, nullptr);
    EXPECT_EQ(server->get_string("host"), "0.0.0.0");
    EXPECT_EQ(server->get_int("port"), 8080);

    const ini_section* features = result->get_section("features");
    ASSERT_NE(features, nullptr);
    EXPECT_TRUE(features->get_bool("logging"));
    EXPECT_FALSE(features->get_bool("caching"));
    EXPECT_EQ(features->get_int("timeout"), 30);
}

TEST_F(IniBuilderTest, BuildSectionOverwrite) {
    ini_builder builder;
    auto result = builder.begin_section("settings")
                          .key("key1")
                          .value("value1")
                          .end_section()
                          .begin_section("settings")
                          .key("key2")
                          .value("value2")
                          .end_section()
                          .build();

    const ini_section* settings = result->get_section("settings");
    ASSERT_NE(settings, nullptr);
    EXPECT_EQ(settings->get_string("key2"), "value2");
}

TEST_F(IniBuilderTest, BuildMove) {
    ini_builder builder1;
    builder1.key("key1").value("value1");

    ini_builder builder2(move(builder1));
    auto result = builder2.key("key2").value("value2").build();

    EXPECT_EQ(result->get_string("", "key1"), "value1");
    EXPECT_EQ(result->get_string("", "key2"), "value2");
}

TEST_F(IniBuilderTest, BuildEndSectionResetsToGlobal) {
    ini_builder builder;
    auto result = builder.begin_section("section1")
                          .key("section_key")
                          .value("section_value")
                          .end_section()
                          .key("global_key")
                          .value("global_value")
                          .build();

    EXPECT_EQ(result->get_string("section1", "section_key"), "section_value");
    EXPECT_EQ(result->get_string("", "global_key"), "global_value");
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
