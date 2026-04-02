#include <NeForce/core/async/thread_pool.hpp>
#include <NeForce/core/file/file.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/core/time/click.hpp>

using namespace neforce;

static auto& pool() {
    static thread_pool instance;
    return instance;
}

static atomic_int32_t counter{0};

auto io_intensive = [] {
    path pth("test" + to_string(counter++));
    if (!(file::create_and_write(pth, "test data") || pth.remove())) {
        printcln(color::red(), "FILE OPERATE ERROR AT", pth);
    }
};

auto mixed_workload = [](bool is_long) {
    if (is_long) {
        this_thread::sleep_for(seconds(1));
    } else {
        volatile int sum = 0;
        for (int i = 0; i < 1000; ++i) {
            sum += i;
        }
    }
};

auto burst_test = [] {
    for (int i = 0; i < 10000; ++i) {
        pool().submit_task([]() { this_thread::sleep_for(milliseconds(10)); });
    }
    this_thread::sleep_for(seconds(5));
};

int main() {
    auto cpu_intensive = [](int n) {
        double result = 0.0;
        for (int i = 0; i < n; ++i) {
            result += square_root(i) * sine(i);
        }
        return result;
    };

    click clk;
    auto& pool = ::pool();

    pool.set_mode(THREAD_POOL_MODE::MODE_FIXED);
    pool.set_steal_mode(STEAL_STRATEGY::HALF);
    pool.start();

    {
        scoped_click grd(clk);
        pool.submit_task([&pool, cpu_intensive] {
            for (int i = 1; i <= 1000; ++i) {
                pool.submit_task(cpu_intensive, i);
            }
        });
    }
    auto stat1 = pool.stop();
    auto cost1 = clk.during().count();
    println(stat1);
    println(cost1);
    println();

    pool.set_steal_mode(STEAL_STRATEGY::SINGLE);
    pool.start();

    {
        scoped_click grd(clk);
        pool.submit_task([&pool, cpu_intensive] {
            for (int i = 1; i <= 1000; ++i) {
                pool.submit_task(cpu_intensive, i);
            }
        });
    }
    auto stat2 = pool.stop();
    auto cost2 = clk.during().count();
    println(stat2);
    println(cost2);
    println();

    pool.set_steal_mode(STEAL_STRATEGY::FIXED_BATCH);
    pool.start();

    {
        scoped_click grd(clk);
        pool.submit_task([&pool, cpu_intensive] {
            for (int i = 1; i <= 1000; ++i) {
                pool.submit_task(cpu_intensive, i);
            }
        });
    }
    auto stat3 = pool.stop();
    auto cost3 = clk.during().count();
    println(stat3);
    println(cost3);
    println();

    pool.set_steal_mode(STEAL_STRATEGY::ADAPTIVE);
    pool.start();

    {
        scoped_click grd(clk);
        pool.submit_task([&pool, cpu_intensive] {
            for (int i = 1; i <= 1000; ++i) {
                pool.submit_task(cpu_intensive, i);
            }
        });
    }
    auto stat4 = pool.stop();
    auto cost4 = clk.during().count();
    println(stat4);
    println(cost4);
    println();

    console.pause();
}
