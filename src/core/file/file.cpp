#include <NeForce/core/file/file.hpp>
#ifdef NEFORCE_PLATFORM_LINUX
#    include <cerrno>
#    include <cstring>
#    include <sys/stat.h>
#    include <unistd.h>
#endif
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <memoryapi.h>
#    if (_WIN32_WINNT < _WIN32_WINNT_WIN8)
typedef struct _WIN32_MEMORY_RANGE_ENTRY {
    PVOID VirtualAddress;
    SIZE_T NumberOfBytes;
} WIN32_MEMORY_RANGE_ENTRY, *PWIN32_MEMORY_RANGE_ENTRY;
#    endif
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
    const file::native_handle_type invalid_handle =
#ifdef NEFORCE_PLATFORM_WINDOWS
            INVALID_HANDLE_VALUE;
#else
            -1;
#endif

#ifdef NEFORCE_PLATFORM_LINUX
    ::mode_t convert_attributes(const file_attri attr) {
        ::mode_t mode = 0;
        if ((attr & file_attri::READONLY) != file_attri::OTHERS) {
            mode |= S_IRUSR | S_IRGRP | S_IROTH;
        } else {
            mode |= S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
        }
        return mode;
    }
#endif
} // namespace


file::line_iterator::line_iterator(const file* f) :
file_(f) {
    if (file_ != nullptr && file_->is_opened()) {
        ++(*this);
    }
}

file::line_iterator& file::line_iterator::operator++() {
    if (file_ != nullptr && !file_->read_line(current_line_)) {
        file_ = nullptr;
    }
    return *this;
}

file::line_iterator file::line_iterator::operator++(int) {
    line_iterator tmp = {*this};
    ++(*this);
    return tmp;
}

bool file::flush_write_buffer() const noexcept {
    if (write_buffer_pos_ == 0) {
        return true;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    size_type bytes_written = 0;
    const ::BOOL success = ::WriteFile(handle_, write_buffer_.data(), write_buffer_pos_, &bytes_written, nullptr);

    if (success == FALSE || bytes_written != write_buffer_pos_) {
        return false;
    }

#else
    ssize_t total_written = 0;
    while (total_written < static_cast<ssize_t>(write_buffer_pos_)) {
        const ssize_t bytes_written =
                ::write(handle_, write_buffer_.data() + total_written, write_buffer_pos_ - total_written);

        if (bytes_written == -1) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        total_written += bytes_written;
    }

#endif

    write_buffer_pos_ = 0;
    return true;
}

bool file::fill_read_buffer() const {
    if (read_buffer_.empty()) {
        read_buffer_.resize(buffer_size_);
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    size_type bytes_read = 0;
    const ::BOOL success =
            ::ReadFile(handle_, read_buffer_.data(), static_cast<::DWORD>(buffer_size_), &bytes_read, nullptr);

    if (success == FALSE) {
        read_buffer_size_ = 0;
        return false;
    }
    read_buffer_size_ = bytes_read;

#else
    ssize_t bytes_read = 0;
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

void file::init_sub_objects() noexcept {
    map_ = make_unique<file_mapper>(handle_);
    locker_ = make_unique<file_locker>(handle_);
    info_ = make_unique<file_info>(handle_);
    async_ = make_unique<file_async>(handle_);
}

void file::reset_sub_objects() noexcept {
    map_.reset();
    async_.reset();
    locker_.reset();
    info_.reset();
}

void file::set_last_error() const { last_error_code_ = _NEFORCE last_error(); }

void file::adjust_buffer_size() {
    if (!opened_ || handle_ == invalid_handle) {
        return;
    }

    const size_type file_sz = size();

    if (file_sz == 0) {
        buffer_size_ = buffer_size / 4;
    } else if (file_sz < buffer_size) {
        buffer_size_ = file_sz;
    } else if (file_sz > buffer_size * 1000) {
        buffer_size_ = buffer_size * 8;
    } else if (file_sz > buffer_size * 100) {
        buffer_size_ = buffer_size * 4;
    } else {
        buffer_size_ = buffer_size;
    }

    read_buffer_.resize(buffer_size_);
    write_buffer_.resize(buffer_size_);
}

file::file() :
handle_(invalid_handle) {}

file::file(path pth, const bool append, const file_access access, const file_shared share_mode,
           const file_creation creation, const file_attri attributes) :
path_(move(pth)) {
    open(path_, append, access, share_mode, creation, attributes);
}

file::file(file&& other) noexcept :
handle_(other.handle_),
path_(move(other.path_)),
opened_(other.opened_),
append_mode_(other.append_mode_),
read_buffer_(move(other.read_buffer_)),
read_buffer_pos_(other.read_buffer_pos_),
read_buffer_size_(other.read_buffer_size_),
write_buffer_(move(other.write_buffer_)),
write_buffer_pos_(other.write_buffer_pos_),
last_error_code_(other.last_error_code_) {
    other.handle_ = invalid_handle;
    other.opened_ = false;
    other.append_mode_ = false;
    other.path_ = path{};
    other.read_buffer_.clear();
    other.read_buffer_pos_ = 0;
    other.read_buffer_size_ = 0;
    other.write_buffer_.clear();
    other.write_buffer_pos_ = 0;
    other.last_error_code_.clear();
}

file& file::operator=(file&& other) noexcept {
    if (this == addressof(other)) {
        return *this;
    }

    close();
    handle_ = other.handle_;
    path_ = move(other.path_);
    opened_ = other.opened_;
    append_mode_ = other.append_mode_;
    read_buffer_ = move(other.read_buffer_);
    read_buffer_pos_ = other.read_buffer_pos_;
    read_buffer_size_ = other.read_buffer_size_;
    write_buffer_ = move(other.write_buffer_);
    write_buffer_pos_ = other.write_buffer_pos_;

    other.handle_ = invalid_handle;
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

file::~file() { close(); }

bool file::open(path pth, const bool append, file_access access, file_shared share_mode, file_creation creation,
                file_attri attributes) {
    close();
    clear_error();

    read_buffer_.resize(buffer_size_);
    write_buffer_.resize(buffer_size_);
    read_buffer_pos_ = 0;
    read_buffer_size_ = 0;
    write_buffer_pos_ = 0;

#ifdef NEFORCE_PLATFORM_WINDOWS
    handle_ = ::CreateFileA(pth.data(), static_cast<fud_t>(access), static_cast<fud_t>(share_mode), nullptr,
                            static_cast<fud_t>(creation), static_cast<fud_t>(attributes), nullptr);

#else
    auto flags = static_cast<fud_t>(access);
    const auto creation_flags = static_cast<fud_t>(creation);

    if ((creation_flags & O_CREAT) != 0 && (flags & (O_RDONLY | O_WRONLY | O_RDWR)) == 0) {
        flags |= O_RDWR;
    }
    flags |= creation_flags;

    if (append) {
        flags |= O_APPEND;
        if ((creation_flags & O_TRUNC) != 0) {
            flags &= ~O_TRUNC;
        }
    }

    ::mode_t mode = 0;
    if ((creation_flags & O_CREAT) != 0) {
        if (attributes == file_attri::OTHERS) {
            mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
        } else {
            mode = convert_attributes(attributes);
            if (mode == 0) {
                mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
            }
        }
    }

    if ((creation_flags & O_CREAT) != 0) {
        handle_ = ::open(pth.data(), flags, mode);
    } else {
        handle_ = ::open(pth.data(), flags);
    }

#endif

    if (handle_ == invalid_handle) {
        set_last_error();
        return false;
    }
#ifdef NEFORCE_PLATFORM_LINUX
    else {
        const int fd_flags = ::fcntl(handle_, F_GETFD);
        if (fd_flags != -1) {
            ::fcntl(handle_, F_SETFD, fd_flags | FD_CLOEXEC);
        }
    }
#endif

    path_ = move(pth);
    opened_ = true;
    append_mode_ = append;

    if (append && !seek(0, file_pointer::END)) {
        set_last_error();
        return false;
    }

    init_sub_objects();
    adjust_buffer_size();

    return true;
}

bool file::open(const bool append, const file_access access, const file_shared share_mode, const file_creation creation,
                const file_attri attributes) {
    return open(path_, append, access, share_mode, creation, attributes);
}

void file::close() noexcept {
    if (!opened_) {
        return;
    }

    flush_write_buffer();
    reset_sub_objects();

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::CloseHandle(handle_);
#else
    ::close(handle_);
#endif

    handle_ = invalid_handle;
    opened_ = false;
    append_mode_ = false;
    read_buffer_pos_ = 0;
    read_buffer_size_ = 0;
    write_buffer_pos_ = 0;
    buffer_size_ = buffer_size;
}

bool file::flush() noexcept {
    if (!opened_ || handle_ == invalid_handle) {
        return false;
    }
    if (!flush_write_buffer()) {
        return false;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    return ::FlushFileBuffers(handle_) != 0;
#else
    return ::fdatasync(handle_) == 0;
#endif
}

file::size_type file::write(const string& data, const size_type size) {
    if (!opened_ || handle_ == invalid_handle) {
        return 0;
    }
    const size_type real_size = size > data.size() ? data.size() : size;
    return write(data.data(), real_size);
}

file::size_type file::write(const string& data) { return write(data, data.size()); }

file::size_type file::write(const void* data, const size_type size) {
    if (!opened_ || handle_ == invalid_handle || data == nullptr) {
        return 0;
    }
    if (size == 0) {
        return 0;
    }

    if (append_mode_ && !seek(0, file_pointer::END)) {
        set_last_error();
        return 0;
    }

    if (size > buffer_size_ * 4) {
        if (!flush_write_buffer()) {
            return 0;
        }

        size_type total_written = 0;
        const auto* ptr = static_cast<const byte_t*>(data);

        while (total_written < size) {
#ifdef NEFORCE_PLATFORM_WINDOWS
            size_type bytes_written = 0;
            const size_type to_write = min<size_type>(size - total_written, numeric_traits<size_type>::max());

            if (::WriteFile(handle_, ptr + total_written, to_write, &bytes_written, nullptr) == FALSE) {
                set_last_error();
                break;
            }

            total_written += bytes_written;
            if (bytes_written != to_write) {
                break;
            }

#else
            const ssize_t written = ::write(handle_, ptr + total_written, size - total_written);

            if (written == -1) {
                if (errno == EINTR) {
                    continue;
                }
                set_last_error();
                break;
            }

            if (written == 0) {
                break;
            }
            total_written += static_cast<size_type>(written);

#endif
        }
        return total_written;
    }

    const auto* ptr = static_cast<const char*>(data);
    size_type total_written = 0;
    size_type remaining = size;

    while (remaining > 0) {
        const size_type available = buffer_size_ - write_buffer_pos_;
        const size_type to_copy = min(remaining, available);

        copy_n(ptr, static_cast<ptrdiff_t>(to_copy), write_buffer_.begin() + write_buffer_pos_);
        write_buffer_pos_ += to_copy;
        total_written += to_copy;
        ptr += to_copy;
        remaining -= to_copy;

        if (write_buffer_pos_ == buffer_size_ && !flush_write_buffer()) {
            set_last_error();
            break;
        }
    }

    return total_written;
}

file::size_type file::read(string& out, const size_type size) const {
    if (!opened_ || handle_ == invalid_handle) {
        return 0;
    }
    out.clear();
    if (size == 0) {
        return 0;
    }
    out.resize(size);
    return read(out.data(), size);
}

file::size_type file::read(void* buffer, const size_type size) const {
    if (!opened_ || handle_ == invalid_handle || buffer == nullptr) {
        return 0;
    }
    if (size == 0) {
        return 0;
    }

    auto* ptr = static_cast<char*>(buffer);
    size_type total_read = 0;
    size_type remaining = size;

    while (remaining > 0) {
        if (read_buffer_pos_ >= read_buffer_size_) {
            if (!fill_read_buffer() || read_buffer_size_ == 0) {
                break;
            }
        }

        const size_type available_in_buffer = read_buffer_size_ - read_buffer_pos_;
        const size_type to_read = min(remaining, available_in_buffer);

        copy_n(read_buffer_.data() + read_buffer_pos_, static_cast<ptrdiff_t>(to_read), ptr);
        read_buffer_pos_ += to_read;
        ptr += to_read;
        total_read += to_read;
        remaining -= to_read;
    }

    return total_read;
}

file::size_type file::read(string& out) const { return read(out, out.size()); }

string file::read() const {
    if (!opened_ || handle_ == invalid_handle) {
        return {};
    }

    const size_type file_size = size();
    if (file_size == 0) {
        return {};
    }

    const difference_type current_pos = tell();
    if (!seek(0, file_pointer::BEGIN)) {
        return {};
    }

    string content;
    content.resize(file_size);
    const size_type bytes_read = read(content, file_size);

    if (!seek(current_pos, file_pointer::BEGIN)) {
        return {};
    }

    if (bytes_read != file_size) {
        content.resize(bytes_read);
    }
    return content;
}

vector<string> file::read_chunks(const size_type chunk_size) const {
    vector<string> chunks;
    if (!opened_ || handle_ == invalid_handle) {
        return chunks;
    }

    const size_type file_sz = size();
    if (file_sz == 0) {
        return chunks;
    }

    const difference_type original_pos = tell();
    if (original_pos == static_cast<difference_type>(-1)) {
        set_last_error();
        return chunks;
    }

    if (!seek(0, file_pointer::BEGIN)) {
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
#ifdef NEFORCE_PLATFORM_WINDOWS
                size_type bytes_read_now = 0;
                const size_type to_read_now = min<size_type>(to_read - bytes_read, numeric_traits<size_type>::max());

                if (::ReadFile(handle_, data + bytes_read, to_read_now, &bytes_read_now, nullptr) == FALSE) {
                    set_last_error();
                    break;
                }
                bytes_read += bytes_read_now;
                if (bytes_read_now == 0) {
                    break;
                }

#else
                const ssize_t bytes_read_now = ::read(handle_, data + bytes_read, to_read - bytes_read);
                if (bytes_read_now == -1) {
                    if (errno == EINTR) {
                        continue;
                    }
                    set_last_error();
                    break;
                }

                if (bytes_read_now == 0) {
                    break;
                }
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
            chunks.push_back(move(chunk));
        } else {
            break;
        }

        remaining -= chunk.size();
    }

    if (!seek(original_pos, file_pointer::BEGIN)) {
        set_last_error();
    }
    return chunks;
}

bool file::write_chunks(const vector<string>& chunks) {
    if (!opened_ || handle_ == invalid_handle) {
        return false;
    }

    const difference_type original_pos = tell();

    if (append_mode_ && !seek(0, file_pointer::END)) {
        set_last_error();
        return false;
    }
    if (!flush_write_buffer()) {
        set_last_error();
        return false;
    }

    bool success = true;
    size_type total_written = 0;

    for (const auto& chunk: chunks) {
        if (chunk.empty()) {
            continue;
        }

        const char* data = chunk.data();
        size_type remaining = chunk.size();

        while (remaining > 0) {
            size_type bytes_written = 0;
            if (remaining > buffer_size_ * 4) {
                if (!flush_write_buffer()) {
                    success = false;
                    break;
                }

#ifdef NEFORCE_PLATFORM_WINDOWS
                const size_type to_write = min<size_type>(remaining, numeric_traits<size_type>::max());
                size_type written_now = 0;

                if (::WriteFile(handle_, data + (chunk.size() - remaining), to_write, &written_now, nullptr) == FALSE) {
                    set_last_error();
                    success = false;
                    break;
                }
                bytes_written = written_now;

#else
                const ssize_t written_now = ::write(handle_, data + (chunk.size() - remaining), remaining);

                if (written_now == -1) {
                    if (errno == EINTR) {
                        continue;
                    }
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
        if (!success) {
            break;
        }
    }

    if (!flush()) {
        success = false;
    }
    if (!append_mode_ && original_pos >= 0 && !seek(original_pos, file_pointer::BEGIN)) {
        set_last_error();
    }
    return success;
}

vector<file::chunk_info> file::chunks_info(size_type chunk_size) const {
    vector<chunk_info> info;
    if (!opened_ || handle_ == invalid_handle) {
        set_last_error();
        return info;
    }
    if (chunk_size == 0) {
        chunk_size = buffer_size * 16;
    }

    const size_type file_sz = size();
    if (file_sz == 0) {
        return info;
    }

    const size_type num_chunks = (file_sz + chunk_size - 1) / chunk_size;
    info.reserve(num_chunks);

    size_type offset = 0;
    size_type index = 0;

    while (offset < file_sz) {
        chunk_info ci{};
        ci.offset = static_cast<difference_type>(offset);
        ci.index = index;

        const size_type remaining = file_sz - offset;
        ci.size = min(remaining, chunk_size);
        info.push_back(ci);

        if (file_sz - offset < ci.size) {
            break;
        }
        offset += ci.size;
        ++index;
    }

    return info;
}

file::size_type file::read_binary(void* out, const size_type size) const {
    if (!opened_ || handle_ == invalid_handle || out == nullptr) {
        return 0;
    }
    if (size == 0) {
        return 0;
    }

    auto* ptr = static_cast<byte_t*>(out);
    size_type total_read = 0;
    size_type remaining = size;

    while (remaining > 0) {
        if (read_buffer_pos_ >= read_buffer_size_) {
            if (!fill_read_buffer() || read_buffer_size_ == 0) {
                break;
            }
        }

        const size_type available = read_buffer_size_ - read_buffer_pos_;
        const size_type to_read = min(remaining, available);

        memory_copy(ptr, read_buffer_.data() + read_buffer_pos_, to_read);
        read_buffer_pos_ += to_read;
        ptr += to_read;
        total_read += to_read;
        remaining -= to_read;
    }
    return total_read;
}

file::size_type file::read_binary(string& out, const size_type size) const {
    if (!opened_ || handle_ == invalid_handle) {
        return 0;
    }
    if (size == 0) {
        out.clear();
        return 0;
    }
    out.resize(size);
    const size_type total_read = read_binary(out.data(), size);
    if (total_read < size) {
        out.resize(total_read);
    }
    return total_read;
}

file::size_type file::read_binary(string& out) const {
    const size_type s = size();
    out.resize(s);
    return read_binary(out, s);
}

string file::read_binary() const {
    if (!opened_ || handle_ == invalid_handle) {
        return {};
    }

    const size_type sz = size();
    string content;
    content.resize(sz);

    if (sz > 0) {
        const size_type bytes_read = read_binary(content, sz);
        if (bytes_read != sz) {
            content.resize(bytes_read);
        }
    }
    return content;
}

bool file::read_line(string& line) const {
    if (!opened_ || handle_ == invalid_handle) {
        return false;
    }

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
    if (!read_line(line)) {
        return {};
    }
    return line;
}

vector<string> file::read_lines() const {
    vector<string> lines;
    if (!opened_ || handle_ == invalid_handle) {
        return lines;
    }

    const string content = read();
    if (content.empty()) {
        return lines;
    }

    size_t start = 0;
    size_t end = content.find('\n');

    while (end != string::npos) {
        string line = content.substr(start, end - start);
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.emplace_back(move(line));
        start = end + 1;
        end = content.find('\n', start);
    }

    if (start < content.size()) {
        lines.emplace_back(content.view(start));
    }

    return lines;
}

file::size_type file::size() const {
    if (!opened_ || handle_ == invalid_handle) {
        last_error_code_ = errc::bad_file_descriptor;
        return 0;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::LARGE_INTEGER sz{};
    if (::GetFileSizeEx(handle_, &sz) == FALSE) {
        set_last_error();
        return 0;
    }
    return static_cast<size_type>(sz.QuadPart);
#else
    struct ::stat64 st{};
    if (::fstat64(handle_, &st) == -1) {
        set_last_error();
        return 0;
    }
    return static_cast<size_type>(st.st_size);
#endif
}

bool file::size(size_type& out_size) const {
    out_size = 0;

    if (!opened_ || handle_ == invalid_handle) {
        last_error_code_ = errc::bad_file_descriptor;
        return false;
    }

    out_size = size();
    return last_error_code_ == errc::success;
}

uint64_t file::size64() const {
    if (!opened_ || handle_ == invalid_handle) {
        last_error_code_ = errc::bad_file_descriptor;
        return 0;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::LARGE_INTEGER sz{};
    if (::GetFileSizeEx(handle_, &sz) == FALSE) {
        return 0;
    }
    return static_cast<uint64_t>(sz.QuadPart);
#else
    struct ::stat64 st{};
    if (::fstat64(handle_, &st) == -1) {
        return 0;
    }
    return static_cast<uint64_t>(st.st_size);
#endif
}

void file::clear_error() noexcept { last_error_code_.clear(); }

bool file::seek(const difference_type distance, file_pointer method) const {
    if (!opened_ || handle_ == invalid_handle) {
        return false;
    }

    if (append_mode_) {
        if (method != file_pointer::END || distance != 0) {
            last_error_code_ = errc::operation_not_permitted;
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

    if (map_ && map_->is_mapped()) {
        last_error_code_ = errc::operation_not_permitted;
        return false;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::LARGE_INTEGER li{};
    li.QuadPart = distance;

    ::LARGE_INTEGER new_pointer{};
    if (::SetFilePointerEx(handle_, li, &new_pointer, static_cast<fud_t>(method)) == FALSE) {
        set_last_error();
        return false;
    }
#else
    const difference_type new_pos = ::lseek(handle_, distance, static_cast<fud_t>(method));
    if (new_pos == -1) {
        set_last_error();
        return false;
    }
#endif
    return true;
}

file::difference_type file::tell() const {
    if (!opened_ || handle_ == invalid_handle) {
        last_error_code_ = errc::bad_file_descriptor;
        return -1;
    }

    difference_type system_pos = 0;

#ifdef NEFORCE_PLATFORM_WINDOWS
    constexpr ::LARGE_INTEGER li_zero{};
    ::LARGE_INTEGER current_pos;
    if (::SetFilePointerEx(handle_, li_zero, &current_pos, FILE_CURRENT) == FALSE) {
        set_last_error();
        return -1;
    }
    system_pos = current_pos.QuadPart;

#else
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

file::difference_type file::system_tell() const {
    if (!opened_ || handle_ == invalid_handle) {
        return -1;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    constexpr ::LARGE_INTEGER li_zero{};
    ::LARGE_INTEGER current_pos;
    if (::SetFilePointerEx(handle_, li_zero, &current_pos, FILE_CURRENT) == FALSE) {
        return -1;
    }
    return current_pos.QuadPart;
#else
    const difference_type pos = ::lseek(handle_, 0, SEEK_CUR);
    if (pos == -1) {
        set_last_error();
        return -1;
    }
    return pos;
#endif
}

bool file::prefetch(const size_type hint_size) const {
    if (!opened_ || handle_ == invalid_handle) {
        last_error_code_ = errc::bad_file_descriptor;
        return false;
    }
    if (read_buffer_pos_ < read_buffer_size_) {
        return true;
    }

    size_type prefetch_size = buffer_size_;
    if (hint_size > 0) {
        if (hint_size > numeric_traits<size_type>::max() / 2) {
            prefetch_size = numeric_traits<size_type>::max();
        } else {
            prefetch_size = min(hint_size * 2, buffer_size_);
        }
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    const difference_type current_pos = tell();
    if (current_pos < 0) {
        return false;
    }
    ::LARGE_INTEGER file_size;
    if (::GetFileSizeEx(handle_, &file_size) == FALSE) {
        set_last_error();
        return false;
    }

    const ::ULARGE_INTEGER start_offset = {static_cast<size_type>(current_pos & 0xFFFFFFFF),
                                           static_cast<size_type>(current_pos >> 32)};
    const ::ULARGE_INTEGER end_offset = {static_cast<size_type>(file_size.QuadPart & 0xFFFFFFFF),
                                         static_cast<size_type>(file_size.QuadPart >> 32)};

    size_type region_size = prefetch_size;
    if (start_offset.QuadPart + region_size > end_offset.QuadPart) {
        region_size = static_cast<size_type>(end_offset.QuadPart - start_offset.QuadPart);
    }

    if (region_size == 0) {
        return true;
    }

    const ::HANDLE h_mapping = ::CreateFileMappingA(handle_, nullptr, PAGE_READONLY, 0, 0, nullptr);

    if (h_mapping == nullptr || h_mapping == invalid_handle) {
        set_last_error();
        return false;
    }

    void* p_view = ::MapViewOfFile(h_mapping, FILE_MAP_READ, start_offset.HighPart, start_offset.LowPart, region_size);

    if (p_view != nullptr) {
        ::WIN32_MEMORY_RANGE_ENTRY range{p_view, region_size};
        const ::HMODULE h_kernel32 = ::GetModuleHandleA("kernel32.dll");
        if (h_kernel32 != nullptr) {
            using PFN_PrefetchVirtualMemory =
                    ::BOOL(__stdcall*)(::HANDLE h_process, ::ULONG_PTR number_of_entries,
                                       ::PWIN32_MEMORY_RANGE_ENTRY virtual_addresses, ::ULONG flags);

            static auto pfn_prefetch_virtual_memory =
                    reinterpret_cast<PFN_PrefetchVirtualMemory>(::GetProcAddress(h_kernel32, "PrefetchVirtualMemory"));
            if (pfn_prefetch_virtual_memory != nullptr) {
                if (pfn_prefetch_virtual_memory(::GetCurrentProcess(), 1, &range, 0) == TRUE) {
                    ::UnmapViewOfFile(p_view);
                    ::CloseHandle(h_mapping);
                    return true;
                }
            }
        }
    } else {
        set_last_error();
    }

    ::CloseHandle(h_mapping);
    return p_view != nullptr;

#else
    const difference_type current_pos = tell();
    if (current_pos >= 0) {
        const int advice_result =
                ::posix_fadvise(handle_, current_pos, static_cast<difference_type>(prefetch_size), POSIX_FADV_WILLNEED);

        if (advice_result != 0) {
            last_error_code_ = static_cast<errc>(advice_result);
        }
    }
    return fill_read_buffer();
#endif
}

bool file::truncate(const difference_type size) const {
    if (!opened_ || handle_ == invalid_handle) {
        last_error_code_ = errc::bad_file_descriptor;
        return false;
    }

    if (size < 0) {
        last_error_code_ = errc::invalid_argument;
        return false;
    }

    if (append_mode_) {
        last_error_code_ = errc::operation_not_permitted;
        return false;
    }

    bool buffers_cleared = true;

    if (write_buffer_pos_ > 0) {
        const difference_type current_pos = tell();
        const difference_type buffer_end_pos = current_pos + static_cast<difference_type>(write_buffer_pos_);
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

    if (map_ && map_->is_mapped()) {
        last_error_code_ = errc::operation_not_permitted;
        return false;
    }

    if (!buffers_cleared) {
        return false;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    const difference_type current_pos = tell();
    if (current_pos < 0) {
        return false;
    }

    if (!seek(size, file_pointer::BEGIN)) {
        return false;
    }

    if (::SetEndOfFile(handle_) == FALSE) {
        set_last_error();
        ignore = seek(current_pos, file_pointer::BEGIN);
        return false;
    }

    if (size < current_pos) {
        if (!seek(size, file_pointer::BEGIN)) {
            set_last_error();
        }
    } else {
        ignore = seek(current_pos, file_pointer::BEGIN);
    }

    return true;

#else
    const difference_type current_pos = tell();
    if (current_pos < 0) {
        return false;
    }
    if (::ftruncate(handle_, size) != 0) {
        set_last_error();
        return false;
    }

    if (size < current_pos) {
        if (!seek(size, file_pointer::BEGIN)) {
            set_last_error();
        }
    }
    return true;
#endif
}

NEFORCE_END_NAMESPACE__
