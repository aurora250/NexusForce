#ifndef MSTL_STRING_HPP__
#define MSTL_STRING_HPP__
#include "cstring.hpp"
#include "basic_string.hpp"
#include <typeinfo>
MSTL_BEGIN_NAMESPACE__

using string    = basic_string<char>;
using bstring   = basic_string<byte_t>;
using wstring   = basic_string<wchar_t>;
#ifdef MSTL_STANDARD_20__
using u8string  = basic_string<char8_t>;
#endif
using u16string = basic_string<char16_t>;
using u32string = basic_string<char32_t>;


MSTL_BEGIN_LITERALS__
MSTL_NODISCARD MSTL_CONSTEXPR20 string operator ""_s(const char* str, size_t len) noexcept {
    return {str, len};
}
MSTL_NODISCARD MSTL_CONSTEXPR20 wstring operator ""_s(const wchar_t* str, size_t len) noexcept {
    return {str, len};
}
#ifdef MSTL_STANDARD_20__
MSTL_NODISCARD MSTL_CONSTEXPR20 u8string operator ""_s(const char8_t* str, size_t len) noexcept {
    return {str, len};
}
#endif // MSTL_STANDARD_20__
MSTL_NODISCARD MSTL_CONSTEXPR20 u16string operator ""_s(const char16_t* str, size_t len) noexcept {
    return {str, len};
}
MSTL_NODISCARD MSTL_CONSTEXPR20 u32string operator ""_s(const char32_t* str, size_t len) noexcept {
    return {str, len};
}
MSTL_END_LITERALS__


MSTL_CONSTEXPR20 string escape(const string_view str) {
    string result;
    result.reserve(str.length() + str.length() / 4);

    for (const char c : str) {
        switch (c) {
            case '\"':
                result += "\\\"";
            break;
            case '\'':
                result += "\\\'";
            break;
            case '\\':
                result += "\\\\";
            break;
            case '\b':
                result += "\\b";
            break;
            case '\f':
                result += "\\f";
            break;
            case '\n':
                result += "\\n";
            break;
            case '\r':
                result += "\\r";
            break;
            case '\t':
                result += "\\t";
            break;
            case '\v':
                result += "\\v";
            break;
            default:
                if (static_cast<byte_t>(c) < 0x20) {
                    result += "\\u";
                    constexpr char hex[] = "0123456789abcdef";
                    result += "00";
                    result += hex[(c >> 4) & 0x0F];
                    result += hex[c & 0x0F];
                } else {
                    result += c;
                }
            break;
        }
    }
    return result;
}

MSTL_CONSTEXPR20 string escape(const string& str) {
    return escape(str.view());
}

MSTL_CONSTEXPR20 string escape(const char* str) {
    return escape(string_view{str});
}


template <typename CharT>
MSTL_CONSTEXPR20 bool getline(const basic_string_view<CharT> data, size_t& pos,
    basic_string<CharT>& str, CharT delim = static_cast<CharT>('\n')) {
    str.clear();
    bool has_read = false;
    while (pos < data.size()) {
        has_read = true;
        const CharT c = data[pos++];
        if (c == delim) break;
        str.push_back(c);
    }
    if (!has_read) {
        return false;
    }
    return true;
}

template <typename CharT>
MSTL_CONSTEXPR20 bool getline(const basic_string<CharT>& data, size_t& pos,
    basic_string<CharT>& str, CharT delim = static_cast<CharT>('\n')) {
    str.clear();
    bool has_read = false;
    while (pos < data.size()) {
        has_read = true;
        const CharT c = data[pos++];
        if (c == delim) break;
        str.push_back(c);
    }
    if (!has_read) {
        return false;
    }
    return true;
}


MSTL_BEGIN_INNER__
#ifdef MSTL_DATA_BUS_WIDTH_64__
MSTL_INLINE17 constexpr uintptr_t ADDRESS_MASK = 0xF000000000000000ULL;
MSTL_INLINE17 constexpr int ADDRESS_SHIFT = 60;
#else
MSTL_INLINE17 constexpr uintptr_t ADDRESS_MASK = 0xF0000000UL;
MSTL_INLINE17 constexpr int ADDRESS_SHIFT = 28;
#endif
MSTL_END_INNER__

MSTL_NODISCARD MSTL_CONSTEXPR20 string address_string(const void* p) {
    if (p == nullptr) return {"nullptr"};
    
    const uintptr_t addr_val = reinterpret_cast<uintptr_t>(p);
    constexpr size_t hex_digit_count = POINTER_SIZE * 2;
    constexpr char hex_digits[] = "0123456789abcdef";
    uintptr_t mask = _INNER ADDRESS_MASK;
    int shift = _INNER ADDRESS_SHIFT;

    string result{"0x"};
    result.reserve(2 + hex_digit_count);

    for (size_t i = 0; i < hex_digit_count; ++i) {
        const byte_t digit = static_cast<byte_t>((addr_val & mask) >> shift);
        result += hex_digits[digit];
        mask >>= 4;
        shift -= 4;
    }
    return result;
}


template <typename T, typename CharT>
struct icharacter : icommon<T> {
    static_assert(is_character_v<CharT>, "icharacter can only be used with characters");
public:
    using base_type = icommon<T>;
    using self = icharacter<T, CharT>;
    using child_type = T;
    using value_type = CharT;

private:
    static constexpr child_type* to_template(const self* o) noexcept {
        return base_type::to_template(o);
    }

public:
    MSTL_CONSTEXPR20 ~icharacter() = default;

    constexpr bool is_space() const
    noexcept(noexcept(self::to_template(this)->is_space())) {
        return self::to_template(this)->is_space();
    }
    constexpr bool is_alpha() const
    noexcept(noexcept(self::to_template(this)->is_alpha())) {
        return self::to_template(this)->is_alpha();
    }
    constexpr bool is_digit() const
    noexcept(noexcept(self::to_template(this)->is_digit())) {
        return self::to_template(this)->is_digit();
    }
    constexpr bool is_xdigit() const
    noexcept(noexcept(self::to_template(this)->is_xdigit())) {
        return self::to_template(this)->is_xdigit();
    }
    constexpr bool is_alpha_or_digit() const
    noexcept(noexcept(self::to_template(this)->is_alpha_or_digit())) {
        return self::to_template(this)->is_alpha_or_digit();
    }
    constexpr bool is_digit_or_alpha() const
    noexcept(noexcept(self::to_template(this)->is_digit_or_alpha())) {
        return self::to_template(this)->is_digit_or_alpha();
    }

    constexpr void to_lowercase()
    noexcept(noexcept(self::to_template(this)->to_lowercase())) {
        return self::to_template(this)->to_lowercase();
    }
    constexpr void to_uppercase()
    noexcept(noexcept(self::to_template(this)->to_uppercase())) {
        return self::to_template(this)->to_uppercase();
    }

    static constexpr child_type to_lowercase(const self& obj)
    noexcept(noexcept(child_type(static_cast<const child_type&>(obj)).to_lowercase())) {
        return child_type(static_cast<const child_type&>(obj)).to_lowercase();
    }
    static constexpr child_type to_uppercase(const self& obj)
    noexcept(noexcept(child_type(static_cast<const child_type&>(obj)).to_uppercase())) {
        return child_type(static_cast<const child_type&>(obj)).to_uppercase();
    }

    static MSTL_CONSTEXPR20 string to_string(const basic_string_view<value_type>& obj) {
        return child_type::to_string(obj);
    }
    static MSTL_CONSTEXPR20 wstring to_wstring(const basic_string_view<value_type>& obj) {
        return child_type::to_wstring(obj);
    }
#ifdef MSTL_STANDARD_20__
    static MSTL_CONSTEXPR20 u8string to_u8string(const basic_string_view<value_type>& obj) {
        return child_type::to_u8string(obj);
    }
#endif
    static MSTL_CONSTEXPR20 u16string to_u16string(const basic_string_view<value_type>& obj) {
        return child_type::to_u16string(obj);
    }
    static MSTL_CONSTEXPR20 u32string to_u32string(const basic_string_view<value_type>& obj) {
        return child_type::to_u32string(obj);
    }
};

template <typename T, typename CharT, enable_if_t<is_base_of_v<icharacter<T, CharT>, T>, int> = 0>
constexpr bool is_space(const T& obj)
noexcept(noexcept(obj.to_space())) {
    return obj.is_space();
}
template <typename T, typename CharT, enable_if_t<is_base_of_v<icharacter<T, CharT>, T>, int> = 0>
constexpr bool is_alpha(const T& obj)
noexcept(noexcept(obj.is_alpha())) {
    return obj.is_alpha();
}
template <typename T, typename CharT, enable_if_t<is_base_of_v<icharacter<T, CharT>, T>, int> = 0>
constexpr bool is_digit(const T& obj)
noexcept(noexcept(obj.is_digit())) {
    return obj.is_digit();
}
template <typename T, typename CharT, enable_if_t<is_base_of_v<icharacter<T, CharT>, T>, int> = 0>
constexpr bool is_xdigit(const T& obj)
noexcept(noexcept(obj.is_xdigit())) {
    return obj.is_xdigit();
}
template <typename T, typename CharT, enable_if_t<is_base_of_v<icharacter<T, CharT>, T>, int> = 0>
constexpr bool is_alpha_or_digit(const T& obj)
noexcept(noexcept(obj.is_alpha_or_digit())) {
    return obj.is_alpha_or_digit();
}
template <typename T, typename CharT, enable_if_t<is_base_of_v<icharacter<T, CharT>, T>, int> = 0>
constexpr bool is_digit_or_alpha(const T& obj)
noexcept(noexcept(obj.is_digit_or_alpha())) {
    return obj.is_digit_or_alpha();
}

template <typename T, typename CharT, enable_if_t<is_base_of_v<icharacter<T, CharT>, T>, int> = 0>
constexpr T to_lowercase(const T& obj)
noexcept(noexcept(icharacter<T, CharT>::to_lowercase(obj))) {
    return icharacter<T, CharT>::to_lowercase(obj);
}
template <typename T, typename CharT, enable_if_t<is_base_of_v<icharacter<T, CharT>, T>, int> = 0>
constexpr T to_uppercase(const T& obj)
noexcept(noexcept(icharacter<T, CharT>::to_uppercase(obj))) {
    return icharacter<T, CharT>::to_uppercase(obj);
}


template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 string to_string(const CharT& x) {
    return icharacter<package_t<CharT>, CharT>::to_string(basic_string<CharT>(1, x).view());
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 string to_string(const CharT* x) {
    return icharacter<package_t<CharT>, CharT>::to_string(basic_string_view<CharT>(x));
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 string to_string(const basic_string_view<CharT> x) {
    return icharacter<package_t<CharT>, CharT>::to_string(x);
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 string to_string(const basic_string<CharT>& x) {
    return icharacter<package_t<CharT>, CharT>::to_string(x.view());
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 string to_string(basic_string<CharT>&& x) {
    return icharacter<package_t<CharT>, CharT>::to_string(x.view());
}
template <>
MSTL_CONSTEXPR20 string to_string<char>(string&& x) {
    return _MSTL move(x);
}

template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 wstring to_wstring(const CharT& x) {
    return icharacter<package_t<CharT>, CharT>::to_wstring(basic_string<CharT>(1, x).view());
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 wstring to_wstring(const CharT* x) {
    return icharacter<package_t<CharT>, CharT>::to_wstring(basic_string_view<CharT>(x));
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 wstring to_wstring(const basic_string_view<CharT> x) {
    return icharacter<package_t<CharT>, CharT>::to_wstring(x);
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 wstring to_wstring(const basic_string<CharT>& x) {
    return icharacter<package_t<CharT>, CharT>::to_wstring(x.view());
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 wstring to_wstring(basic_string<CharT>&& x) {
    return icharacter<package_t<CharT>, CharT>::to_wstring(x.view());
}
template <>
MSTL_CONSTEXPR20 wstring to_wstring<wchar_t>(wstring&& x) {
    return _MSTL move(x);
}

#ifdef MSTL_STANDARD_20__
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 u8string to_u8string(const CharT& x) {
    return icharacter<package_t<CharT>, CharT>::to_u8string(basic_string<CharT>(1, x).view());
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 u8string to_u8string(const CharT* x) {
    return icharacter<package_t<CharT>, CharT>::to_u8string(basic_string_view<CharT>(x));
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 u8string to_u8string(const basic_string_view<CharT> x) {
    return icharacter<package_t<CharT>, CharT>::to_u8string(x);
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 u8string to_u8string(const basic_string<CharT>& x) {
    return icharacter<package_t<CharT>, CharT>::to_u8string(x.view());
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 u8string to_u8string(basic_string<CharT>&& x) {
    return icharacter<package_t<CharT>, CharT>::to_u8string(x.view());
}
template <>
MSTL_CONSTEXPR20 u8string to_u8string<char8_t>(u8string&& x) {
    return _MSTL move(x);
}
#endif

template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 u16string to_u16string(const CharT& x) {
    return icharacter<package_t<CharT>, CharT>::to_u16string(basic_string<CharT>(1, x).view());
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 u16string to_u16string(const CharT* x) {
    return icharacter<package_t<CharT>, CharT>::to_u16string(basic_string_view<CharT>(x));
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 u16string to_u16string(const basic_string_view<CharT> x) {
    return icharacter<package_t<CharT>, CharT>::to_u16string(x);
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 u16string to_u16string(const basic_string<CharT>& x) {
    return icharacter<package_t<CharT>, CharT>::to_u16string(x.view());
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 u16string to_u16string(basic_string<CharT>&& x) {
    return icharacter<package_t<CharT>, CharT>::to_u16string(x.view());
}
template <>
MSTL_CONSTEXPR20 u16string to_u16string<char16_t>(u16string&& x) {
    return _MSTL move(x);
}

template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 u32string to_u32string(const CharT& x) {
    return icharacter<package_t<CharT>, CharT>::to_u32string(basic_string<CharT>(1, x).view());
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 u32string to_u32string(const CharT* x) {
    return icharacter<package_t<CharT>, CharT>::to_u32string(basic_string_view<CharT>(x));
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 u32string to_u32string(const basic_string_view<CharT> x) {
    return icharacter<package_t<CharT>, CharT>::to_u32string(x);
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 u32string to_u32string(const basic_string<CharT>& x) {
    return icharacter<package_t<CharT>, CharT>::to_u32string(x.view());
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 u32string to_u32string(basic_string<CharT>&& x) {
    return icharacter<package_t<CharT>, CharT>::to_u32string(x.view());
}
template <>
MSTL_CONSTEXPR20 u32string to_u32string<char32_t>(u32string&& x) {
    return _MSTL move(x);
}


template <typename T>
struct istringify {
    using self = istringify<T>;
    using child_type = T;

private:
    static constexpr child_type* to_template(const self* o) noexcept {
        return const_cast<child_type*>(static_cast<const child_type*>(o));
    }

public:
    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string() const {
        return self::to_template(this)->to_string();
    }
};

template <typename T, enable_if_t<is_base_of_v<istringify<T>, T>, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const T& obj) {
    return obj.to_string();
}
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

template <typename T, enable_if_t<is_union_v<T>, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const T& x) {
    return _MSTL address_string(&x);
}


template <typename T, enable_if_t<is_function_v<T>, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(T&&) {
    return {typeid(T).name()};
}

template <typename T, enable_if_t<is_member_object_pointer_v<T>, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(T&&) {
    return {typeid(T).name()};
}
template <typename T, enable_if_t<is_member_function_pointer_v<T>, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(T&&) {
    return {typeid(T).name()};
}


MSTL_BEGIN_INNER__
template <typename Collector>
MSTL_NODISCARD MSTL_CONSTEXPR20 string collector_to_string(const Collector& c) {
    if (_MSTL empty(c)) return {"[]"};
    string result;
    result += "[ ";
    for (auto iter = _MSTL cbegin(c); iter != _MSTL cend(c); ++iter) {
        if (iter != _MSTL cbegin(c)) result += ", ";
        string tmp = to_string(*iter);
        result += tmp;
    }
    result += " ]";
    return result;
}
MSTL_END_INNER__

template <typename T, enable_if_t<is_unbounded_array_v<T>, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const T&) {
    return {"[]"};
}
template <typename T, enable_if_t<is_bounded_array_v<T> && !is_cstring_v<T>, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const T& x) {
    return _INNER collector_to_string(x);
}


template <typename T, enable_if_t<is_base_of_v<_MSTL Error, T>, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const T& obj) {
    return string(obj.type_) + "(" + obj.info_ + ")";
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


template <typename T>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const shared_ptr<T>& sp) {
    return address_string(sp.get());
}
template <typename T, typename Deleter>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const unique_ptr<T, Deleter>& sp) {
    return address_string(sp.get());
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
    _MSTL memory_zero(buffer, count);
    return basic_string<CharT>(rnext, count);
}

template <typename CharT, typename T, enable_if_t<conjunction<is_integral<T>, is_unsigned<T>>::value, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 basic_string<CharT> __uint_to_string(T x) {
    CharT buffer[21];
    CharT* const buffer_end = buffer + 21;
    CharT* const rnext = _INNER __uint_to_buff(buffer_end, x);
    const size_t count = buffer_end - rnext;
    _MSTL memory_zero(buffer, count);
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

template <typename T, enable_if_t<is_floating_point<T>::value, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string __to_string_with_precision(T x, int precision, bool scientific = false) {
    return _INNER __float_to_string_with_precision<char>(x, precision, scientific, scientific);
}

template <typename T, enable_if_t<is_floating_point<T>::value, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string __to_string_general(T x, int precision = 6) {
    return _INNER __float_to_string_with_precision<char>(x, precision, false, false);
}

template <typename T, enable_if_t<is_floating_point<T>::value, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string __to_string_fixed(T x, int precision = 6) {
    return _INNER __float_to_string_with_precision<char>(x, precision, false, true);
}

template <typename T, enable_if_t<is_floating_point<T>::value, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string __to_string_scientific(T x, int precision = 6) {
    return _INNER __float_to_string_with_precision<char>(x, precision, true, false);
}

MSTL_END_INNER__

MSTL_END_NAMESPACE__
#endif // MSTL_STRING_HPP__
