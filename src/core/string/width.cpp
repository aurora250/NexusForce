#include <NeForce/core/simd/bytes.hpp>
#include <NeForce/core/simd/memory.hpp>
#include <NeForce/core/string/codepoint.hpp>
#include <NeForce/core/string/width.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    int codepoint_width(const codepoint& cp) {
        const auto value = cp.value();
        if (value < 0x20 || value == 0x7F) {
            return 1;
        }
        // Combining marks and variation selectors are zero width;
        // clamping them to one column would break every grapheme that carries a mark.
        return cp.display_width();
    }

    // Every ASCII byte occupies exactly one column (control bytes included),
    // so ASCII runs can be counted without decoding: this is the dominating case for code and English prose,
    // and one 16-byte block costs a single load plus a movemask.
    constexpr size_t ascii_block = 16;

    bool block_is_ascii(const char* data) noexcept { return simd::to_bitmask(simd::loadu_si128(data)) == 0; }

    bool is_ascii(const char* data, const size_t size) noexcept {
        size_t i = 0;
        for (; i + ascii_block <= size; i += ascii_block) {
            if (!block_is_ascii(data + i)) {
                return false;
            }
        }
        for (; i < size; ++i) {
            if (static_cast<byte_t>(data[i]) >= 0x80) {
                return false;
            }
        }
        return true;
    }

    // Widths are queried repeatedly for the same strings while streaming and scrolling,
    // so memoise the non-ASCII ones: they are the only strings whose width is not the byte count.
    // A 64-bit hash plus the byte length keeps collisions out of reach;
    // long strings bypass the cache.
    // The table is per thread, so concurrent readers never race on each other's slots.
    constexpr size_t width_cache_capacity = 1024;
    constexpr size_t width_cache_max_length = 512;

    struct width_cache_slot {
        uint64_t hash = 0;
        uint32_t length = 0;
        int32_t width = 0;
        uint32_t used = 0;
    };

    uint64_t hash_bytes(const char* data, const size_t size) noexcept {
        uint64_t hash = 0xcbf29ce484222325ULL;
        for (size_t i = 0; i < size; ++i) {
            hash ^= static_cast<byte_t>(data[i]);
            hash *= 0x100000001b3ULL;
        }
        return hash;
    }

    int compute_display_width(const string_view text) {
        const char* raw = text.data();
        const size_t size = text.size();
        size_t index = 0;
        int width = 0;
        while (index < size) {
            while (index + ascii_block <= size) {
                if (!block_is_ascii(raw + index)) {
                    break;
                }
                index += ascii_block;
                width += static_cast<int>(ascii_block);
            }
            while (index < size && static_cast<byte_t>(raw[index]) < 0x80) {
                ++index;
                ++width;
            }
            if (index >= size) {
                break;
            }
            size_t consumed = 0;
            const codepoint cp =
                    codepoint::decode_utf8(reinterpret_cast<const byte_t*>(raw + index), consumed, size - index);
            width += codepoint_width(cp);
            index += consumed;
        }
        return width;
    }
} // namespace


int text_display_width(const string_view text) {
    if (text.empty()) {
        return 0;
    }
    if (text.size() > width_cache_max_length) {
        return compute_display_width(text);
    }
    // ASCII needs no memoisation:
    // the byte count is the width and the scan is a word test per 8 bytes,
    // which is cheaper than hashing the string for a cache lookup.
    if (is_ascii(text.data(), text.size())) {
        return static_cast<int>(text.size());
    }

    thread_local width_cache_slot cache[width_cache_capacity];

    const uint64_t hash = hash_bytes(text.data(), text.size());
    const size_t probe = static_cast<size_t>(hash) & (width_cache_capacity - 1);
    for (size_t step = 0; step < 8; ++step) {
        const width_cache_slot& slot = cache[(probe + step) & (width_cache_capacity - 1)];
        if (slot.used == 0) {
            break;
        }
        if (slot.hash == hash && slot.length == text.size()) {
            return slot.width;
        }
    }

    const int width = compute_display_width(text);
    width_cache_slot& slot = cache[probe];
    slot.hash = hash;
    slot.length = static_cast<uint32_t>(text.size());
    slot.width = width;
    slot.used = 1;
    return width;
}

string display_truncate(const string_view text, const int width) {
    if (width <= 0) {
        return {};
    }
    int used = 0;
    size_t index = 0;
    while (index < text.size()) {
        const size_t begin = index;
        const auto point = codepoint::decode_utf8(reinterpret_cast<const byte_t*>(text.data()), index, text.size());
        const int cells = point.display_width();
        if (used + cells > width) {
            index = begin;
            break;
        }
        used += cells;
    }
    return string(text.data(), index);
}

string display_fit(const string_view text, const int width, const bool right_align) {
    if (width <= 0) {
        return {};
    }
    const int current = text_display_width(text);
    if (current >= width) {
        return display_truncate(text, width);
    }
    string out;
    out.reserve(static_cast<size_t>(width));
    if (right_align) {
        out.append(static_cast<size_t>(width - current), ' ');
        out.append(text.data(), text.size());
    } else {
        out.append(text.data(), text.size());
        out.append(static_cast<size_t>(width - current), ' ');
    }
    return out;
}

NEFORCE_END_NAMESPACE__
