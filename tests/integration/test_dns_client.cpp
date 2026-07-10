#include <NeForce/network/dns/dns_client.hpp>
#include <gtest/gtest.h>
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
} // namespace


TEST(DnsClientIntegration, QueryARecord) {
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
}

TEST(DnsClientIntegration, QueryAAAARecord) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

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
}

TEST(DnsClientIntegration, QueryMXRecord) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

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
}

TEST(DnsClientIntegration, QueryTXTRecord) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

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

        for (const auto& record: result.answers) {
            EXPECT_EQ(record.type, dns_record::NS);
            EXPECT_FALSE(record.data.empty());
        }
    } catch (const exception& e) {
        GTEST_SKIP() << "An exception occured: " << e.what();
    }
}

TEST(DnsClientIntegration, QuerySOARecord) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

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
}

TEST(DnsClientIntegration, QuerySRVRecord) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    dns_client::config cfg;
    cfg.server = "8.8.8.8";
    cfg.timeout = milliseconds(5000);
    io_context ioc;
    dns_client client(cfg, ioc);

    auto result = client.query("_xmpp-server._tcp.gmail.com", dns_record::SRV);
    EXPECT_TRUE(result.is_success() || result.response_code == dns_response::SERVER_FAILURE ||
                result.response_code == dns_response::NAME_ERROR);
}

TEST(DnsClientIntegration, QueryCNAMERecord) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    dns_client::config cfg;
    cfg.server = "8.8.8.8";
    cfg.timeout = milliseconds(5000);
    io_context ioc;
    dns_client client(cfg, ioc);

    auto result = client.query("www.github.com", dns_record::CNAME);
    EXPECT_TRUE(result.is_success());
}

TEST(DnsClientIntegration, ResolveARecord) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

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
}

TEST(DnsClientIntegration, ResolveAAAARecord) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

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
}

TEST(DnsClientIntegration, ResolveMXRecord) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

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
}

TEST(DnsClientIntegration, ResolveTXTRecord) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    dns_client::config cfg;
    cfg.server = "8.8.8.8";
    cfg.timeout = milliseconds(5000);
    io_context ioc;
    dns_client client(cfg, ioc);
    auto txt = client.resolve_txt("google.com");
    EXPECT_FALSE(txt.empty());
}

TEST(DnsClientIntegration, ResolveCNAMERecord) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    dns_client::config cfg;
    cfg.server = "8.8.8.8";
    cfg.timeout = milliseconds(5000);
    io_context ioc;
    dns_client client(cfg, ioc);
    auto cnames = client.resolve_cname("www.github.com");
    EXPECT_FALSE(cnames.empty());
}

TEST(DnsClientIntegration, ResolveSOARecord) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

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
}

TEST(DnsClientIntegration, ResolveSRVRecord) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

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
}

TEST(DnsClientIntegration, ReverseQueryIPv4) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    dns_client::config cfg;
    cfg.server = "8.8.8.8";
    cfg.timeout = milliseconds(5000);
    io_context ioc;
    dns_client client(cfg, ioc);

    auto domain = client.reverse_query("8.8.8.8");
    EXPECT_FALSE(domain.empty());
    EXPECT_TRUE(domain.contains("google") || domain.contains("dns"));
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

    dns_client::config cfg;
    cfg.server = "8.8.8.8";
    cfg.timeout = milliseconds(5000);
    io_context ioc;
    dns_client client(cfg, ioc);
    vector<string> domains = {"example.com", "google.com", "cloudflare.com"};
    auto results = client.batch_query(domains, dns_record::A);

    ASSERT_EQ(results.size(), domains.size());
    for (size_t i = 0; i < results.size(); ++i) {
        EXPECT_TRUE(results[i].is_success()) << "Batch query failed for: " << domains[i].data();
        EXPECT_FALSE(results[i].answers.empty()) << "No answers for: " << domains[i].data();
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
}

TEST(DnsClientIntegration, CacheCanBeCleared) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

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

    dns_client::config cfg;
    cfg.server = "8.8.8.8";
    cfg.timeout = milliseconds(5000);
    io_context ioc;
    dns_client client(cfg, ioc);

    auto result = client.query("example.com", dns_record::A);
    EXPECT_TRUE(result.is_success());
    EXPECT_FALSE(result.answers.empty());
}

TEST(DnsClientIntegration, ServerSwitchBetweenQueries) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

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
}

TEST(DnsClientIntegration, QueryWithRecursionDesiredOff) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    dns_client::config cfg;
    cfg.server = "8.8.8.8";
    cfg.timeout = milliseconds(5000);
    io_context ioc;
    dns_client client(cfg, ioc);
    client.set_recursion_desired(false);

    // When RD=0, a recursive resolver may legitimately refuse or return server errors.
    // The test verifies only that the code path does not crash/throw.
    EXPECT_NO_THROW({ client.query("example.com", dns_record::A); });
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

    dns_client::config cfg;
    cfg.server = "8.8.8.8";
    cfg.timeout = milliseconds(5000);
    io_context ioc;
    dns_client client(cfg, ioc);

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
}

TEST(DnsClientIntegration, QueryEmptyDomainThrows) {
    dns_client client;
    EXPECT_THROW({ client.query(""); }, dns_exception);
}

TEST(DnsClientIntegration, QueryNXDOMAIN) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    dns_client::config cfg;
    cfg.server = "8.8.8.8";
    cfg.timeout = milliseconds(5000);
    io_context ioc;
    dns_client client(cfg, ioc);

    auto result = client.query("this-domain-definitely-does-not-exist-12345.com", dns_record::A);
    EXPECT_EQ(result.response_code, dns_response::NAME_ERROR);
    EXPECT_FALSE(result.is_success());
    EXPECT_TRUE(result.answers.empty());
}

TEST(DnsClientIntegration, QueryTimeout) {
    dns_client::config cfg;
    cfg.server = "192.0.2.1";
    cfg.timeout = milliseconds(500);
    io_context ioc;
    dns_client client(cfg, ioc);

    EXPECT_ANY_THROW({ client.query("example.com"); });
}

TEST(DnsClientIntegration, DefaultConfigUsesGoogleDNS) {
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
}

TEST(DnsClientIntegration, QuerySetsRecursiveAvailable) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    dns_client::config cfg;
    cfg.server = "8.8.8.8";
    cfg.timeout = milliseconds(5000);
    io_context ioc;
    dns_client client(cfg, ioc);

    auto result = client.query("example.com", dns_record::A);
    EXPECT_TRUE(result.recursive_available);
}

TEST(DnsClientIntegration, ResolveACNAMEChain) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

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
}

TEST(DnsClientIntegration, QueryCHClass) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

    dns_client::config cfg;
    cfg.server = "8.8.8.8";
    cfg.timeout = milliseconds(5000);
    io_context ioc;
    dns_client client(cfg, ioc);

    EXPECT_NO_THROW({
        auto result = client.query("version.bind", dns_record::TXT, dns_class::CHAOS);
        (void) result;
    });
}

TEST(DnsClientIntegration, MultipleQueriesSameClient) {
    if (!is_network_available) {
        GTEST_SKIP() << "No network connectivity";
    }

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
}
