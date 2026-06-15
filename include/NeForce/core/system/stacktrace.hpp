#ifndef NEFORCE_CORE_SYSTEM_STACKTRACE_HPP__
#define NEFORCE_CORE_SYSTEM_STACKTRACE_HPP__

/**
 * @file stacktrace.hpp
 * @brief 堆栈跟踪工具
 *
 * 此文件提供了获取和显示函数调用堆栈的功能，用于调试和错误诊断。
 * 支持跨平台获取当前调用堆栈，并解析函数符号名称。
 */

#include "NeForce/core/interface/istringify.hpp"
#include "NeForce/core/exception/exception_ptr.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Stacktrace 堆栈跟踪
 * @brief 堆栈跟踪工具，用于调试和错误诊断
 * @{
 */

/**
 * @class stacktrace
 * @brief 堆栈跟踪类
 *
 * 捕获当前线程的调用堆栈，并提供对堆栈帧的访问。
 * 支持符号名称解析和格式化的堆栈输出。
 */
class NEFORCE_API stacktrace : public istringify<stacktrace> {
public:
    /**
     * @class frame
     * @brief 堆栈帧类
     *
     * 表示堆栈中的一个函数调用帧，包含返回地址和符号信息。
     */
    class frame : public istringify<frame> {
    private:
        void* address_ = nullptr; ///< 返回地址

    public:
        /**
         * @brief 默认构造函数
         */
        frame() noexcept = default;

        /**
         * @brief 从地址构造堆栈帧
         * @param addr 返回地址
         */
        explicit frame(void* addr) noexcept :
        address_(addr) {}

        /**
         * @brief 获取返回地址
         * @return 返回地址指针
         */
        NEFORCE_NODISCARD void* address() const noexcept { return address_; }

        /**
         * @brief 获取函数符号名称
         * @return 解析后的函数名称
         *
         * 解析还原符号名称。
         */
        NEFORCE_NODISCARD string name() const;

        /**
         * @brief 获取源文件路径
         * @return 源文件路径，不可用时返回空字符串
         */
        NEFORCE_NODISCARD string source_file() const;

        /**
         * @brief 获取源文件行号
         * @return 行号，不可用时返回 0
         */
        NEFORCE_NODISCARD size_t source_line() const;

        /**
         * @brief 相等比较
         * @param other 另一个堆栈帧
         * @return 是否相等
         */
        NEFORCE_NODISCARD bool operator==(const frame& other) const noexcept { return address_ == other.address_; }

        /**
         * @brief 不等比较
         * @param other 另一个堆栈帧
         * @return 是否不等
         */
        NEFORCE_NODISCARD bool operator!=(const frame& other) const noexcept { return !(*this == other); }

        /**
         * @brief 转换为字符串
         * @return 格式化的堆栈帧信息
         *
         * 格式：地址 [in 函数名]
         */
        NEFORCE_NODISCARD string to_string() const;
    };

    /**
     * @brief 格式化标志
     */
    enum format_flags {
        FMT_DEFAULT = 0,     /**< 默认格式 */
        FMT_SHOW_SOURCE = 1, /**< 显示源文件位置信息 */
        FMT_NO_ADDRESS = 2,  /**< 不显示地址 */
    };

    /**
     * @brief 格式化标志位或运算
     * @param a 左操作数
     * @param b 右操作数
     * @return 组合标志
     */
    friend constexpr format_flags operator|(format_flags a, format_flags b) noexcept {
        return static_cast<format_flags>(static_cast<int>(a) | static_cast<int>(b));
    }

    /**
     * @brief 格式化标志位与运算
     * @param a 左操作数
     * @param b 右操作数
     * @return 交集标志
     */
    friend constexpr format_flags operator&(format_flags a, format_flags b) noexcept {
        return static_cast<format_flags>(static_cast<int>(a) & static_cast<int>(b));
    }

private:
    vector<frame> frames_; ///< 堆栈帧列表

public:
    /**
     * @brief 构造函数，捕获当前堆栈
     * @param skip 要跳过的帧数（包括当前函数）
     * @param max_depth 最大捕获深度
     *
     * 捕获当前线程的调用堆栈，跳过指定数量的顶层帧。
     */
    explicit stacktrace(size_t skip = 0, size_t max_depth = 64);

    /**
     * @brief 获取当前线程堆栈
     * @param skip 要跳过的帧数
     * @param max_depth 最大捕获深度
     * @return 堆栈跟踪对象
     */
    NEFORCE_NODISCARD static stacktrace current(size_t skip = 0, size_t max_depth = 64);

    /**
     * @brief 从异常指针获取堆栈跟踪
     * @param ep 异常指针
     * @param max_depth 最大捕获深度
     * @return 堆栈跟踪对象
     *
     * 提取嵌套异常链中的堆栈信息。
     */
    NEFORCE_NODISCARD static stacktrace from_exception(const exception_ptr& ep, size_t max_depth = 64);

    /**
     * @brief 获取堆栈深度
     * @return 堆栈帧数量
     */
    NEFORCE_NODISCARD size_t size() const noexcept { return frames_.size(); }

    /**
     * @brief 检查堆栈是否为空
     * @return 是否为空
     */
    NEFORCE_NODISCARD bool empty() const noexcept { return frames_.empty(); }

    /**
     * @brief 常量索引访问
     * @param idx 索引位置
     * @return 对应位置的堆栈帧常量引用
     */
    NEFORCE_NODISCARD const frame& operator[](const size_t idx) const noexcept { return frames_[idx]; }

    /**
     * @brief 索引访问
     * @param idx 索引位置
     * @return 对应位置的堆栈帧引用
     */
    NEFORCE_NODISCARD frame& operator[](const size_t idx) noexcept { return frames_[idx]; }

    /**
     * @brief 获取起始常量迭代器
     * @return 常量迭代器
     */
    NEFORCE_NODISCARD auto begin() const noexcept { return frames_.begin(); }

    /**
     * @brief 获取结束常量迭代器
     * @return 常量迭代器
     */
    NEFORCE_NODISCARD auto end() const noexcept { return frames_.end(); }

    /**
     * @brief 获取起始常量迭代器
     * @return 常量迭代器
     */
    NEFORCE_NODISCARD auto cbegin() const noexcept { return frames_.cbegin(); }

    /**
     * @brief 获取结束常量迭代器
     * @return 常量迭代器
     */
    NEFORCE_NODISCARD auto cend() const noexcept { return frames_.cend(); }

    /**
     * @brief 转换为字符串
     * @return 格式化的完整堆栈信息
     *
     * 每行格式：#索引 地址 [in 函数名]
     */
    NEFORCE_NODISCARD string to_string() const;

    /**
     * @brief 以指定格式转换为字符串
     * @param flags 格式化标志
     * @return 格式化的完整堆栈信息
     */
    NEFORCE_NODISCARD string to_string(format_flags flags) const;
};

/** @} */ // Stacktrace

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SYSTEM_STACKTRACE_HPP__
