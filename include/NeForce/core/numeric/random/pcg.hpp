#ifndef NEFORCE_CORE_NUMERIC_RANDOM_PCG_HPP__
#define NEFORCE_CORE_NUMERIC_RANDOM_PCG_HPP__

/**
 * @file pcg.hpp
 * @brief 置换同余随机数生成器
 */

#include "NeForce/core/numeric/random/engine.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup RandomGenerators 随机数生成器
 * @{
 */

/**
 * @class random_pcg32
 * @brief PCG 系列 32 位随机数生成器
 *
 * 采用 O'Neill 提出的 PCG（置换同余）算法：64 位线性同余状态配合 XSH-RR 输出函数，周期为 2^64，
 * 速度与统计质量均优于经典线性同余与梅森旋转，适合高性能模拟与并行计算。
 *
 * @note 同一 seed 配合不同 stream 会得到相互独立的随机序列，可用于多线程并行
 * @warning 该生成器不具备密码学安全性
 */
class NEFORCE_API random_pcg32 : public random_engine<random_pcg32> {
public:
    using result_type = uint32_t; ///< 结果类型
    using seed_type = uint64_t;   ///< 种子类型

private:
    template <typename>
    friend class random_engine;

    static constexpr seed_type multiplier = 6364136223846793005ULL;    ///< 默认乘数
    static constexpr seed_type default_stream = 0xda3e39cb94b95bdbULL; ///< 默认流标识

    seed_type state_ = 0;     ///< 线性同余状态
    seed_type increment_ = 1; ///< 线性同余增量

    static constexpr size_t word_bits() noexcept { return 32; }

    result_type generate_word() noexcept {
        const seed_type old = state_;
        state_ = old * multiplier + increment_;

        const auto xorshifted = static_cast<uint32_t>(((old >> 18U) ^ old) >> 27U);
        const auto rot = static_cast<uint32_t>(old >> 59U);
        return static_cast<uint32_t>((xorshifted >> rot) | (xorshifted << ((0U - rot) & 31U)));
    }

public:
    /**
     * @brief 默认构造函数
     * 默认使用 @ref random_seed 生成的种子。
     */
    random_pcg32() noexcept;

    /**
     * @brief 带种子构造函数
     * @param seed 初始种子值
     */
    explicit random_pcg32(seed_type seed) noexcept;

    /**
     * @brief 带种子和流构造函数
     * @param seed 初始种子值
     * @param stream 流标识，不同流之间序列相互独立
     */
    random_pcg32(seed_type seed, seed_type stream) noexcept;

    /**
     * @brief 设置随机数种子
     * @param seed 新种子值
     */
    void set_seed(seed_type seed) noexcept;

    /**
     * @brief 设置随机数种子和流
     * @param seed 新种子值
     * @param stream 流标识，不同流之间序列相互独立
     */
    void set_seed(seed_type seed, seed_type stream) noexcept;
};

/**
 * @class random_pcg64
 * @brief PCG 系列 64 位随机数生成器
 *
 * 采用 PCG XSL-RR 128/64 算法：128 位线性同余状态配合异或移位循环输出函数，周期为 2^128，
 * 适合大规模蒙特卡洛等需要超长周期与高维均匀性的场景。
 *
 * @note 同一 seed 配合不同 stream 会得到相互独立的随机序列，可用于多线程并行
 * @warning 该生成器不具备密码学安全性
 */
class NEFORCE_API random_pcg64 : public random_engine<random_pcg64> {
public:
    using result_type = uint64_t; ///< 结果类型
    using seed_type = uint64_t;   ///< 种子类型

private:
    template <typename>
    friend class random_engine;

    static constexpr seed_type multiplier_hi = 0x2360ed051fc65da4ULL;        ///< 128 位乘数高位
    static constexpr seed_type multiplier_lo = 0x4385df649fccf645ULL;        ///< 128 位乘数低位
    static constexpr seed_type default_increment_hi = 0x5851f42d4c957f2dULL; ///< 默认增量高位
    static constexpr seed_type default_increment_lo = 0x14057b7ef767814fULL; ///< 默认增量低位

    seed_type state_hi_ = 0;     ///< 状态高位
    seed_type state_lo_ = 0;     ///< 状态低位
    seed_type increment_hi_ = 0; ///< 增量高位
    seed_type increment_lo_ = 1; ///< 增量低位（奇数）

    static constexpr size_t word_bits() noexcept { return 64; }

    void step() noexcept {
        seed_type product_hi = 0;
        const seed_type product_lo = _NEFORCE __umul128(state_lo_, multiplier_lo, &product_hi);
        product_hi += state_lo_ * multiplier_hi + state_hi_ * multiplier_lo;

        const seed_type carry = (product_lo + increment_lo_ < product_lo) ? 1 : 0;
        state_lo_ = product_lo + increment_lo_;
        state_hi_ = product_hi + increment_hi_ + carry;
    }

    result_type generate_word() noexcept {
        step();

        const seed_type xorshifted = state_hi_ ^ state_lo_;
        const auto rot = static_cast<uint32_t>(state_hi_ >> 58U);
        return (xorshifted >> rot) | (xorshifted << ((0U - rot) & 63U));
    }

public:
    /**
     * @brief 默认构造函数
     */
    random_pcg64() noexcept;

    /**
     * @brief 带种子构造函数
     * @param seed 初始种子值
     */
    explicit random_pcg64(seed_type seed) noexcept;

    /**
     * @brief 带种子和流构造函数
     * @param seed 初始种子值
     * @param stream 流标识，不同流之间序列相互独立
     */
    random_pcg64(seed_type seed, seed_type stream) noexcept;

    /**
     * @brief 设置随机数种子
     * @param seed 新种子值
     */
    void set_seed(seed_type seed) noexcept;

    /**
     * @brief 设置随机数种子和流
     * @param seed 新种子值
     * @param stream 流标识，不同流之间序列相互独立
     */
    void set_seed(seed_type seed, seed_type stream) noexcept;
};

/** @} */ // RandomGenerators

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_NUMERIC_RANDOM_PCG_HPP__
