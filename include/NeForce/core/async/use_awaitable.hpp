#ifndef NEFORCE_CORE_ASYNC_USE_AWAITABLE_HPP__
#define NEFORCE_CORE_ASYNC_USE_AWAITABLE_HPP__

/**
 * @file use_awaitable.hpp
 * @brief 协程 awaitable 完成令牌
 *
 * use_awaitable 令牌使 async_xxx 操作可以被 co_await。
 * 依赖于 C++20 协程支持。
 */

#include "NeForce/core/async/async_result.hpp"
#ifdef NEFORCE_STANDARD_20
#    include "NeForce/core/async/coroutine.hpp"
#    include "NeForce/core/exception/exception_ptr.hpp"
#    include "NeForce/core/utility/tuple.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup CompletionTokens 完成令牌
 * @{
 */

/// @brief 协程 awaitable 完成令牌
struct use_awaitable_t {};
/// @brief use_awaitable 常量
constexpr use_awaitable_t use_awaitable{};

/**
 * @class awaitable
 * @brief 可被 co_await 的异步操作结果
 * @tparam Args 异步操作的参数类型
 *
 * 包装了一个 async_result 的 handler，使其可以作为协程的 awaiter。
 * 当异步操作完成时，恢复挂起的协程。
 */
template <typename... Args>
class awaitable {
public:
    awaitable() = default;

    /**
     * @brief 设置完成值
     * @param args 异步操作的结果参数
     */
    void set_value(Args... args) {
        data_ = _NEFORCE make_tuple(_NEFORCE move(args)...);
        ready_ = true;
        if (continuation_) {
            continuation_.resume();
        }
    }

    /**
     * @brief 设置异常
     */
    void set_exception(exception_ptr e) {
        exception_ = move(e);
        ready_ = true;
        if (continuation_) {
            continuation_.resume();
        }
    }

    NEFORCE_NODISCARD bool await_ready() const noexcept { return ready_; }

    void await_suspend(coroutine_handle<> h) noexcept { continuation_ = h; }

    auto await_resume() {
        if (exception_) {
            rethrow_exception(exception_);
        }
        if constexpr (sizeof...(Args) == 1) {
            return _NEFORCE move(_NEFORCE get<0>(data_));
        } else if constexpr (sizeof...(Args) == 0) {
            return;
        } else {
            return _NEFORCE move(data_);
        }
    }

private:
    tuple<Args...> data_{};
    bool ready_{false};
    exception_ptr exception_{nullptr};
    coroutine_handle<> continuation_{nullptr};
};


NEFORCE_BEGIN_INNER__

template <typename... Args>
struct awaitable_handler {
    shared_ptr<awaitable<Args...>> awaitable_;

    void operator()(Args... args) { awaitable_->set_value(_NEFORCE move(args)...); }
};

NEFORCE_END_INNER__

template <typename... Args>
struct async_result<use_awaitable_t, void(Args...)> {
    using handler_type = inner::awaitable_handler<Args...>;
    using return_type = awaitable<Args...>;

    shared_ptr<return_type> awaitable_;
    handler_type handler_;

    explicit async_result(use_awaitable_t /*unused*/) {
        awaitable_ = make_shared<return_type>();
        handler_.awaitable_ = awaitable_;
    }

    handler_type get_handler() { return handler_; }

    return_type get() { return _NEFORCE move(*awaitable_); }
};

/**
 * @brief awaitable 的 void 特化
 *
 * 用于返回类型为 void 的异步操作（如 async_handshake、async_connect）。
 * 同时支持作为协程返回类型——lambda 中可使用 co_await 构建异步链，
 * 协程完成时自动触发 awaitable::set_value()。
 */
template <>
class awaitable<void> {
public:
    struct promise_type;

private:
    struct state {
        bool ready_{false};
        exception_ptr exception_{nullptr};
        coroutine_handle<> continuation_{nullptr};
    };
    shared_ptr<state> state_;

    explicit awaitable(shared_ptr<state> s) :
    state_(move(s)) {}

public:
    /**
     * @brief 默认构造函数
     *
     * 构造一个独立的、未就绪的 awaitable。
     */
    awaitable() :
    state_(make_shared<state>()) {}

    /**
     * @brief 标记完成
     */
    void set_value() {
        state_->ready_ = true;
        if (state_->continuation_) {
            state_->continuation_.resume();
        }
    }

    /**
     * @brief 设置异常
     */
    void set_exception(exception_ptr e) {
        state_->exception_ = move(e);
        state_->ready_ = true;
        if (state_->continuation_) {
            state_->continuation_.resume();
        }
    }

    NEFORCE_NODISCARD bool await_ready() const noexcept { return state_->ready_; }
    void await_suspend(coroutine_handle<> h) noexcept { state_->continuation_ = h; }

    void await_resume() {
        if (state_->exception_) {
            rethrow_exception(state_->exception_);
        }
    }
};

struct awaitable<void>::promise_type {
    shared_ptr<awaitable<void>::state> state_;

    promise_type() :
    state_(make_shared<awaitable<void>::state>()) {}

    awaitable<void> get_return_object() { return awaitable<void>(state_); }
    suspend_never initial_suspend() noexcept { return {}; }

    auto final_suspend() noexcept {
        struct awaiter {
            shared_ptr<awaitable<void>::state> state;
            NEFORCE_NODISCARD bool await_ready() const noexcept { return false; }
            void await_suspend(coroutine_handle<> h) noexcept {
                state->ready_ = true;
                if (state->continuation_) {
                    state->continuation_.resume();
                }
                h.destroy();
            }
            void await_resume() noexcept {}
        };
        return awaiter{state_};
    }

    void return_void() noexcept {}
    void unhandled_exception() {
        state_->exception_ = current_exception();
        state_->ready_ = true;
    }
};

/** @} */ // CompletionTokens

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_STANDARD_20
#endif // NEFORCE_CORE_ASYNC_USE_AWAITABLE_HPP__
