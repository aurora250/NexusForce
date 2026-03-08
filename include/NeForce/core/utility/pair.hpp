#ifndef NEFORCE_CORE_UTILITY_PAIR_HPP__
#define NEFORCE_CORE_UTILITY_PAIR_HPP__

/**
 * @file pair.hpp
 * @brief 键值对
 *
 * 此文件提供了键值对及相关工具的实现
 */

#include "NeForce/core/interface/icommon.hpp"
#include "NeForce/core/utility/reference_wrapper.hpp"
#include "NeForce/core/utility/integer_sequence.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Tuple 元组
 * @brief 元组的主模板、特化实现和辅助函数
 * @{
 */

template <typename...>
struct tuple;

template <typename>
struct tuple_size;

#ifdef NEFORCE_STANDARD_14
/**
 * @var tuple_size_v
 * @brief tuple_size的类型别名
 * @tparam T tuple类型
 *
 * tuple_size的类型别名，便于获取tuple大小
 */
template <typename T>
constexpr size_t tuple_size_v = tuple_size<remove_cvref_t<T>>::value;
#endif


template <size_t Index, typename... Tuple>
struct tuple_element;

/**
 * @brief tuple_element的类型别名
 * @tparam Index 元素索引
 * @tparam Types tuple的元素类型
 *
 * tuple_element的类型别名，便于获取tuple元素类型
 */
template <size_t Index, typename... Types>
using tuple_element_t = typename tuple_element<Index, Types...>::type;

/**
 * @brief 获取tuple元素基类型的类型别名
 * @tparam Index 元素索引
 * @tparam Types tuple的元素类型
 */
template <size_t Index, typename... Types>
using tuple_extract_base_t = typename tuple_element<Index, Types...>::tuple_type;

template <size_t Index, typename... Types>
NEFORCE_NODISCARD constexpr tuple_element_t<Index, Types...>& get(tuple<Types...>& t) noexcept;
template <size_t Index, typename... Types>
NEFORCE_NODISCARD constexpr const tuple_element_t<Index, Types...>& get(const tuple<Types...>& t) noexcept;
template <size_t Index, typename... Types>
NEFORCE_NODISCARD constexpr tuple_element_t<Index, Types...>&& get(tuple<Types...>&& t) noexcept;
template <size_t Index, typename... Types>
NEFORCE_NODISCARD constexpr const tuple_element_t<Index, Types...>&& get(const tuple<Types...>&& t) noexcept;

/// @cond
NEFORCE_BEGIN_INNER__
template <size_t Index, typename... Types>
NEFORCE_NODISCARD constexpr tuple_element_t<Index, Types...>&&
__pair_get_from_tuple(tuple<Types...>&& t) noexcept;
NEFORCE_END_INNER__
/// @endcond

/** @} */ // Tuple

/**
 * @defgroup Pair 键值对
 * @brief 键值对及辅助函数实现
 * @{
 */

/**
 * @class pair
 * @brief 存储两个值的元组对
 * @tparam T1 第一个元素的类型
 * @tparam T2 第二个元素的类型
 *
 * pair是一个可以存储两个不同类型的值的模板类，常用于返回多个值。
 * 支持tuple-like接口，可用于结构化绑定。
 */
template <typename T1, typename T2>
struct pair : icommon<pair<T1, T2>> {
	using first_type	= T1;  ///< 第一个元素的类型
	using second_type	= T2;  ///< 第二个元素的类型

	T1 first;   ///< 第一个元素
	T2 second;  ///< 第二个元素

#ifdef NEFORCE_STANDARD_20
	/**
     * @brief 默认构造函数
     * @tparam U1 第一个元素的推导类型，默认为T1
     * @tparam U2 第二个元素的推导类型，默认为T2
     *
     * 当T1和T2都可默认构造时可用，根据是否可隐式构造决定explicit
     */
	template <typename U1 = T1, typename U2 = T2, enable_if_t<
		conjunction_v<is_default_constructible<U1>, is_default_constructible<U2>>, int> = 0>
	constexpr explicit(!conjunction_v<
		is_implicitly_default_constructible<U1>, is_implicitly_default_constructible<U2>>)
		pair() noexcept(conjunction_v<
			is_nothrow_default_constructible<U1>, is_nothrow_default_constructible<U2>>)
		: first(), second() {}

	/**
	 * @brief 拷贝构造函数
	 * @tparam U1 第一个元素的推导类型，默认为T1
	 * @tparam U2 第二个元素的推导类型，默认为T2
	 * @param a 第一个元素的值
	 * @param b 第二个元素的值
	 *
	 * 从两个同类型值构造pair
	 */
	template <typename U1 = T1, typename U2 = T2, enable_if_t<
		conjunction_v<is_copy_constructible<U1>, is_copy_constructible<U2>>, int> = 0>
	constexpr explicit(!conjunction_v<is_convertible<const U1&, U1>, is_convertible<const U2&, U2>>)
		pair(const T1& a, const T2& b) noexcept(conjunction_v<
			is_nothrow_copy_constructible<U1>, is_nothrow_copy_constructible<U2>>)
		: first(a), second(b) {}

	/**
	 * @brief 通用值构造函数
	 * @tparam U1 第一个元素的类型
	 * @tparam U2 第二个元素的类型
	 * @param a 第一个元素的值
	 * @param b 第二个元素的值
	 *
	 * 从两个任意类型的值构造pair
	 */
	template <typename U1, typename U2, enable_if_t<
		conjunction_v<is_constructible<T1, U1>, is_constructible<T2, U2>>, int> = 0>
	constexpr explicit(!conjunction_v<is_convertible<U1, T1>, is_convertible<U2, T2>>)
		pair(U1&& a, U2&& b) noexcept(conjunction_v<
			is_nothrow_constructible<T1, U1>, is_nothrow_constructible<T2, U2>>)
		: first(_NEFORCE forward<U1>(a)), second(_NEFORCE forward<U2>(b)) {}

	/**
	 * @brief 拷贝pair构造函数
	 * @tparam U1 源pair第一个元素的类型
	 * @tparam U2 源pair第二个元素的类型
	 * @param p 源pair对象
	 *
	 * 从另一个pair构造，进行元素拷贝
	 */
	template <typename U1, typename U2, enable_if_t<
		conjunction_v<is_constructible<T1, const U1&>, is_constructible<T2, const U2&>>, int> = 0>
	constexpr explicit(!conjunction_v<is_convertible<const U1&, T1>, is_convertible<const U2&, T2>>)
		pair(const pair<U1, U2>& p) noexcept(conjunction_v<
			is_nothrow_constructible<T1, const U1&>, is_nothrow_constructible<T2, const U2&>>)
		: first(p.first), second(p.second) {}

	/**
	 * @brief 移动pair构造函数
	 * @tparam U1 源pair第一个元素的类型
	 * @tparam U2 源pair第二个元素的类型
	 * @param p 源pair对象
	 *
	 * 从另一个pair构造，进行元素移动
	 */
	template <typename U1, typename U2, enable_if_t<
		conjunction_v<is_constructible<T1, U1>, is_constructible<T2, U2>>, int> = 0>
	constexpr explicit(!conjunction_v<
		is_convertible<U1, T1>, is_convertible<U2, T2>>)
		pair(pair<U1, U2>&& p) noexcept(conjunction_v<
			is_nothrow_constructible<T1, U1>, is_nothrow_constructible<T2, U2>>)
		: first(_NEFORCE forward<U1>(p.first)), second(_NEFORCE forward<U2>(p.second)) {}
#else
	/**
     * @brief 显式默认构造函数
     */
	template <typename U1 = T1, typename U2 = T2, enable_if_t<
		conjunction<is_default_constructible<U1>, is_default_constructible<U2>>::value &&
		!conjunction<is_implicitly_default_constructible<U1>,
		is_implicitly_default_constructible<U2>>::value, int> = 0>
	explicit pair() noexcept(conjunction<
		is_nothrow_default_constructible<U1>, is_nothrow_default_constructible<U2>>::value)
		: first(), second() {}
	/**
	 * @brief 隐式默认构造函数
	 */
	template <typename U1 = T1, typename U2 = T2, enable_if_t<
		conjunction<is_default_constructible<U1>, is_default_constructible<U2>>::value &&
		conjunction<is_implicitly_default_constructible<U1>,
		is_implicitly_default_constructible<U2>>::value, int> = 0>
	pair() noexcept(conjunction<
		is_nothrow_default_constructible<U1>, is_nothrow_default_constructible<U2>>::value)
		: first(), second() {}

	/**
	 * @brief 显式拷贝构造函数
	 */
	template <typename U1 = T1, typename U2 = T2, enable_if_t<
		conjunction<is_copy_constructible<U1>, is_copy_constructible<U2>>::value &&
		!conjunction<is_convertible<const U1&, U1>, is_convertible<const U2&, U2>>::value, int> = 0>
	explicit pair(const T1& a, const T2& b) noexcept(conjunction<
		is_nothrow_copy_constructible<U1>, is_nothrow_copy_constructible<U2>>::value)
		: first(a), second(b) {}

	/**
	 * @brief 隐式拷贝构造函数
	 */
	template <typename U1 = T1, typename U2 = T2, enable_if_t<
		conjunction<is_copy_constructible<U1>, is_copy_constructible<U2>>::value &&
		conjunction<is_convertible<const U1&, U1>, is_convertible<const U2&, U2>>::value, int> = 0>
	pair(const T1& a, const T2& b) noexcept(conjunction<
		is_nothrow_copy_constructible<U1>, is_nothrow_copy_constructible<U2>>::value)
		: first(a), second(b) {}

	/**
	 * @brief 显式通用值构造函数
	 */
	template <typename U1, typename U2, enable_if_t<
		conjunction<is_constructible<T1, U1>, is_constructible<T2, U2>>::value &&
		!conjunction<is_convertible<U1, T1>, is_convertible<U2, T2>>::value, int> = 0>
	explicit pair(U1&& a, U2&& b) noexcept(conjunction<
		is_nothrow_constructible<T1, U1>, is_nothrow_constructible<T2, U2>>::value)
		: first(_NEFORCE forward<U1>(a)), second(_NEFORCE forward<U2>(b)) {}

	/**
	 * @brief 隐式通用值构造函数
	 */
	template <typename U1, typename U2, enable_if_t<
		conjunction<is_constructible<T1, U1>, is_constructible<T2, U2>>::value &&
		conjunction<is_convertible<U1, T1>, is_convertible<U2, T2>>::value, int> = 0>
	pair(U1&& a, U2&& b) noexcept(conjunction<
		is_nothrow_constructible<T1, U1>, is_nothrow_constructible<T2, U2>>::value)
		: first(_NEFORCE forward<U1>(a)), second(_NEFORCE forward<U2>(b)) {}

	/**
	 * @brief 显式拷贝pair构造函数
	 */
	template <typename U1, typename U2, enable_if_t<
		conjunction<is_constructible<T1, const U1&>, is_constructible<T2, const U2&>>::value &&
		!conjunction<is_convertible<const U1&, T1>, is_convertible<const U2&, T2>>::value, int> = 0>
	explicit pair(const pair<U1, U2>& p) noexcept(conjunction<
		is_nothrow_constructible<T1, const U1&>, is_nothrow_constructible<T2, const U2&>>::value)
		: first(p.first), second(p.second) {}

	/**
	 * @brief 隐式拷贝pair构造函数
	 */
	template <typename U1, typename U2, enable_if_t<
		conjunction<is_constructible<T1, const U1&>, is_constructible<T2, const U2&>>::value &&
		conjunction<is_convertible<const U1&, T1>, is_convertible<const U2&, T2>>::value, int> = 0>
	pair(const pair<U1, U2>& p) noexcept(conjunction<
		is_nothrow_constructible<T1, const U1&>, is_nothrow_constructible<T2, const U2&>>::value)
		: first(p.first), second(p.second) {}

	/**
	 * @brief 显式移动pair构造函数
	 */
	template <typename U1, typename U2, enable_if_t<
		conjunction<is_constructible<T1, U1>, is_constructible<T2, U2>>::value &&
		!conjunction<is_convertible<U1, T1>, is_convertible<U2, T2>>::value, int> = 0>
	explicit pair(pair<U1, U2>&& p) noexcept(conjunction<
		is_nothrow_constructible<T1, U1>, is_nothrow_constructible<T2, U2>>::value)
		: first(_NEFORCE forward<U1>(p.first)), second(_NEFORCE forward<U2>(p.second)) {}

	/**
	 * @brief 隐式移动pair构造函数
	 */
	template <typename U1, typename U2, enable_if_t<
		conjunction<is_constructible<T1, U1>, is_constructible<T2, U2>>::value &&
		conjunction<is_convertible<U1, T1>, is_convertible<U2, T2>>::value, int> = 0>
	pair(pair<U1, U2>&& p) noexcept(conjunction<
		is_nothrow_constructible<T1, U1>, is_nothrow_constructible<T2, U2>>::value)
		: first(_NEFORCE forward<U1>(p.first)), second(_NEFORCE forward<U2>(p.second)) {}
#endif

	pair(const pair& p) = default;  ///< 默认拷贝构造函数
	pair(pair&& p) = default;       ///< 默认移动构造函数

	/**
	 * @brief 从两个tuple构造pair
	 * @tparam Tuple1 第一个tuple的类型
	 * @tparam Tuple2 第二个tuple的类型
	 * @tparam Index1 第一个tuple的索引序列
	 * @tparam Index2 第二个tuple的索引序列
	 * @param t1 第一个tuple
	 * @param t2 第二个tuple
	 * @param idx1 第一个tuple的索引序列
	 * @param idx2 第二个tuple的索引序列
	 *
	 * 内部构造函数，用于从tuple构造pair
	 */
	template <typename Tuple1, typename Tuple2, size_t... Index1, size_t... Index2>
	constexpr pair(Tuple1& t1, Tuple2& t2, index_sequence<Index1...> idx1, index_sequence<Index2...> idx2)
		: first(_INNER __pair_get_from_tuple<Index1>(_NEFORCE move(t1))...),
		second(_INNER __pair_get_from_tuple<Index2>(_NEFORCE move(t2))...) {}

	/**
	 * @brief 从两个tuple构造pair
	 * @tparam Types1 第一个tuple的元素类型
	 * @tparam Types2 第二个tuple的元素类型
	 * @param t1 第一个tuple
	 * @param t2 第二个tuple
	 *
	 * 标签分发构造函数，用于从tuple构造pair
	 */
	template <typename... Types1, typename... Types2>
	constexpr pair(unpack_utility_construct_tag, tuple<Types1...> t1, tuple<Types2...> t2)
		: pair(t1, t2, index_sequence_for<Types1...>{}, index_sequence_for<Types2...>{}) {}

	/**
	 * @brief 拷贝赋值运算符
	 * @tparam T pair类型，用于SFINAE
	 * @param p 源pair
	 * @return 当前pair的引用
	 */
	template <typename T = pair, enable_if_t<conjunction<
		is_copy_assignable<typename T::first_type>, is_copy_assignable<typename T::second_type>>::value, int> = 0>
	NEFORCE_CONSTEXPR14 pair& operator =(type_identity_t<const T&> p) noexcept(conjunction<
		is_nothrow_copy_assignable<T1>, is_nothrow_copy_assignable<T2>>::value) {
		first = p.first;
		second = p.second;
		return *this;
	}

	/**
	 * @brief 移动赋值运算符
	 * @tparam T pair类型，用于SFINAE
	 * @param p 源pair
	 * @return 当前pair的引用
	 */
	template <typename T = pair, enable_if_t<conjunction<
		is_move_assignable<typename T::first_type>, is_move_assignable<typename T::second_type>>::value, int> = 0>
	NEFORCE_CONSTEXPR14 pair& operator =(type_identity_t<T&&> p) noexcept(conjunction<
		is_nothrow_move_assignable<T1>, is_nothrow_move_assignable<T2>>::value) {
		first = _NEFORCE forward<T1>(p.first);
		second = _NEFORCE forward<T2>(p.second);
		return *this;
	}

	/**
	 * @brief 从不同类型pair的拷贝赋值运算符
	 * @tparam U1 源pair第一个元素的类型
	 * @tparam U2 源pair第二个元素的类型
	 * @param p 源pair
	 * @return 当前pair的引用
	 */
	template <typename U1, typename U2, enable_if_t<conjunction<negation<
		is_same<pair, pair<U1, U2>>>, is_assignable<T1&, const U1&>, is_assignable<T2&, const U2&>>::value, int> = 0>
	NEFORCE_CONSTEXPR14 pair& operator =(const pair<U1, U2>& p) noexcept(conjunction<
		is_nothrow_assignable<T1&, const U1&>, is_nothrow_assignable<T2&, const U2&>>::value) {
		first = p.first;
		second = p.second;
		return *this;
	}

	/**
	 * @brief 从不同类型pair的移动赋值运算符
	 * @tparam U1 源pair第一个元素的类型
	 * @tparam U2 源pair第二个元素的类型
	 * @param p 源pair
	 * @return 当前pair的引用
	 */
	template <typename U1, typename U2, enable_if_t<conjunction<negation<
		is_same<pair, pair<U1, U2>>>, is_assignable<T1&, U1>, is_assignable<T2&, U2>>::value, int> = 0>
	NEFORCE_CONSTEXPR14 pair& operator =(pair<U1, U2>&& p) noexcept(conjunction<
		is_nothrow_assignable<T1&, U1>, is_nothrow_assignable<T2&, U2>>::value) {
		first = _NEFORCE forward<U1>(p.first);
		second = _NEFORCE forward<U2>(p.second);
		return *this;
	}

	pair& operator =(const volatile pair&) = delete;  ///< 禁止volatile拷贝赋值

	NEFORCE_CONSTEXPR20 ~pair() = default;  ///< 析构函数

	/**
	 * @brief 相等比较运算符
	 * @param y 要比较的pair
	 * @return 比较结果
	 *
	 * 如果两个pair的所有元素都相等则返回true
	 */
	constexpr bool operator ==(const pair& y) const
	noexcept(noexcept(this->first == y.first && this->second == y.second)) {
		return this->first == y.first && this->second == y.second;
	}

	/**
	 * @brief 小于比较运算符
	 * @param y 要比较的pair
	 * @return 比较结果
	 *
	 * 先比较first，如果相等则比较second
	 */
	constexpr bool operator <(const pair& y) const
	noexcept(noexcept(this->first < y.first || (!(y.first < this->first) && this->second < y.second))) {
		return this->first < y.first || (!(y.first < this->first) && this->second < y.second);
	}

	/**
	 * @brief 计算hash值
	 * @return pair的hash值
	 *
	 * 使用两个元素的hash值进行异或操作
	 */
	NEFORCE_NODISCARD constexpr size_t to_hash() const
	noexcept(noexcept(hash<remove_cvref_t<T1>>()(first) ^ hash<remove_cvref_t<T2>>()(second))) {
		return hash<remove_cvref_t<T1>>()(first) ^ hash<remove_cvref_t<T2>>()(second);
	}

	/**
	 * @brief 交换两个pair的内容
	 * @param p 要交换的pair
	 */
	NEFORCE_CONSTEXPR14 void swap(pair& p)
	noexcept(conjunction<is_nothrow_swappable<T1>, is_nothrow_swappable<T2>>::value) {
		_NEFORCE swap(first, p.first);
		_NEFORCE swap(second, p.second);
	}
};

#ifdef NEFORCE_STANDARD_17
template <typename T1, typename T2>
pair(T1, T2) -> pair<T1, T2>;
#endif


/**
 * @brief 创建pair的辅助函数
 * @tparam T1 第一个元素的类型
 * @tparam T2 第二个元素的类型
 * @param x 第一个元素的值
 * @param y 第二个元素的值
 * @return 构造的pair对象
 *
 * 会自动解包reference_wrapper，支持完美转发
 */
template <typename T1, typename T2>
constexpr pair<unwrap_ref_decay_t<T1>, unwrap_ref_decay_t<T2>> make_pair(T1&& x, T2&& y)
noexcept(conjunction<is_nothrow_constructible<unwrap_ref_decay_t<T1>, T1>,
	is_nothrow_constructible<unwrap_ref_decay_t<T2>, T2>>::value) {
	using unwrap_pair = pair<unwrap_ref_decay_t<T1>, unwrap_ref_decay_t<T2>>;
	return unwrap_pair(_NEFORCE forward<T1>(x), _NEFORCE forward<T2>(y));
}

/** @} */ // Pair

/**
 * @defgroup Tuple 元组
 * @brief 元组的主模板、特化实现和辅助函数
 * @{
 */

/**
 * @struct tuple_size
 * @brief 获取tuple大小的特化
 * @tparam Types tuple的元素类型
 */
template <typename... Types>
struct tuple_size<tuple<Types...>> : integral_constant<size_t, sizeof...(Types)> {};

/**
 * @struct tuple_element
 * @brief 获取tuple元素类型的特化
 * @tparam Index 元素索引
 */
template <size_t Index>
struct tuple_element<Index, tuple<>> {};

/**
 * @brief 获取tuple第一个元素类型的特化
 * @tparam This 第一个元素的类型
 * @tparam Rest 剩余元素的类型
 */
template <typename This, typename... Rest>
struct tuple_element<0, tuple<This, Rest...>> {
	using type = This;
	using tuple_type = tuple<This, Rest...>;
};

/**
 * @brief 递归获取tuple元素类型的特化
 * @tparam Index 元素索引
 * @tparam This 第一个元素的类型
 * @tparam Rest 剩余元素的类型
 */
template <size_t Index, typename This, typename... Rest>
struct tuple_element<Index, tuple<This, Rest...>>
	: tuple_element<Index - 1, tuple<Rest...>> {};

/**
 * @struct tuple_element
 * @brief tuple_element的通用版本
 * @tparam Index 元素索引
 * @tparam Types tuple的元素类型
 */
template <size_t Index, typename... Types>
struct tuple_element : tuple_element<Index, tuple<Types...>> {};


/**
 * @brief pair的tuple_size特化，固定为2
 * @tparam T1 第一个元素的类型
 * @tparam T2 第二个元素的类型
 */
template <typename T1, typename T2>
struct tuple_size<pair<T1, T2>> : integral_constant<size_t, 2> {};

/**
 * @brief pair的tuple_element特化
 * @tparam Index 元素索引，0或1
 * @tparam T1 第一个元素的类型
 * @tparam T2 第二个元素的类型
 */
template <size_t Index, typename T1, typename T2>
struct tuple_element<Index, pair<T1, T2>> {
	static_assert(Index < 2, "pair element index out of range.");

	using type = conditional_t<Index == 0, T1, T2>;  ///< 根据索引返回对应类型
	using tuple_type = tuple<T1, T2>;                ///< 对应的tuple类型
};

/** @} */ // Tuple

/**
 * @defgroup Pair 键值对
 * @brief 键值对及辅助函数实现
 * @{
 */

#ifndef NEFORCE_STANDARD_17
/// @cond
NEFORCE_BEGIN_INNER__

template <size_t Index, typename T1, typename T2>
struct __pair_get_helper;
template <typename T1, typename T2>
struct __pair_get_helper<0, T1, T2> {
	NEFORCE_NODISCARD constexpr static tuple_element_t<0, pair<T1, T2>>&
		get(pair<T1, T2>& pir) noexcept {
		return pir.first;
	}
	NEFORCE_NODISCARD constexpr static const tuple_element_t<0, pair<T1, T2>>&
		get(const pair<T1, T2>& pir) noexcept {
		return pir.first;
	}
	NEFORCE_NODISCARD constexpr static tuple_element_t<0, pair<T1, T2>>&&
		get(pair<T1, T2>&& pir) noexcept {
		return _NEFORCE forward<T1>(pir.first);
	}
	NEFORCE_NODISCARD constexpr static const tuple_element_t<0, pair<T1, T2>>&&
		get(const pair<T1, T2>&& pir) noexcept {
		return _NEFORCE forward<const T1>(pir.first);
	}
};

template <typename T1, typename T2>
struct __pair_get_helper<1, T1, T2> {
	NEFORCE_NODISCARD constexpr static tuple_element_t<1, pair<T1, T2>>&
		get(pair<T1, T2>& pir) noexcept {
		return pir.second;
	}
	NEFORCE_NODISCARD constexpr static const tuple_element_t<1, pair<T1, T2>>&
		get(const pair<T1, T2>& pir) noexcept {
		return pir.second;
	}
	NEFORCE_NODISCARD constexpr static tuple_element_t<1, pair<T1, T2>>&&
		get(pair<T1, T2>&& pir) noexcept {
		return _NEFORCE forward<T2>(pir.second);
	}
	NEFORCE_NODISCARD constexpr static const tuple_element_t<1, pair<T1, T2>>&&
		get(const pair<T1, T2>&& pir) noexcept {
		return _NEFORCE forward<const T2>(pir.second);
	}
};

NEFORCE_END_INNER__
/// @endcond
#endif // !NEFORCE_STANDARD_17

/**
 * @brief 按索引获取pair元素的左值引用
 */
template <size_t Index, typename T1, typename T2>
NEFORCE_NODISCARD constexpr tuple_element_t<Index, pair<T1, T2>>&
get(pair<T1, T2>& pir) noexcept;

#ifdef NEFORCE_STANDARD_17
template <size_t Index, typename T1, typename T2>
NEFORCE_NODISCARD constexpr tuple_element_t<Index, pair<T1, T2>>&
get(pair<T1, T2>& pir) noexcept {
	if constexpr (Index == 0) {
		return pir.first;
	} else {
		return pir.second;
	}
}
#else
template <size_t Index, typename T1, typename T2>
NEFORCE_NODISCARD constexpr tuple_element_t<Index, pair<T1, T2>>&
get(pair<T1, T2>& pir) noexcept {
	return _INNER __pair_get_helper<Index, T1, T2>::get(pir);
}
#endif // NEFORCE_STANDARD_17

/**
 * @brief 按类型获取pair第一个元素的左值引用
 */
template <typename T1, typename T2>
NEFORCE_NODISCARD constexpr T1& get(pair<T1, T2>& pir) noexcept {
	return pir.first;
}

/**
 * @brief 按类型获取pair第二个元素的左值引用
 */
template <typename T2, typename T1>
NEFORCE_NODISCARD constexpr T2& get(pair<T1, T2>& pir) noexcept {
	return pir.second;
}


/**
 * @brief 按索引获取pair元素的const左值引用
 */
template <size_t Index, typename T1, typename T2>
NEFORCE_NODISCARD constexpr const tuple_element_t<Index, pair<T1, T2>>&
get(const pair<T1, T2>& pir) noexcept;

#ifdef NEFORCE_STANDARD_17
template <size_t Index, typename T1, typename T2>
NEFORCE_NODISCARD constexpr const tuple_element_t<Index, pair<T1, T2>>&
get(const pair<T1, T2>& pir) noexcept {
	if constexpr (Index == 0)
		return pir.first;
	else
		return pir.second;
}
#else
template <size_t Index, typename T1, typename T2>
NEFORCE_NODISCARD constexpr const tuple_element_t<Index, pair<T1, T2>>&
get(const pair<T1, T2>& pir) noexcept {
	return _INNER __pair_get_helper<Index, T1, T2>::get(pir);
}
#endif // NEFORCE_STANDARD_17

/**
 * @brief 按类型获取pair第一个元素的const左值引用
 */
template <typename T1, typename T2>
NEFORCE_NODISCARD constexpr const T1& get(const pair<T1, T2>& pir) noexcept {
	return pir.first;
}

/**
 * @brief 按类型获取pair第二个元素的const左值引用
 */
template <typename T2, typename T1>
NEFORCE_NODISCARD constexpr const T2& get(const pair<T1, T2>& pir) noexcept {
	return pir.second;
}


/**
 * @brief 按索引获取pair元素的右值引用
 */
template <size_t Index, typename T1, typename T2>
NEFORCE_NODISCARD constexpr tuple_element_t<Index, pair<T1, T2>>&&
get(pair<T1, T2>&& pir) noexcept;

#ifdef NEFORCE_STANDARD_17
template <size_t Index, typename T1, typename T2>
NEFORCE_NODISCARD constexpr tuple_element_t<Index, pair<T1, T2>>&&
get(pair<T1, T2>&& pir) noexcept {
	if constexpr (Index == 0)
		return _NEFORCE forward<T1>(pir.first);
	else
		return _NEFORCE forward<T2>(pir.second);
}
#else
template <size_t Index, typename T1, typename T2>
NEFORCE_NODISCARD constexpr tuple_element_t<Index, pair<T1, T2>>&&
get(pair<T1, T2>&& pir) noexcept {
	return _NEFORCE forward<tuple_element_t<Index, pair<T1, T2>>>(
		_INNER __pair_get_helper<Index, T1, T2>::get(_NEFORCE forward<pair<T1, T2>>(pir)));
}
#endif // NEFORCE_STANDARD_17

/**
 * @brief 按类型获取pair第一个元素的右值引用
 */
template <typename T1, typename T2>
NEFORCE_NODISCARD constexpr T1&& get(pair<T1, T2>&& pir) noexcept {
	return _NEFORCE forward<T1>(pir.first);
}

/**
 * @brief 按类型获取pair第二个元素的右值引用
 */
template <typename T2, typename T1>
NEFORCE_NODISCARD constexpr T2&& get(pair<T1, T2>&& pir) noexcept {
	return _NEFORCE forward<T2>(pir.second);
}


/**
 * @brief 按索引获取pair元素的const右值引用
 */
template <size_t Index, typename T1, typename T2>
NEFORCE_NODISCARD constexpr const tuple_element_t<Index, pair<T1, T2>>&&
get(const pair<T1, T2>&& pir) noexcept;

#ifdef NEFORCE_STANDARD_17
template <size_t Index, typename T1, typename T2>
NEFORCE_NODISCARD constexpr const tuple_element_t<Index, pair<T1, T2>>&&
get(const pair<T1, T2>&& pir) noexcept {
	if constexpr (Index == 0)
		return _NEFORCE forward<const T1>(pir.first);
	else
		return _NEFORCE forward<const T2>(pir.second);
}
#else
template <size_t Index, typename T1, typename T2>
NEFORCE_NODISCARD constexpr const tuple_element_t<Index, pair<T1, T2>>&&
get(const pair<T1, T2>&& pir) noexcept {
	return _NEFORCE forward<const tuple_element_t<Index, pair<T1, T2>>>(
		_INNER __pair_get_helper<Index, T1, T2>::get(_NEFORCE forward<const pair<T1, T2>>(pir)));
}
#endif // NEFORCE_STANDARD_17

/**
 * @brief 按类型获取pair第一个元素的const右值引用
 */
template <typename T1, typename T2>
NEFORCE_NODISCARD constexpr const T1&& get(const pair<T1, T2>&& pir) noexcept {
	return _NEFORCE forward<const T1>(pir.first);
}

/**
 * @brief 按类型获取pair第二个元素的const右值引用
 */
template <typename T2, typename T1>
NEFORCE_NODISCARD constexpr const T2&& get(const pair<T1, T2>&& pir) noexcept {
	return _NEFORCE forward<const T2>(pir.second);
}

/** @} */ // Pair

NEFORCE_END_NAMESPACE__

/// @cond
/// 为支持结构化解绑添加的std特化
namespace std {
	template <typename T>
	struct tuple_size;

	template <_NEFORCE size_t I, typename T>
	struct tuple_element;

	template <typename T1, typename T2>
	struct tuple_size<_NEFORCE pair<T1, T2>> : _NEFORCE integral_constant<_NEFORCE size_t, 2> {};

	template <_NEFORCE size_t I, typename T1, typename T2>
	struct tuple_element<I, _NEFORCE pair<T1, T2>> {
		using type = _NEFORCE tuple_element_t<I, _NEFORCE pair<T1, T2>>;
	};
}
/// @endcond

#endif // NEFORCE_CORE_UTILITY_PAIR_HPP__
