#ifndef NEFORCE_TUI_INPUT_HPP__
#define NEFORCE_TUI_INPUT_HPP__

/**
 * @file input.hpp
 * @brief 终端输入驱动
 *
 * 解析 ANSI 转义序列和 UTF-8 多字节序列，将输入事件分发到 Reconciler。
 */

#include "NeForce/core/async/strand.hpp"
#include "NeForce/core/system/console.hpp"
#include "NeForce/tui/events.hpp"
#include "NeForce/tui/input_parser.hpp"
#ifdef NEFORCE_PLATFORM_LINUX
#    include <termios.h>
#endif
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/// @cond
class reconciler;
/// @endcond

/**
 * @brief 终端输入驱动
 *
 * 监听输入，交由 input_parser 解析为按键 / 鼠标 / 粘贴事件后分发到 reconciler。
 */
class NEFORCE_API input_driver {
public:
    /**
     * @brief 构造函数
     * @param ctx 事件循环上下文
     * @param r reconciler 引用
     * @param s 串行执行器
     */
    input_driver(io_context& ctx, reconciler& r, strand& s);
    ~input_driver();

    input_driver(const input_driver&) = delete;
    input_driver& operator=(const input_driver&) = delete;
    input_driver(input_driver&&) = delete;
    input_driver& operator=(input_driver&&) = delete;

    /**
     * @brief 启动 stdin 监听
     */
    void start();

    /**
     * @brief 停止 stdin 监听
     */
    void stop();

    /**
     * @brief 检查并清除终端尺寸变更标记
     * @return true 表示终端尺寸已改变
     */
    static bool check_resize_flag();

private:
#ifdef NEFORCE_PLATFORM_LINUX
    void drain_stdin(int fd, epoll_event events, error_code ec);
#else
    static unsigned long WINAPI input_thread_proc(void* selfp);
#endif

    void process_byte(byte_t byte);
    void process_accumulated_bytes(const byte_t* data, size_t len);
    void deliver(const parsed_input& input);
    void dispatch_key(const key_event& e);
    void dispatch_mouse(const mouse_event& e);
    void dispatch_paste(const paste_event& e);
    void dispatch_focus(const parsed_input& input);

    io_context& ctx_;
    reconciler& reconiler_;
    strand& strand_;
    sys_console& console_;

    input_parser parser_;    ///< 输入序列解析器
    bool listening_ = false; ///< 是否已启动监听

    bool term_saved_ = false; ///< 是否已保存原始终端属性
#ifdef NEFORCE_PLATFORM_LINUX
    ::termios old_termios_{}; ///< 原始终端属性
#elif defined(NEFORCE_PLATFORM_WINDOWS)
    void* stdin_handle_ = nullptr;
    void* input_thread_ = nullptr;  ///< 后台输入线程句柄
    unsigned long old_in_mode_ = 0; ///< 原始控制台输入模式
#endif
};

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_INPUT_HPP__
