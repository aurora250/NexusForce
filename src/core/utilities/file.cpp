#include <MSTL/core/system/file.hpp>
#ifdef MSTL_PLATFORM_LINUX__
#include <sys/file.h>
#include <sys/time.h>
#include <ctime>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <dirent.h>
#include <cstdio>
#endif
MSTL_BEGIN_NAMESPACE__

bool file::flush_write_buffer() const noexcept {
    if (write_buffer_pos_ == 0) return true;
#ifdef MSTL_PLATFORM_WINDOWS__
    size_type bytes_written;
    const ::BOOL success = ::WriteFile(handle_, write_buffer_.data(),
        write_buffer_pos_, &bytes_written, nullptr
    );
    if (!success || bytes_written != write_buffer_pos_) {
        return false;
    }
#elif defined(MSTL_PLATFORM_LINUX__)
    const ssize_t bytes_written = ::write(handle_, write_buffer_.data(), write_buffer_pos_);
    if (bytes_written != static_cast<ssize_t>(write_buffer_pos_)) return false;
#endif
    write_buffer_pos_ = 0;
    return true;
}

bool file::fill_read_buffer() const noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    size_type bytes_read;
    const ::BOOL success = ::ReadFile(
        handle_, read_buffer_.data(),
        FILE_BUFFER_SIZE,
        &bytes_read, nullptr
    );
    if (!success) {
        read_buffer_size_ = 0;
        return false;
    }
    read_buffer_size_ = bytes_read;
#elif defined(MSTL_PLATFORM_LINUX__)
    const ssize_t bytes_read = ::read(handle_, read_buffer_.data(), FILE_BUFFER_SIZE);
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
        tm_local.tm_hour, tm_local.tm_min, tm_local.tm_sec
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
    const size_t size = ::FormatMessageA(
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

#ifdef MSTL_PLATFORM_LINUX__
::mode_t file::convert_attributes(const FILE_ATTRI attr) {
    ::mode_t mode = 0;

    if ((attr & FILE_ATTRI::DIRECTORY) != FILE_ATTRI::OTHERS) {
        mode |= S_IFDIR;
    } else if ((attr & FILE_ATTRI::DEVICE) != FILE_ATTRI::OTHERS) {
        mode |= S_IFBLK | S_IFCHR;
    } else if ((attr & FILE_ATTRI::REPARSE_POINT) != FILE_ATTRI::OTHERS) {
        mode |= S_IFLNK;
    } else {
        mode |= S_IFREG;
    }

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
      write_buffer_pos_(other.write_buffer_pos_) {

    other.handle_ = INVALID_HANDLE();
    other.opened_ = false;
    other.append_mode_ = false;
    other.path_ = string{};
    other.read_buffer_.clear();
    other.read_buffer_pos_ = 0;
    other.read_buffer_size_ = 0;
    other.write_buffer_.clear();
    other.write_buffer_pos_ = 0;
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
    other.path_ = string{};
    other.read_buffer_.clear();
    other.read_buffer_pos_ = 0;
    other.read_buffer_size_ = 0;
    other.write_buffer_.clear();
    other.write_buffer_pos_ = 0;

    return *this;
}

file::~file() {
    this->close();

    read_buffer_.clear();
    write_buffer_.clear();
    read_buffer_pos_ = 0;
    read_buffer_size_ = 0;
    write_buffer_pos_ = 0;
}

bool file::open(string path, const bool append,
    FILE_ACCESS access, FILE_SHARED share_mode,
    FILE_CREATION creation, FILE_ATTRI attributes) {
    this->close();

    read_buffer_.resize(FILE_BUFFER_SIZE);
    write_buffer_.resize(FILE_BUFFER_SIZE);
    read_buffer_pos_ = 0;
    read_buffer_size_ = 0;
    write_buffer_pos_ = 0;

#ifdef MSTL_PLATFORM_WINDOWS__
    handle_ = ::CreateFileA(
        path.data(),
        static_cast<file_underlying_type_t>(access),
        static_cast<file_underlying_type_t>(share_mode),
        nullptr, static_cast<file_underlying_type_t>(creation),
        static_cast<file_underlying_type_t>(attributes),
        nullptr
        );
#elif defined(MSTL_PLATFORM_LINUX__)
    int flags = static_cast<int>(access);
    flags |= static_cast<int>(creation);
    if (append) flags |= O_APPEND;

    const ::mode_t mode = convert_attributes(attributes);
    handle_ = ::open(path.data(), flags, mode);
#endif
    if (handle_ == INVALID_HANDLE()) return false;

    path_ = _MSTL move(path);
    opened_ = true;
    append_mode_ = append;

    if (append && !this->seek(0, FILE_POINTER::END)) return false;
    return true;
}

void file::close() noexcept {
    if (opened_ && handle_ != INVALID_HANDLE()) {
        this->flush_write_buffer();
#ifdef MSTL_PLATFORM_WINDOWS__
        ::CloseHandle(handle_);
#elif defined(MSTL_PLATFORM_LINUX__)
        ::close(handle_);
#endif
        handle_ = INVALID_HANDLE();
        opened_ = false;
        append_mode_ = false;
    }
}

bool file::flush() const noexcept {
    if (!opened_ || handle_ == INVALID_HANDLE()) return false;
    if (!flush_write_buffer()) return false;
#ifdef MSTL_PLATFORM_WINDOWS__
    return ::FlushFileBuffers(handle_) != 0;
#elif defined(MSTL_PLATFORM_LINUX__)
    return ::fsync(handle_) == 0;
#endif
}

file::size_type file::write(const string& data, const size_type size) const {
    if (!opened_ || handle_ == INVALID_HANDLE()) return 0;

    const size_type real_size = size > data.size() ? data.size() : size;
    if (real_size == 0) return 0;

    if (real_size > FILE_BUFFER_SIZE * 4) {
        if (!flush_write_buffer()) return 0;
#ifdef MSTL_PLATFORM_WINDOWS__
        size_type bytes_written = 0;
        if (::WriteFile(handle_, data.data(), real_size, &bytes_written, nullptr)) {
            return bytes_written;
        }
        return 0;
#elif defined(MSTL_PLATFORM_LINUX__)
        ssize_t written;
        do {
            written = ::write(handle_, data.data(), real_size);
        } while (written == -1 && errno == EINTR);
        return written > 0 ? static_cast<size_type>(written) : 0;
#else
        return 0;
#endif
    }

    const char* ptr = data.data();
    size_type total_written = 0;
    size_type remaining = size;

    while (remaining > 0) {
        const size_type available = FILE_BUFFER_SIZE - write_buffer_pos_;
        const size_type to_copy = remaining < available ? remaining : available;

        _MSTL copy_n(ptr, to_copy, write_buffer_.begin() + static_cast<ptrdiff_t>(write_buffer_pos_));
        write_buffer_pos_ += to_copy;
        total_written += to_copy;
        ptr += to_copy;
        remaining -= to_copy;

        if (write_buffer_pos_ == FILE_BUFFER_SIZE && !flush_write_buffer()) {
            break;
        }
    }
    return total_written;
}

file::size_type file::read(string& str, const size_type size) const {
    if (!opened_ || handle_ == INVALID_HANDLE()) return 0;

    str.clear();
    str.reserve(size);
    size_type total_read = 0;

    while (total_read < size) {
        if (read_buffer_pos_ >= read_buffer_size_) {
            if (!fill_read_buffer() || read_buffer_size_ == 0) {
                break;
            }
        }
        const size_type available_in_buffer = read_buffer_size_ - read_buffer_pos_;
        const size_type needed = size - total_read;
        const size_type to_read = (needed < available_in_buffer) ? needed : available_in_buffer;

        str.append(read_buffer_.data() + read_buffer_pos_, to_read);
        read_buffer_pos_ += to_read;
        total_read += to_read;
    }
    return total_read;
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

file::size_type file::read_binary(string& str, const size_type size) const {
    if (!opened_ || handle_ == INVALID_HANDLE() || str.empty() || size == 0) return 0;
    size_type total_read = 0;

    while (total_read < size) {
        if (read_buffer_pos_ >= read_buffer_size_) {
            if (!fill_read_buffer() || read_buffer_size_ == 0) {
                break;
            }
        }
        const size_type available = read_buffer_size_ - read_buffer_pos_;
        const size_type to_read = size - total_read < available ? size - total_read : available;

        const auto buffer = str.data();
        _MSTL copy_n(read_buffer_.data() + read_buffer_pos_, to_read, buffer + total_read);
        read_buffer_pos_ += to_read;
        total_read += to_read;
    }
    return total_read;
}

bool file::read_line(string& line) const {
    if (!opened_ || handle_ == INVALID_HANDLE())
        return false;

    line.clear();
    bool line_complete = false;

    while (!line_complete) {
        if (read_buffer_pos_ >= read_buffer_size_) {
            if (!fill_read_buffer() || read_buffer_size_ == 0) {
                break;
            }
        }

        while (read_buffer_pos_ < read_buffer_size_ && !line_complete) {
            const char ch = read_buffer_[read_buffer_pos_++];
            if (ch == '\n') {
                line_complete = true;
            } else if (ch != '\r') {
                line += ch;
            }
        }
    }
    return !line.empty() || line_complete;
}

vector<string> file::read_lines() const {
    vector<string> lines;
    if (!opened_ || handle_ == INVALID_HANDLE()) return lines;

    string content;
    if (this->read(path_, content)) {
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
    }
    return lines;
}

file::size_type file::size() const noexcept {
    if (!opened_ || handle_ == INVALID_HANDLE()) return 0;
    if (write_buffer_pos_ > 0 && !flush_write_buffer()) return 0;
#ifdef MSTL_PLATFORM_WINDOWS__
    ::LARGE_INTEGER file_size;
    if (!::GetFileSizeEx(handle_, &file_size)) {
        return 0;
    }
    return static_cast<size_type>(file_size.QuadPart);
#elif defined(MSTL_PLATFORM_LINUX__)
    struct ::stat64 st{};
    if (::fstat64(handle_, &st) == -1) return 0;
    return static_cast<size_type>(st.st_size);
#endif
}

file::size_type file::size(const string& path) {
    size_type sz = 0;
    {
        file f;
        if (f.open(path, false, FILE_ACCESS::READ)) {
            sz = f.size();
        }
    }
    return sz;
}

bool file::size(const string& path, size_type& size) {
    {
        file f;
        if (f.open(path, false, FILE_ACCESS::READ)) {
            size = f.size();
            return true;
        }
    }
    return false;
}

bool file::seek(const difference_type distance, FILE_POINTER method) const noexcept {
    if (!opened_ || handle_ == INVALID_HANDLE()) return false;
    if (write_buffer_pos_ > 0 && !flush_write_buffer()) return false;

    read_buffer_pos_ = 0;
    read_buffer_size_ = 0;
#ifdef MSTL_PLATFORM_WINDOWS__
    ::LARGE_INTEGER li;
    li.QuadPart = distance;
    return ::SetFilePointerEx(handle_, li, nullptr,
        static_cast<file_underlying_type_t>(method))
    != 0;
#elif defined(MSTL_PLATFORM_LINUX__)
    const ::off_t ret = ::lseek(handle_, distance, static_cast<file_underlying_type_t>(method));
    return ret != static_cast<off_t>(-1);
#endif
}

file::difference_type file::tell() const noexcept {
    if (!opened_ || handle_ == INVALID_HANDLE()) return 0;
#ifdef MSTL_PLATFORM_WINDOWS__
    constexpr ::LARGE_INTEGER li = {};
    ::LARGE_INTEGER new_pos;
    if (!::SetFilePointerEx(handle_, li, &new_pos, FILE_CURRENT)) {
        return 0;
    }
    return static_cast<difference_type>(new_pos.QuadPart);
#elif defined(MSTL_PLATFORM_LINUX__)
    const ::off_t pos = ::lseek(handle_, 0, SEEK_CUR);
    return pos == static_cast<::off_t>(-1) ? 0 : pos;
#endif
}

bool file::prefetch(const size_type hint_size) const noexcept {
    if (read_buffer_pos_ < read_buffer_size_) return true;

#ifdef MSTL_PLATFORM_LINUX__
    const size_type read_size = hint_size > 0 ?
        _MSTL min(hint_size * 2, FILE_BUFFER_SIZE) :
        FILE_BUFFER_SIZE;

    ::posix_fadvise(handle_, this->tell(), static_cast<difference_type>(read_size), POSIX_FADV_WILLNEED);
#endif
    return fill_read_buffer();
}

bool file::truncate(const difference_type size) const noexcept {
    if (!opened_ || handle_ == INVALID_HANDLE()) return false;
#ifdef MSTL_PLATFORM_WINDOWS__
    if (!this->seek(size, FILE_POINTER::BEGIN)) return false;
    return ::SetEndOfFile(handle_) != 0;
#elif defined(MSTL_PLATFORM_LINUX__)
    return ::ftruncate(handle_, size) == 0;
#endif
}

bool file::lock(const difference_type offset,
    const difference_type length, FILE_LOCK mode) const noexcept {
    if (!opened_ || handle_ == INVALID_HANDLE()) return false;
#ifdef MSTL_PLATFORM_WINDOWS__
    ::OVERLAPPED ov = {};
    const uint64_t offset_64 = offset;
    ov.Offset = static_cast<size_type>(offset_64 & 0xFFFFFFFF);
    ov.OffsetHigh = static_cast<size_type>(offset_64 >> 32);

    const uint64_t length_64 = length;
    return ::LockFileEx(handle_, static_cast<file_underlying_type_t>(mode), 0,
        length_64 & 0xFFFFFFFF, length_64 >> 32, &ov)
    != 0;
#elif defined(MSTL_PLATFORM_LINUX__)
    struct ::flock fl{};
    if ((mode & FILE_LOCK::EXCLUSIVE) != FILE_LOCK::SHARED) {
        fl.l_type = F_WRLCK;
    } else {
        fl.l_type = F_RDLCK;
    }
    fl.l_whence = SEEK_SET;
    fl.l_start = offset;
    fl.l_len = length;

    const int cmd = static_cast<int>(mode) & LOCK_NB ? F_SETLK : F_SETLKW;
    return ::fcntl(handle_, cmd, &fl) != -1;
#endif
}

bool file::unlock(const difference_type offset, const difference_type length) const noexcept {
    if (!opened_ || handle_ == INVALID_HANDLE()) return false;
#ifdef MSTL_PLATFORM_WINDOWS__
    ::OVERLAPPED ov = {};
    const uint64_t offset_64 = offset;
    ov.Offset = static_cast<size_type>(offset_64 & 0xFFFFFFFF);
    ov.OffsetHigh = static_cast<size_type>(offset_64 >> 32);

    const uint64_t length_64 = length;
    return ::UnlockFileEx(handle_, 0, length_64 & 0xFFFFFFFF, length_64 >> 32, &ov) != 0;
#elif defined(MSTL_PLATFORM_LINUX__)
    struct ::flock fl{};
    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = offset;
    fl.l_len = length;
    return ::fcntl(handle_, F_SETLK, &fl) != -1;
#endif
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
    return ::SetFileAttributesA(path_.c_str(),
        static_cast<file_underlying_type_t>(attr))
    != 0;
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
    return set_all_times(last_access_time(), datetime::to_UTC(dt));
#endif
}

bool file::exists(const string_view path) noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    return ::GetFileAttributesA(path.data()) != INVALID_FILE_ATTRIBUTES;
#elif defined(MSTL_PLATFORM_LINUX__)
    struct ::stat64 st{};
    return ::stat64(path.data(), &st) != -1;
#endif
}

bool file::is_directory(const string_view path) noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    const size_type attrib = ::GetFileAttributesA(path.data());
    return attrib != INVALID_FILE_ATTRIBUTES && attrib & FILE_ATTRIBUTE_DIRECTORY;
#elif defined(MSTL_PLATFORM_LINUX__)
    struct ::stat64 st{};
    if (::stat64(path.data(), &st) == -1) return false;
    return S_ISDIR(st.st_mode);
#endif
}

bool file::is_file(const string_view path) noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    const size_type attrib = ::GetFileAttributesA(path.data());
    return attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY);
#elif defined(MSTL_PLATFORM_LINUX__)
    struct ::stat64 st{};
    if (::stat64(path.data(), &st) == -1) return false;
    return S_ISREG(st.st_mode) || S_ISLNK(st.st_mode);
#endif
}

string_view file::extension(const string_view path) noexcept {
    const size_t last_sep = path.find_last_of(FILE_SPLITER);
    const string_view filename = last_sep == string::npos ? path : path.substr(last_sep + 1);

    const size_t last_dot = filename.find_last_of('.');
    if (last_dot == string::npos || last_dot == 0 || last_dot == filename.size() - 1) {
        return {};
    }
    return filename.substr(last_dot + 1);
}

bool file::create_directories(const string& path) {
    if (path.empty()) return false;
    if (file::is_directory(path)) return true;
    size_t pos = 0;
    string subdir;
#ifdef MSTL_PLATFORM_WINDOWS__
    while ((pos = path.find_first_of(FILE_SPLITER, pos + 1)) != string::npos) {
        subdir = path.substr(0, pos);
        if (!subdir.empty() && !file::is_directory(subdir)) {
            if (!::CreateDirectoryA(subdir.data(), nullptr)) {
                if (::GetLastError() != ERROR_ALREADY_EXISTS) {
                    return false;
                }
            }
        }
    }
    return ::CreateDirectoryA(path.data(), nullptr) || ::GetLastError() == ERROR_ALREADY_EXISTS;
#elif defined(MSTL_PLATFORM_LINUX__)
    while ((pos = path.find_first_of(FILE_SPLITER, pos + 1)) != string::npos) {
        subdir = path.substr(0, pos);
        if (::mkdir(subdir.data(), 0755) == -1 && errno != EEXIST) {
            return false;
        }
    }
    return ::mkdir(path.data(), 0755) == 0 || errno == EEXIST;
#endif
}

bool file::create_and_write(const string& path, const string& content, const bool append) {
    const size_t last_slash = path.find_last_of(FILE_SPLITER);
    if (last_slash != string::npos) {
        const string dir = path.substr(0, last_slash);
        if (!dir.empty() && !is_directory(dir)) {
            if (!create_directories(dir)) {
                return false;
            }
        }
    }
#ifdef MSTL_PLATFORM_WINDOWS__
    file file;
    if (!file.open(path, append,
        append ? FILE_ACCESS::APPEND : FILE_ACCESS::WRITE,
        FILE_SHARED::NO_SHARE, FILE_CREATION::OPEN_FORCE,
        FILE_ATTRI::NORMAL)) {
        return false;
        }
    const size_type bytes_written = file.write(content.c_str(), content.size());
    return bytes_written == content.size();
#elif defined(MSTL_PLATFORM_LINUX__)
    int flags = O_WRONLY | O_CREAT;
    if (append) flags |= O_APPEND;
    else flags |= O_TRUNC;

    const int fd = ::open(path.c_str(), flags, 0644);
    if (fd == -1) return false;

    const ssize_t written = ::write(fd, content.c_str(), content.size());
    ::close(fd);
    return written == static_cast<ssize_t>(content.size());
#endif
}

bool file::remove(const string_view path) noexcept {
    if (file::is_file(path)) {
#ifdef MSTL_PLATFORM_WINDOWS__
        return ::DeleteFileA(path.data()) != 0;
#elif defined(MSTL_PLATFORM_LINUX__)
        return ::unlink(path.data()) == 0;
#endif
    }
    return false;
}

bool file::remove_directory(const string_view path) noexcept {
    if (file::is_directory(path)) {
#ifdef MSTL_PLATFORM_WINDOWS__
        return ::RemoveDirectoryA(path.data()) != 0;
#elif defined(MSTL_PLATFORM_LINUX__)
        return ::rmdir(path.data()) == 0;
#endif
    }
    return false;
}

bool file::remove_all_in_directory(const string& directory_path, const bool recursive) noexcept {
    if (!file::is_directory(directory_path)) return false;
    bool success = true;
#ifdef MSTL_PLATFORM_WINDOWS__
    const string search_pattern = directory_path + "\\*";
    ::WIN32_FIND_DATAA find_data;
    const file_handle find_handle = ::FindFirstFileA(search_pattern.c_str(), &find_data);
    if (find_handle == INVALID_HANDLE()) return false;

    do {
        string item_name = find_data.cFileName;
        if (item_name == "." || item_name == "..") continue;
        string full_path = directory_path + "\\" + item_name;

        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (recursive) {
                if (!remove_all_in_directory(full_path, true)) {
                    success = false;
                }
                if (!::RemoveDirectoryA(full_path.c_str())) {
                    if (::GetLastError() != ERROR_DIR_NOT_EMPTY) {
                        success = false;
                    }
                }
            }
        } else {
            if (find_data.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
                ::SetFileAttributesA(full_path.c_str(), find_data.dwFileAttributes & ~FILE_ATTRIBUTE_READONLY);
            }
            if (!::DeleteFileA(full_path.c_str())) {
                ::DWORD error = ::GetLastError();
                if (error == ERROR_ACCESS_DENIED) {
                    ::SetFileAttributesA(full_path.c_str(), FILE_ATTRIBUTE_NORMAL);

                    if (!::DeleteFileA(full_path.c_str())) {
                        error = ::GetLastError();
                        success = false;
                    }
                } else {
                    success = false;
                }
            }
        }
    } while (::FindNextFileA(find_handle, &find_data) != 0);
    ::FindClose(find_handle);
#elif defined(MSTL_PLATFORM_LINUX__)
    ::DIR* dir = ::opendir(directory_path.c_str());
    if (dir == nullptr) return false;

    ::dirent* entry;
    while ((entry = ::readdir(dir)) != nullptr) {
        string item_name = entry->d_name;
        if (item_name == "." || item_name == "..") continue;

        string full_path = directory_path + "/" + item_name;

        if (file::is_directory(full_path)) {
            if (recursive) {
                if (!remove_all_in_directory(full_path, true)) {
                    success = false;
                }
                if (::rmdir(full_path.c_str()) != 0 && errno != ENOTEMPTY) {
                    success = false;
                }
            }
        } else {
            if (::unlink(full_path.c_str()) != 0) {
                if (errno == EACCES) {
                    ::chmod(full_path.c_str(), 0644);
                    if (::unlink(full_path.c_str()) != 0) {
                        success = false;
                    }
                } else {
                    success = false;
                }
            }
        }
    }
    ::closedir(dir);
#endif
    return success;
}

bool file::read(
    const string& path, string& content,
    const FILE_CREATION creation, const FILE_ATTRI attributes) {
#ifdef MSTL_PLATFORM_WINDOWS__
    file file;
    if (!file.open(path, false,
        FILE_ACCESS::READ, FILE_SHARED::SHARE_READ,
        creation, attributes)) {
        return false;
    }
    const size_type size = file.size();
    if (size == 0) {
        content.clear();
        return true;
    }
    content.resize(size);
    const size_type bytes_read = file.read(content, size);
    return bytes_read == size;
#elif defined(MSTL_PLATFORM_LINUX__)
    const int fd = ::open(path.c_str(), O_RDONLY);
    if (fd == -1) return false;

    struct ::stat64 st{};
    if (::fstat64(fd, &st) == -1) {
        ::close(fd);
        return false;
    }
    content.resize(st.st_size);
    const ssize_t read = ::read(fd, content.data(), st.st_size);
    ::close(fd);
    return read == st.st_size;
#endif
}

bool file::read_binary(const string& path, string& content,
    const FILE_CREATION creation, const FILE_ATTRI attributes) {
    file f;
    if (!f.open(path, false,
        FILE_ACCESS::READ,
        FILE_SHARED::SHARE_READ,
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

bool file::copy(const string& from, const string& to, const bool overwrite) {
    if (!file::exists(from)) return false;
    if (file::is_directory(from)) return false;
    if (!overwrite && file::exists(to)) return false;

    const string dest_dir = to.substr(0, to.find_last_of(FILE_SPLITER));
    if (dest_dir != to && !file::exists(dest_dir)) {
        if (!file::create_directories(dest_dir)) return false;
    }

#ifdef MSTL_PLATFORM_WINDOWS__
    string actual_to = to;
    if (file::exists(to) && file::is_directory(to)) {
        const string filename = from.substr(from.find_last_of(FILE_SPLITER) + 1);
        actual_to = to + "\\" + filename;
    }

    if (overwrite && file::exists(actual_to)) {
        const ::DWORD attrs = ::GetFileAttributesA(actual_to.c_str());
        if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_READONLY)) {
            ::SetFileAttributesA(actual_to.c_str(), attrs & ~FILE_ATTRIBUTE_READONLY);
        }
    }
    if (::CopyFileA(from.c_str(), actual_to.c_str(), !overwrite)) return true;
    const ::DWORD error = ::GetLastError();
    return false;
#elif defined(MSTL_PLATFORM_LINUX__)
    const int source_fd = ::open(from.c_str(), O_RDONLY);
    if (source_fd == -1) return false;
    struct ::stat64 st;
    if (::fstat64(source_fd, &st) == -1) {
        ::close(source_fd);
        return false;
    }

    int flags = O_WRONLY | O_CREAT;
    if (overwrite) {
        flags |= O_TRUNC;
    } else {
        flags |= O_EXCL;
    }
    const int dest_fd = ::open(to.c_str(), flags, 0644);
    if (dest_fd == -1) {
        ::close(source_fd);
        return false;
    }

    char buffer[FILE_BUFFER_SIZE];
    ssize_t bytes_read;
    bool success = true;

    while ((bytes_read = ::read(source_fd, buffer, FILE_BUFFER_SIZE)) > 0) {
        const ssize_t bytes_written = ::write(dest_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            success = false;
            break;
        }
    }
    if (bytes_read < 0) success = false;

    if (success) {
        if (::fchmod(dest_fd, st.st_mode & 0777) == -1) {}
        ::timeval times[2];
        times[0].tv_sec = st.st_atime;
        times[0].tv_usec = 0;
        times[1].tv_sec = st.st_mtime;
        times[1].tv_usec = 0;
        if (::futimes(dest_fd, times) == -1) {}
    }

    ::close(source_fd);
    ::close(dest_fd);
    if (!success) ::unlink(to.c_str());
    return success;
#endif
}


bool file::copy_directory(const string& source, const string& destination, const bool overwrite) {
    if (!file::is_directory(source)) return false;
    if (!file::exists(destination) && !file::create_directories(destination)) return false;

#ifdef MSTL_PLATFORM_WINDOWS__
    const string findPattern = source + "/*";
    ::WIN32_FIND_DATAA findData;
    const file_handle hFind = ::FindFirstFileA(findPattern.c_str(), &findData);

    if (hFind == INVALID_HANDLE()) {
        if (::GetLastError() == ERROR_FILE_NOT_FOUND) return true;
        return false;
    }

    do {
        string item = findData.cFileName;
        if (item == "." || item == "..") continue;

        string srcPath = source + "/" + item;
        string dstPath = destination + "/" + item;

        if (file::is_directory(srcPath)) {
            if (!copy_directory(srcPath, dstPath, overwrite)) {
                ::FindClose(hFind);
                return false;
            }
        } else {
            if (!file::copy(srcPath, dstPath, overwrite)) {
                ::FindClose(hFind);
                return false;
            }
        }
    } while (::FindNextFileA(hFind, &findData) != 0);

    ::FindClose(hFind);

#elif defined(MSTL_PLATFORM_LINUX__)
    ::DIR *dir = ::opendir(source.c_str());
    if (dir == nullptr) return false;

    ::dirent *entry;
    while ((entry = ::readdir(dir)) != nullptr) {
        string item = entry->d_name;
        if (item == "." || item == "..") continue;

        string src_path = source + "/" + item;
        string dst_path = destination + "/" + item;
        if (file::is_directory(src_path)) {
            if (!file::copy_directory(src_path, dst_path, overwrite)) {
                ::closedir(dir);
                return false;
            }
        } else {
            if (!file::copy(src_path, dst_path, overwrite)) {
                ::closedir(dir);
                return false;
            }
        }
    }
    ::closedir(dir);
#endif
    return true;
}


bool file::move(const string& from, const string& to, const bool overwrite) noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::DWORD flags = MOVEFILE_COPY_ALLOWED;
    if (overwrite) flags |= MOVEFILE_REPLACE_EXISTING;
    if (::MoveFileExA(from.c_str(), to.c_str(), flags)) return true;

    ::DWORD error = ::GetLastError();
    if (file::copy(from, to, overwrite)) {
        if (file::remove(from)) return true;
        return false;
    }
    return false;
#elif defined(MSTL_PLATFORM_LINUX__)
    if (overwrite) {
        if (::rename(from.c_str(), to.c_str()) == 0) return true;
        if (errno != EEXIST) return false;
        if (file::remove(to) != 0) return false;
        return ::rename(from.c_str(), to.c_str()) == 0;
    }
    return ::rename(from.c_str(), to.c_str()) == 0;
#endif
}

MSTL_END_NAMESPACE__
