#ifndef NEFORCE_CORE_ASYNC_STRAND_HPP__
#define NEFORCE_CORE_ASYNC_STRAND_HPP__

/**
 * @file strand.hpp
 * @brief 串行化执行器
 *
 * strand 保证投递到它的 handler 严格串行执行。
 */

#include "NeForce/core/async/atomic.hpp"
#include "NeForce/core/async/io_context.hpp"
#include "NeForce/core/async/lock_free_queue.hpp"
#include "NeForce/core/exception/exception_ptr.hpp"
#include "NeForce/core/functional/function.hpp"
#include "NeForce/core/memory/shared_ptr.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Strand 串行化执行器
 * @brief 无锁串行执行
 * @{
 */

/**
 * @class strand
 * @brief 串行化执行器
 */
class strand {
public:
    /// @brief handler 类型
    using handler_type = function<void()>;

private:
    struct impl {
        io_context* ctx;
        atomic<bool> locked_{false};
        lock_free_queue<function<void()>> queue_;

        explicit impl(io_context& c) :
        ctx(&c) {}

        void drain_one(const shared_ptr<impl>& self) {
            bool expected = false;
            if (!self->locked_.compare_exchange_strong(expected, true, memory_order_acquire, memory_order_relaxed)) {
                return;
            }
            const auto handler = self->queue_.try_pop();
            if (handler) {
                auto shared_handler = make_shared<function<void()>>(move(*handler));
                self->ctx->post([self, shared_handler]() {
                    try {
                        (*shared_handler)();
                        // NOLINTNEXTLINE(bugprone-empty-catch)
                    } catch (const exception& e) {
                        NEFORCE_REPORT_EXCEPTION(e);
                        // NOLINTNEXTLINE(bugprone-empty-catch)
                    } catch (...) {
                        NEFORCE_REPORT_EXCEPTION(value_exception("unknown exception in strand handler"));
                    }
                    self->locked_.store(false, memory_order_release);
                    self->drain_one(self);
                });
            } else {
                self->locked_.store(false, memory_order_release);
            }
        }
    };

    shared_ptr<impl> impl_;

public:
    /**
     * @brief 构造 strand
     * @param ctx 关联的 io_context
     *
     * strand 绑定到指定的 io_context。
     */
    explicit strand(io_context& ctx) :
    impl_(make_shared<impl>(ctx)) {}

    /**
     * @brief 投递到队列末尾
     * @param handler 要执行的 handler
     */
    void post(handler_type handler) {
        impl_->queue_.push(move(handler));
        impl_->drain_one(impl_);
    }

    /**
     * @brief 投递到队列末尾
     * @param handler 要执行的 handler
     */
    void execute(handler_type handler) { post(move(handler)); }

    /**
     * @brief 尝试同步执行
     * @param handler 要执行的 handler
     *
     * 若当前没有其他 handler 在执行，则立即执行 handler；否则投递到队列等待执行。
     */
    void dispatch(handler_type handler) {
        bool expected = false;
        if (!impl_->locked_.compare_exchange_strong(expected, true, memory_order_acquire, memory_order_relaxed)) {
            post(move(handler));
            return;
        }

        exception_ptr failure;
        try {
            handler();
        } catch (const exception& e) {
            NEFORCE_REPORT_EXCEPTION(e);
            failure = current_exception();
        } catch (...) {
            failure = current_exception();
        }

        impl_->locked_.store(false, memory_order_release);
        impl_->drain_one(impl_);

        if (failure) {
            rethrow_exception(failure);
        }
    }

    /**
     * @brief 获取关联的 io_context 执行器
     * @return io_context::executor 句柄
     */
    NEFORCE_NODISCARD io_context::executor get_executor() const noexcept { return impl_->ctx->get_executor(); }

    /**
     * @brief 获取关联的 io_context 引用
     * @return io_context 引用
     */
    NEFORCE_NODISCARD io_context& context() const noexcept { return *impl_->ctx; }
};

/** @} */ // Strand

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_STRAND_HPP__
