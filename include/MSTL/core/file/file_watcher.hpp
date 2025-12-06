#ifndef MSTL_CORE_FILE_FILE_WATCHER_HPP__
#define MSTL_CORE_FILE_FILE_WATCHER_HPP__
#include "MSTL/core/functional/function.hpp"
#include "MSTL/core/async/atomic.hpp"
#include "MSTL/core/async/thread.hpp"
#ifdef MSTL_PLATFORM_WINDOWS__
#include "MSTL/core/container/vector.hpp"
#endif
#include "path.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API file_watcher {
public:
    using callback_t = function<void(const path&, FILE_WATCH_EVENT)>;

    explicit file_watcher(const path& watch_path, bool recursive = false);
    ~file_watcher();

    file_watcher(const file_watcher&) = delete;
    file_watcher& operator=(const file_watcher&) = delete;

    bool start(callback_t callback);
    void stop();
    bool is_watching() const noexcept { return watching_.load(); }

private:
    path watch_path_;
    bool recursive_;
    atomic<bool> watching_{false};
    callback_t callback_;

#ifdef MSTL_PLATFORM_WINDOWS__
    ::HANDLE dir_handle_ = INVALID_HANDLE_VALUE;
    ::OVERLAPPED overlapped_{};
    vector<char> buffer_;
    void watch_thread_func_windows();
#elif defined(MSTL_PLATFORM_LINUX__)
    int inotify_fd_ = -1;
    int watch_descriptor_ = -1;
    void watch_thread_func_linux();
#endif

    thread watch_thread_;
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FILE_FILE_WATCHER_HPP__
