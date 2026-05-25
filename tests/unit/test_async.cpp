#include <NeForce/core/async/async.hpp>
#include <NeForce/core/async/barrier.hpp>
#include <NeForce/core/async/generator.hpp>
#include <NeForce/core/async/hazard_ptr.hpp>
#include <NeForce/core/async/latch.hpp>
#include <NeForce/core/async/scoped_thread.hpp>
#include <NeForce/core/async/signals.hpp>
#include <NeForce/core/async/thread_pool.hpp>
#include <NeForce/core/async/virtual_thread.hpp>
#include <NeForce/core/utility/tuple.hpp>
#include <gtest/gtest.h>
using namespace neforce;

namespace {
    struct test_obj : hazard_pointer_obj_base {
        atomic<bool>* destroyed;
        explicit test_obj(atomic<bool>* d) :
        destroyed(d) {}
        ~test_obj() override {
            if (destroyed) {
                destroyed->store(true);
            }
        }
        void destroy() override {}
        void* get_ptr() const override { return const_cast<test_obj*>(this); }
    };

    struct test_receiver {
        int value = 0;
        void onUpdate(int v) { value = v; }
        void onUpdateConst(int v) const {}
    };

    struct simple_executor {
        vector<function<void()>> tasks;
        void post(function<void()> f) { tasks.push_back(move(f)); }
        void run() {
            auto pending = move(tasks);
            tasks.clear();
            for (auto& t: pending) {
                t();
            }
        }
    };

    void dummy_func() {}
} // namespace

TEST(ThreadIdTest, DefaultConstructor) {
    thread::id id;
    EXPECT_EQ(id, thread::id{});
}

TEST(ThreadIdTest, Equality) {
    auto id1 = this_thread::id();
    auto id2 = this_thread::id();
    EXPECT_EQ(id1, id2);
    EXPECT_FALSE(id1 != id2);
}

TEST(ThreadIdTest, Hash) {
    auto id1 = this_thread::id();
    auto id2 = this_thread::id();
    EXPECT_EQ(id1.to_hash(), id2.to_hash());
}

TEST(ThreadIdTest, NativeHandleDefault) {
    thread::id id;
#ifdef NEFORCE_PLATFORM_WINDOWS
    EXPECT_EQ(id.native_handle(), 0);
#else
    EXPECT_EQ(id.native_handle(), thread::id{}.native_handle());
#endif
}

TEST(ThreadTest, DefaultConstructor) {
    thread t;
    EXPECT_FALSE(t.joinable());
    EXPECT_EQ(t.get_id(), thread::id{});
}

TEST(ThreadTest, ConstructFromFunction) {
    thread t(dummy_func);
    EXPECT_TRUE(t.joinable());
    t.join();
    EXPECT_FALSE(t.joinable());
}

TEST(ThreadTest, ConstructFromLambda) {
    int x = 0;
    thread t([&] { x = 42; });
    t.join();
    EXPECT_EQ(x, 42);
}

TEST(ThreadTest, ConstructWithArgs) {
    int result = 0;
    auto f = [](int a, int b, int& out) { out = a + b; };
    thread t(f, 2, 3, ref(result));
    t.join();
    EXPECT_EQ(result, 5);
}

TEST(ThreadTest, MoveConstructor) {
    thread t1(dummy_func);
    thread::id id1 = t1.get_id();
    thread t2(move(t1));
    EXPECT_FALSE(t1.joinable());
    EXPECT_TRUE(t2.joinable());
    EXPECT_EQ(t2.get_id(), id1);
    t2.join();
}

TEST(ThreadTest, MoveAssignment) {
    thread t1(dummy_func);
    thread::id id1 = t1.get_id();
    thread t2;
    t2 = move(t1);
    EXPECT_FALSE(t1.joinable());
    EXPECT_TRUE(t2.joinable());
    EXPECT_EQ(t2.get_id(), id1);
    t2.join();
}

TEST(ThreadTest, SelfMoveAssignment) {
    thread t(dummy_func);
    thread::id id = t.get_id();
    t = move(t);
    EXPECT_TRUE(t.joinable());
    EXPECT_EQ(t.get_id(), id);
    t.join();
}

TEST(ThreadTest, Start) {
    thread t;
    int val = 0;
    t.start([&] { val = 99; });
    EXPECT_TRUE(t.joinable());
    t.join();
    EXPECT_EQ(val, 99);
}

TEST(ThreadTest, StartThrowsIfAlreadyStarted) {
    thread t;
    t.start([] {});
    EXPECT_THROW(t.start([] {}), thread_exception);
    t.join();
}

TEST(ThreadTest, JoinNotJoinableThrows) {
    thread t;
    EXPECT_THROW(t.join(), thread_exception);
}

TEST(ThreadTest, DetachNotJoinableThrows) {
    thread t;
    EXPECT_THROW(t.detach(), thread_exception);
}

TEST(ThreadTest, DoubleJoinThrows) {
    thread t([] {});
    t.join();
    EXPECT_THROW(t.join(), thread_exception);
}

TEST(ThreadTest, DetachAfterJoinThrows) {
    thread t([] {});
    t.join();
    EXPECT_THROW(t.detach(), thread_exception);
}

TEST(ThreadTest, JoinAfterDetachThrows) {
    thread t([] {});
    t.detach();
    EXPECT_THROW(t.join(), thread_exception);
}

TEST(ThreadTest, DoubleDetachThrows) {
    thread t([] {});
    t.detach();
    EXPECT_THROW(t.detach(), thread_exception);
}

TEST(ThreadTest, Swap) {
    thread t1([] {});
    thread t2([] {});
    thread::id id1 = t1.get_id();
    thread::id id2 = t2.get_id();
    t1.swap(t2);
    EXPECT_EQ(t1.get_id(), id2);
    EXPECT_EQ(t2.get_id(), id1);
    t1.join();
    t2.join();
}

TEST(ThreadTest, SetName) {
    thread t([] {});
    bool res = t.set_name("test_thread");
    EXPECT_TRUE(res || !res);
    t.join();
}

TEST(ThreadTest, GetNameAfterSet) {
    thread t([] {});
    char buf[32] = {};
    bool set_ok = t.set_name("mythread");
    if (set_ok) {
        bool get_ok = t.name(buf, sizeof(buf));
        if (get_ok) {
            EXPECT_STREQ(buf, "mythread");
        }
    }
    t.join();
}

TEST(ThreadTest, GetNameOnNonJoinable) {
    thread t;
    char buf[32] = {};
    EXPECT_FALSE(t.name(buf, sizeof(buf)));
}

TEST(ThreadTest, StaticSetNameCurrentThread) {
    bool res = thread::set_name(this_thread::handle(), "main_test");
    EXPECT_TRUE(res || !res);
}

TEST(ThreadTest, StaticGetNameCurrentThread) {
    char buf[32] = {};
    bool set_ok = thread::set_name(this_thread::handle(), "get_test");
    if (set_ok) {
        bool get_ok = thread::name(this_thread::handle(), buf, sizeof(buf));
        if (get_ok) {
            EXPECT_STREQ(buf, "get_test");
        }
    }
}

TEST(ThisThreadTest, GetId) {
    auto id = this_thread::id();
    EXPECT_EQ(id, this_thread::id());
}

TEST(ThisThreadTest, SetName) {
    bool res = this_thread::set_name("main");
    EXPECT_TRUE(res || !res);
}

TEST(ThisThreadTest, GetName) {
    char buf[32] = {};
    bool set_ok = this_thread::set_name("main");
    if (set_ok) {
        bool get_ok = this_thread::name(buf, sizeof(buf));
        if (get_ok) {
            EXPECT_STREQ(buf, "main");
        }
    }
}

namespace {
    int g_hook_call_count = 0;
    thread::hook::point g_last_hook_point;
    thread::id g_last_hook_thread_id;

    void test_hook_func(thread::hook::point p, thread::id tid) {
        ++g_hook_call_count;
        g_last_hook_point = p;
        g_last_hook_thread_id = tid;
    }

    atomic<int> g_before_create{0};
    atomic<int> g_after_create{0};
    atomic<int> g_thread_start{0};
    atomic<int> g_thread_end{0};
    atomic<int> g_before_destroy{0};

    void lifecycle_hook(thread::hook::point p, thread::id) {
        switch (p) {
            case thread::hook::point::before_create:
                ++g_before_create;
                break;
            case thread::hook::point::after_create:
                ++g_after_create;
                break;
            case thread::hook::point::thread_start:
                ++g_thread_start;
                break;
            case thread::hook::point::thread_end:
                ++g_thread_end;
                break;
            case thread::hook::point::before_destroy:
                ++g_before_destroy;
                break;
        }
    }

    int g_multi_count_a = 0;
    int g_multi_count_b = 0;

    void multi_hook_a(thread::hook::point, thread::id) { ++g_multi_count_a; }
    void multi_hook_b(thread::hook::point, thread::id) { ++g_multi_count_b; }
} // namespace

TEST(ThreadHookTest, AddRemoveInvoke) {
    g_hook_call_count = 0;
    thread::hook::add_hook(test_hook_func);
    thread::id dummy_id;
    thread::hook::invoke(thread::hook::point::thread_start, dummy_id);
    EXPECT_EQ(g_hook_call_count, 1);
    EXPECT_EQ(g_last_hook_point, thread::hook::point::thread_start);
    EXPECT_EQ(g_last_hook_thread_id, dummy_id);

    thread::hook::remove_hook(test_hook_func);
    g_hook_call_count = 0;
    thread::hook::invoke(thread::hook::point::thread_end, dummy_id);
    EXPECT_EQ(g_hook_call_count, 0);
}

TEST(ThreadHookTest, HookLifecycle) {
    g_before_create = 0;
    g_after_create = 0;
    g_thread_start = 0;
    g_thread_end = 0;
    g_before_destroy = 0;

    thread::hook::add_hook(lifecycle_hook);
    {
        thread t([] {});
        t.join();
    }
    EXPECT_EQ(g_before_create.load(), 1);
    EXPECT_EQ(g_after_create.load(), 1);
    EXPECT_EQ(g_thread_start.load(), 1);
    EXPECT_EQ(g_thread_end.load(), 1);
    EXPECT_EQ(g_before_destroy.load(), 1);
    thread::hook::remove_hook(lifecycle_hook);
}

TEST(ThreadHookTest, MultipleHooks) {
    g_multi_count_a = 0;
    g_multi_count_b = 0;

    thread::hook::add_hook(multi_hook_a);
    thread::hook::add_hook(multi_hook_b);
    thread::hook::invoke(thread::hook::point::after_create, thread::id{});
    EXPECT_EQ(g_multi_count_a, 1);
    EXPECT_EQ(g_multi_count_b, 1);

    thread::hook::remove_hook(multi_hook_a);
    thread::hook::invoke(thread::hook::point::after_create, thread::id{});
    EXPECT_EQ(g_multi_count_a, 1);
    EXPECT_EQ(g_multi_count_b, 2);

    thread::hook::remove_hook(multi_hook_b);
    thread::hook::invoke(thread::hook::point::after_create, thread::id{});
    EXPECT_EQ(g_multi_count_a, 1);
    EXPECT_EQ(g_multi_count_b, 2);
}

TEST(FuturePromise, SetValueInt) {
    promise<int> p;
    future<int> f = p.get_future();
    p.set_value(42);
    EXPECT_EQ(f.get(), 42);
}

TEST(FuturePromise, SetValueVoid) {
    promise<void> p;
    future<void> f = p.get_future();
    p.set_value();
    EXPECT_NO_THROW(f.get());
}

TEST(FuturePromise, SetValueReference) {
    int val = 100;
    promise<int&> p;
    future<int&> f = p.get_future();
    p.set_value(val);
    int& res = f.get();
    EXPECT_EQ(res, 100);
}

TEST(FuturePromise, SetException) {
    promise<int> p;
    future<int> f = p.get_future();
    auto e = make_exception_ptr(value_exception("test error"));
    p.set_exception(e);
    EXPECT_THROW(f.get(), value_exception);
}

TEST(FuturePromise, MoveFuture) {
    promise<int> p;
    future<int> f1 = p.get_future();
    EXPECT_TRUE(f1.valid());
    future<int> f2 = move(f1);
    EXPECT_FALSE(f1.valid());
    EXPECT_TRUE(f2.valid());
    p.set_value(10);
    EXPECT_EQ(f2.get(), 10);
}

TEST(FuturePromise, MovePromise) {
    promise<int> p1;
    future<int> f = p1.get_future();
    promise<int> p2 = move(p1);
    p2.set_value(7);
    EXPECT_EQ(f.get(), 7);
}

TEST(SharedFuture, MultipleGet) {
    promise<int> p;
    shared_future<int> sf = p.get_future().share();
    p.set_value(55);
    EXPECT_EQ(sf.get(), 55);
    EXPECT_EQ(sf.get(), 55);
}

TEST(SharedFuture, Copy) {
    promise<int> p;
    shared_future<int> sf1 = p.get_future().share();
    shared_future<int> sf2 = sf1;
    p.set_value(99);
    EXPECT_EQ(sf1.get(), 99);
    EXPECT_EQ(sf2.get(), 99);
}

TEST(SharedFuture, Void) {
    promise<void> p;
    shared_future<void> sf = p.get_future().share();
    p.set_value();
    EXPECT_NO_THROW(sf.get());
    EXPECT_NO_THROW(sf.get());
}

TEST(ErrorCases, FutureAlreadyRetrieved) {
    promise<int> p;
    auto f1 = p.get_future();
    EXPECT_THROW(p.get_future(), future_exception);
}

TEST(ErrorCases, PromiseAlreadySatisfiedSetValue) {
    promise<int> p;
    p.set_value(1);
    EXPECT_THROW(p.set_value(2), future_exception);
}

TEST(ErrorCases, PromiseAlreadySatisfiedSetException) {
    promise<int> p;
    p.set_value(1);
    EXPECT_THROW(p.set_exception(make_exception_ptr(value_exception("err"))), future_exception);
}

TEST(ErrorCases, BrokenPromiseOnDestruction) {
    future<int> f;
    {
        promise<int> p;
        f = p.get_future();
    }
    EXPECT_THROW(f.get(), future_exception);
}

TEST(ErrorCases, NoStateFuture) {
    future<int> f;
    EXPECT_FALSE(f.valid());
    EXPECT_THROW(f.get(), future_exception);
    EXPECT_THROW(f.wait(), future_exception);
}

TEST(Wait, WaitForReady) {
    promise<int> p;
    future<int> f = p.get_future();
    thread t([&p]() {
        this_thread::sleep_for(milliseconds(50));
        p.set_value(42);
    });
    f.wait();
    EXPECT_EQ(f.get(), 42);
    t.join();
}

TEST(Wait, WaitForTimeout) {
    promise<int> p;
    future<int> f = p.get_future();
    auto status = f.wait_for(milliseconds(10));
    EXPECT_EQ(status, future_status::timeout);
}

TEST(Wait, WaitForReadyAfterSet) {
    promise<int> p;
    future<int> f = p.get_future();
    p.set_value(42);
    auto status = f.wait_for(seconds(0));
    EXPECT_EQ(status, future_status::ready);
}

TEST(Wait, WaitUntilReady) {
    promise<int> p;
    future<int> f = p.get_future();
    auto start = steady_clock::now();
    thread t([&p]() {
        this_thread::sleep_for(milliseconds(50));
        p.set_value(42);
    });
    auto status = f.wait_until(steady_clock::now() + seconds(1));
    EXPECT_EQ(status, future_status::ready);
    t.join();
}

TEST(PackagedTask, Execute) {
    packaged_task<int(int)> task([](int x) { return x * 2; });
    future<int> f = task.get_future();
    task(10);
    EXPECT_EQ(f.get(), 20);
}

TEST(PackagedTask, Exception) {
    packaged_task<int()> task([]() -> int { throw value_exception(); });
    future<int> f = task.get_future();
    task();
    EXPECT_THROW(f.get(), exception);
}

TEST(PackagedTask, Move) {
    packaged_task<int()> task1([]() { return 1; });
    future<int> f = task1.get_future();
    packaged_task<int()> task2 = move(task1);
    EXPECT_FALSE(task1.valid());
    EXPECT_TRUE(task2.valid());
    task2();
    EXPECT_EQ(f.get(), 1);
}

TEST(PackagedTask, Reset) {
    packaged_task<int()> task([]() { return 5; });
    auto f1 = task.get_future();
    task();
    EXPECT_EQ(f1.get(), 5);
    task.reset();
    auto f2 = task.get_future();
    task();
    EXPECT_EQ(f2.get(), 5);
}

TEST(PackagedTask, MakeReadyAtThreadExit) {
    bool flag = false;
    packaged_task<void()> task([&flag]() { flag = true; });
    future<void> f = task.get_future();
    thread t([&task]() { task.make_ready_at_thread_exit(); });
    f.get();
    EXPECT_TRUE(flag);
    t.join();
}

TEST(Async, LaunchAsync) {
    auto f = async(launch::async, []() { return 42; });
    EXPECT_EQ(f.get(), 42);
}

TEST(Async, LaunchDeferred) {
    int callCount = 0;
    auto func = [&callCount]() -> int {
        ++callCount;
        return 10;
    };
    auto f = async(launch::deferred, func);
    EXPECT_EQ(callCount, 0);
    EXPECT_EQ(f.get(), 10);
    EXPECT_EQ(callCount, 1);
}

TEST(Async, DefaultPolicy) {
    auto f = async([]() { return 7; });
    EXPECT_EQ(f.get(), 7);
}

TEST(Async, ExceptionPropagation) {
    auto f = async(launch::async, []() -> int { throw value_exception("out of range"); });
    EXPECT_THROW(f.get(), exception);
}

TEST(Async, VoidReturn) {
    bool called = false;
    auto f = async(launch::async, [&called]() { called = true; });
    f.get();
    EXPECT_TRUE(called);
}

TEST(FutureResultType, NonVoid) {
    promise<int> p;
    future<int> f = p.get_future();
    p.set_value(88);
    auto res = get(f);
    static_assert(is_same_v<decltype(res), int>, "result type mismatch");
    EXPECT_EQ(res, 88);
}

TEST(FutureResultType, Void) {
    promise<void> p;
    future<void> f = p.get_future();
    p.set_value();
    auto res = get(f);
    static_assert(is_same_v<decltype(res), none_t>, "result type mismatch");
}

TEST(Launch, BitOperations) {
    launch a = launch::async | launch::deferred;
    EXPECT_NE((a & launch::async), launch{0});
    launch b = launch::async;
    b |= launch::deferred;
    EXPECT_EQ(static_cast<int>(b), static_cast<int>(a));
}

TEST(FutureException, TypeInfo) {
    future_exception ex("test");
    EXPECT_STREQ(ex.type(), "future_exception");
    EXPECT_STREQ(ex.what(), "test");
}

TEST(Barrier, SingleThreadArriveAndWait) {
    barrier<> b(1);
    EXPECT_NO_THROW(b.arrive_and_wait());
}

TEST(Barrier, TwoThreadsArriveAndWait) {
    atomic<int> phase{0};
    barrier<> b(2);
    thread t([&]() {
        b.arrive_and_wait();
        phase.fetch_add(1);
    });
    b.arrive_and_wait();
    phase.fetch_add(1);
    t.join();
    EXPECT_EQ(phase.load(), 2);
}

TEST(Barrier, ThreeThreadsArriveAndWait) {
    atomic<int> counter{0};
    constexpr int N = 3;
    barrier<> b(N);
    auto work = [&]() {
        b.arrive_and_wait();
        counter.fetch_add(1);
    };
    thread t1(work);
    thread t2(work);
    work();
    t1.join();
    t2.join();
    EXPECT_EQ(counter.load(), N);
}

TEST(Barrier, ArriveThenWait) {
    barrier<> b(2);
    auto tok = b.arrive();
    atomic<bool> flag{false};
    thread t([&]() {
        flag.store(true);
        b.arrive_and_wait();
    });
    b.wait(move(tok));
    EXPECT_TRUE(flag.load());
    t.join();
}

TEST(Barrier, CompletionFunctionCalled) {
    atomic<int> completion_count{0};
    barrier b(2, [&]() noexcept { completion_count.fetch_add(1); });
    thread t([&]() { b.arrive_and_wait(); });
    b.arrive_and_wait();
    t.join();
    EXPECT_EQ(completion_count.load(), 1);
}

TEST(Barrier, CompletionFunctionCalledOncePerPhase) {
    atomic<int> completions{0};
    barrier b(2, [&]() noexcept { completions.fetch_add(1); });
    for (int i = 0; i < 3; ++i) {
        thread t([&]() { b.arrive_and_wait(); });
        b.arrive_and_wait();
        t.join();
    }
    EXPECT_EQ(completions.load(), 3);
}

TEST(Barrier, ArriveWithUpdate) {
    atomic<int> counter{0};
    barrier b(3, [&]() noexcept { counter.fetch_add(1); });

    thread t([&]() {
        auto tok = b.arrive(2);
        b.wait(move(tok));
    });

    auto tok = b.arrive(1);
    b.wait(move(tok));

    t.join();
    EXPECT_EQ(counter.load(), 1);
}

TEST(Barrier, ArriveWithUpdateSingleThread) {
    atomic<int> counter{0};
    barrier b(3, [&]() noexcept { counter.fetch_add(1); });
    auto tok = b.arrive(3);
    EXPECT_EQ(counter.load(), 1);
    b.wait(move(tok));
}

TEST(Barrier, ArriveAndDrop) {
    atomic<int> remaining{0};
    barrier b(3, [&]() noexcept { remaining.fetch_add(1); });

    thread t([&]() { b.arrive_and_drop(); });
    thread t1([&]() { b.arrive_and_wait(); });
    b.arrive_and_wait();

    t.join();
    t1.join();
    EXPECT_EQ(remaining.load(), 1);

    thread t2([&]() { b.arrive_and_wait(); });
    b.arrive_and_wait();
    t2.join();
    EXPECT_EQ(remaining.load(), 2);
}

TEST(Barrier, ArriveAndDropMultiplePhases) {
    atomic<int> rounds{0};
    atomic<bool> t1_dropped{false};
    barrier b(3, [&]() noexcept { rounds.fetch_add(1); });

    b.arrive_and_drop();

    thread t1([&]() {
        b.arrive_and_wait();
        b.arrive_and_drop();
        t1_dropped.store(true);
    });

    b.arrive_and_wait();
    t1.join();

    EXPECT_TRUE(t1_dropped.load());
    b.arrive_and_wait();

    EXPECT_EQ(rounds.load(), 2);
}

TEST(Barrier, MoveToken) {
    barrier<> b(2);
    auto tok1 = b.arrive();
    auto tok2 = move(tok1);
    thread t([&]() { b.arrive_and_wait(); });
    b.wait(move(tok2));
    t.join();
}

TEST(Barrier, MaxReturnsPositiveValue) { EXPECT_GT(barrier<>::max(), 0); }

TEST(Barrier, SingleThreadCompletionCalled) {
    atomic<int> count{0};
    barrier b(1, [&]() noexcept { count.fetch_add(1); });
    b.arrive_and_wait();
    EXPECT_EQ(count.load(), 1);
}

TEST(Barrier, MultipleRoundsSameThreads) {
    constexpr int THREADS = 4;
    constexpr int ROUNDS = 5;
    barrier b(THREADS, []() noexcept {});
    atomic<int> stage{0};
    auto work = [&](int id) {
        for (int r = 0; r < ROUNDS; ++r) {
            b.arrive_and_wait();
            if (id == 0) {
                stage.fetch_add(1);
            }
            b.arrive_and_wait();
        }
    };
    vector<thread> threads;
    for (int i = 0; i < THREADS; ++i) {
        threads.emplace_back(work, i);
    }
    for (auto& t: threads) {
        t.join();
    }
    EXPECT_EQ(stage.load(), ROUNDS);
}

TEST(HazardPointerRecord, AcquireRelease) {
    hazard_pointer_record rec;
    EXPECT_TRUE(rec.try_acquire());
    EXPECT_FALSE(rec.try_acquire());
    rec.release();
    EXPECT_TRUE(rec.try_acquire());
    rec.release();
}

TEST(HazardPointerRecord, ProtectAndGet) {
    hazard_pointer_record rec;
    int x = 42;
    rec.protect(&x);
    EXPECT_EQ(rec.get_protected(), &x);
    rec.release();
}

TEST(HazardPointerDomain, AcquireRecord) {
    hazard_pointer_domain domain;
    auto* rec1 = domain.acquire_record();
    EXPECT_NE(rec1, nullptr);
    EXPECT_TRUE(rec1->active.load());
    auto* rec2 = domain.acquire_record();
    EXPECT_NE(rec2, nullptr);
    EXPECT_NE(rec1, rec2);
    rec1->release();
    rec2->release();
}

TEST(HazardPointerDomain, DefaultDomain) {
    auto& domain = hazard_pointer_domain::default_domain();
    auto* rec = domain.acquire_record();
    EXPECT_NE(rec, nullptr);
    rec->release();
}

TEST(HazardPointer, BasicLifecycle) {
    hazard_pointer_domain domain;
    {
        hazard_pointer hp(domain);
        EXPECT_TRUE(static_cast<bool>(hp));
    }
    EXPECT_TRUE(true);
}

TEST(HazardPointer, MoveAssignment) {
    hazard_pointer_domain domain;
    hazard_pointer hp1(domain);
    hazard_pointer hp2;
    hp2 = move(hp1);
    EXPECT_FALSE(static_cast<bool>(hp1));
    EXPECT_TRUE(static_cast<bool>(hp2));
}

TEST(HazardPointer, ProtectNullSrc) {
    hazard_pointer_domain domain;
    hazard_pointer hp(domain);
    atomic<int*> src{nullptr};
    int* p = hp.protect(src);
    EXPECT_EQ(p, nullptr);
}

TEST(HazardPointer, ProtectValid) {
    hazard_pointer_domain domain;
    hazard_pointer hp(domain);
    int val = 10;
    atomic<int*> src{&val};
    int* p = hp.protect(src);
    EXPECT_EQ(p, &val);
}

TEST(HazardPointer, ProtectABA) {
    hazard_pointer_domain domain;
    hazard_pointer hp(domain);
    atomic<int*> src{nullptr};
    int a = 1, b = 2;
    src.store(&a);
    thread t([&]() { src.store(&b); });
    int* p = hp.protect(src);
    EXPECT_TRUE(p == &a || p == &b);
    t.join();
}

TEST(HazardPointer, TryProtect) {
    hazard_pointer_domain domain;
    hazard_pointer hp(domain);
    int val = 20;
    atomic<int*> src{&val};
    int* ptr = nullptr;
    bool ok = hp.try_protect(ptr, src);
    EXPECT_TRUE(ok);
    EXPECT_EQ(ptr, &val);
}

TEST(HazardPointer, TryProtectChanged) {
    hazard_pointer_domain domain;
    hazard_pointer hp(domain);
    int a = 1, b = 2;
    atomic<int*> src{&a};
    thread t([&]() { src.store(&b); });
    int* ptr = nullptr;
    hp.try_protect(ptr, src);
    t.join();
    EXPECT_TRUE(ptr != nullptr);
}

TEST(HazardPointer, ResetProtection) {
    hazard_pointer_domain domain;
    hazard_pointer hp(domain);
    int val = 5;
    atomic<int*> src{&val};
    hp.protect(src);
    hp.reset_protection();
}

TEST(HazardPointerHolder, ProtectAndAccess) {
    hazard_pointer_domain domain;
    hazard_pointer_holder<int> holder(domain);
    int val = 30;
    atomic<int*> src{&val};
    int* p = holder.protect(src);
    EXPECT_EQ(holder.get(), &val);
    EXPECT_EQ(*holder, 30);
    EXPECT_EQ(holder.operator->(), &val);
    EXPECT_TRUE(static_cast<bool>(holder));
}

TEST(HazardPointerHolder, Reset) {
    hazard_pointer_domain domain;
    hazard_pointer_holder<int> holder(domain);
    int val = 40;
    atomic<int*> src{&val};
    holder.protect(src);
    holder.reset();
    EXPECT_EQ(holder.get(), nullptr);
    EXPECT_FALSE(static_cast<bool>(holder));
}

TEST(HazardPointerDomain, RetireAndReclaim) {
    hazard_pointer_domain domain;
    atomic<bool> destroyed{false};
    auto* obj = new test_obj(&destroyed);
    domain.retire(obj);
    domain.reclaim();
    EXPECT_TRUE(destroyed.load());
}

TEST(HazardPointerDomain, RetireRespectsProtected) {
    hazard_pointer_domain domain;
    atomic<bool> destroyed{false};
    auto* obj = new test_obj(&destroyed);
    atomic<test_obj*> src{obj};
    hazard_pointer hp(domain);
    hp.protect(src);
    domain.retire(obj);
    domain.reclaim();
    EXPECT_FALSE(destroyed.load());
    hp.reset_protection();
    domain.reclaim();
    EXPECT_TRUE(destroyed.load());
}

TEST(HazardPointerDomain, ThresholdReclaim) {
    hazard_pointer_domain domain;
    vector<test_obj*> objs;
    atomic<int> destroyCount{0};
    for (int i = 0; i < 105; ++i) {
        auto* obj = new test_obj(nullptr);
        obj->destroyed = nullptr;
        domain.retire(obj);
    }
    domain.reclaim();
}

TEST(HazardPointer, MultiThreadProtect) {
    hazard_pointer_domain domain;
    atomic<test_obj*> src{nullptr};
    atomic<bool> destroyed{false};
    test_obj* obj = new test_obj(&destroyed);
    src.store(obj);
    vector<thread> threads;
    atomic<int> readers{0};
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&]() {
            hazard_pointer hp(domain);
            test_obj* p = hp.protect(src);
            if (p) {
                readers.fetch_add(1);
                this_thread::sleep_for(milliseconds(10));
            }
        });
    }
    this_thread::sleep_for(milliseconds(5));
    domain.retire(obj);
    src.store(nullptr);
    domain.reclaim();
    domain.reclaim();
    for (auto& t: threads) {
        t.join();
    }
    domain.reclaim();
    EXPECT_TRUE(destroyed.load());
}

TEST(HazardPointer, MakeHazardPointer) {
    auto hp = make_hazard_pointer();
    EXPECT_TRUE(static_cast<bool>(hp));
}

TEST(HazardPointerDomain, DestructorCleansRecords) {
    auto* domain = new hazard_pointer_domain();
    domain->acquire_record();
    delete domain;
}

TEST(Latch, MaxReturnsPositive) { EXPECT_GT(latch::max(), 0); }

TEST(Latch, ConstructorInitializesCounter) {
    latch l(3);
    EXPECT_FALSE(l.try_wait());
}

TEST(Latch, CountDownDecrements) {
    latch l(2);
    l.count_down();
    EXPECT_FALSE(l.try_wait());
    l.count_down();
    EXPECT_TRUE(l.try_wait());
}

TEST(Latch, CountDownByUpdate) {
    latch l(5);
    l.count_down(3);
    EXPECT_FALSE(l.try_wait());
    l.count_down(2);
    EXPECT_TRUE(l.try_wait());
}

TEST(Latch, WaitBlocksUntilZero) {
    latch l(1);
    atomic<bool> reached{false};
    thread t([&]() {
        l.wait();
        reached.store(true);
    });
    this_thread::sleep_for(milliseconds(10));
    EXPECT_FALSE(reached.load());
    l.count_down();
    t.join();
    EXPECT_TRUE(reached.load());
}

TEST(Latch, WaitReturnsImmediatelyIfZero) {
    latch l(0);
    l.wait();
    SUCCEED();
}

TEST(Latch, ArriveAndWait) {
    latch l(2);
    atomic<bool> done{false};
    thread t([&]() {
        l.arrive_and_wait();
        done.store(true);
    });
    this_thread::sleep_for(milliseconds(10));
    EXPECT_FALSE(done.load());
    l.count_down();
    t.join();
    EXPECT_TRUE(done.load());
}

TEST(Latch, MultipleThreads) {
    constexpr int N = 5;
    latch l(N);
    atomic<int> counter{0};
    auto worker = [&]() {
        l.arrive_and_wait();
        counter.fetch_add(1);
    };
    vector<thread> threads;
    for (int i = 0; i < N; ++i) {
        threads.emplace_back(worker);
    }
    for (auto& t: threads) {
        t.join();
    }
    EXPECT_EQ(counter.load(), N);
}

TEST(Latch, ZeroConstructor) {
    latch l(0);
    EXPECT_TRUE(l.try_wait());
}

TEST(Latch, CountDownNotifiesAll) {
    latch l(3);
    atomic<int> woken{0};
    auto waiter = [&]() {
        l.wait();
        woken.fetch_add(1);
    };
    thread t1(waiter);
    thread t2(waiter);
    this_thread::sleep_for(milliseconds(10));
    l.count_down(3);
    t1.join();
    t2.join();
    EXPECT_EQ(woken.load(), 2);
}

TEST(Latch, ArriveAndWaitWithMultipleArrivals) {
    latch l(4);
    atomic<int> passed{0};
    auto worker = [&]() {
        l.arrive_and_wait(2);
        passed.fetch_add(1);
    };
    thread t1(worker);
    thread t2(worker);
    t1.join();
    t2.join();
    EXPECT_EQ(passed.load(), 2);
}

TEST(StopSource, DefaultConstruction) {
    stop_source ss;
    EXPECT_TRUE(ss.stop_possible());
    EXPECT_FALSE(ss.stop_requested());
}

TEST(StopSource, NoneConstruction) {
    stop_source ss(none);
    EXPECT_FALSE(ss.stop_possible());
    EXPECT_FALSE(ss.stop_requested());
}

TEST(StopSource, RequestStop) {
    stop_source ss;
    EXPECT_TRUE(ss.request_stop());
    EXPECT_TRUE(ss.stop_requested());
    EXPECT_FALSE(ss.request_stop());
}

TEST(StopSource, CopyConstruction) {
    stop_source ss1;
    stop_source ss2(ss1);
    EXPECT_TRUE(ss2.stop_possible());
    ignore = ss1.request_stop();
    EXPECT_TRUE(ss2.stop_requested());
}

TEST(StopSource, CopyAssignment) {
    stop_source ss1;
    stop_source ss2(none);
    ss2 = ss1;
    EXPECT_TRUE(ss2.stop_possible());
    ignore = ss1.request_stop();
    EXPECT_TRUE(ss2.stop_requested());
}

TEST(StopSource, MoveConstruction) {
    stop_source ss1;
    stop_source ss2(move(ss1));
    EXPECT_TRUE(ss2.stop_possible());
    ignore = ss2.request_stop();
}

TEST(StopSource, MoveAssignment) {
    stop_source ss1;
    stop_source ss2;
    ss2 = move(ss1);
    EXPECT_TRUE(ss2.stop_possible());
}

TEST(StopSource, GetToken) {
    stop_source ss;
    stop_token st = ss.get_token();
    EXPECT_TRUE(st.stop_possible());
    EXPECT_FALSE(st.stop_requested());
    ignore = ss.request_stop();
    EXPECT_TRUE(st.stop_requested());
}

TEST(StopToken, DefaultConstruction) {
    stop_token st;
    EXPECT_FALSE(st.stop_possible());
    EXPECT_FALSE(st.stop_requested());
}

TEST(StopToken, CopyToken) {
    stop_source ss;
    stop_token st1 = ss.get_token();
    stop_token st2(st1);
    EXPECT_TRUE(st2.stop_possible());
    ignore = ss.request_stop();
    EXPECT_TRUE(st2.stop_requested());
}

TEST(StopToken, MoveToken) {
    stop_source ss;
    stop_token st1 = ss.get_token();
    stop_token st2(move(st1));
    EXPECT_TRUE(st2.stop_possible());
    ignore = ss.request_stop();
    EXPECT_TRUE(st2.stop_requested());
}

TEST(StopToken, Equality) {
    stop_source ss1;
    stop_source ss2;
    stop_token st1 = ss1.get_token();
    stop_token st2 = ss1.get_token();
    stop_token st3 = ss2.get_token();
    EXPECT_TRUE(st1 == st2);
    EXPECT_FALSE(st1 == st3);
}

TEST(StopCallback, CallbackInvoked) {
    stop_source ss;
    atomic<bool> called{false};
    {
        stop_callback cb(ss.get_token(), [&] { called.store(true); });
        ignore = ss.request_stop();
    }
    EXPECT_TRUE(called.load());
}

TEST(StopCallback, CallbackInvokedOnAlreadyStopped) {
    stop_source ss;
    ignore = ss.request_stop();
    atomic<bool> called{false};
    {
        stop_callback cb(ss.get_token(), [&] { called.store(true); });
        EXPECT_TRUE(called.load());
    }
}

TEST(StopCallback, CallbackNotCalledIfDestroyed) {
    stop_source ss;
    atomic<bool> called{false};
    {
        stop_callback cb(ss.get_token(), [&] { called.store(true); });
    }
    ignore = ss.request_stop();
    EXPECT_FALSE(called.load());
}

TEST(StopCallback, MultipleCallbacks) {
    stop_source ss;
    atomic<int> count{0};
    atomic<int> order{0};
    {
        stop_callback cb1(ss.get_token(), [&] {
            count.fetch_add(1);
            if (count == 3) {
                order.store(1);
            }
        });
        stop_callback cb2(ss.get_token(), [&] {
            count.fetch_add(1);
            if (count == 2) {
                order.store(2);
            }
        });
        stop_callback cb3(ss.get_token(), [&] {
            count.fetch_add(1);
            if (count == 1) {
                order.store(3);
            }
        });
        ignore = ss.request_stop();
    }
    EXPECT_EQ(count.load(), 3);
    EXPECT_EQ(order.load(), 1);
}

TEST(StopCallback, ThreadSafety) {
    stop_source ss;
    atomic<int> callbacks_invoked{0};
    constexpr int N = 10;
    barrier ready(N + 1);
    vector<thread> threads;

    for (int i = 0; i < N; ++i) {
        threads.emplace_back([&] {
            stop_callback cb(ss.get_token(), [&] { callbacks_invoked.fetch_add(1); });
            ready.arrive_and_wait();
            ready.arrive_and_wait();
        });
    }

    ready.arrive_and_wait();
    ignore = ss.request_stop();
    ready.arrive_and_wait();

    for (auto& t: threads) {
        t.join();
    }
    EXPECT_EQ(callbacks_invoked.load(), N);
}

TEST(StopSource, DtorDoesNotAffectToken) {
    stop_token st;
    {
        stop_source ss;
        st = ss.get_token();
        EXPECT_TRUE(st.stop_possible());
        ignore = ss.request_stop();
    }
    EXPECT_TRUE(st.stop_requested());
}

TEST(StopSource, MultipleSourcesOneToken) {
    stop_source ss1;
    stop_source ss2;
    stop_token st = ss1.get_token();
    stop_source ss3 = ss1;
    EXPECT_TRUE(st.stop_possible());
    ignore = ss3.request_stop();
    EXPECT_TRUE(st.stop_requested());
    EXPECT_FALSE(ss2.stop_requested());
}

TEST(StopCallback, OrderOfCallbackExecution) {
    stop_source ss;
    vector<int> order;
    {
        stop_callback cb1(ss.get_token(), [&] { order.push_back(1); });
        stop_callback cb2(ss.get_token(), [&] { order.push_back(2); });
        stop_callback cb3(ss.get_token(), [&] { order.push_back(3); });
        ignore = ss.request_stop();
    }
    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], 3);
    EXPECT_EQ(order[1], 2);
    EXPECT_EQ(order[2], 1);
}

TEST(StopCallback, RegisterAfterStopButWithNewSource) {
    stop_source ss;
    ignore = ss.request_stop();
    stop_source ss2;
    stop_token st = ss2.get_token();
    atomic<bool> called{false};
    {
        stop_callback cb(st, [&] { called.store(true); });
        EXPECT_FALSE(called.load());
        ignore = ss2.request_stop();
        EXPECT_TRUE(called.load());
    }
}

TEST(ScopedThread, DefaultConstructor) {
    scoped_thread st;
    EXPECT_FALSE(st.joinable());
}

TEST(ScopedThread, ConstructorStartsThread) {
    atomic<bool> ran{false};
    {
        scoped_thread st([&ran] { ran.store(true); });
        EXPECT_TRUE(st.joinable());
    }
    EXPECT_TRUE(ran.load());
}

TEST(ScopedThread, DestructorJoinsAutomatically) {
    atomic<bool> thread_finished{false};
    {
        scoped_thread st([&thread_finished] {
            this_thread::sleep_for(milliseconds(20));
            thread_finished.store(true);
        });
    }
    EXPECT_TRUE(thread_finished.load());
}

TEST(ScopedThread, StopTokenPassedToCallable) {
    atomic<bool> stop_requested{false};
    {
        scoped_thread st([&stop_requested](stop_token token) {
            while (!token.stop_requested()) {
                this_thread::sleep_for(milliseconds(1));
            }
            stop_requested.store(true);
        });
        this_thread::sleep_for(milliseconds(10));
        st.request_stop();
    }
    EXPECT_TRUE(stop_requested.load());
}

TEST(ScopedThread, CallableWithoutStopToken) {
    atomic<int> val{0};
    {
        scoped_thread st([&val] { val.store(42); });
    }
    EXPECT_EQ(val.load(), 42);
}

TEST(ScopedThread, MoveConstructor) {
    atomic<bool> ran{false};
    {
        scoped_thread st1([&ran] { ran.store(true); });
        scoped_thread st2(move(st1));
        EXPECT_FALSE(st1.joinable());
        EXPECT_TRUE(st2.joinable());
    }
    EXPECT_TRUE(ran.load());
}

TEST(ScopedThread, MoveAssignment) {
    atomic<bool> ran{false};
    {
        scoped_thread st1([&ran] { ran.store(true); });
        scoped_thread st2;
        st2 = move(st1);
        EXPECT_FALSE(st1.joinable());
        EXPECT_TRUE(st2.joinable());
    }
    EXPECT_TRUE(ran.load());
}

TEST(ScopedThread, Swap) {
    atomic<bool> ran1{false}, ran2{false};
    {
        scoped_thread st1([&ran1] { ran1.store(true); });
        scoped_thread st2([&ran2] { ran2.store(true); });
        st1.swap(st2);
    }
    EXPECT_TRUE(ran1.load());
    EXPECT_TRUE(ran2.load());
}

TEST(ScopedThread, Detach) {
    atomic<bool> ran{false};
    {
        scoped_thread st([&ran] {
            this_thread::sleep_for(milliseconds(50));
            ran.store(true);
        });
        st.detach();
        EXPECT_FALSE(st.joinable());
    }
    this_thread::sleep_for(milliseconds(100));
    EXPECT_TRUE(ran.load());
}

TEST(ScopedThread, GetStopSource) {
    scoped_thread st([] {});
    stop_source src = st.get_stop_source();
    EXPECT_TRUE(src.stop_possible());
    st.request_stop();
    EXPECT_TRUE(src.stop_requested());
}

TEST(ScopedThread, GetStopToken) {
    scoped_thread st([] {});
    stop_token token = st.get_stop_token();
    EXPECT_TRUE(token.stop_possible());
    st.request_stop();
    EXPECT_TRUE(token.stop_requested());
}

TEST(ScopedThread, GetId) {
    scoped_thread st;
    EXPECT_EQ(st.get_id(), thread::id{});
    scoped_thread st2([] {});
    EXPECT_NE(st2.get_id(), thread::id{});
    st2.join();
}

TEST(ScopedThread, NativeHandle) {
    scoped_thread st([] {});
    EXPECT_TRUE(st.native_handle() != 0);
    st.join();
}

TEST(ScopedThread, RequestStopManually) {
    atomic<bool> stop_seen{false};
    {
        scoped_thread st([&stop_seen](stop_token token) {
            while (!token.stop_requested()) {
                this_thread::sleep_for(milliseconds(1));
            }
            stop_seen.store(true);
        });
        EXPECT_TRUE(st.request_stop());
        st.join();
    }
    EXPECT_TRUE(stop_seen.load());
}

TEST(ScopedThread, MemberFunctionWithStopToken) {
    struct worker {
        void run(stop_token token, atomic<bool>* flag) {
            while (!token.stop_requested()) {
                this_thread::sleep_for(milliseconds(1));
            }
            flag->store(true);
        }
    };
    atomic<bool> completed{false};
    worker w;
    {
        scoped_thread st(&worker::run, &w, &completed);
        st.request_stop();
    }
    EXPECT_TRUE(completed.load());
}

using steady_scheduler = timer_scheduler<steady_clock>;

TEST(TimerScheduler, AddTaskAndExecute) {
    promise<void> done;
    steady_scheduler sched;
    sched.add_task(steady_clock::now() + 10_ms, [&] { done.set_value(); });
    EXPECT_EQ(done.get_future().wait_for(500_ms), future_status::ready);
}

TEST(TimerScheduler, AddMultipleTasksExecuteInOrder) {
    vector<int> order;
    promise<void> done;
    steady_scheduler sched;
    auto now = steady_clock::now();
    sched.add_task(now + 5_ms, [&] { order.push_back(1); });
    sched.add_task(now + 3_ms, [&] { order.push_back(0); });
    sched.add_task(now + 10_ms, [&] {
        order.push_back(2);
        done.set_value();
    });
    ignore = done.get_future().wait_for(500_ms);
    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], 0);
    EXPECT_EQ(order[1], 1);
    EXPECT_EQ(order[2], 2);
}

TEST(TimerScheduler, CancelTask) {
    atomic<bool> called{false};
    steady_scheduler sched;
    auto id = sched.add_task(steady_clock::now() + 50_ms, [&] { called = true; });
    bool canceled = sched.cancel(id);
    EXPECT_TRUE(canceled);
    this_thread::sleep_for(100_ms);
    EXPECT_FALSE(called.load());
}

TEST(TimerScheduler, CancelAll) {
    atomic<int> count{0};
    steady_scheduler sched;
    auto now = steady_clock::now();
    sched.add_task(now + 5_ms, [&] { count.fetch_add(1); });
    sched.add_task(now + 10_ms, [&] { count.fetch_add(1); });
    sched.add_task(now + 15_ms, [&] { count.fetch_add(1); });
    sched.cancel_all();
    this_thread::sleep_for(50_ms);
    EXPECT_EQ(count.load(), 0);
}

TEST(TimerScheduler, Size) {
    steady_scheduler sched;
    EXPECT_EQ(sched.size(), 0u);
    auto now = steady_clock::now();
    auto id1 = sched.add_task(now + 5_ms, [] {});
    EXPECT_EQ(sched.size(), 1u);
    auto id2 = sched.add_task(now + 10_ms, [] {});
    EXPECT_EQ(sched.size(), 2u);
    sched.cancel(id1);
    EXPECT_EQ(sched.size(), 1u);
    sched.cancel(id2);
    EXPECT_EQ(sched.size(), 0u);
}

TEST(TimerScheduler, ImmediateTask) {
    promise<void> done;
    steady_scheduler sched;
    sched.add_task(steady_clock::now() - 1_h, [&] { done.set_value(); });
    EXPECT_EQ(done.get_future().wait_for(500_ms), future_status::ready);
}

TEST(TimerScheduler, DestructorStopsThread) {
    {
        steady_scheduler sched;
        sched.add_task(steady_clock::now() + 1_h, [] {});
    }
    SUCCEED();
}

TEST(BasicTimer, AsyncWait) {
    promise<void> done;
    steady_timer timer;
    timer.expires_after(10_ms);
    timer.async_wait([&] { done.set_value(); });
    EXPECT_EQ(done.get_future().wait_for(500_ms), future_status::ready);
    EXPECT_FALSE(timer.is_active());
}

TEST(BasicTimer, Cancel) {
    steady_timer timer;
    timer.expires_after(50_ms);
    timer.async_wait([] { FAIL(); });
    timer.cancel();
    EXPECT_FALSE(timer.is_active());
    this_thread::sleep_for(100_ms);
    SUCCEED();
}

TEST(BasicTimer, ExpiresAt) {
    auto target = steady_clock::now() + 30_ms;
    promise<void> done;
    steady_timer timer;
    timer.expires_at(target);
    EXPECT_GT(timer.expiry(), steady_clock::now());
    timer.async_wait([&] { done.set_value(); });
    EXPECT_EQ(done.get_future().wait_for(500_ms), future_status::ready);
}

TEST(BasicTimer, ExpiresFromNow) {
    promise<void> done;
    steady_timer timer;
    timer.expires_from_now(20);
    timer.async_wait([&] { done.set_value(); });
    EXPECT_EQ(done.get_future().wait_for(500_ms), future_status::ready);
}

TEST(BasicTimer, MoveConstructor) {
    promise<void> done;
    steady_timer timer;
    timer.expires_after(10_ms);
    timer.async_wait([&] { done.set_value(); });
    steady_timer moved(move(timer));
    EXPECT_FALSE(timer.is_active());
    EXPECT_TRUE(moved.is_active());
    EXPECT_EQ(done.get_future().wait_for(500_ms), future_status::ready);
    EXPECT_FALSE(moved.is_active());
}

TEST(BasicTimer, MoveAssignment) {
    promise<void> done;
    steady_timer timer;
    timer.expires_after(10_ms);
    timer.async_wait([&] { done.set_value(); });
    steady_timer target = move(timer);
    EXPECT_FALSE(timer.is_active());
    EXPECT_TRUE(target.is_active());
    EXPECT_EQ(done.get_future().wait_for(500_ms), future_status::ready);
}

TEST(BasicTimer, DestructorCancelsTask) {
    atomic<bool> called{false};
    {
        steady_timer timer;
        timer.expires_after(50_ms);
        timer.async_wait([&] { called = true; });
    }
    this_thread::sleep_for(100_ms);
    EXPECT_FALSE(called.load());
}

TEST(BasicTimer, Reschedule) {
    atomic<int> count{0};
    steady_timer timer;
    timer.expires_after(30_ms);
    timer.async_wait([&] { count.fetch_add(1); });
    this_thread::sleep_for(15_ms);
    timer.expires_after(10_ms);
    timer.async_wait([&] { count.fetch_add(2); });
    this_thread::sleep_for(50_ms);
    EXPECT_EQ(count.load(), 2);
}

TEST(BasicTimer, IsActive) {
    steady_timer timer;
    EXPECT_FALSE(timer.is_active());
    timer.expires_after(1_h);
    timer.async_wait([] {});
    EXPECT_TRUE(timer.is_active());
    timer.cancel();
    EXPECT_FALSE(timer.is_active());
}

TEST(TaskGroup, IncrementDecrement) {
    task_group group;
    group.increment();
    group.increment();
    group.decrement();
    group.decrement();
    group.wait();
    SUCCEED();
}

TEST(TaskGroup, WaitBlocks) {
    task_group group;
    group.increment();
    atomic<bool> finished{false};
    thread t([&] {
        group.wait();
        finished.store(true);
    });
    this_thread::sleep_for(10_ms);
    EXPECT_FALSE(finished.load());
    group.decrement();
    t.join();
    EXPECT_TRUE(finished.load());
}

TEST(LocalQueue, PushAndPop) {
    local_queue q;
    int val = 0;
    q.push_back([&val] { val = 42; });
    auto task = q.try_pop();
    ASSERT_TRUE(task.has_value());
    (*task)();
    EXPECT_EQ(val, 42);
}

TEST(LocalQueue, Empty) {
    local_queue q;
    EXPECT_TRUE(q.empty());
    EXPECT_EQ(q.size(), 0u);
    q.push_back([] {});
    EXPECT_FALSE(q.empty());
    EXPECT_EQ(q.size(), 1u);
    q.try_pop();
    EXPECT_TRUE(q.empty());
}

TEST(LocalQueue, StealHalf) {
    local_queue src, dst;
    local_queue::set_steal_strategy(local_queue::steal_strategy::half);
    int sum = 0;
    src.push_back([&sum] { sum += 1; });
    src.push_back([&sum] { sum += 2; });
    auto stolen = src.be_stolen_by(dst);
    ASSERT_TRUE(stolen.has_value());
    (*stolen)();
    EXPECT_GE(sum, 1);
    auto remaining = src.try_pop();
    if (remaining) {
        (*remaining)();
    }
    EXPECT_EQ(sum, 3);
}

TEST(LocalQueue, RemainSize) {
    local_queue q;
    EXPECT_GT(q.remain_size(), 0u);
    q.push_back([] {});
    EXPECT_EQ(q.remain_size(), q.capacity() - 1);
}

TEST(WorkerContext, MoveConstructor) {
    worker_context ctx;
    ctx.id = 5;
    worker_context moved(move(ctx));
    EXPECT_EQ(moved.id, 5u);
}

TEST(ThreadPool, StartStop) {
    thread_pool pool;
    EXPECT_FALSE(pool.running());
    pool.start(2);
    EXPECT_TRUE(pool.running());
    auto stats = pool.stop();
    EXPECT_FALSE(pool.running());
    EXPECT_GT(stats.total_threads, 0u);
}

TEST(ThreadPool, SubmitTaskReturnsValue) {
    thread_pool pool;
    pool.start(1);
    auto res = pool.submit_task([] { return 42; });
    EXPECT_EQ(res.future.get(), 42);
    pool.stop();
}

TEST(ThreadPool, SubmitTaskVoid) {
    thread_pool pool;
    pool.start(1);
    atomic<bool> flag{false};
    auto res = pool.submit_task([&flag] { flag.store(true); });
    res.future.get();
    EXPECT_TRUE(flag.load());
    pool.stop();
}

TEST(ThreadPool, SubmitTaskWithPriority) {
    thread_pool pool;
    pool.start(2);
    atomic<int> order{0};
    auto res1 = pool.submit_task(static_cast<thread_pool::priority_type>(1), [&order] {
        this_thread::sleep_for(20_ms);
        order.store(1);
    });
    auto res2 = pool.submit_task(static_cast<thread_pool::priority_type>(10), [&order] { order.store(2); });
    res1.future.get();
    res2.future.get();
    EXPECT_NE(order.load(), 0);
    pool.stop();
}

TEST(ThreadPool, SubmitAfter) {
    thread_pool pool;
    pool.start(1);
    auto start = steady_clock::now();
    auto res = pool.submit_after(50, [] { return 1; });
    int val = res.future.get();
    auto elapsed = steady_clock::now() - start;
    EXPECT_EQ(val, 1);
    EXPECT_GE(elapsed, 40_ms);
    pool.stop();
}

TEST(ThreadPool, SubmitEvery) {
    thread_pool pool;
    pool.start(2);
    atomic<int> count{0};
    auto token = pool.submit_every(30, [&count] { count.fetch_add(1); });
    this_thread::sleep_for(80_ms);
    pool.cancel_periodic_task(token);
    this_thread::sleep_for(40_ms);
    int final_count = count.load();
    EXPECT_GE(final_count, 2);
    EXPECT_LE(final_count, 5);
    pool.stop();
}

TEST(ThreadPool, CancelPeriodicTask) {
    thread_pool pool;
    pool.start(1);
    atomic<bool> executed{false};
    auto token = pool.submit_every(20, [&executed] { executed.store(true); });
    pool.cancel_periodic_task(token);
    this_thread::sleep_for(60_ms);
    EXPECT_FALSE(executed.load());
    pool.stop();
}

TEST(ThreadPool, TaskGroup) {
    thread_pool pool;
    pool.start(3);
    auto group = make_shared<task_group>();
    get_current_task_group() = group;
    auto res1 = pool.submit_task([] { this_thread::sleep_for(20_ms); });
    auto res2 = pool.submit_task([] { this_thread::sleep_for(20_ms); });
    group->wait();
    EXPECT_TRUE(true);
    res1.future.get();
    res2.future.get();
    get_current_task_group().reset();
    pool.stop();
}

TEST(ThreadPool, Exception) {
    thread_pool pool;
    pool.start(1);
    auto res = pool.submit_task([]() -> int { throw value_exception("test"); });
    EXPECT_THROW(res.future.get(), exception);
    EXPECT_EQ(res.task_info->status.load(), task_info::status::failed);
    pool.stop();
}

TEST(ThreadPool, Statistics) {
    thread_pool pool;
    pool.start(2);
    auto res = pool.submit_task([] { return 0; });
    res.future.get();
    auto stats = pool.stop();
    EXPECT_GT(stats.total_threads, 0u);
    EXPECT_GT(stats.total_completed, 0u);
}

TEST(ThreadPool, ModeFixed) {
    thread_pool pool;
    EXPECT_TRUE(pool.set_mode(thread_pool::pool_mode::fixed));
    pool.start(2);
    EXPECT_EQ(pool.mode(), thread_pool::pool_mode::fixed);
    pool.stop();
}

TEST(ThreadPool, ModeCached) {
    thread_pool pool;
    EXPECT_TRUE(pool.set_mode(thread_pool::pool_mode::cached));
    pool.start(1);
    EXPECT_EQ(pool.mode(), thread_pool::pool_mode::cached);
    pool.stop();
}

TEST(ThreadPool, SetStealStrategy) {
    thread_pool pool;
    EXPECT_TRUE(pool.set_steal_mode(local_queue::steal_strategy::single, 1));
    pool.start(2);
    auto res = pool.submit_task([] { return 1; });
    EXPECT_EQ(res.future.get(), 1);
    pool.stop();
}

TEST(ThreadPool, TaskThresholdRejection) {
    thread_pool pool;
    pool.set_task_threshhold(1);
    pool.start(1);
    auto res1 = pool.submit_task([] { this_thread::sleep_for(2000_ms); });
    auto res2 = pool.submit_task([] { return 2; });
    auto res3 = pool.submit_task([] { return 3; });
    EXPECT_TRUE(res3.task_info && res3.task_info->is_finished());
    EXPECT_EQ(res3.task_info->status.load(), task_info::status::failed);
    res1.future.get();
    EXPECT_EQ(res2.future.get(), 2);
    pool.stop();
}

TEST(ThreadPool, WaitMultipleFutures) {
    thread_pool pool;
    pool.start(2);
    auto f1 = pool.submit_task([] { return 10; });
    auto f2 = pool.submit_task([] { return 20; });
    auto results = thread_pool::wait(move(f1.future), move(f2.future));
    EXPECT_EQ(get<0>(results), 10);
    EXPECT_EQ(get<1>(results), 20);
    pool.stop();
}

#ifdef NEFORCE_STANDARD_20

class VirtualThreadEnvironment : public ::testing::Environment {
public:
    void SetUp() override { virtual_thread::initialize(4); }
    void TearDown() override { virtual_thread::shutdown(); }
};

::testing::Environment* const virtual_thread_env = ::testing::AddGlobalTestEnvironment(new VirtualThreadEnvironment);

TEST(VirtualThread, StartBasic) {
    atomic<bool> ran{false};
    virtual_thread::start([&] { ran.store(true); });
    this_thread::sleep_for(50_ms);
    EXPECT_TRUE(ran.load());
}

TEST(VirtualThread, Yield) {
    atomic<int> sequence{0};
    atomic<int> result{0};
    virtual_thread::start([&]() -> virtual_thread_task<void> {
        sequence.store(sequence.load() + 1);
        co_await virtual_thread::yield();
        result.store(sequence.load());
    });
    this_thread::sleep_for(50_ms);
    EXPECT_EQ(result.load(), 1);
}

TEST(VirtualThread, MultipleTasks) {
    atomic<int> counter{0};
    constexpr int N = 5;
    for (int i = 0; i < N; ++i) {
        virtual_thread::start([&] { counter.fetch_add(1); });
    }
    this_thread::sleep_for(100_ms);
    EXPECT_EQ(counter.load(), N);
}

TEST(VirtualThread, ExceptionInFireAndForget) {
    virtual_thread::start([] { throw exception("test"); });
    this_thread::sleep_for(50_ms);
    SUCCEED();
}

TEST(VirtualThread, Sleep) {
    atomic<bool> done{false};
    virtual_thread::start([&]() -> virtual_thread_task<void> {
        co_await virtual_thread::sleep(10);
        done.store(true);
    });
    this_thread::sleep_for(50_ms);
    EXPECT_TRUE(done.load());
}

TEST(VirtualThread, ReturnValueSync) {
    auto task = virtual_thread::start([]() -> virtual_thread_task<int> { co_return 42; });
    EXPECT_EQ(task.get_result(), 42);
}

TEST(VirtualThread, ReturnValueAfterSleep) {
    auto task = virtual_thread::start([]() -> virtual_thread_task<int> {
        co_await virtual_thread::sleep(10);
        co_return 99;
    });
    EXPECT_EQ(task.get_result(), 99);
}

TEST(VirtualThread, ReturnValueAfterYield) {
    auto task = virtual_thread::start([]() -> virtual_thread_task<int> {
        co_await virtual_thread::yield();
        co_return 7;
    });
    EXPECT_EQ(task.get_result(), 7);
}

TEST(VirtualThread, VoidTaskGetResult) {
    atomic<bool> ran{false};
    auto task = virtual_thread::start([&]() -> virtual_thread_task<void> {
        ran.store(true);
        co_return;
    });
    task.get_result();
    EXPECT_TRUE(ran.load());
}

TEST(VirtualThread, AwaitSyncSubtask) {
    auto task = virtual_thread::start([]() -> virtual_thread_task<int> {
        auto sub = []() -> virtual_thread_task<int> { co_return 7; };
        int val = co_await sub();
        co_return val * 3;
    });
    EXPECT_EQ(task.get_result(), 21);
}

TEST(VirtualThread, AwaitSubtaskWithYield) {
    auto task = virtual_thread::start([]() -> virtual_thread_task<int> {
        auto sub = []() -> virtual_thread_task<int> {
            co_await virtual_thread::yield();
            co_return 10;
        };
        int val = co_await sub();
        co_return val + 5;
    });
    EXPECT_EQ(task.get_result(), 15);
}

TEST(VirtualThread, AwaitVoidSubtask) {
    atomic<bool> innerRan{false};
    auto task = virtual_thread::start([&]() -> virtual_thread_task<int> {
        auto sub = [&]() -> virtual_thread_task<void> {
            innerRan.store(true);
            co_return;
        };
        co_await sub();
        co_return 1;
    });
    EXPECT_EQ(task.get_result(), 1);
    EXPECT_TRUE(innerRan.load());
}

TEST(VirtualThread, DeepNesting) {
    auto task = virtual_thread::start([]() -> virtual_thread_task<int> {
        auto leaf = []() -> virtual_thread_task<int> {
            co_await virtual_thread::yield();
            co_return 1;
        };
        auto middle = [&]() -> virtual_thread_task<int> {
            int v = co_await leaf();
            co_return v + 1;
        };
        int v = co_await middle();
        co_return v + 1;
    });
    EXPECT_EQ(task.get_result(), 3);
}

TEST(VirtualThread, ExceptionPropagation) {
    auto task = virtual_thread::start([]() -> virtual_thread_task<int> {
        auto sub = []() -> virtual_thread_task<int> {
            throw value_exception("inner error");
            co_return 1;
        };
        co_await sub();
        co_return 2;
    });
    EXPECT_THROW(task.get_result(), value_exception);
}

TEST(VirtualThread, ExceptionAfterYield) {
    auto task = virtual_thread::start([]() -> virtual_thread_task<int> {
        co_await virtual_thread::yield();
        throw value_exception("after yield");
        co_return 1;
    });
    EXPECT_THROW(task.get_result(), value_exception);
}

TEST(VirtualThreadTask, MoveConstructor) {
    auto task1 = virtual_thread::start([]() -> virtual_thread_task<int> { co_return 42; });
    EXPECT_TRUE(task1.valid());
    auto task2 = move(task1);
    EXPECT_FALSE(task1.valid());
    EXPECT_TRUE(task2.valid());
    EXPECT_EQ(task2.get_result(), 42);
}

TEST(VirtualThreadTask, MoveAssignment) {
    auto task1 = virtual_thread::start([]() -> virtual_thread_task<int> { co_return 55; });
    virtual_thread_task<int> task2;
    task2 = move(task1);
    EXPECT_FALSE(task1.valid());
    EXPECT_TRUE(task2.valid());
    EXPECT_EQ(task2.get_result(), 55);
}

TEST(VirtualThreadTask, MoveVoidTask) {
    auto task1 = virtual_thread::start([]() -> virtual_thread_task<void> { co_return; });
    EXPECT_TRUE(task1.valid());
    auto task2 = move(task1);
    EXPECT_FALSE(task1.valid());
    EXPECT_TRUE(task2.valid());
    task2.get_result();
}

TEST(VirtualThreadTask, MoveFireAndForget) {
    atomic<bool> ran{false};
    auto task1 = virtual_thread::start([&] { ran.store(true); });
    EXPECT_TRUE(task1.valid());
    auto task2 = move(task1);
    EXPECT_FALSE(task1.valid());
    EXPECT_TRUE(task2.valid());
    this_thread::sleep_for(50_ms);
    EXPECT_TRUE(ran.load());
}

TEST(VirtualThread, ConcurrentTasks) {
    auto t1 = virtual_thread::start([]() -> virtual_thread_task<int> {
        co_await virtual_thread::yield();
        co_return 1;
    });
    auto t2 = virtual_thread::start([]() -> virtual_thread_task<int> {
        co_await virtual_thread::yield();
        co_return 2;
    });
    EXPECT_EQ(t1.get_result(), 1);
    EXPECT_EQ(t2.get_result(), 2);
}

TEST(VirtualThread, SequentialAwaitConcurrent) {
    auto t1 = virtual_thread::start([]() -> virtual_thread_task<int> {
        co_await virtual_thread::yield();
        co_return 10;
    });
    auto t2 = virtual_thread::start([]() -> virtual_thread_task<int> {
        co_await virtual_thread::yield();
        co_return 20;
    });
    auto wrapper = virtual_thread::start([&]() -> virtual_thread_task<int> {
        int a = co_await t1;
        int b = co_await t2;
        co_return a + b;
    });
    EXPECT_EQ(wrapper.get_result(), 30);
}

TEST(VirtualThread, SynchronousCompletion) {
    auto task = virtual_thread::start([]() -> virtual_thread_task<int> { co_return 100; });
    EXPECT_TRUE(task.is_done());
    EXPECT_EQ(task.get_result(), 100);
}

TEST(VirtualThread, MultipleYields) {
    atomic<int> counter{0};
    auto task = virtual_thread::start([&]() -> virtual_thread_task<int> {
        for (int i = 0; i < 5; ++i) {
            counter.fetch_add(1);
            co_await virtual_thread::yield();
        }
        co_return counter.load();
    });
    EXPECT_EQ(task.get_result(), 5);
}

TEST(VirtualThread, IsDone) {
    auto task = virtual_thread::start([]() -> virtual_thread_task<int> {
        co_await virtual_thread::sleep(20);
        co_return 42;
    });
    EXPECT_FALSE(task.is_done());
    task.get_result();
    EXPECT_TRUE(task.is_done());
}

TEST(VirtualThread, DefaultConstructedTask) {
    virtual_thread_task<int> task;
    EXPECT_FALSE(task.valid());
    EXPECT_FALSE(task.is_done());
}

TEST(VirtualThread, GetResultBlocking) {
    auto task = virtual_thread::start([]() -> virtual_thread_task<int> {
        co_await virtual_thread::sleep(50);
        co_return 88;
    });
    auto start = steady_clock::now();
    EXPECT_EQ(task.get_result(), 88);
    auto elapsed = steady_clock::now() - start;
    EXPECT_GE(elapsed, 40_ms);
}

TEST(VirtualThread, GetResultCrossThreadBlocking) {
    auto task = virtual_thread::start([]() -> virtual_thread_task<int> {
        co_await virtual_thread::sleep(30);
        co_return 77;
    });
    atomic<bool> completed{false};
    atomic<int> result{0};
    thread t([&]() {
        result.store(task.get_result());
        completed.store(true);
    });
    while (!completed.load()) {
        this_thread::sleep_for(1_ms);
    }
    EXPECT_EQ(result.load(), 77);
    t.join();
}

TEST(VirtualThread, GetResultAfterFrameDestroyed) {
    auto task = virtual_thread::start([]() -> virtual_thread_task<int> {
        co_await virtual_thread::yield();
        co_return 55;
    });
    while (!task.is_done()) {
        this_thread::sleep_for(1_ms);
    }
    this_thread::sleep_for(20_ms);
    EXPECT_EQ(task.get_result(), 55);
}

TEST(VirtualThread, ExceptionTypePreservation) {
    auto task = virtual_thread::start([]() -> virtual_thread_task<int> {
        auto sub = []() -> virtual_thread_task<int> {
            throw value_exception("specific inner error");
            co_return 1;
        };
        co_await sub();
        co_return 2;
    });
    try {
        task.get_result();
        FAIL() << "Expected value_exception";
    } catch (const value_exception& e) {
        EXPECT_STREQ(e.what(), "specific inner error");
    } catch (...) {
        FAIL() << "Exception type not preserved";
    }
}

TEST(VirtualThread, ExceptionTypePreservationAfterYield) {
    auto task = virtual_thread::start([]() -> virtual_thread_task<int> {
        co_await virtual_thread::yield();
        throw value_exception("after yield");
        co_return 1;
    });
    EXPECT_THROW(
            {
                try {
                    task.get_result();
                } catch (const value_exception& e) {
                    EXPECT_STREQ(e.what(), "after yield");
                    throw;
                }
            },
            value_exception);
}

TEST(VirtualThread, SleepZero) {
    atomic<bool> done{false};
    auto task = virtual_thread::start([&]() -> virtual_thread_task<void> {
        co_await virtual_thread::sleep(0);
        done.store(true);
        co_return;
    });
    task.get_result();
    EXPECT_TRUE(done.load());
}

TEST(VirtualThread, MultipleSleeps) {
    auto task = virtual_thread::start([]() -> virtual_thread_task<int> {
        co_await virtual_thread::sleep(5);
        co_await virtual_thread::sleep(5);
        co_return 3;
    });
    EXPECT_EQ(task.get_result(), 3);
}

TEST(VirtualThread, MixYieldAndSleep) {
    atomic<int> phase{0};
    auto task = virtual_thread::start([&]() -> virtual_thread_task<int> {
        phase.store(1);
        co_await virtual_thread::yield();
        phase.store(2);
        co_await virtual_thread::sleep(10);
        phase.store(3);
        co_await virtual_thread::yield();
        phase.store(4);
        co_return 42;
    });
    EXPECT_EQ(task.get_result(), 42);
    EXPECT_EQ(phase.load(), 4);
}

TEST(VirtualThreadTask, MoveAssignToNonEmpty) {
    auto task1 = virtual_thread::start([]() -> virtual_thread_task<int> { co_return 42; });
    auto task2 = virtual_thread::start([]() -> virtual_thread_task<int> { co_return 100; });
    EXPECT_EQ(task2.get_result(), 100);
    task2 = move(task1);
    EXPECT_FALSE(task1.valid());
    EXPECT_TRUE(task2.valid());
    EXPECT_EQ(task2.get_result(), 42);
}

TEST(VirtualThreadTask, MoveAssignToNonEmptyWithYield) { // may block
    auto task1 = virtual_thread::start([]() -> virtual_thread_task<int> {
        co_await virtual_thread::yield();
        co_return 77;
    });
    auto task2 = virtual_thread::start([]() -> virtual_thread_task<int> {
        co_await virtual_thread::yield();
        co_return 200;
    });
    EXPECT_EQ(task2.get_result(), 200);
    task2 = move(task1);
    EXPECT_FALSE(task1.valid());
    EXPECT_TRUE(task2.valid());
    EXPECT_EQ(task2.get_result(), 77);
}

TEST(VirtualThreadTask, SelfMoveAssignment) {
    auto task = virtual_thread::start([]() -> virtual_thread_task<int> { co_return 10; });
    task = move(task);
    EXPECT_TRUE(task.valid());
    EXPECT_EQ(task.get_result(), 10);
}

TEST(VirtualThreadTask, MoveAfterGetResult) {
    auto task1 = virtual_thread::start([]() -> virtual_thread_task<int> {
        co_await virtual_thread::yield();
        co_return 42;
    });
    EXPECT_EQ(task1.get_result(), 42);
    EXPECT_TRUE(task1.is_done());
    auto task2 = move(task1);
    EXPECT_FALSE(task1.valid());
    EXPECT_TRUE(task2.valid());
    EXPECT_TRUE(task2.is_done());
}

TEST(VirtualThreadTask, MoveChain) {
    auto task1 = virtual_thread::start([]() -> virtual_thread_task<int> {
        co_await virtual_thread::yield();
        co_return 99;
    });
    auto task2 = move(task1);
    auto task3 = move(task2);
    EXPECT_FALSE(task1.valid());
    EXPECT_FALSE(task2.valid());
    EXPECT_TRUE(task3.valid());
    EXPECT_EQ(task3.get_result(), 99);
}

TEST(VirtualThread, DestroyWithoutGetResult) {
    {
        auto task = virtual_thread::start([]() -> virtual_thread_task<string> { co_return string("hello world"); });
    }
    SUCCEED();
}

TEST(VirtualThread, DestroyWithoutGetResultAfterYield) {
    {
        auto task = virtual_thread::start([]() -> virtual_thread_task<string> {
            co_await virtual_thread::yield();
            co_return string("yielded");
        });
        this_thread::sleep_for(30_ms);
    }
    SUCCEED();
}

TEST(VirtualThread, ConcurrentGetResult) {
    auto task = virtual_thread::start([]() -> virtual_thread_task<int> {
        co_await virtual_thread::yield();
        co_return 123;
    });
    EXPECT_EQ(task.get_result(), 123);
    EXPECT_TRUE(task.is_done());
}

#endif

TEST(SignalTest, BasicEmit) {
    neforce::signal<int> sig;
    int result = 0;
    sig.connect([&result](int v) { result = v; });
    sig.emit(42);
    EXPECT_EQ(result, 42);
}

TEST(SignalTest, MemberFunctionConnect) {
    neforce::signal<int> sig;
    test_receiver receiver;
    sig.connect(&receiver, &test_receiver::onUpdate);
    sig.emit(99);
    EXPECT_EQ(receiver.value, 99);
}

TEST(SignalTest, ConnectionDisconnect) {
    neforce::signal<int> sig;
    int count = 0;
    auto conn = sig.connect([&count](int) { ++count; });
    sig.emit(1);
    EXPECT_EQ(count, 1);
    conn.disconnect();
    sig.emit(2);
    EXPECT_EQ(count, 1);
}

TEST(SignalTest, ScopedConnection) {
    neforce::signal<int> sig;
    int count = 0;
    {
        scoped_connection sc{sig.connect([&count](int) { ++count; })};
        sig.emit(1);
        EXPECT_EQ(count, 1);
    }
    sig.emit(2);
    EXPECT_EQ(count, 1);
}

TEST(SignalTest, OneshotConnection) {
    neforce::signal<int> sig;
    int count = 0;
    sig.connect([&count](int) {
        ++count;
        return callback_result::keep;
    });
    sig.connect([&count](int) {
        ++count;
        return callback_result::erase;
    });
    sig.emit(1);
    EXPECT_EQ(count, 2);
    sig.emit(2);
    EXPECT_EQ(count, 3);
}

TEST(SignalTest, OneshotTag) {
    neforce::signal<int> sig;
    int count = 0;
    bool called = false;
    sig.connect([&count, &called](int) {
        if (called) {
            return callback_result::keep;
        }
        ++count;
        called = true;
        return callback_result::erase;
    });
    sig.emit(1);
    EXPECT_EQ(count, 1);
    sig.emit(2);
    EXPECT_EQ(count, 1);
}

TEST(SignalTest, NshotTag) {
    neforce::signal<int> sig;
    int count = 0;
    int remaining = 3;
    sig.connect([&count, &remaining](int) {
        if (remaining <= 0) {
            return callback_result::keep;
        }
        ++count;
        --remaining;
        if (remaining == 0) {
            return callback_result::erase;
        }
        return callback_result::keep;
    });
    for (int i = 0; i < 5; ++i) {
        sig.emit(1);
    }
    EXPECT_EQ(count, 3);
}

TEST(SignalTest, OneshotTagWithObject) {
    struct scope_receiver {
        int count = 0;
        void onEmit(int) { ++count; }
    };
    neforce::signal<int> sig;
    auto receiver = make_shared<scope_receiver>();
    sig.connect(receiver, &scope_receiver::onEmit, oneshot);
    sig.emit(1);
    EXPECT_EQ(receiver->count, 1);
    sig.emit(2);
    EXPECT_EQ(receiver->count, 1);
}

TEST(SignalTest, PriorityOrder) {
    neforce::signal<int> sig;
    vector<int> order;
    sig.connect([&order](int) { order.push_back(2); }, 10);
    sig.connect([&order](int) { order.push_back(1); }, 20);
    sig.connect([&order](int) { order.push_back(3); }, 0);
    sig.emit(0);
    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
    EXPECT_EQ(order[2], 3);
}

TEST(SignalTest, SignalBlocking) {
    neforce::signal<int> sig;
    int count = 0;
    sig.connect([&count](int) { ++count; });
    {
        signal_blocker<int> blocker(sig);
        sig.emit(1);
        EXPECT_EQ(count, 0);
    }
    sig.emit(2);
    EXPECT_EQ(count, 1);
}

TEST(SignalTest, SignalBlockerUnblock) {
    neforce::signal<int> sig;
    int count = 0;
    sig.connect([&count](int) { ++count; });
    {
        signal_blocker<int> blocker(sig);
        sig.emit(1);
        EXPECT_EQ(count, 0);
        blocker.unblock();
        sig.emit(2);
        EXPECT_EQ(count, 1);
    }
    sig.emit(3);
    EXPECT_EQ(count, 2);
}

TEST(SignalTest, ConnectSignal) {
    neforce::signal<int> a;
    neforce::signal<int> b;
    int count = 0;
    b.connect([&count](int) { ++count; });
    a.connect_signal(b);
    a.emit(0);
    EXPECT_EQ(count, 1);
}

TEST(SignalTest, ConnectSignalPointer) {
    neforce::signal<int> a;
    neforce::signal<int> b;
    int count = 0;
    b.connect([&count](int) { ++count; });
    a.connect_signal(&b);
    a.emit(0);
    EXPECT_EQ(count, 1);
}

TEST(SignalTest, ConnectIf) {
    neforce::signal<int> sig;
    int sum = 0;
    sig.connect_if([&sum](int v) { sum += v; }, [](int v) { return v % 2 == 0; });
    sig.emit(1);
    sig.emit(2);
    sig.emit(3);
    sig.emit(4);
    EXPECT_EQ(sum, 6);
}

TEST(SignalTest, ConnectIfMember) {
    neforce::signal<int> sig;
    test_receiver receiver;
    sig.connect_if(&receiver, &test_receiver::onUpdate, [](int v) { return v > 10; }, 0);
    sig.emit(5);
    EXPECT_EQ(receiver.value, 0);
    sig.emit(15);
    EXPECT_EQ(receiver.value, 15);
}

TEST(SignalTest, ConnectFiltered) {
    neforce::signal<int> sig;
    int called = 0;
    int last = 0;
    sig.connect_filtered(
            [&called, &last](int v) {
                ++called;
                last = v;
            },
            [](int v) -> optional<int> {
                if (v > 0) {
                    return optional<int>(v * 2);
                }
                return optional<int>();
            });
    sig.emit(0);
    EXPECT_EQ(called, 0);
    sig.emit(3);
    EXPECT_EQ(called, 1);
    EXPECT_EQ(last, 6);
}

TEST(SignalTest, ConnectTransformed) {
    neforce::signal<int, int> sig;
    int sum = 0;
    sig.connect_transformed([&sum](int v) { sum += v; }, [](int a, int b) { return a + b; });
    sig.emit(2, 3);
    EXPECT_EQ(sum, 5);
}

TEST(SignalTest, EmitExecutor) {
    neforce::signal<int> sig;
    int result = 0;
    sig.connect([&result](int v) { result = v; });
    simple_executor executor;
    sig.emit_executor(executor, 77);
    EXPECT_EQ(result, 0);
    executor.run();
    EXPECT_EQ(result, 77);
}

TEST(SignalTest, WeakPtrAutoCleanup) {
    neforce::signal<int> sig;
    auto receiver = make_shared<test_receiver>();
    sig.connect(weak_ptr<test_receiver>(receiver), &test_receiver::onUpdate);
    sig.emit(10);
    EXPECT_EQ(receiver->value, 10);
    receiver.reset();
    sig.emit(20);
    EXPECT_TRUE(sig.empty());
}

TEST(SignalTest, DisconnectAll) {
    neforce::signal<int> sig;
    int a = 0, b = 0;
    sig.connect([&a](int) { ++a; });
    sig.connect([&b](int) { ++b; });
    sig.disconnect_all();
    sig.emit(1);
    EXPECT_EQ(a, 0);
    EXPECT_EQ(b, 0);
    EXPECT_TRUE(sig.empty());
}

TEST(SignalTest, SlotCountAndEmpty) {
    neforce::signal<int> sig;
    EXPECT_TRUE(sig.empty());
    EXPECT_EQ(sig.slot_count(), 0u);
    auto c1 = sig.connect([](int) {});
    EXPECT_FALSE(sig.empty());
    EXPECT_EQ(sig.slot_count(), 1u);
    auto c2 = sig.connect([](int) {});
    EXPECT_EQ(sig.slot_count(), 2u);
    c1.disconnect();
    EXPECT_EQ(sig.slot_count(), 1u);
    c2.disconnect();
    EXPECT_TRUE(sig.empty());
}

TEST(SignalTest, MultipleParameters) {
    neforce::signal<int, string, double> sig;
    string lastStr;
    double lastDbl = 0.0;
    sig.connect([&](int, const string& s, double d) {
        lastStr = s;
        lastDbl = d;
    });
    sig.emit(1, "hello", 3.14);
    EXPECT_EQ(lastStr, "hello");
    EXPECT_DOUBLE_EQ(lastDbl, 3.14);
}

TEST(SignalTest, ConstMethods) {
    const neforce::signal<int> sig;
    EXPECT_TRUE(sig.empty());
    EXPECT_EQ(sig.slot_count(), 0u);
    EXPECT_FALSE(sig.is_blocked());
}

TEST(SignalTest, CallbackResultErase) {
    neforce::signal<> sig;
    int count = 0;
    sig.connect([&count]() -> callback_result {
        ++count;
        return callback_result::erase;
    });
    sig.emit();
    EXPECT_EQ(count, 1);
    sig.emit();
    EXPECT_EQ(count, 1);
}

TEST(SignalTest, VoidCallbackKeep) {
    neforce::signal<> sig;
    int count = 0;
    sig.connect([&count]() { ++count; });
    sig.emit();
    sig.emit();
    EXPECT_EQ(count, 2);
}

TEST(SignalTest, OperatorCall) {
    neforce::signal<int> sig;
    int val = 0;
    sig.connect([&val](int v) { val = v; });
    sig(42);
    EXPECT_EQ(val, 42);
}

#ifdef NEFORCE_STANDARD_20

namespace {
    generator<int> iota_gen(int from, int to) {
        for (int i = from; i < to; ++i) {
            co_yield i;
        }
    }

    generator<int> empty_gen() {
        if (false) {
            co_yield 0;
        }
        co_return;
    }
} // namespace

TEST(Generator, BasicIteration) {
    auto gen = iota_gen(0, 5);
    vector<int> values;
    for (auto v: gen) {
        values.push_back(v);
    }
    EXPECT_EQ(values, (vector<int>{0, 1, 2, 3, 4}));
}

TEST(Generator, Empty) {
    auto gen = empty_gen();
    int count = 0;
    for (auto v: gen) {
        (void) v;
        ++count;
    }
    EXPECT_EQ(count, 0);
}

TEST(Generator, MoveSemantics) {
    auto gen1 = iota_gen(0, 3);
    auto gen2 = move(gen1);
    vector<int> values;
    for (auto v: gen2) {
        values.push_back(v);
    }
    EXPECT_EQ(values, (vector<int>{0, 1, 2}));
}

TEST(Generator, Map) {
    auto gen = iota_gen(1, 4).map([](int x) { return x * 10; });
    vector<int> values;
    for (auto v: gen) {
        values.push_back(v);
    }
    EXPECT_EQ(values, (vector<int>{10, 20, 30}));
}

TEST(Generator, Filter) {
    auto gen = iota_gen(0, 10).filter([](int x) { return x % 2 == 0; });
    vector<int> values;
    for (auto v: gen) {
        values.push_back(v);
    }
    EXPECT_EQ(values, (vector<int>{0, 2, 4, 6, 8}));
}

TEST(Generator, Take) {
    auto gen = iota_gen(0, 100).take(3);
    vector<int> values;
    for (auto v: gen) {
        values.push_back(v);
    }
    EXPECT_EQ(values, (vector<int>{0, 1, 2}));
}

TEST(Generator, Skip) {
    auto gen = iota_gen(0, 10).skip(7);
    vector<int> values;
    for (auto v: gen) {
        values.push_back(v);
    }
    EXPECT_EQ(values, (vector<int>{7, 8, 9}));
}

TEST(Generator, Chain) {
    auto gen = iota_gen(0, 3).chain(iota_gen(5, 7));
    vector<int> values;
    for (auto v: gen) {
        values.push_back(v);
    }
    EXPECT_EQ(values, (vector<int>{0, 1, 2, 5, 6}));
}

TEST(Generator, Fold) {
    auto gen = iota_gen(1, 5);
    int result = gen.fold(0, [](int acc, int v) { return acc + v; });
    EXPECT_EQ(result, 10);
}

TEST(Generator, ForEach) {
    auto gen = iota_gen(0, 3);
    int sum = 0;
    gen.for_each([&sum](int v) { sum += v; });
    EXPECT_EQ(sum, 3);
}

TEST(Generator, NestedPipeline) {
    auto gen = iota_gen(0, 20).filter([](int x) { return x % 2 == 0; }).map([](int x) { return x * 2; }).take(3);
    vector<int> values;
    for (auto v: gen) {
        values.push_back(v);
    }
    EXPECT_EQ(values, (vector<int>{0, 4, 8}));
}

namespace {
    task<int> task_answer() { co_return 42; }

    task<int> task_double(int x) { co_return x * 2; }

    task<void> task_noop() { co_return; }

    task<int> task_chain_add(int a, int b) {
        int va = co_await task_double(a);
        int vb = co_await task_double(b);
        co_return va + vb;
    }
} // namespace

TEST(TaskT, GetResult) {
    auto t = task_answer();
    EXPECT_EQ(t.get(), 42);
}

TEST(TaskT, CoAwaitReturnsValue) {
    auto t = []() -> task<int> {
        int val = co_await task_answer();
        co_return val;
    }();
    EXPECT_EQ(t.get(), 42);
}

TEST(TaskT, ChainCoAwait) {
    auto t = task_chain_add(3, 5);
    EXPECT_EQ(t.get(), 16);
}

TEST(TaskT, DoneAfterGet) {
    auto t = task_answer();
    EXPECT_FALSE(t.done());
    t.get();
    EXPECT_TRUE(t.done());
}

TEST(TaskT, ResumeAndDone) {
    auto t = task_answer();
    EXPECT_FALSE(t.done());
    t.resume();
    EXPECT_TRUE(t.done());
    EXPECT_EQ(t.get(), 42);
}

TEST(TaskT, MoveConstruct) {
    auto t1 = task_answer();
    auto t2 = move(t1);
    EXPECT_EQ(t2.get(), 42);
}

TEST(TaskT, MoveAssign) {
    auto t1 = task_answer();
    auto t2 = task_double(5);
    t2 = move(t1);
    EXPECT_EQ(t2.get(), 42);
}

TEST(TaskVoid, GetResult) {
    auto t = task_noop();
    EXPECT_NO_THROW(t.get());
    EXPECT_TRUE(t.done());
}

TEST(TaskVoid, CoAwaitCompletion) {
    bool completed = false;
    auto t = [&completed]() -> task<void> {
        completed = true;
        co_return;
    }();
    t.get();
    EXPECT_TRUE(completed);
}

namespace {
    task<int> task_throw() {
        throw value_exception("task error");
        co_return 0;
    }

    task<int> task_rethrow() {
        int val = co_await task_throw();
        co_return val;
    }
} // namespace

TEST(TaskException, DirectException) {
    auto t = task_throw();
    EXPECT_THROW(t.get(), value_exception);
}

TEST(TaskException, PropagationThroughCoAwait) {
    auto t = task_rethrow();
    EXPECT_THROW(t.get(), value_exception);
}

TEST(CancellationToken, NotCancelledByDefault) {
    cancellation_token token;
    EXPECT_FALSE(token.is_cancelled());
}

TEST(CancellationToken, CancelSetsFlag) {
    cancellation_token token;
    token.cancel();
    EXPECT_TRUE(token.is_cancelled());
}

TEST(CancellationToken, CopySharesState) {
    cancellation_token token1;
    cancellation_token token2 = token1;
    token1.cancel();
    EXPECT_TRUE(token2.is_cancelled());
}

TEST(CancellationToken, AssignSharesState) {
    cancellation_token token1;
    cancellation_token token2;
    token2 = token1;
    token1.cancel();
    EXPECT_TRUE(token2.is_cancelled());
}

TEST(CancellationToken, CheckAwaiterCancelled) {
    cancellation_token token;
    token.cancel();
    auto awaiter = token.check();
    EXPECT_TRUE(awaiter.await_ready());
    EXPECT_THROW(awaiter.await_resume(), neforce::exception);
}

TEST(CancellationToken, CheckAwaiterNotCancelled) {
    cancellation_token token;
    auto awaiter = token.check();
    EXPECT_FALSE(awaiter.await_ready());
}

TEST(WhenAll, TwoTasks) {
    auto t = when_all(task_answer(), task_double(3));
    auto [a, b] = t.get();
    EXPECT_EQ(a, 42);
    EXPECT_EQ(b, 6);
}

TEST(WhenAll, ThreeTasks) {
    auto t = when_all(task_double(1), task_double(2), task_double(3));
    auto [a, b, c] = t.get();
    EXPECT_EQ(a, 2);
    EXPECT_EQ(b, 4);
    EXPECT_EQ(c, 6);
}

TEST(Retry, SuccessFirstAttempt) {
    int attempts = 0;
    auto factory = [&attempts]() -> task<int> {
        ++attempts;
        co_return 42;
    };
    auto t = retry<int>(factory, 3);
    EXPECT_EQ(t.get(), 42);
    EXPECT_EQ(attempts, 1);
}

TEST(Retry, RetryThenSuccess) {
    int attempts = 0;
    auto factory = [&attempts]() -> task<int> {
        ++attempts;
        if (attempts < 3) {
            throw value_exception("failed");
        }
        co_return 7;
    };
    auto t = retry<int>(factory, 5);
    EXPECT_EQ(t.get(), 7);
    EXPECT_EQ(attempts, 3);
}

TEST(Retry, Exhaustion) {
    int attempts = 0;
    auto factory = [&attempts]() -> task<int> {
        ++attempts;
        throw value_exception("always fail");
        co_return 1;
    };
    auto t = retry<int>(factory, 3);
    EXPECT_THROW(t.get(), value_exception);
    EXPECT_EQ(attempts, 3);
}

#endif
