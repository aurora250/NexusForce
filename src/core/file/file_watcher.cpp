#include <MSTL/core/file/file_watcher.hpp>
#include <MSTL/core/string/to_string.hpp>
#include <MSTL/core/time/duration.hpp>
#ifdef MSTL_PLATFORM_LINUX__
#include <sys/inotify.h>
#include <sys/eventfd.h>
#include <poll.h>
#include <cerrno>
#endif
MSTL_BEGIN_NAMESPACE__

file_watcher::file_watcher(const path& watch_path, const bool recursive)
    : watch_path_(watch_path), recursive_(recursive) {
    if (!watch_path_.exists() || !watch_path_.is_directory()) {
        throw_exception(system_exception("Watch path must be an existing directory"));
    }
}

file_watcher::~file_watcher() {
    stop();

    if (watch_thread_.joinable()) {
        watch_thread_.join();
    }

#ifdef MSTL_PLATFORM_WINDOWS__
    if (completion_port_ != INVALID_HANDLE_VALUE) {
        ::CloseHandle(completion_port_);
        completion_port_ = INVALID_HANDLE_VALUE;
    }
    if (dir_handle_ != INVALID_HANDLE_VALUE) {
        ::CloseHandle(dir_handle_);
        dir_handle_ = INVALID_HANDLE_VALUE;
    }
#elif defined(MSTL_PLATFORM_LINUX__)
    if (event_fd_ != -1) {
        ::close(event_fd_);
        event_fd_ = -1;
    }
    if (inotify_fd_ != -1) {
        ::close(inotify_fd_);
        inotify_fd_ = -1;
    }
#endif
}

bool file_watcher::start(callback_t callback, FILE_WATCH_EVENT events) {
    if (watching_.load()) return false;

    {
        lock<mutex> lock(callback_mutex_);
        callback_ = _MSTL move(callback);
        current_events_ = events;
    }
    watching_.store(true);
    stopping_.store(false);

#ifdef MSTL_PLATFORM_WINDOWS__

    dir_handle_ = ::CreateFile(
        watch_path_.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        nullptr
    );
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

    watch_thread_ = thread(&file_watcher::watch_thread_func, this);

#elif defined(MSTL_PLATFORM_LINUX__)

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
    if (static_cast<int>(events) & static_cast<int>(FILE_WATCH_EVENT::CREATED)) {
        mask |= IN_CREATE | IN_MOVED_TO;
    }
    if (static_cast<int>(events) & static_cast<int>(FILE_WATCH_EVENT::DELETED)) {
        mask |= IN_DELETE | IN_MOVED_FROM;
    }
    if (static_cast<int>(events) & static_cast<int>(FILE_WATCH_EVENT::MODIFIED)) {
        mask |= IN_MODIFY | IN_ATTRIB;
    }
    if (static_cast<int>(events) & static_cast<int>(FILE_WATCH_EVENT::ACCESSED)) {
        mask |= IN_ACCESS;
    }

    watch_descriptor_ = ::inotify_add_watch(
        inotify_fd_,
        watch_path_.c_str(),
        mask
    );
    if (watch_descriptor_ == -1) {
        ::close(event_fd_);
        ::close(inotify_fd_);
        event_fd_ = -1;
        inotify_fd_ = -1;
        watching_.store(false);
        return false;
    }

    watch_thread_ = thread(&file_watcher::watch_thread_func, this);

#endif

    return true;
}

void file_watcher::stop() {
    if (!watching_.exchange(false)) {
        return;
    }

    stopping_.store(true);

#ifdef MSTL_PLATFORM_WINDOWS__
    if (completion_port_ != INVALID_HANDLE_VALUE) {
        ::PostQueuedCompletionStatus(completion_port_, 0, 0, nullptr);
    }
#elif defined(MSTL_PLATFORM_LINUX__)
    if (event_fd_ != -1) {
        constexpr uint64_t value = 1;
        MSTL_IGNORE ::write(event_fd_, &value, sizeof(value));
    }
#endif

    if (watch_thread_.joinable()) {
        watch_thread_.join();
    }

#ifdef MSTL_PLATFORM_WINDOWS__

    if (dir_handle_ != INVALID_HANDLE_VALUE) {
        ::CancelIoEx(dir_handle_, nullptr);
        ::CloseHandle(dir_handle_);
        dir_handle_ = INVALID_HANDLE_VALUE;
    }
    if (overlapped_.hEvent) {
        ::CloseHandle(overlapped_.hEvent);
        overlapped_.hEvent = nullptr;
    }

#elif defined(MSTL_PLATFORM_LINUX__)

    if (watch_descriptor_ != -1) {
        ::inotify_rm_watch(inotify_fd_, watch_descriptor_);
        watch_descriptor_ = -1;
    }
#endif

    {
        lock<mutex> lock(callback_mutex_);
        callback_ = nullptr;
    }
}


void file_watcher::watch_thread_func() {
    constexpr size_t BUFFER_SIZE = 4096;
    buffer_.resize(BUFFER_SIZE);

#ifdef MSTL_PLATFORM_WINDOWS__

    _MSTL memory_zero(&overlapped_);
    overlapped_.hEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!overlapped_.hEvent) {
        watching_.store(false);
        return;
    }

    ::DWORD bytes_returned = 0;
    ::BOOL result = ::ReadDirectoryChangesW(
        dir_handle_,
        buffer_.data(),
        static_cast<::DWORD>(buffer_.size()),
        recursive_ ? TRUE : FALSE,
        FILE_NOTIFY_CHANGE_FILE_NAME |
        FILE_NOTIFY_CHANGE_DIR_NAME |
        FILE_NOTIFY_CHANGE_ATTRIBUTES |
        FILE_NOTIFY_CHANGE_SIZE |
        FILE_NOTIFY_CHANGE_LAST_WRITE |
        FILE_NOTIFY_CHANGE_LAST_ACCESS |
        FILE_NOTIFY_CHANGE_CREATION |
        FILE_NOTIFY_CHANGE_SECURITY,
        &bytes_returned,
        &overlapped_,
        nullptr
    );

    if (!result) {
        ::CloseHandle(overlapped_.hEvent);
        overlapped_.hEvent = nullptr;
        watching_.store(false);
        return;
    }

    while (watching_.load() && !stopping_.load()) {
        ::DWORD bytes_transferred = 0;
        ::ULONG_PTR completion_key = 0;
        ::LPOVERLAPPED lpOverlapped = nullptr;

        const ::BOOL io_completed = ::GetQueuedCompletionStatus(
            completion_port_,
            &bytes_transferred,
            &completion_key,
            &lpOverlapped,
            1000
        );

        if (!io_completed) {
            const ::DWORD error = ::GetLastError();
            if (error == WAIT_TIMEOUT) {
                continue;
            }
            break;
        }

        if (bytes_transferred == 0 && completion_key == 0 && lpOverlapped == nullptr) {
            break;
        }

        if (bytes_transferred > 0) {
            auto* fni = reinterpret_cast<::FILE_NOTIFY_INFORMATION*>(buffer_.data());

            while (fni && watching_.load()) {
                const wstring wide_filename(fni->FileName, fni->FileNameLength / sizeof(wchar_t));
                const string utf8_filename = to_string(wide_filename);
                path full_path = watch_path_ / path(utf8_filename);
                auto event_type = FILE_WATCH_EVENT::ACCESSED;

                switch (fni->Action) {
                    case FILE_ACTION_ADDED:
                    case FILE_ACTION_RENAMED_NEW_NAME: {
                        event_type = FILE_WATCH_EVENT::CREATED;
                        break;
                    }
                    case FILE_ACTION_MODIFIED: {
                        event_type = FILE_WATCH_EVENT::MODIFIED;
                        break;
                    }
                    case FILE_ACTION_REMOVED:
                    case FILE_ACTION_RENAMED_OLD_NAME: {
                        event_type = FILE_WATCH_EVENT::DELETED;
                        break;
                    }
                    default: {
                        break;
                    }
                }

                if (callback_) {
                    lock<mutex> lock(callback_mutex_);
                    if (callback_) {
                        callback_(full_path, move(event_type));
                    }
                }

                if (fni->NextEntryOffset == 0) {
                    fni = nullptr;
                } else {
                    fni = reinterpret_cast<::FILE_NOTIFY_INFORMATION*>(
                        reinterpret_cast<::BYTE*>(fni) + fni->NextEntryOffset
                    );
                }
            }
        }

        if (watching_.load()) {
            ::ResetEvent(overlapped_.hEvent);
            bytes_returned = 0;
            result = ::ReadDirectoryChangesW(
                dir_handle_,
                buffer_.data(),
                static_cast<::DWORD>(buffer_.size()),
                recursive_ ? TRUE : FALSE,
                FILE_NOTIFY_CHANGE_FILE_NAME |
                FILE_NOTIFY_CHANGE_DIR_NAME |
                FILE_NOTIFY_CHANGE_ATTRIBUTES |
                FILE_NOTIFY_CHANGE_SIZE |
                FILE_NOTIFY_CHANGE_LAST_WRITE |
                FILE_NOTIFY_CHANGE_LAST_ACCESS |
                FILE_NOTIFY_CHANGE_CREATION |
                FILE_NOTIFY_CHANGE_SECURITY,
                &bytes_returned,
                &overlapped_,
                nullptr
            );

            if (!result) {
                break;
            }
        }
    }

    if (overlapped_.hEvent) {
        ::CloseHandle(overlapped_.hEvent);
        overlapped_.hEvent = nullptr;
    }

#elif defined(MSTL_PLATFORM_LINUX__)

    ::pollfd fds[2];
    fds[0].fd = inotify_fd_;
    fds[0].events = POLLIN;
    fds[1].fd = event_fd_;
    fds[1].events = POLLIN;

    while (watching_.load() && !stopping_.load()) {
        const int poll_result = ::poll(fds, 2, 1000);
        if (poll_result == -1) {
            if (errno == EINTR) {
                continue;
            }
            break;
        } else if (poll_result == 0) {
            continue;
        }

        if (fds[1].revents & POLLIN) {
            uint64_t value;
            MSTL_IGNORE ::read(event_fd_, &value, sizeof(value));
            break;
        }

        if (fds[0].revents & POLLIN) {
            const ssize_t len = ::read(inotify_fd_, buffer_.data(), BUFFER_SIZE);
            if (len <= 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                }
                break;
            }

            char* ptr = buffer_.data();
            while (ptr < buffer_.data() + len) {
                auto* event = reinterpret_cast<struct ::inotify_event*>(ptr);
                auto evt = FILE_WATCH_EVENT::ACCESSED;

                if (event->mask & (IN_CREATE | IN_MOVED_TO)) {
                    evt = FILE_WATCH_EVENT::CREATED;
                } else if (event->mask & (IN_DELETE | IN_MOVED_FROM)) {
                    evt = FILE_WATCH_EVENT::DELETED;
                } else if (event->mask & (IN_MODIFY | IN_ATTRIB)) {
                    evt = FILE_WATCH_EVENT::MODIFIED;
                } else if (event->mask & IN_ACCESS) {
                    evt = FILE_WATCH_EVENT::ACCESSED;
                } else {
                    ptr += sizeof(::inotify_event) + event->len;
                    continue;
                }

                if (callback_ && event->len > 0) {
                    path full_path = watch_path_ / path(event->name);
                    lock<mutex> lock(callback_mutex_);
                    if (callback_) {
                        callback_(full_path, move(evt));
                    }
                }
                ptr += sizeof(::inotify_event) + event->len;
            }
        }
    }
#endif
}

bool file_watcher::update_watch(const FILE_WATCH_EVENT events) {
    if (current_events_ == events) {
        return true;
    }
    if (!watching_.load()) {
        current_events_ = events;
        return false;
    }

    callback_t saved_callback;

    {
        lock<mutex> lock(callback_mutex_);
        saved_callback = callback_;
    }

    stop();
    current_events_ = events;

    if (saved_callback) {
        return start(_MSTL move(saved_callback), events);
    }
    return true;
}

bool file_watcher::update_recursive(const bool recursive) {
    if (recursive_ == recursive) {
        return true;
    }
    if (!watching_.load()) {
        recursive_ = recursive;
        return true;
    }

    callback_t saved_callback;
    FILE_WATCH_EVENT saved_events;
    {
        lock<mutex> lock(callback_mutex_);
        saved_callback = callback_;
        saved_events = current_events_;
    }

    stop();
    recursive_ = recursive;

    if (saved_callback) {
        return start(_MSTL move(saved_callback), saved_events);
    }
    return true;
}

MSTL_END_NAMESPACE__
