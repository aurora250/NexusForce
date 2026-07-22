#ifndef NEFORCE_TUI_STATE_HPP__
#define NEFORCE_TUI_STATE_HPP__

/**
 * @file state.hpp
 * @brief 响应式状态变量
 *
 * 提供响应式状态模板类，写入状态时自动通过 strand 调度组件重渲染。
 */

#include "NeForce/core/async/signals.hpp"
#include "NeForce/core/async/strand.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/// @cond
class ComponentBase;
/// @endcond


/**
 * @brief 响应式状态变量
 * @tparam T 值类型
 *
 * 写入时自动通过 strand 调度组件重渲染，
 * 支持读取、整值替换、就地修改和批量静默更新四种模式。
 *
 * @note 必须在 Component::createState() 中创建，通过组件生命周期管理。
 */
template <typename T>
class State {
public:
    /// @brief 值变化信号类型
    using ChangedSignal = signal<T>;

    /**
     * @brief 构造函数
     * @param owner 所属组件
     * @param s 串行执行器引用
     * @param initial 初始值
     */
    State(ComponentBase* owner, strand& s, T initial) :
    value_(_NEFORCE move(initial)),
    strand_(&s),
    owner_(owner) {}

    ~State() = default;

    State(const State&) = delete;
    State& operator=(const State&) = delete;
    State(State&&) = delete;
    State& operator=(State&&) = delete;

    /**
     * @brief 读取当前值
     * @return 常量引用
     */
    NEFORCE_NODISCARD const T& value() const noexcept { return value_; }

    /**
     * @brief 读取当前值
     * @return 常量引用
     */
    NEFORCE_NODISCARD const T& operator*() const noexcept { return value_; }

    /**
     * @brief 成员访问
     * @return 常量指针
     */
    NEFORCE_NODISCARD const T* operator->() const noexcept { return &value_; }

    /**
     * @brief 整值替换
     * @param v 新值
     * @return 自身引用
     */
    State& operator=(T v) {
        value_ = _NEFORCE move(v);
        notifyChanged();
        return *this;
    }

    /**
     * @brief 就地修改
     * @param fn 修改函数，接收 T& 引用
     */
    void modify(function<void(T&)> fn) {
        fn(value_);
        notifyChanged();
    }

    /**
     * @brief 静默写入
     * @param v 新值
     *
     * 用于批量操作场景，最后手动调用 notify()。
     */
    void setQuiet(T v) {
        value_ = _NEFORCE move(v);
        onChanged_.emit(value_);
        dirty_ = true;
    }

    /**
     * @brief 手动触发重渲染
     *
     * 配合 setQuiet() 使用，在批量操作结束后调用一次。
     */
    void notify() {
        if (dirty_) {
            dirty_ = false;
            scheduleRender();
        }
    }

    /**
     * @brief 订阅值变化通知
     * @param fn 回调函数，接收新值常量引用
     * @return 连接句柄
     */
    connection onChange(function<void(const T&)> fn) {
        return onChanged_.connect([fn = _NEFORCE move(fn)](const T& val) { fn(val); });
    }

private:
    T value_;
    ChangedSignal onChanged_;
    strand* strand_;
    ComponentBase* owner_;
    bool dirty_ = false;

    void notifyChanged() {
        onChanged_.emit(value_);
        scheduleRender();
    }

    void scheduleRender();
};

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_STATE_HPP__
