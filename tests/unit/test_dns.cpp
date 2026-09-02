#include <NeForce/core/memory/endian.hpp>
#include <NeForce/network/dns/dns_client.hpp>
#include <gtest/gtest.h>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <ws2tcpip.h>
#else
#    include <arpa/inet.h>
#endif
using namespace neforce;

namespace {
    byte_vector make_header(const uint16_t id, const uint16_t flags, const uint16_t qdcount = 0,
                            const uint16_t ancount = 0, const uint16_t nscount = 0, const uint16_t arcount = 0) {
        byte_vector h(12);
        auto write_u16 = [&](const size_t offset, const uint16_t val) {
            const uint16_t nv = endian::host_to_network(val);
            memory_copy(h.data() + offset, &nv, 2);
        };
        write_u16(0, id);
        write_u16(2, flags);
        write_u16(4, qdcount);
        write_u16(6, ancount);
        write_u16(8, nscount);
        write_u16(10, arcount);
        return h;
    }

    byte_vector encode_name(const string& domain) {
        byte_vector e;
        size_t start = 0;
        while (start < domain.size()) {
            auto pos = domain.find('.', start);
            if (pos == string::npos) {
                pos = domain.size();
            }
            auto len = pos - start;
            e.push_back(static_cast<byte_t>(len));
            e.insert(e.end(), domain.begin() + start, domain.begin() + pos);
            start = pos + 1;
        }
        e.push_back(0x00);
        return e;
    }

    void append_question(byte_vector& v, const string& name, const uint16_t qtype, const uint16_t qclass = 1) {
        auto enc = encode_name(name);
        v.insert(v.end(), enc.begin(), enc.end());
        const uint16_t ntype = endian::host_to_network(qtype);
        const uint16_t nclass = endian::host_to_network(qclass);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&ntype), reinterpret_cast<const byte_t*>(&ntype) + 2);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&nclass), reinterpret_cast<const byte_t*>(&nclass) + 2);
    }

    void append_a_record(byte_vector& v, const string& name, const uint32_t ttl, const string& ip) {
        auto enc = encode_name(name);
        v.insert(v.end(), enc.begin(), enc.end());
        const uint16_t ntype = endian::host_to_network<uint16_t>(1);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&ntype), reinterpret_cast<const byte_t*>(&ntype) + 2);
        const uint16_t nclass = endian::host_to_network<uint16_t>(1);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&nclass), reinterpret_cast<const byte_t*>(&nclass) + 2);
        const uint32_t nttl = endian::host_to_network(ttl);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&nttl), reinterpret_cast<const byte_t*>(&nttl) + 4);
        const uint16_t nrdlen = endian::host_to_network<uint16_t>(4);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&nrdlen), reinterpret_cast<const byte_t*>(&nrdlen) + 2);
        in_addr addr;
        ::inet_pton(AF_INET, ip.data(), &addr);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&addr), reinterpret_cast<const byte_t*>(&addr) + 4);
    }

    void append_aaaa_record(byte_vector& v, const string& name, const uint32_t ttl, const string& ip) {
        auto enc = encode_name(name);
        v.insert(v.end(), enc.begin(), enc.end());
        const uint16_t ntype = endian::host_to_network<uint16_t>(28);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&ntype), reinterpret_cast<const byte_t*>(&ntype) + 2);
        const uint16_t nclass = endian::host_to_network<uint16_t>(1);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&nclass), reinterpret_cast<const byte_t*>(&nclass) + 2);
        const uint32_t nttl = endian::host_to_network(ttl);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&nttl), reinterpret_cast<const byte_t*>(&nttl) + 4);
        const uint16_t nrdlen = endian::host_to_network<uint16_t>(16);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&nrdlen), reinterpret_cast<const byte_t*>(&nrdlen) + 2);
        in6_addr addr;
        ::inet_pton(AF_INET6, ip.data(), &addr);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&addr), reinterpret_cast<const byte_t*>(&addr) + 16);
    }

    void append_domain_name_record(byte_vector& v, const string& name, const uint16_t rtype, const uint32_t ttl,
                                   const string& target) {
        auto enc = encode_name(name);
        v.insert(v.end(), enc.begin(), enc.end());
        const uint16_t ntype = endian::host_to_network(rtype);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&ntype), reinterpret_cast<const byte_t*>(&ntype) + 2);
        const uint16_t nclass = endian::host_to_network<uint16_t>(1);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&nclass), reinterpret_cast<const byte_t*>(&nclass) + 2);
        const uint32_t nttl = endian::host_to_network(ttl);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&nttl), reinterpret_cast<const byte_t*>(&nttl) + 4);
        auto tenc = encode_name(target);
        const uint16_t nrdlen = endian::host_to_network<uint16_t>(tenc.size());
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&nrdlen), reinterpret_cast<const byte_t*>(&nrdlen) + 2);
        v.insert(v.end(), tenc.begin(), tenc.end());
    }

    void append_cname_record(byte_vector& v, const string& name, const uint32_t ttl, const string& cname) {
        append_domain_name_record(v, name, 5, ttl, cname);
    }

    void append_mx_record(byte_vector& v, const string& name, const uint32_t ttl, const uint16_t pref,
                          const string& exchange) {
        auto enc = encode_name(name);
        v.insert(v.end(), enc.begin(), enc.end());
        const uint16_t ntype = endian::host_to_network<uint16_t>(15);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&ntype), reinterpret_cast<const byte_t*>(&ntype) + 2);
        const uint16_t nclass = endian::host_to_network<uint16_t>(1);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&nclass), reinterpret_cast<const byte_t*>(&nclass) + 2);
        const uint32_t nttl = endian::host_to_network(ttl);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&nttl), reinterpret_cast<const byte_t*>(&nttl) + 4);
        auto eenc = encode_name(exchange);
        const uint16_t nrdlen = endian::host_to_network<uint16_t>(2 + eenc.size());
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&nrdlen), reinterpret_cast<const byte_t*>(&nrdlen) + 2);
        const uint16_t npref = endian::host_to_network(pref);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&npref), reinterpret_cast<const byte_t*>(&npref) + 2);
        v.insert(v.end(), eenc.begin(), eenc.end());
    }

    void append_txt_record(byte_vector& v, const string& name, const uint32_t ttl, const string& txt) {
        auto enc = encode_name(name);
        v.insert(v.end(), enc.begin(), enc.end());
        const uint16_t ntype = endian::host_to_network<uint16_t>(16);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&ntype), reinterpret_cast<const byte_t*>(&ntype) + 2);
        const uint16_t nclass = endian::host_to_network<uint16_t>(1);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&nclass), reinterpret_cast<const byte_t*>(&nclass) + 2);
        const uint32_t nttl = endian::host_to_network(ttl);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&nttl), reinterpret_cast<const byte_t*>(&nttl) + 4);
        const uint16_t nrdlen = endian::host_to_network<uint16_t>(1 + txt.size());
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&nrdlen), reinterpret_cast<const byte_t*>(&nrdlen) + 2);
        v.push_back(static_cast<byte_t>(txt.size()));
        v.insert(v.end(), txt.begin(), txt.end());
    }

    void append_srv_record(byte_vector& v, const string& name, const uint32_t ttl, const uint16_t priority,
                           const uint16_t weight, const uint16_t port, const string& target) {
        auto enc = encode_name(name);
        v.insert(v.end(), enc.begin(), enc.end());
        const uint16_t ntype = endian::host_to_network<uint16_t>(33);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&ntype), reinterpret_cast<const byte_t*>(&ntype) + 2);
        const uint16_t nclass = endian::host_to_network<uint16_t>(1);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&nclass), reinterpret_cast<const byte_t*>(&nclass) + 2);
        const uint32_t nttl = endian::host_to_network(ttl);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&nttl), reinterpret_cast<const byte_t*>(&nttl) + 4);
        auto tenc = encode_name(target);
        const uint16_t nrdlen = endian::host_to_network<uint16_t>(6 + tenc.size());
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&nrdlen), reinterpret_cast<const byte_t*>(&nrdlen) + 2);
        const uint16_t nprio = endian::host_to_network(priority);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&nprio), reinterpret_cast<const byte_t*>(&nprio) + 2);
        const uint16_t nweight = endian::host_to_network(weight);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&nweight), reinterpret_cast<const byte_t*>(&nweight) + 2);
        const uint16_t nport = endian::host_to_network(port);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&nport), reinterpret_cast<const byte_t*>(&nport) + 2);
        v.insert(v.end(), tenc.begin(), tenc.end());
    }

    void append_soa_record(byte_vector& v, const string& name, const uint32_t ttl, const string& mname,
                           const string& rname, const uint32_t serial, const uint32_t refresh, const uint32_t retry,
                           const uint32_t expire, const uint32_t minimum) {
        auto enc = encode_name(name);
        v.insert(v.end(), enc.begin(), enc.end());
        const uint16_t ntype = endian::host_to_network<uint16_t>(6);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&ntype), reinterpret_cast<const byte_t*>(&ntype) + 2);
        const uint16_t nclass = endian::host_to_network<uint16_t>(1);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&nclass), reinterpret_cast<const byte_t*>(&nclass) + 2);
        const uint32_t nttl = endian::host_to_network(ttl);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&nttl), reinterpret_cast<const byte_t*>(&nttl) + 4);
        auto menc = encode_name(mname);
        auto renc = encode_name(rname);
        const uint16_t nrdlen = endian::host_to_network<uint16_t>(menc.size() + renc.size() + 20);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&nrdlen), reinterpret_cast<const byte_t*>(&nrdlen) + 2);
        v.insert(v.end(), menc.begin(), menc.end());
        v.insert(v.end(), renc.begin(), renc.end());
        for (const auto val: {serial, refresh, retry, expire, minimum}) {
            const uint32_t nv = endian::host_to_network(val);
            v.insert(v.end(), reinterpret_cast<const byte_t*>(&nv), reinterpret_cast<const byte_t*>(&nv) + 4);
        }
    }

    void append_opt_record(byte_vector& v, const uint16_t udp_payload, const uint8_t ext_rcode, const uint8_t edns_ver,
                           const bool dnssec_ok) {
        v.push_back(0x00);
        const uint16_t ntype = endian::host_to_network<uint16_t>(41);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&ntype), reinterpret_cast<const byte_t*>(&ntype) + 2);
        const uint16_t npayload = endian::host_to_network(udp_payload);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&npayload), reinterpret_cast<const byte_t*>(&npayload) + 2);
        uint32_t ttl = (static_cast<uint32_t>(ext_rcode) << 24) | (static_cast<uint32_t>(edns_ver) << 16) |
                       (dnssec_ok ? 0x8000 : 0);
        ttl = endian::host_to_network(ttl);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&ttl), reinterpret_cast<const byte_t*>(&ttl) + 4);
        const uint16_t nrdlen = 0;
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&nrdlen), reinterpret_cast<const byte_t*>(&nrdlen) + 2);
    }
} // namespace

TEST(DnsMessageBuilding, QueryHeaderFormat) {
    auto q = dns_client::build_query("example.com", dns_record::A, dns_class::INTERNET, true, false);

    ASSERT_GE(q.size(), sizeof(dns_header) + 13 + 4);

    const uint16_t id = endian::network_to_host(*reinterpret_cast<const uint16_t*>(q.data()));
    EXPECT_GE(id, 1);
    EXPECT_LE(id, 65535);

    const uint16_t flags = endian::network_to_host(*reinterpret_cast<const uint16_t*>(q.data() + 2));
    EXPECT_EQ(flags & 0x8000, 0);
    EXPECT_NE(flags & 0x0100, 0);

    const uint16_t qdcount = endian::network_to_host(*reinterpret_cast<const uint16_t*>(q.data() + 4));
    EXPECT_EQ(qdcount, 1);

    const uint16_t ancount = endian::network_to_host(*reinterpret_cast<const uint16_t*>(q.data() + 6));
    const uint16_t nscount = endian::network_to_host(*reinterpret_cast<const uint16_t*>(q.data() + 8));
    EXPECT_EQ(ancount, 0);
    EXPECT_EQ(nscount, 0);
}

TEST(DnsMessageBuilding, DomainNameEncoding) {
    auto q = dns_client::build_query("www.example.com", dns_record::A, dns_class::INTERNET, true, false);

    size_t offset = sizeof(dns_header);
    EXPECT_EQ(q[offset], 3);
    EXPECT_EQ(string(reinterpret_cast<const char*>(&q[offset + 1]), 3), "www");
    offset += 4;
    EXPECT_EQ(q[offset], 7);
    EXPECT_EQ(string(reinterpret_cast<const char*>(&q[offset + 1]), 7), "example");
    offset += 8;
    EXPECT_EQ(q[offset], 3);
    EXPECT_EQ(string(reinterpret_cast<const char*>(&q[offset + 1]), 3), "com");
    offset += 4;
    EXPECT_EQ(q[offset], 0x00);
}

TEST(DnsMessageBuilding, TrailingDotIsStripped) {
    auto q1 = dns_client::build_query("example.com.", dns_record::A, dns_class::INTERNET, true, false);
    auto q2 = dns_client::build_query("example.com", dns_record::A, dns_class::INTERNET, true, false);
    EXPECT_EQ(q1.size(), q2.size());
    EXPECT_TRUE(equal(q1.begin() + 2, q1.end(), q2.begin() + 2));
}

TEST(DnsMessageBuilding, QueryWithEDNS0) {
    auto q = dns_client::build_query("example.com", dns_record::A, dns_class::INTERNET, true, true, false, 1232);

    const uint16_t arcount = endian::network_to_host(*reinterpret_cast<const uint16_t*>(q.data() + 10));
    EXPECT_EQ(arcount, 1);

    bool found_opt = false;
    for (size_t i = q.size() - 11; i < q.size() - 1; ++i) {
        const uint16_t val = endian::network_to_host(*reinterpret_cast<const uint16_t*>(&q[i]));
        if (val == 41) {
            found_opt = true;
            break;
        }
    }
    EXPECT_TRUE(found_opt);
}

TEST(DnsMessageBuilding, QueryWithoutRD) {
    auto q = dns_client::build_query("example.com", dns_record::A, dns_class::INTERNET, false, false);
    const uint16_t flags = endian::network_to_host(*reinterpret_cast<const uint16_t*>(q.data() + 2));
    EXPECT_EQ(flags & 0x0100, 0);
    EXPECT_EQ(flags & 0x8000, 0);
}

TEST(DnsMessageBuilding, QueryWithDNSsecOK) {
    auto q = dns_client::build_query("example.com", dns_record::A, dns_class::INTERNET, true, true, true, 1232);

    const uint16_t arcount = endian::network_to_host(*reinterpret_cast<const uint16_t*>(q.data() + 10));
    ASSERT_EQ(arcount, 1);

    const size_t ttl_offset = q.size() - 6;
    const uint32_t opt_ttl = endian::network_to_host(*reinterpret_cast<const uint32_t*>(&q[ttl_offset]));
    EXPECT_NE(opt_ttl & 0x8000, 0);
}

TEST(DnsMessageBuilding, DifferentRecordTypes) {
    auto q_a = dns_client::build_query("example.com", dns_record::A, dns_class::INTERNET, false, false);
    auto q_aaaa = dns_client::build_query("example.com", dns_record::AAAA, dns_class::INTERNET, false, false);
    auto q_mx = dns_client::build_query("example.com", dns_record::MX, dns_class::INTERNET, false, false);
    auto q_srv = dns_client::build_query("example.com", dns_record::SRV, dns_class::INTERNET, false, false);
    auto q_soa = dns_client::build_query("example.com", dns_record::SOA, dns_class::INTERNET, false, false);
    auto q_txt = dns_client::build_query("example.com", dns_record::TXT, dns_class::INTERNET, false, false);

    EXPECT_NE(q_a, q_aaaa);
    EXPECT_NE(q_a, q_mx);
    EXPECT_NE(q_a, q_srv);
    EXPECT_NE(q_a, q_soa);
    EXPECT_NE(q_a, q_txt);
}

TEST(DnsMessageBuilding, DifferentClasses) {
    auto q_in = dns_client::build_query("example.com", dns_record::A, dns_class::INTERNET, false, false);
    auto q_ch = dns_client::build_query("example.com", dns_record::A, dns_class::CHAOS, false, false);
    auto q_hs = dns_client::build_query("example.com", dns_record::A, dns_class::HESIOD, false, false);
    auto q_any = dns_client::build_query("example.com", dns_record::A, dns_class::ANY, false, false);

    EXPECT_NE(q_in, q_ch);
    EXPECT_NE(q_in, q_hs);
    EXPECT_NE(q_in, q_any);
}

TEST(DnsResponseParsing, BasicAResponse) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 1, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1);
    append_a_record(resp, "example.com", 300, "93.184.216.34");

    auto result = dns_client::parse_response(resp, 1234);
    EXPECT_TRUE(result.is_success());
    EXPECT_EQ(result.response_code, dns_response::NON_ERROR);
    EXPECT_TRUE(result.recursive_available);
    ASSERT_EQ(result.answers.size(), 1);
    EXPECT_EQ(result.answers[0].type, dns_record::A);
    EXPECT_EQ(result.answers[0].name, "example.com");
    EXPECT_EQ(result.answers[0].data, "93.184.216.34");
    EXPECT_EQ(result.answers[0].ttl, 300);
}

TEST(DnsResponseParsing, AAAAAResponse) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 1, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "ipv6.example.com", 28);
    append_aaaa_record(resp, "ipv6.example.com", 600, "2001:db8::1");

    auto result = dns_client::parse_response(resp, 1234);
    EXPECT_TRUE(result.is_success());
    ASSERT_EQ(result.answers.size(), 1);
    EXPECT_EQ(result.answers[0].type, dns_record::AAAA);
    EXPECT_EQ(result.answers[0].name, "ipv6.example.com");
    EXPECT_EQ(result.answers[0].data, "2001:db8::1");
    EXPECT_EQ(result.answers[0].ttl, 600);
}

TEST(DnsResponseParsing, CNAMEResponse) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 1, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "alias.example.com", 5);
    append_cname_record(resp, "alias.example.com", 300, "target.example.com");

    auto result = dns_client::parse_response(resp, 1234);
    EXPECT_TRUE(result.is_success());
    ASSERT_EQ(result.answers.size(), 1);
    EXPECT_EQ(result.answers[0].type, dns_record::CNAME);
    EXPECT_EQ(result.answers[0].data, "target.example.com");
}

TEST(DnsResponseParsing, MXResponse) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 2, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 15);
    append_mx_record(resp, "example.com", 300, 10, "mail.example.com");
    append_mx_record(resp, "example.com", 300, 20, "mail2.example.com");

    auto result = dns_client::parse_response(resp, 1234);
    EXPECT_TRUE(result.is_success());
    ASSERT_EQ(result.answers.size(), 2);
    EXPECT_EQ(result.answers[0].type, dns_record::MX);
    EXPECT_EQ(result.answers[0].data, "10 mail.example.com");
    EXPECT_EQ(result.answers[1].data, "20 mail2.example.com");
}

TEST(DnsResponseParsing, TXTResponse) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 1, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 16);
    append_txt_record(resp, "example.com", 300, "v=spf1 -all");

    auto result = dns_client::parse_response(resp, 1234);
    EXPECT_TRUE(result.is_success());
    ASSERT_EQ(result.answers.size(), 1);
    EXPECT_EQ(result.answers[0].type, dns_record::TXT);
    EXPECT_EQ(result.answers[0].data, "v=spf1 -all");
}

TEST(DnsResponseParsing, SRVResponse) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 2, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "_sip._tcp.example.com", 33);
    append_srv_record(resp, "_sip._tcp.example.com", 300, 10, 60, 5060, "sip1.example.com");
    append_srv_record(resp, "_sip._tcp.example.com", 300, 20, 40, 5060, "sip2.example.com");

    auto result = dns_client::parse_response(resp, 1234);
    EXPECT_TRUE(result.is_success());
    ASSERT_EQ(result.answers.size(), 2);
    EXPECT_EQ(result.answers[0].type, dns_record::SRV);
    EXPECT_EQ(result.answers[0].data, "10 60 5060 sip1.example.com");
    EXPECT_EQ(result.answers[1].data, "20 40 5060 sip2.example.com");
}

TEST(DnsResponseParsing, SOAResponse) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 1, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 6);
    append_soa_record(resp, "example.com", 3600, "ns1.example.com", "admin.example.com", 2024010101, 7200, 3600,
                      1209600, 86400);

    auto result = dns_client::parse_response(resp, 1234);
    EXPECT_TRUE(result.is_success());
    ASSERT_EQ(result.answers.size(), 1);
    EXPECT_EQ(result.answers[0].type, dns_record::SOA);
    EXPECT_TRUE(result.answers[0].data.starts_with("ns1.example.com"));
}

TEST(DnsResponseParsing, PTRResponse) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 1, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "1.0.0.127.in-addr.arpa", 12);
    append_cname_record(resp, "1.0.0.127.in-addr.arpa", 300, "localhost");

    auto result = dns_client::parse_response(resp, 1234);
    ASSERT_EQ(result.answers.size(), 1);
    EXPECT_EQ(result.answers[0].type, dns_record::CNAME);
}

TEST(DnsResponseParsing, NSResponse) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 1, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 2);
    append_cname_record(resp, "example.com", 86400, "ns1.example.com");

    auto result = dns_client::parse_response(resp, 1234);
    ASSERT_EQ(result.answers.size(), 1);
}

TEST(DnsHeaderFlags, QRBitMustBeSet) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x0100, 1, 0, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1);

    EXPECT_THROW({ dns_client::parse_response(resp, 1234); }, dns_exception);
}

TEST(DnsHeaderFlags, ResponseIDValidation) {
    byte_vector resp;
    auto hdr = make_header(9999, 0x8180, 1, 0, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1);

    EXPECT_THROW({ dns_client::parse_response(resp, 1234); }, dns_exception);
}

TEST(DnsHeaderFlags, ResponseIDZeroSkipsValidation) {
    byte_vector resp;
    auto hdr = make_header(5678, 0x8180, 1, 0, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1);

    EXPECT_NO_THROW({ dns_client::parse_response(resp, 0); });
}

TEST(DnsHeaderFlags, AABitExtraction) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8580, 1, 0, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1);

    auto result = dns_client::parse_response(resp, 1234);
    EXPECT_TRUE(result.authoritative);
}

TEST(DnsHeaderFlags, TCBitDetection) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8380, 1, 0, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1);

    auto result = dns_client::parse_response(resp, 1234);
    EXPECT_TRUE(result.truncated);
}

TEST(DnsHeaderFlags, RABitExtraction) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8100, 1, 0, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1);

    auto result = dns_client::parse_response(resp, 1234);
    EXPECT_FALSE(result.recursive_available);
}

TEST(DnsHeaderFlags, RCODENoError) {
    auto hdr = make_header(1234, 0x8180, 1, 0, 0, 0);
    byte_vector resp(hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1);
    auto result = dns_client::parse_response(resp, 1234);
    EXPECT_EQ(result.response_code, dns_response::NON_ERROR);
    EXPECT_TRUE(result.is_success());
}

TEST(DnsHeaderFlags, RCODEFormatError) {
    auto hdr = make_header(1234, 0x8181, 1, 0, 0, 0);
    byte_vector resp(hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1);
    auto result = dns_client::parse_response(resp, 1234);
    EXPECT_EQ(result.response_code, dns_response::FORMAT_ERROR);
    EXPECT_FALSE(result.is_success());
}

TEST(DnsHeaderFlags, RCODEServerFailure) {
    auto hdr = make_header(1234, 0x8182, 1, 0, 0, 0);
    byte_vector resp(hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1);
    auto result = dns_client::parse_response(resp, 1234);
    EXPECT_EQ(result.response_code, dns_response::SERVER_FAILURE);
}

TEST(DnsHeaderFlags, RCODENXDomain) {
    auto hdr = make_header(1234, 0x8183, 1, 0, 0, 0);
    byte_vector resp(hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1);
    auto result = dns_client::parse_response(resp, 1234);
    EXPECT_EQ(result.response_code, dns_response::NAME_ERROR);
}

TEST(DnsHeaderFlags, RCODENotImplemented) {
    auto hdr = make_header(1234, 0x8184, 1, 0, 0, 0);
    byte_vector resp(hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1);
    auto result = dns_client::parse_response(resp, 1234);
    EXPECT_EQ(result.response_code, dns_response::NOT_IMPLEMENTED);
}

TEST(DnsHeaderFlags, RCODERefused) {
    auto hdr = make_header(1234, 0x8185, 1, 0, 0, 0);
    byte_vector resp(hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1);
    auto result = dns_client::parse_response(resp, 1234);
    EXPECT_EQ(result.response_code, dns_response::REFUSED);
}

TEST(Edns0, OPTRecordParsing) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 0, 0, 1);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1);
    append_opt_record(resp, 1232, 0, 0, false);

    auto result = dns_client::parse_response(resp, 1234);
    EXPECT_EQ(result.udp_payload_size, 1232);
    EXPECT_EQ(result.extended_rcode, 0);
    EXPECT_EQ(result.edns_version, 0);
    EXPECT_FALSE(result.dnssec_ok);
}

TEST(Edns0, ExtendedRCODE) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8181, 1, 0, 0, 1);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1);
    append_opt_record(resp, 1232, 1, 0, false);

    auto result = dns_client::parse_response(resp, 1234);
    EXPECT_EQ(result.response_code, dns_response::FORMAT_ERROR);
    EXPECT_EQ(result.extended_rcode, 1);
    EXPECT_EQ(result.full_rcode(), 17);
}

TEST(Edns0, UDPPayloadSize) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 0, 0, 1);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1);
    append_opt_record(resp, 4096, 0, 0, false);

    auto result = dns_client::parse_response(resp, 1234);
    EXPECT_EQ(result.udp_payload_size, 4096);
}

TEST(Edns0, EDNSVersion) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 0, 0, 1);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1);
    append_opt_record(resp, 512, 0, 1, false);

    auto result = dns_client::parse_response(resp, 1234);
    EXPECT_EQ(result.edns_version, 1);
}

TEST(Edns0, DNSsecOK) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 0, 0, 1);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1);
    append_opt_record(resp, 1232, 0, 0, true);

    auto result = dns_client::parse_response(resp, 1234);
    EXPECT_TRUE(result.dnssec_ok);
}

TEST(Edns0, NoOPTRecordDefault) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 0, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1);

    auto result = dns_client::parse_response(resp, 1234);
    EXPECT_EQ(result.udp_payload_size, 512);
    EXPECT_EQ(result.extended_rcode, 0);
    EXPECT_EQ(result.edns_version, 0);
    EXPECT_FALSE(result.dnssec_ok);
}

TEST(Edns0, MultipleAdditionalWithOPT) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 1, 0, 2);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1);
    append_a_record(resp, "example.com", 300, "1.2.3.4");
    append_a_record(resp, "extra.example.com", 300, "5.6.7.8");
    append_opt_record(resp, 4096, 0, 0, false);

    auto result = dns_client::parse_response(resp, 1234);
    ASSERT_EQ(result.answers.size(), 1);
    EXPECT_EQ(result.answers[0].data, "1.2.3.4");
    EXPECT_EQ(result.additional.size(), 1);
    EXPECT_EQ(result.additional[0].data, "5.6.7.8");
    EXPECT_EQ(result.udp_payload_size, 4096);
}

TEST(DomainNameCompression, SimplePointer) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 2, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());

    append_question(resp, "example.com", 1);

    append_a_record(resp, "example.com", 300, "93.184.216.34");

    resp.push_back(0xC0);
    resp.push_back(12);
    const uint16_t ntype = endian::host_to_network<uint16_t>(1);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&ntype), reinterpret_cast<const byte_t*>(&ntype) + 2);
    const uint16_t nclass = endian::host_to_network<uint16_t>(1);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nclass), reinterpret_cast<const byte_t*>(&nclass) + 2);
    const uint32_t nttl = endian::host_to_network<uint32_t>(300);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nttl), reinterpret_cast<const byte_t*>(&nttl) + 4);
    const uint16_t nrdlen = endian::host_to_network<uint16_t>(4);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nrdlen), reinterpret_cast<const byte_t*>(&nrdlen) + 2);
    in_addr addr2;
    ::inet_pton(AF_INET, "1.2.3.4", &addr2);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&addr2), reinterpret_cast<const byte_t*>(&addr2) + 4);

    auto result = dns_client::parse_response(resp, 1234);
    ASSERT_EQ(result.answers.size(), 2);
    EXPECT_EQ(result.answers[0].name, "example.com");
    EXPECT_EQ(result.answers[1].name, "example.com");
}

TEST(ErrorHandling, ResponseTooShort) {
    byte_vector resp(5, 0);
    EXPECT_THROW({ dns_client::parse_response(resp, 0); }, dns_exception);
}

TEST(ErrorHandling, IncompleteHeader) {
    byte_vector resp(11, 0);
    EXPECT_THROW({ dns_client::parse_response(resp, 0); }, dns_exception);
}

TEST(ErrorHandling, QuestionSectionExceedsBuffer) {
    auto hdr = make_header(1234, 0x8180, 1, 0, 0, 0);
    byte_vector resp(hdr.begin(), hdr.end());
    EXPECT_THROW({ dns_client::parse_response(resp, 1234); }, dns_exception);
}

TEST(ErrorHandling, UnknownRecordType) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 1, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1);

    auto enc = encode_name("example.com");
    resp.insert(resp.end(), enc.begin(), enc.end());
    const uint16_t ntype = endian::host_to_network<uint16_t>(100);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&ntype), reinterpret_cast<const byte_t*>(&ntype) + 2);
    const uint16_t nclass = endian::host_to_network<uint16_t>(1);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nclass), reinterpret_cast<const byte_t*>(&nclass) + 2);
    const uint32_t nttl = endian::host_to_network<uint32_t>(300);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nttl), reinterpret_cast<const byte_t*>(&nttl) + 4);
    const uint16_t nrdlen = endian::host_to_network<uint16_t>(4);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nrdlen), reinterpret_cast<const byte_t*>(&nrdlen) + 2);
    resp.push_back(0x01);
    resp.push_back(0x02);
    resp.push_back(0x03);
    resp.push_back(0x04);

    auto result = dns_client::parse_response(resp, 1234);
    ASSERT_EQ(result.answers.size(), 1);
    EXPECT_FALSE(result.answers[0].data.empty());
}

TEST(SectionParsing, AuthoritySection) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 1, 1, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1);
    append_a_record(resp, "example.com", 300, "93.184.216.34");
    append_cname_record(resp, "example.com", 86400, "ns1.example.com");

    auto result = dns_client::parse_response(resp, 1234);
    EXPECT_EQ(result.answers.size(), 1);
    EXPECT_EQ(result.authorities.size(), 1);
}

TEST(SectionParsing, MultipleAnswers) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 3, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1);
    append_a_record(resp, "example.com", 300, "1.1.1.1");
    append_a_record(resp, "example.com", 300, "2.2.2.2");
    append_a_record(resp, "example.com", 300, "3.3.3.3");

    auto result = dns_client::parse_response(resp, 1234);
    ASSERT_EQ(result.answers.size(), 3);
    EXPECT_EQ(result.answers[0].data, "1.1.1.1");
    EXPECT_EQ(result.answers[1].data, "2.2.2.2");
    EXPECT_EQ(result.answers[2].data, "3.3.3.3");
}

TEST(SectionParsing, EmptyResponseNoError) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8183, 1, 0, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "nonexistent.example.com", 1);

    auto result = dns_client::parse_response(resp, 1234);
    EXPECT_EQ(result.response_code, dns_response::NAME_ERROR);
    EXPECT_TRUE(result.answers.empty());
    EXPECT_FALSE(result.is_success());
}

TEST(FullRcode, BaseRcodeOnly) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8183, 1, 0, 0, 1);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1);
    append_opt_record(resp, 1232, 0, 0, false);

    auto result = dns_client::parse_response(resp, 1234);
    EXPECT_EQ(result.full_rcode(), 3);
}

TEST(FullRcode, ExtendedRcodeBADVERS) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 0, 0, 1);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1);
    append_opt_record(resp, 1232, 1, 0, false);

    auto result = dns_client::parse_response(resp, 1234);
    EXPECT_EQ(result.full_rcode(), 16);
}

TEST(ClassField, InternetClass) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 1, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1, 1);
    append_a_record(resp, "example.com", 300, "1.2.3.4");

    auto result = dns_client::parse_response(resp, 1234);
    ASSERT_EQ(result.answers.size(), 1);
    EXPECT_EQ(static_cast<uint16_t>(result.answers[0].class_type), 1);
}

TEST(ClassField, ANYClass) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 1, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1, 255);
    append_a_record(resp, "example.com", 300, "1.2.3.4");

    auto result = dns_client::parse_response(resp, 1234);
    ASSERT_EQ(result.answers.size(), 1);
    EXPECT_EQ(static_cast<uint16_t>(result.answers[0].class_type), 1);
}

TEST(DnsQueryResult, DefaultConstruction) {
    dns_query_result r;
    EXPECT_EQ(r.response_code, dns_response::NON_ERROR);
    EXPECT_TRUE(r.is_success());
    EXPECT_FALSE(r.authoritative);
    EXPECT_FALSE(r.truncated);
    EXPECT_FALSE(r.recursive_available);
    EXPECT_TRUE(r.answers.empty());
    EXPECT_TRUE(r.authorities.empty());
    EXPECT_TRUE(r.additional.empty());
}

TEST(DnsQueryResult, NonSuccessCheck) {
    dns_query_result r;
    r.response_code = dns_response::SERVER_FAILURE;
    EXPECT_FALSE(r.is_success());
}

TEST(DnsRecord, DefaultConstruction) {
    dns_record r;
    EXPECT_EQ(r.type, dns_record::A);
    EXPECT_TRUE(r.name.empty());
    EXPECT_TRUE(r.data.empty());
}

TEST(DnsRecord, ValueConstruction) {
    dns_record r("example.com", dns_record::AAAA, dns_class::INTERNET, 600, "2001:db8::1");
    EXPECT_EQ(r.name, "example.com");
    EXPECT_EQ(r.type, dns_record::AAAA);
    EXPECT_EQ(r.class_type, dns_class::INTERNET);
    EXPECT_EQ(r.ttl, 600);
    EXPECT_EQ(r.data, "2001:db8::1");
}

TEST(EnumValues, DnsRecordTypes) {
    EXPECT_EQ(static_cast<uint16_t>(dns_record::A), 1);
    EXPECT_EQ(static_cast<uint16_t>(dns_record::NS), 2);
    EXPECT_EQ(static_cast<uint16_t>(dns_record::CNAME), 5);
    EXPECT_EQ(static_cast<uint16_t>(dns_record::SOA), 6);
    EXPECT_EQ(static_cast<uint16_t>(dns_record::PTR), 12);
    EXPECT_EQ(static_cast<uint16_t>(dns_record::MX), 15);
    EXPECT_EQ(static_cast<uint16_t>(dns_record::TXT), 16);
    EXPECT_EQ(static_cast<uint16_t>(dns_record::AAAA), 28);
    EXPECT_EQ(static_cast<uint16_t>(dns_record::SRV), 33);
}

TEST(EnumValues, DnsClass) {
    EXPECT_EQ(static_cast<uint16_t>(dns_class::INTERNET), 1);
    EXPECT_EQ(static_cast<uint16_t>(dns_class::CHAOS), 3);
    EXPECT_EQ(static_cast<uint16_t>(dns_class::HESIOD), 4);
    EXPECT_EQ(static_cast<uint16_t>(dns_class::ANY), 255);
}

TEST(EnumValues, DnsResponseCodes) {
    EXPECT_EQ(static_cast<uint8_t>(dns_response::NON_ERROR), 0);
    EXPECT_EQ(static_cast<uint8_t>(dns_response::FORMAT_ERROR), 1);
    EXPECT_EQ(static_cast<uint8_t>(dns_response::SERVER_FAILURE), 2);
    EXPECT_EQ(static_cast<uint8_t>(dns_response::NAME_ERROR), 3);
    EXPECT_EQ(static_cast<uint8_t>(dns_response::NOT_IMPLEMENTED), 4);
    EXPECT_EQ(static_cast<uint8_t>(dns_response::REFUSED), 5);
}

TEST(EnumValues, DnsOpcode) {
    EXPECT_EQ(static_cast<uint8_t>(dns_opcode::QUERY), 0);
    EXPECT_EQ(static_cast<uint8_t>(dns_opcode::IQUERY), 1);
    EXPECT_EQ(static_cast<uint8_t>(dns_opcode::STATUS), 2);
    EXPECT_EQ(static_cast<uint8_t>(dns_opcode::NOTIFY), 4);
    EXPECT_EQ(static_cast<uint8_t>(dns_opcode::UPDATE), 5);
}

TEST(DnsSrvRecord, DefaultConstruction) {
    dns_srv_record srv;
    EXPECT_EQ(srv.priority, 0);
    EXPECT_EQ(srv.weight, 0);
    EXPECT_EQ(srv.port, 0);
    EXPECT_TRUE(srv.target.empty());
}

TEST(DnsSrvRecord, ValueConstruction) {
    dns_srv_record srv(10, 60, 5060, "sip.example.com");
    EXPECT_EQ(srv.priority, 10);
    EXPECT_EQ(srv.weight, 60);
    EXPECT_EQ(srv.port, 5060);
    EXPECT_EQ(srv.target, "sip.example.com");
}

TEST(DnsSoaRecord, DefaultConstruction) {
    dns_soa_record soa;
    EXPECT_EQ(soa.serial, 0);
    EXPECT_EQ(soa.refresh, 0);
    EXPECT_EQ(soa.retry, 0);
    EXPECT_EQ(soa.expire, 0);
    EXPECT_EQ(soa.minimum, 0);
    EXPECT_TRUE(soa.mname.empty());
    EXPECT_TRUE(soa.rname.empty());
}

TEST(EdnsConstants, StandardValues) {
    EXPECT_EQ(edns::OPT_TYPE, 41);
    EXPECT_EQ(edns::DEFAULT_UDP_PAYLOAD, 1232);
    EXPECT_EQ(edns::MAX_UDP_PAYLOAD, 4096);
    EXPECT_EQ(edns::VERSION, 0);
    EXPECT_EQ(edns::DO_BIT, 0x8000);
    EXPECT_EQ(edns::NEGATIVE_CACHE_TTL, seconds(30));
    EXPECT_EQ(edns::MAX_UDP_RETRIES, 2);
}

TEST(DnsException, TimeoutCreation) {
    auto e = dns_exception::timeout();
    EXPECT_NE(string(e.what()).find("timeout"), string::npos);
}

TEST(DnsException, NetworkErrorCreation) {
    auto e = dns_exception::network_error("connection refused");
    EXPECT_NE(string(e.what()).find("Network error"), string::npos);
    EXPECT_NE(string(e.what()).find("connection refused"), string::npos);
}

TEST(DnsException, ParseErrorCreation) {
    auto e = dns_exception::parse_error("invalid format");
    EXPECT_NE(string(e.what()).find("Parse error"), string::npos);
    EXPECT_NE(string(e.what()).find("invalid format"), string::npos);
}

TEST(DnsClientConfig, DefaultConfiguration) {
    dns_client::config cfg;
    EXPECT_EQ(cfg.server, "8.8.8.8");
    EXPECT_EQ(cfg.port, ports::DNS);
    EXPECT_EQ(cfg.timeout, milliseconds(5000));
}

TEST(DnsClient, ConstructWithValidConfig) {
    io_context ctx;
    dns_client::config cfg;
    cfg.server = "1.1.1.1";
    cfg.timeout = milliseconds(3000);
    EXPECT_NO_THROW({ dns_client client(cfg, ctx); });
}

TEST(DnsClient, ConstructWithEmptyServerThrows) {
    io_context ctx;
    dns_client::config cfg;
    cfg.server = "";
    EXPECT_THROW({ dns_client client(cfg, ctx); }, dns_exception);
}

TEST(DnsClient, ConstructWithZeroTimeoutThrows) {
    io_context ctx;
    dns_client::config cfg;
    cfg.timeout = milliseconds(0);
    EXPECT_THROW({ dns_client client(cfg, ctx); }, dns_exception);
}

TEST(DnsClient, ConstructWithNegativeTimeoutThrows) {
    io_context ctx;
    dns_client::config cfg;
    cfg.timeout = milliseconds(-1);
    EXPECT_THROW({ dns_client client(cfg, ctx); }, dns_exception);
}

TEST(DnsClient, DefaultConstructorDoesNotThrow) {
    EXPECT_NO_THROW({ dns_client client; });
}

TEST(DnsClient, ConstructWithTCPFlag) {
    io_context ctx;
    dns_client::config cfg;
    EXPECT_NO_THROW({ dns_client client(cfg, ctx, true); });
    EXPECT_NO_THROW({ dns_client client(cfg, ctx, false); });
}

TEST(DnsClient, SetConfig) {
    dns_client client;
    dns_client::config cfg;
    cfg.server = "9.9.9.9";
    cfg.timeout = milliseconds(2000);
    EXPECT_NO_THROW({ client.set_config(cfg); });
}

TEST(DnsClient, SetTimeout) {
    dns_client client;
    EXPECT_NO_THROW({ client.set_timeout(milliseconds(1000)); });
    EXPECT_NO_THROW({ client.set_timeout(milliseconds(10000)); });
}

TEST(DnsClient, SetUseTCP) {
    dns_client client;
    client.set_use_tcp(true);
    client.set_use_tcp(false);
}

TEST(DnsClient, SetCacheTTL) {
    dns_client client;
    client.set_cache_ttl(seconds(60));
    client.set_cache_ttl(seconds(600));
    client.set_cache_ttl(seconds(0));
}

TEST(DnsClient, SetRecursionDesired) {
    dns_client client;
    client.set_recursion_desired(true);
    client.set_recursion_desired(false);
}

TEST(DnsClient, SetEDNSUDPPayload) {
    dns_client client;
    client.set_edns_udp_payload(512);
    client.set_edns_udp_payload(1232);
    client.set_edns_udp_payload(4096);
}

TEST(DnsClient, SetDNSsecOK) {
    dns_client client;
    client.set_dnssec_ok(true);
    client.set_dnssec_ok(false);
}

TEST(DnsClient, ClearCache) {
    dns_client client;
    EXPECT_NO_THROW({ client.clear_cache(); });
}

TEST(DnsMessageBuilding, CustomEDNSPayloadSize) {
    auto q512 = dns_client::build_query("example.com", dns_record::A, dns_class::INTERNET, true, true, false, 512);
    auto q1232 = dns_client::build_query("example.com", dns_record::A, dns_class::INTERNET, true, true, false, 1232);
    auto q4096 = dns_client::build_query("example.com", dns_record::A, dns_class::INTERNET, true, true, false, 4096);

    for (const auto& q: {q512, q1232, q4096}) {
        const uint16_t arcount = endian::network_to_host(*reinterpret_cast<const uint16_t*>(q.data() + 10));
        EXPECT_EQ(arcount, 1);
    }

    EXPECT_NE(q512, q1232);
    EXPECT_NE(q1232, q4096);
}

TEST(DnsMessageBuilding, DNSSECFlagInQuery) {
    auto q_with = dns_client::build_query("example.com", dns_record::A, dns_class::INTERNET, true, true, true, 1232);
    auto q_without =
            dns_client::build_query("example.com", dns_record::A, dns_class::INTERNET, true, true, false, 1232);
    EXPECT_NE(q_with, q_without);
}

TEST(DnsMessageBuilding, EDNSDisabledARCountZero) {
    auto q = dns_client::build_query("example.com", dns_record::A, dns_class::INTERNET, true, false);
    const uint16_t arcount = endian::network_to_host(*reinterpret_cast<const uint16_t*>(q.data() + 10));
    EXPECT_EQ(arcount, 0);
}

TEST(DnsMessageBuilding, QueryForAllRecordTypes) {
    const vector<pair<dns_record::raw, string>> types = {
            {dns_record::A, "A"},     {dns_record::NS, "NS"},     {dns_record::CNAME, "CNAME"},
            {dns_record::SOA, "SOA"}, {dns_record::PTR, "PTR"},   {dns_record::MX, "MX"},
            {dns_record::TXT, "TXT"}, {dns_record::AAAA, "AAAA"}, {dns_record::SRV, "SRV"},
    };

    for (const auto& [type, name]: types) {
        auto q = dns_client::build_query("test.example.com", type, dns_class::INTERNET, true, false);
        ASSERT_GE(q.size(), sizeof(dns_header) + 18 + 4) << "Failed for type: " << name.data();
    }
}

TEST(DnsMessageBuilding, QueryIDIsRandom) {
    auto q1 = dns_client::build_query("example.com");
    auto q2 = dns_client::build_query("example.com");
    auto q3 = dns_client::build_query("example.com");
    const uint16_t id1 = endian::network_to_host(*reinterpret_cast<const uint16_t*>(q1.data()));
    const uint16_t id2 = endian::network_to_host(*reinterpret_cast<const uint16_t*>(q2.data()));
    const uint16_t id3 = endian::network_to_host(*reinterpret_cast<const uint16_t*>(q3.data()));
    EXPECT_TRUE(id1 != id2 || id2 != id3 || id1 != id3);
}

TEST(ParseEdgeCases, InvalidARecordLength) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 1, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1);

    auto enc = encode_name("example.com");
    resp.insert(resp.end(), enc.begin(), enc.end());
    const uint16_t ntype = endian::host_to_network<uint16_t>(1);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&ntype), reinterpret_cast<const byte_t*>(&ntype) + 2);
    const uint16_t nclass = endian::host_to_network<uint16_t>(1);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nclass), reinterpret_cast<const byte_t*>(&nclass) + 2);
    const uint32_t nttl = endian::host_to_network<uint32_t>(300);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nttl), reinterpret_cast<const byte_t*>(&nttl) + 4);
    const uint16_t nrdlen = endian::host_to_network<uint16_t>(3);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nrdlen), reinterpret_cast<const byte_t*>(&nrdlen) + 2);
    resp.push_back(0x01);
    resp.push_back(0x02);
    resp.push_back(0x03);

    EXPECT_THROW({ dns_client::parse_response(resp, 1234); }, dns_exception);
}

TEST(ParseEdgeCases, InvalidAAAARecordLength) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 1, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 28);

    auto enc = encode_name("example.com");
    resp.insert(resp.end(), enc.begin(), enc.end());
    const uint16_t ntype = endian::host_to_network<uint16_t>(28);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&ntype), reinterpret_cast<const byte_t*>(&ntype) + 2);
    const uint16_t nclass = endian::host_to_network<uint16_t>(1);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nclass), reinterpret_cast<const byte_t*>(&nclass) + 2);
    const uint32_t nttl = endian::host_to_network<uint32_t>(300);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nttl), reinterpret_cast<const byte_t*>(&nttl) + 4);
    const uint16_t nrdlen = endian::host_to_network<uint16_t>(4);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nrdlen), reinterpret_cast<const byte_t*>(&nrdlen) + 2);
    resp.push_back(0x01);
    resp.push_back(0x02);
    resp.push_back(0x03);
    resp.push_back(0x04);

    EXPECT_THROW({ dns_client::parse_response(resp, 1234); }, dns_exception);
}

TEST(ParseEdgeCases, InvalidMXRecordLength) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 1, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 15);

    auto enc = encode_name("example.com");
    resp.insert(resp.end(), enc.begin(), enc.end());
    const uint16_t ntype = endian::host_to_network<uint16_t>(15);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&ntype), reinterpret_cast<const byte_t*>(&ntype) + 2);
    const uint16_t nclass = endian::host_to_network<uint16_t>(1);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nclass), reinterpret_cast<const byte_t*>(&nclass) + 2);
    const uint32_t nttl = endian::host_to_network<uint32_t>(300);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nttl), reinterpret_cast<const byte_t*>(&nttl) + 4);
    const uint16_t nrdlen = endian::host_to_network<uint16_t>(1);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nrdlen), reinterpret_cast<const byte_t*>(&nrdlen) + 2);
    resp.push_back(0x00);

    EXPECT_THROW({ dns_client::parse_response(resp, 1234); }, dns_exception);
}

TEST(ParseEdgeCases, InvalidSRVRecordLength) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 1, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "_sip._tcp.example.com", 33);

    auto enc = encode_name("_sip._tcp.example.com");
    resp.insert(resp.end(), enc.begin(), enc.end());
    const uint16_t ntype = endian::host_to_network<uint16_t>(33);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&ntype), reinterpret_cast<const byte_t*>(&ntype) + 2);
    const uint16_t nclass = endian::host_to_network<uint16_t>(1);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nclass), reinterpret_cast<const byte_t*>(&nclass) + 2);
    const uint32_t nttl = endian::host_to_network<uint32_t>(300);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nttl), reinterpret_cast<const byte_t*>(&nttl) + 4);
    const uint16_t nrdlen = endian::host_to_network<uint16_t>(5);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nrdlen), reinterpret_cast<const byte_t*>(&nrdlen) + 2);
    for (int i = 0; i < 5; ++i) {
        resp.push_back(0x00);
    }

    EXPECT_THROW({ dns_client::parse_response(resp, 1234); }, dns_exception);
}

TEST(ParseEdgeCases, InvalidSOARecordLength) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 1, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 6);

    auto enc = encode_name("example.com");
    resp.insert(resp.end(), enc.begin(), enc.end());
    const uint16_t ntype = endian::host_to_network<uint16_t>(6);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&ntype), reinterpret_cast<const byte_t*>(&ntype) + 2);
    const uint16_t nclass = endian::host_to_network<uint16_t>(1);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nclass), reinterpret_cast<const byte_t*>(&nclass) + 2);
    const uint32_t nttl = endian::host_to_network<uint32_t>(300);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nttl), reinterpret_cast<const byte_t*>(&nttl) + 4);
    const uint16_t nrdlen = endian::host_to_network<uint16_t>(5);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nrdlen), reinterpret_cast<const byte_t*>(&nrdlen) + 2);
    for (int i = 0; i < 5; ++i) {
        resp.push_back(0x00);
    }

    EXPECT_THROW({ dns_client::parse_response(resp, 1234); }, dns_exception);
}

TEST(ParseEdgeCases, InvalidTXTRecordLength) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 1, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 16);

    auto enc = encode_name("example.com");
    resp.insert(resp.end(), enc.begin(), enc.end());
    const uint16_t ntype = endian::host_to_network<uint16_t>(16);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&ntype), reinterpret_cast<const byte_t*>(&ntype) + 2);
    const uint16_t nclass = endian::host_to_network<uint16_t>(1);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nclass), reinterpret_cast<const byte_t*>(&nclass) + 2);
    const uint32_t nttl = endian::host_to_network<uint32_t>(300);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nttl), reinterpret_cast<const byte_t*>(&nttl) + 4);
    const uint16_t nrdlen = endian::host_to_network<uint16_t>(5);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nrdlen), reinterpret_cast<const byte_t*>(&nrdlen) + 2);
    resp.push_back(10);
    resp.push_back('A');
    resp.push_back('B');
    resp.push_back('C');
    resp.push_back('D');

    EXPECT_THROW({ dns_client::parse_response(resp, 1234); }, dns_exception);
}

TEST(ParseEdgeCases, RDATAExceedsBuffer) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 1, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1);

    auto enc = encode_name("example.com");
    resp.insert(resp.end(), enc.begin(), enc.end());
    const uint16_t ntype = endian::host_to_network<uint16_t>(1);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&ntype), reinterpret_cast<const byte_t*>(&ntype) + 2);
    const uint16_t nclass = endian::host_to_network<uint16_t>(1);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nclass), reinterpret_cast<const byte_t*>(&nclass) + 2);
    const uint32_t nttl = endian::host_to_network<uint32_t>(300);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nttl), reinterpret_cast<const byte_t*>(&nttl) + 4);
    const uint16_t nrdlen = endian::host_to_network<uint16_t>(100);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nrdlen), reinterpret_cast<const byte_t*>(&nrdlen) + 2);
    resp.push_back(0x00);

    EXPECT_THROW({ dns_client::parse_response(resp, 1234); }, dns_exception);
}

TEST(ParseEdgeCases, DomainNamePointerExceedsBuffer) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 1, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 5);

    auto enc = encode_name("alias.example.com");
    resp.insert(resp.end(), enc.begin(), enc.end());
    const uint16_t ntype = endian::host_to_network<uint16_t>(5);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&ntype), reinterpret_cast<const byte_t*>(&ntype) + 2);
    const uint16_t nclass = endian::host_to_network<uint16_t>(1);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nclass), reinterpret_cast<const byte_t*>(&nclass) + 2);
    const uint32_t nttl = endian::host_to_network<uint32_t>(300);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nttl), reinterpret_cast<const byte_t*>(&nttl) + 4);
    const uint16_t nrdlen = endian::host_to_network<uint16_t>(2);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nrdlen), reinterpret_cast<const byte_t*>(&nrdlen) + 2);
    resp.push_back(0xFF);
    resp.push_back(0xFF);

    EXPECT_THROW({ dns_client::parse_response(resp, 1234); }, dns_exception);
}

TEST(ParseEdgeCases, TooManyCompressionJumps) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 1, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1);

    uint16_t ptr_offset = resp.size();
    resp.push_back(0xC0);
    resp.push_back(static_cast<byte_t>(ptr_offset & 0xFF));
    const uint16_t ntype = endian::host_to_network<uint16_t>(1);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&ntype), reinterpret_cast<const byte_t*>(&ntype) + 2);
    const uint16_t nclass = endian::host_to_network<uint16_t>(1);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nclass), reinterpret_cast<const byte_t*>(&nclass) + 2);
    const uint32_t nttl = endian::host_to_network<uint32_t>(300);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nttl), reinterpret_cast<const byte_t*>(&nttl) + 4);
    const uint16_t nrdlen = endian::host_to_network<uint16_t>(4);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nrdlen), reinterpret_cast<const byte_t*>(&nrdlen) + 2);
    uint32_t ip = 0x01020304;
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&ip), reinterpret_cast<const byte_t*>(&ip) + 4);

    EXPECT_THROW({ dns_client::parse_response(resp, 1234); }, dns_exception);
}

TEST(ParseEdgeCases, OPTRecordNonRootName) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 0, 0, 1);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1);

    resp.push_back(0x03);
    resp.push_back('f');
    resp.push_back('o');
    resp.push_back('o');
    resp.push_back(0x00);
    const uint16_t ntype = endian::host_to_network<uint16_t>(41);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&ntype), reinterpret_cast<const byte_t*>(&ntype) + 2);
    const uint16_t npayload = endian::host_to_network<uint16_t>(1232);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&npayload), reinterpret_cast<const byte_t*>(&npayload) + 2);
    const uint32_t nttl = 0;
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nttl), reinterpret_cast<const byte_t*>(&nttl) + 4);
    const uint16_t nrdlen = 0;
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nrdlen), reinterpret_cast<const byte_t*>(&nrdlen) + 2);

    EXPECT_THROW({ dns_client::parse_response(resp, 1234); }, dns_exception);
}

TEST(ParseEdgeCases, OPTRecordWrongType) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 0, 0, 1);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1);

    resp.push_back(0x00);
    const uint16_t ntype = endian::host_to_network<uint16_t>(1);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&ntype), reinterpret_cast<const byte_t*>(&ntype) + 2);
    const uint16_t npayload = endian::host_to_network<uint16_t>(1232);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&npayload), reinterpret_cast<const byte_t*>(&npayload) + 2);
    const uint32_t nttl = 0;
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nttl), reinterpret_cast<const byte_t*>(&nttl) + 4);
    const uint16_t nrdlen = 0;
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nrdlen), reinterpret_cast<const byte_t*>(&nrdlen) + 2);

    EXPECT_THROW({ dns_client::parse_response(resp, 1234); }, dns_exception);
}

TEST(ParseEdgeCases, IncompleteResourceRecord) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 1, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1);

    auto enc = encode_name("example.com");
    resp.insert(resp.end(), enc.begin(), enc.end());
    const uint16_t ntype = endian::host_to_network<uint16_t>(1);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&ntype), reinterpret_cast<const byte_t*>(&ntype) + 2);

    EXPECT_THROW({ dns_client::parse_response(resp, 1234); }, dns_exception);
}

TEST(ParseEdgeCases, IncompleteOPTRecord) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 0, 0, 1);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1);

    resp.push_back(0x00);
    const uint16_t ntype = endian::host_to_network<uint16_t>(41);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&ntype), reinterpret_cast<const byte_t*>(&ntype) + 2);

    EXPECT_THROW({ dns_client::parse_response(resp, 1234); }, dns_exception);
}

TEST(AdvancedParsing, MixedAAndCNAMEResponse) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 2, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "www.example.com", 1);
    append_cname_record(resp, "www.example.com", 300, "server.example.com");
    append_a_record(resp, "server.example.com", 300, "1.2.3.4");

    auto result = dns_client::parse_response(resp, 1234);
    ASSERT_EQ(result.answers.size(), 2);
    EXPECT_EQ(result.answers[0].type, dns_record::CNAME);
    EXPECT_EQ(result.answers[0].data, "server.example.com");
    EXPECT_EQ(result.answers[1].type, dns_record::A);
    EXPECT_EQ(result.answers[1].data, "1.2.3.4");
}

TEST(AdvancedParsing, MultiLabelDomain) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 1, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "deep.sub.domain.example.com", 1);
    append_a_record(resp, "deep.sub.domain.example.com", 300, "10.0.0.1");

    auto result = dns_client::parse_response(resp, 1234);
    ASSERT_EQ(result.answers.size(), 1);
    EXPECT_EQ(result.answers[0].name, "deep.sub.domain.example.com");
    EXPECT_EQ(result.answers[0].data, "10.0.0.1");
}

TEST(AdvancedParsing, MXWithHighestPreference) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 2, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 15);
    append_mx_record(resp, "example.com", 300, 65535, "backup.example.com");
    append_mx_record(resp, "example.com", 300, 0, "primary.example.com");

    auto result = dns_client::parse_response(resp, 1234);
    ASSERT_EQ(result.answers.size(), 2);
    EXPECT_EQ(result.answers[0].data, "65535 backup.example.com");
    EXPECT_EQ(result.answers[1].data, "0 primary.example.com");
}

TEST(AdvancedParsing, TXTMultiString) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 1, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 16);

    auto enc = encode_name("example.com");
    resp.insert(resp.end(), enc.begin(), enc.end());
    const uint16_t ntype = endian::host_to_network<uint16_t>(16);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&ntype), reinterpret_cast<const byte_t*>(&ntype) + 2);
    const uint16_t nclass = endian::host_to_network<uint16_t>(1);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nclass), reinterpret_cast<const byte_t*>(&nclass) + 2);
    const uint32_t nttl = endian::host_to_network<uint32_t>(300);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nttl), reinterpret_cast<const byte_t*>(&nttl) + 4);
    const uint16_t nrdlen = endian::host_to_network<uint16_t>(8);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nrdlen), reinterpret_cast<const byte_t*>(&nrdlen) + 2);
    resp.push_back(3);
    resp.push_back('a');
    resp.push_back('=');
    resp.push_back('b');
    resp.push_back(3);
    resp.push_back('c');
    resp.push_back('=');
    resp.push_back('d');

    auto result = dns_client::parse_response(resp, 1234);
    ASSERT_EQ(result.answers.size(), 1);
    EXPECT_EQ(result.answers[0].type, dns_record::TXT);
    EXPECT_EQ(result.answers[0].data, "a=bc=d");
}

TEST(AdvancedParsing, ResponseWithAllSections) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 1, 1, 1);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1);
    append_a_record(resp, "example.com", 300, "93.184.216.34");
    append_cname_record(resp, "example.com", 86400, "ns1.iana-servers.net");
    append_a_record(resp, "ns1.iana-servers.net", 86400, "192.0.2.1");

    auto result = dns_client::parse_response(resp, 1234);
    EXPECT_EQ(result.answers.size(), 1);
    EXPECT_EQ(result.authorities.size(), 1);
    EXPECT_EQ(result.additional.size(), 1);
    EXPECT_EQ(result.answers[0].data, "93.184.216.34");
}

TEST(AdvancedParsing, ResponseWithCompressedAuthorities) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 1, 1, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1);
    append_a_record(resp, "example.com", 300, "1.2.3.4");

    resp.push_back(0xC0);
    resp.push_back(12);
    const uint16_t ntype = endian::host_to_network<uint16_t>(2);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&ntype), reinterpret_cast<const byte_t*>(&ntype) + 2);
    const uint16_t nclass = endian::host_to_network<uint16_t>(1);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nclass), reinterpret_cast<const byte_t*>(&nclass) + 2);
    const uint32_t nttl = endian::host_to_network<uint32_t>(86400);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nttl), reinterpret_cast<const byte_t*>(&nttl) + 4);
    auto ns_enc = encode_name("ns1.example.com");
    const uint16_t nrdlen = endian::host_to_network<uint16_t>(ns_enc.size());
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nrdlen), reinterpret_cast<const byte_t*>(&nrdlen) + 2);
    resp.insert(resp.end(), ns_enc.begin(), ns_enc.end());

    auto result = dns_client::parse_response(resp, 1234);
    ASSERT_EQ(result.authorities.size(), 1);
    EXPECT_EQ(result.authorities[0].name, "example.com");
}

TEST(AdvancedParsing, OPTWithRDATA) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 0, 0, 1);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "example.com", 1);

    resp.push_back(0x00);
    const uint16_t ntype = endian::host_to_network<uint16_t>(41);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&ntype), reinterpret_cast<const byte_t*>(&ntype) + 2);
    const uint16_t npayload = endian::host_to_network<uint16_t>(1232);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&npayload), reinterpret_cast<const byte_t*>(&npayload) + 2);
    const uint32_t nttl = endian::host_to_network<uint32_t>(0x8000);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nttl), reinterpret_cast<const byte_t*>(&nttl) + 4);
    const uint16_t nrdlen = endian::host_to_network<uint16_t>(4);
    resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nrdlen), reinterpret_cast<const byte_t*>(&nrdlen) + 2);
    resp.push_back(0xAA);
    resp.push_back(0xBB);
    resp.push_back(0xCC);
    resp.push_back(0xDD);

    auto result = dns_client::parse_response(resp, 1234);
    EXPECT_EQ(result.udp_payload_size, 1232);
    EXPECT_TRUE(result.dnssec_ok);
}

TEST(DnsQueryResult, DefaultUDPPayloadSize) {
    dns_query_result r;
    EXPECT_EQ(r.udp_payload_size, 512);
}

TEST(DnsQueryResult, FullRcodeWithOnlyBaseRcode) {
    dns_query_result r;
    r.response_code = dns_response::REFUSED;
    r.extended_rcode = 0;
    EXPECT_EQ(r.full_rcode(), 5);
}

TEST(DnsQueryResult, FullRcodeWithExtendedBits) {
    dns_query_result r;
    r.response_code = dns_response::NON_ERROR;
    r.extended_rcode = 0x0F;
    EXPECT_EQ(r.full_rcode(), 0xF0);
}

TEST(DnsQueryResult, QueryTimeDefault) {
    dns_query_result r;
    EXPECT_EQ(r.query_time, milliseconds(0));
}

TEST(DnsMessageBuilding, CasePatternApplied) {
    auto q = dns_client::build_query("example.com", dns_record::A, dns_class::INTERNET, true, false, false,
                                     edns::DEFAULT_UDP_PAYLOAD, "ExAmPlE.CoM");

    size_t offset = sizeof(dns_header);
    EXPECT_EQ(q[offset], 7);
    EXPECT_EQ(string(reinterpret_cast<const char*>(&q[offset + 1]), 7), "ExAmPlE");
    offset += 8;
    EXPECT_EQ(q[offset], 3);
    EXPECT_EQ(string(reinterpret_cast<const char*>(&q[offset + 1]), 3), "CoM");
    EXPECT_EQ(q[offset + 4], 0x00);
}

TEST(DnsMessageBuilding, CasePatternDoesNotChangeQuerySize) {
    auto q1 = dns_client::build_query("www.example.com", dns_record::A, dns_class::INTERNET, true, false);
    auto q2 = dns_client::build_query("www.example.com", dns_record::A, dns_class::INTERNET, true, false, false,
                                      edns::DEFAULT_UDP_PAYLOAD, "WwW.ExAmPlE.CoM");
    EXPECT_EQ(q1.size(), q2.size());
}

TEST(DnsResponseParsing, ZeroX20ValidationPasses) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 1, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "WwW.ExAmPlE.CoM", 1);
    append_a_record(resp, "WwW.ExAmPlE.CoM", 60, "1.2.3.4");

    auto result = dns_client::parse_response(resp, 1234, "WwW.ExAmPlE.CoM");
    EXPECT_TRUE(result.is_success());
    ASSERT_EQ(result.answers.size(), 1);
    EXPECT_EQ(result.answers[0].data, "1.2.3.4");
}

TEST(DnsResponseParsing, ZeroX20ValidationRejectsCaseMismatch) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 1, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "www.example.com", 1);
    append_a_record(resp, "www.example.com", 60, "1.2.3.4");

    EXPECT_THROW({ dns_client::parse_response(resp, 1234, "WwW.ExAmPlE.CoM"); }, dns_exception);
}

TEST(DnsResponseParsing, ZeroX20ValidationIsSkippedWhenEmpty) {
    byte_vector resp;
    auto hdr = make_header(1234, 0x8180, 1, 1, 0, 0);
    resp.insert(resp.end(), hdr.begin(), hdr.end());
    append_question(resp, "www.example.com", 1);
    append_a_record(resp, "www.example.com", 60, "1.2.3.4");

    auto result = dns_client::parse_response(resp, 1234);
    EXPECT_TRUE(result.is_success());
}

namespace {
    dns_query_result make_ttl_result(const vector<uint32_t>& ttls, const bool success = true) {
        dns_query_result result;
        result.response_code = success ? dns_response::NON_ERROR : dns_response::NAME_ERROR;
        for (const uint32_t ttl: ttls) {
            result.answers.push_back(dns_record("www.example.com", dns_record::A, dns_class::INTERNET, ttl, "1.2.3.4"));
        }
        return result;
    }
} // namespace

TEST(DnsClientCache, EffectiveTTLUsesMinRecordTTL) {
    const auto result = make_ttl_result({60, 300, 900});
    EXPECT_EQ(dns_client::effective_cache_ttl(result, seconds(300)), seconds(60));
}

TEST(DnsClientCache, EffectiveTTLCappedByConfiguredCap) {
    const auto result = make_ttl_result({86400});
    EXPECT_EQ(dns_client::effective_cache_ttl(result, seconds(300)), seconds(300));
}

TEST(DnsClientCache, EffectiveTTLZeroTTLDisablesCaching) {
    const auto result = make_ttl_result({0});
    EXPECT_EQ(dns_client::effective_cache_ttl(result, seconds(300)), seconds(0));
}

TEST(DnsClientCache, EffectiveTTLNegativeUsesNegativeCacheTTL) {
    const auto result = make_ttl_result({60}, false);
    EXPECT_EQ(dns_client::effective_cache_ttl(result, seconds(300)), edns::NEGATIVE_CACHE_TTL);
}

TEST(DnsClientCache, EffectiveTTLNoAnswersUsesCap) {
    const dns_query_result result;
    EXPECT_EQ(dns_client::effective_cache_ttl(result, seconds(300)), seconds(300));
}

TEST(DnsClientCache, EffectiveTTLCapZeroDisablesCaching) {
    const auto result = make_ttl_result({60});
    EXPECT_EQ(dns_client::effective_cache_ttl(result, seconds(0)), seconds(0));
}

TEST(DnsClient, SetMaxUDPRetries) {
    dns_client client;
    client.set_max_udp_retries(0);
    client.set_max_udp_retries(3);
}

TEST(DnsClient, SetRandomizeCase) {
    dns_client client;
    client.set_randomize_case(false);
    client.set_randomize_case(true);
}
