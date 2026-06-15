#include <NeForce/core/async/call_once.hpp>
#include <NeForce/core/reflect/reflect.hpp>
#include <gtest/gtest.h>

using namespace neforce;
using namespace neforce::reflect;

struct TestSimple {
    int id = 0;
    string name;
    double value = 0.0;
};

struct TestBase {
    int base_field = 42;
};

struct TestDerived : TestBase {
    int extra = 0;
};

enum class TestColor {
    Red,
    Green,
    Blue
};

namespace {

    once_flag g_reflect_init;

    void ensure_reflect_registered() {
        call_once(g_reflect_init, [] {
            reflect::reflect<TestSimple>("TestSimple")
                    .property("id", &TestSimple::id)
                    .property("name", &TestSimple::name)
                    .property("value", &TestSimple::value)
                    .constructor()
                    .enable_clone();

            reflect::reflect<TestBase>("TestBase").property("base_field", &TestBase::base_field).constructor();

            reflect::reflect<TestDerived>("TestDerived")
                    .base("TestBase")
                    .property("extra", &TestDerived::extra)
                    .constructor();

            auto& meta = reflect::registry::instance().register_type<TestColor>("TestColor");
            auto enum_info = make_unique<reflect::meta_enum>("TestColor", type_id_for<int>());
            enum_info->add_entry("Red", static_cast<int64_t>(TestColor::Red));
            enum_info->add_entry("Green", static_cast<int64_t>(TestColor::Green));
            enum_info->add_entry("Blue", static_cast<int64_t>(TestColor::Blue));
            meta.enum_info(move(enum_info));

            NEFORCE_REFLECT_RESOLVE_BASES();
        });
    }

} // namespace

class ReflectRegistryTest : public ::testing::Test {
protected:
    void SetUp() override { ensure_reflect_registered(); }
};

TEST(ReflectMetaAny, DefaultConstructorEmpty) {
    meta_any a;
    EXPECT_FALSE(a.has_value());
    EXPECT_EQ(a.type_id(), 0u);
}

TEST(ReflectMetaAny, ConstructInt) {
    meta_any a(42);
    EXPECT_TRUE(a.has_value());
    EXPECT_NE(a.type_id(), 0u);
    EXPECT_EQ(a.type_id(), type_id_for<int>());
}

TEST(ReflectMetaAny, ConstructDouble) {
    meta_any a(3.14);
    EXPECT_TRUE(a.has_value());
    EXPECT_EQ(a.type_id(), type_id_for<double>());
}

TEST(ReflectMetaAny, ConstructString) {
    meta_any a(string("hello"));
    EXPECT_TRUE(a.has_value());
}

TEST(ReflectMetaAny, CastIntValid) {
    meta_any a(42);
    auto* p = a.cast<int>();
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(*p, 42);
}

TEST(ReflectMetaAny, CastIntInvalidType) {
    meta_any a(42);
    auto* p = a.cast<double>();
    EXPECT_EQ(p, nullptr);
}

TEST(ReflectMetaAny, GetInt) {
    meta_any a(100);
    EXPECT_EQ(a.get<int>(), 100);
}

TEST(ReflectMetaAny, ConvertReturnsCopy) {
    meta_any a(50);
    int val = a.convert<int>();
    EXPECT_EQ(val, 50);
}

TEST(ReflectMetaAny, CanCastTrue) {
    meta_any a(3.14);
    EXPECT_TRUE(a.can_cast<double>());
    EXPECT_FALSE(a.can_cast<int>());
}

TEST(ReflectMetaAny, CopyConstruct) {
    meta_any a(42);
    meta_any b(a);
    EXPECT_TRUE(b.has_value());
    EXPECT_EQ(b.get<int>(), 42);
    EXPECT_EQ(b.type_id(), a.type_id());
}

TEST(ReflectMetaAny, CopyAssignment) {
    meta_any a(42);
    meta_any b;
    b = a;
    EXPECT_TRUE(b.has_value());
    EXPECT_EQ(b.get<int>(), 42);
}

TEST(ReflectMetaAny, MoveConstruct) {
    meta_any a(string("hello"));
    meta_any b(move(a));
    EXPECT_TRUE(b.has_value());
}

TEST(ReflectMetaAny, MoveAssignment) {
    meta_any a(3.14);
    meta_any b;
    b = move(a);
    EXPECT_TRUE(b.has_value());
    EXPECT_TRUE(b.can_cast<double>());
}

TEST(ReflectMetaAny, ResetClearsValue) {
    meta_any a(42);
    a.reset();
    EXPECT_FALSE(a.has_value());
    EXPECT_EQ(a.type_id(), 0u);
}

TEST(ReflectMetaAny, Swap) {
    meta_any a(42);
    meta_any b(3.14);
    swap(a, b);
    EXPECT_EQ(a.get<double>(), 3.14);
    EXPECT_EQ(b.get<int>(), 42);
}

TEST(ReflectMetaAny, SboSmallType) {
    meta_any a(42);
    EXPECT_TRUE(a.has_value());
    auto* p = a.cast<int>();
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(*p, 42);
}

TEST_F(ReflectRegistryTest, RegisterType) {
    auto* meta = reflect::registry::instance().find("TestSimple");
    ASSERT_NE(meta, nullptr);
    EXPECT_EQ(meta->name(), string_view("TestSimple"));
    EXPECT_GT(meta->size(), 0u);
}

TEST_F(ReflectRegistryTest, TypeProperties) {
    auto* meta = reflect::registry::instance().find("TestSimple");
    ASSERT_NE(meta, nullptr);

    auto* prop_id = meta->get_property("id");
    ASSERT_NE(prop_id, nullptr);
    EXPECT_EQ(prop_id->name(), string_view("id"));

    auto* prop_name = meta->get_property("name");
    ASSERT_NE(prop_name, nullptr);
    EXPECT_EQ(prop_name->name(), string_view("name"));

    auto* prop_value = meta->get_property("value");
    ASSERT_NE(prop_value, nullptr);
}

TEST_F(ReflectRegistryTest, TypeCreate) {
    auto* meta = reflect::registry::instance().find("TestSimple");
    ASSERT_NE(meta, nullptr);

    auto obj = meta->create();
    EXPECT_TRUE(obj.has_value());
    EXPECT_EQ(obj.type_id(), meta->type_id());
}

TEST_F(ReflectRegistryTest, PropertyGetSet) {
    auto* meta = reflect::registry::instance().find("TestSimple");
    ASSERT_NE(meta, nullptr);

    auto obj = meta->create();
    auto* raw = obj.raw();
    ASSERT_NE(raw, nullptr);

    auto* prop_id = meta->get_property("id");
    ASSERT_NE(prop_id, nullptr);

    prop_id->set(raw, meta_any(123));
    auto val = prop_id->get(raw);
    EXPECT_TRUE(val.has_value());
    EXPECT_EQ(val.get<int>(), 123);
}

TEST_F(ReflectRegistryTest, InheritanceCheck) {
    auto* derived = reflect::registry::instance().find("TestDerived");
    ASSERT_NE(derived, nullptr);
    EXPECT_TRUE(derived->is_derived_from("TestBase"));
}

TEST_F(ReflectRegistryTest, AllPropertiesIncludesInherited) {
    auto* derived = reflect::registry::instance().find("TestDerived");
    ASSERT_NE(derived, nullptr);

    auto props = derived->all_properties();
    EXPECT_GE(props.size(), 2u);
}

TEST_F(ReflectRegistryTest, CloneSupport) {
    auto* meta = reflect::registry::instance().find("TestSimple");
    ASSERT_NE(meta, nullptr);

    auto obj = meta->create();
    void* raw = obj.raw();
    auto* prop_id = meta->get_property("id");
    prop_id->set(raw, meta_any(42));

    auto cloned = meta->clone(raw);
    EXPECT_TRUE(cloned.has_value());
    EXPECT_EQ(cloned.type_id(), meta->type_id());

    auto* cloned_raw = cloned.raw();
    auto* cloned_prop = meta->get_property("id");
    auto cloned_val = cloned_prop->get(cloned_raw);
    EXPECT_EQ(cloned_val.get<int>(), 42);
}

TEST_F(ReflectRegistryTest, EnumValueLookup) {
    auto* color_meta = reflect::registry::instance().find("TestColor");
    ASSERT_NE(color_meta, nullptr);

    auto* enum_info = color_meta->enum_info();
    ASSERT_NE(enum_info, nullptr);

    int64_t red_val = 0;
    EXPECT_TRUE(enum_info->value_of("Red", red_val));

    auto red_name = enum_info->name_of(red_val);
    EXPECT_EQ(red_name, string_view("Red"));
}

TEST_F(ReflectRegistryTest, EnumEntriesCount) {
    auto* color_meta = reflect::registry::instance().find("TestColor");
    ASSERT_NE(color_meta, nullptr);

    auto* enum_info = color_meta->enum_info();
    ASSERT_NE(enum_info, nullptr);

    EXPECT_EQ(enum_info->entries().size(), 3u);
}

TEST_F(ReflectRegistryTest, TransientCheck) {
    auto* meta = reflect::registry::instance().find("TestSimple");
    ASSERT_NE(meta, nullptr);

    auto* prop = meta->get_property("id");
    ASSERT_NE(prop, nullptr);
    EXPECT_FALSE(prop->is_transient());
    EXPECT_FALSE(prop->is_required());
    EXPECT_FALSE(prop->is_readonly());
}
