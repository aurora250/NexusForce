#include <NeForce/core/async/async.hpp>
#include <NeForce/core/async/barrier.hpp>
#include <NeForce/core/async/hazard_ptr.hpp>
#include <NeForce/core/async/latch.hpp>
#include <NeForce/core/async/scoped_thread.hpp>
#include <NeForce/core/async/thread_pool.hpp>
#include <NeForce/core/async/virtual_thread.hpp>
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
} // namespace


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
        b.arrive_and_wait();
        flag.store(true);
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

#if 0

class VirtualThreadEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        virtual_thread::initialize(1);
    }
    void TearDown() override {
    }
};

::testing::Environment* const virtual_thread_env =
    ::testing::AddGlobalTestEnvironment(new VirtualThreadEnvironment);

TEST(VirtualThread, StartBasic) {
    atomic<bool> ran{false};
    auto vt = virtual_thread::start([&] { ran.store(true); });
    this_thread::sleep_for(50_ms);
    EXPECT_TRUE(ran.load());
}

TEST(VirtualThread, Yield) {
    atomic<int> sequence{0};
    atomic<int> result{0};
    auto vt = virtual_thread::start([&] {
        sequence.store(sequence.load() + 1);
        virtual_thread::yield();
        result.store(sequence.load());
    });
    this_thread::sleep_for(50_ms);
    EXPECT_EQ(result.load(), 1);
}

TEST(VirtualThread, MultipleTasks) {
    atomic<int> counter{0};
    constexpr int N = 5;
    vector<virtual_thread> threads;
    for (int i = 0; i < N; ++i) {
        threads.push_back(virtual_thread::start([&] { counter.fetch_add(1); }));
    }
    this_thread::sleep_for(100_ms);
    EXPECT_EQ(counter.load(), N);
}

TEST(VirtualThread, ExceptionHandling) {
    auto vt = virtual_thread::start([] { throw exception("test"); });
    this_thread::sleep_for(50_ms);
    SUCCEED();
}

TEST(VirtualThread, SleepNoCrash) {
    auto vt = virtual_thread::start([] { virtual_thread::sleep(10); });
    this_thread::sleep_for(50_ms);
    SUCCEED();
}

TEST(VirtualThreadTask, MoveConstructor) {
    auto createTask = []() -> virtual_thread_task {
        co_return;
    };
    virtual_thread_task task1 = createTask();
    EXPECT_TRUE(task1.handle_);
    virtual_thread_task task2(move(task1));
    EXPECT_FALSE(task1.handle_);
    EXPECT_TRUE(task2.handle_);
}

TEST(VirtualThreadTask, MoveAssignment) {
    auto createTask = []() -> virtual_thread_task {
        co_return;
    };
    virtual_thread_task task1 = createTask();
    virtual_thread_task task2;
    task2 = move(task1);
    EXPECT_FALSE(task1.handle_);
    EXPECT_TRUE(task2.handle_);
}

TEST(VirtualThread, MoveVirtualThread) {
    atomic<bool> ran{false};
    auto vt1 = virtual_thread::start([&] { ran.store(true); });
    virtual_thread vt2(move(vt1));
    this_thread::sleep_for(50_ms);
    EXPECT_TRUE(ran.load());
}

#endif
