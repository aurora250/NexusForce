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
#include "NeForce/core/numeric/numeric_types.hpp"
NEFORCE_BEGIN_NAMESPACE__

NEFORCE_BEGIN_CONSTANTS__

/**
 * @defgroup MathConstants 数学常量
 * @brief 常用数学常量定义
 *
 * @section math_constants 数学常量定义
 *
 * 根据 ISO/IEC 10967-2 和 IEEE 754，提供以下高精度常量：
 *
 * | 常量           | 符号 | 值（高精度）                    | 说明                     |
 * |----------------|------|---------------------------------|--------------------------|
 * | EULER          | e    | 2.71828182845904523536          | 自然对数的底             |
 * | PI             | π    | 3.14159265358979323846          | 圆周率                   |
 * | PHI            | φ    | 1.61803398874989484820          | 黄金分割比               |
 * | TWO_PI_HI      | 2π   | 6.28318530717958647692          | 2π 高位部分（用于精确归约）|
 * | TWO_PI_LO      | -    | 2.44929359829470641435e-16      | 2π 低位部分（补偿）      |
 * | LN2            | ln2  | 0.69314718055994530942          | 自然对数 2               |
 * | INV_LN2        | log₂e| 1.44269504088896340736          | log₂(e)                  |
 * | SQRT_2         | √2   | 1.41421356237309504880          | 2 的平方根               |
 * | MACHINE_EPSILON| ε    | 取决于 decimal_t                | 机器精度（最小正数差）   |
 * | DEFAULT_TOLERANCE | - | 1e-12                           | 默认数值容差             |
 * @{
 */

NEFORCE_INLINE17 constexpr decimal_t EULER = 2.71828182845904523536L; ///< 自然常数 e
NEFORCE_INLINE17 constexpr decimal_t PI = 3.14159265358979323846L;    ///< 圆周率 π (弧度制)
NEFORCE_INLINE17 constexpr decimal_t PHI = 1.61803398874989484820L;   ///< 黄金分割比 φ
NEFORCE_INLINE17 constexpr decimal_t SEMI_CIRCLE = 180.0L;            ///< 半圆角度 180° (角度制)
NEFORCE_INLINE17 constexpr decimal_t CIRCLE = 360.0L;                 ///< 全圆角度 360° (角度制)

/// 高精度 2π 高位
NEFORCE_INLINE17 constexpr decimal_t TWO_PI_HI = 6.28318530717958647692L;
/// 高精度 2π 低位
NEFORCE_INLINE17 constexpr decimal_t TWO_PI_LO = 2.44929359829470641435e-16L;

/// 机器精度
NEFORCE_INLINE17 constexpr decimal_t MACHINE_EPSILON = numeric_traits<decimal_t>::epsilon();
/// 默认容差
NEFORCE_INLINE17 constexpr decimal_t DEFAULT_TOLERANCE = 1e-12L;
/// 宽松容差
NEFORCE_INLINE17 constexpr decimal_t LOOSE_TOLERANCE = 1e-9L;

/// 自然对数 2
NEFORCE_INLINE17 constexpr decimal_t LN2 = 0.69314718055994530941723212145817656807L;
/// log₂(e)
NEFORCE_INLINE17 constexpr decimal_t INV_LN2 = 1.44269504088896340735992468100189213743L;
/// 2 的平方根
NEFORCE_INLINE17 constexpr decimal_t SQRT_2 = 1.41421356237309504880168872420969807857L;

/**
 * @brief e^x 泰勒展开系数（1/n!，n = 0..19）
 *
 * 范围归约后 |r| <= ln(2)/2，19 阶即可达到长双精度。
 */
NEFORCE_INLINE17 constexpr decimal_t EXPONENTIAL_COEFFICIENTS[20] = {1.0L / 1.0L,
                                                                     1.0L / 1.0L,
                                                                     1.0L / 2.0L,
                                                                     1.0L / 6.0L,
                                                                     1.0L / 24.0L,
                                                                     1.0L / 120.0L,
                                                                     1.0L / 720.0L,
                                                                     1.0L / 5040.0L,
                                                                     1.0L / 40320.0L,
                                                                     1.0L / 362880.0L,
                                                                     1.0L / 3628800.0L,
                                                                     1.0L / 39916800.0L,
                                                                     1.0L / 479001600.0L,
                                                                     1.0L / 6227020800.0L,
                                                                     1.0L / 87178291200.0L,
                                                                     1.0L / 1307674368000.0L,
                                                                     1.0L / 20922789888000.0L,
                                                                     1.0L / 355687428096000.0L,
                                                                     1.0L / 6402373705728000.0L,
                                                                     1.0L / 121645100408832000.0L};

/**
 * @brief 自然对数级数系数（1/3、1/5 … 1/29）
 *
 * 归约后 |a| <= (√2 - 1)/(√2 + 1)，14 项即可达到长双精度。
 */
NEFORCE_INLINE17 constexpr decimal_t LOGARITHM_COEFFICIENTS[14] = {
        1.0L / 3.0L,  1.0L / 5.0L,  1.0L / 7.0L,  1.0L / 9.0L,  1.0L / 11.0L, 1.0L / 13.0L, 1.0L / 15.0L,
        1.0L / 17.0L, 1.0L / 19.0L, 1.0L / 21.0L, 1.0L / 23.0L, 1.0L / 25.0L, 1.0L / 27.0L, 1.0L / 29.0L};

/**
 * @brief log(1 + x) 级数系数（1/1、1/2 … 1/22）
 *
 * |x| <= 1/8 时 22 项即可达到长双精度。
 */
NEFORCE_INLINE17 constexpr decimal_t LOGARITHM_1P_COEFFICIENTS[22] = {
        1.0L / 1.0L,  1.0L / 2.0L,  1.0L / 3.0L,  1.0L / 4.0L,  1.0L / 5.0L,  1.0L / 6.0L,  1.0L / 7.0L,  1.0L / 8.0L,
        1.0L / 9.0L,  1.0L / 10.0L, 1.0L / 11.0L, 1.0L / 12.0L, 1.0L / 13.0L, 1.0L / 14.0L, 1.0L / 15.0L, 1.0L / 16.0L,
        1.0L / 17.0L, 1.0L / 18.0L, 1.0L / 19.0L, 1.0L / 20.0L, 1.0L / 21.0L, 1.0L / 22.0L};

/**
 * @brief ln(n!) 查表值（n = 0..32）
 */
NEFORCE_INLINE17 constexpr decimal_t LOGARITHM_FACTORIAL[33] = {0.0L,
                                                                0.0L,
                                                                0.6931471805599453094172L,
                                                                1.791759469228055000812L,
                                                                3.178053830347945619647L,
                                                                4.787491742782045994248L,
                                                                6.57925121201010099506L,
                                                                8.525161361065414300166L,
                                                                10.60460290274525022842L,
                                                                12.80182748008146961121L,
                                                                15.10441257307551529523L,
                                                                17.50230784587388583929L,
                                                                19.98721449566188614952L,
                                                                22.55216385312342288557L,
                                                                25.19122118273868150009L,
                                                                27.89927138384089156609L,
                                                                30.67186010608067280376L,
                                                                33.50507345013688888401L,
                                                                36.39544520803305357622L,
                                                                39.33988418719949403622L,
                                                                42.33561646075348502966L,
                                                                45.38013889847690802616L,
                                                                48.47118135183522387964L,
                                                                51.60667556776437357045L,
                                                                54.78472939811231919009L,
                                                                58.00360522298051993929L,
                                                                61.26170176100200198477L,
                                                                64.55753862700633105895L,
                                                                67.88974313718153498289L,
                                                                71.25703896716800901007L,
                                                                74.65823634883016438549L,
                                                                78.09222355331531063142L,
                                                                81.5579594561150371785L};

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
 *
 * @section standards 遵循的国际标准
 * 本实现严格遵循以下数学计算相关标准规范：
 *
 * **浮点数算术标准：**
 * - **IEEE 754-2019**：浮点数算术标准（NaN、无穷大、舍入模式、机器精度）
 *   https://standards.ieee.org/ieee/754/6210/
 * - **ISO/IEC 60559:2020**：信息技术 — 微处理器系统 — 浮点算术（等同于 IEEE 754）
 *   https://www.iso.org/standard/80985.html
 *
 * **数学函数规范：**
 * - **ISO/IEC 10967-2:2001**：信息技术 — 语言独立算术 — 第2部分：初等数值函数
 *   https://www.iso.org/standard/24417.html
 * - **POSIX.1-2017 (IEEE Std 1003.1)**：数学函数接口定义
 *   https://pubs.opengroup.org/onlinepubs/9699919799/
 *
 * @section trigonometric_functions 三角函数实现
 * 本实现采用泰勒级数展开与参数归约相结合的方法：
 *
 * **参数归约**（根据 Cody & Waite 方法）：
 * 1. 将输入 x 映射到 [-π/2, π/2] 区间
 * 2. 使用高精度 π/2 值（高位 + 低位）减少舍入误差
 * 3. 记录象限信息以确定最终符号
 *
 * **泰勒级数展开**：
 * - sin(x) = x - x³/3! + x⁵/5! - x⁷/7! + ...
 * - cos(x) = 1 - x²/2! + x⁴/4! - x⁶/6! + ...
 * - arctan(x) = x - x³/3 + x⁵/5 - x⁷/7 + ... (|x| ≤ 1)
 *
 * 迭代终止条件：当前项小于 `MACHINE_EPSILON × 当前和`。
 *
 * @section logarithm_implementation 对数函数实现
 * **自然对数 ln(x)**：
 * - 使用恒等式：ln(x) = 2 × arctanh((x-1)/(x+1))
 * - arctanh(y) = y + y³/3 + y⁵/5 + ... (|y| < 1)
 *
 * **任意底对数**：
 * - log_base(x) = ln(x) / ln(base)
 *
 * @section root_functions 方根函数实现
 * **平方根 √x**（牛顿迭代法）：
 * - 初始猜测：guess = x × 0.5
 * - 迭代公式：guess = 0.5 × (guess + x / guess)
 * - 终止条件：|guess - prev| ≤ tolerance × |guess|
 *
 * **立方根 ³√x**（牛顿迭代法）：
 * - 迭代公式：guess = (2×prev + x / prev²) / 3
 *
 * @section special_values 特殊值处理
 * 根据 IEEE 754-2019 §6，特殊值处理规则：
 *
 * | 输入情况              | 返回值                               |
 * |-----------------------|--------------------------------------|
 * | 输入为 NaN            | 返回 NaN                             |
 * | 输入为 ±∞             | 根据函数语义返回 ±∞ 或 NaN           |
 * | 负数的平方根          | 返回 NaN                             |
 * | 除数为零              | 返回 NaN 或抛出异常                  |
 * | |x| > 1 的反正弦/反余弦| 返回 NaN                             |
 *
 * @section performance_notes 性能与精度说明
 * - 三角函数使用参数归约减少大输入值的误差
 * - 对数与指数使用 2 的整数次幂阶梯归约后再展开级数，级数项以预计算倒数系数相乘实现
 * - 平方根以线性近似为初值做牛顿迭代，5 次迭代内即可达到长双精度
 * - 阶乘对数采用查表 + 斯特林级数，避免大数阶乘溢出
 * - 预计算的斐波那契数列加速小索引值的查询
 *
 * @see https://standards.ieee.org/ieee/754/6210/
 * @see https://dlmf.nist.gov/
 * @see https://en.cppreference.com/w/cpp/numeric/math
 * @{
 */

NEFORCE_CONST_FUNCTION constexpr int64_t safe_trunc(const decimal_t x) noexcept {
    constexpr int64_t max_val = numeric_traits<int64_t>::max();
    constexpr int64_t min_val = numeric_traits<int64_t>::min();
    constexpr auto max_int64 = static_cast<decimal_t>(max_val);
    constexpr auto min_int64 = static_cast<decimal_t>(min_val);

    if (is_nan(x) || is_infinity(x)) {
        return 0;
    }

    if (x >= max_int64) {
        const auto result = static_cast<int64_t>(x);
        return (result == max_val && x <= max_int64) ? result : 0;
    }
    if (x <= min_int64) {
        const auto result = static_cast<int64_t>(x);
        return (result == min_val && x >= min_int64) ? result : 0;
    }
    return static_cast<int64_t>(x);
}

/**
 * @brief 将非负 decimal_t 安全转换为 uint64_t（避免 >= 2^63 时的 UB）
 * @return 对应的 uint64_t 值
 */
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 uint64_t safe_decimal_to_uint64(const decimal_t x) noexcept {
    constexpr decimal_t TWO_POW_63 = 9223372036854775808.0L;
    if (x >= TWO_POW_63) {
        return static_cast<uint64_t>(x - TWO_POW_63) + 9223372036854775808ULL;
    } else {
        return static_cast<uint64_t>(x);
    }
}

/**
 * @brief 计算斐波那契数
 * @param n 索引位置
 * @return 第n个斐波那契数
 *
 * 如果n小于预计算的数量，直接返回预计算结果；
 * 否则递归计算。
 */
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 uint64_t fibonacci(const uint32_t n) noexcept {
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
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 uint64_t leonardo(const uint32_t n) noexcept {
    return 2 * fibonacci(n + 1) - 1;
}

/**
 * @brief 角度转弧度
 * @tparam T 运算类型
 * @param angular 角度值
 * @return 对应的弧度值
 */
template <typename T>
NEFORCE_PURE_FUNCTION constexpr T angular2radian(const T angular) noexcept {
    static_assert(is_arithmetic_v<T>, "arithmetic required");
    return angular * static_cast<T>(constants::PI) / static_cast<T>(constants::SEMI_CIRCLE);
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
    return radian * (static_cast<T>(constants::SEMI_CIRCLE) / static_cast<T>(constants::PI));
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
constexpr decltype(auto) sum(First first, Rests... args) {
    return first + _NEFORCE sum(args...);
}

/**
 * @brief 计算平均值
 * @tparam Args 参数类型
 * @param args 要求平均值的参数
 * @return 平均值
 */
template <typename... Args, enable_if_t<(sizeof...(Args) > 0), int> = 0>
constexpr decltype(auto) average(Args... args) {
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
NEFORCE_CONSTEXPR14 enable_if_t<is_floating_point_v<T>, T> mod(const T x, const T y) {
    if (_NEFORCE is_nan(x) || _NEFORCE is_nan(y)) {
        return numeric_traits<T>::quiet_nan();
    }
    if (y == 0) {
        NEFORCE_THROW_EXCEPTION(math_exception("zero can not be dividend."));
    }
    if (_NEFORCE is_infinity(x) || _NEFORCE is_infinity(y)) {
        return numeric_traits<T>::quiet_nan();
    }
    return x - static_cast<make_integer_t<sizeof(T)>>(x / y) * y;
}

/**
 * @brief 整数取模运算
 * @tparam T 整数类型
 * @param x 被除数
 * @param y 除数
 * @return x除以y的余数
 * @exception math_exception 除数为0时
 */
template <typename T>
NEFORCE_CONSTEXPR14 enable_if_t<is_integral_v<T>, T> mod(const T x, const T y) {
    if (y == 0) {
        NEFORCE_THROW_EXCEPTION(math_exception("zero can not be dividend."));
    }
    return x % y;
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
 * @brief 计算 2 的整数次幂
 * @param exponent 指数
 * @return 2^exponent
 *
 * 使用二进制幂计算，最多需要 2 * log₂(|exponent|) 次乘法，
 * 结果超出浮点表示范围时按 IEEE 754 规则上溢为无穷大或下溢为 0。
 */
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 decimal_t power_of_two(const int64_t exponent) noexcept {
    decimal_t scale = 1.0L;
    decimal_t factor = exponent >= 0 ? 2.0L : 0.5L;
    auto remaining = static_cast<uint64_t>(exponent >= 0 ? exponent : -exponent);

    while (remaining > 0) {
        if ((remaining & 1U) != 0U) {
            scale *= factor;
        }
        remaining >>= 1U;
        if (remaining > 0) {
            factor *= factor;
        }
    }
    return scale;
}

/**
 * @brief 按 2 的整数次幂归一化正数
 * @param value 输入输出参数，输入为正数，输出归一化到 [1, 2) 的尾数
 * @return 归一化使用的指数
 *
 * 归一化前的数值 = 归一化后的数值 × 2^返回值
 */
NEFORCE_CONSTEXPR14 int64_t normalize_power_of_two(decimal_t& value) noexcept {
    decimal_t steps[10];
    steps[0] = 2.0L;
    for (int i = 1; i < 10; ++i) {
        steps[i] = steps[i - 1] * steps[i - 1];
    }

    int64_t exponent = 0;

    for (int i = 9; i >= 0; --i) {
        while (value >= 2.0L * steps[i]) {
            value /= steps[i];
            exponent += static_cast<int64_t>(1) << i;
        }
    }
    while (value >= 2.0L) {
        value *= 0.5L;
        ++exponent;
    }

    for (int i = 9; i >= 0; --i) {
        while (value < 1.0L && value * steps[i] <= 1.0L) {
            value *= steps[i];
            exponent -= static_cast<int64_t>(1) << i;
        }
    }
    while (value < 1.0L) {
        value *= 2.0L;
        --exponent;
    }
    return exponent;
}

/**
 * @brief 计算 e 的 x 次幂
 * @param x 指数
 * @return e^x
 *
 * 采用范围归约 + 泰勒展开 + 2 的整数次幂缩放
 */
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 decimal_t exponential_e(const decimal_t x) noexcept {
    if (is_nan(x)) {
        return x;
    }
    if (is_infinity(x)) {
        return x > 0 ? x : 0.0L;
    }
    if (x == 0.0L) {
        return 1.0L;
    }

    constexpr decimal_t MAX_EXPONENT = sizeof(decimal_t) == sizeof(double) ? 709.78271289338397L : 11356.523406294143L;
    constexpr decimal_t MIN_EXPONENT =
            sizeof(decimal_t) == sizeof(double) ? -745.13321910194110L : -11433.462743168135L;

    if (x > MAX_EXPONENT) {
        return static_cast<decimal_t>(numeric_traits<decimal_t>::infinity());
    }
    if (x < MIN_EXPONENT) {
        return 0.0L;
    }

    const decimal_t nearest = x * constants::INV_LN2;
    int64_t k = safe_trunc(nearest);
    if (nearest - static_cast<decimal_t>(k) > 0.5L) {
        ++k;
    } else if (nearest - static_cast<decimal_t>(k) < -0.5L) {
        --k;
    }

    const decimal_t r = x - static_cast<decimal_t>(k) * constants::LN2;

    decimal_t polynomial = constants::EXPONENTIAL_COEFFICIENTS[19];
    for (int i = 18; i >= 0; --i) {
        polynomial = polynomial * r + constants::EXPONENTIAL_COEFFICIENTS[i];
    }
    return polynomial * power_of_two(k);
}

/**
 * @brief 计算自然对数
 * @param x 真数
 * @return ln(x)
 *
 * 采用倒数归约 + 2 的整数次幂阶梯归约 + 反正切级数
 */
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 decimal_t logarithm_e(const decimal_t x) noexcept {
    if (is_nan(x)) {
        return x;
    }
    if (x < 0.0L) {
        return numeric_traits<decimal_t>::quiet_nan();
    }
    if (x == 0.0L) {
        return static_cast<decimal_t>(-numeric_traits<decimal_t>::infinity());
    }
    if (is_infinity(x) && x > 0) {
        return x;
    }
    if (x < 1.0L) {
        return -logarithm_e(1.0L / x);
    }

    decimal_t m = x;
    int64_t k = normalize_power_of_two(m);

    if (m > constants::SQRT_2) {
        m *= 0.5L;
        ++k;
    }

    const decimal_t a = (m - 1.0L) / (m + 1.0L);
    const decimal_t a2 = a * a;
    decimal_t term = a;
    decimal_t s = a;

    for (uint32_t n = 1; n <= 14; ++n) {
        term *= a2;
        s += term * constants::LOGARITHM_COEFFICIENTS[n - 1];
    }

    return 2.0L * s + static_cast<decimal_t>(k) * constants::LN2;
}

/**
 * @brief 计算 ln(1 + x)
 * @param x 真数减一
 * @return ln(1 + x)
 */
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 decimal_t logarithm_1p(const decimal_t x) noexcept {
    if (is_nan(x) || x < -1.0L) {
        return numeric_traits<decimal_t>::quiet_nan();
    }
    if (x == -1.0L) {
        return static_cast<decimal_t>(-numeric_traits<decimal_t>::infinity());
    }
    if (is_infinity(x)) {
        return x;
    }
    if (x == 0.0L) {
        return 0.0L;
    }

    if (absolute(x) <= 0.125L) {
        decimal_t term = x;
        decimal_t total = 0.0L;

        for (uint32_t n = 1; n <= 22; ++n) {
            total += term * constants::LOGARITHM_1P_COEFFICIENTS[n - 1];
            term *= -x;
        }
        return total;
    }
    return logarithm_e(1.0L + x);
}

/**
 * @brief 计算任意底数的对数
 * @param x 真数
 * @param base 底数
 * @return 以base为底x的对数
 */
NEFORCE_CONSTEXPR14 decimal_t logarithm(const decimal_t x, const uint32_t base) {
    const decimal_t under = logarithm_e(base);
    if (under == 0.0L) {
        NEFORCE_THROW_EXCEPTION(math_exception("zero can not be dividend."));
    }
    return logarithm_e(x) / under;
}

/**
 * @brief 计算以2为底的对数
 * @param x 真数
 * @return log₂(x)
 */
NEFORCE_CONSTEXPR14 decimal_t logarithm_2(const decimal_t x) { return logarithm(x, 2); }

/**
 * @brief 计算以10为底的对数
 * @param x 真数
 * @return log₁₀(x)
 */
NEFORCE_CONSTEXPR14 decimal_t logarithm_10(const decimal_t x) { return logarithm(x, 10); }

/**
 * @brief 计算平方根
 * @param x 被开方数
 * @param precise 精度要求
 * @return √x
 *
 * 使用 2 的整数次幂阶梯把 x 归一化为 m·2^k（m ∈ [1, 4)、k 为偶数）
 */
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 decimal_t
square_root(const decimal_t x, const decimal_t precise = constants::DEFAULT_TOLERANCE) noexcept {
    if (is_nan(x)) {
        return x;
    }
    if (x < 0.0L) {
        return numeric_traits<decimal_t>::quiet_nan();
    }
    if (x == 0.0L) {
        return x;
    }
    if (is_infinity(x) && x > 0) {
        return x;
    }

    decimal_t m = x;
    int64_t k = normalize_power_of_two(m);
    if ((k & 1) != 0) {
        m *= 2.0L;
        --k;
    }

    decimal_t guess = 1.0L + (m - 1.0L) / 3.0L;
    decimal_t prev = 0.0L;
    do {
        prev = guess;
        guess = 0.5L * (prev + m / prev);
    } while (absolute(guess - prev) > precise * absolute(guess));

    return guess * power_of_two(k / 2);
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
cube_root(const decimal_t x, const decimal_t precise = constants::DEFAULT_TOLERANCE) noexcept {
    if (is_nan(x)) {
        return x;
    }
    if (is_infinity(x)) {
        return x;
    }
    if (x == 0.0L) {
        return x;
    }

    const bool negative = x < 0.0L;
    const decimal_t v = negative ? -x : x;

    decimal_t guess = v > 1.0L ? v / 3.0L : v;
    decimal_t prev = 0.0L;
    do {
        prev = guess;
        guess = (2.0L * prev + v / (prev * prev)) / 3.0L;
    } while (absolute(guess - prev) > precise * absolute(guess));

    return negative ? -guess : guess;
}

/**
 * @brief 计算阶乘
 * @param n 非负整数
 * @return n!
 */
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 uint64_t factorial(const uint32_t n) noexcept {
    uint64_t result = 1;
    for (uint32_t i = 2; i <= n; ++i) {
        result *= i;
    }
    return result;
}

/**
 * @brief 计算阶乘的自然对数
 * @param n 非负整数
 * @return ln(n!)
 *
 * n ≤ 32 时直接查表，n > 32 时使用斯特林级数
 * ln(n!) = (n + 1/2)ln(n) - n + ln(2π)/2 + 1/(12n) - 1/(360n³) + 1/(1260n⁵) - 1/(1680n⁷)
 */
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 decimal_t logarithm_factorial(const uint64_t n) noexcept {
    if (n <= 32) {
        return constants::LOGARITHM_FACTORIAL[n];
    }

    const auto value = static_cast<decimal_t>(n);
    const decimal_t inverse = 1.0L / value;
    const decimal_t inverse2 = inverse * inverse;
    const decimal_t correction =
            1.0L / 12.0L + inverse2 * (-1.0L / 360.0L + inverse2 * (1.0L / 1260.0L + inverse2 * (-1.0L / 1680.0L)));

    return (value + 0.5L) * logarithm_e(value) - value + 0.91893853320467274178032973640561763986L +
           inverse * correction;
}

/**
 * @brief 向下取整
 * @param x 原数值
 * @return 向下取整后的值
 */
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 decimal_t floor(const decimal_t x) noexcept {
    if (!is_finite(x)) {
        return x;
    }

    constexpr auto TWO_POW_64 = static_cast<decimal_t>(numeric_traits<uint64_t>::max());
    if (x >= 0.0L) {
        if (x >= TWO_POW_64) {
            return x;
        }
        return static_cast<decimal_t>(safe_decimal_to_uint64(x));
    } else {
        const decimal_t pos = -x;
        if (pos >= TWO_POW_64) {
            return x;
        }
        const auto floor_pos = static_cast<decimal_t>(safe_decimal_to_uint64(pos));
        if (absolute(pos - floor_pos) < constants::MACHINE_EPSILON) {
            return -floor_pos;
        } else {
            return -(floor_pos + 1.0L);
        }
    }
}

/**
 * @brief 向下取整到指定位数
 * @param x 原数值
 * @param bit 位数
 * @return 向下取整后的值
 */
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 decimal_t floor(const decimal_t x, const uint32_t bit) noexcept {
    const decimal_t factor = power(10.0L, bit);
    return floor(x * factor) / factor;
}

NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 bool is_even_integer(const decimal_t x) noexcept {
    const decimal_t half = x * 0.5L;
    return absolute(half - floor(half)) < constants::MACHINE_EPSILON;
}

/**
 * @brief 向上取整
 * @param x 原数值
 * @return 向上取整后的值
 */
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 decimal_t ceil(const decimal_t x) noexcept {
    if (!is_finite(x)) {
        return x;
    }

    constexpr auto TWO_POW_64 = static_cast<decimal_t>(numeric_traits<uint64_t>::max());
    if (x >= 0.0L) {
        if (x >= TWO_POW_64) {
            return x;
        }
        const auto floor_x = static_cast<decimal_t>(safe_decimal_to_uint64(x));
        if (absolute(x - floor_x) < constants::MACHINE_EPSILON) {
            return floor_x;
        } else {
            return floor_x + 1.0L;
        }
    } else {
        const decimal_t pos = -x;
        if (pos >= TWO_POW_64) {
            return x;
        }
        const uint64_t floor_pos = safe_decimal_to_uint64(pos);
        return -static_cast<decimal_t>(floor_pos);
    }
}

/**
 * @brief 向上取整到指定位数
 * @param x 原数值
 * @param bit 位数
 * @return 向上取整后的值
 */
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 decimal_t ceil(const decimal_t x, const uint32_t bit) noexcept {
    const decimal_t factor = power(10.0L, bit);
    return ceil(x * factor) / factor;
}

/**
 * @brief 四舍五入
 * @param x 原数值
 * @return 四舍五入后的值
 */
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 decimal_t round(const decimal_t x) noexcept {
    if (!is_finite(x)) {
        return x;
    }

    const decimal_t abs_x = absolute(x);
    const decimal_t int_part = floor(abs_x);
    const decimal_t frac = abs_x - int_part;

    if (frac < 0.5L) {
        return (x < 0.0L) ? -int_part : int_part;
    }
    if (frac > 0.5L) {
        return (x < 0.0L) ? -(int_part + 1.0L) : (int_part + 1.0L);
    }

    if (is_even_integer(int_part)) {
        return (x < 0.0L) ? -int_part : int_part;
    } else {
        return (x < 0.0L) ? -(int_part + 1.0L) : (int_part + 1.0L);
    }
}


/**
 * @brief 向下舍入到指定位数
 * @param x 原数值
 * @param bit 位数，>0表示小数位，0表示整数位
 * @return 向下舍入后的值
 */
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 decimal_t floor_bit(const decimal_t x, const uint32_t bit) noexcept {
    const decimal_t factor = power(10.0L, bit);
    return floor(x * factor) / factor;
}

/**
 * @brief 向上舍入到指定位数
 * @param x 原数值
 * @param bit 位数
 * @return 向上舍入后的值
 */
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 decimal_t ceil_bit(const decimal_t x, const uint32_t bit) noexcept {
    const decimal_t factor = power(10.0L, bit);
    return ceil(x * factor) / factor;
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
 * @brief 四舍五入到指定位数
 * @param x 原数值
 * @param bit 位数
 * @return 四舍五入后的值
 */
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 decimal_t round(const decimal_t x, const uint32_t bit) noexcept {
    const decimal_t factor = power(10.0L, bit);
    return round(x * factor) / factor;
}

/**
 * @brief 截断
 * @param x 原数值
 * @param bit 位数
 * @return 截断后的值
 */
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 decimal_t truncate(const decimal_t x, const int bit) noexcept {
    const decimal_t factor = power(10.0L, static_cast<uint32_t>(absolute(bit)));
    return (x < 0 ? ceil(x * factor, 0) : floor(x * factor, 0)) / factor;
}

/**
 * @brief 截断到整数位
 * @param x 原数值
 * @return 截断后的整数值
 */
NEFORCE_CONST_FUNCTION NEFORCE_CONSTEXPR14 decimal_t truncate(const decimal_t x) noexcept {
    if (!is_finite(x)) {
        return x;
    }
    return static_cast<decimal_t>(safe_trunc(x));
}

/**
 * @brief 判断是否接近某个倍数值
 * @param x 待判断值
 * @param axis 基准值
 * @param toler 容差
 * @return 如果x接近axis的整数倍则返回true
 * @exception math_exception 当axis为0时抛出
 */
NEFORCE_CONSTEXPR14 bool around_multiple(const decimal_t x, const decimal_t axis,
                                         const decimal_t toler = constants::DEFAULT_TOLERANCE) {
    if (absolute(axis) < constants::MACHINE_EPSILON) {
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
NEFORCE_CONSTEXPR14 bool around_pi(const decimal_t x, const decimal_t toler = constants::LOOSE_TOLERANCE) {
    return around_multiple(x, constants::PI, toler);
}

/**
 * @brief 判断是否接近零
 * @param x 待判断值
 * @param toler 容差
 * @return 如果|x| < toler则返回true
 */
NEFORCE_CONSTEXPR14 bool around_zero(const decimal_t x, const decimal_t toler = constants::LOOSE_TOLERANCE) noexcept {
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
    if (is_nan(x) || is_nan(y) || is_infinity(x) || y == 0.0L) {
        return numeric_traits<decimal_t>::quiet_nan();
    }
    return x - y * round(x / y);
}

/**
 * @brief 获取小数部分
 * @param x 原数值
 * @return x的小数部分
 */
NEFORCE_CONST_FUNCTION constexpr decimal_t float_part(const decimal_t x) noexcept {
    return x - static_cast<decimal_t>(safe_trunc(x));
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
    *int_ptr = safe_trunc(x);
    return x - static_cast<decimal_t>(*int_ptr);
}

/// @cond
NEFORCE_BEGIN_INNER__

NEFORCE_CONSTEXPR14 void reduce_arg_sincos(decimal_t& x, int& quadrant) noexcept {
    if (x < 0.0L) {
        x = -x;
        quadrant = 3;
    } else {
        quadrant = 0;
    }

    constexpr decimal_t HALF_PI_HI = 1.57079632679489661923L;
    constexpr decimal_t HALF_PI_LO = 6.12323399573676603587e-17L;
    constexpr decimal_t INV_HALF_PI = 0.63661977236758134308L;

    const decimal_t kd = round(x * INV_HALF_PI);
    const auto k = static_cast<int64_t>(kd);
    quadrant = static_cast<int>(quadrant + (k & 3)) & 3;

    decimal_t t = kd * HALF_PI_HI;
    x = x - t;
    t = kd * HALF_PI_LO;
    x = x - t;
}

NEFORCE_CONSTEXPR14 void sincos_taylor(const decimal_t x, decimal_t& sin_val, decimal_t& cos_val) noexcept {
    const decimal_t x2 = x * x;

    decimal_t term_sin = x;
    decimal_t sum_sin = x;

    decimal_t term_cos = 1.0L;
    decimal_t sum_cos = 1.0L;

    decimal_t i = 1.0L;
    bool sin_done = false, cos_done = false;

    while (!sin_done || !cos_done) {
        if (!sin_done) {
            term_sin = -term_sin * x2 / ((i + 1.0L) * (i + 2.0L));
            sum_sin += term_sin;
            if (absolute(term_sin) <= constants::MACHINE_EPSILON * absolute(sum_sin)) {
                sin_done = true;
            }
        }
        if (!cos_done) {
            term_cos = -term_cos * x2 / (i * (i + 1.0L));
            sum_cos += term_cos;
            if (absolute(term_cos) <= constants::MACHINE_EPSILON * absolute(sum_cos)) {
                cos_done = true;
            }
        }
        i += 2.0L;
    }

    sin_val = sum_sin;
    cos_val = sum_cos;
}

NEFORCE_END_INNER__
/// @endcond

/**
 * @brief 计算正弦值
 * @param x 弧度值
 * @return sin(x)
 *
 * 使用泰勒展开计算，先进行周期性处理。
 */
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 decimal_t sine(decimal_t x) noexcept {
    if (is_nan(x) || is_infinity(x)) {
        return numeric_traits<decimal_t>::quiet_nan();
    }

    int quadrant = 0;
    inner::reduce_arg_sincos(x, quadrant);

    decimal_t sin_x = 0.0L, cos_x = 0.0L;
    inner::sincos_taylor(x, sin_x, cos_x);

    switch (quadrant) {
        case 0:
            return sin_x;
        case 1:
            return cos_x;
        case 2:
            return -sin_x;
        default:
            return -cos_x;
    }
}

/**
 * @brief 计算余弦值
 * @param x 弧度值
 * @return cos(x)
 *
 * 利用恒等式 cos(x) = sin(π/2 - x) 计算。
 */
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 decimal_t cosine(decimal_t x) noexcept {
    if (is_nan(x) || is_infinity(x)) {
        return numeric_traits<decimal_t>::quiet_nan();
    }

    int quadrant = 0;
    inner::reduce_arg_sincos(x, quadrant);
    decimal_t sin_x = 0.0L, cos_x = 0.0L;
    inner::sincos_taylor(x, sin_x, cos_x);

    switch (quadrant) {
        case 0:
            return cos_x;
        case 1:
            return -sin_x;
        case 2:
            return -cos_x;
        default:
            return sin_x;
    }
}

/**
 * @brief 计算正切值
 * @param x 弧度值
 * @return tan(x)
 * @exception math_exception 当x接近π/2的奇数倍时抛出
 */
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 decimal_t tangent(decimal_t x) noexcept {
    if (is_nan(x) || is_infinity(x)) {
        return numeric_traits<decimal_t>::quiet_nan();
    }

    int quadrant = 0;
    inner::reduce_arg_sincos(x, quadrant);
    decimal_t sin_x = 0.0L, cos_x = 0.0L;
    inner::sincos_taylor(x, sin_x, cos_x);

    decimal_t s = 0.0L, c = 0.0L;
    switch (quadrant) {
        case 0:
            s = sin_x;
            c = cos_x;
            break;
        case 1:
            s = cos_x;
            c = -sin_x;
            break;
        case 2:
            s = -sin_x;
            c = -cos_x;
            break;
        default:
            s = -cos_x;
            c = sin_x;
            break;
    }

    if (around_zero(c, constants::LOOSE_TOLERANCE)) {
        constexpr auto inf = static_cast<decimal_t>(numeric_traits<decimal_t>::infinity());
        return (s > 0) ? inf : -inf;
    }
    return s / c;
}

/**
 * @brief 计算余切值
 * @param x 弧度值
 * @return cot(x)
 * @exception math_exception 当x接近π的整数倍时抛出
 */
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 decimal_t cotangent(const decimal_t x) { return 1.0L / tangent(x); }

/**
 * @brief 计算反正切值
 * @param x 参数
 * @return arctan(x)（弧度值，范围(-π/2, π/2)）
 *
 * 当|x|>1时，利用恒等式 arctan(x) = π/2 - arctan(1/x)。
 */
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 decimal_t arctangent(decimal_t x) noexcept {
    if (is_nan(x)) {
        return x;
    }
    if (is_infinity(x)) {
        return (x > 0) ? constants::PI / 2 : -constants::PI / 2;
    }

    bool negative = x < 0.0L;
    if (negative) {
        x = -x;
    }

    bool recip = false;
    if (x > 1.0L) {
        x = 1.0L / x;
        recip = true;
    }

    x = x / (1.0L + square_root(1.0L + x * x));
    x = x / (1.0L + square_root(1.0L + x * x));

    const decimal_t x2 = x * x;
    decimal_t term = x, s = x, n = 1.0L;
    while (absolute(term) > constants::MACHINE_EPSILON * absolute(s)) {
        term *= -x2;
        n += 2.0L;
        s += term / n;
    }

    decimal_t result = 4.0L * s;
    if (recip) {
        result = constants::PI / 2 - result;
    }
    if (negative) {
        result = -result;
    }
    return result;
}

/**
 * @brief 计算反正弦值
 * @param x 参数（|x| ≤ 1）
 * @return arcsin(x)（弧度值，范围[-π/2, π/2]）
 */
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 decimal_t arcsine(const decimal_t x) noexcept {
    if (is_nan(x)) {
        return x;
    }
    if (absolute(x) > 1.0L) {
        return numeric_traits<decimal_t>::quiet_nan();
    }
    if (x == 1.0L) {
        return constants::PI / 2;
    }
    if (x == -1.0L) {
        return -constants::PI / 2;
    }
    return arctangent(x / square_root(1.0L - x * x));
}

/**
 * @brief 计算反余弦值
 * @param x 参数（|x| ≤ 1）
 * @return arccos(x)（弧度值，范围[0, π]）
 */
NEFORCE_PURE_FUNCTION NEFORCE_CONSTEXPR14 decimal_t arccosine(const decimal_t x) noexcept {
    if (is_nan(x)) {
        return x;
    }
    if (absolute(x) > 1.0L) {
        return numeric_traits<decimal_t>::quiet_nan();
    }
    if (x == 1.0L) {
        return 0.0L;
    }
    if (x == -1.0L) {
        return constants::PI;
    }
    return constants::PI / 2 - arcsine(x);
}

/** @} */ // MathFunctions

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_NUMERIC_MATH_HPP__
