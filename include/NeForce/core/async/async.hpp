#ifndef NEFORCE_CORE_ASYNC_ASYNC_HPP__
#define NEFORCE_CORE_ASYNC_ASYNC_HPP__

/**
 * @file async.hpp
 * @brief 异步执行函数
 *
 * 此文件提供了async函数的实现，
 * 用于异步执行函数并返回关联的future对象。
 */

#include "NeForce/core/async/packaged_task.hpp"
#include "NeForce/core/functional/call_wrapper.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Async 异步行为
 * @brief 异步编程相关组件
 * @{
 */

/**
 * @brief 异步执行函数（指定策略）
 * @tparam Func 可调用类型
 * @tparam Args 参数类型
 * @param policy 执行策略
 * @param function 要异步执行的函数
 * @param args 函数参数
 * @return 关联的future对象
 *
 * 根据指定的执行策略异步执行函数：
 * - launch::async: 在新线程中异步执行
 * - launch::deferred: 延迟执行，在等待结果时才执行
 * - 混合策略: 实现定义的行为，通常优先选择异步执行
 */
template <typename Func, typename... Args>
NEFORCE_NODISCARD future<async_result_t<Func, Args...>> async(launch policy, Func&& function, Args&&... args) {
    using Wrapper = call_wrapper<Func, Args...>;
    using AsyncState = inner::__future_base::async_state_impl<Wrapper, async_result_t<Func, Args...>>;
    using DeferredState = inner::__future_base::deferred_state<Wrapper, async_result_t<Func, Args...>>;

    shared_ptr<inner::__future_base::state_base> state;
    if ((policy & launch::async) == launch::async) {
        state = _NEFORCE make_shared<AsyncState>(_NEFORCE forward<Func>(function), _NEFORCE forward<Args>(args)...);
    }
    if (!state) {
        state = _NEFORCE make_shared<DeferredState>(_NEFORCE forward<Func>(function), _NEFORCE forward<Args>(args)...);
    }
    return _NEFORCE future<async_result_t<Func, Args...>>(_NEFORCE move(state));
}

/**
 * @brief 异步执行函数（默认策略）
 * @tparam Func 可调用类型
 * @tparam Args 参数类型
 * @param function 要异步执行的函数
 * @param args 函数参数
 * @return 关联的future对象
 *
 * 使用默认执行策略异步执行函数，
 * 允许实现根据系统资源和函数特性选择最佳执行方式。
 */
template <typename Func, typename... Args>
NEFORCE_NODISCARD future<async_result_t<Func, Args...>> async(Func&& function, Args&&... args) {
    return _NEFORCE async(launch::async | launch::deferred, _NEFORCE forward<Func>(function),
                          _NEFORCE forward<Args>(args)...);
}

/** @} */ // Async

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_ASYNC_HPP__
