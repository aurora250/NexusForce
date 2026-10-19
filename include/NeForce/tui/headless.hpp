#ifndef NEFORCE_TUI_HEADLESS_HPP__
#define NEFORCE_TUI_HEADLESS_HPP__

/**
 * @file headless.hpp
 * @brief 无终端应用驱动
 *
 * 在没有 TTY 的环境里驱动完整渲染管线：
 * 固定终端尺寸、帧输出回收到内存、输入按字节注入。
 * 用于快照测试、录制回放与离屏渲染。
 */

#include "NeForce/core/async/io_context.hpp"
#include "NeForce/core/async/strand.hpp"
#include "NeForce/core/memory/unique_ptr.hpp"
#include "NeForce/tui/component/component.hpp"
#include "NeForce/tui/input_parser.hpp"
#include "NeForce/tui/reconciler.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 无终端应用驱动
 *
 * 与 `application` 共享同一条渲染管线，但不触碰终端：
 * 尺寸由构造参数固定，帧输出写入内存缓冲，输入通过 `input_parser` 解析后直接投递到事件分发器。
 *
 * @code
 * struct root : component<> {
 *     element render() override {
 *         return element::text("hello", {}, style::wrap_mode::none);
 *     }
 * };
 * root r;
 * headless app(&r, 10, 3);
 * app.render();
 * EXPECT_EQ(app.plaintext(), "hello");
 * @endcode
 */
class NEFORCE_API headless {
public:
    /**
     * @brief 构造并挂载根组件
     * @param root 根组件指针
     * @param width 终端列数
     * @param height 终端行数
     */
    headless(component_base* root, int width, int height);

    ~headless();

    headless(const headless&) = delete;
    headless& operator=(const headless&) = delete;
    headless(headless&&) = delete;
    headless& operator=(headless&&) = delete;

    /**
     * @brief 设置主题
     * @param t 主题
     */
    void set_theme(const theme& t);

    /**
     * @brief 改变终端尺寸
     * @param width 列数
     * @param height 行数
     */
    void resize(int width, int height);

    /**
     * @brief 注入原始输入字节
     * @param bytes 字节流，按 `input_parser` 规则解析
     *
     * 支持可打印字符、控制键、CSI/SS3 序列、SGR 鼠标与括号粘贴
     */
    void feed(string_view bytes);

    /**
     * @brief 投递一个键盘事件
     * @param e 键盘事件
     */
    void feed_key(const key_event& e);

    /**
     * @brief 处理排队中的状态更新
     * @param max_rounds 最大轮数，防止意外死循环
     * @return 实际处理的事件数
     */
    size_t pump(size_t max_rounds = 32);

    /**
     * @brief 立即渲染一帧
     */
    void render();

    /**
     * @brief 处理排队更新并渲染一帧
     */
    void tick();

    /**
     * @brief 获取最近一帧的纯文本
     * @return 纯文本
     */
    NEFORCE_NODISCARD string plaintext() const;

    /**
     * @brief 获取最近一帧的 ANSI 输出
     * @return 最近一次 flush 写入的 ANSI 字符串
     */
    NEFORCE_NODISCARD const string& ansi() const noexcept { return ansi_; }

    /**
     * @brief 获取帧缓冲
     * @return 帧缓冲引用
     */
    NEFORCE_NODISCARD const screen& frame() const noexcept;

    /**
     * @brief 获取渲染管线
     * @return reconciler 引用
     */
    NEFORCE_NODISCARD reconciler& pipeline() noexcept { return *reconciler_; }

    /**
     * @brief 获取渲染管线
     * @return reconciler 常量引用
     */
    NEFORCE_NODISCARD const reconciler& pipeline() const noexcept { return *reconciler_; }

    /**
     * @brief 获取终端宽度
     * @return 列数
     */
    NEFORCE_NODISCARD int width() const noexcept { return width_; }

    /**
     * @brief 获取终端高度
     * @return 行数
     */
    NEFORCE_NODISCARD int height() const noexcept { return height_; }

private:
    io_context ctx_;
    strand strand_{ctx_};
    unique_ptr<reconciler> reconciler_;
    input_parser parser_;
    string ansi_;
    component_base* root_;
    int width_;
    int height_;
};

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_HEADLESS_HPP__
