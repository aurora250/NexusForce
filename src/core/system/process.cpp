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

#ifdef MSTL_PLATFORM_LINUX__

static char** build_argv(
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

static void free_argv(char** argv) noexcept{
    if (argv) {
        for (int i = 0; argv[i] != nullptr; ++i)
            delete[] argv[i];
        delete[] argv;
    }
}

#endif

#ifdef MSTL_PLATFORM_WINDOWS__

static string build_command_line(
    const string& executable, const vector<string>& args) {
    string cmd_line = "\"" + executable + "\"";
    for (const auto& arg : args) {
        cmd_line += " \"" + arg + "\"";
    }
    return cmd_line;
}

#endif


static void read_pipe_output(process::process_info& info) {
    string output;

#ifdef MSTL_PLATFORM_WINDOWS__
    if (info.hStdoutRead == nullptr) {
        info.stdout_output = move(output);
    }

    constexpr ::DWORD buffer_size = 4096;
    char buffer[buffer_size];
    ::DWORD bytes_read;

    while (true) {
        if (!::PeekNamedPipe(info.hStdoutRead,
            nullptr, 0, nullptr, &bytes_read, nullptr)) {
            break;
        }
        if (bytes_read == 0) {
            break;
        }
        if (!::ReadFile(info.hStdoutRead, buffer, buffer_size - 1, &bytes_read, nullptr)) {
            break;
        }

        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            output.append(buffer, bytes_read);
        }
    }
#else
    if (info.stdout_fd[0] < 0) {
        info.stdout_output = move(output);
        return;
    }

    char buffer[4096];
    const int flags = ::fcntl(info.stdout_fd[0], F_GETFL, 0);
    ::fcntl(info.stdout_fd[0], F_SETFL, flags | O_NONBLOCK);

    while (true) {
        const ssize_t bytes = ::read(info.stdout_fd[0], buffer, sizeof(buffer) - 1);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            output.append(buffer, bytes);
        } else {
            break;
        }
    }

    ::fcntl(info.stdout_fd[0], F_SETFL, flags);
#endif

    info.stdout_output = move(output);
}


process::process_info process::create_process(const string& executable,
    const vector<string>& args, bool capture_output) {
    process_info info{};

#ifdef MSTL_PLATFORM_WINDOWS__
    ::STARTUPINFOA si{};
    si.cb = sizeof(si);

    ::SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    if (capture_output) {
        if (!::CreatePipe(&info.hStdoutRead, &info.hStdoutWrite, &sa, 0)) {
            throw_exception(system_exception("CreatePipe failed"));
        }
        if (!::SetHandleInformation(info.hStdoutRead, HANDLE_FLAG_INHERIT, 0)) {
            throw_exception(system_exception("SetHandleInformation failed"));
        }

        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdOutput = info.hStdoutWrite;
        si.hStdError = info.hStdoutWrite;
    }

    string cmd_line = build_command_line(executable, args);

    const ::BOOL success = ::CreateProcess(
        nullptr, cmd_line.data(),
        nullptr, nullptr,
        capture_output ? TRUE : FALSE,
        0, nullptr, nullptr, &si, &info.pi
    );

    if (!success) {
        if (capture_output) {
            ::CloseHandle(info.hStdoutRead);
            ::CloseHandle(info.hStdoutWrite);
        }
        throw_exception(system_exception("CreateProcess failed"));
    }
    info.process_id = info.pi.dwProcessId;
    if (capture_output) {
        ::CloseHandle(info.hStdoutWrite);
    }

#else
    if (capture_output) {
        if (::pipe(info.stdout_fd) == -1) {
            throw_exception(system_exception(::strerror(errno)));
        }
    }

    const ::pid_t pid = ::fork();
    if (pid < 0) {
        if (capture_output) {
            ::close(info.stdout_fd[0]);
            ::close(info.stdout_fd[1]);
        }
        throw_exception(system_exception(::strerror(errno)));
    }

    if (pid == 0) {
        if (capture_output) {
            ::close(info.stdout_fd[0]);
            if (::dup2(info.stdout_fd[1], STDOUT_FILENO) == -1) {
                printcln(color::red(), "dup2 stdout failed: ", ::strerror(errno));
                ::_exit(1);
            }

            if (::dup2(info.stdout_fd[1], STDERR_FILENO) == -1) {
                printcln(color::red(), "dup2 stderr failed: ", ::strerror(errno));
                ::_exit(1);
            }

            ::close(info.stdout_fd[1]);
        }
        char** argv = build_argv(executable, args);
        ::execvp(executable.c_str(), argv);

        printcln(color::red(), "execvp failed: ", ::strerror(errno));
        free_argv(argv);
        ::_exit(1);
    }

    info.process_id = pid;
    if (capture_output) {
        ::close(info.stdout_fd[1]);
    }
#endif
    info.is_running = true;
    return info;
}

int process::wait_for_process(process_info& info, int timeout_ms) {
#ifdef MSTL_PLATFORM_WINDOWS__
    const ::DWORD timeout = (timeout_ms < 0) ?
        numeric_limits<::DWORD>::max() : static_cast<::DWORD>(timeout_ms);
    ::DWORD result = ::WaitForSingleObject(info.pi.hProcess, timeout);

    if (result == WAIT_TIMEOUT) {
        return -1;
    }
    if (result == WAIT_FAILED) {
        throw_exception(system_exception("WaitForSingleObject failed"));
    }
    read_pipe_output(info);

    ::DWORD exit_code;
    if (!::GetExitCodeProcess(info.pi.hProcess, &exit_code)) {
        throw_exception(system_exception("GetExitCodeProcess failed"));
    }
    return static_cast<int>(exit_code);
#else
    int status;

    if (timeout_ms < 0) {
        if (::waitpid(info.process_id, &status, 0) == -1) {
            throw_exception(system_exception(::strerror(errno)));
        }
    } else {
        int elapsed = 0;

        while (elapsed < timeout_ms) {
            constexpr int sleep_interval = 100;
            const ::pid_t result = ::waitpid(info.process_id, &status, WNOHANG);
            if (result == -1) {
                throw_exception(system_exception(::strerror(errno)));
            }
            if (result > 0) break;

            ::usleep(sleep_interval * 1000);
            elapsed += sleep_interval;
        }

        if (elapsed >= timeout_ms) {
            return -1;
        }
    }

    if (info.stdout_fd[0] >= 0) {
        read_pipe_output(info);
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return -1;
#endif
}

bool process::terminate_process(const process_info& info) noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    const ::BOOL result = ::TerminateProcess(info.pi.hProcess, 1);
    if (result) {
        ::CloseHandle(info.pi.hProcess);
        ::CloseHandle(info.pi.hThread);
        if (info.hStdoutRead) ::CloseHandle(info.hStdoutRead);
        if (info.hStdoutWrite) ::CloseHandle(info.hStdoutWrite);
    }
    return result != 0;
#else
    if (::kill(info.process_id, SIGTERM) == 0) {
        ::usleep(100000);
        if (is_process_running(info)) {
            ::kill(info.process_id, SIGKILL);
        }
        if (info.stdout_fd[0] >= 0) ::close(info.stdout_fd[0]);
        if (info.stdout_fd[1] >= 0) ::close(info.stdout_fd[1]);
        return true;
    }
    return false;
#endif
}

bool process::is_process_running(const process_info& info) noexcept {
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

int process::current_process_id() noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    return ::GetCurrentProcessId();
#else
    return ::getpid();
#endif
}

MSTL_END_NAMESPACE__
