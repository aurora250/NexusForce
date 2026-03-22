#ifndef NEFORCE_CORE_UTILITY_UUID_HPP__
#define NEFORCE_CORE_UTILITY_UUID_HPP__
#include "NeForce/core/interface/istringify.hpp"
#include "NeForce/core/memory/memory_view.hpp"
#include "NeForce/core/numeric/random.hpp"
#include "NeForce/core/utility/optional.hpp"
NEFORCE_BEGIN_NAMESPACE__

class NEFORCE_API uuid : public istringify<uuid> {
private:
    array<byte_t, 16> data_;

    static random_mt& tl_rng() noexcept;

public:
    uuid() noexcept = default;

    explicit uuid(memory_view<const byte_t, 16> bytes) noexcept;
    explicit uuid(string_view bytes);

    void generate_v4() noexcept;
    void generate_v7() noexcept;

    NEFORCE_NODISCARD int version() const noexcept {
        return (data_[6] >> 4) & 0x0F;
    }

    NEFORCE_NODISCARD bool is_v4() const noexcept {
        return version() == 4;
    }
    NEFORCE_NODISCARD bool is_v7() const noexcept {
        return version() == 7;
    }

    NEFORCE_NODISCARD optional<uint64_t> timestamp_v7() const noexcept;

    NEFORCE_NODISCARD string to_string() const;

    NEFORCE_NODISCARD memory_view<const byte_t, 16> bytes() const noexcept {
        return memory_view<const byte_t, 16>(data_);
    }

    NEFORCE_NODISCARD auto begin() const noexcept {
        return data_.begin();
    }

    NEFORCE_NODISCARD auto end() const noexcept {
        return data_.end();
    }

    static uuid v4() noexcept;
    static uuid v7() noexcept;
};

NEFORCE_NODISCARD inline uuid operator ""_uuid(const char* str, size_t len) {
    return uuid(string_view(str, len));
}


template <>
struct hash<uuid> {
    size_t operator ()(const uuid& uuid) const noexcept {
        const auto& bytes = uuid.bytes();
        size_t hash = 0;
        for (size_t i = 0; i < 16; i += sizeof(size_t)) {
            size_t part = 0;
            memory_copy(&part, bytes.data() + i, sizeof(size_t));
            hash ^= part;
        }
        return hash;
    }
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_UTILITY_UUID_HPP__
