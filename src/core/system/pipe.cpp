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
#    include <cerrno>
#    include <cstring>
#    include <fcntl.h>
#    include <unistd.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

pipe::pipe(bool inheritable) {
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
#else
    if (::pipe(fds_) == -1) {
        char errbuf[256];
        char* msg = ::strerror_r(errno, errbuf, sizeof(errbuf));
        NEFORCE_THROW_EXCEPTION(pipe_exception(msg));
    }

    if (!inheritable) {
        ::fcntl(fds_[0], F_SETFD, FD_CLOEXEC);
        ::fcntl(fds_[1], F_SETFD, FD_CLOEXEC);
    }
#endif
}

pipe::~pipe() { close(); }

pipe::pipe(pipe&& other) noexcept
#ifdef NEFORCE_PLATFORM_WINDOWS
:
read_handle_(other.read_handle_),
write_handle_(other.write_handle_) {
    other.read_handle_ = nullptr;
    other.write_handle_ = nullptr;
}
#else
:
fds_{other.fds_[0], other.fds_[1]} {
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

    return *this;
}

int pipe::read(void* buffer, size_t size) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (read_handle_ == nullptr) {
        return -1;
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
    native_handle_type handle = read_handle_;
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
    native_handle_type handle = write_handle_;
    write_handle_ = nullptr;
    return handle;
#else
    const int fd = fds_[1];
    fds_[1] = -1;
    return fd;
#endif
}

NEFORCE_END_NAMESPACE__
