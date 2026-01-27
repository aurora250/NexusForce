#include <MSTL/core/system/sysinfo.hpp>
#include <MSTL/core/string/to_string.hpp>
#include <MSTL/core/utility/packages.hpp>
#include <MSTL/core/async/mutex.hpp>
#ifdef MSTL_PLATFORM_WINDOWS__
#include <MSTL/core/memory/bit.hpp>
#include <intrin.h>
#include <comdef.h>
#include <pdh.h>
#include <psapi.h>
#include <winternl.h>
#endif
#ifdef MSTL_PLATFORM_LINUX__
#include <MSTL/core/file/file.hpp>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <dirent.h>
#endif
MSTL_BEGIN_NAMESPACE__

static mutex& sysinfo_mutex() {
    static mutex sysinfo_mutex_;
    return sysinfo_mutex_;
}

size_t sysinfo::memory_info::available_memory() const noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    return available_physical + available_virtual;
#else
    struct ::sysinfo info{};
    if (::sysinfo(&info) == 0) {
        return info.freeram * info.mem_unit + info.freeswap * info.mem_unit;
    }
    return 0;
#endif
}

string sysinfo::os_version_info::version() const {
    return to_string(major) + "." + to_string(minor) + "." + to_string(build);
}

static void get_cpu_info_internal(sysinfo::CPU_info& CPU_info) {
#ifdef MSTL_PLATFORM_WINDOWS__
    int cpu_info_data[4] = { -1 };
    char vendor[13] = {};
    char brand[49] = {};

    ::__cpuid(cpu_info_data, 0);
    _MSTL memory_copy(vendor, &cpu_info_data[1], 4);
    _MSTL memory_copy(vendor + 4, &cpu_info_data[3], 4);
    _MSTL memory_copy(vendor + 8, &cpu_info_data[2], 4);
    vendor[12] = '\0';

    CPU_info.vendor = vendor;

    for (int i = 0x80000002; i <= 0x80000004; i++) {
        ::__cpuid(cpu_info_data, i);
        _MSTL memory_copy(brand + (i - 0x80000002) * 16, cpu_info_data, sizeof(cpu_info_data));
    }
    brand[48] = '\0';
    CPU_info.brand = brand;
    CPU_info.brand.trim_right();

    ::DWORD buffer_size = 0;
    ::GetLogicalProcessorInformation(nullptr, &buffer_size);

    if (::GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        const auto buffer = static_cast<::SYSTEM_LOGICAL_PROCESSOR_INFORMATION*>(malloc(buffer_size));
        if (buffer) {
            if (::GetLogicalProcessorInformation(buffer, &buffer_size)) {
                ::DWORD logical_processor_count = 0;
                ::DWORD processor_core_count = 0;

                for (::DWORD i = 0; i < buffer_size / sizeof(::SYSTEM_LOGICAL_PROCESSOR_INFORMATION); i++) {
                    if (buffer[i].Relationship == ::RelationProcessorCore) {
                        processor_core_count++;
                        logical_processor_count += _MSTL popcount64(buffer[i].ProcessorMask);
                    }
                }

                CPU_info.cores = processor_core_count;
                CPU_info.logical_processors = logical_processor_count;
            }
            free(buffer);
        }
    }

    ::HKEY hkey = nullptr;
    if (::RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
        0, KEY_READ, &hkey) == ERROR_SUCCESS) {

        ::DWORD mhz = 0;
        ::DWORD size = sizeof(::DWORD);

        if (::RegQueryValueEx(hkey, "~MHz", nullptr, nullptr,
            reinterpret_cast<::LPBYTE>(&mhz), &size) == ERROR_SUCCESS) {
            CPU_info.current_MHZ = mhz;
        }

        ::RegCloseKey(hkey);
    }
#else
    const file cpuinfo(path("/proc/cpuinfo"));
    string_view line;
    bool first_processor = true;
    uint32_t processor_count = 0;
    const string cpuinfo_str = cpuinfo.read();
    size_t pos = 0;
    
    while (_MSTL getline(cpuinfo_str.view(), pos, line)) {
        if (line.empty()) continue;
        
        if (line.find("processor") == 0) {
            processor_count++;
            continue;
        }
        
        if (first_processor) {
            if (line.find("vendor_id") == 0) {
                const size_t colon = line.find(':');
                if (colon != string::npos) {
                    CPU_info.vendor = line.substr(colon + 2);
                }
            } else if (line.find("model name") == 0) {
                const size_t colon = line.find(':');
                if (colon != string::npos) {
                    CPU_info.brand = line.substr(colon + 2);
                }
            } else if (line.find("cpu cores") == 0) {
                const size_t colon = line.find(':');
                if (colon != string::npos) {
                    CPU_info.cores = to_uint32(line.view(colon + 2));
                }
            } else if (line.find("flags") == 0 || line.find("Features") == 0) {
                const size_t colon = line.find(':');
                if (colon != string::npos) {
                    CPU_info.features = line.substr(colon + 2);
                }
            }
        }
        
        if (line.find("cpu MHz") == 0 && CPU_info.current_MHZ == 0) {
            const size_t colon = line.find(':');
            if (colon != string::npos) {
                CPU_info.current_MHZ = to_uint32(line.view(colon + 2));
            }
        }
    }

    CPU_info.logical_processors = ::sysconf(::_SC_NPROCESSORS_ONLN);
    if (CPU_info.cores == 0) {
        CPU_info.cores = ::sysconf(::_SC_NPROCESSORS_CONF);
    }

    const path cpuinfo_max_freq("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
    const file cpu_max_freq(cpuinfo_max_freq);
    if (cpu_max_freq.is_opened()) {
        string_view freq_str;
        size_t cmf_pos = 0;
        const string data = cpu_max_freq.read();

        getline(data.view(), cmf_pos, freq_str,
        [](const char c) {
            return is_space(c);
        });

        if (!freq_str.empty()) {
            CPU_info.max_MHz = to_uint64(freq_str) / 1000;
        }
    }

    if (CPU_info.current_MHZ == 0) {
        const path scaling_cur_freq("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");
        const file cpu_cur_freq(scaling_cur_freq);
        if (cpu_cur_freq.is_opened()) {
            string_view freq_str;
            size_t cmf_pos = 0;
            const string data = cpu_cur_freq.read();

            getline(data.view(), cmf_pos, freq_str,
            [](const char c) {
                return is_space(c);
            });

            if (!freq_str.empty()) {
                CPU_info.current_MHZ = to_uint64(freq_str) / 1000;
            }
        }
    }
#endif
}

static void get_os_version_internal(sysinfo::os_version_info& os_version_info) {
#ifdef MSTL_PLATFORM_WINDOWS__
    const ::HMODULE ntdll = ::GetModuleHandle("ntdll.dll");
    if (ntdll) {
        using RtlGetVersionPtr = ::NTSTATUS(__stdcall*)(::LPOSVERSIONINFOW);
        const auto RtlGetVersion = reinterpret_cast<RtlGetVersionPtr>(
            ::GetProcAddress(ntdll, "RtlGetVersion"));

        ::OSVERSIONINFOW version_info = { sizeof(version_info) };
        if (RtlGetVersion(&version_info) == 0) {
            os_version_info.major = version_info.dwMajorVersion;
            os_version_info.minor = version_info.dwMinorVersion;
            os_version_info.build = version_info.dwBuildNumber;
            os_version_info.platform_id = version_info.dwPlatformId;
            os_version_info.csd_version = wcharacter::to_string(version_info.szCSDVersion);
        }
    }

    ::HKEY hkey{};
    if (::RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
        0, KEY_READ, &hkey) == ERROR_SUCCESS) {

        char product_name[256];
        ::DWORD size = sizeof(product_name);

        if (::RegQueryValueEx(hkey, "ProductName", nullptr, nullptr,
            reinterpret_cast<::LPBYTE>(product_name), &size) == ERROR_SUCCESS) {
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
            if (_MSTL is_digit(c)) {
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
                if (part_index >= 3)
                    break;
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

    const file os_release(path("/etc/os-release"));
    if (!os_release.is_opened()) return;

    string_view line;
    size_t pos = 0;
    const string data = os_release.read();
    while (getline(data.view(), pos, line)) {
        if (line.find("PRETTY_NAME") == 0) {
            const size_t eq_pos = line.find('=');
            if (eq_pos != string::npos) {
                string_view value = line.substr(eq_pos + 1);
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

sysinfo::sysinfo() {
    try {
        lock<mutex> lock(sysinfo_mutex());
        init();
    } catch (...) {
        initialized_.store(false);
        throw;
    }
}

void sysinfo::init() {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::SYSTEM_INFO system_info{};
    ::GetSystemInfo(&system_info);

    system_info_.processor_numbers = system_info.dwNumberOfProcessors;
    system_info_.page_size = system_info.dwPageSize;
    system_info_.allocation_granularity = system_info.dwAllocationGranularity;

#ifdef MSTL_DATA_BUS_WIDTH_64__
    system_info_.min_app_address = system_info.lpMinimumApplicationAddress;
    system_info_.max_app_address = system_info.lpMaximumApplicationAddress;
    system_info_.active_processor_mask = system_info.dwActiveProcessorMask;
#else
    system_info_.min_app_address = reinterpret_cast<uint32_t>(
        system_info.lpMinimumApplicationAddress);
    system_info_.max_app_address = reinterpret_cast<uint32_t>(
        system_info.lpMaximumApplicationAddress);
    system_info_.active_processor_mask = static_cast<uint32_t>(
        system_info.dwActiveProcessorMask);
#endif

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
        case PROCESSOR_ARCHITECTURE_IA64: {
            architecture_ = architecture::IA64;
            break;
        }
        default: {
            architecture_ = architecture::UNKNOWN;
            break;
        }
    }

    get_cpu_info_internal(cpu_info_);
    get_os_version_internal(os_version_info_);

    ::MEMORYSTATUSEX mem_status{};
    mem_status.dwLength = sizeof(mem_status);
    if (::GlobalMemoryStatusEx(&mem_status)) {
        memory_info_.total_physical = mem_status.ullTotalPhys;
        memory_info_.available_physical = mem_status.ullAvailPhys;
        memory_info_.total_virtual = mem_status.ullTotalVirtual;
        memory_info_.available_virtual = mem_status.ullAvailVirtual;
        memory_info_.total_page_file = mem_status.ullTotalPageFile;
        memory_info_.available_page_file = mem_status.ullAvailPageFile;
    }
    
#else
    system_info_.page_size = ::sysconf(::_SC_PAGESIZE);
    system_info_.processor_numbers = ::sysconf(::_SC_NPROCESSORS_ONLN);
    system_info_.allocation_granularity = system_info_.page_size;

#ifdef MSTL_DATA_BUS_WIDTH_64__
    system_info_.min_app_address = reinterpret_cast<void*>(0x400000);
    system_info_.max_app_address = reinterpret_cast<void*>(0x7fffffffffff);
#else
    system_info_.min_app_address = 0x08048000;
    system_info_.max_app_address = 0xC0000000;
#endif
    
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
        } else if (machine == "ia64") {
            architecture_ = architecture::IA64;
        } else {
            architecture_ = architecture::UNKNOWN;
        }
    }
    
    get_cpu_info_internal(cpu_info_);
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

    const file meminfo(path("/proc/meminfo"));
    if (meminfo.is_opened()) {
        string_view line;
        size_t pos = 0;
        const string data = meminfo.read();
        while (getline(data.view(), pos, line)) {
            if (line.find("MemTotal:") == 0) {
                const size_t colon = line.find(':');
                if (colon != string::npos) {
                    string_view value = line.view(colon + 1);
                    MSTL_IGNORE value.trim_left();
                    if (value.ends_with("kB")) {
                        value = value.substr(0, value.length() - 2);
                    }
                    MSTL_IGNORE value.trim_right();
                    if (!value.empty()) {
                        memory_info_.total_physical = to_uint64(value) * 1024;
                    }
                }
            } else if (line.find("MemAvailable:") == 0) {
                const size_t colon = line.find(':');
                if (colon != string::npos) {
                    string_view value = line.view(colon + 1);
                    MSTL_IGNORE value.trim_left();
                    if (value.ends_with("kB")) {
                        value = value.substr(0, value.length() - 2);
                    }
                    MSTL_IGNORE value.trim_right();
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

string sysinfo::format_bytes(const uint64_t bytes) {
    constexpr string_view units[] = { "B", "KB", "MB", "GB", "TB", "PB" };
    int unit_index = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024.0 && unit_index < 5) {
        size /= 1024.0;
        unit_index++;
    }
    return to_string_with_precision(size, 2) + " " + units[unit_index];
}

float64_t sysinfo::cpu_usage() {
#ifdef MSTL_PLATFORM_WINDOWS__
    static ::PDH_HQUERY cpu_query;
    static ::PDH_HCOUNTER cpu_total;
    static bool initialized = false;

    if (!initialized) {
        if (::PdhOpenQuery(nullptr, 0, &cpu_query) != ERROR_SUCCESS) {
            return 0.0;
        }
        if (::PdhAddCounter(cpu_query,
            "\\Processor(_Total)\\% Processor Time",
            0, &cpu_total) != ERROR_SUCCESS) {
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

    if (::PdhGetFormattedCounterValue(cpu_total,
        PDH_FMT_DOUBLE, nullptr, &counter_val) != ERROR_SUCCESS) {
        return 0.0;
    }

    return counter_val.doubleValue;

#else

    static uint64_t prev_total = 0;
    static uint64_t prev_idle = 0;

    const file stat(path("/proc/stat"));
    if (!stat.is_opened()) {
        return 0.0;
    }

    string line;
    size_t pos = 0;
    getline(stat.read(), pos, line);

    if (line.find("cpu ") == 0) {
        const string_view data = line.view(5);
        string_view dsv;
        uint64_t cll[8] = {0,0,0,0,0,0,0,0};
        pos = 0;

        if (getline(data, pos, dsv)) {
            cll[pos] = to_uint64(dsv);
        }

        const uint64_t user = cll[0];
        const uint64_t nice = cll[1];
        const uint64_t system = cll[2];
        const uint64_t idle = cll[3];
        const uint64_t iowait = cll[4];
        const uint64_t irq = cll[5];
        const uint64_t softirq = cll[6];
        const uint64_t steal = cll[7];

        const uint64_t total = user + nice + system + idle + iowait + irq + softirq + steal;
        const uint64_t current_idle = idle + iowait;

        if (prev_total > 0 && prev_idle > 0) {
            const uint64_t total_diff = total - prev_total;
            const uint64_t idle_diff = current_idle - prev_idle;

            if (total_diff > 0) {
                const float64_t usage = 100.0 *
                    (1.0 - static_cast<float64_t>(idle_diff) / total_diff);
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
#ifdef MSTL_PLATFORM_WINDOWS__
    ::DWORD processes[1024];
    ::DWORD needed;
    if (!::EnumProcesses(processes, sizeof(processes), &needed)) {
        return 0;
    }
    return needed / sizeof(::DWORD);
#else
    uint32_t count = 0;
    ::DIR* dir = ::opendir("/proc");
    if (dir) {
        ::dirent* entry;
        while ((entry = ::readdir(dir)) != nullptr) {
            if (entry->d_type == DT_DIR) {
                bool is_numeric = true;
                for (int i = 0; entry->d_name[i] != '\0'; i++) {
                    if (!_MSTL is_digit(entry->d_name[i])) {
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

MSTL_END_NAMESPACE__
