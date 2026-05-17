#include <NeForce/core/exception/exception_ptr.hpp>
#include <NeForce/core/exception/exception.hpp>
#include <NeForce/core/exception/terminate.hpp>
#include <NeForce/core/system/console.hpp>
#include <exception>
#include <gtest/gtest.h>
using namespace neforce;

TEST(TerminateTest, AbortTerminatesWithCode1) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    EXPECT_EXIT(neforce::abort(), ::testing::ExitedWithCode(3), ".*");
#else
    EXPECT_EXIT(neforce::abort(), ::testing::KilledBySignal(SIGABRT), ".*");
#endif
}

TEST(TerminateTest, ExitWithStatus0) { EXPECT_EXIT(neforce::exit(0), ::testing::ExitedWithCode(0), ".*"); }

TEST(TerminateTest, ExitWithStatus42) { EXPECT_EXIT(neforce::exit(42), ::testing::ExitedWithCode(42), ".*"); }

TEST(TerminateTest, ImmediateExitStatus) { EXPECT_EXIT(immediate_exit(7), ::testing::ExitedWithCode(7), ".*"); }

TEST(TerminateTest, QuickExitStatus) { EXPECT_EXIT(neforce::quick_exit(99), ::testing::ExitedWithCode(99), ".*"); }

TEST(TerminateTest, TerminateCallsHandlerAndAborts) {
    neforce::set_terminate([]() { eprint("custom_terminate"); });
#ifdef NEFORCE_PLATFORM_WINDOWS
    EXPECT_EXIT(neforce::terminate(), ::testing::ExitedWithCode(3), "custom_terminate");
#else
    EXPECT_EXIT(neforce::terminate(), ::testing::KilledBySignal(SIGABRT), "custom_terminate");
#endif
}

TEST(TerminateTest, ExitHandlersCalledInReverseOrder) {
    EXPECT_EXIT(
            {
                neforce::set_exit([]() { eprint("A"); });
                neforce::set_exit([]() { eprint("B"); });
                neforce::set_exit([]() { eprint("C"); });
                neforce::exit(0);
            },
            ::testing::ExitedWithCode(0), "CBA");
}

TEST(TerminateTest, QuickExitHandlersCalledInReverseOrder) {
    EXPECT_EXIT(
            {
                neforce::set_quick_exit([]() { eprint("1"); });
                neforce::set_quick_exit([]() { eprint("2"); });
                neforce::quick_exit(0);
            },
            ::testing::ExitedWithCode(0), "21");
}

TEST(TerminateTest, ImmediateExitDoesNotCallExitHandlers) {
    EXPECT_EXIT(
            {
                neforce::set_exit([]() { print("should_not_appear"); });
                neforce::immediate_exit(5);
            },
            ::testing::ExitedWithCode(5), "");
}

TEST(TerminateTest, QuickExitDoesNotCallExitHandlers) {
    EXPECT_EXIT(
            {
                neforce::set_exit([]() { print("should_not_appear"); });
                neforce::quick_exit(8);
            },
            ::testing::ExitedWithCode(8), "");
}

TEST(ExceptionPtrTest, DefaultConstructedIsNull) {
    exception_ptr ep;
    EXPECT_FALSE(ep);
    EXPECT_TRUE(ep == nullptr);
    EXPECT_TRUE(nullptr == ep);
    EXPECT_FALSE(ep != nullptr);
    EXPECT_FALSE(nullptr != ep);
    EXPECT_EQ(ep.exception_type(), typeid(void));
}

TEST(ExceptionPtrTest, MakeExceptionPtrCreatesValidPtr) {
    auto ep = make_exception_ptr(memory_exception("test"));
    EXPECT_TRUE(ep);
    EXPECT_NE(ep, nullptr);
    EXPECT_EQ(ep.exception_type(), typeid(memory_exception));
}

TEST(ExceptionPtrTest, CopyConstructorSharesOwnership) {
    auto ep1 = make_exception_ptr(system_exception("err"));
    exception_ptr ep2(ep1);
    EXPECT_TRUE(ep1);
    EXPECT_TRUE(ep2);
    EXPECT_EQ(ep1, ep2);
    EXPECT_EQ(ep1.exception_type(), ep2.exception_type());
}

TEST(ExceptionPtrTest, MoveConstructorTransfersOwnership) {
    auto ep1 = make_exception_ptr(file_exception("file"));
    exception_ptr ep2(move(ep1));
    EXPECT_FALSE(ep1);
    EXPECT_TRUE(ep2);
    EXPECT_NE(ep1, ep2);
    EXPECT_EQ(ep2.exception_type(), typeid(file_exception));
}

TEST(ExceptionPtrTest, CopyAssignmentSharesOwnership) {
    auto ep1 = make_exception_ptr(network_exception("net"));
    exception_ptr ep2;
    ep2 = ep1;
    EXPECT_TRUE(ep1);
    EXPECT_TRUE(ep2);
    EXPECT_EQ(ep1, ep2);
}

TEST(ExceptionPtrTest, MoveAssignmentTransfersOwnership) {
    auto ep1 = make_exception_ptr(database_exception("db"));
    exception_ptr ep2;
    ep2 = move(ep1);
    EXPECT_FALSE(ep1);
    EXPECT_TRUE(ep2);
    EXPECT_EQ(ep2.exception_type(), typeid(database_exception));
}

TEST(ExceptionPtrTest, SelfAssignmentIsSafe) {
    auto ep = make_exception_ptr(iterator_exception("iter"));
    exception_ptr& ref = ep;
    ep = ref;
    EXPECT_TRUE(ep);
    EXPECT_EQ(ep.exception_type(), typeid(iterator_exception));
    ep = move(ref);
    EXPECT_TRUE(ep);
}

TEST(ExceptionPtrTest, SwapExchangesContents) {
    auto ep1 = make_exception_ptr(math_exception("math"));
    auto ep2 = make_exception_ptr(typecast_exception("cast"));
    auto copy1 = ep1;
    auto copy2 = ep2;
    ep1.swap(ep2);
    EXPECT_EQ(ep1, copy2);
    EXPECT_EQ(ep2, copy1);
}

TEST(ExceptionPtrTest, BoolConversion) {
    exception_ptr empty;
    auto filled = make_exception_ptr(exception("base"));
    EXPECT_FALSE(static_cast<bool>(empty));
    EXPECT_TRUE(static_cast<bool>(filled));
}

TEST(ExceptionPtrTest, NullptrComparisons) {
    exception_ptr ep;
    EXPECT_TRUE(ep == nullptr);
    EXPECT_TRUE(nullptr == ep);
    EXPECT_FALSE(ep != nullptr);
    EXPECT_FALSE(nullptr != ep);
    auto ep2 = make_exception_ptr(exception("x"));
    EXPECT_FALSE(ep2 == nullptr);
    EXPECT_FALSE(nullptr == ep2);
    EXPECT_TRUE(ep2 != nullptr);
    EXPECT_TRUE(nullptr != ep2);
}

TEST(ExceptionPtrTest, ExceptionTypeForDerivedException) {
    auto ep = make_exception_ptr(device_exception("dev"));
    ASSERT_TRUE(ep);
    EXPECT_EQ(ep.exception_type(), typeid(device_exception));
    auto ep2 = make_exception_ptr(value_exception("val"));
    EXPECT_EQ(ep2.exception_type(), typeid(value_exception));
}

TEST(ExceptionPtrTest, ExceptionTypeForNullReturnsVoid) {
    exception_ptr ep;
    EXPECT_EQ(ep.exception_type(), typeid(void));
    ep = nullptr;
    EXPECT_EQ(ep.exception_type(), typeid(void));
}

TEST(ExceptionPtrTest, CurrentExceptionNoneReturnsNull) {
    exception_ptr ep = current_exception();
    EXPECT_FALSE(ep);
    EXPECT_EQ(ep.exception_type(), typeid(void));
}

TEST(ExceptionPtrTest, CurrentExceptionWithNeForceException) {
    try {
        throw math_exception("bad math");
    } catch (...) {
        auto ep = current_exception();
        ASSERT_TRUE(ep);
        EXPECT_EQ(ep.exception_type(), typeid(math_exception));
        return;
    }
    FAIL() << "Expected exception not thrown";
}

TEST(ExceptionPtrTest, CurrentExceptionWithStdException) {
    try {
        throw std::runtime_error("std error");
    } catch (...) {
        auto ep = current_exception();
        ASSERT_TRUE(ep);
        EXPECT_EQ(ep.exception_type(), typeid(value_exception));
        return;
    }
    FAIL() << "Expected exception not thrown";
}

TEST(ExceptionPtrTest, CurrentExceptionWithUnknownType) {
    struct unknown_exception {};
    try {
        throw unknown_exception{};
    } catch (...) {
        auto ep = current_exception();
        ASSERT_TRUE(ep);
        EXPECT_EQ(ep.exception_type(), typeid(value_exception));
        return;
    }
    FAIL() << "Expected exception not thrown";
}

TEST(ExceptionPtrTest, RethrowExceptionRethrowsOriginal) {
    auto original = make_exception_ptr(memory_exception("mem"));
    try {
        rethrow_exception(original);
        FAIL() << "rethrow_exception should have thrown";
    } catch (const memory_exception& e) {
        EXPECT_STREQ(e.what(), "mem");
    } catch (...) {
        FAIL() << "wrong exception type rethrown";
    }
}

TEST(ExceptionPtrTest, RethrowNullExceptionTerminates) {
    exception_ptr ep;
    EXPECT_DEATH(rethrow_exception(ep), "");
}

TEST(ExceptionPtrTest, ReferenceCountingCopySurvivesDestruction) {
    exception_ptr ep2;
    {
        auto ep1 = make_exception_ptr(exception("refcount"));
        ep2 = ep1;
        ASSERT_TRUE(ep1);
        ASSERT_TRUE(ep2);
        EXPECT_EQ(ep1, ep2);
    }
    EXPECT_TRUE(ep2);
    EXPECT_EQ(ep2.exception_type(), typeid(exception));
}

TEST(ExceptionPtrTest, MoveLeavesSourceNullAndTargetValid) {
    auto ep1 = make_exception_ptr(exception("move"));
    exception_ptr ep2(move(ep1));
    EXPECT_FALSE(ep1);
    EXPECT_TRUE(ep2);
    exception_ptr ep3;
    ep3 = move(ep2);
    EXPECT_FALSE(ep2);
    EXPECT_TRUE(ep3);
}

TEST(ExceptionPtrTest, MakeExceptionPtrWithValueType) {
    auto ep = make_exception_ptr(42);
    ASSERT_TRUE(ep);
    EXPECT_EQ(ep.exception_type(), typeid(int));
}

TEST(ExceptionPtrTest, MultipleCopiesShareSame) {
    auto orig = make_exception_ptr(exception("shared"));
    exception_ptr a(orig);
    exception_ptr b(orig);
    exception_ptr c;
    c = orig;
    EXPECT_TRUE(orig);
    EXPECT_TRUE(a);
    EXPECT_TRUE(b);
    EXPECT_TRUE(c);
    EXPECT_EQ(orig, a);
    EXPECT_EQ(a, b);
    EXPECT_EQ(b, c);
}
