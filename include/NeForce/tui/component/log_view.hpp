#ifndef NEFORCE_TUI_COMPONENT_LOG_VIEW_HPP__
#define NEFORCE_TUI_COMPONENT_LOG_VIEW_HPP__

/**
 * @file log_view.hpp
 * @brief 对话记录视图
 *
 * 面向 agent 对话的输出区：变高消息、流式合并追加、贴底自动跟随、
 * Markdown 渲染缓存与"上滚期间新消息"提示。
 */

#include "NeForce/tui/component/component.hpp"
#include "NeForce/tui/component/virtual_list.hpp"
#include "NeForce/tui/dom/element.hpp"
#include "NeForce/tui/dom/markdown.hpp"
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
 * @brief 消息来源
 */
enum class log_role : uint8_t {
    user,      ///< 用户输入
    assistant, ///< 模型回复
    tool,      ///< 工具调用与输出
    system,    ///< 系统信息
    notice,    ///< 提示/警告
};

/**
 * @brief 消息来源样式
 */
struct log_role_style {
    string name;         ///< 显示名，空表示不显示消息头
    tui::style title;    ///< 消息头样式
    tui::style band;     ///< 背景带样式
    bool banded = false; ///< 正文是否包裹背景带
};

/**
 * @brief 对话记录配置
 */
struct log_view_option {
    vector<log_role_style> roles;              ///< 按 `log_role` 索引的来源样式，缺省项使用内置默认
    markdown_option markdown;                  ///< Markdown 渲染配置
    int gap = 1;                               ///< 消息之间的空行数
    int height_hint = 4;                       ///< 未测量消息的估算高度
    bool auto_follow = true;                   ///< 追加消息时是否自动贴底
    bool show_pending_hint = true;             ///< 上滚期间是否显示新消息提示
    size_t cache_capacity = 64;                ///< Markdown 渲染缓存容量
    function<void(uint64_t)> on_message_click; ///< 点击消息回调（参数为消息 id）
};

/**
 * @brief 对话记录视图
 *
 * 消息高度按实测值定位，因此每帧只渲染可视窗口内的消息。
 *
 * @code
 * log_view log;
 * const uint64_t id = log.append(log_role::assistant, "partial");
 * log.append_chunk(id, " answer");   // 流式追加
 * log.finish(id);                    // 渲染缓存
 * @endcode
 */
class NEFORCE_API log_view final : public component<> {
public:
    /**
     * @brief 构造函数
     * @param opt 配置
     */
    explicit log_view(log_view_option opt = {});

    /**
     * @brief 追加一条消息
     * @param role 消息来源
     * @param content Markdown 内容
     * @param title 覆盖来源显示名，空表示用来源样式中的名字
     * @return 消息 id
     */
    uint64_t append(log_role role, string content, string title = string());

    /**
     * @brief 流式追加内容
     * @param id 消息 id
     * @param chunk 追加片段
     */
    void append_chunk(uint64_t id, string_view chunk);

    /**
     * @brief 提交所有未提交的流式片段
     */
    void commit();

    /**
     * @brief 定稿一条消息
     * @param id 消息 id
     */
    void finish(uint64_t id);

    /**
     * @brief 替换消息内容
     * @param id 消息 id
     * @param content 新的 Markdown 内容
     */
    void set_content(uint64_t id, string content);

    /**
     * @brief 替换消息标题
     * @param id 消息 id
     * @param title 新标题
     */
    void set_title(uint64_t id, string title);

    /**
     * @brief 原地修改一条消息
     * @param id 消息 id
     * @param mutate 修改函数
     */
    void modify(uint64_t id, const function<void(log_role&, string&, string&, bool&)>& mutate);

    /**
     * @brief 清空所有消息
     */
    void clear();

    /**
     * @brief 消息数量
     * @return 条数
     */
    NEFORCE_NODISCARD size_t message_count() const noexcept { return messages_.size(); }

    /**
     * @brief 滚动到底部
     */
    void scroll_to_bottom();

    /**
     * @brief 滚动到指定消息
     * @param id 消息 id
     */
    void scroll_to_message(uint64_t id);

    /**
     * @brief 是否停在底部
     * @return 是否在底部
     */
    NEFORCE_NODISCARD bool at_bottom() const;

    /**
     * @brief 上滚期间到达的新消息数量
     * @return 条数
     */
    NEFORCE_NODISCARD size_t pending_messages() const noexcept { return pending_; }

    /**
     * @brief 开关自动跟随
     * @param enable 是否跟随
     */
    void set_auto_follow(bool enable);

    /**
     * @brief 是否正在跟随
     * @return 是否跟随
     */
    NEFORCE_NODISCARD bool auto_follow() const;

    /**
     * @brief 获取 Markdown 渲染缓存
     * @return 缓存引用
     */
    NEFORCE_NODISCARD markdown_cache& cache() noexcept { return cache_; }

    /**
     * @brief 获取内部虚拟列表
     * @return 虚拟列表引用
     */
    NEFORCE_NODISCARD virtual_list_view& list() noexcept { return *list_; }

    /**
     * @brief 获取内部虚拟列表
     * @return 虚拟列表常量引用
     */
    NEFORCE_NODISCARD const virtual_list_view& list() const noexcept { return *list_; }

    void setup() override;

    element render() override;

    bool on_key(const key_event& e) override;

    /**
     * @brief 鼠标事件
     *
     * 左键点击选中命中的消息并触发 `on_message_click` 回调。
     */
    bool on_mouse(const mouse_event& e) override;

    /**
     * @brief 当前选中的消息 id
     * @return 消息 id，未选中时为 0
     */
    NEFORCE_NODISCARD uint64_t selected_message() const noexcept { return selected_; }

    /**
     * @brief 选中指定消息
     * @param id 消息 id
     */
    void select_message(uint64_t id);

    /**
     * @brief 把指定消息的文本写入系统剪贴板
     * @param id 消息 id
     * @return 是否有内容被复制
     */
    bool copy_message(uint64_t id);

    /**
     * @brief 复制当前选中的消息
     * @return 是否有内容被复制
     */
    bool copy_selected();

    void on_animation(int64_t delta) override;

    NEFORCE_NODISCARD bool focusable() const override { return true; }

private:
    struct message {
        uint64_t id = 0;
        log_role role = log_role::assistant;
        string title;
        string content;
        string pending;
        bool streaming = false;
    };

    log_view_option opt_;
    vector<message> messages_;
    markdown_cache cache_;
    virtual_list_view* list_ = nullptr;
    uint64_t next_id_ = 1;
    size_t pending_ = 0;
    uint64_t selected_ = 0;
    function<void(uint64_t)> on_message_click_;

    NEFORCE_NODISCARD message* find(uint64_t id);
    NEFORCE_NODISCARD const log_role_style& style_of(log_role role) const;
    NEFORCE_NODISCARD element render_message(size_t index);
    NEFORCE_NODISCARD uint64_t message_at(int y) const;
    void mark_arrived();
};

/**
 * @brief 创建对话记录视图组件
 * @param opt 配置
 * @return 组件指针
 */
unique_ptr<component_base> NEFORCE_API log_view_component(log_view_option opt = {});

/** @} */ // Components

/** @} */ // TUI

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_COMPONENT_LOG_VIEW_HPP__
