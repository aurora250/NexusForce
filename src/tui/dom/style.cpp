#include <NeForce/tui/dom/style.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    template <typename T>
    void merge_optional(optional<T>& dst, const optional<T>& src) {
        if (src.has_value()) {
            dst = src;
        }
    }
} // namespace


style style::merge(const style& base, const style& over) {
    style result = base;
    merge_optional(result.fg, over.fg);
    merge_optional(result.bg, over.bg);
    merge_optional(result.bold, over.bold);
    merge_optional(result.dim, over.dim);
    merge_optional(result.underline, over.underline);
    merge_optional(result.underlined_double, over.underlined_double);
    merge_optional(result.italic, over.italic);
    merge_optional(result.reverse, over.reverse);
    merge_optional(result.blink, over.blink);
    merge_optional(result.strikethrough, over.strikethrough);
    merge_optional(result.padding, over.padding);
    merge_optional(result.margin, over.margin);
    merge_optional(result.border, over.border);
    merge_optional(result.borderColor, over.borderColor);
    merge_optional(result.width, over.width);
    merge_optional(result.height, over.height);
    merge_optional(result.align, over.align);
    merge_optional(result.flex_grow, over.flex_grow);
    merge_optional(result.flex_shrink, over.flex_shrink);
    merge_optional(result.text_wrap, over.text_wrap);
    return result;
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
