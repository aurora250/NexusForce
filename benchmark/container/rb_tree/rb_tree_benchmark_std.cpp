#include "rb_tree_benchmark_common.hpp"

#define TREE_ARGS Args({1 << 10})->Args({1 << 15})->Args({1 << 18})->Args({1 << 20})

BENCHMARK(BM_StdSet_Insert)->TREE_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_StdSet_InsertHint)->TREE_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_StdSet_Emplace)->TREE_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_StdSet_Find)->TREE_ARGS->Unit(benchmark::kNanosecond);
BENCHMARK(BM_StdSet_FindMiss)->TREE_ARGS->Unit(benchmark::kNanosecond);
BENCHMARK(BM_StdSet_Erase)->TREE_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_StdSet_Iterate)->TREE_ARGS->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_StdSet_Count)->TREE_ARGS->Unit(benchmark::kNanosecond);
BENCHMARK(BM_StdSet_LowerBound)->TREE_ARGS->Unit(benchmark::kNanosecond);
BENCHMARK(BM_StdSet_EqualRange)->TREE_ARGS->Unit(benchmark::kNanosecond);
BENCHMARK(BM_StdSet_Copy)->TREE_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_StdMap_Emplace)->TREE_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_StdMap_Find)->TREE_ARGS->Unit(benchmark::kNanosecond);
BENCHMARK(BM_StdMap_Iterate)->TREE_ARGS->Unit(benchmark::kMicrosecond);
