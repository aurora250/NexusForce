#ifndef NEFORCE_CORE_ASYNC_CANCELLATION_SLOT_HPP__
#define NEFORCE_CORE_ASYNC_CANCELLATION_SLOT_HPP__

/**
 * @file cancellation_slot.hpp
 * @brief 异步操作取消槽
 *
 * cancellation_slot 将 stop_token 集成到异步操作中。
 * 当 stop_source::request_stop() 被调用时，关联的异步操作的 fd 注册将被清理，
 * handler 被调用，error_code = operation_aborted。
 */

#include "NeForce/core/async/stop_token.hpp"
#include "NeForce/core/exception/error_code.hpp"
#include "NeForce/core/functional/function.hpp"
#include "NeForce/core/memory/shared_ptr.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Cancellation 异步取消
 * @brief 异步操作取消支持
 * @{
 */

/// @brief 操作已取消的错误码
inline error_code make_operation_aborted() noexcept {
    return error_code(static_cast<int>(errc::operation_canceled), error_category::system());
}

/**
 * @class cancellation_slot
 * @brief 异步操作取消槽
 *
 * 每个异步操作可以绑定一个 cancellation_slot。
 * 当关联的 stop_source 请求停止时，取消回调被触发，
 * 操作会被清理并且 handler 以 operation_aborted 错误码调用。
 */
class cancellation_slot {
private:
    stop_token token_;          ///< 停止令牌
    shared_ptr<void> callback_; ///< 取消回调

public:
    /// @brief 取消回调类型
    using cancel_handler = function<void()>;

    /**
     * @brief 默认构造
     */
    cancellation_slot() = default;

    /**
     * @brief 从 stop_token 构造
     * @param token 停止令牌
     */
    explicit cancellation_slot(stop_token token) :
    token_(move(token)) {}

    /**
     * @brief 检查是否已请求取消
     */
    NEFORCE_NODISCARD bool is_cancelled() const noexcept { return token_.stop_requested(); }

    /**
     * @brief 注册取消回调
     * @tparam Handler 回调类型
     * @param handler 取消时执行的回调
     * @return 是否注册成功（已取消则返回 false 且不注册）
     *
     * @note 若注册时已取消，handler 会被立即调用
     */
    template <typename Handler>
    bool assign(Handler&& handler) {
        if (is_cancelled()) {
            forward<Handler>(handler)();
            return false;
        }
        if (!token_.stop_possible()) {
            return false;
        }
        callback_ = make_shared<stop_callback<Handler>>(token_, forward<Handler>(handler));
        return true;
    }

    /**
     * @brief 获取关联的 stop_token
     */
    NEFORCE_NODISCARD const stop_token& get_token() const noexcept { return token_; }

    /**
     * @brief 检查是否有取消能力
     */
    NEFORCE_NODISCARD bool has_slot() const noexcept { return token_.stop_possible(); }
};

/** @} */ // Cancellation

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_CANCELLATION_SLOT_HPP__
