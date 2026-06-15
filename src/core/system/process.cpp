#include <NeForce/core/system/process.hpp>
#include <NeForce/core/system/environment.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <NeForce/core/exception/error_code.hpp>
#    include <NeForce/core/config/windef.hpp>
#    include <windef.h>
#    include <WinBase.h>
#    include <WinUser.h>
#    include <Psapi.h>
#    include <shellapi.h>
#    include <securitybaseapi.h>
#    include <TlHelp32.h>
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
#    include <cstdlib>
#    include <sys/select.h>
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

    string escape_for_cmd(const string& cmd) {
        string escaped;
        escaped.reserve(cmd.size() + 2);
        escaped.push_back('"');
        for (const char ch: cmd) {
            if (ch == '"') {
                escaped.push_back('\\');
                escaped.push_back('"');
            } else {
                escaped.push_back(ch);
            }
        }
        escaped.push_back('"');
        return escaped;
    }

    string escape_arg_runas(const string& arg) {
        if (arg.find_first_of(" \t\"") == string::npos) {
            return arg;
        }
        string escaped = "\"";
        for (const char c: arg) {
            if (c == '\"') {
                escaped += "\\\"";
            } else if (c == '\\') {
                escaped += "\\\\";
            } else {
                escaped += c;
            }
        }
        escaped += "\"";
        return escaped;
    }

    string build_params_string(const vector<string>& args) {
        string params;
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) {
                params += ' ';
            }
            params += escape_arg_runas(args[i]);
        }
        return params;
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

    char** build_envp(const unordered_map<string, string>& env_map) {
        auto** const new_env = new char*[env_map.size() + 1];
        int idx = 0;
        for (const auto& entry: env_map) {
            const string full = entry.first + "=" + entry.second;
            auto* str = new char[full.length() + 1];
            string_copy(str, full.data());
            new_env[idx++] = str;
        }
        new_env[idx] = nullptr;
        return new_env;
    }

    void free_envp(char** envp) noexcept {
        if (envp != nullptr) {
            for (int i = 0; envp[i] != nullptr; ++i) {
                delete[] envp[i];
            }
            delete[] envp;
        }
    }

    const auto page_size = sysinfo::instance().get_system_info().page_size;

    bool has_pkexec() noexcept { return ::access("/usr/bin/pkexec", X_OK) == 0; }
    bool has_sudo() noexcept { return ::access("/usr/bin/sudo", X_OK) == 0; }

    char** build_elevated_argv(const string& tool, const string& executable, const vector<string>& args) {
        vector<string> all_args;
        all_args.push_back(tool);
        if (tool == "/usr/bin/sudo") {
            all_args.push_back("--");
        }
        all_args.push_back(executable);
        for (const auto& a: args) {
            all_args.push_back(a);
        }

        auto** argv = new char*[all_args.size() + 1];
        for (size_t i = 0; i < all_args.size(); ++i) {
            argv[i] = new char[all_args[i].length() + 1];
            string_copy(argv[i], all_args[i].data());
        }
        argv[all_args.size()] = nullptr;
        return argv;
    }

    void free_elevated_argv(char** argv) noexcept {
        if (argv != nullptr) {
            for (int i = 0; argv[i] != nullptr; ++i) {
                delete[] argv[i];
            }
            delete[] argv;
        }
    }
#endif
} // namespace


process::~process() { close(); }

process::process(process&& other) noexcept :
process_id_(other.process_id_),
exit_code_(other.exit_code_),
started_(other.started_),
finished_(other.finished_),
work_dir_(move(other.work_dir_)),
env_vars_(move(other.env_vars_)),
capture_stdout_(other.capture_stdout_),
capture_stderr_(other.capture_stderr_),
stdin_data_(move(other.stdin_data_)),
stdout_file_(move(other.stdout_file_)),
stderr_file_(move(other.stderr_file_)),
external_stdin_pipe_(other.external_stdin_pipe_),
external_stdout_pipe_(other.external_stdout_pipe_),
external_stderr_pipe_(other.external_stderr_pipe_),
detached_(other.detached_),
stdout_pipe_(move(other.stdout_pipe_)),
stderr_pipe_(move(other.stderr_pipe_)),
stdin_pipe_(move(other.stdin_pipe_)),
stdout_buf_(move(other.stdout_buf_)),
stderr_buf_(move(other.stderr_buf_)),
reader_thread_(move(other.reader_thread_)),
reader_running_(other.reader_running_.load())
#ifdef NEFORCE_PLATFORM_WINDOWS
,
process_handle_(other.process_handle_),
thread_handle_(other.thread_handle_)
#endif
{
    other.process_id_ = 0;
    other.exit_code_ = -1;
    other.started_ = false;
    other.finished_ = false;
    other.capture_stdout_ = false;
    other.capture_stderr_ = false;
    other.detached_ = false;
    other.external_stdin_pipe_ = nullptr;
    other.external_stdout_pipe_ = nullptr;
    other.external_stderr_pipe_ = nullptr;
    other.reader_running_ = false;
#ifdef NEFORCE_PLATFORM_WINDOWS
    other.process_handle_ = nullptr;
    other.thread_handle_ = nullptr;
#endif
}

process& process::operator=(process&& other) noexcept {
    if (addressof(other) == this) {
        return *this;
    }
    close();

    process_id_ = other.process_id_;
    exit_code_ = other.exit_code_;
    started_ = other.started_;
    finished_ = other.finished_;
    work_dir_ = move(other.work_dir_);
    env_vars_ = move(other.env_vars_);
    capture_stdout_ = other.capture_stdout_;
    capture_stderr_ = other.capture_stderr_;
    stdin_data_ = move(other.stdin_data_);
    stdout_file_ = move(other.stdout_file_);
    stderr_file_ = move(other.stderr_file_);
    external_stdin_pipe_ = other.external_stdin_pipe_;
    external_stdout_pipe_ = other.external_stdout_pipe_;
    external_stderr_pipe_ = other.external_stderr_pipe_;
    detached_ = other.detached_;
    stdout_pipe_ = move(other.stdout_pipe_);
    stderr_pipe_ = move(other.stderr_pipe_);
    stdin_pipe_ = move(other.stdin_pipe_);
    stdout_buf_ = move(other.stdout_buf_);
    stderr_buf_ = move(other.stderr_buf_);
    reader_thread_ = move(other.reader_thread_);
    reader_running_.store(other.reader_running_.load());

#ifdef NEFORCE_PLATFORM_WINDOWS
    process_handle_ = other.process_handle_;
    thread_handle_ = other.thread_handle_;
    other.process_handle_ = nullptr;
    other.thread_handle_ = nullptr;
#endif

    other.process_id_ = 0;
    other.exit_code_ = -1;
    other.started_ = false;
    other.finished_ = false;
    other.capture_stdout_ = false;
    other.capture_stderr_ = false;
    other.detached_ = false;
    other.external_stdin_pipe_ = nullptr;
    other.external_stdout_pipe_ = nullptr;
    other.external_stderr_pipe_ = nullptr;
    other.reader_running_ = false;

    return *this;
}

void process::close() noexcept {
    reader_running_ = false;

    if (reader_thread_.joinable()) {
        reader_thread_.join();
    }

    if (!detached_) {
        close_handles();
    }

    // Only close pipes this own (not externally provided ones)
    if (external_stdout_pipe_ == nullptr) {
        stdout_pipe_.close();
    }
    if (external_stderr_pipe_ == nullptr) {
        stderr_pipe_.close();
    }
    if (external_stdin_pipe_ == nullptr) {
        stdin_pipe_.close();
    }

    external_stdin_pipe_ = nullptr;
    external_stdout_pipe_ = nullptr;
    external_stderr_pipe_ = nullptr;

    process_id_ = 0;
    exit_code_ = -1;
    started_ = false;
    finished_ = false;
}

void process::detach_handles() noexcept {
    if (!detached_) {
        detached_ = true;
        close_handles();
    }
}

void process::close_handles() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (thread_handle_ != nullptr) {
        ::CloseHandle(thread_handle_);
        thread_handle_ = nullptr;
    }
    if (process_handle_ != nullptr) {
        ::CloseHandle(process_handle_);
        process_handle_ = nullptr;
    }
#endif
}

process& process::set_work_dir(const string& dir) {
    if (started_) {
        NEFORCE_THROW_EXCEPTION(process_exception("Cannot configure after process has been started"));
    }
    work_dir_ = dir;
    return *this;
}

process& process::set_env(const string& key, const string& value) {
    if (started_) {
        NEFORCE_THROW_EXCEPTION(process_exception("Cannot configure after process has been started"));
    }
    env_vars_.emplace_back(key, value);
    return *this;
}

process& process::set_capture_stdout(const bool v) {
    if (started_) {
        NEFORCE_THROW_EXCEPTION(process_exception("Cannot configure after process has been started"));
    }
    capture_stdout_ = v;
    return *this;
}

process& process::set_capture_stderr(const bool v) {
    if (started_) {
        NEFORCE_THROW_EXCEPTION(process_exception("Cannot configure after process has been started"));
    }
    capture_stderr_ = v;
    return *this;
}

process& process::set_stdin_data(const string& data) {
    if (started_) {
        NEFORCE_THROW_EXCEPTION(process_exception("Cannot configure after process has been started"));
    }
    stdin_data_ = data;
    return *this;
}

process& process::set_stdout_file(const string& file_path) {
    if (started_) {
        NEFORCE_THROW_EXCEPTION(process_exception("Cannot configure after process has been started"));
    }
    stdout_file_ = file_path;
    return *this;
}

process& process::set_stderr_file(const string& file_path) {
    if (started_) {
        NEFORCE_THROW_EXCEPTION(process_exception("Cannot configure after process has been started"));
    }
    stderr_file_ = file_path;
    return *this;
}

process& process::set_external_stdin(pipe& external_pipe) {
    if (started_) {
        NEFORCE_THROW_EXCEPTION(process_exception("Cannot configure after process has been started"));
    }
    external_stdin_pipe_ = &external_pipe;
    return *this;
}

process& process::set_external_stdout(pipe& external_pipe) {
    if (started_) {
        NEFORCE_THROW_EXCEPTION(process_exception("Cannot configure after process has been started"));
    }
    external_stdout_pipe_ = &external_pipe;
    return *this;
}

process& process::set_external_stderr(pipe& external_pipe) {
    if (started_) {
        NEFORCE_THROW_EXCEPTION(process_exception("Cannot configure after process has been started"));
    }
    external_stderr_pipe_ = &external_pipe;
    return *this;
}

void process::reader_loop() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    constexpr ::DWORD buf_size = MEMORY_BIG_ALLOC_THRESHHOLD;
    char buffer[buf_size];

    while (reader_running_) {
        bool any_data = false;

        if (capture_stdout_) {
            ::DWORD available = 0;
            if (::PeekNamedPipe(stdout_pipe_.native_read_handle(), nullptr, 0, nullptr, &available, nullptr) == TRUE &&
                available > 0) {
                ::DWORD bytes_read = 0;
                if (::ReadFile(stdout_pipe_.native_read_handle(), buffer, buf_size - 1, &bytes_read, nullptr) == TRUE &&
                    bytes_read > 0) {
                    buffer[bytes_read] = '\0';
                    stdout_buf_.append(buffer, bytes_read);
                    any_data = true;
                }
            }
        }

        if (capture_stderr_) {
            ::DWORD available = 0;
            if (::PeekNamedPipe(stderr_pipe_.native_read_handle(), nullptr, 0, nullptr, &available, nullptr) == TRUE &&
                available > 0) {
                ::DWORD bytes_read = 0;
                if (::ReadFile(stderr_pipe_.native_read_handle(), buffer, buf_size - 1, &bytes_read, nullptr) == TRUE &&
                    bytes_read > 0) {
                    buffer[bytes_read] = '\0';
                    stderr_buf_.append(buffer, bytes_read);
                    any_data = true;
                }
            }
        }

        const bool stdout_alive = capture_stdout_ && stdout_pipe_.is_valid();
        const bool stderr_alive = capture_stderr_ && stderr_pipe_.is_valid();

        if (!stdout_alive && !stderr_alive) {
            break;
        }

        if (!any_data) {
            ::Sleep(10);
        }
    }
#else
    char buffer[MEMORY_BIG_ALLOC_THRESHHOLD];

    while (reader_running_) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        int max_fd = -1;

        int stdout_fd = capture_stdout_ ? stdout_pipe_.native_read_handle() : -1;
        int stderr_fd = capture_stderr_ ? stderr_pipe_.native_read_handle() : -1;

        if (stdout_fd >= 0) {
            FD_SET(stdout_fd, &read_fds);
            max_fd = max(stdout_fd, max_fd);
        }
        if (stderr_fd >= 0) {
            FD_SET(stderr_fd, &read_fds);
            max_fd = max(stderr_fd, max_fd);
        }
        if (max_fd < 0) {
            break;
        }

        timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = 100000;

        const int ret = ::select(max_fd + 1, &read_fds, nullptr, nullptr, &tv);
        if (ret < 0) {
            break;
        }

        if (stdout_fd >= 0 && FD_ISSET(stdout_fd, &read_fds)) {
            const ssize_t n = ::read(stdout_fd, buffer, sizeof(buffer) - 1);
            if (n > 0) {
                stdout_buf_.append(buffer, static_cast<size_t>(n));
            } else {
                stdout_pipe_.close_read();
            }
        }

        if (stderr_fd >= 0 && FD_ISSET(stderr_fd, &read_fds)) {
            const ssize_t n = ::read(stderr_fd, buffer, sizeof(buffer) - 1);
            if (n > 0) {
                stderr_buf_.append(buffer, static_cast<size_t>(n));
            } else {
                stderr_pipe_.close_read();
            }
        }
    }
#endif
}

void process::start(const string& executable, const vector<string>& args) {
    if (executable.empty()) {
        NEFORCE_THROW_EXCEPTION(process_exception("Executable path is empty"));
    }
    if (started_) {
        NEFORCE_THROW_EXCEPTION(process_exception("Process already started"));
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    const bool use_stdout_ext = external_stdout_pipe_ != nullptr;
    const bool use_stderr_ext = external_stderr_pipe_ != nullptr;
    const bool use_stdin_ext = external_stdin_pipe_ != nullptr;

    auto* stdout_write_handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
    auto* stderr_write_handle = ::GetStdHandle(STD_ERROR_HANDLE);
    auto* stdin_read_handle = ::GetStdHandle(STD_INPUT_HANDLE);

    if (use_stdout_ext) {
        stdout_write_handle = external_stdout_pipe_->native_write_handle();
    } else if (capture_stdout_) {
        stdout_pipe_ = pipe(true);
        ::SetHandleInformation(stdout_pipe_.native_read_handle(), HANDLE_FLAG_INHERIT, 0);
        stdout_write_handle = stdout_pipe_.native_write_handle();
    } else if (!stdout_file_.empty()) {
        ::SECURITY_ATTRIBUTES sa{sizeof(::SECURITY_ATTRIBUTES), nullptr, TRUE};
        stdout_write_handle = ::CreateFileA(stdout_file_.data(), GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS,
                                            FILE_ATTRIBUTE_NORMAL, nullptr);
        if (stdout_write_handle == INVALID_HANDLE_VALUE) {
            const auto error = last_error();
            NEFORCE_THROW_EXCEPTION(process_exception(error.message().data()));
        }
    }

    if (use_stderr_ext) {
        stderr_write_handle = external_stderr_pipe_->native_write_handle();
    } else if (capture_stderr_) {
        stderr_pipe_ = pipe(true);
        ::SetHandleInformation(stderr_pipe_.native_read_handle(), HANDLE_FLAG_INHERIT, 0);
        stderr_write_handle = stderr_pipe_.native_write_handle();
    } else if (!stderr_file_.empty()) {
        ::SECURITY_ATTRIBUTES sa{sizeof(::SECURITY_ATTRIBUTES), nullptr, TRUE};
        stderr_write_handle = ::CreateFileA(stderr_file_.data(), GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS,
                                            FILE_ATTRIBUTE_NORMAL, nullptr);
        if (stderr_write_handle == INVALID_HANDLE_VALUE) {
            const auto error = last_error();
            NEFORCE_THROW_EXCEPTION(process_exception(error.message().data()));
        }
    } else if (capture_stdout_ && !capture_stderr_) {
        stderr_write_handle = stdout_write_handle;
    }

    if (use_stdin_ext) {
        stdin_read_handle = external_stdin_pipe_->native_read_handle();
    } else if (!stdin_data_.empty()) {
        stdin_pipe_ = pipe(true);
        ::SetHandleInformation(stdin_pipe_.native_write_handle(), HANDLE_FLAG_INHERIT, 0);
        stdin_read_handle = stdin_pipe_.native_read_handle();
    }

    ::STARTUPINFOA si{};
    si.cb = sizeof(::STARTUPINFOA);
    const bool has_custom_io = capture_stdout_ || capture_stderr_ || !stdin_data_.empty() || use_stdout_ext ||
                               use_stderr_ext || use_stdin_ext || !stdout_file_.empty() || !stderr_file_.empty();

    if (has_custom_io) {
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdInput = stdin_read_handle;
        si.hStdOutput = stdout_write_handle;
        si.hStdError = stderr_write_handle;
    }

    const string cmd_line = build_command_line(executable, args);

    void* env_block = nullptr;
    string env_block_str;
    if (!env_vars_.empty()) {
        auto env_map = environment::all_envs();
        for (const auto& ov: env_vars_) {
            env_map[ov.first] = ov.second;
        }
        for (const auto& entry: env_map) {
            env_block_str += entry.first + "=" + entry.second;
            env_block_str.push_back('\0');
        }
        env_block_str.push_back('\0');
        env_block = env_block_str.data();
    }

    ::PROCESS_INFORMATION pi;
    const ::BOOL success = ::CreateProcessA(nullptr, const_cast<char*>(cmd_line.data()), nullptr, nullptr,
                                            has_custom_io ? TRUE : FALSE, CREATE_NO_WINDOW, env_block,
                                            work_dir_.empty() ? nullptr : work_dir_.data(), &si, &pi);

    if (success == FALSE) {
        const auto error = last_error();
        NEFORCE_THROW_EXCEPTION(process_exception(error.message().data()));
    }

    process_handle_ = pi.hProcess;
    thread_handle_ = pi.hThread;
    process_id_ = pi.dwProcessId;

    if (capture_stdout_ && !use_stdout_ext) {
        stdout_pipe_.close_write();
    }
    if (capture_stderr_ && !use_stderr_ext) {
        stderr_pipe_.close_write();
    }
    if (!stdin_data_.empty() && !use_stdin_ext) {
        stdin_pipe_.close_read();
    }

    if (!stdout_file_.empty() && stdout_write_handle != INVALID_HANDLE_VALUE) {
        ::CloseHandle(stdout_write_handle);
    }
    if (!stderr_file_.empty() && stderr_write_handle != INVALID_HANDLE_VALUE) {
        ::CloseHandle(stderr_write_handle);
    }
#else
    const bool use_stdout_ext = external_stdout_pipe_ != nullptr;
    const bool use_stderr_ext = external_stderr_pipe_ != nullptr;
    const bool use_stdin_ext = external_stdin_pipe_ != nullptr;

    if (!use_stdout_ext && capture_stdout_) {
        stdout_pipe_ = pipe(false);
    }
    if (!use_stderr_ext && capture_stderr_) {
        stderr_pipe_ = pipe(false);
    }
    if (!use_stdin_ext && !stdin_data_.empty()) {
        stdin_pipe_ = pipe(false);
    }

    int notify_fds[2] = {-1, -1};
    if (::pipe2(notify_fds, O_CLOEXEC) == -1) {
        const auto error = last_error();
        NEFORCE_THROW_EXCEPTION(process_exception(error.message().data()));
    }

    const ::pid_t pid = ::fork();
    if (pid < 0) {
        ::close(notify_fds[0]);
        ::close(notify_fds[1]);
        const auto error = last_error();
        NEFORCE_THROW_EXCEPTION(process_exception(error.message().data()));
    }

    if (pid == 0) {
        ::close(notify_fds[0]);

        // Setup stdin
        int stdin_fd = -1;
        if (use_stdin_ext && external_stdin_pipe_->native_read_handle() >= 0) {
            stdin_fd = external_stdin_pipe_->native_read_handle();
        } else if (!stdin_data_.empty() && stdin_pipe_.native_read_handle() >= 0) {
            stdin_pipe_.close_write();
            stdin_fd = stdin_pipe_.native_read_handle();
        }

        if (stdin_fd >= 0) {
            if (::dup2(stdin_fd, STDIN_FILENO) == -1) {
                const auto error = last_error();
                const int saved_errno = error.value();
                ::write(notify_fds[1], &saved_errno, sizeof(saved_errno));
                ::_exit(1);
            }
            if (!use_stdin_ext) {
                stdin_pipe_.close_read();
            }
        }

        // Setup stdout
        int stdout_fd = -1;
        if (!stdout_file_.empty()) {
            stdout_fd = ::open(stdout_file_.data(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (stdout_fd < 0) {
                const auto error = last_error();
                const int saved_errno = error.value();
                ::write(notify_fds[1], &saved_errno, sizeof(saved_errno));
                ::_exit(1);
            }
        } else if (use_stdout_ext && external_stdout_pipe_->native_write_handle() >= 0) {
            external_stdout_pipe_->close_read();
            stdout_fd = external_stdout_pipe_->native_write_handle();
        } else if (capture_stdout_ && stdout_pipe_.native_write_handle() >= 0) {
            stdout_pipe_.close_read();
            stdout_fd = stdout_pipe_.native_write_handle();
        }

        if (stdout_fd >= 0) {
            if (::dup2(stdout_fd, STDOUT_FILENO) == -1) {
                const auto error = last_error();
                const int saved_errno = error.value();
                ::write(notify_fds[1], &saved_errno, sizeof(saved_errno));
                ::_exit(1);
            }
            if (!use_stdout_ext) {
                stdout_pipe_.close_write();
            }
        }

        // Setup stderr
        int stderr_fd = -1;
        if (!stderr_file_.empty()) {
            stderr_fd = ::open(stderr_file_.data(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (stderr_fd < 0) {
                const auto error = last_error();
                const int saved_errno = error.value();
                ::write(notify_fds[1], &saved_errno, sizeof(saved_errno));
                ::_exit(1);
            }
        } else if (use_stderr_ext && external_stderr_pipe_->native_write_handle() >= 0) {
            external_stderr_pipe_->close_read();
            stderr_fd = external_stderr_pipe_->native_write_handle();
        } else if (capture_stderr_ && stderr_pipe_.native_write_handle() >= 0) {
            stderr_pipe_.close_read();
            stderr_fd = stderr_pipe_.native_write_handle();
        } else if (capture_stdout_ && stdout_fd >= 0) {
            // Merge stderr into stdout
            if (::dup2(stdout_fd, STDERR_FILENO) == -1) {
                // Non-fatal: stderr merging failed
            }
        }

        if (stderr_fd >= 0) {
            if (::dup2(stderr_fd, STDERR_FILENO) == -1) {
                const auto error = last_error();
                const int saved_errno = error.value();
                ::write(notify_fds[1], &saved_errno, sizeof(saved_errno));
                ::_exit(1);
            }
            if (!use_stderr_ext) {
                stderr_pipe_.close_write();
            }
        }

        if (!work_dir_.empty()) {
            if (::chdir(work_dir_.data()) == -1) {
                const auto error = last_error();
                const int saved_errno = error.value();
                ::write(notify_fds[1], &saved_errno, sizeof(saved_errno));
                ::_exit(1);
            }
        }

        char** argv = build_argv(executable, args);

        if (env_vars_.empty()) {
            ::execvp(executable.data(), argv);
        } else {
            auto env_map = environment::all_envs();
            for (const auto& ov: env_vars_) {
                env_map[ov.first] = ov.second;
            }
            char** envp = build_envp(env_map);
            ::execve(executable.data(), argv, envp);
            free_envp(envp);
        }

        const auto error = last_error();
        const int saved_errno = error.value();
        ::write(notify_fds[1], &saved_errno, sizeof(saved_errno));
        ::close(notify_fds[1]);
        free_argv(argv);
        ::_exit(1);
    }

    ::close(notify_fds[1]);

    if (capture_stdout_ && !use_stdout_ext) {
        stdout_pipe_.close_write();
    }
    if (capture_stderr_ && !use_stderr_ext) {
        stderr_pipe_.close_write();
    }
    if (!stdin_data_.empty() && !use_stdin_ext) {
        stdin_pipe_.close_read();
    }

    int child_errno = 0;
    const ssize_t n = ::read(notify_fds[0], &child_errno, sizeof(child_errno));
    ::close(notify_fds[0]);

    if (n > 0) {
        int status = 0;
        ::waitpid(pid, &status, 0);
        const error_code error{child_errno, system_category()};
        NEFORCE_THROW_EXCEPTION(process_exception(error.message().data()));
    }

    process_id_ = pid;
#endif

    started_ = true;

    if (capture_stdout_ || capture_stderr_) {
        reader_running_ = true;
        reader_thread_ = thread(&process::reader_loop, this);
    }

    if (!stdin_data_.empty()) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        ::DWORD written = 0;
        ::WriteFile(stdin_pipe_.native_write_handle(), stdin_data_.data(), static_cast<::DWORD>(stdin_data_.size()),
                    &written, nullptr);
#else
        stdin_pipe_.write(stdin_data_.data(), stdin_data_.size());
#endif
        close_stdin();
        stdin_data_.clear();
    }
}

void process::start_elevated(const string& executable, const vector<string>& args, elevation_tool tool) {
    if (capture_stdout_ || capture_stderr_ || !stdin_data_.empty()) {
        NEFORCE_THROW_EXCEPTION(process_exception("Pipe capture is not supported for elevated processes"));
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    const string params = build_params_string(args);

    ::SHELLEXECUTEINFOA sei{};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
    sei.lpVerb = "runas";
    sei.lpFile = executable.data();
    sei.lpParameters = params.empty() ? nullptr : params.data();
    sei.lpDirectory = work_dir_.empty() ? nullptr : work_dir_.data();
    sei.nShow = SW_HIDE;

    if (::ShellExecuteExA(&sei) == FALSE) {
        const ::DWORD err = ::GetLastError();
        if (err == ERROR_CANCELLED) {
            NEFORCE_THROW_EXCEPTION(process_exception("User cancelled elevation prompt"));
        }
        const auto error = last_error();
        NEFORCE_THROW_EXCEPTION(process_exception(error.message().data()));
    }

    process_handle_ = sei.hProcess;
    process_id_ = ::GetProcessId(sei.hProcess);
    started_ = true;
#else
    string elevation_tool;
    switch (tool) {
        case elevation_tool::sudo:
            elevation_tool = "/usr/bin/sudo";
            break;
        case elevation_tool::pkexec:
            elevation_tool = "/usr/bin/pkexec";
            break;
        case elevation_tool::auto_:
        default:
            elevation_tool = has_pkexec() ? "/usr/bin/pkexec" : "/usr/bin/sudo";
            break;
    }

    vector<string> elevated_args;
    if (elevation_tool == "/usr/bin/sudo") {
        elevated_args.push_back("--");
    }
    elevated_args.push_back(executable);
    for (const auto& a: args) {
        elevated_args.push_back(a);
    }

    start(elevation_tool, elevated_args);
#endif
}

int process::wait(int timeout_ms) {
    if (!started_) {
        NEFORCE_THROW_EXCEPTION(process_exception("Process not started"));
    }
    if (finished_) {
        return exit_code_;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::DWORD timeout = (timeout_ms < 0) ? numeric_traits<::DWORD>::max() : static_cast<::DWORD>(timeout_ms);
    const ::DWORD result = ::WaitForSingleObject(process_handle_, timeout);

    if (result == WAIT_TIMEOUT) {
        return -1;
    }
    if (result == WAIT_FAILED) {
        NEFORCE_THROW_EXCEPTION(process_exception("WaitForSingleObject failed"));
    }

    ::DWORD exit = 0;
    if (::GetExitCodeProcess(process_handle_, &exit) == FALSE) {
        NEFORCE_THROW_EXCEPTION(process_exception("GetExitCodeProcess failed"));
    }
    exit_code_ = static_cast<int>(exit);
#else
    int status = 0;

    if (timeout_ms < 0) {
        if (::waitpid(process_id_, &status, 0) == -1) {
            const auto error = last_error();
            NEFORCE_THROW_EXCEPTION(process_exception(error.message().data()));
        }
    } else {
        int elapsed = 0;

        while (elapsed < timeout_ms) {
            constexpr int interval = 50;
            const ::pid_t result = ::waitpid(process_id_, &status, WNOHANG);
            if (result == -1) {
                const auto error = last_error();
                if (error.error() == errc::no_child_process) {
                    exit_code_ = 0;
                    finished_ = true;
                    reader_running_ = false;
                    if (reader_thread_.joinable()) {
                        reader_thread_.join();
                    }
                    return exit_code_;
                }
                NEFORCE_THROW_EXCEPTION(process_exception(error.message().data()));
            }
            if (result > 0) {
                break;
            }

            ::usleep(interval * 1000);
            elapsed += interval;
        }

        if (elapsed >= timeout_ms) {
            return -1;
        }
    }

    if (WIFEXITED(status)) {
        exit_code_ = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        exit_code_ = -static_cast<int>(WTERMSIG(status));
    } else {
        exit_code_ = -1;
    }
#endif

    finished_ = true;

    reader_running_ = false;
    if (reader_thread_.joinable()) {
        reader_thread_.join();
    }

    if (capture_stdout_ && stdout_pipe_.is_valid()) {
        stdout_buf_ += stdout_pipe_.read_available();
    }
    if (capture_stderr_ && stderr_pipe_.is_valid()) {
        stderr_buf_ += stderr_pipe_.read_available();
    }

    return exit_code_;
}

void process::terminate() {
    if (!started_ || finished_) {
        return;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::TerminateProcess(process_handle_, 1);
    ::WaitForSingleObject(process_handle_, 5000);
#else
    ::kill(process_id_, SIGTERM);
    ::usleep(100000);

    if (::kill(process_id_, 0) == 0) {
        ::kill(process_id_, SIGKILL);
    }

    int status = 0;
    while (::waitpid(process_id_, &status, 0) == -1 && errno == EINTR) {
        this_thread::yield();
    }
#endif

    finished_ = true;

    reader_running_ = false;
    if (reader_thread_.joinable()) {
        reader_thread_.join();
    }

    if (capture_stdout_ && stdout_pipe_.is_valid()) {
        stdout_buf_ += stdout_pipe_.read_available();
    }
    if (capture_stderr_ && stderr_pipe_.is_valid()) {
        stderr_buf_ += stderr_pipe_.read_available();
    }
}

void process::suspend() {
    if (!started_ || finished_) {
        NEFORCE_THROW_EXCEPTION(process_exception("Process not in a suspendable state"));
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        NEFORCE_THROW_EXCEPTION(process_exception("CreateToolhelp32Snapshot failed"));
    }

    ::THREADENTRY32 te{};
    te.dwSize = sizeof(te);

    if (::Thread32First(hSnapshot, &te) == TRUE) {
        do {
            if (te.th32OwnerProcessID == process_id_) {
                const ::HANDLE hThread = ::OpenThread(
                        THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, te.th32ThreadID);
                if (hThread != nullptr) {
                    ::SuspendThread(hThread);
                    ::CloseHandle(hThread);
                }
            }
        } while (::Thread32Next(hSnapshot, &te) == TRUE);
    }

    ::CloseHandle(hSnapshot);
#else
    if (::kill(process_id_, SIGSTOP) != 0) {
        const auto error = last_error();
        NEFORCE_THROW_EXCEPTION(process_exception(error.message().data()));
    }
#endif
}

void process::resume() {
    if (!started_ || finished_) {
        NEFORCE_THROW_EXCEPTION(process_exception("Process not in a resumable state"));
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        NEFORCE_THROW_EXCEPTION(process_exception("CreateToolhelp32Snapshot failed"));
    }

    ::THREADENTRY32 te{};
    te.dwSize = sizeof(te);

    if (::Thread32First(hSnapshot, &te) == TRUE) {
        do {
            if (te.th32OwnerProcessID == process_id_) {
                const ::HANDLE hThread = ::OpenThread(
                        THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, te.th32ThreadID);
                if (hThread != nullptr) {
                    ::ResumeThread(hThread);
                    ::CloseHandle(hThread);
                }
            }
        } while (::Thread32Next(hSnapshot, &te) == TRUE);
    }

    ::CloseHandle(hSnapshot);
#else
    if (::kill(process_id_, SIGCONT) != 0) {
        const auto error = last_error();
        NEFORCE_THROW_EXCEPTION(process_exception(error.message().data()));
    }
#endif
}

bool process::is_running() const {
    if (!started_ || finished_) {
        return false;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::DWORD exit = 0;
    if (::GetExitCodeProcess(process_handle_, &exit) == TRUE) {
        return exit == STILL_ACTIVE;
    }
    return false;
#else
    const state s = get_state(process_id_);
    if (s == state::exited) {
        return false;
    }
    if (s != state::unknown) {
        return true;
    }
    return ::kill(process_id_, 0) == 0;
#endif
}

process::state process::get_state() const {
    if (!started_) {
        return state::unknown;
    }
    if (finished_) {
        return state::exited;
    }

    return get_state(process_id_);
}

process::memory_info process::get_memory_info() const {
    if (!started_) {
        return {};
    }
    return get_memory_info(process_id_);
}

void process::write_stdin(const string& data) {
    if (!started_ || finished_) {
        NEFORCE_THROW_EXCEPTION(process_exception("Process not running, cannot write to stdin"));
    }
    if (!stdin_pipe_.is_valid()) {
        NEFORCE_THROW_EXCEPTION(process_exception("stdin pipe not open"));
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::DWORD written = 0;
    ::WriteFile(stdin_pipe_.native_write_handle(), data.data(), static_cast<::DWORD>(data.size()), &written, nullptr);
#else
    stdin_pipe_.write(data.data(), data.size());
#endif
}

void process::close_stdin() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    stdin_pipe_.close_write();
#else
    stdin_pipe_.close_write();
#endif
}

process::shell_result process::execute_shell(const string& command, const int timeout_ms) {
    if (command.empty()) {
        NEFORCE_THROW_EXCEPTION(process_exception("Empty shell command"));
    }

    process p;
    p.set_capture_stdout(true);

#ifdef NEFORCE_PLATFORM_WINDOWS
    p.start("cmd.exe", {"/c", command});
#else
    p.start("/bin/sh", {"-c", command});
#endif

    const int exit_code = p.wait(timeout_ms);

    if (exit_code == -1 && timeout_ms >= 0) {
        p.terminate();
        NEFORCE_THROW_EXCEPTION(process_exception("Command execution timeout"));
    }

    return {exit_code, move(p.stdout_buf_)};
}

process::shell_result process::execute_elevated_shell(const string& command, const int timeout_ms,
                                                      elevation_tool tool) {
    if (command.empty()) {
        NEFORCE_THROW_EXCEPTION(process_exception("Empty shell command"));
    }

    process p;

#ifdef NEFORCE_PLATFORM_WINDOWS
    p.start_elevated("cmd.exe", {"/c", command}, tool);
#else
    p.start_elevated("/bin/sh", {"-c", command}, tool);
#endif

    const int exit_code = p.wait(timeout_ms);

    if (exit_code == -1 && timeout_ms >= 0) {
        p.terminate();
        NEFORCE_THROW_EXCEPTION(process_exception("Command execution timeout"));
    }

    return {exit_code, ""};
}

process::native_id_type process::current_id() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return ::GetCurrentProcessId();
#else
    return ::getpid();
#endif
}

process::privilege_level process::current_privilege_level() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::BOOL is_admin = FALSE;
    ::SID_IDENTIFIER_AUTHORITY nt_auth = SECURITY_NT_AUTHORITY;
    ::PSID admin_sid = nullptr;
    if (::AllocateAndInitializeSid(&nt_auth, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                                   &admin_sid) == TRUE) {
        ::CheckTokenMembership(nullptr, admin_sid, &is_admin);
        ::FreeSid(admin_sid);
        return is_admin == TRUE ? privilege_level::privileged : privilege_level::not_privileged;
    }
    return privilege_level::unknown;
#else
    return (::geteuid() == 0) ? privilege_level::privileged : privilege_level::not_privileged;
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

process::memory_info process::get_memory_info(native_id_type process_id) {
    memory_info mem_info;

#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::HANDLE hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, process_id);
    if (hProcess == nullptr) {
        return mem_info;
    }

    ::PROCESS_MEMORY_COUNTERS pmc;
    if (::GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)) == TRUE) {
        mem_info.working_set_size = pmc.WorkingSetSize;
        mem_info.peak_working_set_size = pmc.PeakWorkingSetSize;
        mem_info.pagefile_usage = pmc.PagefileUsage;
        mem_info.peak_pagefile_usage = pmc.PeakPagefileUsage;
    }

    ::CloseHandle(hProcess);
#else
    ::FILE* fp = ::fopen(("/proc/" + to_string(process_id) + "/statm").data(), "r");
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

    fp = ::fopen(("/proc/" + to_string(process_id) + "/status").data(), "r");
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

process::state process::get_state(native_id_type process_id) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::HANDLE hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, process_id);
    if (hProcess == nullptr) {
        return state::unknown;
    }

    ::DWORD exit_code = 0;
    auto result = state::running;
    if (::GetExitCodeProcess(hProcess, &exit_code) == TRUE) {
        if (exit_code != STILL_ACTIVE) {
            result = state::exited;
        } else {
            const ::HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
            if (hSnapshot != INVALID_HANDLE_VALUE) {
                ::THREADENTRY32 te{};
                te.dwSize = sizeof(te);
                if (::Thread32First(hSnapshot, &te) == TRUE) {
                    do {
                        if (te.th32OwnerProcessID == process_id) {
                            const ::HANDLE hThread = ::OpenThread(THREAD_QUERY_INFORMATION, FALSE, te.th32ThreadID);
                            if (hThread != nullptr) {
                                const ::DWORD suspend_count = ::SuspendThread(hThread);
                                if (suspend_count != static_cast<::DWORD>(-1)) {
                                    ::ResumeThread(hThread);
                                    if (suspend_count > 0) {
                                        result = state::suspended;
                                    } else {
                                        result = state::running;
                                    }
                                }
                                ::CloseHandle(hThread);
                            }
                            break;
                        }
                    } while (::Thread32Next(hSnapshot, &te) == TRUE);
                }
                ::CloseHandle(hSnapshot);
            }
        }
    }
    ::CloseHandle(hProcess);
    return result;
#else
    ::FILE* fp = ::fopen(("/proc/" + to_string(process_id) + "/stat").data(), "r");
    if (fp == nullptr) {
        if (::kill(process_id, 0) != 0) {
            return state::exited;
        }
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

    switch (state_ch) {
        case 'T':
        case 't':
            return state::suspended;
        case 'Z':
            return state::exited;
        case 'R':
        case 'S':
        case 'D':
        case 'I':
        case 'W':
            return state::running;
        default:
            if (::kill(process_id, 0) == 0) {
                return state::unknown;
            }
            return state::exited;
    }
#endif
}

process::privilege_level process::get_privilege_level(native_id_type process_id) {
    try {
#ifdef NEFORCE_PLATFORM_WINDOWS
        if (process_id == ::GetCurrentProcessId()) {
            ::BOOL is_admin = FALSE;
            ::SID_IDENTIFIER_AUTHORITY nt_auth = SECURITY_NT_AUTHORITY;
            ::PSID admin_sid = nullptr;
            if (::AllocateAndInitializeSid(&nt_auth, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0,
                                           0, 0, 0, &admin_sid) == TRUE) {
                if (::CheckTokenMembership(nullptr, admin_sid, &is_admin) == FALSE) {
                    is_admin = FALSE;
                }
                ::FreeSid(admin_sid);
                return is_admin == TRUE ? privilege_level::privileged : privilege_level::not_privileged;
            }
            return privilege_level::unknown;
        }

        const ::HANDLE hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, process_id);
        if (hProcess == nullptr) {
            return privilege_level::unknown;
        }

        ::HANDLE hToken = nullptr;
        if (::OpenProcessToken(hProcess, TOKEN_QUERY, &hToken) == FALSE) {
            ::CloseHandle(hProcess);
            return privilege_level::unknown;
        }

        ::SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
        ::PSID pAdminSid = nullptr;
        if (::AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0,
                                       0, 0, 0, &pAdminSid) == FALSE) {
            ::CloseHandle(hToken);
            ::CloseHandle(hProcess);
            return privilege_level::unknown;
        }

        ::BOOL bIsAdmin = FALSE;
        const ::BOOL checkResult = ::CheckTokenMembership(hToken, pAdminSid, &bIsAdmin);
        ::FreeSid(pAdminSid);
        ::CloseHandle(hToken);
        ::CloseHandle(hProcess);

        if (checkResult == FALSE) {
            return privilege_level::unknown;
        }
        return (bIsAdmin == TRUE) ? privilege_level::privileged : privilege_level::not_privileged;
#else
        const string proc_status_path = string("/proc/") + to_string(process_id) + "/status";
        ::FILE* fp = ::fopen(proc_status_path.data(), "r");
        if (fp == nullptr) {
            return privilege_level::unknown;
        }

        char line[256];
        auto result = privilege_level::unknown;
        while (::fgets(line, sizeof(line), fp) != nullptr) {
            if (string_compare(line, "Uid:", 4) == 0) {
                const char* ptr = line + 4;
                char* endptr = nullptr;

                ignore = inner::str_to_ints<long>(ptr, &endptr, 10);
                if (endptr == ptr) {
                    break;
                }

                const long euid = inner::str_to_ints<long>(endptr, &endptr, 10);
                if (endptr == ptr) {
                    break;
                }

                result = (euid == 0) ? privilege_level::privileged : privilege_level::not_privileged;
                break;
            }
        }
        ::fclose(fp);
        return result;
#endif
    } catch (...) {
        return privilege_level::unknown;
    }
}

bool process::check_permission(native_id_type process_id, permission permission) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::DWORD desired_access = 0;

    const int perm_val = static_cast<int>(permission);
    if ((perm_val & static_cast<int>(permission::read)) != 0) {
        desired_access |= PROCESS_VM_READ;
    }
    if ((perm_val & static_cast<int>(permission::write)) != 0) {
        desired_access |= PROCESS_VM_WRITE;
    }
    if ((perm_val & static_cast<int>(permission::terminate)) != 0) {
        desired_access |= PROCESS_TERMINATE;
    }
    if ((perm_val & static_cast<int>(permission::query_info)) != 0) {
        desired_access |= PROCESS_QUERY_INFORMATION;
    }

    const ::HANDLE hProcess = ::OpenProcess(desired_access, FALSE, process_id);
    if (hProcess != nullptr) {
        ::CloseHandle(hProcess);
        return true;
    }
    return false;
#else
    const string proc_path = "/proc/" + to_string(process_id);

    int access_mode = 0;
    const int perm_val = static_cast<int>(permission);
    if ((perm_val & static_cast<int>(permission::read)) != 0) {
        access_mode |= R_OK;
    }
    if ((perm_val & static_cast<int>(permission::write)) != 0) {
        access_mode |= W_OK;
    }
    if ((perm_val & static_cast<int>(permission::execute)) != 0) {
        access_mode |= X_OK;
    }

    if (access_mode == 0) {
        access_mode = F_OK;
    }

    return ::access(proc_path.data(), access_mode) == 0;
#endif
}

process::native_id_type process::spawn(const string& executable, const vector<string>& args) {
    process p;
    p.start(executable, args);
    const native_id_type id = p.id();
    p.detach_handles();
    return id;
}

int process::system(const string& executable, const vector<string>& args, const int timeout_ms) {
    process p;
    p.start(executable, args);
    const int rc = p.wait(timeout_ms);
    if (rc == -1 && timeout_ms >= 0) {
        p.terminate();
        NEFORCE_THROW_EXCEPTION(process_exception("Process execution timeout"));
    }
    return rc;
}

string process::capture(const string& executable, const vector<string>& args, const int timeout_ms) {
    process p;
    p.set_capture_stdout(true);
    p.start(executable, args);
    const int rc = p.wait(timeout_ms);
    if (rc == -1 && timeout_ms >= 0) {
        p.terminate();
        NEFORCE_THROW_EXCEPTION(process_exception("Process execution timeout"));
    }
    return p.stdout_output();
}

void process::chain(process& first, process& second) {
    second.stdin_pipe_ = pipe(false);
    first.set_external_stdout(second.stdin_pipe_);
    second.set_external_stdin(second.stdin_pipe_);
}

void process::chain(vector<process*>& processes) {
    if (processes.size() < 2) {
        return;
    }
    for (size_t i = 0; i < processes.size() - 1; ++i) {
        chain(*processes[i], *processes[i + 1]);
    }
}

string process::search_path(const string& executable) {
    if (executable.empty()) {
        return "";
    }

    if (executable.find('/') != string::npos || executable.find('\\') != string::npos) {
        return executable;
    }

    const auto paths = environment::path_list();

#ifdef NEFORCE_PLATFORM_WINDOWS
    const vector<const char*> extensions = {".exe", ".bat", ".cmd", ""};
    for (const auto& dir: paths) {
        for (const char* ext: extensions) {
            const string full_path = dir + "\\" + executable + ext;
            const ::DWORD attrs = ::GetFileAttributesA(full_path.data());
            if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0) {
                return full_path;
            }
        }
    }
#else
    for (const auto& dir: paths) {
        const string full_path = dir + "/" + executable;
        if (::access(full_path.data(), X_OK) == 0) {
            return full_path;
        }
    }
#endif

    return "";
}

NEFORCE_END_NAMESPACE__
