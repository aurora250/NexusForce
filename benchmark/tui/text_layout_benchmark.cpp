#include <benchmark/benchmark.h>
#include <NeForce/tui/dom/text_layout.hpp>
using namespace neforce;
using namespace neforce::tui;

namespace {
    string make_paragraph(const size_t words) {
        string text;
        for (size_t i = 0; i < words; ++i) {
            text += "word ";
        }
        return text;
    }
} // namespace

static void BmDisplayWidthAscii(benchmark::State& state) {
    const string text = make_paragraph(static_cast<size_t>(state.range(0)));
    for (auto _: state) {
        benchmark::DoNotOptimize(text_display_width(text.view()));
    }
}

static void BmDisplayWidthCjk(benchmark::State& state) {
    string text;
    for (int i = 0; i < state.range(0); ++i) {
        text += "\xe4\xbd\xa0\xe5\xa5\xbd";
    }
    for (auto _: state) {
        benchmark::DoNotOptimize(text_display_width(text.view()));
    }
}

static void BmLayoutWordWrap(benchmark::State& state) {
    const string text = make_paragraph(static_cast<size_t>(state.range(0)));
    for (auto _: state) {
        benchmark::DoNotOptimize(layout_text(text.view(), 80, style::wrap_mode::word));
    }
}

BENCHMARK(BmDisplayWidthAscii)->Range(8, 512);
BENCHMARK(BmDisplayWidthCjk)->Range(8, 512);
BENCHMARK(BmLayoutWordWrap)->Range(8, 512);

BENCHMARK_MAIN();
