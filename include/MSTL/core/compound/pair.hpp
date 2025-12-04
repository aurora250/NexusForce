#ifndef MSTL_CORE_COMPOUND_PAIR_HPP__
#define MSTL_CORE_COMPOUND_PAIR_HPP__
#include "../utility/integer_sequence.hpp"
#include "../utility/interface.hpp"
#include "../typeinfo/tags.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename...>
struct tuple;

template <typename>
struct tuple_size;
template <typename T>
constexpr size_t tuple_size_v = tuple_size<remove_cvref_t<T>>::value;

template <size_t, typename...>
struct tuple_element;
template <size_t Index, typename... Types>
using tuple_element_t = typename tuple_element<Index, Types...>::type;
template <size_t Index, typename... Types>
using tuple_extract_base_t = typename tuple_element<Index, Types...>::tuple_type;

template <size_t Index, typename... Types>
MSTL_NODISCARD constexpr tuple_element_t<Index, Types...>& get(tuple<Types...>& t) noexcept;
template <size_t Index, typename... Types>
MSTL_NODISCARD constexpr const tuple_element_t<Index, Types...>& get(const tuple<Types...>& t) noexcept;
template <size_t Index, typename... Types>
MSTL_NODISCARD constexpr tuple_element_t<Index, Types...>&& get(tuple<Types...>&& t) noexcept;
template <size_t Index, typename... Types>
MSTL_NODISCARD constexpr const tuple_element_t<Index, Types...>&& get(const tuple<Types...>&& t) noexcept;

MSTL_BEGIN_INNER__
template <size_t Index, typename... Types>
MSTL_NODISCARD constexpr tuple_element_t<Index, Types...>&& __pair_get_from_tuple(tuple<Types...>&& t) noexcept;
MSTL_END_INNER__


template <typename T1, typename T2>
struct pair : icommon<pair<T1, T2>> {
	using first_type	= T1;
	using second_type	= T2;

	T1 first;
	T2 second;

#ifdef MSTL_STANDARD_20__
	template <typename U1 = T1, typename U2 = T2, enable_if_t<
		conjunction_v<is_default_constructible<U1>, is_default_constructible<U2>>, int> = 0>
	constexpr explicit(!conjunction_v<
		is_implicitly_default_constructible<U1>, is_implicitly_default_constructible<U2>>)
		pair() noexcept(conjunction_v<
			is_nothrow_default_constructible<U1>, is_nothrow_default_constructible<U2>>)
		: first(), second() {}

	template <typename U1 = T1, typename U2 = T2, enable_if_t<
		conjunction_v<is_copy_constructible<U1>, is_copy_constructible<U2>>, int> = 0>
	constexpr explicit(!conjunction_v<is_convertible<const U1&, U1>, is_convertible<const U2&, U2>>)
		pair(const T1& a, const T2& b) noexcept(conjunction_v<
			is_nothrow_copy_constructible<U1>, is_nothrow_copy_constructible<U2>>)
		: first(a), second(b) {}

	template <typename U1, typename U2, enable_if_t<
		conjunction_v<is_constructible<T1, U1>, is_constructible<T2, U2>>, int> = 0>
	constexpr explicit(!conjunction_v<is_convertible<U1, T1>, is_convertible<U2, T2>>)
		pair(U1&& a, U2&& b) noexcept(conjunction_v<
			is_nothrow_constructible<T1, U1>, is_nothrow_constructible<T2, U2>>)
		: first(_MSTL forward<U1>(a)), second(_MSTL forward<U2>(b)) {}

	template <typename U1, typename U2, enable_if_t<
		conjunction_v<is_constructible<T1, const U1&>, is_constructible<T2, const U2&>>, int> = 0>
	constexpr explicit(!conjunction_v<is_convertible<const U1&, T1>, is_convertible<const U2&, T2>>)
		pair(const pair<U1, U2>& p) noexcept(conjunction_v<
			is_nothrow_constructible<T1, const U1&>, is_nothrow_constructible<T2, const U2&>>)
		: first(p.first), second(p.second) {}

	template <typename U1, typename U2, enable_if_t<
		conjunction_v<is_constructible<T1, U1>, is_constructible<T2, U2>>, int> = 0>
	constexpr explicit(!conjunction_v<
		is_convertible<U1, T1>, is_convertible<U2, T2>>)
		pair(pair<U1, U2>&& p) noexcept(conjunction_v<
			is_nothrow_constructible<T1, U1>, is_nothrow_constructible<T2, U2>>)
		: first(_MSTL forward<U1>(p.first)), second(_MSTL forward<U2>(p.second)) {}
#else
	template <typename U1 = T1, typename U2 = T2, enable_if_t<
		conjunction_v<is_default_constructible<U1>, is_default_constructible<U2>> &&
		!conjunction_v<is_implicitly_default_constructible<U1>,
		is_implicitly_default_constructible<U2>>, int> = 0>
	explicit pair() noexcept(conjunction_v<
		is_nothrow_default_constructible<U1>, is_nothrow_default_constructible<U2>>)
		: first(), second() {}

	template <typename U1 = T1, typename U2 = T2, enable_if_t<
		conjunction_v<is_default_constructible<U1>, is_default_constructible<U2>>&&
		conjunction_v<is_implicitly_default_constructible<U1>,
		is_implicitly_default_constructible<U2>>, int> = 0>
	pair() noexcept(conjunction_v<
		is_nothrow_default_constructible<U1>, is_nothrow_default_constructible<U2>>)
		: first(), second() {}

	template <typename U1 = T1, typename U2 = T2, enable_if_t<
		conjunction_v<is_copy_constructible<U1>, is_copy_constructible<U2>> &&
		!conjunction_v<is_convertible<const U1&, U1>, is_convertible<const U2&, U2>>, int> = 0>
	explicit pair(const T1& a, const T2& b) noexcept(conjunction_v<
		is_nothrow_copy_constructible<U1>, is_nothrow_copy_constructible<U2>>)
		: first(a), second(b) {}

	template <typename U1 = T1, typename U2 = T2, enable_if_t<
		conjunction_v<is_copy_constructible<U1>, is_copy_constructible<U2>>&&
		conjunction_v<is_convertible<const U1&, U1>, is_convertible<const U2&, U2>>, int> = 0>
	pair(const T1& a, const T2& b) noexcept(conjunction_v<
		is_nothrow_copy_constructible<U1>, is_nothrow_copy_constructible<U2>>)
		: first(a), second(b) {}

	template <typename U1, typename U2, enable_if_t<
		conjunction_v<is_constructible<T1, U1>, is_constructible<T2, U2>> &&
		!conjunction_v<is_convertible<U1, T1>, is_convertible<U2, T2>>, int> = 0>
	explicit pair(U1&& a, U2&& b) noexcept(conjunction_v<
		is_nothrow_constructible<T1, U1>, is_nothrow_constructible<T2, U2>>)
		: first(_MSTL forward<U1>(a)), second(_MSTL forward<U2>(b)) {}

	template <typename U1, typename U2, enable_if_t<
		conjunction_v<is_constructible<T1, U1>, is_constructible<T2, U2>> &&
		conjunction_v<is_convertible<U1, T1>, is_convertible<U2, T2>>, int> = 0>
	pair(U1&& a, U2&& b) noexcept(conjunction_v<
		is_nothrow_constructible<T1, U1>, is_nothrow_constructible<T2, U2>>)
		: first(_MSTL forward<U1>(a)), second(_MSTL forward<U2>(b)) {}

	template <typename U1, typename U2, enable_if_t<
		conjunction_v<is_constructible<T1, const U1&>, is_constructible<T2, const U2&>> &&
		!conjunction_v<is_convertible<const U1&, T1>, is_convertible<const U2&, T2>>, int> = 0>
	explicit pair(const pair<U1, U2>& p) noexcept(conjunction_v<
		is_nothrow_constructible<T1, const U1&>, is_nothrow_constructible<T2, const U2&>>)
		: first(p.first), second(p.second) {}

	template <typename U1, typename U2, enable_if_t<
		conjunction_v<is_constructible<T1, const U1&>, is_constructible<T2, const U2&>>&&
		conjunction_v<is_convertible<const U1&, T1>, is_convertible<const U2&, T2>>, int> = 0>
	pair(const pair<U1, U2>& p) noexcept(conjunction_v<
		is_nothrow_constructible<T1, const U1&>, is_nothrow_constructible<T2, const U2&>>)
		: first(p.first), second(p.second) {}

	template <typename U1, typename U2, enable_if_t<
		conjunction_v<is_constructible<T1, U1>, is_constructible<T2, U2>> &&
		!conjunction_v<is_convertible<U1, T1>, is_convertible<U2, T2>>, int> = 0>
	explicit pair(pair<U1, U2>&& p) noexcept(conjunction_v<
		is_nothrow_constructible<T1, U1>, is_nothrow_constructible<T2, U2>>)
		: first(_MSTL forward<U1>(p.first)), second(_MSTL forward<U2>(p.second)) {}

	template <typename U1, typename U2, enable_if_t<
		conjunction_v<is_constructible<T1, U1>, is_constructible<T2, U2>>&&
		conjunction_v<is_convertible<U1, T1>, is_convertible<U2, T2>>, int> = 0>
	pair(pair<U1, U2>&& p) noexcept(conjunction_v<
		is_nothrow_constructible<T1, U1>, is_nothrow_constructible<T2, U2>>)
		: first(_MSTL forward<U1>(p.first)), second(_MSTL forward<U2>(p.second)) {}
#endif

	pair(const pair& p) = default;
	pair(pair&& p) = default;

	template <typename Tuple1, typename Tuple2, size_t... Index1, size_t... Index2>
	constexpr pair(Tuple1& t1, Tuple2& t2, index_sequence<Index1...>, index_sequence<Index2...>)
		: first(_INNER __pair_get_from_tuple<Index1>(_MSTL move(t1))...),
		second(_INNER __pair_get_from_tuple<Index2>(_MSTL move(t2))...) {}

	// construct from tuple
	template <typename... Types1, typename... Types2>
	constexpr pair(_MSTL_TAG unpack_utility_construct_tag, tuple<Types1...> t1, tuple<Types2...> t2)
		: pair(t1, t2, index_sequence_for<Types1...>{}, index_sequence_for<Types2...>{}) {}


	// use identity_t to fasten type information
	template <typename self = pair, enable_if_t<conjunction_v<
		is_copy_assignable<typename self::first_type>, is_copy_assignable<typename self::second_type>>, int> = 0>
	constexpr pair& operator =(type_identity_t<const self&> p) noexcept(conjunction_v<
		is_nothrow_copy_assignable<T1>, is_nothrow_copy_assignable<T2>>) {
		first = p.first;
		second = p.second;
		return *this;
	}

	// use identity_t to fasten type information
	template <typename self = pair, enable_if_t<conjunction_v<
		is_move_assignable<typename self::first_type>, is_move_assignable<typename self::second_type>>, int> = 0>
	constexpr pair& operator =(type_identity_t<self&&> p) noexcept(conjunction_v<
		is_nothrow_move_assignable<T1>, is_nothrow_move_assignable<T2>>) {
		first = _MSTL forward<T1>(p.first);
		second = _MSTL forward<T2>(p.second);
		return *this;
	}

	template <typename U1, typename U2, enable_if_t<conjunction_v<negation<
		is_same<pair, pair<U1, U2>>>, is_assignable<T1&, const U1&>, is_assignable<T2&, const U2&>>, int> = 0>
	constexpr pair& operator =(const pair<U1, U2>& p) noexcept(conjunction_v<
		is_nothrow_assignable<T1&, const U1&>, is_nothrow_assignable<T2&, const U2&>>) {
		first = p.first;
		second = p.second;
		return *this;
	}

	template <typename U1, typename U2, enable_if_t<conjunction_v<negation<
		is_same<pair, pair<U1, U2>>>, is_assignable<T1&, U1>, is_assignable<T2&, U2>>, int> = 0>
	constexpr pair& operator =(pair<U1, U2>&& p) noexcept(conjunction_v<
		is_nothrow_assignable<T1&, U1>, is_nothrow_assignable<T2&, U2>>) {
		first = _MSTL forward<U1>(p.first);
		second = _MSTL forward<U2>(p.second);
		return *this;
	}

	pair& operator=(const volatile pair&) = delete;

	MSTL_CONSTEXPR20 ~pair() = default;

	constexpr bool operator ==(const pair& y) const
	noexcept(noexcept(this->first == y.first && this->second == y.second)) {
		return this->first == y.first && this->second == y.second;
	}
	constexpr bool operator <(const pair& y) const
	noexcept(noexcept(this->first < y.first || (!(y.first < this->first) && this->second < y.second))) {
		return this->first < y.first || (!(y.first < this->first) && this->second < y.second);
	}

	MSTL_NODISCARD constexpr size_t to_hash() const
	noexcept(noexcept(hash<remove_cvref_t<T1>>()(first) ^ hash<remove_cvref_t<T2>>()(second))) {
		return hash<remove_cvref_t<T1>>()(first) ^ hash<remove_cvref_t<T2>>()(second);
	}

	constexpr void swap(pair& p)
	noexcept(conjunction_v<is_nothrow_swappable<T1>, is_nothrow_swappable<T2>>) {
		_MSTL swap(first, p.first);
		_MSTL swap(second, p.second);
	}
};
#if MSTL_SUPPORT_DEDUCTION_GUIDES__
template <typename T1, typename T2>
pair(T1, T2) -> pair<T1, T2>;
#endif


template <typename T1, typename T2>
constexpr pair<unwrap_ref_decay_t<T1>, unwrap_ref_decay_t<T2>> make_pair(T1&& x, T2&& y)
noexcept(conjunction_v<is_nothrow_constructible<unwrap_ref_decay_t<T1>, T1>,
	is_nothrow_constructible<unwrap_ref_decay_t<T2>, T2>>) {
	using unwrap_pair = pair<unwrap_ref_decay_t<T1>, unwrap_ref_decay_t<T2>>;
	return unwrap_pair(_MSTL forward<T1>(x), _MSTL forward<T2>(y));
}


template <typename... Types>
struct tuple_size<tuple<Types...>> : integral_constant<size_t, sizeof...(Types)> {};

template <size_t Index>
struct tuple_element<Index, tuple<>> {};

template <typename This, typename... Rest>
struct tuple_element<0, tuple<This, Rest...>> {
	using type = This;
	using tuple_type = tuple<This, Rest...>;
};
template <size_t Index, typename This, typename... Rest>
struct tuple_element<Index, tuple<This, Rest...>>
	: tuple_element<Index - 1, tuple<Rest...>> {};

template <size_t Index, typename... Types>
struct tuple_element : tuple_element<Index, tuple<Types...>> {};


template <typename T1, typename T2>
struct tuple_size<pair<T1, T2>> : integral_constant<size_t, 2> {};


template <size_t Index, typename T1, typename T2>
struct tuple_element<Index, pair<T1, T2>> {
	static_assert(Index < 2, "pair element index out of range.");

	using type = conditional_t<Index == 0, T1, T2>;
	using tuple_type = tuple<T1, T2>;
};


#ifndef MSTL_STANDARD_17__
MSTL_BEGIN_INNER__

template <size_t Index, typename T1, typename T2>
struct __pair_get_helper;
template <typename T1, typename T2>
struct __pair_get_helper<0, T1, T2> {
	MSTL_NODISCARD constexpr static tuple_element_t<0, pair<T1, T2>>&
		get(pair<T1, T2>& pir) noexcept {
		return pir.first;
	}
	MSTL_NODISCARD constexpr static const tuple_element_t<0, pair<T1, T2>>&
		get(const pair<T1, T2>& pir) noexcept {
		return pir.first;
	}
	MSTL_NODISCARD constexpr static tuple_element_t<0, pair<T1, T2>>&&
		get(pair<T1, T2>&& pir) noexcept {
		return _MSTL forward<T1>(pir.first);
	}
	MSTL_NODISCARD constexpr static const tuple_element_t<0, pair<T1, T2>>&&
		get(const pair<T1, T2>&& pir) noexcept {
		return _MSTL forward<const T1>(pir.first);
	}
};

template <typename T1, typename T2>
struct __pair_get_helper<1, T1, T2> {
	MSTL_NODISCARD constexpr static tuple_element_t<1, pair<T1, T2>>&
		get(pair<T1, T2>& pir) noexcept {
		return pir.second;
	}
	MSTL_NODISCARD constexpr static const tuple_element_t<1, pair<T1, T2>>&
		get(const pair<T1, T2>& pir) noexcept {
		return pir.second;
	}
	MSTL_NODISCARD constexpr static tuple_element_t<1, pair<T1, T2>>&&
		get(pair<T1, T2>&& pir) noexcept {
		return _MSTL forward<T2>(pir.second);
	}
	MSTL_NODISCARD constexpr static const tuple_element_t<1, pair<T1, T2>>&&
		get(const pair<T1, T2>&& pir) noexcept {
		return _MSTL forward<const T2>(pir.second);
	}
};

MSTL_END_INNER__
#endif // !MSTL_STANDARD_17__


#ifdef MSTL_STANDARD_17__
template <size_t Index, typename T1, typename T2>
MSTL_NODISCARD constexpr tuple_element_t<Index, pair<T1, T2>>& get(pair<T1, T2>& pir) noexcept {
	if constexpr (Index == 0)
		return pir.first;
	else
		return pir.second;
}
#else
template <size_t Index, typename T1, typename T2>
MSTL_NODISCARD constexpr tuple_element_t<Index, pair<T1, T2>>&
get(pair<T1, T2>& pir) noexcept {
	return _INNER __pair_get_helper<Index, T1, T2>::get(pir);
}
#endif // MSTL_STANDARD_17__
template <typename T1, typename T2>
MSTL_NODISCARD constexpr T1& get(pair<T1, T2>& pir) noexcept {
	return pir.first;
}
template <typename T2, typename T1>
MSTL_NODISCARD constexpr T2& get(pair<T1, T2>& pir) noexcept {
	return pir.second;
}


#ifdef MSTL_STANDARD_17__
template <size_t Index, typename T1, typename T2>
MSTL_NODISCARD constexpr const tuple_element_t<Index, pair<T1, T2>>&
get(const pair<T1, T2>& pir) noexcept {
	if constexpr (Index == 0)
		return pir.first;
	else
		return pir.second;
}
#else
template <size_t Index, typename T1, typename T2>
MSTL_NODISCARD constexpr const tuple_element_t<Index, pair<T1, T2>>&
get(const pair<T1, T2>& pir) noexcept {
	return _INNER __pair_get_helper<Index, T1, T2>::get(pir);
}
#endif // MSTL_STANDARD_17__
template <typename T1, typename T2>
MSTL_NODISCARD constexpr const T1& get(const pair<T1, T2>& pir) noexcept {
	return pir.first;
}
template <typename T2, typename T1>
MSTL_NODISCARD constexpr const T2& get(const pair<T1, T2>& pir) noexcept {
	return pir.second;
}


#ifdef MSTL_STANDARD_17__
template <size_t Index, typename T1, typename T2>
MSTL_NODISCARD constexpr tuple_element_t<Index, pair<T1, T2>>&&
get(pair<T1, T2>&& pir) noexcept {
	if constexpr (Index == 0)
		return _MSTL forward<T1>(pir.first);
	else
		return _MSTL forward<T2>(pir.second);
}
#else
template <size_t Index, typename T1, typename T2>
MSTL_NODISCARD constexpr tuple_element_t<Index, pair<T1, T2>>&&
get(pair<T1, T2>&& pir) noexcept {
	return _MSTL forward<tuple_element_t<Index, pair<T1, T2>>>(
		_INNER __pair_get_helper<Index, T1, T2>::get(_MSTL forward<pair<T1, T2>>(pir)));
}
#endif // MSTL_STANDARD_17__
template <typename T1, typename T2>
MSTL_NODISCARD constexpr T1&& get(pair<T1, T2>&& pir) noexcept {
	return _MSTL forward<T1>(pir.first);
}
template <typename T2, typename T1>
MSTL_NODISCARD constexpr T2&& get(pair<T1, T2>&& pir) noexcept {
	return _MSTL forward<T2>(pir.second);
}


#ifdef MSTL_STANDARD_17__
template <size_t Index, typename T1, typename T2>
MSTL_NODISCARD constexpr const tuple_element_t<Index, pair<T1, T2>>&&
get(const pair<T1, T2>&& pir) noexcept {
	if constexpr (Index == 0)
		return _MSTL forward<const T1>(pir.first);
	else
		return _MSTL forward<const T2>(pir.second);
}
#else
template <size_t Index, typename T1, typename T2>
MSTL_NODISCARD constexpr const tuple_element_t<Index, pair<T1, T2>>&&
get(const pair<T1, T2>&& pir) noexcept {
	return _MSTL forward<const tuple_element_t<Index, pair<T1, T2>>>(
		_INNER __pair_get_helper<Index, T1, T2>::get(_MSTL forward<const pair<T1, T2>>(pir)));
}
#endif // MSTL_STANDARD_17__
template <typename T1, typename T2>
MSTL_NODISCARD constexpr const T1&& get(const pair<T1, T2>&& pir) noexcept {
	return _MSTL forward<const T1>(pir.first);
}
template <typename T2, typename T1>
MSTL_NODISCARD constexpr const T2&& get(const pair<T1, T2>&& pir) noexcept {
	return _MSTL forward<const T2>(pir.second);
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_COMPOUND_PAIR_HPP__
