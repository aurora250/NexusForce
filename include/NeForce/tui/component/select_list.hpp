#ifndef NEFORCE_TUI_COMPONENT_SELECT_LIST_HPP__
#define NEFORCE_TUI_COMPONENT_SELECT_LIST_HPP__

/**
 * @file select_list.hpp
 * @brief 可选择列表组件
 *
 * 支持过滤、滚动、键盘选择与确认，用于命令面板、候选补全与选项选择。
 */

#include "NeForce/tui/component/component.hpp"
#include "NeForce/tui/dom/element.hpp"
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
 * @brief 可选择列表配置
 */
struct select_list_option {
    vector<string> items;           ///< 候选项
    int visible_rows = 8;           ///< 可视行数
    bool filterable = true;         ///< 是否允许键入过滤
    string filter_prefix = "> ";    ///< 过滤行前缀
    string empty_text = "no match"; ///< 无匹配时的提示
    tui::style item_style;          ///< 普通项样式
    tui::style selected_style;      ///< 选中项样式（未设置时使用反色）
    tui::style filter_style;        ///< 过滤行样式
    state<int>* selected = nullptr; ///< 外部选中索引
    function<void(int)> on_select;  ///< 回车确认回调
    function<void(int)> on_change;  ///< 选中项变化回调
    function<void()> on_cancel;     ///< Esc 且无过滤内容时回调
};

/**
 * @brief 可选择列表组件
 *
 * 键位：`↑`/`↓`/`Ctrl+P`/`Ctrl+N` 移动，`PgUp`/`PgDn` 翻页，`Enter` 确认，
 * `Esc` 先清空过滤再取消；可打印字符写入过滤串，`Backspace` 删除。
 */
class NEFORCE_API select_list_view final : public component_base {
public:
    /**
     * @brief 构造函数
     * @param opt 配置
     */
    explicit select_list_view(select_list_option opt);

    /**
     * @brief 替换候选项
     * @param items 候选项列表
     */
    void set_items(vector<string> items);

    /**
     * @brief 设置过滤串
     * @param filter 过滤串
     */
    void set_filter(string filter);

    /**
     * @brief 获取过滤串
     * @return 过滤串
     */
    NEFORCE_NODISCARD const string& filter() const noexcept { return filter_; }

    /**
     * @brief 获取匹配后的项
     * @return 项列表
     */
    NEFORCE_NODISCARD const vector<string>& matches() const noexcept { return matches_; }

    /**
     * @brief 获取当前选中项在原始列表中的下标
     * @return 下标，无匹配时为 -1
     */
    NEFORCE_NODISCARD int selected_index() const noexcept;

    /**
     * @brief 获取当前选中项在匹配列表中的序号
     * @return 序号，无匹配时为 -1
     */
    NEFORCE_NODISCARD int selected_position() const noexcept { return position_; }

    /**
     * @brief 设置可视行数
     * @param rows 行数
     */
    void set_visible_rows(int rows);

    /**
     * @brief 设置选中序号
     * @param position 匹配列表中的序号
     */
    void set_selected_position(int position);

    /**
     * @brief 触发确认
     * @return 是否确认了某一项
     */
    bool confirm();

    void setup() override;

    element render() override;

    bool on_key(const key_event& e) override;

    NEFORCE_NODISCARD bool focusable() const override { return true; }

private:
    select_list_option opt_;
    vector<string> matches_;
    vector<size_t> match_indices_;
    string filter_;
    int position_ = 0;
    int scroll_ = 0;
    state<int>* viewport_ = nullptr;
    state<int>* scroll_state_ = nullptr;

    void rebuild_matches();
    void move_by(int delta);
};

/**
 * @brief 创建可选择列表组件
 * @param opt 配置
 * @return 组件指针
 */
unique_ptr<component_base> NEFORCE_API select_list(select_list_option opt);

/** @} */ // Components

/** @} */ // TUI

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_COMPONENT_SELECT_LIST_HPP__
