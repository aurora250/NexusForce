#include "rb_tree_benchmark_common.hpp"

namespace {

    // sparse_set 与 set/map 尺寸不同：插入/删除为 O(n) 搬移，规模上限低于红黑树。
    // 本 TU 用一组不与主基准重叠的规模，三方在同一尺寸下对比。

    void BM_SpSet_Insert(benchmark::State& state) {
        const size_t n = static_cast<size_t>(state.range(0));
        const auto keys = make_keys(n);
        nf_sparse_t s;
        BM_Tree_Set_Insert<nf_sparse_t>(state, s, keys);
    }

    void BM_SpSet_InsertHint(benchmark::State& state) {
        nf_sparse_t s;
        BM_Tree_Set_InsertHint<nf_sparse_t>(state, s);
    }

    void BM_SpSet_Emplace(benchmark::State& state) {
        const size_t n = static_cast<size_t>(state.range(0));
        const auto keys = make_keys(n);
        BM_Tree_Set_Emplace<nf_sparse_t>(state, keys);
    }

    void BM_SpSet_Find(benchmark::State& state) {
        const size_t n = static_cast<size_t>(state.range(0));
        const auto keys = make_keys(n);
        nf_sparse_t s;
        fill_set(s, keys);
        BM_Tree_Set_Find<nf_sparse_t>(state, s, keys);
    }

    void BM_SpSet_FindMiss(benchmark::State& state) {
        const size_t n = static_cast<size_t>(state.range(0));
        const auto keys = make_keys(n);
        const auto missing = make_missing_keys(n);
        nf_sparse_t s;
        fill_set(s, keys);
        BM_Tree_Set_FindMiss<nf_sparse_t>(state, s, missing);
    }

    void BM_SpSet_Erase(benchmark::State& state) {
        const size_t n = static_cast<size_t>(state.range(0));
        const auto keys = make_keys(n);
        nf_sparse_t s;
        BM_Tree_Set_Erase<nf_sparse_t>(state, s, keys);
    }

    void BM_SpSet_Iterate(benchmark::State& state) {
        const size_t n = static_cast<size_t>(state.range(0));
        const auto keys = make_keys(n);
        nf_sparse_t s;
        fill_set(s, keys);
        BM_Tree_Set_Iterate<nf_sparse_t>(state, s);
    }

    void BM_SpSet_Count(benchmark::State& state) {
        const size_t n = static_cast<size_t>(state.range(0));
        const auto keys = make_keys(n);
        nf_sparse_t s;
        fill_set(s, keys);
        BM_Tree_Set_Count<nf_sparse_t>(state, s, keys);
    }

    void BM_SpSet_LowerBound(benchmark::State& state) {
        const size_t n = static_cast<size_t>(state.range(0));
        const auto keys = make_keys(n);
        nf_sparse_t s;
        fill_set(s, keys);
        BM_Tree_Set_LowerBound<nf_sparse_t>(state, s, keys);
    }

    void BM_SpSet_EqualRange(benchmark::State& state) {
        const size_t n = static_cast<size_t>(state.range(0));
        const auto keys = make_keys(n);
        nf_sparse_t s;
        fill_set(s, keys);
        BM_Tree_Set_EqualRange<nf_sparse_t>(state, s, keys);
    }

    void BM_SpSet_Copy(benchmark::State& state) {
        const size_t n = static_cast<size_t>(state.range(0));
        const auto keys = make_keys(n);
        nf_sparse_t s;
        fill_set(s, keys);
        BM_Tree_Set_Copy<nf_sparse_t>(state, s);
    }

} // namespace

#define CMP_ARGS Args({1 << 12})->Args({1 << 14})->Args({1 << 16})

BENCHMARK(BM_NfSet_Insert)->CMP_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_NfSet_InsertHint)->CMP_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_NfSet_Emplace)->CMP_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_NfSet_Find)->CMP_ARGS->Unit(benchmark::kNanosecond);
BENCHMARK(BM_NfSet_FindMiss)->CMP_ARGS->Unit(benchmark::kNanosecond);
BENCHMARK(BM_NfSet_Erase)->CMP_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_NfSet_Iterate)->CMP_ARGS->Unit(benchmark::kNanosecond);
BENCHMARK(BM_NfSet_Count)->CMP_ARGS->Unit(benchmark::kNanosecond);
BENCHMARK(BM_NfSet_LowerBound)->CMP_ARGS->Unit(benchmark::kNanosecond);
BENCHMARK(BM_NfSet_EqualRange)->CMP_ARGS->Unit(benchmark::kNanosecond);
BENCHMARK(BM_NfSet_Copy)->CMP_ARGS->Unit(benchmark::kMillisecond);

BENCHMARK(BM_StdSet_Insert)->CMP_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_StdSet_InsertHint)->CMP_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_StdSet_Emplace)->CMP_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_StdSet_Find)->CMP_ARGS->Unit(benchmark::kNanosecond);
BENCHMARK(BM_StdSet_FindMiss)->CMP_ARGS->Unit(benchmark::kNanosecond);
BENCHMARK(BM_StdSet_Erase)->CMP_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_StdSet_Iterate)->CMP_ARGS->Unit(benchmark::kNanosecond);
BENCHMARK(BM_StdSet_Count)->CMP_ARGS->Unit(benchmark::kNanosecond);
BENCHMARK(BM_StdSet_LowerBound)->CMP_ARGS->Unit(benchmark::kNanosecond);
BENCHMARK(BM_StdSet_EqualRange)->CMP_ARGS->Unit(benchmark::kNanosecond);
BENCHMARK(BM_StdSet_Copy)->CMP_ARGS->Unit(benchmark::kMillisecond);

BENCHMARK(BM_SpSet_Insert)->CMP_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_SpSet_InsertHint)->CMP_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_SpSet_Emplace)->CMP_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_SpSet_Find)->CMP_ARGS->Unit(benchmark::kNanosecond);
BENCHMARK(BM_SpSet_FindMiss)->CMP_ARGS->Unit(benchmark::kNanosecond);
BENCHMARK(BM_SpSet_Erase)->CMP_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_SpSet_Iterate)->CMP_ARGS->Unit(benchmark::kNanosecond);
BENCHMARK(BM_SpSet_Count)->CMP_ARGS->Unit(benchmark::kNanosecond);
BENCHMARK(BM_SpSet_LowerBound)->CMP_ARGS->Unit(benchmark::kNanosecond);
BENCHMARK(BM_SpSet_EqualRange)->CMP_ARGS->Unit(benchmark::kNanosecond);
BENCHMARK(BM_SpSet_Copy)->CMP_ARGS->Unit(benchmark::kMillisecond);
