#include <NeForce/core/memory/endian.hpp>
#include <NeForce/core/utility/packages.hpp>
#include <NeForce/network/ip_address.hpp>
#ifdef NEFORCE_PLATFORM_LINUX
#    include <arpa/inet.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
    struct const_data_visitor {
        template <typename T>
        enable_if_t<!is_same_v<T, none_t>, const ::sockaddr*> operator()(const T& value) noexcept {
            return reinterpret_cast<const ::sockaddr*>(&value);
        }
        template <typename T>
        enable_if_t<is_same_v<T, none_t>, const ::sockaddr*> operator()(const T&) noexcept {
            return nullptr;
        }
    };

    struct data_visitor {
        template <typename T>
        enable_if_t<!is_same_v<T, none_t>, ::sockaddr*> operator()(T& value) noexcept {
            return reinterpret_cast<::sockaddr*>(&value);
        }
        template <typename T>
        enable_if_t<is_same_v<T, none_t>, ::sockaddr*> operator()(T&) noexcept {
            return nullptr;
        }
    };

    struct size_visitor {
        template <typename T>
        enable_if_t<is_same_v<T, ::sockaddr_in>, int> operator()(const T&) noexcept {
            return sizeof(::sockaddr_in);
        }
        template <typename T>
        enable_if_t<is_same_v<T, ::sockaddr_in6>, int> operator()(const T&) noexcept {
            return sizeof(::sockaddr_in6);
        }
        template <typename T>
        enable_if_t<!is_same_v<T, ::sockaddr_in> && !is_same_v<T, ::sockaddr_in6>, int> operator()(const T&) noexcept {
            return 0;
        }
    };

    struct family_visitor {
        template <typename T>
        enable_if_t<is_same_v<T, ::sockaddr_in>, int> operator()(const T& value) noexcept {
            return value.sin_family;
        }
        template <typename T>
        enable_if_t<is_same_v<T, ::sockaddr_in6>, int> operator()(const T& value) noexcept {
            return value.sin6_family;
        }
        template <typename T>
        enable_if_t<!is_same_v<T, ::sockaddr_in> && !is_same_v<T, ::sockaddr_in6>, int> operator()(const T&) noexcept {
            return AF_UNSPEC;
        }
    };

    struct port_visitor {
        template <typename T>
        enable_if_t<is_same_v<T, ::sockaddr_in>, uint16_t> operator()(const T& value) noexcept {
            return endian::network_to_host<uint16_t>(value.sin_port);
        }
        template <typename T>
        enable_if_t<is_same_v<T, ::sockaddr_in6>, uint16_t> operator()(const T& value) noexcept {
            return endian::network_to_host<uint16_t>(value.sin6_port);
        }
        template <typename T>
        enable_if_t<!is_same_v<T, ::sockaddr_in> && !is_same_v<T, ::sockaddr_in6>, uint16_t>
        operator()(const T&) noexcept {
            return 0;
        }
    };

    struct to_string_visitor {
        char buffer[INET6_ADDRSTRLEN];

        template <typename T>
        enable_if_t<is_same_v<T, ::sockaddr_in>, string> operator()(const T& value) {
            if (::inet_ntop(AF_INET, &value.sin_addr, buffer, sizeof(buffer))) {
                return string(buffer) + ":" + _NEFORCE to_string(endian::network_to_host<uint16_t>(value.sin_port));
            }
            return ""_s;
        }
        template <typename T>
        enable_if_t<is_same_v<T, ::sockaddr_in6>, string> operator()(const T& value) {
            if (::inet_ntop(AF_INET6, &value.sin6_addr, buffer, sizeof(buffer))) {
                return "["_s + string(buffer) +
                       "]:" + _NEFORCE to_string(endian::network_to_host<uint16_t>(value.sin6_port));
            }
            return ""_s;
        }
        template <typename T>
        enable_if_t<!is_same_v<T, ::sockaddr_in> && !is_same_v<T, ::sockaddr_in6>, string> operator()(const T&) {
            return ""_s;
        }
    };

    struct equal_visitor {
        const ip_address& other;

        explicit equal_visitor(const ip_address& other) noexcept :
        other(other) {}

        template <typename T>
        enable_if_t<is_same_v<T, ::sockaddr_in>, bool> operator()(const T& value) noexcept {
            if (!other.is_ipv4()) {
                return false;
            }
            const auto& a2 = other.address().get<::sockaddr_in>();
            return _NEFORCE memory_compare(&value.sin_addr, &a2.sin_addr, sizeof(::in_addr)) == 0;
        }
        template <typename T>
        enable_if_t<is_same_v<T, ::sockaddr_in6>, bool> operator()(const T& value) noexcept {
            if (!other.is_ipv6()) {
                return false;
            }
            const auto& a2 = other.address().get<::sockaddr_in6>();
            return _NEFORCE memory_compare(&value.sin6_addr, &a2.sin6_addr, sizeof(::in6_addr)) == 0;
        }
        template <typename T>
        enable_if_t<!is_same_v<T, ::sockaddr_in> && !is_same_v<T, ::sockaddr_in6>, bool> operator()(const T&) noexcept {
            return false;
        }
    };
} // namespace


ip_address ip_address::any(const ports port, const int family) noexcept {
    ip_address result;
    if (family == AF_INET6) {
        ::sockaddr_in6 a6{};
        a6.sin6_family = AF_INET6;
        a6.sin6_addr = ::in6addr_any;
        a6.sin6_port = endian::host_to_network(static_cast<uint16_t>(port));
        result.addr_ = a6;
    } else if (family == AF_INET) {
        ::sockaddr_in a4{};
        a4.sin_family = AF_INET;
        a4.sin_addr.s_addr = INADDR_ANY;
        a4.sin_port = endian::host_to_network(static_cast<uint16_t>(port));
        result.addr_ = a4;
    }
    return result;
}

ip_address ip_address::loopback(const ports port, const int family) noexcept {
    ip_address result;
    if (family == AF_INET6) {
        ::sockaddr_in6 a6{};
        a6.sin6_family = AF_INET6;
        a6.sin6_addr = ::in6addr_loopback;
        a6.sin6_port = endian::host_to_network(static_cast<uint16_t>(port));
        result.addr_ = a6;
    } else if (family == AF_INET) {
        ::sockaddr_in a4{};
        a4.sin_family = AF_INET;
        a4.sin_addr.s_addr = endian::host_to_network<uint32_t>(INADDR_LOOPBACK);
        a4.sin_port = endian::host_to_network(static_cast<uint16_t>(port));
        result.addr_ = a4;
    }
    return move(result);
}

const ::sockaddr* ip_address::data() const noexcept { return addr_.visit(const_data_visitor{}); }

::sockaddr* ip_address::data() noexcept { return addr_.visit(data_visitor{}); }

int ip_address::size() const noexcept { return addr_.visit(size_visitor{}); }

int ip_address::family() const noexcept { return addr_.visit(family_visitor{}); }

ports ip_address::port() const noexcept { return ports{addr_.visit(port_visitor{})}; }

string ip_address::to_string() const { return addr_.visit(to_string_visitor{}); }

optional<ip_address> ip_address::parse(const string& host, const ports port) noexcept {
    ip_address result;

    ::sockaddr_in a4{};
    if (::inet_pton(AF_INET, host.data(), &a4.sin_addr) == 1) {
        a4.sin_family = AF_INET;
        a4.sin_port = endian::host_to_network(static_cast<uint16_t>(port));
        result.addr_ = a4;
        return result;
    }

    ::sockaddr_in6 a6{};
    if (::inet_pton(AF_INET6, host.data(), &a6.sin6_addr) == 1) {
        a6.sin6_family = AF_INET6;
        a6.sin6_port = endian::host_to_network(static_cast<uint16_t>(port));
        result.addr_ = a6;
        return result;
    }

    return none;
}

bool ip_address::operator==(const ip_address& other) const noexcept {
    if (!is_valid() && !other.is_valid()) {
        return true;
    }
    if (!is_valid() || !other.is_valid()) {
        return false;
    }

    if (family() != other.family()) {
        return false;
    }
    if (port() != other.port()) {
        return false;
    }
    return addr_.visit(equal_visitor{other});
}

NEFORCE_END_NAMESPACE__
