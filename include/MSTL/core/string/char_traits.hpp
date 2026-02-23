#ifndef MSTL_CORE_STRING_CHAR_TRAITS_HPP__
#define MSTL_CORE_STRING_CHAR_TRAITS_HPP__

/**
 * @file char_traits.hpp
 * @brief MSTL字符特征模板
 *
 * 此文件提供了字符特征模板，用于定义字符类型的操作。
 * 字符特征类封装了字符的比较、复制、查找等基本操作，是现代字符串操作的基础。
 * 提供了针对不同字符类型的特化版本。
 */

#include "MSTL/core/algorithm/compare.hpp"
#include "MSTL/core/functional/hash.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup CharTraits 字符特征
 * @brief 字符类型的特征定义和操作
 * @{
 */

/**
 * @struct base_char_traits
 * @brief 基础字符特征模板
 * @tparam CharT 字符类型
 * @tparam IntT 整数类型
 *
 * 提供字符类型的基本操作接口，包括复制、移动、比较、查找等。
 * 作为具体字符特征特化的基类。
 */
template <typename CharT, typename IntT>
struct base_char_traits {
    using char_type = CharT;  ///< 字符类型
    using int_type  = IntT;   ///< 整数类型

    static_assert(
        sizeof(int_type) >= sizeof(char_type),
        "int_type must be able to represent all char_type values plus EOF"
    );

    /**
     * @brief 复制字符序列
     * @param dest 目标地址
     * @param srcs 源地址
     * @param count 要复制的字符数
     * @return dest指针
     */
    static constexpr char_type* copy(
        char_type* dest, const char_type* srcs, const size_t count) noexcept {
        _MSTL memory_copy(dest, srcs, count * sizeof(char_type));
        return dest;
    }

    /**
     * @brief 移动字符序列
     * @param dest 目标地址
     * @param srcs 源地址
     * @param count 要移动的字符数
     * @return dest指针
     */
    static constexpr char_type* move(
        char_type* dest, const char_type* srcs, const size_t count) noexcept {
        _MSTL memory_move(dest, srcs, count * sizeof(char_type));
        return dest;
    }

    /**
     * @brief 比较两个字符序列
     * @param lhs 左序列
     * @param rhs 右序列
     * @param count 要比较的字符数
     * @return 负值（lhs < rhs）、0（相等）、正值（lhs > rhs）
     */
    MSTL_NODISCARD static constexpr int compare(
        const char_type* lhs, const char_type* rhs, size_t count) noexcept {
        return _MSTL string_compare(lhs, rhs, count);
    }

    /**
     * @brief 计算字符串长度
     * @param str 以空字符结尾的字符串
     * @return 字符串长度（不含空字符）
     */
    MSTL_NODISCARD static constexpr size_t length(const char_type* str) noexcept {
        return _MSTL string_length(str);
    }

    /**
     * @brief 在字符序列中查找指定字符
     * @param str 字符序列
     * @param count 序列长度
     * @param target 要查找的字符
     * @return 指向第一个匹配字符的指针，未找到则返回nullptr
     */
    MSTL_NODISCARD static constexpr const char_type* find(
        const char_type* str, const size_t count, const char_type target) noexcept {
        return _MSTL string_find<char_type>(str, target, count);
    }

    /**
     * @brief 将字符序列中的每个字符设置为指定值
     * @param str 目标字符序列
     * @param count 序列长度
     * @param chr 要设置的字符
     * @return str指针
     */
    static constexpr char_type* assign(
        char_type* const str, const size_t count, const char_type chr) noexcept {
        return _MSTL string_set<char_type>(str, chr, count);
    }

    /**
     * @brief 赋值单个字符
     * @param lhs 左值引用
     * @param rhs 右值
     */
    static constexpr void assign(char_type& lhs, const char_type rhs) noexcept {
        lhs = rhs;
    }

    /**
     * @brief 相等比较
     * @param lhs 左字符
     * @param rhs 右字符
     * @return 是否相等
     */
    MSTL_NODISCARD static constexpr bool eq(const char_type lhs, const char_type rhs) noexcept {
        return lhs == rhs;
    }

    /**
     * @brief 小于比较
     * @param lhs 左字符
     * @param rhs 右字符
     * @return 是否lhs < rhs
     */
    MSTL_NODISCARD static constexpr bool lt(const char_type lhs, const char_type rhs) noexcept {
        return lhs < rhs;
    }

    /**
     * @brief 如果不是EOF则返回原值，否则返回0
     * @param rsc 输入值
     * @return 非EOF值或0
     */
    MSTL_NODISCARD static constexpr int_type not_eof(const int_type rsc) noexcept {
        return rsc == eof() ? static_cast<int_type>(0) : rsc;
    }

    /**
     * @brief 返回EOF值
     * @return EOF值
     */
    MSTL_NODISCARD static constexpr int_type eof() noexcept {
        return static_cast<int_type>(-1);
    }
};

/**
 * @struct narrow_char_traits
 * @brief 窄字符特征模板
 * @tparam CharT 字符类型
 * @tparam IntT 整数类型
 *
 * 为窄字符类型提供优化的内存操作实现。
 */
template <typename CharT, typename IntT>
struct narrow_char_traits : private base_char_traits<CharT, IntT> {
    static_assert(sizeof(CharT) == sizeof(byte_t), "size of CharT must be the same as byte type");

private:
    using base_type = base_char_traits<CharT, IntT>;

public:
    using char_type = CharT;  ///< 字符类型
    using int_type  = IntT;   ///< 整数类型

    using base_type::copy;
    using base_type::move;
    using base_type::length;
    using base_type::eq;
    using base_type::lt;
    using base_type::not_eof;
    using base_type::eof;

public:
    /**
     * @brief 比较两个字符序列（内存优化版本）
     * @param lhs 左序列
     * @param rhs 右序列
     * @param n 要比较的字符数
     * @return 比较结果
     */
    MSTL_NODISCARD static constexpr int compare(
        const char_type* lhs, const char_type* rhs, const size_t n) noexcept {
        return _MSTL memory_compare(lhs, rhs, n);
    }

    /**
     * @brief 在字符序列中查找指定字符（内存优化版本）
     * @param str 字符序列
     * @param n 序列长度
     * @param chr 要查找的字符
     * @return 指向第一个匹配字符的指针
     */
    MSTL_NODISCARD static constexpr const char_type* find(
        const char_type* str, const size_t n, const char_type chr) noexcept {
        return static_cast<const char_type*>(_MSTL memory_find(str, chr, n));
    }

    /**
     * @brief 将字符序列中的每个字符设置为指定值（内存优化版本）
     * @param str 目标字符序列
     * @param n 序列长度
     * @param chr 要设置的字符
     * @return str指针
     */
    static constexpr char_type* assign(
        char_type* str, size_t n, const char_type chr) noexcept {
        return static_cast<char_type*>(_MSTL memory_set(str, chr, n));
    }

    /**
     * @brief 赋值单个字符
     * @param lhs 左值引用
     * @param rhs 右值引用
     */
    static constexpr void assign(char_type& lhs, const char_type& rhs) noexcept {
        lhs = rhs;
    }
};


/**
 * @struct char_traits
 * @brief 字符特征模板
 * @tparam CharT 字符类型
 *
 * 默认使用base_char_traits，针对具体字符类型有特化。
 */
template <typename CharT>
struct char_traits : base_char_traits<CharT, int64_t> {};

/// char类型的特化
template <> struct char_traits<char>     : narrow_char_traits<char, int32_t> {};

/// wchar_t类型的特化
template <> struct char_traits<wchar_t>  : base_char_traits<wchar_t, uint32_t> {};

#if defined(MSTL_STANDARD_20__) || defined(MSTL_DOXYGEN_GENERATE)
/// char8_t类型的特化
template <> struct char_traits<char8_t>  : narrow_char_traits<char8_t, uint32_t> {};
#endif

/// char16_t类型的特化
template <> struct char_traits<char16_t> : base_char_traits<char16_t, uint32_t> {};

/// char32_t类型的特化
template <> struct char_traits<char32_t> : base_char_traits<char32_t, uint32_t> {};


/**
 * @brief 获取字符特征中的字符类型
 * @tparam Traits 字符特征类型
 */
template <typename Traits>
using char_traits_char_t = typename Traits::char_type;

/**
 * @brief 获取字符特征中的字符指针类型
 * @tparam Traits 字符特征类型
 */
template <typename Traits>
using char_traits_ptr_t = const typename Traits::char_type*;


/// @cond
MSTL_BEGIN_INNER__

/**
 * @class __string_bitmap
 * @brief 字符串查找优化用的简易位图
 * @tparam CharT 字符类型
 * @tparam IsChar 是否为字符类型
 *
 * 用于加速字符串查找操作，通过位图记录字符出现情况。
 */
template <typename CharT, bool IsChar = is_character_v<CharT>>
class __string_bitmap {
private:
    bool matches_[numeric_traits<byte_t>::max() + 1] = {};  ///< 字符匹配位图

public:
    /**
     * @brief 默认构造函数
     */
    constexpr __string_bitmap() = default;

    /**
     * @brief 标记范围内的字符
     * @param first 起始位置
     * @param last 结束位置
     * @return 总是返回true
     */
    constexpr bool mark(const CharT* first, const CharT* const last) noexcept {
        for (; first != last; ++first) {
            matches_[static_cast<byte_t>(*first)] = true;
        }
        return true;
    }

    /**
     * @brief 检查字符是否被标记
     * @param chr 要检查的字符
     * @return 是否被标记
     */
    constexpr bool match(const CharT chr) const noexcept {
        return matches_[static_cast<byte_t>(chr)];
    }
};

template <typename CharT>
class __string_bitmap<CharT, false> {};

MSTL_END_INNER__
/// @endcond

/**
 * @brief 比较两个字符序列是否相等
 * @tparam Traits 字符特征类型
 * @param lhs 左序列
 * @param lh_size 左序列长度
 * @param rhs 右序列
 * @param rh_size 右序列长度
 * @return 是否相等
 */
template <typename Traits>
constexpr bool char_traits_equal(const char_traits_ptr_t<Traits> lhs, const size_t lh_size,
    const char_traits_ptr_t<Traits> rhs, const size_t rh_size) noexcept {
    if (lh_size != rh_size) return false;
    if (lh_size == 0u) return true;

    return Traits::compare(lhs, rhs, lh_size) == 0;
}

/**
 * @brief 比较两个字符序列（三路比较）
 * @tparam Traits 字符特征类型
 * @param lhs 左序列
 * @param lh_size 左序列长度
 * @param rhs 右序列
 * @param rh_size 右序列长度
 * @return 负值（lhs < rhs）、0（相等）、正值（lhs > rhs）
 */
template <typename Traits>
constexpr int char_traits_compare(const char_traits_ptr_t<Traits> lhs, const size_t lh_size,
    const char_traits_ptr_t<Traits> rhs, const size_t rh_size) noexcept {
    const int state = Traits::compare(lhs, rhs, _MSTL min(lh_size, rh_size));
    if (state != 0) return state;

    if (lh_size < rh_size) return -1;
    if (lh_size > rh_size) return 1;
    return 0;
}

/**
 * @brief 在字符序列中查找子序列
 * @tparam Traits 字符特征类型
 * @param dest 目标序列
 * @param dest_size 目标序列长度
 * @param start 起始位置
 * @param rsc 要查找的子序列
 * @param rsc_size 子序列长度
 * @return 子序列首次出现的位置，未找到则返回-1
 */
template <typename Traits>
constexpr size_t char_traits_find(const char_traits_ptr_t<Traits> dest, const size_t dest_size,
    const size_t start, const char_traits_ptr_t<Traits> rsc, const size_t rsc_size) noexcept {
    if (rsc_size > dest_size || start > dest_size - rsc_size) return static_cast<size_t>(-1);
    if (rsc_size == 0)  return start;

    const auto may_match_end = dest + (dest_size - rsc_size) + 1;
    for (auto if_match = dest + start; ; ++if_match) {
        if_match = Traits::find(if_match, static_cast<size_t>(may_match_end - if_match), *rsc);
        if (!if_match) return static_cast<size_t>(-1);

        if (Traits::compare(if_match, rsc, rsc_size) == 0)
            return static_cast<size_t>(if_match - dest);
    }
}

/**
 * @brief 在字符序列中查找单个字符
 * @tparam Traits 字符特征类型
 * @param dest 目标序列
 * @param dest_size 目标序列长度
 * @param start 起始位置
 * @param chr 要查找的字符
 * @return 字符首次出现的位置，未找到则返回-1
 */
template <typename Traits>
constexpr size_t char_traits_find_char(const char_traits_ptr_t<Traits> dest, const size_t dest_size,
    const size_t start, const char_traits_char_t<Traits> chr) noexcept {
    if (start < dest_size) {
        const auto found = Traits::find(dest + start, dest_size - start, chr);
        if (found)
            return static_cast<size_t>(found - dest);
    }
    return static_cast<size_t>(-1);
}

/**
 * @brief 从后向前查找子序列
 * @tparam Traits 字符特征类型
 * @param dest 目标序列
 * @param dest_size 目标序列长度
 * @param start 起始位置
 * @param rsc 要查找的子序列
 * @param rsc_size 子序列长度
 * @return 子序列最后一次出现的位置，未找到则返回-1
 */
template <typename Traits>
constexpr size_t char_traits_rfind(const char_traits_ptr_t<Traits> dest, const size_t dest_size,
    const size_t start, const char_traits_ptr_t<Traits> rsc, const size_t rsc_size) noexcept {
    if (rsc_size == 0) return _MSTL min(start, dest_size);

    if (rsc_size <= dest_size) {
        for (auto if_match = dest + _MSTL min(start, dest_size - rsc_size);; --if_match) {
            if (Traits::eq(*if_match, *rsc) && Traits::compare(if_match, rsc, rsc_size) == 0)
                return static_cast<size_t>(if_match - dest);

            if (if_match == dest) break;
        }
    }
    return static_cast<size_t>(-1);
}

/**
 * @brief 从后向前查找单个字符
 * @tparam Traits 字符特征类型
 * @param dest 目标序列
 * @param dest_size 目标序列长度
 * @param start 起始位置
 * @param chr 要查找的字符
 * @return 字符最后一次出现的位置，未找到则返回-1
 */
template <typename Traits>
constexpr size_t char_traits_rfind_char(const char_traits_ptr_t<Traits> dest, const size_t dest_size,
    const size_t start, const char_traits_char_t<Traits> chr) noexcept {
    if (dest_size != 0) {
        for (auto if_match = dest + _MSTL min(start, dest_size - 1);; --if_match) {
            if (Traits::eq(*if_match, chr))
                return static_cast<size_t>(if_match - dest);

            if (if_match == dest) break;
        }
    }
    return static_cast<size_t>(-1);
}

/**
 * @brief 查找第一个出现在给定集合中的字符（char_traits特化版本）
 * @tparam Traits 字符特征类型
 * @param dest 目标序列
 * @param dest_size 目标序列长度
 * @param start 起始位置
 * @param rsc 字符集合
 * @param rsc_size 集合大小
 * @return 第一个匹配字符的位置，未找到则返回-1
 */
template <typename Traits, enable_if_t<
#ifdef MSTL_STANDARD_17__
    is_specialization_v<Traits, char_traits>
#else
    is_specialization_v<Traits, char_traits>()
#endif
    , int> = 0>
constexpr size_t char_traits_find_first_of(const char_traits_ptr_t<Traits> dest, const size_t dest_size,
    const size_t start, const char_traits_ptr_t<Traits> rsc, const size_t rsc_size) noexcept {
    if (rsc_size != 0 && start < dest_size) {
        _INNER __string_bitmap<char_traits_char_t<Traits>> match;
        if (!match.mark(rsc, rsc + rsc_size)) {
            return (char_traits_find_first_of<Traits, false>)
                (dest, dest_size, start, rsc, rsc_size);
        }
        const auto end = dest + dest_size;
        for (auto if_match = dest + start; if_match < end; ++if_match) {
            if (match.match(*if_match))
                return static_cast<size_t>(if_match - dest);
        }
    }
    return static_cast<size_t>(-1);
}

/**
 * @brief 查找第一个出现在给定集合中的字符（非char_traits版本）
 * @tparam Traits 字符特征类型
 * @param dest 目标序列
 * @param dest_size 目标序列长度
 * @param start 起始位置
 * @param rsc 字符集合
 * @param rsc_size 集合大小
 * @return 第一个匹配字符的位置，未找到则返回-1
 */
template <typename Traits, enable_if_t<
#ifdef MSTL_STANDARD_17__
    !is_specialization_v<Traits, char_traits>
#else
    !is_specialization_v<Traits, char_traits>()
#endif
    , int> = 0>
constexpr size_t char_traits_find_first_of(const char_traits_ptr_t<Traits> dest, const size_t dest_size,
    const size_t start, const char_traits_ptr_t<Traits> rsc, const size_t rsc_size) noexcept {
    if (rsc_size != 0 && start < dest_size) {
        const auto end = dest + dest_size;
        for (auto if_match = dest + start; if_match < end; ++if_match) {
            if (Traits::find(rsc, rsc_size, *if_match))
                return static_cast<size_t>(if_match - dest);
        }
    }
    return static_cast<size_t>(-1);
}

/**
 * @brief 查找最后一个出现在给定集合中的字符（char_traits特化版本）
 * @tparam Traits 字符特征类型
 * @param dest 目标序列
 * @param dest_size 目标序列长度
 * @param start 起始位置
 * @param rsc 字符集合
 * @param rsc_size 集合大小
 * @return 最后一个匹配字符的位置，未找到则返回-1
 */
template <typename Traits, enable_if_t<
#ifdef MSTL_STANDARD_17__
    is_specialization_v<Traits, char_traits>
#else
    is_specialization_v<Traits, char_traits>()
#endif
    , int> = 0>
constexpr size_t char_traits_find_last_of(const char_traits_ptr_t<Traits> dest, const size_t dest_size,
    const size_t start, const char_traits_ptr_t<Traits> rsc, const size_t rsc_size) noexcept {
    if (rsc_size != 0 && dest_size != 0) {
        _INNER __string_bitmap<char_traits_char_t<Traits>> match;
        if (!match.mark(rsc, rsc + rsc_size))
            return (char_traits_find_last_of<Traits, false>)
            (dest, dest_size, start, rsc, rsc_size);

        for (auto if_match = dest + _MSTL min(start, dest_size - 1);; --if_match) {
            if (match.match(*if_match))
                return static_cast<size_t>(if_match - dest);

            if (if_match == dest) break;
        }
    }
    return static_cast<size_t>(-1);
}

/**
 * @brief 查找最后一个出现在给定集合中的字符（非char_traits版本）
 * @tparam Traits 字符特征类型
 * @param dest 目标序列
 * @param dest_size 目标序列长度
 * @param start 起始位置
 * @param rsc 字符集合
 * @param rsc_size 集合大小
 * @return 最后一个匹配字符的位置，未找到则返回-1
 */
template <typename Traits, enable_if_t<
#ifdef MSTL_STANDARD_17__
    !is_specialization_v<Traits, char_traits>
#else
    !is_specialization_v<Traits, char_traits>()
#endif
    , int> = 0>
constexpr size_t char_traits_find_last_of(const char_traits_ptr_t<Traits> dest, const size_t dest_size,
    const size_t start, const char_traits_ptr_t<Traits> rsc, const size_t rsc_size) noexcept {
    if (rsc_size != 0 && dest_size != 0) {
        for (auto if_match = dest + _MSTL min(start, dest_size - 1);; --if_match) {
            if (Traits::find(rsc, rsc_size, *if_match))
                return static_cast<size_t>(if_match - dest);

            if (if_match == dest) break;
        }
    }
    return static_cast<size_t>(-1);
}

/**
 * @brief 查找第一个不在给定集合中的字符（char_traits特化版本）
 * @tparam Traits 字符特征类型
 * @param dest 目标序列
 * @param dest_size 目标序列长度
 * @param start 起始位置
 * @param rsc 字符集合
 * @param rsc_size 集合大小
 * @return 第一个不匹配字符的位置，未找到则返回-1
 */
template <typename Traits, enable_if_t<
#ifdef MSTL_STANDARD_17__
    is_specialization_v<Traits, char_traits>
#else
    is_specialization_v<Traits, char_traits>()
#endif
    , int> = 0>
constexpr size_t char_traits_find_first_not_of(const char_traits_ptr_t<Traits> dest, const size_t dest_size,
    const size_t start, const char_traits_ptr_t<Traits> rsc, const size_t rsc_size) noexcept {
    if (start < dest_size) {
        _INNER __string_bitmap<char_traits_char_t<Traits>> match;
        if (!match.mark(rsc, rsc + rsc_size))
            return (char_traits_find_first_not_of<Traits, false>)
            (dest, dest_size, start, rsc, rsc_size);

        const auto end = dest + dest_size;
        for (auto if_match = dest + start; if_match < end; ++if_match) {
            if (!match.match(*if_match))
                return static_cast<size_t>(if_match - dest);
        }
    }
    return static_cast<size_t>(-1);
}

/**
 * @brief 查找第一个不在给定集合中的字符（非char_traits版本）
 * @tparam Traits 字符特征类型
 * @param dest 目标序列
 * @param dest_size 目标序列长度
 * @param start 起始位置
 * @param rsc 字符集合
 * @param rsc_size 集合大小
 * @return 第一个不匹配字符的位置，未找到则返回-1
 */
template <typename Traits, enable_if_t<
#ifdef MSTL_STANDARD_17__
    !is_specialization_v<Traits, char_traits>
#else
    !is_specialization_v<Traits, char_traits>()
#endif
    , int> = 0>
constexpr size_t char_traits_find_first_not_of(const char_traits_ptr_t<Traits> dest, const size_t dest_size,
    const size_t start, const char_traits_ptr_t<Traits> rsc, const size_t rsc_size) noexcept {
    if (start < dest_size) {
        const auto end = dest + dest_size;
        for (auto if_match = dest + start; if_match < end; ++if_match) {
            if (!Traits::find(rsc, rsc_size, *if_match))
                return static_cast<size_t>(if_match - dest);
        }
    }
    return static_cast<size_t>(-1);
}

/**
 * @brief 查找第一个不等于指定字符的位置
 * @tparam Traits 字符特征类型
 * @param dest 目标序列
 * @param dest_size 目标序列长度
 * @param start 起始位置
 * @param chr 指定字符
 * @return 第一个不等于chr的位置，未找到则返回-1
 */
template <typename Traits>
constexpr size_t char_traits_find_not_char(const char_traits_ptr_t<Traits> dest, const size_t dest_size,
    const size_t start, const char_traits_char_t<Traits> chr) noexcept {
    if (start < dest_size) {
        const auto end = dest + dest_size;
        for (auto if_match = dest + start; if_match < end; ++if_match) {
            if (!Traits::eq(*if_match, chr))
                return static_cast<size_t>(if_match - dest);
        }
    }
    return static_cast<size_t>(-1);
}

/**
 * @brief 查找最后一个不在给定集合中的字符（char_traits特化版本）
 * @tparam Traits 字符特征类型
 * @param dest 目标序列
 * @param dest_size 目标序列长度
 * @param start 起始位置
 * @param rsc 字符集合
 * @param rsc_size 集合大小
 * @return 最后一个不匹配字符的位置，未找到则返回-1
 */
template <typename Traits, enable_if_t<
#ifdef MSTL_STANDARD_17__
    is_specialization_v<Traits, char_traits>
#else
    is_specialization_v<Traits, char_traits>()
#endif
    , int> = 0>
constexpr size_t char_traits_find_last_not_of(const char_traits_ptr_t<Traits> dest, const size_t dest_size,
    const size_t start, const char_traits_ptr_t<Traits> rsc, const size_t rsc_size) noexcept {
    if (dest_size != 0) {
        _INNER __string_bitmap<char_traits_char_t<Traits>> match;
        if (!match.mark(rsc, rsc + rsc_size))
            return (char_traits_find_last_not_of<Traits, false>)
            (dest, dest_size, start, rsc, rsc_size);

        for (auto if_match = dest + _MSTL min(start, dest_size - 1);; --if_match) {
            if (!match.match(*if_match))
                return static_cast<size_t>(if_match - dest);

            if (if_match == dest) break;
        }
    }
    return static_cast<size_t>(-1);
}

/**
 * @brief 查找最后一个不在给定集合中的字符（非char_traits版本）
 * @tparam Traits 字符特征类型
 * @param dest 目标序列
 * @param dest_size 目标序列长度
 * @param start 起始位置
 * @param rsc 字符集合
 * @param rsc_size 集合大小
 * @return 最后一个不匹配字符的位置，未找到则返回-1
 */
template <typename Traits, enable_if_t<
#ifdef MSTL_STANDARD_17__
    !is_specialization_v<Traits, char_traits>
#else
    !is_specialization_v<Traits, char_traits>()
#endif
    , int> = 0>
constexpr size_t char_traits_find_last_not_of(const char_traits_ptr_t<Traits> dest, const size_t dest_size,
    const size_t start, const char_traits_ptr_t<Traits> rsc, const size_t rsc_size) noexcept {
    if (dest_size != 0) {
        for (auto if_match = dest + _MSTL min(start, dest_size - 1);; --if_match) {
            if (!Traits::find(rsc, rsc_size, *if_match))
                return static_cast<size_t>(if_match - dest);

            if (if_match == dest) break;
        }
    }
    return static_cast<size_t>(-1);
}

/**
 * @brief 查找最后一个不等于指定字符的位置
 * @tparam Traits 字符特征类型
 * @param dest 目标序列
 * @param dest_size 目标序列长度
 * @param start 起始位置
 * @param chr 指定字符
 * @return 最后一个不等于chr的位置，未找到则返回-1
 */
template <typename Traits>
constexpr size_t char_traits_rfind_not_char(const char_traits_ptr_t<Traits> dest, const size_t dest_size,
    const size_t start, const char_traits_char_t<Traits> chr) noexcept {
    if (dest_size != 0) {
        for (auto if_match = dest + _MSTL min(start, dest_size - 1);; --if_match) {
            if (!Traits::eq(*if_match, chr))
                return static_cast<size_t>(if_match - dest);

            if (if_match == dest) break;
        }
    }
    return static_cast<size_t>(-1);
}


#define __MSTL_BUILD_CHAR_PTR_HASH(OPT) \
template <> \
struct hash<OPT*> { \
    MSTL_NODISCARD constexpr size_t operator ()(const OPT* str) const noexcept { \
        return _INNER FNV_hash_string(str, char_traits<OPT>::length(str)); \
    } \
}; \
template <> \
struct hash<const OPT*> { \
    MSTL_NODISCARD constexpr size_t operator ()(const OPT* str) const noexcept { \
        return _INNER FNV_hash_string(str, char_traits<OPT>::length(str)); \
    } \
}; \
template <size_t N> \
struct hash<OPT[N]> { \
    MSTL_NODISCARD constexpr size_t operator ()(const OPT (&str)[N]) const noexcept { \
        return _INNER FNV_hash_string(str, N - 1); \
    } \
}; \
template <size_t N> \
struct hash<const OPT[N]> { \
    MSTL_NODISCARD constexpr size_t operator ()(const OPT (&str)[N]) const noexcept { \
        return _INNER FNV_hash_string(str, N - 1); \
    } \
};

MSTL_MACRO_RANGE_CHARS(__MSTL_BUILD_CHAR_PTR_HASH)
#undef __MSTL_BUILD_CHAR_PTR_HASH

/** @} */ // CharTraits

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_STRING_CHAR_TRAITS_HPP__
