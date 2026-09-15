#ifndef NEFORCE_CORE_NUMERIC_RANDOM_LCG_HPP__
#define NEFORCE_CORE_NUMERIC_RANDOM_LCG_HPP__

/**
 * @file lcg.hpp
 * @brief 线性同余随机数生成器
 */

#include "NeForce/core/numeric/random/engine.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup RandomGenerators 随机数生成器
 * @{
 */

/**
 * @class random_lcd
 * @brief 线性同余随机数生成器
 *
 * 使用线性同余算法 LCD 生成伪随机数，状态模数为 2^31，周期为 2^31，
 * 单次输出 31 位均匀随机数，速度快但随机性一般。
 *
 * @warning 该生成器不具备密码学安全性
 */
class NEFORCE_API random_lcd : public random_engine<random_lcd> {
public:
    using result_type = uint32_t; ///< 结果类型
    using seed_type = uint32_t;   ///< 种子类型

private:
    template <typename>
    friend class random_engine;

    static constexpr seed_type a = 1103515245; ///< 乘数
    static constexpr seed_type c = 12345;      ///< 增量
    static constexpr seed_type m = 1U << 31;   ///< 模数

    seed_type seed_; ///< 当前种子值

    static constexpr size_t word_bits() noexcept { return 31; }

    result_type generate_word() noexcept {
        seed_ = a * seed_ + c;
        seed_ %= m;
        return static_cast<uint32_t>(seed_);
    }

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
};

/** @} */ // RandomGenerators

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_NUMERIC_RANDOM_LCG_HPP__
