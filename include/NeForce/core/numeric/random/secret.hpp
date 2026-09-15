#ifndef NEFORCE_CORE_NUMERIC_RANDOM_SECRET_HPP__
#define NEFORCE_CORE_NUMERIC_RANDOM_SECRET_HPP__

/**
 * @file secret.hpp
 * @brief 真随机数生成器
 */

#include "NeForce/core/numeric/random/engine.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup RandomGenerators 随机数生成器
 * @{
 */

/**
 * @class secret
 * @brief 真随机数生成器
 *
 * 使用系统提供的硬件随机源生成真随机数，适用于加密等安全场景。
 * 所有取值接口都是静态的，不需要实例化。
 *
 * @note 该生成器无状态，不提供引擎语义的 set_seed() 与 discard()
 */
class NEFORCE_API secret {
public:
    using result_type = uint32_t; ///< 结果类型

    /**
     * @brief 产生一个真随机数
     * @return [0, 2^32) 范围内的真随机数
     */
    NEFORCE_NODISCARD result_type operator()() const { return generate_32bit(); }

    /**
     * @brief 获取随机数下界
     * @return 0
     */
    NEFORCE_NODISCARD static constexpr result_type min() noexcept { return 0; }

    /**
     * @brief 获取随机数上界
     * @return 2^32 - 1
     */
    NEFORCE_NODISCARD static constexpr result_type max() noexcept { return numeric_traits<result_type>::max(); }

    /**
     * @brief 生成 [0, max) 范围内的随机整数
     * @tparam T 整形类型
     * @param max 上限（不包含）
     * @return [0, max) 范围内的随机整数
     */
    template <typename T>
    NEFORCE_NODISCARD static T next_int(T max) {
        static_assert(is_integral_v<T>, "only integral types are supported");

        if (max <= 0) {
            return 0;
        }
        if (max == 1) {
            return 0;
        }
        return static_cast<T>(
                _NEFORCE lemire_bounded([]() { return secret::generate_64bit(); }, static_cast<uint64_t>(max)));
    }

    /**
     * @brief 生成 [min, max) 范围内的随机整数
     * @tparam T 整形类型
     * @param min 下限（包含）
     * @param max 上限（不包含）
     * @return [min, max) 范围内的随机整数
     */
    template <typename T>
    NEFORCE_NODISCARD static T next_int(T min, T max) {
        if (min >= max) {
            return min;
        }
        return min + secret::next_int<T>(max - min);
    }

    /**
     * @brief 生成完整范围的随机整数
     * @tparam T 整形类型
     * @return 完整范围的随机整数
     */
    template <typename T>
    NEFORCE_NODISCARD static T next_int() {
        static_assert(is_integral_v<T>, "only integral types are supported");
        return static_cast<T>(secret::generate(bool_constant<sizeof(T) <= 4>()));
    }

    /**
     * @brief 生成 [0, max) 范围内的随机 64 位整数
     * @param max 上限（不包含）
     * @return [0, max) 范围内的随机 64 位整数
     */
    NEFORCE_NODISCARD static uint64_t next_uint64(uint64_t max) {
        if (max == 0 || max == 1) {
            return 0;
        }
        return lemire_bounded([]() { return secret::generate_64bit(); }, max);
    }

    /**
     * @brief 生成完整范围的随机 64 位整数
     * @return 完整范围的随机 64 位整数
     */
    NEFORCE_NODISCARD static uint64_t next_uint64() { return generate_64bit(); }

    /**
     * @brief 生成 [0, 1) 范围内的随机浮点数
     * @tparam T 浮点类型
     * @return [0, 1) 范围内的随机浮点数
     */
    template <typename T>
    NEFORCE_NODISCARD static T next_float() {
        static_assert(is_floating_point_v<T>, "only floating point types are supported");
        auto gen = secret::generate(bool_constant<sizeof(T) <= 4>());
        using IntT = decay_t<decltype(gen)>;
        return static_cast<T>(gen) / static_cast<T>(numeric_traits<IntT>::max());
    }

    /**
     * @brief 生成 [min, max) 范围内的随机浮点数
     * @tparam T 浮点类型
     * @param min 下限（包含）
     * @param max 上限（不包含）
     * @return [min, max) 范围内的随机浮点数
     */
    template <typename T>
    NEFORCE_NODISCARD static T next_float(T min, T max) {
        static_assert(is_floating_point_v<T>, "only floating point types are supported");
        if (min >= max) {
            return min;
        }
        return min + (max - min) * secret::next_float<T>();
    }

    /**
     * @brief 生成 [0, max) 范围内的随机浮点数
     * @tparam T 浮点类型
     * @param max 上限（不包含）
     * @return [0, max) 范围内的随机浮点数
     */
    template <typename T>
    NEFORCE_NODISCARD static T next_float(T max) {
        static_assert(is_floating_point_v<T>, "only floating point types are supported");
        return secret::next_float(static_cast<T>(0), max);
    }

    /**
     * @brief 检查系统是否支持真随机数生成
     * @return 如果系统支持真随机数生成则返回true，否则返回false
     */
    NEFORCE_NODISCARD static bool system_supported();

private:
    static void get_random_bytes(byte_t* buffer, size_t length);

    static uint32_t generate_32bit() {
        uint32_t value = 0;
        get_random_bytes(reinterpret_cast<byte_t*>(&value), sizeof(value));
        return value;
    }

    static uint64_t generate_64bit() {
        uint64_t value = 0;
        get_random_bytes(reinterpret_cast<byte_t*>(&value), sizeof(value));
        return value;
    }

    static decltype(auto) generate(true_type /*unused*/) { return generate_32bit(); }
    static decltype(auto) generate(false_type /*unused*/) { return generate_64bit(); }
};

/** @} */ // RandomGenerators

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_NUMERIC_RANDOM_SECRET_HPP__
