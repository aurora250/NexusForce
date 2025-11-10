#ifndef MSTL_CHAR_TRAITS_HPP__
#define MSTL_CHAR_TRAITS_HPP__
#include "algobase.hpp"
#include "hash.hpp"
#include "undef_cmacro.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename CharT, typename IntT>
struct base_char_traits {
    using char_type = CharT;
    using int_type  = IntT;

    static_assert(sizeof(int_type) >= sizeof(char_type),
        "int_type must be able to represent all char_type values plus EOF"
        );

    static constexpr char_type* copy(char_type* const dest,
        const char_type* const srcs, const size_t count) noexcept {
#ifdef MSTL_COMPILER_CLANG__
        __builtin_memcpy(dest, srcs, count * sizeof(char_type));
#else
#ifdef MSTL_STANDARD_20__
        if (_MSTL is_constant_evaluated()) {
            for (size_t i = 0; i != count; ++i)
                dest[i] = srcs[i];
            return dest;
        }
#endif // MSTL_STANDARD_20__
        _MSTL memory_copy(dest, srcs, count * sizeof(char_type));
#endif // MSTL_COMPILER_CLANG__
        return dest;
    }

    static constexpr char_type* move(char_type* const dest,
        const char_type* const srcs, const size_t count) noexcept {
#if defined(MSTL_COMPILER_CLANG__)
#if __has_builtin(__builtin_memmove)
        __builtin_memmove(dest, srcs, count * sizeof(char_type));
#else
        _MSTL memory_move(dest, srcs, count * sizeof(char_type));
#endif
#else
#if MSTL_STANDARD_20__
        if (_MSTL is_constant_evaluated()) {
            bool not_same = true;
            for (const char_type* src = srcs; src != srcs + count; ++src) {
                if (dest == src) {
                    not_same = false;
                    break;
                }
            }
            if (not_same) {
                for (size_t i = 0; i != count; ++i)
                    dest[i] = srcs[i];
            }
            else {
                for (size_t i = count; i != 0; --i)
                    dest[i - 1] = srcs[i - 1];
            }
            return dest;
        }
#endif // MSTL_STANDARD_20__
        _MSTL memory_move(dest, srcs, count * sizeof(char_type));
#endif // MSTL_SUPPORT_MEM_INTRINSICS__
        return dest;
    }

    MSTL_NODISCARD static constexpr int compare(const char_type* lh,
        const char_type* str2, size_t count) noexcept {
        for (; 0 < count; --count, ++lh, ++str2) {
            if (*lh != *str2)
                return *lh < *str2 ? -1 : +1;
        }
        return 0;
    }

    MSTL_NODISCARD static constexpr size_t length(const char_type* str) noexcept {
        size_t count = 0;
        while (*str != static_cast<char_type>(0)) {
            ++count;
            ++str;
        }
        return count;
    }

    MSTL_NODISCARD static constexpr const char_type* find(const char_type* str,
        size_t count, const char_type& target) noexcept {
        for (; 0 < count; --count, ++str) {
            if (*str == target) return str;
        }
        return nullptr;
    }

    static constexpr char_type* assign(char_type* const str, size_t count, const char_type chr) noexcept {
        for (char_type* next = str; count > 0; --count, ++next) {
            *next = chr;
        }
        return str;
    }
    static constexpr void assign(char_type& lh, const char_type& rh) noexcept {
        lh = rh;
    }

    MSTL_NODISCARD static constexpr bool eq(const char_type lh, const char_type rh) noexcept {
        return lh == rh;
    }
    MSTL_NODISCARD static constexpr bool lt(const char_type lh, const char_type rh) noexcept {
        return lh < rh;
    }
    MSTL_NODISCARD static constexpr char_type to_char_type(const int_type str) noexcept {
        return static_cast<char_type>(str);
    }
    MSTL_NODISCARD static constexpr int_type to_int_type(const char_type chr) noexcept {
        return static_cast<int_type>(chr);
    }
    MSTL_NODISCARD static constexpr bool eq_int_type(const int_type lh, const int_type rh) noexcept {
        return lh == rh;
    }
    MSTL_NODISCARD static constexpr int_type not_eof(const int_type rsc) noexcept {
        return eq_int_type(rsc, eof()) ? static_cast<int_type>(0) : rsc;
    }
    MSTL_NODISCARD static constexpr int_type eof() noexcept {
        return static_cast<int_type>(-1);
    }
};

template <class CharT, typename IntT = uint32_t>
struct wide_char_traits : private base_char_traits<CharT, IntT> {
private:
    using base_type = base_char_traits<CharT, IntT>;

public:
    using char_type = CharT;
    using int_type  = IntT;

    using base_type::copy;
    using base_type::move;

public:
    MSTL_NODISCARD static constexpr int compare(const char_type* const lh,
        const char_type* const rh, const size_t n) noexcept {
#if MSTL_STANDARD_20__
        if (_MSTL is_constant_evaluated()) {
            if constexpr (is_same_v<char_type, wchar_t>) {
                return __builtin_wmemcmp(lh, rh, n);
            }
            else {
                return base_type::compare(lh, rh, n);
            }
        }
#endif // MSTL_STANDARD_20__
        return _MSTL wchar_memory_compare(reinterpret_cast<const wchar_t*>(lh),
            reinterpret_cast<const wchar_t*>(rh), n);
    }

    MSTL_NODISCARD static constexpr size_t length(const char_type* str) noexcept {
#if MSTL_STANDARD_20__
        if (_MSTL is_constant_evaluated()) {
            if constexpr (is_same_v<char_type, wchar_t>) {
#if defined(MSTL_COMPILER_MSVC__) || defined(MSTL_COMPILER_CLANG__)
                return __builtin_wcslen(str);
#else
                return _MSTL wstring_length(str);
#endif
            }
            else {
                return base_type::length(str);
            }
        }
#endif // MSTL_STANDARD_20__
        return _MSTL wstring_length(reinterpret_cast<const wchar_t*>(str));
    }

    MSTL_NODISCARD static constexpr const char_type* find(
        const char_type* str, const size_t n, const char_type& chr) noexcept {
#if MSTL_STANDARD_20__
        if (_MSTL is_constant_evaluated()) {
            if constexpr (is_same_v<char_type, wchar_t>) {
                return __builtin_wmemchr(str, chr, n);
            }
            else {
                return base_type::find(str, n, chr);
            }
        }
#endif // MSTL_STANDARD_20__
        return reinterpret_cast<const char_type*>(
            _MSTL wchar_memory_char(reinterpret_cast<const wchar_t*>(str), chr, n));
    }

    static constexpr char_type* assign(char_type* const str, size_t n, const char_type chr) noexcept {
#if MSTL_STANDARD_20__
        if (_MSTL is_constant_evaluated()) {
            return base_type::assign(str, n, chr);
        }
#endif // MSTL_STANDARD_20__
        return reinterpret_cast<char_type*>(_MSTL wchar_memory_set(reinterpret_cast<wchar_t*>(str), chr, n));
    }

    static constexpr void assign(char_type& lh, const char_type& rh) noexcept {
#if MSTL_STANDARD_20__
        if (_MSTL is_constant_evaluated()) {
            return base_type::assign(lh, rh);
        }
#endif // MSTL_STANDARD_20__
        lh = rh;
    }

    MSTL_NODISCARD static constexpr bool eq(const char_type lh, const char_type rh) noexcept {
        return lh == rh;
    }
    MSTL_NODISCARD static constexpr bool lt(const char_type lh, const char_type rh) noexcept {
        return lh < rh;
    }
    MSTL_NODISCARD static constexpr char_type to_char_type(const int_type rsc) noexcept {
        return rsc;
    }
    MSTL_NODISCARD static constexpr int_type to_int_type(const char_type chr) noexcept {
        return chr;
    }
    MSTL_NODISCARD static constexpr bool eq_int_type(const int_type lh, const int_type rh) noexcept {
        return lh == rh;
    }

    MSTL_NODISCARD static constexpr int_type not_eof(const int_type rsc) noexcept {
        return eq_int_type(rsc, eof()) ? static_cast<int_type>(0) : rsc;
    }
    MSTL_NODISCARD static constexpr int_type eof() noexcept {
        return static_cast<int_type>(-1);
    }
};

template <typename CharT, typename IntT>
struct narrow_char_traits : private base_char_traits<CharT, IntT> {
private:
    using base_type = base_char_traits<CharT, IntT>;

public:
    using char_type = CharT;
    using int_type  = IntT;

    using base_type::copy;
    using base_type::move;

public:
    MSTL_NODISCARD static constexpr int compare(const char_type* const lh,
        const char_type* const rh, const size_t n) noexcept {
#ifdef MSTL_STANDARD_17__
        return __builtin_memcmp(lh, rh, n);
#else
        return _MSTL memory_compare(lh, rh, n);
#endif
    }

    MSTL_NODISCARD static constexpr size_t length(const char_type* const str) noexcept {
#ifdef MSTL_STANDARD_17__
#ifdef MSTL_STANDARD_20__
        if constexpr (is_same_v<char_type, char8_t>) {
#if defined(MSTL_STANDARD_20__) && !defined(MSTL_COMPILER_CLANG__)
#ifdef MSTL_COMPILER_MSVC__
            return __builtin_u8strlen(str);
#else
            return _MSTL u8string_length(str);
#endif
#else
            return base_type::length(str);
#endif
        }
        else
#endif // MSTL_STANDARD_20__
        {
            return __builtin_strlen(str);
        }
#else
        return _MSTL string_length(reinterpret_cast<const char*>(str));
#endif // MSTL_STANDARD_17__
    }

    MSTL_NODISCARD static constexpr const char_type* find(const char_type* const str,
        const size_t n, const char_type& chr) noexcept {
#ifdef MSTL_STANDARD_17__
#ifdef MSTL_STANDARD_20__
        if constexpr (is_same_v<char_type, char8_t>) {
#if defined(MSTL_STANDARD_20__) && !defined(MSTL_COMPILER_CLANG__) && !defined(MSTL_COMPILE_WITH_EDG__)
            return __builtin_u8memchr(str, chr, n);
#else
            return base_type::find(str, n, chr);
#endif // MSTL_SUPPORT_U8_INTRINSICS__
        }
        else
#endif // MSTL_STANDARD_20__
        {
#ifdef MSTL_COMPILER_MSVC__
            return __builtin_char_memchr(str, chr, n);
#else
            return base_type::find(str, n, chr);
#endif
        }
#else
        return static_cast<const char_type*>(_MSTL memory_char(str, chr, n));
#endif // MSTL_STANDARD_17__
    }

    static constexpr char_type* assign(char_type* const str, size_t n, const char_type chr) noexcept {
#ifdef MSTL_STANDARD_20__
        if (_MSTL is_constant_evaluated()) {
            return base_type::assign(str, n, chr);
        }
#endif // MSTL_STANDARD_20__
        return static_cast<char_type*>(_MSTL memory_set(str, chr, n));
    }

    static constexpr void assign(char_type& lh, const char_type& rh) noexcept {
#ifdef MSTL_STANDARD_20__
        if (_MSTL is_constant_evaluated()) {
            return base_type::assign(lh, rh);
        }
#endif // MSTL_STANDARD_20__
        lh = rh;
    }

    MSTL_NODISCARD static constexpr bool eq(const char_type lh, const char_type rh) noexcept {
        return lh == rh;
    }
    MSTL_NODISCARD static constexpr bool lt(const char_type lh, const char_type rh) noexcept {
        return static_cast<byte_t>(lh) < static_cast<byte_t>(rh);
    }
    MSTL_NODISCARD static constexpr char_type to_char_type(const int_type rsc) noexcept {
        return static_cast<char_type>(rsc);
    }
    MSTL_NODISCARD static constexpr int_type to_int_type(const char_type chr) noexcept {
        return static_cast<byte_t>(chr);
    }
    MSTL_NODISCARD static constexpr bool eq_int_type(const int_type lh, const int_type rh) noexcept {
        return lh == rh;
    }

    MSTL_NODISCARD static constexpr int_type not_eof(const int_type rsc) noexcept {
        return eq_int_type(rsc, eof()) ? static_cast<int_type>(0) : rsc;
    }
    MSTL_NODISCARD static constexpr int_type eof() noexcept {
        return static_cast<int_type>(-1);
    }
};


template <typename CharT>
struct char_traits : base_char_traits<CharT, int64_t> {};

template <> struct char_traits<char>
    : narrow_char_traits<char, conditional_t<numeric_limits<char>::is_signed, int32_t, uint32_t>>
{};
#ifdef MSTL_STANDARD_20__
template <> struct char_traits<char8_t> : narrow_char_traits<char8_t, uint32_t> {};
#endif
#ifdef MSTL_PLATFORM_WINDOWS__
template <> struct char_traits<wchar_t>  : wide_char_traits<wchar_t> {};
template <> struct char_traits<char16_t> : wide_char_traits<char16_t> {};
template <> struct char_traits<char32_t> : base_char_traits<char32_t, uint32_t> {};
#elif defined(MSTL_PLATFORM_LINUX__)
template <> struct char_traits<wchar_t>  : wide_char_traits<wchar_t> {};
template <> struct char_traits<char16_t> : base_char_traits<char16_t, uint32_t> {};
template <> struct char_traits<char32_t> : wide_char_traits<char32_t> {};
#endif


template <typename Traits>
using char_traits_char_t = typename Traits::char_type;
template <typename Traits>
using char_traits_ptr_t = const typename Traits::char_type*;


MSTL_BEGIN_INNER__

template <typename CharT, bool = is_character_v<CharT>>
class __string_bitmap {
private:
    bool matches_[numeric_limits<byte_t>::max() + 1] = {};

public:
    constexpr __string_bitmap() = default;

    constexpr bool mark(const CharT* first, const CharT* const last) noexcept {
        for (; first != last; ++first)
            matches_[static_cast<byte_t>(*first)] = true;
        return true;
    }
    constexpr bool match(const CharT chr) const noexcept {
        return matches_[static_cast<byte_t>(chr)];
    }
};

template <typename CharT>
class __string_bitmap<CharT, false> {};

MSTL_END_INNER__


template <typename Traits>
constexpr bool char_traits_equal(const char_traits_ptr_t<Traits> lh, const size_t lh_size,
    const char_traits_ptr_t<Traits> rh, const size_t rh_size) noexcept {
    if (lh_size != rh_size) return false;
    if (lh_size == 0u) return true;

    return Traits::compare(lh, rh, lh_size) == 0;
}

template <typename Traits>
constexpr int char_traits_compare(const char_traits_ptr_t<Traits> lh, const size_t lh_size,
    const char_traits_ptr_t<Traits> rh, const size_t rh_size) noexcept {
    const int state = Traits::compare(lh, rh, _MSTL min(lh_size, rh_size));
    if (state != 0) return state;

    if (lh_size < rh_size) return -1;
    if (lh_size > rh_size) return 1;
    return 0;
}

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
    MSTL_NODISCARD constexpr size_t operator()(const OPT (&str)[N]) const noexcept { \
        return _INNER FNV_hash_string(str, N - 1); \
    } \
}; \
template <size_t N> \
struct hash<const OPT[N]> { \
    MSTL_NODISCARD constexpr size_t operator()(const OPT (&str)[N]) const noexcept { \
        return _INNER FNV_hash_string(str, N - 1); \
    } \
};

MSTL_MACRO_RANGE_CHARS(__MSTL_BUILD_CHAR_PTR_HASH)
#undef __MSTL_BUILD_CHAR_PTR_HASH

MSTL_END_NAMESPACE__
#endif // MSTL_CHAR_TRAITS_HPP__
