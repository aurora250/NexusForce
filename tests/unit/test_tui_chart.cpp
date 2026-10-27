#include <NeForce/core/string/utf_iterator.hpp>
#include <NeForce/core/string/width.hpp>
#include <NeForce/tui/component/component.hpp>
#include <NeForce/tui/chart/chart.hpp>
#include <NeForce/tui/dom/layout.hpp>
#include <NeForce/tui/dom/theme.hpp>
#include <NeForce/tui/headless.hpp>
#include <NeForce/tui/renderer.hpp>
#include <NeForce/tui/screen.hpp>
#include <gtest/gtest.h>
using namespace neforce;
using namespace neforce::tui;

namespace {
    theme test_theme() { return make_palette(theme_palette_id::dark); }

    screen render_screen(const element& tree, const int w, const int h, const theme& t = dark_theme) {
        screen scr(w, h);
        scr.clear();
        renderer r(scr, t, w, h);
        const auto layout = compute_layout(tree, w, h);
        r.render(tree, layout, false);
        return scr;
    }

    string render_plain(const element& tree, const int w, const int h, const theme& t = dark_theme) {
        return render_screen(tree, w, h, t).to_plaintext();
    }

    size_t count_text(const string& text, const char* needle) {
        const string pattern(needle);
        size_t count = 0;
        size_t offset = 0;
        while ((offset = text.find(pattern, offset)) != string::npos) {
            ++count;
            offset += pattern.size();
        }
        return count;
    }

    size_t braille_count(const string& text) {
        size_t count = 0;
        for (size_t i = 0; i + 2 < text.size(); ++i) {
            if (static_cast<unsigned char>(text[i]) == 0xE2) {
                const unsigned char second = static_cast<unsigned char>(text[i + 1]);
                if (second >= 0xA0 && second <= 0xA3) {
                    ++count;
                }
            }
        }
        return count;
    }

    bool has_braille(const string& text) { return braille_count(text) > 0; }

    bool same_color(const neforce::color& left, const neforce::color& right) {
        return left.R() == right.R() && left.G() == right.G() && left.B() == right.B() && left.A() == right.A();
    }

    bool has_text_with_color(const screen& scr, const char* glyph, const neforce::color& c) {
        for (int y = 0; y < scr.dimy(); ++y) {
            for (int x = 0; x < scr.dimx(); ++x) {
                const auto& cell = scr.fast_cell_at(x, y);
                if (cell.character == glyph && same_color(cell.foreground, c)) {
                    return true;
                }
            }
        }
        return false;
    }

    bool has_block_with_color(const screen& scr, const neforce::color& c) {
        for (int y = 0; y < scr.dimy(); ++y) {
            for (int x = 0; x < scr.dimx(); ++x) {
                const auto& cell = scr.fast_cell_at(x, y);
                if (cell.character == "\xe2\x96\x88" && same_color(cell.foreground, c)) {
                    return true;
                }
            }
        }
        return false;
    }

    size_t block_count(const screen& scr) {
        size_t count = 0;
        for (int y = 0; y < scr.dimy(); ++y) {
            for (int x = 0; x < scr.dimx(); ++x) {
                if (scr.fast_cell_at(x, y).character == "\xe2\x96\x88") {
                    ++count;
                }
            }
        }
        return count;
    }

    chart_series series_of(const char* name, vector<double> values) {
        chart_series series;
        series.name = name;
        series.values = move(values);
        return series;
    }

    chart_spec line_spec() {
        chart_spec spec;
        spec.kind = chart_kind::line;
        spec.title = "latency";
        spec.labels = {"A", "B", "C", "D"};
        spec.series.push_back(series_of("alpha", {1, 3, 2, 5}));
        spec.series.push_back(series_of("beta", {2, 1, 4, 3}));
        spec.width = 80;
        spec.height = 20;
        return spec;
    }

    chart_spec bar_spec() {
        chart_spec spec;
        spec.kind = chart_kind::bar;
        spec.title = "requests";
        spec.labels = {"A", "B", "C", "D"};
        spec.series.push_back(series_of("alpha", {1, 3, 2, 5}));
        spec.series.push_back(series_of("beta", {2, 1, 4, 3}));
        spec.width = 80;
        spec.height = 20;
        return spec;
    }

    chart_spec pie_spec(const chart_kind kind = chart_kind::pie) {
        chart_spec spec;
        spec.kind = kind;
        spec.title = "share";
        spec.series.push_back(series_of("A", {1}));
        spec.series.push_back(series_of("B", {2}));
        spec.series.push_back(series_of("C", {1}));
        spec.width = 80;
        spec.height = 20;
        return spec;
    }

    chart_spec heatmap_spec() {
        chart_spec spec;
        spec.kind = chart_kind::heatmap;
        spec.title = "load";
        spec.labels = {"x", "y", "z", "w"};
        spec.series.push_back(series_of("row1", {1, 2, 3, 4}));
        spec.series.push_back(series_of("row2", {4, 3, 2, 1}));
        spec.series.push_back(series_of("row3", {2, 4, 1, 3}));
        spec.width = 80;
        spec.height = 20;
        return spec;
    }

    chart_spec radar_spec() {
        chart_spec spec;
        spec.kind = chart_kind::radar;
        spec.title = "radar";
        spec.labels = {"N", "E", "S", "W", "Z"};
        spec.series.push_back(series_of("alpha", {5, 3, 4, 2, 4}));
        spec.series.push_back(series_of("beta", {3, 4, 2, 5, 3}));
        spec.width = 80;
        spec.height = 20;
        return spec;
    }

    vector<string> split_lines(const string& text) {
        vector<string> lines;
        string current;
        for (const char c: text) {
            if (c == '\n') {
                lines.push_back(move(current));
                current = string();
            } else {
                current.push_back(c);
            }
        }
        lines.push_back(move(current));
        return lines;
    }

    int widest_line(const string& text) {
        int width = 0;
        for (auto& line: split_lines(text)) {
            width = max(width, text_display_width(line.view()));
        }
        return width;
    }

    vector<uint32_t> braille_codepoints(const string& text) {
        vector<uint32_t> result;
        for (utf8_iterator it(reinterpret_cast<const byte_t*>(text.data()), text.size()); it != utf8_iterator(); ++it) {
            const uint32_t value = it->value();
            if (value >= 0x2800U && value <= 0x28FFU) {
                result.push_back(value);
            }
        }
        return result;
    }

    size_t eighth_block_count(const string& text) {
        const char* const glyphs[] = {"\xe2\x96\x81", "\xe2\x96\x82", "\xe2\x96\x83", "\xe2\x96\x85",
                                      "\xe2\x96\x86", "\xe2\x96\x87", "\xe2\x96\x94"};
        size_t count = 0;
        for (const char* glyph: glyphs) {
            count += count_text(text, glyph);
        }
        return count;
    }

    size_t chart_ink_count(const string& text) {
        size_t count = count_text(text, "\xe2\x96\x88") + eighth_block_count(text);
        count += count_text(text, "\xe2\x96\x80");
        count += count_text(text, "\xe2\x96\x84");
        return count + braille_count(text);
    }

    string sgr_mouse_press(const int x, const int y) {
        string sequence("\x1b[<0;");
        sequence += to_string(x + 1);
        sequence += ";";
        sequence += to_string(y + 1);
        sequence += "M";
        return sequence;
    }

    struct chart_host : component<> {
        chart_component* view = nullptr;

        void attach(chart_spec spec) {
            auto child = make_unique<chart_component>(move(spec));
            view = child.get();
            add_child(move(child));
        }

        element render() override { return view != nullptr ? view->render() : element::empty(); }
    };

    chart_spec spec_for(const chart_kind kind) {
        switch (kind) {
            case chart_kind::bar:
                return bar_spec();
            case chart_kind::line: {
                auto spec = line_spec();
                return spec;
            }
            case chart_kind::area: {
                auto spec = line_spec();
                spec.kind = chart_kind::area;
                return spec;
            }
            case chart_kind::pie:
            case chart_kind::donut:
                return pie_spec(kind);
            case chart_kind::scatter: {
                auto spec = line_spec();
                spec.kind = chart_kind::scatter;
                return spec;
            }
            case chart_kind::histogram: {
                auto spec = bar_spec();
                spec.kind = chart_kind::histogram;
                return spec;
            }
            case chart_kind::heatmap:
                return heatmap_spec();
            case chart_kind::radar:
                return radar_spec();
        }
        return line_spec();
    }
} // namespace

TEST(TuiChartPainterTest, DrawsLocalCellsWithStylesAndBounds) {
    style canvas_style;
    canvas_style.width = style::size_hint{style::size_hint::fixed, 8};
    canvas_style.height = style::size_hint{style::size_hint::fixed, 3};
    canvas_style.fg = neforce::color::blue();
    const theme palette = test_theme();
    const theme* seen_theme = nullptr;
    bool inside = false;
    string bounds;

    element tree = element::canvas(
            [&](canvas_painter& painter) {
                seen_theme = &painter.theme_ref();
                inside = painter.contains(0, 0) && !painter.contains(8, 0) && !painter.contains(0, 3) &&
                         !painter.contains(-1, 0);
                bounds = to_string(painter.width()) + "x" + to_string(painter.height());
                painter.put(0, 0, "A");
                painter.put(2, 0, "\xe2\x96\x88", [] {
                    style s;
                    s.fg = neforce::color::red();
                    return s;
                }());
                painter.text(0, 1, "\xe4\xb8\xadX");
                painter.fill(6, 1, 4, 4, ".");
                painter.hline(0, 2, 3, "-");
                painter.vline(5, 0, 2, "|");
            },
            canvas_style);

    const screen scr = render_screen(tree, 8, 3, palette);
    EXPECT_EQ(seen_theme, &palette);
    EXPECT_TRUE(inside);
    EXPECT_EQ(bounds, "8x3");
    EXPECT_EQ(scr.fast_cell_at(0, 0).character, "A");
    EXPECT_TRUE(same_color(scr.fast_cell_at(0, 0).foreground, neforce::color::blue()));
    EXPECT_EQ(scr.fast_cell_at(2, 0).character, "\xe2\x96\x88");
    EXPECT_TRUE(same_color(scr.fast_cell_at(2, 0).foreground, neforce::color::red()));
    EXPECT_EQ(scr.fast_cell_at(0, 1).character, "\xe4\xb8\xad");
    EXPECT_TRUE(scr.fast_cell_at(1, 1).automerge);
    EXPECT_EQ(scr.fast_cell_at(2, 1).character, "X");
    EXPECT_EQ(scr.fast_cell_at(6, 1).character, ".");
    EXPECT_EQ(scr.fast_cell_at(2, 2).character, "-");
    EXPECT_EQ(scr.fast_cell_at(5, 0).character, "|");
}

TEST(TuiChartTest, EveryKindRendersContentDeterministically) {
    for (const chart_kind kind:
         {chart_kind::bar, chart_kind::line, chart_kind::area, chart_kind::pie, chart_kind::donut, chart_kind::scatter,
          chart_kind::histogram, chart_kind::heatmap, chart_kind::radar}) {
        const chart_spec spec = spec_for(kind);
        const string first = render_plain(chart(spec), spec.width, spec.height, test_theme());
        const string second = render_plain(chart(spec), spec.width, spec.height, test_theme());
        EXPECT_EQ(first, second) << "kind " << static_cast<int>(kind);
        EXPECT_FALSE(first.empty()) << "kind " << static_cast<int>(kind);
        EXPECT_TRUE(first.contains("latency") || first.contains("requests") || first.contains("share") ||
                    first.contains("load") || first.contains("radar"))
                << "kind " << static_cast<int>(kind);
        if (kind == chart_kind::line || kind == chart_kind::scatter || kind == chart_kind::radar) {
            EXPECT_TRUE(has_braille(first)) << "kind " << static_cast<int>(kind);
        } else {
            EXPECT_GT(block_count(render_screen(chart(spec), spec.width, spec.height, test_theme())), 0U)
                    << "kind " << static_cast<int>(kind);
        }
    }
}

TEST(TuiChartTest, LegendsShowSwatchesAndSeriesNames) {
    chart_spec right = bar_spec();
    const string right_text = render_plain(chart(right), right.width, right.height, test_theme());
    EXPECT_TRUE(right_text.contains("\xe2\x96\xa0"));
    EXPECT_TRUE(right_text.contains("alpha"));
    EXPECT_TRUE(right_text.contains("beta"));

    chart_spec bottom = bar_spec();
    bottom.legend.position = chart_legend_position::bottom;
    const string bottom_text = render_plain(chart(bottom), bottom.width, bottom.height, test_theme());
    EXPECT_TRUE(bottom_text.contains("\xe2\x96\xa0"));
    EXPECT_TRUE(bottom_text.contains("alpha"));
    EXPECT_TRUE(bottom_text.contains("beta"));

    chart_spec hidden = bar_spec();
    hidden.legend.show = false;
    const string hidden_text = render_plain(chart(hidden), hidden.width, hidden.height, test_theme());
    EXPECT_FALSE(hidden_text.contains("alpha"));
    EXPECT_FALSE(hidden_text.contains("beta"));
}

TEST(TuiChartTest, PieEdgesUseSubcellQuadrants) {
    const chart_spec spec = pie_spec();
    const screen scr = render_screen(chart(spec), spec.width, spec.height, test_theme());
    const char* edge_glyphs[] = {"\xe2\x96\x98", "\xe2\x96\x9d", "\xe2\x96\x96", "\xe2\x96\x97", "\xe2\x96\x9a",
                                 "\xe2\x96\x9e", "\xe2\x96\x99", "\xe2\x96\x9f", "\xe2\x96\x9b", "\xe2\x96\x9c",
                                 "\xe2\x96\x80", "\xe2\x96\x84", "\xe2\x96\x8c", "\xe2\x96\x90"};
    size_t edge_cells = 0;
    for (int y = 0; y < scr.dimy(); ++y) {
        for (int x = 0; x < scr.dimx(); ++x) {
            const string& glyph = scr.fast_cell_at(x, y).character;
            for (const char* edge: edge_glyphs) {
                if (glyph == edge) {
                    ++edge_cells;
                    break;
                }
            }
        }
    }
    EXPECT_GT(edge_cells, 12u);
}

TEST(TuiChartTest, PieAndDonutLegendShowPercentages) {
    for (const chart_kind kind: {chart_kind::pie, chart_kind::donut}) {
        const chart_spec spec = pie_spec(kind);
        const string text = render_plain(chart(spec), spec.width, spec.height, test_theme());
        EXPECT_TRUE(text.contains("%")) << static_cast<int>(kind);
        EXPECT_TRUE(text.contains("50.0%")) << static_cast<int>(kind);
        EXPECT_TRUE(text.contains("25.0%")) << static_cast<int>(kind);
        EXPECT_TRUE(text.contains("A"));
        EXPECT_TRUE(text.contains("B"));
        EXPECT_TRUE(text.contains("C"));
    }
    const string pie_text = render_plain(chart(pie_spec(chart_kind::pie)), 80, 20, test_theme());
    const string donut_text = render_plain(chart(pie_spec(chart_kind::donut)), 80, 20, test_theme());
    EXPECT_NE(pie_text, donut_text);

    chart_spec named = pie_spec();
    named.labels.clear();
    named.labels.push_back("One");
    named.labels.push_back("Two");
    named.labels.push_back("Three");
    const string named_text = render_plain(chart(named), 80, 20, test_theme());
    EXPECT_TRUE(named_text.contains("One"));
    EXPECT_TRUE(named_text.contains("Two"));
    EXPECT_TRUE(named_text.contains("Three"));
}

TEST(TuiChartTest, EmptyAndTinyChartsDoNotCrash) {
    for (const chart_kind kind:
         {chart_kind::bar, chart_kind::line, chart_kind::area, chart_kind::pie, chart_kind::donut, chart_kind::scatter,
          chart_kind::histogram, chart_kind::heatmap, chart_kind::radar}) {
        chart_spec spec;
        spec.kind = kind;
        spec.width = 40;
        spec.height = 12;
        spec.labels = {"A", "B"};
        const string empty = render_plain(chart(spec), 40, 12, test_theme());
        EXPECT_FALSE(empty.empty()) << static_cast<int>(kind);

        spec.width = 1;
        spec.height = 1;
        EXPECT_NO_THROW(render_plain(chart(spec), 1, 1, test_theme())) << static_cast<int>(kind);
        spec.width = 3;
        spec.height = 2;
        EXPECT_NO_THROW(render_plain(chart(spec), 3, 2, test_theme())) << static_cast<int>(kind);
        spec.width = 12;
        spec.height = 6;
        spec.series.push_back(series_of("nan", {numeric_traits<double>::quiet_nan(), 2.0}));
        EXPECT_NO_THROW(render_plain(chart(spec), 12, 6, test_theme())) << static_cast<int>(kind);
    }
}

TEST(TuiChartTest, NegativeAndNonFiniteValuesAreClampedAndLabelled) {
    chart_spec spec;
    spec.kind = chart_kind::bar;
    spec.title = "delta";
    spec.labels = {"A", "B", "C"};
    spec.series.push_back(series_of("net", {-5.0, numeric_traits<double>::infinity(), 10.0}));
    spec.series.push_back(series_of("other", {3.0, numeric_traits<double>::quiet_nan(), -2.0}));
    spec.width = 80;
    spec.height = 20;
    const screen scr = render_screen(chart(spec), 80, 20, test_theme());
    const string text = scr.to_plaintext();
    EXPECT_TRUE(text.contains("-5"));
    EXPECT_TRUE(text.contains("10"));
    EXPECT_GT(block_count(scr), 0U);

    chart_spec line = line_spec();
    line.series.push_back(series_of("bad", {numeric_traits<double>::quiet_nan(), numeric_traits<double>::infinity(),
                                            -3.0, numeric_traits<double>::quiet_nan()}));
    EXPECT_NO_THROW(render_plain(chart(line), 80, 20, test_theme()));
}

TEST(TuiChartTest, HistogramBinsAreContiguousComparedToBarGaps) {
    chart_spec bars;
    bars.kind = chart_kind::bar;
    bars.labels = {"1", "2", "3"};
    bars.series.push_back(series_of("count", {5, 5, 5}));
    bars.width = 80;
    bars.height = 20;
    chart_spec histogram = bars;
    histogram.kind = chart_kind::histogram;
    const size_t bar_blocks = block_count(render_screen(chart(bars), 80, 20, test_theme()));
    const size_t histogram_blocks = block_count(render_screen(chart(histogram), 80, 20, test_theme()));
    EXPECT_GT(bar_blocks, 0U);
    EXPECT_GT(histogram_blocks, bar_blocks);
}

TEST(TuiChartTest, ProgressRevealIsDeterministicAndMonotonic) {
    for (const chart_kind kind:
         {chart_kind::bar, chart_kind::line, chart_kind::area, chart_kind::pie, chart_kind::donut, chart_kind::scatter,
          chart_kind::histogram, chart_kind::heatmap, chart_kind::radar}) {
        chart_spec spec = spec_for(kind);
        spec.progress = 0.35F;
        const screen partial_a = render_screen(chart(spec), spec.width, spec.height, test_theme());
        const screen partial_b = render_screen(chart(spec), spec.width, spec.height, test_theme());
        EXPECT_EQ(partial_a.to_plaintext(), partial_b.to_plaintext()) << static_cast<int>(kind);

        const size_t partial_blocks = block_count(partial_a);
        const string partial_text = partial_a.to_plaintext();
        spec.progress = 1.0F;
        const screen full = render_screen(chart(spec), spec.width, spec.height, test_theme());
        if (kind == chart_kind::bar || kind == chart_kind::area || kind == chart_kind::pie ||
            kind == chart_kind::donut || kind == chart_kind::histogram || kind == chart_kind::heatmap) {
            EXPECT_LE(partial_blocks, block_count(full)) << static_cast<int>(kind);
            EXPECT_LT(partial_blocks, block_count(full)) << static_cast<int>(kind);
        } else {
            const size_t partial_markers = braille_count(partial_text);
            const size_t full_markers = braille_count(full.to_plaintext());
            EXPECT_LE(partial_markers, full_markers) << static_cast<int>(kind);
        }
    }

    chart_spec spec = line_spec();
    spec.progress = 0.0F;
    const screen hidden = render_screen(chart(spec), spec.width, spec.height, test_theme());
    EXPECT_EQ(hidden.to_plaintext(), render_plain(chart(spec), spec.width, spec.height, test_theme()));
    EXPECT_FALSE(has_braille(hidden.to_plaintext()));
    spec.progress = numeric_traits<float>::quiet_nan();
    const screen nan_progress = render_screen(chart(spec), spec.width, spec.height, test_theme());
    EXPECT_EQ(nan_progress.to_plaintext(), hidden.to_plaintext());

    chart_spec eased = line_spec();
    eased.progress = 0.5F;
    eased.easing = chart_easing::linear;
    const size_t linear_markers = braille_count(render_plain(chart(eased), 80, 20, test_theme()));
    eased.easing = chart_easing::ease_out;
    const size_t ease_out_markers = braille_count(render_plain(chart(eased), 80, 20, test_theme()));
    EXPECT_LT(linear_markers, ease_out_markers);
}

TEST(TuiChartTest, EasingIsMonotonicAndClamped) {
    EXPECT_FLOAT_EQ(chart_eased_progress(-1.0F, chart_easing::linear), 0.0F);
    EXPECT_FLOAT_EQ(chart_eased_progress(2.0F, chart_easing::linear), 1.0F);
    EXPECT_FLOAT_EQ(chart_eased_progress(numeric_traits<float>::quiet_nan(), chart_easing::linear), 0.0F);
    EXPECT_LT(chart_eased_progress(0.5F, chart_easing::ease_in), 0.5F);
    EXPECT_GT(chart_eased_progress(0.5F, chart_easing::ease_out), 0.5F);
    EXPECT_FLOAT_EQ(chart_eased_progress(0.5F, chart_easing::ease_in_out), 0.5F);
    EXPECT_FLOAT_EQ(chart_eased_progress(0.5F, chart_easing::smooth), 0.5F);
    float previous = -1.0F;
    for (int step = 0; step <= 20; ++step) {
        const float eased = chart_eased_progress(static_cast<float>(step) / 20.0F, chart_easing::smooth);
        EXPECT_GE(eased, previous);
        previous = eased;
    }
}

TEST(TuiChartTest, SelectedIndexDrawsCrosshairAndTooltipWithoutCorruptingAxes) {
    chart_spec spec = line_spec();
    spec.selected_index = 2;
    const screen scr = render_screen(chart(spec), 80, 20, test_theme());
    const string text = scr.to_plaintext();
    EXPECT_TRUE(text.contains("\xe2\x94\x8a"));
    EXPECT_TRUE(text.contains("alpha"));
    EXPECT_TRUE(text.contains("C"));
    EXPECT_TRUE(text.contains("\xe2\x94\x82"));
    EXPECT_TRUE(text.contains("A"));

    spec.selected_index = 99;
    const string ignored = render_plain(chart(spec), 80, 20, test_theme());
    EXPECT_FALSE(ignored.contains("\xe2\x94\x8a"));
}

TEST(TuiChartTest, ChartPointQueriesFollowEachKindSelection) {
    const chart_spec line = line_spec();
    EXPECT_EQ(chart_point_count(line), 4U);
    const auto points = chart_points_at(line, 1);
    ASSERT_EQ(points.size(), 2U);
    EXPECT_EQ(points[0].series_name, "alpha");
    EXPECT_EQ(points[0].label, "B");
    EXPECT_DOUBLE_EQ(points[0].value, 3.0);
    EXPECT_EQ(points[1].series_name, "beta");
    EXPECT_DOUBLE_EQ(points[1].value, 1.0);

    const chart_spec pie = pie_spec();
    EXPECT_EQ(chart_point_count(pie), 3U);
    const auto slice = chart_points_at(pie, 1);
    ASSERT_EQ(slice.size(), 1U);
    EXPECT_EQ(slice[0].series_name, "B");
    EXPECT_DOUBLE_EQ(slice[0].value, 2.0);

    const chart_spec heat = heatmap_spec();
    EXPECT_EQ(chart_point_count(heat), 12U);
    const auto cell = chart_points_at(heat, 5);
    ASSERT_EQ(cell.size(), 1U);
    EXPECT_EQ(cell[0].series_name, "row2");
    EXPECT_EQ(cell[0].label, "y");
    EXPECT_DOUBLE_EQ(cell[0].value, 3.0);

    const chart_spec radar = radar_spec();
    EXPECT_EQ(chart_point_count(radar), 5U);
    const auto spoke = chart_points_at(radar, 2);
    ASSERT_EQ(spoke.size(), 2U);
    EXPECT_DOUBLE_EQ(spoke[0].value, 4.0);
    EXPECT_DOUBLE_EQ(spoke[1].value, 2.0);
}

TEST(TuiChartTest, ChartComponentKeyboardNavigationExposesSelectedPoint) {
    chart_component component(line_spec());
    component.setup();
    EXPECT_EQ(component.selection_count(), 4U);
    EXPECT_FALSE(component.selected_index().has_value());
    EXPECT_FALSE(component.selected_point().has_value());

    EXPECT_TRUE(component.select_next());
    ASSERT_TRUE(component.selected_index().has_value());
    EXPECT_EQ(component.selected_index().value(), 0U);
    ASSERT_TRUE(component.selected_point().has_value());
    EXPECT_EQ(component.selected_point()->label, "A");
    EXPECT_EQ(component.selected_point()->series_name, "alpha");
    EXPECT_DOUBLE_EQ(component.selected_point()->value, 1.0);
    EXPECT_EQ(component.selected_points().size(), 2U);

    key_event right;
    right.key = key_event::type::right;
    EXPECT_TRUE(component.on_key(right));
    EXPECT_EQ(component.selected_index().value(), 1U);

    key_event left;
    left.key = key_event::type::left;
    EXPECT_TRUE(component.on_key(left));
    EXPECT_EQ(component.selected_index().value(), 0U);
    EXPECT_TRUE(component.on_key(left));
    EXPECT_EQ(component.selected_index().value(), 0U);

    key_event end;
    end.key = key_event::type::end;
    EXPECT_TRUE(component.on_key(end));
    EXPECT_EQ(component.selected_index().value(), 3U);

    key_event home;
    home.key = key_event::type::home;
    EXPECT_TRUE(component.on_key(home));
    EXPECT_EQ(component.selected_index().value(), 0U);

    key_event escape;
    escape.key = key_event::type::escape;
    EXPECT_TRUE(component.on_key(escape));
    EXPECT_FALSE(component.selected_index().has_value());
    EXPECT_FALSE(component.selected_point().has_value());
}

TEST(TuiChartTest, ChartComponentHeatmapSupportsRowNavigation) {
    chart_component component(heatmap_spec());
    component.setup();
    EXPECT_TRUE(component.select(4));
    EXPECT_EQ(component.selected_index().value(), 4U);
    EXPECT_TRUE(component.select_up());
    EXPECT_EQ(component.selected_index().value(), 0U);
    EXPECT_FALSE(component.select_up());
    EXPECT_TRUE(component.select_down());
    EXPECT_EQ(component.selected_index().value(), 4U);
    EXPECT_TRUE(component.select_down());
    EXPECT_EQ(component.selected_index().value(), 8U);
    EXPECT_FALSE(component.select_down());
}

TEST(TuiChartTest, ChartComponentMouseHitMappingSelectsRenderedPoint) {
    chart_component component(bar_spec());
    component.setup();
    const element tree = component.render();
    const auto layout = compute_layout(tree, 80, 20);
    screen scr(80, 20);
    scr.clear();
    renderer r(scr, test_theme(), 80, 20);
    r.render(tree, layout, false);

    const layout_rect box = component.area();
    ASSERT_EQ(box.w, 80);
    ASSERT_EQ(box.h, 20);
    const auto hit = component.hit_test(box.w / 2, box.h / 2);
    ASSERT_TRUE(hit.has_value());

    mouse_event press;
    press.x = box.x + box.w / 2;
    press.y = box.y + box.h / 2;
    press.button = mouse_button::left;
    press.action = mouse_action::press;
    EXPECT_TRUE(component.on_mouse(press));
    ASSERT_TRUE(component.selected_index().has_value());
    EXPECT_EQ(component.selected_index().value(), hit.value());
    ASSERT_TRUE(component.selected_point().has_value());

    mouse_event outside;
    outside.x = box.x;
    outside.y = box.y;
    outside.button = mouse_button::left;
    outside.action = mouse_action::press;
    EXPECT_FALSE(component.on_mouse(outside));
    EXPECT_EQ(component.selected_index().value(), hit.value());

    mouse_event move_event = press;
    move_event.action = mouse_action::move;
    EXPECT_FALSE(component.on_mouse(move_event));
}

TEST(TuiChartTest, PieAndHeatmapMouseMappingSelectsScannedCells) {
    chart_component pie(pie_spec());
    pie.setup();
    const element pie_tree = pie.render();
    const auto pie_layout = compute_layout(pie_tree, 80, 20);
    screen pie_screen(80, 20);
    renderer pie_renderer(pie_screen, test_theme(), 80, 20);
    pie_renderer.render(pie_tree, pie_layout, false);
    optional<size_t> pie_hit;
    int pie_x = 0;
    int pie_y = 0;
    for (int y = 0; y < 20 && !pie_hit.has_value(); ++y) {
        for (int x = 0; x < 80 && !pie_hit.has_value(); ++x) {
            pie_hit = pie.hit_test(x, y);
            if (pie_hit.has_value()) {
                pie_x = x;
                pie_y = y;
            }
        }
    }
    ASSERT_TRUE(pie_hit.has_value());
    mouse_event pie_press;
    pie_press.x = pie.area().x + pie_x;
    pie_press.y = pie.area().y + pie_y;
    pie_press.action = mouse_action::press;
    pie_press.button = mouse_button::left;
    EXPECT_TRUE(pie.on_mouse(pie_press));
    EXPECT_EQ(pie.selected_index().value(), pie_hit.value());

    chart_component heat(heatmap_spec());
    heat.setup();
    const element heat_tree = heat.render();
    const auto heat_layout = compute_layout(heat_tree, 80, 20);
    screen heat_screen(80, 20);
    renderer heat_renderer(heat_screen, test_theme(), 80, 20);
    heat_renderer.render(heat_tree, heat_layout, false);
    optional<size_t> heat_hit;
    int hit_x = 0;
    int hit_y = 0;
    for (int y = 0; y < 20 && !heat_hit.has_value(); ++y) {
        for (int x = 0; x < 80 && !heat_hit.has_value(); ++x) {
            heat_hit = heat.hit_test(x, y);
            if (heat_hit.has_value()) {
                hit_x = x;
                hit_y = y;
            }
        }
    }
    ASSERT_TRUE(heat_hit.has_value());
    mouse_event heat_press;
    heat_press.x = heat.area().x + hit_x;
    heat_press.y = heat.area().y + hit_y;
    heat_press.action = mouse_action::press;
    heat_press.button = mouse_button::left;
    EXPECT_TRUE(heat.on_mouse(heat_press));
    EXPECT_EQ(heat.selected_index().value(), heat_hit.value());
}

TEST(TuiChartTest, ComponentAnimationAdvancesProgressFromFrameDeltas) {
    chart_spec spec = line_spec();
    spec.progress = 0.0F;
    chart_component component(spec);
    component.setup();
    EXPECT_FLOAT_EQ(component.progress(), 0.0F);
    component.animate_progress(1.0F, 100_ms);
    EXPECT_TRUE(component.animating());
    component.on_animation(50);
    EXPECT_FLOAT_EQ(component.progress(), 0.5F);
    component.on_animation(25);
    EXPECT_FLOAT_EQ(component.progress(), 0.75F);
    component.on_animation(25);
    EXPECT_FLOAT_EQ(component.progress(), 1.0F);
    EXPECT_FALSE(component.animating());
    component.animate_progress(0.0F, 10_ms);
    component.on_animation(20);
    EXPECT_FLOAT_EQ(component.progress(), 0.0F);
}

TEST(TuiChartTest, ThemeRolesAndExplicitSeriesColorsAreApplied) {
    const theme palette = test_theme();
    chart_spec spec = bar_spec();
    spec.legend.show = false;
    spec.series[0].role = theme_role::accent;
    spec.series[1].color = neforce::color{1, 2, 3, 255};
    const screen scr = render_screen(chart(spec), 80, 20, palette);
    EXPECT_TRUE(has_block_with_color(scr, palette.get(theme_role::accent)));
    EXPECT_TRUE(has_block_with_color(scr, neforce::color{1, 2, 3, 255}));

    const neforce::color resolved = chart_series_color(palette, spec.series[1], 1);
    EXPECT_TRUE(same_color(resolved, neforce::color{1, 2, 3, 255}));
    chart_series role_series;
    role_series.role = theme_role::success;
    EXPECT_TRUE(same_color(chart_series_color(palette, role_series, 0), palette.get(theme_role::success)));

    chart_spec light_spec = bar_spec();
    light_spec.legend.show = false;
    light_spec.series[0].role = theme_role::accent;
    const theme light = make_palette(theme_palette_id::light);
    const screen light_scr = render_screen(chart(light_spec), 80, 20, light);
    EXPECT_TRUE(has_block_with_color(light_scr, light.get(theme_role::accent)));
    EXPECT_FALSE(has_block_with_color(light_scr, palette.get(theme_role::accent)));
}

TEST(TuiChartTest, HeatmapShowsGridAndIntensityLegend) {
    const chart_spec spec = heatmap_spec();
    const string text = render_plain(chart(spec), 80, 20, test_theme());
    EXPECT_TRUE(text.contains("row1"));
    EXPECT_TRUE(text.contains("row2"));
    EXPECT_TRUE(text.contains("row3"));
    EXPECT_TRUE(text.contains("x"));
    EXPECT_TRUE(text.contains("y"));
    EXPECT_TRUE(text.contains("z"));
    EXPECT_TRUE(text.contains("w"));
    EXPECT_GE(count_text(text, "\xe2\x96\x88"), 12U);
}

TEST(TuiChartTest, RadarDrawsSpokesPolygonAndLabels) {
    chart_spec spec = radar_spec();
    spec.selected_index = 1;
    const string text = render_plain(chart(spec), 80, 20, test_theme());
    EXPECT_TRUE(has_braille(text));
    EXPECT_TRUE(text.contains("N"));
    EXPECT_TRUE(text.contains("E"));
    EXPECT_TRUE(text.contains("alpha"));
    EXPECT_TRUE(text.contains("beta"));
}

TEST(TuiChartTest, AreaAndBarsUseEighthBlockPrecision) {
    chart_spec area = line_spec();
    area.kind = chart_kind::area;
    area.labels.clear();
    for (const char* label: {"A", "B", "C", "D", "E"}) {
        area.labels.push_back(label);
    }
    area.series.clear();
    area.series.push_back(series_of("curve", {1.13, 2.71, 1.87, 4.19, 2.35}));
    area.series.push_back(series_of("other", {0.61, 1.49, 0.93, 2.23, 1.08}));
    area.width = 80;
    area.height = 20;
    const string area_text = render_plain(chart(area), 80, 20, test_theme());
    EXPECT_GT(eighth_block_count(area_text), 0U);
    EXPECT_GE(eighth_block_count(area_text), 4U);

    chart_spec bars = bar_spec();
    bars.labels.clear();
    for (const char* label: {"A", "B", "C"}) {
        bars.labels.push_back(label);
    }
    bars.series.clear();
    bars.series.push_back(series_of("fractional", {1.17, 2.43, 3.89}));
    bars.width = 60;
    bars.height = 18;
    const string bar_text = render_plain(chart(bars), 60, 18, test_theme());
    EXPECT_GT(eighth_block_count(bar_text), 0U);
}

TEST(TuiChartTest, HorizontalLineKeepsASingleBrailleDotRow) {
    chart_spec spec = line_spec();
    spec.labels.clear();
    for (const char* label: {"A", "B", "C", "D", "E", "F"}) {
        spec.labels.push_back(label);
    }
    spec.series.clear();
    spec.series.push_back(series_of("flat", {5, 5, 5, 5, 5, 5}));
    spec.width = 80;
    spec.height = 20;
    const string text = render_plain(chart(spec), 80, 20, test_theme());
    const auto glyphs = braille_codepoints(text);
    ASSERT_FALSE(glyphs.empty());
    for (const uint32_t value: glyphs) {
        const uint32_t bits = value - 0x2800U;
        bool rows[4] = {false, false, false, false};
        rows[0] = (bits & 0x09U) != 0U;
        rows[1] = (bits & 0x12U) != 0U;
        rows[2] = (bits & 0x24U) != 0U;
        rows[3] = (bits & 0xC0U) != 0U;
        int row_count = 0;
        for (const bool row: rows) {
            row_count += row ? 1 : 0;
        }
        EXPECT_EQ(row_count, 1) << "braille U+" << static_cast<int>(value);
    }
    EXPECT_GT(glyphs.size(), 10U);
}

TEST(TuiChartTest, AllKindsAreDeterministicAtNarrowMediumAndWideWidths) {
    for (const chart_kind kind:
         {chart_kind::bar, chart_kind::line, chart_kind::area, chart_kind::pie, chart_kind::donut, chart_kind::scatter,
          chart_kind::histogram, chart_kind::heatmap, chart_kind::radar}) {
        for (const int width: {40, 80, 120}) {
            chart_spec spec = spec_for(kind);
            spec.width = width;
            spec.height = 16;
            spec.progress = width == 40 ? 0.37F : width == 80 ? 0.63F : 1.0F;
            spec.easing = chart_easing::smooth;
            const size_t selectable = chart_point_count(spec);
            if (selectable > 1) {
                spec.selected_index = 1;
            }
            const string first = render_plain(chart(spec), width, 16, test_theme());
            const string second = render_plain(chart(spec), width, 16, test_theme());
            EXPECT_EQ(first, second) << "kind " << static_cast<int>(kind) << " width " << width;
            EXPECT_FALSE(first.empty()) << "kind " << static_cast<int>(kind) << " width " << width;
            EXPECT_LE(widest_line(first), width) << "kind " << static_cast<int>(kind) << " width " << width;
            EXPECT_GT(chart_ink_count(first), 0U) << "kind " << static_cast<int>(kind) << " width " << width;
        }
    }
}

TEST(TuiChartTest, NarrowAxisLabelsUseCompactSuffixesAndStayInsideWidth) {
    chart_spec spec;
    spec.kind = chart_kind::bar;
    spec.title = "big";
    spec.labels = {"A", "B", "C"};
    spec.series.push_back(series_of("bytes", {1000000000.0, 2000000000.0, 3000000000.0}));
    spec.width = 40;
    spec.height = 16;
    const string text = render_plain(chart(spec), 40, 16, test_theme());
    EXPECT_TRUE(text.contains("G")) << text.view().data();
    EXPECT_FALSE(text.contains("1000000000"));
    EXPECT_LE(widest_line(text), 40);
    EXPECT_GT(chart_ink_count(text), 0U);
}

TEST(TuiChartTest, LongCategoryLabelsAreEllipsizedWithoutOverlap) {
    chart_spec spec = bar_spec();
    spec.title.clear();
    spec.legend.show = false;
    spec.width = 40;
    spec.height = 16;
    spec.labels.clear();
    spec.labels.push_back("Long category alpha");
    spec.labels.push_back("Long category beta");
    spec.labels.push_back("Long category gamma");
    spec.labels.push_back("Long category delta");
    spec.selected_index = 2;
    const string text = render_plain(chart(spec), 40, 16, test_theme());
    EXPECT_TRUE(text.contains("\xe2\x80\xa6"));
    EXPECT_LE(widest_line(text), 40);
    for (auto& line: split_lines(text)) {
        EXPECT_LE(text_display_width(line.view()), 40);
    }
}

TEST(TuiChartTest, RightLegendTruncatesNamesAndReportsOverflow) {
    chart_spec spec;
    spec.kind = chart_kind::bar;
    spec.title = "many";
    spec.labels = {"A", "B", "C"};
    spec.width = 40;
    spec.height = 14;
    for (int i = 0; i < 18; ++i) {
        const string name = "long-series-name-" + to_string(i);
        spec.series.push_back(
                series_of(name.view().data(), {static_cast<double>(i % 5 + 1), static_cast<double>(i % 3 + 1),
                                               static_cast<double>(i % 7 + 1)}));
    }
    spec.selected_index = 2;
    const string text = render_plain(chart(spec), 40, 14, test_theme());
    EXPECT_GT(count_text(text, "\xe2\x96\xa0"), 0U);
    EXPECT_TRUE(text.contains("\xe2\x80\xa6"));
    EXPECT_TRUE(text.contains("more"));
    EXPECT_LE(widest_line(text), 40);
}

TEST(TuiChartTest, BottomLegendAndPiePercentagesTruncateGracefully) {
    chart_spec bottom = bar_spec();
    bottom.width = 40;
    bottom.height = 14;
    bottom.labels.clear();
    bottom.labels.push_back("A");
    bottom.labels.push_back("B");
    bottom.legend.position = chart_legend_position::bottom;
    bottom.series.clear();
    for (int i = 0; i < 8; ++i) {
        const string name = "category-" + to_string(i);
        bottom.series.push_back(
                series_of(name.view().data(), {static_cast<double>(i + 1), static_cast<double>(i + 2)}));
    }
    const string bottom_text = render_plain(chart(bottom), 40, 14, test_theme());
    EXPECT_GT(count_text(bottom_text, "\xe2\x96\xa0"), 0U);
    EXPECT_TRUE(bottom_text.contains("\xe2\x80\xa6"));
    EXPECT_LE(widest_line(bottom_text), 40);

    chart_spec pie = pie_spec();
    pie.width = 40;
    pie.height = 14;
    pie.labels.clear();
    pie.labels.push_back("a-very-long-slice-name");
    pie.labels.push_back("another-long-slice-name");
    pie.labels.push_back("third-long-slice-name");
    pie.selected_index = 1;
    const string pie_text = render_plain(chart(pie), 40, 14, test_theme());
    EXPECT_TRUE(pie_text.contains("%"));
    EXPECT_TRUE(pie_text.contains("50.0%"));
    EXPECT_TRUE(pie_text.contains("\xe2\x96\xb6"));
    EXPECT_TRUE(pie_text.contains("\xe2\x97\x86"));
    EXPECT_LE(widest_line(pie_text), 40);
}

TEST(TuiChartTest, CrosshairHasVerticalAndHorizontalGuidesWithoutHidingAxes) {
    chart_spec spec = line_spec();
    spec.selected_index = 2;
    const string text = render_plain(chart(spec), 80, 20, test_theme());
    EXPECT_TRUE(text.contains("\xe2\x94\x8a"));
    EXPECT_TRUE(text.contains("\xe2\x94\x88"));
    EXPECT_TRUE(text.contains("\xe2\x94\x82"));
    EXPECT_TRUE(text.contains("\xe2\x94\x80"));
    EXPECT_TRUE(text.contains("C  alpha 2"));
}

TEST(TuiChartTest, SelectedPointAndCrosshairUseThemeAccent) {
    const theme palette = test_theme();
    chart_spec spec = line_spec();
    spec.selected_index = 2;
    const screen scr = render_screen(chart(spec), 80, 20, palette);
    EXPECT_TRUE(has_text_with_color(scr, "C", palette.get(theme_role::accent)));
    EXPECT_TRUE(has_text_with_color(scr, "\xe2\x94\x8a", palette.get(theme_role::subtle)));
}

TEST(TuiChartTest, PieHeatmapAndRadarSelectionsShowTooltipsAndCues) {
    chart_spec pie = pie_spec();
    pie.selected_index = 1;
    const string pie_text = render_plain(chart(pie), 80, 20, test_theme());
    EXPECT_TRUE(pie_text.contains("B"));
    EXPECT_TRUE(pie_text.contains("50.0%"));
    EXPECT_TRUE(pie_text.contains("\xc2\xb7"));
    EXPECT_TRUE(pie_text.contains("\xe2\x97\x86"));

    chart_spec heat = heatmap_spec();
    heat.selected_index = 6;
    const string heat_text = render_plain(chart(heat), 80, 20, test_theme());
    EXPECT_TRUE(heat_text.contains("row2 \xc2\xb7 z"));
    EXPECT_TRUE(heat_text.contains(" 2"));

    chart_spec radar = radar_spec();
    radar.selected_index = 1;
    const string radar_text = render_plain(chart(radar), 80, 20, test_theme());
    EXPECT_TRUE(radar_text.contains("E  alpha 3"));
    EXPECT_TRUE(radar_text.contains("beta 4"));
}

TEST(TuiChartTest, HeadlessKeyboardMouseAndFrameProgressionPath) {
    chart_host host;
    host.attach(bar_spec());
    headless app(&host, 80, 20);
    const theme palette = test_theme();
    app.set_theme(palette);
    app.render();
    ASSERT_NE(host.view, nullptr);
    EXPECT_EQ(host.view->area().w, 80);
    EXPECT_EQ(host.view->area().h, 20);

    key_event tab;
    tab.key = key_event::type::tab;
    app.feed_key(tab);
    app.tick();
    EXPECT_TRUE(host.view->has_focus());

    app.feed("\x1b[C");
    app.tick();
    ASSERT_TRUE(host.view->selected_index().has_value());
    EXPECT_EQ(host.view->selected_index().value(), 0U);
    EXPECT_TRUE(app.plaintext().contains("\xe2\x94\x8a"));
    EXPECT_TRUE(app.plaintext().contains("alpha"));
    EXPECT_TRUE(has_text_with_color(app.frame(), "\xe2\x94\x8a", palette.get(theme_role::accent)));

    key_event right;
    right.key = key_event::type::right;
    app.feed_key(right);
    app.tick();
    EXPECT_EQ(host.view->selected_index().value(), 1U);
    key_event left;
    left.key = key_event::type::left;
    app.feed_key(left);
    app.tick();
    EXPECT_EQ(host.view->selected_index().value(), 0U);

    const layout_rect box = host.view->area();
    optional<size_t> hit;
    int local_x = 0;
    int local_y = 0;
    for (int y = 0; y < box.h && !hit.has_value(); ++y) {
        for (int x = 0; x < box.w && !hit.has_value(); ++x) {
            hit = host.view->hit_test(x, y);
            if (hit.has_value()) {
                local_x = x;
                local_y = y;
            }
        }
    }
    ASSERT_TRUE(hit.has_value());
    const string mouse = sgr_mouse_press(box.x + local_x, box.y + local_y);
    app.feed(mouse.view());
    app.tick();
    ASSERT_TRUE(host.view->selected_index().has_value());
    EXPECT_EQ(host.view->selected_index().value(), hit.value());
    EXPECT_TRUE(app.plaintext().contains("\xe2\x94\x8a"));

    host.view->set_progress(0.0F);
    host.view->animate_progress(1.0F, 100_ms);
    app.tick();
    const size_t start_ink = chart_ink_count(app.plaintext());
    size_t previous = start_ink;
    for (int frame = 0; frame < 4; ++frame) {
        host.view->on_animation(25);
        app.tick();
        const size_t ink = chart_ink_count(app.plaintext());
        EXPECT_GE(ink, previous);
        previous = ink;
    }
    EXPECT_FLOAT_EQ(host.view->progress(), 1.0F);
    EXPECT_FALSE(host.view->animating());
    EXPECT_GT(previous, start_ink);
    EXPECT_LE(widest_line(app.plaintext()), 80);
}

TEST(TuiChartTest, HeadlessLineRevealAdvancesOverFrames) {
    chart_spec spec = line_spec();
    spec.progress = 0.0F;
    chart_host host;
    host.attach(spec);
    headless app(&host, 80, 20);
    app.render();
    ASSERT_NE(host.view, nullptr);
    EXPECT_EQ(braille_count(app.plaintext()), 0U);
    host.view->animate_progress(1.0F, 80_ms);
    app.tick();
    size_t previous = braille_count(app.plaintext());
    for (int frame = 0; frame < 4; ++frame) {
        host.view->on_animation(20);
        app.tick();
        const size_t ink = braille_count(app.plaintext());
        EXPECT_GE(ink, previous);
        previous = ink;
    }
    EXPECT_FLOAT_EQ(host.view->progress(), 1.0F);
    EXPECT_FALSE(host.view->animating());
    EXPECT_GT(previous, 0U);
}

TEST(TuiChartTest, StreamBucketingIsOptInAndAggregates) {
    deque<double> stream;
    for (int index = 1; index <= 10; ++index) {
        stream.push_back(static_cast<double>(index));
    }
    chart_spec spec;
    spec.kind = chart_kind::line;
    spec.series.push_back(chart_series{"s", {}, &stream});

    EXPECT_EQ(chart_point_count(spec), 10U);

    spec.stream_buckets = 5;
    EXPECT_EQ(chart_point_count(spec), 5U);
    const auto last = chart_points_at(spec, 1);
    ASSERT_EQ(last.size(), 1U);
    EXPECT_DOUBLE_EQ(last[0].value, 4.0);

    spec.stream_aggregate = chart_stream_aggregate::high;
    const auto high = chart_points_at(spec, 4);
    ASSERT_EQ(high.size(), 1U);
    EXPECT_DOUBLE_EQ(high[0].value, 10.0);

    spec.stream_aggregate = chart_stream_aggregate::low;
    const auto low = chart_points_at(spec, 0);
    ASSERT_EQ(low.size(), 1U);
    EXPECT_DOUBLE_EQ(low[0].value, 1.0);

    spec.stream_buckets = 20;
    spec.stream_aggregate = chart_stream_aggregate::last;
    EXPECT_EQ(chart_point_count(spec), 20U);
    EXPECT_TRUE(chart_points_at(spec, 0).empty());

    spec.stream_buckets = 0;
    spec.stream_samples = 4;
    EXPECT_EQ(chart_point_count(spec), 4U);
}
