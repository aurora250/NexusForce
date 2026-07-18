#include <NeForce/core/functional/apply.hpp>
#include <NeForce/core/functional/bind.hpp>
#include <NeForce/core/functional/function.hpp>
#include <NeForce/core/functional/hash.hpp>
#include <NeForce/core/memory/shared_ptr.hpp>
#include <NeForce/core/string/string.hpp>
#include <NeForce/core/container/vector.hpp>
#include <NeForce/core/utility/packages.hpp>
#include <gtest/gtest.h>
using namespace neforce;
using namespace neforce::placeholders;

namespace {
    struct test_functor {
        int operator()(int x) const { return x * 2; }
    };

    struct test_functor_with_state {
        int multiplier;
        int operator()(int x) const { return x * multiplier; }
    };

    int free_function(int x) { return x + 1; }
    int free_function_noexcept(int x) noexcept { return x + 10; }

    int free_func(int a, int b, int c) { return a + b + c; }
    void void_free_func(int& out, int a, int b) { out = a + b; }
    string string_free_func(const string& a, const string& b) { return a + b; }

    struct apply_functor_mutable {
        int counter = 0;
        int operator()(int x) { return x + counter++; }
    };

    struct apply_move_only_functor {
        unique_ptr<int> ptr;
        int operator()(int x) && { return x + *ptr; }
    };

    int free_add(int a, int b) { return a + b; }

    int free_add_three(int a, int b, int c) { return a + b + c; }

    string free_concat(const string& a, const string& b) { return a + b; }

    int free_increment(int& x) { return ++x; }

    void free_set(int& out, int val) { out = val; }

    int free_func2(int a, int b) { return a + b; }

    int free_func_noexcept2(int a, int b) noexcept { return a + b; }

    void void_free_func2(int& out, int val) { out = val; }

    struct bind_test_struct {
        int value;
        bind_test_struct(int v) :
        value(v) {}
        int multiply(int x) const { return value * x; }
        int add(int x) { return value + x; }
        void set_value(int x) { value = x; }
    };

    struct bind_call_counter {
        mutable int count = 0;
        int operator()(int a, int b) {
            count++;
            return a + b;
        }
        int operator()(int a, int b) const {
            count++;
            return a + b;
        }
    };

    struct invoke_functor {
        int operator()(int a, int b) const { return a * b; }
    };

    struct invoke_functor_noexcept {
        int operator()(int a, int b) const noexcept { return a + b; }
    };

    struct invoke_overloaded_functor {
        int operator()(int a) const { return a * 2; }
        double operator()(double a) const { return a * 3.0; }
    };
} // namespace

class FunctionTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(FunctionTest, DefaultConstructor) {
    function<int(int)> f;
    EXPECT_FALSE(static_cast<bool>(f));
    EXPECT_TRUE(f == nullptr);
    EXPECT_TRUE(nullptr == f);
    EXPECT_FALSE(f != nullptr);
    EXPECT_FALSE(nullptr != f);
}

TEST_F(FunctionTest, NullptrConstructor) {
    function<int(int)> f(nullptr);
    EXPECT_FALSE(static_cast<bool>(f));
}

TEST_F(FunctionTest, ConstructorFromFreeFunction) {
    function<int(int)> f(free_function);
    EXPECT_TRUE(static_cast<bool>(f));
    EXPECT_EQ(f(5), 6);
}

TEST_F(FunctionTest, ConstructorFromLambda) {
    auto lambda = [](int x) { return x * 3; };
    function<int(int)> f(lambda);
    EXPECT_TRUE(static_cast<bool>(f));
    EXPECT_EQ(f(4), 12);
}

TEST_F(FunctionTest, ConstructorFromFunctor) {
    test_functor functor;
    function<int(int)> f(functor);
    EXPECT_TRUE(static_cast<bool>(f));
    EXPECT_EQ(f(7), 14);
}

TEST_F(FunctionTest, ConstructorFromFunctorWithState) {
    test_functor_with_state functor{5};
    function<int(int)> f(functor);
    EXPECT_TRUE(static_cast<bool>(f));
    EXPECT_EQ(f(3), 15);
}

TEST_F(FunctionTest, ConstructorFromLargeFunctor) {
    struct test_large_functor {
        char data[128];
        int operator()(int x) const { return x + static_cast<int>(data[0]); }
    };
    test_large_functor functor;
    functor.data[0] = 3;
    function<int(int)> f(functor);
    EXPECT_TRUE(static_cast<bool>(f));
    EXPECT_EQ(f(5), 8);
}

TEST_F(FunctionTest, ConstructorFromReferenceWrapper) {
    test_functor_with_state functor{4};
    function<int(int)> f(ref(functor));
    EXPECT_TRUE(static_cast<bool>(f));
    EXPECT_EQ(f(3), 12);
    functor.multiplier = 10;
    EXPECT_EQ(f(3), 30);
}

TEST_F(FunctionTest, ConstructorNoexceptFreeFunction) {
    function<int(int)> f(free_function_noexcept);
    EXPECT_TRUE(static_cast<bool>(f));
    EXPECT_EQ(f(7), 17);
}

TEST_F(FunctionTest, CopyConstructor) {
    function<int(int)> f1(free_function);
    function<int(int)> f2(f1);
    EXPECT_TRUE(static_cast<bool>(f2));
    EXPECT_EQ(f2(3), 4);
}

TEST_F(FunctionTest, CopyConstructorFromEmpty) {
    function<int(int)> f1;
    function<int(int)> f2(f1);
    EXPECT_FALSE(static_cast<bool>(f2));
}

TEST_F(FunctionTest, MoveConstructor) {
    function<int(int)> f1(free_function);
    function<int(int)> f2(move(f1));
    EXPECT_TRUE(static_cast<bool>(f2));
    EXPECT_EQ(f2(3), 4);
    EXPECT_FALSE(static_cast<bool>(f1));
}

TEST_F(FunctionTest, MoveConstructorFromEmpty) {
    function<int(int)> f1;
    function<int(int)> f2(move(f1));
    EXPECT_FALSE(static_cast<bool>(f2));
    EXPECT_FALSE(static_cast<bool>(f1));
}

TEST_F(FunctionTest, CopyAssignment) {
    function<int(int)> f1(free_function);
    function<int(int)> f2;
    f2 = f1;
    EXPECT_TRUE(static_cast<bool>(f2));
    EXPECT_EQ(f2(4), 5);
}

TEST_F(FunctionTest, CopyAssignmentSelf) {
    function<int(int)> f(free_function);
    f = f;
    EXPECT_TRUE(static_cast<bool>(f));
    EXPECT_EQ(f(4), 5);
}

TEST_F(FunctionTest, MoveAssignment) {
    function<int(int)> f1(free_function);
    function<int(int)> f2;
    f2 = move(f1);
    EXPECT_TRUE(static_cast<bool>(f2));
    EXPECT_EQ(f2(4), 5);
    EXPECT_FALSE(static_cast<bool>(f1));
}

TEST_F(FunctionTest, MoveAssignmentSelf) {
    function<int(int)> f(free_function);
    f = move(f);
    EXPECT_TRUE(static_cast<bool>(f));
    EXPECT_EQ(f(4), 5);
}

TEST_F(FunctionTest, NullptrAssignment) {
    function<int(int)> f(free_function);
    f = nullptr;
    EXPECT_FALSE(static_cast<bool>(f));
}

TEST_F(FunctionTest, FunctorAssignment) {
    function<int(int)> f;
    f = test_functor{};
    EXPECT_TRUE(static_cast<bool>(f));
    EXPECT_EQ(f(5), 10);
}

TEST_F(FunctionTest, LambdaAssignment) {
    function<int(int)> f;
    f = [](int x) { return x + 5; };
    EXPECT_TRUE(static_cast<bool>(f));
    EXPECT_EQ(f(10), 15);
}

TEST_F(FunctionTest, ReferenceWrapperAssignment) {
    test_functor_with_state functor{3};
    function<int(int)> f;
    f = ref(functor);
    EXPECT_TRUE(static_cast<bool>(f));
    EXPECT_EQ(f(5), 15);
    functor.multiplier = 7;
    EXPECT_EQ(f(5), 35);
}

TEST_F(FunctionTest, BoolConversionNonEmpty) {
    function<int(int)> f(free_function);
    EXPECT_TRUE(static_cast<bool>(f));
}

TEST_F(FunctionTest, BoolConversionEmpty) {
    function<int(int)> f;
    EXPECT_FALSE(static_cast<bool>(f));
}

TEST_F(FunctionTest, InvokeFreeFunction) {
    function<int(int)> f(free_function);
    EXPECT_EQ(f(100), 101);
}

TEST_F(FunctionTest, InvokeLambda) {
    function<int(int, int)> f([](int a, int b) { return a + b; });
    EXPECT_EQ(f(3, 7), 10);
}

TEST_F(FunctionTest, InvokeVoidFunction) {
    int result = 0;
    function<void()> f([&result]() { result = 99; });
    f();
    EXPECT_EQ(result, 99);
}

TEST_F(FunctionTest, InvokeEmptyThrows) {
    function<int(int)> f;
    EXPECT_THROW(f(5), memory_exception);
}

TEST_F(FunctionTest, InvokeAfterMoveAssignment) {
    function<int(int)> f1([](int x) { return x * 2; });
    function<int(int)> f2;
    f2 = move(f1);
    EXPECT_EQ(f2(5), 10);
}

TEST_F(FunctionTest, InvokeAfterCopyAssignment) {
    function<int(int)> f1([](int x) { return x * 3; });
    function<int(int)> f2;
    f2 = f1;
    EXPECT_EQ(f2(6), 18);
}

TEST_F(FunctionTest, InvokeWithMultipleArgs) {
    function<int(int, int, int)> f([](int a, int b, int c) { return a + b + c; });
    EXPECT_EQ(f(1, 2, 3), 6);
}

TEST_F(FunctionTest, InvokeWithReferenceArg) {
    function<void(int&)> f([](int& x) { x = 100; });
    int val = 0;
    f(val);
    EXPECT_EQ(val, 100);
}

TEST_F(FunctionTest, InvokeWithConstReferenceArg) {
    function<int(const int&)> f([](const int& x) { return x * 2; });
    EXPECT_EQ(f(25), 50);
}

TEST_F(FunctionTest, InvokeWithRvalueReferenceArg) {
    function<int(int&&)> f([](int&& x) { return x + 1; });
    EXPECT_EQ(f(5), 6);
}

TEST_F(FunctionTest, Swap) {
    function<int(int)> f1([](int x) { return x + 1; });
    function<int(int)> f2([](int x) { return x + 2; });
    f1.swap(f2);
    EXPECT_EQ(f1(5), 7);
    EXPECT_EQ(f2(5), 6);
}

TEST_F(FunctionTest, SwapWithEmpty) {
    function<int(int)> f1(free_function);
    function<int(int)> f2;
    f1.swap(f2);
    EXPECT_FALSE(static_cast<bool>(f1));
    EXPECT_TRUE(static_cast<bool>(f2));
    EXPECT_EQ(f2(3), 4);
}

TEST_F(FunctionTest, SwapBothEmpty) {
    function<int(int)> f1;
    function<int(int)> f2;
    f1.swap(f2);
    EXPECT_FALSE(static_cast<bool>(f1));
    EXPECT_FALSE(static_cast<bool>(f2));
}

TEST_F(FunctionTest, TargetOnMatchingType) {
    function<int(int)> f(free_function);
    auto* ptr = f.target<int (*)(int)>();
    EXPECT_NE(ptr, nullptr);
    if (ptr) {
        EXPECT_EQ((*ptr)(10), free_function(10));
    }
}

TEST_F(FunctionTest, TargetOnMismatchedType) {
    function<int(int)> f(free_function);
    auto* ptr = f.target<test_functor>();
    EXPECT_EQ(ptr, nullptr);
}

TEST_F(FunctionTest, TargetOnEmptyFunction) {
    function<int(int)> f;
    auto* ptr = f.target<int (*)(int)>();
    EXPECT_EQ(ptr, nullptr);
}

TEST_F(FunctionTest, TargetConstOverload) {
    function<int(int)> f(free_function);
    const auto& cf = f;
    auto* ptr = cf.target<int (*)(int)>();
    EXPECT_NE(ptr, nullptr);
}

TEST_F(FunctionTest, TargetTypeFreeFunction) {
    function<int(int)> f(free_function);
    EXPECT_EQ(f.target_type(), typeid(int (*)(int)));
}

TEST_F(FunctionTest, TargetTypeLambda) {
    auto lambda = [](int x) { return x; };
    function<int(int)> f(lambda);
    EXPECT_EQ(f.target_type(), typeid(decltype(lambda)));
}

TEST_F(FunctionTest, TargetTypeEmptyFunction) {
    function<int(int)> f;
    EXPECT_EQ(f.target_type(), typeid(void));
}

TEST_F(FunctionTest, EqualityOperator) {
    function<int(int)> f1;
    function<int(int)> f2(free_function);
    EXPECT_TRUE(f1 == nullptr);
    EXPECT_TRUE(nullptr == f1);
    EXPECT_FALSE(f2 == nullptr);
    EXPECT_FALSE(nullptr == f2);
}

TEST_F(FunctionTest, InequalityOperator) {
    function<int(int)> f1;
    function<int(int)> f2(free_function);
    EXPECT_FALSE(f1 != nullptr);
    EXPECT_FALSE(nullptr != f1);
    EXPECT_TRUE(f2 != nullptr);
    EXPECT_TRUE(nullptr != f2);
}

TEST_F(FunctionTest, ConstructFromNoexceptLambda) {
    function<int(int)> f([](int x) noexcept { return x * 2; });
    EXPECT_TRUE(static_cast<bool>(f));
    EXPECT_EQ(f(5), 10);
}

TEST_F(FunctionTest, ConstructFromMutableLambda) {
    int counter = 0;
    function<int()> f([counter]() mutable { return ++counter; });
    EXPECT_EQ(f(), 1);
    EXPECT_EQ(f(), 2);
}

TEST_F(FunctionTest, ChainOfOperations) {
    function<int(int)> f1([](int x) { return x + 1; });
    function<int(int)> f2(f1);
    function<int(int)> f3(move(f2));
    function<int(int)> f4;
    f4 = f3;
    function<int(int)> f5;
    f5 = move(f4);
    EXPECT_EQ(f5(10), 11);
    EXPECT_FALSE(static_cast<bool>(f2));
    EXPECT_FALSE(static_cast<bool>(f4));
}

TEST_F(FunctionTest, ReassignWithDifferentType) {
    function<int(int)> f([](int x) { return x + 1; });
    EXPECT_EQ(f(5), 6);
    f = [](int x) { return x * 10; };
    EXPECT_EQ(f(5), 50);
    f = free_function_noexcept;
    EXPECT_EQ(f(5), 15);
}

TEST_F(FunctionTest, ReturnTypeVoid) {
    bool called = false;
    function<void()> f([&called]() { called = true; });
    f();
    EXPECT_TRUE(called);
}

TEST_F(FunctionTest, ReturnTypeReference) {
    int val = 42;
    function<int&()> f([&val]() -> int& { return val; });
    int& result = f();
    EXPECT_EQ(result, 42);
    result = 100;
    EXPECT_EQ(val, 100);
}

TEST_F(FunctionTest, ReturnTypeConstReference) {
    const int val = 42;
    function<const int&()> f([&val]() -> const int& { return val; });
    const int& result = f();
    EXPECT_EQ(result, 42);
}

TEST_F(FunctionTest, MultipleEmptyAssignments) {
    function<int(int)> f(free_function);
    f = nullptr;
    EXPECT_FALSE(static_cast<bool>(f));
    f = nullptr;
    EXPECT_FALSE(static_cast<bool>(f));
}

TEST_F(FunctionTest, NestedFunctionCall) {
    function<int(int)> outer([&](int x) {
        function<int(int)> inner([](int y) { return y * 2; });
        return inner(x);
    });
    EXPECT_EQ(outer(5), 10);
}

TEST_F(FunctionTest, FunctionReturningFunction) {
    function<function<int(int)>()> factory([]() { return function<int(int)>([](int x) { return x * 3; }); });
    auto f = factory();
    EXPECT_EQ(f(4), 12);
}

TEST_F(FunctionTest, ZeroArguments) {
    function<int()> f([]() { return 42; });
    EXPECT_EQ(f(), 42);
}

TEST_F(FunctionTest, MemFnThroughLambda) {
    struct S {
        int value;
        int get() const { return value; }
    };
    S s{99};
    function<int()> f([&s]() { return s.get(); });
    EXPECT_EQ(f(), 99);
}

TEST_F(FunctionTest, LargeNumberOfArgs) {
    function<int(int, int, int, int, int, int, int, int, int, int)> f(
            [](int a, int b, int c, int d, int e, int f1, int g, int h, int i, int j) {
                return a + b + c + d + e + f1 + g + h + i + j;
            });
    EXPECT_EQ(f(1, 2, 3, 4, 5, 6, 7, 8, 9, 10), 55);
}

TEST_F(FunctionTest, PerfectForwardingOfArgs) {
    function<void(unique_ptr<int>)> f([](unique_ptr<int> p) {
        EXPECT_NE(p, nullptr);
        EXPECT_EQ(*p, 42);
    });
    f(make_unique<int>(42));
}

TEST_F(FunctionTest, SwappedFunctionsRetainCorrectBehavior) {
    function<int(int)> f1([](int x) { return x + 10; });
    function<int(int)> f2([](int x) { return x - 5; });

    EXPECT_EQ(f1(20), 30);
    EXPECT_EQ(f2(20), 15);

    f1.swap(f2);

    EXPECT_EQ(f1(20), 15);
    EXPECT_EQ(f2(20), 30);
}

TEST_F(FunctionTest, CopyAfterAssignmentToNull) {
    function<int(int)> f1([](int x) { return x + 1; });
    function<int(int)> f2 = f1;
    f1 = nullptr;
    EXPECT_FALSE(static_cast<bool>(f1));
    EXPECT_TRUE(static_cast<bool>(f2));
    EXPECT_EQ(f2(5), 6);
}

TEST_F(FunctionTest, MoveAfterSwap) {
    function<int(int)> f1([](int x) { return x * 2; });
    function<int(int)> f2;
    f1.swap(f2);
    function<int(int)> f3(move(f2));
    EXPECT_FALSE(static_cast<bool>(f1));
    EXPECT_FALSE(static_cast<bool>(f2));
    EXPECT_TRUE(static_cast<bool>(f3));
    EXPECT_EQ(f3(10), 20);
}

TEST_F(FunctionTest, DestroyCallsDestructor) {
    static int destruction_count = 0;
    struct DestructorTracker {
        int operator()(int x) const { return x; }
        ~DestructorTracker() { destruction_count++; }
    };
    {
        function<int(int)> f(DestructorTracker{});
        EXPECT_EQ(f(1), 1);
    }
    EXPECT_GT(destruction_count, 0);
}

TEST_F(FunctionTest, LambdaCapturingSharedPtr) {
    auto sp = make_shared<int>(100);
    function<int()> f([sp]() { return *sp; });
    EXPECT_EQ(f(), 100);
    *sp = 200;
    EXPECT_EQ(f(), 200);
}

TEST_F(FunctionTest, TargetWithFunctorType) {
    test_functor_with_state functor{7};
    function<int(int)> f(functor);
    auto* ptr = f.target<test_functor_with_state>();
    EXPECT_NE(ptr, nullptr);
    if (ptr) {
        EXPECT_EQ((*ptr)(3), 21);
    }
}

TEST_F(FunctionTest, TargetWithReferenceWrapper) {
    test_functor_with_state functor{7};
    function<int(int)> f(ref(functor));
    auto* ptr = f.target<test_functor_with_state>();
    EXPECT_EQ(ptr, nullptr);
}

TEST_F(FunctionTest, ConstTargetOnNonMatchingTypeReturnsNull) {
    function<int(int)> f([](int x) { return x; });
    const auto& cf = f;
    auto* ptr = cf.target<test_functor>();
    EXPECT_EQ(ptr, nullptr);
}

TEST_F(FunctionTest, TargetOnNullFunction) {
    function<int(int)> f;
    auto* ptr = f.target<void>();
    EXPECT_EQ(ptr, nullptr);
}

TEST_F(FunctionTest, TargetTypeAfterReassign) {
    function<int(int)> f([](int x) { return x; });
    const auto& t1 = f.target_type();
    f = free_function_noexcept;
    const auto& t2 = f.target_type();
    EXPECT_NE(t1, t2);
    EXPECT_EQ(t2, typeid(int (*)(int) noexcept));
}

TEST_F(FunctionTest, CallableWithConversion) {
    struct convertible {
        operator int() const { return 42; }
    };

    struct callable {
        convertible operator()() const { return convertible{}; }
    };

    function<int()> f(callable{});
    EXPECT_EQ(f(), 42);
}

TEST_F(FunctionTest, FunctionWithNoArgsAndVoidReturn) {
    int counter = 0;
    function<void()> f([&counter]() { counter++; });
    f();
    f();
    f();
    EXPECT_EQ(counter, 3);
}

class ApplyTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ApplyTest, ApplyWithFreeFunction) {
    auto t = make_tuple(1, 2, 3);
    int result = apply(free_func, t);
    EXPECT_EQ(result, 6);
}

TEST_F(ApplyTest, ApplyWithEmptyTuple) {
    auto t = make_tuple();
    auto lambda = []() { return 42; };
    int result = apply(lambda, t);
    EXPECT_EQ(result, 42);
}

TEST_F(ApplyTest, ApplyWithSingleElementTuple) {
    auto t = make_tuple(10);
    auto lambda = [](int x) { return x * 2; };
    int result = apply(lambda, t);
    EXPECT_EQ(result, 20);
}

TEST_F(ApplyTest, ApplyWithLambda) {
    auto t = make_tuple(3, 7);
    auto lambda = [](int a, int b) { return a + b; };
    int result = apply(lambda, t);
    EXPECT_EQ(result, 10);
}

TEST_F(ApplyTest, ApplyWithFunctor) {
    struct apply_functor {
        int operator()(int x, int y) const { return x * y; }
    };
    apply_functor f;
    auto t = make_tuple(6, 7);
    int result = apply(f, t);
    EXPECT_EQ(result, 42);
}

TEST_F(ApplyTest, ApplyWithMutableFunctor) {
    apply_functor_mutable f;
    auto t = make_tuple(5);
    int r1 = apply(f, t);
    int r2 = apply(f, t);
    int r3 = apply(f, t);
    EXPECT_EQ(r1, 5);
    EXPECT_EQ(r2, 6);
    EXPECT_EQ(r3, 7);
}

TEST_F(ApplyTest, ApplyWithVoidReturn) {
    int out = 0;
    apply(void_free_func, forward_as_tuple(out, 3, 4));
    EXPECT_EQ(out, 7);
}

TEST_F(ApplyTest, ApplyWithString) {
    auto t = make_tuple(string("Hello, "), string("World!"));
    string result = apply(string_free_func, t);
    EXPECT_EQ(result, "Hello, World!");
}

TEST_F(ApplyTest, ApplyWithLvalueTuple) {
    auto t = make_tuple(10, 20);
    auto lambda = [](int a, int b) { return a + b; };
    int result = apply(lambda, t);
    EXPECT_EQ(result, 30);
}

TEST_F(ApplyTest, ApplyWithConstTuple) {
    const auto t = make_tuple(5, 15);
    auto lambda = [](int a, int b) { return a + b; };
    int result = apply(lambda, t);
    EXPECT_EQ(result, 20);
}

TEST_F(ApplyTest, ApplyWithRvalueTuple) {
    auto lambda = [](int a, int b) { return a + b; };
    int result = apply(lambda, make_tuple(100, 200));
    EXPECT_EQ(result, 300);
}

TEST_F(ApplyTest, ApplyWithTupleOfReferences) {
    int a = 10;
    int b = 20;
    auto t = tie(a, b);
    auto lambda = [](int& x, int& y) {
        x = 100;
        y = 200;
    };
    apply(lambda, t);
    EXPECT_EQ(a, 100);
    EXPECT_EQ(b, 200);
}

TEST_F(ApplyTest, ApplyWithConstTupleOfReferences) {
    int a = 10;
    int b = 20;
    const auto t = tie(a, b);
    auto lambda = [](const int& x, const int& y) { return x + y; };
    int result = apply(lambda, t);
    EXPECT_EQ(result, 30);
}

TEST_F(ApplyTest, ApplyWithMixedTypes) {
    auto t = make_tuple(1, 2.5, string("test"));
    auto lambda = [](int i, double d, const string& s) -> string {
        return s + to_string(i) + to_string(static_cast<int>(d));
    };
    string result = apply(lambda, t);
    EXPECT_EQ(result, "test12");
}

TEST_F(ApplyTest, ApplyWithNonCopyableTypes) {
    auto up1 = make_unique<int>(10);
    auto up2 = make_unique<int>(20);
    auto lambda = [](unique_ptr<int> p1, unique_ptr<int> p2) { return *p1 + *p2; };
    auto t = make_tuple(move(up1), move(up2));
    int result = apply(lambda, move(t));
    EXPECT_EQ(result, 30);
}

TEST_F(ApplyTest, ApplyForwardingLvalueTuplePreservesValues) {
    auto t = make_tuple(10, 20, 30);
    auto lambda = [](int a, int b, int c) { return a * b * c; };
    int result = apply(lambda, t);
    EXPECT_EQ(result, 6000);
}

TEST_F(ApplyTest, ApplyWithTupleSizeOne) {
    auto t = make_tuple(42);
    auto lambda = [](int x) { return x; };
    EXPECT_EQ(apply(lambda, t), 42);
}

TEST_F(ApplyTest, ApplyWithTupleSizeFour) {
    auto t = make_tuple(1, 2, 3, 4);
    auto lambda = [](int a, int b, int c, int d) { return a * 1000 + b * 100 + c * 10 + d; };
    EXPECT_EQ(apply(lambda, t), 1234);
}

TEST_F(ApplyTest, ApplyWithTupleSizeFive) {
    auto t = make_tuple(1, 2, 3, 4, 5);
    auto lambda = [](int a, int b, int c, int d, int e) { return a + b + c + d + e; };
    EXPECT_EQ(apply(lambda, t), 15);
}

TEST_F(ApplyTest, ApplyWithMoveOnlyFunctor) {
    apply_move_only_functor f{make_unique<int>(25)};
    auto t = make_tuple(5);
    int result = apply(move(f), t);
    EXPECT_EQ(result, 30);
}

TEST_F(ApplyTest, ApplyWithVectorElements) {
    auto t = make_tuple(vector<int>{1, 2, 3}, vector<int>{4, 5, 6});
    auto lambda = [](const vector<int>& v1, const vector<int>& v2) { return v1.size() + v2.size(); };
    EXPECT_EQ(apply(lambda, t), 6);
}

TEST_F(ApplyTest, ApplyWithPerfectForwarding) {
    struct tracker {
        bool was_moved = false;
        tracker() = default;
        tracker(tracker&& other) noexcept :
        was_moved(false) {
            other.was_moved = true;
        }
        tracker(const tracker&) = default;
    };
    auto t = make_tuple(tracker{});
    auto lambda = [](tracker&& tr) { return tr.was_moved; };
    bool result = apply(lambda, move(t));
    EXPECT_FALSE(result);
}

TEST_F(ApplyTest, ApplyReturnTypeIsCorrect) {
    auto t = make_tuple(2, 3);
    auto lambda = [](int a, int b) -> double { return static_cast<double>(a) / b; };
    auto result = apply(lambda, t);
    static_assert(is_same_v<decltype(result), double>);
    EXPECT_DOUBLE_EQ(result, 2.0 / 3.0);
}

TEST_F(ApplyTest, ApplyWithConstexprLambda) {
    const auto t = make_tuple(3, 4);
    const auto lambda = [](int a, int b) { return a * b; };
    const int result = apply(lambda, t);
    EXPECT_EQ(result, 12);
}

TEST_F(ApplyTest, ApplyNoexceptGuarantee) {
    auto t = make_tuple(1, 2);
    auto lambda = [](int a, int b) noexcept { return a + b; };
    EXPECT_TRUE(noexcept(apply(lambda, t)));
    EXPECT_EQ(apply(lambda, t), 3);
}

TEST_F(ApplyTest, ApplyWithRefWrapper) {
    int a = 10;
    int b = 20;
    auto t = make_tuple(ref(a), ref(b));
    auto lambda = [](int& x, int& y) {
        x += 5;
        y += 5;
    };
    apply(lambda, t);
    EXPECT_EQ(a, 15);
    EXPECT_EQ(b, 25);
}

TEST_F(ApplyTest, ApplyWithConstRefWrapper) {
    int a = 10;
    int b = 20;
    auto t = make_tuple(cref(a), cref(b));
    auto lambda = [](const int& x, const int& y) { return x + y; };
    EXPECT_EQ(apply(lambda, t), 30);
}

TEST_F(ApplyTest, ApplyNestedTuple) {
    auto t = make_tuple(make_tuple(1, 2), 3);
    auto lambda = [](const auto& inner, int outer) { return get<0>(inner) + get<1>(inner) + outer; };
    EXPECT_EQ(apply(lambda, t), 6);
}

TEST_F(ApplyTest, ApplyWithPairLikeThroughTupleLike) {
    auto t = make_tuple(10, 20);
    auto lambda = [](int first, int second) { return first * 10 + second; };
    EXPECT_EQ(apply(lambda, t), 120);
}

TEST_F(ApplyTest, ApplyWithZeroArgsReturnsValue) {
    auto t = make_tuple();
    auto lambda = []() { return string("no args"); };
    EXPECT_EQ(apply(lambda, t), "no args");
}

TEST_F(ApplyTest, ApplyWithCapturedLambda) {
    int factor = 10;
    auto t = make_tuple(3, 4);
    auto lambda = [factor](int a, int b) { return (a + b) * factor; };
    EXPECT_EQ(apply(lambda, t), 70);
}

TEST_F(ApplyTest, ApplyWithOverloadedCallOperator) {
    struct overloaded {
        int operator()(int a, int b) const { return a + b; }
        double operator()(double a, double b) const { return a * b; }
    };
    overloaded f;
    auto t1 = make_tuple(3, 4);
    auto t2 = make_tuple(2.5, 2.0);
    EXPECT_EQ(apply(f, t1), 7);
    EXPECT_DOUBLE_EQ(apply(f, t2), 5.0);
}

TEST_F(ApplyTest, ApplyWithGenericLambda) {
    auto t = make_tuple(1, 2.5, string("hello"));
    auto lambda = [](auto&&... args) { return sizeof...(args); };
    EXPECT_EQ(apply(lambda, t), 3);
}

TEST_F(ApplyTest, ApplyPreservesValueCategory) {
    struct move_detector {
        bool moved_from = false;
        move_detector() = default;
        move_detector(move_detector&& o) noexcept { o.moved_from = true; }
        move_detector(const move_detector&) = default;
    };

    auto t = make_tuple(move_detector{});
    auto lambda = [](move_detector&& md) { return md.moved_from; };
    EXPECT_FALSE(apply(lambda, move(t)));
}

TEST_F(ApplyTest, ApplyMultipleTimesOnSameTuple) {
    auto t = make_tuple(10, 20);
    auto lambda = [](int a, int b) { return a + b; };
    EXPECT_EQ(apply(lambda, t), 30);
    EXPECT_EQ(apply(lambda, t), 30);
    EXPECT_EQ(apply(lambda, t), 30);
}

TEST_F(ApplyTest, ApplyTupleOfPointers) {
    int a = 5, b = 10;
    auto t = make_tuple(&a, &b);
    auto lambda = [](int* pa, int* pb) { return *pa + *pb; };
    EXPECT_EQ(apply(lambda, t), 15);
}

TEST_F(ApplyTest, ApplyWithMemberFunctionPointerViaLambda) {
    struct S {
        int value;
        int add(int x) const { return value + x; }
    };
    S s{100};
    auto t = make_tuple(s, 50);
    auto lambda = [](const S& obj, int x) { return obj.add(x); };
    EXPECT_EQ(apply(lambda, t), 150);
}

struct InvokeTestStruct {
    int value;
    InvokeTestStruct(int v) :
    value(v) {}
    int multiply(int x) const { return value * x; }
    int add(int x) { return value + x; }
    void set_value(int x) { value = x; }
    int& get_ref() { return value; }
    const int& get_const_ref() const { return value; }
};

class InvokeTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(InvokeTest, InvokeFreeFunction) {
    int result = invoke(free_func2, 3, 4);
    EXPECT_EQ(result, 7);
}

TEST_F(InvokeTest, InvokeLambda) {
    auto lambda = [](int a, int b) { return a + b; };
    int result = invoke(lambda, 10, 20);
    EXPECT_EQ(result, 30);
}

TEST_F(InvokeTest, InvokeFunctor) {
    invoke_functor f;
    int result = invoke(f, 6, 7);
    EXPECT_EQ(result, 42);
}

TEST_F(InvokeTest, InvokeMemberFunctionRef) {
    InvokeTestStruct obj(5);
    int result = invoke(&InvokeTestStruct::multiply, obj, 3);
    EXPECT_EQ(result, 15);
}

TEST_F(InvokeTest, InvokeMemberFunctionPointer) {
    InvokeTestStruct obj(5);
    int result = invoke(&InvokeTestStruct::multiply, &obj, 4);
    EXPECT_EQ(result, 20);
}

TEST_F(InvokeTest, InvokeMemberFunctionRefWrapper) {
    InvokeTestStruct obj(6);
    int result = invoke(&InvokeTestStruct::multiply, ref(obj), 5);
    EXPECT_EQ(result, 30);
}

TEST_F(InvokeTest, InvokeMemberFunctionSharedPtr) {
    auto sp = make_shared<InvokeTestStruct>(10);
    int result = invoke(&InvokeTestStruct::multiply, sp, 3);
    EXPECT_EQ(result, 30);
}

TEST_F(InvokeTest, InvokeMemberFunctionUniquePtr) {
    auto up = make_unique<InvokeTestStruct>(8);
    int result = invoke(&InvokeTestStruct::multiply, up, 5);
    EXPECT_EQ(result, 40);
}

TEST_F(InvokeTest, InvokeMemberObjectRef) {
    InvokeTestStruct obj(100);
    int result = invoke(&InvokeTestStruct::value, obj);
    EXPECT_EQ(result, 100);
}

TEST_F(InvokeTest, InvokeMemberObjectPointer) {
    InvokeTestStruct obj(200);
    int result = invoke(&InvokeTestStruct::value, &obj);
    EXPECT_EQ(result, 200);
}

TEST_F(InvokeTest, InvokeMemberObjectRefWrapper) {
    InvokeTestStruct obj(300);
    int result = invoke(&InvokeTestStruct::value, ref(obj));
    EXPECT_EQ(result, 300);
}

TEST_F(InvokeTest, InvokeMemberObjectSharedPtr) {
    auto sp = make_shared<InvokeTestStruct>(400);
    int result = invoke(&InvokeTestStruct::value, sp);
    EXPECT_EQ(result, 400);
}

TEST_F(InvokeTest, InvokeNonConstMemberFunction) {
    InvokeTestStruct obj(10);
    invoke(&InvokeTestStruct::set_value, obj, 99);
    EXPECT_EQ(obj.value, 99);
}

TEST_F(InvokeTest, InvokeNonConstMemberFunctionRefWrapper) {
    InvokeTestStruct obj(10);
    invoke(&InvokeTestStruct::set_value, ref(obj), 50);
    EXPECT_EQ(obj.value, 50);
}

TEST_F(InvokeTest, InvokeVoidFunction) {
    int out = 0;
    invoke(void_free_func2, out, 42);
    EXPECT_EQ(out, 42);
}

TEST_F(InvokeTest, InvokeNoexceptFunction) {
    int result = invoke(free_func_noexcept2, 5, 6);
    EXPECT_EQ(result, 11);
}

TEST_F(InvokeTest, InvokeNoexceptFunctor) {
    invoke_functor_noexcept f;
    int result = invoke(f, 3, 4);
    EXPECT_EQ(result, 7);
}

TEST_F(InvokeTest, InvokeWithMultipleArgs) {
    auto lambda = [](int a, int b, int c, int d) { return a + b + c + d; };
    int result = invoke(lambda, 1, 2, 3, 4);
    EXPECT_EQ(result, 10);
}

TEST_F(InvokeTest, InvokeWithNoArgs) {
    auto lambda = []() { return 42; };
    int result = invoke(lambda);
    EXPECT_EQ(result, 42);
}

TEST_F(InvokeTest, InvokeWithReferences) {
    int x = 10;
    auto lambda = [](int& ref) { ref *= 2; };
    invoke(lambda, x);
    EXPECT_EQ(x, 20);
}

TEST_F(InvokeTest, InvokeWithMoveOnlyType) {
    auto lambda = [](unique_ptr<int> p) { return *p; };
    auto up = make_unique<int>(42);
    int result = invoke(lambda, move(up));
    EXPECT_EQ(result, 42);
}

TEST_F(InvokeTest, InvokeRReturnsCorrectType) {
    long result = invoke_r<long>(free_func2, 10, 20);
    EXPECT_EQ(result, 30L);
}

TEST_F(InvokeTest, InvokeRWithVoidReturn) {
    int out = 0;
    invoke_r<void>(void_free_func2, out, 99);
    EXPECT_EQ(out, 99);
}

TEST_F(InvokeTest, InvokeRWithDoubleConversion) {
    double result = invoke_r<double>(free_func2, 5, 7);
    EXPECT_DOUBLE_EQ(result, 12.0);
}

TEST_F(InvokeTest, IsInvocableFreeFunction) {
    EXPECT_TRUE((is_invocable_v<decltype(free_func2), int, int>) );
    EXPECT_FALSE((is_invocable_v<decltype(free_func2), int>) );
}

TEST_F(InvokeTest, IsInvocableMemberFunction) {
    EXPECT_TRUE((is_invocable_v<decltype(&InvokeTestStruct::multiply), InvokeTestStruct, int>) );
    EXPECT_TRUE((is_invocable_v<decltype(&InvokeTestStruct::multiply), InvokeTestStruct*, int>) );
    EXPECT_FALSE((is_invocable_v<decltype(&InvokeTestStruct::multiply), int, int>) );
}

TEST_F(InvokeTest, IsInvocableMemberObject) {
    EXPECT_TRUE((is_invocable_v<decltype(&InvokeTestStruct::value), InvokeTestStruct>) );
    EXPECT_TRUE((is_invocable_v<decltype(&InvokeTestStruct::value), InvokeTestStruct*>) );
}

TEST_F(InvokeTest, IsInvocableRInt) {
    EXPECT_TRUE((is_invocable_r_v<int, decltype(free_func2), int, int>) );
    EXPECT_TRUE((is_invocable_r_v<void, decltype(free_func2), int, int>) );
}

TEST_F(InvokeTest, IsInvocableRVoid) { EXPECT_TRUE((is_invocable_r_v<void, decltype(void_free_func2), int&, int>) ); }

TEST_F(InvokeTest, IsNothrowInvocable) {
    EXPECT_TRUE((is_nothrow_invocable_v<decltype(free_func_noexcept2), int, int>) );
}

TEST_F(InvokeTest, IsNothrowInvocableFalse) {
    auto lambda = [](int a, int b) { return a + b; };
    EXPECT_FALSE((is_nothrow_invocable_v<decltype(lambda), int, int>) );
}

TEST_F(InvokeTest, IsNothrowInvocableFunctor) {
    EXPECT_TRUE((is_nothrow_invocable_v<invoke_functor_noexcept, int, int>) );
    EXPECT_FALSE((is_nothrow_invocable_v<invoke_functor, int, int>) );
}

TEST_F(InvokeTest, InvokeResultTypeFreeFunction) {
    using result = invoke_result_t<decltype(free_func2), int, int>;
    static_assert(is_same_v<decay_t<result>, int>);
    EXPECT_TRUE(true);
}

TEST_F(InvokeTest, InvokeResultTypeMemberFunction) {
    using result = invoke_result_t<decltype(&InvokeTestStruct::multiply), InvokeTestStruct, int>;
    static_assert(is_same_v<result, int>);
    EXPECT_TRUE(true);
}

TEST_F(InvokeTest, InvokeResultTypeMemberObject) {
    using result = invoke_result_t<decltype(&InvokeTestStruct::value), InvokeTestStruct>;
    static_assert(is_same_v<decay_t<result>, int>);
    EXPECT_TRUE(true);
}

TEST_F(InvokeTest, InvokeResultTypeLambda) {
    auto lambda = [](int a) -> double { return a * 1.5; };
    using result = invoke_result_t<decltype(lambda), int>;
    static_assert(is_same_v<result, double>);
    EXPECT_TRUE(true);
}

TEST_F(InvokeTest, InvokeResultWithRefWrapper) {
    InvokeTestStruct obj(5);
    using result = invoke_result_t<decltype(&InvokeTestStruct::multiply), reference_wrapper<InvokeTestStruct>, int>;
    static_assert(is_same_v<result, int>);
    int val = invoke(&InvokeTestStruct::multiply, ref(obj), 4);
    EXPECT_EQ(val, 20);
}

TEST_F(InvokeTest, InvokeResultWithPointer) {
    InvokeTestStruct obj(5);
    using result = invoke_result_t<decltype(&InvokeTestStruct::multiply), InvokeTestStruct*, int>;
    static_assert(is_same_v<result, int>);
    int val = invoke(&InvokeTestStruct::multiply, &obj, 4);
    EXPECT_EQ(val, 20);
}

TEST_F(InvokeTest, InvokeOverloadedFunctorInt) {
    invoke_overloaded_functor f;
    int result = invoke(f, 5);
    EXPECT_EQ(result, 10);
}

TEST_F(InvokeTest, InvokeOverloadedFunctorDouble) {
    invoke_overloaded_functor f;
    double result = invoke(f, 2.5);
    EXPECT_DOUBLE_EQ(result, 7.5);
}

TEST_F(InvokeTest, InvokeWithCapturedLambda) {
    int factor = 10;
    auto lambda = [factor](int x) { return x * factor; };
    int result = invoke(lambda, 5);
    EXPECT_EQ(result, 50);
}

TEST_F(InvokeTest, InvokeMutatingLambda) {
    auto lambda = [count = 0]() mutable { return ++count; };
    EXPECT_EQ(invoke(lambda), 1);
    EXPECT_EQ(invoke(lambda), 2);
    EXPECT_EQ(invoke(lambda), 3);
}

TEST_F(InvokeTest, InvokeWithTupleLikeObjectViaRef) {
    InvokeTestStruct obj(25);
    int& ref = invoke(&InvokeTestStruct::get_ref, obj);
    EXPECT_EQ(ref, 25);
    ref = 100;
    EXPECT_EQ(obj.value, 100);
}

TEST_F(InvokeTest, InvokeWithTupleLikeObjectViaPointer) {
    InvokeTestStruct obj(25);
    int& ref = invoke(&InvokeTestStruct::get_ref, &obj);
    EXPECT_EQ(ref, 25);
    ref = 200;
    EXPECT_EQ(obj.value, 200);
}

TEST_F(InvokeTest, InvokeConstMemberFunction) {
    const InvokeTestStruct obj(15);
    int result = invoke(&InvokeTestStruct::multiply, obj, 2);
    EXPECT_EQ(result, 30);
}

TEST_F(InvokeTest, InvokeConstMemberFunctionPointer) {
    const InvokeTestStruct obj(20);
    int result = invoke(&InvokeTestStruct::multiply, &obj, 3);
    EXPECT_EQ(result, 60);
}

TEST_F(InvokeTest, InvokeDerivedClassMemberFunction) {
    struct Derived : InvokeTestStruct {
        Derived(int v) :
        InvokeTestStruct(v) {}
    };
    Derived d(8);
    int result = invoke(&InvokeTestStruct::multiply, d, 3);
    EXPECT_EQ(result, 24);
}

TEST_F(InvokeTest, InvokeDerivedClassMemberObject) {
    struct Derived : InvokeTestStruct {
        Derived(int v) :
        InvokeTestStruct(v) {}
    };
    Derived d(50);
    int result = invoke(&InvokeTestStruct::value, d);
    EXPECT_EQ(result, 50);
}

TEST_F(InvokeTest, InvokeRWithVoidFunctionVoidReturn) {
    int out = 0;
    invoke_r<void>(void_free_func2, out, 77);
    EXPECT_EQ(out, 77);
}

TEST_F(InvokeTest, InvokeWithStdFunction) {
    function<int(int, int)> f = [](int a, int b) { return a + b; };
    int result = invoke(f, 3, 4);
    EXPECT_EQ(result, 7);
}

TEST_F(InvokeTest, IsInvocableWithBaseClass) {
    struct Base {};
    struct Derived : Base {
        int member = 42;
    };
    Derived d;
    EXPECT_TRUE((is_invocable_v<decltype(&Derived::member), Derived>) );
}

TEST_F(InvokeTest, InvokeResultWithBaseClass) {
    struct Base {
        int value = 10;
    };
    struct Derived : Base {};
    Derived d;
    int result = invoke(&Base::value, d);
    EXPECT_EQ(result, 10);
}

class BindTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(BindTest, BindFreeFunctionAllArgs) {
    auto bound = bind(free_add, 10, 20);
    EXPECT_EQ(bound(), 30);
}

TEST_F(BindTest, BindFreeFunctionWithPlaceholders) {
    auto bound = bind(free_add, p1, p2);
    EXPECT_EQ(bound(10, 20), 30);
}

TEST_F(BindTest, BindFreeFunctionSinglePlaceholder) {
    auto bound = bind(free_add, p1, 100);
    EXPECT_EQ(bound(50), 150);
}

TEST_F(BindTest, BindFreeFunctionReorderedPlaceholders) {
    auto bound = bind(free_add, p2, p1);
    EXPECT_EQ(bound(100, 200), 300);
}

TEST_F(BindTest, BindFreeFunctionThreeArgsAllBound) {
    auto bound = bind(free_add_three, 1, 2, 3);
    EXPECT_EQ(bound(), 6);
}

TEST_F(BindTest, BindFreeFunctionThreeArgsWithPlaceholders) {
    auto bound = bind(free_add_three, p1, p2, 10);
    EXPECT_EQ(bound(5, 15), 30);
}

TEST_F(BindTest, BindFreeFunctionThreeArgsReordered) {
    auto bound = bind(free_add_three, p3, p1, p2);
    EXPECT_EQ(bound(10, 20, 100), 130);
}

TEST_F(BindTest, BindWithStrings) {
    auto bound = bind(free_concat, p1, string(" World"));
    EXPECT_EQ(bound(string("Hello")), "Hello World");
}

TEST_F(BindTest, BindWithReferenceWrapper) {
    int x = 5;
    auto bound = bind(free_increment, ref(x));
    int result = bound();
    EXPECT_EQ(result, 6);
    EXPECT_EQ(x, 6);
}

TEST_F(BindTest, BindWithConstReferenceWrapper) {
    int x = 42;
    auto lambda = [](const int& val) { return val * 2; };
    auto bound = bind(lambda, cref(x));
    EXPECT_EQ(bound(), 84);
}

TEST_F(BindTest, BindMemberFunction) {
    bind_test_struct obj(10);
    auto bound = bind(&bind_test_struct::multiply, p1, 5);
    EXPECT_EQ(bound(obj), 50);
}

TEST_F(BindTest, BindMemberFunctionWithObject) {
    bind_test_struct obj(10);
    auto bound = bind(&bind_test_struct::multiply, obj, p1);
    EXPECT_EQ(bound(3), 30);
}

TEST_F(BindTest, BindMemberFunctionWithPointer) {
    bind_test_struct obj(7);
    auto bound = bind(&bind_test_struct::multiply, &obj, p1);
    EXPECT_EQ(bound(6), 42);
}

TEST_F(BindTest, BindMemberFunctionWithRefWrapper) {
    bind_test_struct obj(4);
    auto bound = bind(&bind_test_struct::multiply, ref(obj), p1);
    EXPECT_EQ(bound(5), 20);
}

TEST_F(BindTest, BindMemberObject) {
    bind_test_struct obj(100);
    auto bound = bind(&bind_test_struct::value, p1);
    EXPECT_EQ(bound(obj), 100);
}

TEST_F(BindTest, BindMemberObjectWithRefWrapper) {
    bind_test_struct obj(200);
    auto bound = bind(&bind_test_struct::value, ref(obj));
    EXPECT_EQ(bound(), 200);
}

TEST_F(BindTest, BindNonConstMemberFunction) {
    bind_test_struct obj(5);
    auto bound = bind(&bind_test_struct::set_value, ref(obj), p1);
    bound(99);
    EXPECT_EQ(obj.value, 99);
}

TEST_F(BindTest, BindLambda) {
    auto lambda = [](int a, int b, int c) { return a * b + c; };
    auto bound = bind(lambda, p1, p2, 10);
    EXPECT_EQ(bound(3, 4), 22);
}

TEST_F(BindTest, BindNestedBind) {
    auto inner = bind(free_add, p1, 100);
    auto outer = bind(free_add, p1, inner);
    EXPECT_EQ(outer(50), 200);
}

TEST_F(BindTest, BindNestedBindWithPlaceholders) {
    auto inner = bind(free_add, p1, p2);
    auto outer = bind(free_add, p1, inner);
    EXPECT_EQ(outer(10, 20, 30), 40);
}

TEST_F(BindTest, BindWithNoPlaceholders) {
    auto bound = bind([]() { return 42; });
    EXPECT_EQ(bound(), 42);
}

TEST_F(BindTest, BindCopySemantics) {
    auto bound1 = bind(free_add, p1, 50);
    auto bound2 = bound1;
    EXPECT_EQ(bound1(10), 60);
    EXPECT_EQ(bound2(10), 60);
}

TEST_F(BindTest, BindMoveSemantics) {
    auto bound1 = bind(free_add, p1, 50);
    auto bound2 = move(bound1);
    EXPECT_EQ(bound2(10), 60);
}

TEST_F(BindTest, BindResultTypeCorrect) {
    auto bound = bind(free_add, p1, p2);
    int result = bound(3, 5);
    EXPECT_EQ(result, 8);
}

TEST_F(BindTest, BindVoidFunction) {
    int out = 0;
    auto bound = bind(free_set, ref(out), p1);
    bound(42);
    EXPECT_EQ(out, 42);
}

TEST_F(BindTest, IsPlaceholder) {
    EXPECT_TRUE((is_placeholder_v<placeholder<1>>) == 1);
    EXPECT_TRUE((is_placeholder_v<placeholder<29>>) == 29);
    EXPECT_TRUE((is_placeholder_v<int>) == 0);
    EXPECT_TRUE((is_placeholder_v<string>) == 0);
}

TEST_F(BindTest, IsBindExpression) {
    auto b = bind(free_add, p1, p2);
    EXPECT_TRUE((is_bind_expression_v<decltype(b)>) );
    EXPECT_FALSE((is_bind_expression_v<int>) );
}

TEST_F(BindTest, BindWithMultiplePlaceholdersOfSameNumber) {
    auto bound = bind(free_add, p1, p1);
    EXPECT_EQ(bound(10), 20);
}

TEST_F(BindTest, BindWithHighPlaceholder) {
    auto bound = bind(free_add_three, p5, p1, p3);
    EXPECT_EQ(bound(1, 0, 3, 0, 10), 14);
}

TEST_F(BindTest, BindrerReturnTypeConversion) {
    auto bound = bind<long>(free_add, p1, p2);
    auto result = bound(100000, 200000);
    static_assert(is_same_v<decltype(result), long>);
    EXPECT_EQ(result, 300000L);
}

TEST_F(BindTest, BindrerWithDifferentReturnType) {
    auto bound = bind<double>(free_add, p1, p2);
    auto result = bound(5, 7);
    EXPECT_DOUBLE_EQ(result, 12.0);
}

TEST_F(BindTest, BindFrontFreeFunction) {
    auto bound = bind_front(free_add, 10);
    EXPECT_EQ(bound(20), 30);
}

TEST_F(BindTest, BindFrontMultipleArgs) {
    auto bound = bind_front(free_add_three, 1, 2);
    EXPECT_EQ(bound(3), 6);
}

TEST_F(BindTest, BindFrontAllArgs) {
    auto bound = bind_front(free_add, 10, 20);
    EXPECT_EQ(bound(), 30);
}

TEST_F(BindTest, BindFrontNoArgs) {
    auto bound = bind_front(free_add);
    EXPECT_EQ(bound(10, 20), 30);
}

TEST_F(BindTest, BindFrontLambda) {
    auto lambda = [](int a, int b, int c) { return a + b + c; };
    auto bound = bind_front(lambda, 10, 20);
    EXPECT_EQ(bound(30), 60);
}

TEST_F(BindTest, BindFrontMemberFunction) {
    bind_test_struct obj(5);
    auto bound = bind_front(&bind_test_struct::multiply, obj);
    EXPECT_EQ(bound(4), 20);
}

TEST_F(BindTest, BindFrontMemberFunctionWithPointer) {
    bind_test_struct obj(5);
    auto bound = bind_front(&bind_test_struct::multiply, &obj);
    EXPECT_EQ(bound(6), 30);
}

TEST_F(BindTest, BindFrontCopySemantics) {
    auto bound1 = bind_front(free_add, 10);
    auto bound2 = bound1;
    EXPECT_EQ(bound1(20), 30);
    EXPECT_EQ(bound2(20), 30);
}

TEST_F(BindTest, BindFrontMoveSemantics) {
    auto bound1 = bind_front(free_add, 10);
    auto bound2 = move(bound1);
    EXPECT_EQ(bound2(20), 30);
}

TEST_F(BindTest, BindFrontLvalueRefQualified) {
    auto bound = bind_front(free_add, 10);
    EXPECT_EQ(bound(20), 30);
}

TEST_F(BindTest, BindFrontRvalueRefQualified) {
    auto bound = bind_front(free_add, 10);
    auto result = move(bound)(20);
    EXPECT_EQ(result, 30);
}

TEST_F(BindTest, BindFrontConstLvalueRefQualified) {
    const auto bound = bind_front(free_add, 10);
    EXPECT_EQ(bound(20), 30);
}

TEST_F(BindTest, BindFrontConstRvalueRefQualified) {
    const auto bound = bind_front(free_add, 10);
    auto result = move(bound)(20);
    EXPECT_EQ(result, 30);
}

TEST_F(BindTest, BindFrontWithString) {
    auto bound = bind_front(free_concat, string("Hello, "));
    EXPECT_EQ(bound(string("World")), "Hello, World");
}

TEST_F(BindTest, BindFrontWithMoveOnlyType) {
    auto lambda = [](unique_ptr<int> p, int x) { return *p + x; };
    auto up = make_unique<int>(10);
    auto bound = bind_front(lambda, move(up));
    EXPECT_EQ(move(bound)(5), 15);
}

TEST_F(BindTest, BindFrontWithReferenceWrapper) {
    int x = 100;
    auto lambda = [](int& ref, int add) { ref += add; };
    auto bound = bind_front(lambda, ref(x));
    bound(50);
    EXPECT_EQ(x, 150);
}

TEST_F(BindTest, BindFrontReturnType) {
    auto bound = bind_front(free_add, 10);
    int result = bound(20);
    EXPECT_EQ(result, 30);
}

TEST_F(BindTest, BindMultipleCalls) {
    bind_call_counter counter;
    auto bound = bind(ref(counter), p1, p2);
    EXPECT_EQ(bound(1, 2), 3);
    EXPECT_EQ(bound(3, 4), 7);
    EXPECT_EQ(bound(5, 6), 11);
    EXPECT_EQ(counter.count, 3);
}

TEST_F(BindTest, BindConstCallOperator) {
    bind_call_counter counter;
    auto bound = bind(ref(counter), p1, p2);
    const auto& cbound = bound;
    EXPECT_EQ(cbound(1, 2), 3);
    EXPECT_EQ(cbound(3, 4), 7);
}

TEST_F(BindTest, BindMemFuncBaseWithRefWrapper) {
    bind_test_struct obj(3);
    auto bound = bind(&bind_test_struct::value, ref(obj));
    EXPECT_EQ(bound(), 3);
    obj.value = 10;
    EXPECT_EQ(bound(), 10);
}

TEST_F(BindTest, BindFrontMultipleCalls) {
    auto bound = bind_front(free_add, 5);
    EXPECT_EQ(bound(10), 15);
    EXPECT_EQ(bound(20), 25);
    EXPECT_EQ(bound(30), 35);
}

TEST_F(BindTest, BindFrontLvalueAssignment) {
    auto bound1 = bind_front(free_add, 10);
    decltype(bound1) bound2 = bind_front(free_add, 20);
    bound1 = bound2;
    EXPECT_EQ(bound1(5), 25);
}

TEST_F(BindTest, BindFrontMoveAssignment) {
    auto bound1 = bind_front(free_add, 10);
    auto bound2 = bind_front(free_add, 20);
    bound1 = move(bound2);
    EXPECT_EQ(bound1(5), 25);
}

TEST_F(BindTest, BinderFrontTypeAlias) {
    using BinderType = binder_front_type<decltype(free_add), int>;
    auto bound = BinderType(0, free_add, 10);
    EXPECT_EQ(bound(20), 30);
}

class HashTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(HashTest, IntegerHashDeterministic) {
    EXPECT_EQ(hash<int>()(42), hash<int>()(42));
    EXPECT_EQ(hash<unsigned long long>()(123456789ULL), hash<unsigned long long>()(123456789ULL));
}

TEST_F(HashTest, IntegerHashNonZero) {
    EXPECT_NE(hash<int>()(0), static_cast<size_t>(0));
    EXPECT_NE(hash<char>()(0), static_cast<size_t>(0));
    EXPECT_NE(hash<unsigned short>()(0), static_cast<size_t>(0));
}

TEST_F(HashTest, FloatHashDeterministic) {
    EXPECT_EQ(hash<double>()(3.14), hash<double>()(3.14));
    EXPECT_EQ(hash<float>()(1.5f), hash<float>()(1.5f));
}

TEST_F(HashTest, FloatHashZero) {
    EXPECT_EQ(hash<float>()(0.0f), static_cast<size_t>(0));
    EXPECT_EQ(hash<double>()(0.0), static_cast<size_t>(0));
}

TEST_F(HashTest, BoolHashDifferent) {
    EXPECT_NE(hash<bool>()(true), hash<bool>()(false));
    EXPECT_EQ(hash<bool>()(true), 0x9e3779b9);
    EXPECT_EQ(hash<bool>()(false), 0x7f4a7c15);
}

TEST_F(HashTest, PointerHashIdentity) {
    int x = 0;
    EXPECT_EQ(hash<int*>()(&x), reinterpret_cast<size_t>(&x));
}

TEST_F(HashTest, EnumHashDelegates) {
    enum class Color {
        Red,
        Green,
        Blue
    };
    EXPECT_EQ(hash<Color>()(Color::Red), hash<int>()(static_cast<int>(Color::Red)));
}

TEST_F(HashTest, LowLevelHashAvalanche) { EXPECT_NE(low_level_hash(static_cast<size_t>(0)), static_cast<size_t>(0)); }

TEST_F(HashTest, LowLevelHashDeterministic) {
    EXPECT_EQ(low_level_hash(static_cast<size_t>(42)), low_level_hash(static_cast<size_t>(42)));
    EXPECT_NE(low_level_hash(static_cast<size_t>(1)), low_level_hash(static_cast<size_t>(2)));
}

TEST_F(HashTest, HashCombineDeterministic) {
    size_t seed1 = 0;
    size_t seed2 = 0;
    hash_combine(seed1, 42);
    hash_combine(seed2, 42);
    EXPECT_EQ(seed1, seed2);
}

TEST_F(HashTest, HashCombineOrderMatters) {
    size_t seed1 = 0;
    size_t seed2 = 0;
    hash_combine(seed1, 1);
    hash_combine(seed1, 2);
    hash_combine(seed2, 2);
    hash_combine(seed2, 1);
    EXPECT_NE(seed1, seed2);
}

TEST_F(HashTest, HashCombineAll) {
    size_t h1 = hash_combine_all(1, 2, 3);
    size_t h2 = hash_combine_all(1, 2, 3);
    size_t h3 = hash_combine_all(3, 2, 1);
    EXPECT_EQ(h1, h2);
    EXPECT_NE(h1, h3);
}

TEST_F(HashTest, XXH32Empty) { EXPECT_EQ(XXH32("", 0, 0), 0x02CC5D05U); }

TEST_F(HashTest, XXH32Deterministic) {
    const char data[] = "test data for xxh32";
    EXPECT_EQ(XXH32(data, sizeof(data) - 1, 0), XXH32(data, sizeof(data) - 1, 0));
}

TEST_F(HashTest, FNVHashPreserved) {
    EXPECT_EQ(FNV_hash(nullptr, 0), constants::FNV_OFFSET_BASIS);
    EXPECT_EQ(FNV_hash_string("", 0), constants::FNV_OFFSET_BASIS);
}

TEST_F(HashTest, DJB2HashPreserved) { EXPECT_EQ(DJB2_hash("", 0), static_cast<size_t>(5381)); }

TEST_F(HashTest, MurmurHashX32Empty) { EXPECT_EQ(murmur_hash32("", 0, 0), 0U); }

TEST_F(HashTest, MurmurHashX32Deterministic) {
    const char data[] = "murmur test";
    uint32_t h1 = murmur_hash32(data, sizeof(data) - 1, 42);
    uint32_t h2 = murmur_hash32(data, sizeof(data) - 1, 42);
    EXPECT_EQ(h1, h2);
}

TEST_F(HashTest, AllAlgorithmsDeterministic) {
    const byte_t data[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                           0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    const size_t len = sizeof(data);

    EXPECT_EQ(XXH32(data, len, 0), XXH32(data, len, 0));
    EXPECT_EQ(murmur_hash32(data, len, 0), murmur_hash32(data, len, 0));
}

#ifdef NEFORCE_ARCH_BITS_64

TEST_F(HashTest, XXH64Empty) { EXPECT_EQ(XXH64("", 0, 0), 0xEF46DB3751D8E999ULL); }

TEST_F(HashTest, XXH64Deterministic) {
    const char data[] = "test data for xxh64";
    EXPECT_EQ(XXH64(data, sizeof(data) - 1, 0), XXH64(data, sizeof(data) - 1, 0));
}

TEST_F(HashTest, XXH3Empty) {
    uint64_t h1 = XXH3_64("", 0);
    uint64_t h2 = XXH3_64("", 0);
    EXPECT_EQ(h1, h2);
    EXPECT_NE(h1, static_cast<uint64_t>(0));
}

TEST_F(HashTest, XXH3Deterministic) {
    const char data[] = "xxh3 test data";
    EXPECT_EQ(XXH3_64(data, sizeof(data) - 1), XXH3_64(data, sizeof(data) - 1));
}

TEST_F(HashTest, WyhashEmpty) { EXPECT_NE(wyhash("", 0, 0), static_cast<uint64_t>(0)); }

TEST_F(HashTest, WyhashDeterministic) {
    const char data[] = "wyhash test data";
    EXPECT_EQ(wyhash(data, sizeof(data) - 1, 42), wyhash(data, sizeof(data) - 1, 42));
}

TEST_F(HashTest, CityHash64Empty) { EXPECT_EQ(city_hash64("", 0), static_cast<size_t>(0x9ae16a3b2f90404fULL)); }

TEST_F(HashTest, CityHash64Deterministic) {
    const char data[] = "cityhash test data";
    EXPECT_EQ(city_hash64(data, sizeof(data) - 1), city_hash64(data, sizeof(data) - 1));
}

TEST_F(HashTest, MurmurHashX64Empty) {
    murmur_hash h = murmur_hash64("", 0, 0);
    EXPECT_EQ(h.low, static_cast<size_t>(0));
    EXPECT_EQ(h.high, static_cast<size_t>(0));
}

TEST_F(HashTest, MurmurHashX64Deterministic) {
    const char data[] = "murmur x64 test";
    murmur_hash h1 = murmur_hash64(data, sizeof(data) - 1, 42);
    murmur_hash h2 = murmur_hash64(data, sizeof(data) - 1, 42);
    EXPECT_EQ(h1.low, h2.low);
    EXPECT_EQ(h1.high, h2.high);
}

TEST_F(HashTest, All64BitAlgorithmsDeterministic) {
    const byte_t data[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                           0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    const size_t len = sizeof(data);

    EXPECT_EQ(XXH64(data, len, 0), XXH64(data, len, 0));
    EXPECT_EQ(wyhash(data, len, 0), wyhash(data, len, 0));
    EXPECT_EQ(city_hash64(data, len), city_hash64(data, len));
    EXPECT_EQ(XXH3_64(data, len), XXH3_64(data, len));
}

#endif
