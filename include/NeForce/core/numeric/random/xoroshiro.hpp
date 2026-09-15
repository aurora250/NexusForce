#ifndef NEFORCE_CORE_NUMERIC_RANDOM_XOROSHIRO_HPP__
#define NEFORCE_CORE_NUMERIC_RANDOM_XOROSHIRO_HPP__

/**
 * @file xoroshiro.hpp
 * @brief 异或移位旋转随机数生成器
 */

#include "NeForce/core/numeric/random/engine.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup RandomGenerators 随机数生成器
 * @{
 */

/**
 * @enum xoroshiro_scramble
 * @brief 异或移位旋转生成器的输出混淆方式
 */
enum class xoroshiro_scramble : byte_t {
    PLUS = 0,      ///< + 混淆：输出为两个状态字之和，速度最快
    STAR_STAR = 1, ///< ** 混淆：乘法混淆后循环移位，速度与质量折中
    PLUS_PLUS = 2  ///< ++ 混淆：求和循环移位后加回状态字，统计质量最好
};

/**
 * @class random_xoroshiro128
 * @brief 128 位异或移位旋转随机数生成器
 *
 * 使用 2 个 64 位状态字，周期为 2^128 - 1，状态转移仅由循环移位、异或与加法组成，
 * 在保持高质量统计性质的同时具有很高的吞吐量。
 *
 * @tparam Scramble 输出混淆方式
 * @warning 该生成器不具备密码学安全性
 */
template <xoroshiro_scramble Scramble = xoroshiro_scramble::PLUS_PLUS>
class random_xoroshiro128 : public random_engine<random_xoroshiro128<Scramble>> {
public:
    using result_type = uint64_t; ///< 结果类型
    using seed_type = uint64_t;   ///< 种子类型

private:
    template <typename>
    friend class random_engine;

    static constexpr bool is_plus = (Scramble == xoroshiro_scramble::PLUS);           ///< 是否为 + 混淆
    static constexpr bool is_star_star = (Scramble == xoroshiro_scramble::STAR_STAR); ///< 是否为 ** 混淆

    seed_type state_[2] = {0, 0}; ///< 状态字

    static uint64_t rotate_left(const uint64_t value, const int shift) noexcept {
        return (value << shift) | (value >> (64 - shift));
    }

    static constexpr int rotate_a() noexcept {
        return is_plus ? 55 : (is_star_star ? 24 : 49);
    } ///< 状态转移循环移位量 a
    static constexpr int rotate_b() noexcept { return is_plus ? 14 : (is_star_star ? 16 : 21); } ///< 状态转移左移量 b
    static constexpr int rotate_c() noexcept {
        return is_plus ? 36 : (is_star_star ? 37 : 28);
    } ///< 状态转移循环移位量 c

    static constexpr size_t word_bits() noexcept { return 64; }

    result_type generate_word() noexcept {
        const seed_type s0 = state_[0];
        seed_type s1 = state_[1];
        const seed_type result =
                is_plus ? (s0 + s1) : (is_star_star ? rotate_left(s0 * 5, 7) * 9 : rotate_left(s0 + s1, 17) + s0);

        s1 ^= s0;
        state_[0] = rotate_left(s0, rotate_a()) ^ s1 ^ (s1 << rotate_b());
        state_[1] = rotate_left(s1, rotate_c());
        return result;
    }

public:
    /**
     * @brief 默认构造函数
     */
    random_xoroshiro128() noexcept { set_seed(random_seed()); }

    /**
     * @brief 带种子构造函数
     * @param seed 初始种子值
     */
    explicit random_xoroshiro128(seed_type seed) noexcept { set_seed(seed); }

    /**
     * @brief 设置随机数种子
     * @param seed 新种子值
     */
    void set_seed(seed_type seed) noexcept {
        seed_type state = seed;
        state_[0] = splitmix64(state);
        state_[1] = splitmix64(state);

        if ((state_[0] | state_[1]) == 0) {
            state_[0] = 0x9e3779b97f4a7c15ULL;
        }
    }
};

/**
 * @class random_xoroshiro256
 * @brief 256 位异或移位旋转随机数生成器
 *
 * 使用 4 个 64 位状态字，周期为 2^256 - 1，适合需要极长周期的大规模模拟。
 * 状态转移采用 Blackman 与 Vigna 2018 年提出的 256 位异或移位旋转形式，每轮移位异或 4 个状态字并按固定量循环移位。
 *
 * @tparam Scramble 输出混淆方式
 * @warning 该生成器不具备密码学安全性
 */
template <xoroshiro_scramble Scramble = xoroshiro_scramble::PLUS_PLUS>
class random_xoroshiro256 : public random_engine<random_xoroshiro256<Scramble>> {
public:
    using result_type = uint64_t; ///< 结果类型
    using seed_type = uint64_t;   ///< 种子类型

private:
    template <typename>
    friend class random_engine;

    static constexpr bool is_plus = (Scramble == xoroshiro_scramble::PLUS);           ///< 是否为 + 混淆
    static constexpr bool is_star_star = (Scramble == xoroshiro_scramble::STAR_STAR); ///< 是否为 ** 混淆

    seed_type state_[4] = {0, 0, 0, 0}; ///< 状态字

    static uint64_t rotate_left(const uint64_t value, const int shift) noexcept {
        return (value << shift) | (value >> (64 - shift));
    }

    static constexpr size_t word_bits() noexcept { return 64; }

    result_type generate_word() noexcept {
        const seed_type s0 = state_[0];
        const seed_type s3 = state_[3];
        const seed_type result =
                is_plus ? (s0 + s3)
                        : (is_star_star ? rotate_left(state_[1] * 5, 7) * 9 : rotate_left(s0 + s3, 23) + s0);

        const seed_type t = state_[1] << 17;
        state_[2] ^= state_[0];
        state_[3] ^= state_[1];
        state_[1] ^= state_[2];
        state_[0] ^= state_[3];
        state_[2] ^= t;
        state_[3] = rotate_left(state_[3], 45);
        return result;
    }

public:
    /**
     * @brief 默认构造函数
     * 默认使用 @ref random_seed 生成的种子。
     */
    random_xoroshiro256() noexcept { set_seed(random_seed()); }

    /**
     * @brief 带种子构造函数
     * @param seed 初始种子值
     */
    explicit random_xoroshiro256(seed_type seed) noexcept { set_seed(seed); }

    /**
     * @brief 设置随机数种子
     * @param seed 新种子值
     */
    void set_seed(seed_type seed) noexcept {
        seed_type state = seed;
        for (size_t i = 0; i < 4; ++i) {
            state_[i] = splitmix64(state);
        }

        if ((state_[0] | state_[1] | state_[2] | state_[3]) == 0) {
            state_[0] = 0x9e3779b97f4a7c15ULL;
        }
    }
};

/** @} */ // RandomGenerators

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_NUMERIC_RANDOM_XOROSHIRO_HPP__
