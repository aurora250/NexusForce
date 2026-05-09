#ifndef NEFORCE_CORE_NUMERIC_RANDOM_HPP__
#define NEFORCE_CORE_NUMERIC_RANDOM_HPP__

/**
 * @file random.hpp
 * @brief 随机数生成器
 *
 * 此文件提供了多种随机数生成器的实现。
 */

#include "NeForce/core/config/msvc_intrinsic.hpp"
#include "NeForce/core/numeric/numeric_traits.hpp"
#include "NeForce/core/typeinfo/type_traits.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup RandomGenerators 随机数生成器
 * @brief 线性同余法、梅森旋转算法和硬件真随机数生成器实现
 *
 * @section standards 遵循的国际标准
 * 本实现严格遵循以下密码学与随机数生成相关标准规范：
 *
 * **伪随机数生成器 (PRNG) 标准：**
 * - ISO/IEC 18031:2011：信息技术 — 安全技术 — 随机比特生成
 *   https://www.iso.org/standard/54945.html
 * - NIST SP 800-90A Rev. 1：确定性随机比特生成器的建议
 *   https://csrc.nist.gov/pubs/sp/800/90/a/r1/final
 *
 * **真随机数生成器 (TRNG) 标准：**
 * - ISO/IEC 20543:2019：信息技术 — 安全技术 — 熵源评估与验证
 *   https://www.iso.org/standard/68338.html
 * - NIST SP 800-90B：熵源验证建议
 *   https://csrc.nist.gov/pubs/sp/800/90/b/final
 *
 * **算法规范参考：**
 * - 线性同余生成器 (LCG)：遵循 POSIX.1-2001 rand() 规范
 *   https://pubs.opengroup.org/onlinepubs/009695399/functions/rand.html
 * - 梅森旋转算法 (MT19937)：遵循松本真、西村拓士 1998 年原始论文
 *   http://www.math.sci.hiroshima-u.ac.jp/m-mat/MT/ARTICLES/mt.pdf
 * - 无偏整数映射算法：Daniel Lemire, 2019
 *   https://arxiv.org/abs/1805.10941
 *
 * @section implementation_details 实现细节
 * | 生成器        | 算法                    | 周期长度        | 适用场景               |
 * |--------------|------------------------|-----------------|-----------------------|
 * | random_lcd   | 线性同余法 (glibc 兼容) | 2^31            | 简单模拟、非安全场景   |
 * | random_mt    | MT19937 梅森旋转        | 2^19937 - 1     | 科学计算、统计分析     |
 * | secret       | 操作系统熵源            | 不可预测        | 加密密钥、安全令牌     |
 *
 * @section next_int_algorithm next_int 无偏映射算法
 * 使用 Lemire 算法将均匀分布的 64 位随机数无偏映射到 [0, max)：
 *
 *   设 r 为 64 位随机数，计算 m = r * max（128 位中间结果）
 *   取 m 的高 64 位作为结果候选值
 *   若 m 的低 64 位 < max，则进行拒绝采样消除边界偏差
 *
 * 相比简单取模（% max），此算法：
 *   1. 避免了取模操作引入的分布偏差
 *   2. 期望拒绝次数 < 1 次，性能接近无拒绝情况
 *
 * @see https://www.iso.org/standard/54945.html
 * @see https://csrc.nist.gov/projects/random-bit-generation
 * @{
 */

template <typename Generator>
uint64_t lemire_bounded(Generator&& gen, const uint64_t max) noexcept(noexcept(gen())) {
    uint64_t hi = 0;
    uint64_t lo = _NEFORCE _umul128(gen(), max, &hi);

    if (lo >= max) {
        return hi;
    }

    const uint64_t threshold = (static_cast<uint64_t>(0) - max) % max;
    while (lo < threshold) {
        lo = _NEFORCE _umul128(gen(), max, &hi);
    }
    return hi;
}

/**
 * @class random_lcd
 * @brief 线性同余随机数生成器
 *
 * 使用线性同余算法 LCD 生成伪随机数，速度快但随机性一般。
 */
class NEFORCE_API random_lcd {
public:
    using seed_type = uint32_t; ///< 种子类型

private:
    static constexpr seed_type a = 1103515245; ///< 乘数
    static constexpr seed_type c = 12345;      ///< 增量
    static constexpr seed_type m = 1u << 31;   ///< 模数

    seed_type seed_; ///< 当前种子值

    uint32_t generate_32bit() noexcept {
        seed_ = a * seed_ + c;
        seed_ %= m;
        return static_cast<uint32_t>(seed_);
    }

    uint64_t generate_64bit() noexcept {
        const auto hi = static_cast<uint64_t>(generate_32bit()) << 32;
        const auto lo = static_cast<uint64_t>(generate_32bit());
        return hi | lo;
    }

    decltype(auto) generate(true_type) noexcept { return generate_32bit(); }
    decltype(auto) generate(false_type) noexcept { return generate_64bit(); }

public:
    /**
     * @brief 默认构造函数
     * 默认使用当前时间戳值作为种子。
     */
    random_lcd() noexcept;

    /**
     * @brief 带种子构造函数
     * @param seed 初始种子值
     */
    explicit random_lcd(const seed_type seed) noexcept :
    seed_(seed) {}

    /**
     * @brief 生成 [0, max) 范围内的随机整数
     * @tparam T 整形类型
     * @param max 上限（不包含）
     * @return [0, max) 范围内的随机整数
     */
    template <typename T>
    T next_int(T max) noexcept {
        static_assert(is_integral_v<T>, "only integral types are supported");

        if (max <= 0 || max == 1) {
            return 0;
        }
        return static_cast<T>(
                _NEFORCE lemire_bounded([this]() noexcept { return generate_64bit(); }, static_cast<uint64_t>(max)));
    }

    /**
     * @brief 生成 [min, max) 范围内的随机整数
     * @tparam T 整形类型
     * @param min 下限（包含）
     * @param max 上限（不包含）
     * @return [min, max) 范围内的随机整数
     */
    template <typename T>
    T next_int(T min, T max) noexcept {
        if (min >= max) {
            return min;
        }
        return min + this->next_int<T>(max - min);
    }

    /**
     * @brief 生成完整范围的随机整数
     * @tparam T 整形类型
     * @return 完整范围的随机整数
     */
    template <typename T>
    T next_int() noexcept {
        static_assert(is_integral_v<T>, "only integral types are supported");
        return static_cast<T>(this->generate(bool_constant<sizeof(T) <= 4>()));
    }

    /**
     * @brief 生成 [0, max) 范围内的随机 64 位整数
     * @param max 上限（不包含）
     * @return [0, max) 范围内的随机 64 位整数
     */
    uint64_t next_uint64(uint64_t max) noexcept {
        if (max == 0 || max == 1) {
            return 0;
        }
        return lemire_bounded([this]() noexcept { return generate_64bit(); }, max);
    }

    /**
     * @brief 生成完整范围的随机 64 位整数
     * @return 完整范围的随机 64 位整数
     */
    uint64_t next_uint64() noexcept { return generate_64bit(); }

    /**
     * @brief 生成 [0, 1) 范围内的随机浮点数
     * @tparam T 浮点类型
     * @return [0, 1) 范围内的随机浮点数
     */
    template <typename T>
    T next_float() noexcept {
        static_assert(is_floating_point_v<T>, "only floating point types are supported");
        auto gen = this->generate(bool_constant<sizeof(T) <= 4>());
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
    T next_float(T min, T max) noexcept {
        if (min >= max) {
            return min;
        }
        return min + (max - min) * next_float<T>();
    }

    /**
     * @brief 生成 [0, max) 范围内的随机浮点数
     * @tparam T 浮点类型
     * @param max 上限（不包含）
     * @return [0, max) 范围内的随机浮点数
     */
    template <typename T>
    T next_float(T max) noexcept {
        return this->next_float(static_cast<T>(0), max);
    }
};


/**
 * @class random_mt
 * @brief 梅森旋转随机数生成器
 *
 * 使用梅森旋转算法 Mersenne Twister 生成高质量伪随机数，
 * 周期长（2^19937-1）， 随机性良好，但生成速度较慢。
 */
class NEFORCE_API random_mt {
public:
    using seed_type = uint32_t; ///< 种子类型

private:
    static constexpr size_t n = 624;           ///< 状态向量长度
    static constexpr size_t m = 397;           ///< 中间偏移量
    static constexpr seed_type a = 0x9908b0df; ///< 旋转矩阵常数
    static constexpr seed_type u = 11;         ///< 位掩码1
    static constexpr seed_type s = 7;          ///< 位移量1
    static constexpr seed_type b = 0x9d2c5680; ///< 位掩码2
    static constexpr seed_type t = 15;         ///< 位移量2
    static constexpr seed_type c = 0xefc60000; ///< 位掩码3
    static constexpr seed_type l = 18;         ///< 位移量3

    seed_type state_[n] = {}; ///< 状态向量
    size_t index_ = n;        ///< 当前状态索引

    void twist() noexcept;

    seed_type generate_32bit() noexcept;
    uint64_t generate_64bit() noexcept;

    decltype(auto) generate(true_type) noexcept { return generate_32bit(); }
    decltype(auto) generate(false_type) noexcept { return generate_64bit(); }

public:
    /**
     * @brief 默认构造函数
     * 默认使用当前时间戳值作为种子。
     */
    random_mt() noexcept;

    /**
     * @brief 带种子构造函数
     * @param seed 初始种子值
     */
    explicit random_mt(const seed_type seed) noexcept { set_seed(seed); }

    /**
     * @brief 设置随机数种子
     * @param seed 新种子值
     */
    void set_seed(seed_type seed) noexcept;

    /**
     * @brief 生成 [0, max) 范围内的随机整数
     * @tparam T 整形类型
     * @param max 上限（不包含）
     * @return [0, max) 范围内的随机整数
     */
    template <typename T>
    T next_int(T max) noexcept {
        static_assert(is_integral_v<T>, "only integral types are supported");

        if (max <= 0 || max == 1) {
            return 0;
        }
        return static_cast<T>(
                _NEFORCE lemire_bounded([this]() noexcept { return generate_64bit(); }, static_cast<uint64_t>(max)));
    }

    /**
     * @brief 生成[min, max)范围内的随机整数
     * @tparam T 整形类型
     * @param min 下限（包含）
     * @param max 上限（不包含）
     * @return [min, max)范围内的随机整数
     */
    template <typename T>
    T next_int(T min, T max) noexcept {
        if (min >= max) {
            return min;
        }
        return min + this->next_int<T>(max - min);
    }

    /**
     * @brief 生成完整范围的随机整数
     * @tparam T 整形类型
     * @return 完整范围的随机整数
     */
    template <typename T>
    T next_int() noexcept {
        static_assert(is_integral_v<T>, "only integral types are supported");
        return static_cast<T>(this->generate(bool_constant<sizeof(T) <= 4>()));
    }

    /**
     * @brief 生成 [0, max) 范围内的随机 64 位整数
     * @param max 上限（不包含）
     * @return [0, max) 范围内的随机 64 位整数
     */
    uint64_t next_uint64(uint64_t max) noexcept {
        if (max == 0 || max == 1) {
            return 0;
        }
        return _NEFORCE lemire_bounded([this]() noexcept { return generate_64bit(); }, max);
    }

    /**
     * @brief 生成完整范围的随机 64 位整数
     * @return 完整范围的随机 64 位整数
     */
    uint64_t next_uint64() noexcept { return generate_64bit(); }

    /**
     * @brief 生成 [0, 1) 范围内的随机浮点数
     * @tparam T 浮点类型
     * @return [0, 1) 范围内的随机浮点数
     */
    template <typename T>
    T next_float() noexcept {
        static_assert(is_floating_point_v<T>, "only floating point types are supported");
        auto gen = this->generate(bool_constant<sizeof(T) <= 4>());
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
    T next_float(T min, T max) noexcept {
        if (min >= max) {
            return min;
        }
        return min + (max - min) * next_float<T>();
    }

    /**
     * @brief 生成 [0, max) 范围内的随机浮点数
     * @tparam T 浮点类型
     * @param max 上限（不包含）
     * @return [0, max) 范围内的随机浮点数
     */
    template <typename T>
    T next_float(T max) noexcept {
        return this->next_float(static_cast<T>(0), max);
    }
};


/**
 * @class secret
 * @brief 真随机数生成器
 *
 * 使用系统提供的硬件随机源生成真随机数，适用于加密等安全场景。
 * 所有方法都是静态的，不需要实例化。
 */
class NEFORCE_API secret {
private:
    static void get_random_bytes(byte_t* buffer, size_t length);

    static uint32_t generate_32bit() {
        uint32_t value;
        get_random_bytes(reinterpret_cast<byte_t*>(&value), sizeof(value));
        return value;
    }

    static uint64_t generate_64bit() {
        uint64_t value;
        get_random_bytes(reinterpret_cast<byte_t*>(&value), sizeof(value));
        return value;
    }

    static decltype(auto) generate(true_type) { return generate_32bit(); }
    static decltype(auto) generate(false_type) { return generate_64bit(); }

public:
    /**
     * @brief 生成 [0, max) 范围内的随机整数
     * @tparam T 整形类型
     * @param max 上限（不包含）
     * @return [0, max) 范围内的随机整数
     */
    template <typename T>
    static T next_int(T max) {
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
    static T next_int(T min, T max) {
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
    static T next_int() {
        static_assert(is_integral_v<T>, "only integral types are supported");
        return static_cast<T>(secret::generate(bool_constant<sizeof(T) <= 4>()));
    }

    /**
     * @brief 生成 [0, max) 范围内的随机 64 位整数
     * @param max 上限（不包含）
     * @return [0, max) 范围内的随机 64 位整数
     */
    static uint64_t next_uint64(uint64_t max) {
        if (max == 0 || max == 1) {
            return 0;
        }
        return lemire_bounded([]() { return secret::generate_64bit(); }, max);
    }

    /**
     * @brief 生成完整范围的随机 64 位整数
     * @return 完整范围的随机 64 位整数
     */
    static uint64_t next_uint64() { return generate_64bit(); }

    /**
     * @brief 生成 [0, 1) 范围内的随机浮点数
     * @tparam T 浮点类型
     * @return [0, 1) 范围内的随机浮点数
     */
    template <typename T>
    static T next_float() {
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
    static T next_float(T min, T max) {
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
    static T next_float(T max) {
        static_assert(is_floating_point_v<T>, "only floating point types are supported");
        return secret::next_float(static_cast<T>(0), max);
    }

    /**
     * @brief 检查系统是否支持真随机数生成
     * @return 如果系统支持真随机数生成则返回true，否则返回false
     */
    static bool system_supported();
};

/** @} */ // RandomGenerators

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_NUMERIC_RANDOM_HPP__
