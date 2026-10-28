#include <benchmark/benchmark.h>
#include <NeForce/core/string/format.hpp>
#include <NeForce/tui/dom/element.hpp>
#include <NeForce/tui/dom/layout.hpp>
#include <NeForce/tui/headless.hpp>
#include <NeForce/tui/renderer.hpp>
#include <NeForce/tui/screen.hpp>
using namespace neforce;
using namespace neforce::tui;

namespace {
    element make_document(const size_t rows, const int width) {
        elements lines;
        lines.reserve(rows);
        for (size_t i = 0; i < rows; ++i) {
            lines.push_back(element::text(format("line {} of a long transcript that scrolls", i), tui::style{},
                                          style::wrap_mode::none));
        }
        return element::vbox(move(lines), box_props{});
    }

    // Renders the tree, then scrolls the viewport one row per iteration and repaints.
    void scroll_frame(benchmark::State& state, const element& tree, const int width, const int height) {
        screen target(width, height);
        screen previous(width, height);
        renderer draw(target, dark_theme, width, height);
        size_t offset = 0;
        for (auto _: state) {
            const vector<layout_rect> layout = compute_layout(tree, width, height);
            draw.render(tree, layout, false);
            const string ansi = target.to_string(previous);
            benchmark::DoNotOptimize(ansi.size());
            previous = target;
            previous.clear();
            benchmark::DoNotOptimize(offset);
            ++offset;
        }
    }
} // namespace

// scroll_view over a long document: layout + render + diff of one scrolled frame.
static void BmScrollViewFrame(benchmark::State& state) {
    const int width = static_cast<int>(state.range(0));
    const int height = 40;
    element inner = make_document(500, width);
    element tree = element::scroll_view(move(inner), tui::style{}, 0, 0);
    screen target(width, height);
    screen previous(width, height);
    renderer draw(target, dark_theme, width, height);
    // Prime both buffers so the measured frame is a normal incremental one.
    const vector<layout_rect> layout = compute_layout(tree, width, height);
    draw.render(tree, layout, false);
    previous = target;
    for (auto _: state) {
        draw.render(tree, layout, false);
        const string ansi = target.to_string(previous);
        benchmark::DoNotOptimize(ansi.size());
    }
}
BENCHMARK(BmScrollViewFrame)->Arg(120)->Arg(240);

// virtual_list: the argument is the item count, so the scaling with the data set shows up.
static void BmVirtualListFrame(benchmark::State& state) {
    const int width = 120;
    const int height = 40;
    const auto items = static_cast<size_t>(state.range(0));
    element tree = element::virtual_list(
            items, 1,
            [](const size_t index) { return element::text(format("item {}", index), tui::style{}, style::wrap_mode::none); },
            0);
    screen target(width, height);
    renderer draw(target, dark_theme, width, height);
    for (auto _: state) {
        const vector<layout_rect> layout = compute_layout(tree, width, height);
        draw.render(tree, layout, false);
        benchmark::DoNotOptimize(layout.size());
    }
}
BENCHMARK(BmVirtualListFrame)->Arg(1000)->Arg(100000)->Arg(1000000);

namespace {
    class transcript_component final : public component_base {
    public:
        explicit transcript_component(const size_t rows) :
        rows_(rows) {}

        element render() override {
            elements lines;
            lines.reserve(40);
            for (size_t i = 0; i < rows_; ++i) {
                lines.push_back(element::text(format("message {} from the transcript", i), tui::style{},
                                              style::wrap_mode::none));
            }
            return element::vbox(move(lines));
        }

    private:
        size_t rows_;
    };
} // namespace

// Full pipeline through the reconciler: component render -> diff -> layout -> draw -> ANSI.
static void BmHeadlessFrame(benchmark::State& state) {
    const int width = 120;
    const auto rows = static_cast<size_t>(state.range(0));
    const int height = 40;
    transcript_component root(rows);
    headless app(&root, width, height);
    app.render();
    for (auto _: state) {
        app.render();
    }
}
BENCHMARK(BmHeadlessFrame)->Arg(40)->Arg(400);

BENCHMARK_MAIN();
