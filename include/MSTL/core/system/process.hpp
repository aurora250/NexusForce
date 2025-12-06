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

class process_creator {
public:
    struct process_info {
#ifdef MSTL_PLATFORM_WINDOWS__
        ::PROCESS_INFORMATION pi;
        ::DWORD process_id;
#else
        ::pid_t process_id;
#endif
        bool is_running;
    };

    static process_info create_process(const string& executable, const vector<string>& args = {});

    static int wait_for_process(const process_info& info, int timeout_ms = -1);

    static bool terminate_process(const process_info& info);

    static bool is_process_running(const process_info& info);

private:
#ifdef MSTL_PLATFORM_WINDOWS__
    static string build_command_line(const string& executable, const vector<string>& args);
#else
    static char** build_argv(const string& executable, const vector<string>& args);
    static void free_argv(char** argv);
#endif
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_SYSTEM_PROCESS_HPP__
