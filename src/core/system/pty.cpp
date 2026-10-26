#include <NeForce/core/string/to_string.hpp>
#include <NeForce/core/system/environment.hpp>
#include <NeForce/core/system/process.hpp>
#include <NeForce/core/system/pty.hpp>
#ifndef NEFORCE_PLATFORM_WINDOWS
#    include <cerrno>
#    include <csignal>
#    include <cstdlib>
#    include <fcntl.h>
#    include <sys/ioctl.h>
#    include <sys/wait.h>
#    include <unistd.h>
#else
#    include <NeForce/core/string/utf.hpp>
#    include <windef.h>
#    include <WinBase.h>
#    include <WinUser.h>
#    include <handleapi.h>
#    include <heapapi.h>
#    include <libloaderapi.h>
#    include <namedpipeapi.h>
#    include <processthreadsapi.h>
#    include <synchapi.h>

// Older SDKs predate the ConPTY attribute; the value is fixed by the ABI (ProcThreadAttributeValue(22)).
#    ifndef PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE
#        define PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE ProcThreadAttributeValue(22, FALSE, TRUE, FALSE)
#    endif
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
#ifdef NEFORCE_PLATFORM_WINDOWS
    // ConPTY lives in kernel32 and only exists on Windows 10 1809+. Resolving it at runtime
    // keeps the library loadable on older systems, where pty::is_supported() reports false.
    struct conpty_api {
        using create_fn = ::HRESULT(WINAPI*)(::COORD, ::HANDLE, ::HANDLE, ::DWORD, ::HANDLE*);
        using resize_fn = ::HRESULT(WINAPI*)(::HANDLE, ::COORD);
        using close_fn = void(WINAPI*)(::HANDLE);

        create_fn create{nullptr};
        resize_fn resize{nullptr};
        close_fn close{nullptr};

        NEFORCE_NODISCARD bool valid() const noexcept {
            return create != nullptr && resize != nullptr && close != nullptr;
        }
    };

    const conpty_api& conpty() {
        static const conpty_api api = [] {
            conpty_api loaded{};
            const ::HMODULE kernel = ::GetModuleHandleW(L"kernel32.dll");
            if (kernel == nullptr) {
                return loaded;
            }
            loaded.create = reinterpret_cast<conpty_api::create_fn>(::GetProcAddress(kernel, "CreatePseudoConsole"));
            loaded.resize = reinterpret_cast<conpty_api::resize_fn>(::GetProcAddress(kernel, "ResizePseudoConsole"));
            loaded.close = reinterpret_cast<conpty_api::close_fn>(::GetProcAddress(kernel, "ClosePseudoConsole"));
            return loaded;
        }();
        return api;
    }

    ::COORD console_coord(const int columns, const int rows) noexcept {
        ::COORD size{};
        size.X = static_cast<::SHORT>(max(1, columns));
        size.Y = static_cast<::SHORT>(max(1, rows));
        return size;
    }

    bool create_inheritable_pipe(::HANDLE& read_end, ::HANDLE& write_end) {
        ::SECURITY_ATTRIBUTES attributes{};
        attributes.nLength = sizeof(attributes);
        attributes.bInheritHandle = TRUE;
        return ::CreatePipe(&read_end, &write_end, &attributes, 0) != FALSE;
    }

    wstring windows_environment_block(const vector<string>& env) {
        wstring block;
        if (env.empty()) {
            return block;
        }

        flat_unordered_map<string, string> env_map = environment::all_envs();
        for (const auto& item: env) {
            const auto separator = item.find('=');
            if (separator != string::npos && separator > 0) {
                env_map[string(item.view().head(separator))] = string(item.view().tail(separator + 1));
            }
        }

        for (const auto& entry: env_map) {
            block += character::to_wstring((entry.first + "=" + entry.second).view());
            block.push_back(L'\0');
        }
        block.push_back(L'\0');
        return block;
    }
#else
    int set_nonblocking(const int fd) {
        const int flags = ::fcntl(fd, F_GETFL, 0);
        if (flags < 0) {
            return -1;
        }
        return ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }

    void apply_window_size(const int fd, const int columns, const int rows) {
        ::winsize size{};
        size.ws_col = static_cast<unsigned short>(max(1, columns));
        size.ws_row = static_cast<unsigned short>(max(1, rows));
        (void) ::ioctl(fd, TIOCSWINSZ, &size);
    }

    int decode_status(const int status) noexcept {
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
        if (WIFSIGNALED(status)) {
            // Same convention as process::wait(): a signalled child reports a negative code.
            return -static_cast<int>(WTERMSIG(status));
        }
        return -1;
    }

    void notify_parent(const int fd, const int code) noexcept { (void) ::write(fd, &code, sizeof(code)); }

    string resolve_program(const string& executable) {
        const string program = process::search_path(executable);
        if (program.empty()) {
            const error_code error{ENOENT, system_category()};
            NEFORCE_THROW_EXCEPTION(pty_exception("executable not found: " + executable, error));
        }
        return program;
    }
#endif

    constexpr native_handle_type invalid_handle =
#ifdef NEFORCE_PLATFORM_WINDOWS
            nullptr;
#else
            -1;
#endif
} // namespace


pty::pty() noexcept :
handle_(invalid_handle) {}

pty::~pty() { close(); }

pty::pty(pty&& other) noexcept :
handle_(other.handle_),
child_id_(other.child_id_),
exit_code_(other.exit_code_),
finished_(other.finished_),
columns_(other.columns_),
rows_(other.rows_)
#ifdef NEFORCE_PLATFORM_WINDOWS
,
read_handle_(other.read_handle_),
write_handle_(other.write_handle_),
process_handle_(other.process_handle_)
#endif
{
    other.handle_ = invalid_handle;
    other.child_id_ = 0;
    other.finished_ = true;
#ifdef NEFORCE_PLATFORM_WINDOWS
    other.read_handle_ = invalid_handle;
    other.write_handle_ = invalid_handle;
    other.process_handle_ = invalid_handle;
#endif
}

pty& pty::operator=(pty&& other) noexcept {
    if (this != &other) {
        close();
        handle_ = other.handle_;
        child_id_ = other.child_id_;
        exit_code_ = other.exit_code_;
        finished_ = other.finished_;
        columns_ = other.columns_;
        rows_ = other.rows_;
#ifdef NEFORCE_PLATFORM_WINDOWS
        read_handle_ = other.read_handle_;
        write_handle_ = other.write_handle_;
        process_handle_ = other.process_handle_;
#endif

        other.handle_ = invalid_handle;
        other.child_id_ = 0;
        other.finished_ = true;
#ifdef NEFORCE_PLATFORM_WINDOWS
        other.read_handle_ = invalid_handle;
        other.write_handle_ = invalid_handle;
        other.process_handle_ = invalid_handle;
#endif
    }
    return *this;
}

bool pty::is_supported() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return conpty().valid();
#else
    return true;
#endif
}

bool pty::is_available() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (!conpty().valid()) {
        return false;
    }
    // CreatePseudoConsole() exists on every Windows 10 1809+ build,
    // but the console host behind it can still be absent or broken: the child may start and
    // exit normally while its output never reaches the pseudoconsole pipe, which would leave
    // callers with an empty tool result. Probe the full data path once and cache the answer.
    static const bool usable = [] {
        try {
            pty probe;
            probe.open(console_size{80, 24});
            probe.start("cmd.exe", {"/c", "echo neforce-pty-probe"});
            string seen;
            int empty_after_exit = 0;
            for (int attempt = 0; attempt < 400; ++attempt) {
                const string chunk = probe.read_available();
                seen += chunk;
                if (seen.contains("neforce-pty-probe")) {
                    return true;
                }
                if (probe.poll_exit() && chunk.empty()) {
                    if (++empty_after_exit >= 100) {
                        break;
                    }
                } else {
                    empty_after_exit = 0;
                }
                ::Sleep(5);
            }
            return false;
        } catch (...) {
            return false;
        }
    }();
    return usable;
#else
    static const bool available = [] {
        const int fd = ::posix_openpt(O_RDWR | O_NOCTTY);
        if (fd < 0) {
            return false;
        }
        ::close(fd);
        return true;
    }();
    return available;
#endif
}

void pty::open(const console_size size) {
    close();

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (!conpty().valid()) {
        NEFORCE_THROW_EXCEPTION(pty_exception("ConPTY requires Windows 10 1809 or newer"));
    }

    columns_ = max(1, size.width);
    rows_ = max(1, size.height);

    ::HANDLE input_read = nullptr;
    ::HANDLE input_write = nullptr;
    ::HANDLE output_read = nullptr;
    ::HANDLE output_write = nullptr;

    if (!create_inheritable_pipe(input_read, input_write) || !create_inheritable_pipe(output_read, output_write)) {
        if (input_read != nullptr) {
            ::CloseHandle(input_read);
        }
        if (input_write != nullptr) {
            ::CloseHandle(input_write);
        }
        if (output_read != nullptr) {
            ::CloseHandle(output_read);
        }
        NEFORCE_THROW_EXCEPTION(pty_exception(last_error()));
    }

    // The pseudo console owns the child side of both pipes;
    // the parent keeps the write side of the input pipe and the read side of the output pipe.
    ::HANDLE pseudo = nullptr;
    const ::HRESULT result = conpty().create(console_coord(columns_, rows_), input_read, output_write, 0, &pseudo);
    if (result != S_OK || pseudo == nullptr) {
        ::CloseHandle(input_read);
        ::CloseHandle(input_write);
        ::CloseHandle(output_read);
        ::CloseHandle(output_write);
        NEFORCE_THROW_EXCEPTION(pty_exception("CreatePseudoConsole failed", last_error()));
    }

    (void) ::SetHandleInformation(input_write, HANDLE_FLAG_INHERIT, 0);
    (void) ::SetHandleInformation(output_read, HANDLE_FLAG_INHERIT, 0);
    ::CloseHandle(input_read);
    ::CloseHandle(output_write);

    handle_ = pseudo;
    write_handle_ = input_write;
    read_handle_ = output_read;
#else
    columns_ = max(1, size.width);
    rows_ = max(1, size.height);

    handle_ = ::posix_openpt(O_RDWR | O_NOCTTY);
    if (handle_ < 0) {
        NEFORCE_THROW_EXCEPTION(pty_exception("posix_openpt failed"));
    }
    if (::grantpt(handle_) != 0 || ::unlockpt(handle_) != 0) {
        ::close(handle_);
        handle_ = -1;
        NEFORCE_THROW_EXCEPTION(pty_exception("grantpt/unlockpt failed"));
    }
    (void) set_nonblocking(handle_);
    apply_window_size(handle_, columns_, rows_);
#endif
}

void pty::start(const string& executable, const vector<string>& args, const string& work_dir, const vector<string>& env,
                const bool merged_stderr) {
    if (handle_ == invalid_handle) {
        NEFORCE_THROW_EXCEPTION(pty_exception("pseudo terminal is not open"));
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (!finished_ && process_handle_ != nullptr) {
        NEFORCE_THROW_EXCEPTION(pty_exception("a child process is already running on this terminal"));
    }
    // A finished child keeps its handle so the exit code stays readable; drop it before the
    // next start() overwrites the field, otherwise every restart leaks one process handle.
    if (process_handle_ != invalid_handle) {
        ::CloseHandle(process_handle_);
        process_handle_ = invalid_handle;
    }
    // A terminal exposes a single data stream, so stderr is always merged.
    ignore = merged_stderr;

    const string command_line = process::build_command_line(executable, args);
    const wstring wide_command = character::to_wstring(command_line.view());
    const wstring wide_directory = character::to_wstring(work_dir.view());
    const wstring env_block = windows_environment_block(env);

    ::SIZE_T attribute_bytes = 0;
    (void) ::InitializeProcThreadAttributeList(nullptr, 1, 0, &attribute_bytes);
    auto* const attributes =
            static_cast<::LPPROC_THREAD_ATTRIBUTE_LIST>(::HeapAlloc(::GetProcessHeap(), 0, attribute_bytes));
    if (attributes == nullptr) {
        NEFORCE_THROW_EXCEPTION(pty_exception("failed to allocate the process attribute list"));
    }

    struct attribute_guard {
        ::LPPROC_THREAD_ATTRIBUTE_LIST list;

        ~attribute_guard() {
            ::DeleteProcThreadAttributeList(list);
            (void) ::HeapFree(::GetProcessHeap(), 0, list);
        }
    };

    if (::InitializeProcThreadAttributeList(attributes, 1, 0, &attribute_bytes) == FALSE) {
        (void) ::HeapFree(::GetProcessHeap(), 0, attributes);
        NEFORCE_THROW_EXCEPTION(pty_exception(last_error()));
    }

    const attribute_guard guard{attributes};
    // UpdateProcThreadAttribute copies cbSize bytes from lpValue; PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE
    // expects the HPCON handle value itself, not the address of the variable holding it.
    ::HANDLE pseudo = handle_;
    if (::UpdateProcThreadAttribute(attributes, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE, static_cast<::PVOID>(pseudo),
                                    sizeof(pseudo), nullptr, nullptr) == FALSE) {
        NEFORCE_THROW_EXCEPTION(pty_exception(last_error()));
    }

    ::STARTUPINFOEXW startup{};
    startup.StartupInfo.cb = sizeof(startup);
    startup.lpAttributeList = attributes;

    ::PROCESS_INFORMATION info{};
    ::DWORD flags = EXTENDED_STARTUPINFO_PRESENT;
    wchar_t* environment = nullptr;
    if (!env_block.empty()) {
        // The block is a sequence of UTF-16 "key=value" entries ending with two terminators.
        environment = const_cast<wchar_t*>(env_block.data());
        flags |= CREATE_UNICODE_ENVIRONMENT;
    }

    if (::CreateProcessW(nullptr, const_cast<wchar_t*>(wide_command.view().data()), nullptr, nullptr, FALSE, flags,
                         environment, work_dir.empty() ? nullptr : wide_directory.view().data(), &startup.StartupInfo,
                         &info) == FALSE) {
        NEFORCE_THROW_EXCEPTION(pty_exception(last_error()));
    }

    ::CloseHandle(info.hThread);
    process_handle_ = info.hProcess;
    child_id_ = static_cast<int>(info.dwProcessId);

#else
    if (!finished_ && child_id_ > 0) {
        NEFORCE_THROW_EXCEPTION(pty_exception("a child process is already running on this terminal"));
    }
    // A terminal exposes a single data stream, so stderr is always merged;
    // the flag only keeps the signature in sync with the pipe based process_watcher mode.
    ignore = merged_stderr;

    // Resolve through PATH with the shared helper:
    // the program is looked up here instead of in the child,
    // so a missing executable is reported by start() and not as a silent 127.
    const string program = resolve_program(executable);

    char slave_buffer[128];
    slave_buffer[0] = '\0';
    if (::ptsname_r(handle_, slave_buffer, sizeof(slave_buffer)) != 0 || slave_buffer[0] == '\0') {
        NEFORCE_THROW_EXCEPTION(pty_exception("ptsname failed"));
    }

    const string slave_path(slave_buffer);
    const int slave = ::open(slave_path.view().data(), O_RDWR | O_NOCTTY);
    if (slave < 0) {
        NEFORCE_THROW_EXCEPTION(pty_exception("failed to open the pseudo terminal slave"));
    }

    vector<string> argv_storage;
    argv_storage.reserve(args.size() + 2);
    argv_storage.push_back(executable);
    for (const auto& arg: args) {
        argv_storage.push_back(arg);
    }
    vector<char*> argv;
    argv.reserve(argv_storage.size() + 1);
    for (auto& item: argv_storage) {
        argv.push_back(const_cast<char*>(item.view().data()));
    }
    argv.push_back(nullptr);

    // Merge the caller's variables over the current environment and exec with the complete
    // block, mirroring process::start(): no putenv() mutation inside the child.
    vector<string> env_storage;
    vector<char*> envp;
    if (!env.empty()) {
        flat_unordered_map<string, string> env_map = environment::all_envs();
        for (const auto& item: env) {
            const string::size_type separator = item.find('=');
            if (separator != string::npos && separator > 0) {
                env_map[item.head(separator)] = item.tail(separator + 1);
            }
        }
        env_storage.reserve(env_map.size());
        for (const auto& entry: env_map) {
            env_storage.push_back(entry.first + "=" + entry.second);
        }
        envp.reserve(env_storage.size() + 1);
        for (auto& item: env_storage) {
            envp.push_back(item.data());
        }
        envp.push_back(nullptr);
    }

    int notify[2] = {-1, -1};
    if (::pipe2(notify, O_CLOEXEC) == -1) {
        ::close(slave);
        NEFORCE_THROW_EXCEPTION(pty_exception(last_error()));
    }

    const ::pid_t pid = ::fork();
    if (pid < 0) {
        ::close(notify[0]);
        ::close(notify[1]);
        ::close(slave);
        NEFORCE_THROW_EXCEPTION(pty_exception(last_error()));
    }

    if (pid == 0) {
        ::close(notify[0]);
        // Become the session leader and take the slave as the controlling terminal so that isatty(),
        // job control and ^C behave exactly like on a real terminal.
        if (::setsid() < 0) {
            notify_parent(notify[1], errno);
            ::_exit(127);
        }
        if (::ioctl(slave, TIOCSCTTY, 0) != 0) {
            notify_parent(notify[1], errno);
            ::_exit(127);
        }
        if (::dup2(slave, STDIN_FILENO) < 0 || ::dup2(slave, STDOUT_FILENO) < 0 || ::dup2(slave, STDERR_FILENO) < 0) {
            notify_parent(notify[1], errno);
            ::_exit(127);
        }
        if (slave > STDERR_FILENO) {
            ::close(slave);
        }
        ::close(handle_);

        if (!work_dir.empty() && ::chdir(work_dir.view().data()) != 0) {
            notify_parent(notify[1], errno);
            ::_exit(127);
        }

        if (!envp.empty()) {
            ::execve(program.view().data(), argv.data(), envp.data());
        } else {
            ::execv(program.view().data(), argv.data());
        }
        notify_parent(notify[1], errno);
        ::_exit(127);
    }

    ::close(slave);
    ::close(notify[1]);

    int child_errno = 0;
    const ssize_t notified = ::read(notify[0], &child_errno, sizeof(child_errno));
    ::close(notify[0]);
    if (notified > 0) {
        int status = 0;
        (void) ::waitpid(pid, &status, 0);
        const error_code error{child_errno, system_category()};
        NEFORCE_THROW_EXCEPTION(pty_exception(error));
    }

    child_id_ = pid;

#endif

    exit_code_ = -1;
    finished_ = false;
}

bool pty::is_running() const noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (process_handle_ == nullptr || finished_) {
        return false;
    }
    return ::WaitForSingleObject(process_handle_, 0) == WAIT_TIMEOUT;
#else
    return child_id_ > 0 && !finished_;
#endif
}

native_handle_type pty::master_handle() const noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return read_handle_;
#else
    return handle_;
#endif
}

bool pty::is_master_handle_valid() const noexcept { return master_handle() != invalid_handle; }

native_id_type pty::process_id() const noexcept { return static_cast<native_id_type>(child_id_); }

string pty::read_available() {
    string result;

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (read_handle_ == nullptr) {
        return result;
    }

    char buffer[4096];
    while (true) {
        ::DWORD available = 0;
        if (::PeekNamedPipe(read_handle_, nullptr, 0, nullptr, &available, nullptr) == FALSE || available == 0) {
            break;
        }
        ::DWORD read = 0;
        if (::ReadFile(read_handle_, buffer, sizeof(buffer), &read, nullptr) == FALSE || read == 0) {
            break;
        }
        result.append(buffer, static_cast<size_t>(read));
    }

#else
    if (handle_ < 0) {
        return result;
    }

    char buffer[4096];
    while (true) {
        const auto n = ::read(handle_, buffer, sizeof(buffer));
        if (n > 0) {
            result.append(buffer, static_cast<size_t>(n));
            continue;
        }
        break;
    }

#endif

    return result;
}

int pty::read(void* buffer, const size_t size) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (read_handle_ == nullptr || buffer == nullptr || size == 0) {
        return -1;
    }

    // A raw ReadFile on a pipe blocks, while the POSIX master is non-blocking. Peek first so
    // both platforms report "nothing available" the same way.
    ::DWORD available = 0;
    if (::PeekNamedPipe(read_handle_, nullptr, 0, nullptr, &available, nullptr) == FALSE || available == 0) {
        return -1;
    }

    ::DWORD read = 0;
    const auto wanted = static_cast<::DWORD>(min(static_cast<size_t>(available), size));
    if (::ReadFile(read_handle_, buffer, wanted, &read, nullptr) == FALSE) {
        return -1;
    }
    return static_cast<int>(read);
#else
    if (handle_ < 0) {
        return -1;
    }
    return static_cast<int>(::read(handle_, buffer, size));
#endif
}

int pty::write(const string_view data) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (write_handle_ == nullptr || data.empty()) {
        return 0;
    }
    ::DWORD written = 0;
    if (::WriteFile(write_handle_, data.data(), static_cast<::DWORD>(data.size()), &written, nullptr) == FALSE) {
        return -1;
    }
    return static_cast<int>(written);
#else
    if (handle_ < 0 || data.empty()) {
        return 0;
    }
    return static_cast<int>(::write(handle_, data.data(), data.size()));
#endif
}

void pty::resize(const int columns, const int rows) {
    columns_ = max(1, columns);
    rows_ = max(1, rows);

    if (handle_ == invalid_handle) {
        return;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    (void) conpty().resize(handle_, console_coord(columns_, rows_));
#else
    apply_window_size(handle_, columns_, rows_);
    if (child_id_ > 0 && !finished_) {
        (void) ::kill(-child_id_, SIGWINCH);
    }
#endif
}

void pty::send_control(const char byte) { (void) write(string_view(&byte, 1)); }

void pty::terminate(const system_signal::event signal) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ignore = signal;
    // A Windows console application has no signal of its own: terminating the process is the
    // only way to interrupt it, and the pseudo console takes the whole process tree with it.
    if (process_handle_ != nullptr && !finished_) {
        (void) ::TerminateProcess(process_handle_, 1);
    }
#else
    if (child_id_ > 0 && !finished_) {
        (void) ::kill(-child_id_, static_cast<int>(signal));
        (void) ::kill(child_id_, static_cast<int>(signal));
    }
#endif
}

void pty::kill() { terminate(system_signal::event::TERMINATE); }

void pty::reap(const bool blocking, const int timeout_ms) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (process_handle_ == nullptr || finished_) {
        return;
    }

    const ::DWORD wait_ms = blocking ? (timeout_ms < 0 ? INFINITE : static_cast<::DWORD>(timeout_ms)) : 0;
    if (::WaitForSingleObject(process_handle_, wait_ms) != WAIT_OBJECT_0) {
        return;
    }

    ::DWORD code = 0;
    if (::GetExitCodeProcess(process_handle_, &code) == FALSE) {
        return;
    }
    exit_code_ = static_cast<int>(code);
    finished_ = true;

#else
    if (child_id_ <= 0) {
        return;
    }

    if (!blocking) {
        int status = 0;
        const ::pid_t pid = ::waitpid(child_id_, &status, WNOHANG);
        if (pid == child_id_) {
            exit_code_ = decode_status(status);
            finished_ = true;
        }
        return;
    }

    if (timeout_ms < 0) {
        int status = 0;
        if (::waitpid(child_id_, &status, 0) == child_id_) {
            exit_code_ = decode_status(status);
            finished_ = true;
        }
        return;
    }

    constexpr int step_ms = 5;
    int waited = 0;
    while (waited <= timeout_ms) {
        int status = 0;
        const ::pid_t pid = ::waitpid(child_id_, &status, WNOHANG);
        if (pid == child_id_) {
            exit_code_ = decode_status(status);
            finished_ = true;
            return;
        }
        waited += step_ms;
        this_thread::sleep_for_ms(step_ms);
    }
#endif
}

int pty::wait(const int timeout_ms) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (process_handle_ == nullptr) {
        return exit_code_;
    }
#else
    if (child_id_ <= 0) {
        return exit_code_;
    }
#endif
    if (!finished_) {
        reap(true, timeout_ms);
    }
    return finished_ ? exit_code_ : -1;
}

int pty::exit_code() const noexcept { return exit_code_; }

bool pty::poll_exit() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (process_handle_ == nullptr) {
        return true;
    }
#else
    if (child_id_ <= 0 || finished_) {
        return true;
    }
#endif
    if (!finished_) {
        reap(false, 0);
    }
    return finished_;
}

void pty::close() noexcept {
    if (handle_ != invalid_handle) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        conpty().close(handle_);
#else
        ::close(handle_);
#endif
        handle_ = invalid_handle;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (process_handle_ != invalid_handle) {
        if (!finished_) {
            reap(true, 200);
            if (!finished_) {
                (void) ::TerminateProcess(process_handle_, 1);
                reap(true, 200);
            }
        }
        ::CloseHandle(process_handle_);
        process_handle_ = invalid_handle;
    }

    if (read_handle_ != invalid_handle) {
        ::CloseHandle(read_handle_);
        read_handle_ = invalid_handle;
    }
    if (write_handle_ != invalid_handle) {
        ::CloseHandle(write_handle_);
        write_handle_ = invalid_handle;
    }

#else
    if (child_id_ > 0 && !finished_) {
        (void) ::kill(-child_id_, SIGTERM);
        (void) ::kill(child_id_, SIGTERM);
        reap(true, 200);
        if (!finished_) {
            (void) ::kill(-child_id_, SIGKILL);
            (void) ::kill(child_id_, SIGKILL);
            reap(true, 200);
        }
    }
#endif

    child_id_ = 0;
}

NEFORCE_END_NAMESPACE__
