#include <NeForce/core/async/thread_pool.hpp>
#include <benchmark/benchmark.h>
using namespace neforce;

class mean_counter {
public:
    explicit mean_counter(std::string name) :
    name_(std::move(name)) {}

    void add(double value) {
        sum_ += value;
        ++count_;
    }

    NEFORCE_NODISCARD double mean() const { return count_ == 0 ? 0.0 : sum_ / static_cast<double>(count_); }

    NEFORCE_NODISCARD int64_t count() const { return count_; }

    void publish(benchmark::State& state) const { state.counters[name_] = benchmark::Counter(mean()); }

private:
    std::string name_;
    double sum_{0.0};
    int64_t count_{0};
};

using clock_point = steady_clock::time_point;

// Cost of a single submit() + future.get() round trip is decomposed into the
// producer side (queueing every task) and the consumer side (everything left
// after the last task was handed over). Reporting only the sum hides which side
// is the bottleneck.
NEFORCE_NODISCARD inline double elapsed_us(const clock_point& from, const clock_point& to) {
    return duration<double, micro>(to - from).count();
}

// Synthetic work unit.
//
// The previous implementation summed an arithmetic progression, which LLVM folds
// into a closed form (N*(N-1)/2, emitted as mulq/shldq) at every optimization
// level. Every workload in this file therefore had exactly zero cost, and the
// "compute", "unbalanced workload" and "steal strategy" benchmarks measured
// nothing but submission and dispatch overhead. A xorshift64 carries a serial
// dependency chain, so it can neither be folded nor vectorized nor computed in
// parallel inside one call: the cost is linear in `iterations` and identical on
// every hardware thread. BM_Ref_BurnCalibration converts the unit into ns.
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

// Reference implementation: the textbook mutex + condition_variable pool with
// the same submit()/future contract as thread_pool. Absolute latency numbers are
// meaningless without a baseline to compare them against.
class ref_mutex_pool {
public:
    explicit ref_mutex_pool(size_t thread_count) {
        workers_.reserve(thread_count);
        for (size_t i = 0; i < thread_count; ++i) {
            workers_.emplace_back([this] { worker_loop(); });
        }
    }

    ~ref_mutex_pool() { stop(); }

    ref_mutex_pool(const ref_mutex_pool&) = delete;
    ref_mutex_pool& operator=(const ref_mutex_pool&) = delete;
    ref_mutex_pool(ref_mutex_pool&&) = delete;
    ref_mutex_pool& operator=(ref_mutex_pool&&) = delete;

    future<void> submit(function<void()> fn) {
        auto task = make_shared<packaged_task<void()>>(move(fn));
        auto fut = task->get_future();
        post([task] { (*task)(); });
        return fut;
    }

    // Fire and forget: no shared state, no future. This is the cost floor of
    // "hand a closure to an idle worker and wake it up".
    void post(function<void()> fn) {
        outstanding_.fetch_add(1, memory_order_relaxed);
        {
            lock<mutex> lk(mtx_);
            tasks_.push_back([this, fn = move(fn)] {
                fn();
                outstanding_.fetch_sub(1, memory_order_release);
            });
        }
        cv_.notify_one();
    }

    void wait_idle() {
        while (outstanding_.load(memory_order_acquire) > 0) {
            this_thread::yield();
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
                task = move(tasks_.front());
                tasks_.pop_front();
            }
            task();
        }
    }

    mutex mtx_;
    condition_variable cv_;
    deque<function<void()>> tasks_;
    vector<thread> workers_;
    atomic<int64_t> outstanding_{0};
    bool stopped_{false};
};

// ============================================================
// 0. Work unit calibration
// ============================================================

// Everything below is sized in burn() units; without this benchmark those sizes
// cannot be converted into time.
static void BM_Ref_BurnCalibration(benchmark::State& state) {
    const auto iterations = static_cast<uint64_t>(state.range(0));

    for (auto _: state) {
        burn(iterations);
    }

    // kIsRate divides by the duration of the entire run, so the counter must
    // carry the total item count of the run, not the per-iteration count.
    state.SetItemsProcessed(state.range(0) * state.iterations());
    state.SetLabel("burn(" + std::to_string(iterations) + ")");
}
BENCHMARK(BM_Ref_BurnCalibration)->Arg(1000)->Arg(10000)->Arg(100000)->Unit(benchmark::kNanosecond);

// ============================================================
// 1. Single task round trip latency
// ============================================================

static void BM_ThreadPool_NoopLatency(benchmark::State& state) {
    const auto thread_count = static_cast<size_t>(state.range(0));
    thread_pool pool;
    pool.start(thread_count);

    for (auto _: state) {
        auto result = pool.submit_task([] {});
        result.future.get();
    }

    // One item per iteration; kIsRate turns this into 1 / mean iteration time.
    state.SetItemsProcessed(state.iterations());

    pool.stop();
}
BENCHMARK(BM_ThreadPool_NoopLatency)->Arg(1)->Arg(2)->Arg(4)->Arg(8)->Unit(benchmark::kNanosecond);

static void BM_Ref_MutexPool_NoopLatency(benchmark::State& state) {
    const auto thread_count = static_cast<size_t>(state.range(0));
    ref_mutex_pool pool(thread_count);

    for (auto _: state) {
        auto fut = pool.submit([] {});
        fut.get();
    }

    state.SetItemsProcessed(state.iterations());

    pool.stop();
}
BENCHMARK(BM_Ref_MutexPool_NoopLatency)->Arg(1)->Arg(2)->Arg(4)->Arg(8)->Unit(benchmark::kNanosecond);

// ============================================================
// 2. Noop submission throughput
// ============================================================

static void BM_ThreadPool_Throughput_Noop(benchmark::State& state) {
    const auto thread_count = static_cast<size_t>(state.range(0));
    const int64_t task_count = state.range(1);

    thread_pool pool;
    pool.start(thread_count);

    vector<future<void>> futures;
    futures.reserve(static_cast<size_t>(task_count));

    mean_counter submit_us("submit_us");
    mean_counter drain_us("drain_us");

    for (auto _: state) {
        futures.clear();

        const auto t0 = steady_clock::now();
        for (int64_t i = 0; i < task_count; ++i) {
            auto result = pool.submit_task([] {});
            futures.push_back(move(result.future));
        }
        const auto t1 = steady_clock::now();
        for (auto& f: futures) {
            f.get();
        }
        const auto t2 = steady_clock::now();

        submit_us.add(elapsed_us(t0, t1));
        drain_us.add(elapsed_us(t1, t2));
    }

    submit_us.publish(state);
    drain_us.publish(state);
    state.SetItemsProcessed(task_count * state.iterations());

    pool.stop();
}
BENCHMARK(BM_ThreadPool_Throughput_Noop)
        ->Args({4, 10000})
        ->Args({8, 10000})
        ->Args({16, 10000})
        ->Args({4, 100000})
        ->Args({8, 100000})
        ->Unit(benchmark::kMicrosecond);

// Hand-off only: submission is timed, completion is not waited for. This
// isolates producer-side capacity from future wait cost, and is directly
// comparable with the fire-and-forget reference below.
static void BM_ThreadPool_SubmitOnly_Noop(benchmark::State& state) {
    const auto thread_count = static_cast<size_t>(state.range(0));
    const int64_t task_count = state.range(1);

    thread_pool pool;
    pool.start(thread_count);

    for (auto _: state) {
        const auto t0 = steady_clock::now();
        for (int64_t i = 0; i < task_count; ++i) {
            pool.submit_task([] {});
        }
        const auto t1 = steady_clock::now();
        state.SetIterationTime(elapsed_us(t0, t1) / 1e6);

        // Drain before the next iteration so the queue stays comparable.
        const auto idle_deadline = steady_clock::now() + seconds(5);
        while (steady_clock::now() < idle_deadline) {
            const auto stats = pool.statistics();
            if (stats.queue_size == 0 && stats.busy_threads == 0) {
                break;
            }
            this_thread::yield();
        }
    }

    state.SetItemsProcessed(task_count * state.iterations());

    pool.stop();
}
BENCHMARK(BM_ThreadPool_SubmitOnly_Noop)->Args({8, 100000})->Unit(benchmark::kMicrosecond);

// post_task(): the same hand-off without a task_info allocation, promise or
// future. This is the fire-and-forget shape that industrial pools expose
// (asio post, TBB task_arena::enqueue).
static void BM_ThreadPool_PostOnly_Noop(benchmark::State& state) {
    const auto thread_count = static_cast<size_t>(state.range(0));
    const int64_t task_count = state.range(1);

    thread_pool pool;
    pool.start(thread_count);

    for (auto _: state) {
        const auto t0 = steady_clock::now();
        for (int64_t i = 0; i < task_count; ++i) {
            pool.post_task([] {});
        }
        const auto t1 = steady_clock::now();
        state.SetIterationTime(elapsed_us(t0, t1) / 1e6);

        const auto idle_deadline = steady_clock::now() + seconds(5);
        while (steady_clock::now() < idle_deadline) {
            const auto stats = pool.statistics();
            if (stats.queue_size == 0 && stats.busy_threads == 0) {
                break;
            }
            this_thread::yield();
        }
    }

    state.SetItemsProcessed(task_count);

    pool.stop();
}
BENCHMARK(BM_ThreadPool_PostOnly_Noop)->Args({8, 100000})->Unit(benchmark::kMicrosecond);

static void BM_Ref_FireAndForget_Noop(benchmark::State& state) {
    const auto thread_count = static_cast<size_t>(state.range(0));
    const int64_t task_count = state.range(1);

    ref_mutex_pool pool(thread_count);

    for (auto _: state) {
        const auto t0 = steady_clock::now();
        for (int64_t i = 0; i < task_count; ++i) {
            pool.post([] {});
        }
        const auto t1 = steady_clock::now();
        state.SetIterationTime(elapsed_us(t0, t1) / 1e6);

        pool.wait_idle();
    }

    state.SetItemsProcessed(task_count * state.iterations());

    pool.stop();
}
BENCHMARK(BM_Ref_FireAndForget_Noop)->Args({8, 100000})->Unit(benchmark::kMicrosecond);

static void BM_Ref_MutexPool_Throughput_Noop(benchmark::State& state) {
    const auto thread_count = static_cast<size_t>(state.range(0));
    const int64_t task_count = state.range(1);

    ref_mutex_pool pool(thread_count);

    vector<future<void>> futures;
    futures.reserve(static_cast<size_t>(task_count));

    for (auto _: state) {
        futures.clear();
        for (int64_t i = 0; i < task_count; ++i) {
            futures.push_back(pool.submit([] {}));
        }
        for (auto& f: futures) {
            f.get();
        }
    }

    state.SetItemsProcessed(task_count * state.iterations());

    pool.stop();
}
BENCHMARK(BM_Ref_MutexPool_Throughput_Noop)
        ->Args({4, 10000})
        ->Args({8, 10000})
        ->Args({16, 10000})
        ->Unit(benchmark::kMicrosecond);

// ============================================================
// 3. Compute scaling
// ============================================================

// The reported time is the *parallel makespan* (last submission handed over ->
// last task finished). The previous version timed the whole submit loop, which
// at ~1us per submit is an order of magnitude above the work itself, so it
// measured the producer, not the pool.
static void BM_ThreadPool_Throughput_Compute(benchmark::State& state) {
    const auto thread_count = static_cast<size_t>(state.range(0));
    const int64_t task_count = state.range(1);
    const auto cost = static_cast<uint64_t>(state.range(2));

    thread_pool pool;
    pool.start(thread_count);

    vector<future<void>> futures;
    futures.reserve(static_cast<size_t>(task_count));

    mean_counter submit_us("submit_us");
    mean_counter makespan_us("makespan_us");

    for (auto _: state) {
        futures.clear();

        const auto t0 = steady_clock::now();
        for (int64_t i = 0; i < task_count; ++i) {
            auto result = pool.submit_task([cost] { burn(cost); });
            futures.push_back(move(result.future));
        }
        const auto t1 = steady_clock::now();
        for (auto& f: futures) {
            f.get();
        }
        const auto t2 = steady_clock::now();

        submit_us.add(elapsed_us(t0, t1));
        makespan_us.add(elapsed_us(t1, t2));
        state.SetIterationTime(elapsed_us(t1, t2) / 1e6);
    }

    submit_us.publish(state);
    makespan_us.publish(state);
    state.SetItemsProcessed(task_count * state.iterations());

    pool.stop();
}
BENCHMARK(BM_ThreadPool_Throughput_Compute)
        ->Args({4, 256, 100000})
        ->Args({8, 256, 100000})
        ->Args({16, 256, 100000})
        ->Unit(benchmark::kMicrosecond);

// Same total amount of work executed inline: this is the reference for the
// speedup of the pool variants above.
static void BM_Ref_Serial_Compute(benchmark::State& state) {
    const int64_t task_count = state.range(0);
    const auto cost = static_cast<uint64_t>(state.range(1));

    for (auto _: state) {
        for (int64_t i = 0; i < task_count; ++i) {
            burn(cost);
        }
    }

    state.SetItemsProcessed(task_count * state.iterations());
}
BENCHMARK(BM_Ref_Serial_Compute)->Args({256, 100000})->Unit(benchmark::kMicrosecond);

// ============================================================
// 4. Steal strategy under an unbalanced nested workload
// ============================================================

// A task submitted from a worker lands in that worker's *local* queue, so
// work stealing can only be observed with nested submission. The previous
// version submitted a flat batch from the main thread: every task went to the
// global queue, no local queue ever held an element, and all four strategies
// therefore measured the same code path.
struct fanout_shape {
    int64_t children; ///< number of child tasks spawned by this root
    uint64_t cost;    ///< cost of every child (burn units)
};

static vector<fanout_shape> make_steal_shape(int64_t root_count, int64_t light_children, uint64_t light_cost,
                                             int64_t heavy_children, uint64_t heavy_cost) {
    vector<fanout_shape> shape(static_cast<size_t>(root_count), fanout_shape{light_children, light_cost});
    shape[0] = fanout_shape{heavy_children, heavy_cost};
    return shape;
}

static const char* steal_strategy_name(int64_t strategy) {
    switch (static_cast<thread_pool::steal_strategy>(strategy)) {
        case thread_pool::steal_strategy::half: {
            return "half";
        }
        case thread_pool::steal_strategy::fixed_batch: {
            return "fixed_batch";
        }
        case thread_pool::steal_strategy::single: {
            return "single";
        }
        case thread_pool::steal_strategy::adaptive: {
            return "adaptive";
        }
        default: {
            return "unknown";
        }
    }
}

static void BM_ThreadPool_StealStrategy(benchmark::State& state) {
    const auto thread_count = static_cast<size_t>(state.range(0));
    const auto strategy = static_cast<thread_pool::steal_strategy>(state.range(1));

    constexpr int64_t root_count = 40;
    constexpr int64_t light_children = 5;
    constexpr uint64_t light_cost = 2000;
    constexpr int64_t heavy_children = 100;
    constexpr uint64_t heavy_cost = 20000;

    const auto shape = make_steal_shape(root_count, light_children, light_cost, heavy_children, heavy_cost);
    int64_t child_total = 0;
    for (const auto& root: shape) {
        child_total += root.children;
    }

    thread_pool pool;
    pool.set_steal_mode(strategy);
    pool.start(thread_count);

    vector<vector<future<void>>> child_futures(static_cast<size_t>(root_count));
    for (auto& slot: child_futures) {
        slot.reserve(static_cast<size_t>(heavy_children));
    }
    vector<future<void>> root_futures;
    root_futures.reserve(static_cast<size_t>(root_count));

    atomic<int64_t> children_left{0};
    atomic<int64_t> last_child_us{0};

    mean_counter submit_us("submit_us");
    mean_counter makespan_us("makespan_us");
    mean_counter stolen_per_iteration("stolen");
    mean_counter task_failures("task_failures");

    int64_t stolen_before = 0;

    for (auto _: state) {
        for (auto& slot: child_futures) {
            slot.clear();
        }
        root_futures.clear();
        last_child_us.store(0, memory_order_relaxed);
        children_left.store(child_total, memory_order_relaxed);

        const auto t0 = steady_clock::now();

        for (int64_t r = 0; r < root_count; ++r) {
            auto& slot = child_futures[static_cast<size_t>(r)];
            const int64_t children = shape[static_cast<size_t>(r)].children;
            const uint64_t cost = shape[static_cast<size_t>(r)].cost;

            auto root = pool.submit_task([&pool, &slot, &children_left, &last_child_us, t0, children, cost] {
                for (int64_t c = 0; c < children; ++c) {
                    auto child = pool.submit_task([&children_left, &last_child_us, t0, cost] {
                        burn(cost);
                        // The counter is pre-seeded with the total child count,
                        // so the child that drives it to zero is the last one to
                        // finish and owns the makespan timestamp.
                        if (children_left.fetch_sub(1, memory_order_acq_rel) == 1) {
                            last_child_us.store(
                                    static_cast<int64_t>(time_cast<microseconds>(steady_clock::now() - t0).count()),
                                    memory_order_release);
                        }
                    });
                    slot.push_back(move(child.future));
                }
            });
            root_futures.push_back(move(root.future));
        }

        const auto t1 = steady_clock::now();
        submit_us.add(elapsed_us(t0, t1));

        for (auto& f: root_futures) {
            try {
                f.get();
            } catch (const exception&) {
                task_failures.add(1.0);
            }
        }
        for (auto& slot: child_futures) {
            for (auto& f: slot) {
                try {
                    f.get();
                } catch (const exception&) {
                    task_failures.add(1.0);
                }
            }
        }

        makespan_us.add(static_cast<double>(last_child_us.load(memory_order_acquire)));

        const int64_t stolen_now = static_cast<int64_t>(pool.statistics().total_stolen);
        stolen_per_iteration.add(static_cast<double>(stolen_now - stolen_before));
        stolen_before = stolen_now;
    }

    submit_us.publish(state);
    makespan_us.publish(state);
    stolen_per_iteration.publish(state);
    task_failures.publish(state);
    state.counters["children"] = benchmark::Counter(static_cast<double>(child_total));
    state.SetItemsProcessed(child_total * state.iterations());
    state.SetLabel(steal_strategy_name(state.range(1)));

    pool.stop();
}
BENCHMARK(BM_ThreadPool_StealStrategy)
        ->ArgsProduct({benchmark::CreateRange(4, 16, 2), {0, 1, 2, 3}})
        ->ArgNames({"threads", "strategy"})
        ->Unit(benchmark::kMicrosecond);

// Nested fan-out where every parent waits for its own children. The parents use
// thread_pool::wait_for_all(), which keeps the waiting worker executing pool work: a parent
// that only blocks on future.get() holds up the pool because a task blocked inside a worker
// cannot be unblocked by anything the pool does, and once every worker of a small pool is
// waiting, the children those workers would have run have nobody left to run them (the plain
// get() variant of this benchmark makes no progress at all: 32 waiting roots on 4 threads).
static void BM_ThreadPool_NestedWait(benchmark::State& state) {
    const auto thread_count = static_cast<size_t>(state.range(0));
    const bool assisting = state.range(1) != 0;
    constexpr int64_t root_count = 32;
    constexpr int64_t children_per_root = 8;
    constexpr uint64_t child_cost = 20000;

    thread_pool pool;
    pool.start(thread_count);

    mean_counter makespan_us("makespan_us");
    mean_counter task_failures("task_failures");

    for (auto _: state) {
        atomic<int64_t> children_left{root_count * children_per_root};
        atomic<int64_t> last_child_us{0};

        const auto t0 = steady_clock::now();

        vector<future<void>> roots;
        roots.reserve(static_cast<size_t>(root_count));
        for (int64_t r = 0; r < root_count; ++r) {
            auto root = pool.submit_task([&pool, &children_left, &last_child_us, t0, assisting] {
                vector<future<void>> children;
                children.reserve(static_cast<size_t>(children_per_root));
                for (int64_t c = 0; c < children_per_root; ++c) {
                    auto child = pool.submit_task([&children_left, &last_child_us, t0] {
                        burn(child_cost);
                        if (children_left.fetch_sub(1, memory_order_acq_rel) == 1) {
                            last_child_us.store(
                                    static_cast<int64_t>(time_cast<microseconds>(steady_clock::now() - t0).count()),
                                    memory_order_release);
                        }
                    });
                    children.push_back(move(child.future));
                }

                pool.wait_for_all(children.begin(), children.end());
                (void) assisting;
            });
            roots.push_back(move(root.future));
        }

        for (auto& f: roots) {
            try {
                f.get();
            } catch (const exception&) {
                task_failures.add(1.0);
            }
        }

        makespan_us.add(static_cast<double>(last_child_us.load(memory_order_acquire)));
    }

    makespan_us.publish(state);
    task_failures.publish(state);
    pool.stop();
}
BENCHMARK(BM_ThreadPool_NestedWait)->Arg(4)->Arg(8)->Arg(16)->Unit(benchmark::kMicrosecond);

// ============================================================
// 5. Pool mode comparison
// ============================================================

static void run_mode_benchmark(benchmark::State& state, thread_pool::pool_mode mode) {
    const auto thread_count = static_cast<size_t>(state.range(0));
    constexpr int64_t task_count = 10000;
    constexpr uint64_t cost = 2000;

    thread_pool pool;
    pool.set_mode(mode);
    pool.start(thread_count);

    vector<future<void>> futures;
    futures.reserve(static_cast<size_t>(task_count));

    mean_counter submit_us("submit_us");
    mean_counter drain_us("drain_us");
    mean_counter threads_used("threads_created");
    mean_counter threads_busy("busy_threads");

    for (auto _: state) {
        futures.clear();

        const auto t0 = steady_clock::now();
        for (int64_t i = 0; i < task_count; ++i) {
            auto result = pool.submit_task([] { burn(cost); });
            futures.push_back(move(result.future));
        }
        const auto t1 = steady_clock::now();

        const auto stats = pool.statistics();
        threads_used.add(static_cast<double>(stats.total_threads));
        threads_busy.add(static_cast<double>(stats.busy_threads));

        for (auto& f: futures) {
            f.get();
        }
        const auto t2 = steady_clock::now();

        submit_us.add(elapsed_us(t0, t1));
        drain_us.add(elapsed_us(t1, t2));
    }

    submit_us.publish(state);
    drain_us.publish(state);
    threads_used.publish(state);
    threads_busy.publish(state);
    state.SetItemsProcessed(task_count * state.iterations());

    pool.stop();
}

static void BM_ThreadPool_Mode_Fixed(benchmark::State& state) {
    run_mode_benchmark(state, thread_pool::pool_mode::fixed);
}
BENCHMARK(BM_ThreadPool_Mode_Fixed)->Arg(4)->Arg(8)->Arg(16)->Unit(benchmark::kMicrosecond);

static void BM_ThreadPool_Mode_Cached(benchmark::State& state) {
    run_mode_benchmark(state, thread_pool::pool_mode::cached);
}
BENCHMARK(BM_ThreadPool_Mode_Cached)->Arg(4)->Arg(8)->Arg(16)->Unit(benchmark::kMicrosecond);

// ============================================================
// 6. submit_after latency accuracy
// ============================================================

static void BM_ThreadPool_SubmitAfter_Accuracy(benchmark::State& state) {
    const int64_t delay_ms = state.range(0);
    thread_pool pool;
    pool.start(4);

    mean_counter deviation_us("abs_deviation_us");
    mean_counter overshoot_us("mean_overshoot_us");
    double worst_deviation_us = 0.0;

    for (auto _: state) {
        atomic<int64_t> actual_us{0};

        const auto t0 = steady_clock::now();
        auto result = pool.submit_after(delay_ms, [&actual_us, t0] {
            const auto elapsed = time_cast<microseconds>(steady_clock::now() - t0);
            actual_us.store(elapsed.count(), memory_order_relaxed);
        });
        result.future.get();

        const double expected_us = static_cast<double>(delay_ms) * 1000.0;
        const double actual = static_cast<double>(actual_us.load(memory_order_relaxed));
        const double deviation = absolute(actual - expected_us);

        deviation_us.add(deviation);
        overshoot_us.add(actual - expected_us);
        worst_deviation_us = max(worst_deviation_us, deviation);
    }

    deviation_us.publish(state);
    overshoot_us.publish(state);
    state.counters["worst_deviation_us"] = benchmark::Counter(worst_deviation_us);
    state.counters["samples"] = benchmark::Counter(static_cast<double>(deviation_us.count()));
    state.SetItemsProcessed(state.iterations());

    pool.stop();
}
BENCHMARK(BM_ThreadPool_SubmitAfter_Accuracy)
        ->Arg(1)
        ->Arg(10)
        ->Arg(50)
        ->Arg(100)
        ->Arg(500)
        ->Unit(benchmark::kMicrosecond);

// ============================================================
// 7. Priority ordering
// ============================================================

// Priority can only be observed while both queues hold a backlog. The producer
// (~1us per submit) is faster than the pool drains a burn(2000) task, so an
// ungated run never builds a backlog and can never show an inversion. Every
// worker is therefore parked on a gate first, then all high priority tasks are
// submitted, then all low priority tasks, and only then is the gate released:
// whatever a worker picks up first after release is the real scheduling order.
static void BM_ThreadPool_PriorityOrdering(benchmark::State& state) {
    const int64_t task_count = state.range(0);
    const int64_t high_count = task_count / 2;
    const int64_t low_count = task_count - high_count;
    constexpr size_t thread_count = 4;
    constexpr uint64_t cost = 2000;

    thread_pool pool;
    pool.start(thread_count);

    atomic<int64_t> high_done{0};
    atomic<int64_t> gate_engaged{0};
    mean_counter low_before_high("low_running_while_high_pending");
    mean_counter low_before_high_ratio("low_before_high_ratio");
    mean_counter gate_ok("gate_engaged");
    mean_counter drain_us("drain_us");

    vector<future<void>> futures;
    futures.reserve(static_cast<size_t>(task_count) + thread_count);

    for (auto _: state) {
        futures.clear();
        high_done.store(0, memory_order_relaxed);
        gate_engaged.store(0, memory_order_relaxed);

        atomic<bool> gate{false};
        atomic<int64_t> low_inversions{0};

        for (size_t i = 0; i < thread_count; ++i) {
            auto result = pool.submit_task([&gate, &gate_engaged] {
                gate_engaged.fetch_add(1, memory_order_release);
                while (!gate.load(memory_order_acquire)) {
                    this_thread::yield();
                }
            });
            futures.push_back(move(result.future));
        }

        const auto gate_deadline = steady_clock::now() + seconds(5);
        while (gate_engaged.load(memory_order_acquire) < static_cast<int64_t>(thread_count) &&
               steady_clock::now() < gate_deadline) {
            this_thread::yield();
        }
        gate_ok.add(gate_engaged.load(memory_order_acquire) == static_cast<int64_t>(thread_count) ? 1.0 : 0.0);

        for (int64_t i = 0; i < high_count; ++i) {
            auto result = pool.submit_task(static_cast<thread_pool::priority_type>(1), [&high_done] {
                burn(cost);
                high_done.fetch_add(1, memory_order_acq_rel);
            });
            futures.push_back(move(result.future));
        }
        for (int64_t i = 0; i < low_count; ++i) {
            auto result = pool.submit_task([&high_done, &low_inversions, high_count] {
                burn(cost);
                if (high_done.load(memory_order_acquire) < high_count) {
                    low_inversions.fetch_add(1, memory_order_relaxed);
                }
            });
            futures.push_back(move(result.future));
        }

        const auto t0 = steady_clock::now();
        gate.store(true, memory_order_release);
        for (auto& f: futures) {
            f.get();
        }
        const auto t1 = steady_clock::now();

        const auto inversions = low_inversions.load(memory_order_relaxed);
        low_before_high.add(static_cast<double>(inversions));
        low_before_high_ratio.add(static_cast<double>(inversions) / static_cast<double>(low_count));
        drain_us.add(elapsed_us(t0, t1));
    }

    gate_ok.publish(state);
    low_before_high.publish(state);
    low_before_high_ratio.publish(state);
    drain_us.publish(state);
    state.SetItemsProcessed(task_count * state.iterations());

    pool.stop();
}
BENCHMARK(BM_ThreadPool_PriorityOrdering)->Arg(200)->Arg(500)->Arg(1000)->Unit(benchmark::kMicrosecond);

// ============================================================
// 8. Multi-producer contention
// ============================================================

static void BM_ThreadPool_MultiProducer(benchmark::State& state) {
    const auto producer_count = static_cast<size_t>(state.range(0));
    constexpr int64_t tasks_per_producer = 500;
    constexpr size_t consumer_count = 8;

    thread_pool pool;
    pool.start(consumer_count);

    int64_t submitted_total = 0;

    for (auto _: state) {
        atomic<int64_t> total_submitted{0};
        vector<thread> producers;
        producers.reserve(producer_count);

        const auto t0 = steady_clock::now();

        for (size_t p = 0; p < producer_count; ++p) {
            producers.emplace_back([&pool, &total_submitted] {
                for (int64_t i = 0; i < tasks_per_producer; ++i) {
                    auto result = pool.submit_task([] { burn(50); });
                    result.future.get();
                    total_submitted.fetch_add(1, memory_order_relaxed);
                }
            });
        }

        for (auto& t: producers) {
            t.join();
        }

        const auto t1 = steady_clock::now();
        state.SetIterationTime(elapsed_us(t0, t1) / 1e6);
        submitted_total += total_submitted.load();
    }

    state.SetItemsProcessed(submitted_total);

    pool.stop();
}
BENCHMARK(BM_ThreadPool_MultiProducer)->Arg(1)->Arg(2)->Arg(4)->Arg(8)->Unit(benchmark::kMicrosecond);

// Producer scalability without the round trip: producers only enqueue, the
// consumers are drained afterwards. The round-trip variant above also contains
// the futex wake of every waiting producer, which hides producer-side contention.
static void BM_ThreadPool_MultiProducer_SubmitOnly(benchmark::State& state) {
    const auto producer_count = static_cast<size_t>(state.range(0));
    constexpr int64_t tasks_per_producer = 500;
    constexpr size_t consumer_count = 8;

    thread_pool pool;
    pool.start(consumer_count);

    vector<vector<future<void>>> producer_futures(producer_count);
    for (auto& slot: producer_futures) {
        slot.reserve(static_cast<size_t>(tasks_per_producer));
    }

    mean_counter submit_us("submit_us");
    mean_counter drain_us("drain_us");

    for (auto _: state) {
        for (auto& slot: producer_futures) {
            slot.clear();
        }

        vector<thread> producers;
        producers.reserve(producer_count);

        const auto t0 = steady_clock::now();
        for (size_t p = 0; p < producer_count; ++p) {
            producers.emplace_back([&pool, &producer_futures, p] {
                auto& slot = producer_futures[p];
                for (int64_t i = 0; i < tasks_per_producer; ++i) {
                    auto result = pool.submit_task([] { burn(50); });
                    slot.push_back(move(result.future));
                }
            });
        }
        for (auto& t: producers) {
            t.join();
        }
        const auto t1 = steady_clock::now();

        for (auto& slot: producer_futures) {
            for (auto& f: slot) {
                f.get();
            }
        }
        const auto t2 = steady_clock::now();

        submit_us.add(elapsed_us(t0, t1));
        drain_us.add(elapsed_us(t1, t2));
        state.SetIterationTime(elapsed_us(t0, t2) / 1e6);
    }

    submit_us.publish(state);
    drain_us.publish(state);
    state.SetItemsProcessed(static_cast<int64_t>(producer_count) * tasks_per_producer * state.iterations());

    pool.stop();
}
BENCHMARK(BM_ThreadPool_MultiProducer_SubmitOnly)->Arg(1)->Arg(2)->Arg(4)->Arg(8)->Unit(benchmark::kMicrosecond);

// ============================================================
// 9. Idle cost
// ============================================================

// A parked worker should cost nothing. This measures the CPU time an idle pool
// burns per second (process wide CPU time, so it covers every worker).
static void BM_ThreadPool_IdleCpu(benchmark::State& state) {
    const auto thread_count = static_cast<size_t>(state.range(0));
    constexpr int64_t sample_ms = 50;

    thread_pool pool;
    pool.start(thread_count);

    // Let the workers leave the spin/yield phase and park.
    this_thread::sleep_for(milliseconds(200));

    mean_counter cpu_ms("idle_cpu_ms");
    mean_counter wall_ms("idle_wall_ms");

    for (auto _: state) {
        const clock_t cpu0 = clock();
        const auto t0 = steady_clock::now();
        this_thread::sleep_for(milliseconds(sample_ms));
        const auto t1 = steady_clock::now();
        const clock_t cpu1 = clock();

        cpu_ms.add(1000.0 * static_cast<double>(cpu1 - cpu0) / static_cast<double>(CLOCKS_PER_SEC));
        wall_ms.add(elapsed_us(t0, t1) / 1000.0);
    }

    cpu_ms.publish(state);
    wall_ms.publish(state);

    const double cpu_percent = wall_ms.mean() > 0.0 ? cpu_ms.mean() / wall_ms.mean() * 100.0 : 0.0;
    state.counters["idle_cpu_pct_of_one_core"] = benchmark::Counter(cpu_percent);
    state.counters["idle_cpu_pct_per_worker"] = benchmark::Counter(cpu_percent / static_cast<double>(thread_count));

    pool.stop();
}
BENCHMARK(BM_ThreadPool_IdleCpu)->Arg(1)->Arg(4)->Arg(8)->Arg(16)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
