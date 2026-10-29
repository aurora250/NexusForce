#ifndef NEFORCE_TUI_CHART_GEOMETRY_HPP__
#define NEFORCE_TUI_CHART_GEOMETRY_HPP__

/**
 * @file chart_geometry.hpp
 * @brief DOM chart 与 canvas chart 共用的刻度与数值几何
 *
 * 两种图表视觉不同：
 *
 * - DOM chart：声明式坐标轴、图例、选中提示，支持多种图表类型
 * - canvas chart：流式时间窗口、缺口、CPU 镜像光栅
 *
 * 但刻度量化、数值归一化与“数值 → 行”的映射必须一致，
 * 统一实现在这里，避免两套后端各自演化出不同口径。
 */

#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/numeric/math.hpp"
#include "NeForce/core/numeric/numeric_types.hpp"
#include "NeForce/core/utility/optional.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 坐标轴刻度
 */
struct chart_axis_scale {
    double min = 0.0;     ///< 轴下限
    double max = 1.0;     ///< 轴上限
    double step = 1.0;    ///< 量化步长
    vector<double> ticks; ///< 刻度值
    bool valid = false;   ///< 是否由数据推导得到
};

/**
 * @brief 将原始步长量化为 1/2/5×10^n
 * @param raw 原始步长
 * @return 量化步长，非法输入返回 1
 */
inline double chart_nice_step(const double raw) {
    if (!is_finite(raw) || raw <= 0.0) {
        return 1.0;
    }
    const auto exponent = static_cast<double>(floor(logarithm_10(static_cast<decimal_t>(raw))));
    const int magnitude = static_cast<int>(absolute(exponent));
    double base = 1.0;
    for (int index = 0; index < magnitude && is_finite(base); ++index) {
        base *= 10.0;
    }
    if (exponent < 0.0) {
        base = 1.0 / base;
    }
    if (!is_finite(base) || base <= 0.0) {
        return 1.0;
    }
    const double fraction = raw / base;
    double nice = 10.0;
    if (fraction <= 1.0) {
        nice = 1.0;
    } else if (fraction <= 2.0) {
        nice = 2.0;
    } else if (fraction <= 5.0) {
        nice = 5.0;
    }
    return nice * base;
}

/**
 * @brief 将图表上限量化为 1/2/5×10^n
 * @param value 原始上限
 * @return 量化上限，下限为 1
 */
inline double chart_nice_max(const double value) noexcept {
    if (!is_finite(value) || value <= 1.0) {
        return 1.0;
    }
    double scale = 1.0;
    while (scale * 10.0 <= value) {
        scale *= 10.0;
    }
    const double fraction = value / scale;
    const double step = fraction <= 1.0 ? 1.0 : (fraction <= 2.0 ? 2.0 : (fraction <= 5.0 ? 5.0 : 10.0));
    return max(1.0, scale * step);
}

/**
 * @brief 计算坐标轴刻度
 * @param data_min 数据下限
 * @param data_max 数据上限
 * @param tick_count 期望刻度数，至少 2
 * @param zero_baseline 是否强制包含零基线
 * @param forced_min 强制下限
 * @param forced_max 强制上限
 * @return 刻度
 */
inline chart_axis_scale chart_axis_scale_for(const double data_min, const double data_max, const int tick_count,
                                             const bool zero_baseline, const optional<double> forced_min = none,
                                             const optional<double> forced_max = none) {
    chart_axis_scale scale;
    double lower = data_min;
    double upper = data_max;
    if (forced_min.has_value() && is_finite(forced_min.value())) {
        lower = forced_min.value();
    }
    if (forced_max.has_value() && is_finite(forced_max.value())) {
        upper = forced_max.value();
    }
    if (zero_baseline) {
        lower = min(lower, 0.0);
        upper = max(upper, 0.0);
    }
    if (lower > upper) {
        const double swap = lower;
        lower = upper;
        upper = swap;
    }
    if (upper - lower <= 0.0 || !is_finite(upper - lower)) {
        const double magnitude = max(1.0, absolute(lower));
        lower -= magnitude * 0.5;
        upper += magnitude * 0.5;
    }
    const int target_ticks = max(2, tick_count);
    const double raw_step = (upper - lower) / static_cast<double>(target_ticks);
    const double step = chart_nice_step(raw_step);
    scale.step = step;
    if (forced_min.has_value() || forced_max.has_value()) {
        scale.min = lower;
        scale.max = upper;
    } else {
        scale.min = static_cast<double>(floor(lower / step)) * step;
        scale.max = static_cast<double>(ceil(upper / step)) * step;
    }
    if (scale.max - scale.min <= 0.0 || !is_finite(scale.max - scale.min)) {
        scale.min = lower;
        scale.max = lower + step;
    }
    const double epsilon = step * 1e-6;
    double tick = static_cast<double>(ceil(scale.min / step)) * step;
    for (int index = 0; index < 64 && tick <= scale.max + epsilon; ++index) {
        scale.ticks.push_back(tick);
        tick += step;
    }
    if (scale.ticks.empty()) {
        scale.ticks.push_back(scale.min);
        scale.ticks.push_back(scale.max);
    }
    scale.valid = true;
    return scale;
}

/**
 * @brief 将数值归一化到 0..1
 * @param value 数值
 * @param minimum 下限
 * @param maximum 上限
 * @param logarithmic 是否按 log1p 归一化
 * @return 归一化值；非法输入或退化区间返回 0
 */
inline double chart_value_fraction(const double value, const double minimum, const double maximum,
                                   const bool logarithmic = false) {
    if (!is_finite(value) || !is_finite(minimum) || !is_finite(maximum) || maximum <= minimum) {
        return 0.0;
    }
    const double clamped = max(minimum, min(maximum, value));
    double raw = 0.0;
    if (logarithmic) {
        const double denominator =
                static_cast<double>(logarithm_1p(maximum)) - static_cast<double>(logarithm_1p(minimum));
        if (denominator <= 0.0 || !is_finite(denominator)) {
            return 0.0;
        }
        raw = (static_cast<double>(logarithm_1p(clamped)) - static_cast<double>(logarithm_1p(minimum))) / denominator;
    } else {
        raw = (clamped - minimum) / (maximum - minimum);
    }
    if (!is_finite(raw) || raw <= 0.0) {
        return 0.0;
    }
    return raw >= 1.0 ? 1.0 : raw;
}

/**
 * @brief 将 0..1 的归一化值映射到行
 * @param fraction 归一化值
 * @param top 起始行
 * @param height 可用行数
 * @param inverted 为 true 时 0 在底部，否则 0 在顶部
 * @return 行号
 */
inline int chart_row_for_fraction(const double fraction, const int top, const int height, const bool inverted = false) {
    const int usable = max(0, height - 1);
    double ratio = fraction;
    if (!is_finite(ratio)) {
        ratio = 0.0;
    }
    ratio = ratio <= 0.0 ? 0.0 : (ratio >= 1.0 ? 1.0 : ratio);
    const double position = inverted ? 1.0 - ratio : ratio;
    return top + static_cast<int>(round(position * static_cast<double>(usable)));
}

/**
 * @brief 将数值映射到行（浮点）
 * @param value 数值
 * @param scale 坐标轴刻度
 * @param top 起始行
 * @param height 可用行数
 * @return 行位置
 * @note 数值越大行越靠上（终端坐标）
 */
inline double chart_value_row_f(const double value, const chart_axis_scale& scale, const int top, const int height) {
    const double span = scale.max - scale.min;
    const int usable = max(0, height - 1);
    if (span <= 0.0 || !is_finite(span)) {
        return static_cast<double>(top + usable);
    }
    double ratio = (value - scale.min) / span;
    if (!is_finite(ratio) || ratio <= 0.0) {
        ratio = 0.0;
    } else if (ratio >= 1.0) {
        ratio = 1.0;
    }
    return static_cast<double>(top) + (1.0 - ratio) * static_cast<double>(usable);
}

/**
 * @brief 将数值映射到行
 * @param value 数值
 * @param scale 坐标轴刻度
 * @param top 起始行
 * @param height 可用行数
 * @return 行号
 */
inline int chart_value_row(const double value, const chart_axis_scale& scale, const int top, const int height) {
    return static_cast<int>(round(chart_value_row_f(value, scale, top, height)));
}

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_CHART_GEOMETRY_HPP__
