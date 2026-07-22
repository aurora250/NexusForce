#ifndef NEFORCE_TUI_ELEMENT_HPP__
#define NEFORCE_TUI_ELEMENT_HPP__

/**
 * @file element.hpp
 * @brief 虚拟UI元素
 *
 * 定义了虚拟元素树节点类型和声明式 DSL 工厂方法。
 * Element 是纯值类型，不持有任何终端资源。
 */

#include "NeForce/core/functional/function.hpp"
#include "NeForce/core/memory/shared_ptr.hpp"
#include "NeForce/tui/style.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/// @cond
template <typename T>
class State;
/// @endcond


/**
 * @brief 元素类型枚举
 */
enum class ElementKind : uint8_t {
    Empty,      ///< 空元素
    VBox,       ///< 垂直布局容器
    HBox,       ///< 水平布局容器
    ZStack,     ///< 层叠容器
    Text,       ///< 文本节点
    Spacer,     ///< 弹性占位
    Separator,  ///< 分割线
    Button,     ///< 按钮
    TextInput,  ///< 文本输入框
    Checkbox,   ///< 复选框
    ScrollView, ///< 滚动区域
    Canvas,     ///< 自定义绘制
    When,       ///< 条件渲染
    Each,       ///< 列表渲染
};

/**
 * @brief 虚拟UI元素
 *
 * 纯值类型，描述 UI 树的一个节点。每次 render() 重建整个元素树，
 * Reconciler 通过 diff 新旧树生成最小 ANSI 更新。
 *
 * @note 列表元素应通过 withKey() 分配 key 以帮助 Reconciler 追踪身份。
 */
class Element {
public:
    Element();
    ~Element();

    Element(const Element& other);
    Element& operator=(const Element& other);
    Element(Element&& other) noexcept;
    Element& operator=(Element&& other) noexcept;

    /**
     * @brief 分配列表 key，帮助 Reconciler 追踪元素身份
     * @param k 唯一标识
     * @return 自身引用（链式调用）
     */
    Element& withKey(size_t k);

    /** @name 内部访问器（Reconciler / Layout 使用） */
    ///@{
    NEFORCE_NODISCARD ElementKind kind() const noexcept;
    NEFORCE_NODISCARD size_t key() const noexcept;
    NEFORCE_NODISCARD const vector<Element>& children() const;
    NEFORCE_NODISCARD const Style& style() const;
    NEFORCE_NODISCARD const BoxProps& layout() const;
    NEFORCE_NODISCARD const string& text() const;
    NEFORCE_NODISCARD const function<void()>& onClick() const;
    NEFORCE_NODISCARD int flex() const noexcept;
    NEFORCE_NODISCARD void* stateRef() const noexcept;
    NEFORCE_NODISCARD Variant variant() const noexcept;
    NEFORCE_NODISCARD const function<void(int, int, int, int)>& drawFn() const;
    NEFORCE_NODISCARD void* owner() const noexcept;
    void setOwner(void* o);
    NEFORCE_NODISCARD int scrollX() const noexcept;
    NEFORCE_NODISCARD int scrollY() const noexcept;
    ///@}

    static Element empty();
    static Element vbox(vector<Element> children, BoxProps props = {});
    static Element hbox(vector<Element> children, BoxProps props = {});
    static Element zstack(vector<Element> children);
    static Element text(string content, Style style = {});
    static Element spacer(int flex = 1);
    static Element separator();
    static Element button(string label, function<void()> onClick, Style style = {}, Variant variant = Variant::Default);
    static Element textInput(State<string>& text, Style style = {});
    static Element checkbox(string label, State<bool>& checked, Style style = {});
    static Element scrollView(Element child, Style style = {}, int scrollX = 0, int scrollY = 0);

    /// @brief 自定义绘制区域
    static Element canvas(function<void(int, int, int, int)> drawFn, Style style = {});

    /**
     * @brief 条件渲染
     * @param cond 条件
     * @param then 条件为真时的渲染函数
     * @param otherwise 条件为假时的渲染函数
     * @return When 元素
     */
    static Element
    when(bool cond, function<Element()> then, function<Element()> otherwise = []() { return Element::empty(); });

    /**
     * @brief 列表渲染
     * @tparam T 列表元素类型
     * @param items 数据列表
     * @param render 渲染回调，接收 (item引用, index)
     * @return Each 元素
     * @note 列表元素应通过 withKey() 分配唯一 key。
     */
    template <typename T>
    static Element each(const vector<T>& items, function<Element(const T&, size_t)> render) {
        Element el;
        el.initKind(ElementKind::Each);
        el.reserveChildren(items.size());
        for (size_t i = 0; i < items.size(); ++i) {
            el.addChild(render(items[i], i));
        }
        return el;
    }

private:
    struct Node;
    shared_ptr<Node> node_;

    void ensureNode();
    void initKind(ElementKind k);
    void reserveChildren(size_t n);
    void addChild(Element child);
};

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_ELEMENT_HPP__
