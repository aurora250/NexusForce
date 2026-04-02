#ifndef NEFORCE_NETWORK_PORTS_HPP__
#define NEFORCE_NETWORK_PORTS_HPP__
#include "NeForce/core/interface/iobject.hpp"
NEFORCE_BEGIN_NAMESPACE__

struct alignas(uint16_t) NEFORCE_API ports : iobject<ports> {
    enum raw : uint16_t {
        undef = 0,
        http = 80,
        ws = 80,
        https = 443,
        wss = 443,
        ftp = 21,
        tftp = 69,
        ssh = 22,
        telnet = 23,
        smtp = 25,
        dns = 53,
        pop3 = 110,
        imap = 143
    };

    raw port{raw::undef};

    constexpr ports() noexcept = default;

    constexpr ports(const raw port) noexcept :
    port(port) {}

    constexpr explicit ports(const uint16_t port) noexcept :
    port(static_cast<raw>(port)) {}

    constexpr explicit operator bool() const noexcept { return port != ports::undef; }

    constexpr explicit operator uint16_t() const noexcept { return static_cast<uint16_t>(port); }

    static ports parse(string_view scheme) noexcept;
    string to_string() const;
    string to_string(bool is_ws) const;
};


NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE_INLINE constexpr bool operator==(const ports lhs, const uint16_t rhs) noexcept {
    return lhs.port == static_cast<ports::raw>(rhs);
}

NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE_INLINE constexpr bool operator==(const uint16_t lhs, const ports rhs) noexcept {
    return static_cast<ports::raw>(lhs) == rhs.port;
}

NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE_INLINE constexpr bool operator==(const ports lhs,
                                                                         const ports::raw rhs) noexcept {
    return lhs.port == rhs;
}

NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE_INLINE constexpr bool operator==(const ports::raw lhs,
                                                                         const ports rhs) noexcept {
    return lhs == rhs.port;
}

NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE_INLINE constexpr bool operator==(const ports lhs, const ports rhs) noexcept {
    return lhs.port == rhs.port;
}

NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE_INLINE constexpr bool operator!=(const ports lhs, const uint16_t rhs) noexcept {
    return lhs.port != static_cast<ports::raw>(rhs);
}

NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE_INLINE constexpr bool operator!=(const uint16_t lhs, const ports rhs) noexcept {
    return static_cast<ports::raw>(lhs) != rhs.port;
}

NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE_INLINE constexpr bool operator!=(const ports lhs,
                                                                         const ports::raw rhs) noexcept {
    return lhs.port != rhs;
}

NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE_INLINE constexpr bool operator!=(const ports::raw lhs,
                                                                         const ports rhs) noexcept {
    return lhs != rhs.port;
}

NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE_INLINE constexpr bool operator!=(const ports lhs, const ports rhs) noexcept {
    return lhs.port != rhs.port;
}

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_PORTS_HPP__
