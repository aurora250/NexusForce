#ifndef MSTL_CORE_SYSTEM_PROCESS_HPP__
#define MSTL_CORE_SYSTEM_PROCESS_HPP__
#include "../string/string.hpp"
#include "../container/vector.hpp"
#ifdef MSTL_PLATFORM_LINUX__
#include <fcntl.h>
#endif
#ifdef MSTL_PLATFORM_WINDOWS__
#include <Windows.h>
#include "../config/undef_cmacro.hpp"
#endif
MSTL_BEGIN_NAMESPACE__

struct process_times {
    uint64_t user_time_ms;
    uint64_t kernel_time_ms;
    uint64_t wall_time_ms;
};

struct process_memory_info {
    size_t working_set_size;
    size_t peak_working_set_size;
    size_t pagefile_usage;
    size_t peak_pagefile_usage;
};

enum class process_state {
    running,    // 进程正在运行
    suspended,  // 进程被挂起
    stopped,    // 进程被停止
    exited,     // 进程已退出
    unknown     // 未知状态
};

enum class process_permission {
    read = 0x01,
    write = 0x02,
    execute = 0x04,
    terminate = 0x08,
    query_info = 0x10,
    all = 0xFF
};


class MSTL_API process {
public:
#ifdef MSTL_PLATFORM_WINDOWS__
    using process_id_t = ::DWORD;
#else
    using process_id_t = ::pid_t;
#endif

    struct process_info {
        process_id_t process_id;
#ifdef MSTL_PLATFORM_WINDOWS__
        ::PROCESS_INFORMATION pi;
        ::HANDLE hStdoutRead;
        ::HANDLE hStdoutWrite;
#else
        int stdout_fd[2];  // [0] read，[1] write
#endif
        bool is_running;
        string stdout_output;
    };


    static process_info create_process(
        const string& executable,
        const vector<string>& args = {},
        bool capture_output = false);

    static int wait_for_process(process_info& info, int timeout_ms = -1);

    static bool terminate_process(const process_info& info) noexcept;
    static bool suspend_process(const process_info& info) noexcept;
    static bool resume_process(const process_info& info) noexcept;

    static bool is_process_running(const process_info& info) noexcept;

    static process_id_t current_process_id() noexcept;

    static process_memory_info get_process_memory_info(const process_info& info) noexcept;
    static process_state get_process_state(const process_info& info) noexcept;

    static bool check_process_permission(
        const process_info& info,
        process_permission permission) noexcept;

    static string get_process_name(process_id_t process_id) noexcept;

    static vector<process_info> find_processes_by_name(const string& name);
    static vector<process_info> get_child_processes(const process_info& parent_info);
};

class MSTL_API process_group {
public:
    process_group() = default;
    ~process_group() = default;

    void add_process(process::process_info info);
    bool terminate_all();
    bool wait_all(int timeout_ms = -1);

private:
    vector<process::process_info> processes;
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_SYSTEM_PROCESS_HPP__
