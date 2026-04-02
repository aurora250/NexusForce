#ifndef NEFORCE_CORE_NUMERIC_NUMERIC_TRAITS_HPP__
#define NEFORCE_CORE_NUMERIC_NUMERIC_TRAITS_HPP__

/**
 * @file numeric_traits.hpp
 * @brief 数值特征
 *
 * 此文件提供了数值类型的数值范围、精度、特殊值等特性信息。
 */

#include "NeForce/core/typeinfo/types.hpp"
#ifdef NEFORCE_PLATFORM_WINDOWS
#    ifdef max
#        undef max
#    endif
#    ifdef min
#        undef min
#    endif
#endif
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup NumericTraits 数值特征
 * @brief 数值类型特性的主模板和特化
 * @{
 */

/**
 * @enum FLOAT_DENORM_TYPE
 * @brief 浮点数次正规化状态
 *
 * 描述浮点数类型是否支持次正规化（denormalized）值。
 */
enum class FLOAT_DENORM_TYPE {
    INDETERMINATE = -1, ///< 状态不确定
    ABSENT,             ///< 不支持次正规化
    PRESENT             ///< 支持次正规化
};

/**
 * @enum FLOAT_ROUND_TYPE
 * @brief 浮点数舍入模式
 *
 * 描述浮点数类型的舍入方式。
 */
enum class FLOAT_ROUND_TYPE {
    INDETERMINATE = -1, ///< 舍入方式不确定
    TOWARD_ZERO,        ///< 向零舍入（截断）
    TO_NEAREST,         ///< 向最近值舍入（四舍五入）
    TOWARD_INFINITY,    ///< 向正无穷舍入（向上取整）
    TOWARD_NEG_INFINITY ///< 向负无穷舍入（向下取整）
};

/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @struct numeric_base
 * @brief 数值特征的基类
 *
 * 提供数值类型的默认特性值，大多数特性初始化为false或0。
 */
struct numeric_base {
    static constexpr auto has_denorm = FLOAT_DENORM_TYPE::ABSENT;      ///< 是否支持次正规化值
    static constexpr auto round_style = FLOAT_ROUND_TYPE::TOWARD_ZERO; ///< 舍入方式
    static constexpr bool has_denorm_loss = false;                     ///< 精度损失时是否可能产生次正规化值
    static constexpr bool has_infinity = false;                        ///< 是否有无穷大表示
    static constexpr bool has_quiet_nan = false;                       ///< 是否有安静NaN表示
    static constexpr bool has_signaling_nan = false;                   ///< 是否有信号NaN表示
    static constexpr bool is_bounded = false;                          ///< 值的集合是否有界
    static constexpr bool is_exact = false;                            ///< 表示是否精确
    static constexpr bool is_iec559 = false;                           ///< 是否符合IEC 559 / IEEE 754 标准
    static constexpr bool is_integer = false;                          ///< 是否为整数类型
    static constexpr bool is_modulo = false;                           ///< 是否为模运算类型
    static constexpr bool is_signed = false;                           ///< 是否为有符号类型
    static constexpr bool is_specialized = false;                      ///< 是否为特化版本
    static constexpr bool tinyness_before = false;                     ///< 是否在舍入前检测下溢
    static constexpr bool traps = false;                               ///< 是否捕获算术异常
    static constexpr int digits = 0;                                   ///< 基数位数
    static constexpr int digits10 = 0;                                 ///< 十进制位数
    static constexpr int max_digits10 = 0;                             ///< 保证精度的最大十进制位数
    static constexpr int max_exponent = 0;                             ///< 最大指数
    static constexpr int max_exponent10 = 0;                           ///< 最大十进制指数
    static constexpr int min_exponent = 0;                             ///< 最小指数
    static constexpr int min_exponent10 = 0;                           ///< 最小十进制指数
    static constexpr int radix = 0;                                    ///< 基数
};

/**
 * @struct numeric_int_base
 * @brief 整数类型的数值特征基类
 * @extends numeric_base
 *
 * 继承自numeric_base，为整数类型设置适当的默认值。
 */
struct numeric_int_base : numeric_base {
    static constexpr bool is_bounded = true;     ///< 整数类型是有界的
    static constexpr bool is_exact = true;       ///< 整数类型是精确的
    static constexpr bool is_integer = true;     ///< 是整数类型
    static constexpr bool is_specialized = true; ///< 是特化版本
    static constexpr int radix = 2;              ///< 整数类型的基数为2
#ifdef NEFORCE_COMPILER_GNUC
    static constexpr bool traps = true; ///< GCC编译器通常捕获算术异常
#endif
};

/**
 * @struct numeric_float_base
 * @brief 浮点数类型的数值特征基类
 * @extends numeric_base
 *
 * numeric_base，为浮点数类型设置适当的默认值。
 */
struct numeric_float_base : numeric_base {
    static constexpr auto has_denorm = FLOAT_DENORM_TYPE::PRESENT;    ///< 浮点数支持次正规化
    static constexpr auto round_style = FLOAT_ROUND_TYPE::TO_NEAREST; ///< 浮点数通常向最近值舍入
    static constexpr bool has_infinity = true;                        ///< 浮点数有无穷大表示
    static constexpr bool has_quiet_nan = true;                       ///< 浮点数有安静nan表示
    static constexpr bool has_signaling_nan = true;                   ///< 浮点数有信号nan表示
    static constexpr bool is_bounded = true;                          ///< 浮点数类型是有界的
    static constexpr bool is_iec559 = true;                           ///< 浮点数符合IEC 559标准
    static constexpr bool is_signed = true;                           ///< 浮点数是有符号的
    static constexpr bool is_specialized = true;                      ///< 是特化版本
    static constexpr int radix = 2;                                   ///< 浮点数的基数为2
};

NEFORCE_END_INNER__
/// @endcond

/**
 * @class numeric_traits
 * @brief 数值类型极限特性主模板
 * @tparam T 数值类型
 * @tparam Dummy SFINAE参数
 * @extends inner::numeric_base
 *
 * 主模板提供默认实现，返回类型T的默认值。
 * 特定类型的特化将提供该类型的具体数值特性。
 */
template <typename T, typename Dummy = void> class numeric_traits : public inner::numeric_base {
public:
    /**
     * @brief 获取类型的最小值
     * @return 类型的最小值
     * @note 对于浮点数为最小正值，对于整数为最小值
     */
    NEFORCE_NODISCARD static constexpr T min() noexcept { return T(); }
    /**
     * @brief 获取类型的最大值
     * @return 类型的最大值
     */
    NEFORCE_NODISCARD static constexpr T max() noexcept { return T(); }

    /**
     * @brief 获取类型的最低值
     * @return 类型的最低值
     * @note 对于浮点数为负无穷方向，对于整数与min相同
     */
    NEFORCE_NODISCARD static constexpr T lowest() noexcept { return T(); }
    /**
     * @brief 获取机器精度
     * @return 机器精度
     *
     * 类型可表示的1与大于1的最小值之差
     */
    NEFORCE_NODISCARD static constexpr T epsilon() noexcept { return T(); }
    /**
     * @brief 获取最大舍入误差
     * @return 最大舍入误差
     */
    NEFORCE_NODISCARD static constexpr T round_error() noexcept { return T(); }
    /**
     * @brief 获取最小的次正规化正值
     * @return 最小的次正规化正值
     */
    NEFORCE_NODISCARD static constexpr T denorm_min() noexcept { return T(); }

    /**
     * @brief 获取正无穷大表示
     * @return 正无穷大
     */
    NEFORCE_NODISCARD static constexpr T infinity() noexcept { return T(); }
    /**
     * @brief 获取安静nan表示
     * @return 安静nan
     *
     * 安静nan在大多数算术操作中不会触发浮点异常，具体特点如下：
     *   - 参与算术运算时，结果通常仍然是安静nan。
     *   - 传播到后续计算中，不会立即中断程序。
     * 其用于表示“无效但可继续运行”的结果
     */
    NEFORCE_NODISCARD static constexpr T quiet_nan() noexcept { return T(); }
    /**
     * @brief 获取信号nan表示
     * @return 信号nan
     *
     * 信号nan在大多数算术操作中会触发浮点异常，如SIGFPE或浮点无效操作异常。
     * 一旦参与运算，如果硬件/系统启用了浮点异常捕获，可能触发陷阱。
     * 其用于调试和诊断，可以捕获未初始化的浮点数使用。
     *
     * @note 主流编译器默认禁用浮点异常，因此可能不会立即崩溃，而是转换为quiet_nan。
     */
    NEFORCE_NODISCARD static constexpr T signaling_nan() noexcept { return T(); }
};

/**
 * @brief numeric_limits的const特化版本
 * @tparam T 数值类型
 */
template <typename T> class numeric_traits<const T> : public numeric_traits<T> {};

/**
 * @brief numeric_limits的volatile特化版本
 * @tparam T 数值类型
 */
template <typename T> class numeric_traits<volatile T> : public numeric_traits<T> {};

/**
 * @brief numeric_limits的const volatile特化版本
 * @tparam T 数值类型
 */
template <typename T> class numeric_traits<const volatile T> : public numeric_traits<T> {};


/**
 * @brief bool类型的数值特征特化
 */
template <> class numeric_traits<bool> : public inner::numeric_int_base {
public:
    NEFORCE_NODISCARD static constexpr bool min() noexcept { return false; }
    NEFORCE_NODISCARD static constexpr bool max() noexcept { return true; }

    NEFORCE_NODISCARD static constexpr bool lowest() noexcept { return min(); }
    NEFORCE_NODISCARD static constexpr bool epsilon() noexcept { return false; }
    NEFORCE_NODISCARD static constexpr bool round_error() noexcept { return false; }
    NEFORCE_NODISCARD static constexpr bool denorm_min() noexcept { return false; }

    NEFORCE_NODISCARD static constexpr bool infinity() noexcept { return false; }
    NEFORCE_NODISCARD static constexpr bool quiet_nan() noexcept { return false; }
    NEFORCE_NODISCARD static constexpr bool signaling_nan() noexcept { return false; }

    static constexpr int digits = 1;
};

/**
 * @brief int8_t类型的数值特征特化
 */
template <> class numeric_traits<int8_t> : public inner::numeric_int_base {
public:
    NEFORCE_NODISCARD static constexpr int8_t min() noexcept { return -128; }
    NEFORCE_NODISCARD static constexpr int8_t max() noexcept { return 127; }

    NEFORCE_NODISCARD static constexpr int8_t lowest() noexcept { return min(); }
    NEFORCE_NODISCARD static constexpr int8_t epsilon() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr int8_t round_error() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr int8_t denorm_min() noexcept { return 0; }

    NEFORCE_NODISCARD static constexpr int8_t infinity() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr int8_t quiet_nan() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr int8_t signaling_nan() noexcept { return 0; }

    static constexpr bool is_signed = true;
    static constexpr int digits = 7;
    static constexpr int digits10 = 2;
};

/**
 * @brief int16_t类型的数值特征特化
 */
template <> class numeric_traits<int16_t> : public inner::numeric_int_base {
public:
    NEFORCE_NODISCARD static constexpr int16_t min() noexcept { return -32768; }
    NEFORCE_NODISCARD static constexpr int16_t max() noexcept { return 32767; }

    NEFORCE_NODISCARD static constexpr int16_t lowest() noexcept { return min(); }
    NEFORCE_NODISCARD static constexpr int16_t epsilon() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr int16_t round_error() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr int16_t denorm_min() noexcept { return 0; }

    NEFORCE_NODISCARD static constexpr int16_t infinity() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr int16_t quiet_nan() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr int16_t signaling_nan() noexcept { return 0; }

    static constexpr bool is_signed = true;
    static constexpr int digits = 15;
    static constexpr int digits10 = 4;
};

/**
 * @brief int32_t类型的数值特征特化
 */
template <> class numeric_traits<int32_t> : public inner::numeric_int_base {
public:
    NEFORCE_NODISCARD static constexpr int32_t min() noexcept { return -2147483647 - 1; }
    NEFORCE_NODISCARD static constexpr int32_t max() noexcept { return 2147483647; }

    NEFORCE_NODISCARD static constexpr int32_t lowest() noexcept { return min(); }
    NEFORCE_NODISCARD static constexpr int32_t epsilon() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr int32_t round_error() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr int32_t denorm_min() noexcept { return 0; }

    NEFORCE_NODISCARD static constexpr int32_t infinity() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr int32_t quiet_nan() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr int32_t signaling_nan() noexcept { return 0; }

    static constexpr bool is_signed = true;
    static constexpr int digits = 31;
    static constexpr int digits10 = 9;
};

/**
 * @brief int64_t类型的数值特征特化
 */
template <> class numeric_traits<int64_t> : public inner::numeric_int_base {
public:
    NEFORCE_NODISCARD static constexpr int64_t min() noexcept { return -9223372036854775807LL - 1; }
    NEFORCE_NODISCARD static constexpr int64_t max() noexcept { return 9223372036854775807LL; }

    NEFORCE_NODISCARD static constexpr int64_t lowest() noexcept { return min(); }
    NEFORCE_NODISCARD static constexpr int64_t epsilon() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr int64_t round_error() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr int64_t denorm_min() noexcept { return 0; }

    NEFORCE_NODISCARD static constexpr int64_t infinity() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr int64_t quiet_nan() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr int64_t signaling_nan() noexcept { return 0; }

    static constexpr bool is_signed = true;
    static constexpr int digits = 63;
    static constexpr int digits10 = 18;
};

#ifdef NEFORCE_PLATFORM_LINUX64
template <> class numeric_traits<long long> : public numeric_traits<int64_t> {};
#else
template <> class numeric_traits<long> : public numeric_traits<int32_t> {};
#endif

/**
 * @brief uint8_t类型的数值特征特化
 */
template <> class numeric_traits<uint8_t> : public inner::numeric_int_base {
public:
    NEFORCE_NODISCARD static constexpr uint8_t min() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr uint8_t max() noexcept { return 0xffU; }

    NEFORCE_NODISCARD static constexpr uint8_t lowest() noexcept { return min(); }
    NEFORCE_NODISCARD static constexpr uint8_t epsilon() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr uint8_t round_error() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr uint8_t denorm_min() noexcept { return 0; }

    NEFORCE_NODISCARD static constexpr uint8_t infinity() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr uint8_t quiet_nan() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr uint8_t signaling_nan() noexcept { return 0; }

    static constexpr bool is_modulo = true;
    static constexpr int digits = 8;
    static constexpr int digits10 = 2;
};

/**
 * @brief uint16_t类型的数值特征特化
 */
template <> class numeric_traits<uint16_t> : public inner::numeric_int_base {
public:
    NEFORCE_NODISCARD static constexpr uint16_t min() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr uint16_t max() noexcept { return 0xffffU; }

    NEFORCE_NODISCARD static constexpr uint16_t lowest() noexcept { return min(); }
    NEFORCE_NODISCARD static constexpr uint16_t epsilon() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr uint16_t round_error() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr uint16_t denorm_min() noexcept { return 0; }

    NEFORCE_NODISCARD static constexpr uint16_t infinity() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr uint16_t quiet_nan() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr uint16_t signaling_nan() noexcept { return 0; }

    static constexpr bool is_modulo = true;
    static constexpr int digits = 16;
    static constexpr int digits10 = 4;
};

/**
 * @brief uint32_t类型的数值特征特化
 */
template <> class numeric_traits<uint32_t> : public inner::numeric_int_base {
public:
    NEFORCE_NODISCARD static constexpr uint32_t min() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr uint32_t max() noexcept { return 0xffffffffU; }

    NEFORCE_NODISCARD static constexpr uint32_t lowest() noexcept { return min(); }
    NEFORCE_NODISCARD static constexpr uint32_t epsilon() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr uint32_t round_error() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr uint32_t denorm_min() noexcept { return 0; }

    NEFORCE_NODISCARD static constexpr uint32_t infinity() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr uint32_t quiet_nan() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr uint32_t signaling_nan() noexcept { return 0; }

    static constexpr bool is_modulo = true;
    static constexpr int digits = 32;
    static constexpr int digits10 = 9;
};

/**
 * @brief uint64_t类型的数值特征特化
 */
template <> class numeric_traits<uint64_t> : public inner::numeric_int_base {
public:
    NEFORCE_NODISCARD static constexpr uint64_t min() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr uint64_t max() noexcept { return 0xffffffffffffffffULL; }

    NEFORCE_NODISCARD static constexpr uint64_t lowest() noexcept { return min(); }
    NEFORCE_NODISCARD static constexpr uint64_t epsilon() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr uint64_t round_error() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr uint64_t denorm_min() noexcept { return 0; }

    NEFORCE_NODISCARD static constexpr uint64_t infinity() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr uint64_t quiet_nan() noexcept { return 0; }
    NEFORCE_NODISCARD static constexpr uint64_t signaling_nan() noexcept { return 0; }

    static constexpr bool is_modulo = true;
    static constexpr int digits = 64;
    static constexpr int digits10 = 19;
};

#ifdef NEFORCE_PLATFORM_LINUX64
template <> class numeric_traits<unsigned long long> : public numeric_traits<uint64_t> {};
#else
template <> class numeric_traits<unsigned long> : public numeric_traits<uint32_t> {};
#endif


/**
 * @brief char类型的数值特征特化
 */
template <> class numeric_traits<char> : public numeric_traits<int8_t> {};

#ifdef NEFORCE_STANDARD_20
/**
 * @brief char8_t类型的数值特征特化
 */
template <> class numeric_traits<char8_t> : public numeric_traits<uint8_t> {};
#endif

/**
 * @brief char16_t类型的数值特征特化
 */
template <> class numeric_traits<char16_t> : public numeric_traits<uint16_t> {};

/**
 * @brief char32_t类型的数值特征特化
 */
template <> class numeric_traits<char32_t> : public numeric_traits<uint32_t> {};

/**
 * @brief wchar_t类型的数值特征特化
 * @note wchar_t在Windows平台为16位，在Linux平台为32位
 */
template <> class numeric_traits<wchar_t>;

#ifdef NEFORCE_PLATFORM_WINDOWS
template <> class numeric_traits<wchar_t> : public numeric_traits<uint16_t> {};
#elif defined(NEFORCE_PLATFORM_LINUX)
template <> class numeric_traits<wchar_t> : public numeric_traits<int32_t> {};
#endif


/**
 * @brief 单精度浮点数类型的数值特征特化
 */
template <> class numeric_traits<float32_t> : public inner::numeric_float_base {
public:
    /**
     * @brief 获取最小正规范值
     * @return 最小正规范值
     */
    NEFORCE_NODISCARD static constexpr float32_t min_posi() noexcept { return 1.175494351e-38f; }
    /**
     * @brief 获取最大正规范值
     * @return 最大正规范值
     */
    NEFORCE_NODISCARD static constexpr float32_t max_posi() noexcept { return 3.402823466e+38f; }
    /**
     * @brief 获取最小负规范值
     * @return 最小负规范值
     */
    NEFORCE_NODISCARD static constexpr float32_t min_nega() noexcept { return -3.402823466e+38f; }
    /**
     * @brief 获取最大负规范值
     * @return 最大负规范值
     */
    NEFORCE_NODISCARD static constexpr float32_t max_nega() noexcept { return -1.175494351e-38f; }

    NEFORCE_NODISCARD static constexpr float32_t min() noexcept { return min_posi(); }
    NEFORCE_NODISCARD static constexpr float32_t max() noexcept { return max_posi(); }

    NEFORCE_NODISCARD static constexpr float32_t lowest() noexcept { return min_nega(); }
    NEFORCE_NODISCARD static constexpr float32_t epsilon() noexcept { return 1.192092896e-07f; }
    NEFORCE_NODISCARD static constexpr float32_t round_error() noexcept { return 0.5F; }
    NEFORCE_NODISCARD static constexpr float32_t denorm_min() noexcept { return 1.401298464e-45f; }

    NEFORCE_NODISCARD static constexpr float32_t infinity() noexcept { return __builtin_huge_valf(); }
    NEFORCE_NODISCARD static constexpr float32_t quiet_nan() noexcept {
#ifdef NEFORCE_COMPILER_GCC
        return __builtin_nanf("");
#else
        return __builtin_nan("0");
#endif
    }
    NEFORCE_NODISCARD static constexpr float32_t signaling_nan() noexcept {
#ifdef NEFORCE_COMPILER_GCC
        return __builtin_nansf("");
#else
        return __builtin_nans("1");
#endif
    }

    static constexpr int digits = 24;          ///< 尾数位数，包括隐藏位
    static constexpr int digits10 = 6;         ///< 十进制有效位数
    static constexpr int max_digits10 = 9;     ///< 保证精度的最大十进制位数
    static constexpr int max_exponent = 128;   ///< 最大指数
    static constexpr int max_exponent10 = 38;  ///< 最大十进制指数
    static constexpr int min_exponent = -125;  ///< 最小指数
    static constexpr int min_exponent10 = -37; ///< 最小十进制指数
};

/**
 * @brief 双精度浮点数类型的数值特征特化
 */
template <> class numeric_traits<float64_t> : public inner::numeric_float_base {
public:
    NEFORCE_NODISCARD static constexpr float64_t min_posi() noexcept { return 2.2250738585072014e-308; }
    NEFORCE_NODISCARD static constexpr float64_t max_posi() noexcept { return 1.7976931348623157e+308; }
    NEFORCE_NODISCARD static constexpr float64_t min_nega() noexcept { return -1.7976931348623157e+308; }
    NEFORCE_NODISCARD static constexpr float64_t max_nega() noexcept { return -2.2250738585072014e-308; }

    NEFORCE_NODISCARD static constexpr float64_t min() noexcept { return min_posi(); }
    NEFORCE_NODISCARD static constexpr float64_t max() noexcept { return max_posi(); }

    NEFORCE_NODISCARD static constexpr float64_t lowest() noexcept { return min_nega(); }
    NEFORCE_NODISCARD static constexpr float64_t epsilon() noexcept { return 2.2204460492503131e-16; }
    NEFORCE_NODISCARD static constexpr float64_t round_error() noexcept { return 0.5; }
    NEFORCE_NODISCARD static constexpr float64_t denorm_min() noexcept { return 4.9406564584124654e-324; }

    NEFORCE_NODISCARD static constexpr float64_t infinity() noexcept { return __builtin_huge_val(); }
    NEFORCE_NODISCARD static constexpr float64_t quiet_nan() noexcept {
#ifdef NEFORCE_COMPILER_GCC
        return __builtin_nan("");
#else
        return __builtin_nan("0");
#endif
    }
    NEFORCE_NODISCARD static constexpr float64_t signaling_nan() noexcept {
#ifdef NEFORCE_COMPILER_GCC
        return __builtin_nans("");
#else
        return __builtin_nans("1");
#endif
    }

    static constexpr int digits = 53;
    static constexpr int digits10 = 15;
    static constexpr int max_digits10 = 17;
    static constexpr int max_exponent = 1024;
    static constexpr int max_exponent10 = 308;
    static constexpr int min_exponent = -1021;
    static constexpr int min_exponent10 = -307;
};

#ifdef NEFORCE_COMPILER_GNUC
/**
 * @brief 扩展精度浮点数类型的数值特征特化
 * @note MSVC的long double等同于double；GNUC则使用更大的数值范围特征。
 */
template <> class numeric_traits<decimal_t> : public inner::numeric_float_base {
public:
    NEFORCE_NODISCARD static constexpr decimal_t min_posi() noexcept {
        return 3.36210314311209350626267781732175260e-4932L;
    }
    NEFORCE_NODISCARD static constexpr decimal_t max_posi() noexcept {
        return 1.18973149535723176502126385303097021e+4932L;
    }
    NEFORCE_NODISCARD static constexpr decimal_t min_nega() noexcept {
        return -1.18973149535723176502126385303097021e+4932L;
    }
    NEFORCE_NODISCARD static constexpr decimal_t max_nega() noexcept {
        return -3.36210314311209350626267781732175260e-4932L;
    }

    NEFORCE_NODISCARD static constexpr decimal_t min() noexcept { return min_posi(); }
    NEFORCE_NODISCARD static constexpr decimal_t max() noexcept { return max_posi(); }

    NEFORCE_NODISCARD static constexpr decimal_t lowest() noexcept { return min_nega(); }
    NEFORCE_NODISCARD static constexpr decimal_t epsilon() noexcept { return 1.08420217248550443401e-19L; }
    NEFORCE_NODISCARD static constexpr decimal_t round_error() noexcept { return 0.5L; }
    NEFORCE_NODISCARD static constexpr decimal_t denorm_min() noexcept { return 3.64519953188247460253e-4951L; }

    NEFORCE_NODISCARD static constexpr decimal_t infinity() noexcept { return __builtin_huge_val(); }
    NEFORCE_NODISCARD static constexpr decimal_t quiet_nan() noexcept { return __builtin_nanl(""); }
    NEFORCE_NODISCARD static constexpr decimal_t signaling_nan() noexcept { return __builtin_nansl(""); }

    static constexpr int digits = 64;
    static constexpr int digits10 = 18;
    static constexpr int max_digits10 = 21;
    static constexpr int max_exponent = 16384;
    static constexpr int max_exponent10 = 4932;
    static constexpr int min_exponent = -16381;
    static constexpr int min_exponent10 = -4931;
};
#else
template <> class numeric_traits<decimal_t> : public numeric_traits<float64_t> {};
#endif

/** @} */ // NumericTraits

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_NUMERIC_NUMERIC_TRAITS_HPP__
