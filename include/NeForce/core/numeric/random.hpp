#ifndef NEFORCE_CORE_NUMERIC_RANDOM_HPP__
#define NEFORCE_CORE_NUMERIC_RANDOM_HPP__

/**
 * @file random.hpp
 * @brief 随机数生成器
 *
 * 此文件提供了多种随机数生成器的实现，
 * 包括线性同余法、梅森旋转算法和硬件真随机数生成器。
 */

#include "NeForce/core/numeric/numeric_traits.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup RandomGenerators 随机数生成器
 * @brief 实现了三种随机数生成器
 * @{
 */

/**
 * @class random_lcd
 * @brief 线性同余随机数生成器
 *
 * 使用线性同余算法 LCD 生成伪随机数，速度快但随机性一般。
 */
class NEFORCE_API random_lcd {
public:
    using seed_type = uint32_t;  ///< 种子类型

private:
    static constexpr seed_type a = 1103515245;  ///< 乘数
    static constexpr seed_type c = 12345;       ///< 增量
    static constexpr seed_type m = 1u << 31;    ///< 模数

    seed_type seed_;  ///< 当前种子值

public:
    /**
     * @brief 默认构造函数
     * 默认使用当前时间戳值作为种子。
     */
    random_lcd();

    /**
     * @brief 带种子构造函数
     * @param seed 初始种子值
     */
    explicit random_lcd(const seed_type seed) : seed_(seed) {}

    random_lcd(const random_lcd&) = default;  ///< 拷贝构造函数
    random_lcd& operator =(const random_lcd&) = default;  ///< 拷贝赋值运算符
    random_lcd(random_lcd&&) = default;  ///< 移动构造函数
    random_lcd& operator =(random_lcd&&) = default;  ///< 移动赋值运算符

    /**
     * @brief 生成[0, max)范围内的随机整数
     * @param max 上限（不包含）
     * @return [0, max)范围内的随机整数
     */
    int next_int(const int max) {
        seed_ = a * seed_ + c;
        seed_ %= m;
        return static_cast<int>(static_cast<double>(seed_) / m * max);
    }

    /**
     * @brief 生成[min, max)范围内的随机整数
     * @param min 下限（包含）
     * @param max 上限（不包含）
     * @return [min, max)范围内的随机整数
     */
    int next_int(const int min, const int max) {
        return min + next_int(max - min);
    }

    /**
     * @brief 生成[0, INT32_MAX)范围内的随机整数
     * @return [0, INT32_MAX)范围内的随机整数
     */
    int next_int() {
        return next_int(0, numeric_traits<int32_t>::max());
    }

    /**
     * @brief 生成[0.0, 1.0)范围内的随机双精度浮点数
     * @return [0.0, 1.0)范围内的随机双精度浮点数
     */
    double next_double() {
        seed_ = a * seed_ + c;
        seed_ %= m;
        return static_cast<double>(seed_) / m;
    }

    /**
     * @brief 生成[min, max)范围内的随机双精度浮点数
     * @param min 下限（包含）
     * @param max 上限（不包含）
     * @return [min, max)范围内的随机双精度浮点数
     */
    double next_double(const double min, const double max) {
        return min + (max - min) * next_double();
    }

    /**
     * @brief 生成[0.0, max)范围内的随机双精度浮点数
     * @param max 上限（不包含）
     * @return [0.0, max)范围内的随机双精度浮点数
     */
    double next_double(const double max)  {
        return next_double(0, max);
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
    using seed_type = uint32_t;  ///< 种子类型

private:
    static constexpr size_t n = 624;            ///< 状态向量长度
    static constexpr size_t m = 397;            ///< 中间偏移量
    static constexpr seed_type a = 0x9908b0df;  ///< 旋转矩阵常数
    static constexpr seed_type u = 11;          ///< 位掩码1
    static constexpr seed_type s = 7;           ///< 位移量1
    static constexpr seed_type b = 0x9d2c5680;  ///< 位掩码2
    static constexpr seed_type t = 15;          ///< 位移量2
    static constexpr seed_type c = 0xefc60000;  ///< 位掩码3
    static constexpr seed_type l = 18;          ///< 位移量3

    seed_type state_[n] = {};  ///< 状态向量
    size_t index_ = n;         ///< 当前状态索引

    /**
     * @brief 执行状态向量的旋转操作
     */
    void twist();

public:
    /**
     * @brief 默认构造函数
     * 默认使用当前时间戳值作为种子。
     */
    random_mt();

    /**
     * @brief 带种子构造函数
     * @param seed 初始种子值
     */
    explicit random_mt(const seed_type seed) { set_seed(seed); }

    random_mt(const random_mt& other) = default;  ///< 拷贝构造函数
    random_mt& operator =(const random_mt& other) = default;  ///< 拷贝赋值运算符
    random_mt(random_mt&& other) noexcept = default;  ///< 移动构造函数
    random_mt& operator =(random_mt&& other) noexcept = default;  ///< 移动赋值运算符

    /**
     * @brief 设置随机数种子
     * @param seed 新种子值
     */
    void set_seed(seed_type seed);

    /**
     * @brief 生成[0, max)范围内的随机整数
     * @param max 上限（不包含）
     * @return [0, max)范围内的随机整数
     */
    int next_int(int max);

    /**
     * @brief 生成[min, max)范围内的随机整数
     * @param min 下限（包含）
     * @param max 上限（不包含）
     * @return [min, max)范围内的随机整数
     */
    int next_int(const int min, const int max) {
        if (min >= max) return min;
        return min + next_int(max - min);
    }

    /**
     * @brief 生成[0, numeric_traits<int32_t>::max())范围内的随机整数
     * @return [0, numeric_traits<int32_t>::max())范围内的随机整数
     */
    int next_int() {
        return next_int(0, numeric_traits<int32_t>::max());
    }

    /**
     * @brief 生成[0.0, 1.0)范围内的随机双精度浮点数
     * @return [0.0, 1.0)范围内的随机双精度浮点数
     */
    double next_double();

    /**
     * @brief 生成[min, max)范围内的随机双精度浮点数
     * @param min 下限（包含）
     * @param max 上限（不包含）
     * @return [min, max)范围内的随机双精度浮点数
     */
    double next_double(const double min, const double max) {
        if (min >= max) return min;
        return min + (max - min) * next_double();
    }

    /**
     * @brief 生成[0.0, max)范围内的随机双精度浮点数
     * @param max 上限（不包含）
     * @return [0.0, max)范围内的随机双精度浮点数
     */
    double next_double(const double max) {
        return next_double(0.0, max);
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
public:
    /**
     * @brief 生成[min, max)范围内的随机整数
     * @param min 下限（包含）
     * @param max 上限（不包含）
     * @return [min, max)范围内的随机整数
     */
    static int32_t next_int(int32_t min, int32_t max);

    /**
     * @brief 生成[0, max)范围内的随机整数
     * @param max 上限（不包含）
     * @return [0, max)范围内的随机整数
     */
    static int32_t next_int(const int32_t max) { return next_int(0, max); }

    /**
     * @brief 生成[0.0, 1.0)范围内的随机双精度浮点数
     * @return [0.0, 1.0)范围内的随机双精度浮点数
     */
    static double next_double();

    /**
     * @brief 检查系统是否支持真随机数生成
     * @return 如果系统支持真随机数生成则返回true，否则返回false
     */
    static bool is_supported();

private:
    /**
     * @brief 从系统获取随机字节
     * @param buffer 存储随机字节的缓冲区
     * @param length 需要的字节数
     *
     * 从系统随机源获取随机字节。
     */
    static void get_random_bytes(byte_t* buffer, size_t length);
};

/** @} */ // RandomGenerators

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_NUMERIC_RANDOM_HPP__
