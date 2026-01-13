#ifndef MSTL_CORE_FUNCTIONAL_CALL_WAPPER_HPP__
#define MSTL_CORE_FUNCTIONAL_CALL_WAPPER_HPP__

/**
 * @file call_wrapper.hpp
 * @brief MSTL调用包装器
 * @namespace MSTL
 * @ingroup CallWrapper
 *
 * 此文件提供了调用包装器的实现，用于延迟执行函数调用和参数打包。
 */

#include "../utility/integer_sequence.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup Invoker 调用器
 * @brief 延迟函数调用的包装器
 * @{
 */

/**
 * @struct invoker
 * @brief 函数调用包装器
 * @tparam Tuple 包含函数和参数的元组类型
 *
 * 将函数对象和其参数打包到一个元组中，可以延迟执行函数调用。
 * 当调用operator() 时，会展开元组并执行函数调用。
 */
template <typename Tuple>
struct invoker {
private:
    template <typename>
    struct result_t;

    template <typename Func, typename... Args>
    struct result_t<tuple<Func, Args...>>
        : _INNER __invoke_result_aux<Func, Args...> {};

    Tuple tup_; ///< 存储函数和参数的元组

public:
    /**
     * @brief 构造函数
     * @tparam Args 构造函数参数类型
     * @param args 函数和参数
     *
     * 将传入的函数和参数完美转发到内部元组中存储。
     */
    template <typename... Args>
    explicit invoker(Args&&... args)
    : tup_(_MSTL forward<Args>(args)...) {}

    /**
     * @brief 使用索引序列调用函数
     * @tparam Index 索引序列
     * @param idx 索引序列对象
     * @return 函数调用结果
     *
     * 使用编译时索引序列展开元组中的元素，并调用函数。
     */
    template <size_t... Index>
    typename result_t<Tuple>::type invoke(index_tuple<Index...> idx) {
        return _MSTL invoke(_MSTL get<Index>(_MSTL move(tup_))...);
    }

    /**
     * @brief 函数调用运算符
     * @return 函数调用结果
     *
     * 主调用接口。自动生成索引序列并调用invoke方法。
     */
    typename result_t<Tuple>::type operator ()() {
        using Indices = build_index_tuple_t<tuple_size_v<Tuple>>;
        return this->invoke(Indices());
    }
};

/** @} */ // Invoker

/**
 * @defgroup CallWrapper 调用包装器别名
 * @brief 方便的调用包装器类型别名
 * @{
*/

/**
 * @typedef call_wrapper
 * @brief 调用包装器的便捷类型别名
 * @tparam T 函数和参数类型
 *
 * 自动推导函数和参数类型，并创建对应的invoker实例。
 */
template <typename... T>
using call_wrapper = invoker<tuple<decay_t<T>...>>;

/** @} */ // CallWrapper

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FUNCTIONAL_CALL_WAPPER_HPP__
