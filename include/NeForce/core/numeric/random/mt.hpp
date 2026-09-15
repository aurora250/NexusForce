#ifndef NEFORCE_CORE_NUMERIC_RANDOM_MT_HPP__
#define NEFORCE_CORE_NUMERIC_RANDOM_MT_HPP__

/**
 * @file mt.hpp
 * @brief 梅森旋转随机数生成器
 */

#include "NeForce/core/numeric/random/engine.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup RandomGenerators 随机数生成器
 * @{
 */

/**
 * @class random_mt
 * @brief 梅森旋转随机数生成器
 *
 * 使用梅森旋转算法 Mersenne Twister (MT19937) 生成高质量伪随机数，
 * 周期长（2^19937 - 1），随机性良好，单次输出 32 位均匀随机数。
 *
 * @warning 该生成器不具备密码学安全性
 */
class NEFORCE_API random_mt : public random_engine<random_mt> {
public:
    using result_type = uint32_t; ///< 结果类型
    using seed_type = uint32_t;   ///< 种子类型

private:
    template <typename>
    friend class random_engine;

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

    static constexpr size_t word_bits() noexcept { return 32; }

    result_type generate_word() noexcept;

public:
    /**
     * @brief 默认构造函数
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
};

/** @} */ // RandomGenerators

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_NUMERIC_RANDOM_MT_HPP__
