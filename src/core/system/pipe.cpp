#include <NeForce/core/system/pipe.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <NeForce/core/string/utf.hpp>
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

namespace {
    constexpr pipe::native_handle_type g_invalid_handle =
#ifdef NEFORCE_PLATFORM_WINDOWS
            nullptr;
#else
            -1;
#endif

    const named_pipe::native_handle_type g_named_invalid_handle =
#ifdef NEFORCE_PLATFORM_WINDOWS
            INVALID_HANDLE_VALUE;
#else
            -1;
#endif
} // namespace


pipe::pipe() noexcept :
handles_(g_invalid_handle, g_invalid_handle) {}

pipe::pipe(const bool inheritable, const bool nonblocking) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = inheritable ? TRUE : FALSE;
    sa.lpSecurityDescriptor = nullptr;

    if (::CreatePipe(&handles_[0], &handles_[1], &sa, 0) == FALSE) {
        NEFORCE_THROW_EXCEPTION(pipe_exception("CreatePipe failed"));
    }

    if (!inheritable) {
        if (::SetHandleInformation(handles_[0], HANDLE_FLAG_INHERIT, 0) == FALSE) {
            ::CloseHandle(handles_[0]);
            ::CloseHandle(handles_[1]);
            NEFORCE_THROW_EXCEPTION(pipe_exception("SetHandleInformation failed"));
        }
    }

    // Windows anonymous pipes don't support non-blocking mode natively.
    // We store the flag and handle it in read() via PeekNamedPipe.
    if (nonblocking) {
        nonblocking_ = true;
    }
#else
    if (::pipe(handles_) == -1) {
        const auto error = last_error();
        NEFORCE_THROW_EXCEPTION(pipe_exception(error));
    }

    if (!inheritable) {
        ::fcntl(handles_[0], F_SETFD, FD_CLOEXEC);
        ::fcntl(handles_[1], F_SETFD, FD_CLOEXEC);
    }

    if (nonblocking) {
        const int flags0 = ::fcntl(handles_[0], F_GETFL, 0);
        ::fcntl(handles_[0], F_SETFL, flags0 | O_NONBLOCK);
        const int flags1 = ::fcntl(handles_[1], F_GETFL, 0);
        ::fcntl(handles_[1], F_SETFL, flags1 | O_NONBLOCK);
        nonblocking_ = true;
    }
#endif
}

pipe::~pipe() { close(); }

pipe::pipe(pipe&& other) noexcept :
handles_{other.handles_[0], other.handles_[1]},
nonblocking_(other.nonblocking_) {
    other.handles_[0] = g_invalid_handle;
    other.handles_[1] = g_invalid_handle;
}

pipe& pipe::operator=(pipe&& other) noexcept {
    if (addressof(other) == this) {
        return *this;
    }

    close();
    handles_[0] = other.handles_[0];
    handles_[1] = other.handles_[1];
    other.handles_[0] = g_invalid_handle;
    other.handles_[1] = g_invalid_handle;
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
    if (handles_[0] == g_invalid_handle) {
        return -1;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (nonblocking_) {
        ::DWORD bytes_available = 0;
        if (::PeekNamedPipe(handles_[0], nullptr, 0, nullptr, &bytes_available, nullptr) == FALSE) {
            return -1;
        }
        if (bytes_available == 0) {
            return 0;
        }
    }

    ::DWORD result = 0;
    if (::ReadFile(handles_[0], buffer, static_cast<::DWORD>(size), &result, nullptr) == FALSE) {
        return -1;
    }
#else
    const ssize_t result = ::read(handles_[0], buffer, size);
    if (result < 0) {
        if (nonblocking_ && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return 0;
        }
        return -1;
    }
#endif

    return static_cast<int>(result);
}

string pipe::read_available() {
    string output;
    if (handles_[0] == g_invalid_handle) {
        return output;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    constexpr ::DWORD buffer_size = MEMORY_BIG_ALLOC_THRESHHOLD;
    char buffer[buffer_size];
    ::DWORD bytes_read = 0;

    while (true) {
        ::DWORD bytes_available = 0;
        if (::PeekNamedPipe(handles_[0], nullptr, 0, nullptr, &bytes_available, nullptr) == FALSE) {
            break;
        }
        if (bytes_available == 0) {
            break;
        }

        if (::ReadFile(handles_[0], buffer, buffer_size - 1, &bytes_read, nullptr) == FALSE) {
            break;
        }

        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            output.append(buffer, bytes_read);
        }
    }
#else

    char buffer[MEMORY_BIG_ALLOC_THRESHHOLD];
    const int flags = ::fcntl(handles_[0], F_GETFL, 0);
    ::fcntl(handles_[0], F_SETFL, flags | O_NONBLOCK);

    while (true) {
        const ssize_t bytes = ::read(handles_[0], buffer, sizeof(buffer) - 1);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            output.append(buffer, bytes);
        } else {
            break;
        }
    }

    ::fcntl(handles_[0], F_SETFL, flags);
#endif

    return output;
}

bool pipe::set_nonblocking(bool v) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (!is_valid()) {
        return false;
    }
#else
    if (handles_[0] >= 0) {
        const int flags = ::fcntl(handles_[0], F_GETFL, 0);
        if (flags < 0) {
            return false;
        }
        if (::fcntl(handles_[0], F_SETFL, v ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK)) < 0) {
            return false;
        }
    }
    if (handles_[1] >= 0) {
        const int flags = ::fcntl(handles_[1], F_GETFL, 0);
        if (flags < 0) {
            return false;
        }
        if (::fcntl(handles_[1], F_SETFL, v ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK)) < 0) {
            return false;
        }
    }
#endif

    nonblocking_ = v;
    return true;
}

int pipe::write(const void* data, size_t size) noexcept {
    if (handles_[1] == g_invalid_handle) {
        return -1;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::DWORD result = 0;
    if (::WriteFile(handles_[1], data, static_cast<::DWORD>(size), &result, nullptr) == FALSE) {
        return -1;
    }
#else
    const ssize_t result = ::write(handles_[1], data, size);
    if (result < 0) {
        if (nonblocking_ && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return 0;
        }
        return -1;
    }
#endif

    return static_cast<int>(result);
}

void pipe::close_read() noexcept {
    if (handles_[0] != g_invalid_handle) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        ::CloseHandle(handles_[0]);
#else
        ::close(handles_[0]);
#endif
        handles_[0] = g_invalid_handle;
    }
}

void pipe::close_write() noexcept {
    if (handles_[1] != g_invalid_handle) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        ::CloseHandle(handles_[1]);
#else
        ::close(handles_[1]);
#endif
        handles_[1] = g_invalid_handle;
    }
}

void pipe::close() noexcept {
    close_read();
    close_write();
}

bool pipe::is_valid() const noexcept { return handles_[0] != g_invalid_handle || handles_[1] != g_invalid_handle; }

pipe::native_handle_type pipe::detach_read_handle() noexcept {
    const native_handle_type handle = handles_[0];
    handles_[0] = g_invalid_handle;
    return handle;
}

pipe::native_handle_type pipe::detach_write_handle() noexcept {
    const native_handle_type handle = handles_[1];
    handles_[1] = g_invalid_handle;
    return handle;
}

struct named_pipe::impl {
    native_handle_type handle_ = g_named_invalid_handle;
    string name_;
    bool nonblocking_ = false;
#ifdef NEFORCE_PLATFORM_WINDOWS
    bool is_server_ = false;
#else
    string fifo_path_;
#endif
};

// NOLINTNEXTLINE(hicpp-use-equals-default,modernize-use-equals-default)
named_pipe::named_pipe() :
impl_(make_unique<impl>()) {}

named_pipe::~named_pipe() { close(); }

named_pipe::named_pipe(named_pipe&& other) noexcept = default;

named_pipe& named_pipe::operator=(named_pipe&& other) noexcept {
    if (this != &other) {
        close();
        impl_ = move(other.impl_);
    }
    return *this;
}

bool named_pipe::create(const string& name, const bool nonblocking) {
    if (!impl_) {
        return false;
    }
    if (is_valid()) {
        close();
    }

    impl_->name_ = name;
    impl_->nonblocking_ = nonblocking;

#ifdef NEFORCE_PLATFORM_WINDOWS
    const wstring full_name = LR"(\\.\pipe\)" + character::to_wstring(name.view());
    ::DWORD open_mode = PIPE_ACCESS_DUPLEX;
    if (nonblocking) {
        open_mode |= PIPE_NOWAIT;
    }

    impl_->handle_ = ::CreateNamedPipeW(full_name.data(), open_mode, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                                        PIPE_UNLIMITED_INSTANCES, 4096, 4096, 0, nullptr);
    if (impl_->handle_ == g_named_invalid_handle) {
        NEFORCE_THROW_EXCEPTION(pipe_exception("CreateNamedPipe failed"));
    }
    impl_->is_server_ = true;
    return true;
#else
    impl_->fifo_path_ = name;
    if (::mkfifo(name.data(), 0666) != 0) {
        if (errno != EEXIST) {
            NEFORCE_THROW_EXCEPTION(pipe_exception("mkfifo failed"));
        }
    }

    int flags = O_RDWR;
    if (nonblocking) {
        flags |= O_NONBLOCK;
    }
    impl_->handle_ = ::open(name.data(), flags);
    if (impl_->handle_ == g_named_invalid_handle) {
        NEFORCE_THROW_EXCEPTION(pipe_exception("open fifo failed"));
    }
    return true;
#endif
}

bool named_pipe::connect(const string& name, const int timeout_ms) {
    if (!impl_) {
        return false;
    }
    if (is_valid()) {
        close();
    }

    impl_->name_ = name;

#ifdef NEFORCE_PLATFORM_WINDOWS
    const wstring full_name = LR"(\\.\pipe\)" + character::to_wstring(name.view());
    const ::DWORD dw_timeout = (timeout_ms < 0) ? NMPWAIT_WAIT_FOREVER : static_cast<::DWORD>(timeout_ms);

    if (::WaitNamedPipeW(full_name.data(), dw_timeout) == FALSE) {
        NEFORCE_THROW_EXCEPTION(pipe_exception("WaitNamedPipe failed"));
    }

    impl_->handle_ = ::CreateFileW(full_name.data(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL, nullptr);
    if (impl_->handle_ == g_named_invalid_handle) {
        NEFORCE_THROW_EXCEPTION(pipe_exception("Connect to named pipe failed"));
    }
    impl_->is_server_ = false;
    return true;
#else
    int flags = O_RDWR;
    if (impl_->nonblocking_) {
        flags |= O_NONBLOCK;
    }
    impl_->handle_ = ::open(name.data(), flags);
    if (impl_->handle_ == g_named_invalid_handle) {
        NEFORCE_THROW_EXCEPTION(pipe_exception("connect fifo failed"));
    }
    return true;
#endif
}

bool named_pipe::wait_for_client() {
    if (!impl_) {
        return false;
    }
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (impl_->handle_ == g_named_invalid_handle || !impl_->is_server_) {
        return false;
    }
    if (::ConnectNamedPipe(impl_->handle_, nullptr) == FALSE) {
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
    if (impl_->handle_ == g_named_invalid_handle) {
        return -1;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::DWORD result = 0;
    if (::ReadFile(impl_->handle_, buffer, static_cast<::DWORD>(size), &result, nullptr) == FALSE) {
        if (impl_->nonblocking_ && ::GetLastError() == ERROR_NO_DATA) {
            return 0;
        }
        return -1;
    }
#else
    const ssize_t result = ::read(impl_->handle_, buffer, size);
    if (result < 0) {
        if (impl_->nonblocking_ && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return 0;
        }
        return -1;
    }
#endif

    return static_cast<int>(result);
}

int named_pipe::write(const void* data, size_t size) noexcept {
    if (impl_->handle_ == g_named_invalid_handle) {
        return -1;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::DWORD result = 0;
    if (::WriteFile(impl_->handle_, data, static_cast<::DWORD>(size), &result, nullptr) == FALSE) {
        return -1;
    }
#else
    const ssize_t result = ::write(impl_->handle_, data, size);
    if (result < 0) {
        if (impl_->nonblocking_ && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return 0;
        }
        return -1;
    }
#endif

    return static_cast<int>(result);
}

void named_pipe::close() noexcept {
    if (impl_ == nullptr) {
        return;
    }
    if (impl_->handle_ != g_named_invalid_handle) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        ::DisconnectNamedPipe(impl_->handle_);
        ::CloseHandle(impl_->handle_);
#else
        ::close(impl_->handle_);
#endif
        impl_->handle_ = g_named_invalid_handle;
    }
    impl_->name_.clear();
}

bool named_pipe::is_valid() const noexcept { return impl_ && impl_->handle_ != g_named_invalid_handle; }

bool named_pipe::is_nonblocking() const noexcept { return impl_ && impl_->nonblocking_; }

string_view named_pipe::name() const noexcept { return impl_ ? impl_->name_.view() : ""; }

named_pipe::native_handle_type named_pipe::native_handle() const noexcept {
    return impl_ ? impl_->handle_ : g_named_invalid_handle;
}

bool named_pipe::set_nonblocking(bool v) noexcept {
    if (impl_->handle_ == g_named_invalid_handle) {
        return false;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::DWORD mode = PIPE_READMODE_BYTE | (v ? PIPE_NOWAIT : PIPE_WAIT);
    if (::SetNamedPipeHandleState(impl_->handle_, &mode, nullptr, nullptr) == FALSE) {
        return false;
    }
#else
    const int flags = ::fcntl(impl_->handle_, F_GETFL, 0);
    if (flags < 0) {
        return false;
    }
    if (::fcntl(impl_->handle_, F_SETFL, v ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK)) < 0) {
        return false;
    }
#endif

    impl_->nonblocking_ = v;
    return true;
}

bool named_pipe::remove(const string& path) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ignore = path;
    return true;
#else
    return ::unlink(path.data()) == 0;
#endif
}

NEFORCE_END_NAMESPACE__
