#include <NeForce/core/utility/packages.hpp>
#include <NeForce/network/ip_address.hpp>
NEFORCE_BEGIN_NAMESPACE__

ip_address ip_address::any(const uint16_t port, const int family) noexcept {
    ip_address result;
    if (family == AF_INET6) {
        ::sockaddr_in6 a6{};
        a6.sin6_family = AF_INET6;
        a6.sin6_addr = ::in6addr_any;
        a6.sin6_port = ::htons(port);
        result.addr_ = a6;
    } else if (family == AF_INET) {
        ::sockaddr_in a4{};
        a4.sin_family = AF_INET;
        a4.sin_addr.s_addr = INADDR_ANY;
        a4.sin_port = ::htons(port);
        result.addr_ = a4;
    }
    return move(result);
}

ip_address ip_address::loopback(const uint16_t port, const int family) noexcept {
    ip_address result;
    if (family == AF_INET6) {
        ::sockaddr_in6 a6{};
        a6.sin6_family = AF_INET6;
        a6.sin6_addr = ::in6addr_loopback;
        a6.sin6_port = ::htons(port);
        result.addr_ = a6;
    } else if (family == AF_INET) {
        ::sockaddr_in a4{};
        a4.sin_family = AF_INET;
        a4.sin_addr.s_addr = ::htonl(INADDR_LOOPBACK);
        a4.sin_port = ::htons(port);
        result.addr_ = a4;
    }
    return move(result);
}

const ::sockaddr* ip_address::data() const noexcept {
    return addr_.visit([](const auto& addr) -> const ::sockaddr* {
        if constexpr (!is_same_v<decay_t<decltype(addr)>, none_t>) {
            return reinterpret_cast<const ::sockaddr*>(&addr);
        }
        return nullptr;
    });
}

::sockaddr* ip_address::data() noexcept {
    return addr_.visit([](auto& addr) -> ::sockaddr* {
        if constexpr (!is_same_v<decay_t<decltype(addr)>, none_t>) {
            return reinterpret_cast<::sockaddr*>(&addr);
        }
        return nullptr;
    });
}

int ip_address::size() const noexcept {
    return addr_.visit([](const auto& addr) -> int {
        if constexpr (is_same_v<decay_t<decltype(addr)>, ::sockaddr_in>) {
            return sizeof(::sockaddr_in);
        } else if constexpr (is_same_v<decay_t<decltype(addr)>, ::sockaddr_in6>) {
            return sizeof(::sockaddr_in6);
        }
        return 0;
    });
}

NEFORCE_NODISCARD int ip_address::family() const noexcept {
    return addr_.visit([](const auto& addr) -> int {
        if constexpr (is_same_v<decay_t<decltype(addr)>, ::sockaddr_in>) {
            return addr.sin_family;
        } else if constexpr (is_same_v<decay_t<decltype(addr)>, ::sockaddr_in6>) {
            return addr.sin6_family;
        }
        return AF_UNSPEC;
    });
}

NEFORCE_NODISCARD uint16_t ip_address::port() const noexcept {
    return addr_.visit([](const auto& addr) -> uint16_t {
        if constexpr (is_same_v<decay_t<decltype(addr)>, ::sockaddr_in>) {
            return ::ntohs(addr.sin_port);
        } else if constexpr (is_same_v<decay_t<decltype(addr)>, ::sockaddr_in6>) {
            return ::ntohs(addr.sin6_port);
        }
        return 0;
    });
}

string ip_address::to_string() const {
    char buffer[INET6_ADDRSTRLEN];

    return addr_.visit([&](const auto& addr) -> string {
        using T = decay_t<decltype(addr)>;
        if constexpr (is_same_v<T, ::sockaddr_in>) {
            if (::inet_ntop(AF_INET, &addr.sin_addr, buffer, sizeof(buffer))) {
                return string(buffer) + ":" + _NEFORCE to_string(::ntohs(addr.sin_port));
            }
        } else if constexpr (is_same_v<T, ::sockaddr_in6>) {
            if (::inet_ntop(AF_INET6, &addr.sin6_addr, buffer, sizeof(buffer))) {
                return "["_s + string(buffer) + "]:" + _NEFORCE to_string(::ntohs(addr.sin6_port));
            }
        }
        return {};
    });
}

optional<ip_address> ip_address::parse(const string& host, const uint16_t port) noexcept {
    ip_address result;

    ::sockaddr_in a4{};
    if (::inet_pton(AF_INET, host.data(), &a4.sin_addr) == 1) {
        a4.sin_family = AF_INET;
        a4.sin_port = ::htons(port);
        result.addr_ = a4;
        return result;
    }

    ::sockaddr_in6 a6{};
    if (::inet_pton(AF_INET6, host.data(), &a6.sin6_addr) == 1) {
        a6.sin6_family = AF_INET6;
        a6.sin6_port = ::htons(port);
        result.addr_ = a6;
        return result;
    }

    return none;
}

bool ip_address::operator ==(const ip_address& other) const noexcept {
    if (!is_valid() || !other.is_valid()) {
        return true;
    }
    if (family() != other.family()) {
        return false;
    }
    if (port() != other.port()) {
        return false;
    }

    return addr_.visit([&](const auto& a1) -> bool {
        using T = decay_t<decltype(a1)>;
        if constexpr (is_same_v<T, ::sockaddr_in>) {
            const auto& a2 = other.addr_.get<::sockaddr_in>();
            return _NEFORCE memory_compare(&a1.sin_addr, &a2.sin_addr, sizeof(::in_addr)) == 0;
        } else if constexpr (is_same_v<T, ::sockaddr_in6>) {
            const auto& a2 = other.addr_.get<::sockaddr_in6>();
            return _NEFORCE memory_compare(&a1.sin6_addr, &a2.sin6_addr, sizeof(::in6_addr)) == 0;
        }
        return false;
    });
}

NEFORCE_END_NAMESPACE__
