#include <MSTL/core/system/device.hpp>
#ifdef MSTL_PLATFORM_LINUX__
#include <MSTL/core/file/file.hpp>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#endif
MSTL_BEGIN_NAMESPACE__

string to_string(DEVICE_TYPE type) {
    switch (type) {
        case DEVICE_TYPE::SERIAL_PORT: return "serial_port";
        case DEVICE_TYPE::STORAGE: return "storage";
        case DEVICE_TYPE::HID: return "hid";
        case DEVICE_TYPE::NETWORK: return "network";
        case DEVICE_TYPE::AUDIO: return "audio";
        case DEVICE_TYPE::VIDEO: return "video";
        case DEVICE_TYPE::GENERIC: return "generic";
        default: return "unknown";
    }
}

DEVICE_TYPE to_device_t(const string& str) {
    if (str == "serial_port") return DEVICE_TYPE::SERIAL_PORT;
    if (str == "storage") return DEVICE_TYPE::STORAGE;
    if (str == "hid") return DEVICE_TYPE::HID;
    if (str == "network") return DEVICE_TYPE::NETWORK;
    if (str == "audio") return DEVICE_TYPE::AUDIO;
    if (str == "video") return DEVICE_TYPE::VIDEO;
    if (str == "generic") return DEVICE_TYPE::GENERIC;
    return DEVICE_TYPE::UNKNOWN;
}

device::device() {
    init();
}

device::device(const string& device_path,
    const DEVICE_OPEN_MODE mode, const DEVICE_OPEN_FLAG flags) {
    init();
    open(device_path, mode, flags);
}

device::~device() {
    cleanup();
}

device::device(device&& other) noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    handle_ = other.handle_;
    cancel_event_ = other.cancel_event_;
    overlapped_ = other.overlapped_;
    overlapped_buffer_ = move(other.overlapped_buffer_);
    overlapped_buffer_size_ = other.overlapped_buffer_size_;
    other.handle_ = INVALID_HANDLE_VALUE;
    other.cancel_event_ = nullptr;
    ZeroMemory(&other.overlapped_, sizeof(OVERLAPPED));
    other.overlapped_buffer_size_ = 0;
#else
    fd_ = other.fd_;
    event_fd_ = other.event_fd_;
    is_non_blocking_ = other.is_non_blocking_;
    other.fd_ = -1;
    other.event_fd_ = -1;
    other.is_non_blocking_ = false;
#endif

    device_path_ = move(other.device_path_);
    device_type_ = other.device_type_;
    timeout_ = other.timeout_;
    is_blocking_ = other.is_blocking_;

    if (other.monitoring_) {
        other.stop_event_monitoring();
    }
    monitoring_ = other.monitoring_.exchange(false);
    monitor_thread_ = move(other.monitor_thread_);
    event_callback_ = move(other.event_callback_);
}

device& device::operator=(device&& other) noexcept {
    if (this != &other) {
        cleanup();

#ifdef MSTL_PLATFORM_WINDOWS__
        handle_ = other.handle_;
        cancel_event_ = other.cancel_event_;
        overlapped_ = other.overlapped_;
        overlapped_buffer_ = move(other.overlapped_buffer_);
        overlapped_buffer_size_ = other.overlapped_buffer_size_;
        other.handle_ = INVALID_HANDLE_VALUE;
        other.cancel_event_ = nullptr;
        ZeroMemory(&other.overlapped_, sizeof(OVERLAPPED));
        other.overlapped_buffer_size_ = 0;
#else
        fd_ = other.fd_;
        event_fd_ = other.event_fd_;
        is_non_blocking_ = other.is_non_blocking_;
        other.fd_ = -1;
        other.event_fd_ = -1;
        other.is_non_blocking_ = false;
#endif

        device_path_ = move(other.device_path_);
        device_type_ = other.device_type_;
        timeout_ = other.timeout_;
        is_blocking_ = other.is_blocking_;

        if (other.monitoring_) {
            other.stop_event_monitoring();
        }
        monitoring_ = other.monitoring_.exchange(false);
        monitor_thread_ = move(other.monitor_thread_);
        event_callback_ = move(other.event_callback_);
    }
    return *this;
}

void device::init() {
#ifdef MSTL_PLATFORM_WINDOWS__
    handle_ = INVALID_HANDLE_VALUE;
    cancel_event_ = nullptr;
    ZeroMemory(&overlapped_, sizeof(OVERLAPPED));
    overlapped_.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
#else
    fd_ = -1;
    event_fd_ = -1;
    is_non_blocking_ = false;
#endif
}

void device::cleanup() noexcept {
    stop_event_monitoring();
    close();

#ifdef MSTL_PLATFORM_WINDOWS__
    if (overlapped_.hEvent) {
        CloseHandle(overlapped_.hEvent);
        overlapped_.hEvent = nullptr;
    }
    if (cancel_event_) {
        CloseHandle(cancel_event_);
        cancel_event_ = nullptr;
    }
#else
    if (event_fd_ != -1) {
        ::close(event_fd_);
        event_fd_ = -1;
    }
#endif
}

#ifdef MSTL_PLATFORM_WINDOWS__
void device::setup_overlapped_io() {
    if (!overlapped_.hEvent) {
        overlapped_.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
        if (!overlapped_.hEvent) {
            throw device_exception("Failed to create overlapped event", GetLastError());
        }
    }
}
#else
void device::setup_overlapped_io() {
    set_non_blocking(true);
}
#endif

void device::check_error(const string& operation, bool result) {
    if (!result) {
#ifdef MSTL_PLATFORM_WINDOWS__
        DWORD error = GetLastError();
        throw_exception(device_exception((operation + " failed: " + to_string(error)).data()));
#else
        throw_exception(device_exception((operation + " failed: " + string(::strerror(errno))).data()));
#endif
    }
}

void device::open(const string& device_path,
    const DEVICE_OPEN_MODE mode, const DEVICE_OPEN_FLAG flags) {
    if (is_open()) {
        close();
    }

    device_path_ = device_path;

#ifdef MSTL_PLATFORM_WINDOWS__
    DWORD desired_access = 0;
    switch (mode) {
        case DEVICE_OPEN_MODE::READ:
            desired_access = GENERIC_READ;
            break;
        case DEVICE_OPEN_MODE::WRITE:
            desired_access = GENERIC_WRITE;
            break;
        case DEVICE_OPEN_MODE::READ_WRITE:
            desired_access = GENERIC_READ | GENERIC_WRITE;
            break;
        case DEVICE_OPEN_MODE::NON_BLOCKING:
            desired_access = GENERIC_READ | GENERIC_WRITE;
            break;
    }

    DWORD share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;

    DWORD creation_disposition = OPEN_EXISTING;
    if ((flags & DEVICE_OPEN_FLAG::CREATE) == DEVICE_OPEN_FLAG::CREATE) {
        creation_disposition = OPEN_ALWAYS;
    }

    DWORD flags_and_attributes = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED;

    if ((flags & DEVICE_OPEN_FLAG::EXCLUSIVE) == DEVICE_OPEN_FLAG::EXCLUSIVE) {
        share_mode = 0;
    }

    if ((flags & DEVICE_OPEN_FLAG::NO_INHERIT) == DEVICE_OPEN_FLAG::NO_INHERIT) {
        flags_and_attributes |= FILE_FLAG_NO_BUFFERING;
    }

    if ((flags & DEVICE_OPEN_FLAG::DIRECT_IO) == DEVICE_OPEN_FLAG::DIRECT_IO) {
        flags_and_attributes |= FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH;
    }

    if ((flags & DEVICE_OPEN_FLAG::SYNC) == DEVICE_OPEN_FLAG::SYNC) {
        flags_and_attributes |= FILE_FLAG_WRITE_THROUGH;
    }

    wstring wide_path;
    if (!device_path.empty()) {
        int size = MultiByteToWideChar(CP_UTF8, 0, device_path.c_str(),
                                      -1, nullptr, 0);
        wide_path.resize(size);
        MultiByteToWideChar(CP_UTF8, 0, device_path.c_str(), -1,
                           &wide_path[0], size);
    }

    handle_ = CreateFileW(
        wide_path.c_str(),
        desired_access,
        share_mode,
        nullptr,
        creation_disposition,
        flags_and_attributes,
        nullptr
    );

    if (handle_ == INVALID_HANDLE_VALUE) {
        throw_exception(device_exception("Failed to open device: " + device_path, GetLastError()));
    }

    setup_cancel_event();

#else
    int open_flags = 0;

    switch (mode) {
        case DEVICE_OPEN_MODE::READ:
            open_flags |= O_RDONLY;
            break;
        case DEVICE_OPEN_MODE::WRITE:
            open_flags |= O_WRONLY;
            break;
        case DEVICE_OPEN_MODE::READ_WRITE:
            open_flags |= O_RDWR;
            break;
        case DEVICE_OPEN_MODE::NON_BLOCKING:
            open_flags |= O_RDWR | O_NONBLOCK;
            is_non_blocking_ = true;
            break;
    }

    if ((flags & DEVICE_OPEN_FLAG::EXCLUSIVE) == DEVICE_OPEN_FLAG::EXCLUSIVE) {
        open_flags |= O_EXCL;
    }
    if ((flags & DEVICE_OPEN_FLAG::NO_INHERIT) == DEVICE_OPEN_FLAG::NO_INHERIT) {
        open_flags |= O_CLOEXEC;
    }
    if ((flags & DEVICE_OPEN_FLAG::DIRECT_IO) == DEVICE_OPEN_FLAG::DIRECT_IO) {
        open_flags |= O_DIRECT;
    }
    if ((flags & DEVICE_OPEN_FLAG::SYNC) == DEVICE_OPEN_FLAG::SYNC) {
        open_flags |= O_SYNC;
    }
    if ((flags & DEVICE_OPEN_FLAG::CREATE) == DEVICE_OPEN_FLAG::CREATE) {
        open_flags |= O_CREAT;
    }

    fd_ = ::open(device_path.c_str(), open_flags, 0666);
    if (fd_ == -1) {
        throw_exception(device_exception(("Failed to open device: " + device_path).data()));
    }
    event_fd_ = ::eventfd(0, EFD_NONBLOCK);
    if (event_fd_ == -1) {
        ::close(fd_);
        fd_ = -1;
        throw_exception(device_exception("Failed to create eventfd"));
    }

    if (mode != DEVICE_OPEN_MODE::NON_BLOCKING) {
        set_non_blocking(false);
    }
#endif
    device_type_ = guess_device_type_from_path(device_path);
    set_timeout(timeout_);
}

void device::close() noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    if (handle_ != INVALID_HANDLE_VALUE) {
        CancelIo(handle_);
        CloseHandle(handle_);
        handle_ = INVALID_HANDLE_VALUE;
    }
#else
    if (fd_ != -1) {
        ::close(fd_);
        fd_ = -1;
    }
#endif
    device_path_.clear();
    device_type_ = DEVICE_TYPE::UNKNOWN;
}

bool device::is_open() const noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    return handle_ != INVALID_HANDLE_VALUE;
#else
    return fd_ != -1;
#endif
}

void device::reopen(const DEVICE_OPEN_MODE new_mode, const DEVICE_OPEN_FLAG new_flags) {
    if (!is_open()) {
        throw_exception(device_exception("Device not open"));
    }

    const string path = device_path_;
    close();
    open(path, new_mode, new_flags);
}

size_t device::read(void* buffer, size_t size, chrono::milliseconds timeout) {
    if (!is_open()) {
        throw_exception(device_exception("Device not open"));
    }

#ifdef MSTL_PLATFORM_WINDOWS__
    DWORD bytes_read = 0;
    OVERLAPPED overlapped = {0};
    overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

    if (!overlapped.hEvent) {
        throw_exception(device_exception("Failed to create event", GetLastError()));
    }

    if (timeout.count() >= 0) {
        overlapped.Internal = static_cast<ULONG_PTR>(timeout.count());
    }

    BOOL result = ReadFile(handle_, buffer, static_cast<DWORD>(size),
                          &bytes_read, &overlapped);

    if (!result) {
        DWORD error = GetLastError();
        if (error == ERROR_IO_PENDING) {
            DWORD wait_result = WaitForSingleObject(overlapped.hEvent,
                timeout.count() >= 0 ? static_cast<DWORD>(timeout.count()) : INFINITE);

            if (wait_result == WAIT_TIMEOUT) {
                CancelIo(handle_);
                CloseHandle(overlapped.hEvent);
                throw device_exception("Read timeout");
            }

            if (!GetOverlappedResult(handle_, &overlapped, &bytes_read, FALSE)) {
                error = GetLastError();
                CloseHandle(overlapped.hEvent);
                throw device_exception("GetOverlappedResult failed", error);
            }
        } else {
            CloseHandle(overlapped.hEvent);
            throw device_exception("ReadFile failed", error);
        }
    }

    CloseHandle(overlapped.hEvent);
    return bytes_read;
#else
    if (timeout.count() >= 0) {
        ::pollfd fds[2];
        fds[0].fd = fd_;
        fds[0].events = POLLIN;
        fds[1].fd = event_fd_;
        fds[1].events = POLLIN;

        const int poll_timeout = timeout.count();
        const int result = ::poll(fds, 2, poll_timeout);

        if (result == -1) {
            throw_exception(device_exception("poll failed"));
        }

        if (result == 0) {
            throw_exception(device_exception("Read timeout"));
        }

        if (fds[1].revents & POLLIN) {
            uint64_t value;
            ::read(event_fd_, &value, sizeof(value));
            throw_exception(device_exception("Operation cancelled"));
        }
    }

    const ssize_t bytes_read = ::read(fd_, buffer, size);
    if (bytes_read < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
        throw_exception(device_exception("read failed"));
    }

    return bytes_read;
#endif
}

size_t device::write(const void* buffer, size_t size, chrono::milliseconds timeout) {
    if (!is_open()) {
        throw_exception(device_exception("Device not open"));
    }

#ifdef MSTL_PLATFORM_WINDOWS__
    DWORD bytes_written = 0;
    OVERLAPPED overlapped = {0};
    overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

    if (!overlapped.hEvent) {
        throw device_exception("Failed to create event", GetLastError());
    }

    if (timeout.count() >= 0) {
        overlapped.Internal = static_cast<ULONG_PTR>(timeout.count());
    }

    BOOL result = WriteFile(handle_, buffer, static_cast<DWORD>(size),
                           &bytes_written, &overlapped);

    if (!result) {
        DWORD error = GetLastError();
        if (error == ERROR_IO_PENDING) {
            DWORD wait_result = WaitForSingleObject(overlapped.hEvent,
                timeout.count() >= 0 ? static_cast<DWORD>(timeout.count()) : INFINITE);

            if (wait_result == WAIT_TIMEOUT) {
                CancelIo(handle_);
                CloseHandle(overlapped.hEvent);
                throw device_exception("Write timeout");
            }

            if (!GetOverlappedResult(handle_, &overlapped, &bytes_written, FALSE)) {
                error = GetLastError();
                CloseHandle(overlapped.hEvent);
                throw device_exception("GetOverlappedResult failed", error);
            }
        } else {
            CloseHandle(overlapped.hEvent);
            throw device_exception("WriteFile failed", error);
        }
    }

    CloseHandle(overlapped.hEvent);
    return bytes_written;
#else
    if (timeout.count() >= 0) {
        ::pollfd fds[2];
        fds[0].fd = fd_;
        fds[0].events = POLLOUT;
        fds[1].fd = event_fd_;
        fds[1].events = POLLIN;

        const int poll_timeout = timeout.count();
        const int result = ::poll(fds, 2, poll_timeout);

        if (result == -1) {
            throw_exception(device_exception("poll failed"));
        }

        if (result == 0) {
            throw_exception(device_exception("Write timeout"));
        }

        if (fds[1].revents & POLLIN) {
            uint64_t value;
            ::read(event_fd_, &value, sizeof(value));
            throw_exception(device_exception("Operation cancelled"));
        }
    }

    const ssize_t bytes_written = ::write(fd_, buffer, size);
    if (bytes_written < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
        throw_exception(device_exception("write failed"));
    }

    return bytes_written;
#endif
}

void device::ioctl(const ioctl_command& cmd) {
    if (!is_open()) {
        throw_exception(device_exception("Device not open"));
    }
#ifdef MSTL_PLATFORM_WINDOWS__
    DWORD bytes_returned = 0;
    if (!DeviceIoControl(handle_, cmd.code(),
                        const_cast<void*>(cmd.in_data()),
                        static_cast<DWORD>(cmd.in_size()),
                        cmd.out_data(),
                        static_cast<DWORD>(cmd.out_size()),
                        &bytes_returned, nullptr)) {
        throw device_exception("DeviceIoControl failed", GetLastError());
    }
#else
    if (::ioctl(fd_, cmd.code(), const_cast<void*>(cmd.in_data())) == -1) {
        throw_exception(device_exception("ioctl failed"));
    }
#endif
}

void device::flush() {
    if (!is_open()) {
        throw_exception(device_exception("Device not open"));
    }

#ifdef MSTL_PLATFORM_WINDOWS__
    if (!FlushFileBuffers(handle_)) {
        throw_exception(device_exception("FlushFileBuffers failed", GetLastError()));
    }
#else
    if (::fsync(fd_) == -1) {
        throw_exception(device_exception("fsync failed"));
    }
#endif
}

void device::sync() noexcept {
    if (!is_open()) {
        return;
    }

#ifdef MSTL_PLATFORM_WINDOWS__
    ::FlushFileBuffers(handle_);
#else
    ::fsync(fd_);
#endif
}

bool device::wait(DEVICE_IO_DIRECT direction, const chrono::milliseconds timeout) const {
    if (!is_open()) {
        return false;
    }

#ifdef MSTL_PLATFORM_WINDOWS__
    DWORD wait_mask = 0;
    switch (direction) {
        case DEVICE_IO_DIRECT::IN:
            wait_mask = EV_RXCHAR;
            break;
        case DEVICE_IO_DIRECT::OUT:
            wait_mask = EV_TXEMPTY;
            break;
        case DEVICE_IO_DIRECT::BOTH:
            wait_mask = EV_RXCHAR | EV_TXEMPTY;
            break;
    }

    ::DWORD events = 0;
    if (!::WaitCommEvent(handle_, &events, nullptr)) {
        return false;
    }

    return (events & wait_mask) != 0;
#else
    ::pollfd fds[1];
    fds[0].fd = fd_;

    switch (direction) {
        case DEVICE_IO_DIRECT::IN: {
            fds[0].events = POLLIN;
            break;
        }
        case DEVICE_IO_DIRECT::OUT: {
            fds[0].events = POLLOUT;
            break;
        }
        case DEVICE_IO_DIRECT::BOTH: {
            fds[0].events = POLLIN | POLLOUT;
            break;
        }
    }

    const int poll_timeout = timeout.count() >= 0 ? static_cast<int>(timeout.count()) : -1;
    const int result = ::poll(fds, 1, poll_timeout);

    if (result <= 0) {
        return false;
    }
    return (fds[0].revents & fds[0].events) != 0;
#endif
}

bool device::is_readable(chrono::milliseconds timeout) const {
    return wait(DEVICE_IO_DIRECT::IN, timeout);
}

bool device::is_writable(chrono::milliseconds timeout) const {
    return wait(DEVICE_IO_DIRECT::OUT, timeout);
}

void device::set_timeout(const chrono::milliseconds timeout) {
    timeout_ = timeout;

    if (!is_open()) {
        return;
    }

#ifdef MSTL_PLATFORM_WINDOWS__
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
    timeouts.ReadTotalTimeoutConstant = (timeout.count() > 0) ?
        static_cast<DWORD>(timeout.count()) : 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = (timeout.count() > 0) ?
        static_cast<DWORD>(timeout.count()) : 0;

    SetCommTimeouts(handle_, &timeouts);
#else
    ::termios tty{};
    if (::tcgetattr(fd_, &tty) == 0) {
        tty.c_cc[VTIME] = timeout.count() / 100;
        tty.c_cc[VMIN] = 0;
        ::tcsetattr(fd_, TCSANOW, &tty);
    }
#endif
}

chrono::milliseconds device::get_timeout() const noexcept {
    return timeout_;
}

void device::set_blocking(bool blocking) {
    is_blocking_ = blocking;

    if (!is_open()) {
        return;
    }

#ifdef MSTL_PLATFORM_WINDOWS__

#else
    set_non_blocking(!blocking);
#endif
}

bool device::is_blocking() const noexcept {
    return is_blocking_;
}

void device::set_event_callback(device_event_callback callback) {
    event_callback_ = move(callback);
}

void device::start_event_monitoring() {
    if (monitoring_ || !is_open() || !event_callback_) {
        return;
    }

    monitoring_ = true;
    monitor_thread_ = thread(&device::monitor_device_events, this);
}

void device::stop_event_monitoring() noexcept {
    if (!monitoring_) {
        return;
    }
    monitoring_ = false;
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
}

device_info device::get_device_info() const {
    device_info info;
    info.device_path = device_path_;
    info.type = device_type_;

    if (!is_open()) {
        return info;
    }

#ifdef MSTL_PLATFORM_WINDOWS__
    LARGE_INTEGER size;
    if (GetFileSizeEx(handle_, &size)) {
        info.size_bytes = size.QuadPart;
    }

    BY_HANDLE_FILE_INFORMATION file_info;
    if (GetFileInformationByHandle(handle_, &file_info)) {

    }
#else
    struct ::stat64 st{};
    if (::fstat64(fd_, &st) == 0) {
        info.size_bytes = st.st_size;
        info.device_id = st.st_rdev;

        if (S_ISCHR(st.st_mode)) {
            info.type = DEVICE_TYPE::SERIAL_PORT;
        } else if (S_ISBLK(st.st_mode)) {
            info.type = DEVICE_TYPE::STORAGE;
        }
    }
#endif

    return info;
}

string device::get_device_path() const noexcept {
    return device_path_;
}

DEVICE_TYPE device::get_device_type() const noexcept {
    return device_type_;
}


#ifdef MSTL_PLATFORM_WINDOWS__
void device_controller::setup_cancel_event() {
    cancel_event_ = CreateEvent(nullptr, TRUE, FALSE, nullptr);
}

bool device_controller::get_overlapped_result(size_t& bytes_transferred, bool wait) {
    DWORD bytes = 0;
    BOOL result = GetOverlappedResult(handle_, &overlapped_, &bytes, wait);
    bytes_transferred = bytes;
    return result != FALSE;
}

device_type device_controller::guess_device_type_from_guid(const GUID& guid) {
    if (IsEqualGUID(guid, GUID_DEVCLASS_PORTS)) {
        return device_type::serial_port;
    } else if (IsEqualGUID(guid, GUID_DEVCLASS_DISKDRIVE)) {
        return device_type::storage;
    } else if (IsEqualGUID(guid, GUID_DEVCLASS_NET)) {
        return device_type::network;
    } else if (IsEqualGUID(guid, GUID_DEVCLASS_MEDIA)) {
        return device_type::audio;
    } else if (IsEqualGUID(guid, GUID_DEVCLASS_IMAGE)) {
        return device_type::video;
    } else if (IsEqualGUID(guid, GUID_DEVCLASS_HIDCLASS)) {
        return device_type::hid;
    }
    return device_type::generic;
}

string device_controller::get_device_property(HDEVINFO dev_info_set,
                                                   PSP_DEVINFO_DATA dev_info_data,
                                                   DWORD property) {
    char buffer[1024];
    DWORD buffer_size = sizeof(buffer);

    if (SetupDiGetDeviceRegistryPropertyA(dev_info_set, dev_info_data,
                                         property, nullptr,
                                         reinterpret_cast<PBYTE>(buffer),
                                         buffer_size, &buffer_size)) {
        return string(buffer);
    }

    return "";
}

#else

bool device::set_non_blocking(bool non_blocking) {
    int flags = ::fcntl(fd_, F_GETFL, 0);
    if (flags == -1) {
        return false;
    }

    if (non_blocking) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }

    if (::fcntl(fd_, F_SETFL, flags) == -1) {
        return false;
    }

    is_non_blocking_ = non_blocking;
    return true;
}

DEVICE_TYPE device::guess_device_type_from_path(const string& path) {
    if (path.find("tty") != string::npos ||
        path.find("serial") != string::npos ||
        path.find("COM") != string::npos) {
        return DEVICE_TYPE::SERIAL_PORT;
    } else if (path.find("sd") != string::npos ||
              path.find("hd") != string::npos ||
              path.find("nvme") != string::npos ||
              path.find("vd") != string::npos) {
        return DEVICE_TYPE::STORAGE;
    } else if (path.find("eth") != string::npos ||
              path.find("wlan") != string::npos ||
              path.find("net") != string::npos) {
        return DEVICE_TYPE::NETWORK;
    } else if (path.find("audio") != string::npos ||
              path.find("snd") != string::npos ||
              path.find("pcm") != string::npos) {
        return DEVICE_TYPE::AUDIO;
    } else if (path.find("video") != string::npos ||
              path.find("camera") != string::npos) {
        return DEVICE_TYPE::VIDEO;
    } else if (path.find("hid") != string::npos ||
              path.find("input") != string::npos) {
        return DEVICE_TYPE::HID;
    }

    struct ::stat64 st{};
    if (::stat64(path.c_str(), &st) == 0) {
        return guess_device_type_from_stat(st);
    }
    return DEVICE_TYPE::UNKNOWN;
}

DEVICE_TYPE device::guess_device_type_from_stat(const struct stat64& st) {
    if (S_ISCHR(st.st_mode)) {
        if (st.st_rdev) {
            const unsigned int major = st.st_rdev;
            if (major == 4) {
                return DEVICE_TYPE::SERIAL_PORT;
            } else if (major == 13) {
                return DEVICE_TYPE::HID;
            } else if (major == 29) {
                return DEVICE_TYPE::VIDEO;
            } else if (major == 116) {
                return DEVICE_TYPE::AUDIO;
            }
        }
        return DEVICE_TYPE::GENERIC;
    } else if (S_ISBLK(st.st_mode)) {
        return DEVICE_TYPE::STORAGE;
    }

    return DEVICE_TYPE::UNKNOWN;
}

string device::read_sysfs_attribute(
    const string& device_path, const string& attribute) {
    return "";
}

#endif

void device::monitor_device_events() {
    while (monitoring_) {
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

#ifdef MSTL_PLATFORM_WINDOWS__
void* device::map_memory(size_t offset, size_t size) {
    if (!is_open()) {
        throw device_exception("Device not open");
    }

    HANDLE mapping = CreateFileMapping(
        handle_,
        nullptr,
        PAGE_READWRITE,
        0,
        static_cast<DWORD>(size),
        nullptr
    );

    if (!mapping) {
        throw device_exception("CreateFileMapping failed", GetLastError());
    }

    void* address = MapViewOfFile(
        mapping,
        FILE_MAP_ALL_ACCESS,
        0,
        static_cast<DWORD>(offset),
        size
    );

    CloseHandle(mapping);

    if (!address) {
        throw device_exception("MapViewOfFile failed", GetLastError());
    }

    return address;
}
#else
void* device::map_memory(const ::off_t offset, const size_t size) const {
    if (!is_open()) {
        throw_exception(device_exception("Device not open"));
    }

    void* address = ::mmap(
        nullptr,
        size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd_,
        offset
    );

    if (address == MAP_FAILED) {
        throw_exception(device_exception(("mmap failed: " + string(strerror(errno))).data()));
    }

    return address;
}
#endif

void device::unmap_memory(void* address, const size_t size) noexcept {
    if (!address) {
        return;
    }

#ifdef MSTL_PLATFORM_WINDOWS__
    UnmapViewOfFile(address);
#else
    ::munmap(address, size);
#endif
}

bool device::supports_direct_io() const noexcept {
    if (!is_open()) {
        return false;
    }
#ifdef MSTL_PLATFORM_WINDOWS__
    DWORD flags = 0;
    if (!GetHandleInformation(handle_, &flags)) {
        return false;
    }
    return (flags & HANDLE_FLAG_PROTECT_FROM_CLOSE) == 0;
#else
    struct ::stat64 st{};
    if (::fstat64(fd_, &st) == -1) {
        return false;
    }
    return S_ISBLK(st.st_mode) || S_ISCHR(st.st_mode);
#endif
}

vector<device_info> device::enumerate_devices(const DEVICE_TYPE type, const string& filter) {
    vector<device_info> devices;

#ifdef MSTL_PLATFORM_WINDOWS__
    HDEVINFO device_info_set = SetupDiGetClassDevs(
        nullptr,
        nullptr,
        nullptr,
        DIGCF_ALLCLASSES | DIGCF_PRESENT
    );

    if (device_info_set == INVALID_HANDLE_VALUE) {
        return devices;
    }

    SP_DEVINFO_DATA device_info_data;
    device_info_data.cbSize = sizeof(SP_DEVINFO_DATA);

    for (DWORD i = 0; SetupDiEnumDeviceInfo(device_info_set, i, &device_info_data); ++i) {
        device_info info;

        char instance_id[256];
        if (SetupDiGetDeviceInstanceIdA(device_info_set, &device_info_data,
                                       instance_id, sizeof(instance_id), nullptr)) {
            info.hardware_id = instance_id;
        }

        info.friendly_name = get_device_property(device_info_set, &device_info_data,
                                                SPDRP_FRIENDLYNAME);

        info.description = get_device_property(device_info_set, &device_info_data,
                                              SPDRP_DEVICEDESC);

        info.manufacturer = get_device_property(device_info_set, &device_info_data,
                                               SPDRP_MFG);

        GUID class_guid;
        if (SetupDiGetDeviceRegistryProperty(device_info_set, &device_info_data,
                                            SPDRP_CLASSGUID, nullptr,
                                            reinterpret_cast<PBYTE>(&class_guid),
                                            sizeof(class_guid), nullptr)) {
            info.type = guess_device_type_from_guid(class_guid);
        }

        if (type != DEVICE_TYPE::GENERIC && info.type != type) {
            continue;
        }

        if (!filter.empty() &&
            info.friendly_name.find(filter) == string::npos &&
            info.description.find(filter) == string::npos) {
            continue;
        }

        char device_path[256];
        if (CM_Get_Device_IDA(device_info_data.DevInst, device_path,
                             sizeof(device_path), 0) == CR_SUCCESS) {
            info.device_path = device_path;
        }

        devices.push_back(info);
    }

    SetupDiDestroyDeviceInfoList(device_info_set);

#else
    vector<string> device_dirs = {
        "/dev",
        "/sys/class",
        "/proc/bus/usb"
    };

    for (const auto& dir_path : device_dirs) {
        ::DIR* dir = ::opendir(dir_path.c_str());
        if (!dir) {
            continue;
        }
        ::dirent* entry;
        while ((entry = ::readdir(dir)) != nullptr) {
            string name = entry->d_name;
            if (name == "." || name == "..") {
                continue;
            }
            string full_path = dir_path + "/" + name;
            if (!is_device(full_path)) {
                continue;
            }

            device_info info;
            info.device_path = full_path;
            info.friendly_name = name;
            info.type = guess_device_type_from_path(full_path);

            if (type != DEVICE_TYPE::GENERIC && info.type != type) {
                continue;
            }
            if (!filter.empty() &&
                info.device_path.find(filter) == string::npos &&
                info.friendly_name.find(filter) == string::npos) {
                continue;
            }

            struct ::stat64 st{};
            if (::stat64(full_path.c_str(), &st) == 0) {
                info.device_id = st.st_rdev;
                info.size_bytes = st.st_size;
            }
            devices.push_back(info);
        }
        ::closedir(dir);
    }
#endif
    return devices;
}

vector<device_info> device::find_devices_by_vid_pid(uint16_t vid, uint16_t pid) {
    vector<device_info> result;

#ifdef MSTL_PLATFORM_WINDOWS__
    string vid_pid_filter = format("VID_{:04X}&PID_{:04X}", vid, pid);
    HDEVINFO device_info_set = SetupDiGetClassDevs(
        nullptr,
        "USB",
        nullptr,
        DIGCF_ALLCLASSES | DIGCF_PRESENT
    );

    if (device_info_set == INVALID_HANDLE_VALUE) {
        return result;
    }

    SP_DEVINFO_DATA device_info_data;
    device_info_data.cbSize = sizeof(SP_DEVINFO_DATA);

    for (DWORD i = 0; SetupDiEnumDeviceInfo(device_info_set, i, &device_info_data); ++i) {
        char hardware_id[256];
        if (SetupDiGetDeviceInstanceIdA(device_info_set, &device_info_data,
                                       hardware_id, sizeof(hardware_id), nullptr)) {
            string hw_id_str(hardware_id);
            if (hw_id_str.find(vid_pid_filter) != string::npos) {
                device_info info;
                info.hardware_id = hardware_id;
                info.vendor_id = vid;
                info.product_id = pid;
                info.friendly_name = get_device_property(device_info_set, &device_info_data,
                                                        SPDRP_FRIENDLYNAME);
                info.description = get_device_property(device_info_set, &device_info_data,
                                                      SPDRP_DEVICEDESC);
                info.manufacturer = get_device_property(device_info_set, &device_info_data,
                                                       SPDRP_MFG);
                info.device_path = get_device_property(device_info_set, &device_info_data,
                                                      SPDRP_PHYSICAL_DEVICE_OBJECT_NAME);
                info.type = DEVICE_TYPE::HID;
                result.push_back(info);
            }
        }
    }

    SetupDiDestroyDeviceInfoList(device_info_set);
#else
    string sysfs_path = "/sys/bus/usb/devices";
    ::DIR* dir = ::opendir(sysfs_path.c_str());
    if (!dir) {
        return result;
    }
    ::dirent* entry;
    while ((entry = ::readdir(dir)) != nullptr) {
        string name = entry->d_name;
        if (name == "." || name == "..") {
            continue;
        }

        path vendor_file(sysfs_path + "/" + name + "/idVendor");
        path product_file(sysfs_path + "/" + name + "/idProduct");

        file vendor(vendor_file);
        file product(product_file);

        for (auto i1 = vendor.begin_lines(), i2 = product.begin_lines();
            i1 != vendor.end_lines() && i2 != vendor.end_lines(); ++i1, ++i2) {
            const string& vendor_str = *i1;
            const string& product_str = *i2;

            uint16_t found_vid = to_int32(vendor_str.view(), nullptr, 16);
            uint16_t found_pid = to_int32(product_str.view(), nullptr, 16);

            if (found_vid == vid && found_pid == pid) {
                device_info info;
                info.vendor_id = vid;
                info.product_id = pid;
                info.hardware_id = "USB\\VID_" + vendor_str + "&PID_" + product_str;

                path manufacturer_file(sysfs_path + "/" + name + "/manufacturer");
                path product_name_file(sysfs_path + "/" + name + "/product");
                file man(manufacturer_file);
                file prod(product_name_file);

                info.manufacturer = man.read_line();
                info.friendly_name = prod.read_line();
                info.description = info.friendly_name;

                string dev_path = "/dev/" + name;
                if (exists(dev_path)) {
                    info.device_path = dev_path;
                } else {
                    string tty_path = sysfs_path + "/" + name + "/tty";
                    if (exists(tty_path)) {
                        ::DIR* tty_dir = ::opendir(tty_path.c_str());
                        if (tty_dir) {
                            ::dirent* tty_entry;
                            while ((tty_entry = ::readdir(tty_dir)) != nullptr) {
                                string tty_name = tty_entry->d_name;
                                if (tty_name != "." && tty_name != "..") {
                                    info.device_path = "/dev/" + tty_name;
                                    info.type = DEVICE_TYPE::SERIAL_PORT;
                                    break;
                                }
                            }
                            ::closedir(tty_dir);
                        }
                    }
                }
                result.push_back(info);
            }
        }
    }
    ::closedir(dir);
#endif
    return result;
}

optional<device_info> device::find_device_by_path(const string& path) {
    if (!exists(path) || !is_device(path)) {
        return nullopt;
    }

    device_info info;
    info.device_path = path;
    info.type = guess_device_type_from_path(path);
#ifdef MSTL_PLATFORM_WINDOWS__
    HANDLE handle = CreateFileA(
        path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (handle != INVALID_HANDLE_VALUE) {
        BY_HANDLE_FILE_INFORMATION file_info;
        if (GetFileInformationByHandle(handle, &file_info)) {
            info.size_bytes = (static_cast<uint64_t>(file_info.nFileSizeHigh) << 32) | file_info.nFileSizeLow;
        }
        CloseHandle(handle);
    }
#else
    struct ::stat64 st{};
    if (::stat64(path.c_str(), &st) == 0) {
        info.size_bytes = st.st_size;
        info.device_id = st.st_rdev;
        info.block_size = st.st_blksize;
    }
#endif
    return info;
}

bool device::exists(const string& device_path) {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::DWORD attrs = ::GetFileAttributesA(device_path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES;
#else
    return ::access(device_path.c_str(), F_OK) == 0;
#endif
}

bool device::is_device(const string& path) {
#ifdef MSTL_PLATFORM_WINDOWS__
    return path.find("\\\\.\\") == 0 || 
           path.find("COM") != string::npos ||
           path.find("LPT") != string::npos;
#else
    struct ::stat64 st{};
    if (::stat64(path.c_str(), &st) != 0) {
        return false;
    }

    return S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode) || 
           S_ISFIFO(st.st_mode) || S_ISSOCK(st.st_mode);
#endif
}

serial_port::serial_port(const string& port_name, const serial_config& config, const DEVICE_OPEN_FLAG flags)
: device(port_name, DEVICE_OPEN_MODE::READ_WRITE, flags) {
    configure(config);
}

void serial_port::configure(const serial_config& config) {
    current_config_ = config;
#ifdef MSTL_PLATFORM_WINDOWS__
    configure_windows(config);
#else
    configure_linux(config);
#endif
}

serial_port::serial_config serial_port::get_configuration() const {
    return current_config_;
}

void serial_port::set_rts(const bool state) {
#ifdef MSTL_PLATFORM_WINDOWS__
    if (state) {
        EscapeCommFunction(native_handle(), SETRTS);
    } else {
        EscapeCommFunction(native_handle(), CLRRTS);
    }
#else
    const int fd = native_handle();
    int flags;
    if (::ioctl(fd, TIOCMGET, &flags) == -1) {
        throw_exception(device_exception("Failed to get modem status"));
    }

    if (state) {
        flags |= TIOCM_RTS;
    } else {
        flags &= ~TIOCM_RTS;
    }

    if (::ioctl(fd, TIOCMSET, &flags) == -1) {
        throw_exception(device_exception("Failed to set RTS"));
    }
#endif
}

void serial_port::set_dtr(const bool state) {
#ifdef MSTL_PLATFORM_WINDOWS__
    if (state) {
        EscapeCommFunction(native_handle(), SETDTR);
    } else {
        EscapeCommFunction(native_handle(), CLRDTR);
    }
#else
    const int fd = native_handle();
    int flags;
    if (::ioctl(fd, TIOCMGET, &flags) == -1) {
        throw_exception(device_exception("Failed to get modem status"));
    }

    if (state) {
        flags |= TIOCM_DTR;
    } else {
        flags &= ~TIOCM_DTR;
    }

    if (::ioctl(fd, TIOCMSET, &flags) == -1) {
        throw_exception(device_exception("Failed to set DTR"));
    }
#endif
}

bool serial_port::get_cts() const {
    return get_modem_status().cts;
}

bool serial_port::get_dsr() const {
    return get_modem_status().dsr;
}

bool serial_port::get_ri() const {
    return get_modem_status().ri;
}

bool serial_port::get_dcd() const {
    return get_modem_status().dcd;
}

serial_port::modem_status serial_port::get_modem_status() const {
#ifdef MSTL_PLATFORM_WINDOWS__
    return get_modem_status_windows();
#else
    return get_modem_status_linux();
#endif
}

void serial_port::set_break(const bool enable) {
#ifdef MSTL_PLATFORM_WINDOWS__
    if (enable) {
        SetCommBreak(native_handle());
    } else {
        ClearCommBreak(native_handle());
    }
#else
    const int fd = native_handle();
    if (::ioctl(fd, enable ? TIOCSBRK : TIOCCBRK, 0) == -1) {
        throw_exception(device_exception("Failed to set break"));
    }
#endif
}

serial_port::line_status serial_port::get_line_status() const {
#ifdef MSTL_PLATFORM_WINDOWS__
    DWORD errors;
    COMSTAT comstat;
    if (!ClearCommError(native_handle(), &errors, &comstat)) {
        throw device_exception("Failed to get line status");
    }

    line_status status;
    status.framing_error = (errors & CE_FRAME) != 0;
    status.parity_error = (errors & CE_RXPARITY) != 0;
    status.overrun_error = (errors & CE_OVERRUN) != 0;
    status.break_detected = (errors & CE_BREAK) != 0;
    return status;
#else
    return get_line_status_linux();
#endif
}

void serial_port::purge_rx_buffer() {
#ifdef MSTL_PLATFORM_WINDOWS__
    PurgeComm(native_handle(), PURGE_RXCLEAR | PURGE_RXABORT);
#else
    const int fd = native_handle();
    if (::ioctl(fd, TCFLSH, TCIFLUSH) == -1) {
        throw_exception(device_exception("Failed to purge RX buffer"));
    }
#endif
}

void serial_port::purge_tx_buffer() {
#ifdef MSTL_PLATFORM_WINDOWS__
    PurgeComm(native_handle(), PURGE_TXCLEAR | PURGE_TXABORT);
#else
    const int fd = native_handle();
    if (::ioctl(fd, TCFLSH, TCOFLUSH) == -1) {
        throw_exception(device_exception("Failed to purge TX buffer"));
    }
#endif
}

void serial_port::purge_both_buffers() {
#ifdef MSTL_PLATFORM_WINDOWS__
    PurgeComm(native_handle(), PURGE_RXCLEAR | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_TXABORT);
#else
    const int fd = native_handle();
    if (::ioctl(fd, TCFLSH, TCIOFLUSH) == -1) {
        throw_exception(device_exception("Failed to purge buffers"));
    }
#endif
}

size_t serial_port::get_rx_queue_size() const {
#ifdef MSTL_PLATFORM_WINDOWS__
    COMSTAT comstat;
    DWORD errors;
    if (!ClearCommError(native_handle(), &errors, &comstat)) {
        throw device_exception("Failed to get RX queue size");
    }
    return comstat.cbInQue;
#else
    const int fd = native_handle();
    int bytes;
    if (::ioctl(fd, FIONREAD, &bytes) == -1) {
        throw_exception(device_exception("Failed to get RX queue size"));
    }
    return bytes;
#endif
}

size_t serial_port::get_tx_queue_size() const {
#ifdef MSTL_PLATFORM_WINDOWS__
    COMSTAT comstat;
    DWORD errors;
    if (!ClearCommError(native_handle(), &errors, &comstat)) {
        throw device_exception("Failed to get TX queue size");
    }
    return comstat.cbOutQue;
#else
    const int fd = native_handle();
    int bytes;
    if (::ioctl(fd, TIOCOUTQ, &bytes) == -1) {
        throw_exception(device_exception("Failed to get TX queue size"));
    }
    return bytes;
#endif
}

#ifdef MSTL_PLATFORM_WINDOWS__
void serial_port::configure_windows(const serial_config& config) {
    HANDLE handle = native_handle();

    DCB dcb = {0};
    dcb.DCBlength = sizeof(DCB);

    if (!GetCommState(handle, &dcb)) {
        throw device_exception("Failed to get serial port state");
    }

    dcb.BaudRate = config.baud_rate;
    dcb.ByteSize = config.data_bits;
    dcb.StopBits = (config.stop_bits == 2) ? TWOSTOPBITS : ONESTOPBIT;

    switch (config.parity) {
        case 'N': dcb.Parity = NOPARITY; break;
        case 'E': dcb.Parity = EVENPARITY; break;
        case 'O': dcb.Parity = ODDPARITY; break;
        case 'M': dcb.Parity = MARKPARITY; break;
        case 'S': dcb.Parity = SPACEPARITY; break;
        default: dcb.Parity = NOPARITY; break;
    }

    dcb.fBinary = TRUE;
    dcb.fParity = (config.parity != 'N');
    dcb.fOutxCtsFlow = config.flow_control;
    dcb.fOutxDsrFlow = config.flow_control;
    dcb.fDtrControl = config.dtr_control ? DTR_CONTROL_ENABLE : DTR_CONTROL_DISABLE;
    dcb.fDsrSensitivity = config.dsr_sensitivity;
    dcb.fOutX = config.xon_xoff;
    dcb.fInX = config.xon_xoff;
    dcb.fRtsControl = config.rts_control ? RTS_CONTROL_ENABLE : RTS_CONTROL_DISABLE;

    if (!SetCommState(handle, &dcb)) {
        throw device_exception("Failed to set serial port state");
    }

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = config.read_interval_timeout.count();
    timeouts.ReadTotalTimeoutMultiplier = config.read_total_timeout_multiplier.count();
    timeouts.ReadTotalTimeoutConstant = config.read_total_timeout_constant.count();
    timeouts.WriteTotalTimeoutMultiplier = config.write_total_timeout_multiplier.count();
    timeouts.WriteTotalTimeoutConstant = config.write_total_timeout_constant.count();

    if (!SetCommTimeouts(handle, &timeouts)) {
        throw device_exception("Failed to set serial port timeouts");
    }
}

serial_port::modem_status serial_port::get_modem_status_windows() const {
    DWORD modem_stat;
    if (!GetCommModemStatus(native_handle(), &modem_stat)) {
        throw device_exception("Failed to get modem status");
    }

    modem_status status;
    status.cts = (modem_stat & MS_CTS_ON) != 0;
    status.dsr = (modem_stat & MS_DSR_ON) != 0;
    status.ri = (modem_stat & MS_RING_ON) != 0;
    status.dcd = (modem_stat & MS_RLSD_ON) != 0;
    return status;
}
#else
void serial_port::configure_linux(const serial_config& config) {
    const int fd = native_handle();
    ::termios tty{};
    if (::tcgetattr(fd, &tty) == -1) {
        throw_exception(device_exception("Failed to get serial port attributes"));
    }
    ::speed_t speed;
    switch (config.baud_rate) {
        case 50: speed = B50; break;
        case 75: speed = B75; break;
        case 110: speed = B110; break;
        case 134: speed = B134; break;
        case 150: speed = B150; break;
        case 200: speed = B200; break;
        case 300: speed = B300; break;
        case 600: speed = B600; break;
        case 1200: speed = B1200; break;
        case 1800: speed = B1800; break;
        case 2400: speed = B2400; break;
        case 4800: speed = B4800; break;
        case 9600: speed = B9600; break;
        case 19200: speed = B19200; break;
        case 38400: speed = B38400; break;
        case 57600: speed = B57600; break;
        case 115200: speed = B115200; break;
        case 230400: speed = B230400; break;
        case 460800: speed = B460800; break;
        case 500000: speed = B500000; break;
        case 576000: speed = B576000; break;
        case 921600: speed = B921600; break;
        case 1000000: speed = B1000000; break;
        case 1152000: speed = B1152000; break;
        case 1500000: speed = B1500000; break;
        case 2000000: speed = B2000000; break;
        case 2500000: speed = B2500000; break;
        case 3000000: speed = B3000000; break;
        case 3500000: speed = B3500000; break;
        case 4000000: speed = B4000000; break;
        default: speed = B115200; break;
    }

    ::cfsetispeed(&tty, speed);
    ::cfsetospeed(&tty, speed);

    tty.c_cflag &= ~CSIZE;
    switch (config.data_bits) {
        case 5: tty.c_cflag |= CS5; break;
        case 6: tty.c_cflag |= CS6; break;
        case 7: tty.c_cflag |= CS7; break;
        case 8: tty.c_cflag |= CS8; break;
        default: tty.c_cflag |= CS8; break;
    }

    if (config.stop_bits == 2) {
        tty.c_cflag |= CSTOPB;
    } else {
        tty.c_cflag &= ~CSTOPB;
    }

    switch (config.parity) {
        case 'N':
            tty.c_cflag &= ~PARENB;
            tty.c_iflag &= ~INPCK;
            break;
        case 'E':
            tty.c_cflag |= PARENB;
            tty.c_cflag &= ~PARODD;
            tty.c_iflag |= INPCK;
            break;
        case 'O':
            tty.c_cflag |= PARENB;
            tty.c_cflag |= PARODD;
            tty.c_iflag |= INPCK;
            break;
        default:
            tty.c_cflag &= ~PARENB;
            tty.c_iflag &= ~INPCK;
            break;
    }

    if (config.flow_control) {
        tty.c_cflag |= CRTSCTS;
    } else {
        tty.c_cflag &= ~CRTSCTS;
    }

    if (config.xon_xoff) {
        tty.c_iflag |= (IXON | IXOFF | IXANY);
    } else {
        tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    }

    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_oflag &= ~OPOST;
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_cflag |= (CLOCAL | CREAD);

    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = (config.read_total_timeout_constant.count() + 99) / 100;

    if (::tcsetattr(fd, TCSANOW, &tty) == -1) {
        throw_exception(device_exception("Failed to set serial port attributes"));
    }
}

serial_port::modem_status serial_port::get_modem_status_linux() const {
    const int fd = native_handle();
    int flags;

    if (::ioctl(fd, TIOCMGET, &flags) == -1) {
        throw_exception(device_exception("Failed to get modem status"));
    }

    modem_status status;
    status.cts = (flags & TIOCM_CTS) != 0;
    status.dsr = (flags & TIOCM_DSR) != 0;
    status.ri = (flags & TIOCM_RI) != 0;
    status.dcd = (flags & TIOCM_CD) != 0;
    return status;
}

serial_port::line_status serial_port::get_line_status_linux() const {
    const int fd = native_handle();
    byte_t lsr = 0;
    if (::ioctl(fd, TIOCSERGETLSR, &lsr) == -1) {
        lsr = 0;
    }

    serial_port::line_status status{};
    status.framing_error    = (lsr & 0x08) != 0;  // UART_LSR_FE
    status.parity_error     = (lsr & 0x04) != 0;  // UART_LSR_PE
    status.overrun_error    = (lsr & 0x02) != 0;  // UART_LSR_OE
    status.break_detected   = (lsr & 0x10) != 0;  // UART_LSR_BI
    return status;
}
#endif

storage_device::storage_device(const string& device_path, const DEVICE_OPEN_FLAG flags)
    : device(device_path, DEVICE_OPEN_MODE::READ_WRITE, flags | DEVICE_OPEN_FLAG::DIRECT_IO | DEVICE_OPEN_FLAG::NO_INHERIT) {
    query_device_geometry();
}

uint64_t storage_device::get_capacity_bytes() const {
    return sector_size_ * get_capacity_sectors();
}

uint64_t storage_device::get_capacity_sectors() const {
    if (!is_open()) {
        return 0;
    }

#ifdef MSTL_PLATFORM_WINDOWS__
    DISK_GEOMETRY_EX geometry;
    DWORD bytes_returned;

    if (!DeviceIoControl(native_handle(), IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
                        nullptr, 0, &geometry, sizeof(geometry),
                        &bytes_returned, nullptr)) {
        return 0;
    }

    return geometry.DiskSize.QuadPart / sector_size_;
#else
    const int fd = native_handle();
    uint64_t sectors = 0;

    if (::ioctl(fd, BLKGETSIZE64, &sectors) == -1) {
        return 0;
    }

    return sectors / sector_size_;
#endif
}

uint32_t storage_device::get_sector_size() const {
    return sector_size_;
}

uint32_t storage_device::get_physical_sector_size() const {
    return physical_sector_size_;
}

vector<storage_device::partition_info> storage_device::get_partitions() const {
    vector<partition_info> partitions;

#ifdef MSTL_PLATFORM_WINDOWS__
    HANDLE handle = native_handle();

    DWORD bytes_returned;
    vector<uint8_t> buffer(sizeof(DRIVE_LAYOUT_INFORMATION_EX) +
                          sizeof(PARTITION_INFORMATION_EX) * 128);

    if (!DeviceIoControl(handle, IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                        nullptr, 0, buffer.data(),
                        static_cast<DWORD>(buffer.size()),
                        &bytes_returned, nullptr)) {
        return partitions;
    }

    auto* layout = reinterpret_cast<DRIVE_LAYOUT_INFORMATION_EX*>(buffer.data());

    for (DWORD i = 0; i < layout->PartitionCount; i++) {
        auto& part = layout->PartitionEntry[i];

        if (part.PartitionNumber == 0) {
            continue;
        }

        partition_info info;
        info.start_sector = part.StartingOffset.QuadPart / sector_size_;
        info.sector_count = part.PartitionLength.QuadPart / sector_size_;
        info.partition_type = part.PartitionStyle == PARTITION_STYLE_MBR ?
                             part.Mbr.PartitionType : 0;
        info.bootable = part.Mbr.BootIndicator != 0;

        if (part.PartitionStyle == PARTITION_STYLE_GPT) {
            wstring_convert<codecvt_utf8<wchar_t>> converter;
            info.label = converter.to_bytes(part.Gpt.Name);
        }

        partitions.push_back(info);
    }
#else
    const string device_path = get_device_path();
    const string sys_path = "/sys/class/block/" + device_path.substr(5);
    ::DIR* dir = opendir(sys_path.c_str());
    if (!dir) {
        return partitions;
    }

    ::dirent* entry;
    while ((entry = ::readdir(dir)) != nullptr) {
        string name = entry->d_name;
        if (name.find(device_path.substr(5) + "p") == 0) {
            partition_info info;
            const path start_file(sys_path + "/" + name + "/start");
            file start_stream(start_file);
            const path size_file(sys_path + "/" + name + "/size");
            file size_stream(size_file);

            info.start_sector = to_uint64(start_stream.read_line().view());
            info.sector_count = to_uint64(size_stream.read_line().view());

            string type_file = sys_path + "/" + name + "/partition";
            if (exists(type_file)) {
                info.partition_type = 0x83;
            }
            partitions.push_back(info);
        }
    }
    ::closedir(dir);
#endif

    return partitions;
}

bool storage_device::has_partitions() const noexcept {
    return !get_partitions().empty();
}

void storage_device::read_sectors(void* buffer,
    const uint64_t sector_start, const size_t sector_count) {
    if (!is_open()) {
        throw_exception(device_exception("Device not open"));
    }

    uint64_t offset = sector_start * sector_size_;
    size_t size = sector_count * sector_size_;

#ifdef MSTL_PLATFORM_WINDOWS__
    LARGE_INTEGER li_offset;
    li_offset.QuadPart = offset;

    overlapped_.Offset = li_offset.LowPart;
    overlapped_.OffsetHigh = li_offset.HighPart;

    DWORD bytes_read;
    if (!ReadFile(native_handle(), buffer, static_cast<DWORD>(size),
                 &bytes_read, &overlapped_)) {
        DWORD error = GetLastError();
        if (error != ERROR_IO_PENDING) {
            throw device_exception("Failed to read sectors", error);
        }

        if (!GetOverlappedResult(native_handle(), &overlapped_,
                                &bytes_read, TRUE)) {
            throw device_exception("Failed to get overlapped result", GetLastError());
        }
    }
#else
    if (::lseek(native_handle(), static_cast<::off_t>(offset), SEEK_SET) == -1) {
        throw_exception(device_exception("Failed to seek to sector"));
    }
    const ssize_t bytes_read = ::read(native_handle(), buffer, size);
    if (bytes_read != static_cast<ssize_t>(size)) {
        throw_exception(device_exception("Failed to read sectors"));
    }
#endif
}

void storage_device::write_sectors(const void* buffer,
    const uint64_t sector_start, const size_t sector_count) {
    if (!is_open()) {
        throw_exception(device_exception("Device not open"));
    }

    uint64_t offset = sector_start * sector_size_;
    size_t size = sector_count * sector_size_;

#ifdef MSTL_PLATFORM_WINDOWS__
    LARGE_INTEGER li_offset;
    li_offset.QuadPart = offset;

    overlapped_.Offset = li_offset.LowPart;
    overlapped_.OffsetHigh = li_offset.HighPart;

    DWORD bytes_written;
    if (!WriteFile(native_handle(), buffer, static_cast<DWORD>(size),
                  &bytes_written, &overlapped_)) {
        DWORD error = GetLastError();
        if (error != ERROR_IO_PENDING) {
            throw device_exception("Failed to write sectors", error);
        }

        if (!GetOverlappedResult(native_handle(), &overlapped_,
                                &bytes_written, TRUE)) {
            throw device_exception("Failed to get overlapped result", GetLastError());
        }
    }
#else
    if (lseek(native_handle(), static_cast<::off_t>(offset), SEEK_SET) == -1) {
        throw_exception(device_exception("Failed to seek to sector"));
    }

    ssize_t bytes_written = ::write(native_handle(), buffer, size);
    if (bytes_written != static_cast<ssize_t>(size)) {
        throw_exception(device_exception("Failed to write sectors"));
    }
#endif
}

bool storage_device::is_removable() const {
    return is_removable_;
}

bool storage_device::is_read_only() const {
    return is_read_only_;
}

bool storage_device::supports_trim() const {
#ifdef MSTL_PLATFORM_WINDOWS__
    DEVICE_TRIM_DESCRIPTOR trim_desc;
    DWORD bytes_returned;

    if (DeviceIoControl(native_handle(), IOCTL_STORAGE_QUERY_PROPERTY,
                       &trim_desc, sizeof(trim_desc), &trim_desc,
                       sizeof(trim_desc), &bytes_returned, nullptr)) {
        return trim_desc.TrimEnabled != 0;
    }
    return false;
#else
    const string device_path = get_device_path();
    const path sys_path("/sys/block/" + device_path.substr(5) + "/queue/discard_max_bytes");
    const file discard_stream(sys_path);
    const string discard_str = discard_stream.read_line();
    if (discard_str.empty()) {
        return false;
    }
    return !discard_str.empty() && discard_str != "0";
#endif
}

bool storage_device::supports_flush() const {
    return true;
}

void storage_device::flush_buffers() {
    flush();
}

void storage_device::trim(const uint64_t sector_start, const size_t sector_count) {
#ifdef MSTL_PLATFORM_WINDOWS__
    FILE_ZERO_DATA_INFORMATION zero_info;
    zero_info.FileOffset.QuadPart = sector_start * sector_size_;
    zero_info.BeyondFinalZero.QuadPart = (sector_start + sector_count) * sector_size_;

    DWORD bytes_returned;
    if (!DeviceIoControl(native_handle(), FSCTL_FILE_ZERO_DATA,
                        &zero_info, sizeof(zero_info), nullptr, 0,
                        &bytes_returned, nullptr)) {
        throw device_exception("Failed to trim sectors", GetLastError());
    }
#else
    uint64_t range[2] = {sector_start * sector_size_, sector_count * sector_size_};

    if (::ioctl(native_handle(), BLKDISCARD, &range) == -1) {
        throw_exception(device_exception("Failed to trim sectors"));
    }
#endif
}

void storage_device::secure_erase() {
#ifdef MSTL_PLATFORM_WINDOWS__
    DWORD bytes_returned;
    if (!DeviceIoControl(native_handle(), IOCTL_STORAGE_REINITIALIZE_MEDIA,
                        nullptr, 0, nullptr, 0, &bytes_returned, nullptr)) {
        throw device_exception("Failed to secure erase", GetLastError());
    }
#else
    throw_exception(device_exception("Secure erase not supported on this platform"));
#endif
}

void storage_device::lock() {
#ifdef MSTL_PLATFORM_WINDOWS__
    DWORD bytes_returned;
    if (!DeviceIoControl(native_handle(), FSCTL_LOCK_VOLUME,
                        nullptr, 0, nullptr, 0, &bytes_returned, nullptr)) {
        throw device_exception("Failed to lock device", GetLastError());
    }
#endif
}

void storage_device::unlock() {
#ifdef MSTL_PLATFORM_WINDOWS__
    DWORD bytes_returned;
    if (!DeviceIoControl(native_handle(), FSCTL_UNLOCK_VOLUME,
                        nullptr, 0, nullptr, 0, &bytes_returned, nullptr)) {
        throw device_exception("Failed to unlock device", GetLastError());
    }
#endif
}

bool storage_device::is_locked() const {
#ifdef MSTL_PLATFORM_WINDOWS__
    DWORD bytes_returned;
    return DeviceIoControl(native_handle(), IOCTL_STORAGE_CHECK_VERIFY2,
                          nullptr, 0, nullptr, 0, &bytes_returned, nullptr);
#else
    const int fd = ::open(get_device_path().c_str(), O_RDONLY | O_EXCL);
    if (fd != -1) {
        ::close(fd);
        return false;
    }
    return errno == EBUSY;
#endif
}

void storage_device::eject() {
#ifdef MSTL_PLATFORM_WINDOWS__
    DWORD bytes_returned;
    if (!DeviceIoControl(native_handle(), IOCTL_STORAGE_EJECT_MEDIA,
                        nullptr, 0, nullptr, 0, &bytes_returned, nullptr)) {
        throw device_exception("Failed to eject device", GetLastError());
    }
#else
    const string device_path = get_device_path();
    const file mounts(path("/proc/mounts"));
    for (auto iter = mounts.begin_lines(); iter != mounts.end_lines(); ++iter) {
        const string& line = *iter;
        if (line.find(device_path) != string::npos) {
            const size_t space_pos = line.find(' ');
            if (space_pos != string::npos) {
                string mount_point = line.substr(space_pos + 1);
                mount_point = mount_point.substr(0, mount_point.find(' '));

                if (::umount(mount_point.c_str()) == -1) {
                    throw_exception(device_exception("Failed to unmount partition"));
                }
            }
        }
    }

    if (is_removable_) {
        const string eject_cmd = "eject " + device_path;
        if (::system(eject_cmd.c_str()) != 0) {
            throw_exception(device_exception("Failed to eject device"));
        }
    }
#endif
}

void storage_device::query_device_geometry() {
#ifdef MSTL_PLATFORM_WINDOWS__
    query_device_geometry_windows();
#else
    query_device_geometry_linux();
#endif
}

#ifdef MSTL_PLATFORM_WINDOWS__
void storage_device::query_device_geometry_windows() {
    HANDLE handle = native_handle();

    DISK_GEOMETRY_EX geometry;
    DWORD bytes_returned;

    if (DeviceIoControl(handle, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
                       nullptr, 0, &geometry, sizeof(geometry),
                       &bytes_returned, nullptr)) {
        sector_size_ = geometry.Geometry.BytesPerSector;
        physical_sector_size_ = sector_size_;

        STORAGE_PROPERTY_QUERY query;
        query.PropertyId = StorageDeviceProperty;
        query.QueryType = PropertyStandardQuery;

        STORAGE_DEVICE_DESCRIPTOR desc;
        if (DeviceIoControl(handle, IOCTL_STORAGE_QUERY_PROPERTY,
                           &query, sizeof(query), &desc, sizeof(desc),
                           &bytes_returned, nullptr)) {
            is_removable_ = desc.RemovableMedia != 0;
        }
    }

    DWORD file_attributes = GetFileAttributes(get_device_path().c_str());
    is_read_only_ = (file_attributes & FILE_ATTRIBUTE_READONLY) != 0;
}
#else
void storage_device::query_device_geometry_linux() {
    const int fd = native_handle();

    if (::ioctl(fd, BLKSSZGET, &sector_size_) == -1) {
        sector_size_ = 512;
    }

    if (::ioctl(fd, BLKBSZGET, &physical_sector_size_) == -1) {
        physical_sector_size_ = sector_size_;
    }

    const string device_path = get_device_path();
    const path removable_path("/sys/block/" + device_path.substr(5) + "/removable");
    const file removable_stream(removable_path);
    is_removable_ = removable_stream.read_line() == "1";

    const path ro_path("/sys/block/" + device_path.substr(5) + "/ro");
    const file ro_stream(ro_path);
    is_read_only_ = ro_stream.read_line() == "1";

    if (!is_read_only_) {
        struct ::stat64 st{};
        if (::stat64(device_path.c_str(), &st) == 0) {
            is_read_only_ = (st.st_mode & S_IWUSR) == 0;
        }
    }
}
#endif

string device_utils::get_last_error_string() {
#ifdef MSTL_PLATFORM_WINDOWS__
    DWORD error = GetLastError();
    if (error == 0) {
        return "Success";
    }
    
    char* buffer = nullptr;
    DWORD length = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<char*>(&buffer),
        0,
        nullptr
    );
    
    if (length == 0) {
        return "Unknown error";
    }
    
    string message(buffer, length);
    LocalFree(buffer);

    while (!message.empty() && 
           (message.back() == '\n' || message.back() == '\r')) {
        message.pop_back();
    }
    
    return message;
#else
    return ::strerror(errno);
#endif
}

bool device_utils::is_special_file(const string& path) {
#ifdef MSTL_PLATFORM_WINDOWS__
    return false;
#else
    struct ::stat64 st{};
    if (::stat64(path.c_str(), &st) != 0) {
        return false;
    }
    return S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode) ||
           S_ISFIFO(st.st_mode) || S_ISSOCK(st.st_mode);
#endif
}

string device_utils::normalize_device_path(const string& path) {
    string normalized = path;

#ifdef MSTL_PLATFORM_WINDOWS__
    if (normalized.find("\\\\.\\") != 0) {
        if (normalized.find("COM") == 0 || normalized.find("LPT") == 0) {
            normalized = "\\\\.\\" + normalized;
        } else if (normalized.find("PhysicalDrive") == 0) {
            normalized = "\\\\.\\" + normalized;
        }
    }

    transform(normalized.begin(), normalized.end(), normalized.begin(), ::toupper);
#else
    char resolved_path[PATH_MAX];
    if (::realpath(path.c_str(), resolved_path)) {
        normalized = resolved_path;
    }

    if (normalized.find("/dev/") != 0) {
        struct ::stat64 st{};
        if (::stat64(normalized.c_str(), &st) == 0) {
            if (S_ISBLK(st.st_mode) || S_ISCHR(st.st_mode)) {
                ::DIR* dev_dir = ::opendir("/dev");
                if (dev_dir) {
                    ::dirent* entry;
                    while ((entry = readdir(dev_dir)) != nullptr) {
                        string dev_name = entry->d_name;
                        if (dev_name == "." || dev_name == "..") {
                            continue;
                        }
                        string dev_path = "/dev/" + dev_name;
                        struct ::stat64 dev_st{};
                        if (::stat64(dev_path.c_str(), &dev_st) == 0) {
                            if (dev_st.st_rdev == st.st_rdev) {
                                normalized = dev_path;
                                break;
                            }
                        }
                    }
                    ::closedir(dev_dir);
                }
            }
        }
    }
#endif
    return normalized;
}

bool device_utils::is_serial_port_name(const string& name) {
    if (name.find("tty") == 0) {
        return true;
    }
    if (name.find("COM") == 0) {
        return true;
    }
    if (name.find("serial") != string::npos) {
        return true;
    }

    vector<string_view> patterns = {
        "ttyS", "ttyUSB", "ttyACM", "ttyAMA", "cu.", "COM"
    };
    for (const auto& pattern : patterns) {
        if (name.find(pattern) != string::npos) {
            return true;
        }
    }
    return false;
}

string device_utils::generate_device_id(const device_info& info) {
    string result;

    result += "type:" + to_string(info.type);
    result += ":path:" + info.device_path;

    if (!info.hardware_id.empty()) {
        result += ":hwid:" + info.hardware_id;
    }
    if (info.vendor_id > 0 || info.product_id > 0) {
        result += format(":vid:{:04x}", info.vendor_id);
        result += format(":pid:{:04x}", info.vendor_id);
    }
    if (info.size_bytes > 0) {
        result += ":size:" + to_string(info.size_bytes);
    }
    if (info.device_id > 0) {
        result += ":devid:" + to_string(info.device_id);
    }

    const size_t hash_value = hash<string>()(result);
    return format("dev_{:016x}", static_cast<uint64_t>(hash_value));
}

MSTL_END_NAMESPACE__
