#include <NeForce/core/async/atomic.hpp>
#include <NeForce/core/async/mutex.hpp>
#include <NeForce/core/async/thread.hpp>
#include <NeForce/core/container/flat_unordered_map.hpp>
#include <NeForce/core/container/vector.hpp>
#include <NeForce/core/exception/system_exception.hpp>
#include <NeForce/core/file/file.hpp>
#include <NeForce/core/file/file_watcher.hpp>
#include <NeForce/core/memory/unique_ptr.hpp>
#include <NeForce/core/string/to_string.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <NeForce/core/config/windef.hpp>
#    include <WinBase.h>
#    include <winnt.h>
#endif
#ifdef NEFORCE_PLATFORM_POSIX
#    include <cerrno>
#    include <dirent.h>
#    include <poll.h>
#    include <sys/stat.h>
#    include <unistd.h>
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#    include <sys/eventfd.h>
#    include <sys/inotify.h>
#endif
#ifdef NEFORCE_PLATFORM_FREEBSD
#    include <fcntl.h>
#    include <sys/event.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
    // inotify event ~= sizeof(inotify_event) + NAME_MAX = 272 bytes.
    // 64KB holds ~240 events per read(), preventing kernel queue overflow
    // under bursty workloads such as builds or checkouts.
    constexpr size_t g_watch_buffer_size = 65536;
    // Batch pre-allocation: enough for a single read() worth of events.
    constexpr size_t g_event_batch_capacity = 256;

#ifdef NEFORCE_PLATFORM_LINUX
    // Register an inotify watch for root and every real (non-symlink) directory below it,
    // remembering the relative prefix of each watch so events can be reported with the correct full path.
    // Symbolic links are not followed (cycle safety).
    void add_subdir_watches(const int inotify_fd, const uint32_t mask, const string& root,
                            flat_unordered_map<int, string>& watch_map) {
        vector<string> pending;
        pending.emplace_back();
        while (!pending.empty()) {
            string rel = move(pending.back());
            pending.pop_back();
            const string full = rel.empty() ? root : root + "/" + rel;

            const int wd = ::inotify_add_watch(inotify_fd, full.data(), mask);
            if (wd != -1) {
                watch_map[wd] = rel;
            }

            ::DIR* const dir = ::opendir(full.data());
            if (dir == nullptr) {
                continue;
            }
            const ::dirent* entry = nullptr;
            // NOLINTNEXTLINE(concurrency-mt-unsafe)
            while ((entry = ::readdir(dir)) != nullptr) {
                const string_view name(entry->d_name);
                if (name == "." || name == "..") {
                    continue;
                }
                const string child = full + "/" + name.data();
                struct ::stat st{};
                if (::lstat(child.data(), &st) == 0 && S_ISDIR(st.st_mode) && !S_ISLNK(st.st_mode)) {
                    pending.push_back(rel.empty() ? string(name) : rel + "/" + string(name));
                }
            }
            ::closedir(dir);
        }
    }
#endif
} // namespace


struct file_watcher::impl {
    path watch_path_;
    bool recursive_;
    atomic<bool> watching_{false};
    atomic<bool> stopping_{false};
    callback_t callback_;
    file_watcher::event current_events_{file_watcher::event::all};
    vector<char> buffer_;
    mutex callback_mutex_;
    thread watch_thread_;

#ifdef NEFORCE_PLATFORM_WINDOWS
    native_handle_type dir_handle_ = INVALID_HANDLE_VALUE;
    native_handle_type completion_port_ = INVALID_HANDLE_VALUE;
    ::OVERLAPPED overlapped_{};
#elif defined(NEFORCE_PLATFORM_LINUX)
    native_handle_type inotify_fd_ = -1;
    native_handle_type watch_descriptor_ = -1;
    native_handle_type event_fd_ = -1;
    uint32_t watch_mask_ = 0;
    flat_unordered_map<int, string> linux_watch_map_;
#else
    int kq_fd_ = -1;

    struct kqueue_watch {
        string relative;
        bool directory;
    };

    flat_unordered_map<int, kqueue_watch> kqueue_watches_;
    flat_unordered_map<string, vector<string>> kqueue_snapshots_;

    bool kqueue_add_watch(const string& relative, bool directory);
    void kqueue_remove_watch(int fd);
    vector<string> kqueue_entries(const string& relative) const;
    void kqueue_collect(const string& relative);
    void kqueue_rescan(const string& relative);
    void kqueue_emit(const string& relative, file_watcher::event event);
#endif

    impl(path watch_path, bool recursive);
    ~impl() = default;

    bool start(callback_t callback, file_watcher::event events);
    void stop();
    bool update_watch(file_watcher::event events);
    bool update_recursive(bool recursive);
    void watch_thread_func();
};


file_watcher::impl::impl(path watch_path, const bool recursive) :
watch_path_(move(watch_path)),
recursive_(recursive) {
    if (!watch_path_.exists() || !watch_path_.is_directory()) {
        NEFORCE_THROW_EXCEPTION(system_exception("Watch path must be an existing directory"));
    }
}


file_watcher::file_watcher(path watch_path, const bool recursive) :
impl_(make_unique<impl>(move(watch_path), recursive)) {}

file_watcher::~file_watcher() {
    if (impl_) {
        stop();
    }
}

bool file_watcher::start(callback_t callback, file_watcher::event events) {
    return impl_->start(move(callback), events);
}

void file_watcher::stop() { impl_->stop(); }

const path& file_watcher::watch_path() const noexcept { return impl_->watch_path_; }

file_watcher::event file_watcher::current_events() const noexcept { return impl_->current_events_; }

bool file_watcher::is_watching() const noexcept { return impl_->watching_.load(); }

bool file_watcher::is_recursive() const noexcept { return impl_->recursive_; }

bool file_watcher::update_watch(const file_watcher::event events) { return impl_->update_watch(events); }

bool file_watcher::update_recursive(const bool recursive) { return impl_->update_recursive(recursive); }

bool file_watcher::impl::start(callback_t callback, file_watcher::event events) {
    if (watching_.load()) {
        return false;
    }

    {
        lock<mutex> lock(callback_mutex_);
        callback_ = move(callback);
        current_events_ = events;
    }
    watching_.store(true);
    stopping_.store(false);

#ifdef NEFORCE_PLATFORM_WINDOWS

    const wstring watch_path = character::to_wstring(watch_path_.str().view());
    dir_handle_ = ::CreateFileW(watch_path.data(), FILE_LIST_DIRECTORY,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                                FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);
    if (dir_handle_ == INVALID_HANDLE_VALUE) {
        watching_.store(false);
        return false;
    }

    completion_port_ = ::CreateIoCompletionPort(dir_handle_, nullptr, 0, 0);
    if (completion_port_ == INVALID_HANDLE_VALUE) {
        ::CloseHandle(dir_handle_);
        dir_handle_ = INVALID_HANDLE_VALUE;
        watching_.store(false);
        return false;
    }

#elif defined(NEFORCE_PLATFORM_LINUX)
    inotify_fd_ = ::inotify_init1(IN_NONBLOCK);
    if (inotify_fd_ == -1) {
        watching_.store(false);
        return false;
    }

    event_fd_ = ::eventfd(0, EFD_NONBLOCK);
    if (event_fd_ == -1) {
        ::close(inotify_fd_);
        inotify_fd_ = -1;
        watching_.store(false);
        return false;
    }

    uint32_t mask = 0;
    if ((static_cast<int>(events) & static_cast<int>(file_watcher::event::created)) != 0U) {
        mask |= IN_CREATE | IN_MOVED_TO;
    }
    if ((static_cast<int>(events) & static_cast<int>(file_watcher::event::deleted)) != 0U) {
        mask |= IN_DELETE | IN_MOVED_FROM;
    }
    if ((static_cast<int>(events) & static_cast<int>(file_watcher::event::modified)) != 0U) {
        mask |= IN_MODIFY | IN_ATTRIB;
    }
    if ((static_cast<int>(events) & static_cast<int>(file_watcher::event::accessed)) != 0U) {
        mask |= IN_ACCESS;
    }

    watch_descriptor_ = ::inotify_add_watch(inotify_fd_, watch_path_.data(), mask);
    if (watch_descriptor_ == -1) {
        ::close(event_fd_);
        ::close(inotify_fd_);
        event_fd_ = -1;
        inotify_fd_ = -1;
        watching_.store(false);
        return false;
    }

    watch_mask_ = mask;
    linux_watch_map_.clear();
    linux_watch_map_[watch_descriptor_] = string{};
    if (recursive_) {
        add_subdir_watches(inotify_fd_, mask, watch_path_.str(), linux_watch_map_);
    }
#else
    kq_fd_ = ::kqueue();
    if (kq_fd_ < 0) {
        watching_.store(false);
        return false;
    }
    kqueue_collect(string{});
#endif

    watch_thread_ = thread(&impl::watch_thread_func, this);

    return true;
}

void file_watcher::impl::stop() {
    if (!watching_.exchange(false)) {
        return;
    }

    stopping_.store(true);

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (completion_port_ != INVALID_HANDLE_VALUE) {
        ::PostQueuedCompletionStatus(completion_port_, 0, 0, nullptr);
    }
    if (dir_handle_ != INVALID_HANDLE_VALUE) {
        ::CancelIoEx(dir_handle_, nullptr);
    }
#elif defined(NEFORCE_PLATFORM_LINUX)
    if (event_fd_ != -1) {
        constexpr uint64_t value = 1;
        ignore = ::write(event_fd_, &value, sizeof(value));
    }
#endif

    if (watch_thread_.joinable()) {
        watch_thread_.join();
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (completion_port_ != INVALID_HANDLE_VALUE) {
        ::CloseHandle(completion_port_);
        completion_port_ = INVALID_HANDLE_VALUE;
    }
    if (dir_handle_ != INVALID_HANDLE_VALUE) {
        ::CloseHandle(dir_handle_);
        dir_handle_ = INVALID_HANDLE_VALUE;
    }
#elif defined(NEFORCE_PLATFORM_LINUX)
    if (watch_descriptor_ != -1) {
        ::inotify_rm_watch(inotify_fd_, watch_descriptor_);
        watch_descriptor_ = -1;
    }
    if (event_fd_ != -1) {
        ::close(event_fd_);
        event_fd_ = -1;
    }
    if (inotify_fd_ != -1) {
        ::close(inotify_fd_);
        inotify_fd_ = -1;
    }
    linux_watch_map_.clear();
#else
    for (auto& pair: kqueue_watches_) {
        ::close(pair.first);
    }
    kqueue_watches_.clear();
    kqueue_snapshots_.clear();
    if (kq_fd_ >= 0) {
        ::close(kq_fd_);
        kq_fd_ = -1;
    }
#endif

    {
        lock<mutex> lock(callback_mutex_);
        callback_ = nullptr;
    }
}

void file_watcher::impl::watch_thread_func() {
#ifdef NEFORCE_PLATFORM_FREEBSD
    struct ::kevent events[64];
    while (!stopping_.load(memory_order_acquire)) {
        const ::timespec timeout{0, 50'000'000L};
        const int count = ::kevent(kq_fd_, nullptr, 0, events, 64, &timeout);
        if (count < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }

        for (int index = 0; index < count; ++index) {
            const auto it = kqueue_watches_.find(static_cast<int>(events[index].ident));
            if (it == kqueue_watches_.end()) {
                continue;
            }
            const int fd = static_cast<int>(events[index].ident);
            const string relative = it->second.relative;
            const bool directory = it->second.directory;
            const auto flags = events[index].fflags;

            if ((flags & (NOTE_DELETE | NOTE_RENAME | NOTE_REVOKE)) != 0) {
                if (!relative.empty()) {
                    kqueue_emit(relative, file_watcher::event::deleted);
                }
                kqueue_remove_watch(fd);
                continue;
            }

            if (directory) {
                kqueue_rescan(relative);
            } else {
                kqueue_emit(relative, file_watcher::event::modified);
            }
        }
    }
#else
    buffer_.resize(g_watch_buffer_size);

    string path_prefix = watch_path_.str();
    if (!path_prefix.empty() && path_prefix.back() != path::preferred_separator) {
        path_prefix += path::preferred_separator;
    }
#    ifdef NEFORCE_PLATFORM_WINDOWS
    const size_t prefix_len = path_prefix.size();
#    endif

    struct batched_event {
        string file_name;
        file_watcher::event type;
    };
    vector<batched_event> event_batch;
    event_batch.reserve(g_event_batch_capacity);

#    ifdef NEFORCE_PLATFORM_WINDOWS
    ::OVERLAPPED local_overlapped{};
    memory_zero(&local_overlapped);
    local_overlapped.hEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (local_overlapped.hEvent == nullptr) {
        watching_.store(false);
        return;
    }

    ::DWORD notify_filter = 0;
    {
        const int ev = static_cast<int>(current_events_);
        if ((ev & static_cast<int>(file_watcher::event::created)) != 0) {
            notify_filter |= FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_CREATION;
        }
        if ((ev & static_cast<int>(file_watcher::event::deleted)) != 0) {
            notify_filter |= FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME;
        }
        if ((ev & static_cast<int>(file_watcher::event::modified)) != 0) {
            notify_filter |= FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_ATTRIBUTES |
                             FILE_NOTIFY_CHANGE_SECURITY;
        }
        if ((ev & static_cast<int>(file_watcher::event::accessed)) != 0) {
            notify_filter |= FILE_NOTIFY_CHANGE_LAST_ACCESS;
        }
        if (notify_filter == 0) {
            notify_filter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME;
        }
    }

    ::DWORD bytes_returned = 0;
    ::BOOL result = ::ReadDirectoryChangesW(dir_handle_, buffer_.data(), static_cast<::DWORD>(buffer_.size()),
                                            recursive_ ? TRUE : FALSE, notify_filter, &bytes_returned,
                                            &local_overlapped, nullptr);

    if (result == FALSE) {
        ::CloseHandle(local_overlapped.hEvent);
        watching_.store(false);
        return;
    }

    while (watching_.load() && !stopping_.load()) {
        ::DWORD bytes_transferred = 0;
        ::ULONG_PTR completion_key = 0;
        ::LPOVERLAPPED lp_overlapped = nullptr;

        const ::BOOL io_completed =
                ::GetQueuedCompletionStatus(completion_port_, &bytes_transferred, &completion_key, &lp_overlapped, 100);

        if (io_completed == FALSE) {
            const ::DWORD error = ::GetLastError();
            if (error == WAIT_TIMEOUT) {
                continue;
            }
            break;
        }

        if (bytes_transferred == 0 && completion_key == 0 && lp_overlapped == nullptr) {
            break;
        }

        if (bytes_transferred > 0) {
            event_batch.clear();
            auto* fni = reinterpret_cast<::FILE_NOTIFY_INFORMATION*>(buffer_.data());

            while (fni != nullptr && watching_.load()) {
                const wstring wide_filename(fni->FileName, fni->FileNameLength / sizeof(wchar_t));
                string utf8_name = to_string(wide_filename);
                if (!utf8_name.empty()) {
                    auto event_type = file_watcher::event::accessed;

                    switch (fni->Action) {
                        case FILE_ACTION_ADDED:
                        case FILE_ACTION_RENAMED_NEW_NAME: {
                            event_type = file_watcher::event::created;
                            break;
                        }
                        case FILE_ACTION_MODIFIED: {
                            event_type = file_watcher::event::modified;
                            break;
                        }
                        case FILE_ACTION_REMOVED:
                        case FILE_ACTION_RENAMED_OLD_NAME: {
                            event_type = file_watcher::event::deleted;
                            break;
                        }
                        default: {
                            break;
                        }
                    }

                    event_batch.push_back({move(utf8_name), event_type});
                }

                if (fni->NextEntryOffset == 0) {
                    fni = nullptr;
                } else {
                    fni = reinterpret_cast<::FILE_NOTIFY_INFORMATION*>(reinterpret_cast<::BYTE*>(fni) +
                                                                       fni->NextEntryOffset);
                }
            }

            if (!event_batch.empty()) {
                callback_t cb_copy;
                {
                    lock<mutex> lk(callback_mutex_);
                    cb_copy = callback_;
                }
                if (cb_copy) {
                    for (auto& be: event_batch) {
                        path_prefix.resize(prefix_len);
                        path_prefix.append(be.file_name);
                        cb_copy(path(path_prefix), be.type);
                    }
                }
            }
        }

        if (watching_.load()) {
            ::ResetEvent(local_overlapped.hEvent);
            bytes_returned = 0;
            result = ::ReadDirectoryChangesW(dir_handle_, buffer_.data(), static_cast<::DWORD>(buffer_.size()),
                                             recursive_ ? TRUE : FALSE, notify_filter, &bytes_returned,
                                             &local_overlapped, nullptr);
            if (result == FALSE) {
                break;
            }
        }
    }

    ::CloseHandle(local_overlapped.hEvent);

#    elif defined(NEFORCE_PLATFORM_LINUX)
    ::pollfd fds[2];
    fds[0].fd = inotify_fd_;
    fds[0].events = POLLIN;
    fds[1].fd = event_fd_;
    fds[1].events = POLLIN;

    while (watching_.load() && !stopping_.load()) {
        const int poll_result = ::poll(fds, 2, 100);
        if (poll_result == -1) {
            if (errno == EINTR) {
                continue;
            }
            break;
        } else if (poll_result == 0) {
            continue;
        }

        if ((fds[1].revents & POLLIN) != 0U) {
            uint64_t value = 0;
            ignore = ::read(event_fd_, &value, sizeof(value));
            break;
        }

        if ((fds[0].revents & POLLIN) != 0U) {
            const ssize_t len = ::read(inotify_fd_, buffer_.data(), buffer_.size());
            if (len <= 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                }
                break;
            }

            event_batch.clear();
            char* ptr = buffer_.data();
            while (ptr < buffer_.data() + len) {
                auto* event = reinterpret_cast<struct ::inotify_event*>(ptr);

                auto evt = file_watcher::event::accessed;
                bool matched = true;

                if ((event->mask & (IN_CREATE | IN_MOVED_TO)) != 0U) {
                    evt = file_watcher::event::created;
                } else if ((event->mask & (IN_DELETE | IN_MOVED_FROM)) != 0U) {
                    evt = file_watcher::event::deleted;
                } else if ((event->mask & (IN_MODIFY | IN_ATTRIB)) != 0U) {
                    evt = file_watcher::event::modified;
                } else if ((event->mask & IN_ACCESS) != 0U) {
                    evt = file_watcher::event::accessed;
                } else {
                    matched = false;
                }

                string prefix;
                if (recursive_) {
                    const auto it = linux_watch_map_.find(event->wd);
                    if (it != linux_watch_map_.end()) {
                        prefix = it->second;
                    }
                }

                const bool dir_created =
                        (event->mask & IN_ISDIR) != 0U && (event->mask & (IN_CREATE | IN_MOVED_TO)) != 0U;
                const bool self_gone = (event->mask & (IN_DELETE_SELF | IN_MOVE_SELF | IN_IGNORED)) != 0U;

                if (recursive_) {
                    if (dir_created && event->len > 0) {
                        string new_dir = path_prefix + prefix;
                        if (!prefix.empty()) {
                            new_dir += '/';
                        }
                        new_dir += event->name;
                        const int wd = ::inotify_add_watch(inotify_fd_, new_dir.data(), watch_mask_);
                        if (wd != -1) {
                            linux_watch_map_[wd] = prefix.empty() ? string(event->name) : prefix + "/" + event->name;
                        }
                    } else if (self_gone) {
                        linux_watch_map_.erase(event->wd);
                    }
                }

                if (matched && event->len > 0) {
                    string full = path_prefix;
                    if (!prefix.empty()) {
                        full += prefix;
                        full += '/';
                    }
                    full += event->name;
                    event_batch.push_back({move(full), evt});
                }

                ptr += sizeof(::inotify_event) + event->len;
            }

            if (!event_batch.empty()) {
                callback_t cb_copy;
                {
                    lock<mutex> lk(callback_mutex_);
                    cb_copy = callback_;
                }
                if (cb_copy) {
                    for (const auto& be: event_batch) {
                        cb_copy(path(be.file_name), be.type);
                    }
                }
            }
        }
    }
#    endif
#endif
}

#ifdef NEFORCE_PLATFORM_FREEBSD
bool file_watcher::impl::kqueue_add_watch(const string& relative, const bool directory) {
    string full = watch_path_.str();
    if (!relative.empty()) {
        full.push_back(path::preferred_separator);
        full += relative;
    }

    const int fd = ::open(full.data(), O_RDONLY | (directory ? O_DIRECTORY : 0));
    if (fd < 0) {
        return false;
    }

    struct ::kevent change{};
    EV_SET(&change, static_cast<uintptr_t>(fd), EVFILT_VNODE, EV_ADD | EV_CLEAR,
           NOTE_DELETE | NOTE_WRITE | NOTE_EXTEND | NOTE_ATTRIB | NOTE_RENAME | NOTE_REVOKE, 0, nullptr);
    if (::kevent(kq_fd_, &change, 1, nullptr, 0, nullptr) < 0) {
        ::close(fd);
        return false;
    }

    kqueue_watch info;
    info.relative = relative;
    info.directory = directory;
    kqueue_watches_[fd] = move(info);
    return true;
}

void file_watcher::impl::kqueue_remove_watch(const int fd) {
    const auto it = kqueue_watches_.find(fd);
    if (it == kqueue_watches_.end()) {
        return;
    }

    struct ::kevent change{};
    EV_SET(&change, static_cast<uintptr_t>(fd), EVFILT_VNODE, EV_DELETE, 0, 0, nullptr);
    ::kevent(kq_fd_, &change, 1, nullptr, 0, nullptr);
    ::close(fd);
    kqueue_watches_.erase(it);
}

vector<string> file_watcher::impl::kqueue_entries(const string& relative) const {
    vector<string> names;

    string full = watch_path_.str();
    if (!relative.empty()) {
        full.push_back(path::preferred_separator);
        full += relative;
    }

    ::DIR* const dir = ::opendir(full.data());
    if (dir == nullptr) {
        return names;
    }
    while (const ::dirent* entry = ::readdir(dir)) {
        const string_view name(entry->d_name);
        if (name == "." || name == "..") {
            continue;
        }
        names.emplace_back(name);
    }
    ::closedir(dir);
    return names;
}

void file_watcher::impl::kqueue_collect(const string& relative) {
    kqueue_add_watch(relative, true);
    const auto names = kqueue_entries(relative);
    kqueue_snapshots_[relative] = names;

    for (const auto& name: names) {
        const string child = relative.empty() ? name : relative + "/" + name;
        string full = watch_path_.str();
        full.push_back(path::preferred_separator);
        full += child;

        struct ::stat st{};
        if (::lstat(full.data(), &st) != 0) {
            continue;
        }
        if (S_ISDIR(st.st_mode)) {
            if (recursive_) {
                kqueue_collect(child);
            }
        } else {
            kqueue_add_watch(child, false);
        }
    }
}

void file_watcher::impl::kqueue_rescan(const string& relative) {
    const auto names = kqueue_entries(relative);
    const auto snapshot = kqueue_snapshots_.find(relative);
    const vector<string> previous = snapshot != kqueue_snapshots_.end() ? snapshot->second : vector<string>{};

    for (const auto& name: previous) {
        bool present = false;
        for (const auto& current: names) {
            if (current == name) {
                present = true;
                break;
            }
        }
        if (present) {
            continue;
        }

        const string child = relative.empty() ? name : relative + "/" + name;
        for (auto watch = kqueue_watches_.begin(); watch != kqueue_watches_.end(); ++watch) {
            if (watch->second.relative == child) {
                kqueue_remove_watch(watch->first);
                break;
            }
        }
        kqueue_emit(child, file_watcher::event::deleted);
    }

    for (const auto& name: names) {
        bool known = false;
        for (const auto& old: previous) {
            if (old == name) {
                known = true;
                break;
            }
        }
        if (known) {
            continue;
        }

        const string child = relative.empty() ? name : relative + "/" + name;
        string full = watch_path_.str();
        full.push_back(path::preferred_separator);
        full += child;

        struct ::stat st{};
        if (::lstat(full.data(), &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                if (recursive_) {
                    kqueue_collect(child);
                }
            } else {
                kqueue_add_watch(child, false);
            }
        }
        kqueue_emit(child, file_watcher::event::created);
    }

    kqueue_snapshots_[relative] = names;
}

void file_watcher::impl::kqueue_emit(const string& relative, const file_watcher::event event) {
    if ((static_cast<int>(current_events_) & static_cast<int>(event)) == 0) {
        return;
    }

    string full = watch_path_.str();
    if (!relative.empty()) {
        full.push_back(path::preferred_separator);
        full += relative;
    }

    callback_t cb;
    {
        lock<mutex> lk(callback_mutex_);
        cb = callback_;
    }
    if (cb) {
        cb(path(move(full)), event);
    }
}
#endif

bool file_watcher::impl::update_watch(const file_watcher::event events) {
    if (current_events_ == events) {
        return true;
    }
    if (!watching_.load()) {
        current_events_ = events;
        return true;
    }

    callback_t saved_callback;
    {
        lock<mutex> lk(callback_mutex_);
        saved_callback = callback_;
    }

    stop();
    current_events_ = events;

    if (saved_callback) {
        return start(move(saved_callback), events);
    }
    return true;
}

bool file_watcher::impl::update_recursive(const bool recursive) {
    if (recursive_ == recursive) {
        return true;
    }
    if (!watching_.load()) {
        recursive_ = recursive;
        return true;
    }

    callback_t saved_callback;
    auto saved_events = file_watcher::event::all;
    {
        lock<mutex> lock(callback_mutex_);
        saved_callback = callback_;
        saved_events = current_events_;
    }

    stop();
    recursive_ = recursive;

    if (saved_callback) {
        return start(move(saved_callback), saved_events);
    }
    return true;
}

NEFORCE_END_NAMESPACE__
