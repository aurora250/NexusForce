#ifndef NEFORCE_CORE_ASYNC_ASYNC_RESULT_HPP__
#define NEFORCE_CORE_ASYNC_ASYNC_RESULT_HPP__

/**
 * @file async_result.hpp
 * @brief 完成令牌模型
 *
 * 完成令牌允许同一 async_xxx 函数根据 token 参数返回不同结果类型。
 *
 *  - 默认（callable）：void，handler 即回调
 *  - use_future：返回 future<Result>
 *  - detached：fire-and-forget，void
 *  - deferred：延迟执行，void
 */

#include "NeForce/core/async/promise.hpp"
#include "NeForce/core/exception/exception_ptr.hpp"
#include "NeForce/core/exception/system_exception.hpp"
#include "NeForce/core/functional/function.hpp"
#include "NeForce/core/memory/shared_ptr.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup CompletionTokens 完成令牌
 * @brief 异步操作完成令牌
 * @{
 */

/// @brief 返回 future<T> 的完成令牌
struct use_future_t {};
/// @brief fire-and-forget 完成令牌
struct detached_t {};
/// @brief 延迟执行完成令牌
struct deferred_t {};

/// @brief use_future 常量
NEFORCE_INLINE17 constexpr use_future_t use_future{};
/// @brief detached 常量
NEFORCE_INLINE17 constexpr detached_t detached{};
/// @brief deferred 常量
NEFORCE_INLINE17 constexpr deferred_t deferred{};


/**
 * @class async_result
 * @brief 完成令牌 trait 主模板
 * @tparam Token 完成令牌类型
 * @tparam Signature 异步操作签名（如 void(error_code, size_t)）
 *
 * 默认特化：Token 本身即是 handler callable。
 * 其他特化通过 get_handler() 返回适配的 handler，get() 返回令牌对应的结果对象。
 */
template <typename Token, typename Signature>
struct async_result {
    using handler_type = Token; ///< handler 类型
    using return_type = void;   ///< 返回类型

    Token token_; ///< 存储的令牌

    explicit async_result(Token& t) :
    token_(move(t)) {}

    handler_type get_handler() { return move(token_); }
    void get() {}
};

/// @cond
NEFORCE_BEGIN_INNER__

template <typename... Args>
struct future_handler {
    shared_ptr<promise<tuple<Args...>>> promise_;

    void operator()(Args... args) { promise_->set_value(make_tuple(move(args)...)); }
};

template <>
struct future_handler<error_code> {
    shared_ptr<promise<void>> promise_;

    void operator()(error_code ec) {
        if (ec) {
            promise_->set_exception(_NEFORCE make_exception_ptr(system_exception(ec)));
        } else {
            promise_->set_value();
        }
    }
};

template <>
struct future_handler<error_code, size_t> {
    shared_ptr<promise<size_t>> promise_;

    void operator()(error_code ec, size_t n) {
        if (ec) {
            promise_->set_exception(_NEFORCE make_exception_ptr(system_exception(ec)));
        } else {
            promise_->set_value(n);
        }
    }
};

NEFORCE_END_INNER__
/// @endcond

template <>
struct async_result<use_future_t, void(error_code)> {
    using handler_type = inner::future_handler<error_code>;
    using return_type = future<void>;
    handler_type handler_;
    explicit async_result(use_future_t /*unused*/) { handler_.promise_ = make_shared<promise<void>>(); }
    handler_type get_handler() { return handler_; }
    return_type get() { return handler_.promise_->get_future(); }
};

template <>
struct async_result<use_future_t, void(error_code, size_t)> {
    using handler_type = inner::future_handler<error_code, size_t>;
    using return_type = future<size_t>;
    handler_type handler_;
    explicit async_result(use_future_t /*unused*/) { handler_.promise_ = make_shared<promise<size_t>>(); }
    handler_type get_handler() { return handler_; }
    return_type get() { return handler_.promise_->get_future(); }
};

template <typename... Args>
struct async_result<detached_t, void(Args...)> {
    struct detached_handler {
        function<void(Args...)> inner_;

        void operator()(Args... args) {
            try {
                inner_(args...);
                // NOLINTNEXTLINE(bugprone-empty-catch)
            } catch (...) {
                // ignore
            }
        }
    };

    using handler_type = detached_handler;
    using return_type = void;

    explicit async_result(detached_t /*unused*/) {}
    handler_type get_handler() {
        return detached_handler{[](Args...) {}};
    }
    void get() {}
};

/** @} */ // CompletionTokens

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_ASYNC_RESULT_HPP__
