#include "rb_tree_benchmark_common.hpp"

#define TREE_ARGS Args({1 << 10})->Args({1 << 15})->Args({1 << 18})->Args({1 << 20})

BENCHMARK(BM_NfSet_Insert)->TREE_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_NfSet_InsertHint)->TREE_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_NfSet_Emplace)->TREE_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_NfSet_Find)->TREE_ARGS->Unit(benchmark::kNanosecond);
BENCHMARK(BM_NfSet_FindMiss)->TREE_ARGS->Unit(benchmark::kNanosecond);
BENCHMARK(BM_NfSet_Erase)->TREE_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_NfSet_Iterate)->TREE_ARGS->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_NfSet_Count)->TREE_ARGS->Unit(benchmark::kNanosecond);
BENCHMARK(BM_NfSet_LowerBound)->TREE_ARGS->Unit(benchmark::kNanosecond);
BENCHMARK(BM_NfSet_EqualRange)->TREE_ARGS->Unit(benchmark::kNanosecond);
BENCHMARK(BM_NfSet_Copy)->TREE_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_NfMap_Emplace)->TREE_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_NfMap_Find)->TREE_ARGS->Unit(benchmark::kNanosecond);
BENCHMARK(BM_NfMap_Iterate)->TREE_ARGS->Unit(benchmark::kMicrosecond);
