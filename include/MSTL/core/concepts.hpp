#ifndef MSTL_CONCEPTS_HPP__
#define MSTL_CONCEPTS_HPP__
#include "type_traits.hpp"
MSTL_BEGIN_NAMESPACE__

#ifdef MSTL_VERSION_20__
template <typename T1, typename T2>
concept same_as = is_same_v<T1, T2> && is_same_v<T2, T1>;

template <typename T1, typename T2>
concept common_reference_with = requires {
	typename common_reference_t<T1, T2>;
	typename common_reference_t<T2, T1>;
}
&& same_as<common_reference_t<T1, T2>, common_reference_t<T2, T1>>
&& convertible_to<T1, common_reference_t<T1, T2>>
&& convertible_to<T2, common_reference_t<T1, T2>>;

template <typename T1, typename T2>
concept common_with = requires { typename common_type_t<T1, T2>; typename common_type_t<T2, T1>; }
&& same_as<common_type_t<T1, T2>, common_type_t<T2, T1>> && requires {
	static_cast<common_type_t<T1, T2>>(_MSTL declval<T1>());
	static_cast<common_type_t<T1, T2>>(_MSTL declval<T2>());
} && common_reference_with<add_lvalue_reference_t<const T1>, add_lvalue_reference_t<const T2>>
&& common_reference_with<add_lvalue_reference_t<common_type_t<T1, T2>>,
	common_reference_t<add_lvalue_reference_t<const T1>, add_lvalue_reference_t<const T2>>>;

template <typename Derived, typename Base>
concept derived_from = is_base_of_v<Base, Derived> && convertible_to<const volatile Derived*, const volatile Base*>;


template<typename T, typename... Args>
concept constructible_from = is_constructible_v<T, Args...>;
template <typename T>
concept move_constructible = is_move_constructible_v<T>;
template<typename T>
concept copy_constructible = move_constructible<T>
&& constructible_from<T, T&>&& convertible_to<T&, T>
&& constructible_from<T, const T&>&& convertible_to<const T&, T>
&& constructible_from<T, const T>&& convertible_to<const T, T>;


template <typename T>
concept default_initializable = constructible_from<T> && requires {
	T{};
	::new (static_cast<void*>(nullptr)) T;
};


template <typename To, typename From>
concept assignable_from = is_lvalue_reference_v<To>
&& common_reference_with<const remove_reference_t<To>&, const remove_reference_t<From>&>
&& requires(To x, From&& y) {
	{ x = static_cast<From&&>(y) } -> same_as<To>;
};


template <typename T>
concept movable = is_object_v<T>
&& move_constructible<T>
&& assignable_from<T&, T>
&& is_swappable_v<T>;

template <typename T>
concept copyable = copy_constructible<T>
&& movable<T>
&& assignable_from<T&, T&>
&& assignable_from<T&, const T&>
&& assignable_from<T&, const T>;


template <typename T1, typename T2>
concept one_way_equality_comparable =
	requires(const remove_reference_t<T1>& x, const remove_reference_t<T2>& y) {
		{ x == y } -> convertible_to<bool>;
		{ x != y } -> convertible_to<bool>;
};

template <typename T1, typename T2>
concept both_equality_comparable =
one_way_equality_comparable<T1, T2>&& one_way_equality_comparable<T2, T1>;

template <typename T>
concept equality_comparable = one_way_equality_comparable<T, T>;

template <typename T1, typename T2>
concept equality_comparable_with = equality_comparable<T1> && equality_comparable<T2>
&& common_reference_with<const remove_reference_t<T1>&, const remove_reference_t<T2>&>
&& equality_comparable<common_reference_t<const remove_reference_t<T1>&, const remove_reference_t<T2>&>>
&& both_equality_comparable<T1, T2>;


template <typename T1, typename T2>
concept one_way_ordered = requires(const remove_reference_t<T1>& x, const remove_reference_t<T2>& y) {
	{ x < y } -> convertible_to<bool>;
	{ x > y } -> convertible_to<bool>;
	{ x <= y } -> convertible_to<bool>;
	{ x >= y } -> convertible_to<bool>;
};

template <typename T1, typename T2>
concept both_ordered_with = one_way_ordered<T1, T2>&& one_way_ordered<T2, T1>;

template <typename T>
concept totally_ordered = equality_comparable<T> && one_way_ordered<T, T>;

template <typename T1, typename T2>
concept totally_ordered_with = totally_ordered<T1> && totally_ordered<T2>
&& equality_comparable_with<T1, T2>
&& totally_ordered<common_reference_t<const remove_reference_t<T1>&, const remove_reference_t<T2>&>>
&& both_ordered_with<T1, T2>;


template <typename T>
concept semiregular = copyable<T> && default_initializable<T>;
template <typename T>
concept regular = semiregular<T> && equality_comparable<T>;


template <typename T>
concept iterator_typedef = requires() {
	typename iterator_traits<T>::iterator_category;
	typename iterator_traits<T>::value_type;
	typename iterator_traits<T>::difference_type;
	typename iterator_traits<T>::pointer;
	typename iterator_traits<T>::reference;
};

template <typename Iterator>
concept input_iterator = both_equality_comparable<Iterator, Iterator>
&& iterator_typedef<Iterator> && requires(Iterator it) {
	{ *it } -> convertible_to<typename iterator_traits<Iterator>::value_type>;
	{ ++it } -> same_as<Iterator&>;
	{ it++ } -> same_as<Iterator>;
};
template <typename Iterator>
concept forward_iterator = both_ordered_with<Iterator, Iterator> && semiregular<Iterator>
&& input_iterator<Iterator> && requires(Iterator it1, Iterator it2) {
	{ it1 - it2 } -> convertible_to<typename iterator_traits<Iterator>::difference_type>;
};
template <typename Iterator>
concept bidirectional_iterator = forward_iterator<Iterator> && requires(Iterator it) {
	{ --it } -> same_as<Iterator&>;
	{ it-- } -> same_as<Iterator>;
};
template <typename Iterator>
concept random_access_iterator = bidirectional_iterator<Iterator>
&& requires(Iterator it1, Iterator it2, typename iterator_traits<Iterator>::difference_type n) {
	{ it1 + n } -> convertible_to<Iterator>;
	{ n + it1 } -> convertible_to<Iterator>;
	{ it1 - n } -> convertible_to<Iterator>;
	{ it1 += n } -> convertible_to<Iterator>;
	{ it1 -= n } -> convertible_to<Iterator>;
	{ it2 - it1 } -> convertible_to<typename iterator_traits<Iterator>::difference_type>;
	{ it1[n] } -> convertible_to<typename iterator_traits<Iterator>::value_type>;
};
#endif // MSTL_VERSION_20__

MSTL_BEGIN_INNER__
template <typename, typename = void>
MSTL_INLINE17 constexpr bool __is_iterator_with_cate_v = false;
template <typename Iterator>
MSTL_INLINE17 constexpr bool __is_iterator_with_cate_v<Iterator, void_t<iter_cat_t<Iterator>>> = true;
MSTL_END_INNER__


template <typename Iterator>
MSTL_INLINE17 constexpr bool is_ranges_iter_v = _INNER __is_iterator_with_cate_v<Iterator>;

template <typename Iterator>
MSTL_INLINE17 constexpr bool is_iter_v =
#ifdef MSTL_VERSION_20__
iterator_typedef<Iterator> &&
#endif
is_ranges_iter_v<Iterator>;


template <typename Iterator>
MSTL_INLINE17 constexpr bool is_ranges_input_iter_v = is_convertible_v<iter_cat_t<Iterator>, input_iterator_tag>;

template <typename Iterator>
MSTL_INLINE17 constexpr bool is_input_iter_v =
#ifdef MSTL_VERSION_20__
input_iterator<Iterator> &&
#endif
is_ranges_input_iter_v<Iterator>;


template <typename Iterator>
MSTL_INLINE17 constexpr bool is_ranges_fwd_iter_v = is_convertible_v<iter_cat_t<Iterator>, forward_iterator_tag>;

template <typename Iterator>
MSTL_INLINE17 constexpr bool is_fwd_iter_v =
#ifdef MSTL_VERSION_20__
forward_iterator<Iterator> &&
#endif
is_ranges_fwd_iter_v<Iterator>;


template <typename Iterator>
MSTL_INLINE17 constexpr bool is_ranges_bid_iter_v = is_convertible_v<iter_cat_t<Iterator>, bidirectional_iterator_tag>;

template <typename Iterator>
MSTL_INLINE17 constexpr bool is_bid_iter_v =
#ifdef MSTL_VERSION_20__
bidirectional_iterator<Iterator> &&
#endif
is_ranges_bid_iter_v<Iterator>;


template <typename Iterator>
MSTL_INLINE17 constexpr bool is_ranges_rnd_iter_v = is_convertible_v<iter_cat_t<Iterator>, random_access_iterator_tag>;

template <typename Iterator>
MSTL_INLINE17 constexpr bool is_rnd_iter_v =
#ifdef MSTL_VERSION_20__
random_access_iterator<Iterator> &&
#endif
is_ranges_rnd_iter_v<Iterator>;


template <typename Iterator>
MSTL_INLINE17 constexpr bool is_ranges_cot_iter_v =
#ifdef MSTL_VERSION_20__
is_convertible_v<iter_cat_t<Iterator>, contiguous_iterator_tag>;
#else
is_pointer_v<Iterator>;
#endif // MSTL_VERSION_20__

template <typename Iterator>
MSTL_INLINE17 constexpr bool is_cot_iter_v =
#ifdef MSTL_VERSION_20__
random_access_iterator<Iterator> && 
#endif // MSTL_VERSION_20__
is_lvalue_reference_v<decltype(*_MSTL declval<Iterator&>())> && is_same_v<remove_cv_t<Iterator>, Iterator>
&& is_pod_v<iter_val_t<Iterator>> && is_ranges_cot_iter_v<Iterator>;

MSTL_END_NAMESPACE__
#endif // MSTL_CONCEPTS_HPP__
