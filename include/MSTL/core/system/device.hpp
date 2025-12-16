#ifndef MSTL_CORE_SYSTEM_DEVICE_HPP__
#define MSTL_CORE_SYSTEM_DEVICE_HPP__
#include "../async/atomic.hpp"
#include "../async/mutex.hpp"
#include "../async/thread.hpp"
#include "../container/vector.hpp"
#include "../functional/function.hpp"
#include "../interface/istringify.hpp"
#include "../time/duration.hpp"
#include "../utility/optional.hpp"
#ifdef MSTL_PLATFORM_LINUX__
#include <sys/stat.h>
#endif
MSTL_BEGIN_NAMESPACE__

enum class DEVICE_TYPE {
    SERIAL_PORT,
    STORAGE,
    HID,
    NETWORK,
    AUDIO,
    VIDEO,
    GENERIC,
    UNKNOWN
};

string to_string(DEVICE_TYPE type);

DEVICE_TYPE to_device_t(const string& str);


enum class DEVICE_OPEN_MODE {
    READ,
    WRITE,
    READ_WRITE,
    NON_BLOCKING
};

enum class DEVICE_OPEN_FLAG : uint32_t {
    NONE            = 1 >> 1,
    EXCLUSIVE       = 1 << 0,
    NO_INHERIT      = 1 << 1,
    ASYNC           = 1 << 2,
    DIRECT_IO       = 1 << 3,
    SYNC            = 1 << 4,
    CREATE          = 1 << 5
};

constexpr DEVICE_OPEN_FLAG operator |(DEVICE_OPEN_FLAG a, DEVICE_OPEN_FLAG b) {
    return static_cast<DEVICE_OPEN_FLAG>(
        static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

constexpr DEVICE_OPEN_FLAG operator &(DEVICE_OPEN_FLAG a, DEVICE_OPEN_FLAG b) {
    return static_cast<DEVICE_OPEN_FLAG>(
        static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

constexpr DEVICE_OPEN_FLAG& operator |=(DEVICE_OPEN_FLAG& a, DEVICE_OPEN_FLAG b) {
    a = a | b;
    return a;
}

constexpr DEVICE_OPEN_FLAG& operator &=(DEVICE_OPEN_FLAG& a, DEVICE_OPEN_FLAG b) {
    a = a & b;
    return a;
}


enum class DEVICE_IO_DIRECT {
    IN,
    OUT,
    BOTH
};

enum class DEVICE_EVENT {
    DATA_AVAILABLE,
    WRITE_READY,
    ERROR_OCCURRED,
    DISCONNECTED
};

struct device_info {
    string device_path;
    string friendly_name;
    string hardware_id;
    string manufacturer;
    string description;
    DEVICE_TYPE type{DEVICE_TYPE::UNKNOWN};
    uint32_t vendor_id{0};
    uint32_t product_id{0};
    uint64_t size_bytes{0};
    uint32_t block_size{512};
    bool removable{false};
    bool present{false};
    bool exclusive{false};


#ifdef MSTL_PLATFORM_WINDOWS__
    DEVINST devinst{0};
#else
    ::dev_t device_id{0};
#endif
};


class ioctl_command {
public:
#ifdef MSTL_PLATFORM_WINDOWS__
    using native_type = DWORD;
#else
    using native_type = unsigned long;
#endif
    
    explicit ioctl_command(const native_type code, const void* in_data = nullptr,
        const size_t in_size = 0, void* out_data = nullptr, const size_t out_size = 0)
    : code_(code), in_data_(in_data), in_size_(in_size),
    out_data_(out_data), out_size_(out_size) {}
    
    MSTL_NODISCARD native_type code() const noexcept { return code_; }
    MSTL_NODISCARD const void* in_data() const noexcept { return in_data_; }
    MSTL_NODISCARD size_t in_size() const noexcept { return in_size_; }
    MSTL_NODISCARD void* out_data() const noexcept { return out_data_; }
    MSTL_NODISCARD size_t out_size() const noexcept { return out_size_; }

    template<typename T>
    static ioctl_command make(const native_type code, const T& data) {
        return ioctl_command(code, &data, sizeof(T));
    }
    
    template<typename T>
    static ioctl_command make_with_output(const native_type code, T& output) {
        return ioctl_command(code, nullptr, 0, &output, sizeof(T));
    }
    
private:
    native_type code_;
    const void* in_data_;
    size_t in_size_;
    void* out_data_;
    size_t out_size_;
};


template <typename T>
struct async_result {
    T value;
    size_t bytes_transferred{0};
    int err_code{0};
    bool completed{false};
    bool cancelled{false};
};

using device_event_callback = function<void(DEVICE_EVENT)>;
using data_received_callback = function<void(const void*, size_t)>;
using io_completion_callback = function<void(size_t, const int&)>;


class MSTL_API device {
public:
    device();
    explicit device(const string& device_path,
        DEVICE_OPEN_MODE mode = DEVICE_OPEN_MODE::READ_WRITE,
        DEVICE_OPEN_FLAG flags = DEVICE_OPEN_FLAG::NONE);
    ~device();

    device(const device&) = delete;
    device& operator=(const device&) = delete;

    device(device&& other) noexcept;
    device& operator=(device&& other) noexcept;

    void open(const string& device_path, 
        DEVICE_OPEN_MODE mode = DEVICE_OPEN_MODE::READ_WRITE,
        DEVICE_OPEN_FLAG flags = DEVICE_OPEN_FLAG::NONE);
    void close() noexcept;
    bool is_open() const noexcept;
    void reopen(DEVICE_OPEN_MODE new_mode, DEVICE_OPEN_FLAG new_flags = DEVICE_OPEN_FLAG::NONE);

    size_t read(void* buffer, size_t size, chrono::milliseconds timeout = chrono::milliseconds(-1));
    size_t write(const void* buffer, size_t size, chrono::milliseconds timeout = chrono::milliseconds(-1));

    void ioctl(const ioctl_command& cmd);
    void flush();
    void sync() noexcept;

    bool wait(DEVICE_IO_DIRECT direction, chrono::milliseconds timeout = chrono::milliseconds(-1)) const;
    bool is_readable(chrono::milliseconds timeout = chrono::milliseconds(0)) const;
    bool is_writable(chrono::milliseconds timeout = chrono::milliseconds(0)) const;

    void set_timeout(chrono::milliseconds timeout);
    chrono::milliseconds get_timeout() const noexcept;
    void set_blocking(bool blocking);
    bool is_blocking() const noexcept;

    void set_event_callback(device_event_callback callback);
    void start_event_monitoring();
    void stop_event_monitoring() noexcept;

    device_info get_device_info() const;
    string get_device_path() const noexcept;
    DEVICE_TYPE get_device_type() const noexcept;

#ifdef MSTL_PLATFORM_WINDOWS__
    void* map_memory(size_t offset, size_t size);
#else
    void* map_memory(::off_t offset, size_t size) const;
#endif
    static void unmap_memory(void* address, size_t size) noexcept;

    bool supports_direct_io() const noexcept;

#ifdef MSTL_PLATFORM_WINDOWS__
    HANDLE native_handle() const noexcept { return handle_; }
#else
    int native_handle() const noexcept { return fd_; }
#endif

    static vector<device_info> enumerate_devices(
        DEVICE_TYPE type = DEVICE_TYPE::GENERIC,
        const string& filter = "");
    
    static vector<device_info> find_devices_by_vid_pid(uint16_t vid, uint16_t pid);
    static optional<device_info> find_device_by_path(const string& path);

    static bool exists(const string& device_path);
    static bool is_device(const string& path);
    
private:
#ifdef MSTL_PLATFORM_WINDOWS__
    HANDLE handle_{INVALID_HANDLE_VALUE};
    HANDLE cancel_event_{nullptr};
    OVERLAPPED overlapped_{};
    unique_ptr<uint8_t[]> overlapped_buffer_;
    size_t overlapped_buffer_size_{0};
#else
    int fd_{-1};
    int event_fd_{-1};
    bool is_non_blocking_{false};
#endif

    string device_path_;
    DEVICE_TYPE device_type_{DEVICE_TYPE::UNKNOWN};
    chrono::milliseconds timeout_{1000};
    bool is_blocking_{true};

    atomic<bool> monitoring_{false};
    thread monitor_thread_;
    device_event_callback event_callback_;

    mutable mutex io_mutex_;

    void init();
    void cleanup() noexcept;
    void setup_overlapped_io();
    static void check_error(const string& operation, bool result) ;

#ifdef MSTL_PLATFORM_WINDOWS__
    void setup_cancel_event();
    bool get_overlapped_result(size_t& bytes_transferred, bool wait);
    static DEVICE_TYPE guess_device_type_from_guid(const GUID& guid);
    static string get_device_property(HDEVINFO dev_info_set, 
                                           PSP_DEVINFO_DATA dev_info_data,
                                           DWORD property);
#else
    bool set_non_blocking(bool non_blocking);
    static DEVICE_TYPE guess_device_type_from_path(const string& path);
    static DEVICE_TYPE guess_device_type_from_stat(const struct stat64& st);
    static string read_sysfs_attribute(const string& device_path,
                                           const string& attribute);
#endif

    void monitor_device_events();
};


class MSTL_API serial_port : public device {
public:
    struct serial_config {
        uint32_t baud_rate = 115200u;
        uint8_t data_bits = 8u;
        uint8_t stop_bits = 1u;
        char parity = 'N';
        bool flow_control = false;
        bool xon_xoff = false;

        bool break_enable = false;
        bool dsr_sensitivity = false;
        bool dtr_control = true;
        bool rts_control = true;

        chrono::milliseconds read_interval_timeout{0};
        chrono::milliseconds read_total_timeout_multiplier{0};
        chrono::milliseconds read_total_timeout_constant{0};
        chrono::milliseconds write_total_timeout_multiplier{0};
        chrono::milliseconds write_total_timeout_constant{0};

        explicit serial_config(const uint32_t baud_rate) : baud_rate(baud_rate) {}
        serial_config() : serial_config(115200) {}
    };

    struct modem_status {
        bool cts{false};
        bool dsr{false};
        bool ri{false};
        bool dcd{false};
    };

    struct line_status {
        bool framing_error{false};
        bool parity_error{false};
        bool overrun_error{false};
        bool break_detected{false};
    };
    
    explicit serial_port(const string& port_name,
        const serial_config& config = serial_config{},
        DEVICE_OPEN_FLAG flags = DEVICE_OPEN_FLAG::NONE);

    void configure(const serial_config& config);
    serial_config get_configuration() const;

    void set_rts(bool state);
    void set_dtr(bool state);
    bool get_cts() const;
    bool get_dsr() const;
    bool get_ri() const;
    bool get_dcd() const;
    modem_status get_modem_status() const;

    void set_break(bool enable);
    line_status get_line_status() const;

    void purge_rx_buffer();
    void purge_tx_buffer();
    void purge_both_buffers();

    size_t get_rx_queue_size() const;
    size_t get_tx_queue_size() const;
    
private:
    serial_config current_config_{};
    
#ifdef MSTL_PLATFORM_WINDOWS__
    void configure_windows(const serial_config& config);
    modem_status get_modem_status_windows() const;
#else
    void configure_linux(const serial_config& config);
    modem_status get_modem_status_linux() const;
    line_status get_line_status_linux() const;
#endif
};


class MSTL_API storage_device : public device {
public:
    struct partition_info {
        uint64_t start_sector{0};
        uint64_t sector_count{0};
        uint32_t partition_type{0};
        bool bootable{false};
        string label;
    };
    
    explicit storage_device(const string& device_path, DEVICE_OPEN_FLAG flags = DEVICE_OPEN_FLAG::DIRECT_IO);

    uint64_t get_capacity_bytes() const;
    uint64_t get_capacity_sectors() const;
    uint32_t get_sector_size() const;
    uint32_t get_physical_sector_size() const;

    vector<partition_info> get_partitions() const;
    bool has_partitions() const noexcept;

    void read_sectors(void* buffer, uint64_t sector_start, size_t sector_count);
    void write_sectors(const void* buffer, uint64_t sector_start, size_t sector_count);

    bool is_removable() const;
    bool is_read_only() const;
    bool supports_trim() const;
    bool supports_flush() const;

    void flush_buffers();
    void trim(uint64_t sector_start, size_t sector_count);
    void secure_erase();

    void lock();
    void unlock();
    bool is_locked() const;

    void eject();
    
private:
    uint32_t sector_size_{512};
    uint32_t physical_sector_size_{512};
    bool is_removable_{false};
    bool is_read_only_{false};
    
    void query_device_geometry();
    
#ifdef MSTL_PLATFORM_WINDOWS__
    void query_device_geometry_windows();
#else
    void query_device_geometry_linux();
#endif
};


namespace device_utils {
    string get_last_error_string();
    bool is_special_file(const string& path);
    string normalize_device_path(const string& path);

    template<typename T>
    T swap_endian(T value) {
        static_assert(is_arithmetic<T>::value, "Type must be arithmetic");
        
        union {
            T value;
            uint8_t bytes[sizeof(T)];
        } source, dest;
        
        source.value = value;
        for (size_t i = 0; i < sizeof(T); i++) {
            dest.bytes[i] = source.bytes[sizeof(T) - i - 1];
        }
        
        return dest.value;
    }

    bool is_serial_port_name(const string& name);
    string generate_device_id(const device_info& info);
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_SYSTEM_DEVICE_HPP__
