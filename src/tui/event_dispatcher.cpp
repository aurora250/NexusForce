#include <NeForce/tui/event_dispatcher.hpp>
#include <NeForce/tui/component/component.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

event_dispatcher::event_dispatcher(focus_manager& fm) :
focus_mgr_(fm) {}

bool event_dispatcher::dispatch_key(const key_event& e, component_base* root) {
    if (root == nullptr) {
        return false;
    }

    if (e.key == key_event::type::tab || e.key == key_event::type::tab_reverse) {
        if (e.key == key_event::type::tab_reverse || (e.mods & key_modifier::shift) != key_modifier::none) {
            focus_mgr_.focus_prev();
        } else {
            focus_mgr_.focus_next();
        }
        if (mark_dirty_) {
            mark_dirty_();
        }
        return true;
    }

    auto* focused = focus_mgr_.focused();
    if (focused != nullptr) {
        if (focused->on_key(e)) {
            return true;
        }
        if (!focused->pass_through()) {
            return false;
        }
    }
    if (root->on_key(e)) {
        return true;
    }
    return false;
}

bool event_dispatcher::dispatch_mouse(const mouse_event& e, component_base* root, const vector<layout_rect>& layout,
                                      const element& tree, const vector<scrollbar_hit>& hits) {
    if (root == nullptr) {
        return false;
    }

    if (drag_state_ != nullptr && e.action == mouse_action::release) {
        drag_state_ = nullptr;
        return true;
    }

    if (drag_state_ != nullptr) {
        for (const auto& info: hits) {
            if (info.scroll_state == drag_state_->scroll_state) {
                const int max_scroll = max(1, info.content_h - info.track_h);
                const int track_range = max(1, info.track_h - info.thumb_size);
                auto* st = drag_state_->scroll_state;
                if (drag_state_->vertical) {
                    int rel = e.y - info.track_y - info.thumb_size / 2;
                    rel = max(rel, 0);
                    rel = min(rel, track_range);
                    *st = rel * max_scroll / track_range;
                } else {
                    int rel = e.x - info.track_x - info.thumb_size / 2;
                    rel = max(rel, 0);
                    rel = min(rel, track_range);
                    *st = rel * max_scroll / track_range;
                }
                if (mark_dirty_) {
                    mark_dirty_();
                }
                return true;
            }
        }
    }

    if (e.action == mouse_action::wheel) {
        const int wheel_delta = (e.button == mouse_button::wheelup)     ? -3
                                : (e.button == mouse_button::wheeldown) ? 3
                                                                        : 0;
        if (wheel_delta != 0) {
            for (const auto& info: hits) {
                if (!info.vertical) {
                    continue;
                }
                if (e.x >= info.sv_x && e.x < info.sv_x + info.sv_w && e.y >= info.track_y &&
                    e.y < info.track_y + info.track_h) {
                    auto* st = static_cast<state<int>*>(info.scroll_state);
                    const int max_scroll = max(0, info.content_h - info.track_h);
                    const int new_y = st->value() + wheel_delta;
                    *st = max(0, min(new_y, max_scroll));
                    if (mark_dirty_) {
                        mark_dirty_();
                    }
                    return true;
                }
            }
        }
    }

    if (e.action == mouse_action::press) {
        for (const auto& info: hits) {
            if (e.x == info.track_x && e.y >= info.track_y && e.y < info.track_y + info.track_h) {
                const int rel_y = e.y - info.track_y;
                const int max_scroll = max(1, info.content_h - info.track_h);
                const int track_range = max(1, info.track_h - info.thumb_size);
                auto* st = static_cast<state<int>*>(info.scroll_state);
                if (rel_y >= info.thumb_pos && rel_y < info.thumb_pos + info.thumb_size) {
                    drag_state_ = &drag_state_storage_;
                    drag_state_storage_.scroll_state = st;
                    drag_state_storage_.anchor_position = e.y;
                    drag_state_storage_.anchor_value = st->value();
                    drag_state_storage_.vertical = true;
                    return true;
                }
                int target_rel = rel_y - info.thumb_size / 2;
                target_rel = max(target_rel, 0);
                target_rel = min(target_rel, track_range);
                const int new_sy = target_rel * max_scroll / track_range;
                *st = new_sy;
                drag_state_ = &drag_state_storage_;
                drag_state_storage_.scroll_state = st;
                drag_state_storage_.anchor_position = e.y;
                drag_state_storage_.anchor_value = new_sy;
                drag_state_storage_.vertical = true;
                if (mark_dirty_) {
                    mark_dirty_();
                }
                return true;
            }

            if (e.y == info.track_y && e.x >= info.track_x && e.x < info.track_x + info.track_h) {
                const int rel_x = e.x - info.track_x;
                const int max_scroll = max(1, info.content_h - info.track_h);
                const int track_range = max(1, info.track_h - info.thumb_size);
                auto* st = static_cast<state<int>*>(info.scroll_state);
                if (rel_x >= info.thumb_pos && rel_x < info.thumb_pos + info.thumb_size) {
                    drag_state_ = &drag_state_storage_;
                    drag_state_storage_.scroll_state = st;
                    drag_state_storage_.anchor_position = e.x;
                    drag_state_storage_.anchor_value = st->value();
                    drag_state_storage_.vertical = false;
                    return true;
                }
                int target_rel = rel_x - info.thumb_size / 2;
                target_rel = max(target_rel, 0);
                target_rel = min(target_rel, track_range);
                const int new_sx = target_rel * max_scroll / track_range;
                *st = new_sx;
                drag_state_ = &drag_state_storage_;
                drag_state_storage_.scroll_state = st;
                drag_state_storage_.anchor_position = e.x;
                drag_state_storage_.anchor_value = new_sx;
                drag_state_storage_.vertical = false;
                if (mark_dirty_) {
                    mark_dirty_();
                }
                return true;
            }
        }
    }

    if (e.action == mouse_action::press) {
        int idx = 0;
        const element* hitEl = find_element_at(layout, tree, e.x, e.y, idx);
        if (hitEl != nullptr && hitEl->on_click()) {
            hitEl->on_click()();
            if (root != nullptr && mark_dirty_) {
                mark_dirty_();
            }
            return true;
        }
    }

    int idx = 0;
    component_base* target = hit_test_at(layout, tree, e.x, e.y, idx, root);
    if (target != last_mouse_target_) {
        if (last_mouse_target_ != nullptr) {
            last_mouse_target_->on_mouse_leave();
        }
        last_mouse_target_ = target;
    }

    if (target != nullptr && root != nullptr) {
        if (e.action == mouse_action::press) {
            if (target->focusable()) {
                focus_mgr_.set_focus(target);
                if (mark_dirty_) {
                    mark_dirty_();
                }
            } else if (focus_mgr_.focused() != nullptr) {
                focus_mgr_.set_focus(nullptr);
                if (mark_dirty_) {
                    mark_dirty_();
                }
            }
        }
        return target->on_mouse(e);
    }

    if (e.action == mouse_action::press && focus_mgr_.focused() != nullptr) {
        focus_mgr_.set_focus(nullptr);
        if (mark_dirty_) {
            mark_dirty_();
        }
    }
    return false;
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
