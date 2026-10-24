#ifndef NEFORCE_TUI_COMPONENT_COMPOSER_HPP__
#define NEFORCE_TUI_COMPONENT_COMPOSER_HPP__

/**
 * @file composer.hpp
 * @brief 输入编排组件
 *
 * 把多行编辑器、补全弹层与状态提示行组合成 agent 对话的输入区：
 * 提交与换行语义、斜杠命令/`@文件` 补全、运行中的中断提示。
 */

#include "NeForce/tui/component/component.hpp"
#include "NeForce/tui/component/select_list.hpp"
#include "NeForce/tui/component/text_area.hpp"
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
 * @brief 补全项
 */
struct completion_item {
    string label;  ///< 展示文本
    string detail; ///< 右侧说明（可为空）
    string insert; ///< 接受后插入的文本，空表示使用 label
};

/**
 * @brief 补全请求
 */
struct completion_request {
    string query;      ///< 触发符之后到光标之间的查询串
    size_t cursor = 0; ///< 光标的字节偏移
};

/**
 * @brief 输入区阶段
 */
enum class composer_phase : uint8_t {
    idle,    ///< 空闲，可以提交
    running, ///< 正在执行，Esc 触发中断
};

/**
 * @brief 输入区配置
 */
struct composer_option {
    state<string>* text = nullptr;     ///< 外部文本状态，空表示由组件内部持有
    string placeholder;                ///< 占位提示
    string hint;                       ///< 右侧静态提示，默认 `enter send · shift+enter newline`
    string running_hint;               ///< 运行中提示，默认 `esc to interrupt`
    int visible_completions = 6;       ///< 补全弹层可视行数
    char32_t trigger = U'/';           ///< 自动触发补全的字符，0 表示不自动触发
    bool trigger_at_line_start = true; ///< 触发符是否必须位于行首（斜杠命令）
    function<void(const completion_request&, vector<completion_item>&)> complete; ///< 补全提供者
    function<void(const string&)> on_submit;                                      ///< 提交回调
    function<void()> on_interrupt;                                                ///< 运行中按 Esc 的中断回调
    tui::style style;                                                             ///< 输入框样式（传给编辑器）
    tui::style bar_style;                                                         ///< 状态行样式
    size_t history_limit = 100;                                                   ///< 提交历史条数上限，0 表示不记录
    bool submit_on_enter = true; ///< Enter 直接提交；false 时 Enter 换行、Ctrl+Enter 提交
    bool history_enabled = true; ///< 是否启用单行下的 `Up`/`Down` 历史浏览
};

/**
 * @brief 输入编排组件
 *
 * 键位：
 * - 可打印字符、编辑键全部转发给内部编辑器
 * - 补全弹层开启时：`Up`/`Down`/`PgUp`/`PgDn` 移动，`Tab`/`Enter` 接受，`Esc` 关闭
 * - `Enter` 提交，`Shift+Enter` 换行，`Ctrl+Enter` 提交
 * - 单行文本时 `Up`/`Down` 浏览提交历史
 * - 空闲时 `Esc` 冒泡给上层，运行中 `Esc` 触发中断
 *
 * @note `Ctrl+C` 等全局快捷键不在此消费，交给 `keymap` 的全局层处理。
 */
class NEFORCE_API composer final : public component_base {
public:
    /**
     * @brief 构造函数
     * @param opt 配置
     */
    explicit composer(composer_option opt = {});

    /**
     * @brief 设置阶段
     * @param phase 空闲或运行中
     */
    void set_phase(composer_phase phase);

    /**
     * @brief 当前阶段
     * @return 阶段
     */
    NEFORCE_NODISCARD composer_phase phase() const noexcept { return phase_; }

    /**
     * @brief 设置右侧提示文本
     * @param hint 提示文本，空表示使用默认提示
     */
    void set_hint(string hint);

    /**
     * @brief 当前文本
     * @return 文本
     */
    NEFORCE_NODISCARD string_view text() const noexcept;

    /**
     * @brief 替换文本并清空选区
     * @param value 新文本
     */
    void set_text(string value);

    /**
     * @brief 提交当前文本
     */
    void submit();

    /**
     * @brief 提交历史条目数
     * @return 条目数
     */
    NEFORCE_NODISCARD size_t history_size() const noexcept;

    /**
     * @brief 调整提交历史上限
     * @param limit 新上限，0 表示清空并停止记录
     */
    void set_history_limit(size_t limit);

    /**
     * @brief 设置 Enter 是否直接提交
     * @param enabled true 时 Enter 提交、Shift+Enter 换行；false 时 Enter 换行、Ctrl+Enter 提交
     */
    void set_submit_on_enter(bool enabled);

    /**
     * @brief 是否正在显示补全弹层
     * @return 是否显示
     */
    NEFORCE_NODISCARD bool completion_open() const noexcept { return popup_ != nullptr; }

    /**
     * @brief 关闭补全弹层
     */
    void close_completion();

    /**
     * @brief 主动请求补全（按当前光标位置计算查询串）
     * @return 是否有可选项
     */
    bool request_completion();

    /**
     * @brief 获取内部编辑器
     * @return 编辑器引用
     */
    NEFORCE_NODISCARD text_area_view& editor() noexcept { return *input_; }

    void setup() override;

    /**
     * @brief 请求焦点
     *
     * 把焦点同时转交给内部编辑器，键盘事件因此可以经由本组件转发处理。
     */
    void take_focus() override;

    element render() override;

    bool on_key(const key_event& e) override;

    bool on_paste(const paste_event& e) override;

    NEFORCE_NODISCARD bool focusable() const override { return true; }

private:
    composer_option opt_;
    state<string>* text_storage_ = nullptr;
    text_area_view* input_ = nullptr;
    select_list_view* popup_ = nullptr;
    composer_phase phase_ = composer_phase::idle;
    string hint_override_;
    size_t trigger_begin_ = 0;

    NEFORCE_NODISCARD bool extract_query(string& query) const;
    void refresh_completions();
    void accept_completion();
    NEFORCE_NODISCARD string current_hint() const;
    NEFORCE_NODISCARD bool history_browsable() const;
};

/**
 * @brief 创建输入编排组件
 * @param opt 配置
 * @return 组件指针
 */
unique_ptr<component_base> NEFORCE_API composer_component(composer_option opt = {});

/** @} */ // Components

/** @} */ // TUI

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_COMPONENT_COMPOSER_HPP__
