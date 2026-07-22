#include <NeForce/tui/reconciler.hpp>
#ifdef NEFORCE_PLATFORM_LINUX
#    include <unistd.h>
#elif defined(NEFORCE_PLATFORM_WINDOWS)
#    include <WinBase.h>
#endif
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    void tagOwners(Element& el, ComponentBase* comp) {
        el.setOwner(comp);
        for (const auto& child: el.children()) {
            tagOwners(const_cast<Element&>(child), comp);
        }
    }

    State<string>* findFirstTextInput(const Element& tree) {
        if (tree.kind() == ElementKind::TextInput && tree.stateRef() != nullptr) {
            return static_cast<State<string>*>(tree.stateRef());
        }
        for (const auto& child: tree.children()) {
            auto* found = findFirstTextInput(child);
            if (found != nullptr) {
                return found;
            }
        }
        return nullptr;
    }
} // namespace


Reconciler::Reconciler(sys_console& console, strand& s, io_context& ctx) :
console_(console),
strand_(s),
ctx_(ctx) {
    refreshTermSize();
    prevTree_ = Element::empty();
}

Reconciler::~Reconciler() {
    mounted_ = false;
    if (root_ != nullptr) {
        root_->cleanup();
        root_ = nullptr;
    }
}

function<void(ComponentBase*)> Reconciler::scheduleRenderCallback() {
    return [this](ComponentBase* comp) { scheduleUpdate(comp); };
}

void Reconciler::refreshTermSize() {
    const auto sz = console_.get_console_size();
    if (sz.width > 0 && sz.height > 0) {
        termW_ = sz.width;
        termH_ = sz.height;
    }
}

void Reconciler::setupTree(ComponentBase* comp) {
    comp->strand_ = &strand_;
    comp->ctx_ = &ctx_;
    comp->scheduleRenderCb_ = scheduleRenderCallback();
    comp->setup();
}

void Reconciler::buildFocusChain() {
    focusChain_.clear();
    if (root_ == nullptr) {
        return;
    }

    vector<ComponentBase*> stack;
    stack.push_back(root_);
    while (!stack.empty()) {
        auto* comp = stack.back();
        stack.pop_back();
        comp->collectFocusable(focusChain_);
    }
}

void Reconciler::setFocus(ComponentBase* comp) {
    if (focused_ == comp) {
        return;
    }
    focused_ = comp;
}

void Reconciler::focusNext() {
    if (focusChain_.empty()) {
        return;
    }
    size_t idx = 0;
    if (focused_ != nullptr) {
        const auto it = find(focusChain_.begin(), focusChain_.end(), focused_);
        if (it != focusChain_.end()) {
            idx = static_cast<size_t>(it - focusChain_.begin()) + 1;
        }
    }
    if (idx >= focusChain_.size()) {
        idx = 0;
    }
    setFocus(focusChain_[idx]);
}

void Reconciler::focusPrev() {
    if (focusChain_.empty()) {
        return;
    }
    size_t idx = focusChain_.size() - 1;
    if (focused_ != nullptr) {
        const auto it = find(focusChain_.begin(), focusChain_.end(), focused_);
        if (it != focusChain_.end() && it != focusChain_.begin()) {
            idx = static_cast<size_t>(it - focusChain_.begin()) - 1;
        }
    }
    setFocus(focusChain_[idx]);
}

void Reconciler::mount(ComponentBase* root) {
    root_ = root;
    root_->parent_ = nullptr;
    setupTree(root_);
    mounted_ = true;
    markDirty();
    flush();
}

void Reconciler::scheduleUpdate(ComponentBase* /*comp*/) {
    if (!mounted_ || updatePending_) {
        return;
    }
    updatePending_ = true;
    strand_.post([this] {
        updatePending_ = false;
        dirty_ = true;
        ctx_.post([this] { flush(); });
    });
}

void Reconciler::markDirty() { dirty_ = true; }

bool Reconciler::dispatchKey(const KeyEvent& e) {
    if (root_ == nullptr || !mounted_) {
        return false;
    }

    if (e.key == KeyEvent::Key::Tab) {
        if ((e.mods & Modifier::Shift) != Modifier::None) {
            focusPrev();
        } else {
            focusNext();
        }
        markDirty();
        return true;
    }

    if (focused_ != nullptr) {
        if (focused_->onKey(e)) {
            return true;
        }
    }
    if (root_->onKey(e)) {
        return true;
    }

    auto* tiState = findFirstTextInput(prevTree_);
    if (tiState != nullptr) {
        if (e.key == KeyEvent::Key::Backspace) {
            string val = tiState->value();
            if (!val.empty()) {
                val.pop_back();
                *tiState = val;
            }
            return true;
        }
        if (e.key == KeyEvent::Key::Printable && e.ch >= 0x20) {
            string val = tiState->value();
            val += static_cast<char>(e.ch);
            *tiState = val;
            return true;
        }
    }
    return false;
}

bool Reconciler::dispatchMouse(const MouseEvent& e) {
    if (root_ == nullptr || !mounted_) {
        return false;
    }
    if (dirty_) {
        flush();
    }
    int idx = 0;
    const Element* hitEl = findElementAt(prevLayout_, prevTree_, e.x, e.y, idx);
    if (hitEl != nullptr && hitEl->onClick()) {
        hitEl->onClick()();
        if (root_ != nullptr && mounted_) {
            markDirty();
        }
        return true;
    }
    idx = 0;
    ComponentBase* target = hitTestAt(prevLayout_, prevTree_, e.x, e.y, idx);
    if (target != nullptr && root_ != nullptr) {
        return target->onMouse(e);
    }
    return false;
}

void Reconciler::flush() {
    if (!mounted_ || root_ == nullptr) {
        return;
    }
    if (!dirty_) {
        return;
    }
    if (rendering_) {
        return;
    }
    rendering_ = true;
    dirty_ = false;

    refreshTermSize();
    Element newTree = root_->render();

    buildFocusChain();

    tagOwners(newTree, root_);

    auto newLayout = computeLayout(newTree, termW_, termH_);

    outputBuffer_.clear();
    outputBuffer_.reserve(8192);

    if (prevLayout_.empty() || prevTree_.kind() == ElementKind::Empty) {
        int idx = 0;
        renderSubtree(newTree, newLayout, idx);
    } else {
        int oldIdx = 0;
        int newIdx = 0;
        renderDiff(prevTree_, newTree, prevLayout_, newLayout, oldIdx, newIdx);
    }

    prevTree_ = move(newTree);
    prevLayout_ = move(newLayout);

    if (!outputBuffer_.empty()) {
#ifdef NEFORCE_PLATFORM_LINUX
        ::write(STDOUT_FILENO, outputBuffer_.data(), outputBuffer_.size());
#elif defined(NEFORCE_PLATFORM_WINDOWS)
        ::DWORD written = 0;
        ::WriteFile(::GetStdHandle(STD_OUTPUT_HANDLE), outputBuffer_.data(), static_cast<::DWORD>(outputBuffer_.size()),
                    &written, nullptr);
#endif
        outputBuffer_.clear();
    }
    rendering_ = false;
}

void Reconciler::renderDiff(const Element& oldTree, const Element& newTree, const vector<LayoutRect>& oldLayout,
                            const vector<LayoutRect>& newLayout, int& oldIdx, int& newIdx) {
    if (oldTree.kind() != newTree.kind()) {
        renderSubtree(newTree, newLayout, newIdx);
        return;
    }

    switch (newTree.kind()) {
        case ElementKind::Empty: {
            return;
        }
        case ElementKind::ScrollView: {
            ++oldIdx;
            if (oldTree.children().empty() != newTree.children().empty()) {
                renderSubtree(newTree, newLayout, newIdx);
                return;
            }
            if (!oldTree.children().empty()) {
                renderDiff(oldTree.children()[0], newTree.children()[0],
                           oldLayout, newLayout, oldIdx, newIdx);
            }
            return;
        }
        case ElementKind::Text:
        case ElementKind::Button:
        case ElementKind::Checkbox:
        case ElementKind::TextInput:
        case ElementKind::Separator:
        case ElementKind::Spacer: {
            bool same = (oldTree.text() == newTree.text());
            if (same) {
                const auto& os = oldTree.style();
                const auto& ns = newTree.style();
                same = (os.fg == ns.fg && os.bg == ns.bg &&
                        os.bold == ns.bold && os.underline == ns.underline &&
                        os.italic == ns.italic && os.reverse == ns.reverse);
            }
            if (same && newTree.kind() == ElementKind::Checkbox) {
                const auto* oldS = static_cast<State<bool>*>(oldTree.stateRef());
                const auto* newS = static_cast<State<bool>*>(newTree.stateRef());
                same = (oldS != nullptr && newS != nullptr && oldS->value() == newS->value());
            }
            if (same && newTree.kind() == ElementKind::TextInput) {
                const auto* oldS = static_cast<State<string>*>(oldTree.stateRef());
                const auto* newS = static_cast<State<string>*>(newTree.stateRef());
                same = (oldS != nullptr && newS != nullptr && oldS->value() == newS->value());
            }

            ++oldIdx;
            if (!same && newIdx < newLayout.size()) {
                if (oldIdx > 0 && static_cast<size_t>(oldIdx - 1) < oldLayout.size()) {
                    const auto& oldRect = oldLayout[static_cast<size_t>(oldIdx - 1)];
                    applyClear(oldRect.x, oldRect.y, oldRect.w, oldRect.h);
                }
                renderSubtree(newTree, newLayout, newIdx);
            } else {
                ++newIdx;
            }
            return;
        }
        case ElementKind::VBox:
        case ElementKind::HBox:
        case ElementKind::ZStack:
        case ElementKind::Each:
        case ElementKind::When: {
            if (newTree.kind() == ElementKind::Each) {
                const auto& oldChildren = oldTree.children();
                const auto& newChildren = newTree.children();

                unordered_map<size_t, size_t> oldKeyMap;
                for (size_t i = 0; i < oldChildren.size(); ++i) {
                    oldKeyMap[oldChildren[i].key() != 0 ? oldChildren[i].key() : i + 1] = i;
                }

                for (size_t ni = 0; ni < newChildren.size(); ++ni) {
                    size_t nk = newChildren[ni].key() != 0 ? newChildren[ni].key() : ni + 1;
                    auto oit = oldKeyMap.find(nk);
                    if (oit != oldKeyMap.end()) {
                        const auto& oldChild = oldChildren[oit->second];
                        renderDiff(oldChild, newChildren[ni], oldLayout, newLayout, oldIdx, newIdx);
                        oldKeyMap.erase(oit);
                    } else {
                        renderSubtree(newChildren[ni], newLayout, newIdx);
                    }
                }
                for (const auto& remaining: oldKeyMap) {
                    const auto& oldChild = oldChildren[remaining.second];
                    clearElementArea(oldChild, oldLayout, oldIdx);
                }
            } else {
                const auto& oldChildren = oldTree.children();
                const auto& newChildren = newTree.children();
                const size_t n = min(oldChildren.size(), newChildren.size());
                for (size_t i = 0; i < n; ++i) {
                    renderDiff(oldChildren[i], newChildren[i], oldLayout, newLayout, oldIdx, newIdx);
                }
                for (size_t i = n; i < newChildren.size(); ++i) {
                    renderSubtree(newChildren[i], newLayout, newIdx);
                }
                oldIdx += static_cast<int>(oldChildren.size() > n ? oldChildren.size() - n : 0);
            }
            return;
        }
        case ElementKind::Canvas:
        default: {
            ++oldIdx;
            renderSubtree(newTree, newLayout, newIdx);
            return;
        }
    }
}

void Reconciler::renderSubtree(const Element& el, const vector<LayoutRect>& layout, int& idx) {
    switch (el.kind()) {
        case ElementKind::Empty: {
            return;
        }
        case ElementKind::Text: {
            if (idx < layout.size()) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                applyText(rect.x, rect.y, el.text(), el.style());
                ++idx;
            }
            return;
        }
        case ElementKind::Button: {
            if (idx < layout.size()) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                renderButton(rect.x, rect.y, rect.w, rect.h, el.text(), el.style(), theme_, el.variant());
                ++idx;
            }
            return;
        }
        case ElementKind::Checkbox: {
            if (idx < layout.size()) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                bool checked = false;
                if (el.stateRef() != nullptr) {
                    const auto* s = static_cast<State<bool>*>(el.stateRef());
                    checked = s->value();
                }
                renderCheckbox(rect.x, rect.y, el.text(), checked, el.style());
                ++idx;
            }
            return;
        }
        case ElementKind::TextInput: {
            if (idx < layout.size()) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                applyBorder(rect.x, rect.y, rect.w, rect.h, focused_ != nullptr ? Border::Single : Border::Single,
                            focused_ != nullptr ? theme_.primary : theme_.border);
                string display;
                if (el.stateRef() != nullptr) {
                    const auto* s = static_cast<State<string>*>(el.stateRef());
                    display = s->value();
                } else {
                    display = el.text();
                }
                if (!display.empty()) {
                    applyText(rect.x + 1, rect.y, display, el.style());
                }
                ++idx;
            }
            return;
        }
        case ElementKind::Separator: {
            if (idx < layout.size()) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                renderSeparator(rect.x, rect.y, rect.w);
                ++idx;
            }
            return;
        }
        case ElementKind::Spacer: {
            ++idx;
            return;
        }
        case ElementKind::ScrollView: {
            if (idx < layout.size()) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                ++idx;
                const Style& svStyle = el.style();
                const Border svBorder = svStyle.border.value_or(Border::Single);
                applyBorder(rect.x, rect.y, rect.w, rect.h, svBorder, theme_.border);
                if (!el.children().empty() && rect.w > 2 && rect.h > 2) {
                    const Padding pad = svStyle.padding.value_or(Padding{});
                    const ClipRect clip{rect.x + 1 + pad.left, rect.y + 1 + pad.top,
                                         rect.w - 2 - pad.left - pad.right,
                                         rect.h - 2 - pad.top - pad.bottom};
                    clipStack_.push_back(clip);
                    applyClear(clip.x, clip.y, clip.w, clip.h);
                    renderSubtree(el.children()[0], layout, idx);
                    clipStack_.pop_back();
                }
            }
            return;
        }
        case ElementKind::VBox:
        case ElementKind::HBox:
        case ElementKind::ZStack:
        case ElementKind::Each:
        case ElementKind::When: {
            for (const auto& child: el.children()) {
                renderSubtree(child, layout, idx);
            }
            return;
        }
        case ElementKind::Canvas: {
            if (idx < layout.size()) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                const auto& fn = el.drawFn();
                if (fn) {
                    fn(rect.x, rect.y, rect.w, rect.h);
                }
                ++idx;
            }
            return;
        }
        default: {
            return;
        }
    }
}

void Reconciler::clearElementArea(const Element& el, const vector<LayoutRect>& layout, int& idx) {
    switch (el.kind()) {
        case ElementKind::Text:
        case ElementKind::Button:
        case ElementKind::Checkbox:
        case ElementKind::TextInput:
        case ElementKind::Separator:
        case ElementKind::Spacer: {
            if (idx < layout.size()) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                applyClear(rect.x, rect.y, rect.w, rect.h);
                ++idx;
            }
            return;
        }
        case ElementKind::ScrollView: {
            if (idx < layout.size()) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                applyClear(rect.x, rect.y, rect.w, rect.h);
                ++idx;
            }
            for (const auto& child: el.children()) {
                clearElementArea(child, layout, idx);
            }
            return;
        }
        case ElementKind::VBox:
        case ElementKind::HBox:
        case ElementKind::ZStack:
        case ElementKind::Each:
        case ElementKind::When: {
            for (const auto& child: el.children()) {
                clearElementArea(child, layout, idx);
            }
            return;
        }
        default: {
            return;
        }
    }
}

const Element* Reconciler::findElementAt(const vector<LayoutRect>& layout, const Element& tree, int mx, int my,
                                         int& idx) {
    switch (tree.kind()) {
        case ElementKind::Text:
        case ElementKind::Button:
        case ElementKind::Checkbox:
        case ElementKind::TextInput:
        case ElementKind::Separator:
        case ElementKind::Spacer: {
            if (idx < layout.size()) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                ++idx;
                if (mx >= rect.x && mx < rect.x + rect.w && my >= rect.y && my < rect.y + rect.h) {
                    return &tree;
                }
            }
            return nullptr;
        }
        case ElementKind::ScrollView: {
            if (idx < layout.size()) {
                ++idx;
            }
            for (const auto& child: tree.children()) {
                const Element* hit = findElementAt(layout, child, mx, my, idx);
                if (hit != nullptr) {
                    return hit;
                }
            }
            return nullptr;
        }
        case ElementKind::VBox:
        case ElementKind::HBox:
        case ElementKind::ZStack:
        case ElementKind::Each:
        case ElementKind::When: {
            for (const auto& child: tree.children()) {
                const Element* hit = findElementAt(layout, child, mx, my, idx);
                if (hit != nullptr) {
                    return hit;
                }
            }
            return nullptr;
        }
        default: {
            return nullptr;
        }
    }
}

ComponentBase* Reconciler::hitTestAt(const vector<LayoutRect>& layout, const Element& tree, int mx, int my, int& idx) {
    switch (tree.kind()) {
        case ElementKind::Text:
        case ElementKind::Button:
        case ElementKind::Checkbox:
        case ElementKind::TextInput:
        case ElementKind::Separator:
        case ElementKind::Spacer: {
            if (idx < layout.size()) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                ++idx;
                if (mx >= rect.x && mx < rect.x + rect.w && my >= rect.y && my < rect.y + rect.h) {
                    auto* owner = static_cast<ComponentBase*>(tree.owner());
                    return owner != nullptr ? owner : root_;
                }
            }
            return nullptr;
        }
        case ElementKind::ScrollView: {
            if (idx < layout.size()) {
                ++idx;
            }
            for (const auto& child: tree.children()) {
                auto* hit = hitTestAt(layout, child, mx, my, idx);
                if (hit != nullptr) {
                    return hit;
                }
            }
            return nullptr;
        }
        case ElementKind::VBox:
        case ElementKind::HBox:
        case ElementKind::ZStack:
        case ElementKind::Each:
        case ElementKind::When: {
            for (const auto& child: tree.children()) {
                auto* hit = hitTestAt(layout, child, mx, my, idx);
                if (hit != nullptr) {
                    return hit;
                }
            }
            return nullptr;
        }
        default: {
            return nullptr;
        }
    }
}

void Reconciler::applyText(int x, int y, const string& text, const Style& style) {
    if (x < 0 || y < 0 || x >= termW_ || y >= termH_) {
        return;
    }
    for (const auto& clip: clipStack_) {
        if (x < clip.x || y < clip.y || x >= clip.x + clip.w || y >= clip.y + clip.h) {
            return;
        }
    }
    string buf = "\033[" + to_string(y + 1) + ";" + to_string(x + 1) + "H";
    if (style.fg.has_value()) {
        buf += "\033[38;5;" + to_string(style.fg.value().to_ansi_256()) + "m";
    } else {
        buf += "\033[38;5;" + to_string(theme_.fg.to_ansi_256()) + "m";
    }
    if (style.bg.has_value()) {
        buf += "\033[48;5;" + to_string(style.bg.value().to_ansi_256()) + "m";
    }
    if (style.bold.value_or(false)) {
        buf += "\033[1m";
    }
    buf += text;
    buf += "\033[0m";
    outputBuffer_ += buf;
}

void Reconciler::applyBorder(int x, int y, int w, int h, Border border, const color& c) {
    if (border == Border::None || w < 2 || h < 2) {
        return;
    }
    const auto* tl = "┌";
    const auto* tr = "┐";
    const auto* bl = "└";
    const auto* br = "┘";
    const auto* hz = "─";
    const auto* vt = "│";

    switch (border) {
        case Border::Double: {
            tl = "╔";
            tr = "╗";
            bl = "╚";
            br = "╝";
            hz = "═";
            vt = "║";
            break;
        }
        case Border::Rounded: {
            tl = "╭";
            tr = "╮";
            bl = "╰";
            br = "╯";
            hz = "─";
            vt = "│";
            break;
        }
        default: {
            break;
        }
    }

    string buf = "\033[38;5;" + to_string(c.to_ansi_256()) + "m";

    buf += "\033[" + to_string(y + 1) + ";" + to_string(x + 1) + "H";
    buf += tl;
    for (int i = 0; i < w - 2; ++i) {
        buf += hz;
    }
    buf += tr;

    for (int i = 1; i < h - 1; ++i) {
        buf += "\033[" + to_string(y + i + 1) + ";" + to_string(x + 1) + "H";
        buf += vt;
        buf += "\033[" + to_string(y + i + 1) + ";" + to_string(x + w - 1) + "H";
        buf += vt;
    }

    buf += "\033[" + to_string(y + h) + ";" + to_string(x + 1) + "H";
    buf += bl;
    for (int i = 0; i < w - 2; ++i) {
        buf += hz;
    }
    buf += br;

    buf += "\033[0m";
    outputBuffer_ += buf;
}

void Reconciler::applyClear(int x, int y, int w, int h) {
    if (w <= 0 || h <= 0) {
        return;
    }
    for (const auto& clip: clipStack_) {
        const int nx = max(x, clip.x);
        const int ny = max(y, clip.y);
        const int nw = min(x + w, clip.x + clip.w) - nx;
        const int nh = min(y + h, clip.y + clip.h) - ny;
        x = nx;
        y = ny;
        w = nw;
        h = nh;
        if (w <= 0 || h <= 0) {
            return;
        }
    }
    string buf;
    for (int i = 0; i < h; ++i) {
        buf += "\033[" + to_string(y + i + 1) + ";" + to_string(x + 1) + "H";
        for (int j = 0; j < w; ++j) {
            buf += ' ';
        }
    }
    outputBuffer_ += buf;
}

void Reconciler::renderButton(int x, int y, int w, int h, const string& label, const Style& style, const Theme& theme,
                              Variant variant) {
    Style s = theme.buttonStyle(variant);
    s = Style::merge(s, style);

    const color borderColor = s.borderColor.value_or(theme.border);
    applyBorder(x, y, w, h, Border::Single, borderColor);

    int labelX = x + (w - static_cast<int>(label.size())) / 2;
    labelX = max(labelX, x + 1);

    applyText(labelX, y, label, s);
}

void Reconciler::renderCheckbox(int x, int y, const string& label, bool checked, const Style& style) {
    string display = checked ? "[x] " : "[ ] ";
    display += label;
    applyText(x, y, display, style);
}

void Reconciler::renderSeparator(int x, int y, int w) {
    if (w <= 0) {
        return;
    }
    string buf = "\033[38;5;" + to_string(theme_.border.to_ansi_256()) + "m";
    buf += "\033[" + to_string(y + 1) + ";" + to_string(x + 1) + "H";
    for (int i = 0; i < w; ++i) {
        buf += '-';
    }
    buf += "\033[0m";
    outputBuffer_ += buf;
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
