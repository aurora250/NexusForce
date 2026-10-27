#include <NeForce/core/container/vector.hpp>
#include <NeForce/core/utility/pair.hpp>
#include <NeForce/core/string/format.hpp>
#include <NeForce/core/string/to_numerics.hpp>
#include <NeForce/core/string/utf.hpp>
#include <NeForce/core/system/sysinfo.hpp>
#include <cstdio>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <Windows.h>
#    include <SetupAPI.h>
#endif
#if defined(NEFORCE_PLATFORM_LINUX) && defined(NEFORCE_ARCH_X86)
#    include <cpuid.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {

    string read_text(const char* filepath) {
        // TODO: using neforce::file instead
        string content;
        ::FILE* file = ::fopen(filepath, "r");
        if (file == nullptr) {
            return content;
        }
        char buffer[4096];
        for (;;) {
            const size_t count = ::fread(buffer, 1, sizeof(buffer), file);
            if (count > 0) {
                content.append(buffer, count);
            }
            if (count < sizeof(buffer)) {
                break;
            }
        }
        ::fclose(file);
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
#elif defined(NEFORCE_PLATFORM_LINUX) && defined(NEFORCE_ARCH_X86)
        unsigned int registers[4] = {};
        if (::__get_cpuid(1, &registers[0], &registers[1], &registers[2], &registers[3]) == 0) {
            return;
        }
#else
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
        cpu.model_id = model;
        cpu.family = format("{}", base_family);
        cpu.model = format("{}", base_model);
        cpu.stepping = format("{}", eax & 0xFU);
        cpu.ext_family = format("{}", family);
        const string model_hex = hex_value(model);
        cpu.ext_model = model_hex.size() > 2 ? model_hex.substr(2) : model_hex;
    }

    void fill_cpu_instructions(sysinfo::cpu_info& cpu) {
#if defined(NEFORCE_PLATFORM_WINDOWS) && defined(NEFORCE_ARCH_X86)
        int leaf1[4] = {};
        int leaf7[4] = {};
        ::__cpuid(leaf1, 1);
        ::__cpuidex(leaf7, 7, 0);
#elif defined(NEFORCE_PLATFORM_LINUX) && defined(NEFORCE_ARCH_X86)
        unsigned int leaf1[4] = {};
        unsigned int leaf7[4] = {};
        if (::__get_cpuid(1, &leaf1[0], &leaf1[1], &leaf1[2], &leaf1[3]) == 0 ||
            ::__get_cpuid_count(7, 0, &leaf7[0], &leaf7[1], &leaf7[2], &leaf7[3]) == 0) {
            return;
        }
#else
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
                    cpu.package = socket;
                }
                const uint32_t max_speed = smbios_word(data, cursor, length, 0x14);
                const uint32_t current_speed = smbios_word(data, cursor, length, 0x16);
                const uint32_t external_clock = smbios_word(data, cursor, length, 0x12);
                if (external_clock != 0) {
                    cpu.bus_speed_mhz = external_clock;
                }
                const uint32_t cores = smbios_byte(data, cursor, length, 0x23);
                const uint32_t threads = smbios_byte(data, cursor, length, 0x25);
                const uint8_t voltage = smbios_byte(data, cursor, length, 0x11);
                if (voltage != 0 && cpu.core_vid.empty()) {
                    const double volts = (voltage & 0x80U) != 0 ? static_cast<double>(voltage & 0x7FU) / 10.0
                                                                : static_cast<double>(voltage);
                    cpu.core_vid = format("{:.2f} V (SMBIOS)", volts);
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
        if (cpu.family != "6") {
            return;
        }
        const uint32_t model = cpu.model_id;
        if (model == 0xB7 || model == 0xBF) {
            if (cpu.revision.empty()) {
                cpu.revision = "B0";
            }
            if (cpu.technology.empty()) {
                cpu.technology = "10 nm";
            }
            if (cpu.code_name.empty()) {
                cpu.code_name = "Raptor Lake";
            }
        } else if (model == 0x97 || model == 0x9A) {
            if (cpu.technology.empty()) {
                cpu.technology = "10 nm";
            }
            if (cpu.code_name.empty()) {
                cpu.code_name = "Alder Lake";
            }
        } else if (model == 0x8C || model == 0x8D) {
            if (cpu.technology.empty()) {
                cpu.technology = "10 nm";
            }
            if (cpu.code_name.empty()) {
                cpu.code_name = "Tiger Lake";
            }
        } else if (model == 0x7D || model == 0x7E) {
            if (cpu.technology.empty()) {
                cpu.technology = "10 nm";
            }
            if (cpu.code_name.empty()) {
                cpu.code_name = "Ice Lake";
            }
        } else if (model == 0x5C || model == 0x5E || model == 0x5F) {
            if (cpu.technology.empty()) {
                cpu.technology = "14 nm";
            }
            if (cpu.code_name.empty()) {
                cpu.code_name = "Skylake";
            }
        }
        if (cpu.max_tdp_watts == 0 && cpu.brand.ends_with("HX")) {
            cpu.max_tdp_watts = 55;
        } else if (cpu.max_tdp_watts == 0 && cpu.brand.ends_with("H")) {
            cpu.max_tdp_watts = 45;
        } else if (cpu.max_tdp_watts == 0 && cpu.brand.ends_with("P")) {
            cpu.max_tdp_watts = 28;
        } else if (cpu.max_tdp_watts == 0 && cpu.brand.ends_with("U")) {
            cpu.max_tdp_watts = 15;
        } else if (cpu.max_tdp_watts == 0 && cpu.brand.ends_with("K")) {
            cpu.max_tdp_watts = 125;
        }
    }

#ifdef NEFORCE_PLATFORM_LINUX

    string cpuinfo_field(const string& text, const char* key) {
        const size_t key_length = ::strlen(key);
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
            multiplier = 1024;
            value.resize(value.size() - 1);
        } else if (suffix == 'M' || suffix == 'm') {
            multiplier = 1024 * 1024;
            value.resize(value.size() - 1);
        } else if (suffix == 'G' || suffix == 'g') {
            multiplier = 1024 * 1024 * 1024;
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
            const uint32_t level = number32_of(level_text);
            const uint64_t bytes = cache_size_bytes(read_text((base + "size").data()));
            const uint32_t kib = static_cast<uint32_t>(bytes / 1024);
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

    uint32_t cpu_list_count(const string& text) {
        uint32_t count = 0;
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
                if (dash == string::npos) {
                    ++count;
                } else {
                    const auto first = number_of(token.substr(0, dash));
                    const auto last = number_of(token.substr(dash + 1));
                    if (last >= first) {
                        count += static_cast<uint32_t>(last - first + 1);
                    }
                }
            }
            position = end + 1;
        }
        return count;
    }

    void probe_platform(sysinfo::hardware_info& hardware, sysinfo::cpu_info& cpu) {
        fill_cpu_cpuid(cpu);
        const string info = read_text("/proc/cpuinfo");
        cpu.family = cpuinfo_field(info, "cpu family");
        cpu.model = cpuinfo_field(info, "model");
        cpu.stepping = cpuinfo_field(info, "stepping");
        cpu.revision = cpu.stepping;
        cpu.features = linux_instructions(cpuinfo_field(info, "flags"));
        fill_linux_caches(cpu);
        apply_cpu_metadata(cpu);
        const string pcore_list = read_text("/sys/devices/cpu_core/cpus");
        const string ecore_list = read_text("/sys/devices/cpu_atom/cpus");
        if (!pcore_list.empty() || !ecore_list.empty()) {
            cpu.pcore_count = cpu_list_count(pcore_list);
            cpu.ecore_count = cpu_list_count(ecore_list);
        }
        if (cpu.brand.empty()) {
            cpu.brand = cpuinfo_field(info, "model name");
        }

        const string package = read_text("/sys/devices/system/cpu/cpu0/topology/physical_package_id");
        if (!package.empty()) {
            cpu.package = "Socket " + string(package);
            cpu.package.trim();
        }

        string rapl = read_text("/sys/class/powercap/intel-rapl:0/constraint_0_max_power_uw");
        rapl.trim();
        const uint64_t tdp_uw = number_of(rapl);
        if (tdp_uw != 0) {
            cpu.max_tdp_watts = static_cast<uint32_t>(tdp_uw / 1'000'000U);
        }

        hardware.bios.brand = dmi_value("bios_vendor");
        hardware.bios.version = dmi_value("bios_version");
        hardware.bios.date = dmi_value("bios_date");
        hardware.bios.microcode = cpuinfo_field(info, "microcode");

        hardware.board.vendor = dmi_value("board_vendor");
        hardware.board.model = dmi_value("board_name");

        if (cpu.l3_kib != 0) {
            hardware.memory.llc_ring =
                    cpu.l3_kib >= 1024 ? format("{} MiB L3", cpu.l3_kib / 1024) : format("{} KiB L3", cpu.l3_kib);
        }
    }

#endif // NEFORCE_PLATFORM_LINUX

} // namespace

void sysinfo::probe_hardware() { probe_platform(hardware_info_, cpu_info_); }

NEFORCE_END_NAMESPACE__
