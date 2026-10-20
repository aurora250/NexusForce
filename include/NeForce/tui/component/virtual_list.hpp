#ifndef NEFORCE_TUI_COMPONENT_VIRTUAL_LIST_HPP__
#define NEFORCE_TUI_COMPONENT_VIRTUAL_LIST_HPP__

/**
 * @file virtual_list.hpp
 * @brief 虚拟列表组件
 *
 * 只对可视窗口内的项做布局与渲染，因此项总数可以很大；
 * 支持贴底自动跟随、滚动条拖拽与键盘滚动。
 */

#include "NeForce/tui/component/component.hpp"
#include "NeForce/tui/dom/element.hpp"
#include "NeForce/tui/dom/layout_types.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__
NEFORCE_BEGIN_COMPONENTS__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @addtogroup Components 用户组件
 * @{
 */

/**
 * @brief 虚拟列表配置
 */
struct virtual_list_option {
    size_t item_count = 0;                 ///< 项总数
    int item_height = 1;                   ///< 每项占用的固定行数（变高模式下为未测量项的估算高度）
    bool variable_height = false;          ///< 是否启用变高项（按实测高度定位）
    function<element(size_t)> render_item; ///< 项构建回调，只对可视项调用
    bool auto_follow = true;               ///< 追加项时是否自动贴底
    tui::style style;                      ///< 列表样式
};

/**
 * @brief 虚拟列表组件
 *
 * 持有滚动状态、可视高度，并在此之上实现"追加内容时贴底跟随"与键盘滚动。
 *
 * @code
 * virtual_list_view list({.item_count = messages.size(),
 *                         .render_item = [this](size_t i) { return render_message(i); }});
 * list.set_item_count(messages.size());   // 追加消息后调用，会自动贴底
 * @endcode
 */
class NEFORCE_API virtual_list_view final : public component<> {
public:
    /**
     * @brief 构造函数
     * @param opt 配置
     */
    explicit virtual_list_view(virtual_list_option opt);

    /**
     * @brief 设置项总数
     * @param count 项总数
     *
     * 原先处于底部时会自动贴底，用户上滚之后不再强制跟随。
     */
    void set_item_count(size_t count);

    /**
     * @brief 替换项构建回调
     * @param render_item 回调
     */
    void set_item_renderer(function<element(size_t)> render_item);

    /**
     * @brief 设置项高度
     * @param height 行数；变高模式下作为未测量项的估算高度
     */
    void set_item_height(int height);

    /**
     * @brief 是否启用变高项
     * @return 是否变高
     */
    NEFORCE_NODISCARD bool variable_height() const noexcept { return opt_.variable_height; }

    /**
     * @brief 启用或关闭变高模式
     * @param enable 是否变高
     */
    void set_variable_height(bool enable);

    /**
     * @brief 丢弃已测量的项高度
     *
     * 宽度或内容变化后调用，下一次渲染会重新测量可视项。
     */
    void invalidate_heights();

    /**
     * @brief 获取指定项的已测量高度
     * @param index 项索引
     * @return 行数，0 表示尚未测量
     */
    NEFORCE_NODISCARD int measured_height(size_t index) const noexcept;

    /**
     * @brief 已测量项的数量
     * @return 项数量
     */
    NEFORCE_NODISCARD size_t measured_count() const noexcept;

    /**
     * @brief 获取项总数
     * @return 项总数
     */
    NEFORCE_NODISCARD size_t item_count() const noexcept { return opt_.item_count; }

    /**
     * @brief 获取固定项高度
     * @return 行数
     */
    NEFORCE_NODISCARD int item_height() const noexcept { return max(1, opt_.item_height); }

    /**
     * @brief 开关自动跟随
     * @param enable 是否跟随
     */
    void set_auto_follow(bool enable);

    /**
     * @brief 是否正在跟随底部
     * @return 是否跟随
     */
    NEFORCE_NODISCARD bool auto_follow() const noexcept { return follow_; }

    /**
     * @brief 获取可视行数
     * @return 行数
     */
    NEFORCE_NODISCARD int viewport_rows() const noexcept;

    /**
     * @brief 获取内容总行数
     * @return 行数
     */
    NEFORCE_NODISCARD int content_rows() const noexcept;

    /**
     * @brief 获取当前滚动偏移
     * @return 行偏移
     */
    NEFORCE_NODISCARD int scroll_offset() const noexcept;

    /**
     * @brief 是否停在底部
     * @return 是否在底部
     */
    NEFORCE_NODISCARD bool at_bottom() const noexcept;

    /**
     * @brief 滚动到指定偏移
     * @param offset 行偏移
     */
    void scroll_to(int offset);

    /**
     * @brief 相对滚动
     * @param delta 行增量，负值向上
     */
    void scroll_by(int delta);

    /**
     * @brief 把指定项滚动到可视区顶部
     * @param index 项索引
     */
    void scroll_to_item(size_t index);

    /**
     * @brief 滚动到底部
     */
    void scroll_to_bottom();

    /**
     * @brief 获取滚动状态
     * @return `state<int>` 引用，可用于外部绑定或监听
     */
    state<int>& scroll_state() noexcept { return *scroll_; }

    void setup() override;

    element render() override;

    bool on_key(const key_event& e) override;

    NEFORCE_NODISCARD bool focusable() const override { return true; }

private:
    virtual_list_option opt_;
    state<int>* scroll_ = nullptr;
    state<int>* viewport_ = nullptr;
    state<layout_rect>* rect_ = nullptr;
    vector<int> heights_;
    vector<int> heights_snapshot_;
    int layout_width_ = 0;
    bool follow_ = true;
    bool follow_pending_ = true;

    void clamp_scroll();
    NEFORCE_NODISCARD int rows_of(size_t index) const noexcept;
};

/**
 * @brief 创建虚拟列表组件
 * @param opt 配置
 * @return 组件指针
 */
unique_ptr<component_base> NEFORCE_API virtual_list(virtual_list_option opt);

/** @} */ // Components

/** @} */ // TUI

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_COMPONENT_VIRTUAL_LIST_HPP__
