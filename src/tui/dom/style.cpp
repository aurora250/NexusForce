#include <NeForce/tui/dom/style.hpp>
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


style style::merge(const style& base, const style& over) {
    style result = base;
    mergeOptional(result.fg, over.fg);
    mergeOptional(result.bg, over.bg);
    mergeOptional(result.bold, over.bold);
    mergeOptional(result.dim, over.dim);
    mergeOptional(result.underline, over.underline);
    mergeOptional(result.underlined_double, over.underlined_double);
    mergeOptional(result.italic, over.italic);
    mergeOptional(result.reverse, over.reverse);
    mergeOptional(result.blink, over.blink);
    mergeOptional(result.strikethrough, over.strikethrough);
    mergeOptional(result.padding, over.padding);
    mergeOptional(result.margin, over.margin);
    mergeOptional(result.border, over.border);
    mergeOptional(result.borderColor, over.borderColor);
    mergeOptional(result.width, over.width);
    mergeOptional(result.height, over.height);
    mergeOptional(result.align, over.align);
    mergeOptional(result.flex_grow, over.flex_grow);
    mergeOptional(result.flex_shrink, over.flex_shrink);
    mergeOptional(result.text_wrap, over.text_wrap);
    return result;
}

style theme::button_style(style::variant v) const {
    style s;
    s.fg = fg;
    s.padding = {0, 2, 0, 2};
    switch (v) {
        case style::variant::primary:
            s.bg = primary;
            s.bold = true;
            break;
        case style::variant::secondary:
            s.bg = secondary;
            break;
        case style::variant::danger:
            s.fg = danger;
            s.bold = true;
            break;
        case style::variant::success:
            s.fg = success;
            s.bold = true;
            break;
        case style::variant::default_:
        default:
            break;
    }
    return s;
}

style theme::text_style() const {
    style s;
    s.fg = fg;
    return s;
}

style theme::input_style(bool focused) const {
    style s;
    s.fg = fg;
    s.bg = bg;
    if (focused) {
        s.border = style::border::single;
        s.borderColor = primary;
    } else {
        s.border = style::border::single;
        s.borderColor = border;
    }
    s.padding = {0, 1, 0, 1};
    return s;
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
