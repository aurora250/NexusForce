#include <MSTL/core/file/file.hpp>
#ifdef MSTL_PLATFORM_LINUX__
#include <sys/file.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <ctime>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#endif
MSTL_BEGIN_NAMESPACE__

bool file::flush_write_buffer() const noexcept {
    if (write_buffer_pos_ == 0) return true;

#ifdef MSTL_PLATFORM_WINDOWS__
    size_type bytes_written;
    const ::BOOL success = ::WriteFile(handle_, write_buffer_.data(),
        write_buffer_pos_, &bytes_written, nullptr);
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
    other.path_ = path{};
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
    other.path_ = path{};
    other.read_buffer_.clear();
    other.read_buffer_pos_ = 0;
    other.read_buffer_size_ = 0;
    other.write_buffer_.clear();
    other.write_buffer_pos_ = 0;

    return *this;
}

file::~file() {
    unmap();

#ifdef MSTL_PLATFORM_WINDOWS__
    for (auto* ov : async_operations_) {
        if (ov) {
            ::DWORD bytes_transferred = 0;
            ::GetOverlappedResult(handle_, ov, &bytes_transferred, TRUE);
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

            delete aiocb;
        }
    }
#endif
    async_operations_.clear();

    this->close();
}

bool file::open(path p, const bool append,
    FILE_ACCESS access, FILE_SHARED share_mode,
    FILE_CREATION creation, FILE_ATTRI attributes) {
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
    fud_t flags = static_cast<fud_t>(access);
    flags |= static_cast<fud_t>(creation);
    if (append) flags |= O_APPEND;

    const ::mode_t mode = convert_attributes(attributes);
    handle_ = ::open(p.c_str(), flags, mode);
#endif

    if (handle_ == INVALID_HANDLE()) {
        set_last_error();
        return false;
    }

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

    if (real_size > buffer_size_ * 4) {
        if (!flush_write_buffer()) return 0;

        size_type total_written = 0;
        const char* ptr = data.data();

        while (total_written < real_size) {
#ifdef MSTL_PLATFORM_WINDOWS__
            size_type bytes_written = 0;
            const size_type to_write = real_size - total_written;
            if (!::WriteFile(handle_, ptr + total_written, to_write, &bytes_written, nullptr)) {
                set_last_error();
                break;
            }
            if (bytes_written == 0) break;
            total_written += bytes_written;
#elif defined(MSTL_PLATFORM_LINUX__)
            ssize_t written = ::write(
                handle_, ptr + total_written, real_size - total_written);
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

    const char* ptr = data.data();
    size_type total_written = 0;
    size_type remaining = real_size;

    while (remaining > 0) {
        const size_type available = buffer_size_ - write_buffer_pos_;
        const size_type to_copy = _MSTL min(remaining, available);

        _MSTL copy_n(ptr, to_copy,
                     write_buffer_.begin() + write_buffer_pos_);
        write_buffer_pos_ += to_copy;
        total_written += to_copy;
        ptr += to_copy;
        remaining -= to_copy;

        if (write_buffer_pos_ == buffer_size_ && !flush_write_buffer()) {
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

vector<string> file::read_chunks(size_type chunk_size) const {
    vector<string> chunks;
    if (!opened_ || handle_ == INVALID_HANDLE()) return chunks;

    const size_type file_sz = size();
    if (file_sz == 0) return chunks;

    const difference_type original_pos = tell();
    seek(0, FILE_POINTER::BEGIN);

    size_type remaining = file_sz;
    while (remaining > 0) {
        const size_type to_read = remaining < chunk_size ? remaining : chunk_size;
        string chunk;
        chunk.resize(to_read);

        const size_type bytes_read = read(chunk, to_read);
        if (bytes_read != to_read) {
            chunk.resize(bytes_read);
        }

        if (!chunk.empty()) {
            chunks.emplace_back(_MSTL move(chunk));
        }

        remaining -= bytes_read;
        if (bytes_read == 0) break;
    }

    seek(original_pos, FILE_POINTER::BEGIN);
    return chunks;
}

bool file::write_chunks(const vector<string>& chunks) {
    if (!opened_ || handle_ == INVALID_HANDLE()) return false;

    for (const auto& chunk : chunks) {
        const size_type bytes_written = write(chunk, chunk.size());
        if (bytes_written != chunk.size()) {
            set_last_error();
            return false;
        }
    }

    return flush();
}

vector<file::chunk_info> file::get_chunk_info(size_type chunk_size) const {
    vector<chunk_info> info;
    if (!opened_ || handle_ == INVALID_HANDLE()) return info;

    const size_type file_sz = size();
    if (file_sz == 0) return info;

    difference_type offset = 0;
    size_type index = 0;

    while (offset < static_cast<difference_type>(file_sz)) {
        chunk_info ci;
        ci.offset = offset;
        ci.chunk_index = index;

        const size_type remaining = file_sz - offset;
        ci.size = remaining < chunk_size ? remaining : chunk_size;

        info.push_back(ci);

        offset += ci.size;
        ++index;
    }

    return info;
}

file::size_type file::read_binary(string& str, const size_type size) const {
    if (!opened_ || handle_ == INVALID_HANDLE() || str.empty() || size == 0) return 0;

    if (str.size() < size) {
        str.resize(size);
    }

    size_type total_read = 0;
    char* buffer = str.data();

    while (total_read < size) {
        if (read_buffer_pos_ >= read_buffer_size_) {
            if (!fill_read_buffer() || read_buffer_size_ == 0) {
                break;
            }
        }

        const size_type available = read_buffer_size_ - read_buffer_pos_;
        const size_type to_read = _MSTL min(size - total_read, available);

        _MSTL copy_n(read_buffer_.data() + read_buffer_pos_,
                     to_read, buffer + total_read);
        read_buffer_pos_ += to_read;
        total_read += to_read;
    }

    if (total_read < size) {
        str.resize(total_read);
    }

    return total_read;
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
            const char ch = read_buffer_[read_buffer_pos_++];

            if (ch == '\r') {
                if (read_buffer_pos_ < read_buffer_size_) {
                    if (read_buffer_[read_buffer_pos_] == '\n') {
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


file::async_result file::async_read(string& buffer, size_type size, difference_type offset) {
    async_result result;
    if (!opened_ || handle_ == INVALID_HANDLE()) {
        set_last_error();
        result.error_code = last_error_code_;
        return result;
    }

    buffer.resize(size);

#ifdef MSTL_PLATFORM_WINDOWS__
    buffer.resize(size);

    auto* ov = new ::OVERLAPPED{};
    _MSTL fill_n(reinterpret_cast<char*>(ov), sizeof(::OVERLAPPED), 0);

    if (offset >= 0) {
        const uint64_t offset_64 = static_cast<uint64_t>(offset);
        ov->Offset = static_cast<::DWORD>(offset_64 & 0xFFFFFFFF);
        ov->OffsetHigh = static_cast<::DWORD>(offset_64 >> 32);
    }

    ov->hEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!ov->hEvent) {
        delete ov;
        set_last_error();
        result.error_code = last_error_code_;
        return result;
    }

    ::DWORD bytes_read = 0;
    if (::ReadFile(handle_, buffer.data(), size, &bytes_read, ov)) {
        result.completed = true;
        result.bytes_transferred = bytes_read;
        ::CloseHandle(ov->hEvent);
        delete ov;
    } else {
        const ::DWORD error = ::GetLastError();
        if (error == ERROR_IO_PENDING) {
            result.completed = false;
            result.overlapped = ov;
            async_operations_.push_back(ov);
        } else {
            set_last_error();
            result.error_code = last_error_code_;
            ::CloseHandle(ov->hEvent);
            delete ov;
        }
    }
#elif defined(MSTL_PLATFORM_LINUX__)
    auto* aiocb = new ::aiocb{};
    _MSTL fill_n(reinterpret_cast<char*>(aiocb), sizeof(::aiocb), 0);

    aiocb->aio_fildes = handle_;
    aiocb->aio_buf = buffer.data();
    aiocb->aio_nbytes = size;
    aiocb->aio_offset = offset >= 0 ? offset : this->tell();
    aiocb->aio_sigevent.sigev_notify = SIGEV_NONE;

    if (::aio_read(aiocb) == 0) {
        result.completed = false;
        result.cb = aiocb;
        async_operations_.push_back(aiocb);
    } else {
        set_last_error();
        result.error_code = last_error_code_;
        delete aiocb;
    }
#endif
    return result;
}

file::async_result file::async_write(const string& data, size_type size, difference_type offset) {
    async_result result;
    if (!opened_ || handle_ == INVALID_HANDLE()) {
        set_last_error();
        result.error_code = last_error_code_;
        return result;
    }

    const size_type real_size = size > data.size() ? data.size() : size;

#ifdef MSTL_PLATFORM_WINDOWS__
    auto* ov = new ::OVERLAPPED{};
    _MSTL fill_n(reinterpret_cast<char*>(ov), sizeof(::OVERLAPPED), 0);

    if (offset >= 0) {
        const uint64_t offset_64 = static_cast<uint64_t>(offset);
        ov->Offset = static_cast<::DWORD>(offset_64 & 0xFFFFFFFF);
        ov->OffsetHigh = static_cast<::DWORD>(offset_64 >> 32);
    }

    ov->hEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!ov->hEvent) {
        delete ov;
        set_last_error();
        result.error_code = last_error_code_;
        return result;
    }

    ::DWORD bytes_written = 0;
    if (::WriteFile(handle_, data.data(), real_size, &bytes_written, ov)) {
        result.completed = true;
        result.bytes_transferred = bytes_written;
        ::CloseHandle(ov->hEvent);
        delete ov;
    } else {
        const ::DWORD error = ::GetLastError();
        if (error == ERROR_IO_PENDING) {
            result.completed = false;
            result.overlapped = ov;
            async_operations_.push_back(ov);
        } else {
            set_last_error();
            result.error_code = last_error_code_;
            ::CloseHandle(ov->hEvent);
            delete ov;
        }
    }
#elif defined(MSTL_PLATFORM_LINUX__)
    auto* aiocb = new ::aiocb{};
    _MSTL fill_n(reinterpret_cast<char*>(aiocb), sizeof(struct ::aiocb), 0);

    aiocb->aio_fildes = handle_;
    aiocb->aio_buf = const_cast<char*>(data.data());
    aiocb->aio_nbytes = real_size;
    aiocb->aio_offset = offset >= 0 ? offset : this->tell();
    aiocb->aio_sigevent.sigev_notify = SIGEV_NONE;

    if (::aio_write(aiocb) == 0) {
        result.completed = false;
        result.cb = aiocb;
        async_operations_.push_back(aiocb);
    } else {
        set_last_error();
        result.error_code = last_error_code_;
        delete aiocb;
    }
#endif
    return result;
}

bool file::wait_async(async_result& result, uint32_t timeout_ms) {
    if (result.completed) return true;
#ifdef MSTL_PLATFORM_WINDOWS__
    if (!result.overlapped) return false;

    ::DWORD bytes_transferred = 0;
    const ::BOOL success = ::GetOverlappedResult(
        handle_,
        result.overlapped,
        &bytes_transferred,
        TRUE
    );

    if (success) {
        result.completed = true;
        result.bytes_transferred = bytes_transferred;
        ::CloseHandle(result.overlapped->hEvent);

        auto it = _MSTL find(async_operations_.begin(), async_operations_.end(), result.overlapped);
        if (it != async_operations_.end()) {
            async_operations_.erase(it);
        }

        delete result.overlapped;
        result.overlapped = nullptr;
        return true;
    }

    set_last_error();
    result.error_code = last_error_code_;
    return false;
#elif defined(MSTL_PLATFORM_LINUX__)
    if (!result.cb) {
        result.error_code = EINVAL;
        return false;
    }

    ::aiocb* target_aiocb = result.cb;
    const ::aiocb* aiocb_list[1] = { target_aiocb };
    ::timespec timeout;
    ::timespec* timeout_ptr = nullptr;

    if (timeout_ms != 0xFFFFFFFF) {
        timeout.tv_sec = timeout_ms / 1000;
        timeout.tv_nsec = (timeout_ms % 1000) * 1000000;
        timeout_ptr = &timeout;
    }

    const int suspend_result = ::aio_suspend(aiocb_list, 1, timeout_ptr);

    if (suspend_result == 0) {
        const int error = ::aio_error(target_aiocb);

        if (error == 0) {
            const ssize_t return_value = ::aio_return(target_aiocb);
            if (return_value >= 0) {
                result.completed = true;
                result.bytes_transferred = static_cast<size_type>(return_value);
                result.error_code = 0;

                auto it = _MSTL find(async_operations_.begin(), async_operations_.end(), target_aiocb);
                if (it != async_operations_.end()) {
                    async_operations_.erase(it);
                }

                delete target_aiocb;
                result.cb = nullptr;
                return true;
            } else {
                result.error_code = errno;
                set_last_error();

                auto it = _MSTL find(async_operations_.begin(), async_operations_.end(), target_aiocb);
                if (it != async_operations_.end()) {
                    async_operations_.erase(it);
                }

                delete target_aiocb;
                result.cb = nullptr;
                return false;
            }
        } else if (error == EINPROGRESS) {
            result.error_code = EINPROGRESS;
            return false;
        } else {
            result.error_code = error;
            last_error_code_ = error;
            last_error_msg_ = ::strerror(error);

            ::aio_return(target_aiocb);

            auto it = _MSTL find(async_operations_.begin(), async_operations_.end(), target_aiocb);
            if (it != async_operations_.end()) {
                async_operations_.erase(it);
            }

            delete target_aiocb;
            result.cb = nullptr;
            return false;
        }
    } else {
        if (errno == EAGAIN) {
            result.error_code = ETIMEDOUT;
            return false;
        } else {
            set_last_error();
            result.error_code = last_error_code_;
            return false;
        }
    }
#endif
}

void file::cancel_async(async_result& result) {
    if (result.completed) return;
#ifdef MSTL_PLATFORM_WINDOWS__
    if (!result.overlapped) return;

    ::CancelIoEx(handle_, result.overlapped);
    ::CloseHandle(result.overlapped->hEvent);

    auto it = _MSTL find(async_operations_.begin(), async_operations_.end(), result.overlapped);
    if (it != async_operations_.end()) {
        async_operations_.erase(it);
    }

    delete result.overlapped;
    result.overlapped = nullptr;
#elif defined(MSTL_PLATFORM_LINUX__)
    if (!result.cb) return;

    ::aiocb* target_aiocb = result.cb;
    const int cancel_result = ::aio_cancel(handle_, target_aiocb);

    switch (cancel_result) {
        case AIO_CANCELED: {
            result.completed = true;
            result.error_code = ECANCELED;
            break;
        }
        case AIO_NOTCANCELED: {
            const ::aiocb* aiocb_list[1] = { target_aiocb };
            ::aio_suspend(aiocb_list, 1, nullptr);
            const ssize_t return_value = ::aio_return(target_aiocb);
            result.completed = true;
            result.bytes_transferred = return_value >= 0 ?
                static_cast<size_type>(return_value) : 0;
            result.error_code = return_value >= 0 ? 0 : errno;
            break;
        }
        case AIO_ALLDONE: {
            const ssize_t return_value = ::aio_return(target_aiocb);
            result.completed = true;
            result.bytes_transferred = return_value >= 0 ?
                static_cast<size_type>(return_value) : 0;
            result.error_code = return_value >= 0 ? 0 : errno;
            break;
        }
        default: {
            set_last_error();
            result.error_code = last_error_code_;
            break;
        }
    }

    auto it = _MSTL find(
        async_operations_.begin(),
        async_operations_.end(),
        target_aiocb);

    if (it != async_operations_.end()) {
        async_operations_.erase(it);
    }
    delete target_aiocb;
    result.cb = nullptr;
#endif
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

file::size_type file::size(const path& p) {
    size_type sz = 0;
    {
        file f;
        if (f.open(p, false, FILE_ACCESS::READ)) {
            sz = f.size();
        }
    }
    return sz;
}

bool file::size(const path& p, size_type& size) {
    {
        file f;
        if (f.open(p, false, FILE_ACCESS::READ)) {
            size = f.size();
            return true;
        }
    }
    return false;
}

bool file::create_and_write(const path& p, const string& content, const bool append) {
    const path parent = p.parent_path();
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

bool file::compare(const path& file1, const path& file2, bool binary) {
    return binary ? compare_binary(file1, file2) : compare_text(file1, file2);
}

bool file::compare_binary(const path& file1, const path& file2) {
    file f1, f2;
    if (!f1.open(file1, false, FILE_ACCESS::READ, FILE_SHARED::SHARE_READ)) {
        return false;
    }
    if (!f2.open(file2, false, FILE_ACCESS::READ, FILE_SHARED::SHARE_READ)) {
        return false;
    }

    const size_type size1 = f1.size();
    const size_type size2 = f2.size();
    if (size1 != size2) return false;
    if (size1 == 0) return true;

    constexpr size_type COMPARE_BUFFER_SIZE = FILE_BUFFER_SIZE * 4;
    string buffer1, buffer2;

    while (true) {
        buffer1.resize(COMPARE_BUFFER_SIZE);
        buffer2.resize(COMPARE_BUFFER_SIZE);

        const size_type bytes_read1 = f1.read(buffer1, COMPARE_BUFFER_SIZE);
        const size_type bytes_read2 = f2.read(buffer2, COMPARE_BUFFER_SIZE);

        if (bytes_read1 != bytes_read2) return false;
        if (bytes_read1 == 0) break;

        buffer1.resize(bytes_read1);
        buffer2.resize(bytes_read2);

        if (buffer1 != buffer2) return false;
    }

    return true;
}

bool file::compare_text(const path& file1, const path& file2) {
    file f1, f2;
    if (!f1.open(file1, false, FILE_ACCESS::READ, FILE_SHARED::SHARE_READ)) {
        return false;
    }
    if (!f2.open(file2, false, FILE_ACCESS::READ, FILE_SHARED::SHARE_READ)) {
        return false;
    }

    string line1, line2;
    while (true) {
        const bool has_line1 = f1.read_line(line1);
        const bool has_line2 = f2.read_line(line2);

        if (has_line1 != has_line2) return false;
        if (!has_line1) break;

        if (line1 != line2) return false;
    }

    return true;
}

vector<file::binary_diff_entry> file::binary_diff(
    const path& file1, const path& file2, size_type max_diffs) {
    vector<binary_diff_entry> diffs;

    file f1, f2;
    if (!f1.open(file1, false, FILE_ACCESS::READ, FILE_SHARED::SHARE_READ)) {
        return diffs;
    }
    if (!f2.open(file2, false, FILE_ACCESS::READ, FILE_SHARED::SHARE_READ)) {
        return diffs;
    }

    const size_type size1 = f1.size();
    const size_type size2 = f2.size();
    const size_type min_size = size1 < size2 ? size1 : size2;

    constexpr size_type COMPARE_BUFFER_SIZE = FILE_BUFFER_SIZE * 4;
    string buffer1, buffer2;
    difference_type offset = 0;

    while (offset < static_cast<difference_type>(min_size) && diffs.size() < max_diffs) {
        buffer1.resize(COMPARE_BUFFER_SIZE);
        buffer2.resize(COMPARE_BUFFER_SIZE);

        const size_type bytes_read1 = f1.read(buffer1, COMPARE_BUFFER_SIZE);
        const size_type bytes_read2 = f2.read(buffer2, COMPARE_BUFFER_SIZE);

        if (bytes_read1 == 0 || bytes_read2 == 0) break;

        buffer1.resize(bytes_read1);
        buffer2.resize(bytes_read2);

        const size_type compare_size = bytes_read1 < bytes_read2 ? bytes_read1 : bytes_read2;

        for (size_type i = 0; i < compare_size && diffs.size() < max_diffs; ++i) {
            if (buffer1[i] != buffer2[i]) {
                binary_diff_entry entry;
                entry.offset = offset + static_cast<difference_type>(i);
                entry.byte1 = static_cast<unsigned char>(buffer1[i]);
                entry.byte2 = static_cast<unsigned char>(buffer2[i]);
                diffs.push_back(entry);
            }
        }

        offset += static_cast<difference_type>(compare_size);
    }

    if (size1 != size2 && diffs.size() < max_diffs) {
        binary_diff_entry entry;
        entry.offset = static_cast<difference_type>(min_size);
        entry.byte1 = size1 > size2 ? 0xFF : 0x00;
        entry.byte2 = size2 > size1 ? 0xFF : 0x00;
        diffs.push_back(entry);
    }

    return diffs;
}

bool file::seek(const difference_type distance, FILE_POINTER method) const noexcept {
    if (!opened_ || handle_ == INVALID_HANDLE()) return false;
    if (write_buffer_pos_ > 0 && !flush_write_buffer()) return false;

    read_buffer_pos_ = 0;
    read_buffer_size_ = 0;

#ifdef MSTL_PLATFORM_WINDOWS__
    ::LARGE_INTEGER li{};
    li.QuadPart = distance;
    return ::SetFilePointerEx(handle_, li, nullptr, static_cast<fud_t>(method)) != 0;
#elif defined(MSTL_PLATFORM_LINUX__)
    const ::off_t ret = ::lseek(handle_, distance, static_cast<fud_t>(method));
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
        _MSTL min(hint_size * 2, buffer_size_) :
        buffer_size_;

    ::posix_fadvise(handle_, this->tell(),
        static_cast<difference_type>(read_size), POSIX_FADV_WILLNEED);
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
    return ::LockFileEx(handle_, static_cast<fud_t>(mode), 0,
        length_64 & 0xFFFFFFFF, length_64 >> 32, &ov) != 0;
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

    const fud_t cmd = static_cast<fud_t>(mode) & LOCK_NB ? F_SETLK : F_SETLKW;
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

bool file::map(size_type offset, size_type size,
    const FILE_ACCESS access, const FILE_MAP_HINT hint) {
    if (mapped_ptr_) unmap();
    if (!opened_ || handle_ == INVALID_HANDLE()) {
        set_last_error();
        return false;
    }

    if (size == 0) {
        const size_type file_size = this->size();
        if (offset >= file_size) {
            last_error_msg_ = "Offset exceeds file size";
            return false;
        }
        size = file_size - offset;
    }

#ifdef MSTL_PLATFORM_WINDOWS__
    ::DWORD protect = PAGE_READONLY;
    ::DWORD map_access = FILE_MAP_READ;

    if ((access & FILE_ACCESS::WRITE) != static_cast<FILE_ACCESS>(0)) {
        protect = PAGE_READWRITE;
        map_access = FILE_MAP_WRITE | FILE_MAP_READ;
    }

    const uint64_t map_size = static_cast<uint64_t>(size);
    mapping_handle_ = ::CreateFileMappingA(
        handle_, nullptr, protect,
        static_cast<::DWORD>(map_size >> 32),
        static_cast<::DWORD>(map_size & 0xFFFFFFFF),
        nullptr
    );

    if (!mapping_handle_ || mapping_handle_ == INVALID_HANDLE_VALUE) {
        set_last_error();
        mapping_handle_ = INVALID_HANDLE_VALUE;
        return false;
    }

    const uint64_t offset_64 = static_cast<uint64_t>(offset);
    mapped_ptr_ = ::MapViewOfFile(
        mapping_handle_, map_access,
        static_cast<::DWORD>(offset_64 >> 32),
        static_cast<::DWORD>(offset_64 & 0xFFFFFFFF),
        size
    );

    if (!mapped_ptr_) {
        set_last_error();
        ::CloseHandle(mapping_handle_);
        mapping_handle_ = INVALID_HANDLE_VALUE;
        return false;
    }

#elif defined(MSTL_PLATFORM_LINUX__)
    int prot = PROT_READ;
    if ((access & FILE_ACCESS::WRITE) != static_cast<FILE_ACCESS>(0)) {
        prot |= PROT_WRITE;
    }

    mapped_ptr_ = ::mmap(nullptr, size, prot, MAP_SHARED, handle_, offset);
    if (mapped_ptr_ == MAP_FAILED) {
        set_last_error();
        mapped_ptr_ = nullptr;
        return false;
    }

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
        default: {
            advice = MADV_NORMAL;
            break;
        }
    }
    ::madvise(mapped_ptr_, size, advice);
#endif

    mapped_size_ = size;
    return true;
}

void file::unmap() noexcept {
    if (!mapped_ptr_) return;

#ifdef MSTL_PLATFORM_WINDOWS__
    ::UnmapViewOfFile(mapped_ptr_);
    if (mapping_handle_ != INVALID_HANDLE_VALUE) {
        ::CloseHandle(mapping_handle_);
        mapping_handle_ = INVALID_HANDLE_VALUE;
    }
#elif defined(MSTL_PLATFORM_LINUX__)
    ::munmap(mapped_ptr_, mapped_size_);
#endif

    mapped_ptr_ = nullptr;
    mapped_size_ = 0;
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

bool file::read(const path& p, string& content, FILE_CREATION creation, FILE_ATTRI attributes) {
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

bool file::read_binary(const path& p, string& content,
    FILE_CREATION creation, FILE_ATTRI attributes) {
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

MSTL_END_NAMESPACE__
