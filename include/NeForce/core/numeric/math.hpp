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
#include "NeForce/core/numeric/constants.hpp"
#include "NeForce/core/numeric/numeric_types.hpp"
NEFORCE_BEGIN_NAMESPACE__

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
 * @section precision_support 精度类型支持
 *
 * | 类型      | 级数系数  | 机器精度                              | 默认容差 | 宽松容差 |
 * |-----------|-----------|---------------------------------------|----------|----------|
 * | float     | float     | 1.192e-7                              | 1e-6     | 1e-5     |
 * | double    | double    | 2.220e-16                             | 1e-12    | 1e-9     |
 * | decimal_t | decimal_t | x87 扩展 1.084e-19；Windows 上同 double | 1e-12    | 1e-9     |
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
 * 迭代终止条件：当前项小于对应类型的机器精度与当前和的乘积。
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
 * - 初始猜测：guess = 1 + (m - 1) / 3，m 为 2 的偶数次幂归一化后的尾数
 * - 迭代公式：guess = 0.5 × (guess + m / guess)
 * - 终止条件：相邻两次迭代之差不超过 tolerance × |guess|，且最多迭代 64 次，
 *   收敛后再补一次迭代以保证结果达到 T 的机器精度
 *
 * **立方根 ³√x**（牛顿迭代法）：
 * - 先用 2 的整数次幂阶梯把 x 归一化为 m·2^k（k 为 3 的倍数），避免初值偏离导致迭代次数激增
 * - 迭代公式：guess = (2×prev + m / prev²) / 3
 * - 收敛后结果乘以 2^(k/3)
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

/**
 * @brief 将浮点数安全转换为 int64_t
 * @tparam T 浮点类型
 * @param x 原数值
 * @return 截断后的整数；NaN、无穷大或超出 int64_t 表示范围时返回 0
 */
template <typename T>
NEFORCE_CONST_FUNCTION constexpr int64_t safe_truncate(const T x) noexcept {
    static_assert(is_floating_point_v<T>, "floating point required");
    if (is_nan(x) || is_infinity(x)) {
        return 0;
    }

    constexpr auto upper = static_cast<T>(9223372036854775808.0L);
    constexpr auto lower = static_cast<T>(-9223372036854775808.0L);
    if (x >= upper || x < lower) {
        return 0;
    }
    return static_cast<int64_t>(x);
}

/**
 * @brief 将非负浮点数安全转换为 uint64_t（避免 >= 2^63 时的 UB）
 * @tparam T 浮点类型
 * @param x 原数值
 * @return 对应的 uint64_t 值
 */
template <typename T>
NEFORCE_CONST_FUNCTION constexpr uint64_t safe_decimal_to_uint64(const T x) noexcept {
    static_assert(is_floating_point_v<T>, "floating point required");
    constexpr auto two_pow_63 = static_cast<T>(9223372036854775808.0L);
    if (x >= two_pow_63) {
        return static_cast<uint64_t>(x - two_pow_63) + 9223372036854775808ULL;
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
NEFORCE_PURE_FUNCTION constexpr uint64_t fibonacci(const uint32_t n) noexcept {
    constexpr uint32_t fibonacci_table_count = extent_v<decltype(constants::fibonacci_table)>;
    if (n < fibonacci_table_count) {
        return constants::fibonacci_table[n];
    }
    uint64_t a = constants::fibonacci_table[fibonacci_table_count - 2];
    uint64_t b = constants::fibonacci_table[fibonacci_table_count - 1];
    for (uint32_t i = fibonacci_table_count; i <= n; ++i) {
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
NEFORCE_PURE_FUNCTION constexpr uint64_t leonardo(const uint32_t n) noexcept { return 2 * fibonacci(n + 1) - 1; }

/**
 * @brief 角度转弧度
 * @tparam T 运算类型
 * @param angular 角度值
 * @return 对应的弧度值
 */
template <typename T>
NEFORCE_PURE_FUNCTION constexpr T angular2radian(const T angular) noexcept {
    static_assert(is_floating_point_v<T>, "arithmetic required");
    return angular * constants::pi_v<T> / constants::semi_circle_v<T>;
}

/**
 * @brief 弧度转角度
 * @tparam T 运算类型
 * @param radian 弧度值
 * @return 对应的角度值
 */
template <typename T>
NEFORCE_PURE_FUNCTION constexpr T radian2angular(const T radian) noexcept {
    static_assert(is_floating_point_v<T>, "arithmetic required");
    return radian * (constants::semi_circle_v<T> / constants::pi_v<T>);
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
constexpr int sign(const T& value) noexcept {
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
NEFORCE_CONST_FUNCTION constexpr T gcd(const T& m, const T& n) noexcept {
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
NEFORCE_CONST_FUNCTION constexpr T lcm(const T& m, const T& n) noexcept {
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
constexpr enable_if_t<is_floating_point_v<T>, T> mod(const T x, const T y) {
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
constexpr enable_if_t<is_integral_v<T>, T> mod(const T x, const T y) {
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
NEFORCE_CONST_FUNCTION constexpr T power(const T& x, uint32_t n) noexcept {
    static_assert(is_arithmetic_v<T>, "arithmetic required");
    if (n == 0) {
        return T(1);
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
 * @tparam T 浮点类型，默认为 decimal_t
 * @param n 指数
 * @return e^n
 */
template <typename T = decimal_t>
NEFORCE_PURE_FUNCTION constexpr T exponential(const uint32_t n) noexcept {
    static_assert(is_floating_point_v<T>, "floating point required");
    return power(constants::e_v<T>, n);
}

/**
 * @brief 计算 2 的整数次幂
 * @tparam T 浮点类型，默认为 decimal_t
 * @param exponent 指数
 * @return 2^exponent
 *
 * 使用二进制幂计算，最多需要 2 * log₂(|exponent|) 次乘法，
 * 结果超出浮点表示范围时按 IEEE 754 规则上溢为无穷大或下溢为 0。
 */
template <typename T = decimal_t>
NEFORCE_PURE_FUNCTION constexpr T power2(const int64_t exponent) noexcept {
    static_assert(is_floating_point_v<T>, "floating point required");
    T scale(1);
    T factor = exponent >= 0 ? T(2) : T(0.5);
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
 * @tparam T 浮点类型
 * @param value 输入输出参数，输入为正数，输出归一化到 [1, 2) 的尾数
 * @return 归一化使用的指数
 *
 * 归一化前的数值 = 归一化后的数值 × 2^返回值
 */
template <typename T>
constexpr int64_t normalize_power2(T& value) noexcept {
    static_assert(is_floating_point_v<T>, "floating point required");
    T steps[10] = {T(0)};
    steps[0] = T(2);
    for (int i = 1; i < 10; ++i) {
        steps[i] = steps[i - 1] * steps[i - 1];
    }

    int64_t exponent = 0;

    for (int i = 9; i >= 0; --i) {
        while (value >= T(2) * steps[i]) {
            value /= steps[i];
            exponent += static_cast<int64_t>(1) << i;
        }
    }
    while (value >= T(2)) {
        value *= T(0.5);
        ++exponent;
    }

    for (int i = 9; i >= 0; --i) {
        while (value < T(1) && value * steps[i] <= T(1)) {
            value *= steps[i];
            exponent -= static_cast<int64_t>(1) << i;
        }
    }
    while (value < T(1)) {
        value *= T(2);
        --exponent;
    }
    return exponent;
}

/**
 * @brief 计算 e 的 x 次幂
 * @tparam T 浮点类型
 * @param x 指数
 * @return e^x
 *
 * 采用范围归约 + 泰勒展开 + 2 的整数次幂缩放
 */
template <typename T>
NEFORCE_PURE_FUNCTION constexpr T exponential_e(const T x) noexcept {
    static_assert(is_floating_point_v<T>, "floating point required");
    if (is_nan(x)) {
        return x;
    }
    if (is_infinity(x)) {
        return x > T(0) ? x : T(0);
    }
    if (x == T(0)) {
        return T(1);
    }

    constexpr T max_exponent = sizeof(T) <= sizeof(float)
                                       ? static_cast<T>(88.72283905206835L)
                                       : (sizeof(T) <= sizeof(double) ? static_cast<T>(709.78271289338397L)
                                                                      : static_cast<T>(11356.523406294143L));
    constexpr T min_exponent = sizeof(T) <= sizeof(float)
                                       ? static_cast<T>(-103.97207708399179L)
                                       : (sizeof(T) <= sizeof(double) ? static_cast<T>(-745.13321910194110L)
                                                                      : static_cast<T>(-11433.462743168135L));

    if (x > max_exponent) {
        return numeric_traits<T>::infinity();
    }
    if (x < min_exponent) {
        return T(0);
    }

    const T nearest = x * constants::log2e_v<T>;
    int64_t k = safe_truncate(nearest);
    if (nearest - static_cast<T>(k) > T(0.5)) {
        ++k;
    } else if (nearest - static_cast<T>(k) < T(-0.5)) {
        --k;
    }

    const T r = x - static_cast<T>(k) * constants::ln2_v<T>;

    const T* const coefficients = constants::exponential_coefficients_table<T>::value;
    T polynomial = coefficients[19];
    for (int i = 18; i >= 0; --i) {
        polynomial = polynomial * r + coefficients[i];
    }
    return polynomial * power2<T>(k);
}

/**
 * @brief 计算自然对数
 * @tparam T 浮点类型
 * @param x 真数
 * @return ln(x)
 *
 * 采用倒数归约 + 2 的整数次幂阶梯归约 + 反正切级数
 */
template <typename T>
NEFORCE_PURE_FUNCTION constexpr T logarithm_e(const T x) noexcept {
    static_assert(is_floating_point_v<T>, "floating point required");
    if (is_nan(x)) {
        return x;
    }
    if (x < T(0)) {
        return numeric_traits<T>::quiet_nan();
    }
    if (x == T(0)) {
        return static_cast<T>(-numeric_traits<T>::infinity());
    }
    if (is_infinity(x) && x > T(0)) {
        return x;
    }
    if (x < T(1)) {
        return -logarithm_e(T(1) / x);
    }

    T m = x;
    int64_t k = normalize_power2(m);

    if (m > constants::sqrt2_v<T>) {
        m *= T(0.5);
        ++k;
    }

    const T a = (m - T(1)) / (m + T(1));
    const T a2 = a * a;
    T term = a;
    T s = a;

    const T* const coefficients = constants::logarithm_coefficients_table<T>::value;
    for (uint32_t n = 1; n <= 14; ++n) {
        term *= a2;
        s += term * coefficients[n - 1];
    }

    return T(2) * s + static_cast<T>(k) * constants::ln2_v<T>;
}

/**
 * @brief 浮点幂运算
 * @tparam T 浮点类型
 * @param x 底数
 * @param y 指数
 * @return x 的 y 次幂
 */
template <typename T, enable_if_t<is_floating_point_v<T>, int> = 0>
NEFORCE_NODISCARD constexpr T power(const T& x, const T& y) noexcept {
    if (y == T(0)) {
        return T(1);
    }
    if (x <= T(0)) {
        return numeric_traits<T>::quiet_nan();
    }
    return exponential_e(y * logarithm_e(x));
}

/**
 * @brief 计算 ln(1 + x)
 * @tparam T 浮点类型
 * @param x 真数减一
 * @return ln(1 + x)
 */
template <typename T>
NEFORCE_PURE_FUNCTION constexpr T logarithm_1p(const T x) noexcept {
    static_assert(is_floating_point_v<T>, "floating point required");
    if (is_nan(x) || x < T(-1)) {
        return numeric_traits<T>::quiet_nan();
    }
    if (x == T(-1)) {
        return static_cast<T>(-numeric_traits<T>::infinity());
    }
    if (is_infinity(x)) {
        return x;
    }
    if (x == T(0)) {
        return T(0);
    }

    if (absolute(x) <= T(0.125)) {
        const T* const coefficients = constants::logarithm_1p_coefficients_table<T>::value;
        T term = x;
        T total = T(0);

        for (uint32_t n = 1; n <= 22; ++n) {
            total += term * coefficients[n - 1];
            term *= -x;
        }
        return total;
    }
    return logarithm_e(T(1) + x);
}

/**
 * @brief 计算任意底数的对数
 * @tparam T 浮点类型
 * @param x 真数
 * @param base 底数
 * @return 以base为底x的对数
 * @exception math_exception 底数为1时
 */
template <typename T>
constexpr T logarithm(const T x, const uint32_t base) {
    static_assert(is_floating_point_v<T>, "floating point required");
    const T under = logarithm_e(static_cast<T>(base));
    if (under == T(0)) {
        NEFORCE_THROW_EXCEPTION(math_exception("zero can not be dividend."));
    }
    return logarithm_e(x) / under;
}

/**
 * @brief 计算以2为底的对数
 * @tparam T 浮点类型
 * @param x 真数
 * @return log₂(x)
 */
template <typename T>
constexpr T logarithm_2(const T x) {
    return logarithm(x, 2);
}

/**
 * @brief 计算以10为底的对数
 * @tparam T 浮点类型
 * @param x 真数
 * @return log₁₀(x)
 */
template <typename T>
constexpr T logarithm_10(const T x) {
    return logarithm(x, 10);
}

/**
 * @brief 计算平方根
 * @tparam T 浮点类型
 * @param x 被开方数
 * @param precise 精度要求
 * @return √x
 *
 * 使用 2 的整数次幂阶梯把 x 归一化为 m·2^k（m ∈ [1, 4)、k 为偶数）后做牛顿迭代，
 * 收敛后再补一次迭代，使结果达到 T 的机器精度。
 */
template <typename T>
NEFORCE_CONST_FUNCTION constexpr T
square_root(const T x, const type_identity_t<T> precise = constants::default_tolerance_v<T>) noexcept {
    static_assert(is_floating_point_v<T>, "floating point required");
    if (is_nan(x)) {
        return x;
    }
    if (x < T(0)) {
        return numeric_traits<T>::quiet_nan();
    }
    if (x == T(0)) {
        return x;
    }
    if (is_infinity(x) && x > T(0)) {
        return x;
    }

    T m = x;
    int64_t k = normalize_power2(m);
    if ((k & 1) != 0) {
        m *= T(2);
        --k;
    }

    T guess = T(1) + (m - T(1)) / T(3);
    for (uint32_t iteration = 0; iteration < 64; ++iteration) {
        const T next = (guess + m / guess) * T(0.5);
        const bool converged = absolute(next - guess) <= precise * absolute(next);
        guess = next;
        if (converged) {
            break;
        }
    }
    guess = (guess + m / guess) * T(0.5);

    return guess * power2<T>(k / 2);
}

/**
 * @brief 计算立方根
 * @tparam T 浮点类型
 * @param x 被开方数
 * @param precise 精度要求
 * @return ³√x
 *
 * 使用 2 的整数次幂阶梯把 x 归一化为 m·2^k（m ∈ [1, 8)、k 为 3 的倍数）后做牛顿迭代，
 * 收敛后再补一次迭代，使结果达到 T 的机器精度。
 */
template <typename T>
NEFORCE_PURE_FUNCTION constexpr T
cube_root(const T x, const type_identity_t<T> precise = constants::default_tolerance_v<T>) noexcept {
    static_assert(is_floating_point_v<T>, "floating point required");
    if (is_nan(x)) {
        return x;
    }
    if (is_infinity(x)) {
        return x;
    }
    if (x == T(0)) {
        return x;
    }

    const bool negative = x < T(0);
    T m = negative ? -x : x;

    int64_t k = normalize_power2(m);
    while (k % 3 != 0) {
        m *= T(2);
        --k;
    }

    T guess = T(1) + (m - T(1)) / T(3);
    for (uint32_t iteration = 0; iteration < 64; ++iteration) {
        const T next = (T(2) * guess + m / (guess * guess)) / T(3);
        const bool converged = absolute(next - guess) <= precise * absolute(next);
        guess = next;
        if (converged) {
            break;
        }
    }
    guess = (T(2) * guess + m / (guess * guess)) / T(3);

    const T result = guess * power2<T>(k / 3);
    return negative ? -result : result;
}

/**
 * @brief 计算阶乘
 * @param n 非负整数
 * @return n!
 */
NEFORCE_CONST_FUNCTION constexpr uint64_t factorial(const uint32_t n) noexcept {
    uint64_t result = 1;
    for (uint32_t i = 2; i <= n; ++i) {
        result *= i;
    }
    return result;
}

/**
 * @brief 计算阶乘的自然对数
 * @tparam T 浮点类型，默认为 decimal_t
 * @param n 非负整数
 * @return ln(n!)
 *
 * n ≤ 32 时直接查表，n > 32 时使用斯特林级数
 * ln(n!) = (n + 1/2)ln(n) - n + ln(2π)/2 + 1/(12n) - 1/(360n³) + 1/(1260n⁵) - 1/(1680n⁷)
 */
template <typename T = decimal_t>
NEFORCE_PURE_FUNCTION constexpr T logarithm_factorial(const uint64_t n) noexcept {
    static_assert(is_floating_point_v<T>, "floating point required");
    if (n <= 32) {
        return constants::logarithm_factorial_table<T>::value[n];
    }

    const auto value = static_cast<T>(n);
    const T inverse = T(1) / value;
    const T inverse2 = inverse * inverse;
    const T correction =
            T(1) / T(12) + inverse2 * (T(-1) / T(360) + inverse2 * (T(1) / T(1260) + inverse2 * (T(-1) / T(1680))));

    return (value + T(0.5)) * logarithm_e(value) - value + T(0.91893853320467274178032973640561763986L) +
           inverse * correction;
}

/**
 * @brief 向下取整
 * @tparam T 浮点类型
 * @param x 原数值
 * @return 向下取整后的值
 */
template <typename T>
NEFORCE_CONST_FUNCTION constexpr T floor(const T x) noexcept {
    static_assert(is_floating_point_v<T>, "floating point required");
    if (!is_finite(x)) {
        return x;
    }

    constexpr auto TWO_POW_64 = static_cast<T>(numeric_traits<uint64_t>::max());
    if (x >= T(0)) {
        if (x >= TWO_POW_64) {
            return x;
        }
        return static_cast<T>(safe_decimal_to_uint64(x));
    } else {
        const T pos = -x;
        if (pos >= TWO_POW_64) {
            return x;
        }
        const auto floor_pos = static_cast<T>(safe_decimal_to_uint64(pos));
        if (absolute(pos - floor_pos) < constants::machine_epsilon_v<T>) {
            return -floor_pos;
        } else {
            return -(floor_pos + T(1));
        }
    }
}

/**
 * @brief 向下取整到指定位数
 * @tparam T 浮点类型
 * @param x 原数值
 * @param bit 位数
 * @return 向下取整后的值
 */
template <typename T>
NEFORCE_CONST_FUNCTION constexpr T floor(const T x, const uint32_t bit) noexcept {
    static_assert(is_floating_point_v<T>, "floating point required");
    const T factor = power(T(10), bit);
    return floor(x * factor) / factor;
}

template <typename T>
NEFORCE_CONST_FUNCTION constexpr bool is_even_integer(const T x) noexcept {
    static_assert(is_floating_point_v<T>, "floating point required");
    const T half = x * T(0.5);
    return absolute(half - floor(half)) < constants::machine_epsilon_v<T>;
}

/**
 * @brief 向上取整
 * @tparam T 浮点类型
 * @param x 原数值
 * @return 向上取整后的值
 */
template <typename T>
NEFORCE_CONST_FUNCTION constexpr T ceil(const T x) noexcept {
    static_assert(is_floating_point_v<T>, "floating point required");
    if (!is_finite(x)) {
        return x;
    }

    constexpr auto two_pow_64 = static_cast<T>(numeric_traits<uint64_t>::max());
    if (x >= T(0)) {
        if (x >= two_pow_64) {
            return x;
        }
        const auto floor_x = static_cast<T>(safe_decimal_to_uint64(x));
        if (absolute(x - floor_x) < constants::machine_epsilon_v<T>) {
            return floor_x;
        } else {
            return floor_x + T(1);
        }
    } else {
        const T pos = -x;
        if (pos >= two_pow_64) {
            return x;
        }
        const uint64_t floor_pos = safe_decimal_to_uint64(pos);
        return -static_cast<T>(floor_pos);
    }
}

/**
 * @brief 向上取整到指定位数
 * @tparam T 浮点类型
 * @param x 原数值
 * @param bit 位数
 * @return 向上取整后的值
 */
template <typename T>
NEFORCE_CONST_FUNCTION constexpr T ceil(const T x, const uint32_t bit) noexcept {
    static_assert(is_floating_point_v<T>, "floating point required");
    const T factor = power(T(10), bit);
    return ceil(x * factor) / factor;
}

/**
 * @brief 四舍五入
 * @tparam T 浮点类型
 * @param x 原数值
 * @return 四舍五入后的值
 */
template <typename T>
NEFORCE_CONST_FUNCTION constexpr T round(const T x) noexcept {
    static_assert(is_floating_point_v<T>, "floating point required");
    if (!is_finite(x)) {
        return x;
    }

    const T abs_x = absolute(x);
    const T int_part = floor(abs_x);
    const T frac = abs_x - int_part;

    if (frac < T(0.5)) {
        return (x < T(0)) ? -int_part : int_part;
    }
    if (frac > T(0.5)) {
        return (x < T(0)) ? -(int_part + T(1)) : (int_part + T(1));
    }

    if (is_even_integer(int_part)) {
        return (x < T(0)) ? -int_part : int_part;
    } else {
        return (x < T(0)) ? -(int_part + T(1)) : (int_part + T(1));
    }
}

/**
 * @brief 向下舍入到指定位数
 * @tparam T 浮点类型
 * @param x 原数值
 * @param bit 位数，>0表示小数位，0表示整数位
 * @return 向下舍入后的值
 */
template <typename T>
NEFORCE_CONST_FUNCTION constexpr T floor_bit(const T x, const uint32_t bit) noexcept {
    static_assert(is_floating_point_v<T>, "floating point required");
    const T factor = power(T(10), bit);
    return floor(x * factor) / factor;
}

/**
 * @brief 向上舍入到指定位数
 * @tparam T 浮点类型
 * @param x 原数值
 * @param bit 位数
 * @return 向上舍入后的值
 */
template <typename T>
NEFORCE_CONST_FUNCTION constexpr T ceil_bit(const T x, const uint32_t bit) noexcept {
    static_assert(is_floating_point_v<T>, "floating point required");
    const T factor = power(T(10), bit);
    return ceil(x * factor) / factor;
}

/**
 * @brief 四舍五入到指定位数
 * @tparam T 浮点类型
 * @param x 原数值
 * @param bit 位数
 * @return 四舍五入后的值
 */
template <typename T>
NEFORCE_CONST_FUNCTION constexpr T round_bit(const T x, const uint32_t bit) noexcept {
    static_assert(is_floating_point_v<T>, "floating point required");
    return x < T(0) ? ceil_bit(x - T(0.5) / power(T(10), bit), bit) : floor_bit(x + T(0.5) / power(T(10), bit), bit);
}

/**
 * @brief 截断到指定位数
 * @tparam T 浮点类型
 * @param x 原数值
 * @param bit 位数
 * @return 截断后的值
 */
template <typename T>
NEFORCE_CONST_FUNCTION constexpr T truncate_bit(const T x, const uint32_t bit) noexcept {
    static_assert(is_floating_point_v<T>, "floating point required");
    return x < T(0) ? ceil_bit(x, bit) : floor_bit(x, bit);
}

/**
 * @brief 四舍五入到指定位数
 * @tparam T 浮点类型
 * @param x 原数值
 * @param bit 位数
 * @return 四舍五入后的值
 */
template <typename T>
NEFORCE_CONST_FUNCTION constexpr T round(const T x, const uint32_t bit) noexcept {
    static_assert(is_floating_point_v<T>, "floating point required");
    const T factor = power(T(10), bit);
    return round(x * factor) / factor;
}

/**
 * @brief 截断
 * @tparam T 浮点类型
 * @param x 原数值
 * @param bit 位数
 * @return 截断后的值
 */
template <typename T>
NEFORCE_CONST_FUNCTION constexpr T truncate(const T x, const int bit) noexcept {
    static_assert(is_floating_point_v<T>, "floating point required");
    const T factor = power(T(10), static_cast<uint32_t>(absolute(bit)));
    return (x < T(0) ? ceil(x * factor, 0) : floor(x * factor, 0)) / factor;
}

/**
 * @brief 截断到整数位
 * @tparam T 浮点类型
 * @param x 原数值
 * @return 截断后的整数值
 */
template <typename T>
NEFORCE_CONST_FUNCTION constexpr T truncate(const T x) noexcept {
    static_assert(is_floating_point_v<T>, "floating point required");
    if (!is_finite(x)) {
        return x;
    }
    return static_cast<T>(safe_truncate(x));
}

/**
 * @brief 判断是否接近某个倍数值
 * @tparam T 浮点类型
 * @param x 待判断值
 * @param axis 基准值
 * @param toler 容差
 * @return 如果x接近axis的整数倍则返回true
 * @exception math_exception 当axis为0时抛出
 */
template <typename T>
constexpr bool around_multiple(const T x, const type_identity_t<T> axis,
                               const type_identity_t<T> toler = constants::default_tolerance_v<T>) {
    static_assert(is_floating_point_v<T>, "floating point required");
    if (absolute(axis) < constants::machine_epsilon_v<T>) {
        NEFORCE_THROW_EXCEPTION(math_exception("Axis Cannot be 0"));
    }
    const T multi = round(x / axis) * axis;
    return absolute(x - multi) < toler;
}

/**
 * @brief 判断是否接近π的整数倍
 * @tparam T 浮点类型
 * @param x 待判断值
 * @param toler 容差
 * @return 如果x接近π的整数倍则返回true
 */
template <typename T>
constexpr bool around_pi(const T x, const type_identity_t<T> toler = constants::loose_tolerance_v<T>) {
    return around_multiple(x, constants::pi_v<T>, toler);
}

/**
 * @brief 判断是否接近零
 * @tparam T 浮点类型
 * @param x 待判断值
 * @param toler 容差
 * @return 如果|x| < toler则返回true
 */
template <typename T>
constexpr bool around_zero(const T x, const type_identity_t<T> toler = constants::loose_tolerance_v<T>) noexcept {
    return absolute(x) < toler;
}

/**
 * @brief 计算余数
 * @tparam T 浮点类型
 * @param x 被除数
 * @param y 除数
 * @return x除以y的余数
 *
 * 使用对称舍入规则
 */
template <typename T>
NEFORCE_CONST_FUNCTION constexpr T remainder(const T x, const type_identity_t<T> y) noexcept {
    if (is_nan(x) || is_nan(y) || is_infinity(x) || y == T(0)) {
        return numeric_traits<T>::quiet_nan();
    }
    return x - y * round(x / y);
}

/**
 * @brief 获取小数部分
 * @tparam T 浮点类型
 * @param x 原数值
 * @return x的小数部分
 */
template <typename T>
NEFORCE_CONST_FUNCTION constexpr T float_part(const T x) noexcept {
    return x - static_cast<T>(safe_truncate(x));
}

/**
 * @brief 分离整数和小数部分
 * @tparam T 浮点类型
 * @param x 原数值
 * @param int_ptr 指向整数部分的指针
 * @return 小数部分
 *
 * 将x的整数部分存入int_ptr，返回小数部分。
 */
template <typename T>
NEFORCE_CONST_FUNCTION constexpr T float_apart(T x, int64_t* int_ptr) noexcept {
    *int_ptr = safe_truncate(x);
    return x - static_cast<T>(*int_ptr);
}

/// @cond
NEFORCE_BEGIN_INNER__

template <typename T>
constexpr void reduce_arg_sincos(T& x, int& quadrant) noexcept {
    constexpr T half_pi_hi = static_cast<T>(1.570796326794896619231321691639751442L);
    constexpr T half_pi_lo = static_cast<T>(6.123233995736766035868820147291983023e-17L);
    constexpr T inv_half_pi = static_cast<T>(0.636619772367581343075535053490057448L);

    const T kd = round(x * inv_half_pi);
    const auto k = safe_truncate(kd);
    quadrant = static_cast<int>(k & 3);

    T t = kd * half_pi_hi;
    x = x - t;
    t = kd * half_pi_lo;
    x = x - t;
}

template <typename T>
constexpr void sincos_taylor(const T x, T& sin_val, T& cos_val) noexcept {
    const T x2 = x * x;

    T term_sin = x;
    T sum_sin = x;

    T term_cos = T(1);
    T sum_cos = T(1);

    T i = T(1);
    bool sin_done = false, cos_done = false;

    while (!sin_done || !cos_done) {
        if (!sin_done) {
            term_sin = -term_sin * x2 / ((i + T(1)) * (i + T(2)));
            sum_sin += term_sin;
            if (absolute(term_sin) <= constants::machine_epsilon_v<T> * absolute(sum_sin)) {
                sin_done = true;
            }
        }
        if (!cos_done) {
            term_cos = -term_cos * x2 / (i * (i + T(1)));
            sum_cos += term_cos;
            if (absolute(term_cos) <= constants::machine_epsilon_v<T> * absolute(sum_cos)) {
                cos_done = true;
            }
        }
        i += T(2);
    }

    sin_val = sum_sin;
    cos_val = sum_cos;
}

NEFORCE_END_INNER__
/// @endcond

/**
 * @brief 计算正弦值
 * @tparam T 浮点类型
 * @param x 弧度值
 * @return sin(x)
 *
 * 使用泰勒展开计算，先进行周期性处理。
 */
template <typename T>
NEFORCE_PURE_FUNCTION constexpr T sine(T x) noexcept {
    static_assert(is_floating_point_v<T>, "floating point required");
    if (is_nan(x) || is_infinity(x)) {
        return numeric_traits<T>::quiet_nan();
    }

    const bool negative = x < T(0);
    if (negative) {
        x = -x;
    }

    int quadrant = 0;
    inner::reduce_arg_sincos(x, quadrant);

    T sin_x(0), cos_x(0);
    inner::sincos_taylor(x, sin_x, cos_x);

    T result(0);
    switch (quadrant) {
        case 0:
            result = sin_x;
            break;
        case 1:
            result = cos_x;
            break;
        case 2:
            result = -sin_x;
            break;
        default:
            result = -cos_x;
            break;
    }
    return negative ? -result : result;
}

/**
 * @brief 计算余弦值
 * @tparam T 浮点类型
 * @param x 弧度值
 * @return cos(x)
 *
 * 利用恒等式 cos(x) = sin(π/2 - x) 计算。
 */
template <typename T>
NEFORCE_PURE_FUNCTION constexpr T cosine(T x) noexcept {
    static_assert(is_floating_point_v<T>, "floating point required");
    if (is_nan(x) || is_infinity(x)) {
        return numeric_traits<T>::quiet_nan();
    }

    if (x < T(0)) {
        x = -x;
    }

    int quadrant = 0;
    inner::reduce_arg_sincos(x, quadrant);
    T sin_x(0), cos_x(0);
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
 * @tparam T 浮点类型
 * @param x 弧度值
 * @return tan(x)
 * @exception math_exception 当x接近π/2的奇数倍时抛出
 */
template <typename T>
NEFORCE_PURE_FUNCTION constexpr T tangent(T x) noexcept {
    static_assert(is_floating_point_v<T>, "floating point required");
    if (is_nan(x) || is_infinity(x)) {
        return numeric_traits<T>::quiet_nan();
    }

    const bool negative = x < T(0);
    if (negative) {
        x = -x;
    }

    int quadrant = 0;
    inner::reduce_arg_sincos(x, quadrant);
    T sin_x(0), cos_x(0);
    inner::sincos_taylor(x, sin_x, cos_x);

    T s(0), c(0);
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

    T result(0);
    if (around_zero(c, constants::loose_tolerance_v<T>)) {
        constexpr auto inf = static_cast<T>(numeric_traits<T>::infinity());
        result = (s > T(0)) ? inf : -inf;
    } else {
        result = s / c;
    }
    return negative ? -result : result;
}

/**
 * @brief 计算余切值
 * @tparam T 浮点类型
 * @param x 弧度值
 * @return cot(x)
 * @exception math_exception 当x接近π的整数倍时抛出
 */
template <typename T>
NEFORCE_PURE_FUNCTION constexpr T cotangent(const T x) noexcept {
    return T(1) / tangent(x);
}

/**
 * @brief 计算反正切值
 * @tparam T 浮点类型
 * @param x 参数
 * @return arctan(x)（弧度值，范围(-π/2, π/2)）
 *
 * 当|x|>1时，利用恒等式 arctan(x) = π/2 - arctan(1/x)。
 */
template <typename T>
NEFORCE_PURE_FUNCTION constexpr T arctangent(T x) noexcept {
    static_assert(is_floating_point_v<T>, "floating point required");
    if (is_nan(x)) {
        return x;
    }
    if (is_infinity(x)) {
        return (x > T(0)) ? constants::pi_v<T> / T(2) : -constants::pi_v<T> / T(2);
    }

    const bool negative = x < T(0);
    if (negative) {
        x = -x;
    }

    bool recip = false;
    if (x > T(1)) {
        x = T(1) / x;
        recip = true;
    }

    x = x / (T(1) + square_root(T(1) + x * x));
    x = x / (T(1) + square_root(T(1) + x * x));

    const T x2 = x * x;
    T term = x, s = x, n = T(1);
    while (absolute(term) > constants::machine_epsilon_v<T> * absolute(s)) {
        term *= -x2;
        n += T(2);
        s += term / n;
    }

    T result = T(4) * s;
    if (recip) {
        result = constants::pi_v<T> / T(2) - result;
    }
    if (negative) {
        result = -result;
    }
    return result;
}

/**
 * @brief 计算反正弦值
 * @tparam T 浮点类型
 * @param x 参数（|x| ≤ 1）
 * @return arcsin(x)（弧度值，范围[-π/2, π/2]）
 */
template <typename T>
NEFORCE_PURE_FUNCTION constexpr T arcsine(const T x) noexcept {
    static_assert(is_floating_point_v<T>, "floating point required");
    if (is_nan(x)) {
        return x;
    }
    if (absolute(x) > T(1)) {
        return numeric_traits<T>::quiet_nan();
    }
    if (x == T(1)) {
        return constants::pi_v<T> / T(2);
    }
    if (x == T(-1)) {
        return -constants::pi_v<T> / T(2);
    }
    return arctangent(x / square_root(T(1) - x * x));
}

/**
 * @brief 计算反余弦值
 * @tparam T 浮点类型
 * @param x 参数（|x| ≤ 1）
 * @return arccos(x)（弧度值，范围[0, π]）
 */
template <typename T>
NEFORCE_PURE_FUNCTION constexpr T arccosine(const T x) noexcept {
    static_assert(is_floating_point_v<T>, "floating point required");
    if (is_nan(x)) {
        return x;
    }
    if (absolute(x) > T(1)) {
        return numeric_traits<T>::quiet_nan();
    }
    if (x == T(1)) {
        return T(0);
    }
    if (x == T(-1)) {
        return constants::pi_v<T>;
    }
    return constants::pi_v<T> / T(2) - arcsine(x);
}

/** @} */ // MathFunctions

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_NUMERIC_MATH_HPP__
