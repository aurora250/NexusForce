#ifndef NEFORCE_NETWORK_MAC_ADDRESS_HPP__
#define NEFORCE_NETWORK_MAC_ADDRESS_HPP__
#include "NeForce/core/container/array.hpp"
#include "NeForce/network/ip_address.hpp"
NEFORCE_BEGIN_NAMESPACE__

class NEFORCE_API mac_address : public istringify<mac_address> {
public:
    static constexpr size_t MAC_LEN = 6;
    using bytes_type = array<byte_t, MAC_LEN>;

private:
    bytes_type bytes_{};

public:
    mac_address() noexcept = default;

    explicit mac_address(const byte_t* bytes) noexcept {
        copy(bytes, bytes + MAC_LEN, bytes_.begin());
    }

    explicit mac_address(const bytes_type& bytes) noexcept
    : bytes_(bytes) {}

    static optional<mac_address> parse(string_view str) noexcept;
    static optional<mac_address> parse(const ip_address& ip, const char* iface = nullptr) noexcept;

    string to_string() const;

    bool operator ==(const mac_address& other) const noexcept {
        return bytes_ == other.bytes_;
    }

    bool operator !=(const mac_address& other) const noexcept {
        return !(*this == other);
    }

    const bytes_type& bytes() const noexcept { return bytes_; }
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_MAC_ADDRESS_HPP__
