#ifndef NEFORCE_CORE_SYSTEM_DEVICE_STORAGE_DEVICE_HPP__
#define NEFORCE_CORE_SYSTEM_DEVICE_STORAGE_DEVICE_HPP__
#include "device.hpp"
NEFORCE_BEGIN_NAMESPACE__

class NEFORCE_API storage_device final : public device {
public:
    struct partition_info {
        uint64_t start_sector{0};
        uint64_t sector_count{0};
        uint32_t partition_type{0};
        bool bootable{false};
        string label;
    };

    explicit storage_device(const string& device_path,
        DEVICE_OPEN_FLAG flags = DEVICE_OPEN_FLAG::DIRECT_IO);

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

#ifdef NEFORCE_PLATFORM_WINDOWS
    void query_device_geometry_windows();
#else
    void query_device_geometry_linux();
#endif
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SYSTEM_DEVICE_STORAGE_DEVICE_HPP__
