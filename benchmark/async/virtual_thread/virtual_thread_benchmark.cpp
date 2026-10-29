#include <NeForce/core/async/future.hpp>
#include <NeForce/core/async/packaged_task.hpp>
#include <NeForce/core/async/virtual_thread.hpp>
#include <NeForce/core/container/deque.hpp>
#include <NeForce/core/exception/terminate.hpp>
#include <NeForce/core/functional/function.hpp>
#include <NeForce/core/memory/shared_ptr.hpp>
#include <NeForce/core/numeric/numeric_traits.hpp>
#include <benchmark/benchmark.h>
#include <ctime>
using namespace neforce;

using clock_point = steady_clock::time_point;

NEFORCE_NODISCARD inline double elapsed_us(const clock_point& from, const clock_point& to) {
    return duration<double, micro>(to - from).count();
}

class mean_counter {
public:
    explicit mean_counter(std::string name) :
    name_(std::move(name)) {}

    void add(double value) {
        sum_ += value;
        ++count_;
        if (count_ == 1 || value > max_) {
            max_ = value;
        }
    }

    NEFORCE_NODISCARD double mean() const { return count_ == 0 ? 0.0 : sum_ / static_cast<double>(count_); }

    NEFORCE_NODISCARD double max() const { return max_; }

    void publish(benchmark::State& state) const {
        state.counters[name_] = benchmark::Counter(mean());
        state.counters[name_ + "_max"] = benchmark::Counter(max_);
    }

private:
    std::string name_;
    double sum_{0.0};
    double max_{0.0};
    int64_t count_{0};
};

#ifdef NEFORCE_STANDARD_20

#    include <coroutine>

namespace {
    // Same work unit as the thread pool benchmark: a xorshift64 chain carries a
    // serial dependency, so it can neither be folded nor vectorized and its cost
    // is identical on every worker. BM_Ref_BurnCalibration converts it to ns.
    inline void burn(uint64_t iterations) {
        uint64_t x = 0x9e3779b97f4a7c15ULL;
        for (uint64_t i = 0; i < iterations; ++i) {
            x ^= x << 13;
            x ^= x >> 7;
            x ^= x << 17;
        }
        benchmark::DoNotOptimize(x);
        benchmark::ClobberMemory();
    }

    // The scheduler is a process-wide singleton with an explicit lifecycle, so every
    // benchmark owns a scope: worker threads are started before the timed loop and
    // joined after it. Restarting is what makes a worker-count sweep possible in a
    // single process.
    class scheduler_scope {
    public:
        explicit scheduler_scope(const size_t workers) { virtual_thread::initialize(workers); }
        ~scheduler_scope() { virtual_thread::shutdown(); }

        scheduler_scope(const scheduler_scope&) = delete;
        scheduler_scope& operator=(const scheduler_scope&) = delete;
        scheduler_scope(scheduler_scope&&) = delete;
        scheduler_scope& operator=(scheduler_scope&&) = delete;
    };

    // Reference implementation: mutex + condition_variable worker pool with the same
    // "hand a task over, wait until it ran" contract as start() + get_result().
    class ref_pool {
    public:
        explicit ref_pool(const size_t workers) {
            workers_.reserve(workers);
            for (size_t i = 0; i < workers; ++i) {
                workers_.emplace_back([this] { worker_loop(); });
            }
        }

        ~ref_pool() { stop(); }

        ref_pool(const ref_pool&) = delete;
        ref_pool& operator=(const ref_pool&) = delete;

        future<void> submit(function<void()> fn) {
            auto task = make_shared<packaged_task<void()>>(_NEFORCE move(fn));
            auto fut = task->get_future();
            {
                lock<mutex> lk(mtx_);
                tasks_.push_back([task] { (*task)(); });
            }
            cv_.notify_one();
            return fut;
        }

    private:
        void worker_loop() {
            for (;;) {
                function<void()> task;
                {
                    unique_lock<mutex> lk(mtx_);
                    cv_.wait(lk, [this] { return stopped_ || !tasks_.empty(); });
                    if (tasks_.empty()) {
                        return;
                    }
                    task = _NEFORCE move(tasks_.front());
                    tasks_.pop_front();
                }
                task();
            }
        }

        void stop() {
            {
                lock<mutex> lk(mtx_);
                if (stopped_) {
                    return;
                }
                stopped_ = true;
            }
            cv_.notify_all();
            for (auto& worker: workers_) {
                if (worker.joinable()) {
                    worker.join();
                }
            }
            workers_.clear();
        }

        mutex mtx_;
        condition_variable cv_;
        deque<function<void()>> tasks_;
        vector<thread> workers_;
        bool stopped_{false};
    };

    // Floor for the coroutine machinery alone: one frame allocation, two resumes and
    // the frame destruction, without shared state, scheduler or condition variables.
    struct bare_task {
        struct promise_type {
            bare_task get_return_object() noexcept {
                return bare_task{coroutine_handle<promise_type>::from_promise(*this)};
            }
            suspend_always initial_suspend() noexcept { return {}; }
            suspend_always final_suspend() noexcept { return {}; }
            void return_void() noexcept {}
            void unhandled_exception() { terminate(); }
        };

        coroutine_handle<promise_type> handle;
    };

    bare_task bare_coroutine(volatile uint64_t& sink) {
        co_await suspend_always{};
        sink = sink + 1;
    }
} // namespace

// ============================================================
// 0. Work unit calibration
// ============================================================

static void BM_Ref_BurnCalibration(benchmark::State& state) {
    const auto iterations = static_cast<uint64_t>(state.range(0));

    for (auto _: state) {
        burn(iterations);
    }

    state.SetItemsProcessed(state.range(0) * state.iterations());
    state.SetLabel("burn(" + std::to_string(iterations) + ")");
}
BENCHMARK(BM_Ref_BurnCalibration)->Arg(1000)->Arg(10000)->Arg(100000)->Unit(benchmark::kNanosecond);

// ============================================================
// 1. Task machinery (no suspension: the body runs inline in start())
// ============================================================

// A task that returns before its first suspension point never reaches a worker:
// start() executes it on the calling thread and final_suspend() only publishes the
// completion. What remains is shared-state allocation, frame allocation, the
// reference counts and one mutex + condition_variable notification.
static void BM_VirtualThread_NoopTask(benchmark::State& state) {
    auto body = []() -> virtual_thread_task<void> { co_return; };

    for (auto _: state) {
        auto task = virtual_thread::start(body);
        task.get_result();
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_VirtualThread_NoopTask)->Unit(benchmark::kNanosecond);

// Same inline task without touching get_result(): the delta against NoopTask is the
// cost of the result accessor, including its "already completed" fast path.
static void BM_VirtualThread_NoopTaskNoResult(benchmark::State& state) {
    auto body = []() -> virtual_thread_task<void> { co_return; };

    for (auto _: state) {
        auto task = virtual_thread::start(body);
        benchmark::DoNotOptimize(task.is_done());
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_VirtualThread_NoopTaskNoResult)->Unit(benchmark::kNanosecond);

static void BM_VirtualThread_ResultTask(benchmark::State& state) {
    auto body = []() -> virtual_thread_task<int> { co_return 42; };

    for (auto _: state) {
        auto task = virtual_thread::start(body);
        benchmark::DoNotOptimize(task.get_result());
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_VirtualThread_ResultTask)->Unit(benchmark::kNanosecond);

// ============================================================
// 2. Scheduler round trip (co_await yield)
// ============================================================

// One yield is the cheapest point where the coroutine actually leaves the calling
// thread: the frame is queued, a worker resumes it, completion wakes the waiter.
static void BM_VirtualThread_YieldRoundTrip(benchmark::State& state) {
    const auto workers = static_cast<size_t>(state.range(0));
    scheduler_scope scope(workers);

    auto body = []() -> virtual_thread_task<void> { co_await virtual_thread::yield(); };

    for (auto _: state) {
        auto task = virtual_thread::start(body);
        task.get_result();
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["workers"] = benchmark::Counter(static_cast<double>(workers));
}
BENCHMARK(BM_VirtualThread_YieldRoundTrip)->Arg(1)->Arg(2)->Arg(4)->Arg(8)->Arg(16)->Unit(benchmark::kNanosecond);

// Amortises the completion wait: the same task yields `yields` times, so the
// difference against the 1-yield case is the pure "requeue and resume" cost.
static void BM_VirtualThread_YieldChain(benchmark::State& state) {
    const auto workers = static_cast<size_t>(state.range(0));
    const auto yields = state.range(1);
    scheduler_scope scope(workers);

    auto body = [yields]() -> virtual_thread_task<void> {
        for (int64_t i = 0; i < yields; ++i) {
            co_await virtual_thread::yield();
        }
    };

    for (auto _: state) {
        auto task = virtual_thread::start(body);
        task.get_result();
    }

    state.SetItemsProcessed(state.iterations() * yields);
    state.counters["yields"] = benchmark::Counter(static_cast<double>(yields));
}
BENCHMARK(BM_VirtualThread_YieldChain)
        ->Args({4, 1})
        ->Args({4, 8})
        ->Args({4, 64})
        ->Args({4, 512})
        ->Unit(benchmark::kNanosecond);

// ============================================================
// 3. Throughput
// ============================================================

// Dispatch throughput: every task yields once, so each one costs a queue push, a
// worker wake-up, a resume and a completion notification.
static void BM_VirtualThread_Throughput(benchmark::State& state) {
    const auto workers = static_cast<size_t>(state.range(0));
    const auto task_count = static_cast<size_t>(state.range(1));
    scheduler_scope scope(workers);

    auto body = []() -> virtual_thread_task<void> { co_await virtual_thread::yield(); };
    vector<virtual_thread_task<void>> tasks;
    tasks.reserve(task_count);

    for (auto _: state) {
        for (size_t i = 0; i < task_count; ++i) {
            tasks.push_back(virtual_thread::start(body));
        }
        for (auto& task: tasks) {
            task.get_result();
        }
        tasks.clear();
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(task_count));
    state.counters["workers"] = benchmark::Counter(static_cast<double>(workers));
}
BENCHMARK(BM_VirtualThread_Throughput)
        ->Apply([](benchmark::Benchmark* bench) {
            for (int64_t workers: {1, 4, 16}) {
                for (int64_t tasks: {1000, 10000}) {
                    bench->Args({workers, tasks});
                }
            }
        })
        ->Unit(benchmark::kMicrosecond);

// Production side only (start) and consumption side only (get_result) reported
// separately: a single total hides which of the two is the bottleneck.
static void BM_VirtualThread_MultiProducer(benchmark::State& state) {
    const auto workers = static_cast<size_t>(state.range(0));
    const auto producer_count = static_cast<size_t>(state.range(1));
    const auto tasks_per_producer = state.range(2);
    scheduler_scope scope(workers);

    auto body = []() -> virtual_thread_task<void> { co_await virtual_thread::yield(); };

    mean_counter submit_us("submit_us");
    mean_counter drain_us("drain_us");

    for (auto _: state) {
        vector<vector<virtual_thread_task<void>>> slots(producer_count);
        vector<thread> producers;
        producers.reserve(producer_count);

        const auto t0 = steady_clock::now();
        for (size_t p = 0; p < producer_count; ++p) {
            producers.emplace_back([&slots, &body, p, tasks_per_producer] {
                auto& slot = slots[p];
                slot.reserve(static_cast<size_t>(tasks_per_producer));
                for (int64_t i = 0; i < tasks_per_producer; ++i) {
                    slot.push_back(virtual_thread::start(body));
                }
            });
        }
        for (auto& t: producers) {
            t.join();
        }
        const auto t1 = steady_clock::now();

        for (auto& slot: slots) {
            for (auto& task: slot) {
                task.get_result();
            }
        }
        const auto t2 = steady_clock::now();

        submit_us.add(elapsed_us(t0, t1));
        drain_us.add(elapsed_us(t1, t2));
        state.SetIterationTime(elapsed_us(t0, t2) / 1e6);
    }

    submit_us.publish(state);
    drain_us.publish(state);
}
BENCHMARK(BM_VirtualThread_MultiProducer)
        ->Args({8, 1, 500})
        ->Args({8, 2, 500})
        ->Args({8, 4, 500})
        ->Args({8, 8, 500})
        ->Unit(benchmark::kMicrosecond);

// ============================================================
// 4. Compute scaling
// ============================================================

// Work is dispatched by an initial yield, otherwise start() would run the body
// inline on the calling thread and the scheduler would never be involved.
static void BM_VirtualThread_ComputeScaling(benchmark::State& state) {
    const auto workers = static_cast<size_t>(state.range(0));
    const auto task_count = static_cast<size_t>(state.range(1));
    const auto iterations = static_cast<uint64_t>(state.range(2));
    scheduler_scope scope(workers);

    auto body = [iterations]() -> virtual_thread_task<void> {
        co_await virtual_thread::yield();
        burn(iterations);
    };

    // Serial baseline for the same total amount of work. The best of three samples is
    // the closest to a fully clocked-up core; a single sample swings with the governor.
    double serial_us = numeric_traits<double>::max();
    for (int sample = 0; sample < 3; ++sample) {
        const auto serial_start = steady_clock::now();
        burn(iterations * task_count);
        serial_us = min(serial_us, elapsed_us(serial_start, steady_clock::now()));
    }

    mean_counter makespan_us("makespan_us");
    vector<virtual_thread_task<void>> tasks;
    tasks.reserve(task_count);

    for (auto _: state) {
        const auto t0 = steady_clock::now();
        for (size_t i = 0; i < task_count; ++i) {
            tasks.push_back(virtual_thread::start(body));
        }
        for (auto& task: tasks) {
            task.get_result();
        }
        const auto t1 = steady_clock::now();
        makespan_us.add(elapsed_us(t0, t1));
        state.SetIterationTime(elapsed_us(t0, t1) / 1e6);
        tasks.clear();
    }

    const double makespan = makespan_us.mean();
    makespan_us.publish(state);
    state.counters["serial_us"] = benchmark::Counter(serial_us);
    state.counters["speedup"] = benchmark::Counter(makespan > 0.0 ? serial_us / makespan : 0.0);
    state.counters["efficiency_pct"] =
            benchmark::Counter(makespan > 0.0 ? 100.0 * serial_us / (makespan * static_cast<double>(workers)) : 0.0);
}
BENCHMARK(BM_VirtualThread_ComputeScaling)
        ->Apply([](benchmark::Benchmark* bench) {
            for (int64_t workers: {1, 2, 4, 8, 16}) {
                bench->Args({workers, 256, 50000});
            }
        })
        ->Unit(benchmark::kMicrosecond);

static void BM_Ref_SerialCompute(benchmark::State& state) {
    const auto task_count = static_cast<uint64_t>(state.range(0));
    const auto iterations = static_cast<uint64_t>(state.range(1));

    for (auto _: state) {
        for (uint64_t i = 0; i < task_count; ++i) {
            burn(iterations);
        }
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(task_count * iterations));
}
BENCHMARK(BM_Ref_SerialCompute)->Args({256, 50000})->Unit(benchmark::kMicrosecond);

// ============================================================
// 5. co_await of child tasks
// ============================================================

// Every await of a not-yet-complete task costs two scheduler hops: the child has to
// be resumed on a worker, and its final_suspend() has to schedule the parent back.
static void BM_VirtualThread_NestedAwait(benchmark::State& state) {
    const auto workers = static_cast<size_t>(state.range(0));
    const auto depth = state.range(1);
    scheduler_scope scope(workers);

    auto child_body = []() -> virtual_thread_task<void> { co_await virtual_thread::yield(); };
    auto parent_body = [&child_body, depth]() -> virtual_thread_task<void> {
        for (int64_t i = 0; i < depth; ++i) {
            auto child = virtual_thread::start(child_body);
            co_await child;
        }
    };

    for (auto _: state) {
        auto task = virtual_thread::start(parent_body);
        task.get_result();
    }

    state.SetItemsProcessed(state.iterations() * depth);
    state.counters["depth"] = benchmark::Counter(static_cast<double>(depth));
}
BENCHMARK(BM_VirtualThread_NestedAwait)->Args({4, 1})->Args({4, 32})->Args({4, 256})->Unit(benchmark::kNanosecond);

// Fan-out: the parent starts every child before awaiting any of them, so all child
// frames are queued at once and the scheduler sees a burst.
static void BM_VirtualThread_FanOut(benchmark::State& state) {
    const auto workers = static_cast<size_t>(state.range(0));
    const auto children = state.range(1);
    scheduler_scope scope(workers);

    auto child_body = []() -> virtual_thread_task<int> {
        co_await virtual_thread::yield();
        co_return 1;
    };
    auto parent_body = [&child_body, children]() -> virtual_thread_task<int> {
        vector<virtual_thread_task<int>> tasks;
        tasks.reserve(static_cast<size_t>(children));
        for (int64_t i = 0; i < children; ++i) {
            tasks.push_back(virtual_thread::start(child_body));
        }
        int sum = 0;
        for (auto& task: tasks) {
            sum += co_await task;
        }
        co_return sum;
    };

    for (auto _: state) {
        auto task = virtual_thread::start(parent_body);
        benchmark::DoNotOptimize(task.get_result());
    }

    state.SetItemsProcessed(state.iterations() * children * 2);
    state.counters["children"] = benchmark::Counter(static_cast<double>(children));
}
BENCHMARK(BM_VirtualThread_FanOut)->Args({4, 8})->Args({4, 32})->Args({4, 128})->Unit(benchmark::kMicrosecond);

// ============================================================
// 6. Timed suspension
// ============================================================

// sleep() goes through the scheduler timer heap instead of blocking a worker, so
// the deviation is timer-heap wake-up latency plus the resume hop.
static void BM_VirtualThread_SleepAccuracy(benchmark::State& state) {
    const auto workers = static_cast<size_t>(state.range(0));
    const auto delay_ms = state.range(1);
    scheduler_scope scope(workers);

    auto body = [delay_ms]() -> virtual_thread_task<void> { co_await virtual_thread::sleep(delay_ms); };

    mean_counter deviation_us("deviation_us");

    for (auto _: state) {
        const auto t0 = steady_clock::now();
        auto task = virtual_thread::start(body);
        task.get_result();
        const double elapsed = elapsed_us(t0, steady_clock::now());
        deviation_us.add(elapsed - static_cast<double>(delay_ms) * 1000.0);
        state.SetIterationTime(elapsed / 1e6);
    }

    deviation_us.publish(state);
    state.counters["delay_ms"] = benchmark::Counter(static_cast<double>(delay_ms));
}
BENCHMARK(BM_VirtualThread_SleepAccuracy)
        ->Args({4, 1})
        ->Args({4, 10})
        ->Args({4, 50})
        ->Args({4, 200})
        ->Unit(benchmark::kMicrosecond);

// Environment floor for the table above: the same delay through a plain blocking
// sleep, so the scheduler's own share of the deviation can be separated from the
// wake-up granularity of the machine.
static void BM_Ref_SleepFor(benchmark::State& state) {
    const auto delay_ms = state.range(0);

    mean_counter deviation_us("deviation_us");

    for (auto _: state) {
        const auto t0 = steady_clock::now();
        this_thread::sleep_for(milliseconds(delay_ms));
        const double elapsed = elapsed_us(t0, steady_clock::now());
        deviation_us.add(elapsed - static_cast<double>(delay_ms) * 1000.0);
        state.SetIterationTime(elapsed / 1e6);
    }

    deviation_us.publish(state);
}
BENCHMARK(BM_Ref_SleepFor)->Args({1})->Args({10})->Args({200})->Unit(benchmark::kMicrosecond);

// ============================================================
// 7. Idle cost
// ============================================================

// Parked workers must cost nothing: they block on the scheduler condition variable.
static void BM_VirtualThread_IdleCpu(benchmark::State& state) {
    const auto workers = static_cast<size_t>(state.range(0));
    constexpr int64_t sample_ms = 50;
    scheduler_scope scope(workers);

    this_thread::sleep_for(milliseconds(200));

    mean_counter cpu_ms("idle_cpu_ms");
    mean_counter wall_ms("idle_wall_ms");

    for (auto _: state) {
        const ::clock_t cpu0 = ::clock();
        const auto t0 = steady_clock::now();
        this_thread::sleep_for(milliseconds(sample_ms));
        const auto t1 = steady_clock::now();
        const ::clock_t cpu1 = ::clock();

        cpu_ms.add(1000.0 * static_cast<double>(cpu1 - cpu0) / static_cast<double>(CLOCKS_PER_SEC));
        wall_ms.add(elapsed_us(t0, t1) / 1000.0);
    }

    cpu_ms.publish(state);
    wall_ms.publish(state);

    const double cpu_percent = wall_ms.mean() > 0.0 ? cpu_ms.mean() / wall_ms.mean() * 100.0 : 0.0;
    state.counters["idle_cpu_pct_of_one_core"] = benchmark::Counter(cpu_percent);
    state.counters["idle_cpu_pct_per_worker"] = benchmark::Counter(cpu_percent / static_cast<double>(workers));
}
BENCHMARK(BM_VirtualThread_IdleCpu)->Arg(1)->Arg(4)->Arg(16)->Unit(benchmark::kMicrosecond);

// ============================================================
// 8. Reference baselines
// ============================================================

static void BM_Ref_BareCoroutine(benchmark::State& state) {
    volatile uint64_t sink = 0;

    for (auto _: state) {
        auto task = bare_coroutine(sink);
        task.handle.resume();
        task.handle.resume();
        benchmark::DoNotOptimize(task.handle);
        task.handle.destroy();
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Ref_BareCoroutine)->Unit(benchmark::kNanosecond);

static void BM_Ref_StdThreadTask(benchmark::State& state) {
    for (auto _: state) {
        thread task([] {});
        task.join();
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Ref_StdThreadTask)->Unit(benchmark::kMicrosecond);

static void BM_Ref_MutexPoolRoundTrip(benchmark::State& state) {
    const auto workers = static_cast<size_t>(state.range(0));
    ref_pool pool(workers);

    for (auto _: state) {
        auto fut = pool.submit([] {});
        fut.get();
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Ref_MutexPoolRoundTrip)->Arg(1)->Arg(4)->Arg(8)->Unit(benchmark::kNanosecond);

#endif // NEFORCE_STANDARD_20

BENCHMARK_MAIN();
