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

namespace {
    // Marks every node, which is what a freshly built tree looks like: nothing can be
    // reused and the whole tree has to be measured and assigned.
    void mark_all_dirty(element& el) {
        el.set_layout_dirty(true);
        for (auto& child: el.children()) {
            mark_all_dirty(child);
        }
    }
} // namespace

// Cold layout: the whole tree is marked dirty every iteration.
static void BmLayoutPanelTableCold(benchmark::State& state) {
    const int width = static_cast<int>(state.range(0));
    const int height = 48;
    element tree = make_dashboard(height - 4);
    for (auto _: state) {
        mark_all_dirty(tree);
        benchmark::DoNotOptimize(compute_layout(tree, width, height));
    }
}
BENCHMARK(BmLayoutPanelTableCold)->Arg(120)->Arg(160)->Arg(240);

// Dirty subtree: the tree stays alive, one cell changes, everything else is clean. Only
// the path down to that cell should be measured and assigned.
static void BmLayoutPanelTableDirty(benchmark::State& state) {
    const int width = static_cast<int>(state.range(0));
    const int height = 48;
    element tree = make_dashboard(height - 4);
    benchmark::DoNotOptimize(compute_layout(tree, width, height));
    for (auto _: state) {
        auto& rows = tree.children();
        if (rows.empty() || rows[0].children().empty()) {
            break;
        }
        rows[0].children()[0].set_layout_dirty(true);
        benchmark::DoNotOptimize(compute_layout(tree, width, height));
    }
}
BENCHMARK(BmLayoutPanelTableDirty)->Arg(120)->Arg(160)->Arg(240);

// Only the root is marked dirty (e.g. a restyle that keeps the size): children keep their
// measurements and fragments, which is what incremental layout is for.
static void BmLayoutPanelTableRootDirty(benchmark::State& state) {
    const int width = static_cast<int>(state.range(0));
    const int height = 48;
    element tree = make_dashboard(height - 4);
    benchmark::DoNotOptimize(compute_layout(tree, width, height));
    for (auto _: state) {
        tree.set_layout_dirty(true);
        benchmark::DoNotOptimize(compute_layout(tree, width, height));
    }
}
BENCHMARK(BmLayoutPanelTableRootDirty)->Arg(120)->Arg(160)->Arg(240);

// Table layout scaling: the same width with 44 and 200 rows.
static void BmLayoutPanelTableRows(benchmark::State& state) {
    const int width = 160;
    const int rows = static_cast<int>(state.range(0));
    element tree = make_dashboard(rows);
    for (auto _: state) {
        mark_all_dirty(tree);
        benchmark::DoNotOptimize(compute_layout(tree, width, rows + 4));
    }
}
BENCHMARK(BmLayoutPanelTableRows)->Arg(44)->Arg(200)->Arg(800);

BENCHMARK_MAIN();
