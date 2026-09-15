#ifndef NEFORCE_CORE_NUMERIC_RANDOM_BIT_GENERATOR_HPP__
#define NEFORCE_CORE_NUMERIC_RANDOM_BIT_GENERATOR_HPP__

/**
 * @file bit_generator.hpp
 * @brief 指定位宽的位生成器适配器
 */

#include "NeForce/core/numeric/random/engine.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup RandomGenerators 随机数生成器
 * @{
 */

/**
 * @class bit_generator
 * @brief 指定位宽的位生成器适配器
 *
 * 把随机数引擎适配为按固定位宽输出随机数的生成器，用于获取非 2 的幂次字宽的均匀随机数，
 * 也便于把引擎接入只接受固定位宽随机源的算法。
 *
 * @tparam Engine 底层随机数引擎，需满足标准库 UniformRandomBitGenerator
 * @tparam W 每次生成的位数，取值范围 [1, 64]
 * @tparam UIntType 结果类型，必须为无符号整型且位宽不小于 W
 */
template <typename Engine, size_t W = 64, typename UIntType = uint64_t>
class bit_generator : public random_engine<bit_generator<Engine, W, UIntType>> {
public:
    using engine_type = Engine;   ///< 底层引擎类型
    using result_type = UIntType; ///< 结果类型

    static constexpr size_t word_size = W; ///< 每次生成的位数

private:
    template <typename>
    friend class random_engine;

    static_assert(W >= 1 && W <= 64, "bit width must be in [1, 64]");
    static_assert(W <= sizeof(UIntType) * 8, "result type is too narrow for the requested bit width");
    static_assert(is_integral_v<UIntType> && is_unsigned_v<UIntType>, "result type must be an unsigned integral type");

    static constexpr size_t source_bits = engine_word_bits<Engine>(); ///< 底层引擎单次输出的均匀随机位数

    static_assert(source_bits >= 1 && source_bits <= 64, "engine word bits must be in [1, 64]");

    Engine engine_{};      ///< 底层引擎
    uint64_t buffer_ = 0;  ///< 位缓冲区，低位为尚未使用的随机位
    size_t available_ = 0; ///< 位缓冲区中可用的随机位数

    static constexpr size_t word_bits() noexcept { return W; }

    result_type generate_word() noexcept {
        return static_cast<result_type>(extract_bits(buffer_, available_, engine_, W));
    }

public:
    /**
     * @brief 默认构造函数
     */
    bit_generator() = default;

    /**
     * @brief 带种子构造函数
     * @tparam SeedType 种子类型，需可转换为底层引擎的种子类型
     * @param seed 初始种子值，转交给底层引擎
     */
    template <typename SeedType>
    explicit bit_generator(const SeedType seed) noexcept :
    engine_(seed) {
        static_assert(is_integral_v<SeedType>, "seed must be an integral type");
    }

    /**
     * @brief 设置随机数种子
     * @tparam SeedType 种子类型，需可转换为底层引擎的种子类型
     * @param seed 新种子值
     *
     * 种子转交给底层引擎，同时清空位缓冲区。
     */
    template <typename SeedType>
    void set_seed(const SeedType seed) noexcept {
        static_assert(is_integral_v<SeedType>, "seed must be an integral type");
        engine_.set_seed(seed);
        buffer_ = 0;
        available_ = 0;
    }

    /**
     * @brief 获取底层引擎
     * @return 底层引擎引用
     */
    NEFORCE_NODISCARD Engine& base() noexcept { return engine_; }

    /**
     * @brief 获取底层引擎
     * @return 底层引擎常量引用
     */
    NEFORCE_NODISCARD const Engine& base() const noexcept { return engine_; }
};

/** @} */ // RandomGenerators

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_NUMERIC_RANDOM_BIT_GENERATOR_HPP__
