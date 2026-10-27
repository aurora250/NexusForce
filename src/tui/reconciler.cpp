#include <NeForce/tui/component/motion.hpp>
#include <NeForce/tui/reconciler.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    void notify_terminal_resize(component_base* comp, const int width, const int height) {
        if (comp == nullptr) {
            return;
        }
        comp->on_terminal_resize(width, height);
        const size_t count = comp->child_count();
        for (size_t index = 0; index < count; ++index) {
            notify_terminal_resize(comp->child_at(index), width, height);
        }
    }

    void tag_owners(element& el, component_base* comp) {
        if (el.owner() == nullptr) {
            el.set_owner(comp);
        }
        for (size_t i = 0; i < el.children().size(); ++i) {
            tag_owners(el.children()[i], comp);
        }
    }

    void reconcile_keys(const element& old_tree, element& new_tree) {
        if (old_tree.kind() != element::kind::each || new_tree.kind() != element::kind::each) {
            const size_t n = min(old_tree.children().size(), new_tree.children().size());
            for (size_t i = 0; i < n; ++i) {
                reconcile_keys(old_tree.children()[i], new_tree.children()[i]);
            }
            return;
        }

        flat_unordered_map<size_t, void*> old_owners;
        for (const auto& old_child: old_tree.children()) {
            const size_t k = old_child.key();
            if (k != 0 && old_child.owner() != nullptr) {
                old_owners[k] = old_child.owner();
            }
        }

        for (auto& new_child: new_tree.children()) {
            const size_t k = new_child.key();
            if (k != 0) {
                const auto it = old_owners.find(k);
                if (it != old_owners.end() && new_child.owner() == nullptr) {
                    new_child.set_owner(static_cast<component_base*>(it->second));
                }
            }
            for (const auto& old_child: old_tree.children()) {
                if (old_child.key() == k && k != 0) {
                    reconcile_keys(old_child, new_child);
                    break;
                }
            }
        }
    }

    bool is_copy_shortcut(const key_event& e) noexcept {
        if (e.key != key_event::type::printable) {
            return false;
        }
        const auto cp = e.cp.value();
        if (cp != static_cast<uint32_t>(U'c') && cp != static_cast<uint32_t>(U'C')) {
            return false;
        }
        return (e.mods & key_modifier::ctrl) != key_modifier::none;
    }
} // namespace


reconciler::reconciler(sys_console& console, strand& s, io_context& ctx) :
console_(console),
strand_(s),
ctx_(ctx),
event_dispatcher_(focus_mgr_),
renderer_(current_screen_, theme_, term_w_, term_h_) {
    event_dispatcher_.set_dirty_callback([this] { mark_dirty(); });

    auto depth = color_depth::ansi256;
    if (console_.supports_truecolor()) {
        depth = color_depth::truecolor;
    } else if (!console_.supports_256_color()) {
        depth = color_depth::ansi16;
    }
    current_screen_.set_color_depth(depth);
    prev_screen_.set_color_depth(depth);

    refresh_term_size();
    current_screen_.resize(term_w_, term_h_);
    prev_screen_.resize(term_w_, term_h_);
    prev_tree_ = element::empty();
}

reconciler::~reconciler() {
    mounted_ = false;
    if (root_ != nullptr) {
        root_->release_tree();
        root_ = nullptr;
    }
}

function<void(component_base*)> reconciler::schedule_render_callback() {
    return [this](component_base* comp) { schedule_update(comp); };
}

void reconciler::refresh_term_size() {
    if (forced_w_ > 0 && forced_h_ > 0) {
        term_w_ = forced_w_;
        term_h_ = forced_h_;
        return;
    }
    const auto sz = console_.get_console_size();
    if (sz.width > 0 && sz.height > 0) {
        term_w_ = sz.width;
        term_h_ = sz.height;
    }
}

void reconciler::set_term_size(const int width, const int height) noexcept {
    forced_w_ = width;
    forced_h_ = height;
    mark_dirty();
}

void reconciler::set_color_depth(const color_depth depth) {
    current_screen_.set_color_depth(depth);
    prev_screen_.set_color_depth(depth);
    mark_dirty();
}

color_depth reconciler::color_depth_mode() const noexcept { return current_screen_.color_depth_mode(); }

void reconciler::setup_tree(component_base* comp) {
    comp->strand_ = &strand_;
    comp->ctx_ = &ctx_;
    comp->schedule_render_cb_ = schedule_render_callback();
    comp->focus_request_cb_ = [this](component_base* target) {
        focus_mgr_.set_focus(target);
        mark_dirty();
    };
    comp->setup();
}

void reconciler::mount(component_base* root) {
    root_ = root;
    root_->parent_ = nullptr;
    setup_tree(root_);
    focus_mgr_.mark_chain_dirty();
    mounted_ = true;
    mark_dirty();
    flush();
}

void reconciler::schedule_update(component_base* comp) {
    if (!mounted_ || update_pending_) {
        return;
    }
    if (comp != nullptr && !comp->should_update()) {
        return;
    }
    update_pending_ = true;
    strand_.post([this] {
        update_pending_ = false;
        dirty_ = true;
        ctx_.post([this] { flush(); });
    });
}

void reconciler::mark_dirty() { dirty_ = true; }

void reconciler::invalidate() {
    painted_w_ = 0;
    painted_h_ = 0;
    dirty_ = true;
}

void reconciler::set_theme(const theme& t) {
    theme_ = t;
    mark_dirty();
}

bool reconciler::dispatch_key(const key_event& e) {
    if (root_ == nullptr || !mounted_) {
        return false;
    }
    // A pending selection owns ^C / ^Shift+C:
    // copying beats quitting, and any other key drops the highlight so it never survives a content change.
    if (!selection_.empty()) {
        if (is_copy_shortcut(e)) {
            copy_selection();
            return true;
        }
        clear_selection();
    }
    if (shortcuts_.dispatch(e)) {
        return true;
    }
    if (event_dispatcher_.dispatch_key(e, root_)) {
        return true;
    }
    return false;
}

bool reconciler::dispatch_mouse(const mouse_event& e) {
    if (root_ == nullptr || !mounted_) {
        return false;
    }
    if (dirty_) {
        flush();
    }

    if (e.action == mouse_action::wheel && !selection_.empty()) {
        clear_selection();
    }
    if (handle_selection_mouse(e)) {
        return true;
    }

    return event_dispatcher_.dispatch_mouse(e, root_, prev_layout_, prev_tree_, renderer_.scrollbar_hits());
}

bool reconciler::handle_selection_mouse(const mouse_event& e) {
    if (!selection_enabled_ || e.button != mouse_button::left) {
        return false;
    }
    // A scrollbar drag owns the pointer; dragging its thumb must not paint a selection.
    if (event_dispatcher_.is_dragging()) {
        selecting_ = false;
        return false;
    }

    switch (e.action) {
        case mouse_action::press: {
            selecting_ = true;
            select_origin_x_ = e.x;
            select_origin_y_ = e.y;
            if (!selection_.empty()) {
                clear_selection();
            }
            return false;
        }
        case mouse_action::move: {
            if (!selecting_) {
                return false;
            }
            if (selection_.empty() && e.x == select_origin_x_ && e.y == select_origin_y_) {
                return true;
            }
            if (selection_.empty()) {
                selection_.start(select_origin_x_, select_origin_y_);
            }
            selection_.extend_to(e.x, e.y);
            mark_dirty();
            return true;
        }
        case mouse_action::release: {
            if (!selecting_) {
                return false;
            }
            selecting_ = false;
            if (selection_.empty()) {
                return false;
            }
            if (copy_on_select_) {
                write_selection(true);
            }
            return true;
        }
        default: {
            return false;
        }
    }
}

bool reconciler::write_selection(const bool keep) {
    if (selection_.empty()) {
        return false;
    }

    const string text = tui::selection_text(current_screen_, selection_);
    if (!keep) {
        clear_selection();
    }
    if (text.empty()) {
        return false;
    }

    if (clipboard_writer_) {
        clipboard_writer_(text.view());
        return true;
    }

    try {
        console_.copy_to_clipboard(text.view());
    } catch (const exception& e) {
        NEFORCE_REPORT_EXCEPTION(e);
        return false;
    }
    return true;
}

string reconciler::selection_text() const {
    if (selection_.empty()) {
        return string();
    }
    return tui::selection_text(current_screen_, selection_);
}

void reconciler::scroll_selection(const int delta_rows) {
    if (selection_.empty()) {
        return;
    }
    selection_.translate(delta_rows);
    mark_dirty();
}

bool reconciler::copy_selection() { return write_selection(false); }

void reconciler::clear_selection() {
    if (selection_.empty() && !selecting_) {
        return;
    }
    selection_.clear();
    selecting_ = false;
    mark_dirty();
}

bool reconciler::dispatch_focus(const focus_event& e) {
    focused_ = e.focused;
    frame_clock::instance().set_paused(!e.focused);
    if (root_ != nullptr) {
        vector<component_base*> stack;
        stack.push_back(root_);
        while (!stack.empty()) {
            component_base* comp = stack.back();
            stack.pop_back();
            comp->on_focus(e.focused);
            for (size_t i = 0; i < comp->child_count(); ++i) {
                stack.push_back(comp->child_at(i));
            }
        }
    }
    mark_dirty();
    return true;
}

bool reconciler::dispatch_paste(const paste_event& e) {
    if (root_ == nullptr || !mounted_) {
        return false;
    }
    return event_dispatcher_.dispatch_paste(e, root_);
}

void reconciler::flush() {
    if (!mounted_ || root_ == nullptr) {
        return;
    }
    if (!dirty_) {
        return;
    }
    if (rendering_) {
        return;
    }

    struct rendering_guard {
        bool& flag;

        ~rendering_guard() { flag = false; }
    };

    rendering_ = true;
    rendering_guard guard{rendering_};

    refresh_term_size();
    renderer_.set_term_size(term_w_, term_h_);
    notify_terminal_resize(root_, term_w_, term_h_);

    const bool repaint_all = (term_w_ != painted_w_ || term_h_ != painted_h_);

    if (current_screen_.dimx() != term_w_ || current_screen_.dimy() != term_h_) {
        current_screen_.resize(term_w_, term_h_);
        prev_screen_.resize(term_w_, term_h_);
    }

    element new_tree = root_->render();

    if (root_->structure_dirty()) {
        root_->clear_structure_dirty();
        focus_mgr_.mark_chain_dirty();
    }
    focus_mgr_.rebuild_chain(root_);
    focus_mgr_.validate_focus();

    reconcile_keys(prev_tree_, new_tree);
    tag_owners(new_tree, root_);

    auto newLayout = compute_layout(new_tree, term_w_, term_h_);

    current_screen_.clear();

    const bool has_focus = (focus_mgr_.focused() != nullptr);
    renderer_.render(new_tree, newLayout, has_focus);
    paint_selection(current_screen_, selection_);

    string output;
    if (repaint_all) {
        output += "\033[2J";
    }
    if (synchronized_output_) {
        output += "\033[?2026h";
    }
    output += current_screen_.to_string(prev_screen_);
    if (synchronized_output_) {
        output += "\033[?2026l";
    }

    prev_screen_ = current_screen_;
    prev_tree_ = move(new_tree);
    prev_layout_ = move(newLayout);
    painted_w_ = term_w_;
    painted_h_ = term_h_;
    dirty_ = false;

    if (!output.empty()) {
        if (output_sink_) {
            output_sink_(output.view());
        } else {
            console_.print_string(output);
        }
    }
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
