#include <benchmark/benchmark.h>
#include <NeForce/tui/dom/element.hpp>
#include <NeForce/tui/dom/layout.hpp>
#include <NeForce/core/string/format.hpp>
#include <NeForce/tui/dom/table.hpp>
#include <NeForce/tui/renderer.hpp>
#include <NeForce/tui/screen.hpp>
using namespace neforce;
using namespace neforce::tui;

namespace {
    element make_dashboard(const int rows) {
        vector<vector<element>> cells;
        cells.reserve(static_cast<size_t>(rows));
        for (int row = 0; row < rows; ++row) {
            cells.push_back({
                    element::text(format("{}", static_cast<uint64_t>(1000 + row))),
                    element::text("worker"),
                    element::text(format("{:.1f}", static_cast<double>(row % 100) * 0.7)),
                    element::text("12.3 MiB"),
                    element::text("/usr/bin/worker --serve --threads 4"),
            });
        }
        table grid(move(cells));
        grid.column_widths({7, 12, 8, 10, 0});
        grid.column_aligns(
                {style::align::end, style::align::start, style::align::end, style::align::end, style::align::start});
        tui::style panel_style;
        panel_style.border = style::border::rounded;
        panel_style.bg = neforce::color{24, 26, 32};
        panel_style.fg = neforce::color{120, 220, 180};
        return element::panel(grid.render(), "PROCESSES · benchmark", panel_style);
    }
} // namespace

static void BmRenderPanelTable(benchmark::State& state) {
    const int width = static_cast<int>(state.range(0));
    const int height = 48;
    screen target(width, height);
    renderer draw(target, dark_theme, width, height);
    const element tree = make_dashboard(height - 4);
    const vector<layout_rect> layout = compute_layout(tree, width, height);
    for (auto _: state) {
        draw.render(tree, layout, false);
    }
}
BENCHMARK(BmRenderPanelTable)->Arg(120)->Arg(160)->Arg(240);

static void BmLayoutPanelTable(benchmark::State& state) {
    const int width = static_cast<int>(state.range(0));
    const int height = 48;
    const element tree = make_dashboard(height - 4);
    for (auto _: state) {
        benchmark::DoNotOptimize(compute_layout(tree, width, height));
    }
}
BENCHMARK(BmLayoutPanelTable)->Arg(120)->Arg(160)->Arg(240);

// Cold layout: the tree is marked dirty every iteration, so the engine walks the whole
// subtree instead of returning the cached rectangles.
static void BmLayoutPanelTableCold(benchmark::State& state) {
    const int width = static_cast<int>(state.range(0));
    const int height = 48;
    element tree = make_dashboard(height - 4);
    for (auto _: state) {
        tree.set_layout_dirty(true);
        benchmark::DoNotOptimize(compute_layout(tree, width, height));
    }
}
BENCHMARK(BmLayoutPanelTableCold)->Arg(120)->Arg(160)->Arg(240);

BENCHMARK_MAIN();
