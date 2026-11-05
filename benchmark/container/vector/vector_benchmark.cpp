#include <benchmark/benchmark.h>
#include <NeForce/core/container/vector.hpp>
#include <NeForce/core/string/basic_string.hpp>
#include <vector>

namespace {

    using value_t = int;
    using blob_t = neforce::basic_string<char>;

    /// NeForce vector with the default allocator (pool-backed when NEXUSFORCE_USING_MEMORY_POOL is on)
    using nf_vec_t = neforce::vector<value_t>;
    /// NeForce vector with std::allocator, so the container overhead can be separated from the allocator
    using nf_sa_vec_t = neforce::vector<value_t, std::allocator<value_t>>;
    /// Standard library vector
    using std_vec_t = std::vector<value_t>;

    /// Non-trivial element type: an SSO-sized NeForce string shared by both containers
    using nf_str_vec_t = neforce::vector<blob_t>;
    using std_str_vec_t = std::vector<blob_t>;

    blob_t make_blob(const size_t i) {
        char buf[32];
        for (char& c: buf) {
            c = 'a' + static_cast<char>(i % 26);
        }
        return blob_t(buf, sizeof(buf) - 1);
    }

    template <typename V>
    void fill_vector(V& v, const size_t n) {
        v.resize(n);
        for (size_t i = 0; i < n; ++i) {
            v[i] = static_cast<value_t>(i);
        }
    }

} // namespace

// ============================================================
// 1. push_back with reserved capacity
// ============================================================

template <typename V>
static void BM_Vec_PushBackReserved(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    for (auto _: state) {
        V v;
        v.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            v.push_back(static_cast<value_t>(i));
        }
        benchmark::DoNotOptimize(v.data());
        benchmark::DoNotOptimize(v.size());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}

static void BM_Vec_Nf_PushBackReserved(benchmark::State& state) { BM_Vec_PushBackReserved<nf_vec_t>(state); }
static void BM_Vec_NfStdAlloc_PushBackReserved(benchmark::State& state) { BM_Vec_PushBackReserved<nf_sa_vec_t>(state); }
static void BM_Vec_Std_PushBackReserved(benchmark::State& state) { BM_Vec_PushBackReserved<std_vec_t>(state); }

// ============================================================
// 2. push_back without reserve (includes growth reallocation)
// ============================================================

template <typename V>
static void BM_Vec_PushBackGrow(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    for (auto _: state) {
        V v;
        for (size_t i = 0; i < n; ++i) {
            v.push_back(static_cast<value_t>(i));
        }
        benchmark::DoNotOptimize(v.data());
        benchmark::DoNotOptimize(v.size());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}

static void BM_Vec_Nf_PushBackGrow(benchmark::State& state) { BM_Vec_PushBackGrow<nf_vec_t>(state); }
static void BM_Vec_NfStdAlloc_PushBackGrow(benchmark::State& state) { BM_Vec_PushBackGrow<nf_sa_vec_t>(state); }
static void BM_Vec_Std_PushBackGrow(benchmark::State& state) { BM_Vec_PushBackGrow<std_vec_t>(state); }

// ============================================================
// 3. emplace_back with reserved capacity
// ============================================================

template <typename V>
static void BM_Vec_EmplaceBackReserved(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    for (auto _: state) {
        V v;
        v.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            v.emplace_back(static_cast<value_t>(i));
        }
        benchmark::DoNotOptimize(v.data());
        benchmark::DoNotOptimize(v.size());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}

static void BM_Vec_Nf_EmplaceBackReserved(benchmark::State& state) { BM_Vec_EmplaceBackReserved<nf_vec_t>(state); }
static void BM_Vec_Std_EmplaceBackReserved(benchmark::State& state) { BM_Vec_EmplaceBackReserved<std_vec_t>(state); }

// ============================================================
// 4. resize growth
// ============================================================

template <typename V>
static void BM_Vec_ResizeGrow(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    for (auto _: state) {
        V v;
        v.resize(n);
        benchmark::DoNotOptimize(v.data());
        benchmark::DoNotOptimize(v.size());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}

static void BM_Vec_Nf_ResizeGrow(benchmark::State& state) { BM_Vec_ResizeGrow<nf_vec_t>(state); }
static void BM_Vec_NfStdAlloc_ResizeGrow(benchmark::State& state) { BM_Vec_ResizeGrow<nf_sa_vec_t>(state); }
static void BM_Vec_Std_ResizeGrow(benchmark::State& state) { BM_Vec_ResizeGrow<std_vec_t>(state); }

// ============================================================
// 5. insert at the middle (element shift)
// ============================================================

/// Shift benchmarks run k_shift_batch operations per timed region to amortize the paused O(n) reset
constexpr size_t k_shift_batch = 64;

template <typename V>
static void BM_Vec_InsertMiddle(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    V tmpl;
    tmpl.reserve(2 * n);
    fill_vector(tmpl, n);

    V v;
    v.reserve(2 * n);
    for (auto _: state) {
        state.PauseTiming();
        v = tmpl;
        v.reserve(2 * n);
        state.ResumeTiming();

        for (size_t k = 0; k < k_shift_batch; ++k) {
            v.insert(v.begin() + static_cast<ptrdiff_t>(n / 2), static_cast<value_t>(7));
        }

        benchmark::DoNotOptimize(v.data());
        benchmark::DoNotOptimize(v.size());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(k_shift_batch));
}

static void BM_Vec_Nf_InsertMiddle(benchmark::State& state) { BM_Vec_InsertMiddle<nf_vec_t>(state); }
static void BM_Vec_Std_InsertMiddle(benchmark::State& state) { BM_Vec_InsertMiddle<std_vec_t>(state); }

// ============================================================
// 6. erase at the middle (element shift)
// ============================================================

template <typename V>
static void BM_Vec_EraseMiddle(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    V tmpl;
    fill_vector(tmpl, n);

    V v;
    for (auto _: state) {
        state.PauseTiming();
        v = tmpl;
        state.ResumeTiming();

        for (size_t k = 0; k < k_shift_batch; ++k) {
            v.erase(v.begin() + static_cast<ptrdiff_t>(n / 2));
        }

        benchmark::DoNotOptimize(v.data());
        benchmark::DoNotOptimize(v.size());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(k_shift_batch));
}

static void BM_Vec_Nf_EraseMiddle(benchmark::State& state) { BM_Vec_EraseMiddle<nf_vec_t>(state); }
static void BM_Vec_Std_EraseMiddle(benchmark::State& state) { BM_Vec_EraseMiddle<std_vec_t>(state); }

// ============================================================
// 7. sequential indexed read
// ============================================================

template <typename V>
static void BM_Vec_IndexSum(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    V v;
    fill_vector(v, n);

    for (auto _: state) {
        int64_t sum = 0;
        for (size_t i = 0; i < n; ++i) {
            sum += v[i];
        }
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}

static void BM_Vec_Nf_IndexSum(benchmark::State& state) { BM_Vec_IndexSum<nf_vec_t>(state); }
static void BM_Vec_Std_IndexSum(benchmark::State& state) { BM_Vec_IndexSum<std_vec_t>(state); }

// ============================================================
// 8. iterator traversal
// ============================================================

template <typename V>
static void BM_Vec_IterateSum(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    V v;
    fill_vector(v, n);

    for (auto _: state) {
        int64_t sum = 0;
        for (auto it = v.begin(); it != v.end(); ++it) {
            sum += *it;
        }
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}

static void BM_Vec_Nf_IterateSum(benchmark::State& state) { BM_Vec_IterateSum<nf_vec_t>(state); }
static void BM_Vec_Std_IterateSum(benchmark::State& state) { BM_Vec_IterateSum<std_vec_t>(state); }

// ============================================================
// 9. copy construction
// ============================================================

template <typename V, typename Elems = value_t>
static void BM_Vec_CopyConstruct(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    V src(n);
    for (size_t i = 0; i < n; ++i) {
        src[i] = static_cast<Elems>(i);
    }

    for (auto _: state) {
        V copy(src);
        benchmark::DoNotOptimize(copy.data());
        benchmark::DoNotOptimize(copy.size());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}

static void BM_Vec_Nf_CopyConstruct(benchmark::State& state) { BM_Vec_CopyConstruct<nf_vec_t>(state); }
static void BM_Vec_NfStdAlloc_CopyConstruct(benchmark::State& state) { BM_Vec_CopyConstruct<nf_sa_vec_t>(state); }
static void BM_Vec_Std_CopyConstruct(benchmark::State& state) { BM_Vec_CopyConstruct<std_vec_t>(state); }

// ============================================================
// 10. non-trivial element (SSO string): push_back without reserve
// ============================================================

template <typename V>
static void BM_Vec_StrPushBackGrow(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    for (auto _: state) {
        V v;
        for (size_t i = 0; i < n; ++i) {
            v.push_back(make_blob(i));
        }
        benchmark::DoNotOptimize(v.data());
        benchmark::DoNotOptimize(v.size());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}

static void BM_Vec_Nf_StrPushBackGrow(benchmark::State& state) { BM_Vec_StrPushBackGrow<nf_str_vec_t>(state); }
static void BM_Vec_Std_StrPushBackGrow(benchmark::State& state) { BM_Vec_StrPushBackGrow<std_str_vec_t>(state); }

// ============================================================
// 11. non-trivial element (SSO string): copy construction
// ============================================================

template <typename V>
static void BM_Vec_StrCopyConstruct(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    V src;
    src.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        src.push_back(make_blob(i));
    }

    for (auto _: state) {
        V copy(src);
        benchmark::DoNotOptimize(copy.data());
        benchmark::DoNotOptimize(copy.size());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}

static void BM_Vec_Nf_StrCopyConstruct(benchmark::State& state) { BM_Vec_StrCopyConstruct<nf_str_vec_t>(state); }
static void BM_Vec_Std_StrCopyConstruct(benchmark::State& state) { BM_Vec_StrCopyConstruct<std_str_vec_t>(state); }

// ============================================================
// Registration
// ============================================================

#define VEC_ARGS Args({1 << 10})->Args({1 << 15})->Args({1 << 18})->Args({1 << 20})

BENCHMARK(BM_Vec_Nf_PushBackReserved)->VEC_ARGS->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Vec_NfStdAlloc_PushBackReserved)->VEC_ARGS->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Vec_Std_PushBackReserved)->VEC_ARGS->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Vec_Nf_PushBackGrow)->VEC_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Vec_NfStdAlloc_PushBackGrow)->VEC_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Vec_Std_PushBackGrow)->VEC_ARGS->Unit(benchmark::kMillisecond);

BENCHMARK(BM_Vec_Nf_EmplaceBackReserved)->VEC_ARGS->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Vec_Std_EmplaceBackReserved)->VEC_ARGS->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Vec_Nf_ResizeGrow)->VEC_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Vec_NfStdAlloc_ResizeGrow)->VEC_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Vec_Std_ResizeGrow)->VEC_ARGS->Unit(benchmark::kMillisecond);

BENCHMARK(BM_Vec_Nf_InsertMiddle)->VEC_ARGS->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Vec_Std_InsertMiddle)->VEC_ARGS->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Vec_Nf_EraseMiddle)->VEC_ARGS->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Vec_Std_EraseMiddle)->VEC_ARGS->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Vec_Nf_IndexSum)->VEC_ARGS->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Vec_Std_IndexSum)->VEC_ARGS->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Vec_Nf_IterateSum)->VEC_ARGS->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Vec_Std_IterateSum)->VEC_ARGS->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Vec_Nf_CopyConstruct)->VEC_ARGS->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Vec_NfStdAlloc_CopyConstruct)->VEC_ARGS->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Vec_Std_CopyConstruct)->VEC_ARGS->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Vec_Nf_StrPushBackGrow)->VEC_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Vec_Std_StrPushBackGrow)->VEC_ARGS->Unit(benchmark::kMillisecond);

BENCHMARK(BM_Vec_Nf_StrCopyConstruct)->VEC_ARGS->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Vec_Std_StrCopyConstruct)->VEC_ARGS->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
