#ifndef NEFORCE_TUI_DOM_ELEMENT_HPP__
#define NEFORCE_TUI_DOM_ELEMENT_HPP__

/**
 * @file element.hpp
 * @brief 虚拟UI元素
 *
 * 定义了虚拟元素树节点类型和声明式 DSL 工厂方法。
 * element 是纯值类型，不持有任何终端资源。
 */

#include "NeForce/core/functional/function.hpp"
#include "NeForce/core/memory/shared_ptr.hpp"
#include "NeForce/tui/dom/layout_types.hpp"
#include "NeForce/tui/dom/style.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/// @cond
template <typename T>
class state;
/// @endcond

/**
 * @brief 富文本片段
 *
 * 一段具有统一样式的文本。多个片段拼接成一行富文本，
 * 使同一行内可以混排不同颜色、粗体、下划线等样式。
 */
struct text_span {
    string content;   ///< 片段文本，可包含 `\n` 表示硬换行
    tui::style style; ///< 片段样式，覆盖富文本元素的基础样式
    string link;      ///< 超链接地址（OSC-8），空表示普通文本
};

/**
 * @brief 虚拟UI元素
 *
 * 纯值类型，描述 UI 树的一个节点。
 *
 * @note 列表元素应通过 with_key() 分配 key 以帮助 reconciler 追踪身份。
 */
class NEFORCE_API element {
public:
    /**
     * @brief 元素类型枚举
     */
    enum class kind : uint8_t {
        empty,        ///< 空元素
        vbox,         ///< 垂直布局容器
        hbox,         ///< 水平布局容器
        zstack,       ///< 层叠容器
        text,         ///< 文本节点
        rich_text,    ///< 富文本节点
        virtual_list, ///< 虚拟列表节点
        spacer,       ///< 弹性占位
        separator,    ///< 分割线
        button,       ///< 按钮
        text_input,   ///< 文本输入框
        checkbox,     ///< 复选框
        scroll_view,  ///< 滚动区域
        canvas,       ///< 自定义绘制
        panel,        ///< 带边框的面板容器
        band,         ///< 背景色带容器
        when,         ///< 条件渲染
        each,         ///< 列表渲染
        flexbox,      ///< CSS-flexbox 弹性容器
        gridbox,      ///< 网格布局容器
    };

    element();
    ~element();

    element(const element& other);
    element& operator=(const element& other);
    element(element&& other) noexcept;
    element& operator=(element&& other) noexcept;

    /**
     * @brief 分配列表 key，帮助 Reconciler 追踪元素身份
     * @param k 唯一标识
     * @return 自身引用
     */
    element& with_key(size_t k);

    NEFORCE_NODISCARD kind kind() const noexcept;
    NEFORCE_NODISCARD size_t key() const noexcept;
    NEFORCE_NODISCARD const vector<element>& children() const;
    NEFORCE_NODISCARD vector<element>& children();
    NEFORCE_NODISCARD const tui::style& style() const;
    NEFORCE_NODISCARD const box_props& layout() const;
    NEFORCE_NODISCARD const string& text() const;

    /**
     * @brief 获取富文本片段列表
     * @return 片段列表
     */
    NEFORCE_NODISCARD const vector<text_span>& spans() const;

    /**
     * @brief 获取虚拟列表的项数量
     * @return 项数量
     */
    NEFORCE_NODISCARD size_t item_count() const noexcept;

    /**
     * @brief 获取虚拟列表的固定项高度
     * @return 每项占用的行数
     */
    NEFORCE_NODISCARD int item_height() const noexcept;

    /**
     * @brief 获取虚拟列表的项构建回调
     * @return 以项索引为参数的元素构建回调
     */
    NEFORCE_NODISCARD const function<element(size_t)>& item_builder() const;

    /**
     * @brief 获取回写布局矩形的状态指针
     * @return `state<layout_rect>*` 指针，未设置时为 nullptr
     */
    NEFORCE_NODISCARD void* rect_state() const noexcept;

    /**
     * @brief 绑定回写布局矩形的状态
     * @param s `state<layout_rect>` 指针
     * @return 自身引用
     *
     * 渲染器会把该元素最终占用的矩形写入状态，供需要知道自身尺寸的自定义组件使用。
     */
    element& with_rect_state(void* s);

    /**
     * @brief 设置纵向滚动偏移
     * @param y 行偏移
     * @return 自身引用
     *
     * 文本输入框用它实现多行文本的垂直滚动。
     */
    element& with_scroll_y(int y);

    /**
     * @brief 获取选中区间起点
     * @return 字节偏移
     */
    NEFORCE_NODISCARD size_t selection_begin() const noexcept;

    /**
     * @brief 获取选中区间终点
     * @return 字节偏移
     */
    NEFORCE_NODISCARD size_t selection_end() const noexcept;

    /**
     * @brief 设置选中区间
     * @param begin 起点字节偏移
     * @param end 终点字节偏移
     * @return 自身引用
     */
    element& with_selection(size_t begin, size_t end);

    /**
     * @brief 获取变高虚拟列表的高度表
     * @return 每项实测高度数组指针，固定高度模式下为 nullptr
     *
     * 数组长度等于项数量，元素为 0 表示该项尚未测量。
     */
    NEFORCE_NODISCARD const vector<int>* item_heights() const noexcept;

    /**
     * @brief 获取变高虚拟列表的估算项高度
     * @return 尚未测量项占位使用的行数
     */
    NEFORCE_NODISCARD int item_height_hint() const noexcept;

    /**
     * @brief 获取视口矩形回写状态
     * @return `state<layout_rect>*` 指针，未设置时为 nullptr
     */
    NEFORCE_NODISCARD void* viewport_rect_state() const noexcept;

    /**
     * @brief 绑定视口矩形回写状态
     * @param s `state<layout_rect>` 指针
     * @return 自身引用
     */
    element& with_viewport_rect_state(void* s);

    NEFORCE_NODISCARD void* viewport_state() const noexcept;

    /**
     * @brief 绑定回写可视高度的状态
     * @param s `state<int>` 指针
     * @return 自身引用
     */
    element& with_viewport_state(void* s);
    NEFORCE_NODISCARD const function<void()>& on_click() const;
    NEFORCE_NODISCARD int flex() const noexcept;
    NEFORCE_NODISCARD void* state_ref() const noexcept;
    NEFORCE_NODISCARD style::variant variant() const noexcept;
    NEFORCE_NODISCARD const function<void(int, int, int, int)>& draw_function() const;

    NEFORCE_NODISCARD void* owner() const noexcept;
    void set_owner(void* o);

    NEFORCE_NODISCARD int scroll_x() const noexcept;
    NEFORCE_NODISCARD int scroll_y() const noexcept;

    NEFORCE_NODISCARD int grid_columns() const noexcept;

    NEFORCE_NODISCARD const string& placeholder() const noexcept;

    NEFORCE_NODISCARD bool cursor_visible() const noexcept;
    element& with_cursor_visible(bool v);

    NEFORCE_NODISCARD size_t cursor_pos() const noexcept;
    element& with_cursor_pos(size_t pos);

    NEFORCE_NODISCARD style::wrap_mode wrap_mode() const noexcept;

    NEFORCE_NODISCARD void* scroll_x_state() const noexcept;
    element& with_scroll_x_state(void* s);
    NEFORCE_NODISCARD void* scroll_y_state() const noexcept;
    element& with_scroll_y_state(void* s);

    NEFORCE_NODISCARD bool is_layout_dirty() const noexcept;
    void set_layout_dirty(bool dirty);

    NEFORCE_NODISCARD const vector<layout_rect>& cached_layout() const noexcept;
    void set_cached_layout(const vector<layout_rect>& layout, int constraint_w, int constraint_h) const;
    NEFORCE_NODISCARD int cached_constraint_w() const noexcept;
    NEFORCE_NODISCARD int cached_constraint_h() const noexcept;

    static element empty();
    static element vbox(vector<element> children, box_props props = {});
    static element hbox(vector<element> children, box_props props = {});
    static element zstack(vector<element> children);
    static element text(string content, tui::style style = {}, style::wrap_mode wrap = style::wrap_mode::none);

    /**
     * @brief 富文本元素
     * @param spans 样式片段列表
     * @param style 基础样式，片段样式在其之上合并
     * @param wrap 换行模式
     * @return 富文本元素
     */
    static element rich_text(vector<text_span> spans, tui::style style = {},
                             style::wrap_mode wrap = style::wrap_mode::word);

    /**
     * @brief 虚拟列表元素
     * @param item_count 项总数
     * @param item_height 每项占用的固定行数
     * @param builder 项构建回调，仅对可视窗口内的索引调用
     * @param scroll_y 初始纵向滚动偏移
     * @return 虚拟列表元素
     * @note 虚拟列表没有内容高度：布局时按 `item_count × item_height` 估算固有高度，
     *       需要铺满剩余空间时仍应显式设置 `style.height = fill`
     */
    static element virtual_list(size_t item_count, int item_height, function<element(size_t)> builder,
                                int scroll_y = 0);

    /**
     * @brief 变高虚拟列表元素
     * @param item_count 项总数
     * @param item_height_hint 未测量项的估算高度
     * @param heights 每项实测高度数组，长度需不小于 item_count；元素 0 表示未测量
     * @param builder 项构建回调，仅对可视窗口内的索引调用
     * @param scroll_y 初始纵向滚动偏移
     * @return 虚拟列表元素
     */
    static element virtual_list(size_t item_count, int item_height_hint, vector<int>* heights,
                                function<element(size_t)> builder, int scroll_y = 0);

    static element spacer(int flex = 1);
    static element separator();
    static element button(string label, function<void()> on_click, tui::style style = {},
                          tui::style::variant variant = tui::style::variant::default_);
    static element text_input(state<string>& text, tui::style style = {}, string placeholder = "");
    static element checkbox(string label, state<bool>& checked, tui::style style = {});
    static element scroll_view(element child, tui::style style = {}, int scroll_x = 0, int scroll_y = 0);

    /**
     * @brief 合并样式
     * @param s 要合并的样式
     * @return 自身引用
     */
    element& with_style(const tui::style& s);

    /**
     * @brief 自定义绘制区域
     * @param draw_function 绘制函数
     * @param style 绘制样式
     */
    static element canvas(function<void(int, int, int, int)> draw_function, tui::style style = tui::style{});

    /**
     * @brief 面板容器
     * @param child 子元素
     * @param style 样式，`border` 决定是否绘制边框，`padding` 决定内边距
     * @return 面板元素
     *
     * 面板自身占用一个布局矩形，因此可以绘制边框并把子元素排布在内边距之内。
     */
    static element panel(element child, tui::style style = {});

    /**
     * @brief 背景色带容器
     * @param child 子元素
     * @param style 样式，`bg` 用于填充整个矩形，`padding` 决定内边距
     * @return 色带元素
     */
    static element band(element child, tui::style style = {});

    /**
     * @brief CSS-flexbox 弹性容器
     * @param children 子元素列表
     * @param config 布局配置
     * @return FlexBox 元素
     */
    static element flexbox(vector<element> children, box_props config = {});

    /**
     * @brief 网格布局容器
     * @param grid 二维子元素网格
     * @return GridBox 元素
     */
    static element gridbox(vector<vector<element>> grid);

    /**
     * @brief 条件渲染
     * @param cond 条件
     * @param then 条件为真时的渲染函数
     * @param otherwise 条件为假时的渲染函数
     * @return When 元素
     */
    static element
    when(bool cond, function<element()> then, function<element()> otherwise = []() { return element::empty(); });

    /**
     * @brief 列表渲染
     * @tparam T 列表元素类型
     * @param items 数据列表
     * @param render 渲染回调，接收 (item引用, index)
     * @return Each 元素
     * @note 列表元素应通过 withKey() 分配唯一 key。
     */
    template <typename T>
    static element each(const vector<T>& items, function<element(const T&, size_t)> render) {
        element el;
        el.init_kind(kind::each);
        el.reserve_children(items.size());
        for (size_t i = 0; i < items.size(); ++i) {
            el.add_child(render(items[i], i));
        }
        return el;
    }

private:
    struct node;
    shared_ptr<node> node_;

    void ensure_node();
    void init_kind(enum kind k);
    void reserve_children(size_t n);
    void add_child(element child);
};

/**
 * @brief 元素装饰器
 *
 * 接收一个元素并返回包装后的元素。可通过 operator| 链式组合。
 */
using decorator = function<element(element)>;

/**
 * @brief 元素列表类型
 */
using elements = vector<element>;


/**
 * @brief 将装饰器应用于元素
 * @param e 源元素
 * @param d 装饰器
 * @return 装饰后的元素
 */
inline element operator|(element e, const decorator& d) { return d(move(e)); }

/**
 * @brief 就地装饰元素
 * @param e 源元素引用
 * @param d 装饰器
 * @return 元素引用
 */
inline element& operator|=(element& e, const decorator& d) {
    e = d(move(e));
    return e;
}

/**
 * @brief 为元素列表的每个元素应用装饰器
 * @param es 元素列表
 * @param d 装饰器
 * @return 装饰后的元素列表
 */
inline vector<element> operator|(const vector<element>& es, const decorator& d) {
    vector<element> result;
    result.reserve(es.size());
    for (const auto& e: es) {
        result.push_back(d(e));
    }
    return result;
}

/**
 * @brief 组合两个装饰器
 * @param a 外层装饰器
 * @param b 内层装饰器
 * @return 组合装饰器
 */
inline decorator operator|(decorator a, decorator b) {
    return [a = move(a), b = move(b)](element e) { return a(b(move(e))); };
}


/// @brief 粗体装饰器
inline decorator bold() {
    return [](element e) {
        style s;
        s.bold = true;
        return e.with_style(s);
    };
}

/// @brief 暗色装饰器
inline decorator dim() {
    return [](element e) {
        style s;
        s.dim = true;
        return e.with_style(s);
    };
}

/// @brief 斜体装饰器
inline decorator italic() {
    return [](element e) {
        style s;
        s.italic = true;
        return e.with_style(s);
    };
}

/// @brief 单下划线装饰器
inline decorator underlined() {
    return [](element e) {
        style s;
        s.underline = true;
        return e.with_style(s);
    };
}

/// @brief 双下划线装饰器
inline decorator underlined_double() {
    return [](element e) {
        style s;
        s.underlined_double = true;
        return e.with_style(s);
    };
}

/// @brief 闪烁装饰器
inline decorator blink() {
    return [](element e) {
        style s;
        s.blink = true;
        return e.with_style(s);
    };
}

/// @brief 删除线装饰器
inline decorator strikethrough() {
    return [](element e) {
        style s;
        s.strikethrough = true;
        return e.with_style(s);
    };
}

/// @brief 反色装饰器
inline decorator inverted() {
    return [](element e) {
        style s;
        s.reverse = true;
        return e.with_style(s);
    };
}

/// @brief 前景色装饰器
/// @param c 颜色
inline decorator color(color c) {
    return [c = move(c)](element e) {
        style s;
        s.fg = c;
        return e.with_style(s);
    };
}

/// @brief 背景色装饰器
/// @param c 颜色
inline decorator bgcolor(_NEFORCE color c) {
    return [c = move(c)](element e) {
        style s;
        s.bg = c;
        return e.with_style(s);
    };
}

/// @brief 边框装饰器
/// @param b 边框样式
inline decorator border(enum style::border b) {
    return [b](element e) {
        style s;
        s.border = b;
        return e.with_style(s);
    };
}

/// @brief 弹性增长装饰器
/// @param factor 增长权重
inline decorator flex_grow_el(float factor) {
    return [factor](element e) {
        style s;
        s.flex_grow = factor;
        return e.with_style(s);
    };
}

/// @brief 弹性收缩装饰器
/// @param factor 收缩权重
inline decorator flex_shrink_el(float factor) {
    return [factor](element e) {
        style s;
        s.flex_shrink = factor;
        return e.with_style(s);
    };
}

/// @brief 弹性装饰器
/// @param grow 增长权重
/// @param shrink 收缩权重
inline decorator flex_el(float grow, float shrink) {
    return [grow, shrink](element e) {
        style s;
        s.flex_grow = grow;
        s.flex_shrink = shrink;
        return e.with_style(s);
    };
}

/// @brief 文本换行装饰器
/// @param wm 换行模式
inline decorator text_wrap(style::wrap_mode wm) {
    return [wm](element e) {
        style s;
        s.text_wrap = wm;
        return e.with_style(s);
    };
}

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_DOM_ELEMENT_HPP__
