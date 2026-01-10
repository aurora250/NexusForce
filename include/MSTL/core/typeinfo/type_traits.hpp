#ifndef MSTL_CORE_TYPEINFO_TYPE_TRAITS_HPP__
#define MSTL_CORE_TYPEINFO_TYPE_TRAITS_HPP__
#include "../typeinfo/types.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename T, T Value>
struct integral_constant {
    static constexpr T value = Value;

    using value_type    = T;
    using type          = integral_constant<T, Value>;

    constexpr explicit operator value_type() const noexcept {
        return value;
    }
    MSTL_NODISCARD constexpr value_type operator()() const noexcept {
        return value;
    }
};


template <bool Value>
using bool_constant = integral_constant<bool, Value>;

using true_type = bool_constant<true>;
using false_type = bool_constant<false>;


template <int16_t Value>
using int16_constant = integral_constant<int16_t, Value>;
template <uint16_t Value>
using uint16_constant = integral_constant<uint16_t, Value>;

template <int32_t Value>
using int32_constant = integral_constant<int32_t, Value>;
template <uint32_t Value>
using uint32_constant = integral_constant<uint32_t, Value>;

template <int64_t Value>
using int64_constant = integral_constant<int64_t, Value>;
template <uint64_t Value>
using uint64_constant = integral_constant<uint64_t, Value>;


// Test is false, SFINAF
template <bool Test, typename T = void>
struct enable_if {};
// Test is true
template <typename T>
struct enable_if<true, T> {
    using type = T;
};
template <bool Test, typename T = void>
using enable_if_t = typename enable_if<Test, T>::type;


// Test is true, using type = T1
template <bool Test, typename T1, typename T2>
struct conditional {
    using type = T1;
};
// Test is false, using type = T2
template <typename T1, typename T2>
struct conditional<false, T1, T2> {
    using type = T2;
};
template <bool Test, typename T1, typename T2>
using conditional_t = typename conditional<Test, T1, T2>::type;


template <typename T>
struct negation : bool_constant<!static_cast<bool>(T::value)> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool negation_v = negation<T>::value;
#endif


template <typename, typename>
struct is_same : false_type {};
template <typename T>
struct is_same<T, T> : true_type {};

#ifdef MSTL_STANDARD_14__
template <typename T1, typename T2>
MSTL_INLINE17 constexpr bool is_same_v = is_same<T1, T2>::value;
#endif


template <typename T>
struct type_identity {
    using type = T;
};
template <typename T>
using type_identity_t = typename type_identity<T>::type;


#ifdef MSTL_STANDARD_17__
template <typename T, typename... Types>
MSTL_INLINE17 constexpr bool is_any_of_v = (is_same_v<T, Types> || ...);
template <typename T, typename... Types>
struct is_any_of : bool_constant<is_any_of_v<T, Types...>> {};
#else

template <typename T, typename... Types>
struct is_any_of : false_type {};
template <typename T, typename U>
struct is_any_of<T, U> : is_same<T, U> {};

template <typename T, typename U, typename... Types>
struct is_any_of<T, U, Types...> 
    : conditional<is_same<T, U>::value, true_type, is_any_of<T, Types...>>::type {};

#ifdef MSTL_STANDARD_14__
template <typename T, typename... Types>
MSTL_INLINE17 constexpr bool is_any_of_v = is_any_of<T, Types...>::value;
#endif

#endif // MSTL_STANDARD_17__


MSTL_BEGIN_INNER__
// Test is true, stop recursion and set type
template <bool, typename first, typename...>
struct __disjunction_aux {
    using type = first;
};
// Curr`s Test is false, continue checking next
template <typename Curr, typename Next, typename... Rest>
struct __disjunction_aux<false, Curr, Next, Rest...> {
    using type = typename __disjunction_aux<static_cast<bool>(Next::value), Next, Rest...>::type;
};
MSTL_END_INNER__

// recursion end
template <typename...>
struct disjunction : false_type {};
// start recursion
template <typename First, typename... Rest>
struct disjunction<First, Rest...>
    : _INNER __disjunction_aux<static_cast<bool>(First::value), First, Rest...>::type {
};

#ifdef MSTL_STANDARD_14__
template <typename... Args>
MSTL_INLINE17 constexpr bool disjunction_v = disjunction<Args...>::value;
#endif


MSTL_BEGIN_INNER__
// Test is false, stop recursion and set type
template <bool, typename First, typename...>
struct __conjunction_aux {
    using type = First;
};
// Curr`s Test is true, continue checking next
template <typename Curr, typename Next, typename... Rest>
struct __conjunction_aux<true, Curr, Next, Rest...> {
    using type = typename __conjunction_aux<static_cast<bool>(Next::value), Next, Rest...>::type;
};
MSTL_END_INNER__

// recursion finished
template <typename...>
struct conjunction : true_type {};
// start recursion
template <typename First, typename... Rest>
struct conjunction<First, Rest...>
    : _INNER __conjunction_aux<static_cast<bool>(First::value), First, Rest...>::type {
};

#ifdef MSTL_STANDARD_14__
template <typename... Args>
MSTL_INLINE17 constexpr bool conjunction_v = conjunction<Args...>::value;
#endif


template <typename T>
struct remove_const {
    using type = T;
};
template <typename T>
struct remove_const<const T> {
    using type = T;
};
template <typename T>
using remove_const_t = typename remove_const<T>::type;


template <typename T>
struct remove_volatile {
    using type = T;
};
template <typename T>
struct remove_volatile<volatile T> {
    using type = T;
};
template <typename T>
using remove_volatile_t = typename remove_volatile<T>::type;


template <typename T>
struct remove_cv {
    using type = T;

    template <typename wrapper>
    using bind_cv_t = wrapper;
};
template <typename T>
struct remove_cv<const T> {
    using type = T;

    template <typename wrapper>
    using bind_cv_t = const wrapper;
};
template <typename T>
struct remove_cv<volatile T> {
    using type = T;

    template <typename wrapper>
    using bind_cv_t = volatile wrapper;
};
template <typename T>
struct remove_cv<const volatile T> {
    using type = T;

    template <typename wrapper>
    using bind_cv_t = const volatile wrapper;
};
template <typename T>
using remove_cv_t = typename remove_cv<T>::type;
template <typename From, typename To>
using copy_cv_t = typename remove_cv<From>::template bind_cv_t<To>;


template <typename T>
struct remove_reference {
    using type = T;

    template <typename wrapper>
    using bind_ref_t = wrapper;
};
template <typename T>
struct remove_reference<T&> {
    using type = T;

    template <typename wrapper>
    using bind_ref_t = wrapper&;
};
template <typename T>
struct remove_reference<T&&> {
    using type = T;

    template <typename wrapper>
    using bind_ref_t = wrapper&&;
};
template <typename T>
using remove_reference_t = typename remove_reference<T>::type;
template <typename From, typename To>
using copy_ref_t = typename remove_reference<From>::template bind_ref_t<To>;

template <typename From, typename To>
using copy_cvref_t = copy_ref_t<From, copy_cv_t<From, To>>;


template <typename T>
using remove_cvref_t = remove_cv_t<remove_reference_t<T>>;
template <typename T>
struct remove_cvref {
    using type = remove_cvref_t<T>;
};


template <typename T>
struct remove_extent {
    using type = T;
};
template <typename T, size_t Idx>
struct remove_extent<T[Idx]> {
    using type = T;
};
template <typename T>
struct remove_extent<T[]> {
    using type = T;
};

template <typename T>
using remove_extent_t = typename remove_extent<T>::type;


template <typename T>
struct remove_all_extents {
    using type = T;
};
template <typename T, size_t Idx>
struct remove_all_extents<T[Idx]> {
    using type = typename remove_all_extents<T>::type;
};
template <typename T>
struct remove_all_extents<T[]> {
    using type = typename remove_all_extents<T>::type;
};

template <typename T>
using remove_all_extents_t = typename remove_all_extents<T>::type;


template <typename T>
struct remove_pointer {
    using type = T;

    template <typename wrapper>
    using bind_pointer_t = wrapper;
};
template <typename T>
struct remove_pointer<T*> {
    using type = T;

    template <typename wrapper>
    using bind_pointer_t = wrapper*;
};
template <typename T>
struct remove_pointer<T* const> {
    using type = T;

    template <typename wrapper>
    using bind_pointer_t = const wrapper*;
};
template <typename T>
struct remove_pointer<T* volatile> {
    using type = T;

    template <typename wrapper>
    using bind_pointer_t = volatile wrapper*;
};
template <typename T>
struct remove_pointer<T* const volatile> {
    using type = T;

    template <typename wrapper>
    using bind_pointer_t = volatile const wrapper*;
};

template <typename T>
using remove_pointer_t = typename remove_pointer<T>::type;
template <typename From, typename To>
using copy_pointer_t = typename remove_pointer<From>::template bind_pointer_t<To>;


template <typename T>
struct remove_function_qualifiers {
    using type = T;
};
template <typename Ret, typename... Args>
struct remove_function_qualifiers<Ret(Args...) const> {
    using type = Ret(Args...);
};
template <typename Ret, typename... Args>
struct remove_function_qualifiers<Ret(Args...) volatile> {
    using type = Ret(Args...);
};
template <typename Ret, typename... Args>
struct remove_function_qualifiers<Ret(Args...) const volatile> {
    using type = Ret(Args...);
};
#ifdef MSTL_STANDARD_17__
template <typename Ret, typename... Args>
struct remove_function_qualifiers<Ret(Args...) noexcept> {
    using type = Ret(Args...);
};
template <typename Ret, typename... Args>
struct remove_function_qualifiers<Ret(Args...) const noexcept> {
    using type = Ret(Args...);
};
template <typename Ret, typename... Args>
struct remove_function_qualifiers<Ret(Args...) volatile noexcept> {
    using type = Ret(Args...);
};
template <typename Ret, typename... Args>
struct remove_function_qualifiers<Ret(Args...) const volatile noexcept> {
    using type = Ret(Args...);
};
#endif

template <typename T>
using remove_function_qualifiers_t = typename remove_function_qualifiers<T>::type;


MSTL_BEGIN_INNER__
template <typename>
struct __is_void_helper : false_type {};
template <>
struct __is_void_helper<void> : true_type {};
MSTL_END_INNER__

template <typename T>
struct is_void : _INNER __is_void_helper<remove_cv_t<T>>::type {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_void_v = is_void<T>::value;
#endif

template <typename...>
using void_t = void;


template <typename T, typename = void>
struct package {
    using type = T;
};

template <typename T>
using package_t = typename package<T>::type;


template <typename T>
struct is_packaged : bool_constant<!is_same<package_t<T>, T>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_packaged_v = is_packaged<T>::value;
#endif


template <typename T, typename = void>
struct unpackage {
    using type = T;
};

template <typename T>
using unpackage_t = typename unpackage<T>::type;

template <typename T>
using unpack_remove_cvref_t = unpackage_t<remove_cvref_t<T>>;

template <typename T>
struct is_unpackaged : bool_constant<!is_same<unpackage_t<T>, T>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_unpackaged_v = is_unpackaged<T>::value;
#endif


template <typename T>
struct is_character : bool_constant<is_any_of<unpack_remove_cvref_t<T>,
    char, signed char, unsigned char, wchar_t,
#ifdef MSTL_STANDARD_20__
    char8_t,
#endif
    char16_t, char32_t
>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_character_v = is_character<T>::value;
#endif


template <typename T>
struct is_standard_character : bool_constant<is_any_of<unpack_remove_cvref_t<T>,
    char, wchar_t,
#ifdef MSTL_STANDARD_20__
    char8_t,
#endif
    char16_t, char32_t
>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_standard_character_v = is_standard_character<T>::value;
#endif


template <typename T>
struct is_boolean : bool_constant<is_same<unpack_remove_cvref_t<T>, bool>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_boolean_v = is_boolean<T>::value;
#endif


template <typename T>
struct is_standard_integral : bool_constant<is_any_of<unpack_remove_cvref_t<T>,
    short, int, long, long long,
    unsigned short, unsigned int, unsigned long, unsigned long long>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_standard_integer_v = is_standard_integral<T>::value;
#endif


template <typename T>
struct is_integral : bool_constant<disjunction<is_standard_integral<T>, is_character<T>, is_boolean<T>>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_integral_v = is_integral<T>::value;
#endif


template <typename T>
struct is_floating_point : bool_constant<is_any_of<unpack_remove_cvref_t<T>, float, double, long double>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_floating_point_v = is_floating_point<T>::value;
#endif


template <typename T>
struct is_arithmetic : bool_constant<disjunction<is_integral<T>, is_floating_point<T>>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_arithmetic_v = is_arithmetic<T>::value;
#endif


MSTL_BEGIN_INNER__
template <typename T, bool = is_integral<T>::value>
struct __check_sign_aux {
    static constexpr bool is_signed = static_cast<remove_cvref_t<T>>(-1) < static_cast<remove_cv_t<T>>(0);
    static constexpr bool is_unsigned = !is_signed;
};
template <typename T>
struct __check_sign_aux<T, false> {
    static constexpr bool is_signed = is_floating_point<T>::value;
    static constexpr bool is_unsigned = false;
};
MSTL_END_INNER__

template <typename T>
struct is_signed : bool_constant<_INNER __check_sign_aux<unpack_remove_cvref_t<T>>::is_signed> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_signed_v = is_signed<T>::value;
#endif

template <typename T>
struct is_unsigned : bool_constant<_INNER __check_sign_aux<unpack_remove_cvref_t<T>>::is_unsigned> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_unsigned_v = is_unsigned<T>::value;
#endif


template <typename T>
struct add_const {
    using type = const T;
};
template <typename T>
using add_const_t = typename add_const<T>::type;

template <typename T>
MSTL_NODISCARD constexpr add_const_t<T>& as_const(T& val) noexcept {
    return val;
}
template <typename T>
void as_const(const T&&) = delete;


template <typename T>
struct add_volatile {
    using type = volatile T;
};
template <typename T>
using add_volatile_t = typename add_volatile<T>::type;


template <typename T>
struct add_cv {
    using type = const volatile T;
};
template <typename T>
using add_cv_t = typename add_cv<T>::type;


template <typename T, typename = void>
struct add_reference {
    using lvalue = T;
    using rvalue = T;
};
template <typename T>
struct add_reference<T, void_t<T&>> {
    using lvalue = T&;
    using rvalue = T&&;
};

template <typename T>
struct add_lvalue_reference {
    using type = typename add_reference<T>::lvalue;
};
template <typename T>
using add_lvalue_reference_t = typename add_reference<T>::lvalue;

template <typename T>
struct add_rvalue_reference {
    using type = typename add_reference<T>::rvalue;
};
template <typename T>
using add_rvalue_reference_t = typename add_reference<T>::rvalue;


template <typename T, typename = void>
struct add_pointer {
    using type = T;
};
template <typename T>
struct add_pointer<T, void_t<remove_reference_t<T>*>> {
    using type = remove_reference_t<T>*;
};

template <typename T>
using add_pointer_t = typename add_pointer<T>::type;


// declval will only work in no evaluation context (decltype or sizeof) to quickly get rvalue types. 
template <typename T>
add_rvalue_reference_t<T> declval() noexcept;

// try to copy type T
template <typename T>
type_identity_t<T> declcopy(type_identity_t<T>) noexcept;

// work with is_void_t
template <typename T>
void declvoid(type_identity_t<T>) noexcept;


template <typename>
struct rank : integral_constant<size_t, 0> {};
template <typename T, size_t Idx>
struct rank<T[Idx]> : integral_constant<size_t, rank<T>::value + 1> {};
template <typename T>
struct rank<T[]> : integral_constant<size_t, rank<T>::value + 1> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr size_t rank_v = rank<T>::value;
#endif


template <typename T, uint32_t Idx = 0>
struct extent : integral_constant<size_t, 0> {};
template <typename T, size_t N>
struct extent<T[N], 0> : integral_constant<size_t, N> {};
template <typename T, uint32_t Idx, size_t N>
struct extent<T[N], Idx> : extent<T, Idx - 1> {};
template <typename T, uint32_t Idx>
struct extent<T[], Idx> : extent<T, Idx - 1> {};

#ifdef MSTL_STANDARD_14__
template <typename T, uint32_t Idx = 0>
MSTL_INLINE17 constexpr size_t extent_v = extent<T, Idx>::value;
#endif


template <typename T>
struct is_bounded_array : false_type {};
template <typename T, size_t Idx>
struct is_bounded_array<T[Idx]> : true_type {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_bounded_array_v = is_bounded_array<T>::value;
#endif


template <typename T>
struct is_unbounded_array : false_type {};
template <typename T>
struct is_unbounded_array<T[]> : true_type {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_unbounded_array_v = is_unbounded_array<T>::value;
#endif


template <typename T>
struct is_array : bool_constant<is_unbounded_array<T>::value || is_bounded_array<T>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_array_v = is_array<T>::value;
#endif


template <typename>
struct is_lvalue_reference : false_type {};
template <typename T>
struct is_lvalue_reference<T&> : true_type {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_lvalue_reference_v = is_lvalue_reference<T>::value;
#endif


template <typename>
struct is_rvalue_reference : false_type {};
template <typename T>
struct is_rvalue_reference<T&&> : true_type {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_rvalue_reference_v = is_rvalue_reference<T>::value;
#endif


template <typename T>
struct is_reference : bool_constant<is_lvalue_reference<T>::value || is_rvalue_reference<T>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_reference_v = is_reference<T>::value;
#endif


template <typename T>
struct is_null_pointer : bool_constant<is_same<remove_cvref_t<T>, nullptr_t>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_null_pointer_v = is_null_pointer<T>::value;
#endif


template <typename>
struct is_pointer : false_type {};
template <typename T>
struct is_pointer<T*> : true_type {};
template <typename T>
struct is_pointer<T* const> : true_type {};
template <typename T>
struct is_pointer<T* volatile> : true_type {};
template <typename T>
struct is_pointer<T* const volatile> : true_type {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_pointer_v = is_pointer<T>::value;
#endif


template<typename T>
struct is_enum : bool_constant<__is_enum(T)> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_enum_v = is_enum<T>::value;
#endif


template <typename T>
struct is_integral_like : bool_constant<disjunction<is_integral<T>, is_enum<T>>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_integral_like_v = is_integral_like<T>::value;
#endif


template <typename T>
struct is_union : bool_constant<__is_union(T)> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_union_v = is_union<T>::value;
#endif


template <typename T>
struct is_class : bool_constant<__is_class(T)> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_class_v = is_class<T>::value;
#endif


template <typename T>
struct is_fundamental : bool_constant<disjunction<is_arithmetic<T>, is_void<T>, is_null_pointer<T>>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_fundamental_v = is_fundamental<T>::value;
#endif


template <typename T>
struct is_compound : bool_constant<!is_fundamental<T>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_compound_v = is_compound<T>::value;
#endif


template <typename>
struct is_const : false_type {};
template <typename T>
struct is_const<const T> : true_type {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_const_v = is_const<T>::value;
#endif


template <typename>
struct is_volatile : false_type {};
template <typename T>
struct is_volatile<volatile T> : true_type {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_volatile_v = is_volatile<T>::value;
#endif


#pragma warning(push)
#pragma warning(disable: 4180)

// function cannot be const qualified.
template <typename T>
struct is_function : bool_constant<
    !is_const<const remove_function_qualifiers_t<T>>::value
    && !is_reference<remove_function_qualifiers_t<T>>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_function_v = is_function<T>::value;
#endif

#pragma warning(pop)


template <typename T>
struct is_allocable : bool_constant<
    !(is_void<T>::value || is_reference<T>::value || is_function<T>::value || is_const<T>::value)
    && (sizeof(T) > 0)> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_allocable_v = is_allocable<T>::value;
#endif


// object types are types excepts function, reference or void types.
// especially, function and reference types cannot be const–qualified.
template <typename T>
struct is_object : bool_constant<is_const<const T>::value && !is_void<T>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_object_v = is_object<T>::value;
#endif


template <typename T>
struct is_cstring : bool_constant<
    (is_pointer<remove_cvref_t<T>>::value &&
        is_character<remove_pointer_t<remove_cvref_t<T>>>::value) ||
    (is_bounded_array<remove_cvref_t<T>>::value &&
        is_character<remove_all_extents_t<remove_cvref_t<T>>>::value)> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
constexpr bool is_cstring_v = is_cstring<T>::value;
#endif


template <typename T>
using cstring_char_t = remove_cv_t<remove_all_extents_t<remove_pointer_t<remove_cvref_t<T>>>>;

template <typename T>
struct cstring_char {
    using type = cstring_char_t<T>;
};


#ifdef MSTL_COMPILER_CLANG__
template <typename T>
struct is_member_function_pointer : bool_constant<__is_member_function_pointer(T)> {};
#else
MSTL_BEGIN_INNER__
template <typename>
struct __is_member_function_pointer_aux : false_type {};
template <typename T, typename C>
struct __is_member_function_pointer_aux<T C::*> : is_function<T> {};
MSTL_END_INNER__

template<typename T>
struct is_member_function_pointer : _INNER __is_member_function_pointer_aux<remove_cv_t<T>> {};
#endif

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_member_function_pointer_v = is_member_function_pointer<T>::value;
#endif


#ifdef MSTL_COMPILER_CLANG__
template <typename T>
struct is_member_object_pointer : bool_constant<__is_member_object_pointer(T)> {};
#else
template <typename>
struct is_member_object_pointer : false_type {};
template <typename T, typename C>
struct is_member_object_pointer<T C::*> : bool_constant<!is_function_v<T>> {};
#endif

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_member_object_pointer_v = is_member_object_pointer<remove_cv_t<T>>::value;
#endif


#ifdef MSTL_COMPILER_CLANG__
template <typename T>
struct is_member_pointer : bool_constant<__is_member_pointer(T)> {};
#else
template <typename T>
struct is_member_pointer : bool_constant<is_member_object_pointer<T>::value || is_member_function_pointer<T>::value> {};
#endif

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_member_pointer_v = is_member_pointer<T>::value;
#endif


// arithmetic, enum and pointer types are scalar types.
template <typename T>
struct is_scalar : bool_constant<disjunction<
    is_arithmetic<T>, is_enum<T>, is_pointer<T>, is_member_pointer<T>, is_null_pointer<T>
>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_scalar_v = is_scalar<T>::value;
#endif


// empty types don't have non-static members, virtual functions and virtual base class.
// if it has a base class, that should also be an empty type.
template <typename T>
struct is_empty : bool_constant<__is_empty(T)> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_empty_v = is_empty<T>::value;
#endif


template <typename T>
struct is_polymorphic : bool_constant<__is_polymorphic(T)> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_polymorphic_v = is_polymorphic<T>::value;
#endif


template <typename T>
struct is_abstract : bool_constant<__is_abstract(T)> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_abstract_v = is_abstract<T>::value;
#endif


template <typename T>
struct is_final : bool_constant<__is_final(T)> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_final_v = is_final<T>::value;
#endif


MSTL_BEGIN_INNER__
template <typename T, bool = is_enum<T>::value>
struct __underlying_type_aux {
    using type = __underlying_type(T);
};
template <typename T>
struct __underlying_type_aux<T, false> {};
MSTL_END_INNER__

template <typename T>
struct underlying_type : _INNER __underlying_type_aux<T> {};
template <typename T>
using underlying_type_t = typename underlying_type<T>::type;


// standard layout types have these characteristics:
//   consistent access control for non-static members (all public).
//   no virtual functions and virtual base classes.
//   no base class or there is only one base class and the base class is also a standard layout type.
//   non-static data members cannot appear in both the base class and the derived class.
template <typename T>
struct is_standard_layout : bool_constant<__is_standard_layout(T)> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_standard_layout_v = is_standard_layout<T>::value;
#endif


// pod types meet the conditions of is_standard_layout and is_trivial.
template <typename T>
struct is_pod : bool_constant<__is_pod(T)> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_pod_v = is_pod<T>::value;
#endif


#ifdef MSTL_COMPILER_CLANG__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(MSTL_COMPILER_GCC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(MSTL_COMPILER_MSVC__)
#pragma warning(push)
#pragma warning(disable: 4996)
#endif

template <typename T>
struct MSTL_DEPRECATED is_literal_type : bool_constant<__is_literal_type(T)> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_DEPRECATED MSTL_INLINE17 constexpr bool is_literal_type_v = is_literal_type<T>::value;
#endif

#ifdef MSTL_COMPILER_CLANG__
#pragma clang diagnostic pop
#elif defined(MSTL_COMPILER_GCC__)
#pragma GCC diagnostic pop
#elif defined(MSTL_COMPILER_MSVC__)
#pragma warning(pop)
#endif


#ifdef MSTL_STANDARD_20__
// Determines whether a non-static member pointer points to the start of a class object
template <typename T, typename Mem>
constexpr bool is_pointer_interconvertible_with_class(Mem T::*mp) noexcept {
    return __builtin_is_pointer_interconvertible_with_class(mp);
}

// Whether the member pointer in two different classes points to the member in the corresponding position
template <typename S1, typename S2, typename M1, typename M2>
constexpr bool is_corresponding_member(M1 S1::* m1, M2 S2::* m2) noexcept {
    return __builtin_is_corresponding_member(m1, m2);
}
#endif


// trivial types have these characteristics:
//   the constructor, copy constructor, move constructor, copy assignment operator, 
//   move assignment operator, and destructor are automatically generated by the compiler (is default).
//   no virtual functions and virtual base classes.
template <typename T>
struct is_trivial : bool_constant<__is_trivial(T)> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_trivial_v = is_trivial<T>::value;
#endif


template <typename T>
struct is_trivially_copyable : bool_constant<__is_trivially_copyable(T)> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_trivially_copyable_v = is_trivially_copyable<T>::value;
#endif


#ifdef MSTL_STANDARD_20__
template <typename To, typename From>
    requires (sizeof(To) == sizeof(From)) && is_trivially_copyable_v<To> && is_trivially_copyable_v<From>
MSTL_NODISCARD constexpr To bit_cast(const From& value) noexcept {
    return __builtin_bit_cast(To, value);
}
#endif


template <typename T>
struct has_virtual_destructor : bool_constant<__has_virtual_destructor(T)> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool has_virtual_destructor_v = has_virtual_destructor<T>::value;
#endif


// each different value of a type has a unique binary representation, 
// but padding bytes and implementation dependencies should also be taken into account.
// if a type has unique object representations,
// we can use memory comparison to easily determine the equality.
// standard layout types and trivial types match unique object representations.
template <typename T>
struct has_unique_object_representations : bool_constant<__has_unique_object_representations(T)> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool has_unique_object_representations_v = has_unique_object_representations<T>::value;
#endif


#ifdef MSTL_COMPILER_MSVC__
template <typename T>
struct is_aggregate : bool_constant<is_array<T>::value || __is_aggregate(T)> {};
#else
template <typename T>
struct is_aggregate : bool_constant<__is_aggregate(remove_cv_t<T>)> {};
#endif

#ifdef MSTL_STANDARD_14__
// aggregate types have these characteristics:
//   is array types;
//   is class types (include struct and union) which have the following characteristics:
//     no user-defined constructors;
//     all non-static data members are public; 
//     no virtual functions and virtual base classes.
template <typename T>
MSTL_INLINE17 constexpr bool is_aggregate_v = is_aggregate<T>::value;
#endif


template <typename T, typename... Args>
struct is_constructible : bool_constant<__is_constructible(T, Args...)> {};

#ifdef MSTL_STANDARD_14__
template <typename T, typename... Args>
MSTL_INLINE17 constexpr bool is_constructible_v = is_constructible<T, Args...>::value;
#endif


template <typename T>
struct is_copy_constructible 
    : bool_constant<is_constructible<T, add_lvalue_reference_t<const T>>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_copy_constructible_v = is_copy_constructible<T>::value;
#endif


template <typename T>
struct is_default_constructible : bool_constant<is_constructible<T>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_default_constructible_v = is_default_constructible<T>::value;
#endif


MSTL_BEGIN_INNER__
template <typename T>
void __implicitly_default_construct_aux(const T&) noexcept;
MSTL_END_INNER__

template <typename, typename = void>
struct is_implicitly_default_constructible : false_type {};
template <typename T>
struct is_implicitly_default_constructible 
    <T, void_t<decltype(_INNER __implicitly_default_construct_aux<T>({}))>> : true_type {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_implicitly_default_constructible_v = is_implicitly_default_constructible<T>::value;
#endif


template <typename T>
struct is_move_constructible : bool_constant<is_constructible<T, T>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_move_constructible_v = is_move_constructible<T>::value;
#endif


template <typename To, typename From>
struct is_assignable : bool_constant<__is_assignable(To, From)> {};

#ifdef MSTL_STANDARD_14__
template <typename To, typename From>
MSTL_INLINE17 constexpr bool is_assignable_v = is_assignable<To, From>::value;
#endif


template <typename T>
struct is_copy_assignable
    : bool_constant<is_assignable<add_lvalue_reference_t<T>, add_lvalue_reference_t<const T>>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_copy_assignable_v = is_copy_assignable<T>::value;
#endif


template <typename T>
struct is_move_assignable : bool_constant<is_assignable<add_lvalue_reference_t<T>, T>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_move_assignable_v = is_move_assignable<T>::value;
#endif


#ifdef MSTL_COMPILER_MSVC__
template <typename T>
struct is_destructible : bool_constant<__is_destructible(T)> {};
#else
MSTL_BEGIN_INNER__
template <typename T>
struct __destructible_aux {
private:
    template <typename T1, typename = decltype(declval<T1&>().~T1())>
    static true_type __test(int);
    template <typename>
    static false_type __test(...);

public:
    using type =  decltype(__test<T>(0));
};

template <typename T,
    bool = disjunction<is_void<T>, is_unbounded_array<T>, is_function<T>>::value,
    bool = disjunction<is_reference<T>, is_scalar<T>>::value>
struct __is_destructible_dispatch;

template <typename T>
struct __is_destructible_dispatch<T, false, false>
    : __destructible_aux<remove_all_extents_t<T>>::type {};

template <typename T>
struct __is_destructible_dispatch<T, true, false> : false_type {};

template <typename T>
struct __is_destructible_dispatch<T, false, true> : true_type {};
MSTL_END_INNER__


template <typename T>
struct is_destructible : _INNER __is_destructible_dispatch<T>::type {};
#endif

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_destructible_v = is_destructible<T>::value;
#endif


template <typename T, typename... Args>
struct is_trivially_constructible : bool_constant<__is_trivially_constructible(T, Args...)> {};

#ifdef MSTL_STANDARD_14__
template <typename T, typename... Args>
MSTL_INLINE17 constexpr bool is_trivially_constructible_v = is_trivially_constructible<T, Args...>::value;
#endif


template <typename T>
struct is_trivially_copy_constructible
    : bool_constant<is_trivially_constructible<T, add_lvalue_reference_t<const T>>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_trivially_copy_constructible_v = is_trivially_copy_constructible<T>::value;
#endif


template <typename T>
struct is_trivially_default_constructible : bool_constant<is_trivially_constructible<T>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_trivially_default_constructible_v = is_trivially_default_constructible<T>::value;
#endif


template <typename T>
struct is_trivially_move_constructible : bool_constant<is_trivially_constructible<T, T>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_trivially_move_constructible_v = is_trivially_move_constructible<T>::value;
#endif


template <typename To, typename From>
struct is_trivially_assignable : bool_constant<__is_trivially_assignable(To, From)> {};

#ifdef MSTL_STANDARD_14__
template <typename To, typename From>
MSTL_INLINE17 constexpr bool is_trivially_assignable_v = is_trivially_assignable<To, From>::value;
#endif


template <typename T>
struct is_trivially_copy_assignable : bool_constant<
    is_trivially_assignable<add_lvalue_reference_t<T>, add_lvalue_reference_t<const T>>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_trivially_copy_assignable_v = is_trivially_copy_assignable<T>::value;
#endif


template <typename T>
struct is_trivially_move_assignable : bool_constant<
    is_trivially_assignable<add_lvalue_reference_t<T>, T>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_trivially_move_assignable_v = is_trivially_move_assignable<T>::value;
#endif


#if defined(MSTL_COMPILER_MSVC__) || defined(MSTL_COMPILER_CLANG__)
template <typename T>
struct is_trivially_destructible : bool_constant<__is_trivially_destructible(T)> {};
#else
template <typename T>
struct is_trivially_destructible : conjunction<is_destructible<T>, bool_constant<__has_trivial_destructor(T)>> {};
#endif

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_trivially_destructible_v = is_trivially_destructible<T>::value;
#endif


#ifdef MSTL_COMPILER_MSVC__
template <typename T, typename... Args>
struct is_nothrow_constructible : bool_constant<__is_nothrow_constructible(T, Args...)> {};
#else
MSTL_BEGIN_INNER__
template <typename T, bool = is_array<T>::value>
struct __is_nothrow_default_constructible_dispatch;

template <typename T>
struct __is_nothrow_default_constructible_dispatch<T, true> : conjunction<
    is_bounded_array<T>, bool_constant<noexcept(remove_all_extents_t<T>())>> {};

template <typename T>
struct __is_nothrow_default_constructible_dispatch<T, false>
    : bool_constant<noexcept(T())> {};
MSTL_END_INNER__

template <typename T>
struct is_nothrow_default_constructible : conjunction<
    is_default_constructible<T>, _INNER __is_nothrow_default_constructible_dispatch<T>> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_nothrow_default_constructible_v = is_nothrow_default_constructible<T>::value;
#endif


MSTL_BEGIN_INNER__
template <typename T, typename... Args>
struct __is_nothrow_constructible_dispatch
    : bool_constant<noexcept(T(_MSTL declval<Args>()...))> {};

template <typename T>
struct __is_nothrow_constructible_dispatch<T>
    : is_nothrow_default_constructible<T> {};
MSTL_END_INNER__

template <typename T, typename... Args>
struct is_nothrow_constructible : conjunction<
    is_constructible<T, Args...>, _INNER __is_nothrow_constructible_dispatch<T, Args...>> {};
#endif

#ifdef MSTL_STANDARD_14__
template <typename T, typename... Args>
MSTL_INLINE17 constexpr bool is_nothrow_constructible_v = is_nothrow_constructible<T, Args...>::value;
#endif


template <typename T>
struct is_nothrow_copy_constructible
    : bool_constant<is_nothrow_constructible<T, add_lvalue_reference_t<const T>>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_nothrow_copy_constructible_v = is_nothrow_copy_constructible<T>::value;
#endif


#ifdef MSTL_COMPILER_MSVC__
template <typename T>
struct is_nothrow_default_constructible : bool_constant<is_nothrow_constructible_v<T>> {};
template <typename T>
MSTL_INLINE17 constexpr bool is_nothrow_default_constructible_v = is_nothrow_default_constructible<T>::value;
#endif


template <typename T>
struct is_nothrow_move_constructible : bool_constant<is_nothrow_constructible<T, T>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_nothrow_move_constructible_v = is_nothrow_move_constructible<T>::value;
#endif


#ifdef MSTL_COMPILER_MSVC__
template <typename To, typename From>
struct is_nothrow_assignable : bool_constant<__is_nothrow_assignable(To, From)> {};
#else
template <typename To, typename From>
struct is_nothrow_assignable : conjunction<
    is_assignable<To, From>, bool_constant<noexcept(_MSTL declval<To>() = _MSTL declval<From>())>> {};
#endif

#ifdef MSTL_STANDARD_14__
template <typename To, typename From>
MSTL_INLINE17 constexpr bool is_nothrow_assignable_v = is_nothrow_assignable<To, From>::value;
#endif


template <typename T>
struct is_nothrow_copy_assignable : bool_constant<
    is_nothrow_assignable<add_lvalue_reference_t<T>, add_lvalue_reference_t<const T>>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_nothrow_copy_assignable_v = is_nothrow_copy_assignable<T>::value;
#endif


template <typename T>
struct is_nothrow_move_assignable : bool_constant<
    is_nothrow_assignable<add_lvalue_reference_t<T>, T>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_nothrow_move_assignable_v = is_nothrow_move_assignable<T>::value;
#endif


#ifdef MSTL_COMPILER_MSVC__
template <typename T>
struct is_nothrow_destructible : bool_constant<__is_nothrow_destructible(T)> {};
#else
MSTL_BEGIN_INNER__
template <typename T>
struct __is_nothrow_destructible_aux {
private:
    template <typename T1>
    static bool_constant<noexcept(declval<T1&>().~T1())> __test(int);

    template <typename>
    static false_type __test(...);

public:
    using type =  decltype(__test<T>(0));
};

template <typename T,
    bool = disjunction<is_void<T>, is_unbounded_array<T>, is_function<T>>::value,
    bool = disjunction<is_reference<T>, is_scalar<T>>::value>
struct __is_nothrow_destructible_dispatch;

template <typename T>
struct __is_nothrow_destructible_dispatch<T, false, false>
    : __is_nothrow_destructible_aux<remove_all_extents_t<T>>::type {};

template <typename T>
struct __is_nothrow_destructible_dispatch<T, true, false> : false_type {};

template <typename T>
struct __is_nothrow_destructible_dispatch<T, false, true> : true_type {};
MSTL_END_INNER__

template<typename T>
struct is_nothrow_destructible : _INNER __is_nothrow_destructible_dispatch<T>::type {};
#endif

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_nothrow_destructible_v = is_nothrow_destructible<T>::value;
#endif


template <typename T>
MSTL_NODISCARD constexpr T&& forward(remove_reference_t<T>& x) noexcept {
    return static_cast<T&&>(x);
}
template <typename T>
MSTL_NODISCARD constexpr T&& forward(remove_reference_t<T>&& x) noexcept {
    static_assert(!is_lvalue_reference<T>::value, "forward failed.");
    return static_cast<T&&>(x);
}
template <typename T>
MSTL_NODISCARD constexpr remove_reference_t<T>&& move(T&& x) noexcept {
    return static_cast<remove_reference_t<T>&&>(x);
}
template <typename T>
MSTL_NODISCARD constexpr
conditional_t<!is_nothrow_move_constructible<T>::value && is_copy_constructible<T>::value, const T&, T&&>
move_if_noexcept(T& x) noexcept {
    return _MSTL move(x);
}


template <typename T>
MSTL_NODISCARD constexpr T* addressof(T& x) noexcept {
    return __builtin_addressof(x);
}
template <typename T>
const T* addressof(const T&&) = delete;


#ifdef MSTL_STANDARD_20__
MSTL_NODISCARD constexpr bool is_constant_evaluated() noexcept {
    return __builtin_is_constant_evaluated();
}
#endif


#ifdef MSTL_COMPILER_MSVC__
template <typename From, typename To>
struct is_convertible : bool_constant<__is_convertible_to(From, To)> {};
#elif defined(MSTL_COMPILER_CLANG__)
template <typename From, typename To>
struct is_convertible : bool_constant<__is_convertible(From, To)> {};
#else
template <typename From, typename To, bool = disjunction_v<is_void<From>, is_function<To>, is_array<To>>>
struct __is_convertible_helper {
    using type = typename is_void<To>::type;
};

template <typename From, typename To>
struct __is_convertible_helper<From, To, false> {
private:
    template <typename From1, typename To1, typename = decltype(_MSTL declvoid<To1>(_MSTL declval<From1>()))>
    static true_type __test(int);

    template <typename, typename>
    static false_type __test(...);

public:
    using type = decltype(__test<From, To>(0));
};

template<typename From, typename To>
struct is_convertible : __is_convertible_helper<From, To>::type {};
#endif

#ifdef MSTL_STANDARD_14__
template <typename From, typename To>
MSTL_INLINE17 constexpr bool is_convertible_v = is_convertible<From, To>::value;
#endif


template <typename ToElement, typename FromElement>
using is_array_convertible = is_convertible<FromElement(*)[], ToElement(*)[]>;

#ifdef MSTL_STANDARD_14__
template <typename ToElement, typename FromElement>
MSTL_INLINE17 constexpr bool is_array_convertible_v = is_array_convertible<ToElement, FromElement>::value;
#endif


MSTL_BEGIN_INNER__
template <size_t>
struct __sign_byte_aux;

template <>
struct __sign_byte_aux<1> {
    template <typename>
    using signed_t = signed char;
    template <typename>
    using unsigned_t = unsigned char;
};
template <>
struct __sign_byte_aux<2> {
    template <typename>
    using signed_t = signed short;
    template <typename>
    using unsigned_t = unsigned short;
};
template <>
struct __sign_byte_aux<4> {
#ifdef MSTL_PLATFORM_WINDOWS__
    template <typename T>
    using signed_t = 
        conditional_t<is_same_v<T, signed long> || is_same_v<T, unsigned long>, signed long, signed int>;

    template <typename T>
    using unsigned_t = 
        conditional_t<is_same_v<T, signed long> || is_same_v<T, unsigned long>, unsigned long, unsigned int>;
#elif defined(MSTL_PLATFORM_LINUX__)
    template <typename>
    using signed_t = signed int;
    template <typename>
    using unsigned_t = unsigned int;
#endif
};
template <>
struct __sign_byte_aux<8> {
#ifdef MSTL_PLATFORM_WINDOWS__
    template <typename>
    using signed_t = signed long long;
    template <typename>
    using unsigned_t = unsigned long long;
#elif defined(MSTL_PLATFORM_LINUX__)
    template <typename T>
    using signed_t =
        conditional_t<is_same<T, signed long>::value || is_same<T, unsigned long>::value, signed long, signed long long>;

    template <typename T>
    using unsigned_t =
        conditional_t<is_same<T, signed long>::value || is_same<T, unsigned long>::value, unsigned long, unsigned long long>;
#endif
};

template <typename T>
using __set_signed_byte = typename __sign_byte_aux<sizeof(T)>::template signed_t<T>;
template <typename T>
using __set_unsigned_byte = typename __sign_byte_aux<sizeof(T)>::template unsigned_t<T>;

template <typename T>
struct __set_sign {
    static_assert((is_integral<T>::value && !is_boolean<T>::value) || is_enum<T>::value,
        "make signed only support non bool integral types and enum types");

    using signed_type   = copy_cv_t<T, __set_signed_byte<T>>;
    using unsigned_type = copy_cv_t<T, __set_unsigned_byte<T>>;
};
MSTL_END_INNER__

template <typename T>
using make_signed_t = typename _INNER __set_sign<T>::signed_type;
template <typename T>
using make_unsigned_t = typename _INNER __set_sign<T>::unsigned_type;

template <typename T>
struct make_signed {
    using type = make_signed_t<T>;
};
template <typename T>
struct make_unsigned {
    using type = make_unsigned_t<T>;
};

template <typename T>
constexpr make_signed_t<T> signed_value(const T x) {
    return static_cast<make_signed_t<T>>(x);
}
template <typename T>
constexpr make_unsigned_t<T> unsigned_value(const T x) {
    return static_cast<make_unsigned_t<T>>(x);
}


MSTL_BEGIN_INNER__
template <size_t Size, bool IsSigned>
struct __make_integer_impl;

template <size_t Size>
struct __make_integer_impl<Size, true> {
    using type = typename __sign_byte_aux<Size>::template signed_t<int>;
};
template <size_t Size>
struct __make_integer_impl<Size, false> {
    using type = typename __sign_byte_aux<Size>::template unsigned_t<int>;
};
MSTL_END_INNER__

template <size_t Size, bool IsSigned = true>
struct make_integer {
    using type = typename _INNER __make_integer_impl<Size, IsSigned>::type;
};

template <size_t Size, bool IsSigned = true>
using make_integer_t = typename make_integer<Size, IsSigned>::type;

template <typename T, bool IsSigned = is_signed<T>::value>
constexpr make_integer_t<sizeof(T), IsSigned> integer_value(const T x) {
    return static_cast<make_integer_t<sizeof(T), IsSigned>>(x);
}


template <typename T>
struct alignment_of : integral_constant<size_t, alignof(T)> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
constexpr size_t alignment_of_v = alignment_of<T>::value;
#endif


template <typename Base, typename Derived>
struct is_base_of : bool_constant<__is_base_of(Base, Derived)> {};

#ifdef MSTL_STANDARD_14__
template <typename Base, typename Derived>
MSTL_INLINE17 constexpr bool is_base_of_v = is_base_of<Base, Derived>::value;
#endif


MSTL_BEGIN_INNER__
template <size_t Len>
struct __aligned_storage_aux {
    union type {
        unsigned char data[Len];
        struct MSTL_ALIGNED_DEFAULT() {} align;
    };
};
MSTL_END_INNER__

template <size_t Len, size_t Align = alignof(typename _INNER __aligned_storage_aux<Len>::type)>
struct aligned_storage {
    union type {
        unsigned char data[Len];
        struct MSTL_ALIGNED(Align) {} align;
    };
};

template <size_t Len, size_t Align = alignof(typename _INNER __aligned_storage_aux<Len>::type)>
using aligned_storage_t = typename aligned_storage<Len, Align>::type;


MSTL_BEGIN_INNER__
template <typename...>
struct __aligned_union_aux {
    static const size_t align_ = 0;
    static const size_t size_ = 0;
};

template <typename T, typename... Types>
struct __aligned_union_aux<T, Types...> {
    static const size_t align_ = alignof(T) > __aligned_union_aux<Types...>::align_
        ? alignof(T) : __aligned_union_aux<Types...>::align_;
    static const size_t size_ = sizeof(T) > __aligned_union_aux<Types...>::size_
        ? sizeof(T) : __aligned_union_aux<Types...>::size_;
};
MSTL_END_INNER__

template <size_t Len, typename... Types>
struct aligned_union {
private:
    static_assert(sizeof...(Types) != 0, "aligned_union requires at least one type.");

    using strictest_t = _INNER __aligned_union_aux<Types...>;
    static const size_t len_ = Len > strictest_t::size_ ? Len : strictest_t::size_;

public:
    static const size_t align_value = strictest_t::align_;
    using type = typename aligned_storage<len_, align_value>::type;
};

template <size_t Len, typename... Types>
using aligned_union_t = typename aligned_union<Len, Types...>::type;

#ifdef MSTL_STANDARD_14__
template <size_t Len, typename... Types>
constexpr size_t aligned_union_v = aligned_union<Len, Types...>::align_value;
#endif


// decay_t will simulate the decline in the argument passing of function:
//   ref and cv qualifier will be removed;
//   array and function types will be converted to pointer types unless they are used to initialize reference.
template <typename T>
struct decay {
    using remove_ref_t = remove_reference_t<T>;
    using check_func_t = conditional_t<is_function<remove_ref_t>::value, add_pointer_t<remove_ref_t>, remove_cv_t<remove_ref_t>>;
    using type = conditional_t<is_array<remove_ref_t>::value, add_pointer_t<remove_extent_t<remove_ref_t>>, check_func_t>;
};
template <typename T>
using decay_t = typename decay<T>::type;
template <typename T>
using remove_cv_decay_t = remove_cv_t<decay_t<T>>;
template <typename T>
using remove_cvref_decay_t = remove_cvref_t<decay_t<T>>;


MSTL_BEGIN_INNER__
template <typename Default, typename, template<typename...> class, typename...>
struct __detector {
    using value_t = false_type;
    using type = Default;
};
template <typename Default, template<typename...> class Op, typename... Args>
struct __detector<Default, void_t<Op<Args...>>, Op, Args...> {
    using value_t = true_type;
    using type = Op<Args...>;
};
MSTL_END_INNER__

template <typename Default, template<typename...> class Op, typename... Args>
using detected_or = _INNER __detector<Default, void, Op, Args...>;

template <typename Default, template<typename...> class Op, typename... Args>
using detected_or_t = typename detected_or<Default, Op, Args...>::type;


// ternary operator (expr1 ? expr2 : expr3) will try to find a suitable common type,
// so that both expr2 and expr3 can implicitly convert to this type:
//   if expr2 and expr3 are of the same type, then the type of the entire expression is that type.
//   if expr2 and expr3 are different types, the compiler will attempt to type a common type.
//   conversion rules are typically based on standard implicit type conversion rules.
template <typename T1, typename T2>
using common_ternary_operator_t = decltype(true ? _MSTL declval<T1>() : _MSTL declval<T2>());


MSTL_BEGIN_INNER__
template <typename, typename, typename = void>
struct __oper_decay_aux {};
template <typename T1, typename T2>
struct __oper_decay_aux<T1, T2, void_t<common_ternary_operator_t<decay_t<T1>, decay_t<T2>>>> {
    using type = decay_t<common_ternary_operator_t<decay_t<T1>, decay_t<T2>>>;
};
MSTL_END_INNER__

template <typename...>
struct common_type;
template <typename... T>
using common_type_t = typename common_type<T...>::type;

template <>
struct common_type<> {};
template <typename T1>
struct common_type<T1> : common_type<T1, T1> {};
template <typename T1, typename T2>
struct common_type<T1, T2> : _INNER __oper_decay_aux<T1, T2> {};
template <typename T1, typename T2, typename... Rest>
struct common_type<T1, T2, Rest...> : common_type<common_type_t<T1, T2>, Rest...> {};


#ifdef MSTL_STANDARD_20__

template <typename...>
struct common_reference;
template <typename... Types>
using common_reference_t = typename common_reference<Types...>::type;

template <>
struct common_reference<> {};
template <typename T>
struct common_reference<T> {
    using type = T;
};


MSTL_BEGIN_INNER__

template <typename T1, typename T2>
struct __common_reference_base_aux : common_type<T1, T2> {};

template <typename T1, typename T2> requires requires {
    typename _MSTL common_ternary_operator_t<T1, T2>;
}
struct __common_reference_base_aux<T1, T2> {
    using type = _MSTL common_ternary_operator_t<T1, T2>;
};

template <typename, typename, template <typename> typename, template <typename> typename>
struct __basic_common_reference {};

template <typename T1>
struct __add_qualifier_aux {
    template <typename T2>
    using apply_t = copy_ref_t<T1, copy_cv_t<T1, T2>>;
};

template <typename T1, typename T2>
using qualifier_extract = typename __basic_common_reference<remove_cvref_t<T1>, remove_cvref_t<T2>,
    __add_qualifier_aux<T1>::template apply_t, __add_qualifier_aux<T2>::template apply_t>::type;

template <typename T1, typename T2>
struct __common_ref_qualify_aux : __common_reference_base_aux<T1, T2> {};

template <typename T1, typename T2>
    requires requires { typename qualifier_extract<T1, T2>; }
struct __common_ref_qualify_aux<T1, T2> {
    using type = qualifier_extract<T1, T2>;
};

template <typename T1, typename T2>
struct __common_reference_ptr_aux : __common_ref_qualify_aux<T1, T2> {};

template <typename T1, typename T2> requires
    is_lvalue_reference_v<common_ternary_operator_t<copy_cv_t<T1, T2>&, copy_cv_t<T2, T1>&>>
using __common_lvalue_aux = common_ternary_operator_t<copy_cv_t<T1, T2>&, copy_cv_t<T2, T1>&>;

template <typename, typename>
struct __common_reference_aux {};

template <typename T1, typename T2>
    requires requires { typename __common_lvalue_aux<T1, T2>; }
struct __common_reference_aux<T1&, T2&> {
    using type = __common_lvalue_aux<T1, T2>;
};

template <typename T1, typename T2> requires
    is_convertible_v<T1&&, __common_lvalue_aux<const T1, T2>>
struct __common_reference_aux<T1&&, T2&> {
    using type = __common_lvalue_aux<const T1, T2>;
};

template <typename T1, typename T2> requires
    is_convertible_v<T2&&, __common_lvalue_aux<const T2, T1>>
struct __common_reference_aux<T1&, T2&&> {
    using type = __common_lvalue_aux<const T2, T1>;
};

template <typename T1, typename T2>
using __common_rvalue_aux = remove_reference_t<__common_lvalue_aux<T1, T2>>&&;

template <typename T1, typename T2> requires
    is_convertible_v<T1&&, __common_rvalue_aux<T1, T2>> &&
    is_convertible_v<T2&&, __common_rvalue_aux<T1, T2>>
struct __common_reference_aux<T1&&, T2&&> {
    using type = __common_rvalue_aux<T1, T2>;
};

template <typename T1, typename T2>
using __common_reference_aux_t = typename __common_reference_aux<T1, T2>::type;

template <typename T1, typename T2> requires
    is_convertible_v<add_pointer_t<T1>, add_pointer_t<__common_reference_aux_t<T1, T2>>> &&
    is_convertible_v<add_pointer_t<T2>, add_pointer_t<__common_reference_aux_t<T1, T2>>>
struct __common_reference_ptr_aux<T1, T2> {
    using type = __common_reference_aux_t<T1, T2>;
};

MSTL_END_INNER__


template <typename T1, typename T2>
struct common_reference<T1, T2> : _INNER __common_reference_ptr_aux<T1, T2> {};

template <typename T1, typename T2, typename T3, typename... Rest>
struct common_reference<T1, T2, T3, Rest...> {};

template <typename T1, typename T2, typename T3, typename... Rest>
    requires requires { typename common_reference_t<T1, T2>; }
struct common_reference<T1, T2, T3, Rest...> : common_reference<common_reference_t<T1, T2>, T3, Rest...> {};

#endif // MSTL_STANDARD_20__


#ifdef MSTL_STANDARD_17__
template <typename T, template <typename...> typename Template>
MSTL_INLINE17 constexpr bool is_specialization_v = false;
template <template <typename...> typename Template, typename... Args>
MSTL_INLINE17 constexpr bool is_specialization_v<Template<Args...>, Template> = true;

template <typename T, template <typename...> typename Template>
struct is_specialization : bool_constant<is_specialization_v<T, Template>> {};
#else
template <typename T, template <typename...> class Template>
struct is_specialization : false_type {};
template <template <typename...> class Template, typename... Args>
struct is_specialization<Template<Args...>, Template> : true_type {};

template <typename T, template <typename...> class Template>
constexpr bool is_specialization_v() {
    return is_specialization<T, Template>::value;
}
#endif


template <typename From, typename To, bool = is_convertible<From, To>::value, bool = is_void<To>::value>
struct is_nothrow_convertible : bool_constant<noexcept(_MSTL declcopy<To>(_MSTL declval<From>()))> {};
template <typename From, typename To, bool IsVoid>
struct is_nothrow_convertible<From, To, false, IsVoid> : false_type {};
template <typename From, typename To>
struct is_nothrow_convertible<From, To, true, true> : true_type {};

#ifdef MSTL_STANDARD_14__
template <typename From, typename To>
MSTL_INLINE17 constexpr bool is_nothrow_convertible_v = is_nothrow_convertible<From, To>::value;
#endif


#ifdef MSTL_COMPILER_MSVC__
// layout compatible types have the same layout in memory.
// that is, their member variables are arranged in the same order and alignment.
// they have the same member variable types, number, and arrangement order.
template <typename T1, typename T2>
struct is_layout_compatible : bool_constant<__is_layout_compatible(T1, T2)> {};
template <typename T1, typename T2>
MSTL_INLINE17 constexpr bool is_layout_compatible_v = is_layout_compatible<T1, T2>::value;


template <typename Base, typename Derived>
struct is_pointer_interconvertible_base_of 
    : bool_constant<__is_pointer_interconvertible_base_of(Base, Derived)> {};
template <typename Base, typename Derived>
MSTL_INLINE17 constexpr bool is_pointer_interconvertible_base_of_v = is_pointer_interconvertible_base_of<Base, Derived>::value;
#endif


template <typename>
struct is_swappable;

template <typename>
struct is_nothrow_swappable;


template <typename T, enable_if_t<conjunction<is_move_constructible<T>, is_move_assignable<T>>::value, int> = 0>
MSTL_CONSTEXPR14 void swap(T&, T&)
noexcept(is_nothrow_move_constructible<T>::value && is_nothrow_move_assignable<T>::value);

template <typename T, size_t Size, enable_if_t<is_swappable<T>::value, int> = 0>
MSTL_CONSTEXPR14 void swap(T(&)[Size], T(&)[Size]) noexcept(is_nothrow_swappable<T>::value);

void swap() = delete;

template <typename T, typename U = T>
MSTL_CONSTEXPR14 T exchange(T&, U&&)
noexcept(conjunction<is_nothrow_move_constructible<T>, is_nothrow_assignable<T&, U>>::value);


template <typename, typename, typename = void>
struct is_swappable_from : false_type {};

template <typename T1, typename T2>
struct is_swappable_from<T1, T2, void_t<decltype(
    _MSTL swap(_MSTL declval<T1>(), _MSTL declval<T2>()))>> : true_type {};


template <typename T1, typename T2>
struct is_swappable_with : bool_constant<
    conjunction<is_swappable_from<T1, T2>, is_swappable_from<T2, T1>>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T1, typename T2>
MSTL_INLINE17 constexpr bool is_swappable_with_v = is_swappable_with<T1, T2>::value;
#endif


template <typename T>
struct is_swappable : bool_constant<
    is_swappable_with<add_lvalue_reference_t<T>, add_lvalue_reference_t<T>>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_swappable_v = is_swappable<T>::value;
#endif


template <typename T1, typename T2>
struct is_nothrow_swappable_from : bool_constant<noexcept(_MSTL swap(_MSTL declval<T1>(), _MSTL declval<T2>()))
    && noexcept(_MSTL swap(_MSTL declval<T2>(), _MSTL declval<T1>()))> {};


template <typename T1, typename T2>
struct is_nothrow_swappable_with : bool_constant<
    conjunction<is_swappable_with<T1, T2>, is_nothrow_swappable_from<T1, T2>>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T1, typename T2>
MSTL_INLINE17 constexpr bool is_nothrow_swappable_with_v = is_nothrow_swappable_with<T1, T2>::value;
#endif


template <typename T>
struct is_nothrow_swappable : bool_constant<
    is_nothrow_swappable_with<add_lvalue_reference_t<T>, add_lvalue_reference_t<T>>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_nothrow_swappable_v = is_nothrow_swappable<T>::value;
#endif


// ADL (Argument Dependent Lookup) is a function–lookup mechanism.
// specifically, when a function is called, in addition to the global and local scopes,
// the compiler will also look up the function declaration in the namespace to which the argument type belongs.
template <typename, typename = void>
struct is_ADL_swappable : false_type {};

template <typename T>
struct is_ADL_swappable<T, void_t<decltype(swap(_MSTL declval<T&>(), _MSTL declval<T&>()))>> : true_type {};


template <typename T>
struct is_trivially_swappable : bool_constant<conjunction<is_trivially_destructible<T>,
    is_trivially_move_constructible<T>, is_trivially_move_assignable<T>, negation<is_ADL_swappable<T>>>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_trivially_swappable_v = is_trivially_swappable<T>::value;
#endif


#ifdef MSTL_STANDARD_20__
template <typename From, typename To>
concept convertible_to = is_convertible_v<From, To> && requires { static_cast<To>(_MSTL declval<From>()); };
#endif // MSTL_STANDARD_20__


template <typename Iterator, typename Ptr, bool = is_pointer<remove_cvref_t<Iterator>>::value>
struct is_nothrow_arrow : bool_constant<is_nothrow_convertible<Iterator, Ptr>::value> {};

template <typename Iterator, typename Ptr>
struct is_nothrow_arrow<Iterator, Ptr, false> : bool_constant<
    noexcept(_MSTL declcopy<Ptr>(_MSTL declval<Iterator>().operator->()))> {};

#ifdef MSTL_STANDARD_14__
template <typename Iterator, typename Ptr>
MSTL_INLINE17 constexpr bool is_nothrow_arrow_v = is_nothrow_arrow<Iterator, Ptr>::value;
#endif


#ifdef MSTL_STANDARD_20__
template <typename T>
concept is_pair_v = requires(T p) {
    typename T::first_type;
    typename T::second_type;
    p.first;
    p.second;
};
#endif // MSTL_STANDARD_20__


template <typename, typename = void>
struct is_allocator : false_type {};

template <typename Alloc>
struct is_allocator<Alloc, void_t<
    typename Alloc::value_type, decltype(declval<Alloc&>().allocate(size_t{}))>>
    : true_type {};

#ifdef MSTL_STANDARD_14__
template <typename Alloc>
MSTL_INLINE17 constexpr bool is_allocator_v = is_allocator<Alloc>::value;
#endif


// have to compile perform NRVO(Named Return Value Optimization) instead of moving it.
// after C++ 11, compilers do NRVO when the following conditions are met:
//   the function returns a class-type object and the object is a local object of the function.
//   the function's return statement returns the local object directly.
template <typename T, enable_if_t<is_default_constructible<T>::value, int> = 0>
constexpr T initialize() noexcept(is_nothrow_default_constructible<T>::value) {
    return T();
}

#define INITIALIZE_BASIC_FUNCTION__(OPT) \
    template <> constexpr OPT initialize() noexcept { return static_cast<OPT>(0); }
MSTL_MACRO_RANGE_CHARS(INITIALIZE_BASIC_FUNCTION__)
MSTL_MACRO_RANGE_FLOAT(INITIALIZE_BASIC_FUNCTION__)
MSTL_MACRO_RANGE_INT(INITIALIZE_BASIC_FUNCTION__)

#undef INITIALIZE_BASIC_FUNCTION__


MSTL_BEGIN_INNER__

template <typename T>
struct __has_valid_begin_end {
private:
    template <typename U>
    static auto __test(int) -> decltype(
        declval<U>().begin(), declval<U>().end(),
        is_same<decltype(declval<U>().begin()), decltype(declval<U>().end())>(),
        true_type{}
    );

    template <typename U>
    static false_type __test(...);
public:
    static constexpr bool value = decltype(__test<T>(0))::value;
};

MSTL_END_INNER__


template <typename Iterator>
struct is_incrementable {
private:
    template <typename U>
    static auto __test(int) -> decltype(
        ++declval<U&>(),
        true_type{}
    );

    template <typename U>
    static false_type __test(...);
public:
    static constexpr bool value = decltype(__test<Iterator>(0))::value;
};

#ifdef MSTL_STANDARD_14__
template <typename Iterator>
MSTL_INLINE17 constexpr bool is_incrementable_v = is_incrementable<Iterator>::value;
#endif


template <typename Iterator>
struct is_decrementable {
private:
    template <typename U>
    static auto __test(int) -> decltype(
        --declval<U&>(),
        true_type{}
    );

    template <typename U>
    static false_type __test(...);
public:
    static constexpr bool value = decltype(__test<Iterator>(0))::value;
};

#ifdef MSTL_STANDARD_14__
template <typename Iterator>
MSTL_INLINE17 constexpr bool is_decrementable_v = is_decrementable<Iterator>::value;
#endif


template <typename Container>
struct is_iterable : bool_constant<
    _INNER __has_valid_begin_end<Container>::value &&
    is_incrementable<decltype(declval<Container>().begin())>::value
> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_iterable_v = is_iterable<T>::value;
#endif


MSTL_BEGIN_INNER__

template <typename T>
struct __has_first_and_second {
private:
    template <typename U>
    static auto __test(int) -> decltype(
        declval<U>().first, declval<U>().second,
        true_type{}
    );

    template <typename U>
    static false_type __test(...);
public:
    static constexpr bool value = decltype(__test<T>(0))::value;
};

MSTL_END_INNER__

template <typename Map>
struct is_maplike : bool_constant<
    is_iterable<Map>::value &&
    _INNER __has_first_and_second<decltype(*declval<decltype(declval<Map>().begin())>())>::value
> {};

#ifdef MSTL_STANDARD_14__
template <typename Map>
MSTL_INLINE17 constexpr bool is_maplike_v = is_maplike<Map>::value;
#endif


MSTL_BEGIN_INNER__
template <typename T>
struct __has_to_string_impl {
private:
    template <typename U>
    static auto __test(int) -> decltype(_MSTL declval<const U>().to_string(), true_type{});

    template <typename U>
    static false_type __test(...);
public:
    static constexpr bool value = decltype(__test<T>(0))::value;
};
MSTL_END_INNER__

template <typename T>
struct has_to_string : bool_constant<_INNER __has_to_string_impl<T>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool has_to_string_v = has_to_string<T>::value;
#endif


MSTL_BEGIN_INNER__
template <typename T>
struct __has_to_hash_impl {
private:
    template <typename U>
    static auto __test(int) -> decltype(_MSTL declval<const U>().to_hash(), true_type{});

    template <typename U>
    static false_type __test(...);
public:
    static constexpr bool value = decltype(__test<T>(0))::value;
};
MSTL_END_INNER__

template <typename T>
struct has_to_hash : bool_constant<_INNER __has_to_hash_impl<T>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool has_to_hash_v = has_to_hash<T>::value;
#endif


MSTL_BEGIN_INNER__
template <typename T>
struct __has_swap_impl {
private:
    template <typename U>
    static auto __test(int) -> decltype(_MSTL declval<U>().swap(_MSTL declval<U>()), true_type{});

    template <typename U>
    static false_type __test(...);
public:
    static constexpr bool value = decltype(__test<T>(0))::value;
};
MSTL_END_INNER__

template <typename T>
struct has_swap : bool_constant<_INNER __has_swap_impl<T>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool has_swap_v = has_swap<T>::value;
#endif


MSTL_BEGIN_INNER__
template <typename Alloc, typename T, typename... Args>
struct __has_construct_impl {
private:
    template <typename Alloc1,
    typename = decltype(_MSTL declval<Alloc1*>()->construct(_MSTL declval<T*>(), _MSTL declval<Args>()...))>
    static true_type __test(int);

    template<typename>
    static false_type __test(...);

public:
    using type = decltype(__test<Alloc>(0));
};
MSTL_END_INNER__

template <typename Alloc, typename T, typename... Args>
struct has_construct : _INNER __has_construct_impl<Alloc, T, Args...>::type {};

#ifdef MSTL_STANDARD_14__
template <typename Alloc, typename T, typename... Args>
MSTL_INLINE17 constexpr bool has_construct_v = has_construct<Alloc, T, Args...>::value;
#endif


MSTL_BEGIN_INNER__
template <typename T>
struct __has_base_impl {
private:
    template <typename U>
    static auto __test(int) -> decltype(_MSTL declval<const U>().base(), true_type{});

    template <typename U>
    static false_type __test(...);
public:
    static constexpr bool value = decltype(__test<T>(0))::value;
};
MSTL_END_INNER__

template <typename T>
struct has_base : bool_constant<_INNER __has_base_impl<T>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool has_base_v = has_base<T>::value;
#endif


template <typename T, enable_if_t<conjunction<is_move_constructible<T>, is_move_assignable<T>>::value, int>>
MSTL_CONSTEXPR14 void swap(T& lhs, T& rhs)
noexcept(is_nothrow_move_constructible<T>::value && is_nothrow_move_assignable<T>::value) {
    T tmp = _MSTL move(lhs);
    lhs = _MSTL move(rhs);
    rhs = _MSTL move(tmp);
}

template <typename T, size_t Size, enable_if_t<is_swappable<T>::value, int>>
MSTL_CONSTEXPR14 void swap(T(& lhs)[Size], T(& rhs)[Size]) noexcept(is_nothrow_swappable<T>::value) {
    if (&lhs == &rhs) return;
    T* first1 = lhs;
    T* last1 = first1 + Size;
    T* first2 = rhs;
    for (; first1 != last1; ++first1, ++first2) {
        _MSTL swap(*first1, *first2);
    }
}

template <typename T, typename U>
MSTL_CONSTEXPR14 T exchange(T& val, U&& new_val) noexcept(conjunction<
    is_nothrow_move_constructible<T>, is_nothrow_assignable<T&, U>>::value) {
    T old_val = _MSTL move(val);
    val = _MSTL forward<U>(new_val);
    return old_val;
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_TYPEINFO_TYPE_TRAITS_HPP__
