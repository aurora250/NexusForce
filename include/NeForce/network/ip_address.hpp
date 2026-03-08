#ifndef NEFORCE_NETWORK_IP_ADDRESS_HPP__
#define NEFORCE_NETWORK_IP_ADDRESS_HPP__
#include "NeForce/core/utility/variant.hpp"
#include "NeForce/core/utility/optional.hpp"
#include "NeForce/core/interface/istringify.hpp"
#ifdef NEFORCE_PLATFORM_WINDOWS
#include <ws2tcpip.h>
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#include <netinet/in.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

class NEFORCE_API ip_address : public istringify<ip_address> {
public:
    using address_type = variant<none_t, ::sockaddr_in, ::sockaddr_in6>;

private:
    address_type addr_;

public:
    ip_address() noexcept = default;

    explicit ip_address(const ::sockaddr_in& addr4) noexcept
    : addr_(addr4) {}

    explicit ip_address(const ::sockaddr_in6& addr6) noexcept
    : addr_(addr6) {}

    ip_address(const ip_address& other) noexcept = default;
    ip_address& operator =(const ip_address& other) noexcept = default;

    ip_address(ip_address&& other) noexcept = default;
    ip_address& operator =(ip_address&& other) noexcept = default;

    NEFORCE_NODISCARD bool is_valid() const noexcept {
        return !addr_.holds_alternative<none_t>();
    }

    NEFORCE_NODISCARD bool is_ipv4() const noexcept {
        return addr_.holds_alternative<::sockaddr_in>();
    }

    NEFORCE_NODISCARD bool is_ipv6() const noexcept {
        return addr_.holds_alternative<::sockaddr_in6>();
    }

    NEFORCE_NODISCARD static ip_address any(uint16_t port, int family = AF_INET) noexcept;
    NEFORCE_NODISCARD static ip_address loopback(uint16_t port, int family = AF_INET) noexcept;

    NEFORCE_NODISCARD const ::sockaddr* data() const noexcept;
    NEFORCE_NODISCARD ::sockaddr* data() noexcept;
    NEFORCE_NODISCARD int size() const noexcept;

    NEFORCE_NODISCARD const address_type& address() const noexcept {
        return addr_;
    }

    NEFORCE_NODISCARD int family() const noexcept;
    NEFORCE_NODISCARD uint16_t port() const noexcept;

    NEFORCE_NODISCARD string to_string() const;
    NEFORCE_NODISCARD static optional<ip_address> parse(const string& host, uint16_t port) noexcept;

    NEFORCE_NODISCARD bool operator ==(const ip_address& other) const noexcept;
    NEFORCE_NODISCARD bool operator !=(const ip_address& other) const noexcept {
        return !(*this == other);
    }
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_IP_ADDRESS_HPP__
