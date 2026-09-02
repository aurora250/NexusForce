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
NEFORCE_INLINE17 constexpr use_awaitable_t use_awaitable{};


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
private:
    struct state {
        tuple<Args...> data{};                    ///< 完成值
        bool ready{false};                        ///< 是否已完成
        exception_ptr exception{nullptr};         ///< 完成异常
        coroutine_handle<> continuation{nullptr}; ///< 挂起的协程句柄
    };

    shared_ptr<state> state_; ///< 共享状态

    explicit awaitable(shared_ptr<state> s) :
    state_(_NEFORCE move(s)) {}

public:
    /**
     * @struct promise_type
     * @brief 协程 promise 类型
     */
    struct promise_type {
        shared_ptr<state> state_; ///< 与协程返回对象共享的状态

        /**
         * @brief 构造函数
         */
        promise_type() :
        state_(_NEFORCE make_shared<state>()) {}

        /**
         * @brief 获取协程返回对象
         */
        awaitable get_return_object() { return awaitable(state_); }

        /**
         * @brief 协程创建后立即执行
         */
        suspend_never initial_suspend() noexcept { return {}; }

        /**
         * @brief 协程结束钩子：标记完成并恢复外部等待者
         */
        auto final_suspend() noexcept {
            struct awaiter {
                shared_ptr<state> state;
                NEFORCE_NODISCARD bool await_ready() const noexcept { return false; }
                void await_suspend(coroutine_handle<> h) noexcept {
                    state->ready = true;
                    if (state->continuation) {
                        state->continuation.resume();
                    }
                    h.destroy();
                }
                void await_resume() noexcept {}
            };
            return awaiter{state_};
        }

        /**
         * @brief co_return 结果存储
         * @param value 协程返回值
         */
        template <typename U>
        void return_value(U&& value) {
            static_assert(sizeof...(Args) == 1, "awaitable with multiple results cannot be a coroutine return type");
            _NEFORCE get<0>(state_->data) = _NEFORCE forward<U>(value);
            state_->ready = true;
            if (state_->continuation) {
                state_->continuation.resume();
            }
        }

        /**
         * @brief 协程异常捕获
         */
        void unhandled_exception() {
            state_->exception = current_exception();
            state_->ready = true;
            if (state_->continuation) {
                state_->continuation.resume();
            }
        }
    };

    /**
     * @brief 默认构造函数
     *
     * 构造一个独立的未就绪 awaitable。
     */
    awaitable() :
    state_(_NEFORCE make_shared<state>()) {}

    /**
     * @brief 设置完成值
     * @param args 异步操作的结果参数
     */
    void set_value(Args... args) {
        state_->data = _NEFORCE make_tuple(_NEFORCE move(args)...);
        state_->ready = true;
        if (state_->continuation) {
            state_->continuation.resume();
        }
    }

    /**
     * @brief 设置异常
     */
    void set_exception(exception_ptr e) {
        state_->exception = _NEFORCE move(e);
        state_->ready = true;
        if (state_->continuation) {
            state_->continuation.resume();
        }
    }

    NEFORCE_NODISCARD bool await_ready() const noexcept { return state_->ready; }

    void await_suspend(coroutine_handle<> h) noexcept { state_->continuation = h; }

    /**
     * @brief 协程恢复时的结果获取
     * @return 单参数返回该参数值，多参数返回 tuple，异常时重抛
     */
    auto await_resume() {
        if (state_->exception) {
            rethrow_exception(state_->exception);
        }
        if constexpr (sizeof...(Args) == 1) {
            return _NEFORCE move(_NEFORCE get<0>(state_->data));
        } else if constexpr (sizeof...(Args) == 0) {
            return;
        } else {
            return _NEFORCE move(state_->data);
        }
    }
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
        awaitable_ = _NEFORCE make_shared<return_type>();
        handler_.awaitable_ = awaitable_;
    }

    handler_type get_handler() { return handler_; }

    return_type get() noexcept { return *awaitable_; }
};

/**
 * @brief awaitable 的 void 特化
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
