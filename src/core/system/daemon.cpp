#include <NeForce/core/system/daemon.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <NeForce/core/utility/packages.hpp>
#    include <NeForce/core/config/windef.hpp>
#    include <fileapi.h>
#    include <windef.h>
#    include <WinBase.h>
#else
#    include <cerrno>
#    include <cstdio>
#    include <cstdlib>
#    include <fcntl.h>
#    include <sys/file.h>
#    include <sys/socket.h>
#    include <sys/stat.h>
#    include <sys/un.h>
#    include <unistd.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

#ifdef NEFORCE_PLATFORM_LINUX
namespace {
    bool sd_notify_impl(const string& state) {
        const char* socket_path = ::getenv("NOTIFY_SOCKET");
        if (socket_path == nullptr || socket_path[0] == '\0') {
            return false;
        }

        const int fd = ::socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC, 0);
        if (fd < 0) {
            return false;
        }

        struct ::sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        string_copy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

        const ssize_t sent = ::sendto(fd, state.data(), state.size(), MSG_NOSIGNAL,
                                      reinterpret_cast<struct ::sockaddr*>(&addr), sizeof(addr));
        ::close(fd);
        return sent >= 0;
    }
} // namespace
#endif


daemon::~daemon() {
    if (state_ != daemon_state::stopped) {
        request_shutdown();
    }
    remove_pid_file();
}

bool daemon::daemonize(const string& work_dir) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ignore = work_dir;
    return true;
#else
    work_dir_ = work_dir;
    return daemonize_impl();
#endif
}

#ifndef NEFORCE_PLATFORM_WINDOWS
bool daemon::daemonize_impl() noexcept {
    const ::pid_t pid = ::fork();
    if (pid < 0) {
        return false;
    }
    if (pid > 0) {
        ::_exit(0);
    }

    if (::setsid() < 0) {
        return false;
    }

    const ::pid_t pid2 = ::fork();
    if (pid2 < 0) {
        return false;
    }
    if (pid2 > 0) {
        ::_exit(0);
    }

    if (::chdir(work_dir_.data()) < 0) {
        // best-effort: ignore chdir failure
    }

    ::umask(0);

    const int dev_null = ::open("/dev/null", O_RDWR);
    if (dev_null >= 0) {
        ::dup2(dev_null, STDIN_FILENO);
        ::dup2(dev_null, STDOUT_FILENO);
        ::dup2(dev_null, STDERR_FILENO);
        if (dev_null > STDERR_FILENO) {
            ::close(dev_null);
        }
    }

    return true;
}
#endif

bool daemon::write_pid_file(const string& path) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::HANDLE hFile = ::CreateFileA(path.data(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                                         FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        NEFORCE_THROW_EXCEPTION(daemon_exception("Failed to create PID file"));
    }

    ::OVERLAPPED ov{};
    if (::LockFileEx(hFile, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, 1, 0, &ov) == FALSE) {
        ::CloseHandle(hFile);
        return false;
    }

    const string id = to_string(::GetCurrentProcessId());
    ::DWORD written = 0;
    ::WriteFile(hFile, id.data(), static_cast<::DWORD>(id.size()), &written, nullptr);

    pid_handle_ = hFile;
    pid_path_ = path;
    return true;
#else
    const int fd = ::open(path.data(), O_CREAT | O_RDWR | O_CLOEXEC, 0644);
    if (fd < 0) {
        NEFORCE_THROW_EXCEPTION(daemon_exception("Failed to create PID file"));
    }

    if (::flock(fd, LOCK_EX | LOCK_NB) != 0) {
        ::close(fd);
        return false;
    }

    ::ftruncate(fd, 0);
    char buf[32];
    const int len = ::snprintf(buf, sizeof(buf), "%d\n", static_cast<int>(process::current_id()));
    ignore = ::write(fd, buf, static_cast<size_t>(len));

    pid_fd_ = fd;
    pid_path_ = path;
    return true;
#endif
}

void daemon::remove_pid_file() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (pid_handle_ != nullptr) {
        ::CloseHandle(pid_handle_);
        pid_handle_ = nullptr;
    }
    if (!pid_path_.empty()) {
        ::DeleteFileA(pid_path_.data());
        pid_path_.clear();
    }
#else
    if (pid_fd_ >= 0) {
        ::flock(pid_fd_, LOCK_UN);
        ::close(pid_fd_);
        pid_fd_ = -1;
        ::unlink(pid_path_.data());
        pid_path_.clear();
    }
#endif
}

bool daemon::is_pid_file_locked(const string& path) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::HANDLE hFile = ::CreateFileA(path.data(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                         FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }
    ::OVERLAPPED ov{};
    const bool locked =
            (::LockFileEx(hFile, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, 1, 0, &ov) == FALSE);
    if (!locked) {
        ::UnlockFileEx(hFile, 0, 1, 0, &ov);
    }
    ::CloseHandle(hFile);
    return locked;
#else
    const int fd = ::open(path.data(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        return false;
    }
    const bool locked = (::flock(fd, LOCK_EX | LOCK_NB) != 0);
    if (!locked) {
        ::flock(fd, LOCK_UN);
    }
    ::close(fd);
    return locked;
#endif
}

bool daemon::add_child(const child_config& cfg) {
    if (cfg.name.empty()) {
        return false;
    }

    unique_lock<mutex> lock(children_mutex_);

    if (children_.find(cfg.name) != children_.end()) {
        return false;
    }

    child_entry entry;
    entry.config = cfg;
    children_[cfg.name] = move(entry);
    return true;
}

void daemon::remove_child(const string& name) {
    unique_lock<mutex> lock(children_mutex_);

    const auto it = children_.find(name);
    if (it == children_.end()) {
        return;
    }

    if (it->second.proc != nullptr && it->second.running) {
        it->second.proc->terminate();
    }

    children_.erase(it);
}

daemon::child_status daemon::get_child_status(const string& name) const {
    unique_lock<mutex> lock(children_mutex_);

    const auto it = children_.find(name);
    if (it == children_.end()) {
        return {};
    }

    child_status st;
    st.name = it->second.config.name;
    st.running = it->second.running;
    st.exit_code = it->second.exit_code;
    st.restart_count = it->second.restart_count;
    if (it->second.proc != nullptr) {
        st.pid = it->second.proc->id();
    }
    return st;
}

vector<daemon::child_status> daemon::all_child_statuses() const {
    unique_lock<mutex> lock(children_mutex_);

    vector<child_status> result;
    for (const auto& kv: children_) {
        child_status st;
        st.name = kv.second.config.name;
        st.running = kv.second.running;
        st.exit_code = kv.second.exit_code;
        st.restart_count = kv.second.restart_count;
        if (kv.second.proc != nullptr) {
            st.pid = kv.second.proc->id();
        }
        result.push_back(st);
    }
    return result;
}

void daemon::start_child(child_entry& entry) {
    auto proc = make_unique<process>();

    if (entry.config.capture_output) {
        proc->set_capture_stdout(true);
        proc->set_capture_stderr(true);
    }
    if (!entry.config.work_dir.empty()) {
        proc->set_work_dir(entry.config.work_dir);
    }
    for (const auto& env: entry.config.envs) {
        proc->set_env(env.first, env.second);
    }

    proc->start(entry.config.executable, entry.config.args);

    entry.proc = move(proc);
    entry.running = true;
    entry.exit_code = 0;
}

void daemon::start_all_children() {
    unique_lock<mutex> lock(children_mutex_);

    for (auto& kv: children_) {
        if (!kv.second.running) {
            start_child(kv.second);
        }
    }
}

void daemon::stop_all_children() {
    unique_lock<mutex> lock(children_mutex_);

    for (auto& kv: children_) {
        if (kv.second.running && kv.second.proc != nullptr) {
            kv.second.proc->terminate();
            const int ec = kv.second.proc->wait(kv.second.config.graceful_timeout_ms);
            kv.second.exit_code = ec;
            kv.second.running = false;
            kv.second.proc.reset();
        }
    }
}

void daemon::check_and_restart_children() {
    unique_lock<mutex> lock(children_mutex_);

    for (auto& kv: children_) {
        child_entry& entry = kv.second;

        if (!entry.running || entry.proc == nullptr) {
            continue;
        }

        if (entry.config.health_check_interval_ms > 0 && entry.proc->is_running()) {
            const int64_t now = steady_clock::now().since_epoch().to_milli().count();
            if (now - entry.last_health_check_ms >= entry.config.health_check_interval_ms) {
                entry.last_health_check_ms = now;
                const bool healthy = entry.proc->is_running();
                if (!healthy) {
                    entry.running = false;
                    entry.exit_code = -1;
                }
            }
        }

        if (!entry.proc->is_running()) {
            entry.exit_code = entry.proc->exit_code();
            entry.running = false;

            if (on_child_exit_) {
                on_child_exit_(entry.config.name, entry.exit_code);
            }

            if (entry.config.max_restarts > 0 && entry.restart_count < entry.config.max_restarts) {
                this_thread::sleep_for(milliseconds(entry.config.restart_delay_ms));

                try {
                    start_child(entry);
                    entry.restart_count++;
                } catch (...) {
                    entry.proc.reset();
                }
            } else {
                entry.proc.reset();
            }
        }
    }
}

void daemon::setup_signal_handlers() {
    auto& mgr = system_signal_manager::instance();

    const auto shutdown_hander = [this](system_signal_manager::event, void*) -> bool {
        request_shutdown();
        return false;
    };
    mgr.register_handler(system_signal_manager::event::TERMINATE, shutdown_hander);
    mgr.register_handler(system_signal_manager::event::INTERRUPT, shutdown_hander);

    mgr.register_handler(system_signal_manager::event::HANGUP, [this](system_signal_manager::event, void*) -> bool {
        request_reload();
        return true;
    });

#ifdef NEFORCE_PLATFORM_WINDOWS
    mgr.register_handler(system_signal_manager::event::CLOSE, shutdown_hander);
    mgr.register_handler(system_signal_manager::event::SHUTDOWN, shutdown_hander);
    mgr.register_handler(system_signal_manager::event::LOGOFF, shutdown_hander);
#endif
}

int daemon::run() {
    if (state_ != daemon_state::stopped) {
        NEFORCE_THROW_EXCEPTION(daemon_exception("Daemon is already running"));
    }

    state_ = daemon_state::starting;

    setup_signal_handlers();

    signal_guard_ = make_unique<signal_guard>();

    if (watchdog_timeout_ms_.load() > 0) {
        watchdog_enabled_ = true;
        watchdog_ping();
        watchdog_thread_ = thread(&daemon::watchdog_loop, this);
    }

    if (on_start_) {
        on_start_();
    }

    start_all_children();

    state_ = daemon_state::running;

    constexpr uint32_t check_interval_ms = 500;
    while (state_ == daemon_state::running || state_ == daemon_state::reloading) {
        if (state_ == daemon_state::reloading) {
            state_ = daemon_state::running;
            if (on_reload_) {
                if (!on_reload_()) {
                    state_ = daemon_state::stopping;
                    break;
                }
            }
        }

        const bool should_stop = shutdown_event_.wait(check_interval_ms);

        if (should_stop) {
            state_ = daemon_state::stopping;
            break;
        }

        check_and_restart_children();
    }

    stop_all_children();

    if (on_stop_) {
        on_stop_();
    }

    if (watchdog_enabled_) {
        watchdog_enabled_ = false;
        if (watchdog_thread_.joinable()) {
            watchdog_thread_.join();
        }
    }

    remove_pid_file();

    signal_guard_.reset();

    state_ = daemon_state::stopped;
    return 0;
}

void daemon::request_shutdown() { shutdown_event_.set(); }

void daemon::request_reload() {
    if (state_ == daemon_state::running) {
        state_ = daemon_state::reloading;
    }
    reload_event_.set();
}

void daemon::watchdog_ping() { last_ping_.store(steady_clock::now().since_epoch().to_milli().count()); }

void daemon::set_watchdog_timeout(const int timeout_ms) { watchdog_timeout_ms_ = timeout_ms; }

void daemon::watchdog_loop() {
    while (watchdog_enabled_) {
        this_thread::sleep_for(milliseconds(500));

        if (!watchdog_enabled_) {
            break;
        }

        const int timeout_ms = watchdog_timeout_ms_.load();
        if (timeout_ms <= 0) {
            continue;
        }

        const int64_t last = last_ping_.load();
        const int64_t now = steady_clock::now().since_epoch().to_milli().count();

        if (now - last > static_cast<int64_t>(timeout_ms)) {
            request_shutdown();
            break;
        }
    }
}

#ifdef NEFORCE_PLATFORM_LINUX

void daemon::notify_ready() { sd_notify_impl("READY=1"); }

void daemon::notify_status(const string& status) { sd_notify_impl("STATUS=" + status); }

void daemon::notify_watchdog() { sd_notify_impl("WATCHDOG=1"); }

#endif

NEFORCE_END_NAMESPACE__
