#include <MSTL/core/system/process.hpp>
#include <MSTL/core/system/console.hpp>
#ifdef MSTL_PLATFORM_LINUX__
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <csignal>
#include <sys/wait.h>
#endif
MSTL_BEGIN_NAMESPACE__

#ifdef MSTL_PLATFORM_WINDOWS__
string process::build_command_line(
    const string& executable, const vector<string>& args) {
    string cmd_line = "\"" + executable + "\"";
    for (const auto& arg : args) {
        cmd_line += " \"" + arg + "\"";
    }
    return cmd_line;
}
#endif

process::process_info process::create_process(
    const string& executable, const vector<string>& args) {
    process_info info = {};
#ifdef MSTL_PLATFORM_WINDOWS__
    ::STARTUPINFOA si = {};
    si.cb = sizeof(si);

    string cmd_line = build_command_line(executable, args);

    vector<char> cmd_line_buf(cmd_line.begin(), cmd_line.end());
    cmd_line_buf.push_back('\0');

    ::BOOL success = ::CreateProcessA(
        nullptr, cmd_line_buf.data(),
        nullptr, nullptr, 0, 0,
        nullptr, nullptr, &si, &info.pi
    );

    if (!success) {
        throw_exception(value_exception("CreateProcess failed"));
    }
    info.process_id = info.pi.dwProcessId;
#else
    const ::pid_t pid = ::fork();
    if (pid < 0) {
        throw_exception(value_exception(::strerror(errno)));
    }

    if (pid == 0) {
        char** argv = build_argv(executable, args);
        ::execvp(executable.c_str(), argv);
        printcln(color::red(), "execvp failed: ", ::strerror(errno));
        free_argv(argv);
        ::_exit(1);
    }

    info.process_id = pid;
#endif
    info.is_running = true;
    return info;
}

int process::wait_for_process(const process_info& info, int timeout_ms) {
#ifdef MSTL_PLATFORM_WINDOWS__
    const ::DWORD timeout = (timeout_ms < 0) ?
        numeric_limits<uint32_t>::max() : static_cast<::DWORD>(timeout_ms);
    ::DWORD result = ::WaitForSingleObject(info.pi.hProcess, timeout);

    if (result == WAIT_TIMEOUT) {
        return -1;
    }
    if (result == WAIT_FAILED) {
        throw_exception(value_exception("WaitForSingleObject failed"));
    }

    ::DWORD exit_code;
    if (!::GetExitCodeProcess(info.pi.hProcess, &exit_code)) {
        throw_exception(value_exception("GetExitCodeProcess failed"));
    }
    return static_cast<int>(exit_code);
#else
    int status;

    if (timeout_ms < 0) {
        if (::waitpid(info.process_id, &status, 0) == -1) {
            throw_exception(value_exception(::strerror(errno)));
        }
    } else {
        int elapsed = 0;
        constexpr int sleep_interval = 100;

        while (elapsed < timeout_ms) {
            ::pid_t result = ::waitpid(info.process_id, &status, WNOHANG);
            if (result == -1) {
                throw_exception(value_exception(::strerror(errno)));
            }
            if (result > 0) break;

            ::usleep(sleep_interval * 1000);
            elapsed += sleep_interval;
        }

        if (elapsed >= timeout_ms) {
            return -1;
        }
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return -1;
#endif
}

bool process::terminate_process(const process_info& info) {
#ifdef MSTL_PLATFORM_WINDOWS__
    const BOOL result = ::TerminateProcess(info.pi.hProcess, 1);
    if (result) {
        ::CloseHandle(info.pi.hProcess);
        ::CloseHandle(info.pi.hThread);
    }
    return result != 0;
#else
    if (::kill(info.process_id, SIGTERM) == 0) {
        ::usleep(100000);
        if (is_process_running(info)) {
            ::kill(info.process_id, SIGKILL);
        }
        return true;
    }
    return false;
#endif
}

bool process::is_process_running(const process_info& info) {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::DWORD exit_code;
    if (::GetExitCodeProcess(info.pi.hProcess, &exit_code)) {
        return exit_code == STILL_ACTIVE;
    }
    return false;
#else
    int status;
    return ::waitpid(info.process_id, &status, WNOHANG) == 0;
#endif
}

#ifdef MSTL_PLATFORM_LINUX__

char** process::build_argv(
    const string& executable, const vector<string>& args) {
    const size_t argc = args.size() + 2;
    const auto argv = new char*[argc];

    argv[0] = new char[executable.length() + 1];
    _MSTL string_copy(argv[0], executable.c_str());

    for (size_t i = 0; i < args.size(); ++i) {
        argv[i + 1] = new char[args[i].length() + 1];
        _MSTL string_copy(argv[i + 1], args[i].c_str());
    }

    argv[argc - 1] = nullptr;
    return argv;
}

void process::free_argv(char** argv) {
    if (argv) {
        for (int i = 0; argv[i] != nullptr; ++i)
            delete[] argv[i];
        delete[] argv;
    }
}

#endif

MSTL_END_NAMESPACE__
