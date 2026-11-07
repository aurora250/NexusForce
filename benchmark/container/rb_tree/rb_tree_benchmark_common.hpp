#ifndef NEFORCE_BENCHMARK_RB_TREE_COMMON_HPP__
#define NEFORCE_BENCHMARK_RB_TREE_COMMON_HPP__
#include <benchmark/benchmark.h>
#include <NeForce/core/container/map.hpp>
#include <NeForce/core/container/set.hpp>
#include <NeForce/core/container/sparse_set.hpp>
#include <NeForce/core/container/vector.hpp>
#include <map>
#include <set>
using namespace neforce;

namespace {

    using key_t = int;
    using mapped_t = int;

    using nf_set_t = neforce::set<key_t>;
    using nf_sparse_t = neforce::sparse_set<key_t>;
    using std_set_t = std::set<key_t>;
    using nf_map_t = neforce::map<key_t, mapped_t>;
    using std_map_t = std::map<key_t, mapped_t>;

    uint64_t next_random(uint64_t& state) noexcept {
        state ^= state << 13;
        state ^= state >> 7;
        state ^= state << 17;
        return state;
    }

    vector<key_t> make_keys(size_t count) {
        vector<key_t> keys(count);
        uint64_t state = 0x2545F4914F6CDD1DULL;
        for (size_t i = 0; i < count; ++i) {
            keys[i] = static_cast<key_t>(next_random(state) & 0x7FFFFFFFULL);
        }
        return keys;
    }

    vector<key_t> make_missing_keys(size_t count) {
        vector<key_t> keys(count);
        uint64_t state = 0x9E3779B97F4A7C15ULL;
        for (size_t i = 0; i < count; ++i) {
            keys[i] = -static_cast<key_t>(1 + (next_random(state) & 0x3FFFFFFFULL));
        }
        return keys;
    }

    template <typename Set>
    void fill_set(Set& s, const vector<key_t>& keys) {
        for (size_t i = 0; i < keys.size(); ++i) {
            s.insert(keys[i]);
        }
    }

} // namespace

// ============================================================
// 1. Insert (unique keys)
// ============================================================

template <typename Set>
static void BM_Tree_Set_Insert(benchmark::State& state, Set& s, const vector<key_t>& keys) {
    for (auto _: state) {
        s.clear();
        for (size_t i = 0; i < keys.size(); ++i) {
            s.insert(keys[i]);
        }
        benchmark::DoNotOptimize(s.size());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(keys.size()));
}

static void BM_NfSet_Insert(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    const auto keys = make_keys(n);
    nf_set_t s;
    BM_Tree_Set_Insert<nf_set_t>(state, s, keys);
}

static void BM_StdSet_Insert(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    const auto keys = make_keys(n);
    std_set_t s;
    BM_Tree_Set_Insert<std_set_t>(state, s, keys);
}

// ============================================================
// 2. Insert with hint (ascending keys, end() hint)
// ============================================================

template <typename Set>
static void BM_Tree_Set_InsertHint(benchmark::State& state, Set& s) {
    const size_t n = static_cast<size_t>(state.range(0));
    for (auto _: state) {
        s.clear();
        for (size_t i = 0; i < n; ++i) {
            s.insert(s.end(), static_cast<key_t>(i));
        }
        benchmark::DoNotOptimize(s.size());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}

static void BM_NfSet_InsertHint(benchmark::State& state) {
    nf_set_t s;
    BM_Tree_Set_InsertHint<nf_set_t>(state, s);
}

static void BM_StdSet_InsertHint(benchmark::State& state) {
    std_set_t s;
    BM_Tree_Set_InsertHint<std_set_t>(state, s);
}

// ============================================================
// 3. Emplace
// ============================================================

template <typename Set>
static void BM_Tree_Set_Emplace(benchmark::State& state, const vector<key_t>& keys) {
    for (auto _: state) {
        Set s;
        for (size_t i = 0; i < keys.size(); ++i) {
            s.emplace(keys[i]);
        }
        benchmark::DoNotOptimize(s.size());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(keys.size()));
}

static void BM_NfSet_Emplace(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    const auto keys = make_keys(n);
    BM_Tree_Set_Emplace<nf_set_t>(state, keys);
}

static void BM_StdSet_Emplace(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    const auto keys = make_keys(n);
    BM_Tree_Set_Emplace<std_set_t>(state, keys);
}

// ============================================================
// 4. Find (hit)
// ============================================================

template <typename Set>
static void BM_Tree_Set_Find(benchmark::State& state, const Set& s, const vector<key_t>& keys) {
    for (auto _: state) {
        size_t found = 0;
        for (size_t i = 0; i < keys.size(); ++i) {
            if (s.find(keys[i]) != s.end()) {
                ++found;
            }
        }
        benchmark::DoNotOptimize(found);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(keys.size()));
}

static void BM_NfSet_Find(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    const auto keys = make_keys(n);
    nf_set_t s;
    fill_set(s, keys);
    BM_Tree_Set_Find<nf_set_t>(state, s, keys);
}

static void BM_StdSet_Find(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    const auto keys = make_keys(n);
    std_set_t s;
    fill_set(s, keys);
    BM_Tree_Set_Find<std_set_t>(state, s, keys);
}

// ============================================================
// 5. Find (miss)
// ============================================================

template <typename Set>
static void BM_Tree_Set_FindMiss(benchmark::State& state, const Set& s, const vector<key_t>& missing) {
    for (auto _: state) {
        size_t missed = 0;
        for (size_t i = 0; i < missing.size(); ++i) {
            if (s.find(missing[i]) == s.end()) {
                ++missed;
            }
        }
        benchmark::DoNotOptimize(missed);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(missing.size()));
}

static void BM_NfSet_FindMiss(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    const auto keys = make_keys(n);
    const auto missing = make_missing_keys(n);
    nf_set_t s;
    fill_set(s, keys);
    BM_Tree_Set_FindMiss<nf_set_t>(state, s, missing);
}

static void BM_StdSet_FindMiss(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    const auto keys = make_keys(n);
    const auto missing = make_missing_keys(n);
    std_set_t s;
    fill_set(s, keys);
    BM_Tree_Set_FindMiss<std_set_t>(state, s, missing);
}

// ============================================================
// 6. Erase (by key, full teardown)
// ============================================================

template <typename Set>
static void BM_Tree_Set_Erase(benchmark::State& state, Set& s, const vector<key_t>& keys) {
    for (auto _: state) {
        state.PauseTiming();
        fill_set(s, keys);
        state.ResumeTiming();

        size_t erased = 0;
        for (size_t i = 0; i < keys.size(); ++i) {
            erased += s.erase(keys[i]);
        }
        benchmark::DoNotOptimize(erased);
        benchmark::DoNotOptimize(s.size());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(keys.size()));
}

static void BM_NfSet_Erase(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    const auto keys = make_keys(n);
    nf_set_t s;
    BM_Tree_Set_Erase<nf_set_t>(state, s, keys);
}

static void BM_StdSet_Erase(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    const auto keys = make_keys(n);
    std_set_t s;
    BM_Tree_Set_Erase<std_set_t>(state, s, keys);
}

// ============================================================
// 7. Iteration
// ============================================================

template <typename Set>
static void BM_Tree_Set_Iterate(benchmark::State& state, const Set& s) {
    for (auto _: state) {
        int64_t sum = 0;
        for (auto it = s.begin(); it != s.end(); ++it) {
            sum += *it;
        }
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(s.size()));
}

static void BM_NfSet_Iterate(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    const auto keys = make_keys(n);
    nf_set_t s;
    fill_set(s, keys);
    BM_Tree_Set_Iterate<nf_set_t>(state, s);
}

static void BM_StdSet_Iterate(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    const auto keys = make_keys(n);
    std_set_t s;
    fill_set(s, keys);
    BM_Tree_Set_Iterate<std_set_t>(state, s);
}

// ============================================================
// 8. Count
// ============================================================

template <typename Set>
static void BM_Tree_Set_Count(benchmark::State& state, const Set& s, const vector<key_t>& keys) {
    for (auto _: state) {
        size_t total = 0;
        for (size_t i = 0; i < keys.size(); ++i) {
            total += s.count(keys[i]);
        }
        benchmark::DoNotOptimize(total);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(keys.size()));
}

static void BM_NfSet_Count(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    const auto keys = make_keys(n);
    nf_set_t s;
    fill_set(s, keys);
    BM_Tree_Set_Count<nf_set_t>(state, s, keys);
}

static void BM_StdSet_Count(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    const auto keys = make_keys(n);
    std_set_t s;
    fill_set(s, keys);
    BM_Tree_Set_Count<std_set_t>(state, s, keys);
}

// ============================================================
// 9. Lower bound
// ============================================================

template <typename Set>
static void BM_Tree_Set_LowerBound(benchmark::State& state, const Set& s, const vector<key_t>& keys) {
    for (auto _: state) {
        size_t hits = 0;
        for (size_t i = 0; i < keys.size(); ++i) {
            if (s.lower_bound(keys[i]) != s.end()) {
                ++hits;
            }
        }
        benchmark::DoNotOptimize(hits);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(keys.size()));
}

static void BM_NfSet_LowerBound(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    const auto keys = make_keys(n);
    nf_set_t s;
    fill_set(s, keys);
    BM_Tree_Set_LowerBound<nf_set_t>(state, s, keys);
}

static void BM_StdSet_LowerBound(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    const auto keys = make_keys(n);
    std_set_t s;
    fill_set(s, keys);
    BM_Tree_Set_LowerBound<std_set_t>(state, s, keys);
}

// ============================================================
// 10. Equal range
// ============================================================

template <typename Set>
static void BM_Tree_Set_EqualRange(benchmark::State& state, const Set& s, const vector<key_t>& keys) {
    for (auto _: state) {
        size_t dist = 0;
        for (size_t i = 0; i < keys.size(); ++i) {
            const auto range = s.equal_range(keys[i]);
            dist += static_cast<size_t>(range.first != range.second);
        }
        benchmark::DoNotOptimize(dist);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(keys.size()));
}

static void BM_NfSet_EqualRange(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    const auto keys = make_keys(n);
    nf_set_t s;
    fill_set(s, keys);
    BM_Tree_Set_EqualRange<nf_set_t>(state, s, keys);
}

static void BM_StdSet_EqualRange(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    const auto keys = make_keys(n);
    std_set_t s;
    fill_set(s, keys);
    BM_Tree_Set_EqualRange<std_set_t>(state, s, keys);
}

// ============================================================
// 11. Copy construction
// ============================================================

template <typename Set>
static void BM_Tree_Set_Copy(benchmark::State& state, const Set& s) {
    for (auto _: state) {
        Set copy(s);
        benchmark::DoNotOptimize(copy.size());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(s.size()));
}

static void BM_NfSet_Copy(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    const auto keys = make_keys(n);
    nf_set_t s;
    fill_set(s, keys);
    BM_Tree_Set_Copy<nf_set_t>(state, s);
}

static void BM_StdSet_Copy(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    const auto keys = make_keys(n);
    std_set_t s;
    fill_set(s, keys);
    BM_Tree_Set_Copy<std_set_t>(state, s);
}

// ============================================================
// 12. map: emplace
// ============================================================

template <typename Map>
static void BM_Tree_Map_Emplace(benchmark::State& state, const vector<key_t>& keys) {
    for (auto _: state) {
        Map m;
        for (size_t i = 0; i < keys.size(); ++i) {
            m.emplace(keys[i], static_cast<mapped_t>(i));
        }
        benchmark::DoNotOptimize(m.size());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(keys.size()));
}

static void BM_NfMap_Emplace(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    const auto keys = make_keys(n);
    BM_Tree_Map_Emplace<nf_map_t>(state, keys);
}

static void BM_StdMap_Emplace(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    const auto keys = make_keys(n);
    BM_Tree_Map_Emplace<std_map_t>(state, keys);
}

// ============================================================
// 13. map: find (hit)
// ============================================================

template <typename Map>
static void BM_Tree_Map_Find(benchmark::State& state, const Map& m, const vector<key_t>& keys) {
    for (auto _: state) {
        int64_t sum = 0;
        for (size_t i = 0; i < keys.size(); ++i) {
            const auto it = m.find(keys[i]);
            if (it != m.end()) {
                sum += it->second;
            }
        }
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(keys.size()));
}

static void BM_NfMap_Find(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    const auto keys = make_keys(n);
    nf_map_t m;
    for (size_t i = 0; i < keys.size(); ++i) {
        m.emplace(keys[i], static_cast<mapped_t>(i));
    }
    BM_Tree_Map_Find<nf_map_t>(state, m, keys);
}

static void BM_StdMap_Find(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    const auto keys = make_keys(n);
    std_map_t m;
    for (size_t i = 0; i < keys.size(); ++i) {
        m.emplace(keys[i], static_cast<mapped_t>(i));
    }
    BM_Tree_Map_Find<std_map_t>(state, m, keys);
}

// ============================================================
// 14. map: iteration
// ============================================================

template <typename Map>
static void BM_Tree_Map_Iterate(benchmark::State& state, const Map& m) {
    for (auto _: state) {
        int64_t sum = 0;
        for (auto it = m.begin(); it != m.end(); ++it) {
            sum += it->second;
        }
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(m.size()));
}

static void BM_NfMap_Iterate(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    const auto keys = make_keys(n);
    nf_map_t m;
    for (size_t i = 0; i < keys.size(); ++i) {
        m.emplace(keys[i], static_cast<mapped_t>(i));
    }
    BM_Tree_Map_Iterate<nf_map_t>(state, m);
}

static void BM_StdMap_Iterate(benchmark::State& state) {
    const size_t n = static_cast<size_t>(state.range(0));
    const auto keys = make_keys(n);
    std_map_t m;
    for (size_t i = 0; i < keys.size(); ++i) {
        m.emplace(keys[i], static_cast<mapped_t>(i));
    }
    BM_Tree_Map_Iterate<std_map_t>(state, m);
}


#endif // NEFORCE_BENCHMARK_RB_TREE_COMMON_HPP__
