#include <benchmark/benchmark.h>
#include <NeForce/tui/dom/element.hpp>
#include <NeForce/tui/dom/layout.hpp>
#include <NeForce/tui/renderer.hpp>
#include <NeForce/tui/screen.hpp>
using namespace neforce;
using namespace neforce::tui;

namespace {
    element make_tree(const int rows, const bool cjk = false) {
        elements lines;
        for (int i = 0; i < rows; ++i) {
            const string prefix = cjk ? "第 " + to_string(static_cast<int64_t>(i)) + " 行：" : "row ";
            lines.push_back(element::text(cjk ? prefix + "终端文本渲染与布局性能"
                                              : prefix + to_string(static_cast<int64_t>(i)) + " of the transcript",
                                          style{}, style::wrap_mode::none));
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

// Non-ASCII rows exercise the cached glyph run (decode + offsets are reused across frames).
static void BmRenderFrameCjk(benchmark::State& state) {
    const int width = static_cast<int>(state.range(0));
    const int height = 40;
    screen target(width, height);
    renderer draw(target, dark_theme, width, height);
    const element tree = make_tree(height, true);
    const vector<layout_rect> layout = compute_layout(tree, width, height);
    for (auto _: state) {
        draw.render(tree, layout, false);
    }
}
BENCHMARK(BmRenderFrameCjk)->Arg(80)->Arg(160)->Arg(240);

static void BmFrameDiffIdle(benchmark::State& state) {
    const int width = static_cast<int>(state.range(0));
    const int height = 40;
    screen frame(width, height);
    screen previous(width, height);
    renderer draw(frame, dark_theme, width, height);
    renderer draw_previous(previous, dark_theme, width, height);
    const element tree = make_tree(height);
    const vector<layout_rect> layout = compute_layout(tree, width, height);
    draw.render(tree, layout, false);
    draw_previous.render(tree, layout, false);
    for (auto _: state) {
        const string ansi = frame.to_string(previous);
        benchmark::DoNotOptimize(ansi.size());
    }
}

static void BmFrameDiffFull(benchmark::State& state) {
    const int width = static_cast<int>(state.range(0));
    const int height = 40;
    screen frame(width, height);
    screen blank(width, height);
    renderer draw(frame, dark_theme, width, height);
    const element tree = make_tree(height);
    const vector<layout_rect> layout = compute_layout(tree, width, height);
    draw.render(tree, layout, false);
    for (auto _: state) {
        const string ansi = frame.to_string(blank);
        benchmark::DoNotOptimize(ansi.size());
    }
}

BENCHMARK(BmFrameDiffIdle)->Arg(80)->Arg(160)->Arg(240);
BENCHMARK(BmFrameDiffFull)->Arg(80)->Arg(160)->Arg(240);

// Rich text: five styled spans per row (plain, bold, inline code, link, dim).
static void BmRenderFrameRich(benchmark::State& state) {
    const int width = static_cast<int>(state.range(0));
    const int height = 40;
    tui::style bold_style;
    bold_style.bold = true;
    tui::style code_style;
    code_style.bg = neforce::color{40, 44, 52};
    tui::style link_style;
    link_style.underline = true;
    tui::style dim_style;
    dim_style.dim = true;
    elements lines;
    for (int i = 0; i < height; ++i) {
        const string index = to_string(static_cast<int64_t>(i));
        vector<text_span> spans;
        spans.push_back(text_span{"row " + index + " ", bold_style, string()});
        spans.push_back(text_span{"of the ", tui::style{}, string()});
        spans.push_back(text_span{"transcript", code_style, string()});
        spans.push_back(text_span{" link", link_style, "https://example.com/" + index});
        spans.push_back(text_span{" dim", dim_style, string()});
        lines.push_back(element::rich_text(move(spans), tui::style{}, style::wrap_mode::none));
    }
    const element tree = element::vbox(move(lines));
    screen target(width, height);
    renderer draw(target, dark_theme, width, height);
    const vector<layout_rect> layout = compute_layout(tree, width, height);
    for (auto _: state) {
        draw.render(tree, layout, false);
    }
}
BENCHMARK(BmRenderFrameRich)->Arg(80)->Arg(160)->Arg(240);

// One row changes: the typical incremental frame of a transcript.
static void BmFrameDiffSingleRow(benchmark::State& state) {
    const int width = static_cast<int>(state.range(0));
    const int height = 40;
    screen frame(width, height);
    screen previous(width, height);
    renderer draw(frame, dark_theme, width, height);
    renderer draw_previous(previous, dark_theme, width, height);
    const element tree = make_tree(height);
    const vector<layout_rect> layout = compute_layout(tree, width, height);
    draw.render(tree, layout, false);
    draw_previous.render(tree, layout, false);
    // Touch one row of the new frame so exactly one row differs.
    frame.fast_cell_at(0, height / 2).character = "@";
    for (auto _: state) {
        const string ansi = frame.to_string(previous);
        benchmark::DoNotOptimize(ansi.size());
    }
}
BENCHMARK(BmFrameDiffSingleRow)->Arg(80)->Arg(160)->Arg(240);

// The whole transcript scrolls up by one row: every row changes, but only by one line.
static void BmFrameDiffScroll(benchmark::State& state) {
    const int width = static_cast<int>(state.range(0));
    const int height = 40;
    screen frame(width, height);
    screen previous(width, height);
    renderer draw(frame, dark_theme, width, height);
    renderer draw_previous(previous, dark_theme, width, height);
    const element tree = make_tree(height);
    const vector<layout_rect> layout = compute_layout(tree, width, height);
    draw.render(tree, layout, false);
    draw_previous.render(tree, layout, false);
    for (int y = 0; y + 1 < height; ++y) {
        for (int x = 0; x < width; ++x) {
            frame.fast_cell_at(x, y) = previous.fast_cell_at(x, y + 1);
        }
    }
    for (auto _: state) {
        const string ansi = frame.to_string(previous);
        benchmark::DoNotOptimize(ansi.size());
    }
}
BENCHMARK(BmFrameDiffScroll)->Arg(80)->Arg(160)->Arg(240);

BENCHMARK_MAIN();
