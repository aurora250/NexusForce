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

class process {
public:
    struct process_info {
#ifdef MSTL_PLATFORM_WINDOWS__
        ::PROCESS_INFORMATION pi;
        ::DWORD process_id;
        ::HANDLE hStdoutRead;
        ::HANDLE hStdoutWrite;
#else
        ::pid_t process_id;
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

    static bool is_process_running(const process_info& info) noexcept;
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_SYSTEM_PROCESS_HPP__
