#include <NeForce/tui/dom/gauge.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

element gauge(const float progress, const int width) {
    return gauge_direction(progress, gauge_direction::right, width);
}

element gauge_direction(const float progress, const enum class gauge_direction direction, const int size) {
    const float clamped = (progress < 0.0F) ? 0.0F : ((progress > 1.0F) ? 1.0F : progress);
    const int filled = static_cast<int>(clamped) * size;
    const int empty = size - filled;

    const string fill_char = "█";
    const string empty_char = "░";

    string bar;
    bar.reserve(static_cast<size_t>(size) * 3);

    switch (direction) {
        case gauge_direction::right:
            for (int i = 0; i < filled; ++i) {
                bar += fill_char;
            }
            for (int i = 0; i < empty; ++i) {
                bar += empty_char;
            }
            break;
        case gauge_direction::left:
            for (int i = 0; i < empty; ++i) {
                bar += empty_char;
            }
            for (int i = 0; i < filled; ++i) {
                bar += fill_char;
            }
            break;
        case gauge_direction::down: {
            elements vert_bars;
            for (int i = 0; i < filled; ++i) {
                vert_bars.push_back(element::text(fill_char));
            }
            for (int i = 0; i < empty; ++i) {
                vert_bars.push_back(element::text(empty_char));
            }
            return element::vbox(move(vert_bars));
        }
        case gauge_direction::up: {
            elements vert_bars;
            for (int i = 0; i < empty; ++i) {
                vert_bars.push_back(element::text(empty_char));
            }
            for (int i = 0; i < filled; ++i) {
                vert_bars.push_back(element::text(fill_char));
            }
            return element::vbox(move(vert_bars));
        }
    }

    return element::text(bar);
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
