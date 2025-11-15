#ifndef MSTL_VARIANT_HPP__
#define MSTL_VARIANT_HPP__
#include "serialize.hpp"
#include "undef_cmacro.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename, typename>
struct variant_index;
template <typename Variant, typename T>
constexpr size_t variant_index_v = variant_index<Variant, T>::value;

template <typename, size_t>
struct variant_alternative;
template <typename Variant, size_t Idx>
using variant_alternative_t = typename variant_alternative<Variant, Idx>::type;

template <typename... Types>
struct variant : iobject<variant<Types...>> {
private:
    using self = variant<Types...>;

    size_t index_;

    alignas(_MSTL max({ alignof(Types)... })) char union_[_MSTL max({ sizeof(Types)... })]{};

    using destruct_function = void(*)(char*);

    static destruct_function* destructors_table() noexcept {
        static destruct_function function_ptrs[sizeof...(Types)] = {
            [](char* union_p) noexcept {
                reinterpret_cast<Types*>(union_p)->~Types();
            }...
        };
        return function_ptrs;
    }

    using copy_construct_function = void(*)(char*, char const*);

    static copy_construct_function* copy_constructors_table() noexcept {
        static copy_construct_function function_ptrs[sizeof...(Types)] = {
            [](char* union_dst, char const* union_src) noexcept {
                new (union_dst) Types(*reinterpret_cast<Types const*>(union_src));
            }...
        };
        return function_ptrs;
    }

    using copy_assignment_function = void(*)(char*, char const*);

    static copy_assignment_function* copy_assigment_functions_table() noexcept {
        static copy_assignment_function function_ptrs[sizeof...(Types)] = {
            [](char* union_dst, char const* union_src) noexcept {
                *reinterpret_cast<Types*>(union_dst) = *reinterpret_cast<Types const*>(union_src);
            }...
        };
        return function_ptrs;
    }

    using move_construct_function = void(*)(char*, char*);

    static move_construct_function* move_constructors_table() noexcept {
        static move_construct_function function_ptrs[sizeof...(Types)] = {
            [](char* union_dst, const char* union_src) noexcept {
                new (union_dst) Types(_MSTL move(*reinterpret_cast<Types const*>(union_src)));
            }...
        };
        return function_ptrs;
    }

    using move_assignment_function = void(*)(char*, char*);

    static move_assignment_function* move_assigment_functions_table() noexcept {
        static move_assignment_function function_ptrs[sizeof...(Types)] = {
            [](char* union_dst, const char* union_src) noexcept {
                *reinterpret_cast<Types*>(union_dst) = _MSTL move(*reinterpret_cast<Types*>(union_src));
            }...
        };
        return function_ptrs;
    }

    template <typename Lambda>
    using const_visitor_function = common_type_t<
        _MSTL invoke_result_t<Lambda, Types const&>...>(*)(char const*, Lambda&&);

    template <typename Lambda>
    static const_visitor_function<Lambda>* const_visitors_table() noexcept {
        static const_visitor_function<Lambda> function_ptrs[sizeof...(Types)] = {
            [](char const* union_p, Lambda&& lambda) -> _MSTL invoke_result_t<Lambda, Types const&> {
                return _MSTL invoke(_MSTL forward<Lambda>(lambda), *reinterpret_cast<Types const*>(union_p));
            }...
        };
        return function_ptrs;
    }

    template <typename Lambda>
    using visitor_function = common_type_t<_MSTL invoke_result_t<Lambda, Types&>...>(*)(char*, Lambda&&);

    template <typename Lambda>
    static visitor_function<Lambda>* visitors_table() noexcept {
        static visitor_function<Lambda> function_ptrs[sizeof...(Types)] = {
            [](char* union_p, Lambda&& lambda) -> common_type_t<_MSTL invoke_result_t<Lambda, Types&>...> {
                return _MSTL invoke(_MSTL forward<Lambda>(lambda), *reinterpret_cast<Types*>(union_p));
            }...
        };
        return function_ptrs;
    }

    template <size_t I, typename... Args>
    constexpr bool try_construct_impl(Args&&... args) {
        MSTL_IF_CONSTEXPR (I < sizeof...(Types)) {
            MSTL_IF_CONSTEXPR (is_constructible_v<variant_alternative_t<variant, I>, Args...>) {
                index_ = I;
                new (union_) variant_alternative_t<variant, I>(_MSTL forward<Args>(args)...);
                return true;
            } else {
                return try_construct_impl<I + 1>(_MSTL forward<Args>(args)...);
            }
        }
        return false;
    }

    template <size_t I = 0, typename... Args>
    constexpr bool try_construct(Args&&... args) {
        return try_construct_impl<I>(_MSTL forward<Args>(args)...);
    }

public:
    MSTL_CONSTEXPR20 variant()
    noexcept(is_nothrow_default_constructible_v<variant_alternative_t<variant, 0>>)
    : index_(0) {
        new (union_) variant_alternative_t<variant, 0>();
    }

    template <typename T, enable_if_t<disjunction_v<is_same<T, Types>...>, int> = 0>
    MSTL_CONSTEXPR20 explicit variant(T&& value)
    noexcept(is_nothrow_move_constructible_v<T>)
    : index_(variant_index_v<variant, T>) {
        T* p = reinterpret_cast<T*>(union_);
        new (p) T(_MSTL forward<T>(value));
    }

    template <typename T, enable_if_t<disjunction_v<is_same<T, Types>...>, int> = 0>
    MSTL_CONSTEXPR20 explicit variant(const T& value)
    noexcept(is_nothrow_copy_constructible_v<T>)
    : index_(variant_index_v<variant, T>) {
        T* p = reinterpret_cast<T*>(union_);
        new (p) T(value);
    }

    MSTL_CONSTEXPR20 variant(const self& x)
    : index_(x.index_) {
        copy_constructors_table()[index()](union_, x.union_);
    }
    MSTL_CONSTEXPR20 self& operator =(const self& x) {
        if(_MSTL addressof(x) == this) return *this;
        index_ = x.index_;
        copy_assigment_functions_table()[index()](union_, x.union_);
        return *this;
    }

    MSTL_CONSTEXPR20 variant(self&& x) noexcept : index_(x.index_) {
        move_constructors_table()[index()](union_, x.union_);
    }
    MSTL_CONSTEXPR20 self& operator =(self&& x) noexcept {
        if(_MSTL addressof(x) == this) return *this;
        index_ = x.index_;
        move_assigment_functions_table()[index()](union_, x.union_);
        return *this;
    }

    template <size_t Idx, typename... Args,
        enable_if_t<is_constructible_v<variant_alternative_t<variant, Idx>, Args...>, int> = 0>
    MSTL_CONSTEXPR20 explicit variant(_MSTL_TAG inplace_construct_tag, Args&&... args)
    noexcept(is_nothrow_constructible_v<variant_alternative_t<variant, Idx>, Args...>)
    : index_(Idx) {
        new (union_) variant_alternative_t<variant, Idx>(_MSTL forward<Args>(args)...);
    }

    template <size_t Idx, typename U, typename... Args,
        enable_if_t<is_constructible_v<variant_alternative_t<variant, Idx>, std::initializer_list<U>&, Args...>, int> = 0>
    MSTL_CONSTEXPR20 explicit variant(_MSTL_TAG inplace_construct_tag, std::initializer_list<U> ilist, Args&&... args)
    noexcept(is_nothrow_constructible_v<variant_alternative_t<variant, Idx>, std::initializer_list<U>&, Args...>)
    : index_(Idx) {
        new (union_) variant_alternative_t<variant, Idx>(ilist, _MSTL forward<Args>(args)...);
    }

#ifdef MSTL_STANDARD_20__
    template <typename... Args, enable_if_t<disjunction_v<is_constructible<Types, Args...>...>, int> = 0>
    variant(Args&&... args) {
        if (!try_construct(_MSTL forward<Args>(args)...)) {
            index_ = 0;
            new (union_) variant_alternative_t<variant, 0>();
        }
    }
#endif

    MSTL_CONSTEXPR20 ~variant() noexcept {
        destructors_table()[index()](union_);
    }

    template <typename Lambda,
        enable_if_t<conjunction_v<is_invocable<Lambda, Types&>...>, int> = 0>
    MSTL_CONSTEXPR20 common_type_t<invoke_result_t<Lambda, Types&>...> visit(Lambda&& lambda)
    noexcept(conjunction_v<is_nothrow_invocable<Lambda, Types&>...>) {
        return visitors_table<Lambda>()[index()](union_, _MSTL forward<Lambda>(lambda));
    }
    template <typename Lambda,
        enable_if_t<conjunction_v<is_invocable<Lambda, const Types&>...>, int> = 0>
    MSTL_CONSTEXPR20 common_type_t<invoke_result_t<Lambda, const Types&>...> visit(Lambda&& lambda) const
    noexcept(conjunction_v<is_nothrow_invocable<Lambda, const Types&>...>) {
        return const_visitors_table<Lambda>()[index()](union_, _MSTL forward<Lambda>(lambda));
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20  size_t index() const noexcept {
        return index_;
    }
    template <typename T, enable_if_t<is_any_of_v<T, Types...>, int> = 0>
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool holds_alternative() const noexcept {
        return variant_index_v<variant, T> == index_;
    }

    template <size_t Idx, enable_if_t<(Idx < sizeof...(Types)), int> = 0>
    MSTL_CONSTEXPR20 variant_alternative_t<variant, Idx>& get() {
        if(index_ != Idx) Exception(ValueError("Template index not match."));
        return *reinterpret_cast<variant_alternative_t<variant, Idx>*>(union_);
    }
    template <typename T>
    MSTL_CONSTEXPR20 T& get() {
        return get<variant_index_v<variant, T>>();
    }
    template <size_t Idx, enable_if_t<(Idx < sizeof...(Types)), int> = 0>
    MSTL_CONSTEXPR20 variant_alternative_t<variant, Idx> const& get() const {
        if(index_ != Idx) Exception(ValueError("Template index not match."));
        return *reinterpret_cast<variant_alternative_t<variant, Idx> const*>(union_);
    }
    template <typename T>
    MSTL_CONSTEXPR20 T const& get() const {
        return get<variant_index_v<variant, T>>();
    }

    template <size_t Idx, enable_if_t<(Idx < sizeof...(Types)), int> = 0>
    MSTL_CONSTEXPR20 variant_alternative_t<variant, Idx>* get_if() noexcept {
        if (index_ != Idx) return nullptr;
        return reinterpret_cast<variant_alternative_t<variant, Idx>*>(union_);
    }
    template <typename T>
    MSTL_CONSTEXPR20 T* get_if() noexcept {
        return get_if<variant_index_v<variant, T>>();
    }
    template <size_t Idx, enable_if_t<(Idx < sizeof...(Types)), int> = 0>
    MSTL_CONSTEXPR20 variant_alternative_t<variant, Idx> const* get_if() const noexcept {
        if (index_ != Idx) return nullptr;
        return reinterpret_cast<variant_alternative_t<variant, Idx> const*>(union_);
    }
    template <typename T>
    MSTL_CONSTEXPR20 T const* get_if() const noexcept {
        return get_if<variant_index_v<variant, T>>();
    }

    template <size_t Idx, typename... Args,
        enable_if_t<(Idx < sizeof...(Types)) && is_constructible_v<variant_alternative_t<variant, Idx>, Args...>, int> = 0>
    MSTL_CONSTEXPR20 void emplace(Args&&... args)
    noexcept(is_nothrow_constructible_v<variant_alternative_t<variant, Idx>, Args...>) {
        destructors_table()[index()](union_);
        index_ = Idx;
        new (union_) variant_alternative_t<variant, Idx>(_MSTL forward<Args>(args)...);
    }

    template <typename T, typename... Args, enable_if_t<is_constructible_v<T, Args...>, int> = 0>
    MSTL_CONSTEXPR20 void emplace(Args&&... args)
    noexcept(is_nothrow_constructible_v<T, Args...>) {
        emplace<variant_index_v<variant, T>>(_MSTL forward<Args>(args)...);
    }

    MSTL_CONSTEXPR20 void swap(variant& x) noexcept {
        if (_MSTL addressof(x) == this) return;

        size_t this_index = index_;
        const size_t other_index = x.index_;
        alignas(_MSTL max({ alignof(Types)... })) char temp_union[_MSTL max({ sizeof(Types)... })];
        move_constructors_table()[this_index](reinterpret_cast<char*>(temp_union), union_);
        destructors_table()[this_index](union_);

        move_constructors_table()[other_index](union_, x.union_);
        destructors_table()[other_index](x.union_);
        move_constructors_table()[this_index](x.union_, reinterpret_cast<char*>(temp_union));
        destructors_table()[this_index](reinterpret_cast<char*>(temp_union));

        index_ = other_index;
        x.index_ = this_index;
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator ==(const variant& rh) const {
        if (index_ != rh.index_) return false;
        return this->visit([&](const auto& value) {
            return rh.visit([&](const auto& other_value) {
                return value == other_value;
            });
        });
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator !=(const variant& rh) const {
        return !(*this == rh);
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator <(const variant& rh) const {
        if (index_ != rh.index_) return false;
        return this->visit([&](const auto& value) {
            return rh.visit([&](const auto& other_value) {
                return value < other_value;
            });
        });
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator >(const variant& rh) const {
        return rh < *this;
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator <=(const variant& rh) const {
        return !(*this > rh);
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator >=(const variant& rh) const {
        return !(*this < rh);
    }

    MSTL_CONSTEXPR20 size_t to_hash() const;
    MSTL_CONSTEXPR20 string to_string() const;
};

template <typename T, typename ...Types>
struct variant_alternative<variant<T, Types...>, 0> {
    using type = T;
};
template <typename T, typename ...Types, size_t Idx>
struct variant_alternative<variant<T, Types...>, Idx> {
    using type = typename variant_alternative<variant<Types...>, Idx - 1>::type;
};

template <typename T, typename ...Types>
struct variant_index<variant<T, Types...>, T> {
    static constexpr size_t value = 0;
};
template <typename T0, typename T, typename ...Types>
struct variant_index<variant<T0, Types...>, T> {
    static constexpr size_t value = variant_index<variant<Types...>, T>::value + 1;
};

template <typename T, typename... Types>
MSTL_CONSTEXPR20 bool holds_alternative(const variant<Types...>& v) noexcept {
    return v.template holds_alternative<T>();
}

template <size_t Idx, typename... Types>
MSTL_CONSTEXPR20 variant_alternative_t<variant<Types...>, Idx>& get(variant<Types...>& v) {
    using T = variant_alternative_t<variant<Types...>, Idx>;
    using variant_type = variant<Types...>;
    return static_cast<T&>(static_cast<variant_type&>(v).template get<Idx>());
}
template <size_t Idx, typename... Types>
MSTL_CONSTEXPR20 const variant_alternative_t<variant<Types...>, Idx>& get(const variant<Types...>& v) {
    using T = variant_alternative_t<variant<Types...>, Idx>;
    using variant_type = variant<Types...>;
    return static_cast<const T&>(static_cast<const variant_type&>(v).template get<Idx>());
}
template <size_t Idx, typename... Types>
MSTL_CONSTEXPR20 variant_alternative_t<variant<Types...>, Idx>&& get(variant<Types...>&& v) {
    using T = variant_alternative_t<variant<Types...>, Idx>;
    using variant_type = variant<Types...>;
    return static_cast<T&&>(static_cast<variant_type&&>(v).template get<Idx>());
}
template <size_t Idx, typename... Types>
MSTL_CONSTEXPR20 const variant_alternative_t<variant<Types...>, Idx>&& get(const variant<Types...>&& v) {
    using T = variant_alternative_t<variant<Types...>, Idx>;
    using variant_type = variant<Types...>;
    return static_cast<const T&&>(static_cast<const variant_type&&>(v).template get<Idx>());
}

template <size_t Idx, typename... Types>
MSTL_CONSTEXPR20 variant_alternative_t<variant<Types...>, Idx>* get_if(variant<Types...>* v) noexcept {
    return v ? v->template get_if<Idx>() : nullptr;
}
template <size_t Idx, typename... Types>
MSTL_CONSTEXPR20 const variant_alternative_t<variant<Types...>, Idx>* get_if(const variant<Types...>* v) noexcept {
    return v ? v->template get_if<Idx>() : nullptr;
}
template <typename T, typename... Types>
MSTL_CONSTEXPR20 T* get_if(variant<Types...>* v) noexcept {
    return v ? v->template get_if<T>() : nullptr;
}
template <typename T, typename... Types>
MSTL_CONSTEXPR20 const T* get_if(const variant<Types...>* v) noexcept {
    return v ? v->template get_if<T>() : nullptr;
}

template <typename Lambda, typename... Types>
MSTL_CONSTEXPR20 decltype(auto) visit(Lambda&& lambda, variant<Types...>& var)
noexcept(noexcept(static_cast<variant<Types...>&>(var).visit(_MSTL forward<Lambda>(lambda)))) {
    using variant_type = variant<Types...>;
    return static_cast<variant_type&>(var).visit(_MSTL forward<Lambda>(lambda));
}
template <typename Lambda, typename... Types>
MSTL_CONSTEXPR20 decltype(auto) visit(Lambda&& lambda, const variant<Types...>& var)
noexcept(noexcept(static_cast<const variant<Types...>&>(var).visit(_MSTL forward<Lambda>(lambda)))) {
    using variant_type = variant<Types...>;
    return static_cast<const variant_type&>(var).visit(_MSTL forward<Lambda>(lambda));
}
template <typename Lambda, typename... Types>
MSTL_CONSTEXPR20 decltype(auto) visit(Lambda&& lambda, variant<Types...>&& var)
noexcept(noexcept(static_cast<variant<Types...>&&>(var).visit(_MSTL forward<Lambda>(lambda)))) {
    using variant_type = variant<Types...>;
    return static_cast<variant_type&&>(var).visit(_MSTL forward<Lambda>(lambda));
}
template <typename Lambda, typename... Types>
MSTL_CONSTEXPR20 decltype(auto) visit(Lambda&& lambda, const variant<Types...>&& var)
noexcept(noexcept(static_cast<const variant<Types...>&&>(var).visit(_MSTL forward<Lambda>(lambda)))){
    using variant_type = variant<Types...>;
    return static_cast<const variant_type&&>(var).visit(_MSTL forward<Lambda>(lambda));
}

MSTL_BEGIN_INNER__
struct __variant_elem_hasher {
    template <typename T>
    constexpr size_t operator ()(const T& value) const {
        return hash<decay_t<T>>{}(value);
    }
};
MSTL_END_INNER__

template <typename... Types>
MSTL_CONSTEXPR20 size_t variant<Types...>::to_hash() const {
    constexpr _INNER __variant_elem_hasher hasher{};
    return this->visit(hasher);
}

template <typename... Types>
MSTL_CONSTEXPR20 string variant<Types...>::to_string() const {
    string result;
    _MSTL visit([&result] (auto const &v) {
        result += _MSTL to_string(v);
    }, *this);
    return result;
}

MSTL_END_NAMESPACE__
#endif // MSTL_VARIANT_HPP__
