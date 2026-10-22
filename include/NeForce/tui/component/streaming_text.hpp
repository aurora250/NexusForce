#ifndef NEFORCE_TUI_COMPONENT_STREAMING_TEXT_HPP__
#define NEFORCE_TUI_COMPONENT_STREAMING_TEXT_HPP__

/**
 * @file streaming_text.hpp
 * @brief 流式文本组件
 *
 * 把高频追加的文本攒批到每帧提交一次，避免逐 token 写入触发逐 token 重渲染。
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
 * @brief 流式文本配置
 */
struct streaming_text_option {
    tui::style style;                                         ///< 文本样式
    tui::style::wrap_mode wrap = tui::style::wrap_mode::word; ///< 换行模式
    size_t max_bytes = 1U << 20U;                             ///< 缓冲上限，超出后丢弃最早内容
};

/**
 * @brief 流式文本组件
 *
 * 只写入内部缓冲，不触发渲染。
 *
 * @code
 * streaming_text_view stream;
 * stream.append("token");   // 可高频调用
 * // 渲染循环每帧会自动 commit()
 * @endcode
 */
class NEFORCE_API streaming_text_view final : public component_base {
public:
    streaming_text_view() = default;

    /**
     * @brief 构造函数
     * @param opt 配置
     */
    explicit streaming_text_view(streaming_text_option opt);

    /**
     * @brief 追加文本
     * @param chunk 文本片段
     */
    void append(string_view chunk);

    /**
     * @brief 把缓冲提交到状态并请求重渲染
     */
    void commit();

    /**
     * @brief 清空文本与缓冲
     */
    void clear();

    /**
     * @brief 当前已提交文本
     * @return 文本
     */
    NEFORCE_NODISCARD string_view text() const noexcept;

    /**
     * @brief 是否还有未提交的缓冲
     * @return 是否有待提交内容
     */
    NEFORCE_NODISCARD bool pending() const noexcept { return !pending_.empty(); }

    /**
     * @brief 获取文本状态
     * @return `state<string>` 引用
     */
    state<string>& text_state() noexcept { return *text_; }

    void setup() override;

    element render() override;

    void on_animation(int64_t delta) override;

private:
    streaming_text_option opt_;
    state<string>* text_ = nullptr;
    string pending_;
};

/**
 * @brief 创建流式文本组件
 * @param opt 配置
 * @return 组件指针
 */
unique_ptr<component_base> NEFORCE_API streaming_text(streaming_text_option opt = streaming_text_option{});

/** @} */ // Components

/** @} */ // TUI

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_COMPONENT_STREAMING_TEXT_HPP__
