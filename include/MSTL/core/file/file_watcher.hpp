#ifndef MSTL_CORE_FILE_FILE_WATCHER_HPP__
#define MSTL_CORE_FILE_FILE_WATCHER_HPP__
#include "MSTL/core/container/vector.hpp"
#include "MSTL/core/functional/function.hpp"
#include "MSTL/core/async/atomic.hpp"
#include "MSTL/core/async/thread.hpp"
#include "MSTL/core/async/mutex.hpp"
#include "path.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API file_watcher {
public:
    using callback_t = function<void(const path&, FILE_WATCH_EVENT)>;

private:
    _MSTL path watch_path_;
    bool recursive_;
    atomic<bool> watching_{false};
    atomic<bool> stopping_{false};
    callback_t callback_;
    FILE_WATCH_EVENT current_events_{FILE_WATCH_EVENT::ALL};
    vector<char> buffer_;
    mutex callback_mutex_;

#ifdef MSTL_PLATFORM_WINDOWS__
    ::HANDLE dir_handle_ = INVALID_HANDLE_VALUE;
    ::HANDLE completion_port_ = INVALID_HANDLE_VALUE;
    ::OVERLAPPED overlapped_{};
#elif defined(MSTL_PLATFORM_LINUX__)
    int inotify_fd_ = -1;
    int watch_descriptor_ = -1;
    int event_fd_ = -1;
#endif

    thread watch_thread_;

    void watch_thread_func();

public:
    explicit file_watcher(const path& watch_path, bool recursive = false);
    ~file_watcher();

    file_watcher(const file_watcher&) = delete;
    file_watcher& operator =(const file_watcher&) = delete;

    bool start(callback_t callback, FILE_WATCH_EVENT events = FILE_WATCH_EVENT::ALL);
    void stop();

    const path& watch_path() const noexcept { return watch_path_; }
    FILE_WATCH_EVENT current_events() const noexcept { return current_events_; }

    bool is_watching() const noexcept { return watching_.load(); }
    bool is_recursive() const noexcept { return recursive_; }

    bool update_watch(FILE_WATCH_EVENT events);
    bool update_recursive(bool recursive);
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FILE_FILE_WATCHER_HPP__
