#include <NeForce/tui/component/animation.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

animator::animator(float* value, const float to, const milliseconds dur, easing::function easing_fn,
                   const milliseconds delay) :
value_(value),
from_(*value),
to_(to),
duration_ms_(dur),
delay_ms_(delay),
easing_fn_(move(easing_fn)) {}

bool animator::on_animation(const milliseconds delta) {
    if (done_) {
        return false;
    }
    if (value_ == nullptr) {
        done_ = true;
        return false;
    }

    if (!started_) {
        delay_ms_ -= delta;
        if (delay_ms_ > 0_ms) {
            return true;
        }
        started_ = true;
    }

    elapsed_ms_ += delta;
    if (elapsed_ms_ >= duration_ms_) {
        *value_ = to_;
        done_ = true;
        return false;
    }

    const float t = static_cast<float>(elapsed_ms_.count()) / static_cast<float>(duration_ms_.count());
    const float eased_t = easing_fn_(t < 0.0F ? 0.0F : (t > 1.0F ? 1.0F : t));
    *value_ = from_ + (to_ - from_) * eased_t;
    return true;
}

void animator::reset(const float to, const milliseconds dur) {
    from_ = (value_ != nullptr) ? *value_ : from_;
    to_ = to;
    duration_ms_ = dur;
    elapsed_ms_ = 0_ms;
    done_ = false;
    started_ = false;
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
