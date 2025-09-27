#ifndef MSTL_PRINT_HPP__
#define MSTL_PRINT_HPP__
#include "check_type.hpp"
#include "variant.hpp"
#include "optional.hpp"
#include "functional.hpp"
#include "array.hpp"
#include "list.hpp"
#include "bitmap.hpp"
#include "queue.hpp"
#include "stack.hpp"
#include "any.hpp"
#include "set.hpp"
#include "unordered_map.hpp"
#include "unordered_set.hpp"
#include "file.hpp"
#include "json.hpp"
#include "hexadecimal.hpp"
#include "map.hpp"
#include "MSTL/web/session.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_INLINE17 constexpr uint32_t MSTL_SPLIT_LENGTH = 25U;

inline void split_line(std::ostream& out = std::cout,
    uint32_t size = MSTL_SPLIT_LENGTH, const char split = '-') {
    while (size--) out << split;
    out << '\n';
}

template <typename T>
struct address_printer {
    static void print(const T& t) {
        std::cout << "@" << _MSTL addressof(t);
    }
    static void print_feature(const T& t) {
        std::cout << "@" << _MSTL addressof(t) << "(" << check_type<T>() << ")";
    }
};

template <typename T, typename = void>
struct printer {
    static void print(const T& t) {
        address_printer<T>::print(t);
    }
    static void print_feature(const T& t) {
        address_printer<T>::print_feature(t);
    }
};
#ifdef MSTL_SUPPORT_DEDUCTION_GUIDES__
template <typename T>
explicit printer(const T&) -> printer<T>;
#endif


template <>
struct printer<void> {
    static void print(...) {
        std::cout << "void";
    }
    static void print_feature(...) {
        std::cout << "(void)";
    }
};


template <typename T>
struct printer<T, enable_if_t<is_character_v<T>>> {
    static void print(T t) {
        std::cout << t;
    }
    static void print_feature(T t) {
        std::cout << "\'" << t << "\'";
    }
};

template <typename T>
struct printer<T, enable_if_t<is_boolean_v<T>>> {
    static void print(T t) {
        if (t) std::cout << "true";
        else std::cout << "false";
    }
    static void print_feature(T t) {
        printer::print(t);
    }
};

template <typename T>
struct printer<T, enable_if_t<is_standard_integer_v<T>>> {
private:
    template <typename U, enable_if_t<is_signed<U>::value, int> = 0>
    static void __print_feature_dispatch(const U& t) {
        std::cout << t;
    }
    template <typename U, enable_if_t<is_unsigned<U>::value, int> = 0>
    static void __print_feature_dispatch(const U& t) {
        std::cout << t << "u";
    }

public:
    static void print(const T& t) {
        std::cout << t;
    }
    static void print_feature(const T& t) {
        printer::__print_feature_dispatch<T>(t);
    }
};

template <typename T>
struct printer<T, enable_if_t<is_floating_point<T>::value>> {
    static void print(T t) {
        std::cout << t;
    }
    static void print_feature(T t) {
        std::cout << t << "F";
    }
};


template <typename T>
struct printer<T, enable_if_t<is_null_pointer_v<T>>> {
    static void print(nullptr_t) {
        std::cout << "nullptr";
    }
    static void print_feature(nullptr_t) {
        printer::print(nullptr);
    }
};


template <typename T>
struct printer<T, enable_if_t<is_bounded_array_v<T> && !is_ctype_string_v<T>>> {
    static void print(const T& t) {
        if (t == nullptr) {
            printer<nullptr_t>::print(nullptr);
            return;
        }
        using value_type = remove_extent_t<T>;
        constexpr auto s = extent_v<T>;
        std::cout << "[ ";
        for (auto idx = 0; idx < s; ++idx) {
            if (idx != 0) std::cout << ", ";
            _MSTL printer<value_type>::print(t[idx]);
        }
        std::cout << " ]";
    }

    static void print_feature(const T& t) {
        if (t == nullptr) {
            printer<nullptr_t>::print(nullptr);
            return;
        }
        constexpr auto s = extent_v<T>;
        using value_type = remove_extent_t<T>;
        using size_type = remove_cvref_t<decltype(s)>;

        std::cout << "[ ";
        for (auto idx = 0; idx < s; ++idx) {
            if (idx != 0) std::cout << ", ";
            _MSTL printer<value_type>::print_feature(t[idx]);
        }
        std::cout << " ](" << check_type<value_type>() << "[";
        _MSTL printer<size_type>::print_feature(s);
        std::cout << "])";
    }
};

template <typename T>
struct printer<T, enable_if_t<is_unbounded_array_v<T>>> {
    static void print(const T& t) {
        if (t == nullptr) {
            printer<nullptr_t>::print(nullptr);
            return;
        }
        std::cout << "[]";
    }
    static void print_feature(const T& t) {
        if (t == nullptr) {
            printer<nullptr_t>::print(nullptr);
            return;
        }
        using raw_type = remove_extent_t<T>;
        std::cout << "[](" << check_type<raw_type>() << "[])";
    }
};


template <typename T>
struct printer<T, enable_if_t<is_pointer_v<T> && !is_ctype_string_v<T>>> {
    static void print(const T& t) {
        if (t == nullptr) {
            printer<nullptr_t>::print(nullptr);
            return;
        }
        address_printer<T>::print(t);
    }
    static void print_feature(const T& t) {
        if (t == nullptr) {
            printer<nullptr_t>::print(nullptr);
            return;
        }
        using raw_type = remove_cvref_t<decltype(*t)>;
        std::cout << "@" << _MSTL addressof(t) << "(" << check_type<T>() << " ->";
        printer<raw_type>::print_feature(*t);
        std::cout << ")";
    }
};


template <typename T>
struct printer<T, enable_if_t<is_iter_v<T> && !is_pointer_v<T>>> {
    static void print(const T& t) {
        address_printer<T>::print(t);
    }
    static void print_feature(const T& t) {
        using raw_type = remove_cvref_t<decltype(*t)>;
        std::cout << "@" << _MSTL addressof(t) << "(" << check_type<T>() << " ->";
        printer<raw_type>::print_feature(*t);
        std::cout << ")";
    }
};


template <typename T>
struct printer<T, enable_if_t<is_member_object_pointer_v<T>>> {
    static void print(const T& t) {
        if (t == nullptr) {
            printer<nullptr_t>::print(nullptr);
            return;
        }
        address_printer<T>::print(t);
    }
    static void print_feature(const T& t) {
        if (t == nullptr) {
            printer<nullptr_t>::print(nullptr);
            return;
        }
        address_printer<T>::print_feature(t);
    }
};

template <typename T>
struct printer<T, enable_if_t<is_member_function_pointer_v<T>>> {
    static void print(const T& t) {
        if (t == nullptr) {
            printer<nullptr_t>::print(nullptr);
            return;
        }
        address_printer<T>::print(t);
    }
    static void print_feature(const T& t) {
        if (t == nullptr) {
            printer<nullptr_t>::print(nullptr);
            return;
        }
        address_printer<T>::print_feature(t);
    }
};


template <typename T>
struct printer<T, enable_if_t<is_enum_v<T>>> {
    using underlying_type = underlying_type_t<T>;

    static void print(const T& t) {
        std::cout << static_cast<underlying_type>(t);
    }
    static void print_feature(const T& t) {
        printer::print(t);
        std::cout << "(" << check_type<T>() << ": " << check_type<underlying_type>() << ")";
    }
};

template <typename T>
struct printer<T, enable_if_t<is_union_v<T>>> {
    static void print(const T& t) {
        address_printer<T>::print(t);
    }
    static void print_feature(const T& t) {
        address_printer<T>::print_feature(t);
    }
};


template <typename T>
struct printer<T, enable_if_t<is_function_v<T>>> {
    static void print(T& t) {
        std::cout << "@" << _MSTL addressof(t);
    }
    static void print_feature(T& t) {
        printer::print(t);
        std::cout << "(" << check_type<T>() << ")";
    }
};


template <typename T>
struct printer<T, enable_if_t<is_base_of_v<_MSTL Error, T>>> {
    static void print(const T& t) {
        std::cout << t.type_ << "(" << t.info_ << ")";
    }
    static void print_feature(const T& t) {
        std::cout << t.type_ << "(";
        printer<decltype(t.info_)>::print_feature(t.info_);
        std::cout << ")";
    }
};


template <typename T>
struct printer<hash<T>> {
    static void print(const hash<T>&) {
        std::cout << check_type<hash<T>>();
    }
    static void print_feature(const hash<T>& t) {
        printer::print(t);
    }
};


template <typename IfEmpty, typename T>
struct printer<compressed_pair<IfEmpty, T, true>> {
    static void print(const compressed_pair<IfEmpty, T, true>& t) {
        printer<T>::print(t.value);
    }
    static void print_feature(const compressed_pair<IfEmpty, T, true>& t) {
        std::cout << "{ ";
        printer<T>::print_feature(t.value);
        std::cout << ", (compressed) }";
    }
};

template <typename IfEmpty, typename T>
struct printer<compressed_pair<IfEmpty, T, false>> {
    static void print(const compressed_pair<IfEmpty, T, false>& t) {
        std::cout << "{ ";
        printer<remove_cvref_t<T>>::print(t.value);
        std::cout << ", ";
        printer<remove_cvref_t<IfEmpty>>::print(t.no_compressed);
        std::cout << " }";
    }
    static void print_feature(const compressed_pair<IfEmpty, T, false>& t) {
        std::cout << "{ ";
        printer<remove_cvref_t<T>>::print_feature(t.value);
        std::cout << ", ";
        printer<remove_cvref_t<IfEmpty>>::print_feature(t.no_compressed);
        std::cout << "(no_compressed) }";
    }
};


template <typename T1, typename T2>
struct printer<pair<T1, T2>> {
    static void print(const pair<T1, T2>& t) {
        std::cout << "{ ";
        printer<remove_cvref_t<T1>>::print(t.first);
        std::cout << ", ";
        printer<remove_cvref_t<T2>>::print(t.second);
        std::cout << " }";
    }
    static void print_feature(const pair<T1, T2>& t) {
        std::cout << "{ ";
        printer<remove_cvref_t<T1>>::print_feature(t.first);
        std::cout << ", ";
        printer<remove_cvref_t<T2>>::print_feature(t.second);
        std::cout << " }";
    }
};


MSTL_BEGIN_INNER__

template <typename Tuple, size_t I,
    enable_if_t<I == tuple_size_v<Tuple> - 1, int> = 0>
void __print_tuple_elements(const Tuple& t) {
    using type = remove_cvref_t<decltype(_MSTL get<I>(t))>;
    printer<type>::print(_MSTL get<I>(t));
}

template <typename Tuple, size_t I,
    enable_if_t<I < tuple_size_v<Tuple> - 1, int> = 0>
void __print_tuple_elements(const Tuple& t) {
    using type = remove_cvref_t<decltype(_MSTL get<I>(t))>;
    printer<type>::print(_MSTL get<I>(t));
    std::cout << ", ";
    _INNER __print_tuple_elements<Tuple, I + 1>(t);
}

template <typename Tuple, size_t I,
    enable_if_t<I == tuple_size_v<Tuple> - 1, int> = 0>
void __print_tuple_elements_feature(const Tuple& t) {
    using type = remove_cvref_t<decltype(_MSTL get<I>(t))>;
    printer<type>::print_feature(_MSTL get<I>(t));
}

template <typename Tuple, size_t I,
    enable_if_t<I < tuple_size_v<Tuple> - 1, int> = 0>
void __print_tuple_elements_feature(const Tuple& t) {
    using type = remove_cvref_t<decltype(_MSTL get<I>(t))>;
    printer<type>::print_feature(_MSTL get<I>(t));
    std::cout << ", ";
    _INNER __print_tuple_elements_feature<Tuple, I + 1>(t);
}

MSTL_END_INNER__


template <typename... Args>
struct printer<_MSTL tuple<Args...>> {
private:
    template <typename... UArgs, enable_if_t<sizeof...(UArgs) == 0, int> = 0>
    static void __print_tuple_dispatch(const tuple<UArgs...>&) {
        std::cout << "()";
    }

    template <typename... UArgs, enable_if_t<sizeof...(UArgs) != 0, int> = 0>
    static void __print_tuple_dispatch(const tuple<UArgs...>& t) {
        std::cout << "( ";
        _INNER __print_tuple_elements<decltype(t), 0>(t);
        std::cout << " )";
    }

    template <typename... UArgs, enable_if_t<sizeof...(UArgs) == 0, int> = 0>
    static void __print_tuple_feature_dispatch(const tuple<UArgs...>&) {
        std::cout << "()(" << check_type<tuple<Args...>>() << ")";
    }

    template <typename... UArgs, enable_if_t<sizeof...(UArgs) != 0, int> = 0>
    static void __print_tuple_feature_dispatch(const tuple<UArgs...>& t) {
        std::cout << "( ";
        _INNER __print_tuple_elements_feature<decltype(t), 0>(t);
        std::cout << " )(" << check_type<tuple<Args...>>() << ")";
    }

public:
    static void print(const _MSTL tuple<Args...>& t) {
        printer::__print_tuple_dispatch(t);
    }

    static void print_feature(const _MSTL tuple<Args...>& t) {
        printer::__print_tuple_feature_dispatch(t);
    }
};


template <>
struct printer<_MSTL nullopt_t> {
    static void print(const _MSTL nullopt_t&) {
        std::cout << "nullopt";
    }
    static void print_feature(const _MSTL nullopt_t& t) {
        printer::print(t);
    }
};

template <typename T>
struct printer<_MSTL optional<T>> {
    static void print(const _MSTL optional<T>& t) {
        if (t.has_value()) {
            printer<T>::print(*t);
        }
        else {
            printer<_MSTL nullopt_t>::print(_MSTL nullopt);
        }
    }

    static void print_feature(const _MSTL optional<T>& t) {
        if (t.has_value()) {
            printer<T>::print_feature(*t);
            std::cout << "(optional)";
        }
        else {
            printer<_MSTL nullopt_t>::print_feature(_MSTL nullopt);
        }
    }
};

template <>
struct printer<_MSTL any> {
    static void print(const _MSTL any& t) {
        if (t.has_value()) {
            std::cout << t.type().name();
        }
    }
    static void print_feature(const _MSTL any& t) {
        if (t.has_value()) {
            std::cout << t.type().name() << "(any)";
        }
        else {
            std::cout << "(empty any)";
        }
    }
};


template <typename T>
struct printer<shared_ptr<T>> {
    static void print(const shared_ptr<T>& t) {
        if (!t) {
            printer<nullptr_t>::print(nullptr);
            return;
        }
        std::cout << "@" << _MSTL addressof(t) <<
            "(hold@" << _MSTL addressof(*t.get()) << ")";
    }

    static void print_feature(const shared_ptr<T>& t) {
        if (!t) {
            printer<nullptr_t>::print(nullptr);
            return;
        }
        using raw_type = remove_cvref_t<decltype(*t.get())>;
        std::cout << "@" << _MSTL addressof(t) <<
            "(" << check_type<shared_ptr<T>>() << " hold@" <<
            _MSTL addressof(*t.get()) << "(count=";
        printer<decltype(t.use_count())>::print_feature(t.use_count());
        std::cout << ") ->";
        printer<raw_type>::print_feature(*t.get());
        std::cout << ")";
    }
};

template <typename T, typename Deleter>
struct printer<unique_ptr<T, Deleter>> {
    static void print(const unique_ptr<T, Deleter>& t) {
        if (!t) {
            printer<nullptr_t>::print(nullptr);
            return;
        }
        std::cout << "@" << _MSTL addressof(t) <<
            "(hold@" << _MSTL addressof(*t.get()) << ")";
    }

    static void print_feature(const unique_ptr<T, Deleter>& t) {
        if (!t) {
            printer<nullptr_t>::print(nullptr);
            return;
        }
        using raw_type = remove_cvref_t<decltype(*t.get())>;
        std::cout << "@" << _MSTL addressof(t) <<
            "(" << check_type<unique_ptr<T, Deleter>>() <<
            " hold@" << _MSTL addressof(*t.get());
        std::cout << " ->";
        printer<raw_type>::print_feature(*t.get());
        std::cout << ")";
    }
};


template <typename... Types>
struct printer<_MSTL variant<Types...>> {
    static void print(const _MSTL variant<Types...>& t) {
        _MSTL visit([] (auto const &v) {
            printer<remove_cvref_t<decltype(v)>>::print(v);
        }, t);
    }

    static void print_feature(const _MSTL variant<Types...>& t) {
        _MSTL visit([] (auto const &v) {
            printer<remove_cvref_t<decltype(v)>>::print_feature(v);
        }, t);
        std::cout << "(" << check_type<variant<Types...>>() << ")";
    }
};


template <typename CharT>
struct raw_string_printer {
    static void print(const CharT* t) {
        std::cout << t;
    }
    static void print_feature(const CharT* t) {
        raw_string_printer::print(t);
    }
};

template <>
struct raw_string_printer<char> {
    static void print(const char* t) {
        std::cout << t;
    }

    static void print_feature(const char* t) {
        std::cout << "\"";
        for (const char* p = t; *p != '\0'; ++p) {
            const auto uc = static_cast<byte_t>(*p);
            switch (uc) {
                case '\n': std::cout << "\\n"; break;
                case '\t': std::cout << "\\t"; break;
                case '\r': std::cout << "\\r"; break;
                case '\b': std::cout << "\\b"; break;
                case '\v': std::cout << "\\v"; break;
                case '\f': std::cout << "\\f"; break;
                case '\'': std::cout << "\\\'"; break;
                case '\"': std::cout << "\\\""; break;
                case '\\': std::cout << "\\\\"; break;
                default:   std::cout << *p;
            }
        }
        std::cout << "\"";
    }
};

template <>
struct raw_string_printer<wchar_t> {
    static void print(const wchar_t* t) {
        string utf8_str = _MSTL move(_MSTL wstring_to_utf8(t));
        raw_string_printer<char>::print(utf8_str.c_str());
    }
    static void print_feature(const wchar_t* t) {
        string utf8_str = _MSTL move(_MSTL wstring_to_utf8(t));
        std::cout << "L";
        raw_string_printer<char>::print_feature(utf8_str.c_str());
    }
};

#ifdef MSTL_VERSION_20__
template <>
struct raw_string_printer<char8_t> {
    static void print(const char8_t* t) {
        string utf8_str = _MSTL move(_MSTL u8string_to_utf8(t));
        raw_string_printer<char>::print(utf8_str.c_str());
    }
    static void print_feature(const char8_t* t) {
        string utf8_str = _MSTL move(_MSTL u8string_to_utf8(t));
        std::cout << "U8";
        raw_string_printer<char>::print_feature(utf8_str.c_str());
    }
};
#endif

template <>
struct raw_string_printer<char16_t> {
    static void print(const char16_t* t) {
        string utf8_str = _MSTL move(_MSTL u16string_to_utf8(t));
        raw_string_printer<char>::print(utf8_str.c_str());
    }
    static void print_feature(const char16_t* t) {
        string utf8_str = _MSTL move(_MSTL u16string_to_utf8(t));
        std::cout << "U16";
        raw_string_printer<char>::print_feature(utf8_str.c_str());
    }
};

template <>
struct raw_string_printer<char32_t> {
    static void print(const char32_t* t) {
        string utf8_str = _MSTL move(_MSTL u32string_to_utf8(t));
        raw_string_printer<char>::print(utf8_str.c_str());
    }
    static void print_feature(const char32_t* t) {
        string utf8_str = _MSTL move(_MSTL u32string_to_utf8(t));
        std::cout << "U32";
        raw_string_printer<char>::print_feature(utf8_str.c_str());
    }
};


template <typename T>
struct printer<T, enable_if_t<is_ctype_string_v<T>>> {
private:
    using char_type = char_of_string_t<T>;

    template <typename U, enable_if_t<is_bounded_array_v<U>, int> = 0>
    static void __print_dispatch(const T& t) {
        raw_string_printer<char_type>::print(&t[0]);
    }
    template <typename U, enable_if_t<!is_bounded_array_v<U>, int> = 0>
    static void __print_dispatch(const T& t) {
        raw_string_printer<char_type>::print(t);
    }

    template <typename U, enable_if_t<is_bounded_array_v<U>, int> = 0>
    static void __print_feature_dispatch(const T& t) {
        raw_string_printer<char_type>::print_feature(&t[0]);
    }
    template <typename U, enable_if_t<!is_bounded_array_v<U>, int> = 0>
    static void __print_feature_dispatch(const T& t) {
        raw_string_printer<char_type>::print_feature(t);
    }

public:
    static void print(const T& t) {
        if (t == nullptr) {
            printer<nullptr_t>::print(nullptr);
            return;
        }
        printer::__print_dispatch<T>(t);
    }

    static void print_feature(const T& t) {
        if (t == nullptr) {
            printer<nullptr_t>::print(nullptr);
            return;
        }
        printer::__print_feature_dispatch<T>(t);
    }
};

template <typename CharT, typename Traits>
struct printer<basic_string_view<CharT, Traits>> {
    static void print(const basic_string_view<CharT, Traits>& t) {
        raw_string_printer<CharT>::print(t.data());
    }

    static void print_feature(const basic_string_view<CharT, Traits>& t) {
        raw_string_printer<CharT>::print_feature(t.data());
        std::cout << "sv";
    }
};

template <typename CharT, typename Traits>
std::ostream& operator <<(std::ostream& out, basic_string_view<CharT, Traits> const& t) {
    std::cout << t.data();
    return out;
}

template <typename CharT, typename Traits, typename Alloc>
struct printer<basic_string<CharT, Traits, Alloc>> {
    static void print(const basic_string<CharT, Traits, Alloc>& t) {
        raw_string_printer<CharT>::print(t.data());
    }
    static void print_feature(const basic_string<CharT, Traits, Alloc>& t) {
        raw_string_printer<CharT>::print_feature(t.data());
        std::cout << "s";
    }
};

template <typename CharT, typename Traits, typename Alloc>
std::ostream& operator <<(std::ostream& out, basic_string<CharT, Traits, Alloc> const& t) {
    std::cout << t.data();
    return out;
}

template <typename CharT>
struct printer<basic_istringstream<CharT>> {
    static void print(const basic_istringstream<CharT>& t) {
        raw_string_printer<CharT>::print(t.str().c_str());
    }
    static void print_feature(const basic_istringstream<CharT>& t) {
        raw_string_printer<CharT>::print_feature(t.str().c_str());
        std::cout << "iss";
    }
};

template <typename CharT>
struct printer<basic_ostringstream<CharT>> {
    static void print(const basic_ostringstream<CharT>& t) {
        raw_string_printer<CharT>::print(t.str().c_str());
    }
    static void print_feature(const basic_ostringstream<CharT>& t) {
        raw_string_printer<CharT>::print_feature(t.str().c_str());
        std::cout << "oss";
    }
};

template <typename CharT>
struct printer<basic_stringstream<CharT>> {
    static void print(const basic_stringstream<CharT>& t) {
        raw_string_printer<CharT>::print(t.str().c_str());
    }
    static void print_feature(const basic_stringstream<CharT>& t) {
        raw_string_printer<CharT>::print_feature(t.str().c_str());
        std::cout << "ss";
    }
};


template <typename Res, typename... Args>
struct printer<function<Res(Args...)>> {
    static void print(const function<Res(Args...)>& t) {
        std::cout << check_type<function<Res(Args...)>>(t);
    }
    static void print_feature(const function<Res(Args...)>& t) {
        printer::print(t);
    }
};


template <>
struct printer<file> {
    static void print(const file& t) {
        std::cout << check_type<file>();
    }
    static void print_feature(const file& t) {
        std::cout << check_type<file>() << "(" << t.path() << ")";
    }
};


template <typename Container>
struct range_printer {
    using value_type = typename Container::value_type;

    static void print(const Container& t) {
        if (_MSTL empty(t)) {
            std::cout << "[]";
        }
        else {
            std::cout << "[ ";
            for (auto iter = _MSTL cbegin(t); iter != _MSTL cend(t); ++iter) {
                if (iter != _MSTL cbegin(t)) std::cout << ", ";
                _MSTL printer<value_type>::print(*iter);
            }
            std::cout << " ]";
        }
    }

    static void print_feature(const Container& t) {
        if (_MSTL empty(t)) {
            std::cout << "[]";
        }
        else {
            std::cout << "[ ";
            for (auto iter = _MSTL cbegin(t); iter != _MSTL cend(t); ++iter) {
                if (iter != _MSTL cbegin(t)) std::cout << ", ";
                _MSTL printer<value_type>::print_feature(*iter);
            }
            std::cout << " ]";
        }
        std::cout << "(" << check_type<Container>() << ")";
    }
};


template <typename T, size_t Size>
struct printer<array<T, Size>> {
    static void print(const array<T, Size>& t) {
        range_printer<array<T, Size>>::print(t);
    }

    static void print_feature(const array<T, Size>& t) {
        range_printer<array<T, Size>>::print_feature(t);
    }
};

template <typename T>
struct printer<temporary_buffer<T>> {
    using type = temporary_buffer<T>;

    static void print(const type& t) {
        range_printer<type>::print(t);
    }
    static void print_feature(const type& t) {
        range_printer<type>::print_feature(t);
    }
};

template <typename T, typename Alloc>
struct printer<list<T, Alloc>> {
    using type = list<T, Alloc>;

    static void print(const type& t) {
        range_printer<type>::print(t);
    }
    static void print_feature(const type& t) {
        range_printer<type>::print_feature(t);
    }
};

template <typename T, typename Alloc, size_t BufSize>
struct printer<deque<T, Alloc, BufSize>> {
    using type = deque<T, Alloc, BufSize>;

    static void print(const type& t) {
        range_printer<type>::print(t);
    }
    static void print_feature(const type& t) {
        range_printer<type>::print_feature(t);
    }
};

template <>
struct printer<bitmap> {
    using type = bitmap;

    static void print(const type& t) {
        range_printer<type>::print(t);
    }
    static void print_feature(const type& t) {
        range_printer<type>::print_feature(t);
    }
};

template <typename T, typename Alloc>
struct printer<vector<T, Alloc>> {
    using type = vector<T, Alloc>;

    static void print(const type& t) {
        range_printer<type>::print(t);
    }
    static void print_feature(const type& t) {
        range_printer<type>::print_feature(t);
    }
};

template <typename Key, typename Value, typename KeyOfValue,
    typename Compare, typename Alloc>
struct printer<rb_tree<Key, Value, KeyOfValue, Compare, Alloc>> {
    using type = rb_tree<Key, Value, KeyOfValue, Compare, Alloc>;

    static void print(const type& t) {
        range_printer<type>::print(t);
    }
    static void print_feature(const type& t) {
        range_printer<type>::print_feature(t);
    }
};

template <typename T, typename Sequence>
struct printer<queue<T, Sequence>> {
    using type = queue<T, Sequence>;

    static void print(const type& t) {
        range_printer<type>::print(t);
    }
    static void print_feature(const type& t) {
        range_printer<type>::print_feature(t);
    }
};

template <typename T, typename Sequence, typename Compare>
struct printer<priority_queue<T, Sequence, Compare>> {
    using type = priority_queue<T, Sequence, Compare>;

    static void print(const type& t) {
        range_printer<Sequence>::print(t.get_container());
    }
    static void print_feature(const type& t) {
        range_printer<Sequence>::print_feature(t.get_container());
    }
};

template <typename T, typename Sequence>
struct printer<stack<T, Sequence>> {
    using type = stack<T, Sequence>;

    static void print(const type& t) {
        range_printer<Sequence>::print(t.get_container());
    }
    static void print_feature(const type& t) {
        range_printer<Sequence>::print_feature(t.get_container());
    }
};

template <typename Value, typename Key, typename HashFcn,
    typename ExtractKey, typename EqualKey, typename Alloc>
struct printer<hashtable<Value, Key, HashFcn, ExtractKey, EqualKey, Alloc>> {
    using type = hashtable<Value, Key, HashFcn, ExtractKey, EqualKey, Alloc>;

    static void print(const type& t) {
        range_printer<type>::print(t);
    }
    static void print_feature(const type& t) {
        range_printer<type>::print_feature(t);
    }
};

template <typename Key, typename T, typename Compare, typename Alloc>
struct printer<map<Key, T, Compare, Alloc>> {
    using type = map<Key, T, Compare, Alloc>;

    static void print(const type& t) {
        range_printer<type>::print(t);
    }
    static void print_feature(const type& t) {
        range_printer<type>::print_feature(t);
    }
};

template <typename Key, typename T, typename Compare, typename Alloc>
struct printer<multimap<Key, T, Compare, Alloc>> {
    using type = multimap<Key, T, Compare, Alloc>;

    static void print(const type& t) {
        range_printer<type>::print(t);
    }
    static void print_feature(const type& t) {
        range_printer<type>::print_feature(t);
    }
};

template <typename Key, typename Compare, typename Alloc>
struct printer<set<Key, Compare, Alloc>> {
    using type = set<Key, Compare, Alloc>;

    static void print(const type& t) {
        range_printer<type>::print(t);
    }
    static void print_feature(const type& t) {
        range_printer<type>::print_feature(t);
    }
};

template <typename Key, typename Compare, typename Alloc>
struct printer<multiset<Key, Compare, Alloc>> {
    using type = multiset<Key, Compare, Alloc>;

    static void print(const type& t) {
        range_printer<type>::print(t);
    }
    static void print_feature(const type& t) {
        range_printer<type>::print_feature(t);
    }
};

template <typename Value, typename Key, typename HashFcn,
    typename EqualKey, typename Alloc>
struct printer<unordered_map<Value, Key, HashFcn, EqualKey, Alloc>> {
    using type = unordered_map<Value, Key, HashFcn, EqualKey, Alloc>;

    static void print(const type& t) {
        range_printer<type>::print(t);
    }
    static void print_feature(const type& t) {
        range_printer<type>::print_feature(t);
    }
};

template <typename Value, typename HashFcn, typename EqualKey, typename Alloc>
struct printer<unordered_set<Value, HashFcn, EqualKey, Alloc>> {
    using type = unordered_set<Value, HashFcn, EqualKey, Alloc>;

    static void print(const type& t) {
        range_printer<type>::print(t);
    }
    static void print_feature(const type& t) {
        range_printer<type>::print_feature(t);
    }
};

template <typename Value, typename Key, typename HashFcn,
    typename EqualKey, typename Alloc>
struct printer<unordered_multimap<Value, Key, HashFcn, EqualKey, Alloc>> {
    using type = unordered_multimap<Value, Key, HashFcn, EqualKey, Alloc>;

    static void print(const type& t) {
        range_printer<type>::print(t);
    }
    static void print_feature(const type& t) {
        range_printer<type>::print_feature(t);
    }
};

template <typename Value, typename HashFcn, typename EqualKey, typename Alloc>
struct printer<unordered_multiset<Value, HashFcn, EqualKey, Alloc>> {
    using type = unordered_multiset<Value, HashFcn, EqualKey, Alloc>;

    static void print(const type& t) {
        range_printer<type>::print(t);
    }
    static void print_feature(const type& t) {
        range_printer<type>::print_feature(t);
    }
};


template <>
struct printer<json_value> {
private:
    static void print_json(const json_value* value, int indent = 0) {
        if (!value) {
            std::cout << "null";
            return;
        }
        switch (value->type()) {
            case json_value::Null:
                std::cout << "null";
                break;
            case json_value::Bool:
                printer<bool>::print(value->as_bool()->get_value());
                break;
            case json_value::Number:
                printer<double>::print(value->as_number()->get_value());
                break;
            case json_value::String:
                std::cout << "\"";
                printer<string>::print(value->as_string()->get_value());
                std::cout << "\"";
                break;
            case json_value::Array: {
                const json_array* array = value->as_array();
                std::cout << "[\n";
                for (size_t i = 0; i < array->size(); ++i) {
                    std::cout << string(indent + 2, ' ');
                    print_json(array->get_element(i), indent + 2);
                    if (i < array->size() - 1) {
                        std::cout << ",";
                    }
                    std::cout << "\n";
                }
                std::cout << string(indent, ' ') << "]";
                break;
            }
            case json_value::Object: {
                const json_object* object = value->as_object();
                std::cout << "{\n";
                auto& members = object->get_members();
                size_t count = 0;
                for (auto& pair : members) {
                    std::cout << string(indent + 2, ' ') << "\"" << pair.first << "\": ";
                    print_json(pair.second.get(), indent + 2);
                    if (count < members.size() - 1) {
                        std::cout << ",";
                    }
                    std::cout << "\n";
                    count++;
                }
                std::cout << string(indent, ' ') << "}";
                break;
            }
        }
    }

public:
    static void print(const json_value& value) {
        printer::print_json(&value);
    }
    static void print_feature(const json_value& value) {
        printer::print_json(&value);
    }
};


template <>
struct printer<date> {
    static void print(const date& t) {
        std::cout << t.to_string();
    }
    static void print_feature(const date& t) {
        std::cout << t.to_string() << "(date)";
    }
};

template <>
struct printer<time> {
    static void print(const time& t) {
        std::cout << t.to_string();
    }
    static void print_feature(const time& t) {
        std::cout << t.to_string() << "(time)";
    }
};

template <>
struct printer<datetime> {
    static void print(const datetime& t) {
        std::cout << t.to_string();
    }
    static void print_feature(const datetime& t) {
        std::cout << t.to_string() << "(datetime)";
    }
};

template <>
struct printer<timestamp> {
    static void print(const timestamp& t) {
        std::cout << t.get_seconds();
    }
    static void print_feature(const timestamp& t) {
        std::cout << t.get_seconds() << "(timestamp)";
    }
};

template <>
struct printer<hexadecimal> {
    static void print(const hexadecimal& t) {
        std::cout << t.to_decimal();
    }
    static void print_feature(const hexadecimal& t) {
        std::cout << t.to_string();
    }
};

template <>
struct printer<session> {
    static void print(session const& t) {
        std::cout << "Session ID: [ " << t.get_id() << " ]" << " Data: ";
        printer<unordered_map<string, string>>::print(t.get_data());
    }
    static void print_feature(session const& t) {
        printer::print(t);
    }
};


#ifdef MSTL_COMPILER_MSVC__
// function can not be passed with cv qualifiers
#pragma warning(disable: 4180)
#endif

#ifndef MSTL_VERSION_17__
MSTL_BEGIN_INNER__

inline void __print_single() {
    std::cout.flush();
}
template <typename Last>
void __print_single(Last&& last) {
    printer<remove_cvref_t<Last>>::print(_MSTL forward<Last>(last));
}
template <typename First, typename Second, typename... Rest>
void __print_single(First&& first, Second&& second, Rest&&... rest) {
    printer<remove_cvref_t<First>>::print(_MSTL forward<First>(first));
    std::cout << " ";
    __print_single(_MSTL forward<Second>(second), _MSTL forward<Rest>(rest)...);
}

inline void __print_feature_single() {
    std::cout.flush();
}
template <typename Last>
void __print_feature_single(Last&& last) {
    printer<remove_cvref_t<Last>>::print_feature(_MSTL forward<Last>(last));
}
template <typename First, typename Second, typename... Rest>
void __print_feature_single(First&& first, Second&& second, Rest&&... rest) {
    printer<remove_cvref_t<First>>::print_feature(_MSTL forward<First>(first));
    std::cout << " ";
    __print_feature_single(_MSTL forward<Second>(second), _MSTL forward<Rest>(rest)...);
}

MSTL_END_INNER__


template <typename... Args>
void print(Args&&... args) {
    _INNER __print_single(_MSTL forward<Args>(args)...);
}

template <typename... Args>
void println(Args&&... args) {
    _INNER __print_single(_MSTL forward<Args>(args)...);
    std::cout << "\n";
}

template <typename... Args>
void print_feature(Args&&... args) {
    _INNER __print_feature_single(_MSTL forward<Args>(args)...);
}

template <typename... Args>
void println_feature(Args&&... args) {
    _INNER __print_feature_single(_MSTL forward<Args>(args)...);
    std::cout << "\n";
}

#else

template <typename This, typename ...Rests>
void print(const This& t, const Rests&... r) {
    printer<remove_cvref_t<This>>::print(t);
    ((std::cout << " ", printer<remove_cvref_t<Rests>>::print(r)), ...);
}

inline void print() {
    std::cout.flush();
}

template <typename This, typename ...Rests>
void println(const This& t, const Rests&... r) {
    printer<remove_cvref_t<This>>::print(t);
    ((std::cout << " ", printer<remove_cvref_t<Rests>>::print(r)), ...);
    std::cout << "\n";
}

inline void println() {
    std::cout << "\n";
    std::cout.flush();
}

template <typename This, typename ...Rests>
void print_feature(const This& t, const Rests&... r) {
    printer<remove_cvref_t<This>>::print_feature(t);
    ((std::cout << " ", printer<remove_cvref_t<Rests>>::print_feature(r)), ...);
}

inline void print_feature() {
    std::cout.flush();
}

template <typename This, typename ...Rests>
void println_feature(const This& t, const Rests&... r) {
    printer<remove_cvref_t<This>>::print_feature(t);
    ((std::cout << " ", printer<remove_cvref_t<Rests>>::print_feature(r)), ...);
    std::cout << "\n";
}

inline void println_feature() {
    std::cout << "\n";
    std::cout.flush();
}

#endif

#ifdef MSTL_COMPILER_MSVC__
#pragma warning(pop)
#endif

MSTL_END_NAMESPACE__
#endif // MSTL_PRINT_HPP__
