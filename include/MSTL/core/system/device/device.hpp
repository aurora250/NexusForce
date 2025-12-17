#ifndef MSTL_CORE_SYSTEM_DEVICE_DEVICE_HPP__
#define MSTL_CORE_SYSTEM_DEVICE_DEVICE_HPP__
#include "MSTL/core/file/file.hpp"
#include "MSTL/core/async/mutex.hpp"
#include "MSTL/core/time/duration.hpp"
#include "MSTL/core/utility/optional.hpp"
#include "device_constants.hpp"
#ifdef MSTL_PLATFORM_WINDOWS__
#include <SetupAPI.h>
#pragma comment(lib, "SetupAPI.lib")
#endif
MSTL_BEGIN_NAMESPACE__

class MSTL_API device {
public:
    using async_result = file::async_result;
    using device_handle = file::file_handle;

protected:
    file file_;
    DEVICE_TYPE device_type_{DEVICE_TYPE::UNKNOWN};
    milliseconds timeout_{1000};
    bool is_blocking_{true};

    mutable mutex io_mutex_;

    FILE_ACCESS dmode_to_faccess(DEVICE_OPEN_MODE mode) const;
    FILE_CREATION dflags_to_fcreation(DEVICE_OPEN_FLAG flags) const;
    FILE_ATTRI dflags_to_fattri(DEVICE_OPEN_FLAG flags) const;

    static DEVICE_TYPE try_device_type(const path& pth);

#ifdef MSTL_PLATFORM_WINDOWS__
    static DEVICE_TYPE try_device_type_from_guid(const GUID& guid);
    static string get_device_property(::HDEVINFO dev_info_set,
        ::PSP_DEVINFO_DATA dev_info_data, ::DWORD property);
#else
    static DEVICE_TYPE guess_device_type_from_path(const path& pth);
    static string read_sysfs_attribute(const string& device_path,
        const string& attribute);
#endif

public:
    device() = default;
    virtual ~device();

    explicit device(path device_path,
        DEVICE_OPEN_MODE mode = DEVICE_OPEN_MODE::READ_WRITE,
        DEVICE_OPEN_FLAG flags = DEVICE_OPEN_FLAG::NONE);

    device(const device&) = delete;
    device& operator =(const device&) = delete;

    device(device&& other) noexcept;
    device& operator =(device&& other) noexcept;

    void open(const path& device_path,
        DEVICE_OPEN_MODE mode = DEVICE_OPEN_MODE::READ_WRITE,
        DEVICE_OPEN_FLAG flags = DEVICE_OPEN_FLAG::NONE);

    void reopen(DEVICE_OPEN_MODE new_mode,
        DEVICE_OPEN_FLAG new_flags = DEVICE_OPEN_FLAG::NONE);

    void close() noexcept;
    bool is_open() const noexcept;

    size_t read(void* buffer, size_t size, milliseconds timeout = milliseconds(-1));
    size_t write(const void* buffer, size_t size, milliseconds timeout = milliseconds(-1));

    async_result async_read(string& buffer, size_t size, int64_t offset = -1);
    async_result async_write(const string& data, size_t size, int64_t offset = -1);
    bool wait_async(async_result& result, uint32_t timeout_ms = 0xFFFFFFFF);
    void cancel_async(async_result& result);

    void ioctl(const ioctl_command& cmd);
    void flush();
    void sync() noexcept;

    bool wait(DEVICE_IO_DIRECT direction, milliseconds timeout = milliseconds(-1)) const;
    bool is_readable(milliseconds timeout = milliseconds(0)) const;
    bool is_writable(milliseconds timeout = milliseconds(0)) const;

    void set_timeout(milliseconds timeout);
    milliseconds get_timeout() const noexcept { return timeout_; }
    void set_blocking(bool blocking);
    bool is_blocking() const noexcept { return is_blocking_; }

    _MSTL device_info device_info() const;
    const path& device_path() const noexcept { return file_.get_path(); }
    DEVICE_TYPE device_type() const noexcept { return device_type_; }

    void* map_memory(size_t offset, size_t size);
    void unmap_memory() noexcept;
    bool is_mapped() const noexcept { return file_.is_mapped(); }

    bool supports_direct_io() const noexcept;

    device_handle native_handle() const noexcept { return file_.native_handle(); }
    file& file_handle() noexcept { return file_; }
    const file& file_handle() const noexcept { return file_; }

    static vector<_MSTL device_info> enumerate(const string& filter = "");

    static vector<_MSTL device_info> find_by_vid_pid(uint16_t vid, uint16_t pid);
    static optional<_MSTL device_info> find_by_path(path pth);

    static bool exists(const string& device_path) { return path::exists(device_path); }
    static bool is_device(const string& path);
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_SYSTEM_DEVICE_DEVICE_HPP__
