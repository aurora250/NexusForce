#include <NeForce/tui/dom/scroll_indicator.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

element vscroll_indicator(const int total, const int visible, const int offset, const int height) {
    if (total <= visible) {
        return element::text(string(height, ' '));
    }

    const float ratio = static_cast<float>(visible) / static_cast<float>(total);
    const int thumb_size = static_cast<int>(ratio) * height;
    const int clamped_thumb = (thumb_size < 1) ? 1 : thumb_size;
    const int max_offset = total - visible;
    const float offset_ratio = static_cast<float>(offset) / static_cast<float>(max_offset);
    const int thumb_pos = static_cast<int>(offset_ratio) * (height - clamped_thumb);

    elements rows;
    for (int i = 0; i < height; ++i) {
        if (i >= thumb_pos && i < thumb_pos + clamped_thumb) {
            rows.push_back(element::text("█"));
        } else {
            rows.push_back(element::text("│"));
        }
    }
    return element::vbox(move(rows));
}

element hscroll_indicator(const int total, const int visible, const int offset, const int width) {
    if (total <= visible) {
        return element::text(string(width, ' '));
    }

    const float ratio = static_cast<float>(visible) / static_cast<float>(total);
    const int thumb_size = static_cast<int>(ratio) * width;
    const int clamped_thumb = (thumb_size < 1) ? 1 : thumb_size;
    const int max_offset = total - visible;
    const float offset_ratio = static_cast<float>(offset) / static_cast<float>(max_offset);
    const int thumb_pos = static_cast<int>(offset_ratio) * (width - clamped_thumb);

    string bar;
    bar.reserve(width);
    for (int i = 0; i < width; ++i) {
        if (i >= thumb_pos && i < thumb_pos + clamped_thumb) {
            bar += "█";
        } else {
            bar += "─";
        }
    }
    return element::text(bar);
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
