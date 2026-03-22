#include <NeForce/core/string/format.hpp>
#include <NeForce/core/time/clocks.hpp>
#include <NeForce/core/utility/hexadecimal.hpp>
#include <NeForce/core/utility/uuid.hpp>
NEFORCE_BEGIN_NAMESPACE__

random_mt& uuid::tl_rng() noexcept {
    thread_local random_mt rng;
    return rng;
}

uuid::uuid(memory_view<const byte_t, 16> bytes) noexcept {
    copy(bytes.begin(), bytes.end(), data_.begin());
}

uuid::uuid(const string_view bytes) {
    if (bytes.size() == 16) {
        copy(bytes.begin(), bytes.end(), data_.begin());
        return;
    }

    if (bytes.size() == 36) {
        if (bytes[8] != '-' || bytes[13] != '-' || bytes[18] != '-' || bytes[23] != '-') {
            NEFORCE_THROW_EXCEPTION(value_exception("invalid UUID format: missing or misplaced hyphens"));
        }

        size_t pos = 0;
        size_t byte_index = 0;
        while (byte_index < 16) {
            if (pos == 8 || pos == 13 || pos == 18 || pos == 23) {
                ++pos;
                continue;
            }
            if (pos + 1 >= bytes.size()) {
                NEFORCE_THROW_EXCEPTION(value_exception("unexpected end of UUID string"));
            }
            const byte_t high = hexadecimal::digit_value(bytes[pos]);
            const byte_t low = hexadecimal::digit_value(bytes[pos + 1]);
            data_[byte_index++] = (high << 4) | low;
            pos += 2;
        }
        return;
    }

    if (bytes.size() == 32) {
        for (size_t i = 0; i < 16; ++i) {
            const byte_t high = hexadecimal::digit_value(bytes[i * 2]);
            const byte_t low  = hexadecimal::digit_value(bytes[i * 2 + 1]);
            data_[i] = (high << 4) | low;
        }
        return;
    }

    NEFORCE_THROW_EXCEPTION(value_exception("invalid UUID length"));
}

void uuid::generate_v4() noexcept {
    const uint64_t part1 = tl_rng().next_uint64();
    const uint64_t part2 = tl_rng().next_uint64();

    memory_copy(data_.data(), &part1, 8);
    memory_copy(data_.data() + 8, &part2, 8);

    data_[6] = (data_[6] & 0x0F) | 0x40;
    data_[8] = (data_[8] & 0x3F) | 0x80;
}

void uuid::generate_v7() noexcept {
    const auto now = system_clock::now();
    const auto ms = time_cast<milliseconds>(now.since_epoch()).count();

    for (int i = 0; i < 6; ++i) {
        data_[i] = static_cast<byte_t>((ms >> (40 - i * 8)) & 0xFF);
    }

    const uint64_t rand_part = tl_rng().next_uint64();
    const uint16_t rand_rest = tl_rng().next_int(numeric_traits<uint16_t>::max());

    memory_copy(data_.data() + 6, &rand_part, 8);
    memory_copy(data_.data() + 14, &rand_rest, 2);

    data_[6] = (data_[6] & 0x0F) | 0x70;
    data_[8] = (data_[8] & 0x3F) | 0x80;
}

optional<uint64_t> uuid::timestamp_v7() const noexcept {
    if (!is_v7()) return {};

    uint64_t ts = 0;
    for (int i = 0; i < 6; ++i) {
        ts = (ts << 8) | data_[i];
    }
    return ts;
}

string uuid::to_string() const {
    auto to_hex = [](byte_t b) {
        return format("{:02x}", b);
    };

    return format("{}{}{}{}-{}{}-{}{}-{}{}-{}{}{}{}{}{}",
        to_hex(data_[0]), to_hex(data_[1]), to_hex(data_[2]), to_hex(data_[3]),
        to_hex(data_[4]), to_hex(data_[5]), to_hex(data_[6]), to_hex(data_[7]),
        to_hex(data_[8]), to_hex(data_[9]), to_hex(data_[10]), to_hex(data_[11]),
        to_hex(data_[12]), to_hex(data_[13]), to_hex(data_[14]), to_hex(data_[15]));
}

uuid uuid::v4() noexcept {
    uuid u;
    u.generate_v4();
    return u;
}

uuid uuid::v7() noexcept {
    uuid u;
    u.generate_v7();
    return u;
}

NEFORCE_END_NAMESPACE__
