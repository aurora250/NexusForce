#include <NeForce/core/container/unordered_map.hpp>
#include <NeForce/core/exception/debug.hpp>
#include <NeForce/core/exception/error_code.hpp>
#include <NeForce/core/exception/exception_ptr.hpp>
#include <NeForce/core/exception/source_location.hpp>
#include <NeForce/core/exception/system_exception.hpp>
#include <NeForce/core/exception/terminate.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/network/util/network_exception.hpp>
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

TEST(ErrcTest, SuccessIsZero) { EXPECT_EQ(static_cast<int>(errc::success), 0); }

TEST(ErrcTest, ValuesMatchPlatformErrno) {
    EXPECT_EQ(static_cast<int>(errc::permission_denied), EACCES);
    EXPECT_EQ(static_cast<int>(errc::no_such_file_or_directory), ENOENT);
    EXPECT_EQ(static_cast<int>(errc::io_error), EIO);
    EXPECT_EQ(static_cast<int>(errc::timed_out), ETIMEDOUT);
    EXPECT_EQ(static_cast<int>(errc::operation_would_block), EWOULDBLOCK);
}

TEST(ErrcTest, RoundTripThroughErrorCode) {
    const error_code ec = make_error_code(errc::operation_would_block);
    EXPECT_EQ(ec.error(), errc::operation_would_block);
    EXPECT_EQ(ec.value(), static_cast<int>(errc::operation_would_block));
}

TEST(ErrorCodeTest, DefaultConstructsToSystemCategory) {
    const error_code ec;
    EXPECT_EQ(ec.value(), 0);
    EXPECT_FALSE(ec);
    EXPECT_EQ(&ec.category(), &system_category());
    EXPECT_STREQ(ec.category().name(), "system");
}

TEST(ErrorCodeTest, ConstructFromValueAndCategory) {
    const error_code ec(42, generic_category());
    EXPECT_EQ(ec.value(), 42);
    EXPECT_EQ(&ec.category(), &generic_category());
    EXPECT_TRUE(ec);
}

TEST(ErrorCodeTest, ConstructFromErrcImplicitly) {
    const error_code ec = errc::permission_denied;
    EXPECT_EQ(&ec.category(), &generic_category());
    EXPECT_EQ(ec.value(), static_cast<int>(errc::permission_denied));
}

TEST(ErrorCodeTest, MakeErrorCodeFromErrc) {
    const error_code ec = make_error_code(errc::timed_out);
    EXPECT_EQ(ec, error_code(static_cast<int>(errc::timed_out), generic_category()));
    EXPECT_EQ(ec, error_code(errc::timed_out));
}

TEST(ErrorCodeTest, AssignAndClear) {
    error_code ec;
    ec.assign(7, generic_category());
    EXPECT_EQ(ec.value(), 7);
    EXPECT_EQ(&ec.category(), &generic_category());
    ec.clear();
    EXPECT_EQ(ec.value(), 0);
    EXPECT_EQ(&ec.category(), &system_category());
}

TEST(ErrorCodeTest, BoolConversion) {
    EXPECT_FALSE(error_code());
    EXPECT_TRUE(error_code(1, generic_category()));
}

TEST(ErrorCodeTest, EqualityAndOrdering) {
    const error_code a(1, generic_category());
    const error_code b(1, generic_category());
    const error_code c(2, generic_category());
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
    EXPECT_LT(a, c);
    EXPECT_LE(a, b);
    EXPECT_GT(c, a);
    EXPECT_GE(b, a);
}

TEST(ErrorCodeTest, SameValueDifferentCategoriesAreNotEqual) {
    const error_code a(0, generic_category());
    const error_code b(0, system_category());
    EXPECT_NE(a, b);
    EXPECT_TRUE(a.less_than(b) || b.less_than(a));
}

TEST(ErrorCodeTest, HashStableForEqualCodes) {
    const error_code a = make_error_code(errc::file_exists);
    const error_code b = make_error_code(errc::file_exists);
    EXPECT_EQ(a.to_hash(), b.to_hash());
}

TEST(ErrorCodeTest, UsableAsUnorderedMapKey) {
    unordered_map<error_code, int> m;
    m[make_error_code(errc::file_exists)] = 1;
    m[make_error_code(errc::io_error)] = 2;
    EXPECT_EQ(m[make_error_code(errc::file_exists)], 1);
    EXPECT_EQ(m[make_error_code(errc::io_error)], 2);
    EXPECT_EQ(m.size(), 2u);
}

TEST(ErrorCodeTest, EquivalentToErrorCondition) {
    const error_code ec = make_error_code(errc::file_exists);
    EXPECT_TRUE(ec == make_error_condition(errc::file_exists));
    EXPECT_FALSE(ec == make_error_condition(errc::no_such_file_or_directory));
}

TEST(ErrorCodeTest, SystemCodeMapsToGenericCondition) {
    const error_code ec(2, system_category());
    EXPECT_TRUE(ec == make_error_condition(errc::no_such_file_or_directory));
}

#ifdef NEFORCE_PLATFORM_WINDOWS
TEST(ErrorCodeTest, WindowsSocketCodeMapsToGenericCondition) {
    const error_code ec(10054, system_category());
    EXPECT_TRUE(ec == make_error_condition(errc::connection_reset));
}
#endif

TEST(ErrorCodeTest, MessageIsNotEmpty) {
    EXPECT_FALSE(make_error_code(errc::permission_denied).message().empty());
    EXPECT_FALSE(error_code(999999, generic_category()).message().empty());
}

TEST(ErrorCodeTest, LastErrorReturnsSystemCategory) { EXPECT_EQ(&last_error().category(), &system_category()); }

#ifdef NEFORCE_PLATFORM_LINUX
TEST(ErrorCodeTest, LastErrorCapturesErrnoOnLinux) {
    errno = EACCES;
    EXPECT_EQ(last_error().value(), static_cast<int>(errc::permission_denied));
    errno = 0;
}
#endif

TEST(ErrorConditionTest, DefaultConstructsToGenericCategory) {
    const error_condition cond;
    EXPECT_EQ(cond.value(), 0);
    EXPECT_FALSE(cond);
    EXPECT_EQ(&cond.category(), &generic_category());
}

TEST(ErrorConditionTest, ConstructFromValueAndCategory) {
    const error_condition cond(9, generic_category());
    EXPECT_EQ(cond.value(), 9);
    EXPECT_EQ(&cond.category(), &generic_category());
}

TEST(ErrorConditionTest, ConstructFromErrc) {
    const error_condition cond(errc::connection_reset);
    EXPECT_EQ(cond.value(), static_cast<int>(errc::connection_reset));
    EXPECT_EQ(&cond.category(), &generic_category());
}

TEST(ErrorConditionTest, MakeErrorConditionFromErrc) {
    const error_condition cond = make_error_condition(errc::timed_out);
    EXPECT_EQ(cond, error_condition(static_cast<int>(errc::timed_out), generic_category()));
}

TEST(ErrorConditionTest, EqualityAndOrdering) {
    const error_condition a(1, generic_category());
    const error_condition b(1, generic_category());
    const error_condition c(2, generic_category());
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
    EXPECT_LT(a, c);
    EXPECT_GT(c, a);
}

TEST(ErrorConditionTest, BoolConversion) {
    EXPECT_FALSE(error_condition());
    EXPECT_TRUE(error_condition(1, generic_category()));
}

TEST(ErrorConditionTest, MessageIsNotEmpty) { EXPECT_FALSE(make_error_condition(errc::io_error).message().empty()); }

TEST(ErrorCategoryTest, GenericSingletonIdentity) {
    const error_category& g1 = generic_category();
    const error_category& g2 = generic_category();
    EXPECT_EQ(&g1, &g2);
    EXPECT_EQ(&g1, &error_category::generic());
    EXPECT_STREQ(g1.name(), "generic");
}

TEST(ErrorCategoryTest, SystemSingletonIdentity) {
    const error_category& s1 = system_category();
    const error_category& s2 = system_category();
    EXPECT_EQ(&s1, &s2);
    EXPECT_EQ(&s1, &error_category::system());
    EXPECT_STREQ(s1.name(), "system");
}

TEST(ErrorCategoryTest, GenericAndSystemAreDistinct) { EXPECT_NE(&generic_category(), &system_category()); }

TEST(ErrorCategoryTest, CategoriesAreNonCopyable) {
    static_assert(!is_copy_constructible<error_category>::value, "error_category must be non-copyable");
    static_assert(!is_copy_assignable<error_category>::value, "error_category must be non-assignable");
}

TEST(ErrorCategoryTest, DefaultErrorConditionStaysInCategory) {
    const auto cond = generic_category().default_error_condition(static_cast<int>(errc::broken_pipe));
    EXPECT_EQ(cond, make_error_condition(errc::broken_pipe));
}

TEST(ErrorCategoryTest, EquivalentIntToCondition) {
    EXPECT_TRUE(generic_category().equivalent(static_cast<int>(errc::file_exists),
                                              make_error_condition(errc::file_exists)));
    EXPECT_FALSE(
            generic_category().equivalent(static_cast<int>(errc::file_exists), make_error_condition(errc::io_error)));
}

TEST(ErrorCategoryTest, EquivalentCodeToInt) {
    EXPECT_TRUE(generic_category().equivalent(make_error_code(errc::file_exists), static_cast<int>(errc::file_exists)));
    EXPECT_FALSE(system_category().equivalent(make_error_code(errc::file_exists), static_cast<int>(errc::file_exists)));
}

TEST(ErrorCategoryTest, MessagesAvailable) {
    EXPECT_FALSE(generic_category().message(static_cast<int>(errc::io_error)).empty());
    EXPECT_FALSE(system_category().message(2).empty());
}

TEST(ErrorCategoryTest, SystemMessageZeroIsSuccess) {
    const string msg = system_category().message(0);
    EXPECT_FALSE(msg.empty());
#ifdef NEFORCE_PLATFORM_WINDOWS
    EXPECT_EQ(msg, "success");
#endif
}

TEST(ErrorCategoryTest, SystemMessageResolvesSystemText) {
    // ERROR_FILE_NOT_FOUND (Windows) / ENOENT (Linux): real system text, not the fallback
    const string msg = system_category().message(2);
    EXPECT_FALSE(msg.empty());
    EXPECT_EQ(msg.find("unknown system error"), string::npos);
#ifdef NEFORCE_PLATFORM_LINUX
    EXPECT_EQ(msg.find("unknown error"), string::npos);
    EXPECT_EQ(msg.find("Unknown error"), string::npos);
#endif
}

TEST(ErrorCategoryTest, SystemMessageUnknownFallsBack) {
    const string msg = system_category().message(999999);
    EXPECT_FALSE(msg.empty());
    EXPECT_TRUE(msg.find("error") != string::npos);
}

TEST(SystemExceptionTest, CarriesErrorCodeAndInfo) {
    const error_code ec = make_error_code(errc::permission_denied);
    const system_exception ex("access denied", ec);
    EXPECT_STREQ(ex.what(), "access denied");
    EXPECT_EQ(ex.code(), ec);
    EXPECT_STREQ(ex.type(), "system_exception");
}

TEST(SystemExceptionTest, DefaultCodeComesFromLastErrorCategory) {
    const system_exception ex("message");
    EXPECT_STREQ(ex.what(), "message");
    EXPECT_EQ(&ex.code().category(), &system_category());
}

TEST(SystemExceptionTest, ConstructFromCodeOnly) {
    const system_exception ex(make_error_code(errc::timed_out));
    EXPECT_EQ(ex.code(), make_error_code(errc::timed_out));
    EXPECT_STREQ(ex.type(), "system_exception");
}

TEST(SystemExceptionTest, DerivedTypesReportOwnType) {
    EXPECT_STREQ(device_exception("d").type(), "device_exception");
    EXPECT_STREQ(file_exception("f").type(), "file_exception");
}

TEST(SystemExceptionTest, DerivedExceptionsCarryCode) {
    const error_code ec = make_error_code(errc::io_error);
    const device_exception de("dev", ec);
    EXPECT_EQ(de.code(), ec);
    const file_exception fe("file", ec);
    EXPECT_EQ(fe.code(), ec);
}

TEST(SystemExceptionTest, CopyFromExceptionPreservesInfo) {
    const file_exception fe("original");
    const system_exception se(fe);
    EXPECT_STREQ(se.what(), "original");
}

TEST(SystemExceptionTest, ThrowAndCatchPreservesCode) {
    try {
        NEFORCE_THROW_EXCEPTION(system_exception("boom", make_error_code(errc::connection_reset)));
    } catch (const system_exception& e) {
        EXPECT_EQ(e.code(), make_error_code(errc::connection_reset));
        EXPECT_STREQ(e.what(), "boom");
        return;
    }
    FAIL() << "system_exception was not thrown";
}

TEST(ExceptionTest, DefaultConstruction) {
    const exception e;
    EXPECT_STREQ(e.what(), "");
    EXPECT_STREQ(e.type(), "exception");
}

TEST(ExceptionTest, CustomInfo) {
    const exception e("custom message");
    EXPECT_STREQ(e.what(), "custom message");
}

TEST(ExceptionTest, CopyPreservesInfo) {
    const exception src("copy me");
    const exception dst(src);
    EXPECT_STREQ(dst.what(), "copy me");
    exception assigned;
    assigned = src;
    EXPECT_STREQ(assigned.what(), "copy me");
}

TEST(ExceptionTest, MovePreservesInfo) {
    exception src("move me");
    const exception dst(move(src));
    EXPECT_STREQ(dst.what(), "move me");
}

TEST(ExceptionTest, LongInfoIsTruncatedTo255) {
    const string long_msg(300, 'a');
    const exception e(long_msg.data());
    EXPECT_EQ(::strlen(e.what()), static_cast<size_t>(255));
    EXPECT_EQ(e.what()[254], 'a');
}

TEST(ExceptionTest, DerivedClassesViaMacros) {
    EXPECT_STREQ(memory_exception().type(), "memory_exception");
    EXPECT_STREQ(memory_exception().what(), "Memory Operation Failed.");
    EXPECT_STREQ(allocate_exception("custom alloc").type(), "allocate_exception");
    EXPECT_STREQ(allocate_exception("custom alloc").what(), "custom alloc");
    EXPECT_STREQ(iterator_exception().type(), "iterator_exception");
    EXPECT_STREQ(typecast_exception().type(), "typecast_exception");
    EXPECT_STREQ(value_exception().type(), "value_exception");
    EXPECT_STREQ(math_exception().type(), "math_exception");
    EXPECT_STREQ(thirdparty_exception().type(), "thirdparty_exception");
    EXPECT_STREQ(database_exception().type(), "database_exception");
}

TEST(ExceptionTest, DerivedHierarchy) {
    static_assert(is_base_of<exception, memory_exception>::value, "");
    static_assert(is_base_of<memory_exception, allocate_exception>::value, "");
    static_assert(is_base_of<memory_exception, iterator_exception>::value, "");
    static_assert(is_base_of<memory_exception, typecast_exception>::value, "");
    static_assert(is_base_of<value_exception, math_exception>::value, "");
    static_assert(is_base_of<thirdparty_exception, database_exception>::value, "");
}

TEST(UncaughtExceptionsTest, ZeroOutsideExceptionHandling) { EXPECT_EQ(uncaught_exceptions(), 0); }

TEST(ThrowMacroTest, ThrowsExpectedType) {
    EXPECT_THROW(NEFORCE_THROW_EXCEPTION(value_exception("v")), value_exception);
}

TEST(AssertionMacroTest, PassingConditionIsNoOp) { NEFORCE_ASSERTION(true); }

TEST(DebugUtilTest, DebuggerPresentQueryIsCallable) {
    const bool present = is_debugger_present();
    (void) present;
}

TEST(DebugUtilTest, DebugAssertWithTrueCondition) { debug_assert(true); }

TEST(DebugUtilTest, DebugAssertWithFalseConditionDoesNotAbortWithoutDebugger) {
    debug_assert(false, "intentional failure in DebugUtilTest");
}

TEST(DebugUtilTest, VerifyMacroPassingCondition) { NEFORCE_DEBUG_VERIFY(true, "pass"); }

TEST(DebugUtilTest, ConstexprAssertPassingCondition) { NEFORCE_CONSTEXPR_ASSERT(true); }

TEST(DebugUtilTest, IsConstantEvaluatedIsFalseAtRuntime) { EXPECT_FALSE(is_constant_evaluated()); }

TEST(SourceLocationTest, DefaultConstructIsEmpty) {
    constexpr source_location loc;
    EXPECT_STREQ(loc.file_name(), "");
    EXPECT_STREQ(loc.func_name(), "");
    EXPECT_EQ(loc.line(), 0u);
    EXPECT_EQ(loc.column(), 0u);
}

TEST(SourceLocationTest, ExplicitConstruction) {
    constexpr source_location loc("file.cpp", "func", 42, 7);
    EXPECT_STREQ(loc.file_name(), "file.cpp");
    EXPECT_STREQ(loc.func_name(), "func");
    EXPECT_EQ(loc.line(), 42u);
    EXPECT_EQ(loc.column(), 7u);
}

TEST(SourceLocationTest, CurrentProvidesPosition) {
    constexpr source_location loc = source_location::current();
    EXPECT_STRNE(loc.file_name(), "");
    EXPECT_STRNE(loc.func_name(), "");
    EXPECT_NE(loc.line(), 0u);
}
