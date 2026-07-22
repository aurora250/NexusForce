#include <NeForce/tui/style.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    template <typename T>
    void mergeOptional(optional<T>& dst, const optional<T>& src) {
        if (src.has_value()) {
            dst = src;
        }
    }
} // namespace


Style Style::merge(const Style& base, const Style& over) {
    Style result = base;
    mergeOptional(result.fg, over.fg);
    mergeOptional(result.bg, over.bg);
    mergeOptional(result.bold, over.bold);
    mergeOptional(result.underline, over.underline);
    mergeOptional(result.italic, over.italic);
    mergeOptional(result.reverse, over.reverse);
    mergeOptional(result.padding, over.padding);
    mergeOptional(result.margin, over.margin);
    mergeOptional(result.border, over.border);
    mergeOptional(result.borderColor, over.borderColor);
    mergeOptional(result.width, over.width);
    mergeOptional(result.height, over.height);
    mergeOptional(result.align, over.align);
    return result;
}

Style Theme::buttonStyle(Variant v) const {
    Style s;
    s.fg = fg;
    s.padding = {0, 2, 0, 2};
    switch (v) {
        case Variant::Primary:
            s.bg = primary;
            s.bold = true;
            break;
        case Variant::Secondary:
            s.bg = secondary;
            break;
        case Variant::Danger:
            s.fg = danger;
            s.bold = true;
            break;
        case Variant::Success:
            s.fg = success;
            s.bold = true;
            break;
        case Variant::Default:
        default:
            break;
    }
    return s;
}

Style Theme::textStyle() const {
    Style s;
    s.fg = fg;
    return s;
}

Style Theme::inputStyle(bool focused) const {
    Style s;
    s.fg = fg;
    s.bg = bg;
    if (focused) {
        s.border = Border::Single;
        s.borderColor = primary;
    } else {
        s.border = Border::Single;
        s.borderColor = border;
    }
    s.padding = {0, 1, 0, 1};
    return s;
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
