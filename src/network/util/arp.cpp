#include <NeForce/core/memory/endian.hpp>
#include <NeForce/core/string/format.hpp>
#include <NeForce/network/util/arp.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <iphlpapi.h>
#endif
#ifdef NEFORCE_PLATFORM_POSIX
#    include <NeForce/core/time/clocks.hpp>
#    include <cerrno>
#    include <fcntl.h>
#    include <sys/ioctl.h>
#    include <sys/select.h>
#    include <unistd.h>
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#    include <NeForce/network/socket_base.hpp>
#    include <linux/if_arp.h>
#    include <linux/if_ether.h>
#    include <linux/if_packet.h>
#    include <linux/if.h>
#endif
#ifdef NEFORCE_PLATFORM_FREEBSD
#    include <ifaddrs.h>
#    include <net/bpf.h>
#    include <net/ethernet.h>
#    include <net/if.h>
#    include <net/if_arp.h>
#    include <net/if_dl.h>
#    include <netinet/in.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
#pragma pack(push, 1)

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

#pragma pack(pop)

    constexpr uint16_t ARP_ETHER_TYPE = 0x0806;
    constexpr uint16_t ARP_PROTO_IP = 0x0800;
    constexpr uint16_t ARP_REQUEST = 1;
    constexpr uint16_t ARP_REPLY = 2;
    constexpr byte_t BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
} // namespace


struct arp::impl {
    string iface_;

#ifdef NEFORCE_PLATFORM_WINDOWS
    bool opened_ = false;
#else
    mac_address local_mac_;
    uint32_t local_ip_ = 0;
    unsigned int ifindex_ = 0;

#    ifdef NEFORCE_PLATFORM_LINUX
    socket_base sock_;

    bool local_info(const char* iface);
#    elif defined(NEFORCE_PLATFORM_FREEBSD)
    int bpf_fd_ = -1;

    bool local_info(const char* iface);
#    endif
#endif
};


#ifdef NEFORCE_PLATFORM_LINUX
bool arp::impl::local_info(const char* iface) {
    socket_base query_sock;

    const bool res = query_sock.try_open(ip_family::inet4, socket_type::dgram);
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

    ifindex_ = static_cast<unsigned int>(ifr.ifr_ifindex);
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


#ifdef NEFORCE_PLATFORM_FREEBSD
bool arp::impl::local_info(const char* iface) {
    ::ifaddrs* list = nullptr;
    if (::getifaddrs(&list) != 0) {
        return false;
    }

    string chosen;
    if (iface != nullptr && *iface != '\0') {
        chosen = iface;
    } else {
        for (const ::ifaddrs* current = list; current != nullptr; current = current->ifa_next) {
            if (current->ifa_name == nullptr || current->ifa_addr == nullptr) {
                continue;
            }
            if (current->ifa_addr->sa_family != AF_INET) {
                continue;
            }
            if ((current->ifa_flags & IFF_UP) == 0 || (current->ifa_flags & IFF_LOOPBACK) != 0) {
                continue;
            }
            chosen = current->ifa_name;
            break;
        }
    }
    if (chosen.empty()) {
        ::freeifaddrs(list);
        return false;
    }

    bool got_ip = false;
    for (const ::ifaddrs* current = list; current != nullptr; current = current->ifa_next) {
        if (current->ifa_name == nullptr || current->ifa_addr == nullptr) {
            continue;
        }
        if (string_compare(current->ifa_name, chosen.data()) != 0) {
            continue;
        }
        if (current->ifa_addr->sa_family == AF_INET) {
            const auto* value = reinterpret_cast<const ::sockaddr_in*>(current->ifa_addr);
            local_ip_ = value->sin_addr.s_addr;
            got_ip = true;
        } else if (current->ifa_addr->sa_family == AF_LINK) {
            const auto* link = reinterpret_cast<const ::sockaddr_dl*>(current->ifa_addr);
            if (link->sdl_alen == 6) {
                memory_copy(const_cast<byte_t*>(local_mac_.bytes().data()), LLADDR(link), 6);
            }
        }
    }
    ::freeifaddrs(list);

    if (!got_ip) {
        return false;
    }
    iface_ = move(chosen);
    ifindex_ = ::if_nametoindex(iface_.data());
    return ifindex_ != 0;
}
#endif


arp::arp() :
impl_(make_unique<impl>()) {}

arp::~arp() {
    if (impl_) {
        close();
    }
}

arp::arp(arp&& other) noexcept = default;

arp& arp::operator=(arp&& other) noexcept = default;

bool arp::open(const char* iface) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (impl_->opened_) {
        return true;
    }

    if (iface != nullptr) {
        impl_->iface_ = iface;
    }
    impl_->opened_ = true;
    return true;

#elif defined(NEFORCE_PLATFORM_FREEBSD)
    if (impl_->bpf_fd_ >= 0) {
        return true;
    }
    if (!impl_->local_info(iface)) {
        return false;
    }

    for (int index = 0; index < 16; ++index) {
        const string device = format("/dev/bpf{}", index);
        impl_->bpf_fd_ = ::open(device.data(), O_RDWR);
        if (impl_->bpf_fd_ >= 0 || errno != EBUSY) {
            break;
        }
    }
    if (impl_->bpf_fd_ < 0) {
        return false;
    }

    ::ifreq ifr{};
    string_copy(ifr.ifr_name, impl_->iface_.data(), IFNAMSIZ - 1);
    if (::ioctl(impl_->bpf_fd_, BIOCSETIF, &ifr) < 0) {
        close();
        return false;
    }

    unsigned int enabled = 1;
    if (::ioctl(impl_->bpf_fd_, BIOCIMMEDIATE, &enabled) < 0 || ::ioctl(impl_->bpf_fd_, BIOCSHDRCMPLT, &enabled) < 0) {
        close();
        return false;
    }

    const int flags = ::fcntl(impl_->bpf_fd_, F_GETFL, 0);
    if (flags < 0 || ::fcntl(impl_->bpf_fd_, F_SETFL, flags | O_NONBLOCK) < 0) {
        close();
        return false;
    }

    return true;

#else
    if (impl_->sock_.is_open()) {
        return true;
    }
    if (!impl_->local_info(iface)) {
        return false;
    }

    const bool res = impl_->sock_.try_open(ip_family::packet, socket_type::raw,
                                           // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
                                           static_cast<socket_base::protocol>(endian::host_to_network<int>(ETH_P_ARP)));
    if (!res) {
        return false;
    }

    ::sockaddr_ll addr{};
    memory_zero(&addr);
    addr.sll_family = AF_PACKET;
    addr.sll_protocol = endian::host_to_network<uint16_t>(ETH_P_ARP);
    addr.sll_ifindex = static_cast<int>(impl_->ifindex_);
    if (::bind(impl_->sock_.native_handle(), reinterpret_cast<::sockaddr*>(&addr), sizeof(addr)) < 0) {
        close();
        return false;
    }

    if (!impl_->sock_.set_nonblocking(true)) {
        close();
        return false;
    }

    return true;

#endif
}

void arp::close() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    impl_->opened_ = false;
#elif defined(NEFORCE_PLATFORM_FREEBSD)
    if (impl_->bpf_fd_ >= 0) {
        ::close(impl_->bpf_fd_);
        impl_->bpf_fd_ = -1;
    }
    impl_->local_mac_ = mac_address{};
    impl_->local_ip_ = 0;
    impl_->ifindex_ = 0;
#else
    impl_->sock_.close();
#endif
}

optional<mac_address> arp::resolve(const ip_address& target, const milliseconds timeout) {
    if (!target.is_valid() || !target.is_ipv4()) {
        return none;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    // Using system SendARP
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

    // Construct ARP request packet
    byte_t packet[sizeof(arp_ether_header) + sizeof(arp_packet)];
    memory_zero(packet);

    auto* eth = reinterpret_cast<arp_ether_header*>(packet);
    memory_copy(eth->dest_mac, BROADCAST_MAC, 6);
    memory_copy(eth->src_mac, impl_->local_mac_.bytes().data(), 6);
    eth->ether_type = endian::host_to_network<uint16_t>(ARP_ETHER_TYPE);

    // Fill ARP packet
    auto* const arp_pkt = reinterpret_cast<arp_packet*>(packet + sizeof(arp_ether_header));
    arp_pkt->hw_type = endian::host_to_network<uint16_t>(1);
    arp_pkt->proto_type = endian::host_to_network<uint16_t>(ARP_PROTO_IP);
    arp_pkt->hw_addr_len = 6;
    arp_pkt->proto_addr_len = 4;
    arp_pkt->opcode = endian::host_to_network<uint16_t>(ARP_REQUEST);
    memory_copy(arp_pkt->sender_mac, impl_->local_mac_.bytes().data(), 6);
    memory_copy(arp_pkt->sender_ip, &impl_->local_ip_, 4);
    memory_zero(arp_pkt->target_mac, 6);
    const uint32_t target_ip = target.address().get<::sockaddr_in>().sin_addr.s_addr;
    memory_copy(arp_pkt->target_ip, &target_ip, 4);

#    ifdef NEFORCE_PLATFORM_FREEBSD
    if (impl_->bpf_fd_ < 0 && !open(impl_->iface_.empty() ? nullptr : impl_->iface_.data())) {
        return none;
    }

    const ssize_t sent = ::write(impl_->bpf_fd_, packet, sizeof(packet));
    if (sent != static_cast<ssize_t>(sizeof(packet))) {
        return none;
    }

    unsigned int buffer_length = 0;
    if (::ioctl(impl_->bpf_fd_, BIOCGBLEN, &buffer_length) < 0 || buffer_length < sizeof(::bpf_hdr)) {
        return none;
    }
    vector<byte_t> buffer(buffer_length);

    const auto start = steady_clock::now();
    auto remaining = timeout;

    while (remaining.count() > 0) {
        ::fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(impl_->bpf_fd_, &readfds);
        ::timeval tv;
        tv.tv_sec = remaining.count() / 1000;
        tv.tv_usec = (remaining.count() % 1000) * 1000;

        const int sel = ::select(impl_->bpf_fd_ + 1, &readfds, nullptr, nullptr, &tv);
        if (sel < 0) {
            if (errno == EINTR) {
                const auto elapsed = steady_clock::now() - start;
                remaining = timeout - time_cast<milliseconds>(elapsed);
                continue;
            }
            return none;
        }
        if (sel == 0) {
            break;
        }

        const ssize_t received = ::read(impl_->bpf_fd_, buffer.data(), buffer.size());
        if (received < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                const auto elapsed = steady_clock::now() - start;
                remaining = timeout - time_cast<milliseconds>(elapsed);
                continue;
            }
            return none;
        }

        const byte_t* cursor = buffer.data();
        const byte_t* const limit = buffer.data() + received;
        while (cursor + sizeof(::bpf_hdr) <= limit) {
            const auto* header = reinterpret_cast<const ::bpf_hdr*>(cursor);
            if (header->bh_hdrlen + header->bh_caplen > static_cast<unsigned int>(limit - cursor)) {
                break;
            }
            const byte_t* const frame = cursor + header->bh_hdrlen;
            const unsigned int captured = header->bh_caplen;

            if (captured >= sizeof(arp_ether_header) + sizeof(arp_packet)) {
                const auto* recv_eth = reinterpret_cast<const arp_ether_header*>(frame);
                if (endian::network_to_host<uint16_t>(recv_eth->ether_type) == ARP_ETHER_TYPE) {
                    const auto* recv_arp = reinterpret_cast<const arp_packet*>(frame + sizeof(arp_ether_header));
                    if (endian::network_to_host<uint16_t>(recv_arp->opcode) == ARP_REPLY) {
                        uint32_t recv_target_ip = 0;
                        memory_copy(&recv_target_ip, recv_arp->target_ip, 4);
                        uint32_t recv_sender_ip = 0;
                        memory_copy(&recv_sender_ip, recv_arp->sender_ip, 4);
                        if (recv_target_ip == impl_->local_ip_ && recv_sender_ip == target_ip) {
                            byte_t mac_bytes[6];
                            memory_copy(mac_bytes, recv_arp->sender_mac, 6);
                            return mac_address(mac_bytes);
                        }
                    }
                }
            }

            cursor += BPF_WORDALIGN(header->bh_hdrlen + header->bh_caplen);
        }
    }

    return none;

#    else
    if (!impl_->sock_.is_open() && !open(impl_->iface_.empty() ? nullptr : impl_->iface_.data())) {
        return none;
    }
    const int fd = impl_->sock_.native_handle();

    // Set dest address and broadcast
    ::sockaddr_ll dest_addr{};
    memory_zero(&dest_addr);
    dest_addr.sll_family = AF_PACKET;
    dest_addr.sll_protocol = endian::host_to_network<uint16_t>(ETH_P_ARP);
    dest_addr.sll_ifindex = static_cast<int>(impl_->ifindex_);
    dest_addr.sll_hatype = ARPHRD_ETHER;
    dest_addr.sll_pkttype = PACKET_BROADCAST;
    dest_addr.sll_halen = 6;
    memory_copy(dest_addr.sll_addr, BROADCAST_MAC, 6);

    const ssize_t sent =
            ::sendto(fd, packet, sizeof(packet), 0, reinterpret_cast<::sockaddr*>(&dest_addr), sizeof(dest_addr));
    if (sent != sizeof(packet)) {
        return none;
    }

    // Wait ARP response
    const auto start = steady_clock::now();
    auto remaining = timeout;

    while (remaining.count() > 0) {
        ::fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(impl_->sock_.native_handle(), &readfds);
        ::timeval tv;
        tv.tv_sec = remaining.count() / 1000;
        tv.tv_usec = (remaining.count() % 1000) * 1000;

        const int sel = ::select(impl_->sock_.native_handle() + 1, &readfds, nullptr, nullptr, &tv);
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

        // Receive and resolve packet
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

        if (static_cast<size_t>(recv_len) < sizeof(arp_ether_header) + sizeof(arp_packet)) {
            continue;
        }

        const auto* recv_eth = reinterpret_cast<const arp_ether_header*>(recv_buf);
        if (endian::network_to_host<uint16_t>(recv_eth->ether_type) != ARP_ETHER_TYPE) {
            continue;
        }

        const auto* recv_arp = reinterpret_cast<const arp_packet*>(recv_buf + sizeof(arp_ether_header));
        if (endian::network_to_host<uint16_t>(recv_arp->opcode) != ARP_REPLY) {
            continue;
        }

        uint32_t recv_target_ip = 0;
        memory_copy(&recv_target_ip, recv_arp->target_ip, 4);
        if (recv_target_ip != impl_->local_ip_) {
            continue;
        }

        uint32_t recv_sender_ip = 0;
        memory_copy(&recv_sender_ip, recv_arp->sender_ip, 4);
        if (recv_sender_ip != target_ip) {
            continue;
        }

        // Get MAC address
        byte_t mac_bytes[6];
        memory_copy(mac_bytes, recv_arp->sender_mac, 6);
        return mac_address(mac_bytes);
    }

    return none;
#    endif
#endif
}

NEFORCE_END_NAMESPACE__
