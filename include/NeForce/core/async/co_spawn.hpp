#ifndef NEFORCE_CORE_ASYNC_CO_SPAWN_HPP__
#define NEFORCE_CORE_ASYNC_CO_SPAWN_HPP__

/**
 * @file co_spawn.hpp
 * @brief 协程启动器
 *
 * co_spawn 在指定的 executor 上启动协程。
 * 依赖于 C++20 协程支持。
 */

#ifdef NEFORCE_STANDARD_20
#    include "NeForce/core/async/coroutine.hpp"
#    include "NeForce/core/exception/exception_ptr.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup CoroutineSpawn 协程启动
 * @brief 在 executor 上启动协程
 * @{
 */

/**
 * @struct co_spawn_task
 * @brief co_spawn 使用的协程任务类型
 *
 * initial_suspend 将协程恢复投递到 executor。
 */
struct co_spawn_task {
    struct promise_type {
        exception_ptr exception_;

        co_spawn_task get_return_object() { return co_spawn_task{coroutine_handle<promise_type>::from_promise(*this)}; }
        suspend_always initial_suspend() noexcept { return {}; }
        auto final_suspend() noexcept {
            struct final_awaiter {
                bool await_ready() noexcept { return false; }
                void await_suspend(coroutine_handle<promise_type> h) noexcept { h.destroy(); }
                void await_resume() noexcept {}
            };
            return final_awaiter{};
        }
        void return_void() noexcept {}
        void unhandled_exception() { exception_ = current_exception(); }

        template <typename U>
        auto await_transform(U&& u) {
            return forward<U>(u);
        }
    };

    coroutine_handle<promise_type> handle_;

    explicit co_spawn_task(coroutine_handle<promise_type> h) :
    handle_(h) {}
};

/**
 * @brief 在 executor 上启动协程
 * @tparam Executor 执行器类型
 * @tparam Func 返回 awaitable 的可调用对象
 * @param exec 执行器（如 io_context::executor 或 strand）
 * @param func 协程工厂函数，返回 awaitable
 *
 * @code
 * co_spawn(ctx.get_executor(), [&]() -> awaitable<void> {
 *     auto n = co_await sock.async_receive(buf, use_awaitable);
 *     println("read ", n, " bytes");
 * });
 * @endcode
 */
template <typename Executor, typename Func>
void co_spawn(Executor&& exec, Func func) {
    auto inner = [](Func f) -> co_spawn_task { co_await f(); };
    auto task = inner(func);
    exec.execute([h = task.handle_]() mutable { h.resume(); });
}

/** @} */ // CoroutineSpawn

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_STANDARD_20
#endif // NEFORCE_CORE_ASYNC_CO_SPAWN_HPP__
