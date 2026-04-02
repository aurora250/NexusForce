#ifndef NEFORCE_CORE_FUNCTIONAL_CALL_WRAPPER_HPP__
#define NEFORCE_CORE_FUNCTIONAL_CALL_WRAPPER_HPP__

/**
 * @file call_wrapper.hpp
 * @brief 延迟调用包装
 *
 * 此文件提供了调用包装器的实现，用于延迟执行函数调用和参数打包。
 */

#include "NeForce/core/utility/tuple.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup CallWrapper 延迟调用包装
 * @brief 延迟函数调用的包装器及辅助工具
 * @{
 */

/**
 * @struct call_wrapper
 * @brief 延迟调用包装器
 * @tparam Types 存储元组的参数列表
 *
 * 将函数对象和其参数打包到一个元组中，可以延迟执行函数调用。
 * 当调用operator() 时，会展开元组并执行函数调用。
 */
template <typename... Types> struct call_wrapper {
private:
    template <typename Tuple> struct result_t;

    template <typename Func, typename... Args>
    struct result_t<_NEFORCE tuple<Func, Args...>> : inner::__invoke_result_aux<Func, Args...> {};

    using Tuple = _NEFORCE tuple<decay_t<Types>...>;

    Tuple tup_; ///< 存储函数和参数的元组

    /**
     * @brief 使用索引序列调用函数
     * @tparam Index 索引序列
     * @param idx 索引序列对象
     * @return 函数调用结果
     *
     * 使用编译时索引序列展开元组中的元素，并调用函数。
     */
    template <size_t... Index> typename result_t<Tuple>::type __invoke(index_tuple<Index...> idx) {
        return _NEFORCE invoke(_NEFORCE get<Index>(_NEFORCE move(tup_))...);
    }

public:
    /**
     * @brief 构造函数
     * @tparam Args 构造函数参数类型
     * @param args 函数和参数
     *
     * 将传入的函数和参数完美转发到内部元组中存储。
     */
    template <typename... Args>
    explicit call_wrapper(Args&&... args) :
    tup_(_NEFORCE forward<Args>(args)...) {}

    /**
     * @brief 函数调用运算符
     * @return 函数调用结果
     *
     * 主调用接口。自动生成索引序列并调用invoke方法。
     */
    typename result_t<Tuple>::type operator()() {
        using Indices = build_index_tuple_t<tuple_size_v<Tuple>>;
        return this->__invoke(Indices());
    }
};

#ifdef NEFORCE_STANDARD_17
template <typename... Types> call_wrapper(Types...) -> call_wrapper<Types...>;
#endif

/** @} */ // CallWrapper

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FUNCTIONAL_CALL_WRAPPER_HPP__
