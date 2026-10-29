#include <NeForce/core/container/vector.hpp>
#include <NeForce/core/file/file.hpp>
#include <NeForce/core/utility/pair.hpp>
#include <NeForce/core/string/format.hpp>
#include <NeForce/core/string/to_numerics.hpp>
#include <NeForce/core/string/utf.hpp>
#include <NeForce/core/system/sysinfo.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <WinUser.h>
#    include <winreg.h>
#    include <SetupAPI.h>
#endif
#if defined(NEFORCE_PLATFORM_POSIX) && defined(NEFORCE_ARCH_X86)
#    include <cpuid.h>
#endif
#ifdef NEFORCE_PLATFORM_FREEBSD
#    include <sys/sysctl.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {

    string read_text(const char* filepath) {
        string content;
        const file input(path{filepath}, false, file_access::read, file_shared::share_read_write);
        if (!input.is_opened()) {
            return content;
        }
        char buffer[4096];
        for (;;) {
            const auto count = input.read_binary(buffer, sizeof(buffer));
            if (count == 0) {
                break;
            }
            content.append(buffer, count);
        }
        return content;
    }

    uint64_t number_of(const string& text) {
        if (text.empty()) {
            return 0;
        }
        try {
            return to_uint64(text.view());
        } catch (const exception&) {
            return 0;
        }
    }

    uint32_t number32_of(const string& text) { return static_cast<uint32_t>(number_of(text)); }

    void add_unique(vector<string>& values, const string& value) {
        if (value.empty()) {
            return;
        }
        for (const auto& existing: values) {
            if (existing == value) {
                return;
            }
        }
        values.push_back(value);
    }

    string join(const vector<string>& values, const char* separator) {
        string text;
        for (const auto& value: values) {
            if (!text.empty()) {
                text.append(separator);
            }
            text.append(value);
        }
        return text;
    }

    string hex_value(const uint64_t value) {
        if (value == 0) {
            return "0x0";
        }
        constexpr char digits[] = "0123456789ABCDEF";
        char buffer[32] = {};
        size_t cursor = sizeof(buffer);
        uint64_t remaining = value;
        while (remaining != 0 && cursor > 0) {
            buffer[--cursor] = digits[remaining & 0xFU];
            remaining >>= 4U;
        }
        string text("0x");
        text.append(buffer + cursor, sizeof(buffer) - cursor);
        return text;
    }

    bool contains_ci(const string& text, const char* needle) {
        if (needle == nullptr || *needle == '\0') {
            return false;
        }
        const auto fold = [](const char value) {
            return value >= 'A' && value <= 'Z' ? static_cast<char>(value + ('a' - 'A')) : value;
        };
        const size_t length = string_length(needle);
        if (text.size() < length) {
            return false;
        }
        for (size_t i = 0; i + length <= text.size(); ++i) {
            size_t j = 0;
            while (j < length && fold(text[i + j]) == fold(needle[j])) {
                ++j;
            }
            if (j == length) {
                return true;
            }
        }
        return false;
    }

    string memory_type_name(const uint32_t type) {
        switch (type) {
            case 18:
                return "DDR";
            case 19:
                return "DDR2";
            case 20:
                return "DDR2 FB-DIMM";
            case 22:
                return "DDR3";
            case 24:
                return "DDR4";
            case 25:
                return "LPDDR";
            case 26:
                return "LPDDR2";
            case 27:
                return "LPDDR3";
            case 28:
                return "LPDDR4";
            case 30:
                return "HBM";
            case 31:
                return "HBM2";
            case 32:
                return "DDR5";
            case 33:
                return "LPDDR5";
            case 34:
                return "DDR5";
            case 35:
                return "LPDDR5";
            default:
                return {};
        }
    }

    void fill_cpu_cpuid(sysinfo::cpu_info& cpu) {
#if defined(NEFORCE_PLATFORM_WINDOWS) && defined(NEFORCE_ARCH_X86)
        int registers[4] = {};
        ::__cpuid(registers, 1);
#elif defined(NEFORCE_PLATFORM_POSIX) && defined(NEFORCE_ARCH_X86)
        unsigned int registers[4] = {};
        if (::__get_cpuid(1, &registers[0], &registers[1], &registers[2], &registers[3]) == 0) {
            return;
        }
#else
        int registers[4] = {};
        return;
#endif
        const auto eax = static_cast<uint32_t>(registers[0]);
        const uint32_t base_family = (eax >> 8U) & 0xFU;
        const uint32_t base_model = (eax >> 4U) & 0xFU;
        const uint32_t ext_family = (eax >> 20U) & 0xFFU;
        const uint32_t ext_model = (eax >> 16U) & 0xFU;
        const uint32_t family = base_family == 0xFU ? base_family + ext_family : base_family;
        const uint32_t model =
                (base_family == 0x6U || base_family == 0xFU) ? (ext_model << 4U) + base_model : base_model;
        cpu.x86.model_id = model;
        cpu.x86.family = format("{}", base_family);
        cpu.x86.model = format("{}", base_model);
        cpu.x86.stepping = format("{}", eax & 0xFU);
        cpu.x86.ext_family = format("{}", family);
        const string model_hex = hex_value(model);
        cpu.x86.ext_model = model_hex.size() > 2 ? model_hex.substr(2) : model_hex;
    }

    void fill_cpu_instructions(sysinfo::cpu_info& cpu) {
#if defined(NEFORCE_PLATFORM_WINDOWS) && defined(NEFORCE_ARCH_X86)
        int leaf1[4] = {};
        int leaf7[4] = {};
        ::__cpuid(leaf1, 1);
        ::__cpuidex(leaf7, 7, 0);
#elif (defined(NEFORCE_PLATFORM_LINUX) || defined(NEFORCE_PLATFORM_FREEBSD)) && defined(NEFORCE_ARCH_X86)
        unsigned int leaf1[4] = {};
        unsigned int leaf7[4] = {};
        if (::__get_cpuid(1, &leaf1[0], &leaf1[1], &leaf1[2], &leaf1[3]) == 0 ||
            ::__get_cpuid_count(7, 0, &leaf7[0], &leaf7[1], &leaf7[2], &leaf7[3]) == 0) {
            return;
        }
#else
        int leaf1[4] = {};
        int leaf7[4] = {};
        return;
#endif
        const auto bit = [](const auto registers[4], const int index, const int position) noexcept {
            return (static_cast<uint32_t>(registers[index]) & (1U << position)) != 0;
        };
        string text;
        const auto add = [&text](const char* name) {
            if (!text.empty()) {
                text.append(", ");
            }
            text.append(name);
        };
        if (bit(leaf1, 3, 23)) {
            add("MMX");
        }
        if (bit(leaf1, 3, 25)) {
            add("SSE");
        }
        if (bit(leaf1, 3, 26)) {
            add("SSE2");
        }
        if (bit(leaf1, 2, 0)) {
            add("SSE3");
        }
        if (bit(leaf1, 2, 9)) {
            add("SSSE3");
        }
        if (bit(leaf1, 2, 19)) {
            add("SSE4.1");
        }
        if (bit(leaf1, 2, 20)) {
            add("SSE4.2");
        }
        if (bit(leaf1, 2, 12)) {
            add("FMA3");
        }
        if (bit(leaf1, 2, 25)) {
            add("AES");
        }
        if (bit(leaf1, 2, 1)) {
            add("PCLMULQDQ");
        }
        if (bit(leaf1, 2, 28)) {
            add("AVX");
        }
        if (bit(leaf1, 2, 29)) {
            add("F16C");
        }
        if (bit(leaf1, 2, 30)) {
            add("RDRAND");
        }
        if (bit(leaf1, 2, 26)) {
            add("XSAVE");
        }
        if (bit(leaf7, 1, 3)) {
            add("BMI1");
        }
        if (bit(leaf7, 1, 5)) {
            add("AVX2");
        }
        if (bit(leaf7, 1, 8)) {
            add("BMI2");
        }
        if (bit(leaf7, 1, 16)) {
            add("AVX-512F");
        }
        if (bit(leaf7, 1, 17)) {
            add("AVX-512DQ");
        }
        if (bit(leaf7, 1, 28)) {
            add("AVX-512CD");
        }
        if (bit(leaf7, 1, 30)) {
            add("AVX-512BW");
        }
        if (bit(leaf7, 1, 31)) {
            add("AVX-512VL");
        }
        if (bit(leaf7, 2, 8)) {
            add("GFNI");
        }
        if (bit(leaf7, 2, 9)) {
            add("VAES");
        }
        if (bit(leaf7, 2, 10)) {
            add("VPCLMULQDQ");
        }
        cpu.features = text;
    }

    void apply_cpu_metadata(sysinfo::cpu_info& cpu);

#ifdef NEFORCE_PLATFORM_WINDOWS

    struct smbios_data {
        vector<uint8_t> bytes;
        size_t table{0};
    };

    bool load_smbios(smbios_data& data) {
        const ::DWORD size = ::GetSystemFirmwareTable('RSMB', 0, nullptr, 0);
        if (size < 16) {
            return false;
        }
        data.bytes.resize(size);
        if (::GetSystemFirmwareTable('RSMB', 0, data.bytes.data(), size) != size) {
            data.bytes.clear();
            return false;
        }
        data.table = 8;
        return data.table + 4 <= data.bytes.size();
    }

    uint8_t smbios_byte(const smbios_data& data, const size_t offset, const uint8_t length, const uint8_t field) {
        if (field + 1 > length) {
            return 0;
        }
        const size_t position = offset + field;
        return position < data.bytes.size() ? data.bytes[position] : 0;
    }

    uint16_t smbios_word(const smbios_data& data, const size_t offset, const uint8_t length, const uint8_t field) {
        if (field + 2 > length) {
            return 0;
        }
        return static_cast<uint16_t>(smbios_byte(data, offset, length, field)) |
               static_cast<uint16_t>(static_cast<uint16_t>(smbios_byte(data, offset, length, field + 1)) << 8U);
    }

    uint32_t smbios_dword(const smbios_data& data, const size_t offset, const uint8_t length, const uint8_t field) {
        if (field + 4 > length) {
            return 0;
        }
        return static_cast<uint32_t>(smbios_word(data, offset, length, field)) |
               (static_cast<uint32_t>(smbios_word(data, offset, length, field + 2)) << 16U);
    }

    string smbios_string(const smbios_data& data, const size_t offset, const uint8_t length, const uint8_t index) {
        if (index == 0) {
            return {};
        }
        size_t position = offset + length;
        for (uint8_t current = 1; current < index; ++current) {
            while (position < data.bytes.size() && data.bytes[position] != 0) {
                ++position;
            }
            if (position >= data.bytes.size()) {
                return {};
            }
            ++position;
        }
        if (position >= data.bytes.size()) {
            return {};
        }
        const size_t begin = position;
        while (position < data.bytes.size() && data.bytes[position] != 0) {
            ++position;
        }
        string text;
        text.append(reinterpret_cast<const char*>(data.bytes.data() + begin), position - begin);
        text.trim();
        return text;
    }

    string registry_string(const wchar_t* path, const wchar_t* name) {
        wchar_t buffer[512] = {};
        ::DWORD size = sizeof(buffer);
        if (::RegGetValueW(HKEY_LOCAL_MACHINE, path, name, RRF_RT_REG_SZ, nullptr, buffer, &size) != ERROR_SUCCESS) {
            return {};
        }
        return wcharacter::to_string(buffer);
    }

    string registry_microcode() {
        ::HKEY key = nullptr;
        if (::RegOpenKeyExW(HKEY_LOCAL_MACHINE, LR"(HARDWARE\DESCRIPTION\System\CentralProcessor\0)", 0, KEY_READ,
                            &key) != ERROR_SUCCESS) {
            return {};
        }
        ::DWORD type = 0;
        ::DWORD size = 0;
        uint8_t data[16] = {};
        ::RegQueryValueExW(key, L"Update Revision", nullptr, &type, nullptr, &size);
        if (size == 0 || size > sizeof(data)) {
            size = sizeof(data);
        }
        const LONG result = ::RegQueryValueExW(key, L"Update Revision", nullptr, &type, data, &size);
        ::RegCloseKey(key);
        if (result != ERROR_SUCCESS) {
            return {};
        }
        uint64_t value = 0;
        if (type == REG_DWORD && size >= 4) {
            value = *reinterpret_cast<const ::DWORD*>(data);
        } else if (type == REG_BINARY && size >= 4) {
            for (size_t i = 0; i < 4; ++i) {
                value |= static_cast<uint64_t>(data[i]) << (8 * i);
            }
        }
        return value == 0 ? string{} : hex_value(value);
    }

    string normalize_bios_date(const string& value) {
        vector<string> parts;
        string current;
        for (const char character: value) {
            if (character >= '0' && character <= '9') {
                current.push_back(character);
            } else if (!current.empty()) {
                parts.push_back(current);
                current.clear();
            }
        }
        if (!current.empty()) {
            parts.push_back(current);
        }
        if (parts.size() == 3) {
            if (parts[0].size() == 4) {
                return parts[0] + "-" + parts[1] + "-" + parts[2];
            }
            if (parts[2].size() == 4) {
                return parts[2] + "-" + parts[0] + "-" + parts[1];
            }
        }
        string digits;
        for (const char character: value) {
            if (character >= '0' && character <= '9') {
                digits.push_back(character);
                if (digits.size() == 8) {
                    break;
                }
            }
        }
        if (digits.size() != 8) {
            return value;
        }
        return digits.substr(0, 4) + "-" + digits.substr(4, 2) + "-" + digits.substr(6, 2);
    }

    void fill_windows_caches(sysinfo::cpu_info& cpu) {
        ::DWORD size = 0;
        ::GetLogicalProcessorInformationEx(::RelationCache, nullptr, &size);
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER || size == 0) {
            return;
        }
        auto* buffer = static_cast<::SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(::malloc(size));
        if (buffer == nullptr) {
            return;
        }
        if (::GetLogicalProcessorInformationEx(::RelationCache, buffer, &size) == TRUE) {
            auto* cursor = reinterpret_cast<unsigned char*>(buffer);
            const auto* end = cursor + size;
            while (cursor + sizeof(::SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX) <= end) {
                const auto* info = reinterpret_cast<::SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(cursor);
                if (info->Relationship == ::RelationCache) {
                    const auto& cache = info->Cache;
                    const uint32_t kib = cache.CacheSize / 1024U;
                    if (cache.Level == 1) {
                        cpu.l1_kib = kib > cpu.l1_kib ? kib : cpu.l1_kib;
                        ++cpu.l1_instances;
                        if (cache.Type == ::CacheData) {
                            cpu.l1_data_kib = kib > cpu.l1_data_kib ? kib : cpu.l1_data_kib;
                            ++cpu.l1_data_instances;
                        } else if (cache.Type == ::CacheInstruction) {
                            cpu.l1_instruction_kib = kib > cpu.l1_instruction_kib ? kib : cpu.l1_instruction_kib;
                            ++cpu.l1_instruction_instances;
                        }
                    } else if (cache.Level == 2) {
                        cpu.l2_kib = kib > cpu.l2_kib ? kib : cpu.l2_kib;
                        ++cpu.l2_instances;
                    } else if (cache.Level == 3) {
                        cpu.l3_kib = kib > cpu.l3_kib ? kib : cpu.l3_kib;
                        ++cpu.l3_instances;
                    }
                }
                if (info->Size == 0) {
                    break;
                }
                cursor += info->Size;
            }
        }
        ::free(buffer);
    }

    void fill_windows_core_types(sysinfo::cpu_info& cpu) {
        ::DWORD size = 0;
        ::GetLogicalProcessorInformationEx(::RelationProcessorCore, nullptr, &size);
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER || size == 0) {
            return;
        }
        auto* buffer = static_cast<::SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(::malloc(size));
        if (buffer == nullptr) {
            return;
        }
        vector<pair<uint8_t, uint32_t>> classes;
        if (::GetLogicalProcessorInformationEx(::RelationProcessorCore, buffer, &size) == TRUE) {
            auto* cursor = reinterpret_cast<unsigned char*>(buffer);
            auto* end = cursor + size;
            while (cursor + sizeof(::SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX) <= end) {
                auto* info = reinterpret_cast<::SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(cursor);
                if (info->Relationship == ::RelationProcessorCore) {
                    const uint8_t efficiency = info->Processor.EfficiencyClass;
                    bool found = false;
                    for (auto& entry: classes) {
                        if (entry.first == efficiency) {
                            ++entry.second;
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        classes.push_back({efficiency, 1});
                    }
                }
                if (info->Size == 0) {
                    break;
                }
                cursor += info->Size;
            }
        }
        ::free(buffer);
        if (!classes.empty()) {
            uint8_t highest = classes[0].first;
            for (const auto& entry: classes) {
                highest = entry.first > highest ? entry.first : highest;
            }
            for (const auto& entry: classes) {
                if (entry.first == highest) {
                    cpu.pcore_count += entry.second;
                } else {
                    cpu.ecore_count += entry.second;
                }
            }
            if (cpu.pcore_count != 0 && cpu.cores > cpu.pcore_count) {
                cpu.ecore_count = cpu.cores - cpu.pcore_count;
            }
        }
    }

    void fill_from_smbios(const smbios_data& data, sysinfo::hardware_info& hardware, sysinfo::cpu_info& cpu) {
        vector<string> memory_types;
        uint32_t modules = 0;
        uint32_t configured_mts = 0;
        uint32_t rated_mts = 0;

        size_t cursor = data.table;
        while (cursor + 4 <= data.bytes.size()) {
            const uint8_t type = data.bytes[cursor];
            const uint8_t length = data.bytes[cursor + 1];
            if (length < 4 || cursor + length > data.bytes.size()) {
                break;
            }
            if (type == 0) {
                hardware.bios.brand = smbios_string(data, cursor, length, smbios_byte(data, cursor, length, 0x04));
                hardware.bios.version = smbios_string(data, cursor, length, smbios_byte(data, cursor, length, 0x05));
                hardware.bios.date = normalize_bios_date(
                        smbios_string(data, cursor, length, smbios_byte(data, cursor, length, 0x08)));
            } else if (type == 2) {
                hardware.board.vendor = smbios_string(data, cursor, length, smbios_byte(data, cursor, length, 0x04));
                hardware.board.model = smbios_string(data, cursor, length, smbios_byte(data, cursor, length, 0x05));
            } else if (type == 4) {
                const string socket = smbios_string(data, cursor, length, smbios_byte(data, cursor, length, 0x04));
                if (!socket.empty()) {
                    cpu.x86.socket = socket;
                }
                const uint32_t max_speed = smbios_word(data, cursor, length, 0x14);
                const uint32_t current_speed = smbios_word(data, cursor, length, 0x16);
                const uint32_t external_clock = smbios_word(data, cursor, length, 0x12);
                if (external_clock != 0) {
                    cpu.x86.bus_speed_mhz = external_clock;
                }
                const uint32_t cores = smbios_byte(data, cursor, length, 0x23);
                const uint32_t threads = smbios_byte(data, cursor, length, 0x25);
                const uint8_t voltage = smbios_byte(data, cursor, length, 0x11);
                if (voltage != 0 && cpu.x86.core_vid.empty()) {
                    const double volts = (voltage & 0x80U) != 0 ? static_cast<double>(voltage & 0x7FU) / 10.0
                                                                : static_cast<double>(voltage);
                    cpu.x86.core_vid = format("{:.2f} V (SMBIOS)", volts);
                }
                if (cpu.max_mhz == 0 && max_speed != 0) {
                    cpu.max_mhz = max_speed;
                }
                if (cpu.current_mhz == 0 && current_speed != 0) {
                    cpu.current_mhz = current_speed;
                }
                if (cores != 0 && cores != 0xFF) {
                    cpu.cores = cores;
                }
                if (threads != 0 && threads != 0xFF) {
                    cpu.logical_processors = threads;
                }
            } else if (type == 17) {
                const uint16_t size_code = smbios_word(data, cursor, length, 0x0C);
                uint64_t bytes = 0;
                if (size_code == 0x7FFF && length >= 0x20) {
                    const uint32_t extended = smbios_dword(data, cursor, length, 0x1C) & 0x7FFFFFFFU;
                    bytes = static_cast<uint64_t>(extended) * 1024U * 1024U;
                } else if ((size_code & 0x8000U) != 0) {
                    bytes = static_cast<uint64_t>(size_code & 0x7FFFU) * 1024U;
                } else {
                    bytes = static_cast<uint64_t>(size_code) * 1024U * 1024U;
                }
                if (bytes != 0) {
                    ++modules;
                    const uint32_t memory_type = smbios_byte(data, cursor, length, 0x12);
                    add_unique(memory_types, memory_type_name(memory_type));
                    const uint32_t speed = smbios_word(data, cursor, length, 0x15);
                    const uint32_t configured =
                            length >= 0x22 ? smbios_word(data, cursor, length, 0x20) : static_cast<uint32_t>(0);
                    rated_mts = speed > rated_mts ? speed : rated_mts;
                    configured_mts = configured > configured_mts ? configured : configured_mts;
                }
            }
            if (type == 127) {
                break;
            }
            size_t next = cursor + length;
            while (next + 1 < data.bytes.size() && (data.bytes[next] != 0 || data.bytes[next + 1] != 0)) {
                ++next;
            }
            next += 2;
            if (next <= cursor) {
                break;
            }
            cursor = next;
        }

        hardware.memory.module_count = modules;
        hardware.memory.type = join(memory_types, " / ");
        hardware.memory.rated_mts = rated_mts;
        hardware.memory.configured_mts = configured_mts != 0 ? configured_mts : rated_mts;
        if (modules >= 2) {
            hardware.memory.channel = format("Dual channel · {} modules", modules);
        } else if (modules == 1) {
            hardware.memory.channel = "Single channel · 1 module";
        }
    }

    void fill_windows_chipset(sysinfo::hardware_info& hardware) {
        const ::HDEVINFO devices = ::SetupDiGetClassDevsW(nullptr, L"PCI", nullptr, DIGCF_PRESENT | DIGCF_ALLCLASSES);
        if (devices == INVALID_HANDLE_VALUE) {
            return;
        }
        ::SP_DEVINFO_DATA info{};
        info.cbSize = sizeof(info);
        for (::DWORD index = 0; ::SetupDiEnumDeviceInfo(devices, index, &info) == TRUE; ++index) {
            wchar_t description[512] = {};
            if (::SetupDiGetDeviceRegistryPropertyW(devices, &info, SPDRP_DEVICEDESC, nullptr,
                                                    reinterpret_cast<::PBYTE>(description), sizeof(description),
                                                    nullptr) != TRUE) {
                continue;
            }
            const string name = wcharacter::to_string(description);
            if (hardware.board.southbridge.empty() && (contains_ci(name, "LPC") || contains_ci(name, "ISA"))) {
                hardware.board.southbridge = name;
            }
            if (hardware.board.chipset.empty() &&
                (contains_ci(name, "Chipset") || contains_ci(name, "Host Bridge") || contains_ci(name, "PCH"))) {
                hardware.board.chipset = name;
            }
            if (!hardware.board.chipset.empty() && !hardware.board.southbridge.empty()) {
                break;
            }
        }
        ::SetupDiDestroyDeviceInfoList(devices);
        if (hardware.board.chipset.empty()) {
            hardware.board.chipset = hardware.board.southbridge;
        }
    }

    void probe_platform(sysinfo::hardware_info& hardware, sysinfo::cpu_info& cpu) {
        fill_cpu_cpuid(cpu);
        fill_cpu_instructions(cpu);
        fill_windows_caches(cpu);
        smbios_data smbios;
        if (load_smbios(smbios)) {
            fill_from_smbios(smbios, hardware, cpu);
        }
        fill_windows_core_types(cpu);
        apply_cpu_metadata(cpu);
        if (hardware.bios.brand.empty()) {
            hardware.bios.brand = registry_string(LR"(HARDWARE\DESCRIPTION\System\BIOS)", L"BIOSVendor");
        }
        if (hardware.bios.version.empty()) {
            hardware.bios.version = registry_string(LR"(HARDWARE\DESCRIPTION\System\BIOS)", L"BIOSVersion");
        }
        if (hardware.bios.date.empty()) {
            hardware.bios.date =
                    normalize_bios_date(registry_string(LR"(HARDWARE\DESCRIPTION\System\BIOS)", L"BIOSReleaseDate"));
        }
        hardware.bios.microcode = registry_microcode();
        if (hardware.board.vendor.empty()) {
            hardware.board.vendor = registry_string(LR"(HARDWARE\DESCRIPTION\System\BIOS)", L"BaseBoardManufacturer");
        }
        if (hardware.board.model.empty()) {
            hardware.board.model = registry_string(LR"(HARDWARE\DESCRIPTION\System\BIOS)", L"BaseBoardProduct");
        }
        fill_windows_chipset(hardware);
        if (cpu.l3_kib != 0) {
            hardware.memory.llc_ring =
                    cpu.l3_kib >= 1024 ? format("{} MiB L3", cpu.l3_kib / 1024) : format("{} KiB L3", cpu.l3_kib);
        }
    }

#endif // NEFORCE_PLATFORM_WINDOWS

    void apply_cpu_metadata(sysinfo::cpu_info& cpu) {
        if (cpu.x86.family != "6") {
            return;
        }
        const uint32_t model = cpu.x86.model_id;
        if (model == 0xB7 || model == 0xBF) {
            if (cpu.x86.revision.empty()) {
                cpu.x86.revision = "B0";
            }
            if (cpu.x86.technology.empty()) {
                cpu.x86.technology = "10 nm";
            }
            if (cpu.x86.code_name.empty()) {
                cpu.x86.code_name = "Raptor Lake";
            }
        } else if (model == 0x97 || model == 0x9A) {
            if (cpu.x86.technology.empty()) {
                cpu.x86.technology = "10 nm";
            }
            if (cpu.x86.code_name.empty()) {
                cpu.x86.code_name = "Alder Lake";
            }
        } else if (model == 0x8C || model == 0x8D) {
            if (cpu.x86.technology.empty()) {
                cpu.x86.technology = "10 nm";
            }
            if (cpu.x86.code_name.empty()) {
                cpu.x86.code_name = "Tiger Lake";
            }
        } else if (model == 0x7D || model == 0x7E) {
            if (cpu.x86.technology.empty()) {
                cpu.x86.technology = "10 nm";
            }
            if (cpu.x86.code_name.empty()) {
                cpu.x86.code_name = "Ice Lake";
            }
        } else if (model == 0x5C || model == 0x5E || model == 0x5F) {
            if (cpu.x86.technology.empty()) {
                cpu.x86.technology = "14 nm";
            }
            if (cpu.x86.code_name.empty()) {
                cpu.x86.code_name = "Skylake";
            }
        }
        if (cpu.x86.max_tdp_watts == 0 && cpu.brand.ends_with("HX")) {
            cpu.x86.max_tdp_watts = 55;
        } else if (cpu.x86.max_tdp_watts == 0 && cpu.brand.ends_with("H")) {
            cpu.x86.max_tdp_watts = 45;
        } else if (cpu.x86.max_tdp_watts == 0 && cpu.brand.ends_with("P")) {
            cpu.x86.max_tdp_watts = 28;
        } else if (cpu.x86.max_tdp_watts == 0 && cpu.brand.ends_with("U")) {
            cpu.x86.max_tdp_watts = 15;
        } else if (cpu.x86.max_tdp_watts == 0 && cpu.brand.ends_with("K")) {
            cpu.x86.max_tdp_watts = 125;
        }
    }

#ifdef NEFORCE_PLATFORM_LINUX

    string cpuinfo_field(const string& text, const char* key) {
        const size_t key_length = string_length(key);
        size_t position = 0;
        while (position < text.size()) {
            size_t line_end = text.find('\n', position);
            if (line_end == string::npos) {
                line_end = text.size();
            }
            const string line = text.substr(position, line_end - position);
            const size_t colon = line.find(':');
            if (colon != string::npos) {
                string name = line.substr(0, colon);
                name.trim();
                if (name.size() == key_length) {
                    bool matches = true;
                    for (size_t i = 0; i < key_length; ++i) {
                        if (name[i] != key[i]) {
                            matches = false;
                            break;
                        }
                    }
                    if (matches) {
                        string value = line.substr(colon + 1);
                        value.trim();
                        return value;
                    }
                }
            }
            position = line_end + 1;
        }
        return {};
    }

    string dmi_value(const char* name) {
        string path("/sys/class/dmi/id/");
        path.append(name);
        string value = read_text(path.data());
        value.trim();
        return value;
    }

    uint64_t cache_size_bytes(const string& text) {
        string value = text;
        value.trim();
        if (value.empty()) {
            return 0;
        }
        uint64_t multiplier = 1;
        const char suffix = value.back();
        if (suffix == 'K' || suffix == 'k') {
            multiplier = 1024UL;
            value.resize(value.size() - 1);
        } else if (suffix == 'M' || suffix == 'm') {
            multiplier = 1024UL * 1024UL;
            value.resize(value.size() - 1);
        } else if (suffix == 'G' || suffix == 'g') {
            multiplier = 1024UL * 1024UL * 1024UL;
            value.resize(value.size() - 1);
        }
        return number_of(value) * multiplier;
    }

    void fill_linux_caches(sysinfo::cpu_info& cpu) {
        for (uint32_t index = 0; index < 16; ++index) {
            const string base = format("/sys/devices/system/cpu/cpu0/cache/index{}/", index);
            string level_text = read_text((base + "level").data());
            level_text.trim();
            if (level_text.empty()) {
                break;
            }
            const auto level = number32_of(level_text);
            const auto bytes = cache_size_bytes(read_text((base + "size").data()));
            const auto kib = static_cast<uint32_t>(bytes / 1024);
            string cache_type = read_text((base + "type").data());
            cache_type.trim();
            if (level == 1) {
                cpu.l1_kib = kib > cpu.l1_kib ? kib : cpu.l1_kib;
                ++cpu.l1_instances;
                if (cache_type == "Data") {
                    cpu.l1_data_kib = kib > cpu.l1_data_kib ? kib : cpu.l1_data_kib;
                    ++cpu.l1_data_instances;
                } else if (cache_type == "Instruction") {
                    cpu.l1_instruction_kib = kib > cpu.l1_instruction_kib ? kib : cpu.l1_instruction_kib;
                    ++cpu.l1_instruction_instances;
                }
            } else if (level == 2) {
                cpu.l2_kib = kib > cpu.l2_kib ? kib : cpu.l2_kib;
                ++cpu.l2_instances;
            } else if (level == 3) {
                cpu.l3_kib = kib > cpu.l3_kib ? kib : cpu.l3_kib;
                ++cpu.l3_instances;
            }
        }
    }

    string linux_instructions(const string& flags) {
        const char* names[] = {"sse", "sse2", "ssse3",     "sse4_1", "sse4_2", "avx",    "avx2",  "avx512f",
                               "fma", "aes",  "pclmulqdq", "bmi1",   "bmi2",   "sha_ni", "rdrand"};
        const char* labels[] = {"SSE",  "SSE2", "SSSE3",     "SSE4.1", "SSE4.2", "AVX", "AVX2",  "AVX-512F",
                                "FMA3", "AES",  "PCLMULQDQ", "BMI1",   "BMI2",   "SHA", "RDRAND"};
        string text;
        for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
            const string needle(names[i]);
            size_t position = 0;
            bool found = false;
            while (position < flags.size()) {
                position = flags.find(needle, position);
                if (position == string::npos) {
                    break;
                }
                const bool left_boundary = position == 0 || flags[position - 1] == ' ';
                const size_t end = position + needle.size();
                const bool right_boundary = end >= flags.size() || flags[end] == ' ';
                if (left_boundary && right_boundary) {
                    found = true;
                    break;
                }
                ++position;
            }
            if (found) {
                if (!text.empty()) {
                    text.append(", ");
                }
                text.append(labels[i]);
            }
        }
        return text;
    }

    uint32_t parse_hex_u32(const string& text) {
        size_t position = 0;
        if (text.size() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
            position = 2;
        }
        uint32_t value = 0;
        for (; position < text.size(); ++position) {
            const char c = text[position];
            const int digit = c >= '0' && c <= '9'   ? c - '0'
                              : c >= 'a' && c <= 'f' ? c - 'a' + 10
                              : c >= 'A' && c <= 'F' ? c - 'A' + 10
                                                     : -1;
            if (digit < 0) {
                return 0;
            }
            value = value * 16U + static_cast<uint32_t>(digit);
        }
        return value;
    }

    string arm_implementer_name(const string& implementer) {
        switch (parse_hex_u32(implementer)) {
            case 0x41:
                return "ARM";
            case 0x42:
                return "Broadcom";
            case 0x43:
                return "Cavium";
            case 0x44:
                return "DEC";
            case 0x46:
                return "Fujitsu";
            case 0x48:
                return "HiSilicon";
            case 0x49:
                return "Infineon";
            case 0x4D:
                return "Motorola/Freescale";
            case 0x4E:
                return "NVIDIA";
            case 0x50:
                return "APM";
            case 0x51:
                return "Qualcomm";
            case 0x53:
                return "Samsung";
            case 0x56:
                return "Marvell";
            case 0x61:
                return "Apple";
            case 0x69:
            case 0xC0:
                return "Ampere";
            default:
                return {};
        }
    }

    string arm_part_name(const string& part) {
        switch (parse_hex_u32(part)) {
            case 0xD03:
                return "Cortex-A53";
            case 0xD04:
                return "Cortex-A35";
            case 0xD05:
                return "Cortex-A55";
            case 0xD07:
                return "Cortex-A57";
            case 0xD08:
                return "Cortex-A72";
            case 0xD09:
                return "Cortex-A73";
            case 0xD0A:
                return "Cortex-A75";
            case 0xD0B:
                return "Cortex-A76";
            case 0xD0C:
                return "Neoverse-N1";
            case 0xD0D:
                return "Cortex-A77";
            case 0xD40:
                return "Neoverse-V1";
            case 0xD41:
                return "Cortex-A78";
            case 0xD44:
                return "Cortex-X1";
            case 0xD46:
                return "Cortex-A510";
            case 0xD47:
                return "Cortex-A710";
            case 0xD48:
                return "Cortex-X2";
            case 0xD49:
                return "Neoverse-N2";
            case 0xD4D:
                return "Cortex-A715";
            case 0xD4E:
                return "Cortex-X3";
            case 0xD4F:
                return "Neoverse-V2";
            case 0xD80:
                return "Cortex-A520";
            case 0xD81:
                return "Cortex-A720";
            case 0xD82:
                return "Cortex-X4";
            default:
                return {};
        }
    }

    string arm_instructions(const string& flags) {
        static constexpr string_view names[] = {"fp",      "asimd",   "aes",   "pmull", "sha1",  "sha2",  "sha512",
                                                "crc32",   "atomics", "sve",   "sve2",  "sme",   "bf16",  "i8mm",
                                                "dotprod", "fp16",    "jscvt", "fcma",  "lrcpc", "dcpop", "sha3",
                                                "sm3",     "sm4",     "rng",   "bti",   "mte"};
        static constexpr string_view labels[] = {"FP",      "NEON",    "AES",   "PMULL", "SHA1",  "SHA2",  "SHA512",
                                                 "CRC32",   "ATOMICS", "SVE",   "SVE2",  "SME",   "BF16",  "I8MM",
                                                 "DOTPROD", "FP16",    "JSCVT", "FCMA",  "LRCPC", "DCPOP", "SHA3",
                                                 "SM3",     "SM4",     "RNG",   "BTI",   "MTE"};
        string text_value;
        for (size_t i = 0; i < extent_v<decltype(names)>; ++i) {
            const string needle(names[i]);
            size_t position = 0;
            bool found = false;
            while (position < flags.size()) {
                position = flags.find(needle, position);
                if (position == string::npos) {
                    break;
                }
                const bool left_boundary = position == 0 || flags[position - 1] == ' ';
                const size_t end = position + needle.size();
                const bool right_boundary = end >= flags.size() || flags[end] == ' ';
                if (left_boundary && right_boundary) {
                    found = true;
                    break;
                }
                ++position;
            }
            if (found) {
                if (!text_value.empty()) {
                    text_value.append(", ");
                }
                text_value.append(labels[i]);
            }
        }
        return text_value;
    }

    void add_unique_core(vector<uint32_t>& values, const uint32_t value) {
        for (const uint32_t existing: values) {
            if (existing == value) {
                return;
            }
        }
        values.push_back(value);
    }

    uint32_t cpu_list_unique_cores(const string& text) {
        vector<uint32_t> cores;
        size_t position = 0;
        while (position < text.size()) {
            size_t end = text.find(',', position);
            if (end == string::npos) {
                end = text.size();
            }
            string token = text.substr(position, end - position);
            token.trim();
            if (!token.empty()) {
                const size_t dash = token.find('-');
                uint64_t first = 0;
                uint64_t last = 0;
                if (dash == string::npos) {
                    first = last = number_of(token);
                } else {
                    first = number_of(token.substr(0, dash));
                    last = number_of(token.substr(dash + 1));
                }
                for (uint64_t cpu = first; cpu <= last && cpu <= 4096; ++cpu) {
                    string path("/sys/devices/system/cpu/cpu");
                    path.append(format("{}", cpu));
                    path.append("/topology/core_id");
                    string core = read_text(path.data());
                    core.trim();
                    if (!core.empty()) {
                        add_unique_core(cores, number32_of(core));
                    }
                }
            }
            position = end + 1;
        }
        return static_cast<uint32_t>(cores.size());
    }

    string ascii_upper(const string& text) {
        string out = text;
        for (auto& c: out) {
            if (c >= 'a' && c <= 'z') {
                c = static_cast<char>(c - 'a' + 'A');
            }
        }
        return out;
    }

    string riscv_instructions(const string& isa) {
        string text_value;
        size_t position = 0;
        while (position < isa.size()) {
            size_t end = isa.find('_', position);
            if (end == string::npos) {
                end = isa.size();
            }
            string token = isa.substr(position, end - position);
            token.trim();
            if (!token.empty()) {
                if (!text_value.empty()) {
                    text_value.append(", ");
                }
                text_value.append(ascii_upper(token));
            }
            position = end + 1;
        }
        return text_value;
    }

    string loongarch_instructions(const string& features) {
        string text_value;
        size_t position = 0;
        while (position < features.size()) {
            while (position < features.size() && (features[position] == ' ' || features[position] == '\t')) {
                ++position;
            }
            const size_t begin = position;
            while (position < features.size() && features[position] != ' ' && features[position] != '\t') {
                ++position;
            }
            if (position > begin) {
                if (!text_value.empty()) {
                    text_value.append(", ");
                }
                text_value.append(ascii_upper(features.substr(begin, position - begin)));
            }
        }
        return text_value;
    }

    void fill_core_types_from_capacity(sysinfo::cpu_info& cpu) {
        const uint32_t count = cpu.logical_processors != 0 ? cpu.logical_processors : 64;
        vector<uint32_t> capacities;
        capacities.reserve(count);
        for (uint32_t index = 0; index < count; ++index) {
            string path("/sys/devices/system/cpu/cpu");
            path.append(format("{}", index));
            path.append("/cpu_capacity");
            string value = read_text(path.data());
            value.trim();
            const uint64_t capacity = number_of(value);
            if (capacity != 0) {
                capacities.push_back(static_cast<uint32_t>(capacity));
            }
        }
        if (capacities.size() < 2) {
            return;
        }
        uint32_t maximum = capacities[0];
        uint32_t minimum = capacities[0];
        for (const uint32_t value: capacities) {
            maximum = value > maximum ? value : maximum;
            minimum = value < minimum ? value : minimum;
        }
        if (maximum == minimum) {
            return;
        }
        uint32_t performance = 0;
        uint32_t efficiency = 0;
        for (const uint32_t value: capacities) {
            if (value == maximum) {
                ++performance;
            } else if (value == minimum) {
                ++efficiency;
            }
        }
        cpu.pcore_count = performance;
        cpu.ecore_count = efficiency;
    }

    void fill_linux_smbios_memory(sysinfo::hardware_info& hardware) {
        const string table = read_text("/sys/firmware/dmi/tables/DMI");
        if (table.size() < 8) {
            return;
        }
        const auto* bytes = reinterpret_cast<const uint8_t*>(table.data());
        const size_t size = table.size();
        const auto byte_at = [bytes, size](const size_t offset) -> uint32_t {
            return offset < size ? bytes[offset] : 0;
        };
        const auto word_at = [&byte_at](const size_t offset) -> uint32_t {
            return byte_at(offset) | (byte_at(offset + 1) << 8U);
        };
        vector<string> types;
        uint32_t modules = 0;
        uint32_t rated_mts = 0;
        uint32_t configured_mts = 0;
        size_t cursor = 0;
        while (cursor + 4 <= size) {
            const uint32_t type = byte_at(cursor);
            const uint32_t length = byte_at(cursor + 1);
            if (length < 4 || cursor + length > size) {
                break;
            }
            if (type == 17 && length >= 0x17) {
                const uint32_t size_code = word_at(cursor + 0x0C);
                uint64_t capacity = 0;
                if (size_code == 0xFFFF) {
                    capacity = 0;
                } else if (size_code == 0x7FFF && length >= 0x20) {
                    capacity = static_cast<uint64_t>(word_at(cursor + 0x1C) | (word_at(cursor + 0x1E) << 16U)) *
                               1024ULL * 1024ULL;
                } else if ((size_code & 0x8000U) != 0) {
                    capacity = static_cast<uint64_t>(size_code & 0x7FFFU) * 1024ULL;
                } else {
                    capacity = static_cast<uint64_t>(size_code) * 1024ULL * 1024ULL;
                }
                if (capacity != 0) {
                    ++modules;
                    if (length > 0x12) {
                        add_unique(types, memory_type_name(byte_at(cursor + 0x12)));
                    }
                    const uint32_t speed = word_at(cursor + 0x15);
                    const uint32_t configured = length >= 0x22 ? word_at(cursor + 0x20) : 0;
                    rated_mts = speed > rated_mts ? speed : rated_mts;
                    configured_mts = configured > configured_mts ? configured : configured_mts;
                }
            }
            if (type == 127) {
                break;
            }
            size_t next = cursor + length;
            while (next + 1 < size && (bytes[next] != 0 || bytes[next + 1] != 0)) {
                ++next;
            }
            next += 2;
            if (next <= cursor) {
                break;
            }
            cursor = next;
        }
        if (modules == 0) {
            return;
        }
        hardware.memory.module_count = modules;
        hardware.memory.type = join(types, " / ");
        hardware.memory.rated_mts = rated_mts;
        hardware.memory.configured_mts = configured_mts != 0 ? configured_mts : rated_mts;
        if (modules >= 2) {
            hardware.memory.channel = format("{} modules", modules);
        } else {
            hardware.memory.channel = "Single channel";
        }
    }

    void probe_platform(sysinfo::hardware_info& hardware, sysinfo::cpu_info& cpu) {
        fill_cpu_cpuid(cpu);
        const string info = read_text("/proc/cpuinfo");
#    if defined(NEFORCE_ARCH_X86)
        cpu.x86.family = cpuinfo_field(info, "cpu family");
        cpu.x86.model = cpuinfo_field(info, "model");
        cpu.x86.stepping = cpuinfo_field(info, "stepping");
        cpu.x86.revision = cpu.x86.stepping;
        cpu.features = linux_instructions(cpuinfo_field(info, "flags"));
#    elif defined(NEFORCE_ARCH_ARM)
        cpu.arm.architecture = cpuinfo_field(info, "CPU architecture");
        cpu.arm.implementer = cpuinfo_field(info, "CPU implementer");
        cpu.arm.part = cpuinfo_field(info, "CPU part");
        cpu.arm.variant = cpuinfo_field(info, "CPU variant");
        cpu.arm.revision = cpuinfo_field(info, "CPU revision");
        cpu.arm.core_name = arm_part_name(cpu.arm.part);
        if (cpu.vendor.empty()) {
            cpu.vendor = arm_implementer_name(cpu.arm.implementer);
        }
        cpu.features = arm_instructions(cpuinfo_field(info, "Features"));
#    elif defined(NEFORCE_ARCH_RISCV)
        cpu.riscv.isa = cpuinfo_field(info, "isa");
        cpu.riscv.mmu = cpuinfo_field(info, "mmu");
        cpu.riscv.uarch = cpuinfo_field(info, "uarch");
        cpu.riscv.mvendorid = cpuinfo_field(info, "mvendorid");
        cpu.riscv.marchid = cpuinfo_field(info, "marchid");
        cpu.riscv.mimpid = cpuinfo_field(info, "mimpid");
        if (cpu.brand.empty()) {
            cpu.brand = cpu.riscv.uarch;
        }
        cpu.features = riscv_instructions(cpu.riscv.isa);
#    elif defined(NEFORCE_ARCH_LOONGARCH)
        cpu.loongarch.family = cpuinfo_field(info, "CPU Family");
        cpu.loongarch.revision = cpuinfo_field(info, "CPU Revision");
        cpu.loongarch.fpu_revision = cpuinfo_field(info, "FPU Revision");
        cpu.loongarch.isa = cpuinfo_field(info, "ISA");
        cpu.loongarch.address_sizes = cpuinfo_field(info, "Address Sizes");
        cpu.loongarch.tlb_entries = static_cast<uint32_t>(number_of(cpuinfo_field(info, "TLB Entries")));
        cpu.loongarch.machine = cpuinfo_field(info, "machine");
        cpu.loongarch.system_type = cpuinfo_field(info, "system type");
        cpu.loongarch.watchpoint = cpuinfo_field(info, "Hardware Watchpoint");
        if (contains_ci(cpu.loongarch.family, "loongson")) {
            cpu.vendor = "Loongson";
        }
        if (cpu.brand.empty()) {
            cpu.brand = cpu.loongarch.family;
        }
        cpu.features = loongarch_instructions(cpuinfo_field(info, "Features"));
#    else
        cpu.features = linux_instructions(cpuinfo_field(info, "flags"));
#    endif
        fill_linux_caches(cpu);
        apply_cpu_metadata(cpu);
        const string pcore_list = read_text("/sys/devices/cpu_core/cpus");
        const string ecore_list = read_text("/sys/devices/cpu_atom/cpus");
        if (!pcore_list.empty() || !ecore_list.empty()) {
            cpu.pcore_count = cpu_list_unique_cores(pcore_list);
            cpu.ecore_count = cpu_list_unique_cores(ecore_list);
        } else {
            fill_core_types_from_capacity(cpu);
        }
        if (cpu.brand.empty()) {
            cpu.brand = cpuinfo_field(info, "model name");
        }

        const string package = read_text("/sys/devices/system/cpu/cpu0/topology/physical_package_id");
        if (!package.empty()) {
            cpu.x86.socket = "Socket " + string(package);
            cpu.x86.socket.trim();
        }

        string rapl = read_text("/sys/class/powercap/intel-rapl:0/constraint_0_max_power_uw");
        rapl.trim();
        const uint64_t tdp_uw = number_of(rapl);
        if (tdp_uw != 0) {
            cpu.x86.max_tdp_watts = static_cast<uint32_t>(tdp_uw / 1'000'000U);
        }

        hardware.bios.brand = dmi_value("bios_vendor");
        hardware.bios.version = dmi_value("bios_version");
        hardware.bios.date = dmi_value("bios_date");
        hardware.bios.microcode = cpuinfo_field(info, "microcode");

        hardware.board.vendor = dmi_value("board_vendor");
        hardware.board.model = dmi_value("board_name");
        fill_linux_smbios_memory(hardware);

        if (cpu.l3_kib != 0) {
            hardware.memory.llc_ring =
                    cpu.l3_kib >= 1024 ? format("{} MiB L3", cpu.l3_kib / 1024) : format("{} KiB L3", cpu.l3_kib);
        }
    }

#endif // NEFORCE_PLATFORM_LINUX

#ifdef NEFORCE_PLATFORM_FREEBSD
    int sysctl_value_of(const char* name) noexcept {
        int value = 0;
        ::size_t length = sizeof(value);
        if (::sysctlbyname(name, &value, &length, nullptr, 0) != 0) {
            return 0;
        }
        return value;
    }

    string sysctl_string_of(const char* name) {
        ::size_t length = 0;
        if (::sysctlbyname(name, nullptr, &length, nullptr, 0) != 0 || length == 0) {
            return {};
        }
        string value;
        value.resize(static_cast<size_t>(length));
        if (::sysctlbyname(name, value.data(), &length, nullptr, 0) != 0) {
            return {};
        }
        while (!value.empty() && (value.back() == '\0' || value.back() == '\n')) {
            value.pop_back();
        }
        return value;
    }

#    if defined(NEFORCE_ARCH_X86)
    void fill_cpuid_caches(sysinfo::cpu_info& cpu) {
        for (unsigned int index = 0; index < 16; ++index) {
            unsigned int eax = 0;
            unsigned int ebx = 0;
            unsigned int ecx = 0;
            unsigned int edx = 0;
            if (::__get_cpuid_count(4, index, &eax, &ebx, &ecx, &edx) == 0) {
                break;
            }
            const unsigned int type = eax & 0x1FU;
            if (type == 0) {
                break;
            }
            const unsigned int level = (eax >> 5U) & 0x7U;
            const unsigned int line = (ebx & 0xFFFU) + 1U;
            const unsigned int partitions = ((ebx >> 12U) & 0x3FFU) + 1U;
            const unsigned int ways = ((ebx >> 22U) & 0x3FFU) + 1U;
            const uint64_t bytes = static_cast<uint64_t>(line) * partitions * ways * (static_cast<uint64_t>(ecx) + 1U);
            const auto kib = static_cast<uint32_t>(bytes / 1024U);
            if (level == 1) {
                cpu.l1_kib = kib > cpu.l1_kib ? kib : cpu.l1_kib;
                ++cpu.l1_instances;
                if (type == 1) {
                    cpu.l1_data_kib = kib > cpu.l1_data_kib ? kib : cpu.l1_data_kib;
                    ++cpu.l1_data_instances;
                } else if (type == 2) {
                    cpu.l1_instruction_kib = kib > cpu.l1_instruction_kib ? kib : cpu.l1_instruction_kib;
                    ++cpu.l1_instruction_instances;
                }
            } else if (level == 2) {
                cpu.l2_kib = kib > cpu.l2_kib ? kib : cpu.l2_kib;
                ++cpu.l2_instances;
            } else if (level == 3) {
                cpu.l3_kib = kib > cpu.l3_kib ? kib : cpu.l3_kib;
                ++cpu.l3_instances;
            }
        }
    }
#    endif

    void probe_platform(sysinfo::hardware_info& hardware, sysinfo::cpu_info& cpu) {
#    if defined(NEFORCE_ARCH_X86)
        fill_cpu_cpuid(cpu);
        fill_cpu_instructions(cpu);
        fill_cpuid_caches(cpu);
        apply_cpu_metadata(cpu);
#    endif

        cpu.brand = sysctl_string_of("hw.model");
        if (cpu.brand.empty()) {
            cpu.brand = sysctl_string_of("hw.machine");
        }
        if (cpu.features.empty()) {
            cpu.features = sysctl_string_of("hw.isa");
        }

        cpu.logical_processors = static_cast<uint32_t>(sysctl_value_of("hw.ncpu"));
        const int cores = sysctl_value_of("kern.smp.cores");
        cpu.cores = cores > 0 ? static_cast<uint32_t>(cores) : cpu.logical_processors;

        const int clockrate = sysctl_value_of("hw.clockrate");
        if (clockrate > 0) {
            cpu.max_mhz = static_cast<uint32_t>(clockrate);
        }
        const int frequency = sysctl_value_of("dev.cpu.0.freq");
        if (frequency > 0) {
            cpu.current_mhz = static_cast<uint32_t>(frequency);
        }

        hardware.bios.brand = sysctl_string_of("kenv.smbios.bios.vendor");
        hardware.bios.version = sysctl_string_of("kenv.smbios.bios.version");
        hardware.bios.date = sysctl_string_of("kenv.smbios.bios.reldate");

        hardware.board.vendor = sysctl_string_of("kenv.smbios.planar.maker");
        hardware.board.model = sysctl_string_of("kenv.smbios.planar.product");
        if (hardware.board.vendor.empty()) {
            hardware.board.vendor = sysctl_string_of("kenv.smbios.system.maker");
        }
        if (hardware.board.model.empty()) {
            hardware.board.model = sysctl_string_of("kenv.smbios.system.product");
        }

        if (cpu.l3_kib != 0) {
            hardware.memory.llc_ring =
                    cpu.l3_kib >= 1024 ? format("{} MiB L3", cpu.l3_kib / 1024) : format("{} KiB L3", cpu.l3_kib);
        }
    }
#endif

} // namespace

void sysinfo::probe_hardware() { probe_platform(hardware_info_, cpu_info_); }

NEFORCE_END_NAMESPACE__
