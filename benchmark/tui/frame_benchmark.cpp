#include <benchmark/benchmark.h>
#include <NeForce/tui/dom/element.hpp>
#include <NeForce/tui/dom/layout.hpp>
#include <NeForce/tui/renderer.hpp>
#include <NeForce/tui/screen.hpp>
using namespace neforce;
using namespace neforce::tui;

namespace {
    element make_tree(const int rows) {
        elements lines;
        for (int i = 0; i < rows; ++i) {
            lines.push_back(element::text("row " + to_string(static_cast<int64_t>(i)) + " of the transcript", style{},
                                          style::wrap_mode::none));
        }
        return element::vbox(move(lines));
    }
} // namespace

static void BmRenderFrame(benchmark::State& state) {
    const int width = static_cast<int>(state.range(0));
    const int height = 40;
    screen target(width, height);
    renderer draw(target, dark_theme, width, height);
    const element tree = make_tree(height);
    const vector<layout_rect> layout = compute_layout(tree, width, height);
    for (auto _: state) {
        draw.render(tree, layout, false);
    }
}

BENCHMARK(BmRenderFrame)->Arg(80)->Arg(160)->Arg(240);

BENCHMARK_MAIN();
