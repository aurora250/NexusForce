#ifndef NEFORCE_NETWORK_SOCKET_ICMP_SOCKET_HPP__
#define NEFORCE_NETWORK_SOCKET_ICMP_SOCKET_HPP__
#include "NeForce/core/container/vector.hpp"
#include "NeForce/network/socket/socket_base.hpp"
NEFORCE_BEGIN_NAMESPACE__

#pragma pack(push, 1)

struct icmp_header {
    uint8_t  type;
    uint8_t  code;
    uint16_t checksum;
    uint16_t id;
    uint16_t sequence;
};

struct ip_header {
    uint8_t  ihl : 4;
    uint8_t  version : 4;
    uint8_t  tos;
    uint16_t total_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t checksum;
    uint32_t src_addr;
    uint32_t dest_addr;
};

#pragma pack(pop)


class NEFORCE_API icmp_socket final : public socket_base {
public:
    enum icmp_type : uint8_t {
        ICMP_ECHO_REPLY   = 0,
        ICMP_ECHO_REQUEST = 8,
        ICMP_TIME_EXCEEDED = 11
    };

    struct ping_result {
        ip_address destination;
        milliseconds rtt;
        size_t reply_size;
        uint8_t reply_ttl;
        bool success;
    };

    struct traceroute_hop {
        ip_address address;
        milliseconds rtt[3];
        bool reached;
    };

private:
    bool receive_reply(
        milliseconds timeout, uint16_t expected_id, uint16_t expected_seq,
        ip_address& sender, icmp_header& out_header,
        vector<char>& out_data, uint8_t& recv_ttl);

    void send_echo_request(
        const ip_address& dest, uint16_t id, uint16_t seq,
        uint8_t ttl, const void* data, size_t data_len);

public:
    icmp_socket() = default;

    explicit icmp_socket(native_handle_type fd) noexcept
    : socket_base(fd) {}

    void open(int family = AF_INET);

    ping_result ping(const ip_address& dest,
                     milliseconds timeout,
                     uint16_t sequence = 0,
                     const void* data = nullptr,
                     size_t data_len = 0);

    vector<traceroute_hop> traceroute(const ip_address& dest,
                                       int max_hops = 30,
                                       milliseconds probe_timeout = milliseconds(1000),
                                       int probes_per_hop = 3);
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_SOCKET_ICMP_SOCKET_HPP__
