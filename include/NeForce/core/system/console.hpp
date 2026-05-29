#ifndef NEFORCE_CORE_SYSTEM_CONSOLE_HPP__
#define NEFORCE_CORE_SYSTEM_CONSOLE_HPP__

/**
 * @file console.hpp
 * @brief 控制台输入输出工具
 */

#include "NeForce/core/async/mutex.hpp"
#include "NeForce/core/time/duration.hpp"
#include "NeForce/core/utility/color.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Exceptions 异常类集
 * @brief 异常类集
 * @{
 */

/**
 * @struct console_exception
 * @brief 控制台行为异常
 */
struct console_exception final : device_exception {
    explicit console_exception(const char* info = "Pipe Operation Failed.", const char* type = static_type,
                               const int code = 0) noexcept :
    device_exception(info, type, code) {}

    explicit console_exception(const exception& e) :
    device_exception(e) {}

    ~console_exception() override = default;
    static constexpr auto static_type = "console_exception";
};

/** @} */ // Exceptions

/**
 * @defgroup ConsoleIO 控制台IO
 * @brief 控制台输入输出工具
 * @{
 */

/**
 * @class sys_console
 * @brief 系统控制台单例类
 *
 * 提供了丰富的控制台输入输出功能，包括：
 * - 彩色文本输出
 * - 光标控制
 * - 进度条显示
 * - 密码输入
 * - 打字机效果
 * - 淡入淡出效果
 * - 终端信息查询
 *
 * 所有方法都是线程安全的。
 */
class NEFORCE_API sys_console {
public:
    /**
     * @struct console_size
     * @brief 控制台尺寸结构
     */
    struct console_size {
        int width;  ///< 宽度（列数）
        int height; ///< 高度（行数）

        explicit console_size(const int w = 0, const int h = 0) :
        width(w),
        height(h) {}

        /**
         * @brief 相等比较
         */
        NEFORCE_NODISCARD bool operator==(const console_size& other) const noexcept {
            return width == other.width && height == other.height;
        }

        /**
         * @brief 不等比较
         */
        NEFORCE_NODISCARD bool operator!=(const console_size& other) const noexcept { return !(*this == other); }
    };

private:
    native_handle_type out_; ///< 标准输出句柄
    native_handle_type in_;  ///< 标准输入句柄
    native_handle_type err_; ///< 标准错误句柄

#ifdef NEFORCE_PLATFORM_WINDOWS
    console_size saved_cursor_pos_{0, 0}; ///< 保存的光标位置
#endif

    mutable mutex mutex_;          ///< 互斥锁
    console_size last_size_{0, 0}; ///< 上次记录的控制台尺寸

    bool alt_buffer_active_ = false; // 交替缓冲区是否激活
    bool mouse_enabled_ = false;     // 鼠标输入是否开启
    int pending_char_ = -1;          // kbhit 预读的字符，-1 表示无待处理字符

private:
    void print_string_unsafe(const string& str) const { print_string_unsafe(str.view()); }
    void print_string_unsafe(string_view str) const;
    void print_string_unsafe(const char* str) const { print_string_unsafe(string_view{str}); }

    void print_error_unsafe(const string& str) const { print_error_unsafe(str.view()); }
    void print_error_unsafe(string_view str) const;
    void print_error_unsafe(const char* str) const { print_error_unsafe(string_view{str}); }

    void set_color_unsafe(const color& color, bool use_256_color) const;
    void typewriter_print_unsafe(string_view text, milliseconds delay_per_char, bool with_sound) const;

    string readln_unsafe() const;
    string read_unsafe() const;
    char read_char_unsafe() const;
    console_size get_console_size_unsafe() const;

    void flush_unsafe() const;
    void ignore_unsafe() const;

    void beep_unsafe() const;
    void flash_screen_unsafe() const;

    void fade_effect_unsafe(string_view text, const color& from, const color& to, milliseconds duration,
                            bool is_fade_in) const;

private:
    sys_console() noexcept;

public:
    sys_console(const sys_console&) = delete;
    sys_console& operator=(const sys_console&) = delete;
    sys_console(sys_console&&) = delete;
    sys_console& operator=(sys_console&&) = delete;

    ~sys_console() = default;

    /**
     * @brief 获取单例实例
     * @return 控制台实例引用
     */
    static sys_console& instance() noexcept {
        static sys_console console;
        return console;
    }

    /**
     * @brief 刷新输出缓冲区
     */
    void flush();

    /**
     * @brief 抛弃输入缓冲区中的一行
     */
    void ignore();

    /**
     * @brief 打印字符串
     * @param str 要打印的字符串
     */
    void print_string(const string& str);

    /**
     * @brief 打印字符串视图
     * @param view 要打印的字符串
     */
    void print_string(const string_view& view);

    /**
     * @brief 打印C风格字符串
     * @param str 要打印的字符串
     */
    void print_string(const char* str);

    /**
     * @brief 打印字符串到错误流
     * @param str 要打印的字符串
     */
    void print_error(const string& str);

    /**
     * @brief 打印字符串视图到错误流
     * @param view 要打印的字符串视图
     */
    void print_error(const string_view& view);

    /**
     * @brief 打印C风格字符串到错误流
     * @param str 要打印的字符串
     */
    void print_error(const char* str);

    /**
     * @brief 读取输入（直到空白字符）
     * @return 读取的字符串
     */
    string read();

    /**
     * @brief 读取一行输入
     * @return 读取的行
     */
    string readln();

    /**
     * @brief 读取单个字符
     * @return 读取的字符
     */
    char read_char();

    /**
     * @brief 打印任意类型的值
     * @tparam Args 参数类型
     * @param args 打印参数
     */
    template <typename... Args>
    void print(Args&&... args) {
        lock<mutex> lock(mutex_);
        this->print_string_unsafe(_NEFORCE to_string(_NEFORCE forward<Args>(args)...));
    }

    /**
     * @brief 格式化打印
     * @tparam Args 参数类型
     * @param fmt 格式字符串
     * @param args 格式化参数
     */
    template <typename... Args>
    void printf(const string_view fmt, Args&&... args) {
        lock<mutex> lock(mutex_);
        this->print_string_unsafe(_NEFORCE format(fmt, _NEFORCE forward<Args>(args)...));
    }

    /**
     * @brief 带颜色的打印
     * @tparam Args 参数类型
     * @param color 颜色
     * @param args 打印参数
     */
    template <typename... Args>
    void printc(const color& color, Args&&... args) {
        lock<mutex> lock(mutex_);
        this->set_color_unsafe(color, false);
        this->print_string_unsafe(_NEFORCE to_string(_NEFORCE forward<Args>(args)...));
        this->print_string_unsafe("\033[0m");
    }

    /**
     * @brief 带颜色的格式化打印
     * @tparam Args 参数类型
     * @param color 颜色
     * @param fmt 格式字符串
     * @param args 打印参数
     */
    template <typename... Args>
    void printcf(const color& color, const string_view fmt, Args&&... args) {
        lock<mutex> lock(mutex_);
        this->set_color_unsafe(color, false);
        this->print_string_unsafe(_NEFORCE format(fmt, _NEFORCE forward<Args>(args)...));
        this->print_string_unsafe("\033[0m");
    }
    /**
     * @brief 打印换行
     */
    void println();

    /**
     * @brief 打印任意类型的值并换行
     * @tparam Args 参数类型
     * @param args 打印参数
     */
    template <typename... Args, enable_if_t<(sizeof...(Args) > 0), int> = 0>
    void println(Args&&... args) {
        lock<mutex> lock(mutex_);
        this->print_string_unsafe(_NEFORCE to_string(_NEFORCE forward<Args>(args)...));
        this->print_string_unsafe("\n");
    }

    /**
     * @brief 格式化打印并换行
     * @tparam Args 参数类型
     * @param fmt 格式字符串
     * @param args 格式化参数
     */
    template <typename... Args>
    void printfln(const string_view fmt, Args&&... args) {
        lock<mutex> lock(mutex_);
        this->print_string_unsafe(_NEFORCE format(fmt, _NEFORCE forward<Args>(args)...));
        this->print_string_unsafe("\n");
    }

    /**
     * @brief 带颜色的打印并换行
     * @tparam Args 参数类型
     * @param color 颜色
     * @param args 打印参数
     */
    template <typename... Args>
    void printcln(const color& color, Args&&... args) {
        lock<mutex> lock(mutex_);
        this->set_color_unsafe(color, false);
        this->print_string_unsafe(_NEFORCE to_string(_NEFORCE forward<Args>(args)...));
        this->print_string_unsafe("\033[0m\n");
    }

    /**
     * @brief 带颜色的格式化打印并换行
     * @tparam Args 参数类型
     * @param color 颜色
     * @param fmt 格式字符串
     * @param args 打印参数
     */
    template <typename... Args>
    void printcfln(const color& color, const string_view fmt, Args&&... args) {
        lock<mutex> lock(mutex_);
        this->set_color_unsafe(color, false);
        this->print_string_unsafe(_NEFORCE format(fmt, _NEFORCE forward<Args>(args)...));
        this->print_string_unsafe("\033[0m\n");
    }

    /**
     * @brief 打印任意类型的值到错误流
     * @tparam Args 参数类型
     * @param args 打印参数
     */
    template <typename... Args>
    void eprint(Args&&... args) {
        lock<mutex> lock(mutex_);
        this->print_error_unsafe(_NEFORCE to_string(_NEFORCE forward<Args>(args)...));
    }

    /**
     * @brief 格式化打印到错误流
     * @tparam Args 参数类型
     * @param fmt 格式字符串
     * @param args 格式化参数
     */
    template <typename... Args>
    void eprintf(const string_view fmt, Args&&... args) {
        lock<mutex> lock(mutex_);
        this->print_error_unsafe(_NEFORCE format(fmt, _NEFORCE forward<Args>(args)...));
    }

    /**
     * @brief 打印换行到错误流
     */
    void eprintln();

    /**
     * @brief 打印任意类型的值到错误流并换行
     * @tparam Args 参数类型
     * @param args 打印参数
     */
    template <typename... Args, enable_if_t<(sizeof...(Args) > 0), int> = 0>
    void eprintln(Args&&... args) {
        lock<mutex> lock(mutex_);
        this->print_error_unsafe(_NEFORCE to_string(_NEFORCE forward<Args>(args)...));
        this->print_error_unsafe("\n");
    }

    /**
     * @brief 格式化打印到错误流并换行
     * @tparam Args 参数类型
     * @param fmt 格式字符串
     * @param args 格式化参数
     */
    template <typename... Args>
    void eprintfln(const string_view fmt, Args&&... args) {
        lock<mutex> lock(mutex_);
        this->print_error_unsafe(_NEFORCE format(fmt, _NEFORCE forward<Args>(args)...));
        this->print_error_unsafe("\n");
    }

    /**
     * @brief 清空屏幕
     */
    void clear();

    /**
     * @brief 暂停并等待用户按键
     * @param msg 提示信息
     */
    void pause(string_view msg = "Press enter to continue...");

    /**
     * @brief 用户确认对话框
     * @param prompt 提示信息
     * @param yes 确认字符
     * @param no 取消字符
     * @return 是否确认
     */
    bool confirmation(string_view prompt = "Are you sure? (y/n): ", char yes = 'y', char no = 'n');

    /**
     * @brief 密码输入
     * @param prompt 提示信息
     * @param mask 掩码字符（'\0'表示不显示）
     * @param show_length 是否显示已输入长度
     * @return 输入的密码
     * @throws console_exception Windows中如果设置控制台模式失败 / Linux中如果进程被终止
     */
    string password(string_view prompt = "Password: ", char mask = '*', bool show_length = false);

    /**
     * @brief 设置颜色（使用ANSI代码）
     * @param color ANSI颜色代码
     */
    void set_color(const integer32& color);

    /**
     * @brief 设置颜色
     * @param color 颜色对象
     * @param use_256_color 是否使用256色模式
     */
    void set_color(const color& color, bool use_256_color = true);

    /**
     * @brief 设置背景色
     * @param color 颜色对象
     * @param use_256_color 是否使用256色模式
     */
    void set_background_color(const color& color, bool use_256_color = true);

    /**
     * @brief 重置颜色
     */
    void reset_color();

    /**
     * @brief 设置粗体
     * @param enable 是否启用粗体
     */
    void set_bold(bool enable = true);

    /**
     * @brief 设置下划线
     * @param enable 是否启用下划线
     */
    void set_underline(bool enable = true);

    /**
     * @brief 设置闪烁
     * @param enable 是否启用闪烁
     */
    void set_blink(bool enable = true);

    /**
     * @brief 设置反色（交换前景色与背景色）
     * @param enable 是否启用反色
     */
    void set_reverse(bool enable = true);

    /**
     * @brief 重置所有文本属性
     */
    void reset_text_attributes();

    /**
     * @brief 设置控制台窗口标题
     * @param title 标题字符串
     */
    void set_window_title(string_view title);

    /**
     * @brief 启用或禁用交替屏幕缓冲区
     * @param enable 是否启用交替屏幕缓冲区
     */
    void enable_alternate_screen_buffer(bool enable = true);

    /**
     * @brief 禁用交替屏幕缓冲区
     */
    void disable_alternate_screen_buffer();

    /**
     * @brief 设置滚动区域
     * @param top 滚动区域起始行（从 1 开始）
     * @param bottom 滚动区域结束行（从 1 开始）
     */
    void set_scroll_region(int top, int bottom);

    /**
     * @brief 重置滚动区域为整个屏幕
     */
    void reset_scroll_region();

    /**
     * @brief 启用或禁用鼠标输入捕获
     * @param enable 是否启用鼠标输入捕获
     * @note 鼠标事件序列以 ESC [< 开头，需通过 read() 或 getch() 自行解析
     */
    void enable_mouse(bool enable = true);

    /**
     * @brief 禁用鼠标输入捕获
     */
    void disable_mouse();

    /**
     * @brief 检测当前是否已启用鼠标输入
     * @return 是否已启用鼠标输入
     */
    NEFORCE_NODISCARD bool is_mouse_enabled() const;

    /**
     * @brief 检查是否有键盘输入可读（非阻塞）
     * @return 是否有键盘输入可读
     */
    NEFORCE_NODISCARD bool kbhit();

    /**
     * @brief 读取一个字符（无回显，若无按键则阻塞）
     * @return 读取的字符（可能为负值表示出错）
     */
    int getch();

    /**
     * @brief 显示进度条
     * @param percentage 进度百分比（0.0-100.0或0.0-1.0）
     * @param width 进度条宽度
     * @param show_percentage 是否显示百分比
     * @param fill_char 填充字符
     * @param empty_char 空白字符
     */
    void progress_bar(double percentage, int width = 50, bool show_percentage = true, char fill_char = '#',
                      char empty_char = ' ');

    /**
     * @brief 设置光标位置
     * @param row 行（从1开始）
     * @param column 列（从1开始）
     */
    void set_cursor_position(int row, int column);

    /**
     * @brief 保存光标位置
     */
    void save_cursor_position();

    /**
     * @brief 恢复光标位置
     */
    void restore_cursor_position();

    /**
     * @brief 隐藏光标
     */
    void hide_cursor();

    /**
     * @brief 显示光标
     */
    void show_cursor();

    /**
     * @brief 获取控制台尺寸
     * @return 控制台尺寸
     */
    NEFORCE_NODISCARD console_size get_console_size() const;

    /**
     * @brief 检查终端是否已调整大小
     * @return 如果大小发生变化返回true
     */
    NEFORCE_NODISCARD bool is_terminal_resized();

    /**
     * @brief 检查是否支持颜色
     * @return 是否支持
     */
    NEFORCE_NODISCARD bool supports_colors() const;

    /**
     * @brief 检查是否支持真彩色
     * @return 是否支持
     */
    NEFORCE_NODISCARD bool supports_truecolor() const;

    /**
     * @brief 检查是否支持Unicode
     * @return 是否支持
     */
    NEFORCE_NODISCARD bool supports_unicode() const;

    /**
     * @brief 检查是否是交互式终端
     * @return 是否是交互式
     */
    NEFORCE_NODISCARD bool is_interactive() const;

    /**
     * @brief 获取终端类型
     * @return 终端类型字符串
     */
    NEFORCE_NODISCARD string console_type() const;

    /**
     * @brief 打字机效果打印
     * @param text 要打印的文本
     * @param delay_per_char 每个字符的延迟
     * @param with_sound 是否伴随声音
     */
    void typewriter_print(string_view text, milliseconds delay_per_char = milliseconds(50), bool with_sound = false);

    /**
     * @brief 打字机效果打印并换行
     * @param text 要打印的文本
     * @param delay_per_char 每个字符的延迟
     * @param with_sound 是否伴随声音
     */
    void typewriter_println(string_view text, milliseconds delay_per_char = milliseconds(50), bool with_sound = false);

    /**
     * @brief 发出蜂鸣声
     */
    void beep();

    /**
     * @brief 屏幕闪烁
     */
    void flash_screen();

    /**
     * @brief 显示通知
     * @param message 通知消息
     * @param duration 显示时长
     * @param play_sound 是否播放声音
     */
    void notification(string_view message, milliseconds duration = milliseconds(2000), bool play_sound = true);

    /**
     * @brief 淡入效果
     * @param text 文本
     * @param duration 持续时间
     * @param start_color 起始颜色
     * @param end_color 结束颜色
     */
    void fade_in(string_view text, milliseconds duration = milliseconds(1000),
                 const color& start_color = color::black(), const color& end_color = color::white());

    /**
     * @brief 淡出效果
     * @param text 文本
     * @param duration 持续时间
     * @param start_color 起始颜色
     * @param end_color 结束颜色
     */
    void fade_out(string_view text, milliseconds duration = milliseconds(1000),
                  const color& start_color = color::white(), const color& end_color = color::black());

    /**
     * @brief 淡入-保持-淡出效果
     * @param text 文本
     * @param in_duration 淡入时间
     * @param hold_duration 保持时间
     * @param out_duration 淡出时间
     */
    void fade_in_out(string_view text, milliseconds in_duration = milliseconds(500),
                     milliseconds hold_duration = milliseconds(1000), milliseconds out_duration = milliseconds(500));
};

/**
 * @brief 全局控制台实例
 */
NEFORCE_API extern sys_console& console;


/**
 * @brief 打印多个值
 */
template <typename... Args>
void print(Args&&... args) {
    console.print(_NEFORCE forward<Args>(args)...);
}

/**
 * @brief 带颜色打印多个值
 */
template <typename... Args>
void printc(const color& color, Args&&... args) {
    console.printc(color, _NEFORCE forward<Args>(args)...);
}

/**
 * @brief 打印多个值并换行
 */
template <typename... Args>
void println(Args&&... args) {
    console.println(_NEFORCE forward<Args>(args)...);
}

/**
 * @brief 带颜色打印多个值并换行
 */
template <typename... Args>
void printcln(const color& color, Args&&... args) {
    console.printcln(color, _NEFORCE forward<Args>(args)...);
}

/**
 * @brief 格式化打印
 */
template <typename... Args>
void printf(const string_view fmt, Args&&... args) {
    console.printf(fmt, _NEFORCE forward<Args>(args)...);
}

/**
 * @brief 格式化打印并换行
 */
template <typename... Args>
void printfln(const string_view fmt, Args&&... args) {
    console.printfln(fmt, _NEFORCE forward<Args>(args)...);
}

/**
 * @brief 格式化颜色打印
 */
template <typename... Args>
void printcf(const color& color, const string_view fmt, Args&&... args) {
    console.printcf(color, fmt, _NEFORCE forward<Args>(args)...);
}

/**
 * @brief 格式化颜色打印并换行
 */
template <typename... Args>
void printcfln(const color& color, const string_view fmt, Args&&... args) {
    console.printcfln(color, fmt, _NEFORCE forward<Args>(args)...);
}

/**
 * @brief 打印多个值到错误流
 */
template <typename... Args>
void eprint(Args&&... args) {
    console.eprint(_NEFORCE forward<Args>(args)...);
}

/**
 * @brief 格式化打印到错误流
 */
template <typename... Args>
void eprintf(const string_view fmt, Args&&... args) {
    console.eprintf(fmt, _NEFORCE forward<Args>(args)...);
}

/**
 * @brief 打印多个值到错误流并换行
 */
template <typename... Args>
void eprintln(Args&&... args) {
    console.eprintln(_NEFORCE forward<Args>(args)...);
}

/**
 * @brief 格式化打印到错误流并换行
 */
template <typename... Args>
void eprintfln(const string_view fmt, Args&&... args) {
    console.eprintfln(fmt, _NEFORCE forward<Args>(args)...);
}

/** @} */ // ConsoleIO

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SYSTEM_CONSOLE_HPP__
