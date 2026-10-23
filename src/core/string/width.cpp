#include <NeForce/core/string/codepoint.hpp>
#include <NeForce/core/string/utf_iterator.hpp>
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

    // Widths are queried repeatedly for the same strings while streaming and scrolling,
    // so memoise them. A 64-bit hash plus the byte length keeps collisions out of reach;
    // long strings bypass the cache.
    constexpr size_t width_cache_capacity = 4096;
    constexpr size_t width_cache_max_length = 512;

    struct width_cache_slot {
        uint64_t hash = 0;
        size_t length = 0;
        int width = 0;
        bool used = false;
    };

    uint64_t hash_bytes(const char* data, const size_t size) noexcept {
        uint64_t hash = 0xcbf29ce484222325ULL;
        for (size_t i = 0; i < size; ++i) {
            hash ^= static_cast<unsigned char>(data[i]);
            hash *= 0x100000001b3ULL;
        }
        return hash;
    }

    int compute_display_width(const string_view text) {
        int width = 0;
        const auto* data = reinterpret_cast<const byte_t*>(text.data());
        for (utf8_iterator it(data, text.size()); it != utf8_iterator(); ++it) {
            width += codepoint_width(*it);
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

    static width_cache_slot cache[width_cache_capacity];
    static size_t used = 0;

    const uint64_t hash = hash_bytes(text.data(), text.size());
    const size_t probe = static_cast<size_t>(hash) & (width_cache_capacity - 1);
    for (size_t step = 0; step < 8; ++step) {
        const width_cache_slot& slot = cache[(probe + step) & (width_cache_capacity - 1)];
        if (!slot.used) {
            break;
        }
        if (slot.hash == hash && slot.length == text.size()) {
            return slot.width;
        }
    }

    const int width = compute_display_width(text);
    const size_t target = probe & (width_cache_capacity - 1);
    width_cache_slot& slot = cache[target];
    slot.hash = hash;
    slot.length = text.size();
    slot.width = width;
    slot.used = true;
    ++used;
    (void) used;
    return width;
}

NEFORCE_END_NAMESPACE__
