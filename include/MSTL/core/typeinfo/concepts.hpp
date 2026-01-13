#ifndef MSTL_CORE_TYPEINFO_CONCEPTS_HPP__
#define MSTL_CORE_TYPEINFO_CONCEPTS_HPP__

/**
 * @file concepts.hpp
 * @brief MSTL概念和类型约束
 *
 * 此文件提供了C++20概念的实现，以及用于类型约束和编译时检查的工具。
 */

#include "../iterator/iterator_traits.hpp"
#include "../typeinfo/pointer_traits.hpp"
#include "../functional/invoke.hpp"
MSTL_BEGIN_NAMESPACE__

#ifdef MSTL_STANDARD_20__

/**
 * @defgroup Concepts 概念约束
 * @brief 概念约束的实现
 * @{
 */

/**
 * @concept same_as
 * @brief 检查两个类型是否完全相同
 * @tparam T1 第一个类型
 * @tparam T2 第二个类型
 *
 * 要求T1和T2是相同的类型，包括cv限定符。
 */
template <typename T1, typename T2>
concept same_as = is_same_v<T1, T2> && is_same_v<T2, T1>;

/**
 * @concept common_reference_with
 * @brief 检查两个类型是否有公共引用类型
 * @tparam T1 第一个类型
 * @tparam T2 第二个类型
 *
 * 要求：
 * 1. T1和T2有公共引用类型
 * 2. 公共引用类型对称
 * 3. T1和T2都可以转换为公共引用类型
 */
template <typename T1, typename T2>
concept common_reference_with = requires {
	typename common_reference_t<T1, T2>;
	typename common_reference_t<T2, T1>;
}
&& same_as<common_reference_t<T1, T2>, common_reference_t<T2, T1>>
&& convertible_to<T1, common_reference_t<T1, T2>>
&& convertible_to<T2, common_reference_t<T1, T2>>;

/**
 * @concept common_with
 * @brief 检查两个类型是否有公共类型
 * @tparam T1 第一个类型
 * @tparam T2 第二个类型
 *
 * 要求：
 * 1. T1和T2有公共类型
 * 2. 公共类型对称
 * 3. T1和T2都可以转换为公共类型
 * 4. 满足公共引用相关约束
 */
template <typename T1, typename T2>
concept common_with = requires { typename common_type_t<T1, T2>; typename common_type_t<T2, T1>; }
&& same_as<common_type_t<T1, T2>, common_type_t<T2, T1>> && requires {
	static_cast<common_type_t<T1, T2>>(_MSTL declval<T1>());
	static_cast<common_type_t<T1, T2>>(_MSTL declval<T2>());
} && common_reference_with<add_lvalue_reference_t<const T1>, add_lvalue_reference_t<const T2>>
&& common_reference_with<add_lvalue_reference_t<common_type_t<T1, T2>>,
	common_reference_t<add_lvalue_reference_t<const T1>, add_lvalue_reference_t<const T2>>>;

/**
 * @concept derived_from
 * @brief 检查类型是否派生自另一个类型
 * @tparam Derived 派生类型
 * @tparam Base 基类型
 *
 * 要求Derived是Base的派生类，并且可以安全地进行指针转换。
 */
template <typename Derived, typename Base>
concept derived_from = is_base_of_v<Base, Derived> && convertible_to<const volatile Derived*, const volatile Base*>;


/**
 * @concept constructible_from
 * @brief 检查类型是否可以使用指定参数构造
 * @tparam T 要构造的类型
 * @tparam Args 构造参数类型
 */
template <typename T, typename... Args>
concept constructible_from = is_constructible_v<T, Args...>;

/**
 * @concept move_constructible
 * @brief 检查类型是否可移动构造
 * @tparam T 要检查的类型
 */
template <typename T>
concept move_constructible = is_move_constructible_v<T>;

/**
 * @concept copy_constructible
 * @brief 检查类型是否可复制构造
 * @tparam T 要检查的类型
 *
 * 要求：
 * 1. 可移动构造
 * 2. 可以从左值引用、const左值引用、const值进行构造和转换
 */
template <typename T>
concept copy_constructible = move_constructible<T>
&& constructible_from<T, T&>&& convertible_to<T&, T>
&& constructible_from<T, const T&>&& convertible_to<const T&, T>
&& constructible_from<T, const T>&& convertible_to<const T, T>;

/**
 * @concept default_initializable
 * @brief 检查类型是否可默认初始化
 * @tparam T 要检查的类型
 *
 * 要求可以通过{}语法和placement new进行默认初始化。
 */
template <typename T>
concept default_initializable = constructible_from<T> && requires {
	T{};
	::new (static_cast<void*>(nullptr)) T;
};

/**
 * @concept assignable_from
 * @brief 检查类型是否可以从另一个类型赋值
 * @tparam To 目标类型
 * @tparam From 源类型
 *
 * 要求To是左值引用，并且可以将From的值赋给To。
 */
template <typename To, typename From>
concept assignable_from = is_lvalue_reference_v<To>
&& common_reference_with<const remove_reference_t<To>&, const remove_reference_t<From>&>
&& requires(To x, From&& y) {
	{ x = static_cast<From&&>(y) } -> same_as<To>;
};


/**
 * @concept movable
 * @brief 检查类型是否可移动
 * @tparam T 要检查的类型
 *
 * 要求：
 * 1. 是对象类型
 * 2. 可移动构造
 * 3. 可自我赋值
 * 4. 可交换
 */
template <typename T>
concept movable = is_object_v<T>
&& move_constructible<T>
&& assignable_from<T&, T>
&& is_swappable_v<T>;

/**
 * @concept copyable
 * @brief 检查类型是否可复制
 * @tparam T 要检查的类型
 *
 * 在movable基础上增加复制构造和复制赋值的要求。
 */
template <typename T>
concept copyable = copy_constructible<T>
&& movable<T>
&& assignable_from<T&, T&>
&& assignable_from<T&, const T&>
&& assignable_from<T&, const T>;


/**
 * @concept one_way_equality_comparable
 * @brief 检查两个类型是否可以单向相等比较
 * @tparam T1 第一个类型
 * @tparam T2 第二个类型
 *
 * 要求T1可以与T2进行==和!=比较，结果可转换为bool。
 */
template <typename T1, typename T2>
concept one_way_equality_comparable =
	requires(const remove_reference_t<T1>& x, const remove_reference_t<T2>& y) {
		{ x == y } -> convertible_to<bool>;
		{ x != y } -> convertible_to<bool>;
};

/**
 * @concept both_equality_comparable
 * @brief 检查两个类型是否可以双向相等比较
 * @tparam T1 第一个类型
 * @tparam T2 第二个类型
 */
template <typename T1, typename T2>
concept both_equality_comparable =
one_way_equality_comparable<T1, T2>&& one_way_equality_comparable<T2, T1>;

/**
 * @concept equality_comparable
 * @brief 检查类型是否可与自身进行相等比较
 * @tparam T 要检查的类型
 */
template <typename T>
concept equality_comparable = one_way_equality_comparable<T, T>;

/**
 * @concept equality_comparable_with
 * @brief 检查两个类型是否可以互相进行相等比较
 * @tparam T1 第一个类型
 * @tparam T2 第二个类型
 */
template <typename T1, typename T2>
concept equality_comparable_with = equality_comparable<T1> && equality_comparable<T2>
&& common_reference_with<const remove_reference_t<T1>&, const remove_reference_t<T2>&>
&& equality_comparable<common_reference_t<const remove_reference_t<T1>&, const remove_reference_t<T2>&>>
&& both_equality_comparable<T1, T2>;


/**
 * @concept one_way_ordered
 * @brief 检查两个类型是否可以单向顺序比较
 * @tparam T1 第一个类型
 * @tparam T2 第二个类型
 */
template <typename T1, typename T2>
concept one_way_ordered = requires(const remove_reference_t<T1>& x, const remove_reference_t<T2>& y) {
	{ x < y } -> convertible_to<bool>;
	{ x > y } -> convertible_to<bool>;
	{ x <= y } -> convertible_to<bool>;
	{ x >= y } -> convertible_to<bool>;
};

/**
 * @concept both_ordered_with
 * @brief 检查两个类型是否可以双向顺序比较
 * @tparam T1 第一个类型
 * @tparam T2 第二个类型
 */
template <typename T1, typename T2>
concept both_ordered_with = one_way_ordered<T1, T2>&& one_way_ordered<T2, T1>;

/**
 * @concept totally_ordered
 * @brief 检查类型是否完全有序
 * @tparam T 要检查的类型
 *
 * 在相等比较基础上增加顺序比较要求。
 */
template <typename T>
concept totally_ordered = equality_comparable<T> && one_way_ordered<T, T>;

/**
 * @concept totally_ordered_with
 * @brief 检查两个类型是否可以互相完全有序比较
 * @tparam T1 第一个类型
 * @tparam T2 第二个类型
 */
template <typename T1, typename T2>
concept totally_ordered_with = totally_ordered<T1> && totally_ordered<T2>
&& equality_comparable_with<T1, T2>
&& totally_ordered<common_reference_t<const remove_reference_t<T1>&, const remove_reference_t<T2>&>>
&& both_ordered_with<T1, T2>;


/**
 * @concept semiregular
 * @brief 检查类型是否为半常规类型
 * @tparam T 要检查的类型
 *
 * 半常规类型要求可复制和可默认初始化。
 */
template <typename T>
concept semiregular = copyable<T> && default_initializable<T>;

/**
 * @concept regular
 * @brief 检查类型是否为常规类型
 * @tparam T 要检查的类型
 *
 * 常规类型在半常规类型基础上增加相等比较要求。
 */
template <typename T>
concept regular = semiregular<T> && equality_comparable<T>;


/**
 * @concept iterator_typedef
 * @brief 检查类型是否具有迭代器所需的所有类型定义
 * @tparam T 要检查的类型
 */
template <typename T>
concept iterator_typedef = requires() {
	typename iterator_traits<T>::iterator_category;
	typename iterator_traits<T>::value_type;
	typename iterator_traits<T>::difference_type;
	typename iterator_traits<T>::pointer;
	typename iterator_traits<T>::reference;
};

/**
 * @concept input_iterator
 * @brief 检查类型是否为输入迭代器
 * @tparam Iterator 迭代器类型
 *
 * 输入迭代器要求：
 * 1. 可进行相等比较
 * 2. 具有所有迭代器类型定义
 * 3. 支持解引用、前缀/后缀递增
 */
template <typename Iterator>
concept input_iterator = both_equality_comparable<Iterator, Iterator>
&& iterator_typedef<Iterator> && requires(Iterator it) {
	{ *it } -> convertible_to<typename iterator_traits<Iterator>::value_type>;
	{ ++it } -> same_as<Iterator&>;
	{ it++ } -> same_as<Iterator>;
};

/**
 * @concept forward_iterator
 * @brief 检查类型是否为前向迭代器
 * @tparam Iterator 迭代器类型
 *
 * 在前向迭代器基础上增加：
 * 1. 顺序比较
 * 2. 半常规类型
 * 3. 迭代器差值计算
 */
template <typename Iterator>
concept forward_iterator = both_ordered_with<Iterator, Iterator> && semiregular<Iterator>
&& input_iterator<Iterator> && requires(Iterator it1, Iterator it2) {
	{ it1 - it2 } -> convertible_to<typename iterator_traits<Iterator>::difference_type>;
};

/**
 * @concept bidirectional_iterator
 * @brief 检查类型是否为双向迭代器
 * @tparam Iterator 迭代器类型
 *
 * 在前向迭代器基础上增加递减操作支持。
 */
template <typename Iterator>
concept bidirectional_iterator = forward_iterator<Iterator> && requires(Iterator it) {
	{ --it } -> same_as<Iterator&>;
	{ it-- } -> same_as<Iterator>;
};

/**
 * @concept random_access_iterator
 * @brief 检查类型是否为随机访问迭代器
 * @tparam Iterator 迭代器类型
 *
 * 在双向迭代器基础上增加随机访问操作支持。
 */
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

/**
 * @concept contiguous_iterator
 * @brief 检查类型是否为连续迭代器
 * @tparam Iterator 迭代器类型
 *
 * 在随机访问迭代器基础上增加连续内存布局要求。
 */
template <typename Iterator>
concept contiguous_iterator = random_access_iterator<Iterator>
&& is_lvalue_reference_v<iter_reference_t<Iterator>>
&& same_as<iter_value_t<Iterator>, remove_cvref_t<iter_reference_t<Iterator>>>
&& requires(const Iterator& i) {
	{ _MSTL to_address(i) } -> same_as<add_pointer_t<iter_reference_t<Iterator>>>;
};


/**
 * @concept sentinel_for
 * @brief 检查类型是否为迭代器的哨兵
 * @tparam Sentinel 哨兵类型
 * @tparam Iterator 迭代器类型
 *
 * 哨兵用于标记迭代器范围的结束位置。
 */
template <typename Sentinel, typename Iterator>
concept sentinel_for =
    input_iterator<Iterator> &&
    semiregular<Sentinel> &&
    requires(const Iterator& i, const Sentinel& s) {
		{ i == s } -> convertible_to<bool>;
		{ i != s } -> convertible_to<bool>;
    };

/**
 * @concept sized_sentinel_for
 * @brief 检查哨兵是否支持大小计算
 * @tparam Sentinel 哨兵类型
 * @tparam Iterator 迭代器类型
 *
 * 支持通过哨兵计算迭代器距离的大小感知哨兵。
 */
template <typename Sentinel, typename Iterator>
concept sized_sentinel_for =
    input_iterator<Iterator> &&
    sentinel_for<Sentinel, Iterator> &&
    requires(const Iterator& i, const Sentinel& s) {
		{ s - i } -> same_as<iter_difference_t<Iterator>>;
    } &&
    requires(const Iterator& i, const Sentinel& s) {
		{ i + (s - i) } -> same_as<Iterator>;
    };


/**
 * @concept predicate
 * @brief 检查类型是否可以作为谓词
 * @tparam F 可调用对象类型
 * @tparam Args 参数类型
 *
 * 谓词要求可调用并且返回可转换为bool的类型。
 */
template <typename F, typename... Args>
concept predicate = is_invocable_v<F, Args...> && convertible_to<invoke_result_t<F, Args...>, bool>;

/** @} */ // Concepts

/**
 * @defgroup ViewChecks 视图检查
 * @brief 检查类型是否为视图的工具
 * @{
 */

MSTL_BEGIN_RANGES__

/**
 * @struct view_base
 * @brief 范围视图的基类模板
 * @tparam Derived 派生类类型
 *
 * 为范围视图提供统一的begin()和end()接口。
 * 派生类只需实现自己的begin()和end()方法。
 */
template <typename Derived>
struct view_base {
    /**
     * @brief 获取范围的起始const迭代器
     * @return 起始迭代器
     */
	constexpr auto begin() const {
		return static_cast<const Derived*>(this)->begin();
	}
    /**
     * @brief 获取范围的结束const迭代器
     * @return 结束迭代器
     */
	constexpr auto end() const {
		return static_cast<const Derived*>(this)->end();
	}
    /**
     * @brief 获取范围的起始迭代器
     * @return 起始迭代器
     */
	constexpr auto begin() {
		return static_cast<Derived*>(this)->begin();
	}
    /**
     * @brief 获取范围的结束迭代器
     * @return 结束迭代器
     */
	constexpr auto end() {
		return static_cast<Derived*>(this)->end();
	}
};

MSTL_END_RANGES__

/**
 * @struct is_view
 * @brief 检查类型是否为视图
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_view : false_type {};

/**
 * @brief view_base特化的视图检查
 * @tparam D 派生类类型
 */
template <typename D>
struct is_view<_MSTL_RANGES view_base<D>> : true_type {};

/**
 * @var is_view_v
 * @brief is_view的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_view_v = is_base_of_v<_MSTL_RANGES view_base<T>, T>;

/** @} */ // ViewChecks

#endif // MSTL_STANDARD_20__

/**
 * @defgroup IteratorCategoryChecks 迭代器类型检查
 * @brief 编译时迭代器类型检查工具
 * @{
 */

/// @cond
MSTL_BEGIN_INNER__
template <typename, typename = void>
MSTL_INLINE17 constexpr bool __is_iterator_with_cate_v = false;
template <typename Iterator>
MSTL_INLINE17 constexpr bool __is_iterator_with_cate_v<Iterator, void_t<iter_category_t<Iterator>>> = true;
MSTL_END_INNER__
/// @endcond

/**
 * @var is_ranges_iter_v
 * @brief 检查类型是否为范围迭代器
 * @tparam Iterator 要检查的迭代器类型
 *
 * 检查迭代器是否具有迭代器类别定义。
 */
template <typename Iterator>
MSTL_INLINE17 constexpr bool is_ranges_iter_v = _INNER __is_iterator_with_cate_v<Iterator>;

/**
 * @var is_iter_v
 * @brief 检查类型是否为迭代器
 * @tparam Iterator 要检查的迭代器类型
 */
template <typename Iterator>
MSTL_INLINE17 constexpr bool is_iter_v =
#ifdef MSTL_STANDARD_20__
iterator_typedef<Iterator> &&
#endif
is_ranges_iter_v<Iterator>;


/**
 * @var is_ranges_input_iter_v
 * @brief 检查是否为范围输入迭代器
 * @tparam Iterator 迭代器类型
 *
 * 通过检查迭代器类别是否可以转换为input_iterator_tag来判断。
 */
template <typename Iterator>
MSTL_INLINE17 constexpr bool is_ranges_input_iter_v = is_convertible_v<iter_category_t<Iterator>, input_iterator_tag>;

/**
 * @var is_input_iter_v
 * @brief 检查是否为输入迭代器
 * @tparam Iterator 迭代器类型
 */
template <typename Iterator>
MSTL_INLINE17 constexpr bool is_input_iter_v =
#ifdef MSTL_STANDARD_20__
input_iterator<Iterator> &&
#endif
is_ranges_input_iter_v<Iterator>;


/**
 * @var is_ranges_fwd_iter_v
 * @brief 检查是否为范围前向迭代器
 */
template <typename Iterator>
MSTL_INLINE17 constexpr bool is_ranges_fwd_iter_v = is_convertible_v<iter_category_t<Iterator>, forward_iterator_tag>;

/**
 * @var is_fwd_iter_v
 * @brief 检查是否为前向迭代器
 */
template <typename Iterator>
MSTL_INLINE17 constexpr bool is_fwd_iter_v =
#ifdef MSTL_STANDARD_20__
forward_iterator<Iterator> &&
#endif
is_ranges_fwd_iter_v<Iterator>;


/**
 * @var is_ranges_bid_iter_v
 * @brief 检查是否为范围双向迭代器
 */
template <typename Iterator>
MSTL_INLINE17 constexpr bool is_ranges_bid_iter_v = is_convertible_v<iter_category_t<Iterator>, bidirectional_iterator_tag>;

/**
 * @var is_bid_iter_v
 * @brief 检查是否为双向迭代器
 */
template <typename Iterator>
MSTL_INLINE17 constexpr bool is_bid_iter_v =
#ifdef MSTL_STANDARD_20__
bidirectional_iterator<Iterator> &&
#endif
is_ranges_bid_iter_v<Iterator>;


/**
 * @var is_ranges_rnd_iter_v
 * @brief 检查是否为范围随机访问迭代器
 */
template <typename Iterator>
MSTL_INLINE17 constexpr bool is_ranges_rnd_iter_v = is_convertible_v<iter_category_t<Iterator>, random_access_iterator_tag>;

/**
 * @var is_rnd_iter_v
 * @brief 检查是否为随机访问迭代器
 */
template <typename Iterator>
MSTL_INLINE17 constexpr bool is_rnd_iter_v =
#ifdef MSTL_STANDARD_20__
random_access_iterator<Iterator> &&
#endif
is_ranges_rnd_iter_v<Iterator>;


/**
 * @var is_ranges_cot_iter_v
 * @brief 检查是否为范围连续迭代器
 */
template <typename Iterator>
MSTL_INLINE17 constexpr bool is_ranges_cot_iter_v =
is_convertible_v<iter_category_t<Iterator>, contiguous_iterator_tag>;

/**
 * @var is_cot_iter_v
 * @brief 检查是否为连续迭代器
 */
template <typename Iterator>
MSTL_INLINE17 constexpr bool is_cot_iter_v =
#ifdef MSTL_STANDARD_20__
random_access_iterator<Iterator> && 
#endif // MSTL_STANDARD_20__
is_lvalue_reference_v<decltype(*_MSTL declval<Iterator&>())> && is_same_v<remove_cv_t<Iterator>, Iterator>
&& is_pod_v<iter_value_t<Iterator>> && is_ranges_cot_iter_v<Iterator>;

/** @} */ // IteratorChecks

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_TYPEINFO_CONCEPTS_HPP__
