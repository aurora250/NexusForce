#include <NeForce/core/memory/endian.hpp>
#include <NeForce/network/util/arp.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <iphlpapi.h>
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#    include <NeForce/core/time/clocks.hpp>
#    include <linux/if_ether.h>
#    include <linux/if_packet.h>
#    include <linux/if_arp.h>
#    include <net/if.h>
#    include <sys/ioctl.h>
#    include <fcntl.h>
#    include <cerrno>
#endif
NEFORCE_BEGIN_NAMESPACE__

#ifdef NEFORCE_PLATFORM_LINUX
namespace {
#    pragma pack(push, 1)

    struct arp_ether_header {
        uint8_t dest_mac[6];
        uint8_t src_mac[6];
        uint16_t ether_type;
    };

    struct arp_packet {
        uint16_t hw_type;
        uint16_t proto_type;
        uint8_t hw_addr_len;
        uint8_t proto_addr_len;
        uint16_t opcode;
        uint8_t sender_mac[6];
        uint8_t sender_ip[4];
        uint8_t target_mac[6];
        uint8_t target_ip[4];
    };

#    pragma pack(pop)

    constexpr uint16_t ARP_ETHER_TYPE = 0x0806;
    constexpr uint16_t ARP_REQUEST = 1;
    constexpr uint16_t ARP_REPLY = 2;
    constexpr byte_t BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
} // namespace
#endif


#ifdef NEFORCE_PLATFORM_LINUX
bool arp::local_info(const char* iface) {
    socket_base query_sock;

    const bool res = query_sock.try_open(AF_INET, SOCK_DGRAM, 0);
    if (!res) {
        return false;
    }

    const int fd = query_sock.native_handle();
    ::ifreq ifr;

    if (iface == nullptr || *iface == '\0') {
        ::ifconf ifc;
        char buf[4096];
        ifc.ifc_len = sizeof(buf);
        ifc.ifc_buf = buf;
        if (::ioctl(fd, SIOCGIFCONF, &ifc) < 0) {
            return false;
        }

        const ::ifreq* it = ifc.ifc_req;
        const ::ifreq* end = it + (ifc.ifc_len / sizeof(::ifreq));
        bool found = false;
        for (; it != end; ++it) {
            string_copy(ifr.ifr_name, it->ifr_name, IFNAMSIZ - 1);
            ifr.ifr_name[IFNAMSIZ - 1] = '\0';

            if (::ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
                continue;
            }
            if ((ifr.ifr_flags & IFF_UP) == 0) {
                continue;
            }
            if ((ifr.ifr_flags & IFF_LOOPBACK) != 0) {
                continue;
            }
            if (::ioctl(fd, SIOCGIFADDR, &ifr) < 0) {
                continue;
            }

            if (ifr.ifr_addr.sa_family == AF_INET) {
                found = true;
                iface_ = ifr.ifr_name;
                break;
            }
        }
        if (!found) {
            return false;
        }
    } else {
        iface_ = iface;
    }

    memory_zero(&ifr);
    string_copy(ifr.ifr_name, iface_.data(), IFNAMSIZ - 1);
    if (::ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
        return false;
    }

    ifindex_ = ifr.ifr_ifindex;
    if (::ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
        return false;
    }
    memory_copy(const_cast<byte_t*>(local_mac_.bytes().data()), ifr.ifr_hwaddr.sa_data, 6);

    if (::ioctl(fd, SIOCGIFADDR, &ifr) < 0) {
        return false;
    }
    const auto* sin = reinterpret_cast<::sockaddr_in*>(&ifr.ifr_addr);
    local_ip_ = sin->sin_addr.s_addr;

    return true;
}
#endif

bool arp::open(const char* iface) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (opened_) {
        return true;
    }

    if (iface != nullptr) {
        iface_ = iface;
    }
    opened_ = true;
    return true;

#else
    if (sock_.is_open()) {
        return true;
    }
    if (!local_info(iface)) {
        return false;
    }

    const bool res = sock_.try_open(AF_PACKET, SOCK_RAW, endian::host_to_network<int>(ETH_P_ARP));
    if (!res) {
        return false;
    }

    ::sockaddr_ll addr{};
    memory_zero(&addr);
    addr.sll_family = AF_PACKET;
    addr.sll_protocol = endian::host_to_network<uint16_t>(ETH_P_ARP);
    addr.sll_ifindex = ifindex_;
    if (::bind(sock_.native_handle(), reinterpret_cast<::sockaddr*>(&addr), sizeof(addr)) < 0) {
        close();
        return false;
    }

    if (!sock_.set_nonblocking(true)) {
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
    sock_.close();
#endif
}

optional<mac_address> arp::resolve(const ip_address& target, const milliseconds timeout) {
    if (!target.is_valid() || !target.is_ipv4()) {
        return none;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    // 使用系统 SendARP
    ::ULONG mac[2] = {0};
    ::ULONG mac_len = 6;
    const auto ip_addr = static_cast<::IPAddr>(target.address().get<::sockaddr_in>().sin_addr.s_addr);

    const ::DWORD ret = ::SendARP(ip_addr, 0, mac, &mac_len);
    if (ret == NO_ERROR && mac_len == 6) {
        byte_t mac_bytes[6];
        memory_copy(mac_bytes, mac, 6);
        return mac_address(mac_bytes);
    }
    return none;

#else
    if (!sock_.is_open() && !open(iface_.empty() ? nullptr : iface_.data())) {
        return none;
    }
    int fd = sock_.native_handle();

    // 构造 ARP 请求报文
    byte_t packet[sizeof(arp_ether_header) + sizeof(arp_packet)];
    memory_zero(packet);

    auto* eth = reinterpret_cast<arp_ether_header*>(packet);
    memory_copy(eth->dest_mac, BROADCAST_MAC, 6);
    memory_copy(eth->src_mac, local_mac_.bytes().data(), 6);
    eth->ether_type = endian::host_to_network<uint16_t>(ARP_ETHER_TYPE);

    // 填充 ARP 报文
    auto* const arp_pkt = reinterpret_cast<arp_packet*>(packet + sizeof(arp_ether_header));
    arp_pkt->hw_type = endian::host_to_network<uint16_t>(1);
    arp_pkt->proto_type = endian::host_to_network<uint16_t>(ETH_P_IP);
    arp_pkt->hw_addr_len = 6;
    arp_pkt->proto_addr_len = 4;
    arp_pkt->opcode = endian::host_to_network<uint16_t>(ARP_REQUEST);
    memory_copy(arp_pkt->sender_mac, local_mac_.bytes().data(), 6);
    memory_copy(arp_pkt->sender_ip, &local_ip_, 4);
    memory_set(arp_pkt->target_mac, 0, 6);
    const uint32_t target_ip = target.address().get<::sockaddr_in>().sin_addr.s_addr;
    memory_copy(arp_pkt->target_ip, &target_ip, 4);

    // 设置目标地址并发送广播
    ::sockaddr_ll dest_addr{};
    memory_zero(&dest_addr);
    dest_addr.sll_family = AF_PACKET;
    dest_addr.sll_protocol = endian::host_to_network<uint16_t>(ETH_P_ARP);
    dest_addr.sll_ifindex = ifindex_;
    dest_addr.sll_hatype = ARPHRD_ETHER;
    dest_addr.sll_pkttype = PACKET_BROADCAST;
    dest_addr.sll_halen = 6;
    memory_copy(dest_addr.sll_addr, BROADCAST_MAC, 6);

    const ssize_t sent =
            ::sendto(fd, packet, sizeof(packet), 0, reinterpret_cast<::sockaddr*>(&dest_addr), sizeof(dest_addr));
    if (sent != static_cast<ssize_t>(sizeof(packet))) {
        return none;
    }

    // 轮询等待 ARP 回复
    const auto start = steady_clock::now();
    auto remaining = timeout;

    while (remaining.count() > 0) {
        ::fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sock_.native_handle(), &readfds);
        ::timeval tv;
        tv.tv_sec = remaining.count() / 1000;
        tv.tv_usec = (remaining.count() % 1000) * 1000;

        const int sel = ::select(sock_.native_handle() + 1, &readfds, nullptr, nullptr, &tv);
        if (sel < 0) {
            if (errno == EINTR) {
                auto elapsed = steady_clock::now() - start;
                remaining = timeout - time_cast<milliseconds>(elapsed);
                continue;
            }
            return none;
        }
        if (sel == 0) {
            break;
        }

        // 接收并解析响应报文
        byte_t recv_buf[2048];
        ::sockaddr_ll from;
        ::socklen_t from_len = sizeof(from);
        const auto recv_len =
                ::recvfrom(fd, recv_buf, sizeof(recv_buf), 0, reinterpret_cast<::sockaddr*>(&from), &from_len);

        if (recv_len < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                auto elapsed = steady_clock::now() - start;
                remaining = timeout - time_cast<milliseconds>(elapsed);
                continue;
            }
            return none;
        }

        // 检查报文长度是否足够
        if (static_cast<size_t>(recv_len) < sizeof(arp_ether_header) + sizeof(arp_packet)) {
            continue;
        }

        // 验证以太类型是否为 ARP
        const auto* recv_eth = reinterpret_cast<const arp_ether_header*>(recv_buf);
        if (endian::network_to_host<uint16_t>(recv_eth->ether_type) != ARP_ETHER_TYPE) {
            continue;
        }

        // 验证是否为 ARP 回复
        const auto* recv_arp = reinterpret_cast<const arp_packet*>(recv_buf + sizeof(arp_ether_header));
        if (endian::network_to_host<uint16_t>(recv_arp->opcode) != ARP_REPLY) {
            continue;
        }

        // 验证目标 IP 是否为本机 IP
        uint32_t recv_target_ip = 0;
        memory_copy(&recv_target_ip, recv_arp->target_ip, 4);
        if (recv_target_ip != local_ip_) {
            continue;
        }

        // 验证发送方 IP 是否为目标 IP
        uint32_t recv_sender_ip = 0;
        memory_copy(&recv_sender_ip, recv_arp->sender_ip, 4);
        if (recv_sender_ip != target_ip) {
            continue;
        }

        // 提取发送方 MAC 地址并返回
        byte_t mac_bytes[6];
        memory_copy(mac_bytes, recv_arp->sender_mac, 6);
        return mac_address(mac_bytes);
    }

    return none;

#endif
}

NEFORCE_END_NAMESPACE__
