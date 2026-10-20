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
#endif
NEFORCE_BEGIN_NAMESPACE__

#ifdef NEFORCE_PLATFORM_WINDOWS

pty::pty() noexcept = default;
pty::~pty() = default;
pty::pty(pty&& other) noexcept :
master_(other.master_),
child_id_(other.child_id_),
exit_code_(other.exit_code_),
finished_(other.finished_) {
    other.master_ = -1;
    other.child_id_ = 0;
}

pty& pty::operator=(pty&& other) noexcept {
    if (this != &other) {
        close();
        master_ = other.master_;
        child_id_ = other.child_id_;
        exit_code_ = other.exit_code_;
        finished_ = other.finished_;
        other.master_ = -1;
        other.child_id_ = 0;
    }
    return *this;
}

bool pty::is_supported() noexcept { return false; }

bool pty::is_available() noexcept { return false; }

void pty::open(console_size) {
    NEFORCE_THROW_EXCEPTION(pty_exception("Pseudo terminal is not supported on this platform"));
}

void pty::start(const string&, const vector<string>&, const string&, const vector<string>&, bool) {
    NEFORCE_THROW_EXCEPTION(pty_exception("Pseudo terminal is not supported on this platform"));
}

bool pty::is_running() const noexcept { return false; }
native_handle_type pty::master_handle() const noexcept { return static_cast<native_handle_type>(-1); }
native_id_type pty::process_id() const noexcept { return 0; }
string pty::read_available() { return {}; }
int pty::read(void*, size_t) noexcept { return -1; }
int pty::write(string_view) noexcept { return -1; }
void pty::resize(int, int) {}
void pty::send_control(char) {}
void pty::terminate(system_signal::event) {}
void pty::kill() {}
int pty::wait(int) { return -1; }
int pty::exit_code() const noexcept { return -1; }
bool pty::poll_exit() noexcept { return true; }
void pty::close() noexcept {}
void pty::reap(bool, int) {}

#else

namespace {
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
} // namespace


pty::pty() noexcept = default;

pty::~pty() { close(); }

pty::pty(pty&& other) noexcept :
master_(other.master_),
child_id_(other.child_id_),
exit_code_(other.exit_code_),
finished_(other.finished_),
columns_(other.columns_),
rows_(other.rows_) {
    other.master_ = -1;
    other.child_id_ = 0;
    other.finished_ = true;
}

pty& pty::operator=(pty&& other) noexcept {
    if (this != &other) {
        close();
        master_ = other.master_;
        child_id_ = other.child_id_;
        exit_code_ = other.exit_code_;
        finished_ = other.finished_;
        columns_ = other.columns_;
        rows_ = other.rows_;
        other.master_ = -1;
        other.child_id_ = 0;
        other.finished_ = true;
    }
    return *this;
}

bool pty::is_supported() noexcept { return true; }

bool pty::is_available() noexcept {
    static const bool available = [] {
        const int fd = ::posix_openpt(O_RDWR | O_NOCTTY);
        if (fd < 0) {
            return false;
        }
        ::close(fd);
        return true;
    }();
    return available;
}

void pty::open(console_size size) {
    close();

    columns_ = max(1, size.width);
    rows_ = max(1, size.height);

    master_ = ::posix_openpt(O_RDWR | O_NOCTTY);
    if (master_ < 0) {
        NEFORCE_THROW_EXCEPTION(pty_exception("posix_openpt failed"));
    }
    if (::grantpt(master_) != 0 || ::unlockpt(master_) != 0) {
        ::close(master_);
        master_ = -1;
        NEFORCE_THROW_EXCEPTION(pty_exception("grantpt/unlockpt failed"));
    }
    (void) set_nonblocking(master_);
    apply_window_size(master_, columns_, rows_);
}

void pty::start(const string& executable, const vector<string>& args, const string& work_dir, const vector<string>& env,
                const bool merged_stderr) {
    if (master_ < 0) {
        NEFORCE_THROW_EXCEPTION(pty_exception("pseudo terminal is not open"));
    }
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
    if (::ptsname_r(master_, slave_buffer, sizeof(slave_buffer)) != 0 || slave_buffer[0] == '\0') {
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
        ::close(master_);

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
    exit_code_ = -1;
    finished_ = false;
}

bool pty::is_running() const noexcept { return child_id_ > 0 && !finished_; }

native_handle_type pty::master_handle() const noexcept { return static_cast<native_handle_type>(master_); }

native_id_type pty::process_id() const noexcept { return static_cast<native_id_type>(child_id_); }

string pty::read_available() {
    string result;
    if (master_ < 0) {
        return result;
    }
    char buffer[4096];
    while (true) {
        const auto n = ::read(master_, buffer, sizeof(buffer));
        if (n > 0) {
            result.append(buffer, static_cast<size_t>(n));
            continue;
        }
        break;
    }
    return result;
}

int pty::read(void* buffer, const size_t size) noexcept {
    if (master_ < 0) {
        return -1;
    }
    return static_cast<int>(::read(master_, buffer, size));
}

int pty::write(const string_view data) noexcept {
    if (master_ < 0 || data.empty()) {
        return 0;
    }
    return static_cast<int>(::write(master_, data.data(), data.size()));
}

void pty::resize(const int columns, const int rows) {
    columns_ = max(1, columns);
    rows_ = max(1, rows);
    if (master_ < 0) {
        return;
    }
    apply_window_size(master_, columns_, rows_);
    if (child_id_ > 0 && !finished_) {
        (void) ::kill(-child_id_, SIGWINCH);
    }
}

void pty::send_control(const char byte) { (void) write(string_view(&byte, 1)); }

void pty::terminate(const system_signal::event signal) {
    if (child_id_ > 0 && !finished_) {
        (void) ::kill(-child_id_, static_cast<int>(signal));
        (void) ::kill(child_id_, static_cast<int>(signal));
    }
}

void pty::kill() { terminate(system_signal::event::TERMINATE); }

void pty::reap(const bool blocking, const int timeout_ms) {
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
}

int pty::wait(const int timeout_ms) {
    if (child_id_ <= 0) {
        return exit_code_;
    }
    if (finished_) {
        return exit_code_;
    }
    reap(true, timeout_ms);
    if (!finished_) {
        return -1;
    }
    return exit_code_;
}

int pty::exit_code() const noexcept { return exit_code_; }

bool pty::poll_exit() noexcept {
    if (child_id_ <= 0 || finished_) {
        return true;
    }
    reap(false, 0);
    return finished_;
}

void pty::close() noexcept {
    if (master_ >= 0) {
        ::close(master_);
        master_ = -1;
    }
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
    child_id_ = 0;
}

#endif // NEFORCE_PLATFORM_WINDOWS

NEFORCE_END_NAMESPACE__
