#include <NeForce/core/numeric/math.hpp>
#include <NeForce/core/numeric/numeric_types.hpp>
#include <NeForce/core/string/to_string.hpp>
#include <NeForce/core/string/utf_iterator.hpp>
#include <NeForce/core/string/width.hpp>
#include <NeForce/tui/dom/chart.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    constexpr double chart_pi = static_cast<double>(constants::PI);
    constexpr int default_chart_width = 60;
    constexpr int default_chart_height = 16;

    double clamp01(const double value) noexcept {
        if (is_nan(value)) {
            return 0.0;
        }
        if (value < 0.0) {
            return 0.0;
        }
        if (value > 1.0) {
            return 1.0;
        }
        return value;
    }

    bool finite_number(const double value) noexcept { return is_finite(value); }

    float ease_progress(const float raw, const chart_easing easing) noexcept {
        const double t = clamp01(static_cast<double>(is_nan(raw) ? 0.0F : raw));
        double result = t;
        switch (easing) {
            case chart_easing::linear:
                result = t;
                break;
            case chart_easing::ease_in:
                result = t * t * t;
                break;
            case chart_easing::ease_out: {
                const double inv = 1.0 - t;
                result = 1.0 - inv * inv * inv;
                break;
            }
            case chart_easing::ease_in_out: {
                if (t < 0.5) {
                    result = 4.0 * t * t * t;
                } else {
                    const double inv = -2.0 * t + 2.0;
                    result = 1.0 - inv * inv * inv / 2.0;
                }
                break;
            }
            case chart_easing::smooth: {
                const double inv = 1.0 - t;
                result = t * t * t * (10.0 - 15.0 * t + 6.0 * t * t);
                (void) inv;
                break;
            }
        }
        return static_cast<float>(clamp01(result));
    }

    int display_width(const string& text) { return text_display_width(text.view()); }

    string truncate_text(const string_view text, const int max_cols) {
        if (max_cols <= 0) {
            return string();
        }
        if (text_display_width(text) <= max_cols) {
            string copy;
            copy.reserve(text.size());
            for (const char c: text) {
                copy.push_back(c);
            }
            return copy;
        }
        if (max_cols <= 1) {
            return string("\xe2\x80\xa6");
        }
        string out;
        int used = 0;
        for (utf8_iterator it(reinterpret_cast<const byte_t*>(text.data()), text.size()); it != utf8_iterator(); ++it) {
            const int width = max(1, it->display_width());
            if (used + width + 1 > max_cols) {
                break;
            }
            it->append_to(out);
            used += width;
        }
        out += "\xe2\x80\xa6";
        return out;
    }

    int decimals_for_step(const double step) {
        if (!finite_number(step) || step <= 0.0) {
            return 2;
        }
        const auto logarithm = static_cast<double>(logarithm_10(static_cast<decimal_t>(step)));
        int decimals = -static_cast<int>(floor(logarithm));
        decimals = max(decimals, 0);
        decimals = min(decimals, 6);
        return decimals;
    }

    string format_value(const double value, const double step) {
        if (!finite_number(value)) {
            return string("-");
        }
        if (value == 0.0) {
            return string("0");
        }
        const double magnitude = absolute(value);
        if (magnitude >= 1000000.0 || magnitude < 0.0001) {
            return to_string_general(value, 4);
        }
        string text = to_string_fixed(value, decimals_for_step(step));
        if (text.contains('.')) {
            while (text.size() > 1 && text.back() == '0') {
                text.pop_back();
            }
            if (!text.empty() && text.back() == '.') {
                text.pop_back();
            }
        }
        if (text == "-0") {
            text = "0";
        }
        return text;
    }

    string format_compact_value(const double value, const double step) {
        if (!finite_number(value)) {
            return string("-");
        }
        if (value == 0.0) {
            return string("0");
        }
        const double magnitude = absolute(value);
        double scaled = value;
        const char* suffix = nullptr;
        if (magnitude >= 1000000000000.0) {
            scaled = value / 1000000000000.0;
            suffix = "T";
        } else if (magnitude >= 1000000000.0) {
            scaled = value / 1000000000.0;
            suffix = "G";
        } else if (magnitude >= 1000000.0) {
            scaled = value / 1000000.0;
            suffix = "M";
        } else if (magnitude >= 1000.0) {
            scaled = value / 1000.0;
            suffix = "k";
        }
        if (suffix != nullptr) {
            string text = to_string_fixed(scaled, 1);
            if (text.contains('.')) {
                while (text.size() > 1 && text.back() == '0') {
                    text.pop_back();
                }
                if (!text.empty() && text.back() == '.') {
                    text.pop_back();
                }
            }
            if (text == "-0") {
                text = "0";
            }
            return text + suffix;
        }
        int decimals = decimals_for_step(step);
        decimals = min(decimals, 2);
        string text = to_string_fixed(value, decimals);
        if (text.contains('.')) {
            while (text.size() > 1 && text.back() == '0') {
                text.pop_back();
            }
            if (!text.empty() && text.back() == '.') {
                text.pop_back();
            }
        }
        if (text == "-0") {
            text = "0";
        }
        return text;
    }

    double nice_step(const double raw) {
        if (!finite_number(raw) || raw <= 0.0) {
            return 1.0;
        }
        const auto exponent = static_cast<double>(floor(logarithm_10(static_cast<decimal_t>(raw))));
        double base = 1.0;
        const int abs_exp = static_cast<int>(absolute(exponent));
        if (exponent >= 0.0) {
            base = power(10.0, static_cast<uint32_t>(abs_exp));
        } else {
            base = 1.0 / power(10.0, static_cast<uint32_t>(abs_exp));
        }
        if (!finite_number(base) || base <= 0.0) {
            return 1.0;
        }
        const double fraction = raw / base;
        double nice = 10.0;
        if (fraction <= 1.0) {
            nice = 1.0;
        } else if (fraction <= 2.0) {
            nice = 2.0;
        } else if (fraction <= 5.0) {
            nice = 5.0;
        }
        return nice * base;
    }

    struct chart_model {
        size_t rows = 0;
        size_t cols = 0;
        size_t categories = 0;
        size_t spokes = 0;
        size_t selectable = 0;
        bool any_finite = false;
        double data_min = 0.0;
        double data_max = 0.0;
        double positive_total = 0.0;
    };

    void observe_value(chart_model& model, const double value) {
        if (!finite_number(value)) {
            return;
        }
        if (!model.any_finite) {
            model.any_finite = true;
            model.data_min = value;
            model.data_max = value;
        } else {
            model.data_min = min(model.data_min, value);
            model.data_max = max(model.data_max, value);
        }
        if (value > 0.0) {
            model.positive_total += value;
        }
    }

    chart_model build_model(const chart_spec& spec) {
        chart_model model;
        model.rows = spec.series.size();
        model.cols = spec.labels.size();
        for (const auto& series: spec.series) {
            model.cols = max(model.cols, series.values.size());
            for (const double value: series.values) {
                observe_value(model, value);
            }
        }
        if (!finite_number(model.positive_total)) {
            model.positive_total = 0.0;
        }
        model.categories = model.cols;
        model.spokes = spec.labels.empty() ? model.cols : spec.labels.size();
        switch (spec.kind) {
            case chart_kind::pie:
            case chart_kind::donut:
                model.selectable = spec.series.size();
                break;
            case chart_kind::heatmap:
                model.selectable = model.rows * model.cols;
                break;
            case chart_kind::radar:
                model.selectable = model.spokes;
                break;
            default:
                model.selectable = model.categories;
                break;
        }
        return model;
    }

    struct axis_scale {
        double min = 0.0;
        double max = 1.0;
        double step = 1.0;
        vector<double> ticks;
        bool valid = false;
    };

    axis_scale make_axis_scale(const chart_spec& spec, const chart_model& model) {
        axis_scale scale;
        if (!model.any_finite) {
            scale.min = 0.0;
            scale.max = 1.0;
            scale.step = 0.5;
            scale.ticks.push_back(0.0);
            scale.ticks.push_back(0.5);
            scale.ticks.push_back(1.0);
            return scale;
        }
        double lower = model.data_min;
        double upper = model.data_max;
        if (spec.axis.min.has_value() && finite_number(spec.axis.min.value())) {
            lower = spec.axis.min.value();
        }
        if (spec.axis.max.has_value() && finite_number(spec.axis.max.value())) {
            upper = spec.axis.max.value();
        }
        const bool include_zero =
                spec.axis.zero_baseline && (spec.kind == chart_kind::bar || spec.kind == chart_kind::histogram ||
                                            spec.kind == chart_kind::area || spec.kind == chart_kind::radar);
        if (include_zero) {
            lower = min(lower, 0.0);
            upper = max(upper, 0.0);
        }
        if (lower > upper) {
            const double swap = lower;
            lower = upper;
            upper = swap;
        }
        if (upper - lower <= 0.0 || !finite_number(upper - lower)) {
            const double magnitude = max(1.0, absolute(lower));
            lower -= magnitude * 0.5;
            upper += magnitude * 0.5;
        }
        const int target_ticks = max(2, spec.axis.tick_count);
        const double raw_step = (upper - lower) / static_cast<double>(target_ticks);
        const double step = nice_step(raw_step);
        scale.step = step;
        if (spec.axis.min.has_value() || spec.axis.max.has_value()) {
            scale.min = lower;
            scale.max = upper;
        } else {
            scale.min = static_cast<double>(floor(lower / step)) * step;
            scale.max = static_cast<double>(ceil(upper / step)) * step;
        }
        if (scale.max - scale.min <= 0.0 || !finite_number(scale.max - scale.min)) {
            scale.min = lower;
            scale.max = lower + step;
        }
        const double epsilon = step * 1e-6;
        double tick = static_cast<double>(ceil(scale.min / step)) * step;
        for (int i = 0; i < 64 && tick <= scale.max + epsilon; ++i) {
            scale.ticks.push_back(tick);
            tick += step;
        }
        if (scale.ticks.empty()) {
            scale.ticks.push_back(scale.min);
            scale.ticks.push_back(scale.max);
        }
        scale.valid = true;
        return scale;
    }

    string tick_text(const chart_spec& spec, const axis_scale& scale, const double value, const bool compact = false) {
        if (spec.axis.format) {
            return spec.axis.format(value);
        }
        return compact ? format_compact_value(value, scale.step) : format_value(value, scale.step);
    }

    struct chart_geometry {
        int w = 0;
        int h = 0;
        bool show_title = false;
        int title_y = -1;
        int status_y = -1;
        bool has_plot = false;
        bool show_axis = false;
        bool show_y_labels = false;
        bool show_x_labels = false;
        bool compact_y_labels = false;
        int y_label_w = 0;
        int plot_x = 0;
        int plot_y = 0;
        int plot_w = 0;
        int plot_h = 0;
        int axis_x = -1;
        int x_label_y = -1;
        bool legend_visible = false;
        bool legend_right = false;
        bool legend_bottom = false;
        int legend_x = 0;
        int legend_y = 0;
        int legend_w = 0;
        int legend_h = 0;
        double pie_cx = 0.0;
        double pie_cy = 0.0;
        double pie_radius = 0.0;
        double pie_inner = 0.0;
        int heat_x = 0;
        int heat_y = 0;
        int heat_cols = 0;
        int heat_rows = 0;
        int heat_cell_w = 1;
        int heat_cell_h = 1;
    };

    bool pie_kind(const chart_kind kind) noexcept { return kind == chart_kind::pie || kind == chart_kind::donut; }

    chart_geometry compute_geometry(const chart_spec& spec, const chart_model& model, const axis_scale& scale,
                                    const int width, const int height) {
        chart_geometry geom;
        geom.w = max(0, width);
        geom.h = max(0, height);
        if (geom.w <= 0 || geom.h <= 0) {
            return geom;
        }
        geom.show_title = !spec.title.empty() && geom.h >= 3;
        geom.title_y = geom.show_title ? 0 : -1;
        geom.status_y = (geom.h >= 8) ? geom.h - 1 : -1;
        const int top = geom.show_title ? 1 : 0;
        int bottom = geom.h - (geom.status_y >= 0 ? 1 : 0);

        const bool is_pie = pie_kind(spec.kind);
        int max_entry = 0;
        if (spec.kind == chart_kind::heatmap) {
            max_entry = 12;
        } else {
            for (const auto& series: spec.series) {
                int entry = display_width(series.name) + 3;
                if (is_pie) {
                    entry += 7;
                }
                max_entry = max(max_entry, entry);
            }
        }
        if (spec.legend.show && spec.legend.position != chart_legend_position::none && bottom - top >= 4) {
            if (spec.legend.position == chart_legend_position::right && geom.w >= 34 && max_entry > 0) {
                const int legend_w = min(max(10, max_entry + 2), max(10, geom.w / 3));
                if (geom.w - legend_w >= 18) {
                    geom.legend_visible = true;
                    geom.legend_right = true;
                    geom.legend_w = legend_w;
                    geom.legend_x = geom.w - legend_w;
                    geom.legend_y = top;
                    geom.legend_h = bottom - top;
                }
            }
            if (!geom.legend_visible && geom.w >= 24 && bottom - top >= 6 && geom.h >= 8) {
                geom.legend_visible = true;
                geom.legend_bottom = true;
                geom.legend_h = 1;
                bottom -= 1;
                geom.legend_x = 1;
                geom.legend_y = bottom;
                geom.legend_w = max(1, geom.w - 2);
            }
        }

        int right_pad = 1;
        if (geom.legend_right) {
            right_pad = geom.legend_w + 1;
        }
        const int avail_w = max(1, geom.w - right_pad);

        if (is_pie) {
            constexpr int main_x = 1;
            const int main_w = max(1, avail_w - 2);
            const int main_y = top;
            const int main_h = max(1, bottom - top);
            geom.pie_cx = static_cast<double>(main_x) + static_cast<double>(main_w) / 2.0;
            geom.pie_cy = static_cast<double>(main_y) + static_cast<double>(main_h) / 2.0;
            double radius = min((static_cast<double>(main_w) - 2.0) / 2.0, static_cast<double>(main_h) - 1.0);
            radius = max(1.0, radius);
            geom.pie_radius = radius;
            if (spec.kind == chart_kind::donut) {
                geom.pie_inner = max(1.0, radius * 0.45);
            }
            geom.has_plot = true;
            return geom;
        }

        if (spec.kind == chart_kind::radar) {
            geom.plot_x = 3;
            geom.plot_y = top;
            geom.plot_w = max(1, avail_w - 4);
            geom.plot_h = max(1, bottom - top);
            geom.has_plot = true;
            return geom;
        }

        if (spec.kind == chart_kind::heatmap) {
            geom.show_x_labels =
                    spec.axis.show_labels && !spec.labels.empty() && bottom - top >= 4 && (avail_w - 2) >= 8;
            if (geom.show_x_labels) {
                bottom -= 1;
                geom.x_label_y = bottom;
            }
            int row_label_w = 0;
            int longest_row = 0;
            for (const auto& series: spec.series) {
                longest_row = max(longest_row, display_width(series.name));
            }
            if (spec.axis.show_labels && longest_row > 0 && avail_w >= longest_row + 12) {
                row_label_w = min(14, longest_row + 1);
            }
            const int grid_x = row_label_w + (row_label_w > 0 ? 1 : 0);
            const int grid_w = max(0, avail_w - grid_x);
            const int grid_h = max(0, bottom - top);
            if (grid_w > 0 && grid_h > 0) {
                geom.heat_x = grid_x;
                geom.heat_y = top;
                geom.has_plot = true;
                if (model.rows > 0 && model.cols > 0) {
                    geom.heat_cell_w = max(1, min(3, grid_w / static_cast<int>(model.cols)));
                    geom.heat_cell_h = max(1, min(3, grid_h / static_cast<int>(model.rows)));
                    geom.heat_cols = min(static_cast<int>(model.cols), grid_w / geom.heat_cell_w);
                    geom.heat_rows = min(static_cast<int>(model.rows), grid_h / geom.heat_cell_h);
                }
            }
            geom.plot_x = grid_x;
            geom.plot_y = top;
            geom.plot_w = grid_w;
            geom.plot_h = grid_h;
            return geom;
        }

        geom.show_x_labels =
                spec.axis.show && spec.axis.show_labels && !spec.labels.empty() && bottom - top >= 4 && avail_w >= 20;
        if (geom.show_x_labels) {
            bottom -= 1;
            geom.x_label_y = bottom;
        }
        geom.show_axis = spec.axis.show;
        int longest_tick = 0;
        int longest_compact = 0;
        if (spec.axis.show_labels) {
            for (const double value: scale.ticks) {
                longest_tick = max(longest_tick, display_width(tick_text(spec, scale, value)));
                longest_compact = max(longest_compact, display_width(tick_text(spec, scale, value, true)));
            }
        }
        int y_label_w = 0;
        if (spec.axis.show_labels && longest_tick > 0 && bottom - top >= 3) {
            // Keep at least a readable plot; when the normal labels cannot fit,
            // fall back to compact suffixes (k/M/G) before giving up entirely.
            const int minimum_plot = geom.w >= 60 ? 14 : 9;
            bool compact = false;
            int width1 = longest_tick;
            if (!spec.axis.format && longest_tick > 8 && longest_compact > 0 && longest_compact < longest_tick) {
                compact = true;
                width1 = longest_compact;
            } else if (!spec.axis.format && avail_w < longest_tick + minimum_plot && longest_compact > 0 &&
                       longest_compact < longest_tick && avail_w >= longest_compact + max(6, minimum_plot - 2)) {
                compact = true;
                width1 = longest_compact;
            }
            if (width1 <= 12 && avail_w >= width1 + max(6, minimum_plot - 3)) {
                geom.show_y_labels = true;
                geom.compact_y_labels = compact;
                y_label_w = width1 + 1;
            }
        }
        geom.y_label_w = y_label_w;
        geom.plot_x = (geom.show_axis ? 1 : 0) + y_label_w;
        geom.axis_x = geom.show_axis ? geom.plot_x - 1 : -1;
        geom.plot_y = top;
        geom.plot_w = max(0, avail_w - geom.plot_x);
        geom.plot_h = max(0, bottom - top);
        geom.has_plot = geom.plot_w > 0 && geom.plot_h > 0;
        return geom;
    }

    double value_row_f(const double value, const axis_scale& scale, const chart_geometry& geom) noexcept {
        const double span = scale.max - scale.min;
        if (span <= 0.0 || !finite_number(span)) {
            return static_cast<double>(geom.plot_y + max(0, geom.plot_h - 1));
        }
        const double ratio = clamp01((value - scale.min) / span);
        return static_cast<double>(geom.plot_y) + (1.0 - ratio) * static_cast<double>(max(0, geom.plot_h - 1));
    }

    int value_row(const double value, const axis_scale& scale, const chart_geometry& geom) noexcept {
        return static_cast<int>(round(value_row_f(value, scale, geom)));
    }

    double zero_row_f(const chart_spec& /*spec*/, const chart_model& model, const axis_scale& scale,
                      const chart_geometry& geom) noexcept {
        if (scale.min <= 0.0 && scale.max >= 0.0) {
            return value_row_f(0.0, scale, geom);
        }
        if (model.any_finite && model.data_max < 0.0) {
            return static_cast<double>(geom.plot_y);
        }
        return static_cast<double>(geom.plot_y + geom.plot_h);
    }

    int category_x(const chart_spec& spec, const chart_geometry& geom, const size_t count,
                   const size_t index) noexcept {
        if (count == 0 || geom.plot_w <= 0) {
            return geom.plot_x;
        }
        if ((spec.kind == chart_kind::bar || spec.kind == chart_kind::histogram) &&
            compare_less_equal(count, geom.plot_w)) {
            const int slot = max(1, geom.plot_w / static_cast<int>(count));
            return geom.plot_x + static_cast<int>(index) * slot + slot / 2;
        }
        if (count <= 1) {
            return geom.plot_x + geom.plot_w / 2;
        }
        return geom.plot_x + static_cast<int>(round(static_cast<double>(index) * static_cast<double>(geom.plot_w - 1) /
                                                    static_cast<double>(count - 1)));
    }

    int category_at_x(const chart_spec& spec, const chart_geometry& geom, const size_t count, const int x) noexcept {
        if (count == 0 || geom.plot_w <= 0) {
            return 0;
        }
        if ((spec.kind == chart_kind::bar || spec.kind == chart_kind::histogram) &&
            compare_less_equal(count, geom.plot_w)) {
            const int slot = max(1, geom.plot_w / static_cast<int>(count));
            int index = (x - geom.plot_x) / slot;
            index = max(0, min(index, static_cast<int>(count) - 1));
            return index;
        }
        if (count <= 1) {
            return 0;
        }
        const double ratio = static_cast<double>(x - geom.plot_x) / static_cast<double>(max(1, geom.plot_w - 1));
        int index = static_cast<int>(round(ratio * static_cast<double>(count - 1)));
        index = max(0, min(index, static_cast<int>(count) - 1));
        return index;
    }

    double clock_angle(const double dx, const double dy) noexcept {
        const double x = -dy;
        const double y = dx;
        double angle = 0.0;
        if (x > 0.0) {
            angle = static_cast<double>(arctangent(static_cast<decimal_t>(y / x)));
        } else if (x < 0.0) {
            angle = static_cast<double>(arctangent(static_cast<decimal_t>(y / x)));
            angle += y >= 0.0 ? chart_pi : -chart_pi;
        } else if (y > 0.0) {
            angle = chart_pi / 2.0;
        } else if (y < 0.0) {
            angle = -chart_pi / 2.0;
        }
        if (angle < 0.0) {
            angle += chart_pi * 2.0;
        }
        return angle;
    }

    struct braille_canvas {
        int w = 0;
        int h = 0;
        vector<uint8_t> dots;

        braille_canvas(const int width, const int height) :
        w(max(0, width)),
        h(max(0, height)),
        dots(static_cast<size_t>(w) * static_cast<size_t>(h), 0) {}

        void clear() {
            for (auto& cell: dots) {
                cell = 0;
            }
        }

        void set_dot(const int dx, const int dy) {
            if (dx < 0 || dy < 0 || dx >= w * 2 || dy >= h * 4) {
                return;
            }
            static constexpr uint8_t bits[4][2] = {{0x01, 0x08}, {0x02, 0x10}, {0x04, 0x20}, {0x40, 0x80}};
            dots[static_cast<size_t>(dy / 4) * static_cast<size_t>(w) + static_cast<size_t>(dx / 2)] |=
                    bits[dy % 4][dx % 2];
        }

        void dot(const double x, const double y) {
            set_dot(static_cast<int>(round(x * 2.0)), static_cast<int>(round(y * 4.0)));
        }

        void line(double x0, double y0, double x1, double y1) {
            // Supersample each dot at four sub-positions.
            // Rounded sub-samples make short segments and shallow diagonals land
            // on the exact braille dot they cross instead of drifting to a neighbour.
            const double ax = x0 * 2.0;
            const double ay = y0 * 4.0;
            const double bx = x1 * 2.0;
            const double by = y1 * 4.0;
            const double dx = bx - ax;
            const double dy = by - ay;
            const double span = max(absolute(dx), absolute(dy));
            if (span <= 1e-9) {
                set_dot(static_cast<int>(round(ax)), static_cast<int>(round(ay)));
                return;
            }
            const int steps = max(1, static_cast<int>(ceil(span * 4.0)));
            for (int step = 0; step <= steps; ++step) {
                const double ratio = static_cast<double>(step) / static_cast<double>(steps);
                set_dot(static_cast<int>(round(ax + dx * ratio)), static_cast<int>(round(ay + dy * ratio)));
            }
        }
    };

    void paint_braille(canvas_painter& painter, const braille_canvas& canvas, const int origin_x, const int origin_y,
                       const _NEFORCE color& c, const bool bold = false) {
        style s;
        s.fg = c;
        if (bold) {
            s.bold = true;
        }
        for (int y = 0; y < canvas.h; ++y) {
            for (int x = 0; x < canvas.w; ++x) {
                const uint8_t bits =
                        canvas.dots[static_cast<size_t>(y) * static_cast<size_t>(canvas.w) + static_cast<size_t>(x)];
                if (bits != 0) {
                    painter.put(origin_x + x, origin_y + y, codepoint(static_cast<uint32_t>(0x2800U + bits)), s);
                }
            }
        }
    }

    _NEFORCE color series_color_at(const theme& t, const chart_spec& spec, const size_t index) {
        if (index < spec.series.size()) {
            return chart_series_color(t, spec.series[index], index);
        }
        return t.get(theme_role::accent);
    }

    _NEFORCE color grid_color(const theme& t, const chart_spec& spec) {
        if (spec.grid.color.has_value()) {
            return spec.grid.color.value();
        }
        if (spec.grid.role.has_value()) {
            return t.get(spec.grid.role.value());
        }
        return t.get(theme_role::subtle);
    }

    _NEFORCE color legend_text_color(const theme& t, const chart_spec& spec) {
        if (spec.legend.color.has_value()) {
            return spec.legend.color.value();
        }
        if (spec.legend.role.has_value()) {
            return t.get(spec.legend.role.value());
        }
        return t.get(theme_role::text);
    }

    void draw_title(canvas_painter& painter, const chart_spec& spec, const chart_geometry& geom, const theme& t) {
        if (!geom.show_title) {
            return;
        }
        const string title = truncate_text(spec.title.view(), geom.w);
        const int x = max(0, (geom.w - display_width(title)) / 2);
        style s;
        s.fg = t.get(theme_role::text);
        s.bold = true;
        painter.text(x, geom.title_y, title.view(), s);
    }

    double pie_total(const chart_spec& spec, const chart_series& series) {
        (void) spec;
        double total = 0.0;
        for (const double value: series.values) {
            if (finite_number(value) && value > 0.0) {
                total += value;
            }
        }
        return total;
    }

    string slice_display_name(const chart_spec& spec, const size_t index) {
        if (index < spec.labels.size() && !spec.labels[index].empty()) {
            return spec.labels[index];
        }
        if (index < spec.series.size()) {
            return spec.series[index].name;
        }
        return string();
    }

    double pie_slice_value(const chart_series& series) {
        double total = 0.0;
        for (const double value: series.values) {
            if (finite_number(value)) {
                total += value;
            }
        }
        return total;
    }

    string percent_text(const double value, const double total) {
        double percent = 0.0;
        if (total > 0.0) {
            percent = max(0.0, value) / total * 100.0;
        }
        string text = to_string_fixed(percent, 1);
        return move(text) + "%";
    }

    string legend_entry_text(const chart_spec& spec, const size_t index, const double total, const int max_width,
                             const bool is_pie) {
        if (max_width <= 0 || index >= spec.series.size()) {
            return string();
        }
        string name = is_pie ? slice_display_name(spec, index) : spec.series[index].name;
        if (name.empty()) {
            name = "series " + to_string(static_cast<int>(index));
        }
        if (!is_pie) {
            return truncate_text(name.view(), max_width);
        }
        string percent = percent_text(pie_slice_value(spec.series[index]), total);
        const int percent_width = display_width(percent);
        if (max_width <= percent_width) {
            return truncate_text(percent.view(), max_width);
        }
        const int name_width = max_width - percent_width - 1;
        if (name_width <= 0) {
            return move(percent);
        }
        return truncate_text(name.view(), name_width) + " " + percent;
    }

    void draw_legend_right(canvas_painter& painter, const chart_spec& spec, const chart_geometry& geom, const theme& t,
                           const chart_model& model, const optional<size_t>& selected, const double total) {
        if (!geom.legend_visible || !geom.legend_right) {
            return;
        }
        const _NEFORCE color text_color = legend_text_color(t, spec);
        const int rows = geom.legend_h;
        if (rows <= 0) {
            return;
        }
        const bool is_pie = pie_kind(spec.kind);
        const int count = static_cast<int>(spec.series.size());
        const bool overflow = count > rows;
        const int entry_rows = overflow ? max(0, rows - 1) : min(rows, count);
        vector<size_t> visible;
        visible.reserve(static_cast<size_t>(max(0, entry_rows)));
        for (int i = 0; i < entry_rows; ++i) {
            visible.push_back(static_cast<size_t>(i));
        }
        if (selected.has_value() && compare_less(selected.value(), count) && entry_rows > 0) {
            bool present = false;
            for (const size_t candidate: visible) {
                if (candidate == selected.value()) {
                    present = true;
                    break;
                }
            }
            if (!present) {
                visible[static_cast<size_t>(entry_rows - 1)] = selected.value();
            }
        }
        for (size_t slot = 0; slot < visible.size(); ++slot) {
            const size_t i = visible[slot];
            const int row = geom.legend_y + static_cast<int>(slot);
            const _NEFORCE color c = series_color_at(t, spec, i);
            const bool is_selected = is_pie && selected.has_value() && selected.value() == i;
            style swatch_style;
            swatch_style.fg = c;
            swatch_style.bold = is_selected;
            if (geom.legend_w >= 1) {
                painter.put(geom.legend_x, row, "\xe2\x96\xa0", swatch_style);
            }
            int label_w = max(0, geom.legend_w - 2);
            if (is_selected && geom.legend_w >= 4) {
                label_w = max(0, label_w - 1);
            }
            const string label = legend_entry_text(spec, i, total, label_w, is_pie);
            style label_style;
            label_style.fg = is_selected ? t.get(theme_role::accent) : text_color;
            label_style.bold = is_selected;
            if (!label.empty() && geom.legend_w > 2) {
                painter.text(geom.legend_x + 2, row, label.view(), label_style);
            }
            if (is_selected && geom.legend_w >= 4) {
                style mark_style;
                mark_style.fg = t.get(theme_role::accent);
                painter.put(geom.legend_x + geom.legend_w - 1, row, "\xe2\x96\xb6", mark_style);
            }
        }
        if (overflow) {
            const int hidden = count - entry_rows;
            const string more = "+" + to_string(static_cast<int>(max(0, hidden))) + " more";
            style more_style;
            more_style.fg = text_color;
            more_style.dim = true;
            const string shown = truncate_text(more.view(), geom.legend_w);
            painter.text(geom.legend_x, geom.legend_y + max(0, entry_rows), shown.view(), more_style);
        }
        (void) model;
    }

    void draw_legend_bottom(canvas_painter& painter, const chart_spec& spec, const chart_geometry& geom, const theme& t,
                            const optional<size_t>& selected, const double total) {
        if (!geom.legend_visible || !geom.legend_bottom) {
            return;
        }
        const bool is_pie = pie_kind(spec.kind);
        const int y = geom.legend_y;
        int x = geom.legend_x;
        const int limit = geom.legend_x + geom.legend_w;
        const _NEFORCE color text_color = legend_text_color(t, spec);
        size_t drawn_entries = 0;
        for (size_t i = 0; i < spec.series.size(); ++i) {
            const bool is_selected = is_pie && selected.has_value() && selected.value() == i;
            const int remaining = limit - x;
            if (remaining < 2) {
                break;
            }
            const int label_w = max(0, remaining - 2);
            const string label = legend_entry_text(spec, i, total, label_w, is_pie);
            if (label.empty()) {
                continue;
            }
            style swatch_style;
            swatch_style.fg = series_color_at(t, spec, i);
            swatch_style.bold = is_selected;
            painter.put(x, y, "\xe2\x96\xa0", swatch_style);
            x += 2;
            style label_style;
            label_style.fg = is_selected ? t.get(theme_role::accent) : text_color;
            label_style.bold = is_selected;
            const int shown_w = min(display_width(label), max(0, limit - x));
            const string shown = truncate_text(label.view(), shown_w);
            painter.text(x, y, shown.view(), label_style);
            x += display_width(shown) + 1;
            ++drawn_entries;
        }
        if (drawn_entries < spec.series.size() && geom.legend_w > 0) {
            style more_style;
            more_style.fg = text_color;
            more_style.dim = true;
            const int marker = min(x, limit - 1);
            if (marker >= geom.legend_x) {
                painter.put(marker, y, "\xe2\x80\xa6", more_style);
            }
        }
    }

    void draw_intensity_legend(canvas_painter& painter, const chart_spec& spec, const chart_geometry& geom,
                               const theme& t, const axis_scale& scale) {
        if (!geom.legend_visible) {
            return;
        }
        const _NEFORCE color high = series_color_at(t, spec, 0);
        const _NEFORCE color low = _NEFORCE color::lerp(t.bg, high, 0.1);
        const _NEFORCE color text_color = legend_text_color(t, spec);
        style text_style;
        text_style.fg = text_color;
        style high_style;
        high_style.fg = high;
        style low_style;
        low_style.fg = low;
        if (geom.legend_right) {
            const int x = geom.legend_x;
            const int y = geom.legend_y;
            const int rows = geom.legend_h;
            if (rows < 3 || geom.legend_w < 6) {
                return;
            }
            const string high_text = truncate_text(tick_text(spec, scale, scale.max).view(), geom.legend_w - 3);
            painter.text(x, y, high_text.view(), high_style);
            if (rows >= 5) {
                const string low_text = truncate_text(tick_text(spec, scale, scale.min).view(), geom.legend_w - 3);
                painter.text(x, y + rows - 1, low_text.view(), low_style);
                const int bar_x = x + geom.legend_w - 2;
                for (int i = 1; i < rows - 1; ++i) {
                    const double ratio = 1.0 - static_cast<double>(i - 1) / static_cast<double>(max(1, rows - 3));
                    style s;
                    s.fg = _NEFORCE color::lerp(low, high, clamp01(ratio));
                    painter.put(bar_x, y + i, "\xe2\x96\x88", s);
                    painter.put(bar_x + 1, y + i, "\xe2\x96\x88", s);
                }
            }
        } else if (geom.legend_bottom) {
            const int x = geom.legend_x;
            const int y = geom.legend_y;
            int cursor = x;
            const int limit = x + geom.legend_w;
            const string high_text = truncate_text(tick_text(spec, scale, scale.max).view(), 8);
            painter.text(cursor, y, high_text.view(), high_style);
            cursor += max(2, display_width(tick_text(spec, scale, scale.max)) + 1);
            for (int i = 0; i < 6 && cursor < limit; ++i) {
                style s;
                s.fg = _NEFORCE color::lerp(low, high, static_cast<double>(i) / 5.0);
                painter.put(cursor, y, "\xe2\x96\x88", s);
                ++cursor;
            }
            if (cursor + 1 < limit) {
                const string low_text = truncate_text(tick_text(spec, scale, scale.min).view(), limit - cursor - 1);
                painter.text(cursor + 1, y, low_text.view(), low_style);
            }
        }
    }

    void draw_status(canvas_painter& painter, const chart_spec& spec, const chart_geometry& geom,
                     const optional<size_t>& selected, const bool focused, const theme& t, const chart_model& model,
                     const axis_scale& scale) {
        if (geom.status_y < 0) {
            return;
        }
        const double span = max(scale.step, 1e-9);
        style s;
        s.fg = t.get(theme_role::text);
        if (!selected.has_value()) {
            if (!focused) {
                return;
            }
            string hint;
            if (spec.kind == chart_kind::heatmap) {
                hint = "left/right/up/down: inspect cell";
            } else if (pie_kind(spec.kind)) {
                hint = "left/right: inspect slice";
            } else {
                hint = "left/right: inspect point";
            }
            style hint_style;
            hint_style.fg = t.get(theme_role::subtle);
            hint_style.dim = true;
            const string shown_hint = truncate_text(hint.view(), geom.w - 2);
            painter.text(1, geom.status_y, shown_hint.view(), hint_style);
            return;
        }
        const size_t index = selected.value();
        string text;
        if (pie_kind(spec.kind)) {
            if (index < spec.series.size()) {
                const auto& series = spec.series[index];
                double total = 0.0;
                for (const auto& item: spec.series) {
                    total += pie_total(spec, item);
                }
                const double value = pie_slice_value(series);
                text = slice_display_name(spec, index);
                if (text.empty()) {
                    text = "slice";
                }
                text += "  ";
                text += format_value(value, span);
                text += "  ";
                text += percent_text(value, total);
            }
        } else if (spec.kind == chart_kind::heatmap) {
            if (model.cols > 0) {
                const size_t row = index / model.cols;
                const size_t col = index % model.cols;
                if (row < spec.series.size()) {
                    text = spec.series[row].name;
                }
                if (col < spec.labels.size()) {
                    if (!text.empty()) {
                        text += " \xc2\xb7 ";
                    }
                    text += spec.labels[col];
                }
                const auto points = chart_points_at(spec, index);
                if (!points.empty()) {
                    text += "  ";
                    text += format_value(points[0].value, span);
                } else {
                    text += "  no data";
                }
            }
        } else {
            const string label = index < spec.labels.size() ? spec.labels[index] : string();
            if (!label.empty()) {
                text = label;
                text += "  ";
            }
            const auto points = chart_points_at(spec, index);
            if (points.empty()) {
                text += "no data";
            } else {
                for (size_t i = 0; i < points.size(); ++i) {
                    if (i > 0) {
                        text += "  ";
                    }
                    if (!points[i].series_name.empty()) {
                        text += points[i].series_name;
                        text += " ";
                    }
                    text += format_value(points[i].value, span);
                }
            }
        }
        const string shown_status = truncate_text(text.view(), geom.w - 2);
        painter.text(1, geom.status_y, shown_status.view(), s);
    }

    void fill_vertical_bar(canvas_painter& painter, const int x, const int w, const chart_geometry& geom,
                           const double top_row, const double bottom_row, const _NEFORCE color& c, const bool bold) {
        if (w <= 0 || geom.plot_h <= 0) {
            return;
        }
        const auto plot_top = static_cast<double>(geom.plot_y);
        const auto plot_bottom = static_cast<double>(geom.plot_y + geom.plot_h);
        const double low = max(plot_top, min(top_row, bottom_row));
        const double high = min(plot_bottom, max(top_row, bottom_row));
        if (high <= low) {
            return;
        }

        // Eighth blocks give eight vertical levels per cell instead of two,
        // which removes the visible stair-step of half-block area fills.
        static constexpr string_view bottom_slices[9] = {"",
                                                         "\xe2\x96\x81",
                                                         "\xe2\x96\x82",
                                                         "\xe2\x96\x83",
                                                         "\xe2\x96\x84",
                                                         "\xe2\x96\x85",
                                                         "\xe2\x96\x86",
                                                         "\xe2\x96\x87",
                                                         "\xe2\x96\x88"};

        style s;
        s.fg = c;
        s.bold = bold;
        const int first_row = max(0, static_cast<int>(floor(low)) - geom.plot_y);
        const int last_row = min(geom.plot_h - 1, static_cast<int>(floor(high - 1e-9)) - geom.plot_y);
        for (int row = first_row; row <= last_row; ++row) {
            const double cell_top = plot_top + static_cast<double>(row);
            const double cell_bottom = cell_top + 1.0;
            const double covered_top = max(low, cell_top);
            const double covered_bottom = min(high, cell_bottom);
            const double coverage = covered_bottom - covered_top;
            if (coverage <= 1e-9) {
                continue;
            }
            string_view glyph = "\xe2\x96\x88";
            if (coverage < 0.999) {
                int eighths = static_cast<int>(round(coverage * 8.0));
                eighths = max(1, min(7, eighths));
                const bool touches_bottom = covered_bottom >= cell_bottom - 1e-9;
                const bool touches_top = covered_top <= cell_top + 1e-9;
                if (touches_bottom) {
                    glyph = bottom_slices[eighths];
                } else if (touches_top) {
                    glyph = eighths <= 2 ? "\xe2\x96\x94" : "\xe2\x96\x80";
                } else {
                    glyph = eighths >= 4 ? "\xe2\x96\x84" : bottom_slices[eighths];
                }
            }
            for (int col = 0; col < w; ++col) {
                painter.put(x + col, geom.plot_y + row, glyph, s);
            }
        }
    }

    void draw_bars(canvas_painter& painter, const chart_spec& spec, const chart_geometry& geom,
                   const chart_model& model, const axis_scale& scale, const theme& t, const optional<size_t>& selected,
                   const float eased) {
        if (!geom.has_plot || model.categories == 0 || spec.series.empty()) {
            return;
        }
        const int category_count = static_cast<int>(model.categories);
        if (category_count <= 0) {
            return;
        }
        const int slot = max(1, geom.plot_w / category_count);
        const int gap = spec.kind == chart_kind::histogram ? 0 : (slot >= 3 ? 1 : 0);
        const int group_w = max(1, slot - gap);
        const int series_count = max(1, static_cast<int>(spec.series.size()));
        const int inner_gap = (series_count > 1 && group_w >= series_count * 2) ? 1 : 0;
        const int bar_w = max(1, (group_w - (series_count - 1) * inner_gap) / series_count);
        const double zero_row = zero_row_f(spec, model, scale, geom);
        const int draw_count = min(category_count, max(1, geom.plot_w));
        for (int slot_index = 0; slot_index < draw_count; ++slot_index) {
            const auto category = static_cast<size_t>(static_cast<int64_t>(slot_index) * category_count / draw_count);
            const int slot_x = geom.plot_x + slot_index * geom.plot_w / draw_count;
            const int group_x = slot_x + max(0, (slot - group_w) / 2);
            const bool category_selected = selected.has_value() && selected.value() == category;
            for (size_t j = 0; j < spec.series.size(); ++j) {
                const auto& series = spec.series[j];
                if (category >= series.values.size()) {
                    continue;
                }
                const double raw = series.values[category];
                if (!finite_number(raw) || raw == 0.0) {
                    continue;
                }
                const double value = raw * static_cast<double>(eased);
                const double row = value_row_f(value, scale, geom);
                const int x = group_x + static_cast<int>(j) * (bar_w + inner_gap);
                const int actual_w = min(bar_w, geom.plot_x + geom.plot_w - x);
                if (actual_w <= 0) {
                    continue;
                }
                _NEFORCE color c = chart_series_color(t, series, j);
                if (category_selected) {
                    c = lighten(c, 0.2);
                }
                fill_vertical_bar(painter, x, actual_w, geom, zero_row, row, c, category_selected);
            }
        }
    }

    void draw_area(canvas_painter& painter, const chart_spec& spec, const chart_geometry& geom,
                   const chart_model& model, const axis_scale& scale, const theme& t, const optional<size_t>& selected,
                   const float eased) {
        if (!geom.has_plot || model.categories == 0 || spec.series.empty() || eased <= 0.0F) {
            return;
        }
        const size_t count = model.categories;
        const double zero_row = zero_row_f(spec, model, scale, geom);
        const int reveal =
                min(geom.plot_w,
                    max(0, static_cast<int>(ceil(static_cast<double>(geom.plot_w) * static_cast<double>(eased)))));
        for (size_t j = spec.series.size(); j-- > 0;) {
            const auto& series = spec.series[j];
            if (series.values.empty()) {
                continue;
            }
            _NEFORCE color c = chart_series_color(t, series, j);
            c = lighten(c, 0.05);
            for (int col = 0; col < reveal; ++col) {
                double position = 0.0;
                if (geom.plot_w > 1) {
                    position = static_cast<double>(col) * static_cast<double>(count - 1) /
                               static_cast<double>(geom.plot_w - 1);
                }
                const size_t index = min(count - 1, static_cast<size_t>(floor(position)));
                const size_t next = min(count - 1, index + 1);
                double value = 0.0;
                double next_value = 0.0;
                bool has_value = false;
                bool has_next = false;
                if (index < series.values.size() && finite_number(series.values[index])) {
                    value = series.values[index];
                    has_value = true;
                }
                if (next < series.values.size() && finite_number(series.values[next])) {
                    next_value = series.values[next];
                    has_next = true;
                }
                if (!has_value && !has_next) {
                    continue;
                }
                if (!has_value) {
                    value = next_value;
                }
                if (!has_next) {
                    next_value = value;
                }
                const double fraction = position - static_cast<double>(index);
                const double interpolated = value + (next_value - value) * clamp01(fraction);
                const double row = value_row_f(interpolated, scale, geom);
                fill_vertical_bar(painter, geom.plot_x + col, 1, geom, zero_row, row, c, false);
            }
        }
        (void) selected;
    }

    void draw_line_like(canvas_painter& painter, const chart_spec& spec, const chart_geometry& geom,
                        const chart_model& model, const axis_scale& scale, const theme& t, const float eased) {
        if (!geom.has_plot || model.categories == 0 || spec.series.empty() || eased <= 0.0F) {
            return;
        }
        const size_t count = model.categories;
        double frontier = static_cast<double>(geom.plot_x) +
                          static_cast<double>(max(0, geom.plot_w - 1)) * static_cast<double>(eased);
        if (count == 1) {
            frontier = static_cast<double>(geom.plot_x + geom.plot_w);
        }
        braille_canvas canvas(geom.plot_w, geom.plot_h);
        for (size_t j = 0; j < spec.series.size(); ++j) {
            const auto& series = spec.series[j];
            if (series.values.empty()) {
                continue;
            }
            canvas.clear();
            bool drew = false;
            int previous_x = 0;
            double previous_y = 0.0;
            bool has_previous = false;
            for (size_t i = 0; i < count && i < series.values.size(); ++i) {
                const double value = series.values[i];
                const int x = category_x(spec, geom, count, i);
                if (!finite_number(value)) {
                    has_previous = false;
                    continue;
                }
                const double y = value_row_f(value, scale, geom);
                if (static_cast<double>(x) <= frontier) {
                    if (has_previous && x != previous_x) {
                        const double y2 = y;
                        canvas.line(static_cast<double>(previous_x - geom.plot_x),
                                    previous_y - static_cast<double>(geom.plot_y), static_cast<double>(x - geom.plot_x),
                                    y2 - static_cast<double>(geom.plot_y));
                        drew = true;
                    } else {
                        canvas.dot(static_cast<double>(x - geom.plot_x), y - static_cast<double>(geom.plot_y));
                        drew = true;
                    }
                    previous_x = x;
                    previous_y = y;
                    has_previous = true;
                } else if (has_previous) {
                    const auto span = static_cast<double>(x - previous_x);
                    const double ratio =
                            span > 0.0 ? clamp01((frontier - static_cast<double>(previous_x)) / span) : 1.0;
                    const double y2 = previous_y + (y - previous_y) * ratio;
                    canvas.line(static_cast<double>(previous_x - geom.plot_x),
                                previous_y - static_cast<double>(geom.plot_y),
                                frontier - static_cast<double>(geom.plot_x), y2 - static_cast<double>(geom.plot_y));
                    drew = true;
                    break;
                }
            }
            if (drew) {
                paint_braille(painter, canvas, geom.plot_x, geom.plot_y, chart_series_color(t, series, j));
            }
        }
    }

    void draw_scatter(canvas_painter& painter, const chart_spec& spec, const chart_geometry& geom,
                      const chart_model& model, const axis_scale& scale, const theme& t, const float eased) {
        if (!geom.has_plot || model.categories == 0 || spec.series.empty() || eased <= 0.0F) {
            return;
        }
        const size_t count = model.categories;
        double frontier = static_cast<double>(geom.plot_x) +
                          static_cast<double>(max(0, geom.plot_w - 1)) * static_cast<double>(eased);
        if (count == 1) {
            frontier = static_cast<double>(geom.plot_x + geom.plot_w);
        }
        braille_canvas canvas(geom.plot_w, geom.plot_h);
        for (size_t j = 0; j < spec.series.size(); ++j) {
            const auto& series = spec.series[j];
            canvas.clear();
            bool drew = false;
            for (size_t i = 0; i < count && i < series.values.size(); ++i) {
                if (!finite_number(series.values[i])) {
                    continue;
                }
                const int x = category_x(spec, geom, count, i);
                if (static_cast<double>(x) > frontier) {
                    continue;
                }
                const double y = value_row_f(series.values[i], scale, geom);
                const auto lx = static_cast<double>(x - geom.plot_x);
                const double ly = y - static_cast<double>(geom.plot_y);
                canvas.dot(lx, ly);
                canvas.dot(lx - 0.25, ly);
                canvas.dot(lx + 0.25, ly);
                canvas.dot(lx, ly - 0.25);
                canvas.dot(lx, ly + 0.25);
                drew = true;
            }
            if (drew) {
                paint_braille(painter, canvas, geom.plot_x, geom.plot_y, chart_series_color(t, series, j));
            }
        }
    }

    void draw_grid_and_axes(canvas_painter& painter, const chart_spec& spec, const chart_geometry& geom,
                            const chart_model& model, const axis_scale& scale, const theme& t) {
        if (!geom.has_plot) {
            return;
        }
        style grid_style;
        grid_style.fg = grid_color(t, spec);
        grid_style.dim = true;
        vector<int> tick_rows;
        vector<double> tick_values;
        for (const double tick: scale.ticks) {
            const int row = value_row(tick, scale, geom);
            if (row < geom.plot_y || row >= geom.plot_y + geom.plot_h) {
                continue;
            }
            bool duplicate = false;
            for (const int existing: tick_rows) {
                if (existing == row) {
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate) {
                tick_rows.push_back(row);
                tick_values.push_back(tick);
                if (spec.grid.show) {
                    painter.hline(geom.plot_x, row, geom.plot_w, "\xc2\xb7", grid_style);
                }
            }
        }
        if (spec.grid.show && spec.grid.vertical && geom.plot_w > 2) {
            for (int i = 1; i < 5; ++i) {
                const int x = geom.plot_x + geom.plot_w * i / 5;
                painter.vline(x, geom.plot_y, geom.plot_h, "\xc2\xb7", grid_style);
            }
        }
        if (geom.show_y_labels) {
            style label_style;
            label_style.fg = t.get(theme_role::subtle);
            const int minimum_gap = geom.plot_h < 8 ? 2 : 1;
            int last_label_row = -1000000;
            for (size_t i = 0; i < tick_rows.size(); ++i) {
                if (tick_rows[i] < geom.plot_y || tick_rows[i] >= geom.plot_y + geom.plot_h) {
                    continue;
                }
                if (absolute(tick_rows[i] - last_label_row) < minimum_gap) {
                    continue;
                }
                const string text = tick_text(spec, scale, tick_values[i], geom.compact_y_labels);
                painter.text_right(0, tick_rows[i], geom.y_label_w, text.view(), label_style);
                last_label_row = tick_rows[i];
            }
        }
        if (!spec.axis.show) {
            return;
        }
        style axis_style;
        axis_style.fg = t.get(theme_role::prompt_border);
        const int baseline =
                scale.min <= 0.0 && scale.max >= 0.0 ? value_row(0.0, scale, geom) : geom.plot_y + geom.plot_h - 1;
        if (geom.axis_x >= 0) {
            painter.vline(geom.axis_x, geom.plot_y, geom.plot_h, "\xe2\x94\x82", axis_style);
            painter.hline(geom.plot_x - 1, baseline, geom.plot_w + 1, "\xe2\x94\x80", axis_style);
            painter.put(geom.axis_x, baseline,
                        baseline == geom.plot_y + geom.plot_h - 1 ? "\xe2\x94\x94" : "\xe2\x94\xbc", axis_style);
            if (spec.axis.show_ticks) {
                for (const int row: tick_rows) {
                    if (row != baseline) {
                        painter.put(geom.axis_x, row, "\xe2\x94\xa4", axis_style);
                    }
                }
            }
        }
        (void) model;
    }

    void draw_x_labels(canvas_painter& painter, const chart_spec& spec, const chart_geometry& geom,
                       const chart_model& model, const theme& t, const optional<size_t>& selected) {
        if (!geom.show_x_labels || geom.x_label_y < 0 || spec.labels.empty()) {
            return;
        }
        const size_t count = model.categories;
        if (count == 0 || geom.plot_w <= 0) {
            return;
        }
        style label_style;
        label_style.fg = t.get(theme_role::subtle);
        style selected_style;
        selected_style.fg = t.get(theme_role::accent);
        selected_style.bold = true;

        const int desired = max(1, geom.plot_w / 9);
        int step = 1;
        if (compare_greater(count, desired)) {
            step = (static_cast<int>(count) + desired - 1) / desired;
        }
        const int max_width = min(12, max(3, geom.plot_w / max(1, min(static_cast<int>(count), desired)) - 1));

        int selected_left = -1;
        int selected_right = -1;
        if (selected.has_value() && selected.value() < count && selected.value() < spec.labels.size()) {
            const string label = truncate_text(spec.labels[selected.value()].view(), max_width);
            if (!label.empty()) {
                const int width = display_width(label);
                int x = category_x(spec, geom, count, selected.value()) - width / 2;
                x = max(geom.plot_x, min(x, geom.plot_x + geom.plot_w - width));
                painter.text(x, geom.x_label_y, label.view(), selected_style);
                selected_left = x;
                selected_right = x + width;
            }
        }

        int last_end = geom.plot_x - 2;
        for (size_t i = 0; i < count && i < spec.labels.size(); ++i) {
            const bool selected_index = selected.has_value() && selected.value() == i;
            if (!selected_index && static_cast<int>(i) % max(1, step) != 0) {
                continue;
            }
            if (selected_index) {
                last_end = max(last_end, selected_right);
                continue;
            }
            const string label = truncate_text(spec.labels[i].view(), max_width);
            if (label.empty()) {
                continue;
            }
            const int width = display_width(label);
            int x = category_x(spec, geom, count, i) - width / 2;
            x = max(geom.plot_x, min(x, geom.plot_x + geom.plot_w - width));
            if (x <= last_end + 1) {
                continue;
            }
            if (selected_right > 0 && x < selected_right && x + width > selected_left) {
                continue;
            }
            painter.text(x, geom.x_label_y, label.view(), label_style);
            last_end = x + width;
        }
    }

    void draw_selection_overlay(canvas_painter& painter, const chart_spec& spec, const chart_geometry& geom,
                                const chart_model& model, const axis_scale& scale, const theme& t,
                                const optional<size_t>& selected, const bool focused) {
        if (!selected.has_value() || !geom.has_plot || selected.value() >= model.selectable) {
            return;
        }
        const size_t index = selected.value();
        if (pie_kind(spec.kind)) {
            return;
        }
        if (spec.kind == chart_kind::heatmap) {
            return;
        }
        if (spec.kind == chart_kind::radar) {
            return;
        }
        style cross_style;
        cross_style.fg = focused ? t.get(theme_role::accent) : t.get(theme_role::subtle);
        cross_style.dim = !focused;
        const int baseline =
                scale.min <= 0.0 && scale.max >= 0.0 ? value_row(0.0, scale, geom) : geom.plot_y + geom.plot_h - 1;
        const int x = category_x(spec, geom, model.categories, index);
        if (x >= geom.plot_x && x < geom.plot_x + geom.plot_w) {
            for (int row = geom.plot_y; row < geom.plot_y + geom.plot_h; ++row) {
                if (row != baseline) {
                    painter.put(x, row, "\xe2\x94\x8a", cross_style);
                }
            }
        }
        int horizontal_row = -1;
        for (size_t j = 0; j < spec.series.size(); ++j) {
            const auto& series = spec.series[j];
            if (index < series.values.size() && finite_number(series.values[index])) {
                horizontal_row = value_row(series.values[index], scale, geom);
                break;
            }
        }
        if (horizontal_row >= geom.plot_y && horizontal_row < geom.plot_y + geom.plot_h && horizontal_row != baseline) {
            for (int col = geom.plot_x; col < geom.plot_x + geom.plot_w; ++col) {
                if (col != x) {
                    painter.put(col, horizontal_row, "\xe2\x94\x88", cross_style);
                }
            }
        }
        for (size_t j = 0; j < spec.series.size(); ++j) {
            const auto& series = spec.series[j];
            if (index >= series.values.size() || !finite_number(series.values[index])) {
                continue;
            }
            const int y = value_row(series.values[index], scale, geom);
            if (y < geom.plot_y || y >= geom.plot_y + geom.plot_h) {
                continue;
            }
            style marker_style;
            marker_style.fg = lighten(chart_series_color(t, series, j), 0.2);
            marker_style.bold = true;
            painter.put(x, y, "\xe2\x97\x8f", marker_style);
        }
    }

    void draw_pie(canvas_painter& painter, const chart_spec& spec, const chart_geometry& geom, const theme& t,
                  const chart_model& model, const optional<size_t>& selected, const float eased) {
        const double total = model.positive_total;
        const double outer = geom.pie_radius;
        const double inner = geom.pie_inner;
        const bool is_donut = spec.kind == chart_kind::donut;
        if (total <= 0.0) {
            const string empty = "no data";
            style s;
            s.fg = t.get(theme_role::subtle);
            s.dim = true;
            const int x = max(0, static_cast<int>(geom.pie_cx) - display_width(empty) / 2);
            const int y = max(0, static_cast<int>(geom.pie_cy));
            painter.text(x, y, empty.view(), s);
            return;
        }
        vector<double> cumulative(spec.series.size(), 0.0);
        double running = 0.0;
        for (size_t i = 0; i < spec.series.size(); ++i) {
            running += max(0.0, pie_slice_value(spec.series[i])) / total;
            cumulative[i] = running;
        }
        const double sweep = chart_pi * 2.0 * static_cast<double>(eased);
        const int min_x = max(0, static_cast<int>(floor(geom.pie_cx - outer)));
        const int max_x = min(geom.w - 1, static_cast<int>(ceil(geom.pie_cx + outer)));
        const int min_y = max(0, static_cast<int>(floor(geom.pie_cy - outer / 2.0)));
        const int max_y = min(geom.h - 1, static_cast<int>(ceil(geom.pie_cy + outer / 2.0)));
        for (int y = min_y; y <= max_y; ++y) {
            for (int x = min_x; x <= max_x; ++x) {
                struct pie_sample {
                    bool inside = false;
                    size_t slice = 0;
                };
                auto sample_at = [&](const double sx, const double sy) {
                    pie_sample sample;
                    const double dx = sx - geom.pie_cx;
                    const double dy = sy - geom.pie_cy;
                    const auto radius =
                            static_cast<double>(square_root(static_cast<decimal_t>(dx * dx + 4.0 * dy * dy)));
                    if (radius > outer || radius < inner) {
                        return sample;
                    }
                    const double angle = clock_angle(dx, dy);
                    if (angle > sweep) {
                        return sample;
                    }
                    size_t slice = 0;
                    while (slice + 1 < cumulative.size() && angle > cumulative[slice] * chart_pi * 2.0) {
                        ++slice;
                    }
                    if (slice < spec.series.size()) {
                        sample.inside = true;
                        sample.slice = slice;
                    }
                    return sample;
                };
                const pie_sample samples[4] = {sample_at(x + 0.25, y + 0.25), sample_at(x + 0.75, y + 0.25),
                                               sample_at(x + 0.25, y + 0.75), sample_at(x + 0.75, y + 0.75)};
                size_t counts[16]{};
                size_t dominant = spec.series.size();
                size_t dominant_count = 0;
                for (const auto& sample: samples) {
                    if (!sample.inside || sample.slice >= 16) {
                        continue;
                    }
                    ++counts[sample.slice];
                    if (counts[sample.slice] > dominant_count ||
                        (counts[sample.slice] == dominant_count &&
                         sample.slice == (selected.value_or(spec.series.size())))) {
                        dominant = sample.slice;
                        dominant_count = counts[sample.slice];
                    }
                }
                if (dominant >= spec.series.size() || dominant_count == 0) {
                    continue;
                }
                size_t second = spec.series.size();
                size_t second_count = 0;
                for (size_t i = 0; i < spec.series.size() && i < 16; ++i) {
                    if (i != dominant && counts[i] > second_count) {
                        second = i;
                        second_count = counts[i];
                    }
                }
                unsigned int mask = 0;
                for (int i = 0; i < 4; ++i) {
                    if (samples[i].inside && samples[i].slice == dominant) {
                        mask |= 1U << i;
                    }
                }
                if (mask == 0) {
                    continue;
                }
                static constexpr string_view glyphs[16] = {"",
                                                           "\xe2\x96\x98",
                                                           "\xe2\x96\x9d",
                                                           "\xe2\x96\x80",
                                                           "\xe2\x96\x96",
                                                           "\xe2\x96\x8c",
                                                           "\xe2\x96\x9e",
                                                           "\xe2\x96\x9b",
                                                           "\xe2\x96\x97",
                                                           "\xe2\x96\x9a",
                                                           "\xe2\x96\x90",
                                                           "\xe2\x96\x9c",
                                                           "\xe2\x96\x84",
                                                           "\xe2\x96\x99",
                                                           "\xe2\x96\x9f",
                                                           "\xe2\x96\x88"};
                _NEFORCE color c = series_color_at(t, spec, dominant);
                const bool is_selected = selected.has_value() && selected.value() == dominant;
                if (is_selected) {
                    c = lighten(c, 0.25);
                }
                style s;
                s.fg = c;
                if (second < spec.series.size()) {
                    s.bg = series_color_at(t, spec, second);
                } else {
                    s.bg = t.get(theme_role::background);
                }
                s.bold = is_selected;
                painter.put(x, y, glyphs[mask], s);
            }
        }
        if (is_donut && outer >= 4.0) {
            const string center = format_value(total, max(1.0, total / 5.0));
            const int width = display_width(center);
            if (width + 2 <= static_cast<int>(inner * 2.0)) {
                style s;
                s.fg = t.get(theme_role::text);
                s.bold = true;
                painter.text(static_cast<int>(geom.pie_cx) - width / 2, static_cast<int>(geom.pie_cy), center.view(),
                             s);
            }
        }
        if (selected.has_value() && selected.value() < spec.series.size()) {
            const size_t slice = selected.value();
            const double slice_value = max(0.0, pie_slice_value(spec.series[slice]));
            if (slice_value > 0.0 && total > 0.0) {
                const double begin = (slice == 0 ? 0.0 : cumulative[slice - 1]) * chart_pi * 2.0;
                const double end = cumulative[slice] * chart_pi * 2.0;
                const double middle = (begin + end) / 2.0;
                style guide_style;
                guide_style.fg = t.get(theme_role::accent);
                guide_style.bold = true;
                const int steps = max(8, static_cast<int>(outer * 2.0));
                for (int step = 0; step <= steps; ++step) {
                    const double ratio = static_cast<double>(step) / static_cast<double>(steps);
                    const double radius = inner + (outer - inner) * ratio;
                    const int gx = static_cast<int>(
                            round(geom.pie_cx + static_cast<double>(cosine(static_cast<decimal_t>(middle))) * radius));
                    const int gy = static_cast<int>(round(
                            geom.pie_cy + static_cast<double>(sine(static_cast<decimal_t>(middle))) * radius / 2.0));
                    painter.put(gx, gy, "\xc2\xb7", guide_style);
                }
                const int mx = static_cast<int>(
                        round(geom.pie_cx + static_cast<double>(cosine(static_cast<decimal_t>(middle))) * outer));
                const int my = static_cast<int>(
                        round(geom.pie_cy + static_cast<double>(sine(static_cast<decimal_t>(middle))) * outer / 2.0));
                painter.put(mx, my, "\xe2\x97\x86", guide_style);
            }
        }
    }

    void draw_heatmap(canvas_painter& painter, const chart_spec& spec, const chart_geometry& geom, const theme& t,
                      const chart_model& model, const axis_scale& scale, const optional<size_t>& selected,
                      const float eased) {
        if (!geom.has_plot) {
            return;
        }
        const _NEFORCE color high = series_color_at(t, spec, 0);
        const _NEFORCE color low = _NEFORCE color::lerp(t.bg, high, 0.1);
        const double span = max(1e-9, scale.max - scale.min);
        const size_t total_cells = static_cast<size_t>(geom.heat_rows) * static_cast<size_t>(geom.heat_cols);
        const auto visible = static_cast<size_t>(
                ceil(static_cast<double>(total_cells) * static_cast<double>(clamp01(static_cast<double>(eased)))));
        style label_style{};
        label_style.fg = t.get(theme_role::subtle);
        for (int row = 0; row < geom.heat_rows; ++row) {
            for (int col = 0; col < geom.heat_cols; ++col) {
                const size_t drawn =
                        static_cast<size_t>(row) * static_cast<size_t>(geom.heat_cols) + static_cast<size_t>(col);
                if (drawn >= visible) {
                    continue;
                }
                const size_t flat = static_cast<size_t>(row) * model.cols + static_cast<size_t>(col);
                double value = 0.0;
                bool has_value = false;
                if (static_cast<size_t>(row) < spec.series.size() &&
                    static_cast<size_t>(col) < spec.series[static_cast<size_t>(row)].values.size()) {
                    value = spec.series[static_cast<size_t>(row)].values[static_cast<size_t>(col)];
                    has_value = finite_number(value);
                }
                const double ratio = has_value ? clamp01((value - scale.min) / span) : 0.0;
                _NEFORCE color c = _NEFORCE color::lerp(low, high, ratio);
                const bool is_selected = selected.has_value() && selected.value() == flat;
                if (is_selected) {
                    c = lighten(c, 0.35);
                }
                style s;
                s.fg = c;
                s.bold = is_selected;
                const int x = geom.heat_x + col * geom.heat_cell_w;
                const int y = geom.heat_y + row * geom.heat_cell_h;
                for (int dy = 0; dy < geom.heat_cell_h && y + dy < geom.heat_y + geom.heat_rows * geom.heat_cell_h;
                     ++dy) {
                    for (int dx = 0; dx < geom.heat_cell_w; ++dx) {
                        painter.put(x + dx, y + dy, "\xe2\x96\x88", s);
                    }
                }
            }
            if (static_cast<size_t>(row) < spec.series.size() && geom.plot_w > 0) {
                const string label =
                        truncate_text(spec.series[static_cast<size_t>(row)].name.view(), max(1, geom.plot_x - 2));
                if (!label.empty()) {
                    style row_style = label_style;
                    if (selected.has_value() && model.cols > 0 &&
                        static_cast<size_t>(row) == selected.value() / model.cols) {
                        row_style.fg = t.get(theme_role::accent);
                        row_style.bold = true;
                    }
                    const int y = geom.heat_y + row * geom.heat_cell_h + geom.heat_cell_h / 2;
                    painter.text_right(0, y, max(0, geom.plot_x - 1), label.view(), row_style);
                }
            }
        }
        if (geom.show_x_labels && geom.x_label_y >= 0 && !spec.labels.empty()) {
            int last_end = geom.heat_x - 2;
            const int grid_right = geom.heat_x + geom.heat_cols * geom.heat_cell_w;
            for (size_t col = 0; col < spec.labels.size() && compare_less(col, geom.heat_cols); ++col) {
                const string label = truncate_text(spec.labels[col].view(), 10);
                if (label.empty()) {
                    continue;
                }
                const int width = display_width(label);
                int x = geom.heat_x + static_cast<int>(col) * geom.heat_cell_w + geom.heat_cell_w / 2 - width / 2;
                x = max(geom.heat_x, min(x, grid_right - width));
                if (x <= last_end) {
                    continue;
                }
                style col_style = label_style;
                if (selected.has_value() && model.cols > 0 && col == selected.value() % model.cols) {
                    col_style.fg = t.get(theme_role::accent);
                    col_style.bold = true;
                }
                painter.text(x, geom.x_label_y, label.view(), col_style);
                last_end = x + width;
            }
        }
        draw_intensity_legend(painter, spec, geom, t, scale);
    }

    struct radar_layout {
        double center_x = 0.0;
        double center_y = 0.0;
        double radius = 1.0;
    };

    radar_layout radar_geometry(const chart_geometry& geom) {
        radar_layout layout;
        layout.center_x = static_cast<double>(geom.plot_x) + static_cast<double>(geom.plot_w) / 2.0;
        layout.center_y = static_cast<double>(geom.plot_y) + static_cast<double>(geom.plot_h) / 2.0;
        layout.radius =
                max(1.0, min((static_cast<double>(geom.plot_w) - 12.0) / 2.0, static_cast<double>(geom.plot_h) - 3.0));
        return layout;
    }

    void draw_radar(canvas_painter& painter, const chart_spec& spec, const chart_geometry& geom, const theme& t,
                    const chart_model& model, const axis_scale& scale, const optional<size_t>& selected,
                    const float eased) {
        if (!geom.has_plot || model.spokes == 0) {
            return;
        }
        const radar_layout layout = radar_geometry(geom);
        const size_t spokes = model.spokes;
        auto vertex = [&](const size_t index, const double ratio) {
            const double angle =
                    -chart_pi / 2.0 + static_cast<double>(index) * chart_pi * 2.0 / static_cast<double>(spokes);
            return pair<double, double>{layout.center_x + static_cast<double>(cosine(static_cast<decimal_t>(angle))) *
                                                                  layout.radius * ratio,
                                        layout.center_y + static_cast<double>(sine(static_cast<decimal_t>(angle))) *
                                                                  layout.radius * ratio / 2.0};
        };
        braille_canvas canvas(geom.plot_w, geom.plot_h);
        style ring_style;
        ring_style.fg = grid_color(t, spec);
        ring_style.dim = true;
        for (int ring = 1; ring <= 4; ++ring) {
            const double ratio = static_cast<double>(ring) / 4.0;
            for (size_t i = 0; i < spokes; ++i) {
                const auto a = vertex(i, ratio);
                const auto b = vertex((i + 1) % spokes, ratio);
                canvas.line(a.first - geom.plot_x, a.second - geom.plot_y, b.first - geom.plot_x,
                            b.second - geom.plot_y);
            }
        }
        for (size_t i = 0; i < spokes; ++i) {
            const auto a = vertex(i, 1.0);
            canvas.line(layout.center_x - geom.plot_x, layout.center_y - geom.plot_y, a.first - geom.plot_x,
                        a.second - geom.plot_y);
        }
        paint_braille(painter, canvas, geom.plot_x, geom.plot_y, ring_style.fg.value());

        const double span = max(1e-9, scale.max - scale.min);
        for (size_t j = 0; j < spec.series.size(); ++j) {
            const auto& series = spec.series[j];
            bool any_finite = false;
            for (const double candidate: series.values) {
                if (finite_number(candidate)) {
                    any_finite = true;
                    break;
                }
            }
            if (!any_finite) {
                continue;
            }
            canvas.clear();
            bool first = true;
            double first_x = 0.0;
            double first_y = 0.0;
            double last_x = 0.0;
            double last_y = 0.0;
            for (size_t i = 0; i < spokes; ++i) {
                double value = 0.0;
                if (i < series.values.size() && finite_number(series.values[i])) {
                    value = series.values[i];
                }
                const double ratio = clamp01((value - scale.min) / span) * static_cast<double>(eased);
                const auto point = vertex(i, ratio);
                if (first) {
                    first_x = point.first;
                    first_y = point.second;
                    first = false;
                } else {
                    canvas.line(last_x - geom.plot_x, last_y - geom.plot_y, point.first - geom.plot_x,
                                point.second - geom.plot_y);
                }
                last_x = point.first;
                last_y = point.second;
            }
            if (!first) {
                canvas.line(last_x - geom.plot_x, last_y - geom.plot_y, first_x - geom.plot_x, first_y - geom.plot_y);
                paint_braille(painter, canvas, geom.plot_x, geom.plot_y, chart_series_color(t, series, j));
            }
        }

        style label_style;
        label_style.fg = t.get(theme_role::subtle);
        const int label_step = spokes > 12 ? static_cast<int>((spokes + 11) / 12) : 1;
        for (size_t i = 0; i < spokes; i += static_cast<size_t>(label_step)) {
            string label = i < spec.labels.size() ? spec.labels[i] : string();
            if (label.empty()) {
                label = to_string(static_cast<int>(i));
            }
            const auto point = vertex(i, 1.0);
            const double angle =
                    -chart_pi / 2.0 + static_cast<double>(i) * chart_pi * 2.0 / static_cast<double>(spokes);
            const auto cos_angle = static_cast<double>(cosine(static_cast<decimal_t>(angle)));
            const auto sin_angle = static_cast<double>(sine(static_cast<decimal_t>(angle)));
            int x = 0;
            int width = 0;
            int y = static_cast<int>(round(point.second));
            if (cos_angle > 0.3) {
                x = static_cast<int>(round(point.first)) + 2;
                width = geom.plot_x + geom.plot_w - x;
            } else if (cos_angle < -0.3) {
                const int end = static_cast<int>(round(point.first)) - 2;
                x = geom.plot_x;
                width = max(0, end - x);
            } else {
                const int label_w = min(12, display_width(label));
                x = static_cast<int>(round(point.first)) - label_w / 2;
                x = max(geom.plot_x, min(x, geom.plot_x + geom.plot_w - label_w));
                width = label_w;
                y += sin_angle > 0.0 ? 1 : -1;
            }
            if (width <= 0 || x < 0 || x >= geom.w) {
                continue;
            }
            const string shown = truncate_text(label.view(), width);
            y = max(geom.plot_y, min(y, geom.plot_y + geom.plot_h - 1));
            style spoke_style = label_style;
            if (selected.has_value() && selected.value() == i) {
                spoke_style.fg = t.get(theme_role::accent);
                spoke_style.bold = true;
            }
            painter.text(x, y, shown.view(), spoke_style);
        }

        if (selected.has_value() && selected.value() < spokes && eased > 0.0F) {
            const auto outer = vertex(selected.value(), 1.0);
            braille_canvas cross_canvas(geom.plot_w, geom.plot_h);
            cross_canvas.line(layout.center_x - geom.plot_x, layout.center_y - geom.plot_y, outer.first - geom.plot_x,
                              outer.second - geom.plot_y);
            paint_braille(painter, cross_canvas, geom.plot_x, geom.plot_y, t.get(theme_role::accent), true);
            for (size_t j = 0; j < spec.series.size(); ++j) {
                const auto& series = spec.series[j];
                if (selected.value() >= series.values.size() || !finite_number(series.values[selected.value()])) {
                    continue;
                }
                const double ratio =
                        clamp01((series.values[selected.value()] - scale.min) / span) * static_cast<double>(eased);
                const auto point = vertex(selected.value(), ratio);
                style marker_style;
                marker_style.fg = lighten(chart_series_color(t, series, j), 0.2);
                marker_style.bold = true;
                painter.put(static_cast<int>(round(point.first)), static_cast<int>(round(point.second)), "\xe2\x97\x8f",
                            marker_style);
            }
            (void) outer;
        }
    }

    void draw_empty_state(canvas_painter& painter, const chart_spec& spec, const chart_geometry& geom, const theme& t) {
        if (!geom.has_plot) {
            return;
        }
        const string empty = "no data";
        style s;
        s.fg = t.get(theme_role::subtle);
        s.dim = true;
        const int x = geom.plot_x + max(0, (geom.plot_w - display_width(empty)) / 2);
        const int y = geom.plot_y + geom.plot_h / 2;
        painter.text(x, y, empty.view(), s);
        (void) spec;
    }

    void draw_chart(canvas_painter& painter, const chart_spec& spec, const bool focused) {
        if (painter.width() <= 0 || painter.height() <= 0) {
            return;
        }
        const chart_model model = build_model(spec);
        const axis_scale scale = make_axis_scale(spec, model);
        const chart_geometry geom = compute_geometry(spec, model, scale, painter.width(), painter.height());
        const theme& t = painter.theme_ref();
        const float eased = ease_progress(spec.progress, spec.easing);
        optional<size_t> selected;
        if (spec.selected_index.has_value() && spec.selected_index.value() < model.selectable) {
            selected = spec.selected_index.value();
        }
        draw_title(painter, spec, geom, t);
        switch (spec.kind) {
            case chart_kind::bar:
            case chart_kind::histogram: {
                draw_grid_and_axes(painter, spec, geom, model, scale, t);
                if (model.categories == 0 || !model.any_finite) {
                    draw_empty_state(painter, spec, geom, t);
                } else {
                    draw_bars(painter, spec, geom, model, scale, t, selected, eased);
                }
                draw_x_labels(painter, spec, geom, model, t, selected);
                break;
            }
            case chart_kind::area: {
                draw_grid_and_axes(painter, spec, geom, model, scale, t);
                if (model.categories == 0 || !model.any_finite) {
                    draw_empty_state(painter, spec, geom, t);
                } else {
                    draw_area(painter, spec, geom, model, scale, t, selected, eased);
                }
                draw_x_labels(painter, spec, geom, model, t, selected);
                break;
            }
            case chart_kind::line:
            case chart_kind::scatter: {
                draw_grid_and_axes(painter, spec, geom, model, scale, t);
                if (model.categories == 0 || !model.any_finite) {
                    draw_empty_state(painter, spec, geom, t);
                } else if (spec.kind == chart_kind::line) {
                    draw_line_like(painter, spec, geom, model, scale, t, eased);
                } else {
                    draw_scatter(painter, spec, geom, model, scale, t, eased);
                }
                draw_x_labels(painter, spec, geom, model, t, selected);
                break;
            }
            case chart_kind::pie:
            case chart_kind::donut: {
                draw_pie(painter, spec, geom, t, model, selected, eased);
                break;
            }
            case chart_kind::heatmap: {
                if (model.rows == 0 || model.cols == 0 || !model.any_finite) {
                    draw_empty_state(painter, spec, geom, t);
                } else {
                    draw_heatmap(painter, spec, geom, t, model, scale, selected, eased);
                }
                break;
            }
            case chart_kind::radar: {
                if (model.spokes == 0 || !model.any_finite) {
                    draw_empty_state(painter, spec, geom, t);
                } else {
                    draw_radar(painter, spec, geom, t, model, scale, selected, eased);
                }
                break;
            }
        }
        if (spec.kind != chart_kind::heatmap) {
            if (geom.legend_right) {
                draw_legend_right(painter, spec, geom, t, model, selected, model.positive_total);
            } else if (geom.legend_bottom) {
                draw_legend_bottom(painter, spec, geom, t, selected, model.positive_total);
            }
        }
        draw_selection_overlay(painter, spec, geom, model, scale, t, selected, focused);
        draw_status(painter, spec, geom, selected, focused, t, model, scale);
    }
} // namespace


float chart_eased_progress(const float progress, const chart_easing easing) noexcept {
    return ease_progress(progress, easing);
}

_NEFORCE color chart_series_color(const theme& t, const chart_series& series, const size_t index) {
    if (series.color.has_value()) {
        return series.color.value();
    }
    if (series.role.has_value()) {
        return t.get(series.role.value());
    }
    const _NEFORCE color palette[8] = {t.get(theme_role::accent),
                                       t.get(theme_role::success),
                                       t.get(theme_role::warning),
                                       t.get(theme_role::error),
                                       t.get(theme_role::suggestion),
                                       lighten(t.get(theme_role::accent), 0.3),
                                       lighten(t.get(theme_role::success), 0.3),
                                       lighten(t.get(theme_role::warning), 0.3)};
    return palette[index % 8];
}

size_t chart_point_count(const chart_spec& spec) noexcept { return build_model(spec).selectable; }

vector<chart_point> chart_points_at(const chart_spec& spec, const size_t index) {
    vector<chart_point> points;
    const chart_model model = build_model(spec);
    if (index >= model.selectable) {
        return points;
    }
    switch (spec.kind) {
        case chart_kind::pie:
        case chart_kind::donut: {
            if (index >= spec.series.size()) {
                return points;
            }
            const auto& series = spec.series[index];
            chart_point point;
            point.series_index = index;
            point.point_index = 0;
            point.series_name = series.name;
            point.label = index < spec.labels.size() ? spec.labels[index] : series.name;
            point.value = pie_slice_value(series);
            points.push_back(move(point));
            break;
        }
        case chart_kind::heatmap: {
            if (model.cols == 0) {
                return points;
            }
            const size_t row = index / model.cols;
            const size_t col = index % model.cols;
            if (row >= spec.series.size()) {
                return points;
            }
            const auto& series = spec.series[row];
            if (col >= series.values.size() || !finite_number(series.values[col])) {
                return points;
            }
            chart_point point;
            point.series_index = row;
            point.point_index = col;
            point.series_name = series.name;
            point.label = col < spec.labels.size() ? spec.labels[col] : string();
            point.value = series.values[col];
            points.push_back(move(point));
            break;
        }
        default: {
            const size_t count = spec.kind == chart_kind::radar ? model.spokes : model.categories;
            if (index >= count) {
                return points;
            }
            for (size_t j = 0; j < spec.series.size(); ++j) {
                const auto& series = spec.series[j];
                if (index >= series.values.size() || !finite_number(series.values[index])) {
                    continue;
                }
                chart_point point;
                point.series_index = j;
                point.point_index = index;
                point.series_name = series.name;
                point.label = index < spec.labels.size() ? spec.labels[index] : string();
                point.value = series.values[index];
                points.push_back(move(point));
            }
            break;
        }
    }
    return points;
}

element chart(const chart_spec& spec) {
    chart_spec view = spec;
    view.progress = static_cast<float>(clamp01(static_cast<double>(spec.progress)));
    style s;
    s.width = style::size_hint{style::size_hint::fixed, spec.width > 0 ? spec.width : default_chart_width};
    s.height = style::size_hint{style::size_hint::fixed, spec.height > 0 ? spec.height : default_chart_height};
    return element::canvas([view](canvas_painter& painter) { draw_chart(painter, view, false); }, move(s));
}

chart_component::chart_component(chart_spec spec) :
spec_(move(spec)) {
    progress_ = static_cast<float>(clamp01(static_cast<double>(spec_.progress)));
    if (spec_.selected_index.has_value()) {
        const size_t count = chart_point_count(spec_);
        if (spec_.selected_index.value() < count) {
            selected_ = spec_.selected_index.value();
        }
    }
}

void chart_component::setup() { rect_ = &create_state<layout_rect>(layout_rect{}); }

void chart_component::clamp_selection() {
    const size_t count = chart_point_count(spec_);
    if (count == 0) {
        selected_ = none;
        return;
    }
    if (selected_.has_value() && selected_.value() >= count) {
        selected_ = count - 1;
    }
}

void chart_component::set_spec(chart_spec spec) {
    spec_ = move(spec);
    progress_ = static_cast<float>(clamp01(static_cast<double>(spec_.progress)));
    animating_ = false;
    selected_ = none;
    if (spec_.selected_index.has_value() && spec_.selected_index.value() < chart_point_count(spec_)) {
        selected_ = spec_.selected_index.value();
    }
    clamp_selection();
    schedule_render();
}

void chart_component::set_progress(const float progress) {
    const auto value = static_cast<float>(clamp01(static_cast<double>(progress)));
    animating_ = false;
    if (value == progress_) {
        return;
    }
    progress_ = value;
    schedule_render();
}

void chart_component::animate_progress(const float target, const milliseconds duration) {
    const auto to = static_cast<float>(clamp01(static_cast<double>(target)));
    const auto millis = static_cast<int64_t>(duration.count());
    if (millis <= 0) {
        set_progress(to);
        return;
    }
    animation_from_ = progress_;
    animation_to_ = to;
    animation_elapsed_ = 0;
    animation_duration_ = millis;
    animating_ = true;
    schedule_render();
}

size_t chart_component::selection_count() const noexcept { return chart_point_count(spec_); }

bool chart_component::select(const size_t index) {
    const size_t count = selection_count();
    if (count == 0) {
        return clear_selection();
    }
    const size_t clamped = min(index, count - 1);
    if (selected_.has_value() && selected_.value() == clamped) {
        return false;
    }
    selected_ = clamped;
    schedule_render();
    return true;
}

bool chart_component::select_next() {
    const size_t count = selection_count();
    if (count == 0) {
        return false;
    }
    if (!selected_.has_value()) {
        return select(0);
    }
    if (selected_.value() + 1 >= count) {
        return false;
    }
    return select(selected_.value() + 1);
}

bool chart_component::select_previous() {
    const size_t count = selection_count();
    if (count == 0) {
        return false;
    }
    if (!selected_.has_value()) {
        return select(count - 1);
    }
    if (selected_.value() == 0) {
        return false;
    }
    return select(selected_.value() - 1);
}

bool chart_component::select_up() {
    if (spec_.kind != chart_kind::heatmap) {
        return select_previous();
    }
    const chart_model model = build_model(spec_);
    if (model.cols == 0 || !selected_.has_value()) {
        return select(0);
    }
    if (selected_.value() < model.cols) {
        return false;
    }
    return select(selected_.value() - model.cols);
}

bool chart_component::select_down() {
    if (spec_.kind != chart_kind::heatmap) {
        return select_next();
    }
    const chart_model model = build_model(spec_);
    if (model.cols == 0 || !selected_.has_value()) {
        return select(0);
    }
    if (selected_.value() + model.cols >= selection_count()) {
        return false;
    }
    return select(selected_.value() + model.cols);
}

bool chart_component::clear_selection() {
    if (!selected_.has_value()) {
        return false;
    }
    selected_ = none;
    schedule_render();
    return true;
}

optional<chart_point> chart_component::selected_point() const {
    if (!selected_.has_value()) {
        return none;
    }
    const auto points = chart_points_at(spec_, selected_.value());
    if (points.empty()) {
        return none;
    }
    return points[0];
}

vector<chart_point> chart_component::selected_points() const {
    if (!selected_.has_value()) {
        return {};
    }
    return chart_points_at(spec_, selected_.value());
}

optional<size_t> chart_component::hit_test(const int x, const int y) const {
    if (rect_ == nullptr) {
        return none;
    }
    const layout_rect box = rect_->value();
    if (box.w <= 0 || box.h <= 0 || x < 0 || y < 0 || x >= box.w || y >= box.h) {
        return none;
    }
    const chart_model model = build_model(spec_);
    if (model.selectable == 0) {
        return none;
    }
    const axis_scale scale = make_axis_scale(spec_, model);
    const chart_geometry geom = compute_geometry(spec_, model, scale, box.w, box.h);
    if (pie_kind(spec_.kind)) {
        const double dx = static_cast<double>(x) + 0.5 - geom.pie_cx;
        const double dy = static_cast<double>(y) + 0.5 - geom.pie_cy;
        const auto radius = static_cast<double>(square_root(static_cast<decimal_t>(dx * dx + 4.0 * dy * dy)));
        if (radius > geom.pie_radius || radius < geom.pie_inner) {
            return none;
        }
        const double total = model.positive_total;
        if (total <= 0.0) {
            return none;
        }
        const double angle = clock_angle(dx, dy);
        double running = 0.0;
        for (size_t i = 0; i < spec_.series.size(); ++i) {
            running += max(0.0, pie_slice_value(spec_.series[i])) / total;
            if (angle <= running * chart_pi * 2.0 + 1e-9) {
                return i;
            }
        }
        return spec_.series.empty() ? none : optional<size_t>(spec_.series.size() - 1);
    }
    if (spec_.kind == chart_kind::heatmap) {
        if (!geom.has_plot || x < geom.heat_x || y < geom.heat_y) {
            return none;
        }
        const int col = (x - geom.heat_x) / geom.heat_cell_w;
        const int row = (y - geom.heat_y) / geom.heat_cell_h;
        if (col < 0 || row < 0 || col >= geom.heat_cols || row >= geom.heat_rows) {
            return none;
        }
        return static_cast<size_t>(row) * static_cast<size_t>(model.cols) + static_cast<size_t>(col);
    }
    if (spec_.kind == chart_kind::radar) {
        if (!geom.has_plot || x < geom.plot_x || x >= geom.plot_x + geom.plot_w || y < geom.plot_y ||
            y >= geom.plot_y + geom.plot_h) {
            return none;
        }
        const radar_layout layout = radar_geometry(geom);
        const double dx = static_cast<double>(x) + 0.5 - layout.center_x;
        const double dy = static_cast<double>(y) + 0.5 - layout.center_y;
        if (absolute(dx) + absolute(dy) < 0.5) {
            return none;
        }
        const double angle = clock_angle(dx, dy);
        const size_t spokes = model.spokes;
        if (spokes == 0) {
            return none;
        }
        auto spoke = static_cast<size_t>(round(angle / (chart_pi * 2.0) * static_cast<double>(spokes)));
        spoke %= spokes;
        return spoke;
    }
    if (!geom.has_plot || x < geom.plot_x || x >= geom.plot_x + geom.plot_w || y < geom.plot_y ||
        y >= geom.plot_y + geom.plot_h) {
        return none;
    }
    return static_cast<size_t>(category_at_x(spec_, geom, model.categories, x));
}

layout_rect chart_component::area() const noexcept {
    if (rect_ == nullptr) {
        return layout_rect{};
    }
    return rect_->value();
}

element chart_component::render() {
    chart_spec view = spec_;
    view.progress = progress_;
    view.selected_index = selected_;
    const bool is_focused = focused();
    style s;
    s.width = style::size_hint{style::size_hint::fixed, spec_.width > 0 ? spec_.width : default_chart_width};
    s.height = style::size_hint{style::size_hint::fixed, spec_.height > 0 ? spec_.height : default_chart_height};
    element result = element::canvas(
            [view, is_focused](canvas_painter& painter) { draw_chart(painter, view, is_focused); }, move(s));
    result.set_owner(this);
    if (rect_ != nullptr) {
        result.with_rect_state(rect_);
    }
    return result;
}

bool chart_component::on_key(const key_event& e) {
    using kind = key_event::type;
    switch (e.key) {
        case kind::left:
            select_previous();
            return true;
        case kind::right:
            select_next();
            return true;
        case kind::up:
            select_up();
            return true;
        case kind::down:
            select_down();
            return true;
        case kind::home:
            select(0);
            return true;
        case kind::end: {
            const size_t count = selection_count();
            if (count > 0) {
                select(count - 1);
            }
            return true;
        }
        case kind::escape:
            return clear_selection();
        default:
            return false;
    }
}

bool chart_component::on_mouse(const mouse_event& e) {
    if (e.action != mouse_action::press) {
        return false;
    }
    const layout_rect box = area();
    if (box.w <= 0 || box.h <= 0) {
        return false;
    }
    const auto hit = hit_test(e.x - box.x, e.y - box.y);
    if (!hit.has_value()) {
        return false;
    }
    select(hit.value());
    return true;
}

void chart_component::on_animation(const int64_t delta) {
    if (!animating_) {
        return;
    }
    if (delta > 0) {
        animation_elapsed_ += delta;
    }
    if (animation_duration_ <= 0 || animation_elapsed_ >= animation_duration_) {
        progress_ = animation_to_;
        animating_ = false;
    } else {
        const auto ratio =
                static_cast<float>(static_cast<double>(animation_elapsed_) / static_cast<double>(animation_duration_));
        progress_ = animation_from_ + (animation_to_ - animation_from_) * ratio;
    }
    progress_ = static_cast<float>(clamp01(static_cast<double>(progress_)));
    schedule_render();
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
