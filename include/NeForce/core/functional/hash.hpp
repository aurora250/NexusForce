#ifndef NEFORCE_CORE_FUNCTIONAL_HASH_HPP__
#define NEFORCE_CORE_FUNCTIONAL_HASH_HPP__

/**
 * @file hash.hpp
 * @brief 哈希函数库
 *
 * 此文件提供了各种哈希算法的实现，包括FNV-1a、DJB2和MurmurHash等，
 * 以及基本数据类型的哈希函数特化。支持编译时哈希计算和运行时高效哈希。
 */

#include "NeForce/core/typeinfo/type_traits.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup HashPrimary 哈希主模板
 * @brief 哈希函数的主模板和基础定义
 * @{
 */

/**
 * @struct hash
 * @brief 哈希函数的主模板
 * @tparam Key 键类型
 * @tparam Dummy SFINAE参数，默认为void
 *
 * 为特定类型提供哈希函数的通用接口。需要为具体类型进行特化。
 * 默认实现要求用户为自定义类型提供特化版本。
 */
template <typename Key, typename Dummy = void>
struct hash;

/**
 * @brief 指针类型的哈希函数特化
 * @tparam T 指针指向的类型
 */
template <typename T>
struct hash<T*> {
    /**
     * @brief 指针哈希运算符
     * @param ptr 要哈希的指针
     * @return 指针地址的哈希值
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_t operator ()(const T* ptr) const noexcept {
        return static_cast<size_t>(reinterpret_cast<uintptr_t>(ptr));
    }
};

/** @} */ // HashPrimary

/**
 * @defgroup FNVHash FNV哈希算法
 * @brief Fowler-Noll-Vo非加密哈希算法实现
 * @{
 */

/// @cond
NEFORCE_BEGIN_CONSTANTS__

/**
 * @var FNV_OFFSET_BASIS
 * @brief FNV哈希算法的偏移基础值
 * @note 根据平台位数使用不同的值，文档以64位为例。
 */
NEFORCE_INLINE17 constexpr size_t FNV_OFFSET_BASIS =
#ifdef NEFORCE_ARCH_BITS_64
    14695981039346656037ULL;
#else
    2166136261U;
#endif

/**
 * @var FNV_PRIME
 * @brief FNV哈希算法的质数乘数
 * @note 根据平台位数使用不同的值，文档以64位为例。
 */
NEFORCE_INLINE17 constexpr size_t FNV_PRIME
#ifdef NEFORCE_ARCH_BITS_64
     = 1099511628211ULL;
#else
     = 16777619U;
#endif

NEFORCE_END_CONSTANTS__
/// @endcond

/**
 * @brief FNV-1a哈希算法
 * @param first 数据的起始字节指针
 * @param count 数据的字节数
 * @return 计算出的哈希值
 *
 * FNV（Fowler-Noll-Vo）是一种非加密哈希算法，具有：
 * 1. 良好的雪崩效应（avalanche effect）
 * 2. 较低的碰撞率
 * 3. 实现简单高效
 *
 * FNV_hash函数使用FNV-1a版本算法，先异或再乘法的顺序。
 */
NEFORCE_CONSTEXPR14 size_t FNV_hash(const byte_t* first, const size_t count) noexcept {
    size_t result = constants::FNV_OFFSET_BASIS;
    for (size_t i = 0; i < count; i++) {
        result ^= static_cast<size_t>(first[i]);
        result *= constants::FNV_PRIME;
    }
    return result;
}

/**
 * @brief 整数类型的FNV哈希
 * @tparam T 整数类型
 * @param value 要哈希的整数值
 * @return 整数的哈希值
 */
template <typename T>
NEFORCE_CONSTEXPR14 size_t FNV_hash_integer(const T value) noexcept {
    static_assert(is_integral<T>::value, "T must be integral");

    size_t result = constants::FNV_OFFSET_BASIS;
    for (size_t i = 0; i < sizeof(T); ++i) {
        const byte_t byte_val = static_cast<byte_t>((value >> (i * 8)) & 0xFF);
        result ^= static_cast<size_t>(byte_val);
        result *= constants::FNV_PRIME;
    }
    return result;
}

/**
 * @brief 字符串类型的FNV哈希
 * @tparam CharT 字符类型
 * @param str 字符串指针
 * @param len 字符串长度
 * @return 字符串的哈希值
 */
template <typename CharT>
NEFORCE_CONSTEXPR14 size_t FNV_hash_string(const CharT* str, const size_t len) noexcept {
    static_assert(is_character<CharT>::value, "CharT must be character types");

    size_t result = constants::FNV_OFFSET_BASIS;
    for (size_t i = 0; i < len; ++i) {
        result ^= static_cast<size_t>(static_cast<byte_t>(str[i]));
        result *= constants::FNV_PRIME;
    }
    return result;
}

/** @} */ // FNVHash


template <>
struct hash<bool> {
    NEFORCE_NODISCARD constexpr size_t operator ()(const bool x) const noexcept {
        return x ? 0x9e3779b9 : 0x7f4a7c15;
    }
};

#define __NEFORCE_BUILD_INTEGER_HASH_STRUCT(OPT) \
template <> struct hash<OPT> { \
    NEFORCE_NODISCARD constexpr size_t operator ()(const OPT x) const noexcept { \
        return x == 0.0f ? 0 : FNV_hash_integer(x); \
    } \
};

NEFORCE_MACRO_RANGE_CHARS(__NEFORCE_BUILD_INTEGER_HASH_STRUCT)
NEFORCE_MACRO_RANGE_INT(__NEFORCE_BUILD_INTEGER_HASH_STRUCT)
#undef __NEFORCE_BUILD_INTEGER_HASH_STRUCT

#define __NEFORCE_BUILD_FLOAT_HASH_STRUCT(OPT) \
template <> \
struct hash<OPT> { \
private: \
    union __float_converter { OPT f; uint64_t i; }; \
    \
public: \
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR14 size_t operator ()(const OPT x) const noexcept { \
        if (x == 0.0f) return 0; \
        __float_converter converter{}; \
        converter.f = x; \
        return FNV_hash_integer(converter.i); \
    } \
};

NEFORCE_MACRO_RANGE_FLOAT(__NEFORCE_BUILD_FLOAT_HASH_STRUCT)
#undef __NEFORCE_BUILD_FLOAT_HASH_STRUCT


/**
 * @defgroup DJB2Hash DJB2哈希算法
 * @brief Daniel J. Bernstein的DJB2哈希算法
 * @{
 */

/**
 * @brief DJB2哈希算法
 * @param str 字符串指针
 * @param len 字符串长度
 * @return 计算出的哈希值
 *
 * DJB2是一种非加密哈希算法，具有以下特点：
 * 1. 实现简单
 * 2. 速度快
 * 3. 分布均匀
 *
 * 但在某些特殊情况下仍可能出现哈希冲突。
 */
NEFORCE_CONSTEXPR14 size_t DJB2_hash(const char* str, const size_t len) noexcept {
    size_t hash = 5381;
    for (size_t i = 0; i < len; ++i) {
        hash = (hash << 5) + hash + static_cast<byte_t>(str[i]);
    }
    return hash;
}

/** @} */ // DJB2Hash

/**
 * @defgroup MurmurHash MurmurHash算法
 * @brief Austin Appleby的MurmurHash非加密哈希算法
 * @{
 */

#ifdef NEFORCE_ARCH_BITS_64

/**
 * @struct murmur_hash
 * @brief MurmurHash_x64的128位哈希结果容器
 *
 * MurmurHash_x64算法产生128位哈希值，拆分为两个64位整数存储。
 */
struct murmur_hash {
    size_t low = 0;  ///< 哈希值的低64位
    size_t high = 0; ///< 哈希值的高64位

    murmur_hash() noexcept = default;
    ~murmur_hash() noexcept = default;

    /**
     * @brief 构造函数
     * @param l 哈希值的低64位
     * @param h 哈希值的高64位
     */
    murmur_hash(const size_t l, const size_t h) noexcept
    : low(l), high(h) {}
};

/**
 * @brief MurmurHash3_x64_128算法
 * @param key 要哈希的数据
 * @param len 数据长度
 * @param seed 哈希种子
 * @return 128位哈希结果
 *
 * MurmurHash是一种非加密哈希算法，具有：
 * 1. 速度快
 * 2. 碰撞率低
 * 3. 可自定义种子
 *
 * MurmurHash_x64是MurmurHash3的64位版本，产生128位哈希值。
 *
 * @note 仅64位系统可用
 */
murmur_hash NEFORCE_API MurmurHash_x64(const void* key, size_t len, uint32_t seed) noexcept;
#endif

/**
 * @brief MurmurHash3_x86_32算法
 * @param key 要哈希的数据
 * @param len 数据长度
 * @param seed 哈希种子
 * @return 32位哈希结果
 *
 * MurmurHash3的32位版本，产生32位哈希值。
 * 适用于32位系统或需要32位哈希的场景。
 */
uint32_t NEFORCE_API MurmurHash_x32(const void* key, size_t len, uint32_t seed) noexcept;

/** @} */ // MurmurHash

/**
 * @defgroup HashTraits 哈希特性检查
 * @brief 检查类型是否支持哈希操作
 * @{
 */

/**
 * @struct is_nothrow_hashable
 * @brief 判断类型是否可无异常哈希
 * @tparam Key 要检查的类型
 * @tparam Dummy SFINAE参数，默认为void
 */
template <typename Key, typename Dummy = void>
struct is_nothrow_hashable : false_type {};

/// @cond
template <typename Key>
struct is_nothrow_hashable<Key, void_t<decltype(_NEFORCE hash<Key>{}(_NEFORCE declval<const Key&>()))>>
    : bool_constant<noexcept(_NEFORCE hash<Key>{}(_NEFORCE declval<const Key&>()))> {};
/// @endcond

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
 *
 * 检查类型是否可以作为哈希函数使用，即是否可调用并返回可转换为size_t的类型。
 */
template <typename Func, typename Arg, typename Dummy = void>
struct is_hash : false_type {};

/// @cond
template <typename Func, typename Arg>
struct is_hash<Func, Arg, enable_if_t<
    is_convertible<decltype(_NEFORCE declval<Func>()(_NEFORCE declval<Arg>())), size_t>::value
>> : true_type {};
/// @endcond

#ifdef NEFORCE_STANDARD_14
/**
 * @var is_hash_v
 * @brief is_hash的便捷变量模板
 */
template <typename Func, typename Arg>
constexpr bool is_hash_v = is_hash<Func, Arg>::value;
#endif

/** @} */ // HashTraits

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FUNCTIONAL_HASH_HPP__
