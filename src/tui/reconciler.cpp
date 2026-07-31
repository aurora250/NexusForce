#include <NeForce/tui/reconciler.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
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

        unordered_map<size_t, void*> old_owners;
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
} // namespace

reconciler::reconciler(sys_console& console, strand& s, io_context& ctx) :
console_(console),
strand_(s),
ctx_(ctx),
event_dispatcher_(focus_mgr_),
renderer_(current_screen_, theme_, term_w_, term_h_) {
    event_dispatcher_.set_dirty_callback([this] { mark_dirty(); });
    refresh_term_size();
    current_screen_.resize(term_w_, term_h_);
    prev_screen_.resize(term_w_, term_h_);
    prev_tree_ = element::empty();
}

reconciler::~reconciler() {
    mounted_ = false;
    if (root_ != nullptr) {
        root_->cleanup();
        root_ = nullptr;
    }
}

function<void(component_base*)> reconciler::schedule_render_callback() {
    return [this](component_base* comp) { schedule_update(comp); };
}

void reconciler::refresh_term_size() {
    const auto sz = console_.get_console_size();
    if (sz.width > 0 && sz.height > 0) {
        term_w_ = sz.width;
        term_h_ = sz.height;
    }
}

void reconciler::setup_tree(component_base* comp) {
    comp->strand_ = &strand_;
    comp->ctx_ = &ctx_;
    comp->schedule_render_cb_ = schedule_render_callback();
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

void reconciler::set_theme(const theme& t) {
    theme_ = t;
    mark_dirty();
}

bool reconciler::dispatch_key(const key_event& e) {
    if (root_ == nullptr || !mounted_) {
        return false;
    }
    return event_dispatcher_.dispatch_key(e, root_);
}

bool reconciler::dispatch_mouse(const mouse_event& e) {
    if (root_ == nullptr || !mounted_) {
        return false;
    }
    if (dirty_) {
        flush();
    }
    return event_dispatcher_.dispatch_mouse(e, root_, prev_layout_, prev_tree_, renderer_.scrollbar_hits());
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
    rendering_ = true;
    dirty_ = false;

    refresh_term_size();
    renderer_.set_term_size(term_w_, term_h_);

    if (current_screen_.dimx() != term_w_ || current_screen_.dimy() != term_h_) {
        current_screen_.resize(term_w_, term_h_);
        prev_screen_.resize(term_w_, term_h_);
    }

    element new_tree = root_->render();

    focus_mgr_.rebuild_chain(root_);

    reconcile_keys(prev_tree_, new_tree);
    tag_owners(new_tree, root_);

    auto newLayout = compute_layout(new_tree, term_w_, term_h_);

    current_screen_.clear();

    const bool has_focus = (focus_mgr_.focused() != nullptr);
    renderer_.render(new_tree, newLayout, has_focus);

    const string output = current_screen_.to_string(prev_screen_);

    prev_screen_ = current_screen_;
    prev_tree_ = move(new_tree);
    prev_layout_ = move(newLayout);

    if (!output.empty()) {
        console_.print_string(output);
    }
    rendering_ = false;
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
