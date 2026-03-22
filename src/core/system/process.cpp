#include <NeForce/core/system/process.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#include <NeForce/core/config/windef.hpp>
#include <windef.h>
#include <WinBase.h>
#include <Psapi.h>
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#include <NeForce/core/system/console.hpp>
#include <NeForce/core/file/file.hpp>
#include <sys/wait.h>
#include <cerrno>
#include <cstring>
#include <csignal>
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
#ifdef NEFORCE_PLATFORM_WINDOWS
    string build_command_line(const string& executable, const vector<string>& args) {
        string cmd_line = "\"" + executable + "\"";
        for (const auto& arg : args) {
            cmd_line += " \"" + arg + "\"";
        }
        return cmd_line;
    }
#endif

#ifdef NEFORCE_PLATFORM_LINUX
    char** build_argv(const string& executable, const vector<string>& args) {
        const size_t argc = args.size() + 2;
        const auto argv = new char*[argc];

        argv[0] = new char[executable.length() + 1];
        _NEFORCE string_copy(argv[0], executable.data());

        for (size_t i = 0; i < args.size(); ++i) {
            argv[i + 1] = new char[args[i].length() + 1];
            _NEFORCE string_copy(argv[i + 1], args[i].data());
        }

        argv[argc - 1] = nullptr;
        return argv;
    }
    void free_argv(char** argv) noexcept{
        if (argv) {
            for (int i = 0; argv[i] != nullptr; ++i)
                delete[] argv[i];
            delete[] argv;
        }
    }
#endif
}


process::state_info process::create(
    const string& executable, const vector<string>& args, bool capture_output) {
    state_info info{};

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::STARTUPINFOA si{};
    si.cb = sizeof(::STARTUPINFOA);

    if (capture_output) {
        info.stdout_pipe = pipe(true);

        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdOutput = info.stdout_pipe.native_write_handle();
        si.hStdError = info.stdout_pipe.native_write_handle();
    }

    string cmd_line = build_command_line(executable, args);

    ::PROCESS_INFORMATION pi;
    const ::BOOL success = ::CreateProcessA(
        nullptr, cmd_line.data(),
        nullptr, nullptr,
        capture_output ? TRUE : FALSE,
        0, nullptr, nullptr, &si, &pi
    );

    if (!success) {
        NEFORCE_THROW_EXCEPTION(process_exception("CreateProcess failed"));
    }

    info.process_handle = pi.hProcess;
    info.thread_handle = pi.hThread;
    info.process_id = pi.dwProcessId;
    info.thread_id = pi.dwThreadId;

    if (capture_output) {
        info.stdout_pipe.close_write();
    }
#else
    if (capture_output) {
        info.stdout_pipe = pipe(true);
    }

    const ::pid_t pid = ::fork();
    if (pid < 0) {
        NEFORCE_THROW_EXCEPTION(process_exception(::strerror(errno)));
    }

    if (pid == 0) {
        if (capture_output) {
            info.stdout_pipe.close_read();

            if (::dup2(info.stdout_pipe.native_write_handle(), STDOUT_FILENO) == -1) {
                printcln(color::red(), "dup2 stdout failed: ", ::strerror(errno));
                ::_exit(1);
            }

            if (::dup2(info.stdout_pipe.native_write_handle(), STDERR_FILENO) == -1) {
                printcln(color::red(), "dup2 stderr failed: ", ::strerror(errno));
                ::_exit(1);
            }

            info.stdout_pipe.close_write();
        }

        char** argv = build_argv(executable, args);
        ::execvp(executable.data(), argv);

        printcln(color::red(), "execvp failed: ", ::strerror(errno));
        free_argv(argv);
        ::_exit(1);
    }

    info.process_id = pid;
    if (capture_output) {
        info.stdout_pipe.close_write();
    }
#endif

    info.is_running = true;
    return info;
}

int process::wait_for(state_info& info, int timeout_ms) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::DWORD timeout = (timeout_ms < 0) ?
        numeric_traits<::DWORD>::max() :
        static_cast<::DWORD>(timeout_ms);
    ::DWORD result = ::WaitForSingleObject(info.process_handle, timeout);

    if (result == WAIT_TIMEOUT) {
        return -1;
    }
    if (result == WAIT_FAILED) {
        NEFORCE_THROW_EXCEPTION(process_exception("WaitForSingleObject failed"));
    }
    info.stdout_output = info.stdout_pipe.read_available();

    ::DWORD exit_code;
    if (!::GetExitCodeProcess(info.process_handle, &exit_code)) {
        NEFORCE_THROW_EXCEPTION(process_exception("GetExitCodeProcess failed"));
    }
    return static_cast<int>(exit_code);
#else
    int status;

    if (timeout_ms < 0) {
        if (::waitpid(info.process_id, &status, 0) == -1) {
            NEFORCE_THROW_EXCEPTION(process_exception(::strerror(errno)));
        }
    } else {
        int elapsed = 0;

        while (elapsed < timeout_ms) {
            constexpr int sleep_interval = 100;
            const ::pid_t result = ::waitpid(info.process_id, &status, WNOHANG);
            if (result == -1) {
                NEFORCE_THROW_EXCEPTION(process_exception(::strerror(errno)));
            }
            if (result > 0) break;

            ::usleep(sleep_interval * 1000);
            elapsed += sleep_interval;
        }

        if (elapsed >= timeout_ms) {
            return -1;
        }
    }

    info.stdout_output = info.stdout_pipe.read_available();
    info.stdout_pipe.close();
    info.is_running = false;

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return -1;
#endif
}

bool process::terminate(const state_info& info) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::BOOL result = ::TerminateProcess(info.process_handle, 1);
    if (result) {
        ::CloseHandle(info.process_handle);
        ::CloseHandle(info.thread_handle);
    }
    return result != 0;
#else
    if (::kill(info.process_id, SIGTERM) == 0) {
        ::usleep(100000);
        if (is_running(info)) {
            ::kill(info.process_id, SIGKILL);
        }
        return true;
    }
    return false;
#endif
}

bool process::suspend(const state_info& info) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (info.thread_handle == nullptr) return false;
    return ::SuspendThread(info.thread_handle) != static_cast<::DWORD>(-1);
#else
    return ::kill(info.process_id, SIGSTOP) == 0;
#endif
}

bool process::resume(const state_info& info) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (info.thread_handle == nullptr) return false;
    return ::ResumeThread(info.thread_handle) != static_cast<::DWORD>(-1);
#else
    return ::kill(info.process_id, SIGCONT) == 0;
#endif
}

bool process::is_running(const state_info& info) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::DWORD exit_code;
    if (::GetExitCodeProcess(info.process_handle, &exit_code)) {
        return exit_code == STILL_ACTIVE;
    }
    return false;
#else
    int status;
    return ::waitpid(info.process_id, &status, WNOHANG) == 0;
#endif
}

process::native_id_type process::current_id() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return ::GetCurrentProcessId();
#else
    return ::getpid();
#endif
}

process::memory_info process::get_memory_info(const state_info& info) noexcept {
    memory_info mem_info = {0, 0, 0, 0};
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::PROCESS_MEMORY_COUNTERS pmc;
    if (::GetProcessMemoryInfo(info.process_handle, &pmc, sizeof(pmc))) {
        mem_info.working_set_size = pmc.WorkingSetSize;
        mem_info.peak_working_set_size = pmc.PeakWorkingSetSize;
        mem_info.pagefile_usage = pmc.PagefileUsage;
        mem_info.peak_pagefile_usage = pmc.PeakPagefileUsage;
    }
#else
    const path path("/proc/" + to_string(info.process_id) + "/statm");
    const file statm(path);
    if (!statm.is_opened()) {
        return mem_info;
    }
    const string text = statm.read();
    if (statm.is_opened()) {
        string tmp;
        size_t pos;
        getline(text, pos, tmp, [](char c) { return is_space(c); });
        size_t size NEFORCE_UNUSED = to_uint64(tmp.view());
        getline(text, pos, tmp, [](char c) { return is_space(c); });
        const size_t rss = to_uint64(tmp.view());
        mem_info.working_set_size = rss * sysconf(_SC_PAGE_SIZE);
        // Peak memory not directly available
    }
#endif
    return mem_info;
}

process::state process::get_state(const state_info& info) noexcept {
    if (!is_running(info)) {
        return state::exited;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::DWORD exit_code;
    if (::GetExitCodeProcess(info.process_handle, &exit_code) && exit_code == STILL_ACTIVE) {
        if (::SuspendThread(info.thread_handle) != static_cast<::DWORD>(-1)) {
            ::ResumeThread(info.thread_handle);
            return state::suspended;
        }
        return state::running;
    }
#else
    const path path("/proc/" + to_string(info.process_id) + "/stat");
    const file stat(path);
    if (stat.is_opened()) {
        string state;
        size_t pos = 0;
        getline(stat.read(), pos, state, [](char c) { return is_space(c); });
        if (state == "T") return state::suspended;
        if (state == "Z") return state::exited;
        return state::running;
    }
#endif
    return state::unknown;
}

bool process::check_permission(const state_info& info, permission permission) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::DWORD desired_access = 0;

    if ((static_cast<int>(permission) & static_cast<int>(permission::read)) != 0) {
        desired_access |= PROCESS_VM_READ;
    }
    if ((static_cast<int>(permission) & static_cast<int>(permission::write)) != 0) {
        desired_access |= PROCESS_VM_WRITE;
    }
    if ((static_cast<int>(permission) & static_cast<int>(permission::terminate)) != 0) {
        desired_access |= PROCESS_TERMINATE;
    }
    if ((static_cast<int>(permission) & static_cast<int>(permission::query_info)) != 0) {
        desired_access |= PROCESS_QUERY_INFORMATION;
    }

    const ::HANDLE hProcess = ::OpenProcess(desired_access, FALSE, info.process_id);
    if (hProcess != nullptr) {
        ::CloseHandle(hProcess);
        return true;
    }
    return false;

#else
    const string proc_path = "/proc/" + to_string(info.process_id);

    int access_mode = 0;
    if ((static_cast<int>(permission) & static_cast<int>(permission::read)) != 0) {
        access_mode |= R_OK;
    }
    if ((static_cast<int>(permission) & static_cast<int>(permission::write)) != 0) {
        access_mode |= W_OK;
    }
    if ((static_cast<int>(permission) & static_cast<int>(permission::execute)) != 0) {
        access_mode |= X_OK;
    }

    if (access_mode == 0) {
        access_mode = F_OK;
    }

    return ::access(proc_path.data(), access_mode) == 0;
#endif
}

string process::name(native_id_type process_id) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::HANDLE hProcess = ::OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        FALSE,
        process_id);

    if (hProcess == nullptr) {
        return "";
    }

    char process_name[MAX_PATH] = {0};
    ::HMODULE hMod;
    ::DWORD cbNeeded;

    if (::EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
        ::GetModuleBaseNameA(hProcess, hMod, process_name, sizeof(process_name));
    }

    ::CloseHandle(hProcess);
    return string(process_name);
#else
    const path path("/proc/" + to_string(process_id) + "/comm");
    const file comm_file(path);

    if (!comm_file.is_opened()) {
        return "";
    }

    string name = comm_file.read_line();

    if (!name.empty() && name.back() == '\n') {
        name.pop_back();
    }

    return name;
#endif
}

NEFORCE_END_NAMESPACE__
