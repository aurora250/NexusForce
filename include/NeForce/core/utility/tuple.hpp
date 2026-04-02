#ifndef NEFORCE_CORE_UTILITY_TUPLE_HPP__
#define NEFORCE_CORE_UTILITY_TUPLE_HPP__

/**
 * @file tuple.hpp
 * @brief 元组实现
 *
 * 此文件提供了元组及其辅助函数的实现。
 */

#include "NeForce/core/utility/pair.hpp"
NEFORCE_BEGIN_NAMESPACE__

/// @cond
NEFORCE_BEGIN_INNER__

template <bool Same, typename Dest, typename... Srcs> struct __tuple_constructible_aux : false_type {};

template <typename... Dests, typename... Srcs>
struct __tuple_constructible_aux<true, tuple<Dests...>, Srcs...>
: bool_constant<conjunction<is_constructible<Dests, Srcs>...>::value> {};

/**
 * @struct tuple_constructible
 * @brief 检查元组是否可以从给定参数构造
 * @tparam Dest 目标元组类型
 * @tparam Srcs 源参数类型
 */
template <typename Dest, typename... Srcs>
struct tuple_constructible
: bool_constant<inner::__tuple_constructible_aux<tuple_size<Dest>::value == sizeof...(Srcs), Dest, Srcs...>::value> {};


template <bool Same, typename Dest, typename... Srcs> struct __tuple_explicitly_convertible_aux : false_type {};

template <typename... Dests, typename... Srcs>
struct __tuple_explicitly_convertible_aux<true, tuple<Dests...>, Srcs...>
: bool_constant<!conjunction<is_convertible<Srcs, Dests>...>::value> {};

/**
 * @struct tuple_explicitly_convertible
 * @brief 检查元组是否需要进行显式转换
 * @tparam Dest 目标元组类型
 * @tparam Srcs 源参数类型
 */
template <typename Dest, typename... Srcs>
struct tuple_explicitly_convertible
: bool_constant<
          inner::__tuple_explicitly_convertible_aux<tuple_size<Dest>::value == sizeof...(Srcs), Dest, Srcs...>::value> {
};


/**
 * @struct tuple_perfect_forward
 * @brief 检查是否可以进行完美转发构造
 * @tparam Tuple1 目标元组类型
 * @tparam Tuple2 源元组类型
 * @tparam U 源参数类型
 */
template <typename Tuple1, typename Tuple2, typename... U> struct tuple_perfect_forward : true_type {};

template <typename Tuple1, typename Tuple2>
struct tuple_perfect_forward<Tuple1, Tuple2> : bool_constant<!is_same<Tuple1, remove_cvref_t<Tuple2>>::value> {};

template <typename T1, typename T2, typename U1, typename U2>
struct tuple_perfect_forward<tuple<T1, T2>, U1, U2>
: bool_constant<disjunction<negation<is_same<remove_cvref_t<U1>, allocator_arg_tag>>,
                            is_same<remove_cvref_t<T1>, allocator_arg_tag>>::value> {};

template <typename T1, typename T2, typename T3, typename U1, typename U2, typename U3>
struct tuple_perfect_forward<tuple<T1, T2, T3>, U1, U2, U3>
: bool_constant<disjunction<negation<is_same<remove_cvref_t<U1>, allocator_arg_tag>>,
                            is_same<remove_cvref_t<T1>, allocator_arg_tag>>::value> {};


template <bool Same, typename Dest, typename... Srcs> struct __tuple_nothrow_constructible_aux : false_type {};

template <typename... Dests, typename... Srcs>
struct __tuple_nothrow_constructible_aux<true, tuple<Dests...>, Srcs...>
: bool_constant<conjunction<is_nothrow_constructible<Dests, Srcs>...>::value> {};

/**
 * @struct tuple_nothrow_constructible
 * @brief 检查元组是否可以无异常构造
 * @tparam Dest 目标元组类型
 * @tparam Srcs 源参数类型
 */
template <typename Dest, typename... Srcs>
struct tuple_nothrow_constructible
: bool_constant<
          inner::__tuple_nothrow_constructible_aux<tuple_size<Dest>::value == sizeof...(Srcs), Dest, Srcs...>::value> {
};


template <typename Self, typename Tuple, typename... U> struct __tuple_convertible_aux : true_type {};

template <typename Self, typename Tuple, typename U>
struct __tuple_convertible_aux<tuple<Self>, Tuple, U>
: bool_constant<!disjunction<is_same<Self, U>, is_constructible<Self, Tuple>, is_convertible<Tuple, Self>>::value> {};

/**
 * @struct tuple_convertible
 * @brief 检查元组是否可以转换
 * @tparam Self 自身元组类型
 * @tparam Tuple 源元组类型
 * @tparam U 源参数类型
 */
template <typename Self, typename Tuple, typename... U>
struct tuple_convertible : bool_constant<inner::__tuple_convertible_aux<Self, Tuple, U...>::value> {};


template <bool Same, typename Dest, typename... Srcs> struct __tuple_assignable_aux : false_type {};

template <typename... Dests, typename... Srcs>
struct __tuple_assignable_aux<true, tuple<Dests...>, Srcs...>
: bool_constant<conjunction<is_assignable<Dests&, Srcs>...>::value> {};

/**
 * @struct tuple_assignable
 * @brief 检查元组是否可以赋值
 * @tparam Dest 目标元组类型
 * @tparam Srcs 源参数类型
 */
template <typename Dest, typename... Srcs>
struct tuple_assignable
: bool_constant<inner::__tuple_assignable_aux<tuple_size<Dest>::value == sizeof...(Srcs), Dest, Srcs...>::value> {};


template <bool Same, typename Dest, typename... Srcs> struct __tuple_nothrow_assignable_aux : false_type {};

template <typename... Dests, typename... Srcs>
struct __tuple_nothrow_assignable_aux<true, tuple<Dests...>, Srcs...>
: bool_constant<conjunction<is_nothrow_assignable<Dests&, Srcs>...>::value> {};

/**
 * @struct tuple_nothrow_assignable
 * @brief 检查元组是否可以无异常赋值
 * @tparam Dest 目标元组类型
 * @tparam Srcs 源参数类型
 */
template <typename Dest, typename... Srcs>
struct tuple_nothrow_assignable
: bool_constant<
          inner::__tuple_nothrow_assignable_aux<tuple_size<Dest>::value == sizeof...(Srcs), Dest, Srcs...>::value> {};

NEFORCE_END_INNER__
/// @endcond

/**
 * @defgroup Tuple 元组
 * @brief 元组的主模板、特化实现和辅助函数
 * @{
 */

/**
 * @brief 空元组特化
 *
 * 表示不包含任何元素的元组，作为递归基类。
 */
template <> struct tuple<> : icommon<tuple<>> {
    constexpr tuple() noexcept = default;             ///< 默认构造函数
    constexpr tuple(const tuple&) noexcept = default; ///< 拷贝构造函数

    /**
     * @brief 精确参数构造标签构造函数
     * @tparam Tag 标签类型
     */
    template <typename Tag, enable_if_t<is_same<Tag, exact_arg_construct_tag>::value, int> = 0>
    constexpr explicit tuple(Tag) noexcept {}

    NEFORCE_CONSTEXPR14 tuple& operator=(const tuple&) noexcept = default; ///< 拷贝赋值运算符

    /**
     * @brief 比较两个空元组是否相等
     * @return 总是返回true
     */
    NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE constexpr bool equal_to(const tuple&) const noexcept { return true; }

    /**
     * @brief 比较两个空元组的大小关系
     * @return 总是返回false
     */
    NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE constexpr bool less_to(const tuple&) const noexcept { return false; }

    /**
     * @brief 交换操作
     */
    NEFORCE_ALWAYS_INLINE NEFORCE_CONSTEXPR14 void swap(tuple&) noexcept {}

    /**
     * @brief 相等比较运算符
     * @return 总是返回true
     */
    NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE constexpr bool operator==(const tuple& rhs) const noexcept {
        return this->equal_to(rhs);
    }

    /**
     * @brief 小于比较运算符
     * @return 总是返回false
     */
    NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE constexpr bool operator<(const tuple& rhs) const noexcept {
        return this->less_to(rhs);
    }

    /**
     * @brief 计算空元组的哈希值
     * @return FNV偏移基准值
     */
    NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE constexpr size_t to_hash() const noexcept {
        return constants::FNV_OFFSET_BASIS;
    }
};


/**
 * @brief 元组主模板
 * @tparam This 第一个元素类型
 * @tparam Rest 剩余元素类型
 *
 * 使用递归继承实现元组，每个元组包含当前元素和继承自剩余元素的元组。
 */
template <typename This, typename... Rest>
struct tuple<This, Rest...> : private tuple<Rest...>, icommon<tuple<This, Rest...>> {
    using this_type = This;           ///< 当前元素类型
    using base_type = tuple<Rest...>; ///< 基类类型，即剩余元素的元组

private:
    this_type data_;

    /**
     * @brief 计算元组的哈希值
     * @tparam Tuple 元组类型
     * @tparam Idx 索引序列
     * @param tup 元组引用
     * @param idx 索引序列
     * @return 计算得到的哈希值
     */
    template <typename Tuple, size_t... Idx>
    static constexpr size_t __broaden_tuple(const Tuple& tup, _NEFORCE index_sequence<Idx...> idx) noexcept;

public:
    /**
     * @brief 精确参数构造函数
     * @tparam Tag 标签类型
     * @tparam U1 第一个参数类型
     * @tparam U2 剩余参数类型
     * @param this_arg 第一个元素参数
     * @param rest_arg 剩余元素参数
     */
    template <typename Tag, typename U1, typename... U2,
              enable_if_t<is_same<Tag, exact_arg_construct_tag>::value, int> = 0>
    constexpr tuple(Tag, U1&& this_arg, U2&&... rest_arg) :
    base_type(exact_arg_construct_tag{}, _NEFORCE forward<U2>(rest_arg)...),
    data_(_NEFORCE forward<U1>(this_arg)) {}

    /**
     * @brief 解包工具构造函数
     * @tparam Tag 标签类型
     * @tparam Tuple 源元组类型
     * @tparam Index 索引序列
     * @param tup 源元组
     * @param idx 索引序列
     */
    template <typename Tag, typename Tuple, size_t... Index,
              enable_if_t<is_same<Tag, unpack_utility_construct_tag>::value, int> = 0>
    constexpr tuple(Tag, Tuple&& tup, index_sequence<Index...> idx);

    /**
     * @brief 解包工具构造函数
     * @tparam Tag 标签类型
     * @tparam Tuple 源元组类型
     * @param tup 源元组
     */
    template <typename Tag, typename Tuple, enable_if_t<is_same<Tag, unpack_utility_construct_tag>::value, int> = 0>
    constexpr tuple(Tag, Tuple&& tup) :
    tuple(unpack_utility_construct_tag{}, _NEFORCE forward<Tuple>(tup),
          make_index_sequence<tuple_size<remove_cvref_t<Tuple>>::value>{}) {}


#ifdef NEFORCE_STANDARD_20
    /**
     * @brief 默认构造函数
     * @tparam T 当前元素类型
     */
    template <typename T = This,
              enable_if_t<conjunction_v<is_default_constructible<T>, is_default_constructible<Rest>...>, int> = 0>
    constexpr explicit(
            !conjunction_v<is_implicitly_default_constructible<T>, is_implicitly_default_constructible<Rest>...>)
            tuple() noexcept(
                    conjunction_v<is_nothrow_default_constructible<T>, is_nothrow_default_constructible<Rest>...>) :
    base_type(),
    data_() {}

    /**
     * @brief 拷贝构造函数
     * @tparam T 当前元素类型
     * @param this_arg 第一个元素
     * @param rest_arg 剩余元素
     */
    template <typename T = This,
              enable_if_t<inner::tuple_constructible<tuple, const T&, const Rest&...>::value, int> = 0>
    constexpr explicit(inner::tuple_explicitly_convertible<tuple, const T&, const Rest&...>::value)
            tuple(const T& this_arg, const Rest&... rest_arg) noexcept(
                    conjunction_v<is_nothrow_copy_constructible<T>, is_nothrow_copy_constructible<Rest>...>) :
    tuple(exact_arg_construct_tag{}, this_arg, rest_arg...) {}

    /**
     * @brief 完美转发构造函数
     * @tparam U1 第一个参数类型
     * @tparam U2 剩余参数类型
     * @param this_arg 第一个元素参数
     * @param rest_arg 剩余元素参数
     */
    template <typename U1, typename... U2,
              enable_if_t<conjunction_v<inner::tuple_perfect_forward<tuple, U1, U2...>,
                                        inner::tuple_constructible<tuple, U1, U2...>>,
                          int> = 0>
    constexpr explicit(inner::tuple_explicitly_convertible<tuple, U1, U2...>::value)
            tuple(U1&& this_arg,
                  U2&&... rest_arg) noexcept(inner::tuple_nothrow_constructible<tuple, U1, U2...>::value) :
    tuple(exact_arg_construct_tag{}, _NEFORCE forward<U1>(this_arg), _NEFORCE forward<U2>(rest_arg)...) {}

    /**
     * @brief 从其他元组拷贝构造
     * @tparam U 源元组元素类型
     * @param tup 源元组
     */
    template <typename... U, enable_if_t<conjunction_v<inner::tuple_constructible<tuple, const U&...>,
                                                       inner::tuple_convertible<tuple, const tuple<U...>&, U...>>,
                                         int> = 0>
    constexpr explicit(inner::tuple_explicitly_convertible<tuple, const U&...>::value)
            tuple(const tuple<U...>& tup) noexcept(inner::tuple_nothrow_constructible<tuple, const U&...>::value) :
    tuple(unpack_utility_construct_tag{}, tup) {}

    /**
     * @brief 从其他元组移动构造
     * @tparam U 源元组元素类型
     * @param tup 源元组
     */
    template <typename... U, enable_if_t<conjunction_v<inner::tuple_constructible<tuple, U...>,
                                                       inner::tuple_convertible<tuple, tuple<U...>, U...>>,
                                         int> = 0>
    constexpr explicit(inner::tuple_explicitly_convertible<tuple, U...>::value)
            tuple(tuple<U...>&& tup) noexcept(inner::tuple_nothrow_constructible<tuple, U...>::value) :
    tuple(unpack_utility_construct_tag{}, _NEFORCE move(tup)) {}

    /**
     * @brief 从pair拷贝构造
     * @tparam T1 第一个元素类型
     * @tparam T2 第二个元素类型
     * @param pir 源pair
     */
    template <typename T1, typename T2,
              enable_if_t<inner::tuple_constructible<tuple, const T1&, const T2&>::value, int> = 0>
    constexpr explicit(inner::tuple_explicitly_convertible<tuple, const T1&, const T2&>::value) tuple(
            const pair<T1, T2>& pir) noexcept(inner::tuple_nothrow_constructible<tuple, const T1&, const T2&>::value) :
    tuple(unpack_utility_construct_tag{}, pir) {}

    /**
     * @brief 从pair移动构造
     * @tparam T1 第一个元素类型
     * @tparam T2 第二个元素类型
     * @param pir 源pair
     */
    template <typename T1, typename T2, enable_if_t<inner::tuple_constructible<tuple, T1, T2>::value, int> = 0>
    constexpr explicit(inner::tuple_explicitly_convertible<tuple, T1, T2>::value)
            tuple(pair<T1, T2>&& pir) noexcept(inner::tuple_nothrow_constructible<tuple, T1, T2>::value) :
    tuple(unpack_utility_construct_tag{}, _NEFORCE move(pir)) {}
#else
    template <typename T = This,
              enable_if_t<conjunction<is_default_constructible<T>, is_default_constructible<Rest>...>::value &&
                                  !conjunction<is_implicitly_default_constructible<T>,
                                               is_implicitly_default_constructible<Rest>...>::value,
                          int> = 0>
    explicit tuple() noexcept(
            conjunction<is_nothrow_default_constructible<T>, is_nothrow_default_constructible<Rest>...>::value) :
    base_type(),
    data_() {}

    template <typename T = This,
              enable_if_t<conjunction<is_default_constructible<T>, is_default_constructible<Rest>...>::value &&
                                  conjunction<is_implicitly_default_constructible<T>,
                                              is_implicitly_default_constructible<Rest>...>::value,
                          int> = 0>
    tuple() noexcept(
            conjunction<is_nothrow_default_constructible<T>, is_nothrow_default_constructible<Rest>...>::value) :
    base_type(),
    data_() {}

    template <typename T = This,
              enable_if_t<inner::tuple_constructible<tuple, const T&, const Rest&...>::value &&
                                  inner::tuple_explicitly_convertible<tuple, const T&, const Rest&...>::value,
                          int> = 0>
    explicit tuple(const T& this_arg, const Rest&... rest_arg) noexcept(
            conjunction<is_nothrow_copy_constructible<T>, is_nothrow_copy_constructible<Rest>...>::value) :
    tuple(exact_arg_construct_tag{}, this_arg, rest_arg...) {}

    template <typename T = This,
              enable_if_t<inner::tuple_constructible<tuple, const T&, const Rest&...>::value &&
                                  !inner::tuple_explicitly_convertible<tuple, const T&, const Rest&...>::value,
                          int> = 0>
    tuple(const T& this_arg, const Rest&... rest_arg) noexcept(
            conjunction<is_nothrow_copy_constructible<T>, is_nothrow_copy_constructible<Rest>...>::value) :
    tuple(exact_arg_construct_tag{}, this_arg, rest_arg...) {}

    template <typename U1, typename... U2,
              enable_if_t<conjunction<inner::tuple_constructible<tuple, U1, U2...>,
                                      inner::tuple_convertible<tuple, U1, U2...>>::value &&
                                  inner::tuple_explicitly_convertible<tuple, U1, U2...>::value,
                          int> = 0>
    explicit tuple(U1&& this_arg,
                   U2&&... rest_arg) noexcept(inner::tuple_nothrow_constructible<tuple, U1, U2...>::value) :
    tuple(exact_arg_construct_tag{}, _NEFORCE forward<U1>(this_arg), _NEFORCE forward<U2>(rest_arg)...) {}

    template <typename U1, typename... U2,
              enable_if_t<conjunction<inner::tuple_constructible<tuple, U1, U2...>,
                                      inner::tuple_convertible<tuple, U1, U2...>>::value &&
                                  !inner::tuple_explicitly_convertible<tuple, U1, U2...>::value,
                          int> = 0>
    tuple(U1&& this_arg, U2&&... rest_arg) noexcept(inner::tuple_nothrow_constructible<tuple, U1, U2...>::value) :
    tuple(exact_arg_construct_tag{}, _NEFORCE forward<U1>(this_arg), _NEFORCE forward<U2>(rest_arg)...) {}

    template <typename... U,
              enable_if_t<conjunction<inner::tuple_constructible<tuple, const U&...>,
                                      inner::tuple_convertible<tuple, const tuple<U...>&, U...>>::value &&
                                  inner::tuple_explicitly_convertible<tuple, const U&...>::value,
                          int> = 0>
    explicit tuple(const tuple<U...>& tup) noexcept(inner::tuple_nothrow_constructible<tuple, const U&...>::value) :
    tuple(unpack_utility_construct_tag{}, tup) {}

    template <typename... U,
              enable_if_t<conjunction<inner::tuple_constructible<tuple, const U&...>,
                                      inner::tuple_convertible<tuple, const tuple<U...>&, U...>>::value &&
                                  !inner::tuple_explicitly_convertible<tuple, const U&...>::value,
                          int> = 0>
    tuple(const tuple<U...>& tup) noexcept(inner::tuple_nothrow_constructible<tuple, const U&...>::value) :
    tuple(unpack_utility_construct_tag{}, tup) {}

    template <typename... U, enable_if_t<conjunction<inner::tuple_constructible<tuple, U...>,
                                                     inner::tuple_convertible<tuple, tuple<U...>, U...>>::value &&
                                                 inner::tuple_explicitly_convertible<tuple, U...>::value,
                                         int> = 0>
    explicit tuple(tuple<U...>&& tup) noexcept(inner::tuple_nothrow_constructible<tuple, U...>::value) :
    tuple(unpack_utility_construct_tag{}, _NEFORCE move(tup)) {}

    template <typename... U, enable_if_t<conjunction<inner::tuple_constructible<tuple, U...>,
                                                     inner::tuple_convertible<tuple, tuple<U...>, U...>>::value &&
                                                 !inner::tuple_explicitly_convertible<tuple, U...>::value,
                                         int> = 0>
    tuple(tuple<U...>&& tup) noexcept(inner::tuple_nothrow_constructible<tuple, U...>::value) :
    tuple(unpack_utility_construct_tag{}, _NEFORCE move(tup)) {}

    template <typename T1, typename T2,
              enable_if_t<inner::tuple_constructible<tuple, const T1&, const T2&>::value &&
                                  inner::tuple_explicitly_convertible<tuple, const T1&, const T2&>::value,
                          int> = 0>
    explicit tuple(const pair<T1, T2>& pir) noexcept(
            inner::tuple_nothrow_constructible<tuple, const T1&, const T2&>::value) :
    tuple(unpack_utility_construct_tag{}, pir) {}

    template <typename T1, typename T2,
              enable_if_t<inner::tuple_constructible<tuple, const T1&, const T2&>::value &&
                                  !inner::tuple_explicitly_convertible<tuple, const T1&, const T2&>::value,
                          int> = 0>
    tuple(const pair<T1, T2>& pir) noexcept(inner::tuple_nothrow_constructible<tuple, const T1&, const T2&>::value) :
    tuple(unpack_utility_construct_tag{}, pir) {}

    template <typename T1, typename T2,
              enable_if_t<inner::tuple_constructible<tuple, T1, T2>::value &&
                                  inner::tuple_explicitly_convertible<tuple, T1, T2>::value,
                          int> = 0>
    explicit tuple(pair<T1, T2>&& pir) noexcept(inner::tuple_nothrow_constructible<tuple, T1, T2>::value) :
    tuple(unpack_utility_construct_tag{}, _NEFORCE move(pir)) {}

    template <typename T1, typename T2,
              enable_if_t<inner::tuple_constructible<tuple, T1, T2>::value &&
                                  !inner::tuple_explicitly_convertible<tuple, T1, T2>::value,
                          int> = 0>
    tuple(pair<T1, T2>&& pir) noexcept(inner::tuple_nothrow_constructible<tuple, T1, T2>::value) :
    tuple(unpack_utility_construct_tag{}, _NEFORCE move(pir)) {}
#endif

    tuple(const tuple&) = default; ///< 拷贝构造函数
    tuple(tuple&&) = default;      ///< 移动构造函数

    /**
     * @brief 拷贝赋值运算符
     * @tparam T 当前元素类型
     * @param tup 源元组
     * @return 当前元组的引用
     */
    template <typename T = This,
              enable_if_t<conjunction<is_copy_assignable<T>, is_copy_assignable<Rest>...>::value, int> = 0>
    NEFORCE_CONSTEXPR14 tuple& operator=(type_identity_t<const tuple&> tup) noexcept(
            conjunction<is_nothrow_copy_assignable<T>, is_nothrow_copy_assignable<Rest>...>::value) {
        data_ = tup.data_;
        get_rest() = tup.get_rest();
        return *this;
    }

    /**
     * @brief 移动赋值运算符
     * @tparam T 当前元素类型
     * @param tup 源元组
     * @return 当前元组的引用
     */
    template <typename T = This,
              enable_if_t<conjunction<is_move_assignable<T>, is_move_assignable<Rest>...>::value, int> = 0>
    NEFORCE_CONSTEXPR14 tuple& operator=(type_identity_t<tuple&&> tup) noexcept(
            conjunction<is_nothrow_move_assignable<T>, is_nothrow_move_assignable<Rest>...>::value) {
        data_ = _NEFORCE forward<T>(tup.data_);
        get_rest() = _NEFORCE forward<base_type>(tup.get_rest());
        return *this;
    }

    /**
     * @brief 从其他元组拷贝赋值
     * @tparam U 源元组元素类型
     * @param tup 源元组
     * @return 当前元组的引用
     */
    template <typename... U, enable_if_t<conjunction<negation<is_same<tuple, tuple<U...>>>,
                                                     inner::tuple_assignable<tuple, const U&...>>::value,
                                         int> = 0>
    NEFORCE_CONSTEXPR14 tuple&
    operator=(const tuple<U...>& tup) noexcept(inner::tuple_nothrow_assignable<tuple, const U&...>::value) {
        data_ = tup.data_;
        get_rest() = tup.get_rest();
        return *this;
    }

    /**
     * @brief 从其他元组移动赋值
     * @tparam U 源元组元素类型
     * @param tup 源元组
     * @return 当前元组的引用
     */
    template <
            typename... U,
            enable_if_t<conjunction<negation<is_same<tuple, tuple<U...>>>, inner::tuple_assignable<tuple, U...>>::value,
                        int> = 0>
    NEFORCE_CONSTEXPR14 tuple&
    operator=(tuple<U...>&& tup) noexcept(inner::tuple_nothrow_assignable<tuple, U...>::value) {
        data_ = _NEFORCE forward<typename tuple<U...>::this_type>(tup.data_);
        get_rest() = _NEFORCE forward<typename tuple<U...>::super>(tup.get_rest());
        return *this;
    }

    /**
     * @brief 从pair拷贝赋值
     * @tparam T1 第一个元素类型
     * @tparam T2 第二个元素类型
     * @param pir 源pair
     * @return 当前元组的引用
     */
    template <typename T1, typename T2,
              enable_if_t<inner::tuple_assignable<tuple, const T1&, const T2&>::value, int> = 0>
    NEFORCE_CONSTEXPR14 tuple&
    operator=(const pair<T1, T2>& pir) noexcept(inner::tuple_nothrow_assignable<tuple, const T1&, const T2&>::value) {
        data_ = pir.first;
        get_rest().data_ = pir.second;
        return *this;
    }

    /**
     * @brief 从pair移动赋值
     * @tparam T1 第一个元素类型
     * @tparam T2 第二个元素类型
     * @param pir 源pair
     * @return 当前元组的引用
     */
    template <typename T1, typename T2, enable_if_t<inner::tuple_assignable<tuple, T1, T2>::value, int> = 0>
    NEFORCE_CONSTEXPR14 tuple&
    operator=(pair<T1, T2>&& pir) noexcept(inner::tuple_nothrow_assignable<tuple, T1, T2>::value) {
        data_ = _NEFORCE forward<T1>(pir.first);
        get_rest().data_ = _NEFORCE forward<T2>(pir.second);
        return *this;
    }

    tuple& operator=(const volatile tuple&) = delete; ///< 禁止volatile拷贝赋值

    /**
     * @brief 获取剩余元素的元组引用
     * @return 基类元组的引用
     */
    NEFORCE_CONSTEXPR14 base_type& get_rest() noexcept { return *this; }

    /**
     * @brief 获取剩余元素的元组常量引用
     * @return 基类元组的常量引用
     */
    NEFORCE_CONSTEXPR14 const base_type& get_rest() const noexcept { return *this; }

    /**
     * @brief 比较两个元组是否相等
     * @tparam U 其他元组元素类型
     * @param t 其他元组
     * @return 如果所有对应元素相等则返回true，否则返回false
     */
    template <typename... U> NEFORCE_NODISCARD constexpr bool equal_to(const tuple<U...>& t) const {
        return data_ == t.data_ && base_type::equal_to(t.get_rest());
    }

    /**
     * @brief 比较两个元组的大小关系
     * @tparam U 其他元组元素类型
     * @param rhs 其他元组
     * @return 如果当前元组小于其他元组则返回true，否则返回false
     */
    template <typename... U> NEFORCE_NODISCARD constexpr bool less_to(const tuple<U...>& rhs) const {
        return data_ < rhs.data_ || (!(rhs.data_ < data_) && base_type::less_to(rhs.get_rest()));
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

    /// 相等比较运算符
    NEFORCE_NODISCARD constexpr bool operator==(const tuple& rhs) const noexcept { return this->equal_to(rhs); }
    /// 小于比较运算符
    NEFORCE_NODISCARD constexpr bool operator<(const tuple& rhs) const noexcept { return this->less_to(rhs); }

    /**
     * @brief 计算元组的哈希值
     * @return 元组的哈希值
     */
    NEFORCE_NODISCARD constexpr size_t to_hash() const noexcept {
        return tuple::__broaden_tuple(*this, _NEFORCE index_sequence_for<This, Rest...>());
    }

    /**
     * @brief 交换两个元组的内容
     * @param t 要交换的元组
     */
    NEFORCE_CONSTEXPR14 void
    swap(tuple& t) noexcept(conjunction<is_nothrow_swappable<This>, is_nothrow_swappable<Rest>...>::value) {
        _NEFORCE swap(data_, t.data_);
        base_type::swap(t.get_rest());
    }
};
#ifdef NEFORCE_STANDARD_17
template <typename... Types> tuple(Types...) -> tuple<Types...>;

template <typename T1, typename T2> tuple(pair<T1, T2>) -> tuple<T1, T2>;
#endif


/**
 * @brief 获取元组中指定位置的元素引用
 * @tparam Index 元素索引
 * @tparam Types 元组元素类型
 * @param t 元组
 * @return 指定位置元素的引用
 */
template <size_t Index, typename... Types>
NEFORCE_NODISCARD constexpr tuple_element_t<Index, Types...>& get(tuple<Types...>& t) noexcept {
    using T = tuple_element_t<Index, tuple<Types...>>;
    using tuple_type = tuple_extract_base_t<Index, tuple<Types...>>;
    return static_cast<T&>(static_cast<tuple_type&>(t).data_);
}

/**
 * @brief 获取元组中指定位置的元素常量引用
 * @tparam Index 元素索引
 * @tparam Types 元组元素类型
 * @param t 元组
 * @return 指定位置元素的常量引用
 */
template <size_t Index, typename... Types>
NEFORCE_NODISCARD constexpr const tuple_element_t<Index, Types...>& get(const tuple<Types...>& t) noexcept {
    using T = tuple_element_t<Index, tuple<Types...>>;
    using tuple_type = tuple_extract_base_t<Index, tuple<Types...>>;
    return static_cast<const T&>(static_cast<const tuple_type&>(t).data_);
}

/**
 * @brief 获取元组中指定位置的元素右值引用
 * @tparam Index 元素索引
 * @tparam Types 元组元素类型
 * @param t 元组
 * @return 指定位置元素的右值引用
 */
template <size_t Index, typename... Types>
NEFORCE_NODISCARD constexpr tuple_element_t<Index, Types...>&& get(tuple<Types...>&& t) noexcept {
    using T = tuple_element_t<Index, tuple<Types...>>;
    using tuple_type = tuple_extract_base_t<Index, tuple<Types...>>;
    return static_cast<T&&>(static_cast<tuple_type&&>(t).data_);
}

/**
 * @brief 获取元组中指定位置的元素常量右值引用
 * @tparam Index 元素索引
 * @tparam Types 元组元素类型
 * @param t 元组
 * @return 指定位置元素的常量右值引用
 */
template <size_t Index, typename... Types>
NEFORCE_NODISCARD constexpr const tuple_element_t<Index, Types...>&& get(const tuple<Types...>&& t) noexcept {
    using T = tuple_element_t<Index, tuple<Types...>>;
    using tuple_type = tuple_extract_base_t<Index, tuple<Types...>>;
    return static_cast<const T&&>(static_cast<const tuple_type&&>(t).data_);
}

/// @cond
NEFORCE_BEGIN_INNER__
/**
 * @brief 从元组中获取元素
 * @tparam Index 元素索引
 * @tparam Types 元组元素类型
 * @param t 元组
 * @return 指定位置元素的右值引用
 */
template <size_t Index, typename... Types>
NEFORCE_NODISCARD constexpr tuple_element_t<Index, Types...>&& __pair_get_from_tuple(tuple<Types...>&& t) noexcept {
    using T = tuple_element_t<Index, tuple<Types...>>;
    using tuple_type = tuple_extract_base_t<Index, tuple<Types...>>;
    return static_cast<T&&>(static_cast<tuple_type&>(t).data_);
}
NEFORCE_END_INNER__

template <typename This, typename... Rest>
template <typename Tag, typename Tuple, size_t... Index,
          enable_if_t<is_same<Tag, unpack_utility_construct_tag>::value, int>>
constexpr tuple<This, Rest...>::tuple(Tag, Tuple&& tup, index_sequence<Index...> idx) :
tuple(exact_arg_construct_tag{}, _NEFORCE get<Index>(_NEFORCE forward<Tuple>(tup))...) {}

/// @endcond

/**
 * @brief 从参数创建元组
 * @tparam Types 参数类型
 * @param args 参数
 * @return 创建的元组
 */
template <typename... Types>
NEFORCE_NODISCARD constexpr tuple<unwrap_ref_decay_t<Types>...> make_tuple(Types&&... args) {
    using tuple_type = tuple<unwrap_ref_decay_t<Types>...>;
    return tuple_type(_NEFORCE forward<Types>(args)...);
}

/**
 * @brief 创建引用元组
 * @tparam Types 引用类型
 * @param args 引用参数
 * @return 创建的引用元组
 */
template <typename... Types> NEFORCE_NODISCARD constexpr tuple<Types&...> tie(Types&... args) noexcept {
    using tuple_type = tuple<Types&...>;
    return tuple_type(args...);
}

/**
 * @brief 创建转发引用元组
 * @tparam Types 转发引用类型
 * @param args 转发引用参数
 * @return 创建的转发引用元组
 */
template <typename... Types> NEFORCE_NODISCARD constexpr tuple<Types&&...> forward_as_tuple(Types&&... args) noexcept {
    using tuple_type = tuple<Types&&...>;
    return tuple_type(_NEFORCE forward<Types>(args)...);
}

/// @cond
NEFORCE_BEGIN_INNER__

template <typename, typename Tuple, typename = make_index_sequence<tuple_size<remove_reference_t<Tuple>>::value>>
struct constructible_from_tuple : false_type {};

template <typename T, typename Tuple, size_t... Index>
struct constructible_from_tuple<T, Tuple, index_sequence<Index...>>
: bool_constant<is_constructible<T, decltype(_NEFORCE get<Index>(_NEFORCE declval<Tuple>()))...>::value> {};

/**
 * @brief 从元组构造对象
 * @tparam T 目标类型
 * @tparam Tuple 源元组类型
 * @tparam Index 索引序列
 * @param tup 源元组
 * @param idx 索引序列
 * @return 构造的对象
 */
template <typename T, typename Tuple, size_t... Index>
NEFORCE_NODISCARD constexpr T __broaden_make_from_tuple(Tuple&& tup, index_sequence<Index...> idx) noexcept(
        is_nothrow_constructible<T, decltype(_NEFORCE get<Index>(_NEFORCE forward<Tuple>(tup)))...>::value) {
    return T(_NEFORCE get<Index>(_NEFORCE forward<Tuple>(tup))...);
}

NEFORCE_END_INNER__
/// @endcond


/**
 * @brief 从元组构造对象
 * @tparam T 目标类型
 * @tparam Tuple 源元组类型
 * @param tup 源元组
 * @return 构造的对象
 */
template <typename T, typename Tuple, enable_if_t<inner::constructible_from_tuple<T, Tuple>::value, int> = 0>
NEFORCE_NODISCARD constexpr T make_from_tuple(Tuple&& tup) noexcept(noexcept(inner::__broaden_make_from_tuple<T>(
        _NEFORCE forward<Tuple>(tup), make_index_sequence<tuple_size<remove_reference_t<Tuple>>::value>{}))) {
    return inner::__broaden_make_from_tuple<T>(_NEFORCE forward<Tuple>(tup),
                                               make_index_sequence<tuple_size<remove_reference_t<Tuple>>::value>{});
}


/// @cond
NEFORCE_BEGIN_INNER__

template <typename, typename, typename, size_t, typename...> struct __tuple_cat_aux;

template <typename Tuple, size_t... ElementIdx, size_t... TupleIdx, size_t NextTuple>
struct __tuple_cat_aux<Tuple, index_sequence<ElementIdx...>, index_sequence<TupleIdx...>, NextTuple> {
    using Ret = tuple<tuple_element_t<ElementIdx, remove_cvref_t<tuple_element_t<TupleIdx, Tuple>>>...>;
    using ElementIdxSeq = index_sequence<ElementIdx...>;
    using TupleIdxSeq = index_sequence<TupleIdx...>;
};

template <typename Tuple, size_t... ElementIdx, size_t... TupleIdx, size_t NextTuple, size_t... NextElement,
          typename... Rest>
struct __tuple_cat_aux<Tuple, index_sequence<ElementIdx...>, index_sequence<TupleIdx...>, NextTuple,
                       index_sequence<NextElement...>, Rest...>
: __tuple_cat_aux<Tuple, index_sequence<ElementIdx..., NextElement...>,
                  index_sequence<TupleIdx..., (NextTuple + 0 * NextElement)...>, NextTuple + 1, Rest...> {};

template <typename... Tuples>
using tuple_cat_bind_t = __tuple_cat_aux<tuple<Tuples&&...>, index_sequence<>, index_sequence<>, 0,
                                         make_index_sequence<tuple_size<remove_cvref_t<Tuples>>::value>...>;

/**
 * @brief 元组连接内部实现
 * @tparam Ret 返回类型
 * @tparam ElementIdx 元素索引序列
 * @tparam TupleIdx 元组索引序列
 * @tparam Tuple 元组类型
 * @param ei 元素索引序列
 * @param ti 元组索引序列
 * @param tup 元组
 * @return 连接后的元组
 */
template <typename Ret, size_t... ElementIdx, size_t... TupleIdx, typename Tuple>
constexpr Ret __tuple_cat_in_turn(index_sequence<ElementIdx...> ei, index_sequence<TupleIdx...> ti, Tuple tup) {
    return Ret{_NEFORCE get<ElementIdx>(_NEFORCE get<TupleIdx>(_NEFORCE move(tup)))...};
}

NEFORCE_END_INNER__
/// @endcond


/**
 * @brief 连接多个元组
 * @tparam Tuples 要连接的元组类型
 * @param tuples 要连接的元组
 * @return 连接后的元组
 */
template <typename... Tuples>
NEFORCE_NODISCARD constexpr typename inner::tuple_cat_bind_t<Tuples...>::Ret tuple_cat(Tuples&&... tuples) {
    using CatImpl = inner::tuple_cat_bind_t<Tuples...>;
    using Ret = typename CatImpl::Ret;
    using ElementIdxSeq = typename CatImpl::ElementIdxSeq;
    using TupleIdxSeq = typename CatImpl::TupleIdxSeq;
    return inner::__tuple_cat_in_turn<Ret>(ElementIdxSeq{}, TupleIdxSeq{},
                                           _NEFORCE forward_as_tuple(_NEFORCE forward<Tuples>(tuples)...));
}

/** @} */ // Tuple

#if !defined(NEFORCE_STANDARD_17)
/// @cond
NEFORCE_BEGIN_INNER__

template <typename Tuple, size_t Index> struct __broadern_tuple_hash_aux {
    static constexpr size_t hash(const Tuple& tup) {
        using ElementType = remove_cvref_t<tuple_element_t<Index - 1, Tuple>>;
        return __broadern_tuple_hash_aux<Tuple, Index - 1>::hash(tup) ^
               _NEFORCE hash<ElementType>()(_NEFORCE get<Index - 1>(tup));
    }
};
template <typename Tuple> struct __broadern_tuple_hash_aux<Tuple, 1> {
    static constexpr size_t hash(const Tuple& tup) {
        using ElementType = remove_cvref_t<tuple_element_t<0, Tuple>>;
        return _NEFORCE hash<ElementType>()(_NEFORCE get<0>(tup));
    }
};
template <typename Tuple> struct __broadern_tuple_hash_aux<Tuple, 0> {
    static constexpr size_t hash(const Tuple&) { return 0; }
};

NEFORCE_END_INNER__
/// @endcond
#endif // !NEFORCE_STANDARD_17


template <typename This, typename... Rest>
template <typename Tuple, size_t... Idx>
constexpr size_t tuple<This, Rest...>::__broaden_tuple(const Tuple& tup, index_sequence<Idx...>) noexcept {
#ifdef NEFORCE_STANDARD_17
    return (hash<remove_cvref_t<tuple_element_t<Idx, Tuple>>>()(_NEFORCE get<Idx>(tup)) ^ ...);
#else
    return inner::__broadern_tuple_hash_aux<Tuple, sizeof...(Idx)>::hash(tup);
#endif // NEFORCE_STANDARD_17
}

NEFORCE_END_NAMESPACE__

#ifdef NEFORCE_STANDARD_17
namespace std {
    template <typename... Types>
    struct tuple_size<_NEFORCE tuple<Types...>> : _NEFORCE integral_constant<_NEFORCE size_t, sizeof...(Types)> {};

    template <_NEFORCE size_t I, typename... Types> struct tuple_element<I, _NEFORCE tuple<Types...>> {
        using type = _NEFORCE tuple_element_t<I, _NEFORCE tuple<Types...>>;
    };
} // namespace std
#endif

#endif // NEFORCE_CORE_UTILITY_TUPLE_HPP__
