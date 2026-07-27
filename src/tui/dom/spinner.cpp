#include <NeForce/tui/dom/spinner.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    const char* frame_sets[4][8] = {
            {"│", "／", "─", "＼"},                   // simple lines (4)
            {"⠁", "⠂", "⠄", "⡀", "⠈", "⠐", "⠠", "⢀"}, // braille (8)
            {"◷", "◶", "◵", "◴"},                     // clock (4)
            {"⣷", "⣯", "⣟", "⡿", "⢿", "⣻", "⣽", "⣾"}, // dots (8)
    };
}


element spinner(const int charset_index, const size_t image_index) {
    const int idx = (charset_index >= 0 && charset_index < 4) ? charset_index : 0;
    const auto* frames = frame_sets[idx];
    const size_t count = (idx == 1 || idx == 3) ? 8U : 4U;
    const size_t i = image_index % count;

    return element::text(string(frames[i]));
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
