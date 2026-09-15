#ifndef NEFORCE_CORE_NUMERIC_RANDOM_ENGINE_HPP__
#define NEFORCE_CORE_NUMERIC_RANDOM_ENGINE_HPP__

/**
 * @file engine.hpp
 * @brief 随机数引擎公共设施
 *
 * 此文件提供了随机数引擎的公共基类、无偏区间映射、均匀位提取、默认种子生成与种子扩展等基础设施。
 */

#include "NeForce/core/config/msvc_intrinsic.hpp"
#include "NeForce/core/numeric/numeric_traits.hpp"
#include "NeForce/core/typeinfo/type_traits.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup RandomGenerators 随机数生成器
 * @{
 */

/**
 * @brief 使用 Lemire 算法将均匀 64 位随机数无偏映射到 [0, max)
 * @tparam Generator 无参可调用对象类型，返回均匀分布的 64 位随机数
 * @param gen 随机数产生器
 * @param max 上限（不包含）
 * @return [0, max) 范围内的随机数，max 为 0 或 1 时返回 0
 *
 * 相比简单取模，该算法避免了分布偏差，且期望拒绝次数小于 1 次。
 */
template <typename Generator>
constexpr uint64_t lemire_bounded(Generator&& gen, const uint64_t max) noexcept(noexcept(gen())) {
    uint64_t hi = 0;
    uint64_t lo = _NEFORCE __umul128(gen(), max, &hi);

    if (lo >= max) {
        return hi;
    }

    const uint64_t threshold = (static_cast<uint64_t>(0) - max) % max;
    while (lo < threshold) {
        lo = _NEFORCE __umul128(gen(), max, &hi);
    }
    return hi;
}

/**
 * @brief 生成默认随机种子
 * @return 64 位随机种子
 *
 * 种子由当前时间戳、线程局部调用序号与地址熵混合得到，
 * 同一时刻的多次调用也会返回互不相同的值，适合为多个生成器分别播种。
 */
NEFORCE_API uint64_t random_seed() noexcept;

/**
 * @brief SplitMix64 种子扩展
 * @param state 输入输出参数，种子状态，调用后推进一个步长
 * @return 混合后的 64 位随机数
 *
 * 使用 SplitMix64 算法把单个种子扩展为任意长度且相互独立的随机状态字，
 * 常用于为异或移位旋转类生成器播种。
 */
NEFORCE_NODISCARD constexpr uint64_t splitmix64(uint64_t& state) noexcept {
    state += 0x9e3779b97f4a7c15ULL;
    uint64_t z = state;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

/**
 * @brief 根据均匀随机数范围计算均匀随机位数
 * @param min_value 随机数下界
 * @param max_value 随机数上界
 * @param type_bits 随机数类型的位数
 * @return 均匀随机位数
 *
 * 用于从随机数引擎的 min() / max() 推导其单次输出包含的均匀随机位数，
 * 要求 max_value - min_value + 1 为 2 的整数次幂。
 */
NEFORCE_NODISCARD constexpr size_t random_range_bits(const uint64_t min_value, const uint64_t max_value,
                                                     const size_t type_bits) noexcept {
    const uint64_t range = max_value - min_value + 1;

    if (range == 0) {
        return type_bits;
    }

    size_t bits = 0;
    uint64_t value = range;
    while (value > 1) {
        value >>= 1;
        ++bits;
    }
    return bits;
}

/**
 * @brief 获取满足 UniformRandomBitGenerator 的引擎单次输出的均匀随机位数
 * @tparam Engine 随机数引擎类型
 * @return 均匀随机位数
 */
template <typename Engine>
NEFORCE_NODISCARD constexpr size_t engine_word_bits() noexcept {
    return random_range_bits(static_cast<uint64_t>(Engine::min()), static_cast<uint64_t>(Engine::max()),
                             sizeof(typename Engine::result_type) * 8);
}

/**
 * @brief 从随机数引擎中提取指定位数的均匀随机位
 * @tparam Engine 随机数引擎类型
 * @param buffer 输入输出参数，位缓冲区，低位为尚未使用的随机位
 * @param available 输入输出参数，位缓冲区中可用的随机位数
 * @param engine 随机数引擎
 * @param count 需要提取的位数，取值范围 [1, 64]
 * @return 提取到的 count 位均匀随机数
 */
template <typename Engine>
uint64_t extract_bits(uint64_t& buffer, size_t& available, Engine& engine, const size_t count) noexcept {
    const size_t source_bits = engine_word_bits<Engine>();
    uint64_t value = 0;
    size_t filled = 0;

    while (filled < count) {
        if (available == 0) {
            const auto draw = static_cast<uint64_t>(engine());
            constexpr uint64_t full_mask = ~static_cast<uint64_t>(0);
            buffer = source_bits >= 64 ? draw : (draw & (full_mask >> (64 - source_bits)));
            available = source_bits;
        }

        const size_t take = (count - filled < available) ? (count - filled) : available;
        const uint64_t mask = (take >= 64) ? ~static_cast<uint64_t>(0) : ((static_cast<uint64_t>(1) << take) - 1);
        value |= (buffer & mask) << (count - filled - take);
        buffer >>= take;
        available -= take;
        filled += take;
    }
    return value;
}

/**
 * @class random_engine
 * @brief 随机数引擎公共基类
 *
 * 为随机数引擎提供符合 UniformRandomBitGenerator 的基础接口以及本库统一的取值接口，
 * 派生引擎可以直接配合分布函数使用。
 *
 * 派生类只需实现原始随机数产生与字宽描述：
 * - result_type：引擎字类型，必须为无符号整型；
 * - static constexpr size_t word_bits() noexcept：单次输出的均匀随机位数；
 * - result_type generate_word() noexcept：产生 [0, 2^word_bits) 上均匀分布的字。
 *
 * @tparam Derived 派生引擎类型
 *
 * @note 派生类需声明 template <typename> friend class random_engine 授予本基类访问权限。
 */
template <typename Derived>
class random_engine {
public:
    /**
     * @brief 产生一个引擎字
     * @return [min(), max()] 上均匀分布的随机数
     */
    NEFORCE_NODISCARD decltype(auto) operator()() noexcept { return derive()->generate_word(); }

    /**
     * @brief 获取引擎字的最小值
     * @return 引擎字下界，恒为 0
     */
    NEFORCE_NODISCARD static constexpr decltype(auto) min() noexcept {
        return static_cast<typename Derived::result_type>(0);
    }

    /**
     * @brief 获取引擎字的最大值
     * @return 引擎字上界 2^word_bits() - 1
     */
    NEFORCE_NODISCARD static constexpr decltype(auto) max() noexcept {
        using word_type = typename Derived::result_type;
        return static_cast<word_type>(static_cast<word_type>(~static_cast<word_type>(0)) >>
                                      (sizeof(word_type) * 8 - Derived::word_bits()));
    }

    /**
     * @brief 丢弃指定数量的引擎字
     * @param count 需要丢弃的引擎字数量
     */
    void discard(const uint64_t count) noexcept {
        for (uint64_t i = 0; i < count; ++i) {
            static_cast<void>(derive()->generate_word());
        }
    }

    /**
     * @brief 生成 [0, max) 范围内的随机整数
     * @tparam T 整形类型
     * @param max 上限（不包含）
     * @return [0, max) 范围内的随机整数
     */
    template <typename T>
    NEFORCE_NODISCARD T next_int(T max) noexcept {
        static_assert(is_integral_v<T>, "only integral types are supported");

        if (max <= 0 || max == 1) {
            return 0;
        }
        return static_cast<T>(
                _NEFORCE lemire_bounded([this]() noexcept { return this->next_raw(64); }, static_cast<uint64_t>(max)));
    }

    /**
     * @brief 生成 [min, max) 范围内的随机整数
     * @tparam T 整形类型
     * @param min 下限（包含）
     * @param max 上限（不包含）
     * @return [min, max) 范围内的随机整数
     */
    template <typename T>
    NEFORCE_NODISCARD T next_int(T min, T max) noexcept {
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
    NEFORCE_NODISCARD T next_int() noexcept {
        static_assert(is_integral_v<T>, "only integral types are supported");
        return static_cast<T>(sizeof(T) <= 4 ? this->next_raw(32) : this->next_raw(64));
    }

    /**
     * @brief 生成 [0, max) 范围内的随机 64 位整数
     * @param max 上限（不包含）
     * @return [0, max) 范围内的随机 64 位整数
     */
    NEFORCE_NODISCARD uint64_t next_uint64(uint64_t max) noexcept {
        if (max == 0 || max == 1) {
            return 0;
        }
        return _NEFORCE lemire_bounded([this]() noexcept { return this->next_raw(64); }, max);
    }

    /**
     * @brief 生成完整范围的随机 64 位整数
     * @return 完整范围的随机 64 位整数
     */
    NEFORCE_NODISCARD uint64_t next_uint64() noexcept { return next_raw(64); }

    /**
     * @brief 生成 [0, 1) 范围内的随机浮点数
     * @tparam T 浮点类型
     * @return [0, 1) 范围内的随机浮点数
     */
    template <typename T>
    NEFORCE_NODISCARD T next_float() noexcept {
        static_assert(is_floating_point_v<T>, "only floating point types are supported");
        return sizeof(T) <= 4 ? static_cast<T>(next_raw(32)) / static_cast<T>(numeric_traits<uint32_t>::max())
                              : static_cast<T>(next_raw(64)) / static_cast<T>(numeric_traits<uint64_t>::max());
    }

    /**
     * @brief 生成 [min, max) 范围内的随机浮点数
     * @tparam T 浮点类型
     * @param min 下限（包含）
     * @param max 上限（不包含）
     * @return [min, max) 范围内的随机浮点数
     */
    template <typename T>
    NEFORCE_NODISCARD T next_float(T min, T max) noexcept {
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
    NEFORCE_NODISCARD T next_float(T max) noexcept {
        return this->next_float(static_cast<T>(0), max);
    }

private:
    uint64_t buffer_ = 0;  ///< 位缓冲区，低位为尚未使用的随机位
    size_t available_ = 0; ///< 位缓冲区中可用的随机位数

    Derived* derive() noexcept { return static_cast<Derived*>(this); }

    uint64_t next_raw(const size_t count) noexcept {
        const size_t bits = Derived::word_bits();
        return bits >= count ? static_cast<uint64_t>(derive()->generate_word()) >> (bits - count)
                             : _NEFORCE extract_bits(buffer_, available_, *derive(), count);
    }
};

/** @} */ // RandomGenerators

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_NUMERIC_RANDOM_ENGINE_HPP__
