#ifndef NEFORCE_CORE_HASH_HASH_HPP__
#define NEFORCE_CORE_HASH_HASH_HPP__

/**
 * @file hash.hpp
 * @brief 通用哈希设施
 *
 * 提供 hash 函数对象主模板、基本类型特化、哈希组合工具以及可哈希性检测 traits。
 */

#include "NeForce/core/typeinfo/type_traits.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Hash 哈希
 * @brief 单向摘要与哈希设施。
 *
 * @section hash_requirements 哈希函数要求
 *
 * 根据 ISO/IEC 14882:2020 §16.4.4，哈希函数应满足：
 * - 可调用类型：接受 Key 类型参数，返回 size_t
 * - 相等性：若 k1 == k2，则 hash(k1) == hash(k2)
 * - 不抛出异常（推荐）
 */

/**
 * @defgroup HashGeneric 通用哈希设施
 * @ingroup Hash
 * @brief hash 函数对象、哈希组合工具与可哈希性 traits。
 * @{
 */

/**
 * @struct hash
 * @brief 哈希函数的主模板
 * @tparam Key 键类型
 * @tparam Dummy SFINAE参数
 */
template <typename Key, typename Dummy = void>
struct hash;

/**
 * @brief 指针类型的哈希特化
 * @tparam T 指针指向的类型
 */
template <typename T>
struct hash<T*> {
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_t operator()(const T* ptr) const noexcept {
        return static_cast<size_t>(reinterpret_cast<uintptr_t>(ptr));
    }
};


/**
 * @brief 快速位混合哈希函数
 * @param x 输入值
 * @return 混合后的哈希值
 */
NEFORCE_CONSTEXPR14 size_t low_level_hash(size_t x) noexcept {
#ifdef NEFORCE_ARCH_BITS_64
    x += 0x9e3779b97f4a7c15ULL;
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return x;
#else
    x += 0x9e3779b9U;
    x ^= x >> 16;
    x *= 0x85ebca6bU;
    x ^= x >> 13;
    x *= 0xc2b2ae35U;
    x ^= x >> 16;
    return x;
#endif
}


/// @cond

template <>
struct hash<bool> {
    NEFORCE_NODISCARD constexpr size_t operator()(const bool x) const noexcept { return x ? 0x9e3779b9 : 0x7f4a7c15; }
};

#define __NEFORCE_BUILD_INTEGER_HASH_STRUCT(OPT)                                                                     \
    template <>                                                                                                      \
    struct hash<OPT> {                                                                                               \
        NEFORCE_NODISCARD constexpr size_t operator()(const OPT x) const noexcept { return static_cast<size_t>(x); } \
    };

NEFORCE_MACRO_RANGE_CHARS(__NEFORCE_BUILD_INTEGER_HASH_STRUCT)
NEFORCE_MACRO_RANGE_INT(__NEFORCE_BUILD_INTEGER_HASH_STRUCT)
#undef __NEFORCE_BUILD_INTEGER_HASH_STRUCT

#define __NEFORCE_BUILD_FLOAT_HASH_STRUCT(OPT)                                                \
    template <>                                                                               \
    struct hash<OPT> {                                                                        \
    private:                                                                                  \
        union __float_converter {                                                             \
            OPT f;                                                                            \
            uint64_t i;                                                                       \
        };                                                                                    \
                                                                                              \
    public:                                                                                   \
        NEFORCE_NODISCARD NEFORCE_CONSTEXPR14 size_t operator()(const OPT x) const noexcept { \
            if (x == 0.0f)                                                                    \
                return 0;                                                                     \
            __float_converter converter{};                                                    \
            converter.f = x;                                                                  \
            return low_level_hash(static_cast<size_t>(converter.i));                          \
        }                                                                                     \
    };

NEFORCE_MACRO_RANGE_FLOAT(__NEFORCE_BUILD_FLOAT_HASH_STRUCT)
#undef __NEFORCE_BUILD_FLOAT_HASH_STRUCT

/// @endcond


/**
 * @brief 混合两个哈希值
 * @tparam T 值类型
 * @param seed 种子哈希值（累加）
 * @param value 要混合的值
 */
template <typename T>
NEFORCE_CONSTEXPR14 void hash_combine(size_t& seed, const T& value) noexcept {
    seed ^= hash<T>()(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

/**
 * @brief 混合多个哈希值
 * @tparam Types 值类型包
 * @param values 要混合的多个值
 * @return 组合后的哈希值
 */
template <typename... Types>
NEFORCE_CONSTEXPR14 size_t hash_combine_all(const Types&... values) noexcept {
    size_t seed = 0;
    int dummy[] = {(_NEFORCE hash_combine(seed, values), 0)...};
    (void) dummy;
    return seed;
}

/**
 * @brief 枚举类型的哈希特化
 * @tparam T 枚举类型
 */
template <typename T>
struct hash<T, enable_if_t<is_enum_v<T>>> {
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR14 size_t operator()(const T e) const {
        using UT = underlying_type_t<T>;
        return hash<UT>()(static_cast<UT>(e));
    }
};

/**
 * @struct is_nothrow_hashable
 * @brief 判断类型是否可无异常哈希
 * @tparam Key 要检查的类型
 * @tparam Dummy SFINAE参数
 */
template <typename Key, typename Dummy = void>
struct is_nothrow_hashable : false_type {};

template <typename Key>
struct is_nothrow_hashable<Key, void_t<decltype(_NEFORCE hash<Key>{}(_NEFORCE declval<const Key&>()))>>
: bool_constant<noexcept(_NEFORCE hash<Key>{}(_NEFORCE declval<const Key&>()))> {};

#ifdef NEFORCE_STANDARD_14
/**
 * @var is_nothrow_hashable_v
 * @brief is_nothrow_hashable的便捷变量模板
 */
template <typename Key>
NEFORCE_INLINE17 constexpr bool is_nothrow_hashable_v = is_nothrow_hashable<Key>::value;
#endif


/**
 * @struct is_hash
 * @brief 判断类型是否为有效的哈希函数
 * @tparam Func 函数类型
 * @tparam Arg 参数类型
 * @tparam Dummy SFINAE参数，默认为void
 */
template <typename Func, typename Arg, typename Dummy = void>
struct is_hash : false_type {};

template <typename Func, typename Arg>
struct is_hash<Func, Arg,
               enable_if_t<is_convertible<decltype(_NEFORCE declval<Func>()(_NEFORCE declval<Arg>())), size_t>::value>>
: true_type {};

#ifdef NEFORCE_STANDARD_14
/**
 * @var is_hash_v
 * @brief is_hash的便捷变量模板
 */
template <typename Func, typename Arg>
constexpr bool is_hash_v = is_hash<Func, Arg>::value;
#endif

/** @} */ // HashGeneric

/**
 * @defgroup HashFast 非加密哈希
 * @ingroup Hash
 * @brief 面向哈希表、布隆过滤器与数据校验的快速非加密哈希算法。
 */

/**
 * @defgroup HashCrypto 密码学哈希
 * @ingroup Hash
 * @brief 具备密码学安全目标的单向摘要算法。
 */

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_HASH_HASH_HPP__
