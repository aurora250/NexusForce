#include <NeForce/tui/layout.hpp>
#include <NeForce/core/string/codepoint.hpp>
#include <NeForce/core/string/utf_iterator.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    int cpDisplayWidth(char32_t cp) {
        if ((cp <= 0x001FU) || (cp >= 0x007FU && cp <= 0x009FU)) {
            return 0;
        }
        if (cp == 0x200DU || cp == 0x200CU || cp == 0x200EU || cp == 0x200FU || (cp >= 0x0300U && cp <= 0x036FU) ||
            (cp >= 0x1AB0U && cp <= 0x1AFFU) || (cp >= 0x1DC0U && cp <= 0x1DFFU) || (cp >= 0x20D0U && cp <= 0x20FFU) ||
            (cp >= 0xFE00U && cp <= 0xFE0FU)) {
            return 0;
        }
        if ((cp >= 0x1100U && cp <= 0x115FU) ||   // Hangul Jamo
            (cp >= 0x2329U && cp <= 0x232AU) ||   // angle brackets
            (cp >= 0x2E80U && cp <= 0x303FU) ||   // CJK Radicals
            (cp >= 0x3040U && cp <= 0x33BFU) ||   // Hiragana, Katakana, Bopomofo, CJK Symbols
            (cp >= 0x3400U && cp <= 0x4DBFU) ||   // CJK Ext-A
            (cp >= 0x4E00U && cp <= 0x9FFFU) ||   // CJK Unified
            (cp >= 0xA000U && cp <= 0xA4CFU) ||   // Yi
            (cp >= 0xAC00U && cp <= 0xD7AFU) ||   // Hangul Syllables
            (cp >= 0xF900U && cp <= 0xFAFFU) ||   // CJK Compatibility
            (cp >= 0xFE10U && cp <= 0xFE19U) ||   // Vertical forms
            (cp >= 0xFE30U && cp <= 0xFE6FU) ||   // CJK Compatibility Forms
            (cp >= 0xFF01U && cp <= 0xFF60U) ||   // Fullwidth Forms
            (cp >= 0xFFE0U && cp <= 0xFFE6U) ||   // Fullwidth Signs
            (cp >= 0x1F004U && cp <= 0x1F9FFU) || // Emoji / Misc Symbols
            (cp >= 0x20000U && cp <= 0x2FFFD) ||  // CJK Ext-B ~
            (cp >= 0x30000U && cp <= 0x3FFFD)) {
            return 2;
        }
        return 1;
    }

    int stringDisplayWidth(const string& text) {
        int width = 0;
        const auto& raw = text.view();
        for (utf8_iterator it(reinterpret_cast<const byte_t*>(raw.data()), raw.size()); it != utf8_iterator(); ++it) {
            width += cpDisplayWidth(it->to_char32());
        }
        return width;
    }

    int measureTextWidth(const string& text) { return stringDisplayWidth(text); }

    int measureElementWidth(const Element& el) {
        const auto k = el.kind();
        const auto& style = el.style();

        int contentW = 0;
        switch (k) {
            case ElementKind::Text:
            case ElementKind::Button:
            case ElementKind::Checkbox: {
                contentW = measureTextWidth(el.text());
                break;
            }
            case ElementKind::Spacer: {
                contentW = 0;
                break;
            }
            case ElementKind::Separator: {
                contentW = 0;
                break;
            }
            case ElementKind::TextInput: {
                contentW = el.text().empty() ? 10 : measureTextWidth(el.text()) + 2;
                break;
            }
            case ElementKind::VBox:
            case ElementKind::HBox:
            case ElementKind::ZStack: {
                int maxChildW = 0;
                int totalChildW = 0;
                for (const auto& child: el.children()) {
                    int cw = measureElementWidth(child);
                    maxChildW = max(maxChildW, cw);
                    totalChildW += cw;
                }
                const auto& lp = el.layout();
                int n = static_cast<int>(el.children().size());
                if (lp.dir == Direction::Row) {
                    contentW = totalChildW + max(0, n - 1) * lp.gap;
                } else {
                    contentW = maxChildW;
                }
                break;
            }
            case ElementKind::ScrollView: {
                contentW = el.children().empty() ? 0 : measureElementWidth(el.children()[0]);
                break;
            }
            case ElementKind::Each:
            case ElementKind::When: {
                int maxChildW = 0;
                for (const auto& child: el.children()) {
                    maxChildW = max(maxChildW, measureElementWidth(child));
                }
                contentW = maxChildW;
                break;
            }
            case ElementKind::Empty:
            default: {
                break;
            }
        }

        const SizeHint widthHint = style.width.value_or(SizeHint{});
        const Padding pad = style.padding.value_or(Padding{});
        const Margin marW = style.margin.value_or(Margin{});
        if (widthHint.mode == SizeHint::Fixed) {
            return widthHint.value + pad.left + pad.right + marW.left + marW.right;
        }

        contentW += pad.left + pad.right + marW.left + marW.right;
        return contentW;
    }

    int measureElementHeight(const Element& el) {
        const auto k = el.kind();
        const auto& style = el.style();
        int contentH = 0;

        switch (k) {
            case ElementKind::Text:
            case ElementKind::Button:
            case ElementKind::Checkbox:
            case ElementKind::TextInput:
            case ElementKind::Separator: {
                contentH = 1;
                break;
            }
            case ElementKind::Spacer: {
                contentH = 0;
                break;
            }
            case ElementKind::VBox:
            case ElementKind::HBox:
            case ElementKind::ZStack: {
                int maxChildH = 0;
                int totalChildH = 0;
                for (const auto& child: el.children()) {
                    int ch = measureElementHeight(child);
                    maxChildH = max(maxChildH, ch);
                    totalChildH += ch;
                }
                const auto& lp = el.layout();
                int n = static_cast<int>(el.children().size());
                if (lp.dir == Direction::Column) {
                    contentH = totalChildH + max(0, n - 1) * lp.gap;
                } else {
                    contentH = maxChildH;
                }
                break;
            }
            case ElementKind::ScrollView: {
                contentH = el.children().empty() ? 1 : measureElementHeight(el.children()[0]);
                break;
            }
            case ElementKind::Each:
            case ElementKind::When: {
                int totalH = 0;
                for (const auto& child: el.children()) {
                    totalH += measureElementHeight(child);
                }
                contentH = totalH;
                break;
            }
            case ElementKind::Empty:
            default: {
                break;
            }
        }

        const SizeHint heightHint = style.height.value_or(SizeHint{});
        const Padding padH = style.padding.value_or(Padding{});
        const Margin marH = style.margin.value_or(Margin{});
        if (heightHint.mode == SizeHint::Fixed) {
            return heightHint.value + padH.top + padH.bottom + marH.top + marH.bottom;
        }

        contentH += padH.top + padH.bottom + marH.top + marH.bottom;
        return contentH;
    }

    void assignLayout(const Element& el, vector<LayoutRect>& out, int x, int y, int totalW, int totalH) {
        const auto k = el.kind();
        const auto& style = el.style();
        const auto& lp = el.layout();

        const Margin margin = style.margin.value_or(Margin{});
        LayoutRect rect;
        rect.x = x + margin.left;
        rect.y = y + margin.top;
        rect.w = totalW - margin.left - margin.right;
        rect.h = totalH - margin.top - margin.bottom;

        switch (k) {
            case ElementKind::Separator:
            case ElementKind::Spacer: {
                out.push_back(rect);
                return;
            }
            case ElementKind::ScrollView: {
                out.push_back(rect);
                if (!el.children().empty()) {
                    const auto& child = el.children()[0];
                    const Padding pad = el.style().padding.value_or(Padding{});
                    int sx = el.scrollX();
                    int sy = el.scrollY();
                    int contentX = rect.x + 1 + pad.left - sx;
                    int contentY = rect.y + 1 + pad.top - sy;
                    int contentW = max(measureElementWidth(child), rect.w - 2 - pad.left - pad.right);
                    int contentH = max(measureElementHeight(child), rect.h - 2 - pad.top - pad.bottom);
                    assignLayout(child, out, contentX, contentY, contentW, contentH);
                }
                return;
            }
            case ElementKind::Canvas: {
                const int contentW = measureElementWidth(el);
                const int contentH = measureElementHeight(el);
                if (contentW > 0 && contentW < rect.w) { rect.w = contentW; }
                if (contentH > 0 && contentH < rect.h) { rect.h = contentH; }
                out.push_back(rect);
                return;
            }
            case ElementKind::Text:
            case ElementKind::Button:
            case ElementKind::Checkbox:
            case ElementKind::TextInput: {
                const int contentW = measureElementWidth(el);
                const int contentH = measureElementHeight(el);
                if (contentW > 0 && contentW < rect.w) {
                    rect.w = contentW;
                }
                if (contentH > 0 && contentH < rect.h) {
                    rect.h = contentH;
                }
                out.push_back(rect);
                return;
            }
            case ElementKind::ZStack: {
                const int contentX = rect.x + lp.padding.left;
                const int contentY = rect.y + lp.padding.top;
                const int contentW = rect.w - lp.padding.left - lp.padding.right;
                const int contentH = rect.h - lp.padding.top - lp.padding.bottom;

                for (const auto& child: el.children()) {
                    assignLayout(child, out, contentX, contentY, contentW, contentH);
                }
                return;
            }
            case ElementKind::VBox:
            case ElementKind::HBox:
            case ElementKind::Each:
            case ElementKind::When: {
                const int contentX = rect.x + lp.padding.left;
                const int contentY = rect.y + lp.padding.top;
                const int contentW = rect.w - lp.padding.left - lp.padding.right;
                const int contentH = rect.h - lp.padding.top - lp.padding.bottom;

                const auto& children = el.children();
                const size_t n = children.size();
                if (n == 0) {
                    return;
                }

                const bool isRow = (lp.dir == Direction::Row || k == ElementKind::HBox);

                int totalFlex = 0;
                int fixedSize = 0;
                for (const auto& child: children) {
                    const auto& cs = child.style();
                    const bool isSpacer = (child.kind() == ElementKind::Spacer);
                    const int childFlex = isSpacer ? child.flex() : 0;
                    const SizeHint cwHint = cs.width.value_or(SizeHint{});
                    const SizeHint chHint = cs.height.value_or(SizeHint{});
                    int flexW = 0;
                    if (isSpacer && childFlex > 0) {
                        totalFlex += childFlex;
                    } else if (isRow && cwHint.mode == SizeHint::Fill) {
                        totalFlex += cwHint.value > 0 ? cwHint.value : 1;
                    } else if (!isRow && chHint.mode == SizeHint::Fill) {
                        totalFlex += chHint.value > 0 ? chHint.value : 1;
                    } else {
                        if (isRow) {
                            flexW = measureElementWidth(child);
                        } else {
                            flexW = measureElementHeight(child);
                        }
                        fixedSize += flexW;
                    }
                }

                const int totalGap = max(0, static_cast<int>(n) - 1) * lp.gap;
                const int flexAvailable = max(0, (isRow ? contentW : contentH) - fixedSize - totalGap);

                vector<int> childSizes(n);
                {
                    int remainingFlex = totalFlex;
                    int remainingSpace = flexAvailable;
                    for (size_t i = 0; i < n; ++i) {
                        const auto& child = children[i];
                        const auto& cs = child.style();
                        const bool isSpacer = (child.kind() == ElementKind::Spacer);
                        const int childFlex = isSpacer ? child.flex() : 0;
                        const SizeHint cwHint = cs.width.value_or(SizeHint{});
                        const SizeHint chHint = cs.height.value_or(SizeHint{});
                        if (isSpacer && childFlex > 0 && totalFlex > 0) {
                            childSizes[i] = (i == n - 1) ? remainingSpace : flexAvailable * childFlex / totalFlex;
                            remainingFlex -= childFlex;
                            remainingSpace -= childSizes[i];
                        } else if (isRow) {
                            if (cwHint.mode == SizeHint::Fill && totalFlex > 0) {
                                const int flexVal = cwHint.value > 0 ? cwHint.value : 1;
                                childSizes[i] = (i == n - 1) ? remainingSpace : flexAvailable * flexVal / totalFlex;
                                remainingFlex -= flexVal;
                                remainingSpace -= childSizes[i];
                            } else {
                                childSizes[i] = measureElementWidth(child);
                            }
                        } else {
                            if (chHint.mode == SizeHint::Fill && totalFlex > 0) {
                                const int flexVal = chHint.value > 0 ? chHint.value : 1;
                                childSizes[i] = (i == n - 1) ? remainingSpace : flexAvailable * flexVal / totalFlex;
                                remainingFlex -= flexVal;
                                remainingSpace -= childSizes[i];
                            } else {
                                childSizes[i] = measureElementHeight(child);
                            }
                        }
                    }
                }

                int totalContentSize = 0;
                for (size_t i = 0; i < n; ++i) {
                    totalContentSize += childSizes[i];
                }
                totalContentSize += totalGap;

                int justifyOffset = 0;
                int adjustedGap = lp.gap;
                const int availableSize = isRow ? contentW : contentH;
                const int freeSpace = max(0, availableSize - totalContentSize);
                switch (lp.justify) {
                    case Justify::Center: {
                        justifyOffset = freeSpace / 2;
                        break;
                    }
                    case Justify::End: {
                        justifyOffset = freeSpace;
                        break;
                    }
                    case Justify::SpaceBetween: {
                        if (n > 1) {
                            adjustedGap = lp.gap + freeSpace / static_cast<int>(n - 1);
                        }
                        break;
                    }
                    case Justify::SpaceAround: {
                        if (n > 0) {
                            const int space = freeSpace / static_cast<int>(n);
                            justifyOffset = space / 2;
                            adjustedGap = lp.gap + space;
                        }
                        break;
                    }
                    case Justify::Start:
                    default: {
                        break;
                    }
                }

                int offset = isRow ? contentX + justifyOffset : contentY + justifyOffset;
                for (size_t i = 0; i < n; ++i) {
                    const auto& child = children[i];
                    const auto& cs = child.style();

                    const int childAllocW = isRow ? childSizes[i] : contentW;
                    const int childAllocH = isRow ? contentH : childSizes[i];

                    const Align childAlign = cs.align.value_or(lp.align);
                    bool isStretch = (childAlign == Align::Stretch);
                    int crossX = contentX;
                    int crossY = contentY;
                    if (!isStretch) {
                        if (isRow) {
                            switch (childAlign) {
                                case Align::Center: {
                                    crossY = contentY + (contentH - childAllocH) / 2;
                                    break;
                                }
                                case Align::End: {
                                    crossY = contentY + contentH - childAllocH;
                                    break;
                                }
                                default: {
                                    break;
                                }
                            }
                        } else {
                            switch (childAlign) {
                                case Align::Center: {
                                    crossX = contentX + (contentW - childAllocW) / 2;
                                    break;
                                }
                                case Align::End: {
                                    crossX = contentX + contentW - childAllocW;
                                    break;
                                }
                                default: {
                                    break;
                                }
                            }
                        }
                    }

                    if (isRow) {
                        assignLayout(child, out, offset, isStretch ? contentY : crossY, childAllocW,
                                     isStretch ? contentH : childAllocH);
                    } else {
                        assignLayout(child, out, isStretch ? contentX : crossX, offset,
                                     isStretch ? contentW : childAllocW, childAllocH);
                    }
                    offset += childSizes[i] + adjustedGap;
                }
                return;
            }
            case ElementKind::Empty:
            default: {
                return;
            }
        }
    }
} // namespace


vector<LayoutRect> computeLayout(const Element& element, int constraintW, int constraintH) {
    vector<LayoutRect> result;
    if (element.kind() == ElementKind::Empty) {
        return result;
    }
    assignLayout(element, result, 0, 0, constraintW, constraintH);
    return result;
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
