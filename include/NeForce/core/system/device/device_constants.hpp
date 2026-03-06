#ifndef NEFORCE_CORE_SYSTEM_DEVICE_DEVICE_CONSTANTS_HPP__
#define NEFORCE_CORE_SYSTEM_DEVICE_DEVICE_CONSTANTS_HPP__
#include "NeForce/core/file/path.hpp"
#include "NeForce/core/functional/function.hpp"
NEFORCE_BEGIN_NAMESPACE__

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

string NEFORCE_API to_string(DEVICE_TYPE type);

DEVICE_TYPE NEFORCE_API to_device_t(const string& str);


enum class DEVICE_OPEN_MODE {
    READ,
    WRITE,
    READ_WRITE,
    NON_BLOCKING
};

enum class DEVICE_OPEN_FLAG : uint32_t {
    NONE            = 0,
    EXCLUSIVE       = 1 << 0,
    NO_INHERIT      = 1 << 1,
    ASYNC           = 1 << 2,
    DIRECT_IO       = 1 << 3,
    SYNC            = 1 << 4,
    CREATE          = 1 << 5
};

constexpr DEVICE_OPEN_FLAG operator|(DEVICE_OPEN_FLAG a, DEVICE_OPEN_FLAG b) {
    return static_cast<DEVICE_OPEN_FLAG>(
        static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

constexpr DEVICE_OPEN_FLAG operator&(DEVICE_OPEN_FLAG a, DEVICE_OPEN_FLAG b) {
    return static_cast<DEVICE_OPEN_FLAG>(
        static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

enum class DEVICE_IO_DIRECT {
    READ, // IN
    WRITE, // OUT
    BOTH
};

enum class DEVICE_EVENT {
    DATA_AVAILABLE,
    WRITE_READY,
    ERROR_OCCURRED,
    DISCONNECTED
};


struct device_info {
    _NEFORCE path path;
    string friendly_name;
    string hardware_id;
    string manufacturer;
    string description;
    DEVICE_TYPE type{DEVICE_TYPE::UNKNOWN};
    uint16_t vendor_id{0};
    uint16_t product_id{0};
    uint32_t device_id{0};
    uint64_t size_bytes{0};
    uint32_t block_size{512};
    bool removable{false};
    bool present{false};
    bool exclusive{false};
};


class ioctl_command {
public:
#ifdef NEFORCE_PLATFORM_WINDOWS
    using native_type = ::DWORD;
#else
    using native_type = unsigned long;
#endif

    explicit ioctl_command(native_type code, const void* in_data = nullptr,
        size_t in_size = 0, void* out_data = nullptr, size_t out_size = 0)
        : code_(code), in_data_(in_data), in_size_(in_size),
          out_data_(out_data), out_size_(out_size) {}

    NEFORCE_NODISCARD native_type code() const noexcept { return code_; }
    NEFORCE_NODISCARD const void* in_data() const noexcept { return in_data_; }
    NEFORCE_NODISCARD size_t in_size() const noexcept { return in_size_; }
    NEFORCE_NODISCARD void* out_data() const noexcept { return out_data_; }
    NEFORCE_NODISCARD size_t out_size() const noexcept { return out_size_; }

    template <typename T>
    static ioctl_command make(native_type code, const T& data) {
        return ioctl_command(code, &data, sizeof(T));
    }

    template <typename T>
    static ioctl_command make_with_output(native_type code, T& output) {
        return ioctl_command(code, nullptr, 0, &output, sizeof(T));
    }

private:
    native_type code_;
    const void* in_data_;
    size_t in_size_;
    void* out_data_;
    size_t out_size_;
};


using device_event_callback = function<void(DEVICE_EVENT)>;
using data_received_callback = function<void(const void*, size_t)>;
using io_completion_callback = function<void(size_t, int)>;

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SYSTEM_DEVICE_DEVICE_CONSTANTS_HPP__
