#ifndef NEFORCE_TUI_COMPONENT_ANIMATION_HPP__
#define NEFORCE_TUI_COMPONENT_ANIMATION_HPP__

/**
 * @file animation.hpp
 * @brief 动画框架
 *
 * 提供缓动函数库和 animator 类，用于驱动组件动画。
 */

#include "NeForce/core/functional/function.hpp"
#include "NeForce/core/time/duration.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 缓动函数命名空间
 */
namespace easing {
    /** @brief 缓动函数类型：输入归一化时间 [0,1]，输出进度 [0,1] */
    using function = function<float(float)>;

    inline function linear() {
        return [](const float t) { return t; };
    }

    inline function quadratic_in() {
        return [](const float t) { return t * t; };
    }

    inline function quadratic_out() {
        return [](const float t) { return t * (2.0F - t); };
    }

    inline function quadratic_in_out() {
        return [](const float t) { return (t < 0.5F) ? 2.0F * t * t : -1.0F + (4.0F - 2.0F * t) * t; };
    }

    inline function cubic_in() {
        return [](const float t) { return t * t * t; };
    }

    inline function cubic_out() {
        return [](const float t) {
            const float t1 = t - 1.0F;
            return t1 * t1 * t1 + 1.0F;
        };
    }

    inline function cubic_in_out() {
        return [](const float t) {
            return (t < 0.5F) ? 4.0F * t * t * t : (t - 1.0F) * (2.0F * t - 2.0F) * (2.0F * t - 2.0F) + 1.0F;
        };
    }

    inline function sine_in() {
        return [](const float t) { return 1.0F - cosine(t * 3.14159265F / 2.0F); };
    }

    inline function sine_out() {
        return [](const float t) { return sine(t * 3.14159265F / 2.0F); };
    }

    inline function sine_in_out() {
        return [](const float t) { return -(cosine(3.14159265F * t) - 1.0F) / 2.0F; };
    }

    inline function elastic_out() {
        return [](const float t) -> float {
            if (t == 0.0F || t == 1.0F) {
                return t;
            }
            return static_cast<float>(power(2.0F, static_cast<uint32_t>(-10.0F * t)) * sine((t - 0.075F) * 6.2831853F / 0.3F)) + 1.0F;
        };
    }

    inline function bounce_out() {
        return [](float t) {
            if (t < 1.0F / 2.75F) {
                return 7.5625F * t * t;
            }
            if (t < 2.0F / 2.75F) {
                t -= 1.5F / 2.75F;
                return 7.5625F * t * t + 0.75F;
            }
            if (t < 2.5F / 2.75F) {
                t -= 2.25F / 2.75F;
                return 7.5625F * t * t + 0.9375F;
            }
            t -= 2.625F / 2.75F;
            return 7.5625F * t * t + 0.984375F;
        };
    }
} // namespace easing


/**
 * @brief 动画状态机
 *
 * 驱动一个 float 值从起始状态到达目标状态，
 * 使用可配置的缓动函数、持续时间和延迟。
 */
class NEFORCE_API animator {
public:
    /**
     * @brief 构造函数
     * @param value 要动画的 float 变量指针
     * @param to 目标值
     * @param dur 持续时间
     * @param easing_fn 缓动函数
     * @param delay 延迟启动
     */
    animator(float* value, float to, milliseconds dur, easing::function easing_fn = easing::quadratic_out(),
             milliseconds delay = 0_ms);

    /**
     * @brief 每帧调用以推进动画
     * @param delta 距上一帧的毫秒数
     * @return 动画是否仍在进行
     */
    bool on_animation(milliseconds delta);

    /**
     * @brief 获取目标值
     * @return 目标值
     */
    NEFORCE_NODISCARD float to() const noexcept { return to_; }

    /**
     * @brief 检查动画是否已完成
     * @return 是否已完成
     */
    NEFORCE_NODISCARD bool is_done() const noexcept { return done_; }

    /**
     * @brief 重置动画
     * @param to 新目标值
     * @param dur 新持续时间
     */
    void reset(float to, milliseconds dur);

private:
    float* value_;
    float from_;
    float to_;
    milliseconds duration_ms_;
    milliseconds delay_ms_;
    milliseconds elapsed_ms_ = 0_ms;
    bool done_ = false;
    bool started_ = false;
    easing::function easing_fn_;
};

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_COMPONENT_ANIMATION_HPP__
