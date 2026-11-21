#ifndef NEFORCE_CORE_NUMERICS_CONSTANTS_HPP__
#define NEFORCE_CORE_NUMERICS_CONSTANTS_HPP__
#include "NeForce/core/numeric/numeric_traits.hpp"
#include "NeForce/core/typeinfo/type_traits.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_CONSTANTS__

/**
 * @defgroup MathConstants 数学常量
 * @brief 常用数学常量定义
 *
 * 根据 ISO/IEC 10967-2 和 IEEE 754，提供以下高精度常量：
 *
 * | 常量                   | 符号 | 值（高精度）               | 说明                          |
 * |------------------------|------|----------------------------|-------------------------------|
 * | e_v<T>                 | e    | 2.71828182845904523536     | 自然对数的底                  |
 * | pi_v<T>                | π    | 3.14159265358979323846     | 圆周率                        |
 * | phi_v<T>               | φ    | 1.61803398874989484820     | 黄金分割比                    |
 * | two_pi_hi/two_pi_lo    | 2π   | 6.28318530717958647692     | 2π 高位部分与低位补偿         |
 * | ln2_v<T>               | ln2  | 0.69314718055994530942     | 2 的自然对数                  |
 * | log2e_v<T>             | log₂e| 1.44269504088896340736     | log₂(e)                       |
 * | sqrt2_v<T>             | √2   | 1.41421356237309504880     | 2 的平方根                    |
 * | machine_epsilon_v<T>   | ε    | 取决于 T                   | 机器精度（1 与最小正数差）    |
 * | default_tolerance_v<T> | -    | float 1e-6，其余 1e-12     | 默认数值容差                  |
 * | loose_tolerance_v<T>   | -    | float 1e-5，其余 1e-9      | 宽松数值容差                  |
 * @{
 */

template <typename T>
NEFORCE_INLINE17 constexpr T e_v = enable_if_t<is_floating_point_v<T>, T>(2.718281828459045235360287471352662498L);

template <typename T>
NEFORCE_INLINE17 constexpr T log2e_v = enable_if_t<is_floating_point_v<T>, T>(1.442695040888963407359924681001892137L);

template <typename T>
NEFORCE_INLINE17 constexpr T log10e_v = enable_if_t<is_floating_point_v<T>, T>(0.434294481903251827651128918916605082L);

template <typename T>
NEFORCE_INLINE17 constexpr T pi_v = enable_if_t<is_floating_point_v<T>, T>(3.141592653589793238462643383279502884L);

template <typename T>
NEFORCE_INLINE17 constexpr T inv_pi_v = enable_if_t<is_floating_point_v<T>, T>(0.318309886183790671537767526745028724L);

template <typename T>
NEFORCE_INLINE17 constexpr T inv_sqrtpi_v =
        enable_if_t<is_floating_point_v<T>, T>(0.564189583547756286948079451560772586L);

template <typename T>
NEFORCE_INLINE17 constexpr T ln2_v = enable_if_t<is_floating_point_v<T>, T>(0.693147180559945309417232121458176568L);

template <typename T>
NEFORCE_INLINE17 constexpr T ln10_v = enable_if_t<is_floating_point_v<T>, T>(2.302585092994045684017991454684364208L);

template <typename T>
NEFORCE_INLINE17 constexpr T sqrt2_v = enable_if_t<is_floating_point_v<T>, T>(1.414213562373095048801688724209698079L);

template <typename T>
NEFORCE_INLINE17 constexpr T sqrt3_v = enable_if_t<is_floating_point_v<T>, T>(1.732050807568877293527446341505872367L);

template <typename T>
NEFORCE_INLINE17 constexpr T inv_sqrt3_v =
        enable_if_t<is_floating_point_v<T>, T>(0.577350269189625764509148780501957456L);

template <typename T>
NEFORCE_INLINE17 constexpr T egamma_v = enable_if_t<is_floating_point_v<T>, T>(0.577215664901532860606512090082402431L);

template <typename T>
NEFORCE_INLINE17 constexpr T phi_v = enable_if_t<is_floating_point_v<T>, T>(1.618033988749894848204586834365638118L);

template <typename T>
NEFORCE_INLINE17 constexpr T semi_circle_v = enable_if_t<is_floating_point_v<T>, T>(180.0L);

template <typename T>
NEFORCE_INLINE17 constexpr T circle_v = enable_if_t<is_floating_point_v<T>, T>(360.0L);


template <typename T>
NEFORCE_INLINE17 constexpr T machine_epsilon_v = enable_if_t<is_floating_point_v<T>, T>(numeric_traits<T>::epsilon());

template <typename T>
NEFORCE_INLINE17 constexpr T default_tolerance_v =
        enable_if_t<is_floating_point_v<T>, T>(sizeof(T) <= sizeof(float) ? 1e-6F : 1e-12L);

template <typename T>
NEFORCE_INLINE17 constexpr T loose_tolerance_v =
        enable_if_t<is_floating_point_v<T>, T>(sizeof(T) <= sizeof(float) ? 1e-5F : 1e-9L);


NEFORCE_INLINE17 constexpr float64_t e = e_v<float64_t>;
NEFORCE_INLINE17 constexpr float64_t log2e = log2e_v<float64_t>;
NEFORCE_INLINE17 constexpr float64_t log10e = log10e_v<float64_t>;
NEFORCE_INLINE17 constexpr float64_t pi = pi_v<float64_t>;
NEFORCE_INLINE17 constexpr float64_t inv_pi = inv_pi_v<float64_t>;
NEFORCE_INLINE17 constexpr float64_t inv_sqrtpi = inv_sqrtpi_v<float64_t>;
NEFORCE_INLINE17 constexpr float64_t ln2 = ln2_v<float64_t>;
NEFORCE_INLINE17 constexpr float64_t ln10 = ln10_v<float64_t>;
NEFORCE_INLINE17 constexpr float64_t sqrt2 = sqrt2_v<float64_t>;
NEFORCE_INLINE17 constexpr float64_t sqrt3 = sqrt3_v<float64_t>;
NEFORCE_INLINE17 constexpr float64_t inv_sqrt3 = inv_sqrt3_v<float64_t>;
NEFORCE_INLINE17 constexpr float64_t egamma = egamma_v<float64_t>;
NEFORCE_INLINE17 constexpr float64_t phi = phi_v<float64_t>;
NEFORCE_INLINE17 constexpr float64_t semi_circle = semi_circle_v<float64_t>;
NEFORCE_INLINE17 constexpr float64_t circle = circle_v<float64_t>;

NEFORCE_INLINE17 constexpr decimal_t machine_epsilon = machine_epsilon_v<decimal_t>;
NEFORCE_INLINE17 constexpr decimal_t default_tolerance = default_tolerance_v<decimal_t>;
NEFORCE_INLINE17 constexpr decimal_t loose_tolerance = loose_tolerance_v<decimal_t>;


/**
 * @brief e^x 泰勒展开系数表（1/n!，n = 0..19）
 * @tparam T 浮点类型
 *
 * 范围归约后 |r| <= ln(2)/2，19 阶即可达到对应浮点类型的机器精度。
 */
template <typename T>
struct exponential_coefficients_table {
    /// 展开系数，第 n 项为 1/n!
    static constexpr T value[20] = {T(1) / T(1),
                                    T(1) / T(1),
                                    T(1) / T(2),
                                    T(1) / T(6),
                                    T(1) / T(24),
                                    T(1) / T(120),
                                    T(1) / T(720),
                                    T(1) / T(5040),
                                    T(1) / T(40320),
                                    T(1) / T(362880),
                                    T(1) / T(3628800),
                                    T(1) / T(39916800),
                                    T(1) / T(479001600),
                                    T(1) / T(6227020800),
                                    T(1) / T(87178291200),
                                    T(1) / T(1307674368000),
                                    T(1) / T(20922789888000),
                                    T(1) / T(355687428096000),
                                    T(1) / T(6402373705728000),
                                    T(1) / T(121645100408832000)};
};

template <typename T>
constexpr T exponential_coefficients_table<T>::value[20];

/**
 * @brief 自然对数级数系数表（1/3、1/5 … 1/29）
 * @tparam T 浮点类型
 *
 * 归约后 |a| <= (√2 - 1)/(√2 + 1)，14 项即可达到对应浮点类型的机器精度。
 */
template <typename T>
struct logarithm_coefficients_table {
    /// 级数系数，第 n 项为 1/(2n + 3)
    static constexpr T value[14] = {T(1) / T(3),  T(1) / T(5),  T(1) / T(7),  T(1) / T(9),  T(1) / T(11),
                                    T(1) / T(13), T(1) / T(15), T(1) / T(17), T(1) / T(19), T(1) / T(21),
                                    T(1) / T(23), T(1) / T(25), T(1) / T(27), T(1) / T(29)};
};

template <typename T>
constexpr T logarithm_coefficients_table<T>::value[14];

/**
 * @brief log(1 + x) 级数系数表（1/1、1/2 … 1/22）
 * @tparam T 浮点类型
 *
 * |x| <= 1/8 时 22 项即可达到对应浮点类型的机器精度。
 */
template <typename T>
struct logarithm_1p_coefficients_table {
    /// 级数系数，第 n 项为 1/n
    static constexpr T value[22] = {T(1) / T(1),  T(1) / T(2),  T(1) / T(3),  T(1) / T(4),  T(1) / T(5),  T(1) / T(6),
                                    T(1) / T(7),  T(1) / T(8),  T(1) / T(9),  T(1) / T(10), T(1) / T(11), T(1) / T(12),
                                    T(1) / T(13), T(1) / T(14), T(1) / T(15), T(1) / T(16), T(1) / T(17), T(1) / T(18),
                                    T(1) / T(19), T(1) / T(20), T(1) / T(21), T(1) / T(22)};
};

template <typename T>
constexpr T logarithm_1p_coefficients_table<T>::value[22];

/**
 * @brief ln(n!) 查表值（n = 0..32）
 * @tparam T 浮点类型
 */
template <typename T>
struct logarithm_factorial_table {
    /// 查表值，第 n 项为 ln(n!)
    static constexpr T value[33] = {T(0),
                                    T(0),
                                    T(0.6931471805599453094172L),
                                    T(1.791759469228055000812L),
                                    T(3.178053830347945619647L),
                                    T(4.787491742782045994248L),
                                    T(6.57925121201010099506L),
                                    T(8.525161361065414300166L),
                                    T(10.60460290274525022842L),
                                    T(12.80182748008146961121L),
                                    T(15.10441257307551529523L),
                                    T(17.50230784587388583929L),
                                    T(19.98721449566188614952L),
                                    T(22.55216385312342288557L),
                                    T(25.19122118273868150009L),
                                    T(27.89927138384089156609L),
                                    T(30.67186010608067280376L),
                                    T(33.50507345013688888401L),
                                    T(36.39544520803305357622L),
                                    T(39.33988418719949403622L),
                                    T(42.33561646075348502966L),
                                    T(45.38013889847690802616L),
                                    T(48.47118135183522387964L),
                                    T(51.60667556776437357045L),
                                    T(54.78472939811231919009L),
                                    T(58.00360522298051993929L),
                                    T(61.26170176100200198477L),
                                    T(64.55753862700633105895L),
                                    T(67.88974313718153498289L),
                                    T(71.25703896716800901007L),
                                    T(74.65823634883016438549L),
                                    T(78.09222355331531063142L),
                                    T(81.5579594561150371785L)};
};

template <typename T>
constexpr T logarithm_factorial_table<T>::value[33];

/**
 * @brief 预计算的斐波那契数列
 */
NEFORCE_INLINE17 constexpr uint64_t fibonacci_table[] = {
        0,          1,          1,          2,          3,         5,         8,         13,        21,
        34,         55,         89,         144,        233,       377,       610,       987,       1597,
        2584,       4181,       6765,       10946,      17711,     28657,     46368,     75025,     121393,
        196418,     317811,     514229,     832040,     1346269,   2178309,   3524578,   5702887,   9227465,
        14930352,   24157817,   39088169,   63245986,   102334155, 165580141, 267914296, 433494437, 701408733,
        1134903170, 1836311903, 2971215073, 4807526976, 7778742049};

/**
 * @brief FNV64 哈希算法的偏移基础值
 */
NEFORCE_INLINE17 constexpr size_t fnv_offset_basis64 = 14695981039346656037ULL;

/**
 * @brief FNV64 哈希算法的质数乘数
 */
NEFORCE_INLINE17 constexpr size_t fnv_prime64 = 1099511628211ULL;

/**
 * @brief FNV32 哈希算法的偏移基础值
 */
NEFORCE_INLINE17 constexpr size_t fnv_offset_basis32 = 2166136261U;

/**
 * @brief FNV32 哈希算法的质数乘数
 */
NEFORCE_INLINE17 constexpr size_t fnv_prime32 = 16777619U;

/** @} */ // MathConstants

NEFORCE_END_CONSTANTS__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_NUMERICS_CONSTANTS_HPP__
