#include <benchmark/benchmark.h>
#include <NeForce/core/string/string.hpp>
#include <string>
using namespace neforce;

namespace {

    const char* g_short_literal = "hello";
    const char* g_long_literal = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
                                 "Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. "
                                 "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris.";
    const size_t g_long_len = std::char_traits<char>::length(g_long_literal);

    string make_nf_string(size_t n) { return string(n, 'x'); }

    std::string make_std_string(size_t n) { return std::string(n, 'x'); }

} // namespace

// ============================================================
// 1. Default construction
// ============================================================

static void BM_NfString_DefaultConstruct(benchmark::State& state) {
    for (auto _: state) {
        string s;
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_NfString_DefaultConstruct);

static void BM_StdString_DefaultConstruct(benchmark::State& state) {
    for (auto _: state) {
        std::string s;
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_StdString_DefaultConstruct);

// ============================================================
// 2. Construction from C-string
// ============================================================

static void BM_NfString_ConstructShort(benchmark::State& state) {
    for (auto _: state) {
        string s(g_short_literal);
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_NfString_ConstructShort);

static void BM_StdString_ConstructShort(benchmark::State& state) {
    for (auto _: state) {
        std::string s(g_short_literal);
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_StdString_ConstructShort);

static void BM_NfString_ConstructLong(benchmark::State& state) {
    for (auto _: state) {
        string s(g_long_literal);
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_NfString_ConstructLong);

static void BM_StdString_ConstructLong(benchmark::State& state) {
    for (auto _: state) {
        std::string s(g_long_literal);
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_StdString_ConstructLong);

// ============================================================
// 3. Construction from repeated char
// ============================================================

static void BM_NfString_ConstructFill(benchmark::State& state) {
    const size_t n = state.range(0);
    for (auto _: state) {
        string s(n, 'x');
        benchmark::DoNotOptimize(s);
    }
    state.SetItemsProcessed(state.iterations() * n);
}

static void BM_StdString_ConstructFill(benchmark::State& state) {
    const size_t n = state.range(0);
    for (auto _: state) {
        std::string s(n, 'x');
        benchmark::DoNotOptimize(s);
    }
    state.SetItemsProcessed(state.iterations() * n);
}

// ============================================================
// 4. Copy construction
// ============================================================

static void BM_NfString_CopyShort(benchmark::State& state) {
    string src(g_short_literal);
    for (auto _: state) {
        string s(src);
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_NfString_CopyShort);

static void BM_StdString_CopyShort(benchmark::State& state) {
    std::string src(g_short_literal);
    for (auto _: state) {
        std::string s(src);
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_StdString_CopyShort);

static void BM_NfString_CopyLong(benchmark::State& state) {
    string src(g_long_literal);
    for (auto _: state) {
        string s(src);
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_NfString_CopyLong);

static void BM_StdString_CopyLong(benchmark::State& state) {
    std::string src(g_long_literal);
    for (auto _: state) {
        std::string s(src);
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_StdString_CopyLong);

// ============================================================
// 5. Append single char
// ============================================================

static void BM_NfString_AppendChar(benchmark::State& state) {
    const size_t n = state.range(0);
    for (auto _: state) {
        string s;
        s.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            s.push_back('x');
        }
        benchmark::DoNotOptimize(s);
    }
    state.SetItemsProcessed(state.iterations() * n);
}

static void BM_StdString_AppendChar(benchmark::State& state) {
    const size_t n = state.range(0);
    for (auto _: state) {
        std::string s;
        s.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            s.push_back('x');
        }
        benchmark::DoNotOptimize(s);
    }
    state.SetItemsProcessed(state.iterations() * n);
}

// ============================================================
// 6. Append C-string (bulk)
// ============================================================

static void BM_NfString_AppendBulk(benchmark::State& state) {
    const size_t n = state.range(0);
    for (auto _: state) {
        string s;
        const size_t chunks = n / g_long_len;
        s.reserve(n);
        for (size_t i = 0; i < chunks; ++i) {
            s.append(g_long_literal);
        }
        benchmark::DoNotOptimize(s);
    }
    state.SetItemsProcessed(state.iterations() * n);
}

static void BM_StdString_AppendBulk(benchmark::State& state) {
    const size_t n = state.range(0);
    for (auto _: state) {
        std::string s;
        const size_t chunks = n / g_long_len;
        s.reserve(n);
        for (size_t i = 0; i < chunks; ++i) {
            s.append(g_long_literal);
        }
        benchmark::DoNotOptimize(s);
    }
    state.SetItemsProcessed(state.iterations() * n);
}

// ============================================================
// 7. Find character
// ============================================================

static void BM_NfString_FindChar(benchmark::State& state) {
    const size_t n = state.range(0);
    string s(n, 'a');
    s[n - 1] = 'z';
    for (auto _: state) {
        auto pos = s.find('z');
        benchmark::DoNotOptimize(pos);
    }
    state.SetItemsProcessed(state.iterations());
}

static void BM_StdString_FindChar(benchmark::State& state) {
    const size_t n = state.range(0);
    std::string s(n, 'a');
    s[n - 1] = 'z';
    for (auto _: state) {
        auto pos = s.find('z');
        benchmark::DoNotOptimize(pos);
    }
    state.SetItemsProcessed(state.iterations());
}

// ============================================================
// 8. Substring
// ============================================================

static void BM_NfString_Substr(benchmark::State& state) {
    const size_t n = state.range(0);
    string s(n, 'x');
    for (auto _: state) {
        auto sub = s.substr(n / 4, n / 2);
        benchmark::DoNotOptimize(sub);
    }
    state.SetItemsProcessed(state.iterations());
}

static void BM_StdString_Substr(benchmark::State& state) {
    const size_t n = state.range(0);
    std::string s(n, 'x');
    for (auto _: state) {
        auto sub = s.substr(n / 4, n / 2);
        benchmark::DoNotOptimize(sub);
    }
    state.SetItemsProcessed(state.iterations());
}

// ============================================================
// 9. Iteration
// ============================================================

static void BM_NfString_Iterate(benchmark::State& state) {
    const size_t n = state.range(0);
    string s(n, 'x');
    for (auto _: state) {
        size_t sum = 0;
        for (const auto& c: s) {
            sum += static_cast<unsigned char>(c);
        }
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * n);
}

static void BM_StdString_Iterate(benchmark::State& state) {
    const size_t n = state.range(0);
    std::string s(n, 'x');
    for (auto _: state) {
        size_t sum = 0;
        for (const auto& c: s) {
            sum += static_cast<unsigned char>(c);
        }
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * n);
}

// ============================================================
// Registration
// ============================================================

#define STRING_ARGS Arg(1 << 6)->Arg(1 << 10)->Arg(1 << 15)->Arg(1 << 20)

BENCHMARK(BM_NfString_ConstructFill)->STRING_ARGS->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_StdString_ConstructFill)->STRING_ARGS->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_NfString_AppendChar)->STRING_ARGS->Unit(benchmark::kMillisecond);
BENCHMARK(BM_StdString_AppendChar)->STRING_ARGS->Unit(benchmark::kMillisecond);

BENCHMARK(BM_NfString_AppendBulk)->STRING_ARGS->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_StdString_AppendBulk)->STRING_ARGS->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_NfString_FindChar)->STRING_ARGS->Unit(benchmark::kNanosecond);
BENCHMARK(BM_StdString_FindChar)->STRING_ARGS->Unit(benchmark::kNanosecond);

BENCHMARK(BM_NfString_Substr)->STRING_ARGS->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_StdString_Substr)->STRING_ARGS->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_NfString_Iterate)->STRING_ARGS->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_StdString_Iterate)->STRING_ARGS->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
