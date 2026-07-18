#ifndef NEFORCE_CORE_FUNCTIONAL_HASH_HPP__
#define NEFORCE_CORE_FUNCTIONAL_HASH_HPP__

/**
 * @file hash.hpp
 * @brief 哈希函数库
 *
 * 此文件提供了各种哈希算法的实现，包括FNV-1a、DJB2和MurmurHash等，
 * 以及基本数据类型的哈希函数特化。支持编译时哈希计算和运行时高效哈希。
 *
 * 对基本类型的哈希特化将不展示在文档内，有需要可自行查看本文件内容。
 */

#include "NeForce/core/memory/bit.hpp"
#include "NeForce/core/memory/endian.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup HashPrimary 哈希算法
 * @brief 哈希模板和哈希算法实现
 *
 * @section standards 遵循的国际标准与参考规范
 * 本实现中的哈希算法参考以下标准规范与学术文献：
 *
 * **哈希算法规范参考：**
 * - **IETF RFC 6234**：US 安全哈希算法 (SHA) 及基于 SHA 的 HMAC 和 HKDF（加密哈希参考）
 *   https://www.rfc-editor.org/rfc/rfc6234.html
 *
 * **非加密哈希算法文献：**
 * - **FNV-1a 哈希算法**：Fowler–Noll–Vo 哈希函数规范
 *   https://datatracker.ietf.org/doc/html/draft-eastlake-fnv-17
 * - **MurmurHash3 算法**：Austin Appleby 设计的非加密哈希函数
 *   https://github.com/aappleby/smhasher/wiki/MurmurHash3
 * - **DJB2 哈希算法**：Daniel J. Bernstein 设计的字符串哈希函数
 *   http://www.cse.yorku.ca/~oz/hash.html
 * - **xxHash 算法**：Yann Collet 设计的极速非加密哈希函数族（XXH32/XXH64/XXH3）
 *   https://github.com/Cyan4973/xxHash
 * - **wyhash 算法**：Wang Yi 设计的现代非加密哈希函数
 *   https://github.com/wangyi-fudan/wyhash
 * - **CityHash 算法**：Google 设计的非加密哈希函数族
 *   https://github.com/google/cityhash
 *
 * **哈希函数安全标准：**
 * - **NIST SP 800-185**：SHA-3 派生函数 — cSHAKE、KMAC、TupleHash、ParallelHash
 *   https://csrc.nist.gov/pubs/sp/800/185/final
 * - **NIST SP 800-107 Rev. 1**：使用已批准哈希算法的应用推荐
 *   https://csrc.nist.gov/pubs/sp/800/107/r1/final
 *
 * @section algorithm_comparison 哈希算法对比
 *
 * | 算法           | 输出位数          | 特点                                 | 适用场景                       |
 * |----------------|------------------|-------------------------------------|-------------------------------|
 * | FNV-1a         | 32/64 位         | 实现简单、雪崩效应好、碰撞率低          | 哈希表、编译时哈希              |
 * | DJB2           | 32/64 位         | 极简实现、速度快                       | 简单字符串哈希                 |
 * | MurmurHash3    | 32/128 位        | 速度快、分布均匀、可自定义种子          | 高性能哈希表、Bloom Filter      |
 * | XXH32          | 32 位            | 极速、适合短数据                      | 32 位哈希表、嵌入式系统          |
 * | XXH64          | 64 位            | 极速、适合中等长度数据                 | 64 位哈希表、数据校验           |
 * | XXH3           | 64 位            | XXH 系列最快、大数据量优化、SIMD 加速   | 大数据哈希、流式处理            |
 * | wyhash         | 64 位            | SMHasher 全通、现代设计、128 位乘法    | 通用哈希表、键值存储            |
 * | CityHash64     | 64 位            | 字符串哈希优化、多级分治               | 字符串哈希、缓存键              |
 * | low_level_hash | size_t           | 单值位混合、零值安全                   | 基本类型 hash 特化的内部基础    |
 * | hash_combine   | size_t           | 组合多个哈希值                        | 自定义结构体哈希                |
 *
 * @section hash_requirements 哈希函数要求
 * 根据 ISO/IEC 14882:2020 §16.4.4，C++ 标准库哈希函数应满足：
 * - 可调用类型：接受 Key 类型参数，返回 size_t
 * - 相等性：若 k1 == k2，则 hash(k1) == hash(k2)
 * - 不抛出异常（推荐）：哈希计算不抛出异常
 *
 * @section security_note 安全注意事项
 * @warning **重要安全提示**：
 *          - 本文件中的所有算法均为**非加密哈希算法**
 *          - 这些算法不应用于安全敏感场景，如密码存储、数字签名、消息认证码
 *          - 非加密哈希算法容易受到哈希碰撞攻击和长度扩展攻击
 *          - 对于安全场景，请使用密码学安全的哈希函数（如 SHA-256、SHA-3、BLAKE2）
 *
 * @see https://datatracker.ietf.org/doc/html/draft-eastlake-fnv-17
 * @see https://github.com/aappleby/smhasher
 * @see https://github.com/Cyan4973/xxHash
 * @see https://github.com/google/cityhash
 * @see https://en.wikipedia.org/wiki/Hash_function
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
 * @brief 指针类型的哈希特化
 * @tparam T 指针指向的类型
 */
template <typename T>
struct hash<T*> {
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_t operator()(const T* ptr) const noexcept {
        return static_cast<size_t>(reinterpret_cast<uintptr_t>(ptr));
    }
};

/** @} */ // HashPrimary

NEFORCE_BEGIN_CONSTANTS__

/**
 * @addtogroup HashPrimary 哈希模板
 * @{
 */

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

/** @} */ // HashPrimary

NEFORCE_END_CONSTANTS__

/**
 * @addtogroup HashPrimary 哈希模板
 * @{
 */

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
        const auto byte_val = static_cast<byte_t>((value >> (i * 8)) & 0xFF);
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

/**
 * @brief 快速位混合哈希函数
 * @param x 输入值
 * @return 混合后的哈希值
 *
 * 将一个 size_t 值通过位混合操作扩散到整个输出空间。
 *
 * @note 64 位平台使用 MurmurHash3 fmix64 风格混合器，32 位平台使用 fmix32 风格混合器
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

/** @} */ // HashPrimary

/// @cond

template <>
struct hash<bool> {
    NEFORCE_NODISCARD constexpr size_t operator()(const bool x) const noexcept { return x ? 0x9e3779b9 : 0x7f4a7c15; }
};

#define __NEFORCE_BUILD_INTEGER_HASH_STRUCT(OPT)                                    \
    template <>                                                                     \
    struct hash<OPT> {                                                              \
        NEFORCE_NODISCARD constexpr size_t operator()(const OPT x) const noexcept { \
            return low_level_hash(static_cast<size_t>(x));                          \
        }                                                                           \
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
 * @param seed 种子哈希值（累加器）
 * @param value 要混合的值
 *
 * 用于组合多个字段的哈希值。
 *
 * @note 该函数调用 hash<T>() 获取值的哈希，然后与种子混合
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
 *
 * 将多个值依次 hash_combine 到一个哈希值中。
 */
template <typename... Types>
NEFORCE_CONSTEXPR14 size_t hash_combine_all(const Types&... values) noexcept {
    size_t seed = 0;
    int dummy[] = {(_NEFORCE hash_combine(seed, values), 0)...};
    ignore = dummy;
    return seed;
}

/**
 * @addtogroup HashPrimary 哈希模板
 * @{
 */

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
    murmur_hash(const size_t l, const size_t h) noexcept :
    low(l),
    high(h) {}
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
murmur_hash NEFORCE_API murmur_hash64(const void* key, size_t len, uint32_t seed) noexcept;
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
uint32_t NEFORCE_API murmur_hash32(const void* key, size_t len, uint32_t seed) noexcept;


#ifdef NEFORCE_COMPILER_MSVC
// use switch penetrate
#    pragma warning(push)
#    pragma warning(disable : 26819)
#endif


/**
 * @brief XXH32 哈希算法
 * @param input 输入数据指针
 * @param len 数据长度
 * @param seed 种子值，默认为 0
 * @return 32 位哈希值
 *
 * xxHash 是一种极速非加密哈希算法，XXH32 是其 32 位版本。
 * 适用于哈希表、数据校验等非安全场景。
 */
NEFORCE_CONSTEXPR14 uint32_t XXH32(const void* input, size_t len, uint32_t seed = 0) noexcept {
    constexpr uint32_t PRIME32_1 = 0x9E3779B1U;
    constexpr uint32_t PRIME32_2 = 0x85EBCA77U;
    constexpr uint32_t PRIME32_3 = 0xC2B2AE3DU;
    constexpr uint32_t PRIME32_4 = 0x27D4EB2FU;
    constexpr uint32_t PRIME32_5 = 0x165667B1U;

    const auto* p = static_cast<const byte_t*>(input);
    uint32_t hash = 0;

    if (len >= 16) {
        uint32_t acc1 = seed + PRIME32_1 + PRIME32_2;
        uint32_t acc2 = seed + PRIME32_2;
        uint32_t acc3 = seed + 0;
        uint32_t acc4 = seed - PRIME32_1;

        const byte_t* limit = p + (len & ~static_cast<size_t>(15));
        for (; p < limit; p += 16) {
            acc1 = rotate_l32(acc1 + endian::read_le32(p + 0) * PRIME32_2, 13) * PRIME32_1;
            acc2 = rotate_l32(acc2 + endian::read_le32(p + 4) * PRIME32_2, 13) * PRIME32_1;
            acc3 = rotate_l32(acc3 + endian::read_le32(p + 8) * PRIME32_2, 13) * PRIME32_1;
            acc4 = rotate_l32(acc4 + endian::read_le32(p + 12) * PRIME32_2, 13) * PRIME32_1;
        }

        hash = rotate_l32(acc1, 1) + rotate_l32(acc2, 7) + rotate_l32(acc3, 12) + rotate_l32(acc4, 18);
    } else {
        hash = seed + PRIME32_5;
    }

    hash += static_cast<uint32_t>(len);

    const byte_t* end = static_cast<const byte_t*>(input) + len;
    for (; p + 4 <= end; p += 4) {
        hash += endian::read_le32(p) * PRIME32_3;
        hash = rotate_l32(hash, 17) * PRIME32_4;
    }
    switch (len & 3) {
        case 3:
            hash += static_cast<uint32_t>(p[2]) * PRIME32_5;
            hash = rotate_l32(hash, 11) * PRIME32_1;
        case 2:
            hash += static_cast<uint32_t>(p[1]) * PRIME32_5;
            hash = rotate_l32(hash, 11) * PRIME32_1;
        case 1:
            hash += static_cast<uint32_t>(p[0]) * PRIME32_5;
            hash = rotate_l32(hash, 11) * PRIME32_1;
        default:
            break;
    }

    hash ^= hash >> 15;
    hash *= PRIME32_2;
    hash ^= hash >> 13;
    hash *= PRIME32_3;
    hash ^= hash >> 16;

    return hash;
}

#ifdef NEFORCE_ARCH_BITS_64

/**
 * @brief XXH64 哈希算法
 * @param input 输入数据指针
 * @param len 数据长度
 * @param seed 种子值，默认为 0
 * @return 64 位哈希值
 *
 * xxHash 的 64 位版本，适合中等到大型数据的哈希。
 * 在 64 位平台上速度极快。
 */
NEFORCE_CONSTEXPR14 uint64_t XXH64(const void* input, size_t len, uint64_t seed = 0) noexcept {
    constexpr uint64_t PRIME64_1 = 0x9E3779B185EBCA87ULL;
    constexpr uint64_t PRIME64_2 = 0xC2B2AE3D27D4EB4FULL;
    constexpr uint64_t PRIME64_3 = 0x165667B19E3779F9ULL;
    constexpr uint64_t PRIME64_4 = 0x85EBCA77C2B2AE63ULL;
    constexpr uint64_t PRIME64_5 = 0x27D4EB2F165667C5ULL;

    const auto* p = static_cast<const byte_t*>(input);
    uint64_t hash = 0;

    if (len >= 32) {
        uint64_t acc1 = seed + PRIME64_1 + PRIME64_2;
        uint64_t acc2 = seed + PRIME64_2;
        uint64_t acc3 = seed + 0;
        uint64_t acc4 = seed - PRIME64_1;

        const byte_t* limit = p + (len & ~static_cast<size_t>(31));
        for (; p < limit; p += 32) {
            acc1 = rotate_l64(acc1 + endian::read_le64(p + 0) * PRIME64_2, 31) * PRIME64_1;
            acc2 = rotate_l64(acc2 + endian::read_le64(p + 8) * PRIME64_2, 31) * PRIME64_1;
            acc3 = rotate_l64(acc3 + endian::read_le64(p + 16) * PRIME64_2, 31) * PRIME64_1;
            acc4 = rotate_l64(acc4 + endian::read_le64(p + 24) * PRIME64_2, 31) * PRIME64_1;
        }

        hash = rotate_l64(acc1, 1) + rotate_l64(acc2, 7) + rotate_l64(acc3, 12) + rotate_l64(acc4, 18);
    } else {
        hash = seed + PRIME64_5;
    }

    hash += static_cast<uint64_t>(len);

    const byte_t* end = static_cast<const byte_t*>(input) + len;
    for (; p + 8 <= end; p += 8) {
        uint64_t k1 = endian::read_le64(p);
        k1 *= PRIME64_2;
        k1 = rotate_l64(k1, 31);
        k1 *= PRIME64_1;
        hash ^= k1;
        hash = rotate_l64(hash, 27) * PRIME64_1 + PRIME64_4;
    }
    for (; p + 4 <= end; p += 4) {
        hash ^= static_cast<uint64_t>(endian::read_le32(p)) * PRIME64_1;
        hash = rotate_l64(hash, 23) * PRIME64_2 + PRIME64_3;
    }
    switch (len & 3) {
        case 3:
            hash ^= static_cast<uint64_t>(p[2]) * PRIME64_5;
            hash = rotate_l64(hash, 11) * PRIME64_1;
        case 2:
            hash ^= static_cast<uint64_t>(p[1]) * PRIME64_5;
            hash = rotate_l64(hash, 11) * PRIME64_1;
        case 1:
            hash ^= static_cast<uint64_t>(p[0]) * PRIME64_5;
            hash = rotate_l64(hash, 11) * PRIME64_1;
        default:
            break;
    }

    hash ^= hash >> 33;
    hash *= PRIME64_2;
    hash ^= hash >> 29;
    hash *= PRIME64_3;
    hash ^= hash >> 32;

    return hash;
}


#    ifdef NEFORCE_COMPILER_MSVC
#        pragma warning(pop)
#    endif


/**
 * @brief wyhash 哈希算法
 * @param key 输入数据指针
 * @param len 数据长度
 * @param seed 种子值
 * @return 64 位哈希值
 *
 * wyhash（Wang Yi's hash）是一种现代非加密哈希算法，
 * 在 SMHasher 测试套件中表现优异，适合哈希表等场景。
 */
uint64_t NEFORCE_API wyhash(const void* key, size_t len, uint64_t seed) noexcept;

#endif


/**
 * @brief CityHash64 哈希算法
 * @param key 输入数据指针
 * @param len 数据长度
 * @return 64 位哈希值
 *
 * Google CityHash 是一种非加密哈希算法，专为字符串哈希优化。
 * 适用于哈希表、字符串匹配等场景。
 */
size_t NEFORCE_API city_hash64(const void* key, size_t len) noexcept;


/**
 * @brief XXH3_64bits 哈希算法
 * @param data 输入数据指针
 * @param len 数据长度
 * @return 64 位哈希值
 *
 * XXH3 是 xxHash 系列的最新版本，在 64 位平台上速度极快。
 * 使用内部 192 字节秘密表进行数据混合，适合大数据量哈希。
 */
uint64_t NEFORCE_API XXH3_64(const void* data, size_t len) noexcept;


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
struct is_hash<Func, Arg,
               enable_if_t<is_convertible<decltype(_NEFORCE declval<Func>()(_NEFORCE declval<Arg>())), size_t>::value>>
: true_type {};
/// @endcond

#ifdef NEFORCE_STANDARD_14
/**
 * @var is_hash_v
 * @brief is_hash的便捷变量模板
 */
template <typename Func, typename Arg>
constexpr bool is_hash_v = is_hash<Func, Arg>::value;
#endif

/** @} */ // HashPrimary

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FUNCTIONAL_HASH_HPP__
