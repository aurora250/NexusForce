#ifndef NEFORCE_NETWORK_ARP_HPP__
#define NEFORCE_NETWORK_ARP_HPP__
#include "NeForce/core/time/duration.hpp"
#include "NeForce/network/mac_address.hpp"
NEFORCE_BEGIN_NAMESPACE__

class NEFORCE_API arp {
private:
#ifdef NEFORCE_PLATFORM_WINDOWS
    string iface_;
    bool opened_ = false;
#else
    int fd_ = -1;
    string iface_;
    mac_address local_mac_;
    uint32_t local_ip_;
    int ifindex_ = -1;
#endif

#ifdef NEFORCE_PLATFORM_LINUX
    bool local_info(const char* iface) noexcept;
#endif

public:
    arp() = default;
    ~arp();

    bool open(const char* iface = nullptr) noexcept;
    void close() noexcept;

    optional<mac_address> resolve(const ip_address& target, milliseconds timeout = milliseconds(1000)) noexcept;
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_ARP_HPP__
