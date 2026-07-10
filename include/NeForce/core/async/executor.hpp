#ifndef NEFORCE_CORE_ASYNC_EXECUTOR_HPP__
#define NEFORCE_CORE_ASYNC_EXECUTOR_HPP__

/**
 * @file executor.hpp
 * @brief 多态执行器抽象
 *
 * 类型擦除的执行器包装，可持有 io_context::executor、strand、
 * thread_pool 或任何满足 executor 概念的类型。
 */

#include "NeForce/core/functional/function.hpp"
#include "NeForce/core/memory/shared_ptr.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup AsyncComponents 异步组件
 * @brief 异步编程相关组件
 * @{
 */

/**
 * @defgroup Executor 执行器
 * @brief 多态执行器抽象
 * @{
 */

/**
 * @class executor
 * @brief 多态执行器
 *
 * 类型擦除的执行器句柄，可包装任意满足 executor 概念的类型。
 * 任意拥有一个可调用 execute(handler_type) 方法的类型都可以
 * 通过模板构造函数包装为 executor。
 */
class executor {
public:
    /// @brief handler 类型
    using handler_type = function<void()>;

    /**
     * @brief 执行器概念基类
     */
    struct executor_base {
        virtual ~executor_base() = default;
        virtual void execute(handler_type handler) = 0;
    };

private:
    shared_ptr<executor_base> exec_;

public:
    executor() = default;

    /**
     * @brief 从任意满足 executor 概念的类型构造
     * @tparam Exec 包装的执行器类型
     * @param exec 要包装的执行器实例
     */
    template <typename Exec, enable_if_t<!is_same_v<decay_t<Exec>, executor>, int> = 0>
    executor(Exec exec) {
        struct wrapper final : executor_base {
            Exec exec_;
            explicit wrapper(Exec e) :
            exec_(move(e)) {}
            void execute(handler_type handler) override { exec_.execute(move(handler)); }
        };
        exec_ = _NEFORCE make_shared<wrapper>(_NEFORCE move(exec));
    }

    /**
     * @brief 投递 handler 到执行器
     * @param handler 要执行的 handler
     */
    void execute(handler_type handler) const {
        if (exec_) {
            exec_->execute(move(handler));
        }
    }

    /**
     * @brief 投递 handler 到执行器
     * @param handler 要执行的 handler
     */
    void post(handler_type handler) const { execute(move(handler)); }

    /**
     * @brief 检查执行器是否有效
     */
    explicit operator bool() const noexcept { return static_cast<bool>(exec_); }

    /**
     * @brief 相等比较
     */
    NEFORCE_NODISCARD bool operator==(const executor& other) const noexcept { return exec_ == other.exec_; }

    /**
     * @brief 不等比较
     */
    NEFORCE_NODISCARD bool operator!=(const executor& other) const noexcept { return exec_ != other.exec_; }
};

/**
 * @brief 向执行器投递 handler
 * @tparam Executor 执行器类型
 * @param ex 执行器
 * @param handler 要执行的 handler
 *
 * 统一的 post 语义，handler 在 ex 的上下文中异步执行。
 */
template <typename Executor>
void post(Executor& ex, function<void()> handler) {
    ex.execute(move(handler));
}

/** @} */ // Executor

/** @} */ // AsyncComponents

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_EXECUTOR_HPP__
