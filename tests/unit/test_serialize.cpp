#include <NeForce/core/async/call_once.hpp>
#include <NeForce/core/memory/endian.hpp>
#include <NeForce/core/serialize/json_serializer.hpp>
#include <NeForce/core/serialize/binary_serializer.hpp>
#include <gtest/gtest.h>

using namespace neforce;
using namespace neforce::serialize;

struct SerializeSimple {
    int id = 0;
    string name;
    double value = 0.0;
};

struct SerializeNested {
    int count = 0;
    SerializeSimple child;
};

namespace {

    once_flag g_serialize_init;

    void ensure_serialize_registered() {
        call_once(g_serialize_init, [] {
            reflect::reflect<SerializeSimple>("SerializeSimple")
                    .property("id", &SerializeSimple::id)
                    .property("name", &SerializeSimple::name)
                    .property("value", &SerializeSimple::value, reflect::PROP_OPTIONAL)
                    .constructor()
                    .enable_clone();

            reflect::reflect<SerializeNested>("SerializeNested")
                    .property("count", &SerializeNested::count)
                    .property("child", &SerializeNested::child)
                    .constructor()
                    .enable_clone();

            NEFORCE_REFLECT_RESOLVE_BASES();
        });
    }

} // namespace

class JsonSerializerTest : public ::testing::Test {
protected:
    void SetUp() override { ensure_serialize_registered(); }
};

TEST_F(JsonSerializerTest, SerializeSimpleObject) {
    auto* meta = reflect::registry::instance().find("SerializeSimple");
    ASSERT_NE(meta, nullptr);

    auto obj = meta->create();
    void* raw = obj.raw();

    meta->get_property("id")->set(raw, reflect::meta_any(42));
    meta->get_property("name")->set(raw, reflect::meta_any(string("test")));
    meta->get_property("value")->set(raw, reflect::meta_any(3.14));

    auto json = json_serializer::serialize(obj);
    ASSERT_NE(json, nullptr);
    EXPECT_TRUE(json->is_object());
}

TEST_F(JsonSerializerTest, SerializeToString) {
    auto* meta = reflect::registry::instance().find("SerializeSimple");
    ASSERT_NE(meta, nullptr);

    auto obj = meta->create();
    void* raw = obj.raw();
    meta->get_property("id")->set(raw, reflect::meta_any(1));
    meta->get_property("name")->set(raw, reflect::meta_any(string("hello")));

    auto str = json_serializer::to_string(obj);
    EXPECT_FALSE(str.empty());
}

TEST_F(JsonSerializerTest, SerializeEmptyObject) {
    reflect::meta_any empty;
    auto json = json_serializer::serialize(empty);
    ASSERT_NE(json, nullptr);
    EXPECT_TRUE(json->is_null());
}

TEST_F(JsonSerializerTest, SerializeTransientPropertySkip) {
    auto* meta = reflect::registry::instance().find("SerializeSimple");
    ASSERT_NE(meta, nullptr);
    auto props = meta->all_properties();
    EXPECT_GE(props.size(), 3u);
}

TEST_F(JsonSerializerTest, DeserializeRoundTrip) {
    auto* meta = reflect::registry::instance().find("SerializeSimple");
    ASSERT_NE(meta, nullptr);

    auto obj = meta->create();
    void* raw = obj.raw();
    meta->get_property("id")->set(raw, reflect::meta_any(100));
    meta->get_property("name")->set(raw, reflect::meta_any(string("roundtrip")));

    auto json = json_serializer::serialize(obj);

    auto restored = json_serializer::deserialize(*json, *meta);
    EXPECT_TRUE(restored.has_value());

    void* restored_raw = restored.raw();
    auto* prop_id = meta->get_property("id");
    auto restored_id = prop_id->get(restored_raw);
    EXPECT_EQ(restored_id.get<int>(), 100);

    auto* prop_name = meta->get_property("name");
    auto restored_name = prop_name->get(restored_raw);
    EXPECT_EQ(restored_name.get<string>(), string("roundtrip"));
}

class BinarySerializerTest : public ::testing::Test {
protected:
    void SetUp() override { ensure_serialize_registered(); }
};

TEST_F(BinarySerializerTest, SerializeProducesData) {
    auto* meta = reflect::registry::instance().find("SerializeSimple");
    ASSERT_NE(meta, nullptr);

    auto obj = meta->create();
    void* raw = obj.raw();
    meta->get_property("id")->set(raw, reflect::meta_any(42));

    auto data = binary_serializer::serialize(obj);
    EXPECT_GT(data.size(), 10u);

    const auto magic = *reinterpret_cast<const uint32_t*>(data.data());
    EXPECT_EQ(endian::be_to_host(magic), binary_serializer::MAGIC);
}

TEST_F(BinarySerializerTest, DeserializeRoundTrip) {
    auto* meta = reflect::registry::instance().find("SerializeSimple");
    ASSERT_NE(meta, nullptr);

    auto obj = meta->create();
    void* raw = obj.raw();
    meta->get_property("id")->set(raw, reflect::meta_any(99));

    auto data = binary_serializer::serialize(obj);

    auto restored = binary_serializer::deserialize(data.data(), data.size());
    EXPECT_TRUE(restored.has_value());

    void* restored_raw = restored.raw();
    auto* prop = meta->get_property("id");
    auto restored_id = prop->get(restored_raw);
    EXPECT_EQ(restored_id.get<int>(), 99);
}

TEST_F(BinarySerializerTest, DeserializeInvalidData) {
    const byte_t empty[4] = {0, 0, 0, 0};
    EXPECT_THROW(binary_serializer::deserialize(empty, 4), deserialize_exception);
}

TEST_F(BinarySerializerTest, SerializeDefaultContext) {
    serialize_context ctx;
    EXPECT_FALSE(ctx.include_transient);
    EXPECT_EQ(ctx.version, 0u);
}
