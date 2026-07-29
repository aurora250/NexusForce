#ifndef NEFORCE_TUI_COMPONENT_COMPONENT_HPP__
#define NEFORCE_TUI_COMPONENT_COMPONENT_HPP__

/**
 * @file component.hpp
 * @brief TUI组件基类
 *
 * 定义了组件基类 component_base 和模板类 Component<P>。
 * 组件是 TUI 框架的核心抽象，每个 UI 单元都是组件，
 * 通过 render() 返回声明式元素树。
 */

#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/memory/unique_ptr.hpp"
#include "NeForce/tui/dom/element.hpp"
#include "NeForce/tui/dom/state.hpp"
#include "NeForce/tui/events.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 空属性标签
 *
 * 用于不需要属性的组件的 Props 类型参数。
 */
struct empty_props {};

/**
 * @brief 组件基类
 */
class component_base {
public:
    virtual ~component_base() = default;

    /**
     * @brief 声明式渲染
     * @returns 虚拟元素树
     *
     * Reconciler 每次 flush 时调用此方法获取最新 UI 描述。
     * 实现必须是纯函数——仅构建 element 树，禁止终端 I/O。
     *
     * @note 禁止在 render() 中修改 State。
     */
    virtual element render() = 0;

    /**
     * @brief 键盘事件处理
     * @param e 键盘事件
     * @return true 表示事件已消费，false 继续向父组件传递
     */
    virtual bool on_key(const key_event& e) {
        ignore = e;
        return false;
    }

    /**
     * @brief 鼠标事件处理
     * @param e 鼠标事件
     * @return true 表示事件已消费
     */
    virtual bool on_mouse(const mouse_event& e) {
        ignore = e;
        return false;
    }

    /**
     * @brief 鼠标离开回调
     *
     * 当鼠标从前一帧的当前组件移动到其他组件时，Reconciler 调用此方法。
     */
    virtual void on_mouse_leave() {}

    /**
     * @brief 设置事件穿透
     * @param v 是否穿透
     *
     * 当设为 true 时，聚焦组件未消费的键盘事件会继续向父组件传递；
     * 默认为 false，未消费的事件将被丢弃。
     */
    void set_pass_through(bool v) noexcept { pass_through_ = v; }

    /**
     * @brief 是否启用事件穿透
     * @return 是否穿透
     */
    NEFORCE_NODISCARD bool pass_through() const noexcept { return pass_through_; }

    /**
     * @brief 是否持有 Reconciler 键盘焦点
     * @return 是否聚焦
     */
    NEFORCE_NODISCARD bool has_focus() const noexcept { return has_focus_; }

    /**
     * @brief 设置 Reconciler 焦点状态（由 Reconciler 调用）
     * @param v 是否聚焦
     */
    void set_has_focus(bool v) noexcept { has_focus_ = v; }

    /**
     * @brief 动画帧回调
     * @param delta 距上一帧的毫秒数
     */
    virtual void on_animation(int64_t delta) { ignore = delta; }

    /**
     * @brief 挂载回调
     *
     * 在首次 render() 之前调用。在此连接 signal、订阅事件、启动异步操作。
     */
    virtual void setup() {}

    /**
     * @brief 卸载回调
     *
     * 在组件被移除前调用。在此断开连接、释放资源。
     */
    virtual void cleanup() noexcept {}

    /**
     * @brief 是否需要更新
     * @return true 表示需要重新 render，false 跳过本次渲染
     *
     * 性能优化钩子。reconciler 在每次 flush 前检查此方法，
     * 返回 false 则复用上次的 element 树。
     */
    virtual bool should_update() { return true; }

    /**
     * @brief 获取父组件
     * @return 父组件指针，根组件返回 nullptr
     */
    NEFORCE_NODISCARD component_base* parent() const noexcept { return parent_; }

    /**
     * @brief 获取子组件数量
     * @return 子组件数
     */
    NEFORCE_NODISCARD size_t child_count() const noexcept { return children_.size(); }

    /**
     * @brief 获取指定索引的子组件
     * @param i 索引
     * @return 子组件指针
     */
    NEFORCE_NODISCARD component_base* child_at(size_t i) const noexcept { return children_[i].get(); }

    /**
     * @brief 获取自身在父组件中的索引
     * @return 索引，无父组件返回 -1
     */
    NEFORCE_NODISCARD int index() const noexcept {
        if (parent_ == nullptr) {
            return -1;
        }
        for (size_t i = 0; i < parent_->children_.size(); ++i) {
            if (parent_->children_[i].get() == this) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    /**
     * @brief 注册子组件
     * @param child 子组件指针
     *
     * 父组件接管子组件生命周期。注册后子组件将参与焦点链遍历和生命周期管理。
     */
    void add_child(unique_ptr<component_base> child) {
        child->parent_ = this;
        child->strand_ = strand_;
        child->ctx_ = ctx_;
        child->schedule_render_cb_ = schedule_render_cb_;
        auto* raw = child.get();
        children_.push_back(move(child));
        raw->setup();
    }

    /**
     * @brief 移除子组件
     * @param child 子组件指针
     * @return 子组件指针，调用者接管其生命周期
     */
    unique_ptr<component_base> remove_child(component_base* child) {
        child->cleanup();
        child->parent_ = nullptr;
        for (auto it = children_.begin(); it != children_.end(); ++it) {
            if (it->get() == child) {
                auto owned = move(*it);
                children_.erase(it);
                return owned;
            }
        }
        return nullptr;
    }

    /**
     * @brief 从父组件中脱离自身
     * @return 自身指针
     */
    NEFORCE_NODISCARD unique_ptr<component_base> detach() {
        if (parent_ != nullptr) {
            return parent_->remove_child(this);
        }
        return nullptr;
    }

    /**
     * @brief 移除所有子组件
     */
    void detach_all_children() {
        while (!children_.empty()) {
            remove_child(children_.back().get());
        }
    }

    /**
     * @brief 是否可获取焦点
     * @return true 表示该组件可接收键盘事件
     */
    NEFORCE_NODISCARD virtual bool focusable() const { return false; }

    /**
     * @brief 获取当前激活的子组件
     * @return 激活的子组件指针
     */
    NEFORCE_NODISCARD component_base* active_child() noexcept { return active_child_; }

    /**
     * @brief 获取当前激活的子组件
     * @return 激活的子组件指针
     */
    NEFORCE_NODISCARD const component_base* active_child() const noexcept { return active_child_; }

    /**
     * @brief 设置激活的子组件
     * @param child 要激活的子组件指针
     */
    void set_active_child(component_base* child) noexcept { active_child_ = child; }

    /**
     * @brief 是否处于激活状态
     * @return 是否活跃
     */
    NEFORCE_NODISCARD bool active() const noexcept { return parent_ == nullptr || parent_->active_child() == this; }

    /**
     * @brief 是否拥有焦点
     * @return 是否已聚焦
     */
    NEFORCE_NODISCARD bool focused() const noexcept {
        if (parent_ == nullptr) {
            return true;
        }
        if (!active()) {
            return false;
        }
        return parent_->focused();
    }

    /**
     * @brief 请求焦点
     */
    virtual void take_focus() {
        auto* focus = this;
        for (auto* p = parent_; p != nullptr; p = p->parent_) {
            p->set_active_child(focus);
            focus = p;
        }
    }

    /**
     * @brief 调度重渲染
     *
     * 由 state<T> 在写入时调用，通知 reconciler。
     */
    void schedule_render() {
        if (schedule_render_cb_) {
            schedule_render_cb_(this);
        }
    }

    /**
     * @brief 设置上下文值
     * @tparam Ctx 上下文类型
     * @param value 上下文值
     */
    template <typename Ctx>
    void provide_context(Ctx value) {
        contexts_[typeid(Ctx).hash_code()] = _NEFORCE make_shared<Ctx>(_NEFORCE move(value));
    }

    /**
     * @brief 获取上下文值
     * @tparam Ctx 上下文类型
     * @return 上下文常量引用
     */
    template <typename Ctx>
    NEFORCE_NODISCARD const Ctx& context() const {
        for (const auto* p = this; p != nullptr; p = p->parent_) {
            auto it = p->contexts_.find(typeid(Ctx).hash_code());
            if (it != p->contexts_.end()) {
                return *static_cast<const Ctx*>(it->second.get());
            }
        }
        NEFORCE_THROW_EXCEPTION(value_exception("Context not found in component tree"));
    }

protected:
    friend class reconciler;
    friend class focus_manager;
    friend class application;

    component_base* parent_ = nullptr;            ///< 父组件指针
    component_base* active_child_ = nullptr;      ///< 当前激活的子组件
    bool has_focus_ = false;                      ///< reconciler 焦点状态
    bool pass_through_ = false;                   ///< 事件穿透标志
    strand* strand_ = nullptr;                    ///< 串行执行器
    io_context* ctx_ = nullptr;                   ///< 事件循环上下文
    vector<unique_ptr<component_base>> children_; ///< 子组件列表

    virtual void collect_focusable(vector<component_base*>& out) {
        if (focusable()) {
            out.push_back(this);
        }
        for (const auto& child: children_) {
            child->collect_focusable(out);
        }
    }

private:
    function<void(component_base*)> schedule_render_cb_; ///< 由 reconciler 注入的调度回调
    unordered_map<size_t, shared_ptr<void>> contexts_;   ///< 上下文映射
};

/**
 * @brief 组件模板
 * @tparam P 属性类型，父组件向子组件传递的只读参数
 *
 * 所有 TUI 组件应继承此类并实现 render()。
 */
template <typename P = empty_props>
class component : public component_base {
public:
    using props_type = P;

    component() = default;
    ~component() override = default;
    component(const component&) = delete;
    component& operator=(const component&) = delete;
    component(component&&) = delete;
    component& operator=(component&&) = delete;

protected:
    /**
     * @brief 获取父组件传入的属性
     * @return 属性常量引用
     */
    NEFORCE_NODISCARD const props_type& props() const noexcept { return props_; }

    /**
     * @brief 创建响应式状态变量
     * @tparam T 值类型
     * @param initial 初始值
     * @return 状态变量引用
     *
     * 状态变量生命周期绑定到组件。
     *
     * @note 必须在构造函数或 setup() 中调用，禁止在 render() 中动态创建。
     */
    template <typename T>
    state<T>& create_state(T initial) {
        auto s = _NEFORCE make_shared<state<T>>(this, *strand_, _NEFORCE move(initial));
        states_.push_back(s);
        return *s;
    }

    /**
     * @brief 注入属性
     * @param p 属性值
     */
    void set_props(props_type p) { props_ = _NEFORCE move(p); }

private:
    friend class reconciler;
    friend class focus_manager;
    friend class application;

    props_type props_;
    vector<shared_ptr<void>> states_; ///< 类型擦除的状态列表
};


/// @cond
template <typename T>
void state<T>::schedule_render() {
    if (owner_ != nullptr) {
        owner_->schedule_render();
    }
}
/// @endcond

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_COMPONENT_COMPONENT_HPP__
