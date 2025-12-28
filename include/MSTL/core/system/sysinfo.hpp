#ifndef MSTL_CORE_SYSTEM_SYSINFO_HPP__
#define MSTL_CORE_SYSTEM_SYSINFO_HPP__
#include "../string/string.hpp"
#include "../async/atomic.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API sysinfo {
public:
    struct system_info {
        uint32_t processor_numbers{0};
        uint32_t page_size{0};
        uint32_t allocation_granularity{0};
#ifdef MSTL_DATA_BUS_WIDTH_64__
        void* min_app_address{nullptr};
        void* max_app_address{nullptr};
        uint64_t active_processor_mask{0};
#else
        uint32_t min_app_address{0};
        uint32_t max_app_address{0};
        uint32_t active_processor_mask{0};
#endif
        uint16_t processor_level{0};
        uint16_t processor_revision{0};
    };

    struct MSTL_API memory_info {
        uint64_t total_physical{0};
        uint64_t available_physical{0};
        uint64_t total_virtual{0};
        uint64_t available_virtual{0};
        uint64_t total_page_file{0};
        uint64_t available_page_file{0};

        MSTL_NODISCARD float64_t physical_memory_usage() const noexcept {
            if (total_physical == 0) return 0.0;
            return 100.0 * (1.0 - static_cast<float64_t>(available_physical) / total_physical);
        }

        MSTL_NODISCARD size_t available_memory() const noexcept;
    };

    struct CPU_info {
        string vendor{};
        string brand{};
        uint32_t max_MHz{0};
        uint32_t current_MHZ{0};
        uint32_t cores{0};
        uint32_t logical_processors{0};
        string features{};

        MSTL_NODISCARD bool hyperthreading() const noexcept {
            return logical_processors > cores;
        }
    };

    struct MSTL_API os_version_info {
        uint32_t major{0};
        uint32_t minor{0};
        uint32_t build{0};
        uint32_t platform_id{0};
        string csd_version{};
        string product_name{};

        MSTL_NODISCARD string version() const;
    };

    enum class architecture {
        UNKNOWN,
        X86, X64,
        ARM, ARM64,
        IA64
    };

private:
    system_info system_info_{};
    memory_info memory_info_{};
    CPU_info cpu_info_{};
    os_version_info os_version_info_{};
    architecture architecture_ = architecture::UNKNOWN;
    atomic_bool initialized_{false};

    sysinfo();
    ~sysinfo() = default;

    void init();

public:
    static sysinfo& instance() {
        static sysinfo instance;
        return instance;
    }

    sysinfo(const sysinfo&) = delete;
    sysinfo& operator =(const sysinfo&) = delete;
    sysinfo(sysinfo&&) = delete;
    sysinfo& operator =(sysinfo&&) = delete;

    void refresh();

    const system_info& get_system_info() const noexcept { return system_info_; }
    const memory_info& get_memory_info() const noexcept { return memory_info_; }
    const CPU_info& get_CPU_info() const noexcept { return cpu_info_; }
    const os_version_info& get_os_version_info() const noexcept { return os_version_info_; }
    architecture get_architecture() const noexcept { return architecture_; }

    MSTL_NODISCARD bool is_initialized() const noexcept {
        return initialized_.load(memory_order_acquire);
    }

    static string format_bytes(uint64_t bytes);
    static float64_t cpu_usage();
    static uint32_t process_count();
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_SYSTEM_SYSINFO_HPP__
