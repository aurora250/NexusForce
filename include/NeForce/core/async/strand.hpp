#ifndef NEFORCE_CORE_ASYNC_STRAND_HPP__
#define NEFORCE_CORE_ASYNC_STRAND_HPP__

/**
 * @file strand.hpp
 * @brief 串行化执行器
 *
 * strand 保证投递到它的 handler 严格串行执行，同一时刻只有一个 handler 在运行，其余排队。
 */

#include "NeForce/core/async/atomic.hpp"
#include "NeForce/core/async/io_context.hpp"
#include "NeForce/core/async/lock_free_queue.hpp"
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
 *
 * 所有投递到 strand 的 handler 严格串行执行，无需显式加锁。
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
            auto handler = self->queue_.try_pop();
            if (handler) {
                auto shared_handler = make_shared<function<void()>>(move(*handler));
                self->ctx->post([self, shared_handler]() {
                    (*shared_handler)();
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
     * @brief 投递 handler 到 strand 队列末尾
     * @param handler 要执行的 handler
     *
     * handler 保证与 strand 中其他 handler 串行执行。
     */
    void post(handler_type handler) {
        impl_->queue_.push(move(handler));
        impl_->drain_one(impl_);
    }

    /**
     * @brief 尽可能在当前线程同步执行 handler
     * @param handler 要执行的 handler
     *
     * 若当前没有其他 handler 在执行（strand 未锁定），则在调用线程立即执行 handler。
     * 否则降级为 post()，投递到队列等待串行执行。
     */
    void dispatch(handler_type handler) {
        bool expected = false;
        if (impl_->locked_.compare_exchange_strong(expected, true, memory_order_acquire, memory_order_relaxed)) {
            handler();
            impl_->locked_.store(false, memory_order_release);
            impl_->drain_one(impl_);
        } else {
            post(move(handler));
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
