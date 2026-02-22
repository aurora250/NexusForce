#ifndef MSTL_CORE_ASYNC_COROUTINE_HPP__
#define MSTL_CORE_ASYNC_COROUTINE_HPP__

/**
 * @file coroutine.hpp
 * @brief 协程支持
 *
 * 此文件提供了协程的基础设施，
 * 包括协程特征、协程句柄、暂停点定义等。
 */

#include "MSTL/core/functional/hash.hpp"
#if defined(MSTL_STANDARD_20__) || defined(MSTL_DOXYGEN_GENERATE)
#include <coroutine>
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup Coroutine 协程
 * @brief 协程支持
 * @{
 */

/**
 * @struct coroutine_traits
 * @brief 协程特征类
 * @tparam Res 协程返回类型
 * @tparam Args 协程参数类型
 *
 * 用于确定协程的promise类型，返回类型必须包含promise_type成员。
 */
template <typename Res, typename... Args>
struct coroutine_traits;

MSTL_BEGIN_INNER__

template <typename, typename = void>
struct coroutine_traits_impl {};

template <typename Res>
struct coroutine_traits_impl<Res, void_t<typename Res::promise_type>> {
    using promise_type = typename Res::promise_type;
};

MSTL_END_INNER__

/**
 * @brief 协程特征主模板
 * @tparam Res 协程返回类型
 * @tparam Args 协程参数类型
 */
template <typename Res, typename... Args>
struct coroutine_traits : _INNER coroutine_traits_impl<Res> {};


/**
 * @brief 协程句柄
 * @tparam Promise promise类型
 *
 * 使用自定义coroutine_handle会编译失败，
 * 故对std::coroutine_handle重新导出，用于管理协程的生命周期。
 * 提供恢复、销毁等操作。
 */
template <typename Promise = void>
using coroutine_handle = std::coroutine_handle<Promise>;

template <typename Promise>
struct hash<coroutine_handle<Promise>> {
    size_t operator()(const coroutine_handle<Promise>& handle) const noexcept {
        return reinterpret_cast<size_t>(handle.address());
    }
};


/**
 * @brief 空操作协程的promise类型别名
 */
using noop_coroutine_promise = std::noop_coroutine_handle;

/**
 * @brief 空操作协程句柄类型别名
 */
using noop_coroutine_handle = coroutine_handle<noop_coroutine_promise>;

/**
 * @brief 获取空操作协程句柄
 * @return 空操作协程句柄
 *
 * 空操作协程是一个特殊协程，永远不会完成，调用resume无效果。
 * 可用于需要协程句柄但不需要实际协程的场景。
 */
inline noop_coroutine_handle noop_coroutine() noexcept {
    return noop_coroutine_handle();
}


/**
 * @struct suspend_always
 * @brief 始终暂停的等待器
 *
 * await_ready总是返回false，表示协程总是在此暂停点暂停。
 * 适用于需要外部恢复的异步操作。
 */
struct suspend_always {
    constexpr bool await_ready() const noexcept { return false; }
    constexpr void await_suspend(coroutine_handle<>) const noexcept {}
    constexpr void await_resume() const noexcept {}
};

/**
 * @struct suspend_never
 * @brief 从不暂停的等待器
 *
 * await_ready总是返回true，表示协程永远不会在此暂停点暂停。
 * 适用于同步操作或不需要暂停的场景。
 */
struct suspend_never {
    constexpr bool await_ready() const noexcept { return true; }
    constexpr void await_suspend(coroutine_handle<>) const noexcept {}
    constexpr void await_resume() const noexcept {}
};

/** @} */ // Coroutine

MSTL_END_NAMESPACE__
#endif
#endif // MSTL_CORE_ASYNC_COROUTINE_HPP__
