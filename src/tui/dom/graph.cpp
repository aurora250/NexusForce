#include <NeForce/tui/dom/graph.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    element graph_from_points(const vector<int>& points, int width, int height) {
        if (points.empty()) {
            return element::text(string(width, ' '));
        }

        int y_min = points[0];
        int y_max = points[0];
        for (const int v: points) {
            y_min = min(v, y_min);
            y_max = max(v, y_max);
        }
        const int range = (y_max - y_min > 0) ? (y_max - y_min) : 1;

        vector<int> scaled(width);
        for (int x = 0; x < width; ++x) {
            const size_t idx = static_cast<size_t>(x) * points.size() / width;
            const float ratio = static_cast<float>(points[idx] - y_min) / static_cast<float>(range);
            scaled[x] = static_cast<int>(ratio) * (height - 1);
        }

        elements rows;
        for (int row = 0; row < height; ++row) {
            const int y = height - 1 - row;
            string line;
            line.reserve(width);
            for (int x = 0; x < width; ++x) {
                if (scaled[x] == y) {
                    line += "█";
                } else if (scaled[x] > y) {
                    if (x > 0 && scaled[x - 1] < y && scaled[x] > y) {
                        line += "▄";
                    } else {
                        line += "█";
                    }
                } else {
                    line += " ";
                }
            }
            rows.push_back(element::text(line));
        }

        return element::vbox(move(rows));
    }
} // anonymous namespace


element graph(const vector<int>& data_points, const int width, const int height) {
    return graph_from_points(data_points, width, height);
}

element graph(graph_function fn, const int width, const int height) {
    const auto points = fn(width, height);
    return graph_from_points(points, width, height);
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
