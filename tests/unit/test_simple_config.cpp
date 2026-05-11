#include <NeForce/core/file/env/env_builder.hpp>
#include <NeForce/core/file/env/env_parser.hpp>
#include <NeForce/core/file/ini/ini_builder.hpp>
#include <NeForce/core/file/ini/ini_parser.hpp>
#include <NeForce/core/file/json/json_builder.hpp>
#include <NeForce/core/file/json/json_parser.hpp>
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


class JsonValueTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(JsonValueTest, JsonNullType) {
    json_null null_val;
    EXPECT_EQ(null_val.type(), json_value::Null);
    EXPECT_TRUE(null_val.is_null());
    EXPECT_FALSE(null_val.is_bool());
    EXPECT_FALSE(null_val.is_number());
    EXPECT_FALSE(null_val.is_string());
    EXPECT_FALSE(null_val.is_object());
    EXPECT_FALSE(null_val.is_array());
}

TEST_F(JsonValueTest, JsonNullAsNull) {
    json_null null_val;
    EXPECT_NE(null_val.as_null(), nullptr);
    EXPECT_EQ(null_val.as_bool(), nullptr);
    EXPECT_EQ(null_val.as_number(), nullptr);
    EXPECT_EQ(null_val.as_string(), nullptr);
    EXPECT_EQ(null_val.as_object(), nullptr);
    EXPECT_EQ(null_val.as_array(), nullptr);
}

TEST_F(JsonValueTest, JsonBoolType) {
    json_bool bool_true(true);
    json_bool bool_false(false);

    EXPECT_EQ(bool_true.type(), json_value::Bool);
    EXPECT_EQ(bool_false.type(), json_value::Bool);

    EXPECT_TRUE(bool_true.is_bool());
    EXPECT_FALSE(bool_true.is_null());
    EXPECT_FALSE(bool_true.is_number());
    EXPECT_FALSE(bool_true.is_string());
    EXPECT_FALSE(bool_true.is_object());
    EXPECT_FALSE(bool_true.is_array());
}

TEST_F(JsonValueTest, JsonBoolGetValue) {
    json_bool bool_true(true);
    json_bool bool_false(false);

    EXPECT_TRUE(bool_true.get_value());
    EXPECT_FALSE(bool_false.get_value());
}

TEST_F(JsonValueTest, JsonBoolAsBool) {
    json_bool bool_val(true);
    EXPECT_NE(bool_val.as_bool(), nullptr);
    EXPECT_EQ(bool_val.as_null(), nullptr);
    EXPECT_EQ(bool_val.as_number(), nullptr);
    EXPECT_EQ(bool_val.as_string(), nullptr);
    EXPECT_EQ(bool_val.as_object(), nullptr);
    EXPECT_EQ(bool_val.as_array(), nullptr);
}

TEST_F(JsonValueTest, JsonNumberType) {
    json_number num(3.14);
    EXPECT_EQ(num.type(), json_value::Number);
    EXPECT_TRUE(num.is_number());
    EXPECT_FALSE(num.is_null());
    EXPECT_FALSE(num.is_bool());
    EXPECT_FALSE(num.is_string());
    EXPECT_FALSE(num.is_object());
    EXPECT_FALSE(num.is_array());
}

TEST_F(JsonValueTest, JsonNumberGetValue) {
    json_number num_int(42.0);
    json_number num_float(3.14);
    json_number num_neg(-1.5);

    EXPECT_DOUBLE_EQ(num_int.get_value(), 42.0);
    EXPECT_DOUBLE_EQ(num_float.get_value(), 3.14);
    EXPECT_DOUBLE_EQ(num_neg.get_value(), -1.5);
}

TEST_F(JsonValueTest, JsonNumberAsNumber) {
    json_number num(0.0);
    EXPECT_NE(num.as_number(), nullptr);
    EXPECT_EQ(num.as_null(), nullptr);
    EXPECT_EQ(num.as_bool(), nullptr);
    EXPECT_EQ(num.as_string(), nullptr);
    EXPECT_EQ(num.as_object(), nullptr);
    EXPECT_EQ(num.as_array(), nullptr);
}

TEST_F(JsonValueTest, JsonStringType) {
    json_string str("hello");
    EXPECT_EQ(str.type(), json_value::String);
    EXPECT_TRUE(str.is_string());
    EXPECT_FALSE(str.is_null());
    EXPECT_FALSE(str.is_bool());
    EXPECT_FALSE(str.is_number());
    EXPECT_FALSE(str.is_object());
    EXPECT_FALSE(str.is_array());
}

TEST_F(JsonValueTest, JsonStringGetValue) {
    json_string str("test_string");
    EXPECT_EQ(str.get_value(), "test_string");
}

TEST_F(JsonValueTest, JsonStringAsString) {
    json_string str("value");
    EXPECT_NE(str.as_string(), nullptr);
    EXPECT_EQ(str.as_null(), nullptr);
    EXPECT_EQ(str.as_bool(), nullptr);
    EXPECT_EQ(str.as_number(), nullptr);
    EXPECT_EQ(str.as_object(), nullptr);
    EXPECT_EQ(str.as_array(), nullptr);
}

TEST_F(JsonValueTest, JsonObjectType) {
    json_object obj;
    EXPECT_EQ(obj.type(), json_value::Object);
    EXPECT_TRUE(obj.is_object());
    EXPECT_FALSE(obj.is_null());
    EXPECT_FALSE(obj.is_bool());
    EXPECT_FALSE(obj.is_number());
    EXPECT_FALSE(obj.is_string());
    EXPECT_FALSE(obj.is_array());
}

TEST_F(JsonValueTest, JsonObjectAsObject) {
    json_object obj;
    EXPECT_NE(obj.as_object(), nullptr);
    EXPECT_EQ(obj.as_null(), nullptr);
    EXPECT_EQ(obj.as_bool(), nullptr);
    EXPECT_EQ(obj.as_number(), nullptr);
    EXPECT_EQ(obj.as_string(), nullptr);
    EXPECT_EQ(obj.as_array(), nullptr);
}

TEST_F(JsonValueTest, JsonObjectAddAndGetMember) {
    json_object obj;
    obj.add_member("key1", make_unique<json_string>("value1"));
    obj.add_member("key2", make_unique<json_number>(42.0));

    const json_value* member1 = obj.get_member("key1");
    ASSERT_NE(member1, nullptr);
    EXPECT_TRUE(member1->is_string());
    EXPECT_EQ(member1->as_string()->get_value(), "value1");

    const json_value* member2 = obj.get_member("key2");
    ASSERT_NE(member2, nullptr);
    EXPECT_TRUE(member2->is_number());
    EXPECT_DOUBLE_EQ(member2->as_number()->get_value(), 42.0);

    const json_value* missing = obj.get_member("nonexistent");
    EXPECT_EQ(missing, nullptr);
}

TEST_F(JsonValueTest, JsonObjectGetMembers) {
    json_object obj;
    obj.add_member("a", make_unique<json_null>());
    obj.add_member("b", make_unique<json_bool>(true));

    const auto& members = obj.get_members();
    EXPECT_EQ(members.size(), 2);
}

TEST_F(JsonValueTest, JsonObjectMoveConstructor) {
    json_object obj;
    obj.add_member("key", make_unique<json_number>(1.0));

    json_object moved_obj(move(obj));
    const json_value* member = moved_obj.get_member("key");
    ASSERT_NE(member, nullptr);
    EXPECT_DOUBLE_EQ(member->as_number()->get_value(), 1.0);
}

TEST_F(JsonValueTest, JsonObjectMoveAssignment) {
    json_object obj1;
    obj1.add_member("key1", make_unique<json_number>(1.0));

    json_object obj2;
    obj2.add_member("key2", make_unique<json_number>(2.0));

    obj2 = move(obj1);
    const json_value* member = obj2.get_member("key1");
    ASSERT_NE(member, nullptr);
    EXPECT_DOUBLE_EQ(member->as_number()->get_value(), 1.0);
    EXPECT_EQ(obj2.get_member("key2"), nullptr);
}

TEST_F(JsonValueTest, JsonArrayType) {
    json_array arr;
    EXPECT_EQ(arr.type(), json_value::Array);
    EXPECT_TRUE(arr.is_array());
    EXPECT_FALSE(arr.is_null());
    EXPECT_FALSE(arr.is_bool());
    EXPECT_FALSE(arr.is_number());
    EXPECT_FALSE(arr.is_string());
    EXPECT_FALSE(arr.is_object());
}

TEST_F(JsonValueTest, JsonArrayAsArray) {
    json_array arr;
    EXPECT_NE(arr.as_array(), nullptr);
    EXPECT_EQ(arr.as_null(), nullptr);
    EXPECT_EQ(arr.as_bool(), nullptr);
    EXPECT_EQ(arr.as_number(), nullptr);
    EXPECT_EQ(arr.as_string(), nullptr);
    EXPECT_EQ(arr.as_object(), nullptr);
}

TEST_F(JsonValueTest, JsonArrayAddAndGetElement) {
    json_array arr;
    arr.add_element(make_unique<json_number>(1.0));
    arr.add_element(make_unique<json_number>(2.0));
    arr.add_element(make_unique<json_number>(3.0));

    EXPECT_EQ(arr.size(), 3);

    const json_value* elem0 = arr.get_element(0);
    ASSERT_NE(elem0, nullptr);
    EXPECT_DOUBLE_EQ(elem0->as_number()->get_value(), 1.0);

    const json_value* elem1 = arr.get_element(1);
    ASSERT_NE(elem1, nullptr);
    EXPECT_DOUBLE_EQ(elem1->as_number()->get_value(), 2.0);

    const json_value* elem2 = arr.get_element(2);
    ASSERT_NE(elem2, nullptr);
    EXPECT_DOUBLE_EQ(elem2->as_number()->get_value(), 3.0);

    const json_value* out_of_bounds = arr.get_element(100);
    EXPECT_EQ(out_of_bounds, nullptr);
}

TEST_F(JsonValueTest, JsonArrayGetElements) {
    json_array arr;
    arr.add_element(make_unique<json_bool>(true));
    arr.add_element(make_unique<json_bool>(false));

    const auto& elements = arr.get_elements();
    EXPECT_EQ(elements.size(), 2);
}

TEST_F(JsonValueTest, JsonArrayMoveConstructor) {
    json_array arr;
    arr.add_element(make_unique<json_number>(42.0));

    json_array moved_arr(move(arr));
    EXPECT_EQ(moved_arr.size(), 1);
    EXPECT_DOUBLE_EQ(moved_arr.get_element(0)->as_number()->get_value(), 42.0);
}

TEST_F(JsonValueTest, JsonArrayMoveAssignment) {
    json_array arr1;
    arr1.add_element(make_unique<json_number>(1.0));

    json_array arr2;
    arr2.add_element(make_unique<json_number>(2.0));

    arr2 = move(arr1);
    EXPECT_EQ(arr2.size(), 1);
    EXPECT_DOUBLE_EQ(arr2.get_element(0)->as_number()->get_value(), 1.0);
}

TEST_F(JsonValueTest, JsonValueIsTypeNull) {
    json_null null_val;
    json_bool bool_val(true);
    json_number num_val(0.0);
    json_string str_val("");
    json_object obj_val;
    json_array arr_val;

    EXPECT_TRUE(null_val.is_null());
    EXPECT_FALSE(bool_val.is_null());
    EXPECT_FALSE(num_val.is_null());
    EXPECT_FALSE(str_val.is_null());
    EXPECT_FALSE(obj_val.is_null());
    EXPECT_FALSE(arr_val.is_null());
}

TEST_F(JsonValueTest, JsonValueIsTypeBool) {
    json_null null_val;
    json_bool bool_val(true);
    json_number num_val(0.0);
    json_string str_val("");
    json_object obj_val;
    json_array arr_val;

    EXPECT_FALSE(null_val.is_bool());
    EXPECT_TRUE(bool_val.is_bool());
    EXPECT_FALSE(num_val.is_bool());
    EXPECT_FALSE(str_val.is_bool());
    EXPECT_FALSE(obj_val.is_bool());
    EXPECT_FALSE(arr_val.is_bool());
}

TEST_F(JsonValueTest, JsonValueIsTypeNumber) {
    json_null null_val;
    json_bool bool_val(true);
    json_number num_val(0.0);
    json_string str_val("");
    json_object obj_val;
    json_array arr_val;

    EXPECT_FALSE(null_val.is_number());
    EXPECT_FALSE(bool_val.is_number());
    EXPECT_TRUE(num_val.is_number());
    EXPECT_FALSE(str_val.is_number());
    EXPECT_FALSE(obj_val.is_number());
    EXPECT_FALSE(arr_val.is_number());
}

TEST_F(JsonValueTest, JsonValueIsTypeString) {
    json_null null_val;
    json_bool bool_val(true);
    json_number num_val(0.0);
    json_string str_val("");
    json_object obj_val;
    json_array arr_val;

    EXPECT_FALSE(null_val.is_string());
    EXPECT_FALSE(bool_val.is_string());
    EXPECT_FALSE(num_val.is_string());
    EXPECT_TRUE(str_val.is_string());
    EXPECT_FALSE(obj_val.is_string());
    EXPECT_FALSE(arr_val.is_string());
}

TEST_F(JsonValueTest, JsonValueIsTypeObject) {
    json_null null_val;
    json_bool bool_val(true);
    json_number num_val(0.0);
    json_string str_val("");
    json_object obj_val;
    json_array arr_val;

    EXPECT_FALSE(null_val.is_object());
    EXPECT_FALSE(bool_val.is_object());
    EXPECT_FALSE(num_val.is_object());
    EXPECT_FALSE(str_val.is_object());
    EXPECT_TRUE(obj_val.is_object());
    EXPECT_FALSE(arr_val.is_object());
}

TEST_F(JsonValueTest, JsonValueIsTypeArray) {
    json_null null_val;
    json_bool bool_val(true);
    json_number num_val(0.0);
    json_string str_val("");
    json_object obj_val;
    json_array arr_val;

    EXPECT_FALSE(null_val.is_array());
    EXPECT_FALSE(bool_val.is_array());
    EXPECT_FALSE(num_val.is_array());
    EXPECT_FALSE(str_val.is_array());
    EXPECT_FALSE(obj_val.is_array());
    EXPECT_TRUE(arr_val.is_array());
}

TEST_F(JsonValueTest, ToStringNull) {
    json_null null_val;
    EXPECT_EQ(null_val.to_string(), "null");
}

TEST_F(JsonValueTest, ToStringBool) {
    json_bool bool_true(true);
    json_bool bool_false(false);

    EXPECT_EQ(bool_true.to_string(), "true");
    EXPECT_EQ(bool_false.to_string(), "false");
}

TEST_F(JsonValueTest, ToStringNumber) {
    json_number num_int(42.0);
    json_number num_float(3.14);
    json_number num_neg(-1.0);

    EXPECT_EQ(num_int.to_string(), "42");
    EXPECT_EQ(num_float.to_string(), "3.14");
    EXPECT_EQ(num_neg.to_string(), "-1");
}

TEST_F(JsonValueTest, ToStringString) {
    json_string str("hello world");
    EXPECT_EQ(str.to_string(), "\"hello world\"");
}

TEST_F(JsonValueTest, ToStringEmptyArray) {
    json_array arr;
    EXPECT_EQ(arr.to_string(), "[]");
}

TEST_F(JsonValueTest, ToStringArrayWithElements) {
    json_array arr;
    arr.add_element(make_unique<json_number>(1.0));
    arr.add_element(make_unique<json_number>(2.0));
    arr.add_element(make_unique<json_number>(3.0));

    EXPECT_EQ(arr.to_string(), "[1,2,3]");
}

TEST_F(JsonValueTest, ToStringEmptyObject) {
    json_object obj;
    EXPECT_EQ(obj.to_string(), "{}");
}

TEST_F(JsonValueTest, ToStringObjectWithMembers) {
    json_object obj;
    obj.add_member("name", make_unique<json_string>("John"));
    obj.add_member("age", make_unique<json_number>(30.0));

    string result = obj.to_string();
    EXPECT_TRUE(result.find("\"name\":\"John\"") != string::npos);
    EXPECT_TRUE(result.find("\"age\":30") != string::npos);
}

TEST_F(JsonValueTest, ToIndentStringNull) {
    json_null null_val;
    EXPECT_EQ(null_val.to_indent_string(), "null");
}

TEST_F(JsonValueTest, ToIndentStringBool) {
    json_bool bool_val(true);
    EXPECT_EQ(bool_val.to_indent_string(), "true");
}

TEST_F(JsonValueTest, ToIndentStringNumber) {
    json_number num(42.0);
    EXPECT_EQ(num.to_indent_string(), "42");
}

TEST_F(JsonValueTest, ToIndentStringString) {
    json_string str("test");
    EXPECT_EQ(str.to_indent_string(), "\"test\"");
}

TEST_F(JsonValueTest, ToIndentStringEmptyArray) {
    json_array arr;
    EXPECT_EQ(arr.to_indent_string(), "[]");
}

TEST_F(JsonValueTest, ToIndentStringArrayWithElements) {
    json_array arr;
    arr.add_element(make_unique<json_number>(1.0));
    arr.add_element(make_unique<json_number>(2.0));

    string result = arr.to_indent_string();
    EXPECT_NE(result.find("[\n"), string::npos);
    EXPECT_NE(result.find("1"), string::npos);
    EXPECT_NE(result.find("2"), string::npos);
    EXPECT_NE(result.find("\n]"), string::npos);
}

TEST_F(JsonValueTest, ToIndentStringEmptyObject) {
    json_object obj;
    EXPECT_EQ(obj.to_indent_string(), "{}");
}

TEST_F(JsonValueTest, ToIndentStringObjectWithMembers) {
    json_object obj;
    obj.add_member("key", make_unique<json_string>("value"));

    string result = obj.to_indent_string();
    EXPECT_NE(result.find("{\n"), string::npos);
    EXPECT_NE(result.find("\"key\""), string::npos);
    EXPECT_NE(result.find("\"value\""), string::npos);
    EXPECT_NE(result.find("\n}"), string::npos);
}


class JsonParserTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(JsonParserTest, ParseNull) {
    json_parser parser("null");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_null());
}

TEST_F(JsonParserTest, ParseTrue) {
    json_parser parser("true");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_bool());
    EXPECT_TRUE(result->as_bool()->get_value());
}

TEST_F(JsonParserTest, ParseFalse) {
    json_parser parser("false");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_bool());
    EXPECT_FALSE(result->as_bool()->get_value());
}

TEST_F(JsonParserTest, ParseInteger) {
    json_parser parser("42");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_number());
    EXPECT_DOUBLE_EQ(result->as_number()->get_value(), 42.0);
}

TEST_F(JsonParserTest, ParseNegativeInteger) {
    json_parser parser("-42");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_number());
    EXPECT_DOUBLE_EQ(result->as_number()->get_value(), -42.0);
}

TEST_F(JsonParserTest, ParseFloat) {
    json_parser parser("3.14");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_number());
    EXPECT_DOUBLE_EQ(result->as_number()->get_value(), 3.14);
}

TEST_F(JsonParserTest, ParseNegativeFloat) {
    json_parser parser("-3.14");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_number());
    EXPECT_DOUBLE_EQ(result->as_number()->get_value(), -3.14);
}

TEST_F(JsonParserTest, ParseExponentNumber) {
    json_parser parser("1e10");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_number());
    EXPECT_DOUBLE_EQ(result->as_number()->get_value(), 1e10);
}

TEST_F(JsonParserTest, ParseExponentNegativeNumber) {
    json_parser parser("2.5e-3");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_number());
    EXPECT_DOUBLE_EQ(result->as_number()->get_value(), 2.5e-3);
}

TEST_F(JsonParserTest, ParseExponentPositiveNumber) {
    json_parser parser("1.5e+3");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_number());
    EXPECT_DOUBLE_EQ(result->as_number()->get_value(), 1.5e+3);
}

TEST_F(JsonParserTest, ParseNumberStartingWithZero) {
    json_parser parser("0.5");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_number());
    EXPECT_DOUBLE_EQ(result->as_number()->get_value(), 0.5);
}

TEST_F(JsonParserTest, ParseEmptyString) {
    json_parser parser("\"\"");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_string());
    EXPECT_EQ(result->as_string()->get_value(), "");
}

TEST_F(JsonParserTest, ParseSimpleString) {
    json_parser parser("\"hello\"");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_string());
    EXPECT_EQ(result->as_string()->get_value(), "hello");
}

TEST_F(JsonParserTest, ParseStringWithSpaces) {
    json_parser parser("\"hello world\"");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_string());
    EXPECT_EQ(result->as_string()->get_value(), "hello world");
}

TEST_F(JsonParserTest, ParseEmptyArray) {
    json_parser parser("[]");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_array());
    EXPECT_EQ(result->as_array()->size(), 0);
}

TEST_F(JsonParserTest, ParseArrayWithSingleElement) {
    json_parser parser("[42]");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_array());
    EXPECT_EQ(result->as_array()->size(), 1);
    EXPECT_DOUBLE_EQ(result->as_array()->get_element(0)->as_number()->get_value(), 42.0);
}

TEST_F(JsonParserTest, ParseArrayWithMultipleElements) {
    json_parser parser("[1, 2, 3]");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_array());
    EXPECT_EQ(result->as_array()->size(), 3);
    EXPECT_DOUBLE_EQ(result->as_array()->get_element(0)->as_number()->get_value(), 1.0);
    EXPECT_DOUBLE_EQ(result->as_array()->get_element(1)->as_number()->get_value(), 2.0);
    EXPECT_DOUBLE_EQ(result->as_array()->get_element(2)->as_number()->get_value(), 3.0);
}

TEST_F(JsonParserTest, ParseArrayWithMixedTypes) {
    json_parser parser("[null, true, 42, \"text\"]");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_array());
    EXPECT_EQ(result->as_array()->size(), 4);
    EXPECT_TRUE(result->as_array()->get_element(0)->is_null());
    EXPECT_TRUE(result->as_array()->get_element(1)->as_bool()->get_value());
    EXPECT_DOUBLE_EQ(result->as_array()->get_element(2)->as_number()->get_value(), 42.0);
    EXPECT_EQ(result->as_array()->get_element(3)->as_string()->get_value(), "text");
}

TEST_F(JsonParserTest, ParseEmptyObject) {
    json_parser parser("{}");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_object());
    EXPECT_EQ(result->as_object()->get_members().size(), 0);
}

TEST_F(JsonParserTest, ParseObjectWithSingleMember) {
    json_parser parser("{\"key\": \"value\"}");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_object());
    const json_value* member = result->as_object()->get_member("key");
    ASSERT_NE(member, nullptr);
    EXPECT_EQ(member->as_string()->get_value(), "value");
}

TEST_F(JsonParserTest, ParseObjectWithMultipleMembers) {
    json_parser parser("{\"name\": \"John\", \"age\": 30}");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_object());
    EXPECT_EQ(result->as_object()->get_member("name")->as_string()->get_value(), "John");
    EXPECT_DOUBLE_EQ(result->as_object()->get_member("age")->as_number()->get_value(), 30.0);
}

TEST_F(JsonParserTest, ParseNestedObject) {
    json_parser parser("{\"outer\": {\"inner\": 42}}");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_object());
    const json_value* outer = result->as_object()->get_member("outer");
    ASSERT_NE(outer, nullptr);
    EXPECT_TRUE(outer->is_object());
    EXPECT_DOUBLE_EQ(outer->as_object()->get_member("inner")->as_number()->get_value(), 42.0);
}

TEST_F(JsonParserTest, ParseNestedArray) {
    json_parser parser("[[1, 2], [3, 4]]");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_array());
    EXPECT_EQ(result->as_array()->size(), 2);
    EXPECT_TRUE(result->as_array()->get_element(0)->is_array());
    EXPECT_TRUE(result->as_array()->get_element(1)->is_array());
}

TEST_F(JsonParserTest, TryParseValid) {
    json_parser parser("42");
    auto result = parser.try_parse();
    EXPECT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result.value()->as_number()->get_value(), 42.0);
}

TEST_F(JsonParserTest, TryParseInvalid) {
    json_parser parser("invalid");
    auto result = parser.try_parse();
    EXPECT_FALSE(result.has_value());
}

TEST_F(JsonParserTest, ParseInvalidThrows) {
    json_parser parser("invalid");
    EXPECT_THROW(parser.parse(), json_exception);
}

TEST_F(JsonParserTest, ParseIncompleteObjectThrows) {
    json_parser parser("{");
    EXPECT_THROW(parser.parse(), json_exception);
}

TEST_F(JsonParserTest, ParseIncompleteArrayThrows) {
    json_parser parser("[");
    EXPECT_THROW(parser.parse(), json_exception);
}

TEST_F(JsonParserTest, ParseMissingColonThrows) {
    json_parser parser("{\"key\" \"value\"}");
    EXPECT_THROW(parser.parse(), json_exception);
}

TEST_F(JsonParserTest, ParseMissingCommaInArrayThrows) {
    json_parser parser("[1 2]");
    EXPECT_THROW(parser.parse(), json_exception);
}

TEST_F(JsonParserTest, ParseMissingCommaInObjectThrows) {
    json_parser parser("{\"a\": 1 \"b\": 2}");
    EXPECT_THROW(parser.parse(), json_exception);
}

TEST_F(JsonParserTest, ParseExtraCharactersThrows) {
    json_parser parser("42 extra");
    EXPECT_THROW(parser.parse(), json_exception);
}

TEST_F(JsonParserTest, ParseUnterminatedStringThrows) {
    json_parser parser("\"hello");
    EXPECT_THROW(parser.parse(), json_exception);
}

TEST_F(JsonParserTest, ParseInvalidNumberFormatThrows) {
    json_parser parser("01");
    EXPECT_THROW(parser.parse(), json_exception);
}

TEST_F(JsonParserTest, ParseEmptyInputThrows) {
    json_parser parser("");
    EXPECT_THROW(parser.parse(), json_exception);
}

TEST_F(JsonParserTest, ParseOnlySpacesThrows) {
    json_parser parser("   ");
    EXPECT_THROW(parser.parse(), json_exception);
}

TEST_F(JsonParserTest, ParseObjectWithNestedArray) {
    json_parser parser("{\"data\": [1, 2, 3]}");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_object());
    const json_value* data = result->as_object()->get_member("data");
    ASSERT_NE(data, nullptr);
    EXPECT_TRUE(data->is_array());
    EXPECT_EQ(data->as_array()->size(), 3);
}

TEST_F(JsonParserTest, ParseArrayOfObjects) {
    json_parser parser("[{\"a\": 1}, {\"b\": 2}]");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_array());
    EXPECT_EQ(result->as_array()->size(), 2);
    EXPECT_TRUE(result->as_array()->get_element(0)->is_object());
    EXPECT_TRUE(result->as_array()->get_element(1)->is_object());
}

TEST_F(JsonParserTest, ParseComplexNestedJson) {
    string json = R"({
        "name": "John",
        "age": 30,
        "isStudent": false,
        "scores": [95, 87, 92],
        "address": {
            "street": "123 Main St",
            "city": "New York"
        },
        "graduated": null
    })";
    json_parser parser(json);
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_object());
    EXPECT_EQ(result->as_object()->get_member("name")->as_string()->get_value(), "John");
    EXPECT_DOUBLE_EQ(result->as_object()->get_member("age")->as_number()->get_value(), 30.0);
    EXPECT_FALSE(result->as_object()->get_member("isStudent")->as_bool()->get_value());
    EXPECT_EQ(result->as_object()->get_member("scores")->as_array()->size(), 3);
    EXPECT_NE(result->as_object()->get_member("address"), nullptr);
    EXPECT_TRUE(result->as_object()->get_member("graduated")->is_null());
}


class JsonBuilderTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(JsonBuilderTest, BuildNull) {
    json_builder builder;
    auto result = builder.value(nullptr).build();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_null());
}

TEST_F(JsonBuilderTest, BuildBoolTrue) {
    json_builder builder;
    auto result = builder.value(true).build();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_bool());
    EXPECT_TRUE(result->as_bool()->get_value());
}

TEST_F(JsonBuilderTest, BuildBoolFalse) {
    json_builder builder;
    auto result = builder.value(false).build();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_bool());
    EXPECT_FALSE(result->as_bool()->get_value());
}

TEST_F(JsonBuilderTest, BuildInt) {
    json_builder builder;
    auto result = builder.value(42).build();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_number());
    EXPECT_DOUBLE_EQ(result->as_number()->get_value(), 42.0);
}

TEST_F(JsonBuilderTest, BuildDouble) {
    json_builder builder;
    auto result = builder.value(3.14).build();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_number());
    EXPECT_DOUBLE_EQ(result->as_number()->get_value(), 3.14);
}

TEST_F(JsonBuilderTest, BuildString) {
    json_builder builder;
    auto result = builder.value(string("hello")).build();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_string());
    EXPECT_EQ(result->as_string()->get_value(), "hello");
}

TEST_F(JsonBuilderTest, BuildCString) {
    json_builder builder;
    auto result = builder.value("hello").build();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_string());
    EXPECT_EQ(result->as_string()->get_value(), "hello");
}

TEST_F(JsonBuilderTest, BuildEmptyObject) {
    json_builder builder;
    builder.begin_object();
    builder.end_object();
    auto result = builder.build();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_object());
    EXPECT_EQ(result->as_object()->get_members().size(), 0);
}

TEST_F(JsonBuilderTest, BuildObjectWithMembers) {
    json_builder builder;
    builder.begin_object();
    builder.key("name").value(string("John"));
    builder.key("age").value(30);
    builder.end_object();
    auto result = builder.build();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_object());
    EXPECT_EQ(result->as_object()->get_member("name")->as_string()->get_value(), "John");
    EXPECT_DOUBLE_EQ(result->as_object()->get_member("age")->as_number()->get_value(), 30.0);
}

TEST_F(JsonBuilderTest, BuildEmptyArray) {
    json_builder builder;
    builder.begin_array();
    builder.end_array();
    auto result = builder.build();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_array());
    EXPECT_EQ(result->as_array()->size(), 0);
}

TEST_F(JsonBuilderTest, BuildArrayWithElements) {
    json_builder builder;
    builder.begin_array();
    builder.value(1);
    builder.value(2);
    builder.value(3);
    builder.end_array();
    auto result = builder.build();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_array());
    EXPECT_EQ(result->as_array()->size(), 3);
    EXPECT_DOUBLE_EQ(result->as_array()->get_element(0)->as_number()->get_value(), 1.0);
    EXPECT_DOUBLE_EQ(result->as_array()->get_element(1)->as_number()->get_value(), 2.0);
    EXPECT_DOUBLE_EQ(result->as_array()->get_element(2)->as_number()->get_value(), 3.0);
}

TEST_F(JsonBuilderTest, BuildNestedObject) {
    json_builder builder;
    builder.begin_object();
    builder.key("data").begin_object();
    builder.key("value").value(42);
    builder.end_object();
    builder.end_object();
    auto result = builder.build();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_object());
    const json_value* data = result->as_object()->get_member("data");
    ASSERT_NE(data, nullptr);
    EXPECT_TRUE(data->is_object());
    EXPECT_DOUBLE_EQ(data->as_object()->get_member("value")->as_number()->get_value(), 42.0);
}

TEST_F(JsonBuilderTest, BuildNestedArray) {
    json_builder builder;
    builder.begin_array();
    builder.begin_array();
    builder.value(1);
    builder.value(2);
    builder.end_array();
    builder.begin_array();
    builder.value(3);
    builder.value(4);
    builder.end_array();
    builder.end_array();
    auto result = builder.build();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_array());
    EXPECT_EQ(result->as_array()->size(), 2);
    EXPECT_TRUE(result->as_array()->get_element(0)->is_array());
    EXPECT_TRUE(result->as_array()->get_element(1)->is_array());
}

TEST_F(JsonBuilderTest, BuildObjectInArray) {
    json_builder builder;
    builder.begin_array();
    builder.begin_object();
    builder.key("a").value(1);
    builder.end_object();
    builder.begin_object();
    builder.key("b").value(2);
    builder.end_object();
    builder.end_array();
    auto result = builder.build();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_array());
    EXPECT_EQ(result->as_array()->size(), 2);
    EXPECT_TRUE(result->as_array()->get_element(0)->is_object());
    EXPECT_TRUE(result->as_array()->get_element(1)->is_object());
}

TEST_F(JsonBuilderTest, BuildArrayInObject) {
    json_builder builder;
    builder.begin_object();
    builder.key("items").begin_array();
    builder.value(1);
    builder.value(2);
    builder.end_array();
    builder.end_object();
    auto result = builder.build();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_object());
    const json_value* items = result->as_object()->get_member("items");
    ASSERT_NE(items, nullptr);
    EXPECT_TRUE(items->is_array());
    EXPECT_EQ(items->as_array()->size(), 2);
}

TEST_F(JsonBuilderTest, BuildValueObjectFunction) {
    json_builder builder;
    builder.value_object([](json_builder& inner) {
        inner.key("x").value(10);
        inner.key("y").value(20);
    });
    auto result = builder.build();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_object());
    EXPECT_DOUBLE_EQ(result->as_object()->get_member("x")->as_number()->get_value(), 10.0);
    EXPECT_DOUBLE_EQ(result->as_object()->get_member("y")->as_number()->get_value(), 20.0);
}

TEST_F(JsonBuilderTest, BuildValueArrayFunction) {
    json_builder builder;
    builder.value_array([](json_builder& inner) {
        inner.value(1);
        inner.value(2);
        inner.value(3);
    });
    auto result = builder.build();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_array());
    EXPECT_EQ(result->as_array()->size(), 3);
}

TEST_F(JsonBuilderTest, BuildComplexStructure) {
    json_builder builder;
    builder.begin_object();
    builder.key("name").value(string("John"));
    builder.key("age").value(30);
    builder.key("scores").begin_array();
    builder.value(95);
    builder.value(87);
    builder.value(92);
    builder.end_array();
    builder.key("address").begin_object();
    builder.key("street").value(string("123 Main St"));
    builder.key("city").value(string("New York"));
    builder.end_object();
    builder.key("graduated").value(nullptr);
    builder.end_object();

    auto result = builder.build();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_object());
    EXPECT_EQ(result->as_object()->get_member("name")->as_string()->get_value(), "John");
    EXPECT_DOUBLE_EQ(result->as_object()->get_member("age")->as_number()->get_value(), 30.0);
    EXPECT_EQ(result->as_object()->get_member("scores")->as_array()->size(), 3);
    EXPECT_NE(result->as_object()->get_member("address"), nullptr);
    EXPECT_TRUE(result->as_object()->get_member("graduated")->is_null());
}

TEST_F(JsonBuilderTest, BuildWithoutEndThrows) {
    json_builder builder;
    builder.begin_object();
    EXPECT_THROW(builder.build(), json_exception);
}

TEST_F(JsonBuilderTest, BuildWithoutValueThrows) {
    json_builder builder;
    EXPECT_THROW(builder.build(), json_exception);
}

TEST_F(JsonBuilderTest, KeyOutsideObjectThrows) {
    json_builder builder;
    builder.begin_array();
    EXPECT_THROW(builder.key("test"), json_exception);
}

TEST_F(JsonBuilderTest, EndObjectWithoutBeginThrows) {
    json_builder builder;
    EXPECT_THROW(builder.end_object(), json_exception);
}

TEST_F(JsonBuilderTest, EndArrayWithoutBeginThrows) {
    json_builder builder;
    EXPECT_THROW(builder.end_array(), json_exception);
}

TEST_F(JsonBuilderTest, EndObjectMismatchArrayThrows) {
    json_builder builder;
    builder.begin_array();
    EXPECT_THROW(builder.end_object(), json_exception);
}

TEST_F(JsonBuilderTest, EndArrayMismatchObjectThrows) {
    json_builder builder;
    builder.begin_object();
    EXPECT_THROW(builder.end_array(), json_exception);
}

TEST_F(JsonBuilderTest, KeyWithoutValueThrows) {
    json_builder builder;
    builder.begin_object();
    builder.key("name");
    EXPECT_THROW(builder.end_object(), json_exception);
}

TEST_F(JsonBuilderTest, MultiValueDispatchesToNull) {
    json_builder builder;
    nullptr_t np = nullptr;
    auto result = builder.value(np).build();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_null());
}
