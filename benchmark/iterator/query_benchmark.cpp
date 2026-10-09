#include <NeForce/core/container/vector.hpp>
#include <NeForce/core/iterator/query.hpp>
#include <NeForce/core/iterator/ranges/views.hpp>
#include <benchmark/benchmark.h>
using namespace neforce;

namespace {
    vector<int> make_data(size_t count) {
        vector<int> data(count);
        for (size_t index = 0; index < count; ++index) {
            data[index] = static_cast<int>(index % 1024);
        }
        return data;
    }

    long manual_loop(const vector<int>& data) {
        long total = 0;
        for (auto value: data) {
            if ((value & 1) != 0) {
                total += value * 3;
            }
        }
        return total;
    }

    const size_t g_count = 100000;
} // namespace


static void BM_ManualLoop(benchmark::State& state) {
    const vector<int> data = make_data(g_count);
    for (auto _: state) {
        benchmark::DoNotOptimize(manual_loop(data));
    }
}
BENCHMARK(BM_ManualLoop);


static void BM_RangePipeline(benchmark::State& state) {
    const vector<int> data = make_data(g_count);
    for (auto _: state) {
        long total = 0;
        for (auto value: data | ranges::views::filter([](int x) { return (x & 1) != 0; }) |
                                 ranges::views::transform([](int x) { return x * 3; })) {
            total += value;
        }
        benchmark::DoNotOptimize(total);
    }
}
BENCHMARK(BM_RangePipeline);


static void BM_ChainQuery(benchmark::State& state) {
    const vector<int> data = make_data(g_count);
    auto query = ranges::from(data).filter([](int x) { return (x & 1) != 0; }).transform([](int x) { return x * 3; });
    for (auto _: state) {
        benchmark::DoNotOptimize(query.sum(0L));
    }
}
BENCHMARK(BM_ChainQuery);


static void BM_ChainQueryToVector(benchmark::State& state) {
    const vector<int> data = make_data(g_count);
    auto query = ranges::from(data).filter([](int x) { return (x & 1) != 0; }).transform([](int x) { return x * 3; });
    for (auto _: state) {
        auto result = query.to_vector();
        benchmark::DoNotOptimize(result.size());
    }
}
BENCHMARK(BM_ChainQueryToVector);


static void BM_ChainSizedToVector(benchmark::State& state) {
    const vector<int> data = make_data(g_count);
    auto query = ranges::from(data).transform([](int x) { return x * 3; }).take(g_count / 2);
    for (auto _: state) {
        auto result = query.to_vector();
        benchmark::DoNotOptimize(result.size());
    }
}
BENCHMARK(BM_ChainSizedToVector);


static void BM_ManualToVector(benchmark::State& state) {
    const vector<int> data = make_data(g_count);
    for (auto _: state) {
        vector<int> result;
        result.reserve(g_count / 2);
        size_t taken = 0;
        for (auto value: data) {
            if (taken == g_count / 2) {
                break;
            }
            result.push_back(value * 3);
            ++taken;
        }
        benchmark::DoNotOptimize(result.size());
    }
}
BENCHMARK(BM_ManualToVector);


static void BM_ChainShortCircuit(benchmark::State& state) {
    const vector<int> data = make_data(g_count);
    auto query = ranges::from(data).filter([](int x) { return x > 512; });
    for (auto _: state) {
        benchmark::DoNotOptimize(query.first_or(-1));
    }
}
BENCHMARK(BM_ChainShortCircuit);


BENCHMARK_MAIN();
