#include <NeForce/core/async/mutex.hpp>
#include <NeForce/core/container/flat_unordered_map.hpp>
#include <NeForce/core/container/map.hpp>
#include <NeForce/core/utility/tuple.hpp>
#include <NeForce/core/string/to_string.hpp>
#include <NeForce/core/system/sysinfo.hpp>
#include <NeForce/core/utility/packages.hpp>
#include <NeForce/core/numeric/math.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <NeForce/core/memory/bit.hpp>
#    include <WinSock2.h>
#    include <ws2ipdef.h>
#    include <ws2tcpip.h>
#    include <comdef.h>
#    include <intrin.h>
#    include <iphlpapi.h>
#    include <netioapi.h>
#    include <pdh.h>
#    include <pdhmsg.h>
#    include <powerbase.h>
#    include <psapi.h>
#    include <winternl.h>
#    include <wbemcli.h>
#    include <cwchar>
#    ifdef NEFORCE_ARCH_X86
#        include <immintrin.h>
#    endif
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#    include <NeForce/core/file/file.hpp>
#    include <dirent.h>
#    include <ifaddrs.h>
#    include <net/if.h>
#    include <netpacket/packet.h>
#    include <arpa/inet.h>
#    include <netinet/in.h>
#    include <sys/auxv.h>
#    include <sys/sysinfo.h>
#    include <sys/statvfs.h>
#    include <sys/utsname.h>
#    include <unistd.h>
#    include <cstdio>
#    include <cstdlib>
#    include <ctime>
#    ifdef NEFORCE_ARCH_X86
#        include <cpuid.h>
#    endif
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
    mutex& sysinfo_mutex() {
        static mutex sysmutex;
        return sysmutex;
    }

    bool char_equals_ignored_case(const char lhs, const char rhs) noexcept {
        const auto fold = [](const char c) noexcept -> char {
            return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + ('a' - 'A')) : c;
        };
        return fold(lhs) == fold(rhs);
    }

    size_t find_ignored_case(const string_view text, const string_view needle) noexcept {
        if (needle.empty() || text.size() < needle.size()) {
            return string_view::npos;
        }

        for (size_t i = 0; i + needle.size() <= text.size(); ++i) {
            size_t j = 0;
            while (j < needle.size() && char_equals_ignored_case(text[i + j], needle[j])) {
                ++j;
            }

            if (j == needle.size()) {
                return i;
            }
        }

        return string_view::npos;
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

    string unescape_mount_field(const string& value) {
        string unescaped;
        unescaped.reserve(value.size());
        for (size_t i = 0; i < value.size(); ++i) {
            if (value[i] == '\\' && i + 3 < value.size() && value[i + 1] >= '0' && value[i + 1] <= '7' &&
                value[i + 2] >= '0' && value[i + 2] <= '7' && value[i + 3] >= '0' && value[i + 3] <= '7') {
                const int code = (value[i + 1] - '0') * 64 + (value[i + 2] - '0') * 8 + (value[i + 3] - '0');
                unescaped.push_back(static_cast<char>(code));
                i += 3;
            } else {
                unescaped.push_back(value[i]);
            }
        }
        return unescaped;
    }

    uint8_t prefix_length_of(const uint32_t mask) noexcept {
        uint32_t value = mask;
        uint8_t bits = 0;
        while ((value & 0x80000000U) != 0) {
            ++bits;
            value <<= 1;
        }
        return bits;
    }

    string mac_address_of(const ::sockaddr_ll& address) {
        const size_t count = address.sll_halen > 6 ? 6 : static_cast<size_t>(address.sll_halen);
        string text;
        text.reserve(count * 3);
        constexpr string_view digits = "0123456789ABCDEF";
        for (size_t i = 0; i < count; ++i) {
            if (i > 0) {
                text.push_back(':');
            }
            text.push_back(digits[(address.sll_addr[i] >> 4) & 0x0F]);
            text.push_back(digits[address.sll_addr[i] & 0x0F]);
        }
        return text;
    }

    vector<sysinfo::mount_point> read_mount_entries() {
        vector<sysinfo::mount_point> entries;
        const string text = read_file_all("/proc/mounts");
        size_t position = 0;
        string line;
        while (getline(text.view(), position, line)) {
            const auto parts = line.split(" \t");
            if (parts.size() < 4) {
                continue;
            }
            sysinfo::mount_point item;
            item.source = unescape_mount_field(parts[0]);
            item.target = unescape_mount_field(parts[1]);
            item.filesystem = parts[2];
            item.options = parts[3];
            struct ::statvfs value{};
            if (::statvfs(item.target.data(), &value) == 0) {
                const uint64_t block_size = value.f_frsize != 0 ? value.f_frsize : value.f_bsize;
                item.total_bytes = static_cast<uint64_t>(value.f_blocks) * block_size;
                item.free_bytes = static_cast<uint64_t>(value.f_bfree) * block_size;
                item.available_bytes = static_cast<uint64_t>(value.f_bavail) * block_size;
            }
            entries.push_back(move(item));
        }
        return entries;
    }

    string filesystem_type_of(const string& target) {
        char resolved[PATH_MAX] = {};
        if (::realpath(target.data(), resolved) == nullptr) {
            return {};
        }
        const string absolute = resolved;
        string best_target;
        string best_type;
        for (const auto& entry: read_mount_entries()) {
            bool matches = false;
            if (absolute == entry.target) {
                matches = true;
            } else if (entry.target == "/") {
                matches = true;
            } else if (absolute.size() > entry.target.size() && absolute.starts_with(entry.target.view()) &&
                       absolute[entry.target.size()] == '/') {
                matches = true;
            }
            if (matches && entry.target.size() >= best_target.size()) {
                best_target = entry.target;
                best_type = entry.filesystem;
            }
        }
        return best_type;
    }

    uint64_t read_number_file(const string& file_path) {
        const string text = read_file_all(file_path.data());
        string_view value = text.view();
        value = value.trim();
        if (value.empty()) {
            return 0;
        }
        try {
            return to_uint64(value);
        } catch (const exception&) {
            return 0;
        }
    }

    string read_trimmed_file(const string& file_path) {
        const string text = read_file_all(file_path.data());
        string_view value = text.view();
        value = value.trim();
        return string(value);
    }

    void fill_network_stats(sysinfo::network_interface& iface) {
        const string base = "/sys/class/net/" + iface.name + "/";
        iface.mtu = read_number_file(base + "mtu");
        const uint64_t speed = read_number_file(base + "speed");
        iface.speed_bps = (speed > 0 && speed < 1000000ULL) ? speed * 1000000ULL : 0;
        iface.operstate = read_trimmed_file(base + "operstate");
        iface.rx_bytes = read_number_file(base + "statistics/rx_bytes");
        iface.tx_bytes = read_number_file(base + "statistics/tx_bytes");
        iface.rx_packets = read_number_file(base + "statistics/rx_packets");
        iface.tx_packets = read_number_file(base + "statistics/tx_packets");
        iface.rx_errors = read_number_file(base + "statistics/rx_errors");
        iface.tx_errors = read_number_file(base + "statistics/tx_errors");
        iface.rx_dropped = read_number_file(base + "statistics/rx_dropped");
        iface.tx_dropped = read_number_file(base + "statistics/tx_dropped");
        if (iface.operstate.empty()) {
            iface.operstate = iface.is_up ? "up" : "down";
        }
    }

    sysinfo::battery_info read_battery_linux() {
        sysinfo::battery_info info;
        info.status = "unknown";
        const path supplies("/sys/class/power_supply");
        double weighted_percent = 0.0;
        double weight_sum = 0.0;
        uint64_t energy_now_sum = 0;
        uint64_t energy_full_sum = 0;
        uint64_t power_sum = 0;
        bool discharging = false;
        for (const auto& entry: supplies.children()) {
            const string base = entry.str() + "/";
            const string type = read_trimmed_file(base + "type");
            if (type == "Mains" || type == "USB" || type == "USB_C") {
                if (read_number_file(base + "online") == 1) {
                    info.on_ac = true;
                }
                continue;
            }
            if (type != "Battery") {
                continue;
            }
            info.present = true;
            const string status = read_trimmed_file(base + "status");
            if (status == "Charging") {
                info.charging = true;
            } else if (status == "Discharging") {
                discharging = true;
            }
            const uint64_t energy_now = read_number_file(base + "energy_now");
            const uint64_t energy_full = read_number_file(base + "energy_full");
            uint64_t power_now = read_number_file(base + "power_now");
            if (power_now == 0) {
                const uint64_t voltage = read_number_file(base + "voltage_now");
                const uint64_t current = read_number_file(base + "current_now");
                power_now = voltage * current / 1000000ULL;
            }
            energy_now_sum += energy_now;
            energy_full_sum += energy_full > 0 ? energy_full : energy_now;
            power_sum += power_now;
            const uint64_t capacity = read_number_file(base + "capacity");
            const double weight = energy_now > 0 ? static_cast<double>(energy_now) : 1.0;
            if (capacity <= 100) {
                weighted_percent += static_cast<double>(capacity) * weight;
                weight_sum += weight;
            }
        }
        if (info.present && weight_sum > 0.0) {
            info.charge_percent = weighted_percent / weight_sum;
        }
        if (info.present && power_sum > 0 && energy_now_sum > 0) {
            if (info.charging && energy_full_sum > energy_now_sum) {
                info.remaining_seconds = static_cast<int64_t>((energy_full_sum - energy_now_sum) * 3600ULL / power_sum);
            } else {
                info.remaining_seconds = static_cast<int64_t>(energy_now_sum * 3600ULL / power_sum);
            }
        }
        if (energy_now_sum > 0) {
            info.capacity_mwh = static_cast<float64_t>(energy_now_sum) / 1000.0;
        }
        if (energy_full_sum > 0) {
            info.full_capacity_mwh = static_cast<float64_t>(energy_full_sum) / 1000.0;
        }
        if (power_sum > 0) {
            info.rate_mw = static_cast<float64_t>(power_sum) / 1000.0;
        }
        if (!info.present) {
            info.status = "unknown";
        } else if (info.charging) {
            info.status = "charging";
        } else if (info.on_ac && info.charge_percent >= 99.0) {
            info.status = "full";
        } else if (info.on_ac) {
            info.status = "idle";
        } else if (discharging) {
            info.status = "discharging";
        }
        return info;
    }

#else

    uint64_t filetime_ticks(const ::FILETIME& value) noexcept {
        ::ULARGE_INTEGER ticks{};
        ticks.LowPart = value.dwLowDateTime;
        ticks.HighPart = value.dwHighDateTime;
        return ticks.QuadPart;
    }

    string mac_address_of(const ::BYTE* bytes, const ::ULONG length) {
        if (bytes == nullptr) {
            return {};
        }
        const size_t count = length > 6 ? 6 : static_cast<size_t>(length);
        string text;
        text.reserve(count * 3);
        constexpr char digits[] = "0123456789ABCDEF";
        for (size_t i = 0; i < count; ++i) {
            if (i > 0) {
                text.push_back(':');
            }
            text.push_back(digits[(bytes[i] >> 4) & 0x0F]);
            text.push_back(digits[bytes[i] & 0x0F]);
        }
        return text;
    }

    const char* oper_state_name(const ::IF_OPER_STATUS status) noexcept {
        switch (status) {
            case IfOperStatusUp:
                return "up";
            case IfOperStatusDown:
                return "down";
            case IfOperStatusTesting:
                return "testing";
            case IfOperStatusUnknown:
                return "unknown";
            case IfOperStatusDormant:
                return "dormant";
            case IfOperStatusNotPresent:
                return "notpresent";
            case IfOperStatusLowerLayerDown:
                return "lowerlayerdown";
            default:
                return "unknown";
        }
    }

    void fill_ipv4_address(sysinfo::network_interface& iface, const ::SOCKADDR* address, const ::ULONG prefix) {
        const auto* value = reinterpret_cast<const ::SOCKADDR_IN*>(address);
        char text[INET_ADDRSTRLEN] = {};
        if (::inet_ntop(AF_INET, &value->sin_addr, text, sizeof(text)) != nullptr) {
            iface.address = text;
        }
        const ::ULONG bits = prefix > 32 ? 32 : prefix;
        const uint32_t mask = bits == 0 ? 0 : (0xFFFFFFFFU << (32 - bits));
        iface.netmask =
                format("{}.{}.{}.{}", (mask >> 24) & 0xFF, (mask >> 16) & 0xFF, (mask >> 8) & 0xFF, mask & 0xFF);
        iface.prefix_length = static_cast<uint8_t>(bits);
    }

    void fill_ipv6_address(sysinfo::network_interface& iface, const ::SOCKADDR* address, const ::ULONG prefix) {
        const auto* value = reinterpret_cast<const ::SOCKADDR_IN6*>(address);
        char text[INET6_ADDRSTRLEN] = {};
        if (::inet_ntop(AF_INET6, &value->sin6_addr, text, sizeof(text)) != nullptr) {
            iface.address = text;
        }
        iface.prefix_length = static_cast<uint8_t>(prefix > 128 ? 128 : prefix);
    }


#endif

    void get_cpu_info_internal(sysinfo::cpu_info& cpu_info, const sysinfo::architecture arch) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        char brand[49] = {};

#    ifdef NEFORCE_ARCH_X86
        int cpu_info_data[4] = {-1};
        char vendor[13] = {};

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
#    else
        // read vendor and brand from registry
        ::HKEY cpu_hkey = nullptr;
        if (::RegOpenKeyExA(HKEY_LOCAL_MACHINE, R"(HARDWARE\DESCRIPTION\System\CentralProcessor\0)", 0, KEY_READ,
                            &cpu_hkey) == ERROR_SUCCESS) {
            char buf[256];
            ::DWORD size = sizeof(buf);

            if (::RegQueryValueExA(cpu_hkey, "VendorIdentifier", nullptr, nullptr, reinterpret_cast<::LPBYTE>(buf),
                                   &size) == ERROR_SUCCESS) {
                cpu_info.vendor = buf;
            }

            size = sizeof(buf);
            if (::RegQueryValueExA(cpu_hkey, "ProcessorNameString", nullptr, nullptr, reinterpret_cast<::LPBYTE>(buf),
                                   &size) == ERROR_SUCCESS) {
                cpu_info.brand = buf;
                cpu_info.brand.trim_right();
            }

            ::RegCloseKey(cpu_hkey);
        }
#    endif

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
        if (::RegOpenKeyExW(HKEY_LOCAL_MACHINE, LR"(HARDWARE\DESCRIPTION\System\CentralProcessor\0)", 0, KEY_READ,
                            &hkey) == ERROR_SUCCESS) {

            ::DWORD mhz = 0;
            ::DWORD size = sizeof(::DWORD);

            if (::RegQueryValueExW(hkey, L"~MHz", nullptr, nullptr, reinterpret_cast<::LPBYTE>(&mhz), &size) ==
                ERROR_SUCCESS) {
                cpu_info.current_mhz = mhz;
            }

            mhz = 0;
            size = sizeof(::DWORD);

            LONG result =
                    ::RegQueryValueExW(hkey, L"MaxMHz", nullptr, nullptr, reinterpret_cast<::LPBYTE>(&mhz), &size);

            if (result != ERROR_SUCCESS) {
                mhz = 0;
                size = sizeof(::DWORD);
                result = ::RegQueryValueExW(hkey, L"MaxClockSpeed", nullptr, nullptr, reinterpret_cast<::LPBYTE>(&mhz),
                                            &size);
            }

            if (result == ERROR_SUCCESS && mhz > 0) {
                cpu_info.max_mhz = mhz;
            }

            ::RegCloseKey(hkey);
        }

#    ifdef NEFORCE_ARCH_X86
        if (cpu_info.max_mhz == 0) {
            ::__cpuid(cpu_info_data, 0x16);
            if (cpu_info_data[0] != 0) {
                cpu_info.max_mhz = cpu_info_data[1];
                if (cpu_info.current_mhz == 0) {
                    cpu_info.current_mhz = cpu_info_data[0];
                }
            }
        }

        if (cpu_info.max_mhz == 0) {
            ::__cpuid(cpu_info_data, 0x15);
            if (cpu_info_data[0] != 0 && cpu_info_data[1] != 0 && cpu_info_data[2] != 0) {
                uint32_t crystal_clock = cpu_info_data[2];
                if (crystal_clock == 0) {
                    crystal_clock = 38400000;
                }

                cpu_info.max_mhz = (crystal_clock / 1000000) * cpu_info_data[1] / cpu_info_data[0];
            }
        }
#    endif

        if (cpu_info.max_mhz == 0) {
            cpu_info.max_mhz = sysinfo::parse_brand_frequency(brand);
            if (cpu_info.max_mhz != 0 && cpu_info.current_mhz == 0) {
                cpu_info.current_mhz = cpu_info.max_mhz;
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

        if (arch == sysinfo::architecture::riscv32 || arch == sysinfo::architecture::riscv64) {
            vendor_key = "mvendorid";
            brand_key = "uarch";
            flags_key = "isa";
            features_key = nullptr;
            freq_key = nullptr;
            cores_key = nullptr;
        } else if (arch == sysinfo::architecture::loongarch64 || arch == sysinfo::architecture::loongarch32) {
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
                if (arch == sysinfo::architecture::riscv32 || arch == sysinfo::architecture::riscv64) {
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
                        cpu_info.current_mhz = to_uint32(line.view(colon + 1));
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
                    cpu_info.current_mhz > 0 && core_count_from_cpuinfo > 0) {
                    first_processor = false;
                }
            }

            if (cpu_info.current_mhz == 0 && line.starts_with("cpu MHz")) {
                const size_t colon = line.find(':');
                if (colon != string::npos) {
                    cpu_info.current_mhz = to_uint32(line.view(colon + 1));
                }
            }
        }

        if (arch == sysinfo::architecture::riscv32 || arch == sysinfo::architecture::riscv64) {
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

        if (cpu_info.current_mhz == 0 || cpu_info.max_mhz == 0) {
            auto try_sysfs = [&cpu_info]() {
                cpu_info.max_mhz = read_sysfs_freq("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
                if (cpu_info.max_mhz == 0) {
                    cpu_info.max_mhz = read_sysfs_freq("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq");
                }
                if (cpu_info.current_mhz == 0) {
                    cpu_info.current_mhz = read_sysfs_freq("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");
                }
            };
            try_sysfs();
        }

#    ifdef NEFORCE_ARCH_X86
        if (cpu_info.max_mhz == 0) {
            unsigned int leaf_16[4] = {0, 0, 0, 0};
            if (::__get_cpuid(0x16, &leaf_16[0], &leaf_16[1], &leaf_16[2], &leaf_16[3]) != 0) {
                cpu_info.max_mhz = (leaf_16[1] != 0) ? leaf_16[1] : leaf_16[0];
                if (cpu_info.current_mhz == 0) {
                    cpu_info.current_mhz = leaf_16[0];
                }
            }
        }

        if (cpu_info.max_mhz == 0) {
            unsigned int leaf_15[4] = {0, 0, 0, 0};
            if (::__get_cpuid(0x15, &leaf_15[0], &leaf_15[1], &leaf_15[2], &leaf_15[3]) != 0 && leaf_15[0] != 0 &&
                leaf_15[1] != 0 && leaf_15[2] != 0) {
                cpu_info.max_mhz = (leaf_15[2] / 1000000) * leaf_15[1] / leaf_15[0];
            }
        }
#    endif

        if (cpu_info.max_mhz == 0 && !cpu_info.brand.empty()) {
            cpu_info.max_mhz = sysinfo::parse_brand_frequency(cpu_info.brand.view());
            if (cpu_info.max_mhz != 0 && cpu_info.current_mhz == 0) {
                cpu_info.current_mhz = cpu_info.max_mhz;
            }
        }
#endif
    }

    void get_os_version_internal(sysinfo::os_version_info& os_version_info) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        const ::HMODULE ntdll = ::GetModuleHandleW(L"ntdll.dll");
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
        if (::RegOpenKeyExW(HKEY_LOCAL_MACHINE, LR"(SOFTWARE\Microsoft\Windows NT\CurrentVersion)", 0, KEY_READ,
                            &hkey) == ERROR_SUCCESS) {

            wchar_t product_name[256] = {};
            ::DWORD size = sizeof(product_name);

            if (::RegQueryValueExW(hkey, L"ProductName", nullptr, nullptr, reinterpret_cast<::LPBYTE>(product_name),
                                   &size) == ERROR_SUCCESS) {
                os_version_info.product_name = wcharacter::to_string(product_name);
            }
            if (os_version_info.major == 0) {
                ::DWORD value = 0;
                ::DWORD value_size = sizeof(value);
                if (::RegQueryValueExW(hkey, L"CurrentMajorVersionNumber", nullptr, nullptr,
                                       reinterpret_cast<::LPBYTE>(&value), &value_size) == ERROR_SUCCESS) {
                    os_version_info.major = value;
                }
                value_size = sizeof(value);
                if (::RegQueryValueExW(hkey, L"CurrentMinorVersionNumber", nullptr, nullptr,
                                       reinterpret_cast<::LPBYTE>(&value), &value_size) == ERROR_SUCCESS) {
                    os_version_info.minor = value;
                }
                wchar_t build[32] = {};
                value_size = sizeof(build);
                if (::RegQueryValueExW(hkey, L"CurrentBuildNumber", nullptr, nullptr, reinterpret_cast<::LPBYTE>(build),
                                       &value_size) == ERROR_SUCCESS) {
                    os_version_info.build = static_cast<uint32_t>(to_uint64(wcharacter::to_string(build).view()));
                }
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

uint32_t sysinfo::parse_brand_frequency(const string_view brand) noexcept {
    const auto digits_before = [&brand](const size_t unit_pos) noexcept -> size_t {
        size_t start = unit_pos;
        while (start > 0 && (is_digit(brand[start - 1]) || brand[start - 1] == '.')) {
            --start;
        }
        return start;
    };

    const size_t ghz_pos = find_ignored_case(brand, "ghz");
    if (ghz_pos != string_view::npos) {
        const size_t start = digits_before(ghz_pos);
        if (start < ghz_pos) {
            try {
                const float32_t freq_ghz = float32::parse(brand.substr(start, ghz_pos - start)).value();
                return static_cast<uint32_t>(_NEFORCE round(freq_ghz * 1000.0F));
                // NOLINTNEXTLINE(bugprone-empty-catch)
            } catch (...) {
                // ignore
            }
        }
        return 0;
    }

    const size_t mhz_pos = find_ignored_case(brand, "mhz");
    if (mhz_pos != string_view::npos) {
        const size_t start = digits_before(mhz_pos);
        if (start < mhz_pos) {
            try {
                return uinteger32::parse(brand.substr(start, mhz_pos - start)).value();
                // NOLINTNEXTLINE(bugprone-empty-catch)
            } catch (...) {
                // ignore
            }
        }
    }

    return 0;
}

string sysinfo::os_version_info::version() const {
    return to_string(major) + "." + to_string(minor) + "." + to_string(build);
}

// NOLINTNEXTLINE(bugprone-exception-escape)
sysinfo::sysinfo() noexcept {
    try {
        {
            lock<mutex> lock(sysinfo_mutex());
            init();
        }
        probe_hardware();
    } catch (const exception& e) {
        NEFORCE_REPORT_EXCEPTION(e);
        initialized_.store(false, memory_order_release);
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
            architecture_ = architecture::x86;
            break;
        }
        case PROCESSOR_ARCHITECTURE_AMD64: {
            architecture_ = architecture::x64;
            break;
        }
        case PROCESSOR_ARCHITECTURE_ARM: {
            architecture_ = architecture::arm;
            break;
        }
        case PROCESSOR_ARCHITECTURE_ARM64: {
            architecture_ = architecture::arm64;
            break;
        }
        default: {
            architecture_ = architecture::unknown;
            break;
        }
    }

    get_cpu_info_internal(cpu_info_, architecture_);
    cpu_info_.simd = probe_simd();
    get_os_version_internal(os_version_info_);

    hostname_.clear();
    wchar_t computer_name[256] = {};
    auto computer_name_length = static_cast<::DWORD>(sizeof(computer_name) / sizeof(wchar_t));
    if (::GetComputerNameW(computer_name, &computer_name_length) == TRUE) {
        hostname_ = wcharacter::to_string(computer_name);
    }
    if (hostname_.empty()) {
        wchar_t environment_name[256] = {};
        const ::DWORD length = ::GetEnvironmentVariableW(
                L"COMPUTERNAME", environment_name, static_cast<::DWORD>(sizeof(environment_name) / sizeof(wchar_t)));
        if (length > 0) {
            hostname_ = wcharacter::to_string(environment_name);
        }
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
            architecture_ = architecture::x64;
        } else if (machine == "i686" || machine == "i386") {
            architecture_ = architecture::x86;
        } else if (machine == "arm" || machine == "armv7l") {
            architecture_ = architecture::arm;
        } else if (machine == "aarch64") {
            architecture_ = architecture::arm64;
        } else if (machine == "riscv32") {
            architecture_ = architecture::riscv32;
        } else if (machine == "riscv64") {
            architecture_ = architecture::riscv64;
        } else if (machine == "loongarch32") {
            architecture_ = architecture::loongarch32;
        } else if (machine == "loongarch64") {
            architecture_ = architecture::loongarch64;
        } else {
            architecture_ = architecture::unknown;
        }
    }

    get_cpu_info_internal(cpu_info_, architecture_);
    cpu_info_.simd = probe_simd();
    get_os_version_internal(os_version_info_);

    hostname_.clear();
    char computer_name[256] = {};
    if (::gethostname(computer_name, sizeof(computer_name) - 1) == 0) {
        computer_name[sizeof(computer_name) - 1] = '\0';
        hostname_ = computer_name;
    }

#endif

    numa_nodes_.clear();

#ifdef NEFORCE_PLATFORM_WINDOWS
    {
        ::DWORD buf_size = 0;
        ::GetLogicalProcessorInformation(nullptr, &buf_size);
        if (::GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            auto* buf = static_cast<::SYSTEM_LOGICAL_PROCESSOR_INFORMATION*>(::malloc(buf_size));
            if (buf != nullptr) {
                if (::GetLogicalProcessorInformation(buf, &buf_size) == TRUE) {
                    const ::DWORD count = buf_size / sizeof(::SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
                    for (::DWORD i = 0; i < count; i++) {
                        if (buf[i].Relationship == ::RelationNumaNode) {
                            const auto node_id = static_cast<uint32_t>(buf[i].NumaNode.NodeNumber);
                            const auto mask = static_cast<uint64_t>(buf[i].ProcessorMask);

                            numa_node_info* target = nullptr;
                            for (auto& node: numa_nodes_) {
                                if (node.node_id == node_id) {
                                    target = &node;
                                    break;
                                }
                            }
                            if (target == nullptr) {
                                numa_node_info info;
                                info.node_id = node_id;
                                numa_nodes_.push_back(move(info));
                                target = &numa_nodes_.back();
                            }

                            target->core_mask |= mask;
                            target->core_list.clear();
                            uint64_t temp = target->core_mask;
                            for (uint32_t bit = 0; bit < 64 && temp != 0; bit++, temp >>= 1) {
                                if ((temp & 1) != 0U) {
                                    target->core_list.push_back(bit);
                                }
                            }
                            target->core_count = static_cast<uint32_t>(target->core_list.size());
                        }
                    }
                }
                ::free(buf);
            }
        }
    }
#else
    for (uint32_t node_id = 0; node_id < 256; node_id++) {
        string cpulist_path = "/sys/devices/system/node/node";
        cpulist_path += to_string(node_id);
        cpulist_path += "/cpulist";
        string content = read_file_all(cpulist_path.data()).trim_right();
        if (content.empty()) {
            break;
        }

        numa_node_info info;
        info.node_id = node_id;

        // parse cpulist: "0-3,8-11" or "0"
        size_t pos = 0;
        string_view token;
        while (getline(content.view(), pos, token, [](char c) { return c == ','; })) {
            if (token.empty()) {
                continue;
            }
            uint32_t start_core = 0;
            uint32_t end_core = 0;

            const auto dash_pos = token.find('-');
            if (dash_pos != string::npos) {
                start_core = to_uint32(token.substr(0, dash_pos));
                end_core = to_uint32(token.substr(dash_pos + 1));
            } else {
                start_core = to_uint32(token);
                end_core = start_core;
            }

            for (uint32_t core = start_core; core <= end_core; core++) {
                info.core_list.push_back(core);
                if (core < 64) {
                    info.core_mask |= (static_cast<uint64_t>(1) << core);
                }
            }
        }

        info.core_count = static_cast<uint32_t>(info.core_list.size());
        numa_nodes_.push_back(move(info));
    }
#endif

    initialized_.store(true, memory_order_release);
}

sysinfo& sysinfo::instance() noexcept {
    static sysinfo instance;
    return instance;
}

string sysinfo::host_name() const { return hostname_; }

sysinfo::os_version_info sysinfo::os_version() const { return os_version_info_; }

void sysinfo::refresh() noexcept {
    try {
        lock<mutex> lock(sysinfo_mutex());
        initialized_.store(false, memory_order_release);
        init();
    } catch (const exception& e) {
        NEFORCE_REPORT_EXCEPTION(e);
        initialized_.store(false, memory_order_release);
    }
}

sysinfo::memory_info sysinfo::memory_status() {
    memory_info info;
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::MEMORYSTATUSEX mem_status{};
    mem_status.dwLength = sizeof(mem_status);
    if (::GlobalMemoryStatusEx(&mem_status) == TRUE) {
        info.total_physical = mem_status.ullTotalPhys;
        info.available_physical = mem_status.ullAvailPhys;
        info.total_virtual = mem_status.ullTotalVirtual;
        info.available_virtual = mem_status.ullAvailVirtual;
        info.total_page_file = mem_status.ullTotalPageFile;
        info.available_page_file = mem_status.ullAvailPageFile;
    }
    ::PERFORMANCE_INFORMATION performance{};
    if (::GetPerformanceInfo(&performance, sizeof(performance)) == TRUE) {
        info.cached_physical = static_cast<uint64_t>(performance.SystemCache) * performance.PageSize;
    }
#else
    struct ::sysinfo linux_sysinfo{};
    if (::sysinfo(&linux_sysinfo) == 0) {
        info.total_physical = linux_sysinfo.totalram * linux_sysinfo.mem_unit;
        info.available_physical = linux_sysinfo.freeram * linux_sysinfo.mem_unit;
        info.total_page_file = linux_sysinfo.totalswap * linux_sysinfo.mem_unit;
        info.available_page_file = linux_sysinfo.freeswap * linux_sysinfo.mem_unit;
        info.total_virtual = info.total_physical + info.total_page_file;
        info.available_virtual = info.available_physical + info.available_page_file;
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
                        info.total_physical = to_uint64(value) * 1024;
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
                        info.available_physical = to_uint64(value) * 1024;
                    }
                }
            } else if (line.starts_with("Cached:") || line.starts_with("SReclaimable:")) {
                const size_t colon = line.find(':');
                if (colon != string::npos) {
                    string_view value = line.view(colon + 1);
                    ignore = value.trim_left();
                    if (value.ends_with("kB")) {
                        value = value.substr(0, value.length() - 2);
                    }
                    ignore = value.trim_right();
                    if (!value.empty()) {
                        info.cached_physical += to_uint64(value) * 1024;
                    }
                }
            }
        }
    }
#endif
    return info;
}

float64_t sysinfo::cpu_usage() {
    static mutex usage_mutex;
    static uint64_t previous_total = 0;
    static uint64_t previous_idle = 0;

    lock<mutex> guard(usage_mutex);

    uint64_t total = 0;
    uint64_t idle = 0;
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::FILETIME idle_time{};
    ::FILETIME kernel_time{};
    ::FILETIME user_time{};
    if (::GetSystemTimes(&idle_time, &kernel_time, &user_time) == FALSE) {
        return 0.0;
    }
    total = filetime_ticks(kernel_time) + filetime_ticks(user_time);
    idle = filetime_ticks(idle_time);
#else
    const string content = read_file_all("/proc/stat");
    if (content.empty()) {
        return 0.0;
    }
    string line;
    size_t position = 0;
    getline(content.view(), position, line);
    if (!line.starts_with("cpu ")) {
        return 0.0;
    }
    const string_view data = line.view(4);
    string_view token;
    uint64_t fields[8] = {};
    size_t offset = 0;
    int index = 0;
    while (index < 8 && getline(data, offset, token, is_space<char>)) {
        if (!token.empty()) {
            fields[index++] = to_uint64(token);
        }
    }
    for (const uint64_t value: fields) {
        total += value;
    }
    idle = fields[3] + fields[4];
#endif

    float64_t usage = 0.0;
    if (previous_total != 0 && total > previous_total) {
        const uint64_t total_delta = total - previous_total;
        const uint64_t idle_delta = idle > previous_idle ? idle - previous_idle : 0;
        if (total_delta > idle_delta) {
            usage = 100.0 * static_cast<float64_t>(total_delta - idle_delta) / static_cast<float64_t>(total_delta);
        }
    }
    previous_total = total;
    previous_idle = idle;
    return usage;
}

sysinfo::simd_info sysinfo::probe_simd() {
    static const simd_info cached = []() -> simd_info {
        simd_info info;
#if defined(NEFORCE_ARCH_X86)
        unsigned int eax = 0;
        unsigned int ebx = 0;
        unsigned int ecx = 0;
        unsigned int edx = 0;
#    ifdef NEFORCE_PLATFORM_WINDOWS
        int registers[4] = {0, 0, 0, 0};
        ::__cpuid(registers, 1);
        ecx = static_cast<unsigned int>(registers[2]);
        edx = static_cast<unsigned int>(registers[3]);
#    else
        if (::__get_cpuid(1, &eax, &ebx, &ecx, &edx) == 0) {
            return info;
        }
#    endif
        ignore = eax;
        ignore = ebx;

        if ((edx & (1U << 26)) != 0) {
            info.features |= static_cast<uint32_t>(simd_instruction::sse2);
        }
        if ((ecx & (1U << 9)) != 0) {
            info.features |= static_cast<uint32_t>(simd_instruction::ssse3);
        }
        if ((ecx & (1U << 19)) != 0) {
            info.features |= static_cast<uint32_t>(simd_instruction::sse4_1);
        }
        if ((ecx & (1U << 20)) != 0) {
            info.features |= static_cast<uint32_t>(simd_instruction::sse4_2);
        }
        if ((ecx & (1U << 25)) != 0) {
            info.features |= static_cast<uint32_t>(simd_instruction::aes);
        }
        if ((ecx & (1U << 1)) != 0) {
            info.features |= static_cast<uint32_t>(simd_instruction::pclmulqdq);
        }

        // AVX family requires the OS to preserve YMM/ZMM state through XSAVE.
        uint64_t xcr0 = 0;
        if ((ecx & (1U << 27)) != 0) {
#    ifdef NEFORCE_PLATFORM_WINDOWS
            xcr0 = ::_xgetbv(0);
#    else
            unsigned int xcr0_low = 0;
            unsigned int xcr0_high = 0;
            __asm__ volatile(".byte 0x0f, 0x01, 0xd0" : "=a"(xcr0_low), "=d"(xcr0_high) : "c"(0));
            xcr0 = (static_cast<uint64_t>(xcr0_high) << 32) | xcr0_low;
#    endif
        }
        const bool has_ymm = (xcr0 & 0x6) == 0x6;
        const bool has_zmm = (xcr0 & 0xE0) == 0xE0;
        if ((ecx & (1U << 28)) != 0 && has_ymm) {
            info.features |= static_cast<uint32_t>(simd_instruction::avx);
        }
        if ((ecx & (1U << 12)) != 0 && has_ymm) {
            info.features |= static_cast<uint32_t>(simd_instruction::fma);
        }

        unsigned int leaf7_ebx = 0;
#    ifdef NEFORCE_PLATFORM_WINDOWS
        ::__cpuidex(registers, 7, 0);
        leaf7_ebx = static_cast<unsigned int>(registers[1]);
#    else
        if (::__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx) != 0) {
            leaf7_ebx = ebx;
        }
#    endif
        if ((leaf7_ebx & (1U << 5)) != 0 && has_ymm) {
            info.features |= static_cast<uint32_t>(simd_instruction::avx2);
        }
        if ((leaf7_ebx & (1U << 16)) != 0 && has_zmm) {
            info.features |= static_cast<uint32_t>(simd_instruction::avx512f);
        }
#elif defined(NEFORCE_ARCH_ARM)
#    ifdef NEFORCE_PLATFORM_WINDOWS
#        if defined(NEFORCE_ARCH_AARCH64)
        info.features |= static_cast<uint32_t>(simd_instruction::neon);
#        else
        if (::IsProcessorFeaturePresent(PF_ARM_NEON_INSTRUCTIONS_AVAILABLE) != FALSE) {
            info.features |= static_cast<uint32_t>(simd_instruction::neon);
        }
#        endif
#    else
        // Linux UAPI HWCAP bits: NEON(12) on arm32, ASIMD(1), AES(3), PMULL(4), SVE(22) on aarch64.
        const unsigned long hwcap = ::getauxval(AT_HWCAP);
#        if defined(NEFORCE_ARCH_AARCH64)
        if ((hwcap & (1UL << 1)) != 0) {
            info.features |= static_cast<uint32_t>(simd_instruction::neon);
        }
        if ((hwcap & (1UL << 3)) != 0) {
            info.features |= static_cast<uint32_t>(simd_instruction::aes);
        }
        if ((hwcap & (1UL << 4)) != 0) {
            info.features |= static_cast<uint32_t>(simd_instruction::pclmulqdq);
        }
        if ((hwcap & (1UL << 22)) != 0) {
            info.features |= static_cast<uint32_t>(simd_instruction::sve);
        }
#        else
        if ((hwcap & (1UL << 12)) != 0) {
            info.features |= static_cast<uint32_t>(simd_instruction::neon);
        }
#        endif
#    endif
#endif
        return info;
    }();
    return cached;
}

vector<float64_t> sysinfo::cpu_usage_per_core() {
    static mutex core_mutex;
    lock<mutex> guard(core_mutex);
    vector<float64_t> usage;
#ifdef NEFORCE_PLATFORM_WINDOWS
    struct processor_performance {
        ::LARGE_INTEGER idle_time;
        ::LARGE_INTEGER kernel_time;
        ::LARGE_INTEGER user_time;
        ::LARGE_INTEGER dpc_time;
        ::LARGE_INTEGER interrupt_time;
        ::ULONG interrupt_count;
    };
    // Raw per-core counters are exact cumulative CPU time, so deltas stay meaningful even for
    // very short sampling intervals; PDH formatted percentages turn invalid in that case.
    static vector<uint64_t> previous_idle;
    static vector<uint64_t> previous_total;
    using nt_query_system_information_fn = ::LONG(WINAPI*)(::ULONG, void*, ::ULONG, ::PULONG);
    constexpr ::ULONG system_processor_performance_information = 8;
    const ::HMODULE ntdll = ::GetModuleHandleW(L"ntdll.dll");
    if (ntdll == nullptr) {
        return usage;
    }
    const auto query =
            reinterpret_cast<nt_query_system_information_fn>(::GetProcAddress(ntdll, "NtQuerySystemInformation"));
    if (query == nullptr) {
        return usage;
    }
    ::ULONG required = 0;
    query(system_processor_performance_information, nullptr, 0, &required);
    if (required == 0) {
        return usage;
    }
    const size_t capacity = static_cast<size_t>(required) + sizeof(processor_performance) * 64;
    vector<::BYTE> buffer(capacity);
    ::ULONG returned = 0;
    if (query(system_processor_performance_information, buffer.data(), static_cast<::ULONG>(buffer.size()), &returned) <
        0) {
        return usage;
    }
    const auto* entries = reinterpret_cast<const processor_performance*>(buffer.data());
    const size_t cores = static_cast<size_t>(returned) / sizeof(processor_performance);
    if (cores == 0) {
        return usage;
    }
    vector<uint64_t> idles(cores, 0);
    vector<uint64_t> totals(cores, 0);
    for (size_t index = 0; index < cores; ++index) {
        idles[index] = static_cast<uint64_t>(entries[index].idle_time.QuadPart);
        totals[index] = static_cast<uint64_t>(entries[index].kernel_time.QuadPart) +
                        static_cast<uint64_t>(entries[index].user_time.QuadPart);
    }
    usage.assign(cores, 0.0);
    if (previous_idle.size() != cores || previous_total.size() != cores) {
        previous_idle = move(idles);
        previous_total = move(totals);
        return usage;
    }
    for (size_t index = 0; index < cores; ++index) {
        if (totals[index] <= previous_total[index] || idles[index] < previous_idle[index]) {
            continue;
        }
        const uint64_t total_delta = totals[index] - previous_total[index];
        const uint64_t idle_delta = idles[index] - previous_idle[index];
        if (total_delta > idle_delta) {
            usage[index] =
                    100.0 * static_cast<float64_t>(total_delta - idle_delta) / static_cast<float64_t>(total_delta);
        }
    }
    previous_idle = move(idles);
    previous_total = move(totals);
#else
    static vector<uint64_t> previous_total;
    static vector<uint64_t> previous_idle;
    const string content = read_file_all("/proc/stat");
    if (content.empty()) {
        return usage;
    }
    vector<uint64_t> totals;
    vector<uint64_t> idles;
    size_t position = 0;
    string line;
    while (getline(content.view(), position, line)) {
        if (!line.starts_with("cpu") || line.size() < 4 || line[3] < '0' || line[3] > '9') {
            continue;
        }
        size_t begin = 3;
        while (begin < line.size() && line[begin] >= '0' && line[begin] <= '9') {
            ++begin;
        }
        const string_view data = line.view(begin);
        string_view token;
        uint64_t fields[8] = {};
        size_t offset = 0;
        int index = 0;
        while (index < 8 && getline(data, offset, token, is_space<char>)) {
            if (!token.empty()) {
                fields[index++] = to_uint64(token);
            }
        }
        uint64_t total = 0;
        for (const uint64_t value: fields) {
            total += value;
        }
        totals.push_back(total);
        idles.push_back(fields[3] + fields[4]);
    }
    if (totals.empty()) {
        return usage;
    }
    if (previous_total.size() != totals.size() || previous_idle.size() != idles.size()) {
        previous_total = totals;
        previous_idle = move(idles);
        usage.assign(totals.size(), 0.0);
        return usage;
    }
    usage.resize(totals.size(), 0.0);
    for (size_t index = 0; index < totals.size(); ++index) {
        if (totals[index] <= previous_total[index] || idles[index] < previous_idle[index]) {
            continue;
        }
        const uint64_t total_delta = totals[index] - previous_total[index];
        const uint64_t idle_delta = idles[index] - previous_idle[index];
        if (total_delta > idle_delta) {
            usage[index] =
                    100.0 * static_cast<float64_t>(total_delta - idle_delta) / static_cast<float64_t>(total_delta);
        }
    }
    previous_total = move(totals);
    previous_idle = move(idles);
#endif
    return usage;
}


sysinfo::cpu_breakdown_info sysinfo::cpu_usage_breakdown() {
    static mutex breakdown_mutex;
    static vector<uint64_t> previous_fields;
    lock<mutex> guard(breakdown_mutex);
    cpu_breakdown_info result;
#ifdef NEFORCE_PLATFORM_WINDOWS
    static ::PDH_HQUERY breakdown_query = nullptr;
    static ::PDH_HCOUNTER user_counter = nullptr;
    static ::PDH_HCOUNTER privileged_counter = nullptr;
    static ::PDH_HCOUNTER interrupt_counter = nullptr;
    static ::PDH_HCOUNTER dpc_counter = nullptr;
    static ::PDH_HCOUNTER idle_counter = nullptr;
    static bool has_previous_raw = false;
    static uint64_t previous_user = 0;
    static uint64_t previous_privileged = 0;
    static uint64_t previous_interrupt = 0;
    static uint64_t previous_dpc = 0;
    static uint64_t previous_idle = 0;
    // Percentage counters expose cumulative CPU time as their raw first value. Deriving the
    // percentages from consecutive raw deltas keeps the breakdown self-consistent even when
    // callers sample with a very short interval, where formatted PDH values are still noisy.
    const auto read_raw = [](const ::PDH_HCOUNTER counter, uint64_t& out) {
        ::PDH_RAW_COUNTER value{};
        if (::PdhGetRawCounterValue(counter, nullptr, &value) != ERROR_SUCCESS) {
            return false;
        }
        if (value.CStatus != PDH_CSTATUS_VALID_DATA && value.CStatus != PDH_CSTATUS_NEW_DATA) {
            return false;
        }
        out = static_cast<uint64_t>(value.FirstValue);
        return true;
    };
    const auto remember_raw = [](const uint64_t user, const uint64_t privileged, const uint64_t interrupt,
                                 const uint64_t dpc, const uint64_t idle) {
        has_previous_raw = true;
        previous_user = user;
        previous_privileged = privileged;
        previous_interrupt = interrupt;
        previous_dpc = dpc;
        previous_idle = idle;
    };
    if (breakdown_query == nullptr) {
        if (::PdhOpenQueryW(nullptr, 0, &breakdown_query) != ERROR_SUCCESS) {
            breakdown_query = nullptr;
            return result;
        }
        using pdh_add_english_counter_fn = ::PDH_STATUS(WINAPI*)(::PDH_HQUERY, ::LPCWSTR, ::DWORD_PTR, ::PDH_HCOUNTER*);
        pdh_add_english_counter_fn add_english = nullptr;
        const ::HMODULE pdh_module = ::GetModuleHandleW(L"pdh.dll");
        if (pdh_module != nullptr) {
            add_english =
                    reinterpret_cast<pdh_add_english_counter_fn>(::GetProcAddress(pdh_module, "PdhAddEnglishCounterW"));
        }
        const auto add = [&](const wchar_t* path, ::PDH_HCOUNTER* counter) -> bool {
            const ::PDH_STATUS status = add_english != nullptr ? add_english(breakdown_query, path, 0, counter)
                                                               : ::PdhAddCounterW(breakdown_query, path, 0, counter);
            return status == ERROR_SUCCESS;
        };
        if (!add(L"\\Processor(_Total)\\% User Time", &user_counter) ||
            !add(L"\\Processor(_Total)\\% Privileged Time", &privileged_counter) ||
            !add(L"\\Processor(_Total)\\% Interrupt Time", &interrupt_counter) ||
            !add(L"\\Processor(_Total)\\% DPC Time", &dpc_counter) ||
            !add(L"\\Processor(_Total)\\% Idle Time", &idle_counter)) {
            ::PdhCloseQuery(breakdown_query);
            breakdown_query = nullptr;
            return result;
        }
        ::PdhCollectQueryData(breakdown_query);
        uint64_t initial_user = 0;
        uint64_t initial_privileged = 0;
        uint64_t initial_interrupt = 0;
        uint64_t initial_dpc = 0;
        uint64_t initial_idle = 0;
        if (read_raw(user_counter, initial_user) && read_raw(privileged_counter, initial_privileged) &&
            read_raw(interrupt_counter, initial_interrupt) && read_raw(dpc_counter, initial_dpc) &&
            read_raw(idle_counter, initial_idle)) {
            remember_raw(initial_user, initial_privileged, initial_interrupt, initial_dpc, initial_idle);
        }
        return result;
    }
    if (::PdhCollectQueryData(breakdown_query) != ERROR_SUCCESS) {
        return result;
    }
    uint64_t user = 0;
    uint64_t privileged = 0;
    uint64_t interrupt = 0;
    uint64_t dpc = 0;
    uint64_t idle = 0;
    if (!read_raw(user_counter, user) || !read_raw(privileged_counter, privileged) ||
        !read_raw(interrupt_counter, interrupt) || !read_raw(dpc_counter, dpc) || !read_raw(idle_counter, idle)) {
        return result;
    }
    if (!has_previous_raw || user < previous_user || privileged < previous_privileged ||
        interrupt < previous_interrupt || dpc < previous_dpc || idle < previous_idle) {
        remember_raw(user, privileged, interrupt, dpc, idle);
        return result;
    }
    const uint64_t user_delta = user - previous_user;
    const uint64_t privileged_delta = privileged - previous_privileged;
    const uint64_t interrupt_delta = interrupt - previous_interrupt;
    const uint64_t dpc_delta = dpc - previous_dpc;
    const uint64_t idle_delta = idle - previous_idle;
    previous_user = user;
    previous_privileged = privileged;
    previous_interrupt = interrupt;
    previous_dpc = dpc;
    previous_idle = idle;
    const uint64_t total_delta = user_delta + privileged_delta + idle_delta;
    if (total_delta == 0) {
        return result;
    }
    const float64_t scale = 100.0 / static_cast<float64_t>(total_delta);
    result.user = static_cast<float64_t>(user_delta) * scale;
    result.idle = static_cast<float64_t>(idle_delta) * scale;
    result.irq = static_cast<float64_t>(interrupt_delta) * scale;
    result.softirq = static_cast<float64_t>(dpc_delta) * scale;
    result.system = privileged_delta > interrupt_delta + dpc_delta
                            ? static_cast<float64_t>(privileged_delta - interrupt_delta - dpc_delta) * scale
                            : 0.0;
#else
    const string content = read_file_all("/proc/stat");
    if (content.empty()) {
        return result;
    }
    string line;
    size_t position = 0;
    if (!getline(content.view(), position, line) || !line.starts_with("cpu ")) {
        return result;
    }
    const string_view data = line.view(4);
    string_view token;
    size_t offset = 0;
    uint64_t fields[10] = {};
    size_t count = 0;
    while (count < 10 && getline(data, offset, token, is_space<char>)) {
        if (!token.empty()) {
            fields[count++] = to_uint64(token);
        }
    }
    if (count < 4) {
        return result;
    }
    vector<uint64_t> current(fields, fields + count);
    if (previous_fields.size() != current.size()) {
        previous_fields = move(current);
        return result;
    }
    uint64_t delta[10] = {};
    uint64_t total = 0;
    for (size_t index = 0; index < count; ++index) {
        if (current[index] < previous_fields[index]) {
            previous_fields = move(current);
            return result;
        }
        delta[index] = current[index] - previous_fields[index];
    }
    for (size_t index = 0; index < neforce::min(count, static_cast<size_t>(8)); ++index) {
        total += delta[index];
    }
    previous_fields = move(current);
    if (total == 0) {
        return result;
    }
    if (count >= 10) {
        delta[0] = delta[0] > delta[8] ? delta[0] - delta[8] : 0;
        delta[1] = delta[1] > delta[9] ? delta[1] - delta[9] : 0;
    }
    const float64_t scale = 100.0 / static_cast<float64_t>(total);
    result.user = static_cast<float64_t>(delta[0]) * scale;
    result.nice = static_cast<float64_t>(delta[1]) * scale;
    result.system = static_cast<float64_t>(delta[2]) * scale;
    result.idle = static_cast<float64_t>(delta[3]) * scale;
    result.iowait = static_cast<float64_t>(delta[4]) * scale;
    result.irq = static_cast<float64_t>(delta[5]) * scale;
    result.softirq = static_cast<float64_t>(delta[6]) * scale;
    result.steal = static_cast<float64_t>(delta[7]) * scale;
    if (count >= 10) {
        result.guest = static_cast<float64_t>(delta[8]) * scale;
        result.guest_nice = static_cast<float64_t>(delta[9]) * scale;
    }
#endif
    return result;
}

uint32_t sysinfo::process_count() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    vector<::DWORD> processes(1024);
    for (;;) {
        ::DWORD needed = 0;
        const auto bytes = static_cast<::DWORD>(processes.size() * sizeof(::DWORD));
        if (::EnumProcesses(processes.data(), bytes, &needed) == FALSE) {
            return 0;
        }
        if (needed < bytes) {
            return needed / static_cast<::DWORD>(sizeof(::DWORD));
        }
        processes.resize(processes.size() * 2);
    }
#else
    uint32_t count = 0;
    ::DIR* directory = ::opendir("/proc");
    if (directory != nullptr) {
        const ::dirent* entry = nullptr;
        // NOLINTNEXTLINE(concurrency-mt-unsafe)
        while ((entry = ::readdir(directory)) != nullptr) {
            if (entry->d_type != DT_DIR) {
                continue;
            }
            bool numeric = entry->d_name[0] != '\0';
            for (int i = 0; entry->d_name[i] != '\0'; i++) {
                if (!is_digit(entry->d_name[i])) {
                    numeric = false;
                    break;
                }
            }
            if (numeric) {
                count++;
            }
        }
        ::closedir(directory);
    }
    return count;
#endif
}

sysinfo::disk_info sysinfo::disk_status(const string& path) {
    disk_info info;

#ifdef NEFORCE_PLATFORM_WINDOWS
    wstring target = character::to_wstring(path.view());
    if (target.empty()) {
        wchar_t current[MAX_PATH] = {};
        if (::GetCurrentDirectoryW(MAX_PATH, current) == 0) {
            return info;
        }
        target = current;
    }
    info.path = wcharacter::to_string(target.view());

    if (::GetFileAttributesW(target.data()) == INVALID_FILE_ATTRIBUTES) {
        return info;
    }

    wchar_t root[MAX_PATH] = {};
    if (::GetVolumePathNameW(target.data(), root, MAX_PATH) == FALSE) {
        return info;
    }

    ::ULARGE_INTEGER available_bytes{};
    ::ULARGE_INTEGER total_bytes{};
    ::ULARGE_INTEGER free_bytes{};
    if (::GetDiskFreeSpaceExW(root, &available_bytes, &total_bytes, &free_bytes) == TRUE) {
        info.total_bytes = total_bytes.QuadPart;
        info.free_bytes = free_bytes.QuadPart;
        info.available_bytes = available_bytes.QuadPart;
        info.used_bytes = info.total_bytes - info.free_bytes;
    }

    wchar_t filesystem[MAX_PATH] = {};
    if (::GetVolumeInformationW(root, nullptr, 0, nullptr, nullptr, nullptr, filesystem, MAX_PATH) == TRUE) {
        info.filesystem = wcharacter::to_string(filesystem);
    }
#else
    string target = path;
    if (target.empty()) {
        char current[PATH_MAX] = {};
        if (::getcwd(current, sizeof(current)) == nullptr) {
            return info;
        }
        target = current;
    }
    info.path = target;

    struct ::statvfs value{};
    if (::statvfs(target.data(), &value) != 0) {
        return info;
    }
    const uint64_t block_size = value.f_frsize != 0 ? value.f_frsize : value.f_bsize;
    info.total_bytes = static_cast<uint64_t>(value.f_blocks) * block_size;
    info.free_bytes = static_cast<uint64_t>(value.f_bfree) * block_size;
    info.available_bytes = static_cast<uint64_t>(value.f_bavail) * block_size;
    info.used_bytes = info.total_bytes - info.free_bytes;
    info.filesystem = filesystem_type_of(target);
#endif

    return info;
}

vector<sysinfo::network_interface> sysinfo::network_interfaces() {
    vector<network_interface> result;

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::ULONG buffer_size = 15000;
    vector<::BYTE> buffer(buffer_size);
    auto* addresses = reinterpret_cast<::PIP_ADAPTER_ADDRESSES>(buffer.data());
    constexpr ::ULONG flags =
            GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;

    ::DWORD status = ::GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, addresses, &buffer_size);
    if (status == ERROR_BUFFER_OVERFLOW) {
        buffer.resize(buffer_size);
        addresses = reinterpret_cast<::PIP_ADAPTER_ADDRESSES>(buffer.data());
        status = ::GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, addresses, &buffer_size);
    }
    if (status != NO_ERROR) {
        return result;
    }

    ::PMIB_IF_TABLE2 mib_table = nullptr;
    const bool have_mib_table = ::GetIfTable2(&mib_table) == NO_ERROR && mib_table != nullptr;

    for (const auto* adapter = addresses; adapter != nullptr; adapter = adapter->Next) {
        network_interface iface;
        if (adapter->FriendlyName != nullptr) {
            iface.name = wcharacter::to_string(adapter->FriendlyName);
        }
        iface.index = adapter->IfIndex != 0 ? adapter->IfIndex : adapter->Ipv6IfIndex;
        iface.is_up = adapter->OperStatus == IfOperStatusUp;
        iface.is_loopback = adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK;
        iface.mac = mac_address_of(adapter->PhysicalAddress, adapter->PhysicalAddressLength);
        if (have_mib_table) {
            for (::ULONG index = 0; index < mib_table->NumEntries; ++index) {
                const auto& row = mib_table->Table[index];
                if (row.InterfaceIndex != iface.index) {
                    continue;
                }
                iface.mtu = row.Mtu;
                iface.speed_bps =
                        row.ReceiveLinkSpeed > row.TransmitLinkSpeed ? row.ReceiveLinkSpeed : row.TransmitLinkSpeed;
                iface.rx_bytes = row.InOctets;
                iface.tx_bytes = row.OutOctets;
                iface.rx_packets = row.InUcastPkts + row.InNUcastPkts;
                iface.tx_packets = row.OutUcastPkts + row.OutNUcastPkts;
                iface.rx_errors = row.InErrors;
                iface.tx_errors = row.OutErrors;
                iface.rx_dropped = row.InDiscards;
                iface.tx_dropped = row.OutDiscards;
                iface.operstate = oper_state_name(row.OperStatus);
                break;
            }
        }

        const ::IP_ADAPTER_UNICAST_ADDRESS* ipv6 = nullptr;
        for (const auto* unicast = adapter->FirstUnicastAddress; unicast != nullptr; unicast = unicast->Next) {
            const ::SOCKADDR* address = unicast->Address.lpSockaddr;
            if (address == nullptr) {
                continue;
            }
            if (address->sa_family == AF_INET) {
                fill_ipv4_address(iface, address, unicast->OnLinkPrefixLength);
                break;
            }
            if (address->sa_family == AF_INET6 && ipv6 == nullptr) {
                ipv6 = unicast;
            }
        }
        if (iface.address.empty() && ipv6 != nullptr) {
            fill_ipv6_address(iface, ipv6->Address.lpSockaddr, ipv6->OnLinkPrefixLength);
        }
        result.push_back(move(iface));
    }
    if (mib_table != nullptr) {
        ::FreeMibTable(mib_table);
    }
#else
    ::ifaddrs* list = nullptr;
    if (::getifaddrs(&list) != 0) {
        return result;
    }

    const auto find_or_add = [&result](const char* name) -> network_interface& {
        for (auto& iface: result) {
            if (iface.name == name) {
                return iface;
            }
        }
        network_interface iface;
        iface.name = name;
        iface.index = ::if_nametoindex(name);
        result.push_back(move(iface));
        return result.back();
    };

    for (const ::ifaddrs* current = list; current != nullptr; current = current->ifa_next) {
        if (current->ifa_name == nullptr || current->ifa_addr == nullptr) {
            continue;
        }
        network_interface& iface = find_or_add(current->ifa_name);
        iface.is_up = (current->ifa_flags & IFF_UP) != 0;
        iface.is_loopback = (current->ifa_flags & IFF_LOOPBACK) != 0;

        if (current->ifa_addr->sa_family == AF_PACKET) {
            if (iface.mac.empty()) {
                iface.mac = mac_address_of(*reinterpret_cast<const ::sockaddr_ll*>(current->ifa_addr));
            }
            continue;
        }
        if (current->ifa_addr->sa_family != AF_INET || !iface.address.empty()) {
            continue;
        }
        const auto* value = reinterpret_cast<const ::sockaddr_in*>(current->ifa_addr);
        char text[INET_ADDRSTRLEN] = {};
        if (::inet_ntop(AF_INET, &value->sin_addr, text, sizeof(text)) != nullptr) {
            iface.address = text;
        }
        if (current->ifa_netmask != nullptr && current->ifa_netmask->sa_family == AF_INET) {
            const auto* mask = reinterpret_cast<const ::sockaddr_in*>(current->ifa_netmask);
            char mask_text[INET_ADDRSTRLEN] = {};
            if (::inet_ntop(AF_INET, &mask->sin_addr, mask_text, sizeof(mask_text)) != nullptr) {
                iface.netmask = mask_text;
            }
            iface.prefix_length = prefix_length_of(endian::network_to_host(mask->sin_addr.s_addr));
        }
    }

    for (int pass = 0; pass < 2; ++pass) {
        for (const ::ifaddrs* current = list; current != nullptr; current = current->ifa_next) {
            if (current->ifa_name == nullptr || current->ifa_addr == nullptr ||
                current->ifa_addr->sa_family != AF_INET6) {
                continue;
            }
            network_interface& iface = find_or_add(current->ifa_name);
            if (!iface.address.empty()) {
                continue;
            }
            const auto* value = reinterpret_cast<const ::sockaddr_in6*>(current->ifa_addr);
            const bool link_local = IN6_IS_ADDR_LINKLOCAL(&value->sin6_addr) != 0;
            if (pass == 0 && link_local) {
                continue;
            }
            char text[INET6_ADDRSTRLEN] = {};
            if (::inet_ntop(AF_INET6, &value->sin6_addr, text, sizeof(text)) != nullptr) {
                iface.address = text;
            }
            if (current->ifa_netmask != nullptr && current->ifa_netmask->sa_family == AF_INET6) {
                const auto* mask = reinterpret_cast<const ::sockaddr_in6*>(current->ifa_netmask);
                const auto* bytes = reinterpret_cast<const uint8_t*>(&mask->sin6_addr);
                uint8_t bits = 0;
                bool counting = true;
                for (size_t i = 0; i < 16 && counting; ++i) {
                    for (int bit = 7; bit >= 0; --bit) {
                        if ((bytes[i] & (1U << bit)) != 0) {
                            ++bits;
                        } else {
                            counting = false;
                            break;
                        }
                    }
                }
                iface.prefix_length = bits;
            }
        }
    }
    ::freeifaddrs(list);
    for (auto& iface: result) {
        fill_network_stats(iface);
    }
#endif

    return result;
}

uint64_t sysinfo::uptime_seconds() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return static_cast<uint64_t>(::GetTickCount64()) / 1000;
#else
    struct ::sysinfo info;
    if (::sysinfo(&info) != 0) {
        return 0;
    }
    return static_cast<uint64_t>(info.uptime);
#endif
}

vector<sysinfo::disk_io_info> sysinfo::disk_io_rates() {
    static mutex disk_io_mutex;
    static flat_unordered_map<string, tuple<uint64_t, uint64_t, uint64_t>> previous;
    static uint64_t previous_ms = 0;
    lock<mutex> guard(disk_io_mutex);
    vector<disk_io_info> result;
#ifdef NEFORCE_PLATFORM_WINDOWS
    static ::PDH_HQUERY disk_query = nullptr;
    static ::PDH_HCOUNTER read_counter = nullptr;
    static ::PDH_HCOUNTER write_counter = nullptr;
    static ::PDH_HCOUNTER busy_counter = nullptr;
    if (disk_query == nullptr) {
        if (::PdhOpenQueryW(nullptr, 0, &disk_query) != ERROR_SUCCESS) {
            disk_query = nullptr;
            return result;
        }
        using pdh_add_english_counter_fn = ::PDH_STATUS(WINAPI*)(::PDH_HQUERY, ::LPCWSTR, ::DWORD_PTR, ::PDH_HCOUNTER*);
        pdh_add_english_counter_fn add_english = nullptr;
        const ::HMODULE pdh_module = ::GetModuleHandleW(L"pdh.dll");
        if (pdh_module != nullptr) {
            add_english =
                    reinterpret_cast<pdh_add_english_counter_fn>(::GetProcAddress(pdh_module, "PdhAddEnglishCounterW"));
        }
        const auto add = [&](const wchar_t* path, ::PDH_HCOUNTER* counter) -> bool {
            const ::PDH_STATUS status = add_english != nullptr ? add_english(disk_query, path, 0, counter)
                                                               : ::PdhAddCounterW(disk_query, path, 0, counter);
            return status == ERROR_SUCCESS;
        };
        if (!add(L"\\PhysicalDisk(*)\\Disk Read Bytes/sec", &read_counter) ||
            !add(L"\\PhysicalDisk(*)\\Disk Write Bytes/sec", &write_counter) ||
            !add(L"\\PhysicalDisk(*)\\% Disk Time", &busy_counter)) {
            ::PdhCloseQuery(disk_query);
            disk_query = nullptr;
            return result;
        }
        ::PdhCollectQueryData(disk_query);
        return result;
    }
    if (::PdhCollectQueryData(disk_query) != ERROR_SUCCESS) {
        return result;
    }
    const auto collect_array = [](const ::PDH_HCOUNTER counter) {
        flat_unordered_map<string, float64_t> values;
        ::DWORD size = 0;
        ::DWORD count = 0;
        if (::PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE, &size, &count, nullptr) != PDH_MORE_DATA) {
            return values;
        }
        vector<::BYTE> buffer(size);
        auto* items = reinterpret_cast<::PDH_FMT_COUNTERVALUE_ITEM_W*>(buffer.data());
        if (::PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE, &size, &count, items) != ERROR_SUCCESS) {
            return values;
        }
        for (::DWORD index = 0; index < count; ++index) {
            if (items[index].szName == nullptr) {
                continue;
            }
            if (items[index].FmtValue.CStatus != PDH_CSTATUS_VALID_DATA &&
                items[index].FmtValue.CStatus != PDH_CSTATUS_NEW_DATA) {
                continue;
            }
            values[wcharacter::to_string(items[index].szName)] = items[index].FmtValue.doubleValue;
        }
        return values;
    };
    const auto reads = collect_array(read_counter);
    const auto writes = collect_array(write_counter);
    const auto busy = collect_array(busy_counter);
    map<string, disk_io_info> entries;
    for (auto iterator = reads.begin(); iterator != reads.end(); ++iterator) {
        if (iterator->first == "_Total") {
            continue;
        }
        disk_io_info value;
        value.name = iterator->first;
        value.read_bytes_per_second = iterator->second;
        value.aggregate = true;
        entries[iterator->first] = move(value);
    }
    for (auto iterator = writes.begin(); iterator != writes.end(); ++iterator) {
        if (iterator->first == "_Total") {
            continue;
        }
        disk_io_info& value = entries[iterator->first];
        if (value.name.empty()) {
            value.name = iterator->first;
            value.aggregate = true;
        }
        value.write_bytes_per_second = iterator->second;
    }
    for (auto iterator = busy.begin(); iterator != busy.end(); ++iterator) {
        if (iterator->first == "_Total") {
            continue;
        }
        disk_io_info& value = entries[iterator->first];
        if (value.name.empty()) {
            value.name = iterator->first;
            value.aggregate = true;
        }
        value.busy_percent = iterator->second < 0.0 ? 0.0 : (iterator->second > 100.0 ? 100.0 : iterator->second);
    }
    for (auto iterator = entries.begin(); iterator != entries.end(); ++iterator) {
        result.push_back(move(iterator->second));
    }
#else
    const string content = read_file_all("/proc/diskstats");
    if (content.empty()) {
        return result;
    }
    ::timespec now_time{};
    ::clock_gettime(CLOCK_MONOTONIC, &now_time);
    const uint64_t now =
            static_cast<uint64_t>(now_time.tv_sec) * 1000ULL + static_cast<uint64_t>(now_time.tv_nsec) / 1000000ULL;
    const uint64_t elapsed = previous_ms != 0 && now > previous_ms ? now - previous_ms : 0;

    map<string, disk_io_info> entries;
    flat_unordered_map<string, tuple<uint64_t, uint64_t, uint64_t>> next;
    size_t position = 0;
    string line;
    while (getline(content.view(), position, line)) {
        const string_view data = line.view();
        string_view token;
        size_t offset = 0;
        string name;
        uint64_t read_sectors = 0;
        uint64_t write_sectors = 0;
        uint64_t busy_ms = 0;
        size_t field = 0;
        bool valid = true;
        while (getline(data, offset, token, is_space<char>)) {
            if (token.empty()) {
                continue;
            }
            if (field == 2) {
                name = string(token);
            } else if (field == 5) {
                read_sectors = to_uint64(token);
            } else if (field == 9) {
                write_sectors = to_uint64(token);
            } else if (field == 12) {
                busy_ms = to_uint64(token);
            }
            ++field;
            if (field > 12) {
                break;
            }
        }
        if (!valid || name.empty() || name.starts_with("loop") || name.starts_with("ram")) {
            continue;
        }
        const uint64_t read_bytes = read_sectors * 512ULL;
        const uint64_t write_bytes = write_sectors * 512ULL;
        disk_io_info value;
        value.name = name;
        const string device = "/sys/class/block/" + name;
        value.aggregate =
                ::access((device + "/device").data(), F_OK) == 0 && ::access((device + "/partition").data(), F_OK) != 0;
        if (elapsed > 0) {
            const auto found = previous.find(name);
            if (found != previous.end()) {
                if (read_bytes >= get<0>(found->second)) {
                    value.read_bytes_per_second = static_cast<float64_t>(read_bytes - get<0>(found->second)) * 1000.0 /
                                                  static_cast<float64_t>(elapsed);
                }
                if (write_bytes >= get<1>(found->second)) {
                    value.write_bytes_per_second = static_cast<float64_t>(write_bytes - get<1>(found->second)) *
                                                   1000.0 / static_cast<float64_t>(elapsed);
                }
                if (busy_ms >= get<2>(found->second)) {
                    const float64_t percent = static_cast<float64_t>(busy_ms - get<2>(found->second)) * 100.0 /
                                              static_cast<float64_t>(elapsed);
                    value.busy_percent = percent < 0.0 ? 0.0 : (percent > 100.0 ? 100.0 : percent);
                }
            }
        }
        next[name] = {read_bytes, write_bytes, busy_ms};
        entries[name] = move(value);
    }
    previous = move(next);
    previous_ms = now;
    for (auto iterator = entries.begin(); iterator != entries.end(); ++iterator) {
        result.push_back(move(iterator->second));
    }
#endif
    return result;
}


sysinfo::battery_info sysinfo::battery_status() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    battery_info info;
    ::SYSTEM_POWER_STATUS status{};
    if (::GetSystemPowerStatus(&status) == FALSE) {
        info.status = "unknown";
        return info;
    }
    info.present = (status.BatteryFlag & 128) == 0;
    info.on_ac = status.ACLineStatus == 1;
    info.charging = (status.BatteryFlag & 8) != 0;
    info.charge_percent = status.BatteryLifePercent == 255 ? -1.0 : static_cast<float64_t>(status.BatteryLifePercent);
    info.remaining_seconds = status.BatteryLifeTime == 0xFFFFFFFFUL ? -1 : static_cast<int64_t>(status.BatteryLifeTime);

    ::SYSTEM_BATTERY_STATE battery_state{};
    if (::CallNtPowerInformation(::SystemBatteryState, nullptr, 0, &battery_state, sizeof(battery_state)) == 0) {
        info.present = info.present || battery_state.BatteryPresent != FALSE;
        info.on_ac = info.on_ac || battery_state.AcOnLine != FALSE;
        info.charging = info.charging || battery_state.Charging != FALSE;
        if (battery_state.MaxCapacity > 0 && battery_state.RemainingCapacity <= battery_state.MaxCapacity) {
            info.charge_percent = 100.0 * static_cast<float64_t>(battery_state.RemainingCapacity) /
                                  static_cast<float64_t>(battery_state.MaxCapacity);
            info.capacity_mwh = static_cast<float64_t>(battery_state.RemainingCapacity);
            info.full_capacity_mwh = static_cast<float64_t>(battery_state.MaxCapacity);
        }
        const auto rate = static_cast<::LONG>(battery_state.Rate);
        info.rate_mw = static_cast<float64_t>(rate < 0 ? -rate : rate);
        if (battery_state.EstimatedTime > 0 && info.remaining_seconds < 0) {
            info.remaining_seconds = static_cast<int64_t>(battery_state.EstimatedTime);
        }
    }
    if (!info.present) {
        info.status = "unknown";
    } else if (info.charging) {
        info.status = "charging";
    } else if (info.on_ac && info.charge_percent >= 99.0) {
        info.status = "full";
    } else if (info.on_ac) {
        info.status = "idle";
    } else {
        info.status = "discharging";
    }
    return info;
#else
    return read_battery_linux();
#endif
}

vector<sysinfo::mount_point> sysinfo::mount_points() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    vector<mount_point> out;
    wchar_t drives[512] = {};
    const ::DWORD length = ::GetLogicalDriveStringsW(static_cast<::DWORD>(sizeof(drives) / sizeof(wchar_t)), drives);
    if (length == 0) {
        return out;
    }
    for (const wchar_t* drive = drives; *drive != L'\0'; drive += ::wcslen(drive) + 1) {
        if (::GetDriveTypeW(drive) == DRIVE_NO_ROOT_DIR) {
            continue;
        }
        mount_point item;
        item.target = wcharacter::to_string(drive);
        item.source = item.target;

        ::ULARGE_INTEGER available_bytes{};
        ::ULARGE_INTEGER total_bytes{};
        ::ULARGE_INTEGER free_bytes{};
        if (::GetDiskFreeSpaceExW(drive, &available_bytes, &total_bytes, &free_bytes) == TRUE) {
            item.total_bytes = total_bytes.QuadPart;
            item.free_bytes = free_bytes.QuadPart;
            item.available_bytes = available_bytes.QuadPart;
        }

        wchar_t filesystem[MAX_PATH] = {};
        ::DWORD flags = 0;
        if (::GetVolumeInformationW(drive, nullptr, 0, nullptr, nullptr, &flags, filesystem, MAX_PATH) == TRUE) {
            item.filesystem = wcharacter::to_string(filesystem);
            item.options = (flags & FILE_READ_ONLY_VOLUME) != 0 ? "ro" : "rw";
        }
        out.push_back(move(item));
    }
    return out;
#else
    return read_mount_entries();
#endif
}


NEFORCE_END_NAMESPACE__
