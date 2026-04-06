#ifndef NEFORCE_NETWORK_ARP_HPP__
#define NEFORCE_NETWORK_ARP_HPP__
#include "NeForce/core/time/duration.hpp"
#include "NeForce/network/mac_address.hpp"
#ifdef NEFORCE_PLATFORM_LINUX
#    include "NeForce/network/socket/socket_base.hpp"
#endif
NEFORCE_BEGIN_NAMESPACE__

class NEFORCE_API arp {
private:
    string iface_;
#ifdef NEFORCE_PLATFORM_WINDOWS
    bool opened_ = false;
#else
    socket_base sock_;
    mac_address local_mac_;
    uint32_t local_ip_;
    int ifindex_ = -1;
#endif

#ifdef NEFORCE_PLATFORM_LINUX
    bool local_info(const char* iface);
#endif

public:
    arp() = default;
    ~arp() = default;

    bool open(const char* iface = nullptr);
    void close() noexcept;

    optional<mac_address> resolve(const ip_address& target, milliseconds timeout = milliseconds(1000));
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_ARP_HPP__
