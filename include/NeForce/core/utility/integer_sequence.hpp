#ifndef NEFORCE_CORE_UTILITY_INTEGER_SEQUENCE_HPP__
#define NEFORCE_CORE_UTILITY_INTEGER_SEQUENCE_HPP__

/**
 * @file integer_sequence.hpp
 * @brief 整数序列工具
 *
 * 此文件提供了编译时整数序列的实现，用于模板元编程和可变模板参数展开，
 * 支持相关工具，以及编译器特定的优化实现。
 */

#include "NeForce/core/typeinfo/type_traits.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup IntegerSequence 整数序列
 * @brief 编译时整数序列的定义和操作
 * @{
 */

/**
 * @struct integer_sequence
 * @brief 编译时整数序列容器
 * @tparam T 整数类型
 * @tparam Values 整数序列值
 *
 * 将一组编译时整数包装为类型，用于模板元编程中展开可变模板参数。
 *
 * @note T必须是整数类型。
 */
template <typename T, T... Values> struct integer_sequence {
    static_assert(is_integral<T>::value, "integer sequence requires integral types.");

    using value_type = T; ///< 序列中整数的类型

    /**
     * @brief 获取序列中整数的数量
     * @return 序列大小
     */
    NEFORCE_NODISCARD static constexpr size_t size() noexcept { return sizeof...(Values); }
};

/**
 * @typedef make_integer_sequence
 * @brief 生成指定长度的整数序列
 * @tparam T 整数类型
 * @tparam Size 序列长度
 *
 * 生成一个从0到Size-1的整数序列。
 */
template <typename T, T Size>
using make_integer_sequence =
#if defined(NEFORCE_COMPILER_MSVC) || defined(NEFORCE_COMPILER_CLANG)
        __make_integer_seq<integer_sequence, T, Size>;
#else
        integer_sequence<T, __integer_pack(Size)...>;
#endif

/**
 * @typedef index_sequence
 * @brief 索引序列
 * @tparam Values 索引值序列
 *
 * 使用size_t作为整数类型的integer_sequence特化，专门用于索引操作。
 */
template <size_t... Values> using index_sequence = integer_sequence<size_t, Values...>;

/**
 * @typedef make_index_sequence
 * @brief 生成指定长度的索引序列
 * @tparam Size 序列长度
 *
 * 生成一个从0到Size-1的size_t索引序列。
 */
template <size_t Size> using make_index_sequence = make_integer_sequence<size_t, Size>;

/**
 * @typedef index_sequence_for
 * @brief 根据类型参数包生成索引序列
 * @tparam Types 类型参数包
 *
 * 生成一个长度等于Types参数包大小的索引序列。
 */
template <typename... Types> using index_sequence_for = make_index_sequence<sizeof...(Types)>;

/** @} */ // IntegerSequence

/**
 * @defgroup IndexTuple 索引元组
 * @brief 基于索引序列的元组工具
 * @{
 */

/**
 * @struct index_tuple
 * @brief 索引元组容器
 * @tparam Values 索引值序列
 *
 * 将索引序列包装为独立类型，用于需要区分integer_sequence和其他类型的场景。
 */
template <size_t... Values> struct index_tuple {};

/**
 * @struct build_index_tuple
 * @brief 构建指定长度的索引元组
 * @tparam Num 元组长度
 *
 * 通过类型推导机制从索引序列构建索引元组。
 */
template <size_t Num> struct build_index_tuple {
private:
    template <size_t... Is> static index_tuple<Is...> convert(index_sequence<Is...>);

public:
    /**
     * @brief 生成的索引元组类型
     */
    using type = decltype(build_index_tuple::convert(make_index_sequence<Num>{}));
};

/**
 * @typedef build_index_tuple_t
 * @brief build_index_tuple的便捷别名
 */
template <size_t Num> using build_index_tuple_t = typename build_index_tuple<Num>::type;

/** @} */ // IndexTuple

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_UTILITY_INTEGER_SEQUENCE_HPP__
