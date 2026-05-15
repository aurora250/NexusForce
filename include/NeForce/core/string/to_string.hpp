#ifndef NEFORCE_CORE_STRING_TO_STRING_HPP__
#define NEFORCE_CORE_STRING_TO_STRING_HPP__

/**
 * @file to_string.hpp
 * @brief 类型到字符串的转换函数
 *
 * 此文件提供了将各种类型转换为字符串的通用函数。
 * 支持基本类型、容器、元组、枚举、异常等多种类型的字符串表示。
 */

#include "NeForce/core/algorithm/type_erase.hpp"
#include "NeForce/core/interface/icollector.hpp"
#include "NeForce/core/interface/istringify.hpp"
#include "NeForce/core/numeric/math.hpp"
#include "NeForce/core/numeric/numeric_types.hpp"
#include "NeForce/core/string/utf.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup StringConverts 字符串转换
 * @brief 字符串与数据类型之间的转换功能
 * @{
 */

/**
 * @brief 将可包装类型转换为字符串
 * @tparam T 可包装类型
 * @param value 要转换的值
 * @return 字符串表示
 *
 * 通过包装类型进行字符串转换，要求包装类型实现了istringify接口。
 */
template <typename T, typename P = package_t<T>,
          enable_if_t<is_packaged_v<T> && is_base_of_v<istringify<P>, P>, int> = 0>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string to_string(const T& value) {
    return to_string(package_t<T>(value));
}

/**
 * @brief 将空指针转换为字符串
 * @param np 空指针
 * @return 字符串"nullptr"
 */
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string to_string(nullptr_t np) { return {"nullptr"}; }

/**
 * @brief 将指针转换为字符串
 * @tparam T 指针类型
 * @param ptr 指针
 * @return 指针的地址字符串
 *
 * 将指针转换为地址字符串，排除C风格字符串。
 */
template <typename T, enable_if_t<is_pointer_v<T> && !is_cstring_v<T>, int> = 0>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string to_string(const T& ptr) {
    return _NEFORCE address_string(ptr);
}

/// @cond
NEFORCE_BEGIN_INNER__

template <typename Collector>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string collector_to_string(const Collector& c) {
    if (_NEFORCE empty(c)) {
        return {"[]"};
    }

    string result;
    result += "[ ";

    auto begin = _NEFORCE cbegin(c);
    for (auto iter = begin; iter != _NEFORCE cend(c); ++iter) {
        if (iter != begin) {
            result += ", ";
        }
        result += to_string(*iter);
    }

    result += " ]";
    return result;
}

NEFORCE_END_INNER__
/// @endcond

/**
 * @brief 将容器转换为字符串
 * @tparam T 容器类型
 * @param c 容器
 * @return 字符串表示
 */
template <typename T, enable_if_t<is_base_of_v<icollector<T>, T>, int> = 0>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string to_string(const T& c) {
    return inner::collector_to_string(c);
}

#ifdef NEFORCE_STANDARD_20
template <typename T, enable_if_t<is_base_of_v<ranges::view_base<T>, T>, int> = 0>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string to_string(const T& value) {
    string result;
    if (value.begin() == value.end()) {
        result = "[]";
    } else {
        result += "[ ";
        for (auto iter = value.begin(); iter != value.end(); ++iter) {
            if (iter != value.begin()) {
                result += ", ";
            }
            result += to_string(*iter);
        }
        result += " ]";
    }
    return result;
}
#endif

/**
 * @brief 将无界数组转换为字符串
 * @tparam T 数组类型
 * @return 空数组字符串"[]"
 */
template <typename T, enable_if_t<is_unbounded_array_v<T>, int> = 0>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string to_string(const T& /*unused*/) {
    return {"[]"};
}

/**
 * @brief 将有界数组转换为字符串
 * @tparam T 数组类型
 * @param arr 数组
 * @return 字符串表示
 */
template <typename T, enable_if_t<is_bounded_array_v<T> && !is_cstring_v<T>, int> = 0>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string to_string(const T& arr) {
    return inner::collector_to_string(arr);
}

/**
 * @brief 将压缩对（已压缩版本）转换为字符串
 * @tparam IfEmpty 空基类类型
 * @tparam T 值类型
 * @param obj 压缩对
 * @return 值的字符串表示
 */
template <typename IfEmpty, typename T>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string to_string(const compressed_pair<IfEmpty, T, true>& obj) {
    return to_string(obj.value);
}

/**
 * @brief 将压缩对（未压缩版本）转换为字符串
 * @tparam IfEmpty 空基类类型
 * @tparam T 值类型
 * @param obj 压缩对
 * @return 格式为"{ value, empty }"的字符串
 */
template <typename IfEmpty, typename T>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string to_string(const compressed_pair<IfEmpty, T, false>& obj) {
    return "{ " + to_string(obj.value) + ", " + to_string(obj.no_compressed) + " }";
}

/**
 * @brief 将对转换为字符串
 * @tparam T1 第一个类型
 * @tparam T2 第二个类型
 * @param obj 对
 * @return 格式为"{ first, second }"的字符串
 */
template <typename T1, typename T2>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string to_string(const pair<T1, T2>& obj) {
    return "{ " + to_string(obj.first) + ", " + to_string(obj.second) + " }";
}

/// @cond
NEFORCE_BEGIN_INNER__

template <typename Tuple, size_t I, enable_if_t<I == tuple_size_v<Tuple> - 1, int> = 0>
NEFORCE_CONSTEXPR20 void __to_string_tuple_elements(const Tuple& t, string& result) {
    result += to_string(_NEFORCE get<I>(t));
}

template <typename Tuple, size_t I,
          enable_if_t<I<tuple_size_v<Tuple> - 1, int> = 0> NEFORCE_CONSTEXPR20 void __to_string_tuple_elements(
                  const Tuple& t, string& result) {
    result += to_string(_NEFORCE get<I>(t)) + ", ";
    inner::__to_string_tuple_elements<Tuple, I + 1>(t, result);
}

template <typename... UArgs, enable_if_t<sizeof...(UArgs) == 0, int> = 0>
NEFORCE_CONSTEXPR20 string __to_string_tuple_dispatch(const tuple<UArgs...>& /*unused*/) {
    return {"()"};
}

template <typename... UArgs, enable_if_t<sizeof...(UArgs) != 0, int> = 0>
NEFORCE_CONSTEXPR20 string __to_string_tuple_dispatch(const tuple<UArgs...>& t) {
    string result;
    result += "( ";
    inner::__to_string_tuple_elements<decltype(t), 0>(t, result);
    result += " )";
    return result;
}

NEFORCE_END_INNER__
/// @endcond

/**
 * @brief 将元组转换为字符串
 * @tparam Args 元组元素类型
 * @param tup 元组
 * @return 字符串表示
 */
template <typename... Args>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string to_string(const tuple<Args...>& tup) {
    return inner::__to_string_tuple_dispatch(tup);
}


#ifndef NEFORCE_STANDARD_17
/// @cond
NEFORCE_BEGIN_INNER__
template <typename T>
string to_string_concat(T&& t) {
    return to_string(_NEFORCE forward<T>(t));
}
template <typename First, typename... Rest>
string to_string_concat(First&& first, Rest&&... rest) {
    return to_string(_NEFORCE forward<First>(first)) + to_string_concat(_NEFORCE forward<Rest>(rest)...);
}
NEFORCE_END_INNER__
/// @endcond
#endif

/**
 * @brief 将多个参数转换为字符串并连接
 * @tparam Args 参数类型
 * @param args 参数
 * @return 连接后的字符串
 */
template <typename... Args, enable_if_t<(sizeof...(Args) > 1), int> = 0>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string to_string(Args&&... args) {
#ifdef NEFORCE_STANDARD_17
    return (to_string(_NEFORCE forward<Args>(args)) + ...);
#else
    return inner::to_string_concat(_NEFORCE forward<Args>(args)...);
#endif
}

/// @cond
NEFORCE_BEGIN_INNER__

#ifdef NEFORCE_ARCH_BITS_32

template <typename CharT, typename UT>
constexpr enable_if_t<(sizeof(UT) > 4)> __uint_to_buff_aux(CharT*& riter, UT& ux) {
    while (ux > static_cast<UT>(0xFFFFFFFFU)) {
        auto chunk = static_cast<uint32_t>(ux % static_cast<UT>(1000000000));
        ux /= static_cast<UT>(1000000000);
        for (int idx = 0; idx != 9; ++idx) {
            *--riter = static_cast<CharT>('0' + chunk % 10);
            chunk /= 10;
        }
    }
}

template <typename CharT, typename UT>
constexpr enable_if_t<(sizeof(UT) <= 4)> __uint_to_buff_aux(CharT*, UT&) noexcept {}

#endif

/**
 * @brief 将无符号整数转换为字符缓冲区
 * @tparam CharT 字符类型
 * @tparam UT 无符号整数类型
 * @param riter 反向迭代器
 * @param ux 要转换的值
 * @return 指向转换后字符串起始位置的迭代器
 */
template <typename CharT, typename UT>
NEFORCE_NODISCARD constexpr CharT* __uint_to_buff(CharT* riter, UT ux) {
    static_assert(is_unsigned_v<UT>, "UT must be a unsigned integer type");

#ifdef NEFORCE_ARCH_BITS_64
    UT holder = ux;
    do {
        *--riter = static_cast<CharT>(static_cast<UT>('0') + holder % static_cast<UT>(10));
        holder /= static_cast<UT>(10);
    } while (holder != static_cast<UT>(0));
#else
    inner::__uint_to_buff_aux(riter, ux);
    auto holder = static_cast<uint32_t>(ux);
    do {
        *--riter = static_cast<CharT>('0' + holder % 10);
        holder /= 10;
    } while (holder != 0);
#endif
    return riter;
}

/**
 * @brief 将有符号整数转换为字符串
 * @tparam CharT 字符类型
 * @tparam T 整数类型
 * @param x 要转换的值
 * @return 字符串表示
 */
template <typename CharT, typename T>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 basic_string<CharT> __int_to_string(const T x) {
    static_assert(is_integral_v<T>, "T must be a integral type");

    CharT buffer[40]; // digits10 + sign + '\0'
    CharT* const buffer_end = buffer + 40;
    CharT* rnext = buffer_end;
    using UT = make_unsigned_t<T>;
    const auto unsigned_x = static_cast<UT>(x);
    if (x < 0) {
        rnext = inner::__uint_to_buff(rnext, static_cast<UT>(static_cast<UT>(0) - unsigned_x));
        *--rnext = '-';
    } else {
        rnext = inner::__uint_to_buff(rnext, unsigned_x);
    }
    const size_t count = buffer_end - rnext;
    return basic_string<CharT>(rnext, count);
}

/**
 * @brief 将无符号整数转换为字符串
 * @tparam CharT 字符类型
 * @tparam T 无符号整数类型
 * @param x 要转换的值
 * @return 字符串表示
 */
template <typename CharT, typename T>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 basic_string<CharT> __uint_to_string(T x) {
    static_assert(is_unsigned_v<T>, "T must be a integral type");

    CharT buffer[40]; // digits10 + sign + '\0'
    CharT* const buffer_end = buffer + 40;
    CharT* const rnext = inner::__uint_to_buff(buffer_end, x);
    const size_t count = buffer_end - rnext;
    return basic_string<CharT>(rnext, count);
}

template <typename T, enable_if_t<is_signed_v<T>, int> = 0>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string __int_to_string_dispatch(T x) {
    return inner::__int_to_string<char>(x);
}
template <typename T, enable_if_t<is_unsigned_v<T>, int> = 0>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string __int_to_string_dispatch(T x) {
    return inner::__uint_to_string<char>(x);
}

/**
 * @brief 将浮点数转换为字符串
 * @tparam CharT 字符类型
 * @tparam T 浮点数类型
 * @param x 要转换的值
 * @param precision 精度
 * @param force_scientific 强制科学计数法
 * @param force_fixed 强制固定小数表示
 * @return 字符串表示
 */
template <typename CharT, typename T>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 basic_string<CharT>
__float_to_string_with_precision(T x, int precision = 6, const bool force_scientific = false,
                                 const bool force_fixed = false) {
    static_assert(is_floating_point_v<T>, "T must be a floating point type");

    if (_NEFORCE is_nan(x)) {
        return basic_string<CharT>{"nan"};
    }

    constexpr T inf = numeric_traits<T>::infinity();
    if (x == inf) {
        return basic_string<CharT>{"inf"};
    }
    if (x == -inf) {
        return basic_string<CharT>{"-inf"};
    }

    basic_string<CharT> result;

    const bool is_negative = (x < 0);
    if (is_negative) {
        result += '-';
        x = -x;
    }

    precision = _NEFORCE max(precision, 0);

    bool use_scientific = false;
    int exponent = 0;

    if (force_scientific) {
        use_scientific = true;
    } else if (force_fixed) {
        use_scientific = false;
    } else {
        use_scientific = (x != 0) && (x >= 1e6 || x < 1e-4);
    }

    if (use_scientific && x != 0) {
        const double log_val = logarithm_10(static_cast<double>(x));
        exponent = static_cast<int>(log_val >= 0 ? log_val : log_val - 1.0);

        if (exponent >= 0) {
            for (int i = 0; i < exponent; ++i) {
                x /= static_cast<T>(10);
            }
        } else {
            for (int i = 0; i < -exponent; ++i) {
                x *= static_cast<T>(10);
            }
        }

        if (x >= static_cast<T>(10)) {
            x /= static_cast<T>(10);
            ++exponent;
        } else if (x < static_cast<T>(1) && x > static_cast<T>(0)) {
            x *= static_cast<T>(10);
            --exponent;
        }
    }

    auto integer_part = static_cast<uint64_t>(x);
    T fractional_part = x - static_cast<T>(integer_part);

    uint64_t frac_int = 0;
    uint64_t frac_scale = 1;

    if (precision > 0) {
        const int safe_precision = (precision > 18) ? 18 : precision;
        for (int i = 0; i < safe_precision; ++i) {
            frac_scale *= 10;
        }

        frac_int = static_cast<uint64_t>(fractional_part * static_cast<T>(frac_scale) + static_cast<T>(0.5));

        if (frac_int >= frac_scale) {
            frac_int -= frac_scale;
            ++integer_part;

            if (use_scientific && integer_part >= 10) {
                integer_part /= 10;
                ++exponent;
            }
        }
    }

    result += inner::__uint_to_string<CharT>(integer_part);

    if (precision > 0) {
        result += static_cast<CharT>('.');
        basic_string<CharT> frac_str = inner::__uint_to_string<CharT>(frac_int);

        const int safe_precision = (precision > 18) ? 18 : precision;
        const int leading_zeros = safe_precision - static_cast<int>(frac_str.size());
        for (int i = 0; i < leading_zeros; ++i) {
            result += static_cast<CharT>('0');
        }
        result += frac_str;

        for (int i = safe_precision; i < precision; ++i) {
            result += static_cast<CharT>('0');
        }
    }

    if (use_scientific) {
        result += static_cast<CharT>('e');
        if (exponent >= 0) {
            result += static_cast<CharT>('+');
        } else {
            result += static_cast<CharT>('-');
            exponent = -exponent;
        }
        if (exponent < 10) {
            result += static_cast<CharT>('0');
        }
        result += inner::__uint_to_string<CharT>(static_cast<uint64_t>(exponent));
    }

    return result;
}

/**
 * @brief 将浮点数转换为字符串
 * @tparam CharT 字符类型
 * @tparam T 浮点数类型
 * @param x 要转换的值
 * @return 字符串表示
 */
template <typename CharT, typename T, enable_if_t<is_floating_point<T>::value, int> = 0>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 basic_string<CharT> __float_to_string(T x) {
    return inner::__float_to_string_with_precision<CharT>(x, 6, false, false);
}

NEFORCE_END_INNER__
/// @endcond

/**
 * @brief 将浮点数转换为字符串（带精度控制）
 * @tparam T 浮点数类型
 * @param x 要转换的值
 * @param precision 精度
 * @param scientific 是否使用科学计数法
 * @return 字符串表示
 */
template <typename T, enable_if_t<is_floating_point<T>::value, int> = 0>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string to_string_with_precision(T x, int precision, bool scientific = false) {
    return inner::__float_to_string_with_precision<char>(x, precision, scientific, !scientific);
}

/**
 * @brief 将浮点数转换为字符串（通用格式）
 * @tparam T 浮点数类型
 * @param x 要转换的值
 * @param precision 精度
 * @return 字符串表示
 */
template <typename T, enable_if_t<is_floating_point<T>::value, int> = 0>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string to_string_general(T x, int precision = 6) {
    return inner::__float_to_string_with_precision<char>(x, precision, false, false);
}

/**
 * @brief 将浮点数转换为字符串（固定小数格式）
 * @tparam T 浮点数类型
 * @param x 要转换的值
 * @param precision 精度
 * @return 字符串表示
 */
template <typename T, enable_if_t<is_floating_point<T>::value, int> = 0>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string to_string_fixed(T x, int precision = 6) {
    return inner::__float_to_string_with_precision<char>(x, precision, false, true);
}

/**
 * @brief 将浮点数转换为字符串（科学计数法格式）
 * @tparam T 浮点数类型
 * @param x 要转换的值
 * @param precision 精度
 * @return 字符串表示
 */
template <typename T, enable_if_t<is_floating_point<T>::value, int> = 0>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string to_string_scientific(T x, int precision = 6) {
    return inner::__float_to_string_with_precision<char>(x, precision, true, false);
}

/** @} */ // StringConverts

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_STRING_TO_STRING_HPP__
