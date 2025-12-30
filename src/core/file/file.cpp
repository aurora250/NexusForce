#include <MSTL/core/file/file.hpp>
#include <MSTL/core/system/sysinfo.hpp>
#ifdef MSTL_PLATFORM_LINUX__
#include <sys/file.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <ctime>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#endif
#ifdef MSTL_PLATFORM_WINDOWS__
#include <winioctl.h>
#include <memoryapi.h>
#endif
MSTL_BEGIN_NAMESPACE__

file::async_context::async_context(string&& d)
: data(_MSTL move(d)), is_write(true) {
    cb = new aiocb_type{};
    _MSTL memory_zero(cb, sizeof(aiocb_type));
#ifdef MSTL_PLATFORM_WINDOWS__
    cb->hEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
#endif
}

file::async_context::async_context(string* buf)
: buffer(buf), is_write(false) {
    cb = new aiocb_type{};
    _MSTL memory_zero(cb, sizeof(aiocb_type));
#ifdef MSTL_PLATFORM_WINDOWS__
    cb->hEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
#endif
}

file::async_context::~async_context() {
    if (cb) {
#ifdef MSTL_PLATFORM_WINDOWS__
        if (cb->hEvent) ::CloseHandle(cb->hEvent);
#endif
        delete cb;
    }
}

bool file::complete_async_result(async_result& result, const size_type bytes_transferred) {
    result.completed = true;
    result.bytes_transferred = bytes_transferred;
    result.error_code = 0;

    lock_guard<mutex> lock(async_mutex_);

#ifdef MSTL_PLATFORM_WINDOWS__
    if (result.cb) {
        auto it = _MSTL find(async_operations_.begin(), async_operations_.end(), result.cb);
        if (it != async_operations_.end()) {
            async_operations_.erase(it);
        }

        auto ctx_it = async_contexts_.find(result.cb);
        if (ctx_it != async_contexts_.end()) {
            delete ctx_it->second;
            async_contexts_.erase(ctx_it);
        }

        result.cb = nullptr;
    }
#elif defined(MSTL_PLATFORM_LINUX__)
    if (result.cb) {
        auto it = _MSTL find(async_operations_.begin(), async_operations_.end(), result.cb);
        if (it != async_operations_.end()) {
            async_operations_.erase(it);
        }

        auto ctx_it = async_contexts_.find(result.cb);
        if (ctx_it != async_contexts_.end()) {
            delete ctx_it->second;
            async_contexts_.erase(ctx_it);
        }

        result.cb = nullptr;
    }
#endif

    result.user_context = nullptr;
    return true;
}

bool file::check_async_completion(async_result& result) {
#ifdef MSTL_PLATFORM_LINUX__
    if (!result.cb) {
        result.error_code = EINVAL;
        return false;
    }

    const int error = ::aio_error(result.cb);
    if (error == 0) {
        const ssize_t ret = ::aio_return(result.cb);
        if (ret >= 0) {
            return complete_async_result(result, static_cast<size_type>(ret));
        } else {
            set_last_error();
            result.error_code = last_error_code_;
            return false;
        }
    } else if (error == EINPROGRESS) {
        result.error_code = EINPROGRESS;
        return false;
    } else {
        result.error_code = error;
        last_error_code_ = error;
        last_error_msg_ = ::strerror(error);
        return false;
    }
#else
    return false;
#endif
}

bool file::flush_write_buffer() const noexcept {
    if (write_buffer_pos_ == 0) return true;

#ifdef MSTL_PLATFORM_WINDOWS__
    size_type bytes_written;
    const ::BOOL success = ::WriteFile(
        handle_,
        write_buffer_.data(),
        write_buffer_pos_,
        &bytes_written,
        nullptr
    );
    if (!success || bytes_written != write_buffer_pos_) {
        return false;
    }
#elif defined(MSTL_PLATFORM_LINUX__)
    ssize_t total_written = 0;
    while (total_written < static_cast<ssize_t>(write_buffer_pos_)) {
        const ssize_t bytes_written = ::write(
            handle_,
            write_buffer_.data() + total_written,
            write_buffer_pos_ - total_written
        );
        if (bytes_written == -1) {
            if (errno == EINTR) continue;
            return false;
        }
        total_written += bytes_written;
    }
#endif

    write_buffer_pos_ = 0;
    return true;
}

bool file::fill_read_buffer() const noexcept {
    if (read_buffer_.empty()) {
        return false;
    }

#ifdef MSTL_PLATFORM_WINDOWS__
    size_type bytes_read;
    const ::BOOL success = ::ReadFile(
        handle_, read_buffer_.data(),
        static_cast<::DWORD>(buffer_size_),
        &bytes_read, nullptr
    );
    if (!success) {
        read_buffer_size_ = 0;
        return false;
    }
    read_buffer_size_ = bytes_read;
#elif defined(MSTL_PLATFORM_LINUX__)
    ssize_t bytes_read;
    do {
        bytes_read = ::read(handle_, read_buffer_.data(), buffer_size_);
    } while (bytes_read == -1 && errno == EINTR);

    if (bytes_read <= 0) {
        read_buffer_size_ = 0;
        return false;
    }
    read_buffer_size_ = static_cast<size_type>(bytes_read);
#endif

    read_buffer_pos_ = 0;
    return true;
}

datetime file::filetime_to_datetime(const time_type& ft) noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    if (ft.dwHighDateTime == 0 && ft.dwLowDateTime == 0) {
        return datetime::epoch();
    }
    ::SYSTEMTIME st_utc;
    if (!::FileTimeToSystemTime(&ft, &st_utc)) {
        return datetime::epoch();
    }
    ::SYSTEMTIME st_local;
    if (!::SystemTimeToTzSpecificLocalTime(nullptr, &st_utc, &st_local)) {
        return datetime::epoch();
    }
    return datetime(
        st_local.wYear, st_local.wMonth, st_local.wDay,
        st_local.wHour, st_local.wMinute, st_local.wSecond
    );
#elif defined(MSTL_PLATFORM_LINUX__)
    if (ft == 0) return datetime::epoch();
    ::tm tm_local{};
    ::localtime_r(&ft, &tm_local);
    return datetime(
        tm_local.tm_year + 1900, tm_local.tm_mon + 1, tm_local.tm_mday,
        tm_local.tm_hour, tm_local.tm_min, tm_local.tm_sec, tm_local.tm_gmtoff
    );
#endif
}

file::time_type file::datetime_to_filetime(const datetime& dt) noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    time_type ft = {0, 0};
    ::SYSTEMTIME st_local;
    st_local.wYear = static_cast<::WORD>(dt.year());
    st_local.wMonth = static_cast<::WORD>(dt.month());
    st_local.wDay = static_cast<::WORD>(dt.day());
    st_local.wHour = static_cast<::WORD>(dt.hours());
    st_local.wMinute = static_cast<::WORD>(dt.minutes());
    st_local.wSecond = static_cast<::WORD>(dt.seconds());
    st_local.wMilliseconds = 0;

    ::SYSTEMTIME st_utc;
    if (!::TzSpecificLocalTimeToSystemTime(nullptr, &st_local, &st_utc)) {
        return ft;
    }
    if (!::SystemTimeToFileTime(&st_utc, &ft)) {
        ft.dwHighDateTime = 0;
        ft.dwLowDateTime = 0;
    }
    return ft;
#elif defined(MSTL_PLATFORM_LINUX__)
    ::tm tm_val{};
    tm_val.tm_year = dt.year() - 1900;
    tm_val.tm_mon = dt.month() - 1;
    tm_val.tm_mday = dt.day();
    tm_val.tm_hour = dt.hours();
    tm_val.tm_min = dt.minutes();
    tm_val.tm_sec = dt.seconds();
    tm_val.tm_gmtoff = dt.offset_seconds();
    tm_val.tm_isdst = -1;
    return ::mktime(&tm_val);
#endif
}

string file::get_last_error_msg() {
#ifdef MSTL_PLATFORM_WINDOWS__
    const size_type error_code = ::GetLastError();
    if (error_code == 0) {
        return {};
    }
    ::LPSTR message_buffer = nullptr;
    const size_type size = ::FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<::LPSTR>(&message_buffer), 0, nullptr);

    string message(message_buffer, size);
    ::LocalFree(message_buffer);
    return message;
#elif defined(MSTL_PLATFORM_LINUX__)
    char buffer[256];
    return ::strerror_r(errno, buffer, sizeof(buffer));
#endif
}

void file::set_last_error() const {
#ifdef MSTL_PLATFORM_WINDOWS__
    last_error_code_ = static_cast<int>(::GetLastError());
#elif defined(MSTL_PLATFORM_LINUX__)
    last_error_code_ = errno;
#endif
    last_error_msg_ = get_last_error_msg();
}

void file::adjust_buffer_size() {
    if (!opened_ || handle_ == INVALID_HANDLE()) return;

    const size_type file_sz = size();

    if (file_sz == 0) {
        buffer_size_ = FILE_BUFFER_SIZE / 4;
    } else if (file_sz < FILE_BUFFER_SIZE) {
        buffer_size_ = file_sz;
    } else if (file_sz > FILE_BUFFER_SIZE * 1000) {
        buffer_size_ = FILE_BUFFER_SIZE * 8;
    } else if (file_sz > FILE_BUFFER_SIZE * 100) {
        buffer_size_ = FILE_BUFFER_SIZE * 4;
    } else {
        buffer_size_ = FILE_BUFFER_SIZE;
    }

    read_buffer_.resize(buffer_size_);
    write_buffer_.resize(buffer_size_);
}

#ifdef MSTL_PLATFORM_LINUX__
::mode_t file::convert_attributes(const FILE_ATTRI attr) {
    ::mode_t mode = 0;

    if ((attr & FILE_ATTRI::READONLY) != FILE_ATTRI::OTHERS) {
        mode |= S_IRUSR | S_IRGRP | S_IROTH;
    } else {
        mode |= S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    }
    return mode;
}
#endif


file::file(file&& other) noexcept
    : handle_(other.handle_),
      path_(_MSTL move(other.path_)),
      opened_(other.opened_),
      append_mode_(other.append_mode_),
      read_buffer_(_MSTL move(other.read_buffer_)),
      read_buffer_pos_(other.read_buffer_pos_),
      read_buffer_size_(other.read_buffer_size_),
      write_buffer_(_MSTL move(other.write_buffer_)),
      write_buffer_pos_(other.write_buffer_pos_),
      mapped_ptr_(other.mapped_ptr_),
      mapped_size_(other.mapped_size_),
#ifdef MSTL_PLATFORM_WINDOWS__
      mapping_handle_(other.mapping_handle_),
#endif
      last_error_msg_(_MSTL move(other.last_error_msg_)),
      last_error_code_(other.last_error_code_) {

    other.handle_ = INVALID_HANDLE();
    other.opened_ = false;
    other.append_mode_ = false;
    other.path_ = _MSTL path{};
    other.read_buffer_.clear();
    other.read_buffer_pos_ = 0;
    other.read_buffer_size_ = 0;
    other.write_buffer_.clear();
    other.write_buffer_pos_ = 0;
    other.mapped_ptr_ = nullptr;
    other.mapped_size_ = 0;
#ifdef MSTL_PLATFORM_WINDOWS__
    other.mapping_handle_ = INVALID_HANDLE_VALUE;
#endif
    other.last_error_code_ = 0;
}

file& file::operator =(file&& other) noexcept {
    if (this == _MSTL addressof(other)) return *this;

    this->close();
    handle_ = other.handle_;
    path_ = _MSTL move(other.path_);
    opened_ = other.opened_;
    append_mode_ = other.append_mode_;
    read_buffer_ = _MSTL move(other.read_buffer_);
    read_buffer_pos_ = other.read_buffer_pos_;
    read_buffer_size_ = other.read_buffer_size_;
    write_buffer_ = _MSTL move(other.write_buffer_);
    write_buffer_pos_ = other.write_buffer_pos_;

    other.handle_ = INVALID_HANDLE();
    other.opened_ = false;
    other.append_mode_ = false;
    other.path_ = _MSTL path{};
    other.read_buffer_.clear();
    other.read_buffer_pos_ = 0;
    other.read_buffer_size_ = 0;
    other.write_buffer_.clear();
    other.write_buffer_pos_ = 0;

    return *this;
}

file::~file() {
    unmap();

    lock_guard<mutex> lock(async_mutex_);

#ifdef MSTL_PLATFORM_WINDOWS__
    for (auto* ov : async_operations_) {
        if (ov) {
            ::CancelIoEx(handle_, ov);
            size_type bytes_transferred = 0;
            ::GetOverlappedResult(handle_, ov, &bytes_transferred, TRUE);

            auto ctx_it = async_contexts_.find(ov);
            if (ctx_it != async_contexts_.end()) {
                delete ctx_it->second;
            }

            if (ov->hEvent) {
                ::CloseHandle(ov->hEvent);
            }
            delete ov;
        }
    }
#elif defined(MSTL_PLATFORM_LINUX__)
    for (auto* aiocb : async_operations_) {
        if (aiocb) {
            ::aio_cancel(handle_, aiocb);

            const ::aiocb* list[1] = { aiocb };
            ::aio_suspend(list, 1, nullptr);
            ::aio_return(aiocb);

            auto ctx_it = async_contexts_.find(aiocb);
            if (ctx_it != async_contexts_.end()) {
                delete ctx_it->second;
            }

            delete aiocb;
        }
    }
#endif
    async_operations_.clear();
    async_contexts_.clear();

    this->close();
}

bool file::open(_MSTL path p, const bool append,
    FILE_ACCESS access,
    FILE_SHARED share_mode,
    FILE_CREATION creation,
    FILE_ATTRI attributes) {
    this->close();
    clear_error();

    read_buffer_.resize(buffer_size_);
    write_buffer_.resize(buffer_size_);
    read_buffer_pos_ = 0;
    read_buffer_size_ = 0;
    write_buffer_pos_ = 0;

#ifdef MSTL_PLATFORM_WINDOWS__
    handle_ = ::CreateFileA(
        p.c_str(),
        static_cast<fud_t>(access),
        static_cast<fud_t>(share_mode),
        nullptr,
        static_cast<fud_t>(creation),
        static_cast<fud_t>(attributes),
        nullptr
    );
#elif defined(MSTL_PLATFORM_LINUX__)
    auto flags = static_cast<fud_t>(access);
    const auto creation_flags = static_cast<fud_t>(creation);

    if ((creation_flags & O_CREAT) && !(flags & (O_RDONLY | O_WRONLY | O_RDWR))) {
        flags |= O_RDWR;
    }
    flags |= creation_flags;

    if (append) {
        flags |= O_APPEND;
        if (creation_flags & O_TRUNC) {
            flags &= ~O_TRUNC;
        }
    }

    ::mode_t mode = 0;
    if (creation_flags & O_CREAT) {
        if (attributes == FILE_ATTRI::OTHERS) {
            mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
        } else {
            mode = convert_attributes(attributes);
            if (mode == 0) {
                mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
            }
        }
    }

    if (creation_flags & O_CREAT) {
        handle_ = ::open(p.c_str(), flags, mode);
    } else {
        handle_ = ::open(p.c_str(), flags);
    }
#endif

    if (handle_ == INVALID_HANDLE()) {
        set_last_error();
        return false;
    }
#ifdef MSTL_PLATFORM_LINUX__
    else {
        const int fd_flags = ::fcntl(handle_, F_GETFD);
        if (fd_flags != -1) {
            // close-on-exec
            ::fcntl(handle_, F_SETFD, fd_flags | FD_CLOEXEC);
        }
    }
#endif

    path_ = _MSTL move(p);
    opened_ = true;
    append_mode_ = append;

    if (append && !this->seek(0, FILE_POINTER::END)) {
        set_last_error();
        return false;
    }

    adjust_buffer_size();
    return true;
}

bool file::open(const bool append,
    const FILE_ACCESS access,
    const FILE_SHARED share_mode,
    const FILE_CREATION creation,
    const FILE_ATTRI attributes) {
    return this->open(path_, append, access, share_mode, creation, attributes);
}

void file::close() noexcept {
    if (opened_ && handle_ != INVALID_HANDLE()) {
        MSTL_IGNORE this->flush();
#ifdef MSTL_PLATFORM_WINDOWS__
        ::CloseHandle(handle_);
#elif defined(MSTL_PLATFORM_LINUX__)
        ::close(handle_);
#endif
        handle_ = INVALID_HANDLE();
        opened_ = false;
        append_mode_ = false;
        read_buffer_.clear();
        read_buffer_pos_ = 0;
        read_buffer_size_ = 0;
        write_buffer_.clear();
        write_buffer_pos_ = 0;
        buffer_size_ = FILE_BUFFER_SIZE;
        async_operations_.clear();
    }
}

bool file::flush() const noexcept {
    if (!opened_ || handle_ == INVALID_HANDLE()) return false;
    if (!flush_write_buffer()) return false;

#ifdef MSTL_PLATFORM_WINDOWS__
    return ::FlushFileBuffers(handle_) != 0;
#elif defined(MSTL_PLATFORM_LINUX__)
    return ::fdatasync(handle_) == 0;
#endif
}

file::size_type file::write(const string& data, const size_type size) {
    if (!opened_ || handle_ == INVALID_HANDLE()) return 0;
    const size_type real_size = size > data.size() ? data.size() : size;
    return file::write(data.data(), real_size);
}

file::size_type file::write(const string& data) {
    return this->write(data, data.size());
}

file::size_type file::write(const void* data, const size_type size) {
    if (!opened_ || handle_ == INVALID_HANDLE() || !data) return 0;
    if (size == 0) return 0;

    if (append_mode_ && !seek(0, FILE_POINTER::END)) {
        set_last_error();
        return 0;
    }

    if (size > buffer_size_ * 4) {
        if (!flush_write_buffer()) return 0;

        size_type total_written = 0;
        const auto ptr = static_cast<const byte_t*>(data);

        while (total_written < size) {
#ifdef MSTL_PLATFORM_WINDOWS__
            size_type bytes_written = 0;
            const size_type to_write = _MSTL min<size_type>(
                size - total_written,
                numeric_limits<size_type>::max()
            );
            if (!::WriteFile(handle_, ptr + total_written, to_write, &bytes_written, nullptr)) {
                set_last_error();
                break;
            }
            total_written += bytes_written;
            if (bytes_written != to_write) break;
#elif defined(MSTL_PLATFORM_LINUX__)
            const ssize_t written = ::write(
                handle_, ptr + total_written, size - total_written);
            if (written == -1) {
                if (errno == EINTR) continue;
                set_last_error();
                break;
            }
            if (written == 0) break;
            total_written += static_cast<size_type>(written);
#endif
        }
        return total_written;
    }

    auto ptr = static_cast<const char*>(data);
    size_type total_written = 0;
    size_type remaining = size;

    while (remaining > 0) {
        const size_type available = buffer_size_ - write_buffer_pos_;
        const size_type to_copy = _MSTL min(remaining, available);

        _MSTL copy_n(ptr, to_copy, write_buffer_.begin() + write_buffer_pos_);
        write_buffer_pos_ += to_copy;
        total_written += to_copy;
        ptr += to_copy;
        remaining -= to_copy;

        if (write_buffer_pos_ == buffer_size_ && !flush_write_buffer()) {
            set_last_error();
            break;
        }
    }

    if (append_mode_ && !seek(0, FILE_POINTER::END)) {
        set_last_error();
        return 0;
    }

    return total_written;
}

file::size_type file::read(string& str, const size_type size) const {
    if (!opened_ || handle_ == INVALID_HANDLE()) return 0;
    str.clear();
    if (size == 0) return 0;
    str.resize(size);
    return file::read(str.data(), size);
}

file::size_type file::read(void* buffer, const size_type size) const {
    if (!opened_ || handle_ == INVALID_HANDLE() || !buffer) return 0;
    if (size == 0) return 0;

    auto ptr = static_cast<char*>(buffer);
    size_type total_read = 0;
    size_type remaining = size;

    while (remaining > 0) {
        if (read_buffer_pos_ >= read_buffer_size_) {
            if (!fill_read_buffer() || read_buffer_size_ == 0) {
                break;
            }
        }

        const size_type available_in_buffer = read_buffer_size_ - read_buffer_pos_;
        const size_type to_read = _MSTL min(remaining, available_in_buffer);

        _MSTL copy_n(read_buffer_.data() + read_buffer_pos_, to_read, ptr);
        read_buffer_pos_ += to_read;
        ptr += to_read;
        total_read += to_read;
        remaining -= to_read;
    }

    return total_read;
}

file::size_type file::read(string& str) const {
    return this->read(str, str.size());
}

string file::read() const {
    if (!opened_ || handle_ == INVALID_HANDLE()) return {};

    const size_type file_size = size();
    if (file_size == 0) return {};

    const difference_type current_pos = this->tell();
    if (!this->seek(0, FILE_POINTER::BEGIN)) return {};

    string content;
    content.resize(file_size);
    const size_type bytes_read = this->read(content, file_size);

    if (!this->seek(current_pos, FILE_POINTER::BEGIN)) return {};

    if (bytes_read != file_size) content.resize(bytes_read);
    return content;
}

vector<string> file::read_chunks(const size_type chunk_size) {
    vector<string> chunks;
    if (!opened_ || handle_ == INVALID_HANDLE()) return chunks;

    const size_type file_sz = size();
    if (file_sz == 0) return chunks;

    const difference_type original_pos = tell();
    if (original_pos == static_cast<difference_type>(-1)) {
        set_last_error();
        return chunks;
    }

    if (!seek(0, FILE_POINTER::BEGIN)) {
        set_last_error();
        return chunks;
    }

    size_type remaining = file_sz;
    const size_type direct_threshold = chunk_size * 2;

    while (remaining > 0) {
        const size_type to_read = remaining < chunk_size ? remaining : chunk_size;
        string chunk;

        if (to_read > direct_threshold) {
            chunk.resize(to_read);
            size_type bytes_read = 0;

            read_buffer_pos_ = read_buffer_size_;
            char* data = chunk.data();

            while (bytes_read < to_read) {
#ifdef MSTL_PLATFORM_WINDOWS__
                size_type bytes_read_now = 0;
                const size_type to_read_now = _MSTL min<size_type>(
                    to_read - bytes_read,
                    numeric_limits<size_type>::max()
                );

                if (!::ReadFile(handle_, data + bytes_read,
                    to_read_now, &bytes_read_now, nullptr)) {
                    set_last_error();
                    break;
                              }
                bytes_read += bytes_read_now;
                if (bytes_read_now == 0) break;
#elif defined(MSTL_PLATFORM_LINUX__)
                const ssize_t bytes_read_now = ::read(handle_,
                    data + bytes_read, to_read - bytes_read);
                if (bytes_read_now == -1) {
                    if (errno == EINTR) continue;
                    set_last_error();
                    break;
                }
                if (bytes_read_now == 0) break;
                bytes_read += static_cast<size_type>(bytes_read_now);
#endif
            }
            if (bytes_read < to_read) {
                chunk.resize(bytes_read);
            }
        } else {
            chunk.resize(to_read);
            const size_type bytes_read = read(chunk, to_read);
            if (bytes_read != to_read) {
                chunk.resize(bytes_read);
            }
        }

        if (!chunk.empty()) {
            chunks.push_back(_MSTL move(chunk));
        } else {
            break;
        }

        remaining -= chunk.size();
    }

    if(!seek(original_pos, FILE_POINTER::BEGIN)) {
        set_last_error();
    }
    return chunks;
}

bool file::write_chunks(const vector<string>& chunks) {
    if (!opened_ || handle_ == INVALID_HANDLE()) return false;

    const difference_type original_pos = tell();

    if (append_mode_ && !seek(0, FILE_POINTER::END)) {
        set_last_error();
        return false;
    }
    if (!flush_write_buffer()) {
        set_last_error();
        return false;
    }

    bool success = true;
    size_type total_written = 0;

    for (const auto& chunk : chunks) {
        if (chunk.empty()) continue;

        const char* data = chunk.data();
        size_type remaining = chunk.size();

        while (remaining > 0) {
            size_type bytes_written;
            if (remaining > buffer_size_ * 4) {
                if (!flush_write_buffer()) {
                    success = false;
                    break;
                }

#ifdef MSTL_PLATFORM_WINDOWS__
                const size_type to_write = _MSTL min<size_type>(
                    remaining,
                    numeric_limits<size_type>::max()
                );
                size_type written_now = 0;

                if (!::WriteFile(handle_, data + (chunk.size() - remaining),
                    to_write, &written_now, nullptr)) {
                    set_last_error();
                    success = false;
                    break;
                }
                bytes_written = written_now;
#elif defined(MSTL_PLATFORM_LINUX__)
                const ssize_t written_now = ::write(handle_,
                                            data + (chunk.size() - remaining),
                                            remaining);
                if (written_now == -1) {
                    if (errno == EINTR) continue;
                    set_last_error();
                    success = false;
                    break;
                }
                bytes_written = static_cast<size_type>(written_now);
#endif
            } else {
                bytes_written = write(chunk, remaining);
            }

            if (bytes_written == 0) {
                success = false;
                break;
            }
            remaining -= bytes_written;
            total_written += bytes_written;
        }
        if (!success) break;
    }

    if (!flush()) {
        success = false;
    }
    if (!append_mode_ && original_pos >= 0 &&
        !seek(original_pos, FILE_POINTER::BEGIN)) {
        set_last_error();
    }
    return success;
}

vector<file::chunk_info> file::chunks_info(size_type chunk_size) const {
    vector<chunk_info> info;
    if (!opened_ || handle_ == INVALID_HANDLE()) {
        set_last_error();
        return info;
    }
    if (chunk_size == 0) {
        chunk_size = FILE_BUFFER_SIZE * 16;
    }

    const size_type file_sz = size();
    if (file_sz == 0) return info;

    const size_type num_chunks = (file_sz + chunk_size - 1) / chunk_size;
    info.reserve(num_chunks);

    size_type offset = 0;
    size_type index = 0;

    while (offset < file_sz) {
        chunk_info ci{};
        ci.offset = offset;
        ci.chunk_index = index;

        const size_type remaining = file_sz - offset;
        ci.size = _MSTL min(remaining, chunk_size);
        info.push_back(ci);

        if (file_sz - offset < ci.size) break;
        offset += ci.size;
        ++index;
    }

    return info;
}

file::size_type file::read_binary(void* buffer, const size_type size) const {
    if (!opened_ || handle_ == INVALID_HANDLE() || !buffer) return 0;
    if (size == 0) return 0;

    auto ptr = static_cast<byte_t*>(buffer);
    size_type total_read = 0;
    size_type remaining = size;

    while (remaining > 0) {
        if (read_buffer_pos_ >= read_buffer_size_) {
            if (!fill_read_buffer() || read_buffer_size_ == 0) {
                break;
            }
        }

        const size_type available = read_buffer_size_ - read_buffer_pos_;
        const size_type to_read = _MSTL min(remaining, available);

        _MSTL memory_copy(ptr, read_buffer_.data() + read_buffer_pos_, to_read);
        read_buffer_pos_ += to_read;
        ptr += to_read;
        total_read += to_read;
        remaining -= to_read;
    }
    return total_read;
}

file::size_type file::read_binary(string& str, const size_type size) const {
    if (!opened_ || handle_ == INVALID_HANDLE()) return 0;
    if (size == 0) {
        str.clear();
        return 0;
    }
    str.resize(size);
    const size_type total_read = file::read_binary(str.data(), size);
    if (total_read < size) {
        str.resize(total_read);
    }
    return total_read;
}

file::size_type file::read_binary(string& str) const {
    const size_type s = size();
    str.resize(s);
    return this->read_binary(str, s);
}

string file::read_binary() const {
    if (!opened_ || handle_ == INVALID_HANDLE()) return {};

    const size_type sz = this->size();
    string content;
    content.resize(sz);

    if (sz > 0) {
        const size_type bytes_read = this->read_binary(content, sz);
        if (bytes_read != sz) content.resize(bytes_read);
    }
    return content;
}

bool file::read_line(string& line) const {
    if (!opened_ || handle_ == INVALID_HANDLE()) return false;

    line.clear();
    bool found_eol = false;

    while (!found_eol) {
        if (read_buffer_pos_ >= read_buffer_size_) {
            if (!fill_read_buffer() || read_buffer_size_ == 0) {
                break;
            }
        }

        while (read_buffer_pos_ < read_buffer_size_ && !found_eol) {
            const char ch = static_cast<char>(read_buffer_[read_buffer_pos_++]);

            if (ch == '\r') {
                if (read_buffer_pos_ < read_buffer_size_) {
                    if (static_cast<char>(read_buffer_[read_buffer_pos_]) == '\n') {
                        read_buffer_pos_++;
                    }
                }
                found_eol = true;
            } else if (ch == '\n') {
                found_eol = true;
            } else {
                line += ch;
            }
        }
    }

    return !line.empty() || found_eol;
}

string file::read_line() const {
    string line;
    if (!read_line(line)) return {};
    return line;
}

vector<string> file::read_lines() const {
    vector<string> lines;
    if (!opened_ || handle_ == INVALID_HANDLE()) return lines;

    const string content = this->read();
    if (content.empty()) return lines;

    size_t start = 0;
    size_t end = content.find('\n');

    while (end != string::npos) {
        string line = content.substr(start, end - start);
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.emplace_back(_MSTL move(line));
        start = end + 1;
        end = content.find('\n', start);
    }

    if (start < content.size()) {
        lines.emplace_back(content.view(start));
    }

    return lines;
}


file::async_result file::async_read(string& buffer,
    const size_type size, const difference_type offset) {
    async_result result;

    if (!opened_ || handle_ == INVALID_HANDLE()) {
        set_last_error();
        result.error_code = last_error_code_;
        return result;
    }
    if (size == 0) {
        buffer.clear();
        result.completed = true;
        return result;
    }

    if (buffer.capacity() < size) {
        try {
            buffer.reserve(size);
        } catch (...) {
            last_error_msg_ = "Not enough memory";
            return result;
        }
    }

    auto* context = new async_context(&buffer);

#ifdef MSTL_PLATFORM_WINDOWS__
    if (!context->cb->hEvent) {
        delete context;
        set_last_error();
        result.error_code = last_error_code_;
        return result;
    }

    if (offset >= 0) {
        const uint64_t offset_64 = static_cast<uint64_t>(offset);
        context->cb->Offset = static_cast<size_type>(offset_64 & numeric_limits<size_type>::max());
        context->cb->OffsetHigh = static_cast<size_type>(offset_64 >> 32);
    } else {
        const difference_type current_pos = tell();
        if (current_pos >= 0) {
            const uint64_t offset_64 = static_cast<uint64_t>(current_pos);
            context->cb->Offset = static_cast<size_type>(offset_64 & numeric_limits<size_type>::max());
            context->cb->OffsetHigh = static_cast<size_type>(offset_64 >> 32);
        }
    }

    if (buffer.size() < size) {
        buffer.resize(size);
    }

    size_type bytes_read = 0;
    const size_type read_size = _MSTL min<size_type>(size, numeric_limits<size_type>::max());

    if (::ReadFile(handle_, buffer.data(), read_size, &bytes_read, context->cb)) {
        result.completed = true;
        result.bytes_transferred = bytes_read;
        delete context;
    } else {
        const ::DWORD error = ::GetLastError();
        if (error == ERROR_IO_PENDING) {
            result.completed = false;
            result.cb = context->cb;
            result.user_context = context;

            lock_guard<mutex> lock(async_mutex_);
            async_operations_.push_back(context->cb);
            async_contexts_[context->cb] = context;
        } else {
            set_last_error();
            result.error_code = last_error_code_;
            delete context;
        }
    }
#elif defined(MSTL_PLATFORM_LINUX__)
    if (offset >= 0) {
        context->cb->aio_offset = offset;
    } else {
        const difference_type current_pos = tell();
        if (current_pos >= 0) {
            context->cb->aio_offset = current_pos;
        } else {
            context->cb->aio_offset = 0;
        }
    }

    if (buffer.size() < size) {
        buffer.resize(size);
    }

    context->cb->aio_fildes = handle_;
    context->cb->aio_buf = buffer.data();
    context->cb->aio_nbytes = size;
    context->cb->aio_sigevent.sigev_notify = SIGEV_NONE;

    if (::aio_read(context->cb) == 0) {
        result.completed = false;
        result.cb = context->cb;
        result.user_context = context;

        lock_guard<mutex> lock(async_mutex_);
        async_operations_.push_back(context->cb);
        async_contexts_[context->cb] = context;
    } else {
        set_last_error();
        result.error_code = last_error_code_;
        delete context;
    }
#endif

    return result;
}

file::async_result file::async_write(string data,
    const size_type size, const difference_type offset) {
    async_result result;

    if (!opened_ || handle_ == INVALID_HANDLE()) {
        set_last_error();
        result.error_code = last_error_code_;
        return result;
    }

    const size_type real_size = (size == numeric_limits<size_type>::max()) ?
        data.size() : _MSTL min(size, static_cast<size_type>(data.size()));

    auto* context = new async_context(_MSTL move(data));
#ifdef MSTL_PLATFORM_WINDOWS__
    if (!context->cb->hEvent) {
        delete context;
        set_last_error();
        result.error_code = last_error_code_;
        return result;
    }

    if (offset >= 0) {
        const uint64_t offset_64 = static_cast<uint64_t>(offset);
        context->cb->Offset = static_cast<size_type>(offset_64 & numeric_limits<size_type>::max());
        context->cb->OffsetHigh = static_cast<size_type>(offset_64 >> 32);
    } else {
        const difference_type current_pos = tell();
        if (current_pos >= 0) {
            const uint64_t offset_64 = static_cast<uint64_t>(current_pos);
            context->cb->Offset = static_cast<size_type>(offset_64 & numeric_limits<size_type>::max());
            context->cb->OffsetHigh = static_cast<size_type>(offset_64 >> 32);
        }
    }

    size_type bytes_written = 0;
    const size_type write_size = _MSTL min<size_type>(real_size, numeric_limits<size_type>::max());

    if (::WriteFile(handle_, data.data(), write_size, &bytes_written, context->cb)) {
        result.completed = true;
        result.bytes_transferred = bytes_written;
        delete context;
    } else {
        const ::DWORD error = ::GetLastError();
        if (error == ERROR_IO_PENDING) {
            result.completed = false;
            result.cb = context->cb;
            result.user_context = context;

            lock_guard<mutex> lock(async_mutex_);
            async_operations_.push_back(context->cb);
            async_contexts_[context->cb] = context;
        } else {
            set_last_error();
            result.error_code = last_error_code_;
            delete context;
        }
    }
#elif defined(MSTL_PLATFORM_LINUX__)
    if (offset >= 0) {
        context->cb->aio_offset = offset;
    } else {
        const difference_type current_pos = tell();
        if (current_pos >= 0) {
            context->cb->aio_offset = current_pos;
        } else {
            context->cb->aio_offset = 0;
        }
    }

    context->cb->aio_fildes = handle_;
    context->cb->aio_buf = const_cast<char*>(data.data());
    context->cb->aio_nbytes = real_size;
    context->cb->aio_sigevent.sigev_notify = SIGEV_NONE;

    if (::aio_write(context->cb) == 0) {
        result.completed = false;
        result.cb = context->cb;
        result.user_context = context;

        lock_guard<mutex> lock(async_mutex_);
        async_operations_.push_back(context->cb);
        async_contexts_[context->cb] = context;
    } else {
        set_last_error();
        result.error_code = last_error_code_;
        delete context;
    }
#endif

    return result;
}

bool file::wait_async(async_result& result, const uint32_t timeout_ms) {
    if (result.completed) return true;

#ifdef MSTL_PLATFORM_WINDOWS__
    if (!result.cb) {
        result.error_code = ERROR_INVALID_PARAMETER;
        return false;
    }

    size_type bytes_transferred = 0;

    if (timeout_ms == numeric_limits<uint32_t>::max()) {
        if (::GetOverlappedResult(handle_, result.cb, &bytes_transferred, TRUE)) {
            return complete_async_result(result, bytes_transferred);
        }
    } else {
        const ::HANDLE hEvent = result.cb->hEvent;
        if (hEvent) {
            const auto wait_result = ::WaitForSingleObject(hEvent, timeout_ms);
            if (wait_result == WAIT_OBJECT_0) {
                if (::GetOverlappedResult(handle_, result.cb, &bytes_transferred, FALSE)) {
                    return complete_async_result(result, bytes_transferred);
                }
            } else if (wait_result == WAIT_TIMEOUT) {
                result.error_code = WAIT_TIMEOUT;
                return false;
            } else if (wait_result == WAIT_FAILED) {
                set_last_error();
                result.error_code = last_error_code_;
                return false;
            }
        }
    }
#elif defined(MSTL_PLATFORM_LINUX__)
    if (!result.cb) {
        result.error_code = EINVAL;
        return false;
    }

    const ::aiocb* const aiocb_list[1] = { result.cb };

    if (timeout_ms == 0xFFFFFFFF) {
        if (::aio_suspend(aiocb_list, 1, nullptr) == 0) {
            return check_async_completion(result);
        }
    } else {
        ::timespec timeout{};
        timeout.tv_sec = timeout_ms / 1000;
        timeout.tv_nsec = (timeout_ms % 1000) * 1000000;

        if (::aio_suspend(aiocb_list, 1, &timeout) == 0) {
            return check_async_completion(result);
        } else {
            const int err = errno;
            if (err == EAGAIN || err == ETIMEDOUT) {
                result.error_code = ETIMEDOUT;
                return false;
            } else if (err == EINTR) {
                result.error_code = EINTR;
                return false;
            } else {
                set_last_error();
                result.error_code = last_error_code_;
                return false;
            }
        }
    }
#endif

    set_last_error();
    result.error_code = last_error_code_;
    return false;
}

void file::cancel_async(async_result& result) {
    if (result.completed) return;
    lock_guard<mutex> lock(async_mutex_);
    if (!result.cb) return;

#ifdef MSTL_PLATFORM_WINDOWS__
    if (!::CancelIoEx(handle_, result.cb)) {
        size_type bytes_transferred = 0;
        if (::GetOverlappedResult(handle_, result.cb, &bytes_transferred, FALSE)) {
            result.completed = true;
            result.bytes_transferred = bytes_transferred;
        } else {
            result.completed = true;
            result.error_code = ::GetLastError();
        }
    } else {
        result.completed = true;
        result.error_code = ERROR_OPERATION_ABORTED;
    }

    auto it = _MSTL find(async_operations_.begin(), async_operations_.end(), result.cb);
    if (it != async_operations_.end()) {
        async_operations_.erase(it);
    }

    const auto ctx_it = async_contexts_.find(result.cb);
    if (ctx_it != async_contexts_.end()) {
        delete ctx_it->second;
        async_contexts_.erase(ctx_it);
    }

    if (result.cb) {
        if (result.cb->hEvent) {
            ::CloseHandle(result.cb->hEvent);
        }
        delete result.cb;
        result.cb = nullptr;
    }
#elif defined(MSTL_PLATFORM_LINUX__)
    const int cancel_result = ::aio_cancel(handle_, result.cb);

    if (cancel_result == AIO_CANCELED) {
        result.completed = true;
        result.error_code = ECANCELED;
        result.bytes_transferred = 0;
    } else if (cancel_result == AIO_NOTCANCELED) {
        const ::aiocb* const aiocb_list[1] = { result.cb };
        ::aio_suspend(aiocb_list, 1, nullptr);

        const ssize_t ret = ::aio_return(result.cb);
        result.completed = true;
        result.bytes_transferred = (ret > 0) ? static_cast<size_type>(ret) : 0;
        result.error_code = (ret >= 0) ? 0 : errno;
    } else if (cancel_result == AIO_ALLDONE) {
        const ssize_t ret = ::aio_return(result.cb);
        result.completed = true;
        result.bytes_transferred = (ret > 0) ? static_cast<size_type>(ret) : 0;
        result.error_code = (ret >= 0) ? 0 : errno;
    } else {
        result.completed = true;
        result.error_code = errno;
    }

    const auto it = _MSTL find(async_operations_.begin(), async_operations_.end(), result.cb);
    if (it != async_operations_.end()) {
        async_operations_.erase(it);
    }

    const auto ctx_it = async_contexts_.find(result.cb);
    if (ctx_it != async_contexts_.end()) {
        delete ctx_it->second;
        async_contexts_.erase(ctx_it);
    }

    delete result.cb;
    result.cb = nullptr;

#endif

    result.user_context = nullptr;
}


file::size_type file::size() const {
    if (!opened_ || handle_ == INVALID_HANDLE()) {
        last_error_code_ = EBADF;
        last_error_msg_ = "File not opened";
        return 0;
    }

    if (write_buffer_pos_ > 0) {
        const difference_type current_pos = tell();
        if (current_pos < 0) {
            set_last_error();
            return 0;
        }
        if (!flush_write_buffer()) {
            set_last_error();
            return 0;
        }
        if (!seek(current_pos, FILE_POINTER::BEGIN)) {
            set_last_error();
            return 0;
        }
    }

#ifdef MSTL_PLATFORM_WINDOWS__
    ::LARGE_INTEGER file_size;
    if (!::GetFileSizeEx(handle_, &file_size)) {
        set_last_error();
        return 0;
    }

    if (file_size.QuadPart > static_cast<::LONGLONG>(numeric_limits<size_type>::max())) {
        last_error_code_ = ERROR_FILE_TOO_LARGE;
        last_error_msg_ = "File size exceeds maximum representable size";
        return 0;
    }

    return static_cast<size_type>(file_size.QuadPart);
#elif defined(MSTL_PLATFORM_LINUX__)
    struct ::stat64 st{};
    if (::fstat64(handle_, &st) == -1) {
        set_last_error();
        return 0;
    }

    if (static_cast<uint64_t>(st.st_size) > numeric_limits<size_type>::max()) {
        last_error_code_ = EFBIG;
        last_error_msg_ = "File size exceeds maximum representable size";
        return 0;
    }

    return static_cast<size_type>(st.st_size);
#endif
}

bool file::size(size_type& out_size) const {
    out_size = 0;

    if (!opened_ || handle_ == INVALID_HANDLE()) {
        last_error_code_ = EBADF;
        last_error_msg_ = "File not opened";
        return false;
    }

    out_size = size();
    return last_error_code_ == 0;
}

uint64_t file::size64() const {
    if (!opened_ || handle_ == INVALID_HANDLE()) {
        set_last_error();
        return 0;
    }

    if (write_buffer_pos_ > 0) {
        const difference_type current_pos = tell();
        if (!flush_write_buffer()) {
            return 0;
        }
        MSTL_IGNORE seek(current_pos, FILE_POINTER::BEGIN);
    }

#ifdef MSTL_PLATFORM_WINDOWS__
    ::LARGE_INTEGER file_size;
    if (!::GetFileSizeEx(handle_, &file_size)) {
        set_last_error();
        return 0;
    }
    return static_cast<uint64_t>(file_size.QuadPart);

#elif defined(MSTL_PLATFORM_LINUX__)
    struct ::stat64 st{};
    if (::fstat64(handle_, &st) == -1) {
        set_last_error();
        return 0;
    }
    return static_cast<uint64_t>(st.st_size);
#endif
}

file::size_type file::size(const _MSTL path& p) {
    size_type sz = 0;
    file::size(p, sz);
    return sz;
}

bool file::size(const _MSTL path& p, size_type& size) {
    file f;
    if (f.open(p, false, FILE_ACCESS::READ)) {
        size = f.size();
        return true;
    }
    return false;
}

bool file::create_and_write(const _MSTL path& p, const string& content, const bool append) {
    const _MSTL path parent = p.parent_path();
    if (!parent.empty() && !parent.exists()) {
        if (!parent.create_directories()) {
            return false;
        }
    }

#ifdef MSTL_PLATFORM_WINDOWS__
    file f;
    if (!f.open(p, append,
        append ? FILE_ACCESS::APPEND : FILE_ACCESS::WRITE,
        FILE_SHARED::NO_SHARE,
        FILE_CREATION::OPEN_FORCE,
        FILE_ATTRI::NORMAL)) {
        return false;
        }
    const size_type bytes_written = f.write(content, content.size());
    return bytes_written == content.size();

#elif defined(MSTL_PLATFORM_LINUX__)
    int flags = O_WRONLY | O_CREAT;
    if (append) {
        flags |= O_APPEND;
    } else {
        flags |= O_TRUNC;
    }

    const int fd = ::open(p.c_str(), flags, 0644);
    if (fd == -1) return false;

    const ssize_t written = ::write(fd, content.c_str(), content.size());
    ::close(fd);
    return written == static_cast<ssize_t>(content.size());
#endif
}

void file::clear_error() noexcept {
    last_error_msg_.clear();
    last_error_code_ = 0;
}

bool file::compare(const _MSTL path& file1, const _MSTL path& file2, const bool binary) {
    return binary ? compare_binary(file1, file2) : compare_text(file1, file2);
}

bool file::compare_binary(const _MSTL path& file1, const _MSTL path& file2) {
    const size_type size1 = file::size(file1);
    const size_type size2 = file::size(file2);
    if (size1 != size2) return false;
    if (size1 == 0) return true;

    file f1, f2;
    if (!f1.open(file1, false, FILE_ACCESS::READ, FILE_SHARED::SHARE_READ)) {
        return false;
    }
    if (!f2.open(file2, false, FILE_ACCESS::READ, FILE_SHARED::SHARE_READ)) {
        f1.close();
        return false;
    }

    constexpr size_type COMPARE_BUFFER_SIZE = 64 * 1024;
    string buffer1(COMPARE_BUFFER_SIZE);
    string buffer2(COMPARE_BUFFER_SIZE);

    size_type total_read = 0;
    bool result = true;

    while (total_read < size1) {
        const size_type remaining = size1 - total_read;
        const size_type to_read = _MSTL min(remaining, COMPARE_BUFFER_SIZE);

        const size_type bytes_read1 = f1.read_binary(buffer1, to_read);
        const size_type bytes_read2 = f2.read_binary(buffer2, to_read);

        if (bytes_read1 != bytes_read2 || bytes_read1 != to_read) {
            result = false;
            break;
        }
        if (_MSTL memory_compare(buffer1.data(), buffer2.data(), to_read) != 0) {
            result = false;
            break;
        }

        total_read += to_read;
    }

    return result;
}

bool file::compare_text(const _MSTL path& file1, const _MSTL path& file2,
    const bool ignore_case, const bool ignore_whitespace) {
    if (!ignore_case && !ignore_whitespace) {
        return compare_binary(file1, file2);
    }

    file f1, f2;
    if (!f1.open(file1, false, FILE_ACCESS::READ, FILE_SHARED::SHARE_READ)) {
        return false;
    }
    if (!f2.open(file2, false, FILE_ACCESS::READ, FILE_SHARED::SHARE_READ)) {
        f1.close();
        return false;
    }

    constexpr size_type BUFFER_SIZE = 64 * 1024;
    string buffer1, buffer2;
    buffer1.reserve(BUFFER_SIZE);
    buffer2.reserve(BUFFER_SIZE);
    bool eof1 = false, eof2 = false;

    auto normalize_string = [ignore_case, ignore_whitespace](string& str) {
        if (ignore_whitespace) {
            size_t start = 0;
            size_t end = str.length();

            while (start < end && _MSTL is_space(str[start])) {
                ++start;
            }
            while (end > start && _MSTL is_space(str[end - 1])) {
                --end;
            }

            if (start > 0 || end < str.length()) {
                str = str.substr(start, end - start);
            }
        }

        if (ignore_case) {
            str.lowercase();
        }
    };

    while (!eof1 && !eof2) {
        buffer1.clear();
        buffer2.clear();

        const size_type bytes1 = f1.read(buffer1, BUFFER_SIZE);
        const size_type bytes2 = f2.read(buffer2, BUFFER_SIZE);

        if (bytes1 == 0) eof1 = true;
        if (bytes2 == 0) eof2 = true;
        if (eof1 != eof2) return false;

        if (!eof1) {
            size_t pos1 = 0, pos2 = 0;

            while (pos1 < buffer1.length() && pos2 < buffer2.length()) {
                size_t line_end1 = buffer1.find('\n', pos1);
                size_t line_end2 = buffer2.find('\n', pos2);

                if (line_end1 == string::npos) line_end1 = buffer1.length();
                if (line_end2 == string::npos) line_end2 = buffer2.length();

                string line1 = buffer1.substr(pos1, line_end1 - pos1);
                string line2 = buffer2.substr(pos2, line_end2 - pos2);

                if (!line1.empty() && line1.back() == '\r') line1.pop_back();
                if (!line2.empty() && line2.back() == '\r') line2.pop_back();

                normalize_string(line1);
                normalize_string(line2);

                if (line1 != line2) return false;

                pos1 = line_end1 + 1;
                pos2 = line_end2 + 1;
            }
        }
    }

    return true;
}

vector<file::binary_diff_entry> file::binary_diff(
    const _MSTL path& file1, const _MSTL path& file2, const size_type max_diffs) {
    vector<binary_diff_entry> diffs;
    diffs.reserve(max_diffs);

    file f1, f2;
    if (!f1.open(file1, false, FILE_ACCESS::READ, FILE_SHARED::SHARE_READ)) {
        return diffs;
    }
    if (!f2.open(file2, false, FILE_ACCESS::READ, FILE_SHARED::SHARE_READ)) {
        f1.close();
        return diffs;
    }

    const size_type size1 = f1.size();
    const size_type size2 = f2.size();

    if (size1 != size2 && diffs.size() < max_diffs) {
        binary_diff_entry entry;
        entry.offset = static_cast<difference_type>(_MSTL min(size1, size2));
        entry.byte1 = 0;
        entry.byte2 = 0;
        entry.is_size_diff = true;
        entry.size_diff = static_cast<int64_t>(size1) - static_cast<int64_t>(size2);
        diffs.push_back(entry);
    }

    const size_type min_size = _MSTL min(size1, size2);
    if (min_size == 0) return diffs;

    constexpr size_type BLOCK_SIZE = 64 * 1024;
    string buffer1(BLOCK_SIZE);
    string buffer2(BLOCK_SIZE);
    difference_type offset = 0;

    while (offset < min_size && diffs.size() < max_diffs) {
        const size_type remaining = min_size - offset;
        const size_type to_read = _MSTL min(remaining, BLOCK_SIZE);
        const size_type bytes1 = f1.read_binary(buffer1, to_read);
        const size_type bytes2 = f2.read_binary(buffer2, to_read);

        if (bytes1 != to_read || bytes2 != to_read) {
            break;
        }
        if (_MSTL memory_compare(buffer1.data(), buffer2.data(), to_read) == 0) {
            offset += to_read;
            continue;
        }

        for (size_type i = 0; i < to_read && diffs.size() < max_diffs; ++i) {
            if (buffer1[i] != buffer2[i]) {
                binary_diff_entry entry;
                entry.offset = static_cast<difference_type>(offset + i);
                entry.byte1 = static_cast<byte_t>(buffer1[i]);
                entry.byte2 = static_cast<byte_t>(buffer2[i]);
                diffs.push_back(entry);
            }
        }
        offset += to_read;
    }
    return diffs;
}

bool file::seek(const difference_type distance, FILE_POINTER method) const noexcept {
    if (!opened_ || handle_ == INVALID_HANDLE()) return false;

    if (append_mode_) {
        if (method != FILE_POINTER::END || distance != 0) {
            last_error_code_ = EPERM;
            last_error_msg_ = "Cannot seek in append mode";
            return false;
        }
    }
    if (write_buffer_pos_ > 0) {
        if (!flush_write_buffer()) {
            set_last_error();
            return false;
        }
    }

    read_buffer_pos_ = 0;
    read_buffer_size_ = 0;

    if (mapped_ptr_) {
        last_error_code_ = EPERM;
        last_error_msg_ = "Cannot seek in mapped file";
        return false;
    }

#ifdef MSTL_PLATFORM_WINDOWS__
    ::LARGE_INTEGER li{};
    li.QuadPart = distance;

    ::LARGE_INTEGER new_pointer{};
    if (!::SetFilePointerEx(
        handle_, li, &new_pointer, static_cast<fud_t>(method))) {
        set_last_error();
        return false;
    }
#elif defined(MSTL_PLATFORM_LINUX__)
    const difference_type new_pos = ::lseek(
        handle_, distance, static_cast<fud_t>(method));
    if (new_pos == -1) {
        set_last_error();
        return false;
    }
#endif
    return true;
}

file::difference_type file::tell() const noexcept {
    if (!opened_ || handle_ == INVALID_HANDLE()) {
        last_error_code_ = EBADF;
        return -1;
    }
    difference_type system_pos;
#ifdef MSTL_PLATFORM_WINDOWS__
    constexpr ::LARGE_INTEGER li_zero{};
    ::LARGE_INTEGER current_pos;
    if (!::SetFilePointerEx(handle_, li_zero, &current_pos, FILE_CURRENT)) {
        set_last_error();
        return -1;
    }
    system_pos = current_pos.QuadPart;
#elif defined(MSTL_PLATFORM_LINUX__)
    const difference_type pos = ::lseek(handle_, 0, SEEK_CUR);
    if (pos == -1) {
        set_last_error();
        return -1;
    }
    system_pos = pos;
#endif
    difference_type adjusted_pos = system_pos;

    if (write_buffer_pos_ > 0) {
        adjusted_pos += static_cast<difference_type>(write_buffer_pos_);
    } else if (read_buffer_size_ > 0) {
        const size_type unread = read_buffer_size_ - read_buffer_pos_;
        adjusted_pos -= static_cast<difference_type>(unread);
    }

    return adjusted_pos;
}

file::difference_type file::system_tell() const noexcept {
    if (!opened_ || handle_ == INVALID_HANDLE()) return -1;

#ifdef MSTL_PLATFORM_WINDOWS__
    constexpr ::LARGE_INTEGER li_zero{};
    ::LARGE_INTEGER current_pos;
    if (!::SetFilePointerEx(handle_, li_zero, &current_pos, FILE_CURRENT)) {
        return -1;
    }
    return current_pos.QuadPart;
#elif defined(MSTL_PLATFORM_LINUX__)
    const difference_type pos = ::lseek(handle_, 0, SEEK_CUR);
    if (pos == -1) {
        set_last_error();
        return -1;
    }
    return pos;
#endif
}

bool file::prefetch(const size_type hint_size) const noexcept {
    if (!opened_ || handle_ == INVALID_HANDLE()) {
        last_error_code_ = EBADF;
        return false;
    }
    if (read_buffer_pos_ < read_buffer_size_) {
        return true;
    }

    size_type prefetch_size = buffer_size_;
    if (hint_size > 0) {
        if (hint_size > numeric_limits<size_type>::max() / 2) {
            prefetch_size = numeric_limits<size_type>::max();
        } else {
            prefetch_size = _MSTL min(hint_size * 2, buffer_size_);
        }
    }

#ifdef MSTL_PLATFORM_WINDOWS__
    const difference_type current_pos = tell();
    if (current_pos < 0) {
        return false;
    }
    ::LARGE_INTEGER file_size;
    if (!::GetFileSizeEx(handle_, &file_size)) {
        set_last_error();
        return false;
    }

    const ::ULARGE_INTEGER start_offset = {
        static_cast<size_type>(current_pos & 0xFFFFFFFF),
        static_cast<size_type>(current_pos >> 32)
    };
    const ::ULARGE_INTEGER end_offset = {
        static_cast<size_type>(file_size.QuadPart & 0xFFFFFFFF),
        static_cast<size_type>(file_size.QuadPart >> 32)
    };

    size_type region_size = prefetch_size;
    if (start_offset.QuadPart + region_size > end_offset.QuadPart) {
        region_size = static_cast<size_type>(end_offset.QuadPart - start_offset.QuadPart);
    }

    if (region_size == 0) {
        return true;
    }

    const ::HANDLE hMapping = ::CreateFileMapping(
        handle_,
        nullptr,
        PAGE_READONLY,
        0, 0,
        nullptr
    );

    if (!hMapping || hMapping == INVALID_HANDLE_VALUE) {
        set_last_error();
        return false;
    }

    void* pView = ::MapViewOfFile(
        hMapping,
        FILE_MAP_READ,
        start_offset.HighPart,
        start_offset.LowPart,
        region_size
    );

    if (pView) {
        ::WIN32_MEMORY_RANGE_ENTRY range{pView, region_size};
        const ::HMODULE hKernel32 = ::GetModuleHandleA("kernel32.dll");
        if (hKernel32) {
            typedef ::BOOL(WINAPI* PFN_PrefetchVirtualMemory)(
                ::HANDLE hProcess,
                ::ULONG_PTR NumberOfEntries,
                ::PWIN32_MEMORY_RANGE_ENTRY VirtualAddresses,
                ::ULONG Flags
            );

            static auto pfnPrefetchVirtualMemory =
                reinterpret_cast<PFN_PrefetchVirtualMemory>(
                    ::GetProcAddress(hKernel32, "PrefetchVirtualMemory")
                );
            if (pfnPrefetchVirtualMemory) {
                if (pfnPrefetchVirtualMemory(::GetCurrentProcess(), 1, &range, 0)) {
                    ::UnmapViewOfFile(pView);
                    ::CloseHandle(hMapping);
                    return true;
                }
            }
        }
    } else {
        set_last_error();
    }
    ::CloseHandle(hMapping);
    return pView != nullptr;
#elif defined(MSTL_PLATFORM_LINUX__)
    const difference_type current_pos = tell();
    if (current_pos >= 0) {
        const int advice_result = ::posix_fadvise(
            handle_,
            current_pos,
            static_cast<difference_type>(prefetch_size),
            POSIX_FADV_WILLNEED
        );

        if (advice_result != 0) {
            last_error_code_ = advice_result;
            last_error_msg_ = ::strerror(advice_result);
        }
    }
    return fill_read_buffer();
#endif
}

bool file::truncate(const difference_type size) const noexcept {
if (!opened_ || handle_ == INVALID_HANDLE()) {
        last_error_code_ = EBADF;
        return false;
    }

    if (size < 0) {
        last_error_code_ = EINVAL;
        last_error_msg_ = "Negative file size";
        return false;
    }

    if (append_mode_) {
        last_error_code_ = EPERM;
        last_error_msg_ = "Cannot truncate file in append mode";
        return false;
    }

    bool buffers_cleared = true;

    if (write_buffer_pos_ > 0) {
        const difference_type current_pos = tell();
        const difference_type buffer_end_pos =
            current_pos + static_cast<difference_type>(write_buffer_pos_);
        if (size >= current_pos && size < buffer_end_pos) {
            if (!flush_write_buffer()) {
                buffers_cleared = false;
            }
        } else if (size < current_pos) {
            write_buffer_pos_ = 0;
        }
    }

    read_buffer_pos_ = 0;
    read_buffer_size_ = 0;

    if (mapped_ptr_) {
        last_error_code_ = EPERM;
        last_error_msg_ = "Cannot truncate memory-mapped file";
        return false;
    }
    if (!buffers_cleared) {
        return false;
    }

#ifdef MSTL_PLATFORM_WINDOWS__
    const difference_type current_pos = tell();
    if (current_pos < 0) return false;

    if (!seek(size, FILE_POINTER::BEGIN)) {
        return false;
    }

    if (!::SetEndOfFile(handle_)) {
        set_last_error();
        MSTL_IGNORE seek(current_pos, FILE_POINTER::BEGIN);
        return false;
    }

    if (size < current_pos) {
        if (!seek(size, FILE_POINTER::BEGIN)) {
            set_last_error();
        }
    } else {
        MSTL_IGNORE seek(current_pos, FILE_POINTER::BEGIN);
    }

    return true;

#elif defined(MSTL_PLATFORM_LINUX__)
    const difference_type current_pos = tell();
    if (current_pos < 0) {
        return false;
    }
    if (::ftruncate(handle_, size) != 0) {
        set_last_error();
        return false;
    }

    if (size < current_pos) {
        if (!seek(size, FILE_POINTER::BEGIN)) {
            set_last_error();
        }
    }
    return true;
#endif
}

bool file::lock(const difference_type offset,
    const difference_type length, FILE_LOCK mode) const noexcept {
    if (!opened_ || handle_ == INVALID_HANDLE()) {
        last_error_code_ = EBADF;
        return false;
    }

    if (offset < 0) {
        last_error_code_ = EINVAL;
        last_error_msg_ = "Negative offset";
        return false;
    }
    if (length < 0) {
        last_error_code_ = EINVAL;
        last_error_msg_ = "Negative length";
        return false;
    }

#ifdef MSTL_PLATFORM_WINDOWS__
    if (!flush_write_buffer()) {
        set_last_error();
        return false;
    }

    ::OVERLAPPED ov = {};
    const ::ULARGE_INTEGER offset_ul = {
        static_cast<::DWORD>(offset & 0xFFFFFFFF),
        static_cast<::DWORD>(offset >> 32)
    };
    ov.Offset = offset_ul.LowPart;
    ov.OffsetHigh = offset_ul.HighPart;

    fud_t flags = 0;
    if (mode == FILE_LOCK::EXCLUSIVE ||
        (static_cast<fud_t>(mode) & LOCKFILE_EXCLUSIVE_LOCK) != 0) {
        flags |= LOCKFILE_EXCLUSIVE_LOCK;
    }

    if ((static_cast<fud_t>(mode) & LOCKFILE_FAIL_IMMEDIATELY) != 0) {
        flags |= LOCKFILE_FAIL_IMMEDIATELY;
    }

    ::DWORD length_low;
    ::DWORD length_high;

    if (length == 0) {
        length_low = 0;
        length_high = 0;
    } else {
        const ::ULARGE_INTEGER length_ul = {
            static_cast<::DWORD>(length & 0xFFFFFFFF),
            static_cast<::DWORD>(length >> 32)
        };
        length_low = length_ul.LowPart;
        length_high = length_ul.HighPart;
    }

    if (!::LockFileEx(handle_, flags, 0, length_low, length_high, &ov)) {
        set_last_error();
        return false;
    }
    return true;

#elif defined(MSTL_PLATFORM_LINUX__)
    struct ::flock fl{};
    _MSTL memory_zero(&fl, sizeof(struct ::flock));

    if (mode == FILE_LOCK::EXCLUSIVE ||
        (static_cast<fud_t>(mode) & LOCK_EX) != 0) {
        fl.l_type = F_WRLCK;
    } else {
        fl.l_type = F_RDLCK;
    }
    fl.l_whence = SEEK_SET;
    fl.l_start = offset;
    fl.l_len = length;

    fud_t cmd = F_SETLKW;
    if ((static_cast<fud_t>(mode) & LOCK_NB) != 0) {
        cmd = F_SETLK;
    }

    if (::fcntl(handle_, cmd, &fl) == -1) {
        set_last_error();
        return false;
    }
    return true;

#endif
}

bool file::unlock(const difference_type offset, const difference_type length) const noexcept {
    if (!opened_ || handle_ == INVALID_HANDLE()) {
        last_error_code_ = EBADF;
        return false;
    }

    if (offset < 0) {
        last_error_code_ = EINVAL;
        return false;
    }
    if (length < 0) {
        last_error_code_ = EINVAL;
        return false;
    }

#ifdef MSTL_PLATFORM_WINDOWS__
    ::OVERLAPPED ov{};
    const ::ULARGE_INTEGER offset_ul = {
        static_cast<::DWORD>(offset & 0xFFFFFFFF),
        static_cast<::DWORD>(offset >> 32)
    };
    ov.Offset = offset_ul.LowPart;
    ov.OffsetHigh = offset_ul.HighPart;

    ::DWORD length_low;
    ::DWORD length_high;

    if (length == 0) {
        length_low = 0;
        length_high = 0;
    } else {
        const ::ULARGE_INTEGER length_ul = {
            static_cast<::DWORD>(length & 0xFFFFFFFF),
            static_cast<::DWORD>(length >> 32)
        };
        length_low = length_ul.LowPart;
        length_high = length_ul.HighPart;
    }

    if (!::UnlockFileEx(handle_, 0, length_low, length_high, &ov)) {
        set_last_error();
        return false;
    }

#elif defined(MSTL_PLATFORM_LINUX__)
    struct ::flock fl{};
    _MSTL memory_zero(&fl, sizeof(struct ::flock));

    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = offset;
    fl.l_len = length;

    if (::fcntl(handle_, F_SETLK, &fl) == -1) {
        set_last_error();
        return false;
    }

#endif
    return true;
}

bool file::try_lock(const difference_type offset,
    const difference_type length, FILE_LOCK mode) const noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    auto nonblocking_mode = static_cast<FILE_LOCK>(
        static_cast<fud_t>(mode) | LOCKFILE_FAIL_IMMEDIATELY);
#elif defined(MSTL_PLATFORM_LINUX__)
    const auto nonblocking_mode = static_cast<FILE_LOCK>(static_cast<fud_t>(mode) | LOCK_NB);
#endif
    return lock(offset, length, nonblocking_mode);
}

bool file::is_locked(const difference_type offset,
    const difference_type length, FILE_LOCK* out_type) const noexcept {

    if (!opened_ || handle_ == INVALID_HANDLE()) {
        return false;
    }

#ifdef MSTL_PLATFORM_WINDOWS__
    const bool can_lock = try_lock(offset, length, FILE_LOCK::SHARED);
    if (can_lock) {
        MSTL_IGNORE unlock(offset, length);
        if (out_type) *out_type = FILE_LOCK::SHARED;
        return false;
    }

    if (last_error_code_ == ERROR_LOCK_VIOLATION) {
        if (out_type) *out_type = FILE_LOCK::EXCLUSIVE;
        return true;
    }

    return false;

#elif defined(MSTL_PLATFORM_LINUX__)
    struct ::flock fl;
    _MSTL memory_zero(&fl, sizeof(struct ::flock));

    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = offset;
    fl.l_len = length;

    if (::fcntl(handle_, F_GETLK, &fl) == -1) {
        set_last_error();
        return false;
    }

    if (fl.l_type == F_UNLCK) {
        if (out_type) *out_type = static_cast<FILE_LOCK>(0);
        return false;
    } else if (fl.l_type == F_RDLCK) {
        if (out_type) *out_type = FILE_LOCK::SHARED;
        return true;
    } else if (fl.l_type == F_WRLCK) {
        if (out_type) *out_type = FILE_LOCK::EXCLUSIVE;
        return true;
    }

    return false;
#endif
}

bool file::lock_whole(const FILE_LOCK mode) const noexcept {
    return lock(0, 0, mode);
}

bool file::unlock_whole() const noexcept {
    return unlock(0, 0);
}

bool file::map(size_type offset, size_type size,
    const FILE_ACCESS access, const FILE_MAP_HINT hint) {
    lock_guard<mutex> lock(map_mutex_);

    if (mapped_ptr_) {
        unmap();
    }

    if (!opened_ || handle_ == INVALID_HANDLE()) {
        last_error_code_ = EBADF;
        last_error_msg_ = "File not opened";
        return false;
    }
    const size_type file_size = this->size();
    if (offset > file_size) {
        last_error_code_ = EINVAL;
        last_error_msg_ = "Offset exceeds file size";
        return false;
    }

    if (size == 0) {
        size = file_size - offset;
        if (size == 0) {
            mapped_ptr_ = nullptr;
            mapped_size_ = 0;
            return true;
        }
    } else if (offset + size > file_size) {
        if (static_cast<fud_t>(access & FILE_ACCESS::WRITE) == 0) {
            last_error_code_ = EINVAL;
            last_error_msg_ = "Mapping extends beyond file size";
            return false;
        }
    }

    if (!flush_write_buffer()) {
        set_last_error();
        return false;
    }

    read_buffer_pos_ = 0;
    read_buffer_size_ = 0;

#ifdef MSTL_PLATFORM_WINDOWS__

    const uint32_t allocation_granularity =
        sysinfo::instance().get_system_info().allocation_granularity;;

    const uint64_t aligned_offset = offset & ~(allocation_granularity - 1);
    const uint64_t offset_delta = offset - aligned_offset;
    const uint64_t aligned_size = size + offset_delta;

    fud_t protect;
    fud_t map_access;

    if (static_cast<fud_t>(access & FILE_ACCESS::WRITE)) {
        protect = PAGE_READWRITE;
        map_access = FILE_MAP_WRITE | FILE_MAP_READ;

        if (static_cast<fud_t>(access) &
            (static_cast<fud_t>(FILE_ACCESS::APPEND) & ~static_cast<fud_t>(FILE_ACCESS::WRITE))) {
            protect = PAGE_READWRITE;
            map_access = FILE_MAP_WRITE | FILE_MAP_READ;
        }
    } else if (static_cast<fud_t>(access & FILE_ACCESS::READ)) {
        protect = PAGE_READONLY;
        map_access = FILE_MAP_READ;
    } else {
        last_error_code_ = EINVAL;
        last_error_msg_ = "Invalid access mode";
        return false;
    }

    mapping_handle_ = ::CreateFileMappingW(
        handle_,
        nullptr,
        protect,
        static_cast<::DWORD>(aligned_size >> 32),
        static_cast<::DWORD>(aligned_size & 0xFFFFFFFF),
        nullptr
    );

    if (!mapping_handle_ || mapping_handle_ == INVALID_HANDLE_VALUE) {
        set_last_error();
        mapping_handle_ = INVALID_HANDLE_VALUE;
        return false;
    }

    const uint64_t offset_high = aligned_offset >> 32;
    const uint64_t offset_low = aligned_offset & 0xFFFFFFFF;

    mapped_ptr_ = ::MapViewOfFile(
        mapping_handle_, map_access,
        static_cast<::DWORD>(offset_high),
        static_cast<::DWORD>(offset_low),
        aligned_size
    );

    if (!mapped_ptr_) {
        set_last_error();
        ::CloseHandle(mapping_handle_);
        mapping_handle_ = INVALID_HANDLE_VALUE;
        return false;
    }

    mapped_ptr_ = static_cast<char*>(mapped_ptr_) + offset_delta;
    ::WIN32_MEMORY_RANGE_ENTRY range = { mapped_ptr_, size };

    const ::HMODULE hKernel32 = ::GetModuleHandleA("kernel32.dll");
    if (hKernel32) {
        typedef ::BOOL(WINAPI* PFN_PrefetchVirtualMemory)(
            ::HANDLE hProcess,
            ::ULONG_PTR NumberOfEntries,
            ::PWIN32_MEMORY_RANGE_ENTRY VirtualAddresses,
            ::ULONG Flags
        );

        static auto pfnPrefetchVirtualMemory =
            reinterpret_cast<PFN_PrefetchVirtualMemory>(
                ::GetProcAddress(hKernel32, "PrefetchVirtualMemory")
            );

        if (pfnPrefetchVirtualMemory) {
            ::ULONG flags = 0;
            switch (hint) {
                case FILE_MAP_HINT::SEQUENTIAL: {
                    flags = 0;
                    break;
                }
                case FILE_MAP_HINT::RANDOM: {
                    // Windows没有直接的随机访问提示
                    break;
                }
                default: break;
            }
            if (flags != 0) {
                pfnPrefetchVirtualMemory(::GetCurrentProcess(), 1, &range, flags);
            }
        }
    }

#elif defined(MSTL_PLATFORM_LINUX__)

    const difference_type page_size = ::sysconf(_SC_PAGESIZE);
    if (page_size < 0) {
        last_error_code_ = errno;
        last_error_msg_ = "Failed to get page size";
        return false;
    }

    const difference_type page_mask = page_size - 1;
    const difference_type aligned_offset = offset & ~page_mask;
    const difference_type offset_delta = offset - aligned_offset;
    const size_type aligned_size = size + offset_delta;

    int prot = PROT_READ;
    const auto access_flags = static_cast<fud_t>(access);
    if (access_flags & O_RDWR) {
        prot = PROT_READ | PROT_WRITE;
    } else if (access_flags & O_RDONLY) {
        prot = PROT_READ;
    } else if (access_flags & O_WRONLY) {
        prot = PROT_WRITE;
    } else {
        last_error_code_ = EINVAL;
        last_error_msg_ = "Invalid access mode";
        return false;
    }

    void* base_ptr = ::mmap(
        nullptr,
        aligned_size,
        prot,
        MAP_SHARED,
        handle_,
        aligned_offset
    );

    if (base_ptr == MAP_FAILED) {
        set_last_error();
        return false;
    }

    mapped_ptr_ = static_cast<char*>(base_ptr) + offset_delta;

    int advice = MADV_NORMAL;
    switch (hint) {
        case FILE_MAP_HINT::SEQUENTIAL: {
            advice = MADV_SEQUENTIAL;
            break;
        }
        case FILE_MAP_HINT::RANDOM: {
            advice = MADV_RANDOM;
            break;
        }
        case FILE_MAP_HINT::NORMAL: default: {
            advice = MADV_NORMAL;
            break;
        }
    }

    if (::madvise(base_ptr, aligned_size, advice) != 0) {
        // 建议失败不是致命错误
        set_last_error();
    }
    if (size > static_cast<size_type>(10 * 1024 * 1024)) {
        if (::mlock(base_ptr, aligned_size) != 0) {
            // 锁定失败不是致命错误
            set_last_error();
        }
    }

#endif

    mapped_offset_ = offset;
    mapped_access_ = access;
    mapped_size_ = size;
    is_mapped_ = true;
    return true;
}

void file::unmap() noexcept {
    lock_guard<mutex> lock(map_mutex_);

    if (!mapped_ptr_) return;

#ifdef MSTL_PLATFORM_WINDOWS__
    const uint32_t allocation_granularity =
        sysinfo::instance().get_system_info().allocation_granularity;

    const uintptr_t base_address = reinterpret_cast<uintptr_t>(mapped_ptr_) -
        (mapped_offset_ % allocation_granularity);

    if (!::UnmapViewOfFile(reinterpret_cast<::LPVOID>(base_address))) {
        set_last_error();
    }
    if (mapping_handle_ != INVALID_HANDLE_VALUE) {
        ::CloseHandle(mapping_handle_);
        mapping_handle_ = INVALID_HANDLE_VALUE;
    }

#elif defined(MSTL_PLATFORM_LINUX__)

    const long page_size = ::sysconf(_SC_PAGESIZE);
    if (page_size > 0) {
        const difference_type page_mask = page_size - 1;
        const uintptr_t base_address = reinterpret_cast<uintptr_t>(mapped_ptr_) -
            (mapped_offset_ & page_mask);
        const size_type total_size = mapped_size_ + (mapped_offset_ & page_mask);

        if (mapped_size_ > static_cast<size_type>(10 * 1024 * 1024)) {
            ::munlock(reinterpret_cast<void*>(base_address), total_size);
        }
        if (::munmap(reinterpret_cast<void*>(base_address), total_size) != 0) {
            set_last_error();
        }
    }

#endif

    mapped_ptr_ = nullptr;
    mapped_size_ = 0;
    mapped_offset_ = 0;
    is_mapped_ = false;
}

bool file::remap(const size_type new_offset, const size_type new_size) {
    lock_guard<mutex> lock(map_mutex_);

    if (!mapped_ptr_) {
        return map(new_offset, new_size, mapped_access_);
    }
    const FILE_ACCESS current_access = mapped_access_;
    unmap();
    return map(new_offset, new_size, current_access);
}

bool file::flush_mapped(const bool async) const {
    lock_guard<mutex> lock(map_mutex_);

    if (!mapped_ptr_) {
        last_error_code_ = EINVAL;
        last_error_msg_ = "No memory mapping";
        return false;
    }
    if ((static_cast<fud_t>(mapped_access_ & FILE_ACCESS::WRITE)) == 0) {
        return true;
    }

#ifdef MSTL_PLATFORM_WINDOWS__

    if (!::FlushViewOfFile(mapped_ptr_, mapped_size_)) {
        set_last_error();
        return false;
    }

    if (!async && mapping_handle_ != INVALID_HANDLE_VALUE) {
        if (!::FlushFileBuffers(handle_)) {
            set_last_error();
            return false;
        }
    }
    return true;

#elif defined(MSTL_PLATFORM_LINUX__)
    int flags = MS_SYNC;
    if (async) {
        flags = MS_ASYNC;
    }

    const difference_type page_size = ::sysconf(_SC_PAGESIZE);
    if (page_size <= 0) {
        return false;
    }

    const difference_type page_mask = page_size - 1;
    const uintptr_t base_address = reinterpret_cast<uintptr_t>(mapped_ptr_) -
        (mapped_offset_ & page_mask);
    const size_type total_size = mapped_size_ + (mapped_offset_ & page_mask);

    if (::msync(reinterpret_cast<void*>(base_address), total_size, flags) != 0) {
        set_last_error();
        return false;
    }

    return true;
#endif
}

bool file::lock_mapped_pages(const bool lock_in_memory) const noexcept {
    if (!mapped_ptr_) {
        return false;
    }

#ifdef MSTL_PLATFORM_WINDOWS__

    ::SIZE_T min_working_set, max_working_set;
    if (!::GetProcessWorkingSetSize(::GetCurrentProcess(), &min_working_set, &max_working_set)) {
        return false;
    }

    if (lock_in_memory) {
        if (!::VirtualLock(mapped_ptr_, mapped_size_)) {
            return false;
        }
    } else {
        ::VirtualUnlock(mapped_ptr_, mapped_size_);
    }
    return true;

#elif defined(MSTL_PLATFORM_LINUX__)
    const difference_type page_size = ::sysconf(_SC_PAGESIZE);
    if (page_size <= 0) {
        return false;
    }

    const difference_type page_mask = page_size - 1;
    const uintptr_t base_address = reinterpret_cast<uintptr_t>(mapped_ptr_) -
        (mapped_offset_ & page_mask);
    const size_type total_size = mapped_size_ + (mapped_offset_ & page_mask);
    if (lock_in_memory) {
        return ::mlock(reinterpret_cast<void *>(base_address), total_size) == 0;
    }
    return ::munlock(reinterpret_cast<void*>(base_address), total_size) == 0;
#endif
}

file::map_info file::map_infos() const noexcept {
    lock_guard<mutex> lock(map_mutex_);

    map_info info;
    info.address = mapped_ptr_;
    info.size = mapped_size_;
    info.offset = mapped_offset_;
    info.access = mapped_access_;
    info.is_mapped = is_mapped_;

    return info;
}

FILE_ATTRI file::attributes() const noexcept {
    if (!opened_ || handle_ == INVALID_HANDLE()) return FILE_ATTRI::OTHERS;

#ifdef MSTL_PLATFORM_WINDOWS__
    ::BY_HANDLE_FILE_INFORMATION info;
    if (!::GetFileInformationByHandle(handle_, &info)) return FILE_ATTRI::OTHERS;
    return static_cast<FILE_ATTRI>(info.dwFileAttributes);
#elif defined(MSTL_PLATFORM_LINUX__)
    struct ::stat64 st{};
    if (::fstat64(handle_, &st) == -1) return FILE_ATTRI::OTHERS;
    return static_cast<FILE_ATTRI>(st.st_mode);
#endif
}

bool file::set_attributes(FILE_ATTRI attr) const noexcept {
    if (!opened_ || handle_ == INVALID_HANDLE()) return false;

#ifdef MSTL_PLATFORM_WINDOWS__
    return ::SetFileAttributesA(path_.c_str(), static_cast<fud_t>(attr)) != 0;
#elif defined(MSTL_PLATFORM_LINUX__)
    struct ::stat64 st_old{};
    if (::fstat64(handle_, &st_old) == -1) return false;

    const ::mode_t current_mode = st_old.st_mode;
    constexpr ::mode_t perm_mask = S_IRWXU | S_IRWXG | S_IRWXO;
    const ::mode_t new_perm = static_cast<::mode_t>(attr) & perm_mask;
    const ::mode_t new_mode = (current_mode & ~perm_mask) | new_perm;
    return ::fchmod(handle_, new_mode) == 0;
#endif
}

#ifdef MSTL_PLATFORM_WINDOWS__
datetime file::creation_time() const noexcept {
    if (!opened_ || handle_ == INVALID_HANDLE()) return datetime::epoch();

    time_type ft_create, ft_access, ft_write;
    if (!::GetFileTime(handle_, &ft_create, &ft_access, &ft_write))
        return datetime::epoch();
    return filetime_to_datetime(ft_create);
}
#endif

datetime file::last_access_time() const noexcept {
    if (!opened_ || handle_ == INVALID_HANDLE()) return datetime::epoch();

#ifdef MSTL_PLATFORM_WINDOWS__
    time_type ft_create, ft_access, ft_write;
    if (!::GetFileTime(handle_, &ft_create, &ft_access, &ft_write))
        return datetime::epoch();
    return filetime_to_datetime(ft_access);
#elif defined(MSTL_PLATFORM_LINUX__)
    struct ::stat64 st{};
    if (::fstat64(handle_, &st) == -1) {
        const ::time_t now = ::time(nullptr);
        return filetime_to_datetime(now);
    }
    return filetime_to_datetime(st.st_atime);
#endif
}

datetime file::last_write_time() const noexcept {
    if (!opened_ || handle_ == INVALID_HANDLE()) return datetime::epoch();

#ifdef MSTL_PLATFORM_WINDOWS__
    time_type ft_create, ft_access, ft_write;
    if (!::GetFileTime(handle_, &ft_create, &ft_access, &ft_write))
        return datetime::epoch();
    return filetime_to_datetime(ft_write);
#elif defined(MSTL_PLATFORM_LINUX__)
    struct ::stat64 st{};
    if (::fstat64(handle_, &st) == -1) {
        const ::time_t now = ::time(nullptr);
        return filetime_to_datetime(now);
    }
    return filetime_to_datetime(st.st_mtime);
#endif
}

#ifdef MSTL_PLATFORM_WINDOWS__
bool file::set_all_times(const datetime& create,
    const datetime& access, const datetime& write) const noexcept {
    if (!opened_ || handle_ == INVALID_HANDLE()) return false;

    const time_type ft_create = datetime_to_filetime(create);
    const time_type ft_access = datetime_to_filetime(access);
    const time_type ft_write = datetime_to_filetime(write);
    return ::SetFileTime(handle_, &ft_create, &ft_access, &ft_write) != 0;
}
#elif defined(MSTL_PLATFORM_LINUX__)
bool file::set_all_times(const datetime& access, const datetime& write) const noexcept {
    if (!opened_ || handle_ == INVALID_HANDLE()) return false;

    ::timeval times[2];
    times[0].tv_sec = timestamp(access).seconds();
    times[0].tv_usec = 0;
    times[1].tv_sec = timestamp(write).seconds();
    times[1].tv_usec = 0;
    return ::futimes(handle_, times) == 0;
}
#endif

#ifdef MSTL_PLATFORM_WINDOWS__
bool file::set_creation_time(const datetime& dt) const noexcept {
    const time_type ft_create = datetime_to_filetime(dt);
    time_type ft_access, ft_write;

    if (!::GetFileTime(handle_, nullptr, &ft_access, &ft_write)) return false;
    return ::SetFileTime(handle_, &ft_create, &ft_access, &ft_write) != 0;
}
#endif

bool file::set_last_access_time(const datetime& dt) const noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    const time_type ft_access = datetime_to_filetime(dt);
    time_type ft_create, ft_write;

    if (!::GetFileTime(handle_, &ft_create, nullptr, &ft_write)) return false;
    return ::SetFileTime(handle_, &ft_create, &ft_access, &ft_write) != 0;
#elif defined(MSTL_PLATFORM_LINUX__)
    return set_all_times(dt, last_write_time());
#endif
}

bool file::set_last_write_time(const datetime& dt) const noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    const time_type ft_write = datetime_to_filetime(dt);
    time_type ft_create, ft_access;

    if (!::GetFileTime(handle_, &ft_create, &ft_access, nullptr)) return false;
    return ::SetFileTime(handle_, &ft_create, &ft_access, &ft_write) != 0;
#elif defined(MSTL_PLATFORM_LINUX__)
    return set_all_times(last_access_time(), dt.to_UTC());
#endif
}

bool file::read(const _MSTL path& p, string& content,
    FILE_CREATION creation, FILE_ATTRI attributes) {
#ifdef MSTL_PLATFORM_WINDOWS__
    file f;
    if (!f.open(p, false,
        FILE_ACCESS::READ,
        FILE_SHARED::SHARE_READ,
        creation, attributes))
        return false;

    const size_type sz = f.size();
    if (sz == 0) {
        content.clear();
        return true;
    }

    content.resize(sz);
    const size_type bytes_read = f.read(content, sz);
    return bytes_read == sz;
#elif defined(MSTL_PLATFORM_LINUX__)
    const int fd = ::open(p.c_str(), O_RDONLY);
    if (fd == -1) return false;

    struct ::stat64 st{};
    if (::fstat64(fd, &st) == -1) {
        ::close(fd);
        return false;
    }

    content.resize(st.st_size);
    const ssize_t read_bytes = ::read(fd, content.data(), st.st_size);
    ::close(fd);
    return read_bytes == st.st_size;
#endif
}

string file::read(const _MSTL path& p, FILE_CREATION creation, FILE_ATTRI attributes) {
    string content;
    file::read(p, content, creation, attributes);
    return content;
}

bool file::read_binary(const _MSTL path& p, string& content,
    const FILE_CREATION creation, const FILE_ATTRI attributes) {
    file f;
    if (!f.open(p, false,
        FILE_ACCESS::READ,
        FILE_SHARED::SHARE_READ_WRITE,
        creation, attributes)) {
        return false;
    }

    const size_type sz = f.size();
    content.resize(sz);
    if (sz > 0) {
        const size_type bytes_read = f.read_binary(content, sz);
        if (bytes_read != sz) content.resize(bytes_read);
    }
    return true;
}

string file::read_binary(const _MSTL path& p,
    const FILE_CREATION creation, const FILE_ATTRI attributes) {
    string content;
    file::read_binary(p, content, creation, attributes);
    return content;
}

file_lock_guard::file_lock_guard(
    const file& f, const difference_type offset,
    const difference_type length, const FILE_LOCK mode)
: file_(f), offset_(offset), length_(length), locked_(false) {
    locked_ = file_.lock(offset, length, mode);
}

file_lock_guard::~file_lock_guard() {
    if (locked_) {
        MSTL_IGNORE file_.unlock(offset_, length_);
    }
}

bool file_lock_guard::unlock() {
    if (locked_) {
        if (file_.unlock(offset_, length_)) {
            locked_ = false;
            return true;
        }
    }
    return false;
}

MSTL_END_NAMESPACE__
