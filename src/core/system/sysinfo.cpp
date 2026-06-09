#include <NeForce/core/async/mutex.hpp>
#include <NeForce/core/string/to_string.hpp>
#include <NeForce/core/system/sysinfo.hpp>
#include <NeForce/core/utility/packages.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <NeForce/core/memory/bit.hpp>
#    include <comdef.h>
#    include <intrin.h>
#    include <pdh.h>
#    include <psapi.h>
#    include <winternl.h>
#    include <wbemcli.h>
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#    include <dirent.h>
#    include <sys/sysinfo.h>
#    include <sys/statvfs.h>
#    include <sys/utsname.h>
#    include <unistd.h>
#    include <cstdio>
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
    mutex& sysinfo_mutex() {
        static mutex sysmutex;
        return sysmutex;
    }

#ifdef NEFORCE_PLATFORM_LINUX

    string read_file_all(const char* filepath) {
        string content;
        ::FILE* fp = ::fopen(filepath, "r");
        if (fp == nullptr) {
            return content;
        }

        char buf[4096];
        size_t n = 0;

        while (true) {
            n = ::fread(buf, 1, sizeof(buf), fp);
            if (n > 0) {
                content.append(buf, n);
            }

            if (n < sizeof(buf)) {
                if (::feof(fp) != 0) {
                    break;
                }
                if (::ferror(fp) != 0) {
                    break;
                }
            }
        }
        ::fclose(fp);
        return content;
    }

    uint32_t read_sysfs_freq(const char* path_str) {
        const string content = read_file_all(path_str);
        if (content.empty()) {
            return 0;
        }

        string_view sv;
        size_t ipos = 0;
        getline(content.view(), ipos, sv, [](char c) { return is_space(c); });
        if (!sv.empty()) {
            return static_cast<uint32_t>(to_uint64(sv) / 1000); // kHz -> MHz
        }
        return 0;
    }

#endif

    void get_cpu_info_internal(sysinfo::CPU_info& cpu_info, const sysinfo::architecture arch) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        int cpu_info_data[4] = {-1};
        char vendor[13] = {};
        char brand[49] = {};

        // Get CPU manufacturer information
        ::__cpuid(cpu_info_data, 0);
        memory_copy(vendor, &cpu_info_data[1], 4);
        memory_copy(vendor + 4, &cpu_info_data[3], 4);
        memory_copy(vendor + 8, &cpu_info_data[2], 4);
        vendor[12] = '\0';

        cpu_info.vendor = vendor;

        // Get the CPU brand string
        for (int i = static_cast<int>(0x80000002); i <= static_cast<int>(0x80000004); i++) {
            ::__cpuid(cpu_info_data, i);
            memory_copy(brand + static_cast<ptrdiff_t>((i - 0x80000002) * 16), cpu_info_data, sizeof(cpu_info_data));
        }
        brand[48] = '\0';
        cpu_info.brand = brand;
        cpu_info.brand.trim_right();

        // Obtain the relationship between NUMA and processor core
        ::DWORD buffer_size = 0;
        ::GetLogicalProcessorInformation(nullptr, &buffer_size);

        if (::GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            auto* const buffer = static_cast<::SYSTEM_LOGICAL_PROCESSOR_INFORMATION*>(::malloc(buffer_size));
            if (buffer != nullptr) {
                if (::GetLogicalProcessorInformation(buffer, &buffer_size) == TRUE) {
                    ::DWORD logical_processor_count = 0;
                    ::DWORD processor_core_count = 0;

                    for (::DWORD i = 0; i < buffer_size / sizeof(::SYSTEM_LOGICAL_PROCESSOR_INFORMATION); i++) {
                        if (buffer[i].Relationship == ::RelationProcessorCore) {
                            processor_core_count++;
                            logical_processor_count += popcount(buffer[i].ProcessorMask);
                        }
                    }

                    cpu_info.cores = processor_core_count;
                    cpu_info.logical_processors = logical_processor_count;
                }
                ::free(buffer);
            }
        }

        // Get the current CPU frequency from the registry
        ::HKEY hkey = nullptr;
        if (::RegOpenKeyExA(HKEY_LOCAL_MACHINE, R"(HARDWARE\DESCRIPTION\System\CentralProcessor\0)", 0, KEY_READ,
                            &hkey) == ERROR_SUCCESS) {

            ::DWORD mhz = 0;
            ::DWORD size = sizeof(::DWORD);

            if (::RegQueryValueExA(hkey, "~MHz", nullptr, nullptr, reinterpret_cast<::LPBYTE>(&mhz), &size) ==
                ERROR_SUCCESS) {
                cpu_info.current_MHz = mhz;
            }

            mhz = 0;
            size = sizeof(::DWORD);

            LONG result = ::RegQueryValueExA(hkey, "MaxMHz", nullptr, nullptr, reinterpret_cast<::LPBYTE>(&mhz), &size);

            if (result != ERROR_SUCCESS) {
                mhz = 0;
                size = sizeof(::DWORD);
                result = ::RegQueryValueExA(hkey, "MaxClockSpeed", nullptr, nullptr, reinterpret_cast<::LPBYTE>(&mhz),
                                            &size);
            }

            if (result == ERROR_SUCCESS && mhz > 0) {
                cpu_info.max_MHz = mhz;
            }

            ::RegCloseKey(hkey);
        }

        if (cpu_info.max_MHz == 0) {
            ::__cpuid(cpu_info_data, 0x16);
            if (cpu_info_data[0] != 0) {
                cpu_info.max_MHz = cpu_info_data[1];
                if (cpu_info.current_MHz == 0) {
                    cpu_info.current_MHz = cpu_info_data[0];
                }
            }
        }

        if (cpu_info.max_MHz == 0) {
            ::__cpuid(cpu_info_data, 0x15);
            if (cpu_info_data[0] != 0 && cpu_info_data[1] != 0 && cpu_info_data[2] != 0) {
                uint32_t crystal_clock = cpu_info_data[2];
                if (crystal_clock == 0) {
                    crystal_clock = 38400000;
                }

                cpu_info.max_MHz = (crystal_clock / 1000000) * cpu_info_data[1] / cpu_info_data[0];
            }
        }

        if (cpu_info.max_MHz == 0) {
            const char* ghz_pos = string_find_pattern(brand, "GHz");
            if (ghz_pos != nullptr) {
                const char* num_start = ghz_pos - 1;
                while (num_start >= brand && (is_digit(*num_start) || *num_start == '.')) {
                    num_start--;
                }
                num_start++;

                if (num_start < ghz_pos) {
                    try {
                        const float freq_ghz = float32::parse(num_start).value();
                        cpu_info.max_MHz = static_cast<uint32_t>(freq_ghz * 1000);
                        if (cpu_info.current_MHz == 0) {
                            cpu_info.current_MHz = cpu_info.max_MHz;
                        }
                        // NOLINTNEXTLINE(bugprone-empty-catch)
                    } catch (...) {
                        // ignore
                    }
                }
            } else {
                const char* mhz_pos = string_find_pattern(brand, "MHz");
                if (mhz_pos != nullptr) {
                    const char* num_start = mhz_pos - 1;
                    while (num_start >= brand && is_digit(*num_start)) {
                        num_start--;
                    }
                    num_start++;

                    if (num_start < mhz_pos) {
                        try {
                            cpu_info.max_MHz = uinteger32::parse(num_start).value();
                            if (cpu_info.current_MHz == 0) {
                                cpu_info.current_MHz = cpu_info.max_MHz;
                            }
                            // NOLINTNEXTLINE(bugprone-empty-catch)
                        } catch (...) {
                            // ignore
                        }
                    }
                }
            }
        }

#else
        const string data = read_file_all("/proc/cpuinfo");
        if (data.empty()) {
            cpu_info.cores = static_cast<uint32_t>(::sysconf(::_SC_NPROCESSORS_CONF));
            cpu_info.logical_processors = static_cast<uint32_t>(::sysconf(::_SC_NPROCESSORS_ONLN));
            return;
        }

        const auto* vendor_key = "vendor_id";
        const auto* brand_key = "model name";
        const auto* flags_key = "flags";
        const auto* features_key = "Features";
        const auto* freq_key = "cpu MHz";
        const auto* cores_key = "cpu cores";

        if (arch == sysinfo::architecture::RISCV32 || arch == sysinfo::architecture::RISCV64) {
            vendor_key = "mvendorid";
            brand_key = "uarch";
            flags_key = "isa";
            features_key = nullptr;
            freq_key = nullptr;
            cores_key = nullptr;
        } else if (arch == sysinfo::architecture::LOONGARCH64 || arch == sysinfo::architecture::LOONGARCH32) {
            vendor_key = "CPU Family";
            brand_key = "Model Name";
            flags_key = "flags";
            freq_key = "CPU MHz";
            cores_key = "CPU Cores";
        }

        string_view line;
        size_t pos = 0;
        bool first_processor = true;
        uint32_t hart_count = 0;
        uint32_t core_count_from_cpuinfo = 0;

        while (getline(data.view(), pos, line)) {
            if (line.empty()) {
                continue;
            }

            if (line.starts_with("processor")) {
                if (arch == sysinfo::architecture::RISCV32 || arch == sysinfo::architecture::RISCV64) {
                    hart_count++;
                }
                continue;
            }

            if (first_processor) {
                bool extracted = false;

                if (vendor_key != nullptr && line.starts_with(vendor_key)) {
                    const size_t colon = line.find(':');
                    if (colon != string::npos) {
                        cpu_info.vendor = line.tail(colon + 1);
                        cpu_info.vendor.trim();
                        extracted = true;
                    }
                }

                if (brand_key != nullptr && line.starts_with(brand_key)) {
                    const size_t colon = line.find(':');
                    if (colon != string::npos) {
                        cpu_info.brand = line.tail(colon + 1);
                        cpu_info.brand.trim();
                        extracted = true;
                    }
                }

                if (flags_key != nullptr && line.starts_with(flags_key)) {
                    const size_t colon = line.find(':');
                    if (colon != string::npos) {
                        cpu_info.features = line.tail(colon + 1);
                        cpu_info.features.trim();
                        extracted = true;
                    }
                } else if (features_key != nullptr && line.starts_with(features_key)) {
                    const size_t colon = line.find(':');
                    if (colon != string::npos) {
                        cpu_info.features = line.tail(colon + 1);
                        cpu_info.features.trim();
                        extracted = true;
                    }
                }

                if (freq_key != nullptr && line.starts_with(freq_key)) {
                    const size_t colon = line.find(':');
                    if (colon != string::npos) {
                        cpu_info.current_MHz = to_uint32(line.view(colon + 1));
                        extracted = true;
                    }
                }

                if (cores_key != nullptr && line.starts_with(cores_key)) {
                    const size_t colon = line.find(':');
                    if (colon != string::npos) {
                        core_count_from_cpuinfo = to_uint32(line.view(colon + 1));
                        extracted = true;
                    }
                }

                if (extracted && !cpu_info.vendor.empty() && !cpu_info.brand.empty() && !cpu_info.features.empty() &&
                    cpu_info.current_MHz > 0 && core_count_from_cpuinfo > 0) {
                    first_processor = false;
                }
            }

            if (cpu_info.current_MHz == 0 && line.starts_with("cpu MHz")) {
                const size_t colon = line.find(':');
                if (colon != string::npos) {
                    cpu_info.current_MHz = to_uint32(line.view(colon + 1));
                }
            }
        }

        if (arch == sysinfo::architecture::RISCV32 || arch == sysinfo::architecture::RISCV64) {
            cpu_info.logical_processors =
                    (hart_count > 0) ? hart_count : static_cast<uint32_t>(::sysconf(_SC_NPROCESSORS_ONLN));
        } else {
            if (cpu_info.logical_processors == 0) {
                cpu_info.logical_processors = static_cast<uint32_t>(::sysconf(_SC_NPROCESSORS_ONLN));
            }
        }

        if (core_count_from_cpuinfo > 0) {
            cpu_info.cores = core_count_from_cpuinfo;
        } else {
            cpu_info.cores = cpu_info.logical_processors;
        }

        if (cpu_info.current_MHz == 0 || cpu_info.max_MHz == 0) {
            auto try_sysfs = [&cpu_info]() {
                cpu_info.max_MHz = read_sysfs_freq("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
                if (cpu_info.max_MHz == 0) {
                    cpu_info.max_MHz = read_sysfs_freq("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq");
                }
                if (cpu_info.current_MHz == 0) {
                    cpu_info.current_MHz = read_sysfs_freq("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");
                }
            };
            try_sysfs();
        }

        if (cpu_info.max_MHz == 0 && !cpu_info.brand.empty()) {
            const char* ghz_pos = string_find_pattern(cpu_info.brand.data(), "GHz");
            if (ghz_pos != nullptr) {
                const char* num_start = ghz_pos - 1;
                while (num_start >= cpu_info.brand.data() && (is_digit(*num_start) || *num_start == '.')) {
                    num_start--;
                }
                num_start++;
                if (num_start < ghz_pos) {
                    try {
                        const float freq_ghz = float32::parse(num_start).value();
                        cpu_info.max_MHz = static_cast<uint32_t>(freq_ghz * 1000);
                        if (cpu_info.current_MHz == 0) {
                            cpu_info.current_MHz = cpu_info.max_MHz;
                        }
                        // NOLINTNEXTLINE(bugprone-empty-catch)
                    } catch (...) {
                        // ignore
                    }
                }
            } else {
                const char* mhz_pos = string_find_pattern(cpu_info.brand.data(), "MHz");
                if (mhz_pos != nullptr) {
                    const char* num_start = mhz_pos - 1;
                    while (num_start >= cpu_info.brand.data() && is_digit(*num_start)) {
                        num_start--;
                    }
                    num_start++;
                    if (num_start < mhz_pos) {
                        try {
                            cpu_info.max_MHz = uinteger32::parse(num_start).value();
                            if (cpu_info.current_MHz == 0) {
                                cpu_info.current_MHz = cpu_info.max_MHz;
                            }
                            // NOLINTNEXTLINE(bugprone-empty-catch)
                        } catch (...) {
                            // ignore
                        }
                    }
                }
            }
        }
#endif
    }

    void get_os_version_internal(sysinfo::os_version_info& os_version_info) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        const ::HMODULE ntdll = ::GetModuleHandle("ntdll.dll");
        if (ntdll != nullptr) {
            using RtlGetVersionPtr = ::NTSTATUS(__stdcall*)(::LPOSVERSIONINFOW);
            const auto RtlGetVersion = reinterpret_cast<RtlGetVersionPtr>(::GetProcAddress(ntdll, "RtlGetVersion"));

            ::OSVERSIONINFOW version_info = {sizeof(version_info)};
            if (RtlGetVersion(&version_info) == 0) {
                os_version_info.major = version_info.dwMajorVersion;
                os_version_info.minor = version_info.dwMinorVersion;
                os_version_info.build = version_info.dwBuildNumber;
                os_version_info.platform_id = version_info.dwPlatformId;
                os_version_info.csd_version = wcharacter::to_string(version_info.szCSDVersion);
            }
        }

        ::HKEY hkey{};
        if (::RegOpenKeyEx(HKEY_LOCAL_MACHINE, R"(SOFTWARE\Microsoft\Windows NT\CurrentVersion)", 0, KEY_READ, &hkey) ==
            ERROR_SUCCESS) {

            char product_name[256];
            ::DWORD size = sizeof(product_name);

            if (::RegQueryValueEx(hkey, "ProductName", nullptr, nullptr, reinterpret_cast<::LPBYTE>(product_name),
                                  &size) == ERROR_SUCCESS) {
                os_version_info.product_name = product_name;
            }

            ::RegCloseKey(hkey);
        }
#else
        ::utsname uname_data{};
        if (::uname(&uname_data) == 0) {
            string release = uname_data.release;

            int version_parts[3] = {0, 0, 0};
            int part_index = 0;

            string current_number;
            for (const char c: release) {
                if (is_digit(c)) {
                    current_number += c;
                } else if (c == '.' && !current_number.empty() && part_index < 3) {
                    version_parts[part_index] = to_int32(current_number.view());
                    part_index++;
                    current_number.clear();

                    if (part_index >= 3) {
                        break;
                    }
                } else if (!current_number.empty() && part_index < 3) {
                    version_parts[part_index] = to_int32(current_number.view());
                    part_index++;
                    if (part_index >= 3) {
                        break;
                    }
                    current_number.clear();

                    if (c == '-' || c == '+' || c == ' ') {
                        break;
                    }
                }
            }

            if (!current_number.empty() && part_index < 3) {
                version_parts[part_index] = to_int32(current_number.view());
            }

            os_version_info.major = version_parts[0];
            os_version_info.minor = version_parts[1];
            os_version_info.build = version_parts[2];
            os_version_info.product_name = uname_data.sysname;
        }

        const string data = read_file_all("/etc/os-release");
        if (data.empty()) {
            return;
        }

        string_view line;
        size_t pos = 0;
        while (getline(data.view(), pos, line)) {
            if (line.starts_with("PRETTY_NAME")) {
                const size_t eq_pos = line.find('=');
                if (eq_pos != string::npos) {
                    string_view value = line.tail(eq_pos + 1);
                    if (value.front() == '"' && value.back() == '"') {
                        value = value.substr(1, value.length() - 2);
                    }
                    if (!os_version_info.product_name.empty()) {
                        os_version_info.product_name += " ";
                    }
                    os_version_info.product_name += value;
                }
                break;
            }
        }
#endif
    }
} // namespace


string sysinfo::os_version_info::version() const {
    return to_string(major) + "." + to_string(minor) + "." + to_string(build);
}

sysinfo::sysinfo() noexcept {
    try {
        lock<mutex> lock(sysinfo_mutex());
        init();
    } catch (...) {
        initialized_.store(false);
    }
}

void sysinfo::init() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::SYSTEM_INFO system_info{};
    ::GetSystemInfo(&system_info);

    system_info_.processor_numbers = system_info.dwNumberOfProcessors;
    system_info_.page_size = system_info.dwPageSize;
    system_info_.allocation_granularity = system_info.dwAllocationGranularity;

    system_info_.min_app_address = reinterpret_cast<uintptr_t>(system_info.lpMinimumApplicationAddress);
    system_info_.max_app_address = reinterpret_cast<uintptr_t>(system_info.lpMaximumApplicationAddress);
    system_info_.active_processor_mask = static_cast<uintptr_t>(system_info.dwActiveProcessorMask);

    system_info_.processor_level = system_info.wProcessorLevel;
    system_info_.processor_revision = system_info.wProcessorRevision;

    ::SYSTEM_INFO native_sys_info{};
    ::GetNativeSystemInfo(&native_sys_info);

    switch (native_sys_info.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_INTEL: {
            architecture_ = architecture::X86;
            break;
        }
        case PROCESSOR_ARCHITECTURE_AMD64: {
            architecture_ = architecture::X64;
            break;
        }
        case PROCESSOR_ARCHITECTURE_ARM: {
            architecture_ = architecture::ARM;
            break;
        }
        case PROCESSOR_ARCHITECTURE_ARM64: {
            architecture_ = architecture::ARM64;
            break;
        }
        default: {
            architecture_ = architecture::UNKNOWN;
            break;
        }
    }

    get_cpu_info_internal(cpu_info_, architecture_);
    get_os_version_internal(os_version_info_);

    ::MEMORYSTATUSEX mem_status{};
    mem_status.dwLength = sizeof(mem_status);
    if (::GlobalMemoryStatusEx(&mem_status) == TRUE) {
        memory_info_.total_physical = mem_status.ullTotalPhys;
        memory_info_.available_physical = mem_status.ullAvailPhys;
        memory_info_.total_virtual = mem_status.ullTotalVirtual;
        memory_info_.available_virtual = mem_status.ullAvailVirtual;
        memory_info_.total_page_file = mem_status.ullTotalPageFile;
        memory_info_.available_page_file = mem_status.ullAvailPageFile;
    }

#else
    system_info_.page_size = ::sysconf(_SC_PAGESIZE);
    system_info_.processor_numbers = ::sysconf(_SC_NPROCESSORS_ONLN);
    system_info_.allocation_granularity = system_info_.page_size;
    const long online_cpus = ::sysconf(_SC_NPROCESSORS_ONLN);
    if (online_cpus > 0 && static_cast<size_t>(online_cpus) <= sizeof(uintptr_t) * 8) {
        system_info_.active_processor_mask = (static_cast<uintptr_t>(1) << online_cpus) - 1;
    } else {
        system_info_.active_processor_mask = ~static_cast<uintptr_t>(0);
    }

#    ifdef NEFORCE_ARCH_BITS_64
    system_info_.min_app_address = 0x400000;
    system_info_.max_app_address = 0x7fffffffffff;
#    else
    system_info_.min_app_address = 0x08048000;
    system_info_.max_app_address = 0xC0000000;
#    endif

    ::utsname uname_data{};
    if (::uname(&uname_data) == 0) {
        const string& machine = uname_data.machine;
        if (machine == "x86_64") {
            architecture_ = architecture::X64;
        } else if (machine == "i686" || machine == "i386") {
            architecture_ = architecture::X86;
        } else if (machine == "arm" || machine == "armv7l") {
            architecture_ = architecture::ARM;
        } else if (machine == "aarch64") {
            architecture_ = architecture::ARM64;
        } else if (machine == "riscv32") {
            architecture_ = architecture::RISCV32;
        } else if (machine == "riscv64") {
            architecture_ = architecture::RISCV64;
        } else if (machine == "loongarch64") {
            architecture_ = architecture::LOONGARCH64;
        } else if (machine == "loongarch32") {
            architecture_ = architecture::LOONGARCH32;
        } else {
            architecture_ = architecture::UNKNOWN;
        }
    }

    get_cpu_info_internal(cpu_info_, architecture_);
    get_os_version_internal(os_version_info_);

    struct ::sysinfo linux_sysinfo{};
    if (::sysinfo(&linux_sysinfo) == 0) {
        memory_info_.total_physical = linux_sysinfo.totalram * linux_sysinfo.mem_unit;
        memory_info_.available_physical = linux_sysinfo.freeram * linux_sysinfo.mem_unit;

        memory_info_.total_page_file = linux_sysinfo.totalswap * linux_sysinfo.mem_unit;
        memory_info_.available_page_file = linux_sysinfo.freeswap * linux_sysinfo.mem_unit;

        memory_info_.total_virtual = memory_info_.total_physical + memory_info_.total_page_file;
        memory_info_.available_virtual = memory_info_.available_physical + memory_info_.available_page_file;
    }

    const string memdata = read_file_all("/proc/meminfo");
    if (!memdata.empty()) {
        string_view line;
        size_t pos = 0;
        while (getline(memdata.view(), pos, line)) {
            if (line.starts_with("MemTotal:")) {
                const size_t colon = line.find(':');
                if (colon != string::npos) {
                    string_view value = line.view(colon + 1);
                    ignore = value.trim_left();
                    if (value.ends_with("kB")) {
                        value = value.substr(0, value.length() - 2);
                    }
                    ignore = value.trim_right();
                    if (!value.empty()) {
                        memory_info_.total_physical = to_uint64(value) * 1024;
                    }
                }
            } else if (line.starts_with("MemAvailable:")) {
                const size_t colon = line.find(':');
                if (colon != string::npos) {
                    string_view value = line.view(colon + 1);
                    ignore = value.trim_left();
                    if (value.ends_with("kB")) {
                        value = value.substr(0, value.length() - 2);
                    }
                    ignore = value.trim_right();
                    if (!value.empty()) {
                        memory_info_.available_physical = to_uint64(value) * 1024;
                    }
                }
            }
        }
    }

#endif

    initialized_.store(true, memory_order_release);
}

void sysinfo::refresh() {
    lock<mutex> lock(sysinfo_mutex());
    initialized_.store(false, memory_order_release);
    init();
}

float64_t sysinfo::cpu_usage() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    static ::PDH_HQUERY cpu_query;
    static ::PDH_HCOUNTER cpu_total;
    static bool initialized = false;

    if (!initialized) {
        if (::PdhOpenQuery(nullptr, 0, &cpu_query) != ERROR_SUCCESS) {
            return 0.0;
        }
        if (::PdhAddCounter(cpu_query, "\\Processor(_Total)\\% Processor Time", 0, &cpu_total) != ERROR_SUCCESS) {
            ::PdhCloseQuery(cpu_query);
            return 0.0;
        }
        ::PdhCollectQueryData(cpu_query);
        initialized = true;
        return 0.0;
    }

    if (::PdhCollectQueryData(cpu_query) != ERROR_SUCCESS) {
        return 0.0;
    }

    ::PDH_FMT_COUNTERVALUE counter_val{};

    if (::PdhGetFormattedCounterValue(cpu_total, PDH_FMT_DOUBLE, nullptr, &counter_val) != ERROR_SUCCESS) {
        return 0.0;
    }

    return counter_val.doubleValue;

#else

    static uint64_t prev_total = 0;
    static uint64_t prev_idle = 0;

    const string content = read_file_all("/proc/stat");
    if (content.empty()) {
        return 0.0;
    }

    string line;
    size_t pos = 0;
    getline(content.view(), pos, line);

    if (line.starts_with("cpu ")) {
        const string_view data = line.view(4);
        string_view token;
        uint64_t fields[8] = {};
        size_t offset = 0;
        int idx = 0;

        while (idx < 8 && getline(data, offset, token, is_space<char>)) {
            if (!token.empty()) {
                fields[idx++] = to_uint64(token);
            }
        }

        const uint64_t user = fields[0];
        const uint64_t nice = fields[1];
        const uint64_t system = fields[2];
        const uint64_t idle = fields[3];
        const uint64_t iowait = fields[4];
        const uint64_t irq = fields[5];
        const uint64_t softirq = fields[6];
        const uint64_t steal = fields[7];

        const uint64_t total = user + nice + system + idle + iowait + irq + softirq + steal;
        const uint64_t current_idle = idle + iowait;

        if (prev_total > 0 && prev_idle > 0) {
            const uint64_t total_diff = total - prev_total;
            const uint64_t idle_diff = current_idle - prev_idle;

            if (total_diff > 0) {
                const float64_t usage =
                        100.0 * (1.0 - static_cast<float64_t>(idle_diff) / static_cast<float64_t>(total_diff));
                prev_total = total;
                prev_idle = current_idle;
                return usage;
            }
        }

        prev_total = total;
        prev_idle = current_idle;
    }
    return 0.0;
#endif
}

uint32_t sysinfo::process_count() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::DWORD processes[1024];
    ::DWORD needed = 0;
    if (::EnumProcesses(processes, sizeof(processes), &needed) == FALSE) {
        return 0;
    }
    return needed / sizeof(::DWORD);
#else
    uint32_t count = 0;
    ::DIR* dir = ::opendir("/proc");
    if (dir != nullptr) {
        const ::dirent* entry = nullptr;
        // NOLINTNEXTLINE(concurrency-mt-unsafe)
        while ((entry = ::readdir(dir)) != nullptr) {
            if (entry->d_type == DT_DIR) {
                bool is_numeric = true;
                for (int i = 0; entry->d_name[i] != '\0'; i++) {
                    if (!is_digit(entry->d_name[i])) {
                        is_numeric = false;
                        break;
                    }
                }
                if (is_numeric) {
                    count++;
                }
            }
        }
        ::closedir(dir);
    }
    return count;
#endif
}

sysinfo::disk_info sysinfo::get_disk_info(const char* path) {
    disk_info info;

#ifdef NEFORCE_PLATFORM_WINDOWS
    char resolved_path[MAX_PATH];
    if (path == nullptr) {
        if (::GetCurrentDirectoryA(MAX_PATH, resolved_path) == FALSE) {
            return info;
        }
        if (resolved_path[1] == ':') {
            resolved_path[3] = '\0';
        } else {
            string_copy(resolved_path, "C:\\");
        }
    } else {
        string_copy(resolved_path, path);
    }

    ::ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;
    if (::GetDiskFreeSpaceExA(resolved_path, &freeBytesAvailable, &totalBytes, &totalFreeBytes) == TRUE) {
        info.path = resolved_path;
        info.total_bytes = totalBytes.QuadPart;
        info.free_bytes = totalFreeBytes.QuadPart;
        info.used_bytes = info.total_bytes - info.free_bytes;
    }
#else
    const char* target_path = (path != nullptr) ? path : "/";
    struct ::statvfs stat = {};
    if (::statvfs(target_path, &stat) == 0) {
        info.path = target_path;
        unsigned long block_size = stat.f_frsize;
        info.total_bytes = static_cast<uint64_t>(stat.f_blocks) * block_size;
        info.free_bytes = static_cast<uint64_t>(stat.f_bavail) * block_size;
        info.used_bytes = info.total_bytes - info.free_bytes;
    }
#endif

    return info;
}

NEFORCE_END_NAMESPACE__
