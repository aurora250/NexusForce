#ifndef NEXUSFORCE_TEST_NFRS_CLASSES_HPP__
#define NEXUSFORCE_TEST_NFRS_CLASSES_HPP__
#include <NeForce/core/string/string.hpp>
#include <NeForce/core/async/signals.hpp>
#include <NeForce/core/reflect/reflect_macros.hpp>

using namespace neforce;

// --- enum test ---
enum class NFRSTestColor {
    Red,
    Green,
    Blue
};
NEFORCE_REFLECT_ENUM(NFRSTestColor, int)
NEFORCE_REFLECT_ENUM_VAL(NFRSTestColor, Red)
NEFORCE_REFLECT_ENUM_VAL(NFRSTestColor, Green)
NEFORCE_REFLECT_ENUM_VAL(NFRSTestColor, Blue)

// --- simple class ---
struct NFRSTestSimple {
    NEFORCE_REFLECT_OBJ(NFRSTestSimple)
    NEFORCE_REFLECT_PROP(int, id)
    NEFORCE_REFLECT_PROP(string, name)
    NEFORCE_REFLECT_PROP_ATTR(double, value, PROP_OPTIONAL)

    int id = 0;
    string name;
    double value = 0.0;
};

// --- base class ---
struct NFRSTestBase {
    NEFORCE_REFLECT_OBJ(NFRSTestBase)
    NEFORCE_REFLECT_PROP(int, base_field)

    int base_field = 42;
};

// --- derived class ---
struct NFRSTestDerived : public NFRSTestBase {
    NEFORCE_REFLECT_OBJ(NFRSTestDerived)
    NEFORCE_REFLECT_PROP(int, extra)

    int extra = 0;
};

// --- class with functions ---
struct NFRSTestWithFunc {
    NEFORCE_REFLECT_OBJ(NFRSTestWithFunc)
    NEFORCE_REFLECT_PROP(int, count)
    NEFORCE_REFLECT_FUNC(void, reset)
    NEFORCE_REFLECT_FUNC(int, compute, int, int)
    NEFORCE_REFLECT_FUNC(void, setCount, int)

    int count = 0;
    void reset() { count = 0; }
    int compute(int a, int b) { return a + b; }
    void setCount(int c) { count = c; }
};

// --- class with signal ---
struct NFRSTestWithSignal {
    NEFORCE_REFLECT_OBJ(NFRSTestWithSignal)
    NEFORCE_REFLECT_PROP(int, value)
    NEFORCE_REFLECT_SIGNAL(signal<int>, onChanged)

    int value = 0;
    signal<int> onChanged;
};

// --- class with compound attrs ---
struct NFRSTestWithAttrs {
    NEFORCE_REFLECT_OBJ(NFRSTestWithAttrs)
    NEFORCE_REFLECT_PROP_ATTR(int, required_id, PROP_REQUIRED)
    NEFORCE_REFLECT_PROP_ATTR(double, transient_val, PROP_TRANSIENT)

    int required_id = 0;
    double transient_val = 0.0;
};

#endif // NEXUSFORCE_TEST_NFRS_CLASSES_HPP__
