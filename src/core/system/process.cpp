#include <NeForce/core/system/process.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <NeForce/core/config/windef.hpp>
#    include <windef.h>
#    include <WinBase.h>
#    include <Psapi.h>
#    ifdef max
#        undef max
#    endif
#    ifdef min
#        undef min
#    endif
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#    include <NeForce/core/file/file.hpp>
#    include <NeForce/core/system/console.hpp>
#    include <NeForce/core/system/sysinfo.hpp>
#    include <cerrno>
#    include <csignal>
#    include <cstdio>
#    include <sys/wait.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
#ifdef NEFORCE_PLATFORM_WINDOWS
    string build_command_line(const string& executable, const vector<string>& args) {
        auto needs_quoting = [](const string& s) -> bool {
            if (s.empty()) {
                return true;
            }
            for (const char c: s) {
                if (c == ' ' || c == '\t' || c == '"' || c == '&' || c == '|' || c == '<' || c == '>' || c == '^' ||
                    c == '%') {
                    return true;
                }
            }
            return false;
        };

        string cmd_line = needs_quoting(executable) ? ("\"" + executable + "\"") : executable;

        for (const auto& arg: args) {
            cmd_line += ' ';
            if (needs_quoting(arg)) {
                cmd_line += '"' + arg + '"';
            } else {
                cmd_line += arg;
            }
        }
        return cmd_line;
    }
#endif

#ifdef NEFORCE_PLATFORM_LINUX
    char** build_argv(const string& executable, const vector<string>& args) {
        const size_t argc = args.size() + 2;
        auto* const argv = new char*[argc];

        argv[0] = new char[executable.length() + 1];
        string_copy(argv[0], executable.data());

        for (size_t i = 0; i < args.size(); ++i) {
            argv[i + 1] = new char[args[i].length() + 1];
            string_copy(argv[i + 1], args[i].data());
        }

        argv[argc - 1] = nullptr;
        return argv;
    }

    void free_argv(char** argv) noexcept {
        if (argv != nullptr) {
            for (int i = 0; argv[i] != nullptr; ++i) {
                delete[] argv[i];
            }
            delete[] argv;
        }
    }

    const auto page_size = sysinfo::instance().get_system_info().page_size;
#endif
} // namespace


process::state_info process::create(const string& executable, const vector<string>& args, bool capture_output) {
    if (executable.empty()) {
        NEFORCE_THROW_EXCEPTION(process_exception("Executable path is empty"));
    }

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
    const ::BOOL success = ::CreateProcessA(nullptr, cmd_line.data(), nullptr, nullptr, capture_output ? TRUE : FALSE,
                                            0, nullptr, nullptr, &si, &pi);

    if (success == FALSE) {
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

    int notify_fds[2] = {-1, -1};
    if (::pipe2(notify_fds, O_CLOEXEC) == -1) {
        const auto error = last_error();
        NEFORCE_THROW_EXCEPTION(process_exception(error.message().data()));
    }

    if (capture_output) {
        info.stdout_pipe = pipe(true);
    }

    const ::pid_t pid = ::fork();
    if (pid < 0) {
        const auto error = last_error();
        NEFORCE_THROW_EXCEPTION(process_exception(error.message().data()));
    }

    if (pid == 0) {
        ::close(notify_fds[0]);

        if (capture_output) {
            info.stdout_pipe.close_read();

            if (::dup2(info.stdout_pipe.native_write_handle(), STDOUT_FILENO) == -1) {
                const auto error = last_error();
                printcln(color::red(), "dup2 stdout failed: ", error.message());
                ::_exit(1);
            }

            if (::dup2(info.stdout_pipe.native_write_handle(), STDERR_FILENO) == -1) {
                const auto error = last_error();
                printcln(color::red(), "dup2 stderr failed: ", error.message());
                ::_exit(1);
            }

            info.stdout_pipe.close_write();
        }

        char** argv = build_argv(executable, args);
        ::execvp(executable.data(), argv);

        const auto error = last_error();
        const int saved_errno = error.value();
        ::write(notify_fds[1], &saved_errno, sizeof(saved_errno));
        ::close(notify_fds[1]);

        printcln(color::red(), "execvp failed: ", error.message());
        free_argv(argv);
        ::_exit(1);
    }

    ::close(notify_fds[1]);

    int child_errno = 0;
    const ssize_t n = ::read(notify_fds[0], &child_errno, sizeof(child_errno));
    ::close(notify_fds[0]);

    if (n > 0) {
        int status = 0;
        ::waitpid(pid, &status, 0);
        const error_code error{child_errno, system_category()};
        NEFORCE_THROW_EXCEPTION(process_exception(error.message().data()));
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
    const ::DWORD timeout = (timeout_ms < 0) ? numeric_traits<::DWORD>::max() : static_cast<::DWORD>(timeout_ms);
    ::DWORD result = ::WaitForSingleObject(info.process_handle, timeout);

    if (result == WAIT_TIMEOUT) {
        return -1;
    }
    if (result == WAIT_FAILED) {
        NEFORCE_THROW_EXCEPTION(process_exception("WaitForSingleObject failed"));
    }

    if (info.stdout_pipe.native_read_handle() != nullptr) {
        info.stdout_output = info.stdout_pipe.read_available();
    }

    ::DWORD exit_code = 0;
    if (::GetExitCodeProcess(info.process_handle, &exit_code) == FALSE) {
        NEFORCE_THROW_EXCEPTION(process_exception("GetExitCodeProcess failed"));
    }
    info.is_running = false;
    return static_cast<int>(exit_code);

#else

    int status = 0;

    if (timeout_ms < 0) {
        if (::waitpid(info.process_id, &status, 0) == -1) {
            const auto error = last_error();
            NEFORCE_THROW_EXCEPTION(process_exception(error.message().data()));
        }
    } else {
        int elapsed = 0;

        while (elapsed < timeout_ms) {
            constexpr int sleep_interval = 100;
            const ::pid_t result = ::waitpid(info.process_id, &status, WNOHANG);
            if (result == -1) {
                const auto error = last_error();
                if (error.error() == errc::no_child_process) {
                    status = 0;
                    break;
                }
                NEFORCE_THROW_EXCEPTION(process_exception(error.message().data()));
            }
            if (result > 0) {
                break;
            }

            ::usleep(sleep_interval * 1000);
            elapsed += sleep_interval;
        }

        if (elapsed >= timeout_ms) {
            return -1;
        }
    }

    if (info.stdout_pipe.native_read_handle() != -1) {
        info.stdout_output = info.stdout_pipe.read_available();
        info.stdout_pipe.close();
    }

    info.is_running = false;

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return -1;
#endif
}

bool process::terminate(const state_info& info) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return ::TerminateProcess(info.process_handle, 1) == TRUE;
#else
    if (::kill(info.process_id, SIGTERM) == 0) {
        ::usleep(100000);
        if (::kill(info.process_id, 0) == 0) {
            ::kill(info.process_id, SIGKILL);
        }
        int status = 0;
        while (::waitpid(info.process_id, &status, 0) == -1 && errno == EINTR) {
            this_thread::yield();
        }
        return true;
    }
    return false;
#endif
}

bool process::suspend(const state_info& info) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (info.thread_handle == nullptr) {
        return false;
    }
    return ::SuspendThread(info.thread_handle) != static_cast<::DWORD>(-1);
#else
    return ::kill(info.process_id, SIGSTOP) == 0;
#endif
}

bool process::resume(const state_info& info) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (info.thread_handle == nullptr) {
        return false;
    }
    return ::ResumeThread(info.thread_handle) != static_cast<::DWORD>(-1);
#else
    return ::kill(info.process_id, SIGCONT) == 0;
#endif
}

bool process::is_running(const state_info& info) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::DWORD exit_code = 0;
    if (::GetExitCodeProcess(info.process_handle, &exit_code) == TRUE) {
        return exit_code == STILL_ACTIVE;
    }
    return false;
#else
    return ::kill(info.process_id, 0) == 0;
#endif
}

process::native_id_type process::current_id() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return ::GetCurrentProcessId();
#else
    return ::getpid();
#endif
}

process::memory_info process::get_memory_info(const state_info& info) {
    memory_info mem_info;

#ifdef NEFORCE_PLATFORM_WINDOWS

    ::PROCESS_MEMORY_COUNTERS pmc;
    if (::GetProcessMemoryInfo(info.process_handle, &pmc, sizeof(pmc)) == TRUE) {
        mem_info.working_set_size = pmc.WorkingSetSize;
        mem_info.peak_working_set_size = pmc.PeakWorkingSetSize;
        mem_info.pagefile_usage = pmc.PagefileUsage;
        mem_info.peak_pagefile_usage = pmc.PeakPagefileUsage;
    }

#else

    ::FILE* fp = ::fopen(("/proc/" + to_string(info.process_id) + "/statm").data(), "r");
    if (fp != nullptr) {
        char line[256];
        if (::fgets(line, sizeof(line), fp) != nullptr) {
            string_view line_view(line);
            size_t pos = 0;

            auto parse_next_ulong = [&line_view, &pos]() -> unsigned long {
                while (pos < line_view.size() && line_view[pos] == ' ') {
                    pos++;
                }

                if (pos >= line_view.size()) {
                    return 0;
                }

                size_t end = pos;
                while (end < line_view.size() && line_view[end] != ' ' && line_view[end] != '\n') {
                    end++;
                }

                if (end == pos) {
                    return 0;
                }

                const string_view num_sv(line_view.data() + pos, end - pos);
                pos = end;

                try {
                    return static_cast<unsigned long>(to_uint64(num_sv));
                } catch (...) {
                    return 0;
                }
            };

            ignore = parse_next_ulong(); // size
            mem_info.working_set_size = parse_next_ulong() * static_cast<size_t>(page_size);
        }
        ::fclose(fp);
    }

    fp = ::fopen(("/proc/" + to_string(info.process_id) + "/status").data(), "r");
    if (fp != nullptr) {
        char line[256];
        while (::fgets(line, sizeof(line), fp) != nullptr) {
            string_view line_view(line);

            if (line_view.starts_with("VmHWM:")) {
                string_view value_part = line_view.substr(6);
                value_part = value_part.trim_left();

                size_t num_end = 0;
                while (num_end < value_part.size() &&
                       ((value_part[num_end] >= '0' && value_part[num_end] <= '9') || value_part[num_end] == ' ')) {
                    num_end++;
                }

                if (num_end > 0) {
                    string_view num_sv = value_part.substr(0, num_end).trim_right();

                    if (!num_sv.empty()) {
                        try {
                            const auto hwm_kb = static_cast<unsigned long>(to_uint64(num_sv));
                            mem_info.peak_working_set_size = static_cast<size_t>(hwm_kb) * 1024;
                            // NOLINTNEXTLINE(bugprone-empty-catch)
                        } catch (...) {
                            // ignore
                        }
                    }
                }
            } else if (line_view.starts_with("VmSwap:")) {
                string_view value_part = line_view.substr(7);
                value_part = value_part.trim();

                size_t num_end = 0;
                while (num_end < value_part.size() &&
                       ((value_part[num_end] >= '0' && value_part[num_end] <= '9') || value_part[num_end] == ' ')) {
                    num_end++;
                }

                if (num_end > 0) {
                    string_view num_sv = value_part.substr(0, num_end);
                    while (!num_sv.empty() && num_sv.back() == ' ') {
                        num_sv = num_sv.substr(0, num_sv.size() - 1);
                    }

                    if (!num_sv.empty()) {
                        try {
                            const auto swap_kb = static_cast<unsigned long>(to_uint64(num_sv));
                            mem_info.pagefile_usage = static_cast<size_t>(swap_kb) * 1024;
                            // NOLINTNEXTLINE(bugprone-empty-catch)
                        } catch (...) {
                            // ignore
                        }
                    }
                }
            }
        }
        ::fclose(fp);
    }

    mem_info.peak_pagefile_usage = 0;

#endif
    return mem_info;
}

process::state process::get_state(const state_info& info) {
    if (!is_running(info)) {
        return state::exited;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::DWORD exit_code = 0;
    if (::GetExitCodeProcess(info.process_handle, &exit_code) == TRUE && exit_code == STILL_ACTIVE) {
        if (::SuspendThread(info.thread_handle) != static_cast<::DWORD>(-1)) {
            ::ResumeThread(info.thread_handle);
            return state::suspended;
        }
        return state::running;
    }
    return state::unknown;

#else
    ::FILE* fp = ::fopen(("/proc/" + to_string(info.process_id) + "/stat").data(), "r");
    if (fp == nullptr) {
        return state::unknown;
    }

    char line[512];
    if (::fgets(line, sizeof(line), fp) == nullptr) {
        ::fclose(fp);
        return state::unknown;
    }
    ::fclose(fp);

    string_view line_view(line);
    size_t pos = 0;

    auto skip_field = [&line_view, &pos]() noexcept {
        while (pos < line_view.size() && line_view[pos] == ' ') {
            pos++;
        }

        if (pos >= line_view.size()) {
            return;
        }

        if (line_view[pos] == '(') {
            pos++;
            while (pos < line_view.size() && line_view[pos] != ')') {
                pos++;
            }
            if (pos < line_view.size()) {
                pos++;
            }
        } else {
            while (pos < line_view.size() && line_view[pos] != ' ' && line_view[pos] != '\n') {
                pos++;
            }
        }
    };

    skip_field(); // PID
    skip_field(); // PNAME

    while (pos < line_view.size() && line_view[pos] == ' ') {
        pos++;
    }

    char state_ch = '\0';
    if (pos < line_view.size()) {
        state_ch = line_view[pos];
    }

    if (state_ch == 'T') {
        return state::suspended;
    }
    if (state_ch == 'Z') {
        return state::exited;
    }
    return state::running;
#endif
}

bool process::check_permission(const state_info& info, permission permission) {
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

string process::name(native_id_type process_id) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::HANDLE hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, process_id);

    if (hProcess == nullptr) {
        return "";
    }

    char process_name[MAX_PATH] = {0};
    ::HMODULE h_mod = nullptr;
    ::DWORD cb_needed = 0;

    if (::EnumProcessModules(hProcess, &h_mod, sizeof(::HMODULE), &cb_needed) == TRUE) {
        ::GetModuleBaseNameA(hProcess, h_mod, process_name, sizeof(process_name));
    }

    ::CloseHandle(hProcess);
    return {process_name};
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
