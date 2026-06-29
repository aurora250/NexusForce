#include <benchmark/benchmark.h>
#include <NeForce/core/async/thread_pool.hpp>
#include <NeForce/core/numeric/random.hpp>
#include <NeForce/core/exception/terminate.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/core/system/stacktrace.hpp>
using namespace neforce;

namespace {
    int _terminate_handler_reg = []() {
        neforce::set_terminate([]() {
            eprintfln("[TERMINATE] thread_id={}", ::GetCurrentThreadId());
            eprintfln("[TERMINATE] call stack:{}", stacktrace::current().to_string());
            const auto exp = current_exception();
            try {
                if (exp) {
                    rethrow_exception(exp);
                }
            } catch (const exception& e) {
                eprintfln("[TERMINATE] current exception: {}:{}", e.type(), e.what());
            }
        });
        return 0;
    }();
} // namespace

inline void burn(uint64_t iterations) {
    uint64_t sum = 0;
    for (uint64_t i = 0; i < iterations; ++i) {
        sum += i;
    }
    benchmark::DoNotOptimize(sum);
    benchmark::ClobberMemory();
}

static vector<uint64_t> make_unbalanced_workload(size_t total, size_t heavy_pct, uint64_t light_cost,
                                                 uint64_t heavy_cost) {
    vector<uint64_t> workloads(total);
    thread_local random_mt rnd{};
    generate(workloads.begin(), workloads.end(),
             [&] { return (rnd.next_int(0, 99) < heavy_pct) ? heavy_cost : light_cost; });
    return workloads;
}

#if 0

// ============================================================
// 1. Throughput
// ============================================================

static void BM_ThreadPool_NoopLatency(benchmark::State& state) {
    const size_t thread_count = state.range(0);
    thread_pool pool;
    pool.start(thread_count);

    for (auto _: state) {
        auto result = pool.submit_task([] {});
        result.future.get();
        state.SetItemsProcessed(1);
    }

    pool.stop();
}
BENCHMARK(BM_ThreadPool_NoopLatency)->Arg(1)->Arg(2)->Arg(4)->Arg(8)->Unit(benchmark::kNanosecond);

static void BM_ThreadPool_Throughput_Noop(benchmark::State& state) {
    const size_t thread_count = state.range(0);
    const int64_t task_count = state.range(1);

    thread_pool pool;
    pool.start(thread_count);

    for (auto _: state) {
        vector<future<void>> futures;
        futures.reserve(task_count);

        for (int64_t i = 0; i < task_count; ++i) {
            auto result = pool.submit_task([] {});
            futures.push_back(move(result.future));
        }

        for (auto& f: futures) {
            f.get();
        }

        state.SetItemsProcessed(task_count);
    }

    pool.stop();
}
BENCHMARK(BM_ThreadPool_Throughput_Noop)
        ->Args({4, 10000})
        ->Args({8, 10000})
        ->Args({16, 10000})
        ->Args({4, 100000})
        ->Args({8, 100000})
        ->Unit(benchmark::kMicrosecond);

static void BM_ThreadPool_Throughput_Compute(benchmark::State& state) {
    const size_t thread_count = state.range(0);
    const int64_t task_count = state.range(1);

    thread_pool pool;
    pool.start(thread_count);

    for (auto _: state) {
        vector<future<void>> futures;
        futures.reserve(task_count);

        for (int64_t i = 0; i < task_count; ++i) {
            auto result = pool.submit_task([] { burn(1000); });
            futures.push_back(move(result.future));
        }

        for (auto& f: futures) {
            f.get();
        }

        state.SetItemsProcessed(task_count);
    }

    pool.stop();
}
BENCHMARK(BM_ThreadPool_Throughput_Compute)
        ->Args({4, 1000})
        ->Args({8, 1000})
        ->Args({16, 1000})
        ->Unit(benchmark::kMillisecond);

// ============================================================
// 2. Steal strategy — unbalanced workload makespan
// ============================================================

static void BM_ThreadPool_StealStrategy(benchmark::State& state) {
    const size_t thread_count = state.range(0);
    const auto strategy = static_cast<thread_pool::steal_strategy>(state.range(1));
    const int64_t task_count = 1000;

    auto workloads = make_unbalanced_workload(task_count, 10, 100, 20000);

    thread_pool pool;
    pool.set_steal_mode(strategy);
    pool.start(thread_count);

    for (auto _: state) {
        vector<future<void>> futures;
        futures.reserve(task_count);

        auto batch_start = steady_clock::now();

        for (int64_t i = 0; i < task_count; ++i) {
            uint64_t cost = workloads[i];
            auto result = pool.submit_task([cost] { burn(cost); });
            futures.push_back(move(result.future));
        }

        for (auto& f: futures) {
            f.get();
        }

        auto elapsed = duration<double>(steady_clock::now() - batch_start);
        state.SetIterationTime(elapsed.count());
        state.SetItemsProcessed(task_count);
    }

    pool.stop();
}
BENCHMARK(BM_ThreadPool_StealStrategy)
        ->ArgsProduct({benchmark::CreateRange(4, 16, 2), {0, 1, 2, 3}})
        ->ArgNames({"threads", "strategy"})
        ->Unit(benchmark::kMillisecond);

#endif

// ============================================================
// 3. Pool mode comparison
// ============================================================

static void BM_ThreadPool_Mode_Fixed(benchmark::State& state) {
    const size_t thread_count = state.range(0);
    const int64_t task_count = 10000;

    thread_pool pool;
    pool.set_mode(thread_pool::pool_mode::fixed);
    pool.start(thread_count);

    for (auto _: state) {
        vector<future<void>> futures;
        futures.reserve(task_count);

        for (int64_t i = 0; i < task_count; ++i) {
            auto result = pool.submit_task([] { burn(100); });
            futures.push_back(move(result.future));
        }

        for (auto& f: futures) {
            f.get();
        }

        state.SetItemsProcessed(task_count);
    }

    pool.stop();
}
BENCHMARK(BM_ThreadPool_Mode_Fixed)->Arg(4)->Arg(8)->Arg(16)->Unit(benchmark::kMillisecond);

static void BM_ThreadPool_Mode_Cached(benchmark::State& state) {
    const size_t thread_count = state.range(0);
    const int64_t task_count = 10000;

    thread_pool pool;
    pool.set_mode(thread_pool::pool_mode::cached);
    pool.start(thread_count);

    for (auto _: state) {
        vector<future<void>> futures;
        futures.reserve(task_count);

        for (int64_t i = 0; i < task_count; ++i) {
            auto result = pool.submit_task([] { burn(100); });
            futures.push_back(move(result.future));
        }

        for (auto& f: futures) {
            f.get();
        }

        state.SetItemsProcessed(task_count);
    }

    pool.stop();
}
BENCHMARK(BM_ThreadPool_Mode_Cached)->Arg(4)->Arg(8)->Arg(16)->Unit(benchmark::kMillisecond);

// ============================================================
// 4. submit_after latency accuracy
// ============================================================

static void BM_ThreadPool_SubmitAfter_Accuracy(benchmark::State& state) {
    const int64_t delay_ms = state.range(0);
    thread_pool pool;
    pool.start(4);

    for (auto _: state) {
        atomic<int64_t> actual_us{0};

        auto t0 = steady_clock::now();
        auto result = pool.submit_after(delay_ms, [&actual_us, t0] {
            auto t1 = steady_clock::now();
            actual_us.store(time_cast<microseconds>(t1 - t0).count(), memory_order_relaxed);
        });
        result.future.get();

        auto deviation = actual_us.load() - delay_ms * 1000;
        if (deviation < 0) {
            deviation = -deviation;
        }

        state.counters["abs_deviation_us"] =
                benchmark::Counter(static_cast<double>(deviation), benchmark::Counter::kAvgIterations);
        state.SetItemsProcessed(1);
    }

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
// 5. Priority ordering
// ============================================================

static void BM_ThreadPool_PriorityOrdering(benchmark::State& state) {
    const int64_t task_count = state.range(0);
    thread_pool pool;
    pool.start(4);

    for (auto _: state) {
        atomic<int64_t> counter{0};
        atomic<bool> low_beat_high{false};
        vector<future<void>> futures;
        futures.reserve(task_count);

        for (int64_t i = 0; i < task_count; ++i) {
            if (i < task_count / 2) {
                auto result = pool.submit_task(static_cast<thread_pool::priority_type>(1), [&] {
                    burn(2000);
                    int64_t c = counter.fetch_add(1);
                    if (c >= task_count / 2) {
                        low_beat_high.store(true);
                    }
                });
                futures.push_back(move(result.future));
            } else {
                auto result = pool.submit_task(static_cast<thread_pool::priority_type>(0), [&] {
                    burn(2000);
                    counter.fetch_add(1);
                });
                futures.push_back(move(result.future));
            }
        }

        for (auto& f: futures) {
            f.get();
        }

        state.counters["low_beat_high"] =
                benchmark::Counter(low_beat_high.load() ? 1.0 : 0.0, benchmark::Counter::kAvgIterations);
        state.SetItemsProcessed(task_count);
    }

    pool.stop();
}
BENCHMARK(BM_ThreadPool_PriorityOrdering)->Arg(200)->Arg(500)->Arg(1000)->Unit(benchmark::kMillisecond);

// ============================================================
// 6. Multi-producer contention
// ============================================================

static void BM_ThreadPool_MultiProducer(benchmark::State& state) {
    const size_t producer_count = state.range(0);
    const int64_t tasks_per_producer = 500;

    thread_pool pool;
    pool.start(8);

    for (auto _: state) {
        atomic<int64_t> total_submitted{0};
        vector<thread> producers;
        producers.reserve(producer_count);

        auto batch_start = steady_clock::now();

        for (size_t p = 0; p < producer_count; ++p) {
            producers.emplace_back([&, p] {
                for (int64_t i = 0; i < tasks_per_producer; ++i) {
                    auto result = pool.submit_task([] { burn(50); });
                    try {
                        result.future.get();
                    } catch (const exception& e) {
                        auto st = result.task_info->status.load();
                        eprintfln("[PRODUCER] p={} i={} tid={} EXCEPTION: {} {}  task_info_status={} error={}", p, i,
                                  ::GetCurrentThreadId(), e.type(), e.what(), static_cast<int>(st),
                                  result.task_info->error);
                        eprintfln("[PRODUCER] pool stats: total_threads={} idle={} queue={} submitted={} completed={}",
                                  pool.statistics().total_threads, pool.statistics().idle_threads,
                                  pool.statistics().queue_size, pool.statistics().total_submitted,
                                  pool.statistics().total_completed);
                        throw;
                    } catch (...) {
                        eprintfln("[PRODUCER] p={} i={} tid={} UNKNOWN EXCEPTION after get", p, i,
                                  ::GetCurrentThreadId());
                        throw;
                    }
                    total_submitted.fetch_add(1, memory_order_relaxed);
                }
            });
        }

        for (auto& t: producers) {
            t.join();
        }

        auto elapsed = duration<double>(steady_clock::now() - batch_start);
        state.SetIterationTime(elapsed.count());
        state.SetItemsProcessed(total_submitted.load());
    }

    pool.stop();
}
BENCHMARK(BM_ThreadPool_MultiProducer)->Arg(1)->Arg(2)->Arg(4)->Arg(8)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
