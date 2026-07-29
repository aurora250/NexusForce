#ifndef NEFORCE_TUI_DOM_STATE_HPP__
#define NEFORCE_TUI_DOM_STATE_HPP__

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
class component_base;
/// @endcond


/**
 * @brief 响应式状态变量
 * @tparam T 值类型
 *
 * 写入时自动通过 strand 调度组件重渲染，
 * 支持读取、整值替换、就地修改和批量静默更新四种模式。
 *
 * @note 必须在 component::create_state() 中创建，通过组件生命周期管理。
 */
template <typename T>
class state {
public:
    /// @brief 值变化信号类型
    using changed_signal = signal<T>;

    /**
     * @brief 构造函数
     * @param owner 所属组件
     * @param s 串行执行器引用
     * @param initial 初始值
     */
    state(component_base* owner, strand& s, T initial) :
    value_(_NEFORCE move(initial)),
    strand_(&s),
    owner_(owner) {}

    ~state() = default;

    state(const state&) = delete;
    state& operator=(const state&) = delete;
    state(state&&) = delete;
    state& operator=(state&&) = delete;

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
    state& operator=(T v) {
        value_ = _NEFORCE move(v);
        notify_changed();
        return *this;
    }

    /**
     * @brief 就地修改
     * @param fn 修改函数，接收 T& 引用
     */
    void modify(function<void(T&)> fn) {
        fn(value_);
        notify_changed();
    }

    /**
     * @brief 静默写入
     * @param v 新值
     *
     * 用于批量操作场景，最后手动调用 notify()。
     */
    void set_quiet(T v) {
        value_ = _NEFORCE move(v);
        on_changed_.emit(value_);
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
            schedule_render();
        }
    }

    /**
     * @brief 订阅值变化通知
     * @param fn 回调函数，接收新值常量引用
     * @return 连接句柄
     */
    connection on_change(function<void(const T&)> fn) {
        return on_changed_.connect([fn = _NEFORCE move(fn)](const T& val) { fn(val); });
    }

private:
    T value_;
    changed_signal on_changed_;
    strand* strand_;
    component_base* owner_;
    bool dirty_ = false;

    void notify_changed() {
        on_changed_.emit(value_);
        schedule_render();
    }

    void schedule_render();
};

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_DOM_STATE_HPP__
