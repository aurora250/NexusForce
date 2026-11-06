#include <benchmark/benchmark.h>
#include <NeForce/core/container/deque.hpp>
#include <deque>

namespace {

    using value_t = int;
    using nf_deque_t = neforce::deque<value_t>;
    using std_deque_t = std::deque<value_t>;

    /// 线性大小的基准规模枚举
#define NEFORCE_LINEAR_SIZES ->Args({1024})->Args({32768})->Args({262144})->Args({1048576})
    /// 中部插入/删除的基准规模枚举（单次操作 O(n)，控制总耗时）
#define NEFORCE_MIDDLE_SIZES ->Args({1024})->Args({32768})->Args({262144})
    /// 每个计时区间内反复进行的次数
    constexpr size_t LINEAR_REPEATS = 1;
    constexpr size_t MIDDLE_REPEATS = 64;

    /// 确定性伪随机数发生器，避免基准之间共享 RNG 状态
    struct lcg {
        size_t state = 0x2545F4914F6CDD1DULL;

        size_t next(const size_t bound) noexcept {
            state = state * 6364136223846793005ULL + 1442695040888963407ULL;
            return static_cast<size_t>(state >> 33) % bound;
        }
    };

    template <typename Deque>
    void fill_back(Deque& d, const size_t n) {
        for (size_t i = 0; i < n; ++i) {
            d.push_back(static_cast<value_t>(i));
        }
    }

    template <typename Deque>
    void fill_front(Deque& d, const size_t n) {
        for (size_t i = 0; i < n; ++i) {
            d.push_front(static_cast<value_t>(i));
        }
    }

    template <typename Deque>
    long long sum_all(const Deque& d) {
        long long sum = 0;
        for (const auto& v: d) {
            sum += v;
        }
        return sum;
    }

    template <typename Deque>
    long long sum_random(const Deque& d) {
        lcg rng;
        const size_t n = d.size();
        long long sum = 0;
        for (size_t i = 0; i < n; ++i) {
            sum += d[static_cast<ptrdiff_t>(rng.next(n))];
        }
        return sum;
    }

} // namespace

// ============================================================
// 尾部插入
// ============================================================

static void BM_NfDeque_PushBack(benchmark::State& state) {
    const auto n = static_cast<size_t>(state.range(0));
    for (auto _: state) {
        for (size_t r = 0; r < LINEAR_REPEATS; ++r) {
            nf_deque_t d;
            fill_back(d, n);
            benchmark::DoNotOptimize(d);
        }
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n) * LINEAR_REPEATS);
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n) *
                            static_cast<int64_t>(sizeof(value_t)) * LINEAR_REPEATS);
}
BENCHMARK(BM_NfDeque_PushBack) NEFORCE_LINEAR_SIZES;

static void BM_StdDeque_PushBack(benchmark::State& state) {
    const auto n = static_cast<size_t>(state.range(0));
    for (auto _: state) {
        for (size_t r = 0; r < LINEAR_REPEATS; ++r) {
            std_deque_t d;
            fill_back(d, n);
            benchmark::DoNotOptimize(d);
        }
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n) * LINEAR_REPEATS);
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n) *
                            static_cast<int64_t>(sizeof(value_t)) * LINEAR_REPEATS);
}
BENCHMARK(BM_StdDeque_PushBack) NEFORCE_LINEAR_SIZES;

// ============================================================
// 尾部原地构造
// ============================================================

static void BM_NfDeque_EmplaceBack(benchmark::State& state) {
    const auto n = static_cast<size_t>(state.range(0));
    for (auto _: state) {
        for (size_t r = 0; r < LINEAR_REPEATS; ++r) {
            nf_deque_t d;
            for (size_t i = 0; i < n; ++i) {
                d.emplace_back(static_cast<value_t>(i));
            }
            benchmark::DoNotOptimize(d);
        }
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n) * LINEAR_REPEATS);
}
BENCHMARK(BM_NfDeque_EmplaceBack) NEFORCE_LINEAR_SIZES;

static void BM_StdDeque_EmplaceBack(benchmark::State& state) {
    const auto n = static_cast<size_t>(state.range(0));
    for (auto _: state) {
        for (size_t r = 0; r < LINEAR_REPEATS; ++r) {
            std_deque_t d;
            for (size_t i = 0; i < n; ++i) {
                d.emplace_back(static_cast<value_t>(i));
            }
            benchmark::DoNotOptimize(d);
        }
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n) * LINEAR_REPEATS);
}
BENCHMARK(BM_StdDeque_EmplaceBack) NEFORCE_LINEAR_SIZES;

// ============================================================
// 头部插入
// ============================================================

static void BM_NfDeque_PushFront(benchmark::State& state) {
    const auto n = static_cast<size_t>(state.range(0));
    for (auto _: state) {
        for (size_t r = 0; r < LINEAR_REPEATS; ++r) {
            nf_deque_t d;
            fill_front(d, n);
            benchmark::DoNotOptimize(d);
        }
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n) * LINEAR_REPEATS);
}
BENCHMARK(BM_NfDeque_PushFront) NEFORCE_LINEAR_SIZES;

static void BM_StdDeque_PushFront(benchmark::State& state) {
    const auto n = static_cast<size_t>(state.range(0));
    for (auto _: state) {
        for (size_t r = 0; r < LINEAR_REPEATS; ++r) {
            std_deque_t d;
            fill_front(d, n);
            benchmark::DoNotOptimize(d);
        }
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n) * LINEAR_REPEATS);
}
BENCHMARK(BM_StdDeque_PushFront) NEFORCE_LINEAR_SIZES;

// ============================================================
// 尾部弹出
// ============================================================

#define NEFORCE_DEFINE_POP_BACK(IMPL, DEQUE)                                                         \
    static void BM_##IMPL##Deque_PopBack(benchmark::State& state) {                                  \
        const auto n = static_cast<size_t>(state.range(0));                                          \
        DEQUE d;                                                                                     \
        fill_back(d, n);                                                                             \
        for (auto _: state) {                                                                        \
            state.PauseTiming();                                                                     \
            d.clear();                                                                               \
            fill_back(d, n);                                                                         \
            state.ResumeTiming();                                                                    \
            for (size_t i = 0; i < n; ++i) {                                                         \
                d.pop_back();                                                                        \
            }                                                                                        \
            benchmark::DoNotOptimize(d);                                                             \
        }                                                                                            \
        state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n)); \
    }                                                                                                \
    BENCHMARK(BM_##IMPL##Deque_PopBack) NEFORCE_LINEAR_SIZES;
NEFORCE_DEFINE_POP_BACK(Nf, nf_deque_t);
NEFORCE_DEFINE_POP_BACK(Std, std_deque_t);

// ============================================================
// 头部弹出
// ============================================================

#define NEFORCE_DEFINE_POP_FRONT(IMPL, DEQUE)                                                        \
    static void BM_##IMPL##Deque_PopFront(benchmark::State& state) {                                 \
        const auto n = static_cast<size_t>(state.range(0));                                          \
        DEQUE d;                                                                                     \
        fill_back(d, n);                                                                             \
        for (auto _: state) {                                                                        \
            state.PauseTiming();                                                                     \
            d.clear();                                                                               \
            fill_back(d, n);                                                                         \
            state.ResumeTiming();                                                                    \
            for (size_t i = 0; i < n; ++i) {                                                         \
                d.pop_front();                                                                       \
            }                                                                                        \
            benchmark::DoNotOptimize(d);                                                             \
        }                                                                                            \
        state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n)); \
    }                                                                                                \
    BENCHMARK(BM_##IMPL##Deque_PopFront) NEFORCE_LINEAR_SIZES;
NEFORCE_DEFINE_POP_FRONT(Nf, nf_deque_t);
NEFORCE_DEFINE_POP_FRONT(Std, std_deque_t);

// ============================================================
// 随机下标访问
// ============================================================

#define NEFORCE_DEFINE_RANDOM_ACCESS(IMPL, DEQUE)                                                    \
    static void BM_##IMPL##Deque_RandomAccess(benchmark::State& state) {                             \
        const auto n = static_cast<size_t>(state.range(0));                                          \
        DEQUE d;                                                                                     \
        fill_back(d, n);                                                                             \
        for (auto _: state) {                                                                        \
            benchmark::DoNotOptimize(sum_random(d));                                                 \
        }                                                                                            \
        state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n)); \
    }                                                                                                \
    BENCHMARK(BM_##IMPL##Deque_RandomAccess) NEFORCE_LINEAR_SIZES;
NEFORCE_DEFINE_RANDOM_ACCESS(Nf, nf_deque_t);
NEFORCE_DEFINE_RANDOM_ACCESS(Std, std_deque_t);

// ============================================================
// 全量顺序遍历
// ============================================================

#define NEFORCE_DEFINE_ITERATE(IMPL, DEQUE)                                                          \
    static void BM_##IMPL##Deque_Iterate(benchmark::State& state) {                                  \
        const auto n = static_cast<size_t>(state.range(0));                                          \
        DEQUE d;                                                                                     \
        fill_back(d, n);                                                                             \
        for (auto _: state) {                                                                        \
            benchmark::DoNotOptimize(sum_all(d));                                                    \
        }                                                                                            \
        state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n)); \
    }                                                                                                \
    BENCHMARK(BM_##IMPL##Deque_Iterate) NEFORCE_LINEAR_SIZES;
NEFORCE_DEFINE_ITERATE(Nf, nf_deque_t);
NEFORCE_DEFINE_ITERATE(Std, std_deque_t);

// ============================================================
// 中部插入
// ============================================================

#define NEFORCE_DEFINE_INSERT_MIDDLE(IMPL, DEQUE)                                                    \
    static void BM_##IMPL##Deque_InsertMiddle(benchmark::State& state) {                             \
        const auto n = static_cast<size_t>(state.range(0));                                          \
        DEQUE d;                                                                                     \
        fill_back(d, n);                                                                             \
        for (auto _: state) {                                                                        \
            state.PauseTiming();                                                                     \
            d.clear();                                                                               \
            fill_back(d, n);                                                                         \
            state.ResumeTiming();                                                                    \
            for (size_t r = 0; r < MIDDLE_REPEATS; ++r) {                                            \
                d.insert(d.begin() + static_cast<ptrdiff_t>(d.size() / 2), static_cast<value_t>(r)); \
            }                                                                                        \
            benchmark::DoNotOptimize(d);                                                             \
        }                                                                                            \
        state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * MIDDLE_REPEATS);          \
    }                                                                                                \
    BENCHMARK(BM_##IMPL##Deque_InsertMiddle) NEFORCE_MIDDLE_SIZES

NEFORCE_DEFINE_INSERT_MIDDLE(Nf, nf_deque_t);
NEFORCE_DEFINE_INSERT_MIDDLE(Std, std_deque_t);

// ============================================================
// 中部删除
// ============================================================

#define NEFORCE_DEFINE_ERASE_MIDDLE(IMPL, DEQUE)                                            \
    static void BM_##IMPL##Deque_EraseMiddle(benchmark::State& state) {                     \
        const auto n = static_cast<size_t>(state.range(0));                                 \
        DEQUE d;                                                                            \
        fill_back(d, n);                                                                    \
        for (auto _: state) {                                                               \
            state.PauseTiming();                                                            \
            d.clear();                                                                      \
            for (size_t i = 0; i < n; ++i) {                                                \
                d.push_back(static_cast<value_t>(i));                                       \
            }                                                                               \
            state.ResumeTiming();                                                           \
            for (size_t r = 0; r < MIDDLE_REPEATS; ++r) {                                   \
                d.erase(d.begin() + static_cast<ptrdiff_t>(d.size() / 2));                  \
            }                                                                               \
            benchmark::DoNotOptimize(d);                                                    \
        }                                                                                   \
        state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * MIDDLE_REPEATS); \
    }                                                                                       \
    BENCHMARK(BM_##IMPL##Deque_EraseMiddle) NEFORCE_MIDDLE_SIZES

NEFORCE_DEFINE_ERASE_MIDDLE(Nf, nf_deque_t);
NEFORCE_DEFINE_ERASE_MIDDLE(Std, std_deque_t);

// ============================================================
// 拷贝构造
// ============================================================

#define NEFORCE_DEFINE_COPY(IMPL, DEQUE)                                                             \
    static void BM_##IMPL##Deque_Copy(benchmark::State& state) {                                     \
        const auto n = static_cast<size_t>(state.range(0));                                          \
        DEQUE d;                                                                                     \
        fill_back(d, n);                                                                             \
        for (auto _: state) {                                                                        \
            DEQUE c(d);                                                                              \
            benchmark::DoNotOptimize(c);                                                             \
        }                                                                                            \
        state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n)); \
    }                                                                                                \
    BENCHMARK(BM_##IMPL##Deque_Copy) NEFORCE_LINEAR_SIZES;
NEFORCE_DEFINE_COPY(Nf, nf_deque_t);
NEFORCE_DEFINE_COPY(Std, std_deque_t);

// ============================================================
// 首尾访问
// ============================================================

#define NEFORCE_DEFINE_FRONT_BACK(IMPL, DEQUE)                                                       \
    static void BM_##IMPL##Deque_FrontBack(benchmark::State& state) {                                \
        const auto n = static_cast<size_t>(state.range(0));                                          \
        DEQUE d;                                                                                     \
        fill_back(d, n);                                                                             \
        for (auto _: state) {                                                                        \
            long long sum = 0;                                                                       \
            for (size_t i = 0; i < n; ++i) {                                                         \
                sum += d.front();                                                                    \
                sum += d.back();                                                                     \
            }                                                                                        \
            benchmark::DoNotOptimize(sum);                                                           \
        }                                                                                            \
        state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n)); \
    }                                                                                                \
    BENCHMARK(BM_##IMPL##Deque_FrontBack) NEFORCE_LINEAR_SIZES;
NEFORCE_DEFINE_FRONT_BACK(Nf, nf_deque_t);
NEFORCE_DEFINE_FRONT_BACK(Std, std_deque_t);

BENCHMARK_MAIN();
