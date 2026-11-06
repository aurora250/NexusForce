#include <benchmark/benchmark.h>
#include <NeForce/core/string/format.hpp>
#include <iterator>
#include <string>
using namespace neforce;

#if defined(NEFORCE_STANDARD_20)
#    include <format>
#    if defined(__cpp_lib_format)
#        define NEFORCE_BENCH_STD_FORMAT 1
#    endif
#endif

namespace {

    const int g_line = 128;
    const char* g_file = "server.cpp";
    const char* g_user = "alice";
    const unsigned g_mask = 0xDEADBEEFU;
    const double g_pi = 3.141592653589793;

} // namespace

// ============================================================
// 1. Integer
// ============================================================

static void BM_NfFormat_IntegerDefault(benchmark::State& state) {
    for (auto _: state) {
        auto out = format("{}", 42);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK(BM_NfFormat_IntegerDefault)->Unit(benchmark::kNanosecond);

static void BM_NfFormat_IntegerPadded(benchmark::State& state) {
    for (auto _: state) {
        auto out = format("{:06d}", 1234);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK(BM_NfFormat_IntegerPadded)->Unit(benchmark::kNanosecond);

static void BM_NfFormat_IntegerHex(benchmark::State& state) {
    for (auto _: state) {
        auto out = format("{:#x}", g_mask);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK(BM_NfFormat_IntegerHex)->Unit(benchmark::kNanosecond);

// ============================================================
// 2. Floating point
// ============================================================

static void BM_NfFormat_DoubleFixed(benchmark::State& state) {
    for (auto _: state) {
        auto out = format("{:.3f}", g_pi);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK(BM_NfFormat_DoubleFixed)->Unit(benchmark::kNanosecond);

static void BM_NfFormat_DoubleScientific(benchmark::State& state) {
    for (auto _: state) {
        auto out = format("{:e}", 1234.5);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK(BM_NfFormat_DoubleScientific)->Unit(benchmark::kNanosecond);

static void BM_NfFormat_DoubleRightAligned(benchmark::State& state) {
    for (auto _: state) {
        auto out = format("{:>12.4f}", g_pi);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK(BM_NfFormat_DoubleRightAligned)->Unit(benchmark::kNanosecond);

// ============================================================
// 3. Strings and escaped braces
// ============================================================

static void BM_NfFormat_CStringArg(benchmark::State& state) {
    for (auto _: state) {
        auto out = format("user={}", g_user);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK(BM_NfFormat_CStringArg)->Unit(benchmark::kNanosecond);

static void BM_NfFormat_EscapedBraces(benchmark::State& state) {
    for (auto _: state) {
        auto out = format("{{literal}} {}", 42);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK(BM_NfFormat_EscapedBraces)->Unit(benchmark::kNanosecond);

// ============================================================
// 4. Multi-argument templates
// ============================================================

static void BM_NfFormat_MultiMixed(benchmark::State& state) {
    for (auto _: state) {
        auto out = format("{} {:.1f} {} {}", 7, 2.5, g_user, true);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK(BM_NfFormat_MultiMixed)->Unit(benchmark::kNanosecond);

static void BM_NfFormat_LogLine(benchmark::State& state) {
    for (auto _: state) {
        auto out = format("[{}] {}:{} {:.1f}ms", 3, g_file, g_line, 12.5);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK(BM_NfFormat_LogLine)->Unit(benchmark::kNanosecond);

// ============================================================
// 5. Format into a reused buffer
// ============================================================

static void BM_NfFormat_AppendReuse(benchmark::State& state) {
    string buffer;
    buffer.reserve(64);
    for (auto _: state) {
        buffer.clear();
        format_to(buffer, "[{}] {}:{} {:.1f}ms", 3, g_file, g_line, 12.5);
        benchmark::DoNotOptimize(buffer);
    }
}
BENCHMARK(BM_NfFormat_AppendReuse)->Unit(benchmark::kNanosecond);

// ============================================================
// 6. Standard library reference (C++20 only)
// ============================================================

#ifdef NEFORCE_BENCH_STD_FORMAT
static void BM_StdFormat_IntegerDefault(benchmark::State& state) {
    for (auto _: state) {
        auto out = std::format("{}", 42);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK(BM_StdFormat_IntegerDefault)->Unit(benchmark::kNanosecond);

static void BM_StdFormat_IntegerPadded(benchmark::State& state) {
    for (auto _: state) {
        auto out = std::format("{:06d}", 1234);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK(BM_StdFormat_IntegerPadded)->Unit(benchmark::kNanosecond);

static void BM_StdFormat_IntegerHex(benchmark::State& state) {
    for (auto _: state) {
        auto out = std::format("{:#x}", g_mask);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK(BM_StdFormat_IntegerHex)->Unit(benchmark::kNanosecond);

static void BM_StdFormat_DoubleFixed(benchmark::State& state) {
    for (auto _: state) {
        auto out = std::format("{:.3f}", g_pi);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK(BM_StdFormat_DoubleFixed)->Unit(benchmark::kNanosecond);

static void BM_StdFormat_DoubleScientific(benchmark::State& state) {
    for (auto _: state) {
        auto out = std::format("{:e}", 1234.5);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK(BM_StdFormat_DoubleScientific)->Unit(benchmark::kNanosecond);

static void BM_StdFormat_DoubleRightAligned(benchmark::State& state) {
    for (auto _: state) {
        auto out = std::format("{:>12.4f}", g_pi);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK(BM_StdFormat_DoubleRightAligned)->Unit(benchmark::kNanosecond);

static void BM_StdFormat_CStringArg(benchmark::State& state) {
    for (auto _: state) {
        auto out = std::format("user={}", g_user);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK(BM_StdFormat_CStringArg)->Unit(benchmark::kNanosecond);

static void BM_StdFormat_EscapedBraces(benchmark::State& state) {
    for (auto _: state) {
        auto out = std::format("{{literal}} {}", 42);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK(BM_StdFormat_EscapedBraces)->Unit(benchmark::kNanosecond);

static void BM_StdFormat_MultiMixed(benchmark::State& state) {
    for (auto _: state) {
        auto out = std::format("{} {:.1f} {} {}", 7, 2.5, g_user, true);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK(BM_StdFormat_MultiMixed)->Unit(benchmark::kNanosecond);

static void BM_StdFormat_LogLine(benchmark::State& state) {
    for (auto _: state) {
        auto out = std::format("[{}] {}:{} {:.1f}ms", 3, g_file, g_line, 12.5);
        benchmark::DoNotOptimize(out);
    }
}
BENCHMARK(BM_StdFormat_LogLine)->Unit(benchmark::kNanosecond);

static void BM_StdFormat_AppendReuse(benchmark::State& state) {
    std::string buffer;
    buffer.reserve(64);
    for (auto _: state) {
        buffer.clear();
        std::format_to(std::back_inserter(buffer), "[{}] {}:{} {:.1f}ms", 3, g_file, g_line, 12.5);
        benchmark::DoNotOptimize(buffer);
    }
}
BENCHMARK(BM_StdFormat_AppendReuse)->Unit(benchmark::kNanosecond);
#endif

BENCHMARK_MAIN();
