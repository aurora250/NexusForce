#ifndef MSTL_CORE_SYSTEM_PROCESS_HPP__
#define MSTL_CORE_SYSTEM_PROCESS_HPP__

/**
 * @file process.hpp
 * @brief 进程管理工具
 *
 * 此文件提供了跨平台的进程创建、管理和监控功能。
 * 支持进程创建、等待、终止、挂起、恢复等操作。
 */

#include "MSTL/core/container/vector.hpp"
#include "MSTL/core/string/string.hpp"
#ifdef MSTL_PLATFORM_LINUX__
#include <fcntl.h>
#endif
#ifdef MSTL_PLATFORM_WINDOWS__
#include <Windows.h>
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#endif
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup Process 进程
 * @brief 进程管理工具
 * @{
 */

/**
 * @struct process_times
 * @brief 进程时间信息
 *
 * 包含进程的用户态、内核态和墙上时间。
 */
struct process_times {
    uint64_t user_time_ms;   ///< 用户态时间（毫秒）
    uint64_t kernel_time_ms; ///< 内核态时间（毫秒）
    uint64_t wall_time_ms;   ///< 墙上时间（毫秒）
};

/**
 * @struct process_memory_info
 * @brief 进程内存信息
 *
 * 包含进程的工作集和页面文件使用情况。
 */
struct process_memory_info {
    size_t working_set_size;      ///< 当前工作集大小
    size_t peak_working_set_size; ///< 峰值工作集大小
    size_t pagefile_usage;        ///< 页面文件使用量
    size_t peak_pagefile_usage;   ///< 页面文件使用峰值
};

/**
 * @enum process_state
 * @brief 进程状态枚举
 */
enum class process_state {
    running,    ///< 进程正在运行
    suspended,  ///< 进程被挂起
    stopped,    ///< 进程被停止
    exited,     ///< 进程已退出
    unknown     ///< 未知状态
};

/**
 * @enum process_permission
 * @brief 进程权限枚举
 *
 * 用于检查对进程的访问权限。
 */
enum class process_permission {
    read = 0x01,      ///< 读取权限
    write = 0x02,     ///< 写入权限
    execute = 0x04,   ///< 执行权限
    terminate = 0x08, ///< 终止权限
    query_info = 0x10, ///< 查询信息权限
    all = 0xFF        ///< 所有权限
};


/**
 * @class process
 * @brief 进程管理类
 *
 * 提供静态方法用于创建和管理进程。
 */
class MSTL_API process {
public:
    /**
     * @brief 进程ID类型
     */
    using process_id_t =
#ifdef MSTL_PLATFORM_WINDOWS__
        ::DWORD;
#else
        ::pid_t;
#endif

    /**
     * @struct process_info
     * @brief 进程信息结构
     *
     * 包含进程的标识符、状态和输出信息。
     * 不同平台包含不同的实现细节。
     */
    struct process_info {
        process_id_t process_id;  ///< 进程ID

#ifdef MSTL_PLATFORM_WINDOWS__
        ::PROCESS_INFORMATION pi;      ///< Windows进程信息
        ::HANDLE hStdoutRead;          ///< 标准输出读取句柄
        ::HANDLE hStdoutWrite;         ///< 标准输出写入句柄
#else
        int stdout_fd[2];              ///< 标准输出管道文件描述符 [0]读，[1]写
#endif

        bool is_running;               ///< 进程是否正在运行
        string stdout_output;          ///< 捕获的标准输出内容
    };

    /**
     * @brief 创建新进程
     * @param executable 可执行文件路径
     * @param args 命令行参数列表
     * @param capture_output 是否捕获标准输出
     * @return 进程信息结构
     * @throws system_exception 创建失败时抛出
     */
    static process_info create_process(
        const string& executable,
        const vector<string>& args = {},
        bool capture_output = false);

    /**
     * @brief 等待进程结束
     * @param info 进程信息
     * @param timeout_ms 超时时间（毫秒），-1表示无限等待
     * @return 进程退出码，-1表示超时或错误
     * @throws system_exception 等待失败时抛出
     */
    static int wait_for_process(process_info& info, int timeout_ms = -1);

    /**
     * @brief 终止进程
     * @param info 进程信息
     * @return 是否成功终止
     */
    static bool terminate_process(const process_info& info) noexcept;

    /**
     * @brief 挂起进程
     * @param info 进程信息
     * @return 是否成功挂起
     */
    static bool suspend_process(const process_info& info) noexcept;

    /**
     * @brief 恢复进程
     * @param info 进程信息
     * @return 是否成功恢复
     */
    static bool resume_process(const process_info& info) noexcept;

    /**
     * @brief 检查进程是否正在运行
     * @param info 进程信息
     * @return 是否正在运行
     */
    static bool is_process_running(const process_info& info) noexcept;

    /**
     * @brief 获取当前进程ID
     * @return 当前进程ID
     */
    static process_id_t current_process_id() noexcept;

    /**
     * @brief 获取进程内存信息
     * @param info 进程信息
     * @return 内存信息结构
     */
    static process_memory_info get_process_memory_info(const process_info& info) noexcept;

    /**
     * @brief 获取进程状态
     * @param info 进程信息
     * @return 进程状态枚举值
     */
    static process_state get_process_state(const process_info& info) noexcept;

    /**
     * @brief 检查进程权限
     * @param info 进程信息
     * @param permission 要检查的权限
     * @return 是否拥有指定权限
     */
    static bool check_process_permission(const process_info& info, process_permission permission) noexcept;

    /**
     * @brief 根据进程ID获取进程名称
     * @param process_id 进程ID
     * @return 进程名称，失败返回空字符串
     */
    static string get_process_name(process_id_t process_id) noexcept;

    /**
     * @brief 根据名称查找进程
     * @param name 进程名称
     * @return 匹配的进程信息列表
     */
    static vector<process_info> find_processes_by_name(const string& name);

    /**
     * @brief 获取指定进程的子进程列表
     * @param parent_info 父进程信息
     * @return 子进程信息列表
     */
    static vector<process_info> get_child_processes(const process_info& parent_info);
};

/** @} */ // Process

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_SYSTEM_PROCESS_HPP__
