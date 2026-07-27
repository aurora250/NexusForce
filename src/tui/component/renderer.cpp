#include <NeForce/tui/component/renderer.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    class renderer_component final : public component_base {
    public:
        renderer_component(unique_ptr<component_base> child, function<element()> fn) :
        render_fn_(move(fn)) {
            if (child != nullptr) {
                add_child(move(child));
            }
        }

        element render() override { return render_fn_(); }

        bool on_key(const key_event& e) override {
            if (active_child() != nullptr) {
                return active_child()->on_key(e);
            }
            return false;
        }

        bool on_mouse(const mouse_event& e) override {
            if (active_child() != nullptr) {
                return active_child()->on_mouse(e);
            }
            return false;
        }

        NEFORCE_NODISCARD bool focusable() const override {
            if (active_child() != nullptr) {
                return active_child()->focusable();
            }
            return false;
        }

    private:
        function<element()> render_fn_;
    };

    class focus_renderer_component final : public component_base {
    public:
        focus_renderer_component(unique_ptr<component_base> child, function<element(bool)> fn) :
        render_fn_(move(fn)) {
            if (child != nullptr) {
                add_child(move(child));
            }
        }

        element render() override { return render_fn_(focused()); }

        bool on_key(const key_event& e) override {
            if (active_child() != nullptr) {
                return active_child()->on_key(e);
            }
            return false;
        }

        NEFORCE_NODISCARD bool focusable() const override {
            if (active_child() != nullptr) {
                return active_child()->focusable();
            }
            return false;
        }

    private:
        function<element(bool)> render_fn_;
    };

    class catch_event_component final : public component_base {
    public:
        catch_event_component(unique_ptr<component_base> child, function<bool(const key_event&)> fn) :
        handler_(move(fn)) {
            add_child(move(child));
        }

        element render() override { return active_child()->render(); }

        bool on_key(const key_event& e) override {
            if (handler_(e)) {
                return true;
            }
            return active_child()->on_key(e);
        }

        NEFORCE_NODISCARD bool focusable() const override { return active_child()->focusable(); }

    private:
        function<bool(const key_event&)> handler_;
    };

    class maybe_component final : public component_base {
    public:
        maybe_component(unique_ptr<component_base> child, const bool* show) :
        show_(show) {
            add_child(move(child));
        }

        element render() override {
            if (show_ != nullptr && !*show_) {
                return element::empty();
            }
            return active_child()->render();
        }

        bool on_key(const key_event& e) override {
            if (show_ != nullptr && !*show_) {
                return false;
            }
            return active_child()->on_key(e);
        }

        NEFORCE_NODISCARD bool focusable() const override {
            if (show_ != nullptr && !*show_) {
                return false;
            }
            return active_child()->focusable();
        }

    private:
        const bool* show_;
    };
} // anonymous namespace


unique_ptr<component_base> renderer(unique_ptr<component_base> child, function<element()> render_fn) {
    return make_unique<renderer_component>(move(child), move(render_fn));
}

unique_ptr<component_base> renderer(unique_ptr<component_base> child, function<element(bool focused)> render_fn) {
    return make_unique<focus_renderer_component>(move(child), move(render_fn));
}

unique_ptr<component_base> renderer(function<element()> render_fn) {
    return make_unique<renderer_component>(nullptr, move(render_fn));
}

unique_ptr<component_base> catch_event(unique_ptr<component_base> child, function<bool(const key_event&)> on_event) {
    return make_unique<catch_event_component>(move(child), move(on_event));
}

unique_ptr<component_base> maybe(unique_ptr<component_base> child, const bool* show) {
    return make_unique<maybe_component>(move(child), show);
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
