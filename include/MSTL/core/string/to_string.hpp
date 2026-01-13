#ifndef MSTL_CORE_STRING_TO_STRING_HPP__
#define MSTL_CORE_STRING_TO_STRING_HPP__
#include "../interface/istringify.hpp"
#include "../interface/icollector.hpp"
#include "character.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename T, typename P = package_t<T>, enable_if_t<is_packaged_v<T> && is_base_of_v<istringify<P>, P>, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const T& value) {
    return to_string(package_t<T>(value));
}

MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(nullptr_t) {
    return {"nullptr"};
}

template <typename T, enable_if_t<is_pointer_v<T> && !is_cstring_v<T>, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const T& x) {
    return _MSTL address_string(x);
}


MSTL_BEGIN_INNER__

template <typename Collector>
MSTL_NODISCARD MSTL_CONSTEXPR20 string collector_to_string(const Collector& c) {
    if (_MSTL empty(c)) return {"[]"};
    string result;
    result += "[ ";
    for (auto iter = _MSTL cbegin(c); iter != _MSTL cend(c); ++iter) {
        if (iter != _MSTL cbegin(c)) result += ", ";
        result += to_string(*iter);
    }
    result += " ]";
    return result;
}

MSTL_END_INNER__

template <typename T, enable_if_t<is_base_of_v<icollector<T>, T>, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const T& x) {
    return _INNER collector_to_string(x);
}


template <typename T, enable_if_t<is_unbounded_array_v<T>, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const T&) {
    return {"[]"};
}

template <typename T, enable_if_t<is_bounded_array_v<T> && !is_cstring_v<T>, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const T& x) {
    return _INNER collector_to_string(x);
}

MSTL_BEGIN_INNER__

template <typename T, T N>
constexpr const char* __get_enum_name_raw() {
#ifdef MSTL_COMPILER_MSVC__
    return __FUNCSIG__;
#else
    return __PRETTY_FUNCTION__;
#endif
}

template <ssize_t Beg, ssize_t End, typename F, enable_if_t<Beg == End, int> = 0>
MSTL_CONSTEXPR20 void static_for(const F&) {}

template <ssize_t Beg, ssize_t End, typename F, enable_if_t<Beg != End, int> = 0>
MSTL_CONSTEXPR20 void static_for(const F& func) {
    func.template call<Beg>();
    _INNER static_for<Beg + 1, End>(func);
}

template <typename T>
struct __enum_name_functor {
    using UT = underlying_type_t<T>;

    UT n;
    string &s;

    __enum_name_functor(UT n, string &s) : n(n), s(s) {}

    template <UT I>
    MSTL_CONSTEXPR20 void call() const {
        if (n == I) {
            s = __get_enum_name_raw<T, static_cast<T>(I)>();
        }
    }
};

MSTL_END_INNER__

template <typename T, T Beg, T End>
MSTL_CONSTEXPR20 string enum_name(T n) {
    static_assert(is_enum_v<T>, "T must be an enumeration");
    string s;
    using UT = underlying_type_t<T>;
    _INNER static_for<static_cast<UT>(Beg), static_cast<UT>(End) + 1>(
        _INNER __enum_name_functor<T>(static_cast<UT>(n), s));
    if (s.empty()) {
        return "";
    }
#ifdef MSTL_COMPILER_MSVC__
    size_t pos = s.find(',');
    pos += 1;
    size_t pos2 = s.find('>', pos);
#else
    size_t pos = s.find("N = ");
    pos += 4;
    size_t pos2 = s.find_first_of(";]", pos);
#endif
    s = s.substr(pos, pos2 - pos);
    const size_t pos3 = s.rfind("::");
    if (pos3 != s.npos) {
        s = s.substr(pos3 + 2);
    }
    return s;
}

template <typename T>
MSTL_CONSTEXPR20 string enum_name(T n) {
    return _MSTL enum_name<T, static_cast<T>(0), static_cast<T>(256)>(n);
}

template <typename T, enable_if_t<is_enum_v<T>, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const T& x) {
    return _MSTL enum_name(x);
}

template <typename T, enable_if_t<is_base_of_v<_MSTL exception, T>, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const T& obj) {
    return string(obj.type()) + "(" + obj.what() + ")";
}

template <typename IfEmpty, typename T>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const compressed_pair<IfEmpty, T, true>& obj) {
    return to_string(obj.value);
}

template <typename IfEmpty, typename T>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const compressed_pair<IfEmpty, T, false>& obj) {
    return "{ " + to_string(obj.value) + ", " + to_string(obj.no_compressed) + " }";
}

template <typename T1, typename T2>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const pair<T1, T2>& obj) {
    return "{ " + to_string(obj.first) + ", " + to_string(obj.second) + " }";
}


MSTL_BEGIN_INNER__

template <typename Tuple, size_t I, enable_if_t<I == tuple_size_v<Tuple> - 1, int> = 0>
MSTL_CONSTEXPR20 void __to_string_tuple_elements(const Tuple& t, string& result) {
    result += to_string(_MSTL get<I>(t));
}

template <typename Tuple, size_t I, enable_if_t<I < tuple_size_v<Tuple> - 1, int> = 0>
MSTL_CONSTEXPR20 void __to_string_tuple_elements(const Tuple& t, string& result) {
    result += to_string(_MSTL get<I>(t)) + ", ";
    _INNER __to_string_tuple_elements<Tuple, I + 1>(t, result);
}

template <typename... UArgs, enable_if_t<sizeof...(UArgs) == 0, int> = 0>
MSTL_CONSTEXPR20 string __to_string_tuple_dispatch(const tuple<UArgs...>&) {
    return {"()"};
}

template <typename... UArgs, enable_if_t<sizeof...(UArgs) != 0, int> = 0>
MSTL_CONSTEXPR20 string __to_string_tuple_dispatch(const tuple<UArgs...>& t) {
    string result;
    result += "( ";
    _INNER __to_string_tuple_elements<decltype(t), 0>(t, result);
    result += " )";
    return result;
}

MSTL_END_INNER__

template <typename... Args>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const tuple<Args...>& t) {
    return _INNER __to_string_tuple_dispatch(t);
}

MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const bstring& x) {
    return string(x.begin(), x.end());
}

MSTL_NODISCARD MSTL_CONSTEXPR20 bstring to_bstring(const string& x) {
    return bstring(x.begin(), x.end());
}

MSTL_NODISCARD MSTL_CONSTEXPR20 bstring to_bstring(const string_view x) {
    return bstring(x.begin(), x.end());
}


#ifndef MSTL_STANDARD_17__

MSTL_BEGIN_INNER__
template <typename T>
string to_string_concat(T&& t) {
    return to_string(_MSTL forward<T>(t));
}
template <typename First, typename... Rest>
string to_string_concat(First&& first, Rest&&... rest) {
    return to_string(_MSTL forward<First>(first)) + to_string_concat(_MSTL forward<Rest>(rest)...);
}
MSTL_END_INNER__

template <typename... Args, enable_if_t<(sizeof...(Args) > 1), int> = 0>
MSTL_NODISCARD string to_string(Args&&... args) {
    return _INNER to_string_concat(_MSTL forward<Args>(args)...);
}

#else
template <typename... Args, enable_if_t<(sizeof...(Args) > 1), int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(Args&&... args) {
    return (to_string(_MSTL forward<Args>(args)) + ...);
}
#endif


MSTL_BEGIN_INNER__

#ifndef MSTL_DATA_BUS_WIDTH_64__
template <typename CharT, typename UT, enable_if_t<(sizeof(UT) > 4), int> = 0>
constexpr void __uint_to_buff_aux(CharT* riter, UT& ux) noexcept {
    while (ux > static_cast<UT>(0xFFFFFFFFU)) {
        auto chunk = static_cast<uint32_t>(ux % static_cast<UT>(1000000000));
        ux /= static_cast<UT>(1000000000);
        for (int idx = 0; idx != 9; ++idx) {
            *--riter = static_cast<CharT>('0' + chunk % 10);
            chunk /= 10;
        }
    }
}
template <typename CharT, typename UT, enable_if_t<sizeof(UT) <= 4, int> = 0>
constexpr void __uint_to_buff_aux(CharT*, UT&) noexcept {}
#endif // MSTL_DATA_BUS_WIDTH_64__

template <typename CharT, typename UT, enable_if_t<is_unsigned<UT>::value, int> = 0>
MSTL_NODISCARD constexpr CharT* __uint_to_buff(CharT* riter, UT ux) noexcept {
#ifdef MSTL_DATA_BUS_WIDTH_64__
    UT holder = ux;
#else
    _INNER __uint_to_buff_aux(riter, ux);
    auto holder = static_cast<uint32_t>(ux);
#endif
    do {
        *--riter = static_cast<CharT>('0' + holder % static_cast<UT>(10));
        holder /= static_cast<UT>(10);
    } while (static_cast<UT>(holder) != static_cast<UT>(0));
    return riter;
}

template <typename CharT, typename T, enable_if_t<is_integral<T>::value, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 basic_string<CharT> __int_to_string(const T x) {
    CharT buffer[21];
    CharT* const buffer_end = buffer + 21;
    CharT* rnext = buffer_end;
    using UT = make_unsigned_t<T>;
    const auto unsigned_x = static_cast<UT>(x);
    if (x < 0) {
        rnext = _INNER __uint_to_buff(rnext, static_cast<UT>(0 - unsigned_x));
        *--rnext = '-';
    } else {
        rnext = _INNER __uint_to_buff(rnext, unsigned_x);
    }
    const size_t count = buffer_end - rnext;
    return basic_string<CharT>(rnext, count);
}

template <typename CharT, typename T, enable_if_t<conjunction<is_integral<T>, is_unsigned<T>>::value, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 basic_string<CharT> __uint_to_string(T x) {
    CharT buffer[21];
    CharT* const buffer_end = buffer + 21;
    CharT* const rnext = _INNER __uint_to_buff(buffer_end, x);
    const size_t count = buffer_end - rnext;
    return basic_string<CharT>(rnext, count);
}

MSTL_CONSTEXPR20 string __uint_to_string_base(uint64_t value, const int base, const bool uppercase) {
    if (value == 0) {
        return "0";
    }
    string result;
    constexpr auto digits_lower = "0123456789abcdef";
    constexpr auto digits_upper = "0123456789ABCDEF";
    const auto digits = uppercase ? digits_upper : digits_lower;
    while (value > 0) {
        const uint64_t remainder = value % base;
        value /= base;
        result.push_back(digits[remainder]);
    }
    result.reverse();
    return result;
}

template <typename T, enable_if_t<
    disjunction_v<conjunction<is_standard_integral<T>, is_signed<T>>, is_same<T, signed char>>, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string __int_to_string_dispatch(const T x) {
    return _INNER __int_to_string<char>(x);
}
template <typename T, enable_if_t<
    disjunction_v<conjunction<is_standard_integral<T>, is_unsigned<T>>, is_same<T, unsigned char>>, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string __int_to_string_dispatch(const T x) {
    return _INNER __uint_to_string<char>(x);
}


template <typename CharT, typename T, enable_if_t<is_floating_point<T>::value, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 basic_string<CharT> __float_to_string_with_precision(
    T x, int precision = 6, const bool force_scientific = false, const bool force_fixed = false) {
    if (x == numeric_limits<T>::quiet_nan()) return basic_string<CharT>{"nan"};
    constexpr T inf = numeric_limits<T>::infinity();
    if (x == inf || x == -inf) {
        return (x < 0) ? basic_string<CharT>{"-inf"} : basic_string<CharT>{"inf"};
    }

    basic_string<CharT> result;

    if (x < 0) {
        result += '-';
        x = -x;
    }

    if (precision < 0) precision = 0;

    bool use_scientific = false;
    if (force_scientific) {
        use_scientific = true;
    } else if (force_fixed) {
        use_scientific = false;
    } else {
        use_scientific = (x >= 1e6 || (x > 0 && x < 1e-4));
    }

    if (use_scientific) {
        int exponent = 0;

        if (x == 0) {
            exponent = 0;
        } else {
            if (x >= 1) {
                while (x >= 10) {
                    x /= 10;
                    ++exponent;
                }
            } else {
                while (x < 1) {
                    x *= 10;
                    --exponent;
                }
            }
        }

        auto integer_part = static_cast<uint64_t>(x);
        T fractional_part = x - integer_part;

        result += _INNER __uint_to_string<CharT>(integer_part);

        if (precision > 0) {
            result += '.';
            for (int i = 0; i < precision; ++i) {
                fractional_part *= 10;
                auto digit = static_cast<int>(fractional_part);
                result += static_cast<CharT>('0' + digit);
                fractional_part -= digit;
            }
        }

        result += 'e';
        if (exponent >= 0) {
            result += '+';
        } else {
            result += '-';
            exponent = -exponent;
        }

        if (exponent < 10) {
            result += '0';
        }
        result += _INNER __uint_to_string<CharT>(static_cast<uint64_t>(exponent));

    } else {
        auto integer_part = static_cast<uint64_t>(x);
        T fractional_part = x - integer_part;

        result += _INNER __uint_to_string<CharT>(integer_part);

        if (precision > 0) {
            result += '.';
            for (int i = 0; i < precision; ++i) {
                fractional_part *= 10;
                auto digit = static_cast<int>(fractional_part);
                result += static_cast<CharT>('0' + digit);
                fractional_part -= digit;
            }
        }
    }

    return result;
}

template <typename CharT, typename T, enable_if_t<is_floating_point<T>::value, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 basic_string<CharT> __float_to_string(T x) {
    return _INNER __float_to_string_with_precision<CharT>(x, 6, false, false);
}

MSTL_END_INNER__

template <typename T, enable_if_t<is_floating_point<T>::value, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string_with_precision(T x, int precision, bool scientific = false) {
    return _INNER __float_to_string_with_precision<char>(x, precision, scientific, scientific);
}

template <typename T, enable_if_t<is_floating_point<T>::value, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string_general(T x, int precision = 6) {
    return _INNER __float_to_string_with_precision<char>(x, precision, false, false);
}

template <typename T, enable_if_t<is_floating_point<T>::value, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string_fixed(T x, int precision = 6) {
    return _INNER __float_to_string_with_precision<char>(x, precision, false, true);
}

template <typename T, enable_if_t<is_floating_point<T>::value, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string_scientific(T x, int precision = 6) {
    return _INNER __float_to_string_with_precision<char>(x, precision, true, false);
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_STRING_TO_STRING_HPP__
