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
class Reconciler;
/// @endcond

/**
 * @brief 终端输入驱动
 *
 * 监听输入，解析为 KeyEvent 或 MouseEvent 后分发到 Reconciler。
 */
class InputDriver {
public:
    /**
     * @brief 构造函数
     * @param ctx 事件循环上下文
     * @param r Reconciler 引用
     * @param s 串行执行器
     */
    InputDriver(io_context& ctx, Reconciler& r, strand& s);
    ~InputDriver();

    InputDriver(const InputDriver&) = delete;
    InputDriver& operator=(const InputDriver&) = delete;
    InputDriver(InputDriver&&) = delete;
    InputDriver& operator=(InputDriver&&) = delete;

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
    static bool checkResizeFlag();

private:
#ifdef NEFORCE_PLATFORM_LINUX
    void drainStdin(int fd, uint32_t events, error_code ec);
#endif

    void processByte(byte_t byte);
    bool parseEscapeSequence();
    bool parseUtf8Sequence();
    bool parseMouseSequence();
    void dispatchKey(const KeyEvent& e);
    void dispatchMouse(const MouseEvent& e);
    void processAccumulatedBytes(const byte_t* data, size_t len);

    io_context& ctx_;
    Reconciler& reconiler_;
    strand& strand_;
    sys_console& console_;

    string accum_;             ///< 转义序列 / UTF-8 累积缓冲
    bool escActive_ = false;   ///< 是否在处理 ESC 序列
    bool mouseActive_ = false; ///< 是否在处理鼠标序列
    bool listening_ = false;   ///< 是否已启动监听

#ifdef NEFORCE_PLATFORM_LINUX
    bool termSaved_ = false; ///< 是否已保存原始终端属性
    ::termios oldTermios_;   ///< 原始终端属性
#elif defined(NEFORCE_PLATFORM_WINDOWS)
    void* stdinHandle_ = nullptr;
    void* inputThread_ = nullptr; ///< 后台输入线程句柄
    unsigned long oldInMode_ = 0; ///< 原始控制台输入模式
    bool termSaved_ = false;      ///< 是否已保存控制台模式
#endif
};

/** @} */ // TuiInput

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_INPUT_HPP__
