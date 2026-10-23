#include <NeForce/core/numeric/math.hpp>
#include <NeForce/tui/component/motion.hpp>
#include <NeForce/tui/dom/text_layout.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    constexpr string_view braille[] = {"\xe2\xa0\x8b", "\xe2\xa0\x99", "\xe2\xa0\xb9", "\xe2\xa0\xb8", "\xe2\xa0\xbc",
                                       "\xe2\xa0\xb4", "\xe2\xa0\xa6", "\xe2\xa0\xa7", "\xe2\xa0\x87", "\xe2\xa0\x8f"};
    constexpr string_view dots[] = {"\xc2\xb7", "\xe2\x80\xa2", "\xe2\x97\x8f", "\xe2\x80\xa2"};
    constexpr string_view line[] = {"|", "/", "-", "\\"};
    constexpr string_view clock[] = {"\xe2\x97\xb7", "\xe2\x97\xb6", "\xe2\x97\xb5", "\xe2\x97\xb4"};

    constexpr spinner_frame_set spinners[] = {
            {"braille", braille, 10, 80}, {"dots", dots, 4, 140}, {"line", line, 4, 120}, {"clock", clock, 4, 120}};
} // namespace


frame_clock& frame_clock::instance() {
    static frame_clock g_frame_clock;
    return g_frame_clock;
}

void frame_clock::tick(const int64_t delta_ms) noexcept {
    if (paused_) {
        return;
    }
    now_ms_ += delta_ms;
    ++frame_index_;
}

void frame_clock::reset() noexcept {
    now_ms_ = 0;
    frame_index_ = 0;
}

color shimmer_color(const int column, const int width, const int64_t time_ms, const _NEFORCE color base,
                    const _NEFORCE color highlight, const int step_ms, const int window) {
    if (width <= 0 || window <= 0 || step_ms <= 0) {
        return base;
    }
    const int64_t cycle = width + 2 * window;
    const int64_t step = time_ms / step_ms;
    const int64_t position = (step % cycle) - window;
    const int64_t start = position;
    if (column < start || column >= start + window) {
        return base;
    }
    const decimal_t phase = static_cast<decimal_t>(time_ms) / (static_cast<decimal_t>(step_ms) * 2.0);
    const decimal_t opacity = (sine(phase) + 1.0) / 2.0;
    return color::lerp(base, highlight, static_cast<double>(opacity));
}

vector<color> shimmer_sweep(const int width, const int64_t time_ms, const _NEFORCE color base,
                            const _NEFORCE color highlight, const int step_ms, const int window) {
    vector<color> result;
    if (width <= 0) {
        return result;
    }
    result.reserve(static_cast<size_t>(width));
    for (int column = 0; column < width; ++column) {
        result.push_back(shimmer_color(column, width, time_ms, base, highlight, step_ms, window));
    }
    return result;
}

reveal::reveal(const size_t arrived) noexcept :
visible_(arrived) {}

void reveal::reset(const size_t arrived) noexcept { visible_ = arrived; }

size_t reveal::advance(const size_t arrived) noexcept {
    if (visible_ >= arrived) {
        visible_ = arrived;
        return visible_;
    }
    const size_t backlog = arrived - visible_;
    const size_t step = max<size_t>(3, (backlog + 7) / 8);
    visible_ = min(arrived, visible_ + step);
    return visible_;
}

const spinner_frame_set* find_spinner(const string_view name) noexcept {
    for (const auto& set: spinners) {
        if (set.name == name) {
            return &set;
        }
    }
    return nullptr;
}

string_view spinner_frame(const spinner_frame_set& set, const int64_t time_ms) noexcept {
    if (set.count == 0 || set.interval_ms <= 0) {
        return string_view{};
    }
    const int64_t index = (time_ms / set.interval_ms) % static_cast<int64_t>(set.count);
    return string_view{set.frames[static_cast<size_t>(index)]};
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
