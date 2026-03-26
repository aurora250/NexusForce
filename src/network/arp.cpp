#include "NeForce/network/arp.hpp"
#include "NeForce/core/utility/packages.hpp"
#include "NeForce/core/system/process.hpp"
#include "NeForce/core/memory/endian.hpp"
#ifdef NEFORCE_PLATFORM_WINDOWS
#include <iphlpapi.h>
#include <ws2tcpip.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

#ifdef NEFORCE_PLATFORM_LINUX
namespace {
    struct arp_ether_header {
        uint8_t  dest_mac[6];
        uint8_t  src_mac[6];
        uint16_t ether_type;
    };

    struct arp_packet {
        uint16_t hw_type;
        uint16_t proto_type;
        uint8_t  hw_addr_len;
        uint8_t  proto_addr_len;
        uint16_t opcode;
        uint8_t  sender_mac[6];
        uint8_t  sender_ip[4];
        uint8_t  target_mac[6];
        uint8_t  target_ip[4];
    };

    constexpr uint16_t ARP_ETHER_TYPE = 0x0806;
    constexpr uint16_t ARP_REQUEST = 1;
    constexpr uint16_t ARP_REPLY   = 2;
    constexpr uint8_t  BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
}
#endif


arp::~arp() {
    close();
}

bool arp::open(const char* iface) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (opened_) return true;

    if (iface) iface_ = iface;
    opened_ = true;
    return true;

#else
    if (fd_ != -1) return true;

    if (!get_local_info(iface)) return false;

    fd_ = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
    if (fd_ == -1) return false;

    struct sockaddr_ll addr;
    memset(&addr, 0, sizeof(addr));
    addr.sll_family = AF_PACKET;
    addr.sll_protocol = htons(ETH_P_ARP);
    addr.sll_ifindex = ifindex_;
    if (bind(fd_, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        close();
        return false;
    }

    int flags = fcntl(fd_, F_GETFL, 0);
    if (flags == -1) {
        close();
        return false;
    }
    if (fcntl(fd_, F_SETFL, flags | O_NONBLOCK) == -1) {
        close();
        return false;
    }

    return true;

#endif
}

void arp::close() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    opened_ = false;
#else
    if (fd_ != -1) {
        ::close(fd_);
        fd_ = -1;
    }
#endif
}

optional<mac_address> arp::resolve(const ip_address& target, milliseconds timeout) noexcept {
    if (!target.is_valid() || !target.is_ipv4()) return none;

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (!opened_) open(nullptr);

    ::ULONG mac[2] = {0};
    ::ULONG mac_len = 6;
    const ::DWORD ip_addr = endian::host_to_network<::DWORD>(target.address().get<sockaddr_in>().sin_addr.s_addr);

    const ::DWORD ret = ::SendARP(ip_addr, 0, mac, &mac_len);
    if (ret == NO_ERROR && mac_len == 6) {
        byte_t mac_bytes[6];
        memory_copy(mac_bytes, mac, 6);
        return mac_address(mac_bytes);
    }
    return none;

#else
    if (fd_ == -1 && !open(nullptr)) return none;

    uint8_t packet[sizeof(arp_ether_header) + sizeof(arp_packet)];
    memset(packet, 0, sizeof(packet));

    arp_ether_header* eth = (arp_ether_header*)packet;
    memcpy(eth->dest_mac, BROADCAST_MAC, 6);
    memcpy(eth->src_mac, local_mac_.bytes().data(), 6);
    eth->ether_type = htons(ARP_ETHER_TYPE);

    arp_packet* arp = (arp_packet*)(packet + sizeof(arp_ether_header));
    arp->hw_type = htons(1);
    arp->proto_type = htons(ETH_P_IP);
    arp->hw_addr_len = 6;
    arp->proto_addr_len = 4;
    arp->opcode = htons(ARP_REQUEST);
    memcpy(arp->sender_mac, local_mac_.bytes().data(), 6);
    memcpy(arp->sender_ip, &local_ip_, 4);
    memset(arp->target_mac, 0, 6);
    uint32_t target_ip = htonl(target.address().get<sockaddr_in>().sin_addr.s_addr);
    memcpy(arp->target_ip, &target_ip, 4);

    ssize_t sent = send(fd_, packet, sizeof(packet), 0);
    if (sent != (ssize_t)sizeof(packet)) return none;

    auto start = steady_clock::now();
    auto remaining = timeout;

    while (remaining.count() > 0) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(fd_, &readfds);
        struct timeval tv;
        tv.tv_sec = remaining.count() / 1000;
        tv.tv_usec = (remaining.count() % 1000) * 1000;

        int sel = select(fd_ + 1, &readfds, nullptr, nullptr, &tv);
        if (sel < 0) {
            if (errno == EINTR) {
                auto elapsed = steady_clock::now() - start;
                remaining = timeout - time_cast<milliseconds>(elapsed);
                continue;
            }
            return none;
        }
        if (sel == 0) break;

        uint8_t recv_buf[2048];
        struct sockaddr_ll from;
        socklen_t from_len = sizeof(from);
        ssize_t recv_len = recvfrom(fd_, recv_buf, sizeof(recv_buf), 0,
                                     (struct sockaddr*)&from, &from_len);
        if (recv_len < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                auto elapsed = steady_clock::now() - start;
                remaining = timeout - time_cast<milliseconds>(elapsed);
                continue;
            }
            return none;
        }

        if ((size_t)recv_len < sizeof(arp_ether_header) + sizeof(arp_packet)) continue;
        arp_ether_header* recv_eth = (arp_ether_header*)recv_buf;
        if (ntohs(recv_eth->ether_type) != ARP_ETHER_TYPE) continue;

        arp_packet* recv_arp = (arp_packet*)(recv_buf + sizeof(arp_ether_header));
        if (ntohs(recv_arp->opcode) != ARP_REPLY) continue;

        uint32_t recv_target_ip;
        memcpy(&recv_target_ip, recv_arp->target_ip, 4);
        if (recv_target_ip != local_ip_) continue;

        uint32_t recv_sender_ip;
        memcpy(&recv_sender_ip, recv_arp->sender_ip, 4);
        if (recv_sender_ip != target_ip) continue;

        uint8_t mac_bytes[6];
        memcpy(mac_bytes, recv_arp->sender_mac, 6);
        return mac_address(mac_bytes);
    }

    return none;

#endif
}

#ifdef NEFORCE_PLATFORM_LINUX
bool arp::local_info(const char* iface) noexcept {
    if (!iface || *iface == '\0') {
        int tmp = socket(AF_INET, SOCK_DGRAM, 0);
        if (tmp < 0) return false;

        struct ifreq ifr;
        struct ifconf ifc;
        char buf[4096];
        ifc.ifc_len = sizeof(buf);
        ifc.ifc_buf = buf;
        if (ioctl(tmp, SIOCGIFCONF, &ifc) < 0) {
            close(tmp);
            return false;
        }

        struct ifreq* it = ifc.ifc_req;
        const struct ifreq* end = it + (ifc.ifc_len / sizeof(struct ifreq));
        bool found = false;
        for (; it != end; ++it) {
            strncpy(ifr.ifr_name, it->ifr_name, IFNAMSIZ - 1);
            ifr.ifr_name[IFNAMSIZ - 1] = '\0';

            if (ioctl(tmp, SIOCGIFFLAGS, &ifr) < 0) continue;
            if (!(ifr.ifr_flags & IFF_UP)) continue;
            if (ifr.ifr_flags & IFF_LOOPBACK) continue;

            if (ioctl(tmp, SIOCGIFADDR, &ifr) < 0) continue;
            if (ifr.ifr_addr.sa_family == AF_INET) {
                found = true;
                iface_ = ifr.ifr_name;
                break;
            }
        }
        close(tmp);
        if (!found) return false;
    } else {
        iface_ = iface;
    }

    int tmp = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (tmp < 0) return false;

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, iface_.c_str(), IFNAMSIZ - 1);
    if (ioctl(tmp, SIOCGIFINDEX, &ifr) < 0) {
        close(tmp);
        return false;
    }
    ifindex_ = ifr.ifr_ifindex;
    close(tmp);

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return false;
    if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
        close(fd);
        return false;
    }
    memcpy(local_mac_.bytes().data(), ifr.ifr_hwaddr.sa_data, 6);
    close(fd);

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return false;
    if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) {
        close(fd);
        return false;
    }
    struct sockaddr_in* sin = (struct sockaddr_in*)&ifr.ifr_addr;
    local_ip_ = sin->sin_addr.s_addr;
    close(fd);

    return true;
}
#endif

NEFORCE_END_NAMESPACE__
