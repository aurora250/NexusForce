#include <NeForce/core/string/format.hpp>
#include <NeForce/core/utility/hexadecimal.hpp>
#include <NeForce/network/util/arp.hpp>
NEFORCE_BEGIN_NAMESPACE__

optional<mac_address> mac_address::parse(const string_view str) {
    if (str.size() != 17) {
        return none;
    }

    mac_address result;
    byte_t* ptr = result.bytes_.data();
    size_t pos = 0;

    char expected_sep = '\0';

    for (int i = 0; i < 6; ++i) {
        if (i > 0) {
            const char sep = str[pos++];
            if (i == 1) {
                if (sep != ':' && sep != '-') {
                    return none;
                }
                expected_sep = sep;
            } else {
                if (sep != expected_sep) {
                    return none;
                }
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

    arp resolver;
    if (!resolver.open(iface)) {
        return none;
    }

    return resolver.resolve(ip, milliseconds(2000));
}

string mac_address::to_string() const {
    return format("{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}", bytes_[0], bytes_[1], bytes_[2], bytes_[3], bytes_[4],
                  bytes_[5]);
}

NEFORCE_END_NAMESPACE__
