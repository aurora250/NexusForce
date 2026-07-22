#ifndef NEFORCE_TUI_COMPONENT_HPP__
#define NEFORCE_TUI_COMPONENT_HPP__

/**
 * @file component.hpp
 * @brief TUI组件基类
 *
 * 定义了组件基类 ComponentBase 和模板类 Component<P>。
 * 组件是 TUI 框架的核心抽象，每个 UI 单元都是组件，
 * 通过 render() 返回声明式元素树。
 */

#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/tui/element.hpp"
#include "NeForce/tui/events.hpp"
#include "NeForce/tui/state.hpp"
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
struct EmptyProps {};

/**
 * @brief 组件基类
 *
 * Reconciler 和 Application 通过此基类操作异质组件实例，
 * 无需知道具体的 Props 类型。
 */
class ComponentBase {
public:
    virtual ~ComponentBase() = default;

    /**
     * @brief 声明式渲染
     * @returns 虚拟元素树
     *
     * Reconciler 每次 flush 时调用此方法获取最新 UI 描述。
     * 实现必须是纯函数——仅构建 Element 树，禁止终端 I/O。
     *
     * @note 禁止在 render() 中修改 State。
     */
    virtual Element render() = 0;

    /**
     * @brief 键盘事件处理
     * @param e 键盘事件
     * @return true 表示事件已消费，false 继续向父组件传递
     */
    virtual bool onKey(const KeyEvent& e) {
        (void) e;
        return false;
    }

    /**
     * @brief 鼠标事件处理
     * @param e 鼠标事件
     * @return true 表示事件已消费
     */
    virtual bool onMouse(const MouseEvent& e) {
        (void) e;
        return false;
    }

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
    virtual void cleanup() {}

    /**
     * @brief 是否需要更新
     * @return true 表示需要重新 render，false 跳过本次渲染
     *
     * 性能优化钩子。Reconciler 在每次 flush 前检查此方法，
     * 返回 false 则复用上次的 Element 树。
     */
    virtual bool shouldUpdate() { return true; }

    /**
     * @brief 是否可获取焦点
     * @return true 表示该组件可接收键盘事件
     */
    NEFORCE_NODISCARD virtual bool isFocusable() const { return false; }

    /**
     * @brief 获取父组件
     * @return 父组件指针，根组件返回 nullptr
     */
    NEFORCE_NODISCARD ComponentBase* parent() const noexcept { return parent_; }

    /**
     * @brief 注册子组件
     * @param child 子组件指针
     *
     * 注册后子组件将参与焦点链遍历和生命周期管理。
     */
    void addChild(ComponentBase* child) {
        child->parent_ = this;
        child->strand_ = strand_;
        child->ctx_ = ctx_;
        child->scheduleRenderCb_ = scheduleRenderCb_;
        children_.push_back(child);
        child->setup();
    }

    /**
     * @brief 移除子组件
     * @param child 子组件指针
     */
    void removeChild(ComponentBase* child) {
        child->cleanup();
        child->parent_ = nullptr;
        const auto it = find(children_.begin(), children_.end(), child);
        if (it != children_.end()) {
            children_.erase(it);
        }
    }

    /**
     * @brief 调度重渲染
     *
     * 由 State<T> 在写入时调用，通过 strand 通知 Reconciler。
     * scheduleRenderCb_ 由 Reconciler::mount() 注入。
     */
    void scheduleRender() {
        if (scheduleRenderCb_) {
            scheduleRenderCb_(this);
        }
    }

    /**
     * @brief 设置上下文值
     * @tparam Ctx 上下文类型
     * @param value 上下文值
     */
    template <typename Ctx>
    void provideContext(Ctx value) {
        contexts_[typeid(Ctx).hash_code()] = make_shared<Ctx>(_NEFORCE move(value));
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
    friend class Reconciler;
    friend class Application;

    ComponentBase* parent_ = nullptr; ///< 父组件指针
    strand* strand_ = nullptr;        ///< 串行执行器
    io_context* ctx_ = nullptr;       ///< 事件循环上下文
    vector<ComponentBase*> children_; ///< 子组件列表

    virtual void collectFocusable(vector<ComponentBase*>& out) {
        if (isFocusable()) {
            out.push_back(this);
        }
        for (auto* child: children_) {
            child->collectFocusable(out);
        }
    }

private:
    friend class Reconciler;

    function<void(ComponentBase*)> scheduleRenderCb_;  ///< 由 Reconciler 注入的调度回调
    unordered_map<size_t, shared_ptr<void>> contexts_; ///< 上下文映射
};

/**
 * @brief 组件模板
 * @tparam P 属性类型，父组件向子组件传递的只读参数
 *
 * 所有 TUI 组件应继承此类并实现 render()。
 * Props 通过 setProps() 注入，在组件生命周期内保持不变。
 */
template <typename P = EmptyProps>
class Component : public ComponentBase {
public:
    using Props = P;

    Component() = default;
    ~Component() override = default;
    Component(const Component&) = delete;
    Component& operator=(const Component&) = delete;
    Component(Component&&) = delete;
    Component& operator=(Component&&) = delete;

protected:
    /**
     * @brief 获取父组件传入的属性
     * @return 属性常量引用
     */
    NEFORCE_NODISCARD const Props& props() const noexcept { return props_; }

    /**
     * @brief 创建响应式状态变量
     * @tparam T 值类型
     * @param initial 初始值
     * @return 状态变量引用
     *
     * 状态变量生命周期绑定到组件——组件析构时自动清理。
     *
     * @note 必须在构造函数或 setup() 中调用，禁止在 render() 中动态创建。
     */
    template <typename T>
    State<T>& createState(T initial) {
        auto s = _NEFORCE make_shared<State<T>>(this, *strand_, _NEFORCE move(initial));
        states_.push_back(s);
        return *s;
    }

    /**
     * @brief 注入属性
     * @param p 属性值
     */
    void setProps(Props p) { props_ = _NEFORCE move(p); }

private:
    friend class Reconciler;
    friend class Application;

    Props props_;
    vector<shared_ptr<void>> states_; ///< 类型擦除的状态列表
};


/// @cond
template <typename T>
void State<T>::scheduleRender() {
    if (owner_ != nullptr) {
        owner_->scheduleRender();
    }
}
/// @endcond

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_COMPONENT_HPP__
