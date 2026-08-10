#include <benchmark/benchmark.h>
#include <NeForce/core/container/flat_hashtable.hpp>
#include <NeForce/core/container/hashtable.hpp>
#include <NeForce/core/container/vector.hpp>
#include <NeForce/core/numeric/random.hpp>
#include <unordered_set>
using namespace neforce;

namespace {

    struct pair_key_extract {
        const int& operator()(const pair<int, int>& p) const noexcept { return p.first; }
    };

    using value_t = pair<int, int>;
    using key_t = int;

    using flat_ht_t = flat_hashtable<value_t, key_t, hash<int>, pair_key_extract, equal_to<int>, allocator<value_t>>;
    using chain_ht_t =
            hashtable<value_t, key_t, hash<int>, pair_key_extract, equal_to<int>, allocator<hashtable_node<value_t>>>;

    vector<key_t> make_keys(size_t count) {
        vector<key_t> keys(count);
        thread_local random_mt rnd{};
        for (size_t i = 0; i < count; ++i) {
            keys[i] = rnd.next_int(0, numeric_traits<int>::max());
        }
        return keys;
    }

    vector<key_t> make_missing_keys(size_t count) {
        vector<key_t> keys(count);
        thread_local random_mt rnd{};
        for (size_t i = 0; i < count; ++i) {
            keys[i] = rnd.next_int(numeric_traits<int>::min(), -1);
        }
        return keys;
    }

} // namespace

// ============================================================
// 1. Insert
// ============================================================

template <typename HT>
static void BM_HashTable_Insert(benchmark::State& state, HT& ht, const vector<key_t>& keys) {
    for (auto _: state) {
        ht.clear();
        for (size_t i = 0; i < keys.size(); ++i) {
            ht.insert_unique(value_t(keys[i], static_cast<int>(i)));
        }
        benchmark::DoNotOptimize(ht.size());
    }
    state.SetItemsProcessed(state.iterations() * keys.size());
}

static void BM_FlatHT_Insert(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = make_keys(n);
    flat_ht_t ht(n);
    BM_HashTable_Insert<flat_ht_t>(state, ht, keys);
}

static void BM_ChainHT_Insert(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = make_keys(n);
    chain_ht_t ht(n);
    BM_HashTable_Insert<chain_ht_t>(state, ht, keys);
}

static void BM_StdUnorderedSet_Insert(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = make_keys(n);
    for (auto _: state) {
        std::unordered_set<int> s;
        s.reserve(n);
        for (size_t i = 0; i < keys.size(); ++i) {
            s.insert(keys[i]);
        }
        benchmark::DoNotOptimize(s.size());
    }
    state.SetItemsProcessed(state.iterations() * n);
}

// ============================================================
// 2. Emplace (construct in-place)
// ============================================================

static void BM_FlatHT_Emplace(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = make_keys(n);
    for (auto _: state) {
        flat_ht_t ht(n);
        for (size_t i = 0; i < keys.size(); ++i) {
            ht.emplace_unique(keys[i], static_cast<int>(i));
        }
        benchmark::DoNotOptimize(ht.size());
    }
    state.SetItemsProcessed(state.iterations() * n);
}

static void BM_ChainHT_Emplace(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = make_keys(n);
    for (auto _: state) {
        chain_ht_t ht(n);
        for (size_t i = 0; i < keys.size(); ++i) {
            ht.emplace_unique(keys[i], static_cast<int>(i));
        }
        benchmark::DoNotOptimize(ht.size());
    }
    state.SetItemsProcessed(state.iterations() * n);
}

// ============================================================
// 3. Find (hit)
// ============================================================

template <typename HT>
static void BM_HashTable_Find(benchmark::State& state, const HT& ht, const vector<key_t>& keys) {
    for (auto _: state) {
        size_t found = 0;
        for (size_t i = 0; i < keys.size(); ++i) {
            auto it = ht.find(keys[i]);
            if (it != ht.end()) {
                ++found;
            }
        }
        benchmark::DoNotOptimize(found);
    }
    state.SetItemsProcessed(state.iterations() * keys.size());
}

static void BM_FlatHT_Find(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = make_keys(n);
    flat_ht_t ht(n);
    for (size_t i = 0; i < keys.size(); ++i) {
        ht.emplace_unique(keys[i], static_cast<int>(i));
    }
    BM_HashTable_Find<flat_ht_t>(state, ht, keys);
}

static void BM_ChainHT_Find(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = make_keys(n);
    chain_ht_t ht(n);
    for (size_t i = 0; i < keys.size(); ++i) {
        ht.emplace_unique(keys[i], static_cast<int>(i));
    }
    BM_HashTable_Find<chain_ht_t>(state, ht, keys);
}

static void BM_StdUnorderedSet_Find(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = make_keys(n);
    std::unordered_set<int> s;
    s.reserve(n);
    for (size_t i = 0; i < keys.size(); ++i) {
        s.insert(keys[i]);
    }
    for (auto _: state) {
        size_t found = 0;
        for (size_t i = 0; i < keys.size(); ++i) {
            if (s.find(keys[i]) != s.end()) {
                ++found;
            }
        }
        benchmark::DoNotOptimize(found);
    }
    state.SetItemsProcessed(state.iterations() * n);
}

// ============================================================
// 4. Find (miss)
// ============================================================

template <typename HT>
static void BM_HashTable_FindMiss(benchmark::State& state, const HT& ht, const vector<key_t>& keys,
                                  const vector<key_t>& missing) {
    for (auto _: state) {
        size_t missed = 0;
        for (size_t i = 0; i < missing.size(); ++i) {
            if (ht.find(missing[i]) == ht.end()) {
                ++missed;
            }
        }
        benchmark::DoNotOptimize(missed);
    }
    state.SetItemsProcessed(state.iterations() * missing.size());
}

static void BM_FlatHT_FindMiss(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = make_keys(n);
    auto missing = make_missing_keys(n);
    flat_ht_t ht(n);
    for (size_t i = 0; i < keys.size(); ++i) {
        ht.emplace_unique(keys[i], static_cast<int>(i));
    }
    BM_HashTable_FindMiss<flat_ht_t>(state, ht, keys, missing);
}

static void BM_ChainHT_FindMiss(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = make_keys(n);
    auto missing = make_missing_keys(n);
    chain_ht_t ht(n);
    for (size_t i = 0; i < keys.size(); ++i) {
        ht.emplace_unique(keys[i], static_cast<int>(i));
    }
    BM_HashTable_FindMiss<chain_ht_t>(state, ht, keys, missing);
}

static void BM_StdUnorderedSet_FindMiss(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = make_keys(n);
    auto missing = make_missing_keys(n);
    std::unordered_set<int> s;
    s.reserve(n);
    for (size_t i = 0; i < keys.size(); ++i) {
        s.insert(keys[i]);
    }
    for (auto _: state) {
        size_t missed = 0;
        for (size_t i = 0; i < missing.size(); ++i) {
            if (s.find(missing[i]) == s.end()) {
                ++missed;
            }
        }
        benchmark::DoNotOptimize(missed);
    }
    state.SetItemsProcessed(state.iterations() * n);
}

// ============================================================
// 5. Erase
// ============================================================

template <typename HT>
static void BM_HashTable_Erase(benchmark::State& state, HT& ht, const vector<key_t>& keys) {
    for (auto _: state) {
        state.PauseTiming();
        for (size_t i = 0; i < keys.size(); ++i) {
            ht.emplace_unique(keys[i], static_cast<int>(i));
        }
        state.ResumeTiming();

        size_t erased = 0;
        for (size_t i = 0; i < keys.size(); ++i) {
            erased += ht.erase(keys[i]);
        }
        benchmark::DoNotOptimize(erased);
    }
    state.SetItemsProcessed(state.iterations() * keys.size());
}

static void BM_FlatHT_Erase(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = make_keys(n);
    flat_ht_t ht(n);
    BM_HashTable_Erase<flat_ht_t>(state, ht, keys);
}

static void BM_ChainHT_Erase(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = make_keys(n);
    chain_ht_t ht(n);
    BM_HashTable_Erase<chain_ht_t>(state, ht, keys);
}

static void BM_StdUnorderedSet_Erase(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = make_keys(n);
    for (auto _: state) {
        state.PauseTiming();
        std::unordered_set<int> s;
        s.reserve(n);
        for (size_t i = 0; i < keys.size(); ++i) {
            s.insert(keys[i]);
        }
        state.ResumeTiming();

        size_t erased = 0;
        for (size_t i = 0; i < keys.size(); ++i) {
            erased += s.erase(keys[i]);
        }
        benchmark::DoNotOptimize(erased);
    }
    state.SetItemsProcessed(state.iterations() * n);
}

// ============================================================
// 6. Iteration
// ============================================================

template <typename HT>
static void BM_HashTable_Iterate(benchmark::State& state, const HT& ht) {
    for (auto _: state) {
        typename HT::size_type sum = 0;
        for (auto it = ht.begin(); it != ht.end(); ++it) {
            const value_t& v = *it;
            sum += v.second;
            benchmark::DoNotOptimize(sum);
        }
    }
    state.SetItemsProcessed(state.iterations() * ht.size());
}

static void BM_FlatHT_Iterate(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = make_keys(n);
    flat_ht_t ht(n);
    for (size_t i = 0; i < keys.size(); ++i) {
        ht.emplace_unique(keys[i], static_cast<int>(i));
    }
    BM_HashTable_Iterate<flat_ht_t>(state, ht);
}

static void BM_ChainHT_Iterate(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = make_keys(n);
    chain_ht_t ht(n);
    for (size_t i = 0; i < keys.size(); ++i) {
        ht.emplace_unique(keys[i], static_cast<int>(i));
    }
    BM_HashTable_Iterate<chain_ht_t>(state, ht);
}

static void BM_StdUnorderedSet_Iterate(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = make_keys(n);
    std::unordered_set<int> s;
    s.reserve(n);
    for (size_t i = 0; i < keys.size(); ++i) {
        s.insert(keys[i]);
    }
    for (auto _: state) {
        size_t sum = 0;
        for (const auto& v: s) {
            sum += v;
            benchmark::DoNotOptimize(sum);
        }
    }
    state.SetItemsProcessed(state.iterations() * n);
}

// ============================================================
// 7. Contains (boolean check)
// ============================================================

static void BM_FlatHT_Contains(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = make_keys(n);
    auto missing = make_missing_keys(n / 10);
    flat_ht_t ht(n);
    for (size_t i = 0; i < keys.size(); ++i) {
        ht.emplace_unique(keys[i], static_cast<int>(i));
    }
    for (auto _: state) {
        size_t count = 0;
        for (size_t i = 0; i < missing.size(); ++i) {
            if (!ht.contains(missing[i])) {
                ++count;
            }
        }
        benchmark::DoNotOptimize(count);
    }
    state.SetItemsProcessed(state.iterations() * missing.size());
}

static void BM_ChainHT_Contains(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = make_keys(n);
    auto missing = make_missing_keys(n / 10);
    chain_ht_t ht(n);
    for (size_t i = 0; i < keys.size(); ++i) {
        ht.emplace_unique(keys[i], static_cast<int>(i));
    }
    for (auto _: state) {
        size_t count = 0;
        for (size_t i = 0; i < missing.size(); ++i) {
            if (!ht.contains(missing[i])) {
                ++count;
            }
        }
        benchmark::DoNotOptimize(count);
    }
    state.SetItemsProcessed(state.iterations() * missing.size());
}

// ============================================================
// 8. Rehash impact (insert with reserve vs without)
// ============================================================

static void BM_FlatHT_InsertNoReserve(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = make_keys(n);
    for (auto _: state) {
        flat_ht_t ht;
        for (size_t i = 0; i < keys.size(); ++i) {
            ht.emplace_unique(keys[i], static_cast<int>(i));
        }
        benchmark::DoNotOptimize(ht.size());
    }
    state.SetItemsProcessed(state.iterations() * n);
}

static void BM_ChainHT_InsertNoReserve(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = make_keys(n);
    for (auto _: state) {
        chain_ht_t ht(0);
        for (size_t i = 0; i < keys.size(); ++i) {
            ht.emplace_unique(keys[i], static_cast<int>(i));
        }
        benchmark::DoNotOptimize(ht.size());
    }
    state.SetItemsProcessed(state.iterations() * n);
}

// ============================================================
// 9. Copy construction
// ============================================================

static void BM_FlatHT_Copy(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = make_keys(n);
    flat_ht_t src(n);
    for (size_t i = 0; i < keys.size(); ++i) {
        src.emplace_unique(keys[i], static_cast<int>(i));
    }
    for (auto _: state) {
        flat_ht_t copy(src);
        benchmark::DoNotOptimize(copy.size());
    }
    state.SetItemsProcessed(state.iterations() * n);
}

static void BM_ChainHT_Copy(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = make_keys(n);
    chain_ht_t src(n);
    for (size_t i = 0; i < keys.size(); ++i) {
        src.emplace_unique(keys[i], static_cast<int>(i));
    }
    for (auto _: state) {
        chain_ht_t copy(src);
        benchmark::DoNotOptimize(copy.size());
    }
    state.SetItemsProcessed(state.iterations() * n);
}

// ============================================================
// Benchmark registration
// ============================================================

#define INSERT_ARGS Args({1 << 10})->Args({1 << 15})->Args({1 << 18})->Args({1 << 20})

BENCHMARK(BM_FlatHT_Insert)->INSERT_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_ChainHT_Insert)->INSERT_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_StdUnorderedSet_Insert)->INSERT_ARGS->Unit(benchmark::kMillisecond);

BENCHMARK(BM_FlatHT_Emplace)->INSERT_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_ChainHT_Emplace)->INSERT_ARGS->Unit(benchmark::kMillisecond);

BENCHMARK(BM_FlatHT_Find)->INSERT_ARGS->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_ChainHT_Find)->INSERT_ARGS->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_StdUnorderedSet_Find)->INSERT_ARGS->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_FlatHT_FindMiss)->INSERT_ARGS->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_ChainHT_FindMiss)->INSERT_ARGS->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_StdUnorderedSet_FindMiss)->INSERT_ARGS->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_FlatHT_Erase)->INSERT_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_ChainHT_Erase)->INSERT_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_StdUnorderedSet_Erase)->INSERT_ARGS->Unit(benchmark::kMillisecond);

BENCHMARK(BM_FlatHT_Iterate)->INSERT_ARGS->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_ChainHT_Iterate)->INSERT_ARGS->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_StdUnorderedSet_Iterate)->INSERT_ARGS->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_FlatHT_Contains)->INSERT_ARGS->Unit(benchmark::kNanosecond);
BENCHMARK(BM_ChainHT_Contains)->INSERT_ARGS->Unit(benchmark::kNanosecond);

BENCHMARK(BM_FlatHT_InsertNoReserve)->INSERT_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_ChainHT_InsertNoReserve)->INSERT_ARGS->Unit(benchmark::kMillisecond);

BENCHMARK(BM_FlatHT_Copy)->INSERT_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_ChainHT_Copy)->INSERT_ARGS->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
