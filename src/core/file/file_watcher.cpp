#include <MSTL/core/file/file_watcher.hpp>
#include <MSTL/core/string/to_string.hpp>
#include <MSTL/core/time/duration.hpp>
#ifdef MSTL_PLATFORM_LINUX__
#include <sys/inotify.h>
#endif
MSTL_BEGIN_NAMESPACE__

file_watcher::file_watcher(const path& watch_path, bool recursive)
    : watch_path_(watch_path), recursive_(recursive) {
#ifdef MSTL_PLATFORM_LINUX__
    inotify_fd_ = ::inotify_init();
    if (inotify_fd_ == -1) {
        // 错误处理
    }
#endif
}

file_watcher::~file_watcher() {
    stop();
#ifdef MSTL_PLATFORM_LINUX__
    if (inotify_fd_ != -1) {
        ::close(inotify_fd_);
    }
#endif
}

bool file_watcher::start(callback_t callback) {
    if (watching_.load()) return false;

    callback_ = _MSTL move(callback);
    watching_.store(true);

#ifdef MSTL_PLATFORM_WINDOWS__
    watch_thread_ = thread(&file_watcher::watch_thread_func_windows, this);
#elif defined(MSTL_PLATFORM_LINUX__)
    constexpr uint32_t mask = IN_CREATE | IN_MODIFY | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO | IN_ACCESS;
    watch_descriptor_ = ::inotify_add_watch(inotify_fd_, watch_path_.c_str(), mask);
    if (watch_descriptor_ == -1) {
        watching_.store(false);
        return false;
    }
    watch_thread_ = thread(&file_watcher::watch_thread_func_linux, this);
#endif

    return true;
}

void file_watcher::stop() {
    if (!watching_.exchange(false)) return;

    if (watch_thread_.joinable()) {
        watch_thread_.join();
    }

#ifdef MSTL_PLATFORM_WINDOWS__
    if (dir_handle_ != INVALID_HANDLE_VALUE) {
        ::CancelIo(dir_handle_);
        ::CloseHandle(dir_handle_);
        dir_handle_ = INVALID_HANDLE_VALUE;
    }
#elif defined(MSTL_PLATFORM_LINUX__)
    if (watch_descriptor_ != -1) {
        ::inotify_rm_watch(inotify_fd_, watch_descriptor_);
        watch_descriptor_ = -1;
    }
#endif
}


#ifdef MSTL_PLATFORM_WINDOWS__
void file_watcher::watch_thread_func_windows() {
    constexpr size_t BUFFER_SIZE = 4096;
    buffer_.resize(BUFFER_SIZE);

    dir_handle_ = ::CreateFileA(
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
        return;
    }

    _MSTL fill_n(reinterpret_cast<char*>(&overlapped_), sizeof(overlapped_), 0);
    overlapped_.hEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);

    while (watching_.load()) {
        ::DWORD bytes_returned = 0;
        const ::BOOL result = ::ReadDirectoryChangesW(
            dir_handle_,
            buffer_.data(),
            static_cast<::DWORD>(buffer_.size()),
            recursive_,
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
            FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE |
            FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_LAST_ACCESS |
            FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_SECURITY,
            &bytes_returned,
            &overlapped_,
            nullptr
        );

        if (!result) break;

        const ::DWORD wait_result = ::WaitForSingleObject(overlapped_.hEvent, 1000);
        if (wait_result == WAIT_OBJECT_0) {
            if (!watching_.load()) break;

            ::DWORD bytes_transferred = 0;
            if (!::GetOverlappedResult(dir_handle_, &overlapped_, &bytes_transferred, FALSE)) {
                break;
            }

            if (bytes_transferred > 0) {
                auto* fni = reinterpret_cast<::FILE_NOTIFY_INFORMATION*>(buffer_.data());

                while (fni) {
                    const wstring wide_filename(fni->FileName, fni->FileNameLength / sizeof(wchar_t));
                    path full_path = watch_path_ / path(to_string(wide_filename));

                    FILE_WATCH_EVENT event_type;
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
                            event_type = FILE_WATCH_EVENT::ACCESSED;
                            break;
                        }
                    }

                    if (callback_) {
                        callback_(full_path, _MSTL move(event_type));
                    }

                    if (fni->NextEntryOffset == 0) {
                        fni = nullptr;
                    } else {
                        fni = reinterpret_cast<::FILE_NOTIFY_INFORMATION*>(
                            reinterpret_cast<char*>(fni) + fni->NextEntryOffset
                        );
                    }
                }
            }

            ::ResetEvent(overlapped_.hEvent);
        } else if (wait_result != WAIT_TIMEOUT) {
            break;
        }
    }

    if (overlapped_.hEvent) {
        ::CloseHandle(overlapped_.hEvent);
    }
}
#endif


#ifdef MSTL_PLATFORM_LINUX__
void file_watcher::watch_thread_func_linux() {
    constexpr size_t BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];

    while (watching_.load()) {
        const ssize_t len = ::read(inotify_fd_, buffer, BUFFER_SIZE);
        if (len <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                this_thread::sleep_for(chrono::milliseconds(100));
                continue;
            }
            break;
        }

        char* ptr = buffer;
        while (ptr < buffer + len) {
            auto* event = reinterpret_cast<struct ::inotify_event*>(ptr);

            FILE_WATCH_EVENT evt;
            if (event->mask & IN_CREATE) {
                evt = FILE_WATCH_EVENT::CREATED;
            } else if (event->mask & IN_MODIFY) {
                evt = FILE_WATCH_EVENT::MODIFIED;
            } else if (event->mask & IN_DELETE) {
                evt = FILE_WATCH_EVENT::DELETED;
            } else if (event->mask & (IN_MOVED_FROM | IN_MOVED_TO)) {
                evt = FILE_WATCH_EVENT::RENAMED;
            } else if (event->mask & IN_ACCESS) {
                evt = FILE_WATCH_EVENT::ACCESSED;
            } else {
                ptr += sizeof(::inotify_event) + event->len;
                continue;
            }

            if (callback_ && event->len > 0) {
                path full_path = watch_path_ / path(event->name);
                callback_(full_path, _MSTL move(evt));
            }
            ptr += sizeof(::inotify_event) + event->len;
        }
    }
}
#endif

MSTL_END_NAMESPACE__
