#include <NeForce/core/utility/byte_size.hpp>
#include <NeForce/core/utility/packages.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    constexpr uint64_t binary_multipliers[] = {
            1ULL,                                      // B
            1024ULL,                                   // KB
            1024ULL * 1024,                            // MB
            1024ULL * 1024 * 1024,                     // GB
            1024ULL * 1024 * 1024 * 1024,              // TB
            1024ULL * 1024 * 1024 * 1024 * 1024,       // PB
            1024ULL * 1024 * 1024 * 1024 * 1024 * 1024 // EB
    };

    constexpr uint64_t decimal_multipliers[] = {
            1ULL,                                      // B
            1000ULL,                                   // KB
            1000ULL * 1000,                            // MB
            1000ULL * 1000 * 1000,                     // GB
            1000ULL * 1000 * 1000 * 1000,              // TB
            1000ULL * 1000 * 1000 * 1000 * 1000,       // PB
            1000ULL * 1000 * 1000 * 1000 * 1000 * 1000 // EB
    };

    struct unit_mapping {
        string_view name;
        string_view alt_name;
        byte_size::unit unit;
    };

    constexpr unit_mapping unit_mappings[] = {
            {"B", nullptr, byte_size::unit::B}, {"KB", "K", byte_size::unit::KB}, {"MB", "M", byte_size::unit::MB},
            {"GB", "G", byte_size::unit::GB},   {"TB", "T", byte_size::unit::TB}, {"PB", "P", byte_size::unit::PB},
            {"EB", "E", byte_size::unit::EB},
    };

    uint64_t get_multiplier(byte_size::unit u, const bool binary) {
        const auto& table = binary ? binary_multipliers : decimal_multipliers;
        const auto index = static_cast<size_t>(u);
        if (index >= size(table)) {
            NEFORCE_THROW_EXCEPTION(value_exception("Invalid unit for multiplier"));
        }
        return table[index];
    }

    byte_size::unit parse_unit(const string& unit_str) {
        if (unit_str.empty()) {
            return byte_size::unit::B;
        }

        for (const auto& mapping: unit_mappings) {
            if (unit_str.compare_ignore_case(mapping.name) == 0) {
                return mapping.unit;
            }
            if (!mapping.alt_name.empty() && unit_str.compare_ignore_case(mapping.alt_name) == 0) {
                return mapping.unit;
            }
        }

        return byte_size::unit::AUTO;
    }

    string unit_to_string(const byte_size::unit unit) {
        switch (unit) {
            case byte_size::unit::B:
                return "B";
            case byte_size::unit::KB:
                return "KB";
            case byte_size::unit::MB:
                return "MB";
            case byte_size::unit::GB:
                return "GB";
            case byte_size::unit::TB:
                return "TB";
            case byte_size::unit::PB:
                return "PB";
            case byte_size::unit::EB:
                return "EB";
            default:
                return "";
        }
    }
} // namespace


byte_size::byte_size(const decimal_t value, const unit u, const bool binary) {
    if (value < 0.0L) {
        NEFORCE_THROW_EXCEPTION(value_exception("Memory size cannot be negative"));
    }
    if (u == unit::AUTO) {
        NEFORCE_THROW_EXCEPTION(value_exception("Cannot construct byte_size with AUTO unit"));
    }

    const uint64_t multiplier = get_multiplier(u, binary);
    const decimal_t bytes = value * static_cast<decimal_t>(multiplier);

    if (bytes > static_cast<decimal_t>(numeric_traits<uint64_t>::max())) {
        NEFORCE_THROW_EXCEPTION(value_exception("Memory size exceeds maximum representable value"));
    }

    bytes_ = static_cast<uint64_t>(bytes + static_cast<decimal_t>(0.5));
}

byte_size byte_size::parse(string_view str, const bool binary) {
    str = str.trim();
    if (str.empty()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Empty memory size string"));
    }

    size_t i = 0;
    while (i < str.size() && (is_digit(str[i]) || str[i] == '.' || str[i] == '-' || str[i] == '+')) {
        ++i;
    }

    const string_view num_str = str.view(0, i);
    if (num_str.empty()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Missing numeric value"));
    }

    decimal_t value = numeric_traits<decimal_t>::quiet_nan();
    try {
        value = decimal::parse(num_str).value();
    } catch (...) {
        NEFORCE_THROW_EXCEPTION(value_exception(("Invalid numeric value: "_s + num_str).data()));
    }
    if (value < 0.0L) {
        NEFORCE_THROW_EXCEPTION(value_exception("Memory size cannot be negative"));
    }

    const string unit_str = str.view(i).trim();
    const unit unit = parse_unit(unit_str);
    if (unit == unit::AUTO && !unit_str.empty()) {
        NEFORCE_THROW_EXCEPTION(value_exception(("Unknown unit: " + unit_str).data()));
    }

    return {value, unit, binary};
}

decimal_t byte_size::as(const unit u, const bool binary) const {
    if (u == unit::AUTO) {
        NEFORCE_THROW_EXCEPTION(value_exception("unit cannot be Auto for as"));
    }
    const uint64_t divisor = get_multiplier(u, binary);
    return static_cast<decimal_t>(bytes_) / static_cast<decimal_t>(divisor);
}

string byte_size::to_string(const unit u, const int precision, const bool binary) const {
    if (u == unit::AUTO) {
        if (bytes_ == 0) {
            const string fmt = "{" + format(":.{}f", precision) + "} B";
            return format(fmt.view(), static_cast<decimal_t>(0));
        }

        const string fmt = "{" + format(":.{}f", precision) + "} {}";
        const uint64_t base = binary ? 1024 : 1000;
        auto val = static_cast<decimal_t>(bytes_);
        auto current_unit = unit::B;

        constexpr unit units[] = {unit::B, unit::KB, unit::MB, unit::GB, unit::TB, unit::PB, unit::EB};

        for (size_t i = 0; i < size(units) - 1; ++i) {
            if (val < static_cast<decimal_t>(base)) {
                break;
            }
            val /= static_cast<decimal_t>(base);
            current_unit = units[i + 1];
        }

        return format(fmt.view(), val, unit_to_string(current_unit));
    }

    const string fmt = "{" + format(":.{}f", precision) + "} {}";
    const decimal_t val = as(u, binary);
    return format(fmt.view(), val, unit_to_string(u));
}

NEFORCE_END_NAMESPACE__
