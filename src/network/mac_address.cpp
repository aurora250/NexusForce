#include <NeForce/core/memory/endian.hpp>
#include <NeForce/core/string/format.hpp>
#include <NeForce/core/utility/hexadecimal.hpp>
#include <NeForce/network/mac_address.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <iphlpapi.h>
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#    include <linux/if.h>
#    include <net/if_arp.h>
#    include <sys/ioctl.h>
#    include <unistd.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

optional<mac_address> mac_address::parse(const string_view str) {
    if (str.size() != 17) {
        return none;
    }

    mac_address result;
    byte_t* ptr = result.bytes_.data();
    size_t pos = 0;

    for (int i = 0; i < 6; ++i) {
        if (i > 0) {
            const char sep = str[pos++];
            if (sep != ':' && sep != '-') {
                return none;
            }
        }
        if (pos + 2 > str.size()) {
            return none;
        }

        const auto xpair = hexadecimal::xdigit_value(str[pos], str[pos + 1]);
        if (!xpair.first) {
            return none;
        }
        ptr[i] = xpair.second;
        pos += 2;
    }
    return result;
}


optional<mac_address> mac_address::parse(const ip_address& ip, const char* iface) {
    if (!ip.is_valid() || !ip.is_ipv4()) {
        return none;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::ULONG mac[2] = {0};
    ::ULONG mac_len = 6; // byte count SendARP expected

    const auto ip_addr = endian::network_to_host<::ULONG>(ip.address().get<::sockaddr_in>().sin_addr.s_addr);

    const ::DWORD ret = ::SendARP(ip_addr, 0, static_cast<::ULONG*>(mac), &mac_len);
    if (ret == NO_ERROR && mac_len == 6) {
        byte_t mac_bytes[6];
        memory_copy(static_cast<byte_t*>(mac_bytes), mac, 6);
        return mac_address(static_cast<byte_t*>(mac_bytes));
    }
    return none;
#else
    ::arpreq req;
    memory_zero(&req);

    auto* sin = reinterpret_cast<::sockaddr_in*>(&req.arp_pa);
    sin->sin_family = AF_INET;
    sin->sin_addr.s_addr = ip.address().get<::sockaddr_in>().sin_addr.s_addr;

    if (iface) {
        string_copy(req.arp_dev, iface, IFNAMSIZ - 1);
        req.arp_dev[IFNAMSIZ - 1] = '\0';
    }

    const int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        return none;
    }

    const int ret = ::ioctl(fd, SIOCGARP, &req);
    ::close(fd);

    if (ret == 0 && (req.arp_flags & ATF_COM)) {
        uint8_t mac_bytes[6];
        memory_copy(mac_bytes, req.arp_ha.sa_data, 6);
        return mac_address(mac_bytes);
    }
    return none;
#endif
}

string mac_address::to_string() const {
    return format("{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}", bytes_[0], bytes_[1], bytes_[2], bytes_[3], bytes_[4],
                  bytes_[5]);
}

NEFORCE_END_NAMESPACE__
