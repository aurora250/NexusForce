#ifndef NEFORCE_CORE_NUMERIC_RANDOM_DISTRIBUTION_HPP__
#define NEFORCE_CORE_NUMERIC_RANDOM_DISTRIBUTION_HPP__

/**
 * @file distribution.hpp
 * @brief 随机数分布函数
 *
 * 此文件提供了常用的随机数分布函数，全部为无状态自由函数，不保存任何中间状态。
 * 所有函数都基于标准库 UniformRandomBitGenerator 概念，可直接配合本库引擎、标准库引擎或第三方引擎使用。
 */

#include "NeForce/core/numeric/math.hpp"
#include "NeForce/core/numeric/random/engine.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup RandomGenerators 随机数生成器
 * @{
 */

/**
 * @brief 从随机数引擎中提取指定位数的均匀随机位
 * @tparam Engine 随机数引擎类型
 * @param engine 随机数引擎
 * @param bits 提取的位数，取值范围 [1, 64]
 * @return 均匀分布于 [0, 2^bits) 的随机数
 */
template <typename Engine>
constexpr uint64_t uniform_bits(Engine& engine, const size_t bits) noexcept {
    const size_t source_bits = engine_word_bits<Engine>();
    uint64_t value = 0;
    size_t filled = 0;

    while (filled < bits) {
        const auto draw = static_cast<uint64_t>(engine() - Engine::min());
        const size_t take = (bits - filled < source_bits) ? (bits - filled) : source_bits;
        const uint64_t mask = take >= 64 ? ~static_cast<uint64_t>(0) : ((static_cast<uint64_t>(1) << take) - 1);
        value |= (draw & mask) << (bits - filled - take);
        filled += take;
    }
    return value;
}

/**
 * @brief 生成 [0, 1) 范围内均匀分布的浮点数
 * @tparam Engine 随机数引擎类型
 * @param engine 随机数引擎
 * @return [0, 1) 范围内的均匀随机浮点数
 *
 * 使用 53 位随机精度构造，结果严格小于 1。
 */
template <typename Engine>
constexpr double uniform_real(Engine& engine) noexcept {
    return static_cast<double>(_NEFORCE uniform_bits(engine, 53)) * (1.0 / 9007199254740992.0);
}

/**
 * @brief 生成 [min, max) 范围内均匀分布的浮点数
 * @tparam Engine 随机数引擎类型
 * @param engine 随机数引擎
 * @param min 下限（包含）
 * @param max 上限（不包含）
 * @return [min, max) 范围内的均匀随机浮点数，max 不大于 min 时返回 min
 */
template <typename Engine>
constexpr double uniform_real(Engine& engine, const double min, const double max) noexcept {
    if (max <= min) {
        return min;
    }
    return min + (max - min) * _NEFORCE uniform_real(engine);
}

/**
 * @brief 生成 [min, max) 范围内均匀分布的整数
 * @tparam Engine 随机数引擎类型
 * @tparam T 整形类型
 * @param engine 随机数引擎
 * @param min 下限（包含）
 * @param max 上限（不包含）
 * @return [min, max) 范围内的均匀随机整数，max 不大于 min 时返回 min
 */
template <typename Engine, typename T>
constexpr T uniform_int(Engine& engine, const T min, const T max) noexcept {
    static_assert(is_integral_v<T>, "only integral types are supported");

    if (max <= min) {
        return min;
    }

    const uint64_t range = static_cast<uint64_t>(max) - static_cast<uint64_t>(min);
    const uint64_t offset =
            _NEFORCE lemire_bounded([&engine]() noexcept { return _NEFORCE uniform_bits(engine, 64); }, range);
    return static_cast<T>(static_cast<uint64_t>(min) + offset);
}

/**
 * @brief 生成服从伯努利分布的随机布尔值
 * @tparam Engine 随机数引擎类型
 * @param engine 随机数引擎
 * @param probability 返回 true 的概率，取值范围 [0, 1]
 * @return 以 probability 的概率返回 true
 */
template <typename Engine>
constexpr bool bernoulli(Engine& engine, const double probability) noexcept {
    if (probability <= 0.0) {
        return false;
    }
    if (probability >= 1.0) {
        return true;
    }
    return _NEFORCE uniform_real(engine) < probability;
}

/**
 * @brief 生成服从正态分布的随机浮点数
 * @tparam Engine 随机数引擎类型
 * @param engine 随机数引擎
 * @param mean 均值
 * @param stddev 标准差，必须为正数
 * @return 服从 N(mean, stddev^2) 的随机浮点数
 *
 * 使用 Marsaglia 极坐标法，不需要分布对象保存状态。
 */
template <typename Engine>
constexpr double normal(Engine& engine, const double mean = 0.0, const double stddev = 1.0) noexcept {
    double factor = 0.0;

    for (;;) {
        const double u = 2.0 * _NEFORCE uniform_real(engine) - 1.0;
        const double v = 2.0 * _NEFORCE uniform_real(engine) - 1.0;
        const double s = u * u + v * v;

        if (s < 1.0 && s != 0.0) {
            const decimal_t log_s = logarithm_e(static_cast<decimal_t>(s));
            factor = u * static_cast<double>(square_root(-2.0L * log_s / static_cast<decimal_t>(s)));
            break;
        }
    }
    return mean + stddev * factor;
}

/**
 * @brief 生成服从指数分布的随机浮点数
 * @tparam Engine 随机数引擎类型
 * @param engine 随机数引擎
 * @param lambda 速率参数，必须为正数
 * @return 服从 Exp(lambda) 的随机浮点数，期望为 1 / lambda
 */
template <typename Engine>
constexpr double exponential(Engine& engine, const double lambda) noexcept {
    return -static_cast<double>(logarithm_1p(-static_cast<decimal_t>(_NEFORCE uniform_real(engine)))) / lambda;
}

/**
 * @brief 生成服从对数均匀分布的随机浮点数
 * @tparam Engine 随机数引擎类型
 * @param engine 随机数引擎
 * @param min 下限（包含），必须为正数
 * @param max 上限（不包含），必须大于 min
 * @return 服从 [min, max) 上对数均匀分布的随机浮点数
 */
template <typename Engine>
constexpr double log_uniform(Engine& engine, const double min, const double max) noexcept {
    const decimal_t lower = logarithm_e(static_cast<decimal_t>(min));
    const decimal_t upper = logarithm_e(static_cast<decimal_t>(max));
    return static_cast<double>(
            exponential_e(lower + (upper - lower) * static_cast<decimal_t>(_NEFORCE uniform_real(engine))));
}

/**
 * @brief 生成服从泊松分布的随机整数
 * @tparam Engine 随机数引擎类型
 * @param engine 随机数引擎
 * @param mean 均值，必须为非负数
 * @return 服从 Poisson(mean) 的随机整数
 */
template <typename Engine>
constexpr uint64_t poisson(Engine& engine, const double mean) noexcept {
    if (mean <= 0.0) {
        return 0;
    }

    if (mean < 10.0) {
        const decimal_t limit = exponential_e(-static_cast<decimal_t>(mean));
        uint64_t count = 0;
        decimal_t product = 1.0L;

        do {
            ++count;
            product *= static_cast<decimal_t>(_NEFORCE uniform_real(engine));
        } while (product > limit);
        return count - 1;
    }

    const auto mu = static_cast<decimal_t>(mean);
    const decimal_t b = 0.931L + 2.53L * square_root(mu);
    const decimal_t a = -0.059L + 0.02483L * b;
    const decimal_t inverse_alpha = 1.1239L + 1.1328L / (b - 3.4L);
    const decimal_t rejection = 0.9277L - 3.6224L / (b - 2.0L);
    const decimal_t log_mean = logarithm_e(mu);
    const decimal_t log_inverse_alpha = logarithm_e(inverse_alpha);

    for (;;) {
        const auto u = static_cast<decimal_t>(_NEFORCE uniform_real(engine)) - 0.5L;
        const auto v = static_cast<decimal_t>(_NEFORCE uniform_real(engine));
        const decimal_t us = 0.5L - absolute(u);
        const int64_t k = safe_trunc(floor((2.0L * a / us + b) * u + mu + 0.43L));

        if (us >= 0.07L && v <= rejection) {
            return static_cast<uint64_t>(k);
        }
        if (k < 0 || (us < 0.013L && v > us)) {
            continue;
        }
        if (logarithm_e(v) + log_inverse_alpha - logarithm_e(a / (us * us) + b) <=
            static_cast<decimal_t>(k) * log_mean - mu - logarithm_factorial(static_cast<uint64_t>(k))) {
            return static_cast<uint64_t>(k);
        }
    }
}

/** @} */ // RandomGenerators

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_NUMERIC_RANDOM_DISTRIBUTION_HPP__
