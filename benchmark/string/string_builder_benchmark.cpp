#include <benchmark/benchmark.h>
#include <NeForce/core/string/string.hpp>
#include <NeForce/core/string/string_builder.hpp>
#include <sstream>
#include <string>
using namespace neforce;

namespace {

    const size_t g_piece_count = 64;

    const char* g_pieces[] = {
            "alpha",    "beta",     "gamma",   "delta",   "epsilon", "zeta",  "eta",   "theta", "iota",    "kappa",
            "lambda",   "mu",       "nu",      "xi",      "omicron", "pi",    "rho",   "sigma", "tau",     "upsilon",
            "phi",      "chi",      "psi",     "omega",   "Alpha",   "Beta",  "Gamma", "Delta", "Epsilon", "Zeta",
            "Eta",      "Theta",    "Iota",    "Kappa",   "Lambda",  "Mu",    "Nu",    "Xi",    "Omicron", "Pi",
            "Rho",      "Sigma",    "Tau",     "Upsilon", "Phi",     "Chi",   "Psi",   "Omega", "one",     "two",
            "three",    "four",     "five",    "six",     "seven",   "eight", "nine",  "ten",   "eleven",  "twelve",
            "thirteen", "fourteen", "fifteen", "sixteen",
    };

    const int g_numbers[] = {1, 2, 3, 4, 5, 6, 7, 8};

} // namespace

// ============================================================
// 1. string_builder append + build
// ============================================================

static void BM_StringBuilder_Build(benchmark::State& state) {
    const size_t n = state.range(0);
    for (auto _: state) {
        string_builder sb;
        sb.reserve_pieces(n);
        for (size_t i = 0; i < n; ++i) {
            sb.append(g_pieces[i % g_piece_count]);
        }
        auto result = sb.build();
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * n);
}

// ============================================================
// 2. Direct string += concatenation
// ============================================================

static void BM_String_PlusEquals(benchmark::State& state) {
    const size_t n = state.range(0);
    for (auto _: state) {
        string result;
        for (size_t i = 0; i < n; ++i) {
            result += g_pieces[i % g_piece_count];
        }
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * n);
}

// ============================================================
// 3. std::ostringstream
// ============================================================

static void BM_OStringStream(benchmark::State& state) {
    const size_t n = state.range(0);
    for (auto _: state) {
        std::ostringstream oss;
        for (size_t i = 0; i < n; ++i) {
            oss << g_pieces[i % g_piece_count];
        }
        auto result = oss.str();
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * n);
}

// ============================================================
// 4. string_builder append (no build)
// ============================================================

static void BM_StringBuilder_AppendOnly(benchmark::State& state) {
    const size_t n = state.range(0);
    for (auto _: state) {
        string_builder sb;
        sb.reserve_pieces(n);
        for (size_t i = 0; i < n; ++i) {
            sb.append(g_pieces[i % g_piece_count]);
        }
        benchmark::DoNotOptimize(sb);
    }
    state.SetItemsProcessed(state.iterations() * n);
}

// ============================================================
// 5. concatenate() variadic
// ============================================================

static void BM_Concatenate_Variadic(benchmark::State& state) {
    for (auto _: state) {
        auto result = concatenate(g_pieces[0], g_pieces[1], g_pieces[2], g_pieces[3], g_pieces[4], g_pieces[5],
                                  g_pieces[6], g_pieces[7]);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * 8);
}
BENCHMARK(BM_Concatenate_Variadic);

// ============================================================
// 6. Mixed type append (string + int + C-string + char)
// ============================================================

static void BM_StringBuilder_Mixed(benchmark::State& state) {
    const size_t n = state.range(0);
    for (auto _: state) {
        string_builder sb;
        sb.reserve_pieces(n * 3);
        for (size_t i = 0; i < n; ++i) {
            sb.append(g_pieces[i % g_piece_count]);
            sb.append(g_numbers[i % 8]);
            sb.append(' ');
        }
        auto result = sb.build();
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * n);
}

static void BM_OStringStream_Mixed(benchmark::State& state) {
    const size_t n = state.range(0);
    for (auto _: state) {
        std::ostringstream oss;
        for (size_t i = 0; i < n; ++i) {
            oss << g_pieces[i % g_piece_count];
            oss << g_numbers[i % 8];
            oss << ' ';
        }
        auto result = oss.str();
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * n);
}

// ============================================================
// 7. Large single string append (already-known-size optimization)
// ============================================================

static void BM_StringBuilder_Large(benchmark::State& state) {
    const size_t n = state.range(0);
    for (auto _: state) {
        string_builder sb;
        sb.reserve(n * 6); // approximate
        sb.reserve_pieces(n);
        for (size_t i = 0; i < n; ++i) {
            sb.append(g_pieces[i % g_piece_count]);
        }
        auto result = sb.build();
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * n);
}

static void BM_String_LargeReserved(benchmark::State& state) {
    const size_t n = state.range(0);
    for (auto _: state) {
        string result;
        result.reserve(n * 6);
        for (size_t i = 0; i < n; ++i) {
            result.append(g_pieces[i % g_piece_count]);
        }
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * n);
}

// ============================================================
// Registration
// ============================================================

#define SB_ARGS Arg(8)->Arg(128)->Arg(2048)->Arg(32768)

BENCHMARK(BM_StringBuilder_Build)->SB_ARGS->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_String_PlusEquals)->SB_ARGS->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_OStringStream)->SB_ARGS->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_StringBuilder_AppendOnly)->SB_ARGS->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_StringBuilder_Mixed)->Arg(4)->Arg(64)->Arg(1024)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_OStringStream_Mixed)->Arg(4)->Arg(64)->Arg(1024)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_StringBuilder_Large)->Arg(100)->Arg(1000)->Arg(10000)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_String_LargeReserved)->Arg(100)->Arg(1000)->Arg(10000)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
