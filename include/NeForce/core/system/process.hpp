#ifndef NEFORCE_CORE_SYSTEM_PROCESS_HPP__
#define NEFORCE_CORE_SYSTEM_PROCESS_HPP__

/**
 * @file process.hpp
 * @brief 进程管理工具
 *
 * 此文件提供了跨平台的进程创建、管理和监控功能。
 * 支持进程创建、等待、终止、挂起、恢复等操作。
 */

#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/system/pipe.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Exceptions 异常类集
 * @brief 异常类集
 * @{
 */

/**
 * @struct process_exception
 * @brief 进程操作异常
 */
struct process_exception final : system_exception {
    explicit process_exception(
        const char* info = "Process Operation Failed.", const char* type = static_type,
        const int code = 0) noexcept : system_exception(info, type, code) {}

    explicit process_exception(const exception& e)
    : system_exception(e) {}

    ~process_exception() override = default;
    static constexpr auto static_type = "process_exception";
};

/** @} */ // Exceptions

/**
 * @defgroup Process 进程
 * @brief 进程管理工具
 * @{
 */

/**
 * @class process
 * @brief 进程管理类
 *
 * 提供静态方法用于创建和管理进程。
 */
class NEFORCE_API process {
public:
    /**
     * @brief 进程ID类型
     */
    using native_id_type =
#ifdef NEFORCE_PLATFORM_WINDOWS
        unsigned long;
#else
        int;
#endif

    /**
     * @struct state_info
     * @brief 进程信息结构
     *
     * 包含进程的标识符、状态和输出信息。
     * 不同平台包含不同的实现细节。
     */
    struct state_info {
        native_id_type process_id;  ///< 进程ID

#ifdef NEFORCE_PLATFORM_WINDOWS
        native_id_type thread_id;   ///< 主线程ID
        void* process_handle;       ///< 进程句柄
        void* thread_handle;        ///< 主线程句柄
#endif

        pipe stdout_pipe;           ///< 标准输出管道
        bool is_running;            ///< 进程是否正在运行
        string stdout_output;       ///< 捕获的标准输出内容
    };

    /**
     * @struct memory_info
     * @brief 进程内存信息
     *
     * 包含进程的工作集和页面文件使用情况。
     */
    struct memory_info {
        size_t working_set_size;      ///< 当前工作集大小
        size_t peak_working_set_size; ///< 峰值工作集大小
        size_t pagefile_usage;        ///< 页面文件使用量
        size_t peak_pagefile_usage;   ///< 页面文件使用峰值
    };

    /**
     * @struct time_info
     * @brief 进程时间信息
     *
     * 包含进程的用户态、内核态和墙上时间。
     */
    struct time_info {
        uint64_t user_time_ms;   ///< 用户态时间（毫秒）
        uint64_t kernel_time_ms; ///< 内核态时间（毫秒）
        uint64_t wall_time_ms;   ///< 墙上时间（毫秒）
    };

    /**
     * @enum permission
     * @brief 进程权限枚举
     *
     * 用于检查对进程的访问权限。
     */
    enum class permission {
        read = 0x01,        ///< 读取权限
        write = 0x02,       ///< 写入权限
        execute = 0x04,     ///< 执行权限
        terminate = 0x08,   ///< 终止权限
        query_info = 0x10,  ///< 查询信息权限
        all = 0xFF          ///< 所有权限
    };

    /**
     * @enum state
     * @brief 进程状态枚举
     */
    enum class state {
        running,    ///< 进程正在运行
        suspended,  ///< 进程被挂起
        stopped,    ///< 进程被停止
        exited,     ///< 进程已退出
        unknown     ///< 未知状态
    };

    /**
     * @brief 创建新进程
     * @param executable 可执行文件路径
     * @param args 命令行参数列表
     * @param capture_output 是否捕获标准输出
     * @return 进程信息结构
     * @throws process_exception 创建失败时抛出
     */
    static state_info create(const string& executable, const vector<string>& args = {}, bool capture_output = false);

    /**
     * @brief 等待进程结束
     * @param info 进程信息
     * @param timeout_ms 超时时间（毫秒），-1表示无限等待
     * @return 进程退出码，-1表示超时或错误
     * @throws process_exception 等待失败时抛出
     */
    static int wait_for(state_info& info, int timeout_ms = -1);

    /**
     * @brief 终止进程
     * @param info 进程信息
     * @return 是否成功终止
     */
    static bool terminate(const state_info& info) noexcept;

    /**
     * @brief 挂起进程
     * @param info 进程信息
     * @return 是否成功挂起
     */
    static bool suspend(const state_info& info) noexcept;

    /**
     * @brief 恢复进程
     * @param info 进程信息
     * @return 是否成功恢复
     */
    static bool resume(const state_info& info) noexcept;

    /**
     * @brief 检查进程是否正在运行
     * @param info 进程信息
     * @return 是否正在运行
     */
    static bool is_running(const state_info& info) noexcept;

    /**
     * @brief 获取当前进程ID
     * @return 当前进程ID
     */
    static native_id_type current_id() noexcept;

    /**
     * @brief 获取进程内存信息
     * @param info 进程信息
     * @return 内存信息结构
     */
    static memory_info get_memory_info(const state_info& info) noexcept;

    /**
     * @brief 获取进程状态
     * @param info 进程信息
     * @return 进程状态枚举值
     */
    static state get_state(const state_info& info) noexcept;

    /**
     * @brief 检查进程权限
     * @param info 进程信息
     * @param permission 要检查的权限
     * @return 是否拥有指定权限
     */
    static bool check_permission(const state_info& info, permission permission) noexcept;

    /**
     * @brief 根据进程ID获取进程名称
     * @param process_id 进程ID
     * @return 进程名称，失败返回空字符串
     */
    static string name(native_id_type process_id) noexcept;
};

/** @} */ // Process

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SYSTEM_PROCESS_HPP__
