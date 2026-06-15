#include "nfrs_test_data/test_classes.hpp"
#include <NeForce/core/async/signals.hpp>
#include <NeForce/core/reflect/reflect.hpp>
#include <gtest/gtest.h>

using namespace neforce;
using namespace neforce::reflect;

TEST(NFRSIntegration, SimpleClassRegistered) {
    auto* meta = registry::instance().find("NFRSTestSimple");
    ASSERT_NE(meta, nullptr);
    EXPECT_EQ(meta->name(), string_view("NFRSTestSimple"));
    EXPECT_GT(meta->size(), 0u);
}

TEST(NFRSIntegration, SimpleClassPropertiesExist) {
    auto* meta = registry::instance().find("NFRSTestSimple");
    ASSERT_NE(meta, nullptr);

    auto* prop_id = meta->get_property("id");
    ASSERT_NE(prop_id, nullptr);
    EXPECT_EQ(prop_id->name(), string_view("id"));

    auto* prop_name = meta->get_property("name");
    ASSERT_NE(prop_name, nullptr);
    EXPECT_EQ(prop_name->name(), string_view("name"));

    auto* prop_value = meta->get_property("value");
    ASSERT_NE(prop_value, nullptr);
    EXPECT_EQ(prop_value->name(), string_view("value"));
}

TEST(NFRSIntegration, SimpleClassCreateAndGetSet) {
    auto* meta = registry::instance().find("NFRSTestSimple");
    ASSERT_NE(meta, nullptr);

    auto obj = meta->create();
    ASSERT_TRUE(obj.has_value());

    void* raw = obj.raw();
    ASSERT_NE(raw, nullptr);

    auto* prop_id = meta->get_property("id");
    prop_id->set(raw, meta_any(42));
    auto val = prop_id->get(raw);
    EXPECT_EQ(val.get<int>(), 42);
}

TEST(NFRSIntegration, SimpleClassPropertyAttrs) {
    auto* meta = registry::instance().find("NFRSTestSimple");
    ASSERT_NE(meta, nullptr);

    auto* prop_value = meta->get_property("value");
    ASSERT_NE(prop_value, nullptr);
    EXPECT_TRUE(prop_value->is_optional());
}

TEST(NFRSIntegration, BaseClassRegistered) {
    auto* meta = registry::instance().find("NFRSTestBase");
    ASSERT_NE(meta, nullptr);

    auto* prop = meta->get_property("base_field");
    ASSERT_NE(prop, nullptr);
}

TEST(NFRSIntegration, DerivedClassRegistered) {
    auto* derived = registry::instance().find("NFRSTestDerived");
    ASSERT_NE(derived, nullptr);
    EXPECT_TRUE(derived->is_derived_from("NFRSTestBase"));
}

TEST(NFRSIntegration, DerivedClassHasAllProperties) {
    auto* derived = registry::instance().find("NFRSTestDerived");
    ASSERT_NE(derived, nullptr);

    auto props = derived->all_properties();
    EXPECT_GE(props.size(), 2u);

    auto* extra = derived->get_property("extra");
    ASSERT_NE(extra, nullptr);
    EXPECT_EQ(extra->name(), string_view("extra"));
}

TEST(NFRSIntegration, ClassWithFuncRegistered) {
    auto* meta = registry::instance().find("NFRSTestWithFunc");
    ASSERT_NE(meta, nullptr);

    auto* prop_count = meta->get_property("count");
    ASSERT_NE(prop_count, nullptr);

    auto* func_reset = meta->get_function("reset");
    EXPECT_NE(func_reset, nullptr);

    auto* func_compute = meta->get_function("compute");
    EXPECT_NE(func_compute, nullptr);
}

TEST(NFRSIntegration, EnumRegistered) {
    auto* meta = registry::instance().find("NFRSTestColor");
    ASSERT_NE(meta, nullptr);
    EXPECT_TRUE(meta->is_enum());
    EXPECT_NE(meta->enum_info(), nullptr);
}

TEST(NFRSIntegration, EnumValueLookup) {
    auto* meta = registry::instance().find("NFRSTestColor");
    ASSERT_NE(meta, nullptr);
    auto* ei = meta->enum_info();
    ASSERT_NE(ei, nullptr);

    EXPECT_EQ(ei->entries().size(), 3u);

    int64_t val = 0;
    EXPECT_TRUE(ei->value_of("Red", val));
    EXPECT_EQ(val, static_cast<int64_t>(NFRSTestColor::Red));

    EXPECT_EQ(ei->name_of(static_cast<int64_t>(NFRSTestColor::Green)), string_view("Green"));
}

TEST(NFRSIntegration, SignalNameRegistered) {
    auto* meta = registry::instance().find("NFRSTestWithSignal");
    ASSERT_NE(meta, nullptr);

    const auto& sigs = meta->signal_names();
    bool found = false;
    for (const auto& s: sigs) {
        if (s == "onChanged") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST(NFRSIntegration, SignalBaseTypeErasure) {
    NFRSTestWithSignal obj;
    signal_base* base = &obj.onChanged;
    ASSERT_NE(base, nullptr);
    EXPECT_FALSE(base->is_blocked());
    EXPECT_EQ(base->slot_count(), 0u);

    base->block();
    EXPECT_TRUE(base->is_blocked());
    base->unblock();
    EXPECT_FALSE(base->is_blocked());
}

TEST(NFRSIntegration, FunctionInvoke) {
    auto* meta = registry::instance().find("NFRSTestWithFunc");
    ASSERT_NE(meta, nullptr);

    NFRSTestWithFunc obj;
    obj.count = 10;

    auto* func_reset = meta->get_function("reset");
    ASSERT_NE(func_reset, nullptr);
    func_reset->invoke(&obj);
    EXPECT_EQ(obj.count, 0);

    auto* func_compute = meta->get_function("compute");
    ASSERT_NE(func_compute, nullptr);
    vector<meta_any> args;
    args.emplace_back(3);
    args.emplace_back(4);
    auto result = func_compute->invoke(&obj, args);
    EXPECT_EQ(result.get<int>(), 7);
}

TEST(NFRSIntegration, ClassWithAttrsRegistered) {
    auto* meta = registry::instance().find("NFRSTestWithAttrs");
    ASSERT_NE(meta, nullptr);

    auto* prop_req = meta->get_property("required_id");
    ASSERT_NE(prop_req, nullptr);
    EXPECT_TRUE(prop_req->is_required());

    auto* prop_trans = meta->get_property("transient_val");
    ASSERT_NE(prop_trans, nullptr);
    EXPECT_TRUE(prop_trans->is_transient());
}

TEST(NFRSIntegration, DynamicConnectSignalToSlot) {
    NFRSTestWithSignal sender;
    NFRSTestWithFunc receiver;
    receiver.count = 100;

    auto* sender_meta = registry::instance().find("NFRSTestWithSignal");
    if (!sender_meta) {
        FAIL() << "NFRSTestWithSignal not registered";
        return;
    }

    auto* receiver_meta = registry::instance().find("NFRSTestWithFunc");
    if (!receiver_meta) {
        FAIL() << "NFRSTestWithFunc not registered";
        return;
    }

    auto* slot = receiver_meta->get_function("setCount");
    if (!slot) {
        FAIL() << "setCount function not found";
        return;
    }

    signal_base* psig = &sender.onChanged;
    bool ok = registry::connect_signal_to_slot(psig, slot, &receiver);
    EXPECT_TRUE(ok);

    sender.onChanged.emit(42);
    EXPECT_EQ(receiver.count, 42);
}

TEST(NFRSIntegration, DynamicProperty) {
    auto* meta = registry::instance().find("NFRSTestSimple");
    ASSERT_NE(meta, nullptr);

    meta_property::getter get = [](const void*) -> meta_any { return meta_any(99); };
    meta_property::setter set = [](void*, const meta_any&) {};
    auto dyn_prop = make_unique<meta_property>("dynamic_val", type_id_for<int>(), move(get), move(set), PROP_NONE);
    meta->add_property("dynamic_val", move(dyn_prop));

    auto* prop = meta->get_property("dynamic_val");
    ASSERT_NE(prop, nullptr);
    EXPECT_EQ(prop->name(), string_view("dynamic_val"));

    NFRSTestSimple obj;
    EXPECT_EQ(prop->get(&obj).get<int>(), 99);
}
