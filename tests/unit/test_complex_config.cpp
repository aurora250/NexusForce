#include <NeForce/core/container/flat_unordered_map.hpp>
#include <NeForce/core/file/toml/toml_builder.hpp>
#include <NeForce/core/file/toml/toml_parser.hpp>
#include <NeForce/core/file/yaml/yaml_builder.hpp>
#include <NeForce/core/file/yaml/yaml_parser.hpp>
#include <NeForce/core/numeric/math.hpp>
#include <NeForce/core/string/to_numerics.hpp>
#include <gtest/gtest.h>
using namespace neforce;

class TomlValueTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(TomlValueTest, DefaultValueIsUnset) {
    toml_value value;
    EXPECT_EQ(value.kind(), toml_kind::null_);
    EXPECT_FALSE(value.is_boolean());
    EXPECT_FALSE(value.is_integer());
    EXPECT_FALSE(value.is_float());
    EXPECT_FALSE(value.is_string());
    EXPECT_FALSE(value.is_datetime());
    EXPECT_FALSE(value.is_array());
    EXPECT_FALSE(value.is_table());
    EXPECT_EQ(value.element_count(), 0u);
}

TEST_F(TomlValueTest, BooleanValue) {
    const toml_value value = toml_value::make_boolean(true);
    EXPECT_EQ(value.kind(), toml_kind::boolean);
    EXPECT_TRUE(value.is_boolean());
    EXPECT_FALSE(value.is_integer());
    EXPECT_TRUE(value.as_boolean().get_value());
    EXPECT_FALSE(toml_value::make_boolean(false).as_boolean().get_value());
    EXPECT_EQ(value.to_string(), "true");
    EXPECT_EQ(toml_value::make_boolean(false).to_string(), "false");
}

TEST_F(TomlValueTest, IntegerValue) {
    const toml_value value = toml_value::make_integer(42);
    EXPECT_EQ(value.kind(), toml_kind::integer);
    EXPECT_TRUE(value.is_integer());
    EXPECT_EQ(value.as_integer().get_value(), 42);
    EXPECT_EQ(value.to_string(), "42");
    EXPECT_EQ(toml_value::make_integer(-42).as_integer().get_value(), -42);
    EXPECT_EQ(toml_value::make_integer(-9223372036854775807LL - 1).to_string(), "-9223372036854775808");
    EXPECT_EQ(toml_value::make_integer(9223372036854775807LL).to_string(), "9223372036854775807");
}

TEST_F(TomlValueTest, FloatValue) {
    const toml_value value = toml_value::make_float(3.14);
    EXPECT_EQ(value.kind(), toml_kind::floating);
    EXPECT_TRUE(value.is_float());
    EXPECT_DOUBLE_EQ(value.as_float().get_value(), 3.14);
    EXPECT_EQ(value.to_string(), "3.14");
    EXPECT_EQ(toml_value::make_float(2.0).to_string(), "2.0");
    EXPECT_EQ(toml_value::make_float(-0.5).to_string(), "-0.5");
}

TEST_F(TomlValueTest, StringBasicValue) {
    const toml_value value = toml_value::make_string("hello");
    EXPECT_EQ(value.kind(), toml_kind::string);
    EXPECT_TRUE(value.is_string());
    EXPECT_EQ(value.as_string().get_value(), "hello");
    EXPECT_EQ(value.as_string().get_string_type(), toml_string::Basic);
    EXPECT_EQ(value.element_count(), 5u);
    EXPECT_EQ(value.to_string(), "\"hello\"");
}

TEST_F(TomlValueTest, StringInlineAndLong) {
    toml_document doc;
    const toml_value inlined = doc.make_string_value("short");
    EXPECT_EQ(inlined.as_string().get_value(), "short");

    const char* const text = "a string longer than fourteen bytes";
    const toml_value borrowed = toml_value::make_string(text);
    EXPECT_EQ(borrowed.as_string().data(), text);
    EXPECT_EQ(borrowed.as_string().size(), 35u);

    const toml_value copied = doc.make_string_value(text);
    EXPECT_NE(copied.as_string().data(), text);
    EXPECT_EQ(copied.as_string().get_value(), text);
}

TEST_F(TomlValueTest, StringTypes) {
    const toml_value basic = toml_value::make_string("basic", toml_basic_string);
    const toml_value literal = toml_value::make_string("literal", toml_literal_string);
    const toml_value multi_basic = toml_value::make_string("multi_basic", toml_multi_basic_string);
    const toml_value multi_literal = toml_value::make_string("multi_literal", toml_multi_literal_string);

    EXPECT_EQ(basic.as_string().get_string_type(), toml_string::Basic);
    EXPECT_EQ(literal.as_string().get_string_type(), toml_string::Literal);
    EXPECT_EQ(multi_basic.as_string().get_string_type(), toml_string::MultiBasic);
    EXPECT_EQ(multi_literal.as_string().get_string_type(), toml_string::MultiLiteral);

    EXPECT_EQ(basic.to_string(), "\"basic\"");
    EXPECT_EQ(literal.to_string(), "'literal'");
    EXPECT_EQ(multi_basic.to_string(), "\"\"\"multi_basic\"\"\"");
    EXPECT_EQ(multi_literal.to_string(), "'''multi_literal'''");
}

TEST_F(TomlValueTest, DatetimeOffsetDateTime) {
    const toml_value value = toml_value::make_datetime(datetime(1979, 5, 27, 7, 32, 0, 0), toml_offset_datetime);
    EXPECT_EQ(value.kind(), toml_kind::datetime);
    EXPECT_TRUE(value.is_datetime());
    EXPECT_EQ(value.as_datetime().get_datetime_type(), toml_datetime::OffsetDateTime);
    EXPECT_EQ(value.as_datetime().get_string_value(), "1979-05-27T07:32:00Z");
    EXPECT_EQ(value.to_string(), "1979-05-27T07:32:00Z");
}

TEST_F(TomlValueTest, DatetimeLocalDateTime) {
    toml_document doc;
    const toml_value value = doc.make_datetime_value("1979-05-27T07:32:00", toml_local_datetime);
    EXPECT_EQ(value.as_datetime().get_datetime_type(), toml_datetime::LocalDateTime);
    EXPECT_EQ(value.as_datetime().get_string_value(), "1979-05-27T07:32:00");
    EXPECT_EQ(value.as_datetime().get_value().year(), 1979);
    EXPECT_EQ(value.as_datetime().get_value().minutes(), 32);
}

TEST_F(TomlValueTest, DatetimeLocalDate) {
    toml_document doc;
    const toml_value value = doc.make_datetime_value("1979-05-27", toml_local_date);
    EXPECT_EQ(value.as_datetime().get_datetime_type(), toml_datetime::LocalDate);
    EXPECT_EQ(value.as_datetime().get_string_value(), "1979-05-27");
    EXPECT_TRUE(value.as_datetime().get_value().is_valid());
}

TEST_F(TomlValueTest, DatetimeLocalTime) {
    toml_document doc;
    const toml_value value = doc.make_datetime_value("07:32:00", toml_local_time);
    EXPECT_EQ(value.as_datetime().get_datetime_type(), toml_datetime::LocalTime);
    EXPECT_EQ(value.as_datetime().get_string_value(), "07:32:00");
}

TEST_F(TomlValueTest, DatetimeOffsetRoundTrip) {
    toml_document doc;
    const toml_value value = doc.make_datetime_value("1979-05-27T00:32:00-07:00", toml_offset_datetime);
    EXPECT_EQ(value.as_datetime().get_value().offset_seconds(), -7 * 3600);
    EXPECT_EQ(value.as_datetime().get_string_value(), "1979-05-27T00:32:00-07:00");
}

TEST_F(TomlValueTest, ArrayAddAndGetElement) {
    toml_document doc;
    toml_array_mut array = doc.make_array();
    array.add_element(toml_value::make_integer(1));
    array.add_element(toml_value::make_integer(2));
    array.add_element(toml_value::make_string("three"));

    ASSERT_EQ(doc->as_array().size(), 3u);
    EXPECT_EQ(doc->as_array().get_element(0)->as_integer().get_value(), 1);
    EXPECT_EQ(doc->as_array().get_element(1)->as_integer().get_value(), 2);
    EXPECT_EQ(doc->as_array().get_element(2)->as_string().get_value(), "three");
    EXPECT_EQ(doc->as_array().get_element(3), nullptr);
    EXPECT_FALSE(doc->as_array().empty());
}

TEST_F(TomlValueTest, ArrayIteration) {
    toml_document doc;
    toml_array_mut array = doc.make_array();
    for (int i = 0; i < 5; ++i) {
        array.add_element(toml_value::make_integer(i));
    }
    int64_t sum = 0;
    size_t count = 0;
    for (const toml_value& element: doc->as_array().get_elements()) {
        sum += element.as_integer().get_value();
        ++count;
    }
    EXPECT_EQ(count, 5u);
    EXPECT_EQ(sum, 10);
}

TEST_F(TomlValueTest, EmptyArrayAndTable) {
    toml_document doc;
    doc.make_array();
    EXPECT_TRUE(doc->as_array().empty());
    EXPECT_EQ(doc->to_string(), "[]");
    EXPECT_EQ(doc->to_document(), "[]");

    doc.make_table();
    EXPECT_TRUE(doc->as_table().empty());
    EXPECT_EQ(doc->to_string(), "{}");
    EXPECT_EQ(doc->to_document(), "");
}

TEST_F(TomlValueTest, TableAddAndGetMember) {
    toml_document doc;
    toml_table_mut table = doc.make_table();
    table.add_member("key1", doc.make_string_value("value1"));
    table.add_member("key2", toml_value::make_integer(42));

    const toml_value* member1 = doc->as_table().get_member("key1");
    ASSERT_NE(member1, nullptr);
    EXPECT_TRUE(member1->is_string());
    EXPECT_EQ(member1->as_string().get_value(), "value1");

    const toml_value* member2 = doc->as_table().get_member("key2");
    ASSERT_NE(member2, nullptr);
    EXPECT_TRUE(member2->is_integer());
    EXPECT_EQ(member2->as_integer().get_value(), 42);
    EXPECT_EQ(doc->as_table().get_member("nonexistent"), nullptr);
    EXPECT_TRUE(doc->as_table().has_member("key1"));
    EXPECT_FALSE(doc->as_table().has_member("missing"));
}

TEST_F(TomlValueTest, TableKeyOverloads) {
    toml_document doc;
    toml_table_mut table = doc.make_table();
    table.add_member("key", toml_value::make_boolean(true));

    const string key("key");
    EXPECT_NE(doc->as_table().get_member(key), nullptr);
    EXPECT_NE(doc->as_table().get_member("key"), nullptr);
    EXPECT_NE(doc->as_table().get_member(string_view("key")), nullptr);
    EXPECT_EQ(doc->as_table().get_member(string("other")), nullptr);
    EXPECT_TRUE(doc->as_table().has_member(key));
    EXPECT_FALSE(doc->as_table().has_member(string("other")));
}

TEST_F(TomlValueTest, TableInsertionOrder) {
    toml_document doc;
    toml_table_mut table = doc.make_table();
    table.add_member("first", toml_value::make_integer(1));
    table.add_member("second", toml_value::make_integer(2));
    table.add_member("third", toml_value::make_integer(3));

    const toml_table view = doc->as_table();
    ASSERT_EQ(view.size(), 3u);
    auto it = view.begin();
    EXPECT_EQ(it->key(), "first");
    ++it;
    EXPECT_EQ(it->key(), "second");
    ++it;
    EXPECT_EQ(it->key(), "third");
}

TEST_F(TomlValueTest, TableInlineFlag) {
    toml_document doc;
    toml_table_mut standard = doc.make_table();
    EXPECT_FALSE(doc->as_table().is_inline());

    toml_table_mut inlined = doc.new_table(true);
    EXPECT_TRUE(inlined.as_table().is_inline());
    EXPECT_FALSE(standard.as_table().is_inline());
}

TEST_F(TomlValueTest, TableLargeMemberLookup) {
    toml_document doc;
    toml_table_mut table = doc.make_table();
    for (int i = 0; i < 600; ++i) {
        table.add_member(string("key") + to_string(i), toml_value::make_integer(i));
    }
    ASSERT_EQ(doc->as_table().size(), 600u);
    for (int i = 0; i < 600; ++i) {
        const toml_value* value = doc->as_table().get_member(string("key") + to_string(i));
        ASSERT_NE(value, nullptr);
        EXPECT_EQ(value->as_integer().get_value(), i);
    }
    EXPECT_EQ(doc->as_table().get_member("key600"), nullptr);
    EXPECT_EQ(doc->as_table().get_member("key1x"), nullptr);
}

TEST_F(TomlValueTest, InlineKeyAndArenaKey) {
    toml_document doc;
    toml_table_mut table = doc.make_table();
    table.add_member("short", toml_value::make_integer(1));
    table.add_member("a_key_longer_than_eight_bytes", toml_value::make_integer(2));

    const toml_table view = doc->as_table();
    auto it = view.begin();
    EXPECT_TRUE(it->key_inlined());
    EXPECT_EQ(it->key(), "short");
    ++it;
    EXPECT_FALSE(it->key_inlined());
    EXPECT_EQ(it->key(), "a_key_longer_than_eight_bytes");
    EXPECT_NE(view.get_member("a_key_longer_than_eight_bytes"), nullptr);
}

TEST_F(TomlValueTest, KeyPrefixFiltersLookups) {
    toml_member member;
    member.assign_key("hash");
    EXPECT_EQ(member.key_prefix, toml_member::prefix_of("hash"));
    EXPECT_NE(member.key_prefix, toml_member::prefix_of("hasj"));
    EXPECT_FALSE(member.key_equal(toml_member::prefix_of("hasj"), "hasj"));
    EXPECT_TRUE(member.key_equal(toml_member::prefix_of("hash"), "hash"));

    toml_member short_key;
    short_key.assign_key("ab");
    EXPECT_EQ(short_key.key_prefix, toml_member::prefix_of("ab"));
    EXPECT_TRUE(short_key.key_equal(toml_member::prefix_of("ab"), "ab"));
    EXPECT_FALSE(short_key.key_equal(toml_member::prefix_of("ab_"), "ab_"));

    toml_member long_key;
    long_key.assign_key("a-longer-key");
    EXPECT_FALSE(long_key.key_inlined());
    EXPECT_TRUE(long_key.key_equal(toml_member::prefix_of("a-longer-key"), "a-longer-key"));
    EXPECT_FALSE(long_key.key_equal(toml_member::prefix_of("a-longer-kez"), "a-longer-kez"));
}

TEST_F(TomlValueTest, ContainerReserve) {
    toml_document doc;
    toml_table_mut table = doc.make_table();
    doc.table_reserve(doc.root(), 8);
    for (int i = 0; i < 8; ++i) {
        table.add_member(string("key") + to_string(i), toml_value::make_integer(i));
    }
    EXPECT_EQ(table.size(), 8u);
    table.add_member("extra", toml_value::make_integer(99));
    EXPECT_EQ(doc->as_table().get_member("extra")->as_integer().get_value(), 99);
    EXPECT_EQ(doc->as_table().get_member("key7")->as_integer().get_value(), 7);

    doc.make_array();
    doc.array_reserve(doc.root(), 4);
    for (int i = 0; i < 6; ++i) {
        doc.array_append(doc.root(), toml_value::make_integer(i));
    }
    EXPECT_EQ(doc->as_array().size(), 6u);
    EXPECT_EQ(doc->as_array().get_element(5)->as_integer().get_value(), 5);

    doc.table_reserve(doc.root(), 0);
    doc.array_reserve(doc.root(), 70000);
}

TEST_F(TomlValueTest, ValueTypePredicates) {
    toml_document doc;
    const toml_value boolean = toml_value::make_boolean(true);
    const toml_value integer = toml_value::make_integer(0);
    const toml_value floating = toml_value::make_float(0.0);
    const toml_value string_value = doc.make_string_value("");
    const toml_value datetime_value = doc.make_datetime_value("1979-05-27", toml_local_date);
    const toml_value array_value = doc.new_array().to_value();
    const toml_value table_value = doc.new_table().to_value();

    EXPECT_TRUE(boolean.is_boolean());
    EXPECT_TRUE(integer.is_integer());
    EXPECT_TRUE(floating.is_float());
    EXPECT_TRUE(string_value.is_string());
    EXPECT_TRUE(datetime_value.is_datetime());
    EXPECT_TRUE(array_value.is_array());
    EXPECT_TRUE(table_value.is_table());

    EXPECT_FALSE(boolean.is_integer());
    EXPECT_FALSE(integer.is_boolean());
    EXPECT_FALSE(floating.is_integer());
    EXPECT_FALSE(string_value.is_table());
    EXPECT_FALSE(datetime_value.is_string());
    EXPECT_FALSE(array_value.is_table());
    EXPECT_FALSE(table_value.is_array());
}

TEST_F(TomlValueTest, StringEscapedToString) {
    toml_document doc;
    EXPECT_EQ(doc.make_string_value("a\"b\\c\nd\te").to_string(), "\"a\\\"b\\\\c\\nd\\te\"");
    const char raw[] = {'a', char(0x01), '\0'};
    EXPECT_EQ(doc.make_string_value(string_view(raw, 2)).to_string(), "\"a\\u0001\"");
}

TEST_F(TomlValueTest, StringQuotingFallbacks) {
    toml_document doc;
    EXPECT_EQ(doc.make_string_value("it's", toml_literal_string).to_string(), "\"it's\"");
    EXPECT_EQ(doc.make_string_value("a'''b", toml_multi_literal_string).to_string(), "\"a'''b\"");
    EXPECT_EQ(doc.make_string_value("say \"hi\"", toml_multi_basic_string).to_string(), "\"\"\"say \\\"hi\\\"\"\"\"");
}

TEST_F(TomlValueTest, KeyQuoting) {
    toml_document doc;
    toml_table_mut table = doc.make_table();
    table.add_member("plain-key_1", toml_value::make_integer(1));
    table.add_member("needs space", toml_value::make_integer(2));

    EXPECT_EQ(doc->to_string(), "{ plain-key_1 = 1, \"needs space\" = 2 }");
}

TEST_F(TomlValueTest, ToStringArray) {
    toml_document doc;
    toml_array_mut array = doc.make_array();
    array.add_element(toml_value::make_integer(1));
    array.add_element(toml_value::make_string("two"));
    array.add_element(toml_value::make_boolean(true));
    EXPECT_EQ(doc->to_string(), "[1, \"two\", true]");
}

TEST_F(TomlValueTest, ToStringInlineAndStandardTable) {
    toml_document doc;
    toml_table_mut table = doc.make_table();
    table.add_member("key", toml_value::make_integer(42));
    EXPECT_EQ(doc->to_string(), "{ key = 42 }");

    toml_table_mut inlined = doc.new_table(true);
    inlined.add_member("key", toml_value::make_integer(42));
    EXPECT_EQ(inlined.to_value().to_string(), "{ key = 42 }");
}

TEST_F(TomlValueTest, ToDocument) {
    toml_document doc;
    toml_table_mut table = doc.make_table();
    table.add_member("title", doc.make_string_value("Example"));
    table.add_member("enabled", toml_value::make_boolean(true));
    table.add_member("ports", doc.new_array().to_value());

    EXPECT_EQ(doc->to_document(), "title = \"Example\"\nenabled = true\nports = []\n");
}

TEST_F(TomlValueTest, ToDocumentNestedTables) {
    toml_document doc;
    toml_table_mut root = doc.make_table();
    root.add_member("title", doc.make_string_value("Example"));

    toml_value* database = doc.new_container(true);
    doc.table_append(*database, "host", doc.make_string_value("localhost"));
    doc.table_append(*database, "port", toml_value::make_integer(5432));
    root.add_member("database", *database);

    toml_value* servers = doc.new_container(false);
    toml_value* alpha = doc.new_container(true);
    doc.table_append(*alpha, "ip", doc.make_string_value("10.0.0.1"));
    doc.array_append(*servers, *alpha);
    root.add_member("servers", *servers);

    const string text = doc->to_document();
    EXPECT_NE(text.find("title = \"Example\""), string::npos);
    EXPECT_NE(text.find("[database]"), string::npos);
    EXPECT_NE(text.find("host = \"localhost\""), string::npos);
    EXPECT_NE(text.find("[[servers]]"), string::npos);
    EXPECT_NE(text.find("ip = \"10.0.0.1\""), string::npos);
}

TEST_F(TomlValueTest, DocumentMoveConstructor) {
    toml_document doc;
    toml_table_mut table = doc.make_table();
    table.add_member("key", toml_value::make_integer(7));

    toml_document moved(move(doc));
    ASSERT_NE(moved->as_table().get_member("key"), nullptr);
    EXPECT_EQ(moved->as_table().get_member("key")->as_integer().get_value(), 7);
}

TEST_F(TomlValueTest, DocumentMoveAssignment) {
    toml_document doc;
    doc.make_table().add_member("key", toml_value::make_integer(7));

    toml_document other;
    other = move(doc);
    ASSERT_NE(other->as_table().get_member("key"), nullptr);
    EXPECT_EQ(other->as_table().get_member("key")->as_integer().get_value(), 7);
}

TEST_F(TomlValueTest, DocumentBoolAndEmpty) {
    toml_document doc;
    EXPECT_FALSE(static_cast<bool>(doc));
    doc.make_table();
    EXPECT_TRUE(static_cast<bool>(doc));
    EXPECT_TRUE(doc.empty());
    doc.make_table().add_member("key", toml_value::make_integer(1));
    EXPECT_FALSE(doc.empty());
}

class TomlParserTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(TomlParserTest, ParseBareValueThrows) {
    toml_parser parser("true");
    EXPECT_THROW(parser.parse(), toml_exception);
}

TEST_F(TomlParserTest, ParseKeyValueBoolean) {
    toml_parser parser("key = true");
    toml_document result = parser.parse();
    const toml_value* val = result->as_table().get_member("key");
    ASSERT_NE(val, nullptr);
    EXPECT_TRUE(val->is_boolean());
    EXPECT_TRUE(val->as_boolean().get_value());
}

TEST_F(TomlParserTest, ParseKeyValueBooleanFalse) {
    toml_parser parser("key = false");
    toml_document result = parser.parse();
    const toml_value* val = result->as_table().get_member("key");
    ASSERT_NE(val, nullptr);
    EXPECT_TRUE(val->is_boolean());
    EXPECT_FALSE(val->as_boolean().get_value());
}

TEST_F(TomlParserTest, ParseKeyValueInteger) {
    toml_parser parser("key = 42");
    toml_document result = parser.parse();
    const toml_value* val = result->as_table().get_member("key");
    ASSERT_NE(val, nullptr);
    EXPECT_TRUE(val->is_integer());
    EXPECT_EQ(val->as_integer().get_value(), 42);
}

TEST_F(TomlParserTest, ParseKeyValueIntegerNegative) {
    toml_parser parser("key = -42");
    toml_document result = parser.parse();
    EXPECT_EQ(result->as_table().get_member("key")->as_integer().get_value(), -42);
}

TEST_F(TomlParserTest, ParseKeyValueIntegerPositive) {
    toml_parser parser("key = +42");
    toml_document result = parser.parse();
    EXPECT_EQ(result->as_table().get_member("key")->as_integer().get_value(), 42);
}

TEST_F(TomlParserTest, ParseKeyValueFloat) {
    toml_parser parser("key = 3.14");
    toml_document result = parser.parse();
    const toml_value* val = result->as_table().get_member("key");
    ASSERT_NE(val, nullptr);
    EXPECT_TRUE(val->is_float());
    EXPECT_DOUBLE_EQ(val->as_float().get_value(), 3.14);
}

TEST_F(TomlParserTest, ParseKeyValueExponentFloat) {
    toml_parser parser("key = 1e10");
    toml_document result = parser.parse();
    const toml_value* val = result->as_table().get_member("key");
    ASSERT_NE(val, nullptr);
    EXPECT_TRUE(val->is_float());
    EXPECT_DOUBLE_EQ(val->as_float().get_value(), 1e10);
}

TEST_F(TomlParserTest, ParseKeyValueStringBasic) {
    toml_parser parser("key = \"hello\"");
    toml_document result = parser.parse();
    const toml_value* val = result->as_table().get_member("key");
    ASSERT_NE(val, nullptr);
    EXPECT_TRUE(val->is_string());
    EXPECT_EQ(val->as_string().get_value(), "hello");
    EXPECT_EQ(val->as_string().get_string_type(), toml_string::Basic);
}

TEST_F(TomlParserTest, ParseKeyValueStringLiteral) {
    toml_parser parser("key = 'hello'");
    toml_document result = parser.parse();
    EXPECT_EQ(result->as_table().get_member("key")->as_string().get_value(), "hello");
    EXPECT_EQ(result->as_table().get_member("key")->as_string().get_string_type(), toml_string::Literal);
}

TEST_F(TomlParserTest, ParseKeyValueStringMultiBasic) {
    toml_parser parser("key = \"\"\"hello\"\"\"");
    toml_document result = parser.parse();
    EXPECT_EQ(result->as_table().get_member("key")->as_string().get_value(), "hello");
    EXPECT_EQ(result->as_table().get_member("key")->as_string().get_string_type(), toml_string::MultiBasic);
}

TEST_F(TomlParserTest, ParseKeyValueStringMultiLiteral) {
    toml_parser parser("key = '''hello'''");
    toml_document result = parser.parse();
    EXPECT_EQ(result->as_table().get_member("key")->as_string().get_value(), "hello");
    EXPECT_EQ(result->as_table().get_member("key")->as_string().get_string_type(), toml_string::MultiLiteral);
}

TEST_F(TomlParserTest, ParseStringDoesNotEscape) {
    toml_parser parser("key = \"line1\\nline2\"");
    toml_document result = parser.parse();
    EXPECT_EQ(result->as_table().get_member("key")->as_string().get_value(), "line1\nline2");
}

TEST_F(TomlParserTest, ParseStringEscapeTab) {
    toml_parser parser("key = \"col1\\tcol2\"");
    toml_document result = parser.parse();
    EXPECT_EQ(result->as_table().get_member("key")->as_string().get_value(), "col1\tcol2");
}

TEST_F(TomlParserTest, ParseStringUnicodeEscape) {
    toml_parser parser("key = \"\\u0048\\u0065\\u006C\\u006C\\u006F\"");
    toml_document result = parser.parse();
    EXPECT_EQ(result->as_table().get_member("key")->as_string().get_value(), "Hello");
}

TEST_F(TomlParserTest, ParseStringUnicodeFullEscape) {
    toml_parser parser("key = \"\\U0001F600\"");
    toml_document result = parser.parse();
    EXPECT_EQ(result->as_table().get_member("key")->as_string().get_value(), "\xF0\x9F\x98\x80");
}

TEST_F(TomlParserTest, ParseMultiLiteralString) {
    toml_parser parser("key = '''line1\nline2'''");
    toml_document result = parser.parse();
    const toml_value* val = result->as_table().get_member("key");
    ASSERT_NE(val, nullptr);
    EXPECT_TRUE(val->is_string());
    EXPECT_EQ(val->as_string().get_string_type(), toml_string::MultiLiteral);
    EXPECT_EQ(val->as_string().get_value(), "line1\nline2");
}

TEST_F(TomlParserTest, ParseMultiBasicLineContinuation) {
    toml_parser parser("key = \"\"\"line1 \\\n   line2\"\"\"");
    toml_document result = parser.parse();
    EXPECT_EQ(result->as_table().get_member("key")->as_string().get_value(), "line1 line2");
}

TEST_F(TomlParserTest, ParseKeyValueArray) {
    toml_parser parser("key = [1, 2, 3]");
    toml_document result = parser.parse();
    const toml_value* val = result->as_table().get_member("key");
    ASSERT_NE(val, nullptr);
    EXPECT_TRUE(val->is_array());
    EXPECT_EQ(val->as_array().size(), 3u);
    EXPECT_EQ(val->as_array().get_element(0)->as_integer().get_value(), 1);
    EXPECT_EQ(val->as_array().get_element(1)->as_integer().get_value(), 2);
    EXPECT_EQ(val->as_array().get_element(2)->as_integer().get_value(), 3);
    EXPECT_EQ(val->as_array().get_element(3), nullptr);
}

TEST_F(TomlParserTest, ParseArrayWithComments) {
    toml_parser parser("key = [1, # first\n 2, # second\n 3]");
    toml_document result = parser.parse();
    EXPECT_EQ(result->as_table().get_member("key")->as_array().size(), 3u);
}

TEST_F(TomlParserTest, ParseKeyValueInlineTable) {
    toml_parser parser("key = { a = 1, b = 2 }");
    toml_document result = parser.parse();
    const toml_value* val = result->as_table().get_member("key");
    ASSERT_NE(val, nullptr);
    EXPECT_TRUE(val->is_table());
    EXPECT_TRUE(val->as_table().is_inline());
    EXPECT_EQ(val->as_table().get_member("a")->as_integer().get_value(), 1);
    EXPECT_EQ(val->as_table().get_member("b")->as_integer().get_value(), 2);
}

TEST_F(TomlParserTest, ParseTableHeader) {
    toml_parser parser("[table]\nkey = 42");
    toml_document result = parser.parse();
    const toml_value* table = result->as_table().get_member("table");
    ASSERT_NE(table, nullptr);
    EXPECT_TRUE(table->is_table());
    EXPECT_EQ(table->as_table().get_member("key")->as_integer().get_value(), 42);
}

TEST_F(TomlParserTest, ParseNestedTableHeader) {
    toml_parser parser("[parent.child]\nkey = 42");
    toml_document result = parser.parse();
    const toml_value* parent = result->as_table().get_member("parent");
    ASSERT_NE(parent, nullptr);
    EXPECT_TRUE(parent->is_table());
    const toml_value* child = parent->as_table().get_member("child");
    ASSERT_NE(child, nullptr);
    EXPECT_TRUE(child->is_table());
    EXPECT_EQ(child->as_table().get_member("key")->as_integer().get_value(), 42);
}

TEST_F(TomlParserTest, ParseTableHeaderReopen) {
    toml_parser parser("[a]\nx = 1\n[b]\ny = 2\n[a]\nz = 3");
    toml_document result = parser.parse();
    EXPECT_EQ(result->as_table().get_member("a")->as_table().get_member("x")->as_integer().get_value(), 1);
    EXPECT_EQ(result->as_table().get_member("b")->as_table().get_member("y")->as_integer().get_value(), 2);
    EXPECT_EQ(result->as_table().get_member("a")->as_table().get_member("z")->as_integer().get_value(), 3);
}

TEST_F(TomlParserTest, ParseArrayTableHeader) {
    toml_parser parser("[[products]]\nname = \"Hammer\"");
    toml_document result = parser.parse();
    const toml_value* products = result->as_table().get_member("products");
    ASSERT_NE(products, nullptr);
    EXPECT_TRUE(products->is_array());
    EXPECT_EQ(products->as_array().size(), 1u);
    const toml_value* item = products->as_array().get_element(0);
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->is_table());
    EXPECT_EQ(item->as_table().get_member("name")->as_string().get_value(), "Hammer");
}

TEST_F(TomlParserTest, ParseMultipleArrayTableHeaders) {
    toml_parser parser("[[products]]\nname = \"Hammer\"\n[[products]]\nname = \"Nail\"");
    toml_document result = parser.parse();
    const toml_value* products = result->as_table().get_member("products");
    ASSERT_NE(products, nullptr);
    EXPECT_EQ(products->as_array().size(), 2u);
    EXPECT_EQ(products->as_array().get_element(0)->as_table().get_member("name")->as_string().get_value(), "Hammer");
    EXPECT_EQ(products->as_array().get_element(1)->as_table().get_member("name")->as_string().get_value(), "Nail");
}

TEST_F(TomlParserTest, ParseNestedArrayTableHeader) {
    toml_parser parser("[[a.b]]\nkey = 1\n[[a.b]]\nkey = 2");
    toml_document result = parser.parse();
    const toml_value* a = result->as_table().get_member("a");
    ASSERT_NE(a, nullptr);
    const toml_value* b = a->as_table().get_member("b");
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(b->as_array().size(), 2u);
    EXPECT_EQ(b->as_array().get_element(1)->as_table().get_member("key")->as_integer().get_value(), 2);
}

TEST_F(TomlParserTest, ParseDottedKey) {
    toml_parser parser("a.b.c = 42");
    toml_document result = parser.parse();
    const toml_value* a = result->as_table().get_member("a");
    ASSERT_NE(a, nullptr);
    EXPECT_TRUE(a->is_table());
    const toml_value* b = a->as_table().get_member("b");
    ASSERT_NE(b, nullptr);
    EXPECT_TRUE(b->is_table());
    EXPECT_EQ(b->as_table().get_member("c")->as_integer().get_value(), 42);
}

TEST_F(TomlParserTest, ParseDottedKeyRelativeToTable) {
    toml_parser parser("[root]\na.b = 1");
    toml_document result = parser.parse();
    const toml_value* root = result->as_table().get_member("root");
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->as_table().get_member("a")->as_table().get_member("b")->as_integer().get_value(), 1);
}

TEST_F(TomlParserTest, ParseQuotedKey) {
    toml_parser parser("\"key.with.dots\" = 1\n'literal key' = 2\n\"escaped\\u0041\" = 3");
    toml_document result = parser.parse();
    EXPECT_EQ(result->as_table().get_member("key.with.dots")->as_integer().get_value(), 1);
    EXPECT_EQ(result->as_table().get_member("literal key")->as_integer().get_value(), 2);
    EXPECT_EQ(result->as_table().get_member("escapedA")->as_integer().get_value(), 3);
}

TEST_F(TomlParserTest, ParseIntegerWithUnderscores) {
    toml_parser parser("key = 1_000_000");
    toml_document result = parser.parse();
    EXPECT_EQ(result->as_table().get_member("key")->as_integer().get_value(), 1000000);
}

TEST_F(TomlParserTest, ParseFloatWithUnderscores) {
    toml_parser parser("key = 1_000.5_5");
    toml_document result = parser.parse();
    EXPECT_DOUBLE_EQ(result->as_table().get_member("key")->as_float().get_value(), 1000.55);
}

TEST_F(TomlParserTest, ParseIntegerHex) {
    toml_parser parser("key = 0xFF");
    toml_document result = parser.parse();
    EXPECT_EQ(result->as_table().get_member("key")->as_integer().get_value(), 255);
}

TEST_F(TomlParserTest, ParseIntegerOctal) {
    toml_parser parser("key = 0o77");
    toml_document result = parser.parse();
    EXPECT_EQ(result->as_table().get_member("key")->as_integer().get_value(), 63);
}

TEST_F(TomlParserTest, ParseIntegerBinary) {
    toml_parser parser("key = 0b1010");
    toml_document result = parser.parse();
    EXPECT_EQ(result->as_table().get_member("key")->as_integer().get_value(), 10);
}

TEST_F(TomlParserTest, ParseIntegerMax) {
    toml_parser parser("key = 9223372036854775807");
    toml_document result = parser.parse();
    EXPECT_EQ(result->as_table().get_member("key")->as_integer().get_value(), 9223372036854775807LL);
}

TEST_F(TomlParserTest, ParseIntegerOverflowThrows) {
    toml_parser parser("key = 9223372036854775808");
    EXPECT_THROW(parser.parse(), toml_exception);
}

TEST_F(TomlParserTest, ParseDatetimeOffsetDateTime) {
    toml_parser parser("key = 1979-05-27T07:32:00Z");
    toml_document result = parser.parse();
    const toml_value* val = result->as_table().get_member("key");
    ASSERT_NE(val, nullptr);
    EXPECT_TRUE(val->is_datetime());
    EXPECT_EQ(val->as_datetime().get_datetime_type(), toml_datetime::OffsetDateTime);
}

TEST_F(TomlParserTest, ParseDatetimeOffsetWithOffset) {
    toml_parser parser("key = 1979-05-27T00:32:00-07:00");
    toml_document result = parser.parse();
    const toml_value* val = result->as_table().get_member("key");
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(val->as_datetime().get_datetime_type(), toml_datetime::OffsetDateTime);
    EXPECT_EQ(val->as_datetime().get_string_value(), "1979-05-27T00:32:00-07:00");
}

TEST_F(TomlParserTest, ParseDatetimeLocalDateTime) {
    toml_parser parser("key = 1979-05-27T07:32:00");
    toml_document result = parser.parse();
    const toml_value* val = result->as_table().get_member("key");
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(val->as_datetime().get_datetime_type(), toml_datetime::LocalDateTime);
}

TEST_F(TomlParserTest, ParseDatetimeSpaceSeparator) {
    toml_parser parser("key = 1979-05-27 07:32:00");
    toml_document result = parser.parse();
    const toml_value* val = result->as_table().get_member("key");
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(val->as_datetime().get_datetime_type(), toml_datetime::LocalDateTime);
}

TEST_F(TomlParserTest, ParseDatetimeFractional) {
    toml_parser parser("key = 1979-05-27T07:32:00.999Z");
    toml_document result = parser.parse();
    const toml_value* val = result->as_table().get_member("key");
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(val->as_datetime().get_datetime_type(), toml_datetime::OffsetDateTime);
}

TEST_F(TomlParserTest, ParseDatetimeLocalDate) {
    toml_parser parser("key = 1979-05-27");
    toml_document result = parser.parse();
    const toml_value* val = result->as_table().get_member("key");
    ASSERT_NE(val, nullptr);
    EXPECT_TRUE(val->is_datetime());
    EXPECT_EQ(val->as_datetime().get_datetime_type(), toml_datetime::LocalDate);
    EXPECT_EQ(val->as_datetime().get_string_value(), "1979-05-27");
}

TEST_F(TomlParserTest, ParseDatetimeLocalTime) {
    toml_parser parser("key = 07:32:00");
    toml_document result = parser.parse();
    const toml_value* val = result->as_table().get_member("key");
    ASSERT_NE(val, nullptr);
    EXPECT_TRUE(val->is_datetime());
    EXPECT_EQ(val->as_datetime().get_datetime_type(), toml_datetime::LocalTime);
    EXPECT_EQ(val->as_datetime().get_string_value(), "07:32:00");
}

TEST_F(TomlParserTest, ParseComment) {
    toml_parser parser("# This is a comment\nkey = 42");
    toml_document result = parser.parse();
    EXPECT_EQ(result->as_table().get_member("key")->as_integer().get_value(), 42);
}

TEST_F(TomlParserTest, ParseCommentAfterValue) {
    toml_parser parser("key = 42 # comment");
    toml_document result = parser.parse();
    EXPECT_EQ(result->as_table().get_member("key")->as_integer().get_value(), 42);
}

TEST_F(TomlParserTest, ParseFloatSpecialInf) {
    toml_parser parser("key = inf");
    toml_document result = parser.parse();
    const toml_value* val = result->as_table().get_member("key");
    ASSERT_NE(val, nullptr);
    EXPECT_TRUE(val->is_float());
    EXPECT_TRUE(is_infinity(val->as_float().get_value()));
}

TEST_F(TomlParserTest, ParseFloatSpecialNan) {
    toml_parser parser("key = nan");
    toml_document result = parser.parse();
    const toml_value* val = result->as_table().get_member("key");
    ASSERT_NE(val, nullptr);
    EXPECT_TRUE(val->is_float());
    EXPECT_TRUE(is_nan(val->as_float().get_value()));
}

TEST_F(TomlParserTest, ParseFloatSpecialNegInf) {
    toml_parser parser("key = -inf");
    toml_document result = parser.parse();
    const toml_value* val = result->as_table().get_member("key");
    ASSERT_NE(val, nullptr);
    EXPECT_TRUE(val->is_float());
    EXPECT_TRUE(is_infinity(val->as_float().get_value()));
    EXPECT_LT(val->as_float().get_value(), 0);
}

TEST_F(TomlParserTest, ParseComplexDocument) {
    string toml_doc = R"(
        title = "TOML Example"

        [owner]
        name = "Tom Preston-Werner"
        dob = 1979-05-27T07:32:00-08:00

        [database]
        enabled = true
        ports = [8000, 8001, 8002]
        data = [["delta", "phi"], [3.14]]

        [servers]
        [servers.alpha]
        ip = "10.0.0.1"
        role = "frontend"

        [servers.beta]
        ip = "10.0.0.2"
        role = "backend"
    )";

    toml_parser parser(toml_doc);
    toml_document result = parser.parse();

    EXPECT_EQ(result->as_table().get_member("title")->as_string().get_value(), "TOML Example");

    const toml_value* owner = result->as_table().get_member("owner");
    ASSERT_NE(owner, nullptr);
    EXPECT_EQ(owner->as_table().get_member("name")->as_string().get_value(), "Tom Preston-Werner");
    EXPECT_EQ(owner->as_table().get_member("dob")->as_datetime().get_datetime_type(), toml_datetime::OffsetDateTime);

    const toml_value* database = result->as_table().get_member("database");
    ASSERT_NE(database, nullptr);
    EXPECT_TRUE(database->as_table().get_member("enabled")->as_boolean().get_value());
    EXPECT_EQ(database->as_table().get_member("ports")->as_array().size(), 3u);
    EXPECT_EQ(database->as_table().get_member("ports")->as_array().get_element(2)->as_integer().get_value(), 8002);

    const toml_value* servers = result->as_table().get_member("servers");
    ASSERT_NE(servers, nullptr);
    ASSERT_NE(servers->as_table().get_member("alpha"), nullptr);
    ASSERT_NE(servers->as_table().get_member("beta"), nullptr);
}

TEST_F(TomlParserTest, TryParseValid) {
    toml_parser parser("key = 42");
    optional<toml_document> result = parser.try_parse();
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value()->as_table().get_member("key")->as_integer().get_value(), 42);
}

TEST_F(TomlParserTest, TryParseInvalid) {
    toml_parser parser("{ invalid");
    optional<toml_document> result = parser.try_parse();
    EXPECT_FALSE(result.has_value());
}

TEST_F(TomlParserTest, ParseDuplicateKeyThrows) {
    toml_parser parser("key = 1\nkey = 2");
    EXPECT_THROW(parser.parse(), toml_exception);
}

TEST_F(TomlParserTest, ParseMixedArrayTypes) {
    toml_parser parser("key = [1, \"string\", true, 3.14]");
    toml_document result = parser.parse();
    const toml_value* val = result->as_table().get_member("key");
    ASSERT_NE(val, nullptr);
    EXPECT_TRUE(val->is_array());
    EXPECT_EQ(val->as_array().size(), 4u);
    EXPECT_EQ(val->as_array().get_element(0)->as_integer().get_value(), 1);
    EXPECT_EQ(val->as_array().get_element(1)->as_string().get_value(), "string");
    EXPECT_TRUE(val->as_array().get_element(2)->as_boolean().get_value());
    EXPECT_DOUBLE_EQ(val->as_array().get_element(3)->as_float().get_value(), 3.14);
}

TEST_F(TomlParserTest, ParseNestedArrays) {
    toml_parser parser("key = [[1, 2], [\"a\", \"b\"]]");
    toml_document result = parser.parse();
    const toml_value* val = result->as_table().get_member("key");
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(val->as_array().size(), 2u);
    EXPECT_EQ(val->as_array().get_element(0)->as_array().size(), 2u);
    EXPECT_EQ(val->as_array().get_element(1)->as_array().get_element(0)->as_string().get_value(), "a");
}

TEST_F(TomlParserTest, ParseUnterminatedStringThrows) {
    toml_parser parser("key = \"hello");
    EXPECT_THROW(parser.parse(), toml_exception);
}

TEST_F(TomlParserTest, ParseUnterminatedMultilineThrows) {
    toml_parser parser("key = \"\"\"hello");
    EXPECT_THROW(parser.parse(), toml_exception);
}

TEST_F(TomlParserTest, ParseInvalidNumberThrows) {
    toml_parser parser("key = 12.34.56");
    EXPECT_THROW(parser.parse(), toml_exception);
}

TEST_F(TomlParserTest, ParseTrailingContentThrows) {
    toml_parser parser("key = 1 trailing");
    EXPECT_THROW(parser.parse(), toml_exception);
}

TEST_F(TomlParserTest, ParseLeadingZerosIntegerThrows) {
    toml_parser parser("key = 00");
    EXPECT_THROW(parser.parse(), toml_exception);
}

TEST_F(TomlParserTest, ParseLeadingZerosIntegerNegativeThrows) {
    toml_parser parser("key = -01");
    EXPECT_THROW(parser.parse(), toml_exception);
}

TEST_F(TomlParserTest, ParseLeadingZerosIntegerPositiveThrows) {
    toml_parser parser("key = +00");
    EXPECT_THROW(parser.parse(), toml_exception);
}

TEST_F(TomlParserTest, ParseLeadingZerosIntegerMultiDigitThrows) {
    toml_parser parser("key = 0042");
    EXPECT_THROW(parser.parse(), toml_exception);
}

TEST_F(TomlParserTest, ParseZeroIntegerValid) {
    toml_parser parser("key = 0");
    toml_document result = parser.parse();
    EXPECT_EQ(result->as_table().get_member("key")->as_integer().get_value(), 0);
}

TEST_F(TomlParserTest, ParseHexIntegerWithSignThrows) {
    toml_parser parser("key = -0xFF");
    EXPECT_THROW(parser.parse(), toml_exception);
}

TEST_F(TomlParserTest, ParseOctalIntegerWithSignThrows) {
    toml_parser parser("key = +0o77");
    EXPECT_THROW(parser.parse(), toml_exception);
}

TEST_F(TomlParserTest, ParseBinaryIntegerWithSignThrows) {
    toml_parser parser("key = -0b1010");
    EXPECT_THROW(parser.parse(), toml_exception);
}

TEST_F(TomlParserTest, ParseArrayTrailingCommaThrows) {
    toml_parser parser("key = [1, 2, ]");
    EXPECT_THROW(parser.parse(), toml_exception);
}

TEST_F(TomlParserTest, ParseInlineTableTrailingCommaThrows) {
    toml_parser parser("key = { a = 1, }");
    EXPECT_THROW(parser.parse(), toml_exception);
}

TEST_F(TomlParserTest, ParseArrayMissingSeparatorThrows) {
    toml_parser parser("key = [1 2]");
    EXPECT_THROW(parser.parse(), toml_exception);
}

TEST_F(TomlParserTest, ParseInlineTableMissingSeparatorThrows) {
    toml_parser parser("key = { a = 1 b = 2 }");
    EXPECT_THROW(parser.parse(), toml_exception);
}

TEST_F(TomlParserTest, ParseInlineTableNewlineThrows) {
    toml_parser parser("key = { a = 1,\n b = 2 }");
    EXPECT_THROW(parser.parse(), toml_exception);
}

TEST_F(TomlParserTest, ParseUnicodeSurrogateThrows) {
    toml_parser parser("key = \"\\uD800\"");
    EXPECT_THROW(parser.parse(), toml_exception);
}

TEST_F(TomlParserTest, ParseUnicodeSurrogateLowThrows) {
    toml_parser parser("key = \"\\uDFFF\"");
    EXPECT_THROW(parser.parse(), toml_exception);
}

TEST_F(TomlParserTest, ParseUnicodeValidCodepoint) {
    toml_parser parser("key = \"\\u2764\"");
    toml_document result = parser.parse();
    EXPECT_TRUE(result->as_table().get_member("key")->is_string());
    EXPECT_EQ(result->as_table().get_member("key")->as_string().get_value(), "\xE2\x9D\xA4");
}

TEST_F(TomlParserTest, ParseSurrogatePairInMultibasicStringThrows) {
    toml_parser parser("key = \"\"\"\\uD800\"\"\"");
    EXPECT_THROW(parser.parse(), toml_exception);
}

TEST_F(TomlParserTest, ParseBOM) {
    string doc = "\xEF\xBB\xBFkey = 42";
    toml_parser parser(doc);
    toml_document result = parser.parse();
    EXPECT_EQ(result->as_table().get_member("key")->as_integer().get_value(), 42);
}

TEST_F(TomlParserTest, ParseBOMWithTableHeaders) {
    string doc = "\xEF\xBB\xBF[tbl]\nkey = 1";
    toml_parser parser(doc);
    toml_document result = parser.parse();
    const toml_value* tbl = result->as_table().get_member("tbl");
    ASSERT_NE(tbl, nullptr);
    EXPECT_TRUE(tbl->is_table());
    EXPECT_EQ(tbl->as_table().get_member("key")->as_integer().get_value(), 1);
}

TEST_F(TomlParserTest, ParseEmptyInput) {
    toml_parser parser("");
    toml_document result = parser.parse();
    EXPECT_TRUE(result->is_table());
    EXPECT_EQ(result->as_table().size(), 0u);
}

TEST_F(TomlParserTest, ParseOnlyWhitespace) {
    toml_parser parser("   \n   \n  ");
    toml_document result = parser.parse();
    EXPECT_EQ(result->as_table().size(), 0u);
}

TEST_F(TomlParserTest, ParseOnlyComments) {
    toml_parser parser("# comment1\n# comment2");
    toml_document result = parser.parse();
    EXPECT_EQ(result->as_table().size(), 0u);
}

TEST_F(TomlParserTest, ParseCRLF) {
    toml_parser parser("key = 1\r\n[table]\r\nvalue = 2\r\n");
    toml_document result = parser.parse();
    EXPECT_EQ(result->as_table().get_member("key")->as_integer().get_value(), 1);
    EXPECT_EQ(result->as_table().get_member("table")->as_table().get_member("value")->as_integer().get_value(), 2);
}

TEST_F(TomlParserTest, ParseErrorReportsLine) {
    toml_parser parser("key = 1\nkey = 2");
    try {
        parser.parse();
        FAIL() << "expected toml_exception";
    } catch (const toml_exception& e) {
        const string message(e.what());
        EXPECT_NE(message.find("Line 2"), string::npos);
    }
}

TEST_F(TomlParserTest, ParseBorrowKeepsStringPointers) {
    const char* text = "key = \"a borrowed string longer than fourteen bytes\"";
    toml_parser parser{string_view(text)};
    toml_document result = parser.parse();
    const toml_value* val = result->as_table().get_member("key");
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(val->as_string().data(), text + 7);
}

TEST_F(TomlParserTest, ParseOwningMaterializesStrings) {
    string text = "key = \"a borrowed string longer than fourteen bytes\"";
    const char* const source = text.data();
    toml_parser parser(move(text));
    toml_document result = parser.parse();
    const toml_value* val = result->as_table().get_member("key");
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(val->as_string().get_value(), "a borrowed string longer than fourteen bytes");
    EXPECT_NE(val->as_string().data(), source + 7);
}

TEST_F(TomlParserTest, ParseTimeOnlyIsNotNumber) {
    toml_parser parser("time = 12:34:56");
    toml_document result = parser.parse();
    EXPECT_TRUE(result->as_table().get_member("time")->is_datetime());
}

TEST_F(TomlParserTest, ParseNumberIsNotTime) {
    toml_parser parser("value = 123456");
    toml_document result = parser.parse();
    EXPECT_TRUE(result->as_table().get_member("value")->is_integer());
}

class TomlBuilderTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(TomlBuilderTest, BuildBoolean) {
    toml_builder builder;
    builder.key("flag").value(true);
    toml_document result = builder.build();
    const toml_value* val = result->as_table().get_member("flag");
    ASSERT_NE(val, nullptr);
    EXPECT_TRUE(val->is_boolean());
    EXPECT_TRUE(val->as_boolean().get_value());
}

TEST_F(TomlBuilderTest, BuildInteger) {
    toml_builder builder;
    builder.key("count").value(static_cast<int64_t>(42));
    toml_document result = builder.build();
    EXPECT_EQ(result->as_table().get_member("count")->as_integer().get_value(), 42);
}

TEST_F(TomlBuilderTest, BuildIntAsInteger) {
    toml_builder builder;
    builder.key("count").value(42);
    toml_document result = builder.build();
    EXPECT_TRUE(result->as_table().get_member("count")->is_integer());
}

TEST_F(TomlBuilderTest, BuildFloat) {
    toml_builder builder;
    builder.key("pi").value(3.14);
    toml_document result = builder.build();
    EXPECT_DOUBLE_EQ(result->as_table().get_member("pi")->as_float().get_value(), 3.14);
}

TEST_F(TomlBuilderTest, BuildString) {
    toml_builder builder;
    builder.key("name").value(string("John"));
    toml_document result = builder.build();
    EXPECT_EQ(result->as_table().get_member("name")->as_string().get_value(), "John");
}

TEST_F(TomlBuilderTest, BuildCString) {
    toml_builder builder;
    builder.key("name").value("John");
    toml_document result = builder.build();
    EXPECT_EQ(result->as_table().get_member("name")->as_string().get_value(), "John");
}

TEST_F(TomlBuilderTest, BuildStringView) {
    toml_builder builder;
    const string text("a long string value beyond inline capacity");
    builder.key("name").value(string_view(text.data(), text.size()));
    toml_document result = builder.build();
    EXPECT_EQ(result->as_table().get_member("name")->as_string().get_value(), text);
}

TEST_F(TomlBuilderTest, BuildStringLiteral) {
    toml_builder builder;
    builder.key("path").value_string("C:\\Users", toml_string::Literal);
    toml_document result = builder.build();
    const toml_value* val = result->as_table().get_member("path");
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(val->as_string().get_value(), "C:\\Users");
    EXPECT_EQ(val->as_string().get_string_type(), toml_string::Literal);
}

TEST_F(TomlBuilderTest, BuildStringMultiBasic) {
    toml_builder builder;
    builder.key("text").value_string("line1\nline2", toml_string::MultiBasic);
    toml_document result = builder.build();
    const toml_value* val = result->as_table().get_member("text");
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(val->as_string().get_string_type(), toml_string::MultiBasic);
    EXPECT_EQ(val->as_string().get_value(), "line1\nline2");
}

TEST_F(TomlBuilderTest, BuildDatetime) {
    toml_builder builder;
    builder.key("created").value_datetime("1979-05-27T07:32:00Z", toml_datetime::OffsetDateTime);
    toml_document result = builder.build();
    const toml_value* val = result->as_table().get_member("created");
    ASSERT_NE(val, nullptr);
    EXPECT_TRUE(val->is_datetime());
    EXPECT_EQ(val->as_datetime().get_datetime_type(), toml_datetime::OffsetDateTime);
    EXPECT_EQ(val->as_datetime().get_string_value(), "1979-05-27T07:32:00Z");
}

TEST_F(TomlBuilderTest, BuildDatetimeLocalDate) {
    toml_builder builder;
    builder.key("birthday").value_datetime("1979-05-27", toml_datetime::LocalDate);
    toml_document result = builder.build();
    EXPECT_EQ(result->as_table().get_member("birthday")->as_datetime().get_datetime_type(), toml_datetime::LocalDate);
    EXPECT_EQ(result->as_table().get_member("birthday")->as_datetime().get_string_value(), "1979-05-27");
}

TEST_F(TomlBuilderTest, BuildNullAsBoolean) {
    toml_builder builder;
    builder.key("nothing").value(nullptr);
    toml_document result = builder.build();
    EXPECT_TRUE(result->as_table().get_member("nothing")->is_boolean());
}

TEST_F(TomlBuilderTest, BuildArray) {
    toml_builder builder;
    builder.key("numbers").begin_array();
    builder.value(static_cast<int64_t>(1));
    builder.value(static_cast<int64_t>(2));
    builder.value(static_cast<int64_t>(3));
    builder.end_array();
    toml_document result = builder.build();
    const toml_value* val = result->as_table().get_member("numbers");
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(val->as_array().size(), 3u);
    EXPECT_EQ(val->as_array().get_element(2)->as_integer().get_value(), 3);
}

TEST_F(TomlBuilderTest, BuildEmptyArray) {
    toml_builder builder;
    builder.key("empty").begin_array();
    builder.end_array();
    toml_document result = builder.build();
    EXPECT_EQ(result->as_table().get_member("empty")->as_array().size(), 0u);
}

TEST_F(TomlBuilderTest, BuildNestedArray) {
    toml_builder builder;
    builder.key("matrix").begin_array();
    builder.begin_array();
    builder.value(static_cast<int64_t>(1));
    builder.value(static_cast<int64_t>(2));
    builder.end_array();
    builder.begin_array();
    builder.value(static_cast<int64_t>(3));
    builder.value(static_cast<int64_t>(4));
    builder.end_array();
    builder.end_array();
    toml_document result = builder.build();
    const toml_value* val = result->as_table().get_member("matrix");
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(val->as_array().size(), 2u);
    EXPECT_TRUE(val->as_array().get_element(0)->is_array());
    EXPECT_EQ(val->as_array().get_element(1)->as_array().get_element(1)->as_integer().get_value(), 4);
}

TEST_F(TomlBuilderTest, BuildInlineTable) {
    toml_builder builder;
    builder.key("point").begin_inline_table();
    builder.key("x").value(1.0);
    builder.key("y").value(2.0);
    builder.end_inline_table();
    toml_document result = builder.build();
    const toml_value* val = result->as_table().get_member("point");
    ASSERT_NE(val, nullptr);
    EXPECT_TRUE(val->as_table().is_inline());
    EXPECT_DOUBLE_EQ(val->as_table().get_member("x")->as_float().get_value(), 1.0);
    EXPECT_DOUBLE_EQ(val->as_table().get_member("y")->as_float().get_value(), 2.0);
}

TEST_F(TomlBuilderTest, BuildTable) {
    toml_builder builder;
    builder.begin_table("database");
    builder.key("host").value(string("localhost"));
    builder.key("port").value(static_cast<int64_t>(5432));
    builder.end_table();
    toml_document result = builder.build();
    const toml_value* val = result->as_table().get_member("database");
    ASSERT_NE(val, nullptr);
    EXPECT_FALSE(val->as_table().is_inline());
    EXPECT_EQ(val->as_table().get_member("host")->as_string().get_value(), "localhost");
    EXPECT_EQ(val->as_table().get_member("port")->as_integer().get_value(), 5432);
}

TEST_F(TomlBuilderTest, BuildTableWithVectorPath) {
    toml_builder builder;
    vector<string> path = {"parent", "child"};
    builder.begin_table(path);
    builder.key("value").value(static_cast<int64_t>(42));
    builder.end_table();
    toml_document result = builder.build();
    const toml_value* child = result->as_table().get_member("parent")->as_table().get_member("child");
    ASSERT_NE(child, nullptr);
    EXPECT_EQ(child->as_table().get_member("value")->as_integer().get_value(), 42);
}

TEST_F(TomlBuilderTest, BuildArrayTable) {
    toml_builder builder;
    builder.begin_array_table("products");
    builder.key("name").value(string("Hammer"));
    builder.end_array_table();
    builder.begin_array_table("products");
    builder.key("name").value(string("Nail"));
    builder.end_array_table();
    toml_document result = builder.build();
    const toml_value* val = result->as_table().get_member("products");
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(val->as_array().size(), 2u);
    EXPECT_EQ(val->as_array().get_element(0)->as_table().get_member("name")->as_string().get_value(), "Hammer");
    EXPECT_EQ(val->as_array().get_element(1)->as_table().get_member("name")->as_string().get_value(), "Nail");
}

TEST_F(TomlBuilderTest, BuildArrayTableWithVectorPath) {
    toml_builder builder;
    vector<string> path = {"section", "items"};
    builder.begin_array_table(path);
    builder.key("id").value(static_cast<int64_t>(1));
    builder.end_array_table();
    builder.begin_array_table(path);
    builder.key("id").value(static_cast<int64_t>(2));
    builder.end_array_table();
    toml_document result = builder.build();
    const toml_value* items = result->as_table().get_member("section")->as_table().get_member("items");
    ASSERT_NE(items, nullptr);
    EXPECT_EQ(items->as_array().size(), 2u);
}

TEST_F(TomlBuilderTest, BuildValueTableFunction) {
    toml_builder builder;
    builder.key("settings").value_table([](toml_builder& inner) {
        inner.key("enabled").value(true);
        inner.key("timeout").value(static_cast<int64_t>(30));
    });
    toml_document result = builder.build();
    const toml_value* val = result->as_table().get_member("settings");
    ASSERT_NE(val, nullptr);
    EXPECT_TRUE(val->as_table().get_member("enabled")->as_boolean().get_value());
    EXPECT_EQ(val->as_table().get_member("timeout")->as_integer().get_value(), 30);
}

TEST_F(TomlBuilderTest, BuildValueInlineTableFunction) {
    toml_builder builder;
    builder.key("point").value_inline_table([](toml_builder& inner) {
        inner.key("x").value(1.0);
        inner.key("y").value(2.0);
    });
    toml_document result = builder.build();
    const toml_value* val = result->as_table().get_member("point");
    ASSERT_NE(val, nullptr);
    EXPECT_TRUE(val->as_table().is_inline());
    EXPECT_DOUBLE_EQ(val->as_table().get_member("y")->as_float().get_value(), 2.0);
}

TEST_F(TomlBuilderTest, BuildValueArrayFunction) {
    toml_builder builder;
    builder.key("scores").value_array([](toml_builder& inner) {
        inner.value(static_cast<int64_t>(95));
        inner.value(static_cast<int64_t>(87));
        inner.value(static_cast<int64_t>(92));
    });
    toml_document result = builder.build();
    const toml_value* val = result->as_table().get_member("scores");
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(val->as_array().size(), 3u);
    EXPECT_EQ(val->as_array().get_element(1)->as_integer().get_value(), 87);
}

TEST_F(TomlBuilderTest, BuildValueTomlValue) {
    toml_builder builder;
    builder.key("data").value(toml_value::make_integer(42));
    toml_document result = builder.build();
    EXPECT_EQ(result->as_table().get_member("data")->as_integer().get_value(), 42);
}

TEST_F(TomlBuilderTest, BuildValueSmallStringNoAllocation) {
    toml_builder builder;
    builder.key("data").value("inline");
    toml_document result = builder.build();
    EXPECT_EQ(result->as_table().get_member("data")->as_string().get_value(), "inline");
}

TEST_F(TomlBuilderTest, BuildValueIterableArray) {
    toml_builder builder;
    vector<int64_t> vec = {1, 2, 3};
    builder.key("numbers").value_iterable(vec);
    toml_document result = builder.build();
    const toml_value* val = result->as_table().get_member("numbers");
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(val->as_array().size(), 3u);
}

TEST_F(TomlBuilderTest, BuildValueIterableMap) {
    toml_builder builder;
    flat_unordered_map<string, int64_t> map;
    map["a"] = 1;
    map["b"] = 2;
    builder.key("dict").value_iterable(map);
    toml_document result = builder.build();
    const toml_value* val = result->as_table().get_member("dict");
    ASSERT_NE(val, nullptr);
    EXPECT_TRUE(val->is_table());
    EXPECT_TRUE(val->as_table().is_inline());
    EXPECT_EQ(val->as_table().size(), 2u);
}

TEST_F(TomlBuilderTest, BuildComplexDocument) {
    toml_builder builder;
    builder.key("title").value(string("TOML Example"));
    builder.key("enabled").value(true);
    builder.key("count").value(static_cast<int64_t>(42));
    builder.key("ratio").value(0.75);

    builder.key("ports").begin_array();
    builder.value(static_cast<int64_t>(8000));
    builder.value(static_cast<int64_t>(8001));
    builder.end_array();

    builder.begin_table("database");
    builder.key("host").value(string("localhost"));
    builder.key("port").value(static_cast<int64_t>(5432));
    builder.end_table();

    builder.begin_array_table("users");
    builder.key("name").value(string("Alice"));
    builder.end_array_table();
    builder.begin_array_table("users");
    builder.key("name").value(string("Bob"));
    builder.end_array_table();

    toml_document result = builder.build();
    EXPECT_EQ(result->as_table().get_member("title")->as_string().get_value(), "TOML Example");
    EXPECT_TRUE(result->as_table().get_member("enabled")->as_boolean().get_value());
    EXPECT_EQ(result->as_table().get_member("count")->as_integer().get_value(), 42);
    EXPECT_DOUBLE_EQ(result->as_table().get_member("ratio")->as_float().get_value(), 0.75);
    EXPECT_EQ(result->as_table().get_member("ports")->as_array().size(), 2u);
    EXPECT_EQ(result->as_table().get_member("database")->as_table().get_member("port")->as_integer().get_value(), 5432);
    EXPECT_EQ(result->as_table().get_member("users")->as_array().size(), 2u);
}

TEST_F(TomlBuilderTest, BeginTableDiscardsPendingKey) {
    toml_builder builder;
    builder.key("ignored");
    builder.begin_table("table");
    builder.key("value").value(static_cast<int64_t>(1));
    builder.end_table();
    builder.key("real").value(static_cast<int64_t>(2));
    toml_document result = builder.build();
    EXPECT_EQ(result->as_table().get_member("ignored"), nullptr);
    EXPECT_EQ(result->as_table().get_member("real")->as_integer().get_value(), 2);
    EXPECT_EQ(result->as_table().get_member("table")->as_table().get_member("value")->as_integer().get_value(), 1);
}

TEST_F(TomlBuilderTest, BuildWithoutEndThrows) {
    toml_builder builder;
    builder.key("data").begin_array();
    EXPECT_THROW(builder.build(), toml_exception);
}

TEST_F(TomlBuilderTest, BuildKeyOutsideTableThrows) {
    toml_builder builder;
    builder.key("arr").begin_array();
    EXPECT_THROW(builder.key("test"), toml_exception);
}

TEST_F(TomlBuilderTest, BuildValueWithoutKeyThrows) {
    toml_builder builder;
    EXPECT_THROW(builder.value(static_cast<int64_t>(1)), toml_exception);
}

TEST_F(TomlBuilderTest, BuildEndTableWithoutBeginThrows) {
    toml_builder builder;
    EXPECT_THROW(builder.end_table(), toml_exception);
}

TEST_F(TomlBuilderTest, BuildEndArrayWithoutBeginThrows) {
    toml_builder builder;
    EXPECT_THROW(builder.end_array(), toml_exception);
}

TEST_F(TomlBuilderTest, BuildEndInlineTableWithoutBeginThrows) {
    toml_builder builder;
    EXPECT_THROW(builder.end_inline_table(), toml_exception);
}

TEST_F(TomlBuilderTest, BuildEndArrayMismatchTableThrows) {
    toml_builder builder;
    builder.begin_table("test");
    EXPECT_THROW(builder.end_array(), toml_exception);
}

TEST_F(TomlBuilderTest, BuildEndTableMismatchArrayThrows) {
    toml_builder builder;
    builder.key("arr").begin_array();
    EXPECT_THROW(builder.end_table(), toml_exception);
}

TEST_F(TomlBuilderTest, BuildDuplicateKeyThrows) {
    toml_builder builder;
    builder.key("key").value(static_cast<int64_t>(1));
    builder.key("key");
    EXPECT_THROW(builder.value(static_cast<int64_t>(2)), toml_exception);
}

TEST_F(TomlBuilderTest, BuildEmptyArrayTablePathThrows) {
    toml_builder builder;
    vector<string> empty_path;
    EXPECT_THROW(builder.begin_array_table(empty_path), toml_exception);
}

TEST_F(TomlBuilderTest, BuildEmptyTablePathThrows) {
    toml_builder builder;
    vector<string> empty_path;
    EXPECT_THROW(builder.begin_table(empty_path), toml_exception);
}

TEST_F(TomlBuilderTest, BuildRootTableCannotEndThrows) {
    toml_builder builder;
    EXPECT_THROW(builder.end_table(), toml_exception);
}

TEST_F(TomlBuilderTest, BuildRoundTripThroughDocument) {
    toml_builder builder;
    builder.key("title").value(string("Round Trip"));
    builder.begin_table("database");
    builder.key("enabled").value(true);
    builder.key("ports").begin_array();
    builder.value(static_cast<int64_t>(8000));
    builder.value(static_cast<int64_t>(8001));
    builder.end_array();
    builder.end_table();
    toml_document document = builder.build();

    string text = document->to_document();
    toml_document reparsed = toml_parser(string_view(text.data(), text.size())).parse();
    EXPECT_EQ(reparsed->as_table().get_member("title")->as_string().get_value(), "Round Trip");
    EXPECT_TRUE(
            reparsed->as_table().get_member("database")->as_table().get_member("enabled")->as_boolean().get_value());
    EXPECT_EQ(reparsed->as_table().get_member("database")->as_table().get_member("ports")->as_array().size(), 2u);
}
TEST_F(TomlValueTest, WrongKindViewsAreEmpty) {
    toml_document doc;
    const toml_value integer = toml_value::make_integer(7);
    EXPECT_EQ(integer.as_array().size(), 0u);
    EXPECT_TRUE(integer.as_table().empty());
    EXPECT_EQ(integer.as_table().get_member("x"), nullptr);

    const toml_value table = doc.new_table().to_value();
    EXPECT_EQ(table.as_array().size(), 0u);
    EXPECT_EQ(table.as_table().size(), 0u);
}

TEST_F(TomlValueTest, SpecialFloatRoundTrip) {
    toml_document doc;
    const double values[] = {to_float64("inf"), to_float64("-inf"), to_float64("nan")};
    const char* const names[] = {"positive", "negative", "nan"};
    toml_table_mut table = doc.make_table();
    for (size_t i = 0; i < 3; ++i) {
        table.add_member(names[i], toml_value::make_float(values[i]));
    }
    EXPECT_EQ(doc->to_document(), "positive = inf\nnegative = -inf\nnan = nan\n");

    const string text = doc->to_document();
    const toml_document parsed = toml_parser{string_view(text.data(), text.size())}.parse();
    const toml_table parsed_table = parsed->as_table();
    EXPECT_TRUE(is_infinity(parsed_table.get_member("positive")->as_float().get_value()));
    EXPECT_TRUE(is_infinity(parsed_table.get_member("negative")->as_float().get_value()));
    EXPECT_LT(parsed_table.get_member("negative")->as_float().get_value(), 0.0);
    EXPECT_TRUE(is_nan(parsed_table.get_member("nan")->as_float().get_value()));
}

TEST_F(TomlValueTest, InlineRootTable) {
    toml_document doc;
    doc.make_table(true);
    doc.table_append(doc.root(), "key", toml_value::make_integer(1));
    EXPECT_TRUE(doc->as_table().is_inline());
    EXPECT_EQ(doc->to_string(), "{ key = 1 }");
    EXPECT_EQ(doc->to_document(), "{ key = 1 }");
}

TEST_F(TomlValueTest, DateOnlyTableSerialization) {
    toml_document doc;
    toml_table_mut table = doc.make_table();
    table.add_member("dob", doc.make_datetime_value("1979-05-27", toml_local_date));
    table.add_member("time", doc.make_datetime_value("07:32:00", toml_local_time));
    EXPECT_EQ(doc->to_document(), "dob = 1979-05-27\ntime = 07:32:00\n");
}

class YamlParserTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(YamlParserTest, ParseNullTilde) {
    yaml_parser parser("~");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_null());
}

TEST_F(YamlParserTest, ParseNullLowercase) {
    yaml_parser parser("null");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_null());
}

TEST_F(YamlParserTest, ParseNullCapitalized) {
    yaml_parser parser("Null");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_null());
}

TEST_F(YamlParserTest, ParseNullUppercase) {
    yaml_parser parser("NULL");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_null());
}

TEST_F(YamlParserTest, ParseBooleanTrueSynonyms) {
    const vector<string> synonyms = {"true", "True", "TRUE", "y", "Y", "yes", "Yes", "YES", "on", "On", "ON"};
    for (const auto& syn: synonyms) {
        yaml_parser parser(syn);
        auto result = parser.parse();
        ASSERT_NE(result, nullptr) << "Failed for: " << syn.data();
        EXPECT_TRUE(result->is_boolean()) << "Expected boolean for: " << syn.data();
        EXPECT_TRUE(result->as_boolean()->get_value()) << "Expected true for: " << syn.data();
    }
}

TEST_F(YamlParserTest, ParseBooleanFalseSynonyms) {
    const vector<string> synonyms = {"false", "False", "FALSE", "n", "N", "no", "No", "NO", "off", "Off", "OFF"};
    for (const auto& syn: synonyms) {
        yaml_parser parser(syn);
        auto result = parser.parse();
        ASSERT_NE(result, nullptr) << "Failed for: " << syn.data();
        EXPECT_TRUE(result->is_boolean()) << "Expected boolean for: " << syn.data();
        EXPECT_FALSE(result->as_boolean()->get_value()) << "Expected false for: " << syn.data();
    }
}

TEST_F(YamlParserTest, ParseBooleanInMapping) {
    yaml_parser parser("key: true\nflag: no\nenabled: on");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_mapping());
    auto map = result->as_mapping();
    EXPECT_TRUE(map->get_member("key")->as_boolean()->get_value());
    EXPECT_FALSE(map->get_member("flag")->as_boolean()->get_value());
    EXPECT_TRUE(map->get_member("enabled")->as_boolean()->get_value());
}

TEST_F(YamlParserTest, ParseIntegerDecimal) {
    yaml_parser parser("42");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_integer());
    EXPECT_EQ(result->as_integer()->get_value(), 42);
}

TEST_F(YamlParserTest, ParseIntegerNegative) {
    yaml_parser parser("-17");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_integer());
    EXPECT_EQ(result->as_integer()->get_value(), -17);
}

TEST_F(YamlParserTest, ParseIntegerPositive) {
    yaml_parser parser("+99");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_integer());
    EXPECT_EQ(result->as_integer()->get_value(), 99);
}

TEST_F(YamlParserTest, ParseIntegerHex) {
    yaml_parser parser("0xFF");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_integer());
    EXPECT_EQ(result->as_integer()->get_value(), 255);
}

TEST_F(YamlParserTest, ParseIntegerOctal) {
    yaml_parser parser("0o77");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_integer());
    EXPECT_EQ(result->as_integer()->get_value(), 63);
}

TEST_F(YamlParserTest, ParseIntegerBinary) {
    yaml_parser parser("0b1010");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_integer());
    EXPECT_EQ(result->as_integer()->get_value(), 10);
}

TEST_F(YamlParserTest, ParseIntegerUnderscore) {
    yaml_parser parser("1_000_000");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_integer());
    EXPECT_EQ(result->as_integer()->get_value(), 1000000);
}

TEST_F(YamlParserTest, ParseIntegerZero) {
    yaml_parser parser("0");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_integer());
    EXPECT_EQ(result->as_integer()->get_value(), 0);
}

TEST_F(YamlParserTest, ParseFloatSimple) {
    yaml_parser parser("3.14");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_float());
    EXPECT_DOUBLE_EQ(result->as_float()->get_value(), 3.14);
}

TEST_F(YamlParserTest, ParseFloatNegative) {
    yaml_parser parser("-0.5");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_float());
    EXPECT_DOUBLE_EQ(result->as_float()->get_value(), -0.5);
}

TEST_F(YamlParserTest, ParseFloatScientificUpper) {
    yaml_parser parser("1.5E3");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_float());
    EXPECT_DOUBLE_EQ(result->as_float()->get_value(), 1500.0);
}

TEST_F(YamlParserTest, ParseFloatScientificLower) {
    yaml_parser parser("2.5e-2");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_float());
    EXPECT_DOUBLE_EQ(result->as_float()->get_value(), 0.025);
}

TEST_F(YamlParserTest, ParseFloatInfinity) {
    yaml_parser parser(".inf");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_float());
    EXPECT_TRUE(is_infinity(result->as_float()->get_value()));
    EXPECT_GT(result->as_float()->get_value(), 0);
}

TEST_F(YamlParserTest, ParseFloatNegativeInfinity) {
    yaml_parser parser("-.inf");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_float());
    EXPECT_TRUE(is_infinity(result->as_float()->get_value()));
    EXPECT_LT(result->as_float()->get_value(), 0);
}

TEST_F(YamlParserTest, ParseFloatNaN) {
    yaml_parser parser(".nan");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_float());
    EXPECT_TRUE(is_nan(result->as_float()->get_value()));
}

TEST_F(YamlParserTest, ParseFloatInfinityLong) {
    yaml_parser parser(".infinity");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_float());
    EXPECT_TRUE(is_infinity(result->as_float()->get_value()));
}

TEST_F(YamlParserTest, ParseStringPlain) {
    yaml_parser parser("hello world");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_string());
    EXPECT_EQ(result->as_string()->get_value(), "hello world");
}

TEST_F(YamlParserTest, ParseStringSingleQuoted) {
    yaml_parser parser("'hello world'");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_string());
    EXPECT_EQ(result->as_string()->get_value(), "hello world");
    EXPECT_EQ(result->as_string()->get_style(), yaml_string::SingleQuoted);
}

TEST_F(YamlParserTest, ParseStringSingleQuotedWithEscapedQuote) {
    yaml_parser parser("'it''s fine'");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_string());
    EXPECT_EQ(result->as_string()->get_value(), "it's fine");
}

TEST_F(YamlParserTest, ParseStringDoubleQuoted) {
    yaml_parser parser("\"hello world\"");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_string());
    EXPECT_EQ(result->as_string()->get_value(), "hello world");
    EXPECT_EQ(result->as_string()->get_style(), yaml_string::DoubleQuoted);
}

TEST_F(YamlParserTest, ParseStringDoubleQuotedWithEscapes) {
    yaml_parser parser("\"hello\\nworld\"");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_string());
    EXPECT_EQ(result->as_string()->get_value(), "hello\nworld");
}

TEST_F(YamlParserTest, ParseStringDoubleQuotedTabEscape) {
    yaml_parser parser("\"col1\\tcol2\"");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_string());
    EXPECT_EQ(result->as_string()->get_value(), "col1\tcol2");
}

TEST_F(YamlParserTest, ParseStringDoubleQuotedUnicodeEscape) {
    yaml_parser parser("\"\\u0041\\u0042\\u0043\"");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_string());
    EXPECT_EQ(result->as_string()->get_value(), "ABC");
}

TEST_F(YamlParserTest, ParseStringLiteral) {
    yaml_parser parser("|\n  line1\n  line2\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_string());
    EXPECT_EQ(result->as_string()->get_value(), "line1\nline2");
    EXPECT_EQ(result->as_string()->get_style(), yaml_string::Literal);
}

TEST_F(YamlParserTest, ParseStringLiteralStrip) {
    yaml_parser parser("|-\n  line1\n  line2\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_string());
    EXPECT_EQ(result->as_string()->get_value(), "line1\nline2");
}

TEST_F(YamlParserTest, ParseStringLiteralKeep) {
    yaml_parser parser("|+\n  line1\n  line2\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_string());
    EXPECT_EQ(result->as_string()->get_value(), "line1\nline2\n");
}

TEST_F(YamlParserTest, ParseStringFolded) {
    yaml_parser parser(">\n  line1\n  line2\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_string());
    EXPECT_EQ(result->as_string()->get_style(), yaml_string::Folded);
}

TEST_F(YamlParserTest, ParseStringLiteralWithExplicitIndent) {
    yaml_parser parser("|2\n   line1\n   line2\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_string());
    EXPECT_EQ(result->as_string()->get_value(), " line1\n line2");
}

TEST_F(YamlParserTest, ParseStringPlainNotBoolean) {
    yaml_parser parser("yes_sir");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_string());
    EXPECT_EQ(result->as_string()->get_value(), "yes_sir");
}

TEST_F(YamlParserTest, ParseStringPlainNotBooleanN) {
    yaml_parser parser("nobody");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_string());
    EXPECT_EQ(result->as_string()->get_value(), "nobody");
}

TEST_F(YamlParserTest, ParseSequenceBlock) {
    yaml_parser parser("- one\n- two\n- three\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_sequence());
    auto seq = result->as_sequence();
    EXPECT_EQ(seq->size(), 3);
    EXPECT_EQ(seq->get_element(0)->as_string()->get_value(), "one");
    EXPECT_EQ(seq->get_element(1)->as_string()->get_value(), "two");
    EXPECT_EQ(seq->get_element(2)->as_string()->get_value(), "three");
    EXPECT_EQ(seq->get_style(), yaml_sequence::Block);
}

TEST_F(YamlParserTest, ParseSequenceFlow) {
    yaml_parser parser("[one, two, three]");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_sequence());
    auto seq = result->as_sequence();
    EXPECT_EQ(seq->size(), 3);
    EXPECT_EQ(seq->get_element(0)->as_string()->get_value(), "one");
    EXPECT_EQ(seq->get_element(1)->as_string()->get_value(), "two");
    EXPECT_EQ(seq->get_element(2)->as_string()->get_value(), "three");
    EXPECT_EQ(seq->get_style(), yaml_sequence::Flow);
}

TEST_F(YamlParserTest, ParseSequenceEmptyBlock) {
    yaml_parser parser("[]");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_sequence());
    EXPECT_EQ(result->as_sequence()->size(), 0);
}

TEST_F(YamlParserTest, ParseSequenceMixedTypes) {
    yaml_parser parser("- 42\n- true\n- hello\n- 3.14\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_sequence());
    auto seq = result->as_sequence();
    EXPECT_EQ(seq->size(), 4);
    EXPECT_TRUE(seq->get_element(0)->is_integer());
    EXPECT_TRUE(seq->get_element(1)->is_boolean());
    EXPECT_TRUE(seq->get_element(2)->is_string());
    EXPECT_TRUE(seq->get_element(3)->is_float());
}

TEST_F(YamlParserTest, ParseSequenceFlowMixedTypes) {
    yaml_parser parser("[42, true, hello, 3.14]");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_sequence());
    auto seq = result->as_sequence();
    EXPECT_EQ(seq->size(), 4);
    EXPECT_TRUE(seq->get_element(0)->is_integer());
    EXPECT_TRUE(seq->get_element(1)->is_boolean());
    EXPECT_TRUE(seq->get_element(2)->is_string());
    EXPECT_TRUE(seq->get_element(3)->is_float());
}

TEST_F(YamlParserTest, ParseMappingBlock) {
    yaml_parser parser("name: John\nage: 30\nactive: true\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_mapping());
    auto map = result->as_mapping();
    EXPECT_EQ(map->get_member("name")->as_string()->get_value(), "John");
    EXPECT_EQ(map->get_member("age")->as_integer()->get_value(), 30);
    EXPECT_TRUE(map->get_member("active")->as_boolean()->get_value());
    EXPECT_EQ(map->get_style(), yaml_mapping::Block);
}

TEST_F(YamlParserTest, ParseMappingFlow) {
    yaml_parser parser("{name: John, age: 30, active: true}");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_mapping());
    auto map = result->as_mapping();
    EXPECT_EQ(map->get_member("name")->as_string()->get_value(), "John");
    EXPECT_EQ(map->get_member("age")->as_integer()->get_value(), 30);
    EXPECT_TRUE(map->get_member("active")->as_boolean()->get_value());
    EXPECT_EQ(map->get_style(), yaml_mapping::Flow);
}

TEST_F(YamlParserTest, ParseMappingEmptyBlock) {
    yaml_parser parser("{}");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_mapping());
    EXPECT_TRUE(result->as_mapping()->get_members().empty());
}

TEST_F(YamlParserTest, ParseMappingQuotedKeys) {
    yaml_parser parser("\"first name\": John\n'last name': Doe\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_mapping());
    auto map = result->as_mapping();
    EXPECT_EQ(map->get_member("first name")->as_string()->get_value(), "John");
    EXPECT_EQ(map->get_member("last name")->as_string()->get_value(), "Doe");
}

TEST_F(YamlParserTest, ParseMappingMergeKey) {
    yaml_parser parser("_base: &base\n  x: 1\n  y: 2\nderived:\n  <<: *base\n  z: 3\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_mapping());
    auto* derived = result->as_mapping()->get_member("derived");
    ASSERT_NE(derived, nullptr);
    ASSERT_TRUE(derived->is_mapping());
    auto dm = derived->as_mapping();
    EXPECT_EQ(dm->get_member("x")->as_integer()->get_value(), 1);
    EXPECT_EQ(dm->get_member("y")->as_integer()->get_value(), 2);
    EXPECT_EQ(dm->get_member("z")->as_integer()->get_value(), 3);
}

TEST_F(YamlParserTest, ParseMappingMergeKeySequence) {
    yaml_parser parser("_a: &a\n  x: 1\n_b: &b\n  y: 2\nmerged:\n  <<: [*a, *b]\n  z: 3\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_mapping());
    auto* merged = result->as_mapping()->get_member("merged");
    ASSERT_NE(merged, nullptr);
    ASSERT_TRUE(merged->is_mapping());
    auto mm = merged->as_mapping();
    EXPECT_EQ(mm->get_member("x")->as_integer()->get_value(), 1);
    EXPECT_EQ(mm->get_member("y")->as_integer()->get_value(), 2);
    EXPECT_EQ(mm->get_member("z")->as_integer()->get_value(), 3);
}

TEST_F(YamlParserTest, ParseNestedSequenceInMapping) {
    yaml_parser parser("items:\n  - one\n  - two\n  - three\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_mapping());
    auto* items = result->as_mapping()->get_member("items");
    ASSERT_NE(items, nullptr);
    ASSERT_TRUE(items->is_sequence());
    EXPECT_EQ(items->as_sequence()->size(), 3);
}

TEST_F(YamlParserTest, ParseNestedMappingInSequence) {
    yaml_parser parser("- name: Alice\n  age: 30\n- name: Bob\n  age: 25\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_sequence());
    auto seq = result->as_sequence();
    EXPECT_EQ(seq->size(), 2);
    EXPECT_EQ(seq->get_element(0)->as_mapping()->get_member("name")->as_string()->get_value(), "Alice");
    EXPECT_EQ(seq->get_element(1)->as_mapping()->get_member("name")->as_string()->get_value(), "Bob");
}

TEST_F(YamlParserTest, ParseNestedFlowInBlock) {
    yaml_parser parser("matrix:\n  - [1, 2, 3]\n  - [4, 5, 6]\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_mapping());
    auto* matrix = result->as_mapping()->get_member("matrix");
    ASSERT_NE(matrix, nullptr);
    ASSERT_TRUE(matrix->is_sequence());
    EXPECT_EQ(matrix->as_sequence()->size(), 2);
    EXPECT_TRUE(matrix->as_sequence()->get_element(0)->is_sequence());
    EXPECT_EQ(matrix->as_sequence()->get_element(0)->as_sequence()->get_style(), yaml_sequence::Flow);
}

TEST_F(YamlParserTest, ParseAnchorAndAlias) {
    yaml_parser parser("original: &anchor value\ncopy: *anchor\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_mapping());
    auto map = result->as_mapping();
    EXPECT_EQ(map->get_member("original")->as_string()->get_value(), "value");
    EXPECT_EQ(map->get_member("copy")->as_string()->get_value(), "value");
}

TEST_F(YamlParserTest, ParseAnchorOnSequence) {
    yaml_parser parser("list: &mylist\n  - a\n  - b\nother: *mylist\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_mapping());
    auto* other = result->as_mapping()->get_member("other");
    ASSERT_NE(other, nullptr);
    ASSERT_TRUE(other->is_sequence());
    EXPECT_EQ(other->as_sequence()->size(), 2);
}

TEST_F(YamlParserTest, ParseAnchorOnMapping) {
    yaml_parser parser("defaults: &def\n  color: red\n  size: large\nwidget:\n  <<: *def\n  shape: round\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_mapping());
    auto* widget = result->as_mapping()->get_member("widget");
    ASSERT_NE(widget, nullptr);
    ASSERT_TRUE(widget->is_mapping());
    EXPECT_EQ(widget->as_mapping()->get_member("color")->as_string()->get_value(), "red");
    EXPECT_EQ(widget->as_mapping()->get_member("size")->as_string()->get_value(), "large");
    EXPECT_EQ(widget->as_mapping()->get_member("shape")->as_string()->get_value(), "round");
}

TEST_F(YamlParserTest, ParseTagLocal) {
    yaml_parser parser("!myscalar value");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->get_tag(), "myscalar");
    EXPECT_TRUE(result->is_string());
}

TEST_F(YamlParserTest, ParseTagGlobal) {
    yaml_parser parser("!!str value");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_FALSE(result->get_tag().empty());
    EXPECT_TRUE(result->is_string());
}

TEST_F(YamlParserTest, ParseTagVerbatim) {
    yaml_parser parser("!<tag:example.com,2024:mytype> value");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->get_tag(), "tag:example.com,2024:mytype");
}

TEST_F(YamlParserTest, ParseCommentInline) {
    yaml_parser parser("key: value # this is a comment\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_mapping());
    EXPECT_EQ(result->as_mapping()->get_member("key")->as_string()->get_value(), "value");
}

TEST_F(YamlParserTest, ParseCommentLine) {
    yaml_parser parser("# this is a comment\nkey: value\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_mapping());
    EXPECT_EQ(result->as_mapping()->get_member("key")->as_string()->get_value(), "value");
}

TEST_F(YamlParserTest, ParseCommentInSequence) {
    yaml_parser parser("# header\nitems:\n  - one  # first\n  - two  # second\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_mapping());
    auto* items = result->as_mapping()->get_member("items");
    ASSERT_NE(items, nullptr);
    ASSERT_TRUE(items->is_sequence());
    EXPECT_EQ(items->as_sequence()->size(), 2);
}

TEST_F(YamlParserTest, ParseDocumentWithSeparators) {
    yaml_parser parser("---\nkey: value\n...\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_mapping());
    EXPECT_EQ(result->as_mapping()->get_member("key")->as_string()->get_value(), "value");
}

TEST_F(YamlParserTest, ParseMultipleDocuments) {
    yaml_parser parser("---\nfirst: doc\n---\nsecond: doc\n");
    auto docs = parser.parse_documents();
    ASSERT_EQ(docs.size(), 2);
    ASSERT_TRUE(docs[0]->is_mapping());
    EXPECT_TRUE(docs[0]->as_mapping()->has_member("first"));
    ASSERT_TRUE(docs[1]->is_mapping());
    EXPECT_TRUE(docs[1]->as_mapping()->has_member("second"));
}

TEST_F(YamlParserTest, ParseMultipleDocumentsWithEndMarker) {
    yaml_parser parser("---\nfirst: doc\n...\n---\nsecond: doc\n...\n");
    auto docs = parser.parse_documents();
    ASSERT_EQ(docs.size(), 2);
    EXPECT_TRUE(docs[0]->as_mapping()->has_member("first"));
    EXPECT_TRUE(docs[1]->as_mapping()->has_member("second"));
}

TEST_F(YamlParserTest, ParseMultipleDocumentsWithDirectives) {
    yaml_parser parser("%YAML 1.2\n---\nfirst: doc\n%TAG !e! tag:example.com:\n---\nsecond: doc\n");
    auto docs = parser.parse_documents();
    ASSERT_EQ(docs.size(), 2);
    EXPECT_TRUE(docs[0]->as_mapping()->has_member("first"));
    EXPECT_TRUE(docs[1]->as_mapping()->has_member("second"));
}

TEST_F(YamlParserTest, ParseComplexDocument) {
    string yaml = R"(---
name: MyProject
version: 2
enabled: true
config:
  host: localhost
  port: 8080
  features:
    - logging
    - caching
    - auth
tags:
  - name: stable
    value: yes
  - name: testing
    value: no
)";
    yaml_parser parser(yaml);
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_mapping());
    auto root = result->as_mapping();

    EXPECT_EQ(root->get_member("name")->as_string()->get_value(), "MyProject");
    EXPECT_EQ(root->get_member("version")->as_integer()->get_value(), 2);
    EXPECT_TRUE(root->get_member("enabled")->as_boolean()->get_value());

    auto* config = root->get_member("config");
    ASSERT_NE(config, nullptr);
    ASSERT_TRUE(config->is_mapping());
    EXPECT_EQ(config->as_mapping()->get_member("host")->as_string()->get_value(), "localhost");
    EXPECT_EQ(config->as_mapping()->get_member("port")->as_integer()->get_value(), 8080);

    auto* features = config->as_mapping()->get_member("features");
    ASSERT_NE(features, nullptr);
    ASSERT_TRUE(features->is_sequence());
    EXPECT_EQ(features->as_sequence()->size(), 3);

    auto* tags = root->get_member("tags");
    ASSERT_NE(tags, nullptr);
    ASSERT_TRUE(tags->is_sequence());
    EXPECT_EQ(tags->as_sequence()->size(), 2);
    EXPECT_EQ(tags->as_sequence()->get_element(0)->as_mapping()->get_member("name")->as_string()->get_value(),
              "stable");
    EXPECT_TRUE(tags->as_sequence()->get_element(0)->as_mapping()->get_member("value")->as_boolean()->get_value());
    EXPECT_FALSE(tags->as_sequence()->get_element(1)->as_mapping()->get_member("value")->as_boolean()->get_value());
}

TEST_F(YamlParserTest, ParseBOM) {
    string yaml = "\xEF\xBB\xBF"
                  "key: value\n";
    yaml_parser parser(yaml);
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_mapping());
    EXPECT_EQ(result->as_mapping()->get_member("key")->as_string()->get_value(), "value");
}

TEST_F(YamlParserTest, ParseDeeplyNested) {
    string yaml = R"(a:
  b:
    c:
      d:
        e: deep value
)";
    yaml_parser parser(yaml);
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    auto* level = result->as_mapping()
                          ->get_member("a")
                          ->as_mapping()
                          ->get_member("b")
                          ->as_mapping()
                          ->get_member("c")
                          ->as_mapping()
                          ->get_member("d")
                          ->as_mapping()
                          ->get_member("e");
    ASSERT_NE(level, nullptr);
    EXPECT_EQ(level->as_string()->get_value(), "deep value");
}

TEST_F(YamlParserTest, ParseEmptyDocument) {
    yaml_parser parser("");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_null());
}

TEST_F(YamlParserTest, ParseOnlyComments) {
    yaml_parser parser("# just a comment\n# another comment\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_null());
}

TEST_F(YamlParserTest, ParseExitWithDocumentEndMarker) {
    yaml_parser parser("...\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_null());
}

TEST_F(YamlParserTest, ParseTabInContent) {
    yaml_parser parser("\"hello\tworld\"");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_string());
}

TEST_F(YamlParserTest, ParseTabIndentationThrows) {
    yaml_parser parser("\tkey: value\n");
    EXPECT_THROW(ignore = parser.parse(), yaml_exception);
}

TEST_F(YamlParserTest, ParseDuplicateAnchorThrows) {
    yaml_parser parser("a: &dup 1\nb: &dup 2\n");
    EXPECT_THROW(ignore = parser.parse(), yaml_exception);
}

TEST_F(YamlParserTest, ParseUndefinedAliasThrows) {
    yaml_parser parser("a: *undefined\n");
    EXPECT_THROW(ignore = parser.parse(), yaml_exception);
}

TEST_F(YamlParserTest, ParseTryParseValid) {
    yaml_parser parser("key: value\n");
    auto result = parser.try_parse();
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value()->is_mapping());
}

TEST_F(YamlParserTest, ParseTryParseInvalid) {
    yaml_parser parser("{key: value");
    auto result = parser.try_parse();
    EXPECT_FALSE(result.has_value());
}

TEST_F(YamlParserTest, ParseTryParseDocumentsValid) {
    yaml_parser parser("---\nfirst: doc\n---\nsecond: doc\n");
    auto docs = parser.try_parse_documents();
    ASSERT_TRUE(docs.has_value());
    EXPECT_EQ(docs.value().size(), 2);
}

TEST_F(YamlParserTest, ParseTryParseDocumentsInvalid) {
    yaml_parser parser("{key: value");
    auto docs = parser.try_parse_documents();
    EXPECT_FALSE(docs.has_value());
}

TEST_F(YamlParserTest, ParseIntegerLargeValue) {
    yaml_parser parser("9223372036854775807");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_integer());
    EXPECT_EQ(result->as_integer()->get_value(), 9223372036854775807LL);
}

TEST_F(YamlParserTest, ParseIntegerNegativeLargeValue) {
    yaml_parser parser("-9223372036854775808");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_integer());
}

TEST_F(YamlParserTest, ParseSequenceWithNullEntry) {
    yaml_parser parser("- item1\n-\n- item3\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_sequence());
    auto seq = result->as_sequence();
    EXPECT_EQ(seq->size(), 3);
    EXPECT_TRUE(seq->get_element(0)->is_string());
    EXPECT_TRUE(seq->get_element(1)->is_null());
    EXPECT_TRUE(seq->get_element(2)->is_string());
}

TEST_F(YamlParserTest, ParseMappingWithNullValue) {
    yaml_parser parser("key:\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_mapping());
    EXPECT_TRUE(result->as_mapping()->get_member("key")->is_null());
}

TEST_F(YamlParserTest, ParseMappingWithNullKeyword) {
    yaml_parser parser("key: null\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_mapping());
    EXPECT_TRUE(result->as_mapping()->get_member("key")->is_null());
}

TEST_F(YamlParserTest, ParseSequenceWithCommentAtEnd) {
    yaml_parser parser("- one\n- two\n# trailing comment\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_sequence());
    EXPECT_EQ(result->as_sequence()->size(), 2);
}

TEST_F(YamlParserTest, ParseNullTildeInMapping) {
    yaml_parser parser("key: ~\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_mapping());
    EXPECT_TRUE(result->as_mapping()->get_member("key")->is_null());
}

TEST_F(YamlParserTest, ParseFloatIntegerLike) {
    yaml_parser parser("1.0");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_float());
    EXPECT_DOUBLE_EQ(result->as_float()->get_value(), 1.0);
}

TEST_F(YamlParserTest, ParseBooleanInFlowSequence) {
    yaml_parser parser("[true, false, yes, no, on, off]");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_sequence());
    auto seq = result->as_sequence();
    EXPECT_EQ(seq->size(), 6);
    EXPECT_TRUE(seq->get_element(0)->as_boolean()->get_value());
    EXPECT_FALSE(seq->get_element(1)->as_boolean()->get_value());
    EXPECT_TRUE(seq->get_element(2)->as_boolean()->get_value());
    EXPECT_FALSE(seq->get_element(3)->as_boolean()->get_value());
    EXPECT_TRUE(seq->get_element(4)->as_boolean()->get_value());
    EXPECT_FALSE(seq->get_element(5)->as_boolean()->get_value());
}

TEST_F(YamlParserTest, ParseTagsInSequence) {
    yaml_parser parser("- !str 123\n- !!int 456\n- value\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_sequence());
    auto seq = result->as_sequence();
    EXPECT_EQ(seq->size(), 3);
    EXPECT_EQ(seq->get_element(0)->get_tag(), "str");
    EXPECT_FALSE(seq->get_element(1)->get_tag().empty());
    EXPECT_TRUE(seq->get_element(2)->get_tag().empty());
}

TEST_F(YamlParserTest, ParseAnchorWithTag) {
    yaml_parser parser("key: &anchor !mytag value\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_mapping());
    auto* val = result->as_mapping()->get_member("key");
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(val->get_anchor(), "anchor");
    EXPECT_EQ(val->get_tag(), "mytag");
    EXPECT_EQ(val->as_string()->get_value(), "value");
}

TEST_F(YamlParserTest, ParseBlockScalarIndentHeader) {
    yaml_parser parser("|2+\n   line1\n   line2\n\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_string());
    EXPECT_EQ(result->as_string()->get_value(), " line1\n line2\n\n");
}

TEST_F(YamlParserTest, ParseBlockScalarChompingFirst) {
    yaml_parser parser("|-2\n   line1\n   line2\n\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_string());
    EXPECT_EQ(result->as_string()->get_value(), " line1\n line2");
}

TEST_F(YamlParserTest, ParseNullInFlowSequence) {
    yaml_parser parser("[null, ~, Null, NULL]");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_sequence());
    auto seq = result->as_sequence();
    EXPECT_EQ(seq->size(), 4);
    EXPECT_TRUE(seq->get_element(0)->is_null());
    EXPECT_TRUE(seq->get_element(1)->is_null());
    EXPECT_TRUE(seq->get_element(2)->is_null());
    EXPECT_TRUE(seq->get_element(3)->is_null());
}

TEST_F(YamlParserTest, ParseStringDoubleQuotedBackslashEscape) {
    yaml_parser parser("\"path\\\\to\\\\file\"");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_string());
    EXPECT_EQ(result->as_string()->get_value(), "path\\to\\file");
}

TEST_F(YamlParserTest, ParseStringDoubleQuotedBellEscape) {
    yaml_parser parser("\"bell\\ahere\"");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_string());
    EXPECT_EQ(result->as_string()->get_value(), "bell\ahere");
}

TEST_F(YamlParserTest, ParseStringDoubleQuotedHexEscape) {
    yaml_parser parser("\"\\x41\\x42\"");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_string());
    EXPECT_EQ(result->as_string()->get_value(), "AB");
}

TEST_F(YamlParserTest, ParseFlowSequenceWithNewlines) {
    yaml_parser parser("[\n  one,\n  two,\n  three\n]\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_sequence());
    EXPECT_EQ(result->as_sequence()->size(), 3);
}

TEST_F(YamlParserTest, ParseFlowMappingWithNewlines) {
    yaml_parser parser("{\n  key1: value1,\n  key2: value2\n}\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_mapping());
    EXPECT_EQ(result->as_mapping()->get_member("key1")->as_string()->get_value(), "value1");
    EXPECT_EQ(result->as_mapping()->get_member("key2")->as_string()->get_value(), "value2");
}

TEST_F(YamlParserTest, ParseMappingSameLineValue) {
    yaml_parser parser("key: value\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_mapping());
    EXPECT_EQ(result->as_mapping()->get_member("key")->as_string()->get_value(), "value");
}

TEST_F(YamlParserTest, ParseBooleanYAndNInMapping) {
    yaml_parser parser("a: y\nb: n\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_mapping());
    EXPECT_TRUE(result->as_mapping()->get_member("a")->as_boolean()->get_value());
    EXPECT_FALSE(result->as_mapping()->get_member("b")->as_boolean()->get_value());
}

TEST_F(YamlParserTest, ParseSequenceBlockNestedFlow) {
    yaml_parser parser("- [a, b]\n- [c, d]\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_sequence());
    EXPECT_EQ(result->as_sequence()->size(), 2);
    EXPECT_TRUE(result->as_sequence()->get_element(0)->is_sequence());
    EXPECT_TRUE(result->as_sequence()->get_element(1)->is_sequence());
}

TEST_F(YamlParserTest, ToStringRoundTripNull) {
    yaml_parser parser("null");
    auto result = parser.parse();
    EXPECT_EQ(result->to_string(), "null");
}

TEST_F(YamlParserTest, ToStringRoundTripBoolean) {
    yaml_parser parser("true");
    auto result = parser.parse();
    EXPECT_EQ(result->to_string(), "true");
}

TEST_F(YamlParserTest, ToStringRoundTripInteger) {
    yaml_parser parser("42");
    auto result = parser.parse();
    EXPECT_EQ(result->to_string(), "42");
}

TEST_F(YamlParserTest, ToStringRoundTripFloat) {
    yaml_parser parser("3.14");
    auto result = parser.parse();
    EXPECT_EQ(result->to_string(), "3.14");
}

TEST_F(YamlParserTest, ToStringRoundTripSequence) {
    yaml_parser parser("[1, 2, 3]");
    auto result = parser.parse();
    EXPECT_EQ(result->to_string(), "[1, 2, 3]");
}

TEST_F(YamlParserTest, ToStringRoundTripMapping) {
    yaml_parser parser("{a: 1, b: 2}");
    auto result = parser.parse();
    EXPECT_EQ(result->to_string(), "{a: 1, b: 2}");
}

TEST_F(YamlParserTest, ToDocumentBlockMapping) {
    yaml_parser parser("key: value\n");
    auto result = parser.parse();
    string doc = result->to_document();
    EXPECT_TRUE(doc.contains("key"));
    EXPECT_TRUE(doc.contains("value"));
}

TEST_F(YamlParserTest, ParseIntegerHexUppercase) {
    yaml_parser parser("0X1A");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_integer());
    EXPECT_EQ(result->as_integer()->get_value(), 26);
}

TEST_F(YamlParserTest, ParseIntegerOctalUppercase) {
    yaml_parser parser("0O77");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_integer());
    EXPECT_EQ(result->as_integer()->get_value(), 63);
}

TEST_F(YamlParserTest, ParseIntegerBinaryUppercase) {
    yaml_parser parser("0B1010");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_integer());
    EXPECT_EQ(result->as_integer()->get_value(), 10);
}

TEST_F(YamlParserTest, ParseFloatNegativeInfinityLong) {
    yaml_parser parser("-.infinity");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_float());
    EXPECT_TRUE(is_infinity(result->as_float()->get_value()));
    EXPECT_LT(result->as_float()->get_value(), 0);
}

TEST_F(YamlParserTest, ParseStringDoubleQuotedSlashEscape) {
    yaml_parser parser("\"a\\/b\"");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_string());
    EXPECT_EQ(result->as_string()->get_value(), "a/b");
}

TEST_F(YamlParserTest, ParseStringDoubleQuotedFormFeedEscape) {
    yaml_parser parser("\"col1\\fcol2\"");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_string());
    EXPECT_EQ(result->as_string()->get_value(), "col1\fcol2");
}

TEST_F(YamlParserTest, ParseStringDoubleQuotedCarriageReturnEscape) {
    yaml_parser parser("\"line1\\rline2\"");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_string());
    EXPECT_EQ(result->as_string()->get_value(), "line1\rline2");
}

TEST_F(YamlParserTest, ParseStringDoubleQuotedVerticalTabEscape) {
    yaml_parser parser("\"col1\\vcol2\"");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_string());
    EXPECT_EQ(result->as_string()->get_value(), "col1\vcol2");
}

TEST_F(YamlParserTest, ParseStringDoubleQuotedEscapeEscape) {
    yaml_parser parser("\"esc\\eesc\"");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_string());
    EXPECT_EQ(result->as_string()->get_value(), "esc\x1B"
                                                "esc");
}

TEST_F(YamlParserTest, ParseStringDoubleQuotedUnicodeSupplementaryPlane) {
    yaml_parser parser("\"\\U0001F600\"");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_string());
    EXPECT_EQ(result->as_string()->get_value(), "\xF0\x9F\x98\x80");
}

TEST_F(YamlParserTest, ParseStringDoubleQuotedUnicodeMaxCodepoint) {
    yaml_parser parser("\"\\U0010FFFF\"");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_string());
    EXPECT_EQ(result->as_string()->get_value(), "\xF4\x8F\xBF\xBF");
}

TEST_F(YamlParserTest, ParseStringDoubleQuotedUnicodeOutOfRangeThrows) {
    yaml_parser parser("\"\\U00110000\"");
    EXPECT_THROW(ignore = parser.parse(), yaml_exception);
}

TEST_F(YamlParserTest, ParseTagInMappingValue) {
    yaml_parser parser("key: !mytag value\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_mapping());
    auto* val = result->as_mapping()->get_member("key");
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(val->get_tag(), "mytag");
}

TEST_F(YamlParserTest, ParseTagInSequenceItem) {
    yaml_parser parser("- !important item1\n- item2\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_sequence());
    EXPECT_EQ(result->as_sequence()->get_element(0)->get_tag(), "important");
}

TEST_F(YamlParserTest, ParseDocumentStartWithContent) {
    yaml_parser parser("---\nvalue: 42\n");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_mapping());
    EXPECT_EQ(result->as_mapping()->get_member("value")->as_integer()->get_value(), 42);
}

TEST_F(YamlParserTest, ParseFullYaml12Spec) {
    // Comprehensive YAML 1.2 document covering all major features:
    // null, boolean (with synonyms), integer (dec/hex/oct/bin), float (incl. specials),
    // string (all 5 styles), timestamps, sequences, mappings, anchors/aliases,
    // tags, comments, complex keys, merge keys, multiple documents, directives,
    // block scalar chomping, verbatim tags, empty collections
    const char* yaml = R"yaml(%YAML 1.2
---

string_plain: Hello, world!
string_quoted_single: 'It''s a string with ''single quotes'''
string_quoted_double: "It's a \"double quoted\" string\nwith newline"
string_folded: >
  This is a folded string.
  It will be one line with spaces,
  but newlines are replaced by spaces.
string_literal: |
  This is a literal block.
  Newlines are preserved.
  Even     extra   whitespace.

int_decimal: 42
int_hex: 0x2A
int_octal: 0o52
int_binary: 0b101010
float_scientific: 1.23e+4
float_decimal: 3.14159
inf: .inf
neginf: -.inf
nan: .nan

bool_true: true
bool_false: false
bool_yes: yes
bool_no: no
bool_on: on
bool_off: off

null_explicit: null
null_tilde: ~
null_empty:
null_capital: Null
null_upper: NULL

timestamp_utc: 2024-01-15T10:30:00Z
timestamp_date: 2024-01-15

list_inline: [1, 2, 3, "four"]
list_block:
  - apple
  - banana
  - carrot
  - &anchor_fruit orange
  - *anchor_fruit

map_inline: {key1: value1, key2: value2}
map_block:
  name: John Doe
  age: 30
  address:
    # street: 123 Main St
    city: Anytown
    country: Wonderland

nested:
  - name: Alice
    scores: [95, 87, 92]
  - name: Bob
    scores:
      math: 88
      english: 91

multiline_preserve: |2
    Indented literal block.
    Second line.
    Third line.

multiline_fold_strip: >-
  Folded block with stripping.
  Trailing newlines removed.

multiline_literal_keep: |+
  Literal block with keep.
  Trailing newlines preserved.


multiline_fold_keep: >+
  Folded block with keep chomping.
  Trailing newlines kept.


escaped_chars: "\b\t\n\f\r\"\\"

explicit_int: !!int "123"
explicit_bool: !!bool "true"
explicit_float: !!float "3.14"
custom_tag: !mytag "custom data"
verbatim_tag: !<tag:example.com,2019:app/type> "verbatim tag value"

base: &base
  name: Base
  version: 1.0
derived:
  <<: *base
  name: Derived

...
---
second_document: true
...
---
   third_document:
     - item1
     - item2
...
   mixed_flow_and_block:
      flow_seq_in_block: [a, b, {nested: true}]
      block_map_in_flow: {key: [1, 2, 3]}

   empty_seq: []
   empty_map: {}

   ? [complex, key]
     : complex value
   ? {a: 1, b: 2}
     : another complex value
   binary_data: !!binary "SGVsbG8sIFdvcmxkIQ=="
   weird_strings:
     colon_in_value: "key: value"
     braces: "{ not a map }"
     brackets: "[ not a seq ]"

---
!my!type
content: tagged node
...
)yaml";

    auto docs = yaml_parser(yaml).parse_documents();
    ASSERT_EQ(docs.size(), 5);

    auto doc0 = docs[0]->as_mapping();
    ASSERT_NE(doc0, nullptr);

    // Strings
    EXPECT_EQ(doc0->get_member("string_plain")->as_string()->get_value(), "Hello, world!");
    EXPECT_TRUE(doc0->get_member("string_quoted_double")->as_string()->get_value().find("double quoted") !=
                neforce::string::npos);

    // Integers
    EXPECT_EQ(doc0->get_member("int_decimal")->as_integer()->get_value(), 42);
    EXPECT_EQ(doc0->get_member("int_hex")->as_integer()->get_value(), 42);
    EXPECT_EQ(doc0->get_member("int_octal")->as_integer()->get_value(), 42);
    EXPECT_EQ(doc0->get_member("int_binary")->as_integer()->get_value(), 42);

    // Floats
    EXPECT_DOUBLE_EQ(doc0->get_member("float_scientific")->as_float()->get_value(), 12300.0);
    EXPECT_DOUBLE_EQ(doc0->get_member("float_decimal")->as_float()->get_value(), 3.14159);
    EXPECT_TRUE(is_infinity(doc0->get_member("inf")->as_float()->get_value()));
    EXPECT_TRUE(is_infinity(doc0->get_member("neginf")->as_float()->get_value()));
    EXPECT_TRUE(is_nan(doc0->get_member("nan")->as_float()->get_value()));

    // Booleans
    EXPECT_EQ(doc0->get_member("bool_true")->as_boolean()->get_value(), true);
    EXPECT_EQ(doc0->get_member("bool_false")->as_boolean()->get_value(), false);
    EXPECT_EQ(doc0->get_member("bool_yes")->as_boolean()->get_value(), true);
    EXPECT_EQ(doc0->get_member("bool_no")->as_boolean()->get_value(), false);
    EXPECT_EQ(doc0->get_member("bool_on")->as_boolean()->get_value(), true);
    EXPECT_EQ(doc0->get_member("bool_off")->as_boolean()->get_value(), false);

    // Nulls
    EXPECT_TRUE(doc0->get_member("null_explicit")->is_null());
    EXPECT_TRUE(doc0->get_member("null_tilde")->is_null());
    EXPECT_TRUE(doc0->get_member("null_empty")->is_null());
    EXPECT_TRUE(doc0->get_member("null_capital")->is_null());
    EXPECT_TRUE(doc0->get_member("null_upper")->is_null());

    // Timestamps
    EXPECT_TRUE(doc0->get_member("timestamp_utc")->is_timestamp());
    EXPECT_TRUE(doc0->get_member("timestamp_date")->is_string());
    EXPECT_EQ(doc0->get_member("timestamp_date")->as_string()->get_value(), "2024-01-15");

    // Sequence
    auto list_block = doc0->get_member("list_block")->as_sequence();
    ASSERT_NE(list_block, nullptr);
    EXPECT_EQ(list_block->size(), 5);
    EXPECT_EQ(list_block->get_element(0)->as_string()->get_value(), "apple");
    EXPECT_EQ(list_block->get_element(4)->as_string()->get_value(), "orange");

    // Flow sequence
    auto list_inline = doc0->get_member("list_inline")->as_sequence();
    ASSERT_NE(list_inline, nullptr);
    EXPECT_EQ(list_inline->size(), 4);

    // Mapping
    auto map_block = doc0->get_member("map_block")->as_mapping();
    ASSERT_NE(map_block, nullptr);
    EXPECT_EQ(map_block->get_member("name")->as_string()->get_value(), "John Doe");
    EXPECT_EQ(map_block->get_member("age")->as_integer()->get_value(), 30);
    auto address = map_block->get_member("address")->as_mapping();
    ASSERT_NE(address, nullptr);
    EXPECT_EQ(address->get_member("city")->as_string()->get_value(), "Anytown");

    // Nested structure
    auto nested = doc0->get_member("nested")->as_sequence();
    ASSERT_NE(nested, nullptr);
    EXPECT_EQ(nested->size(), 2);
    auto alice = nested->get_element(0)->as_mapping();
    EXPECT_EQ(alice->get_member("name")->as_string()->get_value(), "Alice");
    auto alice_scores = alice->get_member("scores")->as_sequence();
    EXPECT_EQ(alice_scores->size(), 3);

    // Merge key
    auto derived = doc0->get_member("derived")->as_mapping();
    ASSERT_NE(derived, nullptr);
    EXPECT_EQ(derived->get_member("name")->as_string()->get_value(), "Derived");
    EXPECT_FLOAT_EQ(derived->get_member("version")->as_float()->get_value(), 1.0f);

    // Block scalars
    auto lit_keep = doc0->get_member("multiline_literal_keep")->as_string();
    ASSERT_NE(lit_keep, nullptr);
    EXPECT_TRUE(lit_keep->get_value().find("Literal block with keep.") != neforce::string::npos);

    auto fold_keep = doc0->get_member("multiline_fold_keep")->as_string();
    ASSERT_NE(fold_keep, nullptr);
    EXPECT_TRUE(fold_keep->get_value().find("Folded block with keep") != neforce::string::npos);

    // Escaped characters
    auto escaped = doc0->get_member("escaped_chars")->as_string();
    ASSERT_NE(escaped, nullptr);
    EXPECT_TRUE(escaped->get_value().find('\b') != neforce::string::npos);
    EXPECT_TRUE(escaped->get_value().find('\t') != neforce::string::npos);
    EXPECT_TRUE(escaped->get_value().find('\n') != neforce::string::npos);

    // Explicit tags
    EXPECT_EQ(doc0->get_member("explicit_int")->as_string()->get_value(), "123");
    EXPECT_EQ(doc0->get_member("explicit_bool")->as_string()->get_value(), "true");
    EXPECT_EQ(doc0->get_member("explicit_float")->as_string()->get_value(), "3.14");
    EXPECT_EQ(doc0->get_member("custom_tag")->as_string()->get_value(), "custom data");
    EXPECT_EQ(doc0->get_member("verbatim_tag")->as_string()->get_value(), "verbatim tag value");

    auto doc1 = docs[1]->as_mapping();
    ASSERT_NE(doc1, nullptr);
    EXPECT_EQ(doc1->get_member("second_document")->as_boolean()->get_value(), true);

    auto doc2 = docs[2]->as_mapping();
    ASSERT_NE(doc2, nullptr);
    auto third_doc = doc2->get_member("third_document")->as_sequence();
    ASSERT_NE(third_doc, nullptr);
    EXPECT_EQ(third_doc->size(), 2);

    auto doc3 = docs[3]->as_mapping();
    ASSERT_NE(doc3, nullptr);

    // Empty collections
    auto empty_seq = doc3->get_member("empty_seq")->as_sequence();
    ASSERT_NE(empty_seq, nullptr);
    EXPECT_EQ(empty_seq->size(), 0);

    auto empty_map = doc3->get_member("empty_map")->as_mapping();
    ASSERT_NE(empty_map, nullptr);
    EXPECT_EQ(empty_map->get_members().size(), 0);

    // Complex keys
    EXPECT_TRUE(doc3->has_member("[complex, key]"));
    EXPECT_EQ(doc3->get_member("[complex, key]")->as_string()->get_value(), "complex value");
    EXPECT_TRUE(doc3->has_member("{a: 1, b: 2}"));
    EXPECT_EQ(doc3->get_member("{a: 1, b: 2}")->as_string()->get_value(), "another complex value");

    // Weird strings
    auto weird = doc3->get_member("weird_strings")->as_mapping();
    ASSERT_NE(weird, nullptr);
    EXPECT_EQ(weird->get_member("colon_in_value")->as_string()->get_value(), "key: value");
    EXPECT_EQ(weird->get_member("braces")->as_string()->get_value(), "{ not a map }");
    EXPECT_EQ(weird->get_member("brackets")->as_string()->get_value(), "[ not a seq ]");

    // Mixed flow and block
    auto mixed = doc3->get_member("mixed_flow_and_block")->as_mapping();
    ASSERT_NE(mixed, nullptr);
    auto flow_seq = mixed->get_member("flow_seq_in_block")->as_sequence();
    ASSERT_NE(flow_seq, nullptr);
    EXPECT_EQ(flow_seq->size(), 3);
    auto block_in_flow = mixed->get_member("block_map_in_flow")->as_mapping();
    ASSERT_NE(block_in_flow, nullptr);
    auto key_seq = block_in_flow->get_member("key")->as_sequence();
    ASSERT_NE(key_seq, nullptr);
    EXPECT_EQ(key_seq->size(), 3);

    auto doc4 = docs[4]->as_mapping();
    ASSERT_NE(doc4, nullptr);
    EXPECT_EQ(doc4->get_tag(), "my!type");
    EXPECT_EQ(doc4->get_member("content")->as_string()->get_value(), "tagged node");
}

TEST_F(YamlParserTest, ParseIntegerLeadingZeroThrows) {
    yaml_parser parser("07\n");
    EXPECT_THROW(ignore = parser.parse(), yaml_exception);
}

TEST_F(YamlParserTest, ParseIntegerLeadingZeroNegativeThrows) {
    yaml_parser parser("-07\n");
    EXPECT_THROW(ignore = parser.parse(), yaml_exception);
}

TEST_F(YamlParserTest, ParseIntegerLeadingZeroWithUnderscoreThrows) {
    yaml_parser parser("0_7\n");
    EXPECT_THROW(ignore = parser.parse(), yaml_exception);
}

TEST_F(YamlParserTest, ParseStringDoubleQuotedInvalidEscapeThrows) {
    yaml_parser parser("\"\\z\"");
    EXPECT_THROW(ignore = parser.parse(), yaml_exception);
}

TEST_F(YamlParserTest, ParseStringDoubleQuotedHexEscapeHighByte) {
    yaml_parser parser("\"\\xFF\"");
    auto result = parser.parse();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_string());
    EXPECT_EQ(result->as_string()->get_value(), "\xC3\xBF");
}


class YamlBuilderTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(YamlBuilderTest, BuildNull) {
    yaml_builder b;
    b.key("empty").value(nullptr);
    auto result = b.build();

    auto map = result->as_mapping();
    ASSERT_NE(map, nullptr);
    auto* val = map->get_member("empty");
    ASSERT_NE(val, nullptr);
    EXPECT_TRUE(val->is_null());
}

TEST_F(YamlBuilderTest, BuildNullWithKey) {
    yaml_builder b;
    b.key("nothing").value(nullptr);
    auto result = b.build();
    auto* val = result->as_mapping()->get_member("nothing");
    EXPECT_TRUE(val->is_null());
}

TEST_F(YamlBuilderTest, BuildBooleanTrue) {
    yaml_builder b;
    b.key("enabled").value(true);
    auto result = b.build();

    auto map = result->as_mapping();
    auto* val = map->get_member("enabled");
    ASSERT_NE(val, nullptr);
    EXPECT_TRUE(val->is_boolean());
    EXPECT_TRUE(val->as_boolean()->get_value());
}

TEST_F(YamlBuilderTest, BuildBooleanFalse) {
    yaml_builder b;
    b.key("disabled").value(false);
    auto result = b.build();

    auto map = result->as_mapping();
    auto* val = map->get_member("disabled");
    ASSERT_NE(val, nullptr);
    EXPECT_TRUE(val->is_boolean());
    EXPECT_FALSE(val->as_boolean()->get_value());
}

TEST_F(YamlBuilderTest, BuildInteger) {
    yaml_builder b;
    b.key("count").value(static_cast<int64_t>(42));
    auto result = b.build();

    auto map = result->as_mapping();
    auto* val = map->get_member("count");
    ASSERT_NE(val, nullptr);
    EXPECT_TRUE(val->is_integer());
    EXPECT_EQ(val->as_integer()->get_value(), 42);
}

TEST_F(YamlBuilderTest, BuildIntegerNegative) {
    yaml_builder b;
    b.key("neg").value(static_cast<int64_t>(-99));
    auto result = b.build();

    auto* val = result->as_mapping()->get_member("neg");
    EXPECT_EQ(val->as_integer()->get_value(), -99);
}

TEST_F(YamlBuilderTest, BuildIntegerFromInt) {
    yaml_builder b;
    b.key("x").value(100);
    auto result = b.build();

    auto* val = result->as_mapping()->get_member("x");
    EXPECT_TRUE(val->is_integer());
    EXPECT_EQ(val->as_integer()->get_value(), 100);
}

TEST_F(YamlBuilderTest, BuildFloat) {
    yaml_builder b;
    b.key("pi").value(3.14159);
    auto result = b.build();

    auto map = result->as_mapping();
    auto* val = map->get_member("pi");
    ASSERT_NE(val, nullptr);
    EXPECT_TRUE(val->is_float());
    EXPECT_DOUBLE_EQ(val->as_float()->get_value(), 3.14159);
}

TEST_F(YamlBuilderTest, BuildFloatSpecialInf) {
    yaml_builder b;
    b.key("inf").value(numeric_traits<double>::infinity());
    auto result = b.build();

    auto* val = result->as_mapping()->get_member("inf");
    EXPECT_TRUE(val->is_float());
    EXPECT_TRUE(is_infinity(val->as_float()->get_value()));
}

TEST_F(YamlBuilderTest, BuildFloatSpecialNegInf) {
    yaml_builder b;
    b.key("neginf").value(-numeric_traits<double>::infinity());
    auto result = b.build();

    auto* val = result->as_mapping()->get_member("neginf");
    EXPECT_TRUE(val->is_float());
    EXPECT_TRUE(is_infinity(val->as_float()->get_value()));
    EXPECT_LT(val->as_float()->get_value(), 0);
}

TEST_F(YamlBuilderTest, BuildFloatSpecialNaN) {
    yaml_builder b;
    b.key("nan").value(numeric_traits<double>::quiet_nan());
    auto result = b.build();

    auto* val = result->as_mapping()->get_member("nan");
    EXPECT_TRUE(val->is_float());
    EXPECT_TRUE(is_nan(val->as_float()->get_value()));
}

TEST_F(YamlBuilderTest, BuildStringPlain) {
    yaml_builder b;
    b.key("name").value(string("hello"));
    auto result = b.build();

    auto* val = result->as_mapping()->get_member("name");
    ASSERT_NE(val, nullptr);
    EXPECT_TRUE(val->is_string());
    EXPECT_EQ(val->as_string()->get_value(), "hello");
    EXPECT_EQ(val->as_string()->get_style(), yaml_string::Plain);
}

TEST_F(YamlBuilderTest, BuildStringCStr) {
    yaml_builder b;
    b.key("name").value("C-string");
    auto result = b.build();

    auto* val = result->as_mapping()->get_member("name");
    EXPECT_EQ(val->as_string()->get_value(), "C-string");
}

TEST_F(YamlBuilderTest, BuildStringView) {
    yaml_builder b;
    b.key("name").value(string_view("view"));
    auto result = b.build();

    auto* val = result->as_mapping()->get_member("name");
    EXPECT_EQ(val->as_string()->get_value(), "view");
}

TEST_F(YamlBuilderTest, BuildStringSingleQuoted) {
    yaml_builder b;
    b.key("path").value_string("it's quoted", yaml_string::SingleQuoted);
    auto result = b.build();

    auto* val = result->as_mapping()->get_member("path");
    EXPECT_EQ(val->as_string()->get_value(), "it's quoted");
    EXPECT_EQ(val->as_string()->get_style(), yaml_string::SingleQuoted);
}

TEST_F(YamlBuilderTest, BuildStringDoubleQuoted) {
    yaml_builder b;
    b.key("desc").value_string("has \"quotes\"", yaml_string::DoubleQuoted);
    auto result = b.build();

    auto* val = result->as_mapping()->get_member("desc");
    EXPECT_EQ(val->as_string()->get_value(), "has \"quotes\"");
    EXPECT_EQ(val->as_string()->get_style(), yaml_string::DoubleQuoted);
}

TEST_F(YamlBuilderTest, BuildStringLiteral) {
    yaml_builder b;
    b.key("code").value_string("line1\nline2\nline3", yaml_string::Literal);
    auto result = b.build();

    auto* val = result->as_mapping()->get_member("code");
    EXPECT_EQ(val->as_string()->get_value(), "line1\nline2\nline3");
    EXPECT_EQ(val->as_string()->get_style(), yaml_string::Literal);
}

TEST_F(YamlBuilderTest, BuildStringFolded) {
    yaml_builder b;
    b.key("desc").value_string("folded text here", yaml_string::Folded);
    auto result = b.build();

    auto* val = result->as_mapping()->get_member("desc");
    EXPECT_EQ(val->as_string()->get_value(), "folded text here");
    EXPECT_EQ(val->as_string()->get_style(), yaml_string::Folded);
}

TEST_F(YamlBuilderTest, BuildTimestamp) {
    datetime dt;
    dt.try_parse_RFC3339("2024-01-15T10:30:00Z");

    yaml_builder b;
    b.key("created").value_datetime(dt);
    auto result = b.build();

    auto* val = result->as_mapping()->get_member("created");
    ASSERT_NE(val, nullptr);
    EXPECT_TRUE(val->is_timestamp());
}

TEST_F(YamlBuilderTest, BuildMultipleScalars) {
    yaml_builder b;
    b.key("name").value("John").key("age").value(30).key("score").value(98.6).key("active").value(true);

    auto result = b.build();
    auto map = result->as_mapping();

    EXPECT_EQ(map->get_member("name")->as_string()->get_value(), "John");
    EXPECT_EQ(map->get_member("age")->as_integer()->get_value(), 30);
    EXPECT_DOUBLE_EQ(map->get_member("score")->as_float()->get_value(), 98.6);
    EXPECT_TRUE(map->get_member("active")->as_boolean()->get_value());
}

TEST_F(YamlBuilderTest, BuildBlockSequence) {
    yaml_builder b;
    b.key("items").begin_sequence().value(1).value(2).value(3).end_sequence();
    auto result = b.build();

    auto seq = result->as_mapping()->get_member("items")->as_sequence();
    ASSERT_NE(seq, nullptr);
    EXPECT_EQ(seq->size(), 3);
    EXPECT_EQ(seq->get_element(0)->as_integer()->get_value(), 1);
    EXPECT_EQ(seq->get_element(1)->as_integer()->get_value(), 2);
    EXPECT_EQ(seq->get_element(2)->as_integer()->get_value(), 3);
    EXPECT_EQ(seq->get_style(), yaml_sequence::Block);
}

TEST_F(YamlBuilderTest, BuildBlockSequenceMixedTypes) {
    yaml_builder b;
    b.key("mixed").begin_sequence().value("string").value(42).value(3.14).value(true).value(nullptr).end_sequence();
    auto result = b.build();

    auto seq = result->as_mapping()->get_member("mixed")->as_sequence();
    ASSERT_EQ(seq->size(), 5);
    EXPECT_TRUE(seq->get_element(0)->is_string());
    EXPECT_TRUE(seq->get_element(1)->is_integer());
    EXPECT_TRUE(seq->get_element(2)->is_float());
    EXPECT_TRUE(seq->get_element(3)->is_boolean());
    EXPECT_TRUE(seq->get_element(4)->is_null());
}

TEST_F(YamlBuilderTest, BuildNestedSequences) {
    yaml_builder b;
    b.key("matrix")
            .begin_sequence()
            .begin_sequence()
            .value(1)
            .value(2)
            .end_sequence()
            .begin_sequence()
            .value(3)
            .value(4)
            .end_sequence()
            .end_sequence();
    auto result = b.build();

    auto outer = result->as_mapping()->get_member("matrix")->as_sequence();
    ASSERT_EQ(outer->size(), 2);
    auto inner1 = outer->get_element(0)->as_sequence();
    EXPECT_EQ(inner1->get_element(0)->as_integer()->get_value(), 1);
    EXPECT_EQ(inner1->get_element(1)->as_integer()->get_value(), 2);
    auto inner2 = outer->get_element(1)->as_sequence();
    EXPECT_EQ(inner2->get_element(0)->as_integer()->get_value(), 3);
    EXPECT_EQ(inner2->get_element(1)->as_integer()->get_value(), 4);
}

TEST_F(YamlBuilderTest, BuildFlowSequence) {
    yaml_builder b;
    b.key("inline").begin_flow_sequence().value("a").value("b").value("c").end_sequence();
    auto result = b.build();

    auto seq = result->as_mapping()->get_member("inline")->as_sequence();
    ASSERT_NE(seq, nullptr);
    EXPECT_EQ(seq->size(), 3);
    EXPECT_EQ(seq->get_style(), yaml_sequence::Flow);
}

TEST_F(YamlBuilderTest, BuildEmptySequence) {
    yaml_builder b;
    b.key("empty_list").begin_sequence().end_sequence();
    auto result = b.build();

    auto seq = result->as_mapping()->get_member("empty_list")->as_sequence();
    ASSERT_NE(seq, nullptr);
    EXPECT_EQ(seq->size(), 0);
}

TEST_F(YamlBuilderTest, BuildEmptyFlowSequence) {
    yaml_builder b;
    b.key("empty_flow").begin_flow_sequence().end_sequence();
    auto result = b.build();

    auto seq = result->as_mapping()->get_member("empty_flow")->as_sequence();
    ASSERT_NE(seq, nullptr);
    EXPECT_EQ(seq->size(), 0);
    EXPECT_EQ(seq->get_style(), yaml_sequence::Flow);
}

TEST_F(YamlBuilderTest, BuildBlockMapping) {
    yaml_builder b;
    b.key("person").begin_mapping().key("name").value("Alice").key("age").value(25).end_mapping();
    auto result = b.build();

    auto person = result->as_mapping()->get_member("person")->as_mapping();
    ASSERT_NE(person, nullptr);
    EXPECT_EQ(person->get_style(), yaml_mapping::Block);
    EXPECT_EQ(person->get_member("name")->as_string()->get_value(), "Alice");
    EXPECT_EQ(person->get_member("age")->as_integer()->get_value(), 25);
}

TEST_F(YamlBuilderTest, BuildNestedBlockMappings) {
    yaml_builder b;
    b.key("config")
            .begin_mapping()
            .key("database")
            .begin_mapping()
            .key("host")
            .value("localhost")
            .key("port")
            .value(5432)
            .end_mapping()
            .key("cache")
            .begin_mapping()
            .key("enabled")
            .value(true)
            .key("ttl")
            .value(3600)
            .end_mapping()
            .end_mapping();
    auto result = b.build();

    auto config = result->as_mapping()->get_member("config")->as_mapping();
    auto db = config->get_member("database")->as_mapping();
    EXPECT_EQ(db->get_member("host")->as_string()->get_value(), "localhost");
    EXPECT_EQ(db->get_member("port")->as_integer()->get_value(), 5432);

    auto cache = config->get_member("cache")->as_mapping();
    EXPECT_TRUE(cache->get_member("enabled")->as_boolean()->get_value());
    EXPECT_EQ(cache->get_member("ttl")->as_integer()->get_value(), 3600);
}

TEST_F(YamlBuilderTest, BuildEmptyMapping) {
    yaml_builder b;
    b.key("empty_map").begin_mapping().end_mapping();
    auto result = b.build();

    auto map = result->as_mapping()->get_member("empty_map")->as_mapping();
    ASSERT_NE(map, nullptr);
    EXPECT_TRUE(map->get_members().empty());
}

TEST_F(YamlBuilderTest, BuildFlowMapping) {
    yaml_builder b;
    b.key("point").begin_flow_mapping().key("x").value(10).key("y").value(20).end_mapping();
    auto result = b.build();

    auto point = result->as_mapping()->get_member("point")->as_mapping();
    ASSERT_NE(point, nullptr);
    EXPECT_EQ(point->get_style(), yaml_mapping::Flow);
    EXPECT_EQ(point->get_member("x")->as_integer()->get_value(), 10);
    EXPECT_EQ(point->get_member("y")->as_integer()->get_value(), 20);
}

TEST_F(YamlBuilderTest, BuildEmptyFlowMapping) {
    yaml_builder b;
    b.key("empty_flow_map").begin_flow_mapping().end_mapping();
    auto result = b.build();

    auto map = result->as_mapping()->get_member("empty_flow_map")->as_mapping();
    ASSERT_NE(map, nullptr);
    EXPECT_EQ(map->get_style(), yaml_mapping::Flow);
    EXPECT_TRUE(map->get_members().empty());
}

TEST_F(YamlBuilderTest, BuildSequenceInMapping) {
    yaml_builder b;
    b.key("server")
            .begin_mapping()
            .key("ports")
            .begin_sequence()
            .value(80)
            .value(443)
            .value(8080)
            .end_sequence()
            .end_mapping();
    auto result = b.build();

    auto ports = result->as_mapping()->get_member("server")->as_mapping()->get_member("ports")->as_sequence();
    ASSERT_EQ(ports->size(), 3);
}

TEST_F(YamlBuilderTest, BuildMappingInSequence) {
    yaml_builder b;
    b.key("users")
            .begin_sequence()
            .begin_mapping()
            .key("name")
            .value("Alice")
            .key("role")
            .value("admin")
            .end_mapping()
            .begin_mapping()
            .key("name")
            .value("Bob")
            .key("role")
            .value("user")
            .end_mapping()
            .end_sequence();
    auto result = b.build();

    auto users = result->as_mapping()->get_member("users")->as_sequence();
    ASSERT_EQ(users->size(), 2);
    EXPECT_EQ(users->get_element(0)->as_mapping()->get_member("name")->as_string()->get_value(), "Alice");
    EXPECT_EQ(users->get_element(1)->as_mapping()->get_member("name")->as_string()->get_value(), "Bob");
}

TEST_F(YamlBuilderTest, BuildDeepNesting) {
    yaml_builder b;
    b.key("a")
            .begin_mapping()
            .key("b")
            .begin_mapping()
            .key("c")
            .begin_sequence()
            .begin_mapping()
            .key("d")
            .begin_flow_sequence()
            .value("deep")
            .end_sequence()
            .end_mapping()
            .end_sequence()
            .end_mapping()
            .end_mapping();
    auto result = b.build();

    auto deep = result->as_mapping()
                        ->get_member("a")
                        ->as_mapping()
                        ->get_member("b")
                        ->as_mapping()
                        ->get_member("c")
                        ->as_sequence()
                        ->get_element(0)
                        ->as_mapping()
                        ->get_member("d")
                        ->as_sequence();
    ASSERT_EQ(deep->size(), 1);
    EXPECT_EQ(deep->get_element(0)->as_string()->get_value(), "deep");
    EXPECT_EQ(deep->get_style(), yaml_sequence::Flow);
}

TEST_F(YamlBuilderTest, BuildAnchorOnScalar) {
    yaml_builder b;
    b.key("first").anchor("ref").value("shared value");
    auto result = b.build();

    auto* val = result->as_mapping()->get_member("first");
    EXPECT_EQ(val->get_anchor(), "ref");
    EXPECT_EQ(val->as_string()->get_value(), "shared value");
}

TEST_F(YamlBuilderTest, BuildAnchorOnSequence) {
    yaml_builder b;
    b.key("items").anchor("seq_anchor").begin_sequence().value(1).value(2).end_sequence();
    auto result = b.build();

    auto* seq = result->as_mapping()->get_member("items");
    EXPECT_EQ(seq->get_anchor(), "seq_anchor");
    EXPECT_EQ(seq->as_sequence()->size(), 2);
}

TEST_F(YamlBuilderTest, BuildAnchorOnMapping) {
    yaml_builder b;
    b.key("config").anchor("cfg").begin_mapping().key("k").value("v").end_mapping();
    auto result = b.build();

    auto* map = result->as_mapping()->get_member("config");
    EXPECT_EQ(map->get_anchor(), "cfg");
    EXPECT_EQ(map->as_mapping()->get_member("k")->as_string()->get_value(), "v");
}

TEST_F(YamlBuilderTest, BuildAlias) {
    yaml_builder b;
    b.key("original").anchor("shared").value("reusable");
    b.key("copy").alias("shared");
    auto result = b.build();

    auto* orig = result->as_mapping()->get_member("original");
    auto* copy = result->as_mapping()->get_member("copy");
    EXPECT_EQ(orig->as_string()->get_value(), "reusable");
    EXPECT_EQ(copy->as_string()->get_value(), "reusable");
    EXPECT_EQ(orig->as_string().get_value(), copy->as_string().get_value());
}

TEST_F(YamlBuilderTest, BuildAliasUnknownAnchor) {
    yaml_builder b;
    EXPECT_THROW(b.alias("nonexistent"), yaml_exception);
}

TEST_F(YamlBuilderTest, BuildTagOnString) {
    yaml_builder b;
    b.key("data").tag("!!str").value("123");
    auto result = b.build();

    auto* val = result->as_mapping()->get_member("data");
    EXPECT_EQ(val->get_tag(), "!!str");
    EXPECT_EQ(val->as_string()->get_value(), "123");
}

TEST_F(YamlBuilderTest, BuildTagOnSequence) {
    yaml_builder b;
    b.key("items").tag("!mylist").begin_sequence().value(1).value(2).end_sequence();
    auto result = b.build();

    auto* seq = result->as_mapping()->get_member("items");
    EXPECT_EQ(seq->get_tag(), "!mylist");
}

TEST_F(YamlBuilderTest, BuildTagOnMapping) {
    yaml_builder b;
    b.key("obj").tag("!myobj").begin_mapping().key("type").value("custom").end_mapping();
    auto result = b.build();

    auto* map = result->as_mapping()->get_member("obj");
    EXPECT_EQ(map->get_tag(), "!myobj");
}

TEST_F(YamlBuilderTest, BuildAnchorAndTag) {
    yaml_builder b;
    b.key("node").anchor("n1").tag("!tagged").value("both");
    auto result = b.build();

    auto* val = result->as_mapping()->get_member("node");
    EXPECT_EQ(val->get_anchor(), "n1");
    EXPECT_EQ(val->get_tag(), "!tagged");
    EXPECT_EQ(val->as_string()->get_value(), "both");
}

TEST_F(YamlBuilderTest, BuildValueMapping) {
    yaml_builder b;
    b.key("settings").value_mapping([](yaml_builder& inner) {
        inner.key("theme").value("dark");
        inner.key("font_size").value(14);
    });
    auto result = b.build();

    auto settings = result->as_mapping()->get_member("settings")->as_mapping();
    EXPECT_EQ(settings->get_member("theme")->as_string()->get_value(), "dark");
    EXPECT_EQ(settings->get_member("font_size")->as_integer()->get_value(), 14);
    EXPECT_EQ(settings->get_style(), yaml_mapping::Block);
}

TEST_F(YamlBuilderTest, BuildValueBlockMapping) {
    yaml_builder b;
    b.key("block").value_block_mapping([](yaml_builder& inner) { inner.key("a").value(1); });
    auto result = b.build();

    auto map = result->as_mapping()->get_member("block")->as_mapping();
    EXPECT_EQ(map->get_style(), yaml_mapping::Block);
}

TEST_F(YamlBuilderTest, BuildValueFlowMapping) {
    yaml_builder b;
    b.key("flow").value_flow_mapping([](yaml_builder& inner) {
        inner.key("x").value(1);
        inner.key("y").value(2);
    });
    auto result = b.build();

    auto map = result->as_mapping()->get_member("flow")->as_mapping();
    EXPECT_EQ(map->get_style(), yaml_mapping::Flow);
}

TEST_F(YamlBuilderTest, BuildValueSequence) {
    yaml_builder b;
    b.key("values").value_sequence([](yaml_builder& inner) {
        inner.value("a");
        inner.value("b");
        inner.value("c");
    });
    auto result = b.build();

    auto seq = result->as_mapping()->get_member("values")->as_sequence();
    ASSERT_EQ(seq->size(), 3);
    EXPECT_EQ(seq->get_style(), yaml_sequence::Block);
}

TEST_F(YamlBuilderTest, BuildValueBlockSequence) {
    yaml_builder b;
    b.key("bs").value_block_sequence([](yaml_builder& inner) { inner.value(1); });
    auto result = b.build();

    auto seq = result->as_mapping()->get_member("bs")->as_sequence();
    EXPECT_EQ(seq->get_style(), yaml_sequence::Block);
}

TEST_F(YamlBuilderTest, BuildValueFlowSequence) {
    yaml_builder b;
    b.key("fs").value_flow_sequence([](yaml_builder& inner) { inner.value(1).value(2).value(3); });
    auto result = b.build();

    auto seq = result->as_mapping()->get_member("fs")->as_sequence();
    EXPECT_EQ(seq->get_style(), yaml_sequence::Flow);
}

TEST_F(YamlBuilderTest, BuildNestedFunctional) {
    yaml_builder b;
    b.key("root").value_mapping([&](yaml_builder& inner) {
        inner.key("list").value_sequence([&](yaml_builder& seq) {
            seq.value_mapping([&](yaml_builder& item) { item.key("id").value(1); });
            seq.value_flow_mapping([&](yaml_builder& item) { item.key("id").value(2); });
        });
    });
    auto result = b.build();

    auto root = result->as_mapping()->get_member("root")->as_mapping();
    auto list = root->get_member("list")->as_sequence();
    ASSERT_EQ(list->size(), 2);
    EXPECT_EQ(list->get_element(0)->as_mapping()->get_style(), yaml_mapping::Block);
    EXPECT_EQ(list->get_element(1)->as_mapping()->get_style(), yaml_mapping::Flow);
}

TEST_F(YamlBuilderTest, ToDocumentSimpleMapping) {
    yaml_builder b;
    b.key("key").value("value");
    auto result = b.build();

    string doc = result->to_document();
    EXPECT_TRUE(doc.contains("key"));
    EXPECT_TRUE(doc.contains("value"));
}

TEST_F(YamlBuilderTest, ToDocumentBlockSequence) {
    yaml_builder b;
    b.key("items").begin_sequence().value("a").value("b").end_sequence();
    auto result = b.build();

    string doc = result->to_document();
    EXPECT_TRUE(doc.contains("items"));
    EXPECT_TRUE(doc.contains("- a"));
    EXPECT_TRUE(doc.contains("- b"));
}

TEST_F(YamlBuilderTest, ToDocumentFlowSequence) {
    yaml_builder b;
    b.key("tags").begin_flow_sequence().value("fast").value("reliable").end_sequence();
    auto result = b.build();

    string doc = result->to_document();
    EXPECT_TRUE(doc.contains("[fast, reliable]"));
}

TEST_F(YamlBuilderTest, ToDocumentFlowMapping) {
    yaml_builder b;
    b.key("point").begin_flow_mapping().key("x").value(10).key("y").value(20).end_mapping();
    auto result = b.build();

    string doc = result->to_document();
    EXPECT_TRUE(doc.contains("{x: 10, y: 20}"));
}

TEST_F(YamlBuilderTest, ToDocumentNull) {
    yaml_builder b;
    b.key("nothing").value(nullptr);
    auto result = b.build();

    string doc = result->to_document();
    EXPECT_TRUE(doc.contains("null"));
}

TEST_F(YamlBuilderTest, ToDocumentBoolean) {
    yaml_builder b;
    b.key("flag").value(true);
    auto result = b.build();

    string doc = result->to_document();
    EXPECT_TRUE(doc.contains("true"));
}

TEST_F(YamlBuilderTest, ToDocumentTimestamp) {
    datetime dt;
    dt.try_parse_RFC3339("2024-01-15T10:30:00Z");

    yaml_builder b;
    b.key("ts").value_datetime(dt);
    auto result = b.build();

    string doc = result->to_document();
    EXPECT_TRUE(doc.contains("2024-01-15T10:30:00Z"));
}

TEST_F(YamlBuilderTest, ToDocumentStringStyles) {
    yaml_builder b;
    b.key("single")
            .value_string("s", yaml_string::SingleQuoted)
            .key("double")
            .value_string("d", yaml_string::DoubleQuoted)
            .key("literal")
            .value_string("line1\nline2", yaml_string::Literal)
            .key("folded")
            .value_string("folded", yaml_string::Folded);
    auto result = b.build();

    string doc = result->to_document();
    EXPECT_TRUE(doc.contains("'s'"));
    EXPECT_TRUE(doc.contains("\"d\""));
    EXPECT_TRUE(doc.contains("|"));
    EXPECT_TRUE(doc.contains(">"));
}

TEST_F(YamlBuilderTest, ToDocumentAnchor) {
    yaml_builder b;
    b.key("first").anchor("ref").value("shared");
    auto result = b.build();

    string doc = result->to_document();
    EXPECT_TRUE(doc.contains("&ref"));
    EXPECT_TRUE(doc.contains("shared"));
}

TEST_F(YamlBuilderTest, ToDocumentTag) {
    yaml_builder b;
    b.key("data").tag("!!str").value("123");
    auto result = b.build();

    string doc = result->to_document();
    EXPECT_TRUE(doc.contains("!!str"));
}

TEST_F(YamlBuilderTest, RoundTripSimpleMapping) {
    yaml_builder b;
    b.key("name")
            .value("test")
            .key("count")
            .value(42)
            .key("pi")
            .value(3.14)
            .key("active")
            .value(true)
            .key("empty")
            .value(nullptr);

    auto built = b.build();
    string doc = built->to_document();

    yaml_parser parser(doc);
    auto parsed = parser.parse();

    auto map = parsed->as_mapping();
    ASSERT_NE(map, nullptr);
    EXPECT_EQ(map->get_member("name")->as_string()->get_value(), "test");
    EXPECT_EQ(map->get_member("count")->as_integer()->get_value(), 42);
    EXPECT_DOUBLE_EQ(map->get_member("pi")->as_float()->get_value(), 3.14);
    EXPECT_TRUE(map->get_member("active")->as_boolean()->get_value());
    EXPECT_TRUE(map->get_member("empty")->is_null());
}

TEST_F(YamlBuilderTest, RoundTripSequence) {
    yaml_builder b;
    b.key("items").begin_sequence().value("a").value("b").value("c").end_sequence();

    auto built = b.build();
    string doc = built->to_document();

    yaml_parser parser(doc);
    auto parsed = parser.parse();

    auto seq = parsed->as_mapping()->get_member("items")->as_sequence();
    ASSERT_EQ(seq->size(), 3);
    EXPECT_EQ(seq->get_element(0)->as_string()->get_value(), "a");
    EXPECT_EQ(seq->get_element(1)->as_string()->get_value(), "b");
    EXPECT_EQ(seq->get_element(2)->as_string()->get_value(), "c");
}

TEST_F(YamlBuilderTest, RoundTripFlowSequence) {
    yaml_builder b;
    b.key("tags").begin_flow_sequence().value("x").value("y").end_sequence();

    auto built = b.build();
    string doc = built->to_document();

    yaml_parser parser(doc);
    auto parsed = parser.parse();

    auto seq = parsed->as_mapping()->get_member("tags")->as_sequence();
    ASSERT_EQ(seq->size(), 2);
    EXPECT_EQ(seq->get_style(), yaml_sequence::Flow);
}

TEST_F(YamlBuilderTest, RoundTripFlowMapping) {
    yaml_builder b;
    b.key("point").begin_flow_mapping().key("x").value(1).key("y").value(2).end_mapping();

    auto built = b.build();
    string doc = built->to_document();

    yaml_parser parser(doc);
    auto parsed = parser.parse();

    auto point = parsed->as_mapping()->get_member("point")->as_mapping();
    EXPECT_EQ(point->get_member("x")->as_integer()->get_value(), 1);
    EXPECT_EQ(point->get_member("y")->as_integer()->get_value(), 2);
    EXPECT_EQ(point->get_style(), yaml_mapping::Flow);
}

TEST_F(YamlBuilderTest, RoundTripNestedStructures) {
    yaml_builder b;
    b.key("servers")
            .begin_sequence()
            .begin_mapping()
            .key("host")
            .value("srv1")
            .key("ports")
            .begin_flow_sequence()
            .value(80)
            .value(443)
            .end_sequence()
            .end_mapping()
            .begin_mapping()
            .key("host")
            .value("srv2")
            .key("ports")
            .begin_flow_sequence()
            .value(8080)
            .end_sequence()
            .end_mapping()
            .end_sequence();

    auto built = b.build();
    string doc = built->to_document();

    yaml_parser parser(doc);
    auto parsed = parser.parse();

    auto servers = parsed->as_mapping()->get_member("servers")->as_sequence();
    ASSERT_EQ(servers->size(), 2);
    auto srv1 = servers->get_element(0)->as_mapping();
    EXPECT_EQ(srv1->get_member("host")->as_string()->get_value(), "srv1");
    auto ports1 = srv1->get_member("ports")->as_sequence();
    EXPECT_EQ(ports1->size(), 2);

    auto srv2 = servers->get_element(1)->as_mapping();
    EXPECT_EQ(srv2->get_member("host")->as_string()->get_value(), "srv2");
}

TEST_F(YamlBuilderTest, RoundTripTimestamp) {
    datetime dt;
    dt.try_parse_RFC3339("2024-01-15T10:30:00Z");

    yaml_builder b;
    b.key("created").value_datetime(dt);

    auto built = b.build();
    string doc = built->to_document();

    yaml_parser parser(doc);
    auto parsed = parser.parse();

    auto* ts = parsed->as_mapping()->get_member("created");
    ASSERT_NE(ts, nullptr);
    EXPECT_TRUE(ts->is_timestamp());
}

TEST_F(YamlBuilderTest, RoundTripStringStyles) {
    yaml_builder b;
    b.key("quoted")
            .value_string("it's ok", yaml_string::SingleQuoted)
            .key("escaped")
            .value_string("has \"quotes\"", yaml_string::DoubleQuoted);

    auto built = b.build();
    string doc = built->to_document();

    yaml_parser parser(doc);
    auto parsed = parser.parse();

    auto map = parsed->as_mapping();
    EXPECT_EQ(map->get_member("quoted")->as_string()->get_value(), "it's ok");
    EXPECT_EQ(map->get_member("escaped")->as_string()->get_value(), "has \"quotes\"");
}

TEST_F(YamlBuilderTest, BuildIterableVector) {
    yaml_builder b;
    b.key("numbers").value_iterable(vector<int>{1, 2, 3, 4, 5});
    auto result = b.build();

    auto seq = result->as_mapping()->get_member("numbers")->as_sequence();
    ASSERT_EQ(seq->size(), 5);
    EXPECT_EQ(seq->get_element(0)->as_integer()->get_value(), 1);
    EXPECT_EQ(seq->get_element(4)->as_integer()->get_value(), 5);
}

TEST_F(YamlBuilderTest, BuildIterableMap) {
    yaml_builder b;
    flat_unordered_map<string, int> m;
    m["a"] = 1;
    m["b"] = 2;
    b.key("dict").value_iterable(m);
    auto result = b.build();

    auto map = result->as_mapping()->get_member("dict")->as_mapping();
    EXPECT_EQ(map->get_style(), yaml_mapping::Flow);
    EXPECT_EQ(map->get_member("a")->as_integer()->get_value(), 1);
    EXPECT_EQ(map->get_member("b")->as_integer()->get_value(), 2);
}

TEST_F(YamlBuilderTest, BuildIterableEmpty) {
    yaml_builder b;
    b.key("empty").value_iterable(vector<int>{});
    auto result = b.build();

    auto seq = result->as_mapping()->get_member("empty")->as_sequence();
    ASSERT_NE(seq, nullptr);
    EXPECT_EQ(seq->size(), 0);
}

TEST_F(YamlBuilderTest, BuildIterableArray) {
    yaml_builder b;
    b.key("chars").value_iterable(array<int, 3>{10, 20, 30});
    auto result = b.build();

    auto seq = result->as_mapping()->get_member("chars")->as_sequence();
    ASSERT_EQ(seq->size(), 3);
}

TEST_F(YamlBuilderTest, BeginDocumentSavesPrevious) {
    yaml_builder b;
    b.key("doc").value("first");

    b.begin_document();

    b.key("doc").value("second");

    auto docs = b.build_documents();
    ASSERT_EQ(docs.size(), 2);

    auto doc1 = docs[0]->as_mapping();
    EXPECT_EQ(doc1->get_member("doc")->as_string()->get_value(), "first");

    auto doc2 = docs[1]->as_mapping();
    EXPECT_EQ(doc2->get_member("doc")->as_string()->get_value(), "second");
}

TEST_F(YamlBuilderTest, BuildSingleDocument) {
    yaml_builder b;
    b.key("x").value(1);
    auto result = b.build();

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is_mapping());
    EXPECT_EQ(result->as_mapping()->get_member("x")->as_integer()->get_value(), 1);
}

TEST_F(YamlBuilderTest, BuildDocumentsWithoutExplicitBegin) {
    yaml_builder b;
    b.key("only").value("doc");
    auto docs = b.build_documents();

    ASSERT_EQ(docs.size(), 1);
    EXPECT_EQ(docs[0]->as_mapping()->get_member("only")->as_string()->get_value(), "doc");
}

TEST_F(YamlBuilderTest, ErrorDuplicateKey) {
    yaml_builder b;
    b.key("dup").value(1);
    EXPECT_THROW(b.key("dup").value(2), yaml_exception);
}

TEST_F(YamlBuilderTest, ErrorNoKeyInMapping) {
    yaml_builder b;
    EXPECT_THROW(b.value("no key set"), yaml_exception);
}

TEST_F(YamlBuilderTest, ErrorSetKeyInSequence) {
    yaml_builder b;
    b.key("list").begin_sequence();
    EXPECT_THROW(b.key("bad"), yaml_exception);
    b.end_sequence();
}

TEST_F(YamlBuilderTest, ErrorEndWrongContext) {
    yaml_builder b;
    b.key("map").begin_mapping();
    EXPECT_THROW(b.end_sequence(), yaml_exception);
    b.end_mapping();
}

TEST_F(YamlBuilderTest, ErrorEndRootMapping) {
    yaml_builder b;
    EXPECT_THROW(b.end_mapping(), yaml_exception);
}

TEST_F(YamlBuilderTest, ErrorEndRootSequence) {
    yaml_builder b;
    EXPECT_THROW(b.end_sequence(), yaml_exception);
}

TEST_F(YamlBuilderTest, ErrorUnclosedContextInBuild) {
    yaml_builder b;
    b.key("outer").begin_mapping();
    EXPECT_THROW(ignore = b.build(), yaml_exception);
}

TEST_F(YamlBuilderTest, ErrorBeginDocumentWithUnclosed) {
    yaml_builder b;
    b.key("outer").begin_mapping();
    EXPECT_THROW(b.begin_document(), yaml_exception);
}

TEST_F(YamlBuilderTest, BuildSequenceOfSequences) {
    yaml_builder b;
    b.key("grid")
            .begin_sequence()
            .begin_block_sequence()
            .value(1)
            .value(2)
            .end_sequence()
            .begin_flow_sequence()
            .value(3)
            .value(4)
            .end_sequence()
            .end_sequence();
    auto result = b.build();

    auto grid = result->as_mapping()->get_member("grid")->as_sequence();
    ASSERT_EQ(grid->size(), 2);
    EXPECT_EQ(grid->get_element(0)->as_sequence()->get_style(), yaml_sequence::Block);
    EXPECT_EQ(grid->get_element(1)->as_sequence()->get_style(), yaml_sequence::Flow);
}

TEST_F(YamlBuilderTest, BuildMappingOfMixedMappings) {
    yaml_builder b;
    b.key("container")
            .begin_mapping()
            .key("block_map")
            .begin_block_mapping()
            .key("a")
            .value(1)
            .end_mapping()
            .key("flow_map")
            .begin_flow_mapping()
            .key("b")
            .value(2)
            .end_mapping()
            .end_mapping();
    auto result = b.build();

    auto container = result->as_mapping()->get_member("container")->as_mapping();
    EXPECT_EQ(container->get_member("block_map")->as_mapping()->get_style(), yaml_mapping::Block);
    EXPECT_EQ(container->get_member("flow_map")->as_mapping()->get_style(), yaml_mapping::Flow);
}

TEST_F(YamlBuilderTest, BuildFlowSequenceInFlowMapping) {
    yaml_builder b;
    b.key("obj")
            .begin_flow_mapping()
            .key("name")
            .value("test")
            .key("tags")
            .begin_flow_sequence()
            .value("a")
            .value("b")
            .end_sequence()
            .end_mapping();
    auto result = b.build();

    auto obj = result->as_mapping()->get_member("obj")->as_mapping();
    EXPECT_EQ(obj->get_style(), yaml_mapping::Flow);
    auto tags = obj->get_member("tags")->as_sequence();
    EXPECT_EQ(tags->get_style(), yaml_sequence::Flow);
    EXPECT_EQ(tags->size(), 2);
}

TEST_F(YamlBuilderTest, BuildStringWithSpecialCharacters) {
    yaml_builder b;
    b.key("special").value("contains: colon and # hash");
    auto result = b.build();

    string doc = result->to_document();
    EXPECT_TRUE(doc.contains("special"));
}

TEST_F(YamlBuilderTest, BuildManyKeysSortedOutput) {
    yaml_builder b;
    b.key("zebra").value(1).key("apple").value(2).key("mango").value(3);

    auto result = b.build();
    string doc = result->to_document();

    auto pos_apple = doc.find("apple");
    auto pos_mango = doc.find("mango");
    auto pos_zebra = doc.find("zebra");
    EXPECT_LT(pos_apple, pos_mango);
    EXPECT_LT(pos_mango, pos_zebra);
}

TEST_F(YamlBuilderTest, BuildNumericEdgeCases) {
    yaml_builder b;
    b.key("max_int64")
            .value(static_cast<int64_t>(INT64_MAX))
            .key("min_int64")
            .value(static_cast<int64_t>(INT64_MIN))
            .key("zero")
            .value(static_cast<int64_t>(0))
            .key("zero_float")
            .value(0.0)
            .key("negative_float")
            .value(-1.5);
    auto result = b.build();

    auto map = result->as_mapping();
    EXPECT_EQ(map->get_member("max_int64")->as_integer()->get_value(), INT64_MAX);
    EXPECT_EQ(map->get_member("min_int64")->as_integer()->get_value(), INT64_MIN);
    EXPECT_EQ(map->get_member("zero")->as_integer()->get_value(), 0);
    EXPECT_DOUBLE_EQ(map->get_member("zero_float")->as_float()->get_value(), 0.0);
    EXPECT_DOUBLE_EQ(map->get_member("negative_float")->as_float()->get_value(), -1.5);
}

TEST_F(YamlBuilderTest, BuildComplexYaml12Document) {
    yaml_builder b;

    // Scalars
    b.key("string_plain")
            .value("Hello, world!")
            .key("string_quoted")
            .value_string("It's \"quoted\"", yaml_string::SingleQuoted)
            .key("int_value")
            .value(42)
            .key("float_value")
            .value(3.14159)
            .key("bool_true")
            .value(true)
            .key("bool_false")
            .value(false)
            .key("null_value")
            .value(nullptr);

    // Block sequence
    b.key("block_list").begin_block_sequence().value("apple").value("banana").value("carrot").end_sequence();

    // Flow sequence
    b.key("flow_list").begin_flow_sequence().value(1).value(2).value(3).end_sequence();

    // Block mapping
    b.key("person")
            .begin_block_mapping()
            .key("name")
            .value("John Doe")
            .key("age")
            .value(30)
            .key("address")
            .begin_block_mapping()
            .key("city")
            .value("New York")
            .key("zip")
            .value("10001")
            .end_mapping()
            .end_mapping();

    // Flow mapping
    b.key("point").begin_flow_mapping().key("x").value(10).key("y").value(20).end_mapping();

    // Sequence of mappings
    b.key("users")
            .begin_sequence()
            .begin_mapping()
            .key("name")
            .value("Alice")
            .key("role")
            .value("admin")
            .end_mapping()
            .begin_mapping()
            .key("name")
            .value("Bob")
            .key("role")
            .value("user")
            .end_mapping()
            .end_sequence();

    // Anchor and tag
    b.key("shared_config").anchor("cfg").tag("!config").begin_mapping().key("timeout").value(30).end_mapping();

    auto built = b.build();
    string doc = built->to_document();

    // Parse back and verify
    yaml_parser parser(doc);
    auto parsed = parser.parse();
    auto map = parsed->as_mapping();

    ASSERT_NE(map, nullptr);
    EXPECT_EQ(map->get_member("string_plain")->as_string()->get_value(), "Hello, world!");
    EXPECT_EQ(map->get_member("int_value")->as_integer()->get_value(), 42);
    EXPECT_DOUBLE_EQ(map->get_member("float_value")->as_float()->get_value(), 3.14159);
    EXPECT_TRUE(map->get_member("bool_true")->as_boolean()->get_value());
    EXPECT_TRUE(map->get_member("null_value")->is_null());

    auto block_list = map->get_member("block_list")->as_sequence();
    ASSERT_EQ(block_list->size(), 3);

    auto flow_list = map->get_member("flow_list")->as_sequence();
    ASSERT_EQ(flow_list->size(), 3);
    EXPECT_EQ(flow_list->get_style(), yaml_sequence::Flow);

    auto person = map->get_member("person")->as_mapping();
    EXPECT_EQ(person->get_member("name")->as_string()->get_value(), "John Doe");
    auto address = person->get_member("address")->as_mapping();
    EXPECT_EQ(address->get_member("city")->as_string()->get_value(), "New York");

    auto point = map->get_member("point")->as_mapping();
    EXPECT_EQ(point->get_style(), yaml_mapping::Flow);
    EXPECT_EQ(point->get_member("x")->as_integer()->get_value(), 10);

    auto users = map->get_member("users")->as_sequence();
    ASSERT_EQ(users->size(), 2);

    auto shared = map->get_member("shared_config")->as_mapping();
    EXPECT_EQ(shared->get_member("timeout")->as_integer()->get_value(), 30);
}

TEST_F(YamlBuilderTest, BuildAllStringStyles) {
    yaml_builder b;
    b.key("plain")
            .value_string("plain text", yaml_string::Plain)
            .key("single")
            .value_string("single 'quoted'", yaml_string::SingleQuoted)
            .key("double")
            .value_string("double \"quoted\"", yaml_string::DoubleQuoted)
            .key("literal")
            .value_string("literal\nblock\nstring", yaml_string::Literal)
            .key("folded")
            .value_string("folded block string", yaml_string::Folded);
    auto result = b.build();

    auto map = result->as_mapping();
    EXPECT_EQ(map->get_member("plain")->as_string()->get_style(), yaml_string::Plain);
    EXPECT_EQ(map->get_member("single")->as_string()->get_style(), yaml_string::SingleQuoted);
    EXPECT_EQ(map->get_member("double")->as_string()->get_style(), yaml_string::DoubleQuoted);
    EXPECT_EQ(map->get_member("literal")->as_string()->get_style(), yaml_string::Literal);
    EXPECT_EQ(map->get_member("folded")->as_string()->get_style(), yaml_string::Folded);
}

TEST_F(YamlBuilderTest, RawValueSharedPointer) {
    yaml_builder b;
    b.key("manual").anchor("man").tag("!!str").value_string("manual value", yaml_string::DoubleQuoted);
    auto result = b.build();

    const yaml_value* val = result->as_mapping()->get_member("manual");
    EXPECT_EQ(val->as_string().get_value(), "manual value");
    EXPECT_EQ(val->as_string().get_style(), yaml_string::DoubleQuoted);
    EXPECT_EQ(val->get_anchor(), "man");
    EXPECT_EQ(val->get_tag(), "!!str");
}
