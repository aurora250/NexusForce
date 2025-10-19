#ifndef MSTL_TUPLE_HPP__
#define MSTL_TUPLE_HPP__
#include "hash.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_BEGIN_INNER__
template <bool Same, typename Dest, typename... Srcs>
MSTL_INLINE17 constexpr bool __tuple_constructible_aux = false;
template <typename... Dests, typename... Srcs>
MSTL_INLINE17 constexpr bool __tuple_constructible_aux<true, tuple<Dests...>, Srcs...> =
	conjunction_v<is_constructible<Dests, Srcs>...>;
MSTL_END_INNER__

template <typename Dest, typename... Srcs>
MSTL_INLINE17 constexpr bool tuple_constructible_v =
	_INNER __tuple_constructible_aux<tuple_size_v<Dest> == sizeof...(Srcs), Dest, Srcs...>;

template <typename Dest, typename... Srcs>
struct tuple_constructible : bool_constant<tuple_constructible_v<Dest, Srcs...>> {};


MSTL_BEGIN_INNER__
template <bool Same, typename Dest, typename... Srcs>
MSTL_INLINE17 constexpr bool __tuple_explicitly_convertible_aux = false;
template <typename... Dests, typename... Srcs>
MSTL_INLINE17 constexpr bool __tuple_explicitly_convertible_aux<true, tuple<Dests...>, Srcs...> =
!conjunction_v<is_convertible<Srcs, Dests>...>;
MSTL_END_INNER__

template <typename Dest, typename... Srcs>
MSTL_INLINE17 constexpr bool tuple_explicitly_convertible_v =
	_INNER __tuple_explicitly_convertible_aux<tuple_size_v<Dest> == sizeof...(Srcs), Dest, Srcs...>;


template <typename, typename, typename...>
struct tuple_perfect_forward : true_type {};
template <typename Tuple1, typename Tuple2>
struct tuple_perfect_forward<Tuple1, Tuple2> : bool_constant<!is_same_v<Tuple1, remove_cvref_t<Tuple2>>> {};

template <typename T1, typename T2, typename U1, typename U2>
struct tuple_perfect_forward<tuple<T1, T2>, U1, U2>
	: bool_constant<disjunction_v<negation<is_same<remove_cvref_t<U1>, _MSTL_TAG allocator_arg_tag>>,
	is_same<remove_cvref_t<T1>, _MSTL_TAG allocator_arg_tag>>> {};

template <typename T1, typename T2, typename T3, typename U1, typename U2, typename U3>
struct tuple_perfect_forward<tuple<T1, T2, T3>, U1, U2, U3>
	: bool_constant<disjunction_v<negation<is_same<remove_cvref_t<U1>, _MSTL_TAG allocator_arg_tag>>,
	is_same<remove_cvref_t<T1>, _MSTL_TAG allocator_arg_tag>>> {};


MSTL_BEGIN_INNER__
template <bool Same, typename Dest, typename... Srcs>
MSTL_INLINE17 constexpr bool __tuple_nothrow_constructible_aux = false;
template <typename... Dests, typename... Srcs>
MSTL_INLINE17 constexpr bool __tuple_nothrow_constructible_aux<true, tuple<Dests...>, Srcs...> =
conjunction_v<is_nothrow_constructible<Dests, Srcs>...>;
MSTL_END_INNER__

template <typename Dest, typename... Srcs>
MSTL_INLINE17 constexpr bool tuple_nothrow_constructible_v =
	_INNER __tuple_nothrow_constructible_aux<tuple_size_v<Dest> == sizeof...(Srcs), Dest, Srcs...>;


MSTL_BEGIN_INNER__
template <typename Self, typename Tuple, typename... U>
struct __tuple_convertible_aux : true_type {};
template <typename Self, typename Tuple, typename U>
struct __tuple_convertible_aux<tuple<Self>, Tuple, U>
	: bool_constant<!disjunction_v<
	is_same<Self, U>, is_constructible<Self, Tuple>, is_convertible<Tuple, Self>>> {};
MSTL_END_INNER__

template <typename Self, typename Tuple, typename... U>
constexpr bool tuple_convertible_v = _INNER __tuple_convertible_aux<Self, Tuple, U...>::value;
template <typename Self, typename Tuple, typename... U>
struct tuple_convertible : bool_constant<tuple_convertible_v<Self, Tuple, U...>> {};


MSTL_BEGIN_INNER__
template <bool Same, typename Dest, typename... Srcs>
MSTL_INLINE17 constexpr bool __tuple_assignable_aux = false;
template <typename... Dests, typename... Srcs>
MSTL_INLINE17 constexpr bool __tuple_assignable_aux<true, tuple<Dests...>, Srcs...> = conjunction_v<
	is_assignable<Dests&, Srcs>...>;
MSTL_END_INNER__

template <typename Dest, typename... Srcs>
MSTL_INLINE17 constexpr bool tuple_assignable_v =
	_INNER __tuple_assignable_aux<tuple_size_v<Dest> == sizeof...(Srcs), Dest, Srcs...>;

template <typename Dest, typename... Srcs>
struct tuple_assignable : bool_constant<tuple_assignable_v<Dest, Srcs...>> {};


MSTL_BEGIN_INNER__
template <bool Same, typename Dest, typename... Srcs>
MSTL_INLINE17 constexpr bool __tuple_nothrow_assignable_aux = false;
template <typename... Dests, typename... Srcs>
MSTL_INLINE17 constexpr bool __tuple_nothrow_assignable_aux<true, tuple<Dests...>, Srcs...> =
	conjunction_v<is_nothrow_assignable<Dests&, Srcs>...>;
MSTL_END_INNER__

template <typename Dest, typename... Srcs>
MSTL_INLINE17 constexpr bool tuple_nothrow_assignable_v =
	_INNER __tuple_nothrow_assignable_aux<tuple_size_v<Dest> == sizeof...(Srcs), Dest, Srcs...>;


template <>
struct tuple<> : icommon<tuple<>> {
	using self = tuple<>;

	constexpr tuple() noexcept = default;
	constexpr tuple(const tuple&) noexcept = default;
	template <typename Tag, enable_if_t<is_same_v<Tag, _MSTL_TAG exact_arg_construct_tag>, int> = 0>
	constexpr explicit tuple(Tag) noexcept {}

	constexpr tuple& operator =(const tuple&) noexcept = default;

	MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr bool equal_to(const tuple&) const noexcept { return true; }
	MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr bool less_to(const tuple&) const noexcept { return false; }

	MSTL_ALWAYS_INLINE constexpr void swap(tuple&) noexcept {}

	MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr bool operator ==(const self& rh) const noexcept { return this->equal_to(rh); }
	MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr bool operator !=(const self& rh) const noexcept { return !(*this == rh); }
	MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr bool operator <(const self& rh) const noexcept { return this->less_to(rh); }
	MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr bool operator >(const self& rh) const noexcept { return rh < *this; }
	MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr bool operator <=(const self& rh) const noexcept { return !(rh < *this); }
	MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr bool operator >=(const self& rh) const noexcept { return !(*this < rh); }

	MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr size_t to_hash() const noexcept { return FNV_OFFSET_BASIS; }
};

template <typename This, typename... Rest>
struct tuple<This, Rest...> : private tuple<Rest...>, icommon<tuple<This, Rest...>> {
	using this_type = This;
	using base_type = tuple<Rest...>;
	using self = tuple<This, Rest...>;

private:
	this_type data_;

	template <typename Tuple, size_t... Idx>
	static constexpr size_t __broaden_tuple(const Tuple& tup, _MSTL index_sequence<Idx...>) noexcept;

public:
	template <typename Tag, typename U1, typename... U2, enable_if_t<
		is_same_v<Tag, _MSTL_TAG exact_arg_construct_tag>, int> = 0>
	constexpr tuple(Tag, U1&& this_arg, U2&&... rest_arg)
		: base_type(_MSTL_TAG exact_arg_construct_tag{}, _MSTL forward<U2>(rest_arg)...), data_(_MSTL forward<U1>(this_arg)) {}

	template <typename Tag, typename Tuple, size_t... Index, enable_if_t<
		is_same_v<Tag, _MSTL_TAG unpack_utility_construct_tag>, int> = 0>
	constexpr tuple(Tag, Tuple&&, index_sequence<Index...>);

	template <typename Tag, typename Tuple, enable_if_t<
		is_same_v<Tag, _MSTL_TAG unpack_utility_construct_tag>, int> = 0>
	constexpr tuple(Tag, Tuple&& tup) 
		: tuple(_MSTL_TAG unpack_utility_construct_tag{}, _MSTL forward<Tuple>(tup),
		make_index_sequence<tuple_size_v<remove_reference_t<Tuple>>>{}) {}


#ifdef MSTL_VERSION_20__
	template <typename T = This, enable_if_t<
		conjunction_v<is_default_constructible<T>, is_default_constructible<Rest>...>, int> = 0>
	constexpr explicit(!conjunction_v<
		is_implicitly_default_constructible<T>, is_implicitly_default_constructible<Rest>...>)
		tuple() noexcept(conjunction_v<
			is_nothrow_default_constructible<T>, is_nothrow_default_constructible<Rest>...>)
		: base_type(), data_() {}

	template <typename T = This, enable_if_t<
		tuple_constructible_v<tuple, const T&, const Rest&...>, int> = 0>
	constexpr explicit(tuple_explicitly_convertible_v<tuple, const T&, const Rest&...>)
		tuple(const T& this_arg, const Rest&... rest_arg) noexcept(conjunction_v<
			is_nothrow_copy_constructible<T>, is_nothrow_copy_constructible<Rest>...>)
		: tuple(_MSTL_TAG exact_arg_construct_tag{}, this_arg, rest_arg...) {}

	template <typename U1, typename... U2, enable_if_t<conjunction_v<tuple_perfect_forward<tuple, U1, U2...>, 
		tuple_constructible<tuple, U1, U2...>>, int> = 0>
	constexpr explicit(tuple_explicitly_convertible_v<tuple, U1, U2...>)
		tuple(U1&& this_arg, U2&&... rest_arg) noexcept(tuple_nothrow_constructible_v<tuple, U1, U2...>)
		: tuple(_MSTL_TAG exact_arg_construct_tag{}, _MSTL forward<U1>(this_arg), _MSTL forward<U2>(rest_arg)...) {}

	template <typename... U, enable_if_t<conjunction_v<tuple_constructible<tuple, const U&...>,
		tuple_convertible<tuple, const tuple<U...>&, U...>>, int> = 0>
	constexpr explicit(tuple_explicitly_convertible_v<tuple, const U&...>)
		tuple(const tuple<U...>& tup) noexcept(tuple_nothrow_constructible_v<tuple, const U&...>)
		: tuple(_MSTL_TAG unpack_utility_construct_tag{}, tup) {}

	template <typename... U, enable_if_t<conjunction_v<tuple_constructible<tuple, U...>, 
		tuple_convertible<tuple, tuple<U...>, U...>>, int> = 0>
	constexpr explicit(tuple_explicitly_convertible_v<tuple, U...>)
		tuple(tuple<U...>&& tup) noexcept(tuple_nothrow_constructible_v<tuple, U...>)
		: tuple(_MSTL_TAG unpack_utility_construct_tag{}, _MSTL move(tup)) {}

	template <typename T1, typename T2, enable_if_t<
		tuple_constructible_v<tuple, const T1&, const T2&>, int> = 0>
	constexpr explicit(tuple_explicitly_convertible_v<tuple, const T1&, const T2&>)
        tuple(const pair<T1, T2>& pir) noexcept(tuple_nothrow_constructible_v<tuple, const T1&, const T2&>)
        : tuple(_MSTL_TAG unpack_utility_construct_tag{}, pir) {}

    template <typename T1, typename T2, enable_if_t<tuple_constructible_v<tuple, T1, T2>, int> = 0>
	constexpr explicit(tuple_explicitly_convertible_v<tuple, T1, T2>)
		tuple(pair<T1, T2>&& pir) noexcept(tuple_nothrow_constructible_v<tuple, T1, T2>)
        : tuple(_MSTL_TAG unpack_utility_construct_tag{}, _MSTL move(pir)) {}
#else
	template <typename T = This, enable_if_t<
		conjunction_v<is_default_constructible<T>, is_default_constructible<Rest>...>&&
		!conjunction_v<is_implicitly_default_constructible<T>,
		is_implicitly_default_constructible<Rest>...>, int> = 0>
	explicit tuple() noexcept(conjunction_v<
		is_nothrow_default_constructible<T>, is_nothrow_default_constructible<Rest>...>)
		: base_type(), data_() {}

	template <typename T = This, enable_if_t<
		conjunction_v<is_default_constructible<T>, is_default_constructible<Rest>...> &&
		conjunction_v<is_implicitly_default_constructible<T>,
		is_implicitly_default_constructible<Rest>...>, int> = 0>
	tuple() noexcept(conjunction_v<
		is_nothrow_default_constructible<T>, is_nothrow_default_constructible<Rest>...>)
		: base_type(), data_() {}

	template <typename T = This, enable_if_t<tuple_constructible_v<tuple, const T&, const Rest&...>&&
		tuple_explicitly_convertible_v<tuple, const T&, const Rest&...>, int> = 0>
	explicit tuple(const T& this_arg, const Rest&... rest_arg) noexcept(conjunction_v<
		is_nothrow_copy_constructible<T>, is_nothrow_copy_constructible<Rest>...>)
		: tuple(_MSTL_TAG exact_arg_construct_tag{}, this_arg, rest_arg...) {}

	template <typename T = This, enable_if_t<tuple_constructible_v<tuple, const T&, const Rest&...> &&
		!tuple_explicitly_convertible_v<tuple, const T&, const Rest&...>, int> = 0>
	tuple(const T& this_arg, const Rest&... rest_arg) noexcept(conjunction_v<
		is_nothrow_copy_constructible<T>, is_nothrow_copy_constructible<Rest>...>)
		: tuple(_MSTL_TAG exact_arg_construct_tag{}, this_arg, rest_arg...) {}

	template <typename U1, typename... U2, enable_if_t<
		conjunction_v<tuple_constructible<tuple, U1, U2...>, tuple_convertible<tuple, U1, U2...>>&&
		tuple_explicitly_convertible_v<tuple, U1, U2...>, int> = 0>
	explicit tuple(U1&& this_arg, U2&&... rest_arg) noexcept(tuple_nothrow_constructible_v<tuple, U1, U2...>)
		: tuple(_MSTL_TAG exact_arg_construct_tag{}, _MSTL forward<U1>(this_arg), _MSTL forward<U2>(rest_arg)...) {}

	template <typename U1, typename... U2, enable_if_t<
		conjunction_v<tuple_constructible<tuple, U1, U2...>, tuple_convertible<tuple, U1, U2...>> &&
		!tuple_explicitly_convertible_v<tuple, U1, U2...>, int> = 0>
	tuple(U1&& this_arg, U2&&... rest_arg) noexcept(tuple_nothrow_constructible_v<tuple, U1, U2...>)
		: tuple(_MSTL_TAG exact_arg_construct_tag{}, _MSTL forward<U1>(this_arg), _MSTL forward<U2>(rest_arg)...) {}

	template <typename... U, enable_if_t<conjunction_v<tuple_constructible<tuple, const U&...>, 
		tuple_convertible<tuple, const tuple<U...>&, U...>> &&
		tuple_explicitly_convertible_v<tuple, const U&...>, int> = 0>
	explicit tuple(const tuple<U...>& tup) noexcept(tuple_nothrow_constructible_v<tuple, const U&...>)
		: tuple(_MSTL_TAG unpack_utility_construct_tag{}, tup) {}

	template <typename... U, enable_if_t<conjunction_v<tuple_constructible<tuple, const U&...>, 
		tuple_convertible<tuple, const tuple<U...>&, U...>> &&
		!tuple_explicitly_convertible_v<tuple, const U&...>, int> = 0>
	tuple(const tuple<U...>& tup) noexcept(tuple_nothrow_constructible_v<tuple, const U&...>)
		: tuple(_MSTL_TAG unpack_utility_construct_tag{}, tup) {}

	template <typename... U, enable_if_t<
		conjunction_v<tuple_constructible<tuple, U...>, tuple_convertible<tuple, tuple<U...>, U...>>&&
		tuple_explicitly_convertible_v<tuple, U...>, int> = 0>
	explicit tuple(tuple<U...>&& tup) noexcept(tuple_nothrow_constructible_v<tuple, U...>)
		: tuple(_MSTL_TAG unpack_utility_construct_tag{}, _MSTL move(tup)) {}

	template <typename... U, enable_if_t<
		conjunction_v<tuple_constructible<tuple, U...>, tuple_convertible<tuple, tuple<U...>, U...>> &&
		!tuple_explicitly_convertible_v<tuple, U...>, int> = 0>
	tuple(tuple<U...>&& tup) noexcept(tuple_nothrow_constructible_v<tuple, U...>)
		: tuple(_MSTL_TAG unpack_utility_construct_tag{}, _MSTL move(tup)) {}

	template <typename T1, typename T2, enable_if_t<tuple_constructible_v<tuple, const T1&, const T2&>&&
		tuple_explicitly_convertible_v<tuple, const T1&, const T2&>, int> = 0>
	explicit tuple(const pair<T1, T2>& pir) noexcept(tuple_nothrow_constructible_v<tuple, const T1&, const T2&>)
		: tuple(_MSTL_TAG unpack_utility_construct_tag{}, pir) {}

	template <typename T1, typename T2, enable_if_t<tuple_constructible_v<tuple, const T1&, const T2&> && 
		!tuple_explicitly_convertible_v<tuple, const T1&, const T2&>, int> = 0>
	tuple(const pair<T1, T2>& pir) noexcept(tuple_nothrow_constructible_v<tuple, const T1&, const T2&>)
		: tuple(_MSTL_TAG unpack_utility_construct_tag{}, pir) {}

	template <typename T1, typename T2, enable_if_t<
		tuple_constructible_v<tuple, T1, T2> && tuple_explicitly_convertible_v<tuple, T1, T2>, int> = 0>
	explicit tuple(pair<T1, T2>&& pir) noexcept(tuple_nothrow_constructible_v<tuple, T1, T2>)
		: tuple(_MSTL_TAG unpack_utility_construct_tag{}, _MSTL move(pir)) {}

	template <typename T1, typename T2, enable_if_t<
		tuple_constructible_v<tuple, T1, T2> && !tuple_explicitly_convertible_v<tuple, T1, T2>, int> = 0>
	tuple(pair<T1, T2>&& pir) noexcept(tuple_nothrow_constructible_v<tuple, T1, T2>)
		: tuple(_MSTL_TAG unpack_utility_construct_tag{}, _MSTL move(pir)) {}
#endif

	tuple(const tuple&) = default;
	tuple(tuple&&) = default;

	template <typename Self = tuple, typename T = This, enable_if_t<
		conjunction_v<is_copy_assignable<T>, is_copy_assignable<Rest>...>, int> = 0>
	constexpr tuple& operator =(type_identity_t<const tuple&> tup) noexcept(conjunction_v<
		is_nothrow_copy_assignable<T>, is_nothrow_copy_assignable<Rest>...>) {
		data_ = tup.data_;
		get_rest() = tup.get_rest();
		return *this;
	}

	template <typename Self = tuple, typename T = This, enable_if_t<
		conjunction_v<is_move_assignable<T>, is_move_assignable<Rest>...>, int> = 0>
	constexpr tuple& operator =(type_identity_t<tuple&&> tup) noexcept(conjunction_v<
		is_nothrow_move_assignable<T>, is_nothrow_move_assignable<Rest>...>) {
		data_ = _MSTL forward<T>(tup.data_);
		get_rest() = _MSTL forward<base_type>(tup.get_rest());
		return *this;
	}

	template <typename... U, enable_if_t<
		conjunction_v<negation<is_same<tuple, tuple<U...>>>, tuple_assignable<tuple, const U&...>>, int> = 0>
	constexpr tuple& operator =(const tuple<U...>& tup) noexcept(tuple_nothrow_assignable_v<tuple, const U&...>) {
		data_ = tup.data_;
		get_rest() = tup.get_rest();
		return *this;
	}

	template <typename... U, enable_if_t<
		conjunction_v<negation<is_same<tuple, tuple<U...>>>, tuple_assignable<tuple, U...>>, int> = 0>
	constexpr tuple& operator =(tuple<U...>&& tup) noexcept(tuple_nothrow_assignable_v<tuple, U...>) {
		data_ = _MSTL forward<typename tuple<U...>::this_type>(tup.data_);
		get_rest() = _MSTL forward<typename tuple<U...>::super>(tup.get_rest());
		return *this;
	}

	template <typename T1, typename T2, enable_if_t<
		tuple_assignable_v<tuple, const T1&, const T2&>, int> = 0>
	constexpr tuple& operator =(const pair<T1, T2>& pir) noexcept(
		tuple_nothrow_assignable_v<tuple, const T1&, const T2&>) {
		data_ = pir.first;
		get_rest().data_ = pir.second;
		return *this;
	}

	template <typename T1, typename T2, enable_if_t<
		tuple_assignable_v<tuple, T1, T2>, int> = 0>
	constexpr tuple& operator =(pair<T1, T2>&& pir) noexcept(
		tuple_nothrow_assignable_v<tuple, T1, T2>) {
		data_ = _MSTL forward<T1>(pir.first);
		get_rest().data_ = _MSTL forward<T2>(pir.second);
		return *this;
	}

	tuple& operator =(const volatile tuple&) = delete;

	constexpr base_type& get_rest() noexcept { return *this; }
	constexpr const base_type& get_rest() const noexcept { return *this; }

	template <typename... U>
	MSTL_NODISCARD constexpr bool equal_to(const tuple<U...>& t) const {
		return data_ == t.data_ && base_type::equal_to(t.get_rest());
	}
	template <typename... U>
	MSTL_NODISCARD constexpr bool less_to(const tuple<U...>& rh) const {
		return data_ < rh.data_ || (!(rh.data_ < data_) && base_type::less_to(rh.get_rest()));
	}

	template <size_t Index, typename... Types>
	friend constexpr tuple_element_t<Index, Types...>& get(tuple<Types...>&) noexcept;
	template <size_t Index, typename... Types>
	friend constexpr const tuple_element_t<Index, Types...>& get(const tuple<Types...>&) noexcept;
	template <size_t Index, typename... Types>
	friend constexpr tuple_element_t<Index, Types...>&& get(tuple<Types...>&&) noexcept;
	template <size_t Index, typename... Types>
	friend constexpr const tuple_element_t<Index, Types...>&& get(const tuple<Types...>&&) noexcept;

	template <size_t Index, typename... Types>
	friend constexpr tuple_element_t<Index, Types...>&& pair_get_from_tuple(tuple<Types...>&&) noexcept;

	MSTL_NODISCARD constexpr bool operator ==(const self& rh) const noexcept { return this->equal_to(rh); }
	MSTL_NODISCARD constexpr bool operator !=(const self& rh) const noexcept { return !(*this == rh); }
	MSTL_NODISCARD constexpr bool operator <(const self& rh) const noexcept { return this->less_to(rh); }
	MSTL_NODISCARD constexpr bool operator >(const self& rh) const noexcept { return rh < *this; }
	MSTL_NODISCARD constexpr bool operator <=(const self& rh) const noexcept { return !(rh < *this); }
	MSTL_NODISCARD constexpr bool operator >=(const self& rh) const noexcept { return !(*this < rh); }

	MSTL_NODISCARD constexpr size_t to_hash() const noexcept {
		return self::__broaden_tuple(*this, _MSTL index_sequence_for<This, Rest...>());
	}

	constexpr void swap(self& t)
	noexcept(conjunction_v<is_nothrow_swappable<This>, is_nothrow_swappable<Rest>...>) {
		_MSTL swap(data_, t.data_);
		base_type::swap(t.get_rest());
	}
};
#ifdef MSTL_SUPPORT_DEDUCTION_GUIDES__
template <typename... Types>
tuple(Types...) -> tuple<Types...>;

template <typename T1, typename T2>
tuple(pair<T1, T2>) -> tuple<T1, T2>;
#endif


template <size_t Index, typename... Types>
MSTL_NODISCARD constexpr tuple_element_t<Index, Types...>& get(tuple<Types...>& t) noexcept {
	using T = tuple_element_t<Index, tuple<Types...>>;
	using tuple_type = tuple_extract_base_t<Index, tuple<Types...>>;
	return static_cast<T&>(static_cast<tuple_type&>(t).data_);
}
template <size_t Index, typename... Types>
MSTL_NODISCARD constexpr const tuple_element_t<Index, Types...>& get(const tuple<Types...>& t) noexcept {
	using T = tuple_element_t<Index, tuple<Types...>>;
	using tuple_type = tuple_extract_base_t<Index, tuple<Types...>>;
	return static_cast<const T&>(static_cast<const tuple_type&>(t).data_);
}
template <size_t Index, typename... Types>
MSTL_NODISCARD constexpr tuple_element_t<Index, Types...>&& get(tuple<Types...>&& t) noexcept {
	using T = tuple_element_t<Index, tuple<Types...>>;
	using tuple_type = tuple_extract_base_t<Index, tuple<Types...>>;
	return static_cast<T&&>(static_cast<tuple_type&&>(t).data_);
}
template <size_t Index, typename... Types>
MSTL_NODISCARD constexpr const tuple_element_t<Index, Types...>&& get(const tuple<Types...>&& t) noexcept {
	using T = tuple_element_t<Index, tuple<Types...>>;
	using tuple_type = tuple_extract_base_t<Index, tuple<Types...>>;
	return static_cast<const T&&>(static_cast<const tuple_type&&>(t).data_);
}

MSTL_BEGIN_INNER__
template <size_t Index, typename... Types>
MSTL_NODISCARD constexpr tuple_element_t<Index, Types...>&& __pair_get_from_tuple(tuple<Types...>&& t) noexcept {
	using T = tuple_element_t<Index, tuple<Types...>>;
	using tuple_type = tuple_extract_base_t<Index, tuple<Types...>>;
	return static_cast<T&&>(static_cast<tuple_type&>(t).data_);
}
MSTL_END_INNER__


template <typename This, typename... Rest>
template <typename Tag, typename Tuple, size_t... Index, enable_if_t<
	is_same_v<Tag, unpack_utility_construct_tag>, int>>
constexpr tuple<This, Rest...>::tuple(Tag, Tuple&& tup, index_sequence<Index...>)
	: tuple(_MSTL_TAG exact_arg_construct_tag{}, _MSTL get<Index>(_MSTL forward<Tuple>(tup))...) {}


template <typename... Types>
MSTL_NODISCARD constexpr tuple<unwrap_ref_decay_t<Types>...> make_tuple(Types&&... args) {
	using tuple_type = tuple<unwrap_ref_decay_t<Types>...>;
	return tuple_type(_MSTL forward<Types>(args)...);
}
template <typename... Types>
MSTL_NODISCARD constexpr tuple<Types&...> tie(Types&... args) noexcept {
	using tuple_type = tuple<Types&...>;
	return tuple_type(args...);
}
template <typename... Types>
MSTL_NODISCARD constexpr tuple<Types&&...> forward_as_tuple(Types&&... args) noexcept {
	using tuple_type = tuple<Types&&...>;
	return tuple_type(_MSTL forward<Types>(args)...);
}


MSTL_BEGIN_INNER__

template <typename, typename Tuple, typename = make_index_sequence<tuple_size_v<remove_reference_t<Tuple>>>>
MSTL_INLINE17 constexpr bool __constructible_from_tuple = false;
template <typename T, typename Tuple, size_t... Index>
MSTL_INLINE17 constexpr bool __constructible_from_tuple<T, Tuple, index_sequence<Index...>> =
is_constructible_v<T, decltype(_MSTL get<Index>(_MSTL declval<Tuple>()))...>;


template <typename T, typename Tuple, size_t... Index>
MSTL_NODISCARD constexpr T __broaden_make_from_tuple(Tuple&& tup, index_sequence<Index...>)
noexcept(is_nothrow_constructible_v<T, decltype(_MSTL get<Index>(_MSTL forward<Tuple>(tup)))...>) {
	return T(_MSTL get<Index>(_MSTL forward<Tuple>(tup))...);
}

MSTL_END_INNER__


template <typename T, typename Tuple, enable_if_t<_INNER __constructible_from_tuple<T, Tuple>, int> = 0>
MSTL_NODISCARD constexpr T make_from_tuple(Tuple&& tup) noexcept(noexcept(_INNER __broaden_make_from_tuple<T>(
	_MSTL forward<Tuple>(tup), make_index_sequence<tuple_size_v<remove_reference_t<Tuple>>>{}))) {
	return _INNER __broaden_make_from_tuple<T>(
		_MSTL forward<Tuple>(tup), make_index_sequence<tuple_size_v<remove_reference_t<Tuple>>>{});
}


MSTL_BEGIN_INNER__
template <template <typename...> class Trait, typename T, typename Tuple>
struct __apply_unpack_tuple;
MSTL_END_INNER__

template <typename F, typename Tuple>
constexpr decltype(auto) apply(F&& f, Tuple&& t)
noexcept(_INNER __apply_unpack_tuple<_MSTL is_nothrow_invocable, F, Tuple>::value);


MSTL_BEGIN_INNER__

template <typename, typename, typename, size_t, typename...>
struct __tuple_cat_aux;

template <typename Tuple, size_t... ElementIdx, size_t... TupleIdx, size_t NextTuple>
struct __tuple_cat_aux<Tuple, index_sequence<ElementIdx...>, index_sequence<TupleIdx...>, NextTuple> {
	using Ret			= tuple<tuple_element_t<ElementIdx, remove_cvref_t<tuple_element_t<TupleIdx, Tuple>>>...>;
	using ElementIdxSeq = index_sequence<ElementIdx...>;
	using TupleIdxSeq	= index_sequence<TupleIdx...>;
};

template <typename Tuple, size_t... ElementIdx, size_t... TupleIdx, size_t NextTuple, size_t... NextElement, typename... Rest>
struct __tuple_cat_aux<Tuple, index_sequence<ElementIdx...>, index_sequence<TupleIdx...>, NextTuple, index_sequence<NextElement...>, Rest...>
	: __tuple_cat_aux<Tuple, index_sequence<ElementIdx..., NextElement...>,
		index_sequence<TupleIdx..., (NextTuple + 0 * NextElement)...>, NextTuple + 1, Rest...> {};

template <typename... Tuples>
using __tuple_cat_bind_t = __tuple_cat_aux<tuple<Tuples&&...>, index_sequence<>, index_sequence<>, 0,
	make_index_sequence<tuple_size_v<remove_cvref_t<Tuples>>>...>;

template <typename Ret, size_t... ElementIdx, size_t... TupleIdx, typename Tuple>
constexpr Ret __tuple_cat_in_turn(index_sequence<ElementIdx...>, index_sequence<TupleIdx...>, Tuple tup) {
	return Ret{ _MSTL get<ElementIdx>(_MSTL get<TupleIdx>(_MSTL move(tup)))... };
}

MSTL_END_INNER__


template <typename... Tuples>
MSTL_NODISCARD constexpr typename _INNER __tuple_cat_bind_t<Tuples...>::Ret tuple_cat(Tuples&&... tuples) {
	using CatImpl		= _INNER __tuple_cat_bind_t<Tuples...>;
	using Ret			= typename CatImpl::Ret;
	using ElementIdxSeq	= typename CatImpl::ElementIdxSeq;
	using TupleIdxSeq	= typename CatImpl::TupleIdxSeq;
	return _INNER __tuple_cat_in_turn<Ret>(ElementIdxSeq{}, TupleIdxSeq{}, _MSTL forward_as_tuple(_MSTL forward<Tuples>(tuples)...));
}


#if !defined(MSTL_VERSION_17__)
MSTL_BEGIN_INNER__

template <typename Tuple, size_t Index>
struct __broadern_tuple_hash_aux {
	static constexpr size_t hash(const Tuple& tup) {
		using ElementType = remove_cvref_t<tuple_element_t<Index - 1, Tuple>>;
		return __broadern_tuple_hash_aux<Tuple, Index - 1>::hash(tup) 
			^ _MSTL hash<ElementType>()(_MSTL get<Index - 1>(tup));
	}
};
template <typename Tuple>
struct __broadern_tuple_hash_aux<Tuple, 1> {
	static constexpr size_t hash(const Tuple& tup) {
		using ElementType = remove_cvref_t<tuple_element_t<0, Tuple>>;
		return _MSTL hash<ElementType>()(_MSTL get<0>(tup));
	}
};
template <typename Tuple>
struct __broadern_tuple_hash_aux<Tuple, 0> {
	static constexpr size_t hash(const Tuple&) {
		return 0;
	}
};

MSTL_END_INNER__
#endif // !MSTL_VERSION_17__


template <typename This, typename ... Rest>
template <typename Tuple, size_t... Idx>
constexpr size_t tuple<This, Rest...>::__broaden_tuple(const Tuple& tup, index_sequence<Idx...>) noexcept {
#ifdef MSTL_VERSION_17__
	return (hash<remove_cvref_t<tuple_element_t<Idx, Tuple>>>()(_MSTL get<Idx>(tup)) ^ ...);
#else
	return _INNER __broadern_tuple_hash_aux<Tuple, sizeof...(Idx)>::hash(tup);
#endif // MSTL_VERSION_17__
}

MSTL_END_NAMESPACE__
#endif // MSTL_TUPLE_HPP__
