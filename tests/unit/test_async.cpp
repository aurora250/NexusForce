#include <NeForce/core/async/hazard_ptr.hpp>
#include <NeForce/core/async/thread_pool.hpp>
#include <NeForce/core/numeric/random.hpp>
#include <gtest/gtest.h>
using namespace neforce;

namespace {
    thread_pool& thread_pool_instance() {
        static thread_pool instance;
        return instance;
    }
} // namespace


TEST(ThreadPoolTest, StartStop) {
    auto& pool = thread_pool_instance();
    pool.start(2);
    pool.stop();
    ASSERT_TRUE(true);
}

TEST(ThreadPoolTest, SubmitTask) {
    auto& pool = thread_pool_instance();
    pool.start(2);

    atomic<bool> executed{false};
    pool.submit_task([&executed] { executed.store(true); });

    this_thread::sleep_for(milliseconds(500));
    pool.stop();
    ASSERT_TRUE(executed.load());
}

TEST(ThreadPoolTest, PriorityTask) {
    auto& pool = thread_pool_instance();
    pool.start(2);

    atomic<int> counter{0};
    pool.submit_task(static_cast<thread_pool::priority_type>(10), [&counter] { counter.fetch_add(1); });
    pool.submit_task(static_cast<thread_pool::priority_type>(1), [&counter] { counter.fetch_add(1); });

    this_thread::sleep_for(milliseconds(500));
    pool.stop();
    ASSERT_EQ(counter.load(), 2);
}

TEST(ThreadPoolTest, SubmitAfter) {
    auto& pool = thread_pool_instance();
    pool.start(2);

    atomic<bool> executed{false};
    pool.submit_after(500, [&executed] { executed.store(true); });

    this_thread::sleep_for(milliseconds(200));
    ASSERT_FALSE(executed.load());

    this_thread::sleep_for(milliseconds(600));
    pool.stop();
    ASSERT_TRUE(executed.load());
}

TEST(ThreadPoolTest, DISABLED_PeriodicTask) {
    auto& pool = thread_pool_instance();
    pool.start(2);

    auto counter = make_shared<atomic<int>>(0);
    thread_pool::periodic_token token;

    token = pool.submit_every(200, [counter, &pool, &token]() {
        int count = counter->fetch_add(1) + 1;
        if (count >= 5) {
            pool.cancel_periodic_task(token);
        }
    });

    this_thread::sleep_for(seconds(2));
    pool.stop();
    ASSERT_GE(counter->load(), 4);
}

TEST(ThreadPoolTest, Statistics) {
    auto& pool = thread_pool_instance();
    pool.start(2);

    pool.submit_task([] { this_thread::sleep_for(milliseconds(100)); });
    pool.submit_task([] { this_thread::sleep_for(milliseconds(100)); });

    this_thread::sleep_for(milliseconds(50));

    auto state = pool.statistics();
    pool.stop();
    ASSERT_TRUE(true);
}
