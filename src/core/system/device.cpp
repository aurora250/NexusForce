#include <MSTL/core/system/device/device.hpp>
#ifdef MSTL_PLATFORM_WINDOWS__
#include <winioctl.h>
#include <devguid.h>
#endif
#ifdef MSTL_PLATFORM_LINUX__
#include <MSTL/core/algorithm/remove.hpp>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <libudev.h>
#include <cstring>
#include <cerrno>
#endif
MSTL_BEGIN_NAMESPACE__

#ifdef DEVICE_TYPE
#undef DEVICE_TYPE
#endif

string to_string(const DEVICE_TYPE type) {
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

device::device(path device_path,
    const DEVICE_OPEN_MODE mode, const DEVICE_OPEN_FLAG flags) {
    open(move(device_path), mode, flags);
}

device::device(device&& other) noexcept
    : file_(move(other.file_)),
      device_type_(other.device_type_),
      timeout_(other.timeout_),
      is_blocking_(other.is_blocking_) {}

device& device::operator =(device&& other) noexcept {
    if (this != &other) {
        close();
        lock_guard<mutex> lock(io_mutex_);
        file_ = move(other.file_);
        device_type_ = other.device_type_;
        timeout_ = other.timeout_;
        is_blocking_ = other.is_blocking_;
    }
    return *this;
}

device::~device() {
    close();
}

FILE_ACCESS device::dmode_to_faccess(const DEVICE_OPEN_MODE mode) const {
    switch (mode) {
        case DEVICE_OPEN_MODE::READ:
            return FILE_ACCESS::READ;
        case DEVICE_OPEN_MODE::WRITE:
            return FILE_ACCESS::WRITE;
        case DEVICE_OPEN_MODE::READ_WRITE:
            return FILE_ACCESS::READ_WRITE;
        default:
            return FILE_ACCESS::READ_WRITE;
    }
}

FILE_CREATION device::dflags_to_fcreation(const DEVICE_OPEN_FLAG flags) const {
    if ((flags & DEVICE_OPEN_FLAG::CREATE) != DEVICE_OPEN_FLAG::NONE) {
        if ((flags & DEVICE_OPEN_FLAG::EXCLUSIVE) != DEVICE_OPEN_FLAG::NONE) {
            return FILE_CREATION::CREATE_NO_EXIST;
        } else {
            return FILE_CREATION::CREATE_FORCE;
        }
    } else {
        return FILE_CREATION::OPEN_EXIST;
    }
}

FILE_ATTRI device::dflags_to_fattri(DEVICE_OPEN_FLAG flags) const {
    auto attr = FILE_ATTRI::NORMAL;
#ifdef MSTL_PLATFORM_WINDOWS__
    if ((flags & DEVICE_OPEN_FLAG::ASYNC) != DEVICE_OPEN_FLAG::NONE) {
        attr = attr | FILE_ATTRI::OVERLAPPED;
    }
    if ((flags & DEVICE_OPEN_FLAG::DIRECT_IO) != DEVICE_OPEN_FLAG::NONE) {
        attr = attr | FILE_ATTRI::NO_BUFFERING;
    }
    if ((flags & DEVICE_OPEN_FLAG::SYNC) != DEVICE_OPEN_FLAG::NONE) {
        attr = attr | FILE_ATTRI::WRITE_THROUGH;
    }
#elif defined(MSTL_PLATFORM_LINUX__)
    if ((flags & DEVICE_OPEN_FLAG::DIRECT_IO) != DEVICE_OPEN_FLAG::NONE) {
        attr = attr | FILE_ATTRI::NO_BUFFERING;
    }
    if ((flags & DEVICE_OPEN_FLAG::SYNC) != DEVICE_OPEN_FLAG::NONE) {
        attr = attr | FILE_ATTRI::WRITE_THROUGH;
    }
#endif
    return attr;
}

DEVICE_TYPE device::try_device_type(const path& pth) {
#ifdef MSTL_PLATFORM_WINDOWS__
    if (pth.view().find("\\\\.\\COM") != string::npos) {
        return DEVICE_TYPE::SERIAL_PORT;
    } else if (pth.view().find("\\\\.\\PhysicalDrive") != string::npos) {
        return DEVICE_TYPE::STORAGE;
    } else if (pth.view().find("\\\\.\\HID") == 0) {
        return DEVICE_TYPE::HID;
    }

    const ::HDEVINFO dev_info_set = ::SetupDiGetClassDevsA(
        nullptr,
        nullptr,
        nullptr,
        DIGCF_PRESENT | DIGCF_ALLCLASSES
    );
    if (dev_info_set != INVALID_HANDLE_VALUE) {
        ::SP_DEVINFO_DATA dev_info_data;
        dev_info_data.cbSize = sizeof(::SP_DEVINFO_DATA);

        for (::DWORD i = 0; ::SetupDiEnumDeviceInfo(dev_info_set, i, &dev_info_data); i++) {
            ::DWORD size = 0;
            ::SetupDiGetDeviceInstanceIdA(dev_info_set, &dev_info_data, nullptr, 0, &size);

            if (size > 0) {
                vector<char> buffer(size);
                if (::SetupDiGetDeviceInstanceIdA(dev_info_set,
                    &dev_info_data, buffer.data(), size, nullptr)) {
                    string instance_id = buffer.data();

                    if (pth.str().find(instance_id) != string::npos) {
                        ::GUID class_guid;
                        if (::SetupDiGetDeviceInfoListClass(dev_info_set, &class_guid)) {
                            const DEVICE_TYPE type = try_device_type_from_guid(class_guid);
                            ::SetupDiDestroyDeviceInfoList(dev_info_set);
                            return type;
                        }
                    }
                }
            }
        }
        ::SetupDiDestroyDeviceInfoList(dev_info_set);
    }
    return DEVICE_TYPE::GENERIC;
#else
    return guess_device_type_from_path(pth);
#endif
}

#ifdef MSTL_PLATFORM_WINDOWS__
DEVICE_TYPE device::try_device_type_from_guid(const GUID& guid) {
    if (::IsEqualGUID(guid, ::GUID_DEVCLASS_PORTS)) {
        return DEVICE_TYPE::SERIAL_PORT;
    } else if (::IsEqualGUID(guid, ::GUID_DEVCLASS_DISKDRIVE) ||
        ::IsEqualGUID(guid, ::GUID_DEVCLASS_CDROM) ||
        ::IsEqualGUID(guid, ::GUID_DEVCLASS_VOLUME)) {
        return DEVICE_TYPE::STORAGE;
    } else if (::IsEqualGUID(guid, ::GUID_DEVCLASS_HIDCLASS)) {
        return DEVICE_TYPE::HID;
    } else if (::IsEqualGUID(guid, ::GUID_DEVCLASS_NET)) {
        return DEVICE_TYPE::NETWORK;
    } else if (::IsEqualGUID(guid, ::GUID_DEVCLASS_MEDIA)) {
        return DEVICE_TYPE::AUDIO;
    } else if (::IsEqualGUID(guid, ::GUID_DEVCLASS_IMAGE) ||
        ::IsEqualGUID(guid, ::GUID_DEVCLASS_CAMERA)) {
        return DEVICE_TYPE::VIDEO;
    }
    return DEVICE_TYPE::GENERIC;
}

string device::get_device_property(const ::HDEVINFO dev_info_set,
    const ::PSP_DEVINFO_DATA dev_info_data, const ::DWORD property) {
    ::DWORD data_type;
    ::DWORD buffer_size = 0;

    ::SetupDiGetDeviceRegistryPropertyA(dev_info_set,
        dev_info_data, property, &data_type, nullptr, 0, &buffer_size);
    if (buffer_size == 0) {
        return "";
    }

    vector<char> buffer(buffer_size);
    if (::SetupDiGetDeviceRegistryPropertyA(
        dev_info_set, dev_info_data, property, &data_type,
        reinterpret_cast<::PBYTE>(buffer.data()), buffer_size, nullptr)) {
        return string(buffer.data());
    }
    return "";
}
#else
DEVICE_TYPE device::guess_device_type_from_path(const path& pth) {
    if (pth.str().find("/dev/tty") != string::npos ||
        pth.str().find("/dev/serial") != string::npos) {
        return DEVICE_TYPE::SERIAL_PORT;
    } else if (pth.str().find("/dev/sd") != string::npos ||
        pth.str().find("/dev/hd") != string::npos ||
        pth.str().find("/dev/nvme") != string::npos ||
        pth.str().find("/dev/mmcblk") != string::npos ||
        pth.str().find("/dev/loop") != string::npos) {
        return DEVICE_TYPE::STORAGE;
    } else if (pth.str().find("/dev/input") != string::npos ||
        pth.str().find("/dev/hidraw") != string::npos) {
        return DEVICE_TYPE::HID;
    } else if (pth.str().find("/dev/snd") != string::npos ||
        pth.str().find("/dev/audio") != string::npos ||
        pth.str().find("/dev/dsp") != string::npos) {
        return DEVICE_TYPE::AUDIO;
    } else if (pth.str().find("/dev/video") != string::npos) {
        return DEVICE_TYPE::VIDEO;
    } else if (pth.str().find("/dev/net") != string::npos) {
        return DEVICE_TYPE::NETWORK;
    }

    struct ::stat64 st;
    if (::stat64(pth.c_str(), &st) == 0) {
        if (S_ISBLK(st.st_mode)) {
            return DEVICE_TYPE::STORAGE;
        } else if (S_ISCHR(st.st_mode)) {
            return DEVICE_TYPE::GENERIC;
        }
    }
    return DEVICE_TYPE::UNKNOWN;
}

string device::read_sysfs_attribute(const string& device_path, const string& attribute) {
    const path sysfs_path("/sys/class/" + device_path + "/" + attribute);
    file f;
    if (f.open(sysfs_path, false, FILE_ACCESS::READ,
        FILE_SHARED::SHARE_READ, FILE_CREATION::OPEN_EXIST)) {
        return f.read();
    }
    return "";
}
#endif

void device::open(const path& device_path,
    const DEVICE_OPEN_MODE mode, const DEVICE_OPEN_FLAG flags) {
    lock_guard<mutex> lock(io_mutex_);
    if (file_.is_opened()) {
        close();
    }

    const FILE_ACCESS access = dmode_to_faccess(mode);
    const FILE_CREATION creation = dflags_to_fcreation(flags);
    const FILE_ATTRI attr = dflags_to_fattri(flags);

    const bool success = file_.open(device_path, false, access,
        FILE_SHARED::SHARE_READ_WRITE, creation, attr);

    if (!success) {
        throw_exception(device_exception(
            ("Failed to open device: " + file_.path().str() + " - " + file_.last_error()).data()));
    }
    device_type_ = try_device_type(file_.path());
#ifdef MSTL_PLATFORM_LINUX__
    if (!is_blocking_) {
        const int fcntl_flag = ::fcntl(file_.native_handle(), F_GETFL, 0);
        ::fcntl(file_.native_handle(), F_SETFL, fcntl_flag | O_NONBLOCK);
    }
#endif
}

void device::close() noexcept {
    lock_guard<mutex> lock(io_mutex_);
    if (file_.is_opened()) {
        file_.close();
    }
    device_type_ = DEVICE_TYPE::UNKNOWN;
}

bool device::is_open() const noexcept {
    return file_.is_opened();
}

void device::reopen(const DEVICE_OPEN_MODE new_mode,
    const DEVICE_OPEN_FLAG new_flags) {
    close();
    open(file_.path(), new_mode, new_flags);
}

size_t device::read(void* buffer, const size_t size,
    const milliseconds timeout) {
    if (!buffer || size == 0) {
        return 0;
    }
    lock_guard<mutex> lock(io_mutex_);
    if (!file_.is_opened()) {
        return 0;
    }

    milliseconds actual_timeout = (timeout.count() < 0) ? timeout_ : timeout;
    if (actual_timeout.count() > 0 && !is_readable(actual_timeout)) {
        return 0;
    }

    string tmp;
    const size_t read_bytes = file_.read_binary(tmp, size);
    if (read_bytes > 0) {
        memory_copy(buffer, tmp.data(), read_bytes);
    }
    return read_bytes;
}

size_t device::write(const void* buffer, const size_t size,
    const milliseconds timeout) {
    if (!buffer || size == 0) {
        return 0;
    }
    lock_guard<mutex> lock(io_mutex_);
    if (!file_.is_opened()) {
        return 0;
    }

    const milliseconds actual_timeout = (timeout.count() < 0) ? timeout_ : timeout;
    if (actual_timeout.count() > 0 && !is_writable(actual_timeout)) {
        return 0;
    }
    const string data(static_cast<const char*>(buffer), size);
    return file_.write(data, size);
}

device::async_result device::async_read(string& buffer,
    const size_t size, const int64_t offset) {
    lock_guard<mutex> lock(io_mutex_);
    if (!file_.is_opened()) {
        return async_result{false, 0, -1};
    }
    return file_.async_read(buffer, size, offset);
}

device::async_result device::async_write(const string& data,
    const size_t size, const int64_t offset) {
    lock_guard<mutex> lock(io_mutex_);
    if (!file_.is_opened()) {
        return async_result{false, 0, -1};
    }
    return file_.async_write(data, size, offset);
}

bool device::wait_async(async_result& result, const uint32_t timeout_ms) {
    return file_.wait_async(result, timeout_ms);
}

void device::cancel_async(async_result& result) {
    file_.cancel_async(result);
}

void device::ioctl(const ioctl_command& cmd) {
    lock_guard<mutex> lock(io_mutex_);
    if (!file_.is_opened()) {
        throw_exception(device_exception("Device not opened"));
    }

#ifdef MSTL_PLATFORM_WINDOWS__
    ::DWORD bytes_returned = 0;
    const ::BOOL success = ::DeviceIoControl(
        file_.native_handle(),
        cmd.code(),
        const_cast<void*>(cmd.in_data()),
        static_cast<::DWORD>(cmd.in_size()),
        cmd.out_data(),
        static_cast<::DWORD>(cmd.out_size()),
        &bytes_returned,
        nullptr
    );

    if (!success) {
        const ::DWORD error = ::GetLastError();
        throw_exception(device_exception(
            ("DeviceIoControl failed with error code: " + _MSTL to_string(error)).data()));
    }

#elif defined(MSTL_PLATFORM_LINUX__)
    int ret;

    if (cmd.in_data() != nullptr && cmd.out_data() != nullptr) {
        ret = ::ioctl(file_.native_handle(), cmd.code(), cmd.out_data());
    } else if (cmd.out_data() != nullptr) {
        ret = ::ioctl(file_.native_handle(), cmd.code(), cmd.out_data());
    } else if (cmd.in_data() != nullptr) {
        ret = ::ioctl(file_.native_handle(), cmd.code(), cmd.in_data());
    } else {
        ret = ::ioctl(file_.native_handle(), cmd.code());
    }

    if (ret == -1) {
        throw_exception(device_exception(
            ("ioctl failed with error code: " + string(::strerror(errno))).c_str()));
    }
#endif
}

void device::flush() {
    lock_guard<mutex> lock(io_mutex_);
    MSTL_IGNORE file_.flush();
}

void device::sync() noexcept {
    lock_guard<mutex> lock(io_mutex_);
#ifdef MSTL_PLATFORM_LINUX__
    if (file_.is_opened()) {
        ::fsync(file_.native_handle());
    }
#else
    MSTL_IGNORE file_.flush();
#endif
}

bool device::wait(DEVICE_IO_DIRECT direction, milliseconds timeout) const {
    if (!file_.is_opened()) {
        return false;
    }
#ifdef MSTL_PLATFORM_WINDOWS__
    const ::DWORD wait_time = (timeout.count() < 0) ?
        numeric_traits<::DWORD>::max() : static_cast<::DWORD>(timeout.count());
    const ::DWORD result = ::WaitForSingleObject(file_.native_handle(), wait_time);
    return result == WAIT_OBJECT_0;
#elif defined(MSTL_PLATFORM_LINUX__)
    ::fd_set fds;
    FD_ZERO(&fds);
    FD_SET(file_.native_handle(), &fds);

    ::timeval tv;
    tv.tv_sec = timeout.count() / 1000;
    tv.tv_usec = (timeout.count() % 1000) * 1000;

    int ret;
    if (direction == DEVICE_IO_DIRECT::READ) {
        ret = ::select(file_.native_handle() + 1, &fds, nullptr, nullptr,
            timeout.count() < 0 ? nullptr : &tv);
    } else {
        ret = ::select(file_.native_handle() + 1, nullptr, &fds, nullptr,
            timeout.count() < 0 ? nullptr : &tv);
    }
    return ret > 0;
#endif
}

bool device::is_readable(const milliseconds timeout) const {
    return wait(DEVICE_IO_DIRECT::READ, timeout);
}

bool device::is_writable(const milliseconds timeout) const {
    return wait(DEVICE_IO_DIRECT::WRITE, timeout);
}

void device::set_timeout(const milliseconds timeout) {
    lock_guard<mutex> lock(io_mutex_);
    timeout_ = timeout;
}

void device::set_blocking(const bool blocking) {
    lock_guard<mutex> lock(io_mutex_);
    is_blocking_ = blocking;
#ifdef MSTL_PLATFORM_LINUX__
    if (file_.is_opened()) {
        const int flags = ::fcntl(file_.native_handle(), F_GETFL, 0);
        if (blocking) {
            ::fcntl(file_.native_handle(), F_SETFL, flags & ~O_NONBLOCK);
        } else {
            ::fcntl(file_.native_handle(), F_SETFL, flags | O_NONBLOCK);
        }
    }
#endif
}

_MSTL device_info device::device_info() const {
    _MSTL device_info info;
    info.path = file_.path();
    info.type = device_type_;
    info.present = file_.is_opened();

#ifdef MSTL_PLATFORM_WINDOWS__
    if (!file_.is_opened()) {
        return info;
    }

    if (device_type_ == DEVICE_TYPE::STORAGE) {
        ::DISK_GEOMETRY_EX geometry = {0};
        ::DWORD bytes_returned = 0;

        if (::DeviceIoControl(
            file_.native_handle(),
            IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
            nullptr, 0,
            &geometry, sizeof(geometry),
            &bytes_returned,
            nullptr)) {
            info.size_bytes = static_cast<uint64_t>(geometry.DiskSize.QuadPart);
            info.block_size = geometry.Geometry.BytesPerSector;
        }

        ::STORAGE_HOTPLUG_INFO hotplug_info = {};
        if (::DeviceIoControl(
            file_.native_handle(),
            IOCTL_STORAGE_GET_HOTPLUG_INFO,
            nullptr, 0,
            &hotplug_info, sizeof(hotplug_info),
            &bytes_returned,
            nullptr)) {
            info.removable = hotplug_info.MediaRemovable || hotplug_info.DeviceHotplug;
        }
    }

    ::HDEVINFO dev_info_set = ::SetupDiGetClassDevsA(
        nullptr,
        nullptr,
        nullptr,
        DIGCF_PRESENT | DIGCF_ALLCLASSES
    );

    if (dev_info_set != INVALID_HANDLE_VALUE) {
        ::SP_DEVINFO_DATA dev_info_data;
        dev_info_data.cbSize = sizeof(::SP_DEVINFO_DATA);

        for (::DWORD i = 0; ::SetupDiEnumDeviceInfo(dev_info_set, i, &dev_info_data); i++) {
            ::DWORD size = 0;
            ::SetupDiGetDeviceInstanceIdA(dev_info_set, &dev_info_data, nullptr, 0, &size);

            if (size > 0) {
                vector<char> buffer(size);
                if (::SetupDiGetDeviceInstanceIdA(
                    dev_info_set, &dev_info_data, buffer.data(), size, nullptr)) {
                    string instance_id = buffer.data();

                    if (file_.path().str().find(instance_id) != string::npos ||
                        instance_id.find(file_.path().str()) != string::npos) {

                        info.hardware_id = get_device_property(
                            dev_info_set, &dev_info_data, SPDRP_HARDWAREID);
                        info.description = get_device_property(
                            dev_info_set, &dev_info_data, SPDRP_DEVICEDESC);
                        info.manufacturer = get_device_property(
                            dev_info_set, &dev_info_data, SPDRP_MFG);
                        info.friendly_name = get_device_property(
                            dev_info_set, &dev_info_data, SPDRP_FRIENDLYNAME);

                        if (!info.hardware_id.empty()) {
                            size_t vid_pos = info.hardware_id.find("VID_");
                            size_t pid_pos = info.hardware_id.find("PID_");

                            if (vid_pos != string::npos) {
                                string_view vid_str = info.hardware_id.view(vid_pos + 4, 4);
                                info.vendor_id = static_cast<uint16_t>(to_uint64(vid_str, nullptr, 16));
                            }
                            if (pid_pos != string::npos) {
                                string_view pid_str = info.hardware_id.view(pid_pos + 4, 4);
                                info.product_id = static_cast<uint16_t>(to_uint64(pid_str, nullptr, 16));
                            }
                        }
                        break;
                    }
                }
            }
        }
        ::SetupDiDestroyDeviceInfoList(dev_info_set);
    }

#elif defined(MSTL_PLATFORM_LINUX__)
    struct ::stat64 st;
    if (::stat64(file_.path().c_str(), &st) == 0) {
        info.device_id = static_cast<uint32_t>(st.st_rdev);
        info.present = true;

        if (S_ISBLK(st.st_mode)) {
            if (info.type == DEVICE_TYPE::UNKNOWN || info.type == DEVICE_TYPE::GENERIC) {
                info.type = DEVICE_TYPE::STORAGE;
            }

            if (file_.is_opened()) {
                uint64_t size = 0;
                if (::ioctl(file_.native_handle(), BLKGETSIZE64, &size) == 0) {
                    info.size_bytes = size;
                }
                int block_size = 0;
                if (::ioctl(file_.native_handle(), BLKSSZGET, &block_size) == 0) {
                    info.block_size = static_cast<uint32_t>(block_size);
                }
            }
        } else if (S_ISCHR(st.st_mode)) {
            if (info.type == DEVICE_TYPE::UNKNOWN) {
                info.type = DEVICE_TYPE::GENERIC;
            }
        }

        string_view device_name = file_.path().filename();
        path sysfs_base("/sys");

        switch (device_type_) {
            case DEVICE_TYPE::STORAGE: {
                sysfs_base = sysfs_base / "block" / device_name;
                break;
            }
            case DEVICE_TYPE::SERIAL_PORT: {
                sysfs_base = sysfs_base / "class/tty" / device_name;
                break;
            }
            case DEVICE_TYPE::HID: {
                sysfs_base = sysfs_base / "class/hidraw" / device_name;
                break;
            }
            case DEVICE_TYPE::VIDEO: {
                sysfs_base = sysfs_base / "class/video4linux" / device_name;
                break;
            }
            case DEVICE_TYPE::AUDIO: {
                sysfs_base = sysfs_base / "class/sound" / device_name;
                break;
            }
            case DEVICE_TYPE::NETWORK: {
                sysfs_base = sysfs_base / "class/net" / device_name;
                break;
            }
            default: {
                sysfs_base = path("");
                break;
            }
        }

        auto read_sysfs = [](const path& path) -> string {
            file f;
            if (f.open(path, false, FILE_ACCESS::READ,
                FILE_SHARED::SHARE_READ, FILE_CREATION::OPEN_EXIST)) {
                string content = f.read();
                content.erase(remove(content.begin(), content.end(), '\n'), content.end());
                return content;
            }
            return "";
        };

        if (device_type_ == DEVICE_TYPE::STORAGE && !sysfs_base.empty()) {
            string removable_str = read_sysfs(sysfs_base / "removable");
            info.removable = (removable_str == "1");

            if (info.size_bytes == 0) {
                string size_str = read_sysfs(sysfs_base / "size");
                if (!size_str.empty()) {
                    try {
                        uint64_t sectors = to_uint64(size_str.view());
                        info.size_bytes = sectors * info.block_size;
                    } catch (...) {}
                }
            }
        }

        ::DIR* usb_dir = ::opendir("/sys/bus/usb/devices");
        if (usb_dir) {
            ::dirent* entry;
            while ((entry = ::readdir(usb_dir)) != nullptr) {
                if (entry->d_name[0] == '.') continue;

                path usb_device_path("/sys/bus/usb/devices/"_s + entry->d_name);
                path check_path = usb_device_path / "tty" / device_name;
                struct ::stat64 check_st;
                if (::stat64(check_path.c_str(), &check_st) == 0) {
                    string vid_str = read_sysfs(usb_device_path / "idVendor");
                    string pid_str = read_sysfs(usb_device_path / "idProduct");
                    if (!vid_str.empty() && !pid_str.empty()) {
                        try {
                            info.vendor_id = to_uint16(vid_str.view(), nullptr, 16);
                            info.product_id = to_uint16(pid_str.view(), nullptr, 16);
                        } catch (...) {}
                    }
                    info.manufacturer = read_sysfs(usb_device_path / "manufacturer");
                    info.description = read_sysfs(usb_device_path / "product");
                    break;
                }
            }
            ::closedir(usb_dir);
        }
        if (info.friendly_name.empty()) {
            info.friendly_name = device_name;
        }
    }
#endif
    return info;
}

void* device::map_memory(const size_t offset, const size_t size) {
    lock_guard<mutex> lock(io_mutex_);
    if (!file_.is_opened()) {
        return nullptr;
    }
    if (!file_.map(offset, size, FILE_ACCESS::READ_WRITE)) {
        return nullptr;
    }
    return file_.mapped_data();
}

void device::unmap_memory() noexcept {
    lock_guard<mutex> lock(io_mutex_);
    if (file_.is_mapped()) {
        file_.unmap();
    }
}

bool device::supports_direct_io() const noexcept {
#ifdef MSTL_PLATFORM_LINUX__
    return device_type_ == DEVICE_TYPE::STORAGE;
#else
    return false;
#endif
}

vector<_MSTL device_info> device::enumerate(const string& filter) {
    vector<_MSTL device_info> devices;

#ifdef MSTL_PLATFORM_WINDOWS__
    const ::HDEVINFO dev_info_set = ::SetupDiGetClassDevsA(
        nullptr,
        nullptr,
        nullptr,
        DIGCF_PRESENT | DIGCF_ALLCLASSES
    );

    if (dev_info_set == INVALID_HANDLE_VALUE) {
        return devices;
    }

    ::SP_DEVINFO_DATA dev_info_data;
    dev_info_data.cbSize = sizeof(::SP_DEVINFO_DATA);

    for (::DWORD i = 0; ::SetupDiEnumDeviceInfo(dev_info_set, i, &dev_info_data); i++) {
        _MSTL device_info info;

        ::DWORD size = 0;
        ::SetupDiGetDeviceInstanceIdA(dev_info_set, &dev_info_data, nullptr, 0, &size);
        if (size > 0) {
            vector<char> buffer(size);
            if (::SetupDiGetDeviceInstanceIdA(
                dev_info_set, &dev_info_data, buffer.data(), size, nullptr)) {
                info.path = path(buffer.data());
            }
        }
        info.description = get_device_property(dev_info_set, &dev_info_data, SPDRP_DEVICEDESC);
        info.manufacturer = get_device_property(dev_info_set, &dev_info_data, SPDRP_MFG);

        ::GUID class_guid;
        if (::SetupDiGetDeviceInfoListClass(dev_info_set, &class_guid)) {
            info.type = try_device_type_from_guid(class_guid);
        }

        if (!filter.empty() && info.path.str().find(filter) == string::npos) {
            continue;
        }

        devices.push_back(move(info));
    }

    ::SetupDiDestroyDeviceInfoList(dev_info_set);

#elif defined(MSTL_PLATFORM_LINUX__)
    ::DIR* dir = ::opendir("/dev");
    if (!dir) {
        return devices;
    }

    ::dirent* entry;
    while ((entry = ::readdir(dir)) != nullptr) {
        if (entry->d_name[0] == '.') {
            continue;
        }

        path dev_path("/dev/"_s + entry->d_name);

        struct ::stat64 st;
        if (::stat64(dev_path.c_str(), &st) != 0) {
            continue;
        }

        if (!S_ISCHR(st.st_mode) && !S_ISBLK(st.st_mode)) {
            continue;
        }

        _MSTL device_info info;
        info.path = dev_path;
        info.type = guess_device_type_from_path(dev_path);
        info.device_id = st.st_rdev;

        if (!filter.empty() && info.path.str().find(filter) == string::npos) {
            continue;
        }
        devices.push_back(info);
    }
    ::closedir(dir);
#endif
    return devices;
}

vector<_MSTL device_info> device::find_by_vid_pid(uint16_t vid, uint16_t pid) {
    vector<_MSTL device_info> devices;

#ifdef MSTL_PLATFORM_WINDOWS__
    const ::HDEVINFO dev_info_set = ::SetupDiGetClassDevsA(
        &GUID_DEVCLASS_USB,
        nullptr,
        nullptr,
        DIGCF_PRESENT
    );
    if (dev_info_set == INVALID_HANDLE_VALUE) {
        return devices;
    }

    ::SP_DEVINFO_DATA dev_info_data;
    dev_info_data.cbSize = sizeof(::SP_DEVINFO_DATA);

    for (::DWORD i = 0; ::SetupDiEnumDeviceInfo(dev_info_set, i, &dev_info_data); i++) {
        string hardware_id = get_device_property(dev_info_set, &dev_info_data, SPDRP_HARDWAREID);
        string hwid_upper = hardware_id;
        transform(hwid_upper.begin(), hwid_upper.end(), hwid_upper.begin(), ::toupper);

        string vid_pattern = format("VID_{04X}", vid);
        string pid_pattern = format("PID_{04X}", pid);

        if (!hwid_upper.empty() &&
            hwid_upper.find(vid_pattern) != string::npos &&
            hwid_upper.find(pid_pattern) != string::npos) {

            _MSTL device_info info;

            ::DWORD size = 0;
            ::SetupDiGetDeviceInstanceIdA(dev_info_set, &dev_info_data, nullptr, 0, &size);
            if (size > 0) {
                vector<char> buf(size);
                if (::SetupDiGetDeviceInstanceIdA(dev_info_set, &dev_info_data, buf.data(), size, nullptr)) {
                    info.path = path(buf.data());
                }
            }

            info.hardware_id = hardware_id;
            info.description = get_device_property(dev_info_set, &dev_info_data, SPDRP_DEVICEDESC);
            info.manufacturer = get_device_property(dev_info_set, &dev_info_data, SPDRP_MFG);
            info.friendly_name = get_device_property(dev_info_set, &dev_info_data, SPDRP_FRIENDLYNAME);
            info.vendor_id = vid;
            info.product_id = pid;
            info.type = DEVICE_TYPE::GENERIC;

            const ::HKEY dev_key = ::SetupDiOpenDevRegKey(dev_info_set,
                &dev_info_data, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
            if (dev_key != INVALID_HANDLE_VALUE) {
                char port_name[256] = {};
                ::DWORD port_name_size = sizeof(port_name);
                ::DWORD type = 0;
                if (::RegQueryValueExA(dev_key, "PortName", nullptr, &type,
                    reinterpret_cast<::LPBYTE>(port_name), &port_name_size) == ERROR_SUCCESS) {
                    info.friendly_name = port_name;
                    info.path = path("\\\\.\\"_s + port_name);
                }
                ::RegCloseKey(dev_key);
            }
            devices.push_back(move(info));
        }
    }
    ::SetupDiDestroyDeviceInfoList(dev_info_set);

#elif defined(MSTL_PLATFORM_LINUX__)
    ::DIR* usb_devices_dir = ::opendir("/sys/bus/usb/devices");
    if (!usb_devices_dir) {
        return devices;
    }

    ::dirent* entry;
    while ((entry = ::readdir(usb_devices_dir)) != nullptr) {
        if (entry->d_name[0] == '.') {
            continue;
        }

        path usb_dev_path("/sys/bus/usb/devices/"_s + entry->d_name);
        path vid_path = usb_dev_path / "idVendor";
        path pid_path = usb_dev_path / "idProduct";
        file vid_file, pid_file;
        if (!vid_file.open(vid_path, false, FILE_ACCESS::READ, FILE_SHARED::SHARE_READ, FILE_CREATION::OPEN_EXIST) ||
            !pid_file.open(pid_path, false, FILE_ACCESS::READ, FILE_SHARED::SHARE_READ, FILE_CREATION::OPEN_EXIST)) {
            continue;
        }

        string vid_str = vid_file.read();
        string pid_str = pid_file.read();
        vid_str.erase(remove(vid_str.begin(), vid_str.end(), '\n'), vid_str.end());
        pid_str.erase(remove(pid_str.begin(), pid_str.end(), '\n'), pid_str.end());
        uint16_t device_vid = 0, device_pid = 0;
        try {
            device_vid = to_uint16(vid_str.view(), nullptr, 16);
            device_pid = to_uint16(pid_str.view(), nullptr, 16);
        } catch (...) {
            continue;
        }

        if (device_vid == vid && device_pid == pid) {
            _MSTL device_info info;
            info.vendor_id = vid;
            info.product_id = pid;
            info.type = DEVICE_TYPE::GENERIC;
            info.path = usb_dev_path;
            info.present = true;

            auto read_sysfs_file = [](const path& pth) -> string {
                file f;
                if (f.open(pth, false, FILE_ACCESS::READ, FILE_SHARED::SHARE_READ, FILE_CREATION::OPEN_EXIST)) {
                    string s = f.read();
                    s.erase(remove(s.begin(), s.end(), '\n'), s.end());
                    return s;
                }
                return "";
            };

            info.description = read_sysfs_file(usb_dev_path / "product");
            info.manufacturer = read_sysfs_file(usb_dev_path / "manufacturer");
            info.hardware_id = read_sysfs_file(usb_dev_path / "serial");
            ::DIR* dev_dir = ::opendir(usb_dev_path.c_str());
            if (dev_dir) {
                ::dirent* dev_entry;
                while ((dev_entry = ::readdir(dev_dir)) != nullptr) {
                    string_view dname = dev_entry->d_name;
                    if (dname.find("tty") == 0) {
                        _MSTL device_info tty_info = info;
                        tty_info.path = path("/dev") / dname;
                        tty_info.friendly_name = dname;
                        tty_info.type = DEVICE_TYPE::SERIAL_PORT;
                        devices.push_back(move(tty_info));
                    }
                }
                ::closedir(dev_dir);
            }
            else {
                devices.push_back(move(info));
            }
        }
    }
    ::closedir(usb_devices_dir);
#endif
    return devices;
}

optional<_MSTL device_info> device::find_by_path(path pth) {
    if (pth.empty()) {
        return {};
    }

    _MSTL device_info info;
    info.path = move(pth);
    info.present = info.path.exists();

#ifdef MSTL_PLATFORM_WINDOWS__
    if (!info.present) {
        return {};
    }

    const ::HANDLE handle = ::CreateFileA(
        info.path.c_str(),
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr);

    if (handle == INVALID_HANDLE_VALUE) {
        return {};
    }
    ::CloseHandle(handle);

    if (info.path.str().find("\\\\.\\COM") == 0) {
        info.type = DEVICE_TYPE::SERIAL_PORT;
    } else if (info.path.str().find("\\\\.\\PhysicalDrive") == 0) {
        info.type = DEVICE_TYPE::STORAGE;
    } else {
        info.type = DEVICE_TYPE::GENERIC;
    }

#elif defined(MSTL_PLATFORM_LINUX__)
    struct ::stat64 st;
    if (::stat64(pth.c_str(), &st) < 0) {
        return {};
    }

    info.device_id = static_cast<uint32_t>(st.st_rdev);

    if (S_ISCHR(st.st_mode)) {
        info.type = DEVICE_TYPE::GENERIC;
    } else if (S_ISBLK(st.st_mode)) {
        info.type = DEVICE_TYPE::STORAGE;
    } else {
        info.type = DEVICE_TYPE::GENERIC;
    }
#endif
    device dev;
    try {
        dev.open(info.path, DEVICE_OPEN_MODE::READ, DEVICE_OPEN_FLAG::NONE);
        const auto full_info = dev.device_info();
        info.description = full_info.description;
        info.manufacturer = full_info.manufacturer;
        info.friendly_name = full_info.friendly_name;
        info.vendor_id = full_info.vendor_id;
        info.product_id = full_info.product_id;
        info.removable = full_info.removable;
        info.present = full_info.present;
        info.size_bytes = full_info.size_bytes;
        info.block_size = full_info.block_size;
    } catch (...) {}
    return info;
}

bool device::is_device(const string& path) {
    if (path.empty()) {
        return false;
    }
#ifdef MSTL_PLATFORM_WINDOWS__
    return path.find("\\\\.\\") == 0 || path.find("\\\\\\\\.\\") == 0;
#elif defined(MSTL_PLATFORM_LINUX__)
    struct ::stat64 st;
    if (::stat64(path.c_str(), &st) == 0) {
        return S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode);
    }
    return false;
#endif
}

MSTL_END_NAMESPACE__
