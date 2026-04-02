#ifndef NEFORCE_CORE_NUMERIC_MATH_HPP__
#define NEFORCE_CORE_NUMERIC_MATH_HPP__

/**
 * @file math.hpp
 * @brief 数学函数库
 *
 * 此文件提供了数学函数和常量定义，
 * 包括基本数学运算、三角函数、对数函数、数值计算等常用数学功能。
 */

#include "NeForce/core/exception/exception.hpp"
NEFORCE_BEGIN_NAMESPACE__

NEFORCE_BEGIN_CONSTANTS__

/**
 * @defgroup MathConstants 数学常量
 * @brief 常用数学常量定义
 * @{
 */

NEFORCE_INLINE17 constexpr decimal_t EULER = 2.718281828459045L; ///< 自然常数 e
NEFORCE_INLINE17 constexpr decimal_t PI = 3.141592653589793L;    ///< 圆周率 π（弧度制）
NEFORCE_INLINE17 constexpr decimal_t PHI = 1.618033988749895L;   ///< 黄金分割比 φ
NEFORCE_INLINE17 constexpr decimal_t SEMI_CIRCLE = 180.0;        ///< 半圆角度 180°（角度制）
NEFORCE_INLINE17 constexpr decimal_t CIRCLE = 360.0;             ///< 全圆角度 360°（角度制）
NEFORCE_INLINE17 constexpr decimal_t EPSILON = 1e-15L;           ///< 浮点数精度容差

NEFORCE_INLINE17 constexpr uint32_t TAYLOR_CONVERGENCE = 10000U;                       ///< 泰勒展开收敛项数
NEFORCE_INLINE17 constexpr decimal_t PRECISE_TOLERANCE = TAYLOR_CONVERGENCE * EPSILON; ///< 精确容差
NEFORCE_INLINE17 constexpr decimal_t LOW_PRECISE_TOLERANCE = TAYLOR_CONVERGENCE * PRECISE_TOLERANCE; ///< 低精度容差

/**
 * @brief 预计算的斐波那契数列
 *
 * 包含前50个斐波那契数，用于快速查找。
 */
NEFORCE_INLINE17 constexpr uint64_t FIBONACCI_LIST[] = {
        0,          1,          1,          2,          3,         5,         8,         13,        21,
        34,         55,         89,         144,        233,       377,       610,       987,       1597,
        2584,       4181,       6765,       10946,      17711,     28657,     46368,     75025,     121393,
        196418,     317811,     514229,     832040,     1346269,   2178309,   3524578,   5702887,   9227465,
        14930352,   24157817,   39088169,   63245986,   102334155, 165580141, 267914296, 433494437, 701408733,
        1134903170, 1836311903, 2971215073, 4807526976, 7778742049};

NEFORCE_INLINE17 constexpr uint32_t FIBONACCI_COUNT = extent_v<decltype(FIBONACCI_LIST)>; ///< 斐波那契数列预计算数量

/** @} */ // MathConstants

NEFORCE_END_CONSTANTS__

/**
 * @defgroup MathFunctions 数学函数
 * @brief 基本数学运算函数
 * @{
 */

/**
 * @brief 计算斐波那契数
 * @param n 索引位置
 * @return 第n个斐波那契数
 *
 * 如果n小于预计算的数量，直接返回预计算结果；
 * 否则递归计算。
 */
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 uint64_t fibonacci(const uint32_t n) {
    if (n < constants::FIBONACCI_COUNT) {
        return constants::FIBONACCI_LIST[n];
    }
    uint64_t a = constants::FIBONACCI_LIST[constants::FIBONACCI_COUNT - 2];
    uint64_t b = constants::FIBONACCI_LIST[constants::FIBONACCI_COUNT - 1];
    for (uint32_t i = constants::FIBONACCI_COUNT; i <= n; ++i) {
        const uint64_t c = a + b;
        a = b;
        b = c;
    }
    return b;
}

/**
 * @brief 计算莱昂纳多数
 * @param n 索引位置
 * @return 第n个莱昂纳多数
 *
 * 莱昂纳多数：L(n) = 2 * F(n+1) - 1
 */
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 uint64_t leonardo(const uint32_t n) { return 2 * fibonacci(n + 1) - 1; }

/**
 * @brief 角度转弧度
 * @tparam T 运算类型
 * @param angular 角度值
 * @return 对应的弧度值
 */
template <typename T>
NEFORCE_PURE_FUNCTION constexpr T angular2radian(const T angular) noexcept {
    static_assert(is_arithmetic_v<T>, "arithmetic required");
    return angular * constants::PI / constants::SEMI_CIRCLE;
}

/**
 * @brief 弧度转角度
 * @tparam T 运算类型
 * @param radian 弧度值
 * @return 对应的角度值
 */
template <typename T>
NEFORCE_PURE_FUNCTION constexpr T radian2angular(const T radian) noexcept {
    static_assert(is_arithmetic_v<T>, "arithmetic required");
    return radian * (constants::SEMI_CIRCLE / constants::PI);
}

/**
 * @brief 取绝对值（有符号数版本）
 * @tparam T 数值类型
 * @param x 原数值
 * @return 绝对值
 */
template <typename T>
NEFORCE_CONST_FUNCTION constexpr enable_if_t<is_signed_v<T>, T> absolute(const T x) noexcept {
    return x > T(0) ? x : -x;
}

/**
 * @brief 取绝对值（无符号数版本）
 * @tparam T 数值类型
 * @param x 原数值
 * @return 原数值
 */

template <typename T>
NEFORCE_CONST_FUNCTION constexpr enable_if_t<is_unsigned_v<T>, T> absolute(const T x) noexcept {
    return x;
}

/**
 * @brief 单参数求和
 * @tparam T 数值类型
 * @param x 唯一参数
 * @return 参数本身
 *
 * 递归sum的终止位置
 */
template <typename T>
NEFORCE_CONST_FUNCTION constexpr const T& sum(const T& x) noexcept {
    return x;
}

/**
 * @brief 多参数求和
 * @tparam First 第一个参数类型
 * @tparam Rests 剩余参数类型
 * @param first 第一个参数
 * @param args 剩余参数
 * @return 所有参数的和
 */
template <typename First, typename... Rests, enable_if_t<(sizeof...(Rests) > 0), int> = 0>
NEFORCE_CONST_FUNCTION constexpr decltype(auto) sum(First first, Rests... args) {
    return first + _NEFORCE sum(args...);
}

/**
 * @brief 计算平均值
 * @tparam Args 参数类型
 * @param args 要求平均值的参数
 * @return 平均值
 */
template <typename... Args, enable_if_t<(sizeof...(Args) > 0), int> = 0>
NEFORCE_CONST_FUNCTION constexpr decltype(auto) average(Args... args) {
    return _NEFORCE sum(args...) / sizeof...(Args);
}

/**
 * @brief 获取数值的符号
 * @tparam T 数值类型
 * @param value 原数值
 * @return 符号值：正数返回1，负数返回-1，零返回0
 */
template <typename T>
NEFORCE_CONSTEXPR14 int sign(const T& value) noexcept {
    static_assert(is_arithmetic_v<T>, "arithmetic required");
    constexpr T zero = T(0);
    if (value > zero) {
        return 1;
    }
    if (value < zero) {
        return -1;
    }
    return 0;
}

/**
 * @brief 计算最大公约数
 * @tparam T 数值类型
 * @param m 第一个数
 * @param n 第二个数
 * @return 最大公约数
 */
template <typename T>
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 T gcd(const T& m, const T& n) noexcept {
    T x = _NEFORCE absolute(m), y = _NEFORCE absolute(n);
    constexpr T zero = T(0);
    while (y != zero) {
        T t = x % y;
        x = y;
        y = t;
    }
    return x;
}

/**
 * @brief 计算最小公倍数
 * @tparam T 数值类型
 * @param m 第一个数
 * @param n 第二个数
 * @return 最小公倍数
 */
template <typename T>
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 T lcm(const T& m, const T& n) noexcept {
    return (m / _NEFORCE gcd(m, n)) * n;
}


/**
 * @brief 浮点数取模运算
 * @tparam T 浮点类型
 * @param x 被除数
 * @param y 除数
 * @return x除以y的余数
 * @exception math_exception 除数为0时
 */
template <typename T>
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 T float_mod(const T x, const T y) {
    static_assert(is_arithmetic_v<T>, "arithmetic required");
    if (y == 0) {
        NEFORCE_THROW_EXCEPTION(math_exception("zero can not be dividend."));
    }
    const T result = x - static_cast<make_integer_t<sizeof(T)>>(x / y) * y;
    return result;
}

/**
 * @brief 幂运算
 * @tparam T 底数类型
 * @param x 底数
 * @param n 指数
 * @return x的n次幂
 *
 * 使用快速幂算法实现。
 */
template <typename T>
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 T power(const T& x, uint32_t n) noexcept {
    static_assert(is_arithmetic_v<T>, "arithmetic required");
    if (n == 0) {
        return 1;
    }
    T result(1);
    T base = x;
    while (n > 0) {
        if (n % 2 == 1) {
            result *= base;
        }
        base *= base;
        n /= 2;
    }
    return result;
}

/**
 * @brief 计算e的n次幂
 * @param n 指数
 * @return e^n
 */
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 decimal_t exponential(const uint32_t n) noexcept {
    return power(constants::EULER, n);
}

/**
 * @brief 计算自然对数
 * @tparam T 运算类型
 * @param x 真数
 * @return ln(x)
 *
 * 使用反正切泰勒展开计算。
 */
template <typename T>
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 T logarithm_e(const T x) {
    static_assert(is_floating_point_v<T>, "floating point required");
    if (x <= 0) {
        NEFORCE_THROW_EXCEPTION(math_exception("Logarithm domain error"));
    }
    uint32_t N = constants::TAYLOR_CONVERGENCE;
    const T a = (x - 1) / (x + 1);
    const T a_sqrt = a * a;
    T nk = 2 * N + 1;
    T y = 1.0 / nk;
    while (N--) {
        nk -= 2;
        y = 1.0 / nk + a_sqrt * y;
    }
    return 2.0 * a * y;
}

/**
 * @brief 计算任意底数的对数
 * @tparam T 运算类型
 * @param x 真数
 * @param base 底数
 * @return 以base为底x的对数
 */
template <typename T>
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 T logarithm(const T x, const uint32_t base) {
    const auto under = logarithm_e(static_cast<float64_t>(base));
    if (under == 0) {
        NEFORCE_THROW_EXCEPTION(math_exception("zero can not be dividend."));
    }
    return _NEFORCE logarithm_e(x) / under;
}

/**
 * @brief 计算以2为底的对数
 * @tparam T 运算类型
 * @param x 真数
 * @return log₂(x)
 */
template <typename T>
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 T logarithm_2(const T x) {
    return _NEFORCE logarithm(x, 2);
}

/**
 * @brief 计算以10为底的对数
 * @tparam T 运算类型
 * @param x 真数
 * @return log₁₀(x)
 */
template <typename T>
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 T logarithm_10(const T x) {
    return _NEFORCE logarithm(x, 10);
}

/**
 * @brief 计算平方根
 * @param x 被开方数
 * @param precise 精度要求
 * @return √x
 *
 * 使用牛顿迭代法计算。
 */
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 decimal_t
square_root(const decimal_t x, const decimal_t precise = constants::PRECISE_TOLERANCE) noexcept {
    decimal_t t = 0.0;
    decimal_t result = x;
    while (absolute(result - t) > precise) {
        t = result;
        result = 0.5 * (t + x / t);
    }
    return result;
}

/**
 * @brief 计算立方根
 * @param x 被开方数
 * @param precise 精度要求
 * @return ³√x
 *
 * 使用牛顿迭代法计算。
 */
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 decimal_t
cube_root(const decimal_t x, const decimal_t precise = constants::PRECISE_TOLERANCE) noexcept {
    decimal_t t = 0.0;
    decimal_t result = x;
    while (absolute(result - t) > precise) {
        t = result;
        result = (2 * t + x / (t * t)) / 3;
    }
    return result;
}

/**
 * @brief 计算阶乘
 * @param n 非负整数
 * @return n!
 */
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 uint64_t factorial(const uint32_t n) noexcept {
    uint64_t h = 1;
    for (uint32_t i = 1; i <= n; i++) {
        h *= i;
    }
    return h;
}


/**
 * @brief 向下舍入到指定位数
 * @param x 原数值
 * @param bit 位数，>0表示小数位，0表示整数位
 * @return 向下舍入后的值
 */
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 decimal_t floor_bit(const decimal_t x, const uint32_t bit) noexcept {
    const decimal_t times = power(10.0, bit);
    const auto int_part = x * times;
    if (x < 0 && x * times * 10 / 10.0 != int_part) {
        return (int_part - 1) / times;
    }
    return int_part / times;
}

/**
 * @brief 向上舍入到指定位数
 * @param x 原数值
 * @param bit 位数
 * @return 向上舍入后的值
 */
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 decimal_t ceil_bit(const decimal_t x, const uint32_t bit) noexcept {
    const decimal_t times = power(10.0, bit);
    const auto int_part = x * times;
    if (x > 0 && x * times * 10 / 10.0 != int_part) {
        return (int_part + 1) / times;
    }
    return int_part / times;
}

/**
 * @brief 四舍五入到指定位数
 * @param x 原数值
 * @param bit 位数
 * @return 四舍五入后的值
 */
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 decimal_t round_bit(const decimal_t x, const uint32_t bit) noexcept {
    return x < 0 ? ceil_bit(x - 0.5 / power(10.0, bit), bit) : floor_bit(x + 0.5 / power(10.0, bit), bit);
}

/**
 * @brief 截断到指定位数
 * @param x 原数值
 * @param bit 位数
 * @return 截断后的值
 */
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 decimal_t truncate_bit(const decimal_t x, const uint32_t bit) noexcept {
    return x < 0 ? ceil_bit(x, bit) : floor_bit(x, bit);
}


/**
 * @brief 向下取整
 * @param x 原数值
 * @return 向下取整后的值
 */
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 decimal_t floor(const decimal_t x) noexcept {
    const int64_t i = static_cast<int64_t>(x);
    return (x < 0 && x != static_cast<decimal_t>(i)) ? i - 1 : i;
}

/**
 * @brief 向下取整到指定位数
 * @param x 原数值
 * @param bit 位数
 * @return 向下取整后的值
 */
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 decimal_t floor(const decimal_t x, const uint32_t bit) noexcept {
    const decimal_t factor = power(10.0, bit);
    return floor(x * factor) / factor;
}

/**
 * @brief 向上取整
 * @param x 原数值
 * @return 向上取整后的值
 */
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 decimal_t ceil(const decimal_t x) noexcept {
    const int64_t i = static_cast<int64_t>(x);
    return (x > 0 && x != static_cast<decimal_t>(i)) ? i + 1 : i;
}

/**
 * @brief 向上取整到指定位数
 * @param x 原数值
 * @param bit 位数
 * @return 向上取整后的值
 */
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 decimal_t ceil(const decimal_t x, const uint32_t bit) noexcept {
    const decimal_t factor = power(10.0, bit);
    return ceil(x * factor) / factor;
}

/**
 * @brief 四舍五入
 * @param x 原数值
 * @return 四舍五入后的值
 */
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 decimal_t round(const decimal_t x) noexcept {
    return (x >= 0) ? floor(x + 0.5) : ceil(x - 0.5);
}

/**
 * @brief 四舍五入到指定位数
 * @param x 原数值
 * @param bit 位数
 * @return 四舍五入后的值
 */
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 decimal_t round(const decimal_t x, const uint32_t bit) noexcept {
    const decimal_t factor = power(10.0, bit);
    return round(x * factor) / factor;
}

/**
 * @brief 截断
 * @param x 原数值
 * @param bit 位数
 * @return 截断后的值
 */
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 decimal_t truncate(const decimal_t x, const int bit) noexcept {
    return x < 0 ? ceil(x, bit) : floor(x, bit);
}

/**
 * @brief 截断到整数位
 * @param x 原数值
 * @return 截断后的整数值
 */
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 decimal_t truncate(const decimal_t x) noexcept { return truncate(x, 0); }


/**
 * @brief 判断是否接近某个倍数值
 * @param x 待判断值
 * @param axis 基准值
 * @param toler 容差
 * @return 如果x接近axis的整数倍则返回true
 * @exception math_exception 当axis为0时抛出
 */
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 bool around_multiple(const decimal_t x, const decimal_t axis,
                                                                const decimal_t toler = constants::PRECISE_TOLERANCE) {
    if (absolute(axis) < constants::PRECISE_TOLERANCE) {
        NEFORCE_THROW_EXCEPTION(math_exception("Axis Cannot be 0"));
    }
    const decimal_t multi = round(x / axis) * axis;
    return absolute(x - multi) < toler;
}

/**
 * @brief 判断是否接近π的整数倍
 * @param x 待判断值
 * @param toler 容差
 * @return 如果x接近π的整数倍则返回true
 */
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 bool around_pi(const decimal_t x,
                                                          const decimal_t toler = constants::LOW_PRECISE_TOLERANCE) {
    return around_multiple(x, constants::PI, toler);
}

/**
 * @brief 判断是否接近零
 * @param x 待判断值
 * @param toler 容差
 * @return 如果|x| < toler则返回true
 */
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 bool around_zero(const decimal_t x,
                                                            const decimal_t toler = constants::PRECISE_TOLERANCE) {
    return absolute(x) < toler;
}


/**
 * @brief 计算余数
 * @param x 被除数
 * @param y 除数
 * @return x除以y的余数
 *
 * 使用对称舍入规则
 */
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 decimal_t remainder(const decimal_t x, const decimal_t y) noexcept {
    return x - y * round(x / y);
}

/**
 * @brief 获取小数部分
 * @param x 原数值
 * @return x的小数部分
 */
NEFORCE_CONST_FUNCTION constexpr decimal_t float_part(const decimal_t x) noexcept {
    return x - static_cast<int64_t>(x);
}

/**
 * @brief 分离整数和小数部分
 * @param x 原数值
 * @param int_ptr 指向整数部分的指针
 * @return 小数部分
 *
 * 将x的整数部分存入int_ptr，返回小数部分。
 */
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 decimal_t float_apart(decimal_t x, int64_t* int_ptr) noexcept {
    *int_ptr = static_cast<int64_t>(x);
    x -= *int_ptr;
    return x;
}


/**
 * @brief 计算正弦值
 * @param x 弧度值
 * @return sin(x)
 *
 * 使用泰勒展开计算，先进行周期性处理。
 */
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 decimal_t sine(decimal_t x) noexcept {
    decimal_t sign = 1.0;
    if (x < 0) {
        sign = -1.0;
        x = -x;
    }
    constexpr decimal_t twoPi = 2 * constants::PI;
    x = x - twoPi * floor(x / twoPi);

    if (x > constants::PI) {
        x -= constants::PI;
        sign = -sign;
    }
    if (x > constants::PI / 2) {
        x = constants::PI - x;
    }

    decimal_t i = 1;
    int32_t neg = 1;
    decimal_t term = 0;
    decimal_t idx = x;
    decimal_t fac = 1;
    decimal_t taylor = x;
    do {
        fac = fac * (i + 1) * (i + 2);
        idx *= x * x;
        neg = -neg;
        term = idx / fac * neg;
        taylor += term;
        i += 2;
    } while (absolute(term) > constants::EPSILON);
    const auto fin = sign * taylor;
    return around_zero(fin) ? 0 : fin;
}

/**
 * @brief 计算余弦值
 * @param x 弧度值
 * @return cos(x)
 *
 * 利用恒等式 cos(x) = sin(π/2 - x) 计算。
 */
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 decimal_t cosine(const decimal_t x) noexcept {
    return sine(constants::PI / 2.0 - x);
}

/**
 * @brief 计算正切值
 * @param x 弧度值
 * @return tan(x)
 * @exception math_exception 当x接近π/2的奇数倍时抛出
 */
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 decimal_t tangent(const decimal_t x) {
    const decimal_t multiple = (2 * round((2 * x - constants::PI) / (2 * constants::PI)) + 1) * (constants::PI / 2);
    if (absolute(x - multiple) < constants::LOW_PRECISE_TOLERANCE) {
        NEFORCE_THROW_EXCEPTION(math_exception("Tangent Range Exceeded"));
    }
    return sine(x) / cosine(x);
}

/**
 * @brief 计算余切值
 * @param x 弧度值
 * @return cot(x)
 * @exception math_exception 当x接近π的整数倍时抛出
 */
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 decimal_t cotangent(const decimal_t x) { return 1 / tangent(x); }

/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @brief 反正切泰勒展开
 * @param x 参数（|x| ≤ 1）
 * @return arctan(x)
 *
 * 使用反正切函数的泰勒展开公式。
 */
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 decimal_t __arctangent_taylor(const decimal_t x) noexcept {
    const decimal_t x_sq = x * x;
    decimal_t term = x;
    decimal_t sum = x;
    decimal_t n = 1.0;
    while (absolute(term) > constants::EPSILON) {
        term *= -x_sq;
        n += 2.0;
        sum += term / n;
    }
    return sum;
}

NEFORCE_END_INNER__
/// @endcond

/**
 * @brief 计算反正切值
 * @param x 参数
 * @return arctan(x)（弧度值，范围(-π/2, π/2)）
 *
 * 当|x|>1时，利用恒等式 arctan(x) = π/2 - arctan(1/x)。
 */
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 decimal_t arctangent(const decimal_t x) noexcept {
    if (absolute(x) > 1) {
        const decimal_t sign = x > 0 ? 1.0 : -1.0;
        return sign * (constants::PI / 2 - inner::__arctangent_taylor(1 / absolute(x)));
    }
    return inner::__arctangent_taylor(x);
}

/**
 * @brief 计算反正弦值
 * @param x 参数（|x| ≤ 1）
 * @return arcsin(x)（弧度值，范围[-π/2, π/2]）
 * @exception math_exception 当|x| > 1时抛出
 */
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 decimal_t arcsine(const decimal_t x) {
    if (absolute(x) > 1) {
        NEFORCE_THROW_EXCEPTION(math_exception("Arcsine Range Exceeded"));
    }
    if (absolute(x) >= 1 - constants::EPSILON) {
        return x > 0 ? constants::PI / 2 : -constants::PI / 2;
    }
    return arctangent(x / square_root(1 - x * x));
}

/**
 * @brief 计算反余弦值
 * @param x 参数（|x| ≤ 1）
 * @return arccos(x)（弧度值，范围[0, π]）
 * @exception math_exception 当|x| > 1时抛出
 */
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 decimal_t arccosine(const decimal_t x) {
    if (absolute(x) > 1) {
        NEFORCE_THROW_EXCEPTION(math_exception("Arccosine Range Exceeded"));
    }
    return constants::PI / 2.0 - arcsine(x);
}

/** @} */ // MathFunctions

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_NUMERIC_MATH_HPP__
