#ifndef NEFORCE_TUI_COMPONENT_TEXT_AREA_HPP__
#define NEFORCE_TUI_COMPONENT_TEXT_AREA_HPP__

/**
 * @file text_area.hpp
 * @brief 多行文本编辑器组件
 *
 * 软换行编辑、选区、撤销重做、输入历史与光标随动滚动。
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
 * @brief 多行编辑器配置
 */
struct text_area_option {
    state<string>* text = nullptr;                            ///< 文本状态
    string placeholder;                                       ///< 占位提示
    tui::style style;                                         ///< 样式，`border`/`padding` 参与布局
    tui::style::wrap_mode wrap = tui::style::wrap_mode::word; ///< 换行模式
    bool submit_on_enter = false;                             ///< Enter 是否直接提交而不是换行
    size_t history_limit = 100;                               ///< 历史条目上限
    size_t undo_limit = 100;                                  ///< 撤销栈上限
    function<void()> on_submit;                               ///< 提交回调
    function<void(const string&)> on_change;                  ///< 内容变化回调（用于实时过滤/补全）
};

/**
 * @brief 多行文本编辑器
 *
 * 键位：
 * - 可打印字符插入，回车换行
 * - `Backspace` / `Delete` 删除选区或相邻字符
 * - 方向键移动光标，`Shift` 扩展选区，`Ctrl` 按词移动
 * - `Home` / `End` 行首行尾，`Ctrl+Home` / `Ctrl+End` 全文首尾，`PgUp` / `PgDn` 翻页
 * - `Ctrl+A` 全选，`Ctrl+Z` 撤销，`Ctrl+Y` 重做
 * - `Ctrl+P` / `Ctrl+N` 调用历史，`Ctrl+Enter` 提交
 *
 * @note 文本中的换行统一为 `\\n`。
 */
class NEFORCE_API text_area_view final : public component<> {
public:
    /**
     * @brief 构造函数
     * @param opt 配置
     */
    explicit text_area_view(text_area_option opt);

    /**
     * @brief 文本内容
     * @return 文本常量引用
     */
    NEFORCE_NODISCARD const string& text() const;

    /**
     * @brief 光标字节偏移
     * @return 字节偏移
     */
    NEFORCE_NODISCARD size_t cursor() const noexcept { return cursor_; }

    /**
     * @brief 选区起点（无选区时等于光标）
     * @return 字节偏移
     */
    NEFORCE_NODISCARD size_t selection_begin() const noexcept;

    /**
     * @brief 选区终点（无选区时等于光标）
     * @return 字节偏移
     */
    NEFORCE_NODISCARD size_t selection_end() const noexcept;

    /**
     * @brief 是否有选区
     * @return 是否有选区
     */
    NEFORCE_NODISCARD bool has_selection() const noexcept { return anchor_ != cursor_; }

    /**
     * @brief 设置光标位置
     * @param offset 字节偏移
     */
    void set_cursor(size_t offset);

    /**
     * @brief 全选
     */
    void select_all();

    /**
     * @brief 清空选区
     */
    void clear_selection() noexcept { anchor_ = cursor_; }

    /**
     * @brief 用新内容替换全部文本
     * @param value 新文本
     */
    void set_text(string value);

    /**
     * @brief 提交当前文本到历史并回调
     */
    void submit();

    /**
     * @brief 历史条目数
     * @return 条目数
     */
    NEFORCE_NODISCARD size_t history_size() const noexcept { return history_.size(); }

    /**
     * @brief 撤销
     * @return 是否发生了撤销
     */
    bool undo();

    /**
     * @brief 重做
     * @return 是否发生了重做
     */
    bool redo();

    void setup() override;

    element render() override;

    bool on_key(const key_event& e) override;

    bool on_paste(const paste_event& e) override;

    NEFORCE_NODISCARD bool focusable() const override { return true; }

private:
    struct snapshot {
        string text;
        size_t cursor;
        size_t anchor;
    };

    text_area_option opt_;
    state<layout_rect>* rect_ = nullptr;
    size_t cursor_ = 0;
    size_t anchor_ = 0;
    int scroll_row_ = 0;
    bool geometry_known_ = false;
    vector<snapshot> undo_;
    vector<snapshot> redo_;
    vector<string> history_;
    size_t history_pos_ = 0;

    void record_undo();
    void jump_history(int delta);
    void clamp_cursor();
    void insert_text(string_view chunk);
    void erase_selection();
    void move_up_or_down(int delta, bool extend);
    void move_line_boundary(bool to_end, bool extend);
    void move_document_boundary(bool to_end, bool extend);
    void move_page(int direction, bool extend);
    NEFORCE_NODISCARD size_t word_boundary(size_t from, bool forward) const;
    void notify_changed();
    void notify_content_changed();
};

/**
 * @brief 创建多行编辑器组件
 * @param opt 配置
 * @return 组件指针
 */
unique_ptr<component_base> NEFORCE_API text_area(text_area_option opt);

/** @} */ // Components

/** @} */ // TUI

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_COMPONENT_TEXT_AREA_HPP__
