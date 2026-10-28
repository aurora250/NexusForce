#include "NeForce/tui/chart/canvas_chart.hpp"
#include "NeForce/core/numeric/math.hpp"
#include "NeForce/tui/chart/chart_geometry.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__
namespace {
    constexpr string_view braille_up[25] = {
            " ", "⢀", "⢠", "⢰", "⢸", "⡀", "⣀", "⣠", "⣰", "⣸", "⡄", "⣄", "⣤",
            "⣴", "⣼", "⡆", "⣆", "⣦", "⣶", "⣾", "⡇", "⣇", "⣧", "⣷", "⣿",
    };

    constexpr string_view braille_down[25] = {
            " ", "⠈", "⠘", "⠸", "⢸", "⠁", "⠉", "⠙", "⠹", "⢹", "⠃", "⠋", "⠛",
            "⠻", "⢻", "⠇", "⠏", "⠟", "⠿", "⢿", "⡇", "⡏", "⡟", "⡿", "⣿",
    };

    constexpr string_view block_up[25] = {
            " ", "▗", "▗", "▐", "▐", "▖", "▄", "▄", "▟", "▟", "▖", "▄", "▄",
            "▟", "▟", "▌", "▙", "▙", "█", "█", "▌", "▙", "▙", "█", "█",
    };

    constexpr string_view block_down[25] = {
            " ", "▝", "▝", "▐", "▐", "▘", "▀", "▀", "▜", "▜", "▘", "▀", "▀",
            "▜", "▜", "▌", "▛", "▛", "█", "█", "▌", "▛", "▛", "█", "█",
    };

    constexpr string_view tty_glyph[25] = {
            " ", "░", "░", "▒", "▒", "░", "░", "▒", "▒", "█", "░", "▒", "▒",
            "▒", "█", "▒", "▒", "▒", "█", "█", "▒", "█", "█", "█", "█",
    };

    string_view graph_glyph(const string& symbol, const int left, const int right, const bool mirrored) noexcept {
        const int index = clamp(left, 0, 4) * 5 + clamp(right, 0, 4);
        if (symbol == "block") {
            return mirrored ? block_down[index] : block_up[index];
        }
        if (symbol == "tty") {
            return tty_glyph[index];
        }
        return mirrored ? braille_down[index] : braille_up[index];
    }

    _NEFORCE color cpu_gradient(const canvas_chart_style& style, double level) noexcept {
        level = clamp(level, 0.0, 1.0);
        const _NEFORCE color& start = level <= 0.5 ? style.cpu_start : style.cpu_mid;
        const _NEFORCE color& end = level <= 0.5 ? style.cpu_mid : style.cpu_end;
        const double fraction = level <= 0.5 ? level * 2.0 : (level - 0.5) * 2.0;
        const auto mix = [fraction](const int a, const int b) {
            return static_cast<int>(round(static_cast<double>(a) + (static_cast<double>(b) - a) * fraction));
        };
        return _NEFORCE color{mix(start.R(), end.R()), mix(start.G(), end.G()), mix(start.B(), end.B())};
    }

    style glyph_style(_NEFORCE color tint) {
        style value;
        value.fg = tint;
        return value;
    }
} // namespace


size_t canvas_chart_window::stride(const size_t width) const {
    return max(static_cast<size_t>(1), (samples + width / 2) / max(static_cast<size_t>(1), width));
}

size_t canvas_chart_window::capacity(const size_t width) const { return min(samples, width * stride(width)); }

void canvas_chart_window::project(const deque<double>& values, const size_t width,
                                  vector<canvas_chart_range>& out) const {
    out.clear();
    out.resize(width);
    if (width == 0 || values.empty()) {
        return;
    }
    if (has_time) {
        if (time.timestamps == nullptr || time.periods == nullptr || time.timestamps->empty()) {
            return;
        }
        const auto& times = *time.timestamps;
        const auto& periods = *time.periods;
        const uint64_t step = max(static_cast<uint64_t>(1),
                                  (max(static_cast<uint64_t>(1), time.seconds) * 1'000'000 + width - 1) / width);
        const auto bucket = [step](const double t) {
            return static_cast<uint64_t>(round(max(0.0, t) * 1'000'000.0)) / step;
        };
        const uint64_t end = bucket(times.back());
        const size_t count = min(values.size(), times.size());
        deque<bool> gaps(width, false);
        vector<bool> occupied(width, false);
        bool has_previous = false;
        int64_t previous_x = 0;
        double previous_value = 0.0;
        double previous_time = 0.0;
        double previous_period = 1.0;
        for (size_t i = 0; i < count; ++i) {
            const size_t index = times.size() - count + i;
            const double t = times[index];
            const double period = index < periods.size() ? max(0.001, periods[index]) : 1.0;
            const double value = values[values.size() - count + i];
            const int64_t x = static_cast<int64_t>(width) - 1 - static_cast<int64_t>(end - bucket(t));
            if (!is_finite(value)) {
                has_previous = false;
                if (x >= 0) {
                    gaps[static_cast<size_t>(x)] = true;
                    out[static_cast<size_t>(x)].gap = true;
                }
                continue;
            }
            if (has_previous && x > previous_x && t - previous_time <= 2.5 * max(previous_period, period)) {
                for (int64_t col = max<int64_t>(previous_x + 1, 0); col < x; ++col) {
                    const double interpolated = previous_value + (value - previous_value) *
                                                                         static_cast<double>(col - previous_x) /
                                                                         static_cast<double>(x - previous_x);
                    canvas_chart_range range;
                    range.low = interpolated;
                    range.high = interpolated;
                    range.last = interpolated;
                    range.gap = gaps[static_cast<size_t>(col)];
                    out[static_cast<size_t>(col)] = range;
                    occupied[static_cast<size_t>(col)] = true;
                }
            }
            if (x >= 0) {
                auto& range = out[static_cast<size_t>(x)];
                if (!occupied[static_cast<size_t>(x)]) {
                    range = canvas_chart_range{value, value, value, gaps[static_cast<size_t>(x)]};
                    occupied[static_cast<size_t>(x)] = true;
                } else {
                    range.low = min(range.low, value);
                    range.high = max(range.high, value);
                    range.last = value;
                }
            }
            has_previous = true;
            previous_x = x;
            previous_value = value;
            previous_time = t;
            previous_period = period;
        }
        return;
    }
    const size_t stride_value = stride(width);
    const uint64_t end = max<uint64_t>(end, values.size());
    const uint64_t last_bucket = (end - 1) / stride_value;
    const uint64_t start = end - values.size();
    const size_t skip = values.size() > samples ? values.size() - samples : 0;
    deque<bool> gaps(width, false);
    vector<bool> occupied(width, false);
    for (size_t i = skip; i < values.size(); ++i) {
        const uint64_t offset = last_bucket - (start + i) / stride_value;
        if (offset >= width) {
            continue;
        }
        const size_t x = width - 1 - static_cast<size_t>(offset);
        const double value = values[i];
        if (!is_finite(value)) {
            gaps[x] = true;
            out[x].gap = true;
        } else {
            auto& range = out[x];
            if (!occupied[x]) {
                range = canvas_chart_range{value, value, value, gaps[x]};
                occupied[x] = true;
            } else {
                range.low = min(range.low, value);
                range.high = max(range.high, value);
                range.last = value;
            }
        }
    }
}

canvas_chart_style canvas_chart_style::from_theme(const theme& value, string symbol, const bool gradient) {
    canvas_chart_style style;
    style.panel = value.bg;
    style.graph_text = value.muted;
    style.cpu_start = value.success;
    style.cpu_mid = value.warning;
    style.cpu_end = value.danger;
    style.graph_symbol = move(symbol);
    style.gradient = gradient;
    return style;
}

double canvas_chart_nice_max(const double value) { return chart_nice_max(value); }

void canvas_track(canvas_painter& target, const int x, const int y, const int width, optional<double> value,
                  const double maximum, _NEFORCE color fill, _NEFORCE color track, const bool ascii) {
    if (width <= 0) {
        return;
    }
    style empty_style;
    empty_style.fg = track;
    if (!value.has_value() || !is_finite(*value) || !is_finite(maximum) || maximum <= 0.0) {
        for (int index = 0; index < width; ++index) {
            target.put(x + index, y, ascii ? string_view("-") : string_view("┄"), empty_style);
        }
        return;
    }
    style fill_style;
    fill_style.fg = fill;
    const double units = clamp(*value / maximum, 0.0, 1.0) * static_cast<double>(width);
    const int whole = static_cast<int>(floor(units));
    const int fraction = static_cast<int>(floor((units - whole) * 8.0));
    static constexpr const char* partials[] = {"", "▏", "▎", "▍", "▌", "▋", "▊", "▉"};
    for (int index = 0; index < whole && index < width; ++index) {
        target.put(x + index, y, ascii ? string_view("#") : string_view("█"), fill_style);
    }
    int used = whole;
    if (whole < width && fraction > 0) {
        target.put(x + whole, y, ascii ? string_view("#") : string_view(partials[fraction]), fill_style);
        ++used;
    }
    for (int index = used; index < width; ++index) {
        target.put(x + index, y, ascii ? string_view("-") : string_view("░"), empty_style);
    }
}

void canvas_scrollbar(canvas_painter& target, const int x, const int y, const int height, const size_t content,
                      const size_t viewport, const size_t position, _NEFORCE color thumb, _NEFORCE color track,
                      const bool ascii) {
    if (height <= 0 || viewport == 0 || content <= viewport) {
        return;
    }
    const int thumb_size =
            max(1, static_cast<int>(static_cast<double>(viewport) / static_cast<double>(content) * height));
    const int range = max(1, height - thumb_size);
    const int maximum = static_cast<int>(content - viewport);
    const int thumb_pos = maximum == 0
                                  ? 0
                                  : static_cast<int>(static_cast<double>(min(position, static_cast<size_t>(maximum))) /
                                                     maximum * range);
    style thumb_style;
    thumb_style.fg = thumb;
    style track_style;
    track_style.fg = track;
    for (int row = 0; row < height; ++row) {
        const bool active = row >= thumb_pos && row < thumb_pos + thumb_size;
        target.put(x, y + row,
                   active ? (ascii ? string_view("#") : string_view("█"))
                          : (ascii ? string_view("-") : string_view("░")),
                   active ? thumb_style : track_style);
    }
}

canvas_chart::canvas_chart(canvas_painter& target, canvas_chart_style style) :
target_(target),
style_(move(style)) {}

void canvas_chart::raster(const int x, const int y, const int width, const int height, const chart_spec& spec,
                          const double maximum, const canvas_chart_window& window, const bool logarithmic,
                          const int direction) {
    canvas_chart_window effective = window;
    if (effective.samples == 0 && spec.stream_samples > 0) {
        effective.samples = spec.stream_samples;
        for (const auto& series: spec.series) {
            if (series.stream != nullptr) {
                effective.end = max(effective.end, series.stream->size());
            }
        }
    }
    raster(x, y, width, height, spec.series, maximum, effective, logarithmic, direction);
}

void canvas_chart::raster(int x, int y, int width, int height, const vector<chart_series>& series, double maximum,
                          const canvas_chart_window& window, bool logarithmic, int direction) {
    if (width <= 0 || height <= 0) {
        return;
    }
    const int dot_width = width * 2;
    const int dot_height = height * 4;
    vector<uint8_t> bits(static_cast<size_t>(width * height), 0);
    vector<_NEFORCE color> colors(static_cast<size_t>(width * height), style_.panel);
    const bool mirrored = direction > 0;
    const bool cpu_mode = direction != 0;
    vector<_NEFORCE color> row_colors;
    if (cpu_mode) {
        row_colors.reserve(static_cast<size_t>(height));
        for (int row = 0; row < height; ++row) {
            const int distance = mirrored ? row : height - 1 - row;
            row_colors.push_back(
                    cpu_gradient(style_, static_cast<double>(distance) / static_cast<double>(max(1, height - 1))));
        }
    }
    const double max_value = max(1.0, maximum);
    const auto y_of = [&](const double value) {
        const double fraction = chart_value_fraction(value, 0.0, max_value, logarithmic);
        return chart_row_for_fraction(fraction, 0, dot_height, !mirrored);
    };
    const auto dot = [&](const int px, const int py, const _NEFORCE color& tint) {
        if (px < 0 || py < 0 || px >= dot_width || py >= dot_height) {
            return;
        }
        const size_t uy = static_cast<size_t>(py) / 4U;
        const size_t ux = static_cast<size_t>(px) / 2U;
        const auto index = uy * static_cast<size_t>(width) + ux;
        if (index >= bits.size()) {
            return;
        }
        static constexpr int dots[2][4] = {{0, 1, 2, 6}, {3, 4, 5, 7}};
        bits[index] |= static_cast<uint8_t>(1U << dots[px % 2][py % 4]);
        if (cpu_mode) {
            colors[index] = row_colors[static_cast<size_t>(py) / 4U];
        } else if (style_.gradient && height > 1) {
            // Normalise by the actual cell row (not by py/4 against height-1);
            // otherwise short charts (the overview memory sparklines are only 2-3 rows) reach a fade far
            // beyond the intended maximum and the bottom row collapses to panel colour.
            const double row_level = (static_cast<double>(py) / 4.F) / static_cast<double>(max(1, height - 1));
            const double fade = 0.30 * row_level;
            const auto mix = [fade](const int base, const int accent) {
                return static_cast<int>(
                        round(static_cast<double>(base) + (static_cast<double>(accent) - base) * (1.0 - fade)));
            };
            colors[index] = _NEFORCE color{mix(style_.panel.R(), tint.R()), mix(style_.panel.G(), tint.G()),
                                           mix(style_.panel.B(), tint.B())};
        } else {
            colors[index] = tint;
        }
    };
    const auto line = [&](int x0, int y0, const int x1, const int y1, const _NEFORCE color& tint) {
        const int dx = absolute(x1 - x0);
        const int sx = x0 < x1 ? 1 : -1;
        const int dy = -absolute(y1 - y0);
        const int sy = y0 < y1 ? 1 : -1;
        int error = dx + dy;
        for (;;) {
            dot(x0, y0, tint);
            if (x0 == x1 && y0 == y1) {
                break;
            }
            const int twice = 2 * error;
            if (twice >= dy) {
                error += dy;
                x0 += sx;
            }
            if (twice <= dx) {
                error += dx;
                y0 += sy;
            }
        }
    };
    vector<canvas_chart_range> columns;
    for (const auto& item: series) {
        deque<double> converted;
        const deque<double>* source = item.stream;
        if (source == nullptr && !item.values.empty()) {
            converted.assign(item.values.begin(), item.values.end());
            source = &converted;
        }
        if (source == nullptr) {
            continue;
        }
        const _NEFORCE color tint = item.color.has_value() ? *item.color : style_.cpu_start;
        window.project(*source, static_cast<size_t>(dot_width), columns);
        bool has_previous = false;
        int previous_x = 0;
        int previous_y = 0;
        for (size_t column = 0; column < columns.size(); ++column) {
            const auto& range = columns[column];
            if (!range.gap && range.high == 0.0 && range.low == 0.0 && range.last == 0.0) {
                has_previous = false;
                continue;
            }
            const int end = y_of(range.last);
            if (series.size() == 1) {
                const int top = cpu_mode ? y_of(range.high) : end;
                if (mirrored) {
                    for (int row = 0; row <= top; ++row) {
                        dot(static_cast<int>(column), row, tint);
                    }
                } else {
                    for (int row = top; row < dot_height; ++row) {
                        dot(static_cast<int>(column), row, tint);
                    }
                }
            }
            const int high = y_of(range.high);
            const int low = y_of(range.low);
            for (int row = high; row <= low; ++row) {
                dot(static_cast<int>(column), row, tint);
            }
            if (has_previous && !range.gap) {
                line(previous_x, previous_y, static_cast<int>(column), end, tint);
            }
            dot(static_cast<int>(column), end, tint);
            has_previous = !range.gap;
            previous_x = static_cast<int>(column);
            previous_y = end;
        }
    }
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            const auto index = static_cast<size_t>(row) * static_cast<size_t>(width) + static_cast<size_t>(col);
            if (bits[index] == 0) {
                continue;
            }
            int count = 0;
            for (uint8_t mask = 1; mask != 0; mask = static_cast<uint8_t>(mask << 1)) {
                count += (bits[index] & mask) != 0 ? 1 : 0;
            }
            if (style_.ascii || style_.graph_symbol == "ascii") {
                const char glyph = count >= 5 ? '#' : (count >= 3 ? '+' : (count >= 2 ? '-' : '.'));
                target_.put(x + col, y + row, string_view(&glyph, 1), glyph_style(colors[index]));
            } else {
                int left = 0;
                int right = 0;
                const uint8_t value = bits[index];
                for (const int bit: {0, 1, 2, 6}) {
                    left += (value & static_cast<uint8_t>(1U << bit)) != 0 ? 1 : 0;
                }
                for (const int bit: {3, 4, 5, 7}) {
                    right += (value & static_cast<uint8_t>(1U << bit)) != 0 ? 1 : 0;
                }
                const string_view glyph = graph_glyph(style_.graph_symbol, left, right, mirrored);
                target_.put(x + col, y + row, glyph, glyph_style(colors[index]));
            }
        }
    }
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
