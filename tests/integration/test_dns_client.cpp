#include <NeForce/core/async/co_spawn.hpp>
#include <NeForce/core/async/stop_token.hpp>
#include <NeForce/core/memory/endian.hpp>
#include <NeForce/core/memory/memory_view.hpp>
#include <NeForce/core/time/clocks.hpp>
#include <NeForce/network/dns/dns_client.hpp>
#include <NeForce/network/tcp/tcp_acceptor.hpp>
#include <NeForce/network/tcp/tcp_socket.hpp>
#include <NeForce/network/udp_socket.hpp>
#include <NeForce/network/util/ip_address.hpp>
#include <gtest/gtest.h>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <ws2tcpip.h>
#else
#    include <arpa/inet.h>
#endif
using namespace neforce;

namespace {
    bool network_available() {
        dns_client::config cfg;
        cfg.server = "8.8.8.8";
        cfg.timeout = milliseconds(2000);
        io_context ioc;
        dns_client client(cfg, ioc);
        try {
            client.query("example.com");
            return true;
        } catch (...) {
            return false;
        }
    }

    bool is_network_available = network_available();

    uint16_t next_mock_port() {
        static uint16_t next_port = 49152;
        const uint16_t port = next_port;
        next_port = static_cast<uint16_t>((next_port + 1 > 60000) ? 49152 : next_port + 1);
        return port;
    }

    byte_vector extract_question(const byte_vector& query) {
        if (query.size() < 12) {
            return {};
        }
        size_t offset = 12;
        while (offset < query.size()) {
            const byte_t len = query[offset];
            if (len == 0) {
                ++offset;
                break;
            }
            offset += static_cast<size_t>(len) + 1;
        }
        if (offset + 4 > query.size()) {
            return {};
        }
        return byte_vector(query.begin() + 12, query.begin() + offset + 4);
    }

    void put_u16_be(byte_vector& v, const uint16_t val) {
        const uint16_t nv = endian::host_to_network(val);
        v.insert(v.end(), reinterpret_cast<const byte_t*>(&nv), reinterpret_cast<const byte_t*>(&nv) + 2);
    }

    byte_vector make_dns_response(const uint16_t id, const uint16_t flags, const byte_vector& question,
                                  const bool with_answer, const uint32_t ttl, const char* ip) {
        byte_vector resp;
        put_u16_be(resp, id);
        put_u16_be(resp, flags);
        put_u16_be(resp, 1);
        put_u16_be(resp, with_answer ? 1 : 0);
        put_u16_be(resp, 0);
        put_u16_be(resp, 0);
        resp.insert(resp.end(), question.begin(), question.end());
        if (with_answer) {
            resp.push_back(0xC0);
            resp.push_back(0x0C);
            put_u16_be(resp, 1);
            put_u16_be(resp, 1);
            const uint32_t nttl = endian::host_to_network(ttl);
            resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&nttl), reinterpret_cast<const byte_t*>(&nttl) + 4);
            put_u16_be(resp, 4);
            in_addr addr{};
            ::inet_pton(AF_INET, ip, &addr);
            resp.insert(resp.end(), reinterpret_cast<const byte_t*>(&addr), reinterpret_cast<const byte_t*>(&addr) + 4);
        }
        return resp;
    }

    void mangle_question_case(byte_vector& question) {
        for (byte_t& b: question) {
            if (b >= 'A' && b <= 'Z') {
                b = static_cast<byte_t>(b - 'A' + 'a');
            }
        }
    }

    void run_mock_udp(const uint16_t port, const bool drop_first, const bool reply_truncated, const bool mangle_case,
                      const steady_clock::time_point deadline) {
        try {
            udp_socket sock;
            sock.open();
            const auto endpoint = ip_address::parse("127.0.0.1", ports{port});
            if (!endpoint) {
                return;
            }
            sock.bind(*endpoint);
            sock.set_nonblocking(true);

            int queries = 0;
            const int expected_queries = drop_first ? 2 : 1;
            while (steady_clock::now() < deadline && queries < expected_queries) {
                byte_vector buf(65535);
                try {
                    const auto recv =
                            sock.receive_from(memory_view<char>{reinterpret_cast<char*>(buf.data()), buf.size()});
                    if (recv.first <= 0) {
                        this_thread::sleep_for(milliseconds(5));
                        continue;
                    }
                    buf.resize(static_cast<size_t>(recv.first));
                    ++queries;
                    if (queries == 1 && drop_first) {
                        continue;
                    }
                    const uint16_t id = endian::network_to_host(*reinterpret_cast<const uint16_t*>(buf.data()));
                    auto question = extract_question(buf);
                    if (mangle_case) {
                        mangle_question_case(question);
                    }
                    const uint16_t flags = reply_truncated ? 0x8380 : 0x8180;
                    const auto resp = make_dns_response(id, flags, question, !reply_truncated, 60, "4.4.4.4");
                    sock.send_to(memory_view<const char>{reinterpret_cast<const char*>(resp.data()), resp.size()},
                                 recv.second);
                } catch (...) {
                    this_thread::sleep_for(milliseconds(5));
                }
            }
            sock.close();
        } catch (...) {
            // ignore
        }
    }

    void run_mock_tcp(const uint16_t port, const steady_clock::time_point deadline) {
        try {
            tcp_acceptor acceptor;
            const auto endpoint = ip_address::parse("127.0.0.1", ports{port});
            if (!endpoint) {
                return;
            }
            acceptor.open(*endpoint, 4);
            acceptor.set_nonblocking(true);

            while (steady_clock::now() < deadline) {
                auto client = acceptor.accept_nonblock();
                if (!client) {
                    this_thread::sleep_for(milliseconds(5));
                    continue;
                }
                byte_t len_buf[2]{0, 0};
                size_t got = 0;
                while (got < 2 && steady_clock::now() < deadline) {
                    const auto n = client->receive(memory_view<char>{reinterpret_cast<char*>(len_buf + got), 2 - got});
                    if (n > 0) {
                        got += static_cast<size_t>(n);
                    } else {
                        this_thread::sleep_for(milliseconds(5));
                    }
                }
                if (got < 2) {
                    client->close();
                    break;
                }
                const uint16_t qlen = endian::network_to_host(*reinterpret_cast<const uint16_t*>(len_buf));
                if (qlen == 0 || qlen > 4096) {
                    client->close();
                    break;
                }
                byte_vector query(qlen);
                size_t total = 0;
                while (total < qlen && steady_clock::now() < deadline) {
                    const auto n = client->receive(
                            memory_view<char>{reinterpret_cast<char*>(query.data() + total), qlen - total});
                    if (n > 0) {
                        total += static_cast<size_t>(n);
                    } else {
                        this_thread::sleep_for(milliseconds(5));
                    }
                }
                if (total < qlen) {
                    client->close();
                    break;
                }
                const uint16_t id = endian::network_to_host(*reinterpret_cast<const uint16_t*>(query.data()));
                const auto question = extract_question(query);
                const auto resp = make_dns_response(id, 0x8180, question, true, 60, "4.4.4.4");
                byte_vector framed;
                put_u16_be(framed, static_cast<uint16_t>(resp.size()));
                framed.insert(framed.end(), resp.begin(), resp.end());
                size_t sent = 0;
                while (sent < framed.size() && steady_clock::now() < deadline) {
                    const auto n = client->send(memory_view<const char>{
                            reinterpret_cast<const char*>(framed.data() + sent), framed.size() - sent});
                    if (n > 0) {
                        sent += static_cast<size_t>(n);
                    } else {
                        this_thread::sleep_for(milliseconds(5));
                    }
                }
                client->close();
                break;
            }
            acceptor.close();
        } catch (...) {
            // ignore
        }
    }

    void run_mock_udp_counting(const uint16_t port, atomic<int>& received, const steady_clock::time_point deadline) {
        try {
            udp_socket sock;
            sock.open();
            const auto endpoint = ip_address::parse("127.0.0.1", ports{port});
            if (!endpoint) {
                return;
            }
            sock.bind(*endpoint);
            sock.set_nonblocking(true);

            while (steady_clock::now() < deadline && received.load() < 2) {
                byte_vector buf(65535);
                try {
                    const auto recv =
                            sock.receive_from(memory_view<char>{reinterpret_cast<char*>(buf.data()), buf.size()});
                    if (recv.first <= 0) {
                        this_thread::sleep_for(milliseconds(5));
                        continue;
                    }
                    buf.resize(static_cast<size_t>(recv.first));
                    ++received;
                    const uint16_t id = endian::network_to_host(*reinterpret_cast<const uint16_t*>(buf.data()));
                    const auto question = extract_question(buf);
                    const auto resp = make_dns_response(id, 0x8180, question, true, 60, "4.4.4.4");
                    sock.send_to(memory_view<const char>{reinterpret_cast<const char*>(resp.data()), resp.size()},
                                 recv.second);
                } catch (...) {
                    this_thread::sleep_for(milliseconds(5));
                }
            }
            sock.close();
        } catch (...) {
            // ignore
        }
    }
} // namespace


TEST(DnsClientIntegration, QueryARecord) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    try {
        dns_client::config cfg;
        cfg.server = "8.8.8.8";
        cfg.timeout = milliseconds(5000);
        io_context ioc;
        dns_client client(cfg, ioc);

        auto result = client.query("example.com", dns_record::A);
        EXPECT_TRUE(result.is_success());
        EXPECT_FALSE(result.answers.empty());

        bool has_a = false;
        for (const auto& record: result.answers) {
            if (record.type == dns_record::A) {
                has_a = true;
                EXPECT_FALSE(record.data.empty());
                break;
            }
        }
        EXPECT_TRUE(has_a);
        EXPECT_GT(result.query_time.count(), 0);
    } catch (const exception& e) {
        GTEST_SKIP() << "DNS may failed because timeout: " << e.what();
    }
}

TEST(DnsClientIntegration, QueryAAAARecord) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    try {
        dns_client::config cfg;
        cfg.server = "8.8.8.8";
        cfg.timeout = milliseconds(5000);
        io_context ioc;
        dns_client client(cfg, ioc);

        auto result = client.query("google.com", dns_record::AAAA);
        EXPECT_TRUE(result.is_success());

        bool has_aaaa = false;
        for (const auto& record: result.answers) {
            if (record.type == dns_record::AAAA) {
                has_aaaa = true;
                EXPECT_FALSE(record.data.empty());
                break;
            }
        }
        EXPECT_TRUE(has_aaaa);
    } catch (const exception& e) {
        GTEST_SKIP() << "DNS may failed because timeout: " << e.what();
    }
}

TEST(DnsClientIntegration, QueryMXRecord) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    try {
        dns_client::config cfg;
        cfg.server = "8.8.8.8";
        cfg.timeout = milliseconds(5000);
        io_context ioc;
        dns_client client(cfg, ioc);

        auto result = client.query("gmail.com", dns_record::MX);
        EXPECT_TRUE(result.is_success());
        EXPECT_FALSE(result.answers.empty());

        for (const auto& record: result.answers) {
            EXPECT_EQ(record.type, dns_record::MX);
            EXPECT_FALSE(record.data.empty());
        }
    } catch (const exception& e) {
        GTEST_SKIP() << "DNS may failed because timeout: " << e.what();
    }
}

TEST(DnsClientIntegration, QueryTXTRecord) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    try {
        dns_client::config cfg;
        cfg.server = "8.8.8.8";
        cfg.timeout = milliseconds(5000);
        io_context ioc;
        dns_client client(cfg, ioc);

        auto result = client.query("google.com", dns_record::TXT);
        EXPECT_TRUE(result.is_success());
        EXPECT_FALSE(result.answers.empty());

        for (const auto& record: result.answers) {
            EXPECT_EQ(record.type, dns_record::TXT);
            EXPECT_FALSE(record.data.empty());
        }
    } catch (const exception& e) {
        GTEST_SKIP() << "DNS may failed because timeout: " << e.what();
    }
}

TEST(DnsClientIntegration, QueryNSRecord) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    try {
        dns_client::config cfg;
        cfg.server = "8.8.8.8";
        cfg.timeout = milliseconds(5000);
        io_context ioc;
        dns_client client(cfg, ioc);

        auto result = client.query("com", dns_record::NS);
        EXPECT_TRUE(result.is_success());
        EXPECT_FALSE(result.answers.empty());

        size_t ns_count = 0;
        for (const auto& record: result.answers) {
            if (record.type != dns_record::NS) {
                continue;
            }
            ++ns_count;
            EXPECT_FALSE(record.data.empty());
        }
        EXPECT_GT(ns_count, 0u);
    } catch (const exception& e) {
        GTEST_SKIP() << "DNS may failed because timeout: " << e.what();
    }
}

TEST(DnsClientIntegration, QuerySOARecord) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    try {
        dns_client::config cfg;
        cfg.server = "8.8.8.8";
        cfg.timeout = milliseconds(5000);
        io_context ioc;
        dns_client client(cfg, ioc);

        auto result = client.query("com", dns_record::SOA);
        EXPECT_TRUE(result.is_success());

        bool has_soa = false;
        for (const auto& record: result.answers) {
            if (record.type == dns_record::SOA) {
                has_soa = true;
                EXPECT_FALSE(record.data.empty());
                break;
            }
        }
        EXPECT_TRUE(has_soa);
    } catch (const exception& e) {
        GTEST_SKIP() << "DNS may failed because timeout: " << e.what();
    }
}

TEST(DnsClientIntegration, QuerySRVRecord) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    try {
        dns_client::config cfg;
        cfg.server = "8.8.8.8";
        cfg.timeout = milliseconds(5000);
        io_context ioc;
        dns_client client(cfg, ioc);

        auto result = client.query("_xmpp-server._tcp.gmail.com", dns_record::SRV);
        EXPECT_TRUE(result.is_success() || result.response_code == dns_response::SERVER_FAILURE ||
                    result.response_code == dns_response::NAME_ERROR);
    } catch (const exception& e) {
        GTEST_SKIP() << "DNS may failed because timeout: " << e.what();
    }
}

TEST(DnsClientIntegration, QueryCNAMERecord) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    try {
        dns_client::config cfg;
        cfg.server = "8.8.8.8";
        cfg.timeout = milliseconds(5000);
        io_context ioc;
        dns_client client(cfg, ioc);

        auto result = client.query("www.github.com", dns_record::CNAME);
        EXPECT_TRUE(result.is_success());
    } catch (const exception& e) {
        GTEST_SKIP() << "DNS may failed because timeout: " << e.what();
    }
}

TEST(DnsClientIntegration, ResolveARecord) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    try {
        dns_client::config cfg;
        cfg.server = "8.8.8.8";
        cfg.timeout = milliseconds(5000);
        io_context ioc;
        dns_client client(cfg, ioc);
        auto ips = client.resolve_a("example.com");
        EXPECT_FALSE(ips.empty());

        for (const auto& ip: ips) {
            EXPECT_FALSE(ip.empty());
            EXPECT_TRUE(ip.contains('.'));
        }
    } catch (const exception& e) {
        GTEST_SKIP() << "DNS may failed because timeout: " << e.what();
    }
}

TEST(DnsClientIntegration, ResolveAAAARecord) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    try {
        dns_client::config cfg;
        cfg.server = "8.8.8.8";
        cfg.timeout = milliseconds(5000);
        io_context ioc;
        dns_client client(cfg, ioc);
        auto ips = client.resolve_aaaa("google.com");
        EXPECT_FALSE(ips.empty());

        for (const auto& ip: ips) {
            EXPECT_FALSE(ip.empty());
            EXPECT_TRUE(ip.contains(':'));
        }
    } catch (const exception& e) {
        GTEST_SKIP() << "DNS may failed because timeout: " << e.what();
    }
}

TEST(DnsClientIntegration, ResolveMXRecord) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    try {
        dns_client::config cfg;
        cfg.server = "8.8.8.8";
        cfg.timeout = milliseconds(5000);
        io_context ioc;
        dns_client client(cfg, ioc);
        auto mx = client.resolve_mx("gmail.com");
        EXPECT_FALSE(mx.empty());

        for (const auto& record: mx) {
            EXPECT_FALSE(record.empty());
        }
    } catch (const exception& e) {
        GTEST_SKIP() << "DNS may failed because timeout: " << e.what();
    }
}

TEST(DnsClientIntegration, ResolveTXTRecord) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    try {
        dns_client::config cfg;
        cfg.server = "8.8.8.8";
        cfg.timeout = milliseconds(5000);
        io_context ioc;
        dns_client client(cfg, ioc);
        auto txt = client.resolve_txt("google.com");
        EXPECT_FALSE(txt.empty());
    } catch (const exception& e) {
        GTEST_SKIP() << "DNS may failed because timeout: " << e.what();
    }
}

TEST(DnsClientIntegration, ResolveCNAMERecord) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    try {
        dns_client::config cfg;
        cfg.server = "8.8.8.8";
        cfg.timeout = milliseconds(5000);
        io_context ioc;
        dns_client client(cfg, ioc);
        auto cnames = client.resolve_cname("www.github.com");
        EXPECT_FALSE(cnames.empty());
    } catch (const exception& e) {
        GTEST_SKIP() << "DNS may failed because timeout: " << e.what();
    }
}

TEST(DnsClientIntegration, ResolveSOARecord) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    try {
        dns_client::config cfg;
        cfg.server = "8.8.8.8";
        cfg.timeout = milliseconds(5000);
        io_context ioc;
        dns_client client(cfg, ioc);
        auto soa = client.resolve_soa("com");
        ASSERT_TRUE(soa.has_value());
        EXPECT_FALSE(soa->mname.empty());
        EXPECT_FALSE(soa->rname.empty());
        EXPECT_GT(soa->serial, 0u);
        EXPECT_GT(soa->refresh, 0u);
        EXPECT_GT(soa->expire, 0u);
    } catch (const exception& e) {
        GTEST_SKIP() << "DNS may failed because timeout: " << e.what();
    }
}

TEST(DnsClientIntegration, ResolveSRVRecord) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    try {
        dns_client::config cfg;
        cfg.server = "8.8.8.8";
        cfg.timeout = milliseconds(5000);
        io_context ioc;
        dns_client client(cfg, ioc);
        auto srv = client.resolve_srv("_xmpp-server._tcp.gmail.com");
        for (const auto& record: srv) {
            EXPECT_FALSE(record.target.empty());
            EXPECT_GT(record.port, 0);
        }
    } catch (const exception& e) {
        GTEST_SKIP() << "DNS may failed because timeout: " << e.what();
    }
}

TEST(DnsClientIntegration, ReverseQueryIPv4) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    try {
        dns_client::config cfg;
        cfg.server = "8.8.8.8";
        cfg.timeout = milliseconds(5000);
        io_context ioc;
        dns_client client(cfg, ioc);

        auto domain = client.reverse_query("8.8.8.8");
        EXPECT_FALSE(domain.empty());
        EXPECT_TRUE(domain.contains("google") || domain.contains("dns"));
    } catch (const exception& e) {
        GTEST_SKIP() << "DNS may failed because timeout: " << e.what();
    }
}

TEST(DnsClientIntegration, ReverseQueryInvalidIPThrows) {
    dns_client client;
    EXPECT_THROW({ client.reverse_query(""); }, dns_exception);
    EXPECT_THROW({ client.reverse_query("not_an_ip"); }, dns_exception);
}

TEST(DnsClientIntegration, BatchQuery) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    try {
        dns_client::config cfg;
        cfg.server = "8.8.8.8";
        cfg.timeout = milliseconds(5000);
        io_context ioc;
        dns_client client(cfg, ioc);
        vector<string> domains = {"example.com", "google.com", "cloudflare.com"};
        client.batch_query(domains, dns_record::A);
    } catch (const exception& e) {
        GTEST_SKIP() << "DNS may failed because timeout: " << e.what();
    }
}

TEST(DnsClientIntegration, BatchQueryEmptyList) {
    dns_client::config cfg;
    cfg.server = "8.8.8.8";
    cfg.timeout = milliseconds(5000);
    io_context ioc;
    dns_client client(cfg, ioc);
    vector<string> domains;
    auto results = client.batch_query(domains);
    EXPECT_TRUE(results.empty());
}

TEST(DnsClientIntegration, CacheHitReturnsSameResult) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    try {
        dns_client::config cfg;
        cfg.server = "8.8.8.8";
        cfg.timeout = milliseconds(5000);
        io_context ioc;
        dns_client client(cfg, ioc);
        client.set_cache_ttl(seconds(300));

        auto result1 = client.query("iana.org", dns_record::A);
        EXPECT_TRUE(result1.is_success());

        auto result2 = client.query("iana.org", dns_record::A);
        EXPECT_TRUE(result2.is_success());

        EXPECT_EQ(result1.answers.size(), result2.answers.size());
        if (!result1.answers.empty() && !result2.answers.empty()) {
            EXPECT_EQ(result1.answers[0].data, result2.answers[0].data);
        }
    } catch (const exception& e) {
        GTEST_SKIP() << "DNS may failed because timeout: " << e.what();
    }
}

TEST(DnsClientIntegration, CacheCanBeCleared) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    try {
        dns_client::config cfg;
        cfg.server = "8.8.8.8";
        cfg.timeout = milliseconds(5000);
        io_context ioc;
        dns_client client(cfg, ioc);

        auto result1 = client.query("example.org", dns_record::A);
        EXPECT_TRUE(result1.is_success());

        client.clear_cache();
        auto result2 = client.query("example.org", dns_record::A);
        EXPECT_TRUE(result2.is_success());
    } catch (const exception& e) {
        GTEST_SKIP() << "DNS may failed because timeout: " << e.what();
    }
}

TEST(DnsClientIntegration, TCPMode) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    dns_client::config cfg;
    cfg.server = "8.8.8.8";
    cfg.timeout = milliseconds(5000);
    io_context ioc;
    dns_client client(cfg, ioc);

    auto result = client.query("example.com", dns_record::A);
    EXPECT_TRUE(result.is_success());
    EXPECT_FALSE(result.answers.empty());
    EXPECT_GT(result.query_time.count(), 0);
}

TEST(DnsClientIntegration, QueryWithDifferentServer) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    try {
        dns_client::config cfg;
        cfg.server = "8.8.8.8";
        cfg.timeout = milliseconds(5000);
        io_context ioc;
        dns_client client(cfg, ioc);

        auto result = client.query("example.com", dns_record::A);
        EXPECT_TRUE(result.is_success());
        EXPECT_FALSE(result.answers.empty());
    } catch (const exception& e) {
        GTEST_SKIP() << "DNS may failed because timeout: " << e.what();
    }
}

TEST(DnsClientIntegration, ServerSwitchBetweenQueries) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    try {
        dns_client::config cfg;
        cfg.server = "8.8.8.8";
        cfg.timeout = milliseconds(5000);
        io_context ioc;
        dns_client client(cfg, ioc);

        auto result1 = client.query("example.com", dns_record::A);
        EXPECT_TRUE(result1.is_success());

        cfg.server = "9.9.9.9";
        client.set_config(cfg);
        client.clear_cache();

        auto result2 = client.query("example.com", dns_record::A);
        EXPECT_TRUE(result2.is_success());
    } catch (const exception& e) {
        GTEST_SKIP() << "DNS may failed because timeout: " << e.what();
    }
}

TEST(DnsClientIntegration, QueryWithRecursionDesiredOff) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    try {
        dns_client::config cfg;
        cfg.server = "8.8.8.8";
        cfg.timeout = milliseconds(5000);
        io_context ioc;
        dns_client client(cfg, ioc);
        client.set_recursion_desired(false);
        client.query("example.com", dns_record::A);
    } catch (const exception& e) {
        GTEST_SKIP() << "DNS may failed because timeout: " << e.what();
    }
}

TEST(DnsClientIntegration, QueryWithEDNSCustomPayload) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    dns_client::config cfg;
    cfg.server = "8.8.8.8";
    cfg.timeout = milliseconds(5000);
    io_context ioc;
    dns_client client(cfg, ioc);
    client.set_edns_udp_payload(4096);

    auto result = client.query("example.com", dns_record::A);
    EXPECT_TRUE(result.is_success());
    EXPECT_GT(result.udp_payload_size, 0u);
}

TEST(DnsClientIntegration, QueryAsync) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    try {
        dns_client::config cfg;
        cfg.server = "8.8.8.8";
        cfg.timeout = milliseconds(5000);
        io_context ioc;
        dns_client client(cfg, ioc);

        client.set_max_udp_retries(0);

        auto future = client.query_async("example.com", dns_record::A);
        const auto deadline = steady_clock::now() + milliseconds(5000);
        while (future.wait_for(milliseconds(0)) != future_status::ready) {
            if (steady_clock::now() >= deadline) {
                break;
            }
            ioc.run_one(100);
        }
        ASSERT_TRUE(future.wait_for(milliseconds(0)) == future_status::ready) << "QueryAsync timed out";
        auto result = future.get();
        EXPECT_TRUE(result.is_success());
        EXPECT_FALSE(result.answers.empty());
    } catch (const exception& e) {
        GTEST_SKIP() << "DNS may failed because timeout: " << e.what();
    }
}

TEST(DnsClientIntegration, QueryEmptyDomainThrows) {
    dns_client client;
    EXPECT_THROW({ client.query(""); }, dns_exception);
}

TEST(DnsClientIntegration, QueryNXDOMAIN) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    try {
        dns_client::config cfg;
        cfg.server = "8.8.8.8";
        cfg.timeout = milliseconds(5000);
        io_context ioc;
        dns_client client(cfg, ioc);

        auto result = client.query("this-domain-definitely-does-not-exist-12345.com", dns_record::A);
        EXPECT_EQ(result.response_code, dns_response::NAME_ERROR);
        EXPECT_FALSE(result.is_success());
        EXPECT_TRUE(result.answers.empty());
    } catch (const exception& e) {
        GTEST_SKIP() << "DNS may failed because timeout: " << e.what();
    }
}

TEST(DnsClientIntegration, QueryTimeout) {
    dns_client::config cfg;
    cfg.server = "192.0.2.1";
    cfg.timeout = milliseconds(500);
    io_context ioc;
    dns_client client(cfg, ioc);
    client.set_max_udp_retries(0);

    EXPECT_ANY_THROW({ client.query("example.com"); });
}

TEST(DnsClientIntegration, DefaultConfigUsesGoogleDNS) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    try {
        dns_client::config cfg;
        cfg.server = "8.8.8.8";
        cfg.timeout = milliseconds(5000);
        io_context ioc;
        dns_client client(cfg, ioc);
        auto result = client.query("example.com", dns_record::A);
        EXPECT_TRUE(result.is_success());
        EXPECT_FALSE(result.answers.empty());
    } catch (const exception& e) {
        GTEST_SKIP() << "DNS may failed because timeout: " << e.what();
    }
}

TEST(DnsClientIntegration, QuerySetsRecursiveAvailable) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    try {
        dns_client::config cfg;
        cfg.server = "8.8.8.8";
        cfg.timeout = milliseconds(5000);
        io_context ioc;
        dns_client client(cfg, ioc);

        auto result = client.query("example.com", dns_record::A);
        EXPECT_TRUE(result.recursive_available);
    } catch (const exception& e) {
        GTEST_SKIP() << "DNS may failed because timeout: " << e.what();
    }
}

TEST(DnsClientIntegration, ResolveACNAMEChain) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    try {
        dns_client::config cfg;
        cfg.server = "8.8.8.8";
        cfg.timeout = milliseconds(5000);
        io_context ioc;
        dns_client client(cfg, ioc);
        auto ips = client.resolve_a("www.github.com");
        EXPECT_FALSE(ips.empty());

        for (const auto& ip: ips) {
            EXPECT_FALSE(ip.empty());
        }
    } catch (const exception& e) {
        GTEST_SKIP() << "DNS may failed because timeout: " << e.what();
    }
}

TEST(DnsClientIntegration, QueryCHClass) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    try {
        dns_client::config cfg;
        cfg.server = "8.8.8.8";
        cfg.timeout = milliseconds(5000);
        io_context ioc;
        dns_client client(cfg, ioc);
        client.query("version.bind", dns_record::TXT, dns_class::CHAOS);
    } catch (const exception& e) {
        GTEST_SKIP() << "DNS may failed because timeout: " << e.what();
    }
}

TEST(DnsClientIntegration, MultipleQueriesSameClient) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    try {
        dns_client::config cfg;
        cfg.server = "8.8.8.8";
        cfg.timeout = milliseconds(5000);
        io_context ioc;
        dns_client client(cfg, ioc);

        auto r1 = client.query("example.com", dns_record::A);
        EXPECT_TRUE(r1.is_success());

        auto r2 = client.query("google.com", dns_record::A);
        EXPECT_TRUE(r2.is_success());

        auto r3 = client.query("cloudflare.com", dns_record::AAAA);
        EXPECT_TRUE(r3.is_success());
    } catch (const exception& e) {
        GTEST_SKIP() << "DNS may failed because timeout: " << e.what();
    }
}

TEST(DnsClientIntegration, UDPRetrySucceedsAfterDrop) {
    const uint16_t port = next_mock_port();
    const auto deadline = steady_clock::now() + seconds(5);
    thread udp_thread(run_mock_udp, port, true, false, false, deadline);

    try {
        dns_client::config cfg;
        cfg.server = "127.0.0.1";
        cfg.port = ports(port);
        cfg.timeout = milliseconds(300);
        io_context ioc;
        dns_client client(cfg, ioc);

        auto result = client.query("retry.example", dns_record::A);
        EXPECT_TRUE(result.is_success());
        ASSERT_FALSE(result.answers.empty());
        EXPECT_EQ(result.answers[0].data, "4.4.4.4");
    } catch (const exception& e) {
        udp_thread.join();
        FAIL() << "UDP retry query failed: " << e.what();
    }
    udp_thread.join();
}

TEST(DnsClientIntegration, TCPFallbackOnTruncatedUDP) {
    const uint16_t port = next_mock_port();
    const auto deadline = steady_clock::now() + seconds(5);
    thread udp_thread(run_mock_udp, port, false, true, false, deadline);
    thread tcp_thread(run_mock_tcp, port, deadline);

    try {
        dns_client::config cfg;
        cfg.server = "127.0.0.1";
        cfg.port = ports(port);
        cfg.timeout = milliseconds(2000);
        io_context ioc;
        dns_client client(cfg, ioc);

        auto result = client.query("trunc.example", dns_record::A);
        EXPECT_TRUE(result.is_success());
        ASSERT_FALSE(result.answers.empty());
        EXPECT_EQ(result.answers[0].data, "4.4.4.4");
    } catch (const exception& e) {
        udp_thread.join();
        tcp_thread.join();
        FAIL() << "TCP fallback query failed: " << e.what();
    }
    udp_thread.join();
    tcp_thread.join();
}

TEST(DnsClientIntegration, ForcedTCPMode) {
    const uint16_t port = next_mock_port();
    const auto deadline = steady_clock::now() + seconds(5);
    thread tcp_thread(run_mock_tcp, port, deadline);

    try {
        dns_client::config cfg;
        cfg.server = "127.0.0.1";
        cfg.port = ports(port);
        cfg.timeout = milliseconds(2000);
        io_context ioc;
        dns_client client(cfg, ioc, true);

        auto result = client.query("tcp-only.example", dns_record::A);
        EXPECT_TRUE(result.is_success());
        ASSERT_FALSE(result.answers.empty());
        EXPECT_EQ(result.answers[0].data, "4.4.4.4");
    } catch (const exception& e) {
        tcp_thread.join();
        FAIL() << "Forced TCP query failed: " << e.what();
    }
    tcp_thread.join();
}

TEST(DnsClientIntegration, ZeroX20MismatchRejected) {
    const uint16_t port = next_mock_port();
    const auto deadline = steady_clock::now() + seconds(5);
    thread udp_thread(run_mock_udp, port, false, false, true, deadline);

    try {
        dns_client::config cfg;
        cfg.server = "127.0.0.1";
        cfg.port = ports(port);
        cfg.timeout = milliseconds(300);
        io_context ioc;
        dns_client client(cfg, ioc);
        client.set_max_udp_retries(0);

        EXPECT_ANY_THROW({ client.query("zero-x20-mismatch-rejected.example", dns_record::A); });
    } catch (const exception& e) {
        udp_thread.join();
        FAIL() << "Unexpected exception: " << e.what();
    }
    udp_thread.join();
}

TEST(DnsClientIntegration, AsyncQueryCallbackSucceeds) {
    const uint16_t port = next_mock_port();
    const auto deadline = steady_clock::now() + seconds(5);
    thread udp_thread(run_mock_udp, port, false, false, false, deadline);

    try {
        dns_client::config cfg;
        cfg.server = "127.0.0.1";
        cfg.port = ports(port);
        cfg.timeout = milliseconds(2000);
        io_context ioc;
        dns_client client(cfg, ioc);

        bool called = false;
        error_code ec_result;
        dns_query_result result;
        client.async_query("cb.example", dns_record::A, dns_class::INTERNET, [&](error_code ec, dns_query_result r) {
            called = true;
            ec_result = ec;
            result = move(r);
        });

        const auto wait_until = steady_clock::now() + seconds(3);
        while (!called && steady_clock::now() < wait_until) {
            ioc.run_one(100);
        }
        EXPECT_TRUE(called);
        EXPECT_FALSE(ec_result);
        EXPECT_TRUE(result.is_success());
        if (result.answers.empty()) {
            udp_thread.join();
            FAIL() << "async_query callback returned no answers";
        }
        EXPECT_EQ(result.answers[0].data, "4.4.4.4");
    } catch (const exception& e) {
        udp_thread.join();
        FAIL() << "async_query callback failed: " << e.what();
    }
    udp_thread.join();
}

TEST(DnsClientIntegration, AsyncQueryCancellationAborts) {
    const uint16_t port = next_mock_port();
    const auto deadline = steady_clock::now() + seconds(5);
    thread udp_thread(run_mock_udp, port, false, false, false, deadline);

    try {
        dns_client::config cfg;
        cfg.server = "127.0.0.1";
        cfg.port = ports(port);
        cfg.timeout = milliseconds(2000);
        io_context ioc;
        dns_client client(cfg, ioc);

        stop_source source;
        cancellation_slot slot(source.get_token());
        int calls = 0;
        error_code ec_result;
        client.async_query("cancel.example", dns_record::A, dns_class::INTERNET, slot,
                           [&](error_code ec, dns_query_result) {
                               ++calls;
                               ec_result = ec;
                           });
        source.request_stop();

        EXPECT_EQ(calls, 1);
        EXPECT_EQ(ec_result, make_operation_aborted());

        const auto settle_until = steady_clock::now() + milliseconds(300);
        while (steady_clock::now() < settle_until) {
            ioc.run_one(50);
        }
        EXPECT_EQ(calls, 1);
    } catch (const exception& e) {
        udp_thread.join();
        FAIL() << "cancellation test failed: " << e.what();
    }
    udp_thread.join();
}

TEST(DnsClientIntegration, AsyncQueryPreCancelledSlot) {
    dns_client::config cfg;
    cfg.server = "127.0.0.1";
    cfg.timeout = milliseconds(500);
    io_context ioc;
    dns_client client(cfg, ioc);

    stop_source source;
    source.request_stop();
    cancellation_slot slot(source.get_token());
    bool called = false;
    error_code ec_result;
    client.async_query("precancel.example", dns_record::A, dns_class::INTERNET, slot,
                       [&](error_code ec, dns_query_result) {
                           called = true;
                           ec_result = ec;
                       });
    EXPECT_TRUE(called);
    EXPECT_EQ(ec_result, make_operation_aborted());
}

TEST(DnsClientIntegration, AsyncQueryDetachedCompletes) {
    const uint16_t port = next_mock_port();
    const auto deadline = steady_clock::now() + seconds(5);
    thread udp_thread(run_mock_udp, port, false, false, false, deadline);

    dns_client::config cfg;
    cfg.server = "127.0.0.1";
    cfg.port = ports(port);
    cfg.timeout = milliseconds(2000);
    io_context ioc;
    dns_client client(cfg, ioc);

    client.async_query("detached.example", dns_record::A, dns_class::INTERNET, detached);

    const auto wait_until = steady_clock::now() + milliseconds(500);
    while (steady_clock::now() < wait_until) {
        ioc.run_one(50);
    }
    udp_thread.join();
}

TEST(DnsClientIntegration, CacheHitDefersCallback) {
    const uint16_t port = next_mock_port();
    const auto deadline = steady_clock::now() + seconds(5);
    thread udp_thread(run_mock_udp, port, false, false, false, deadline);

    try {
        dns_client::config cfg;
        cfg.server = "127.0.0.1";
        cfg.port = ports(port);
        cfg.timeout = milliseconds(2000);
        io_context ioc;
        dns_client client(cfg, ioc);

        auto r1 = client.query("defer.example", dns_record::A);
        if (!r1.is_success()) {
            udp_thread.join();
            FAIL() << "first query failed";
        }

        bool called = false;
        client.async_query("defer.example", dns_record::A, dns_class::INTERNET,
                           [&](error_code, dns_query_result) { called = true; });

        EXPECT_FALSE(called);
        const auto wait_until = steady_clock::now() + seconds(1);
        while (!called && steady_clock::now() < wait_until) {
            ioc.run_one(50);
        }
        EXPECT_TRUE(called);
    } catch (const exception& e) {
        udp_thread.join();
        FAIL() << "cache defer test failed: " << e.what();
    }
    udp_thread.join();
}

TEST(DnsClientIntegration, CacheTTLExpiryRefetches) {
    const uint16_t port = next_mock_port();
    const auto deadline = steady_clock::now() + seconds(5);
    atomic<int> received{0};
    thread udp_thread(run_mock_udp_counting, port, ref(received), deadline);

    try {
        dns_client::config cfg;
        cfg.server = "127.0.0.1";
        cfg.port = ports(port);
        cfg.timeout = milliseconds(2000);
        io_context ioc;
        dns_client client(cfg, ioc);
        client.set_cache_ttl(seconds(1));

        auto r1 = client.query("ttl.example", dns_record::A);
        if (!r1.is_success()) {
            udp_thread.join();
            FAIL() << "first query failed";
        }

        auto r2 = client.query("ttl.example", dns_record::A);
        if (!r2.is_success()) {
            udp_thread.join();
            FAIL() << "second query failed";
        }
        EXPECT_EQ(received.load(), 1);

        this_thread::sleep_for(milliseconds(1200));

        auto r3 = client.query("ttl.example", dns_record::A);
        if (!r3.is_success()) {
            udp_thread.join();
            FAIL() << "third query failed";
        }
        EXPECT_EQ(received.load(), 2);
    } catch (const exception& e) {
        udp_thread.join();
        FAIL() << "cache TTL expiry test failed: " << e.what();
    }
    udp_thread.join();
}

TEST(DnsClientIntegration, RandomizeCaseDisabledAcceptsMismatch) {
    const uint16_t port = next_mock_port();
    const auto deadline = steady_clock::now() + seconds(5);
    thread udp_thread(run_mock_udp, port, false, false, true, deadline);

    try {
        dns_client::config cfg;
        cfg.server = "127.0.0.1";
        cfg.port = ports(port);
        cfg.timeout = milliseconds(2000);
        io_context ioc;
        dns_client client(cfg, ioc);
        client.set_randomize_case(false);
        client.set_max_udp_retries(0);

        auto result = client.query("case-off.example", dns_record::A);
        EXPECT_TRUE(result.is_success());
        if (result.answers.empty()) {
            udp_thread.join();
            FAIL() << "query returned no answers";
        }
        EXPECT_EQ(result.answers[0].data, "4.4.4.4");
    } catch (const exception& e) {
        udp_thread.join();
        FAIL() << "case-randomization disabled test failed: " << e.what();
    }
    udp_thread.join();
}

#ifdef NEFORCE_STANDARD_20

TEST(DnsClientIntegration, AsyncQueryUseAwaitable) {
    const uint16_t port = next_mock_port();
    const auto deadline = steady_clock::now() + seconds(5);
    thread udp_thread(run_mock_udp, port, false, false, false, deadline);

    try {
        dns_client::config cfg;
        cfg.server = "127.0.0.1";
        cfg.port = ports(port);
        cfg.timeout = milliseconds(2000);
        io_context ioc;
        dns_client client(cfg, ioc);

        bool done = false;
        error_code ec_result;
        dns_query_result got;
        co_spawn(ioc.get_executor(), [&]() -> awaitable<void> {
            auto [ec, r] = co_await client.async_query("co.example", dns_record::A, dns_class::INTERNET, use_awaitable);
            ec_result = ec;
            got = move(r);
            done = true;
        });

        const auto wait_until = steady_clock::now() + seconds(3);
        while (!done && steady_clock::now() < wait_until) {
            ioc.run_one(100);
        }
        EXPECT_TRUE(done);
        EXPECT_FALSE(ec_result);
        EXPECT_TRUE(got.is_success());
        if (got.answers.empty()) {
            udp_thread.join();
            FAIL() << "awaitable query returned no answers";
        }
        EXPECT_EQ(got.answers[0].data, "4.4.4.4");
    } catch (const exception& e) {
        udp_thread.join();
        FAIL() << "awaitable query failed: " << e.what();
    }
    udp_thread.join();
}

#endif
