#include <NeForce/core/system/pipe.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <NeForce/core/config/windef.hpp>
#    include <windef.h>
#    include <WinBase.h>
#    ifdef max
#        undef max
#    endif
#    ifdef min
#        undef min
#    endif
#else
#    include <NeForce/core/exception/error_code.hpp>
#    include <csignal>
#    include <fcntl.h>
#    include <sys/stat.h>
#    include <unistd.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

pipe::pipe(bool inheritable, bool nonblocking) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = inheritable ? TRUE : FALSE;
    sa.lpSecurityDescriptor = nullptr;

    if (::CreatePipe(&read_handle_, &write_handle_, &sa, 0) == FALSE) {
        NEFORCE_THROW_EXCEPTION(pipe_exception("CreatePipe failed"));
    }

    if (!inheritable) {
        if (::SetHandleInformation(read_handle_, HANDLE_FLAG_INHERIT, 0) == FALSE) {
            ::CloseHandle(read_handle_);
            ::CloseHandle(write_handle_);
            NEFORCE_THROW_EXCEPTION(pipe_exception("SetHandleInformation failed"));
        }
    }

    // Windows anonymous pipes don't support non-blocking mode natively.
    // We store the flag and handle it in read() via PeekNamedPipe.
    if (nonblocking) {
        nonblocking_ = true;
    }
#else
    if (::pipe(fds_) == -1) {
        const auto error = last_error();
        NEFORCE_THROW_EXCEPTION(pipe_exception(error.message().data()));
    }

    if (!inheritable) {
        ::fcntl(fds_[0], F_SETFD, FD_CLOEXEC);
        ::fcntl(fds_[1], F_SETFD, FD_CLOEXEC);
    }

    if (nonblocking) {
        int flags0 = ::fcntl(fds_[0], F_GETFL, 0);
        ::fcntl(fds_[0], F_SETFL, flags0 | O_NONBLOCK);
        int flags1 = ::fcntl(fds_[1], F_GETFL, 0);
        ::fcntl(fds_[1], F_SETFL, flags1 | O_NONBLOCK);
        nonblocking_ = true;
    }
#endif
}

pipe::~pipe() { close(); }

pipe::pipe(pipe&& other) noexcept
#ifdef NEFORCE_PLATFORM_WINDOWS
:
read_handle_(other.read_handle_),
write_handle_(other.write_handle_),
nonblocking_(other.nonblocking_) {
    other.read_handle_ = nullptr;
    other.write_handle_ = nullptr;
}
#else
:
fds_{other.fds_[0], other.fds_[1]},
nonblocking_(other.nonblocking_) {
    other.fds_[0] = -1;
    other.fds_[1] = -1;
}
#endif

pipe& pipe::operator=(pipe&& other) noexcept {
    if (addressof(other) == this) {
        return *this;
    }

    close();
#ifdef NEFORCE_PLATFORM_WINDOWS
    read_handle_ = other.read_handle_;
    write_handle_ = other.write_handle_;
    other.read_handle_ = nullptr;
    other.write_handle_ = nullptr;
#else
    fds_[0] = other.fds_[0];
    fds_[1] = other.fds_[1];
    other.fds_[0] = -1;
    other.fds_[1] = -1;
#endif
    nonblocking_ = other.nonblocking_;

    return *this;
}

void pipe::ignore_sigpipe() noexcept {
#ifdef NEFORCE_PLATFORM_LINUX
    ::signal(SIGPIPE, SIG_IGN);
#endif
}

int pipe::read(void* buffer, size_t size) noexcept {
    if (size == 0) {
        return 0;
    }
    if (buffer == nullptr) {
        return -1;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (read_handle_ == nullptr) {
        return -1;
    }

    if (nonblocking_) {
        ::DWORD bytes_available = 0;
        if (::PeekNamedPipe(read_handle_, nullptr, 0, nullptr, &bytes_available, nullptr) == FALSE) {
            return -1;
        }
        if (bytes_available == 0) {
            return 0;
        }
    }

    ::DWORD bytes_read = 0;
    if (::ReadFile(read_handle_, buffer, static_cast<::DWORD>(size), &bytes_read, nullptr) == FALSE) {
        return -1;
    }
    return static_cast<int>(bytes_read);
#else
    if (fds_[0] < 0) {
        return -1;
    }

    const ssize_t result = ::read(fds_[0], buffer, size);
    if (result < 0) {
        if (nonblocking_ && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return 0;
        }
        return -1;
    }
    return static_cast<int>(result);
#endif
}

string pipe::read_available() {
    string output;

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (read_handle_ == nullptr) {
        return output;
    }

    constexpr ::DWORD buffer_size = MEMORY_BIG_ALLOC_THRESHHOLD;
    char buffer[buffer_size];
    ::DWORD bytes_read = 0;

    while (true) {
        ::DWORD bytes_available = 0;
        if (::PeekNamedPipe(read_handle_, nullptr, 0, nullptr, &bytes_available, nullptr) == FALSE) {
            break;
        }
        if (bytes_available == 0) {
            break;
        }

        if (::ReadFile(read_handle_, buffer, buffer_size - 1, &bytes_read, nullptr) == FALSE) {
            break;
        }

        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            output.append(buffer, bytes_read);
        }
    }
#else
    if (fds_[0] < 0) {
        return output;
    }

    char buffer[MEMORY_BIG_ALLOC_THRESHHOLD];
    const int flags = ::fcntl(fds_[0], F_GETFL, 0);
    ::fcntl(fds_[0], F_SETFL, flags | O_NONBLOCK);

    while (true) {
        const ssize_t bytes = ::read(fds_[0], buffer, sizeof(buffer) - 1);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            output.append(buffer, bytes);
        } else {
            break;
        }
    }

    ::fcntl(fds_[0], F_SETFL, flags);
#endif

    return output;
}

bool pipe::set_nonblocking(bool v) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (!is_valid()) {
        return false;
    }
    nonblocking_ = v;
    return true;
#else
    if (fds_[0] >= 0) {
        int flags = ::fcntl(fds_[0], F_GETFL, 0);
        if (flags < 0) {
            return false;
        }
        if (::fcntl(fds_[0], F_SETFL, v ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK)) < 0) {
            return false;
        }
    }
    if (fds_[1] >= 0) {
        int flags = ::fcntl(fds_[1], F_GETFL, 0);
        if (flags < 0) {
            return false;
        }
        if (::fcntl(fds_[1], F_SETFL, v ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK)) < 0) {
            return false;
        }
    }
    nonblocking_ = v;
    return true;
#endif
}

int pipe::write(const void* data, size_t size) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (write_handle_ == nullptr) {
        return -1;
    }

    ::DWORD bytes_written = 0;
    if (::WriteFile(write_handle_, data, static_cast<::DWORD>(size), &bytes_written, nullptr) == FALSE) {
        return -1;
    }
    return static_cast<int>(bytes_written);
#else
    if (fds_[1] < 0) {
        return -1;
    }
    const ssize_t result = ::write(fds_[1], data, size);
    if (result < 0) {
        if (nonblocking_ && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return 0;
        }
        return -1;
    }
    return static_cast<int>(result);
#endif
}

void pipe::close_read() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (read_handle_ != nullptr) {
        ::CloseHandle(read_handle_);
        read_handle_ = nullptr;
    }
#else
    if (fds_[0] >= 0) {
        ::close(fds_[0]);
        fds_[0] = -1;
    }
#endif
}

void pipe::close_write() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (write_handle_ != nullptr) {
        ::CloseHandle(write_handle_);
        write_handle_ = nullptr;
    }
#else
    if (fds_[1] >= 0) {
        ::close(fds_[1]);
        fds_[1] = -1;
    }
#endif
}

void pipe::close() noexcept {
    close_read();
    close_write();
}

bool pipe::is_valid() const noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return read_handle_ != nullptr || write_handle_ != nullptr;
#else
    return fds_[0] >= 0 || fds_[1] >= 0;
#endif
}

pipe::native_handle_type pipe::detach_read_handle() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const native_handle_type handle = read_handle_;
    read_handle_ = nullptr;
    return handle;
#else
    const int fd = fds_[0];
    fds_[0] = -1;
    return fd;
#endif
}

pipe::native_handle_type pipe::detach_write_handle() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const native_handle_type handle = write_handle_;
    write_handle_ = nullptr;
    return handle;
#else
    const int fd = fds_[1];
    fds_[1] = -1;
    return fd;
#endif
}

named_pipe::named_pipe()
#ifdef NEFORCE_PLATFORM_WINDOWS
:
pipe_handle_(INVALID_HANDLE_VALUE)
#endif
{
}

named_pipe::~named_pipe() { close(); }

named_pipe::named_pipe(named_pipe&& other) noexcept :
#ifdef NEFORCE_PLATFORM_WINDOWS
pipe_handle_(other.pipe_handle_),
is_server_(other.is_server_),
#else
fd_(other.fd_),
fifo_path_(move(other.fifo_path_)),
#endif
name_(move(other.name_)),
nonblocking_(other.nonblocking_) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    other.pipe_handle_ = INVALID_HANDLE_VALUE;
    other.is_server_ = false;
#else
    other.fd_ = -1;
#endif
}

named_pipe& named_pipe::operator=(named_pipe&& other) noexcept {
    if (addressof(other) == this) {
        return *this;
    }
    close();
#ifdef NEFORCE_PLATFORM_WINDOWS
    pipe_handle_ = other.pipe_handle_;
    is_server_ = other.is_server_;
    other.pipe_handle_ = INVALID_HANDLE_VALUE;
    other.is_server_ = false;
#else
    fd_ = other.fd_;
    fifo_path_ = move(other.fifo_path_);
    other.fd_ = -1;
#endif
    name_ = move(other.name_);
    nonblocking_ = other.nonblocking_;
    return *this;
}

bool named_pipe::create(const string& name, bool nonblocking) {
    if (is_valid()) {
        close();
    }

    name_ = name;
    nonblocking_ = nonblocking;

#ifdef NEFORCE_PLATFORM_WINDOWS
    const string full_name = R"(\\.\pipe\)" + name;
    ::DWORD open_mode = PIPE_ACCESS_DUPLEX;
    if (nonblocking) {
        open_mode |= PIPE_NOWAIT;
    }

    pipe_handle_ = ::CreateNamedPipeA(full_name.data(), open_mode, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                                      PIPE_UNLIMITED_INSTANCES, 4096, 4096, 0, nullptr);
    if (pipe_handle_ == INVALID_HANDLE_VALUE) {
        NEFORCE_THROW_EXCEPTION(pipe_exception("CreateNamedPipe failed"));
    }
    is_server_ = true;
    return true;
#else
    fifo_path_ = name;
    if (::mkfifo(name.data(), 0666) != 0) {
        if (errno != EEXIST) {
            NEFORCE_THROW_EXCEPTION(pipe_exception("mkfifo failed"));
        }
    }

    int flags = O_RDWR;
    if (nonblocking) {
        flags |= O_NONBLOCK;
    }
    fd_ = ::open(name.data(), flags);
    if (fd_ < 0) {
        NEFORCE_THROW_EXCEPTION(pipe_exception("open fifo failed"));
    }
    return true;
#endif
}

bool named_pipe::connect(const string& name, const int timeout_ms) {
    if (is_valid()) {
        close();
    }

    name_ = name;

#ifdef NEFORCE_PLATFORM_WINDOWS
    const string full_name = R"(\\.\pipe\)" + name;
    const ::DWORD dw_timeout = (timeout_ms < 0) ? NMPWAIT_WAIT_FOREVER : static_cast<::DWORD>(timeout_ms);

    if (::WaitNamedPipeA(full_name.data(), dw_timeout) == FALSE) {
        NEFORCE_THROW_EXCEPTION(pipe_exception("WaitNamedPipe failed"));
    }

    pipe_handle_ = ::CreateFileA(full_name.data(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL, nullptr);
    if (pipe_handle_ == INVALID_HANDLE_VALUE) {
        NEFORCE_THROW_EXCEPTION(pipe_exception("Connect to named pipe failed"));
    }
    is_server_ = false;
    return true;
#else
    int flags = O_RDWR;
    if (nonblocking_) {
        flags |= O_NONBLOCK;
    }
    fd_ = ::open(name.data(), flags);
    if (fd_ < 0) {
        NEFORCE_THROW_EXCEPTION(pipe_exception("connect fifo failed"));
    }
    return true;
#endif
}

bool named_pipe::wait_for_client() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (pipe_handle_ == INVALID_HANDLE_VALUE || !is_server_) {
        return false;
    }
    if (::ConnectNamedPipe(pipe_handle_, nullptr) == FALSE) {
        if (::GetLastError() != ERROR_PIPE_CONNECTED) {
            return false;
        }
    }
    return true;
#else
    // Linux FIFO: open() blocks until client connects, already handled in create()
    return is_valid();
#endif
}

int named_pipe::read(void* buffer, size_t size) noexcept {
    if (size == 0) {
        return 0;
    }
    if (buffer == nullptr) {
        return -1;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (pipe_handle_ == INVALID_HANDLE_VALUE) {
        return -1;
    }
    ::DWORD bytes_read = 0;
    if (::ReadFile(pipe_handle_, buffer, static_cast<::DWORD>(size), &bytes_read, nullptr) == FALSE) {
        if (nonblocking_ && ::GetLastError() == ERROR_NO_DATA) {
            return 0;
        }
        return -1;
    }
    return static_cast<int>(bytes_read);
#else
    if (fd_ < 0) {
        return -1;
    }
    const ssize_t result = ::read(fd_, buffer, size);
    if (result < 0) {
        if (nonblocking_ && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return 0;
        }
        return -1;
    }
    return static_cast<int>(result);
#endif
}

int named_pipe::write(const void* data, size_t size) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (pipe_handle_ == INVALID_HANDLE_VALUE) {
        return -1;
    }
    ::DWORD bytes_written = 0;
    if (::WriteFile(pipe_handle_, data, static_cast<::DWORD>(size), &bytes_written, nullptr) == FALSE) {
        return -1;
    }
    return static_cast<int>(bytes_written);
#else
    if (fd_ < 0) {
        return -1;
    }
    const ssize_t result = ::write(fd_, data, size);
    if (result < 0) {
        if (nonblocking_ && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return 0;
        }
        return -1;
    }
    return static_cast<int>(result);
#endif
}

void named_pipe::close() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (pipe_handle_ != INVALID_HANDLE_VALUE) {
        ::DisconnectNamedPipe(pipe_handle_);
        ::CloseHandle(pipe_handle_);
        pipe_handle_ = INVALID_HANDLE_VALUE;
    }
#else
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
#endif
    name_.clear();
}

bool named_pipe::is_valid() const noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return pipe_handle_ != INVALID_HANDLE_VALUE;
#else
    return fd_ >= 0;
#endif
}

bool named_pipe::set_nonblocking(bool v) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (pipe_handle_ == INVALID_HANDLE_VALUE) {
        return false;
    }
    ::DWORD mode = PIPE_READMODE_BYTE | (v ? PIPE_NOWAIT : PIPE_WAIT);
    if (::SetNamedPipeHandleState(pipe_handle_, &mode, nullptr, nullptr) == FALSE) {
        return false;
    }
    nonblocking_ = v;
    return true;
#else
    if (fd_ < 0) {
        return false;
    }
    int flags = ::fcntl(fd_, F_GETFL, 0);
    if (flags < 0) {
        return false;
    }
    if (::fcntl(fd_, F_SETFL, v ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK)) < 0) {
        return false;
    }
    nonblocking_ = v;
    return true;
#endif
}

bool named_pipe::remove(const string& path) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ignore = path;
    return true;
#else
    return ::unlink(path.data()) == 0;
#endif
}

NEFORCE_END_NAMESPACE__
