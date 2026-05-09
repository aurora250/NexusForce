#include <NeForce/network/util/url.hpp>
#include <NeForce/network/util/mac_address.hpp>
#include <gtest/gtest.h>
#ifdef NEFORCE_PLATFORM_LINUX
#    include <arpa/inet.h>
#endif
using namespace neforce;

class PortsTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(PortsTest, DefaultConstructor) {
    ports p;
    EXPECT_EQ(p.port, ports::UNDEF);
    EXPECT_EQ(p.value(), 0u);
    EXPECT_FALSE(static_cast<bool>(p));
}

TEST_F(PortsTest, EnumConstructor) {
    ports p(ports::HTTP);
    EXPECT_EQ(p.port, ports::HTTP);
    EXPECT_EQ(p.value(), 80u);
    EXPECT_TRUE(static_cast<bool>(p));
}

TEST_F(PortsTest, Uint16Constructor) {
    ports p(static_cast<uint16_t>(443));
    EXPECT_EQ(p.port, ports::HTTPS);
    EXPECT_EQ(p.value(), 443u);
    EXPECT_TRUE(static_cast<bool>(p));
}

TEST_F(PortsTest, BoolConversion) {
    ports undef;
    EXPECT_FALSE(static_cast<bool>(undef));

    ports http(ports::HTTP);
    EXPECT_TRUE(static_cast<bool>(http));

    ports custom(static_cast<uint16_t>(8080));
    EXPECT_TRUE(static_cast<bool>(custom));
}

TEST_F(PortsTest, Uint16Conversion) {
    ports p(ports::SSH);
    uint16_t val = static_cast<uint16_t>(p);
    EXPECT_EQ(val, 22u);
}

TEST_F(PortsTest, ValueMethod) {
    ports p(ports::DNS);
    EXPECT_EQ(p.value(), 53u);
}

TEST_F(PortsTest, IsWellKnown) {
    ports undef;
    EXPECT_TRUE(undef.is_well_known());

    ports http(ports::HTTP);
    EXPECT_TRUE(http.is_well_known());

    ports https(ports::HTTPS);
    EXPECT_TRUE(https.is_well_known());

    ports max_well_known(static_cast<uint16_t>(1023));
    EXPECT_TRUE(max_well_known.is_well_known());

    ports registered(static_cast<uint16_t>(1024));
    EXPECT_FALSE(registered.is_well_known());

    ports dynamic(static_cast<uint16_t>(49152));
    EXPECT_FALSE(dynamic.is_well_known());

    ports mysql(ports::MYSQL);
    EXPECT_FALSE(mysql.is_well_known());
}

TEST_F(PortsTest, IsRegistered) {
    ports undef;
    EXPECT_FALSE(undef.is_registered());

    ports http(ports::HTTP);
    EXPECT_FALSE(http.is_registered());

    ports min_registered(static_cast<uint16_t>(1024));
    EXPECT_TRUE(min_registered.is_registered());

    ports mysql(ports::MYSQL);
    EXPECT_TRUE(mysql.is_registered());

    ports postgresql(ports::POSTGRESQL);
    EXPECT_TRUE(postgresql.is_registered());

    ports redis(ports::REDIS);
    EXPECT_TRUE(redis.is_registered());

    ports max_registered(static_cast<uint16_t>(49151));
    EXPECT_TRUE(max_registered.is_registered());

    ports dynamic(static_cast<uint16_t>(49152));
    EXPECT_FALSE(dynamic.is_registered());
}

TEST_F(PortsTest, IsDynamic) {
    ports undef;
    EXPECT_FALSE(undef.is_dynamic());

    ports http(ports::HTTP);
    EXPECT_FALSE(http.is_dynamic());

    ports registered(static_cast<uint16_t>(49151));
    EXPECT_FALSE(registered.is_dynamic());

    ports min_dynamic(static_cast<uint16_t>(49152));
    EXPECT_TRUE(min_dynamic.is_dynamic());

    ports max_dynamic(static_cast<uint16_t>(65535));
    EXPECT_TRUE(max_dynamic.is_dynamic());
}

TEST_F(PortsTest, ParseHttp) {
    EXPECT_EQ(ports::parse("http"), ports::HTTP);
    EXPECT_EQ(ports::parse("HTTP"), ports::HTTP);
    EXPECT_EQ(ports::parse("Http"), ports::HTTP);
    EXPECT_EQ(ports::parse("hTtP"), ports::HTTP);
}

TEST_F(PortsTest, ParseWs) {
    EXPECT_EQ(ports::parse("ws"), ports::WS);
    EXPECT_EQ(ports::parse("WS"), ports::WS);
    EXPECT_EQ(ports::parse("Ws"), ports::WS);
}

TEST_F(PortsTest, ParseHttps) {
    EXPECT_EQ(ports::parse("https"), ports::HTTPS);
    EXPECT_EQ(ports::parse("HTTPS"), ports::HTTPS);
    EXPECT_EQ(ports::parse("Https"), ports::HTTPS);
}

TEST_F(PortsTest, ParseWss) {
    EXPECT_EQ(ports::parse("wss"), ports::WSS);
    EXPECT_EQ(ports::parse("WSS"), ports::WSS);
    EXPECT_EQ(ports::parse("Wss"), ports::WSS);
}

TEST_F(PortsTest, ParseFtp) {
    EXPECT_EQ(ports::parse("ftp"), ports::FTP);
    EXPECT_EQ(ports::parse("FTP"), ports::FTP);
}

TEST_F(PortsTest, ParseFtpData) {
    EXPECT_EQ(ports::parse("ftp-data"), ports::FTP_DATA);
    EXPECT_EQ(ports::parse("FTP-DATA"), ports::FTP_DATA);
}

TEST_F(PortsTest, ParseTftp) {
    EXPECT_EQ(ports::parse("tftp"), ports::TFTP);
    EXPECT_EQ(ports::parse("TFTP"), ports::TFTP);
}

TEST_F(PortsTest, ParseSsh) {
    EXPECT_EQ(ports::parse("ssh"), ports::SSH);
    EXPECT_EQ(ports::parse("SSH"), ports::SSH);
}

TEST_F(PortsTest, ParseTelnet) {
    EXPECT_EQ(ports::parse("telnet"), ports::TELNET);
    EXPECT_EQ(ports::parse("TELNET"), ports::TELNET);
}

TEST_F(PortsTest, ParseSmtp) {
    EXPECT_EQ(ports::parse("smtp"), ports::SMTP);
    EXPECT_EQ(ports::parse("SMTP"), ports::SMTP);
}

TEST_F(PortsTest, ParseSmtps) {
    EXPECT_EQ(ports::parse("smtps"), ports::SMTPS);
    EXPECT_EQ(ports::parse("SMTPS"), ports::SMTPS);
}

TEST_F(PortsTest, ParseSmtpSubmit) {
    EXPECT_EQ(ports::parse("smtp-submit"), ports::SMTP_SUBMIT);
    EXPECT_EQ(ports::parse("SMTP-SUBMIT"), ports::SMTP_SUBMIT);
    EXPECT_EQ(ports::parse("submission"), ports::SMTP_SUBMIT);
    EXPECT_EQ(ports::parse("SUBMISSION"), ports::SMTP_SUBMIT);
}

TEST_F(PortsTest, ParsePop3) {
    EXPECT_EQ(ports::parse("pop3"), ports::POP3);
    EXPECT_EQ(ports::parse("POP3"), ports::POP3);
}

TEST_F(PortsTest, ParsePop3s) {
    EXPECT_EQ(ports::parse("pop3s"), ports::POP3S);
    EXPECT_EQ(ports::parse("POP3S"), ports::POP3S);
}

TEST_F(PortsTest, ParseImap) {
    EXPECT_EQ(ports::parse("imap"), ports::IMAP);
    EXPECT_EQ(ports::parse("IMAP"), ports::IMAP);
}

TEST_F(PortsTest, ParseImaps) {
    EXPECT_EQ(ports::parse("imaps"), ports::IMAPS);
    EXPECT_EQ(ports::parse("IMAPS"), ports::IMAPS);
}

TEST_F(PortsTest, ParseDns) {
    EXPECT_EQ(ports::parse("dns"), ports::DNS);
    EXPECT_EQ(ports::parse("DNS"), ports::DNS);
}

TEST_F(PortsTest, ParseLdap) {
    EXPECT_EQ(ports::parse("ldap"), ports::LDAP);
    EXPECT_EQ(ports::parse("LDAP"), ports::LDAP);
}

TEST_F(PortsTest, ParseLdaps) {
    EXPECT_EQ(ports::parse("ldaps"), ports::LDAPS);
    EXPECT_EQ(ports::parse("LDAPS"), ports::LDAPS);
}

TEST_F(PortsTest, ParseDhcpServer) {
    EXPECT_EQ(ports::parse("dhcp-server"), ports::DHCP_SERVER);
    EXPECT_EQ(ports::parse("DHCP-SERVER"), ports::DHCP_SERVER);
    EXPECT_EQ(ports::parse("dhcps"), ports::DHCP_SERVER);
    EXPECT_EQ(ports::parse("DHCPS"), ports::DHCP_SERVER);
}

TEST_F(PortsTest, ParseDhcpClient) {
    EXPECT_EQ(ports::parse("dhcp-client"), ports::DHCP_CLIENT);
    EXPECT_EQ(ports::parse("DHCP-CLIENT"), ports::DHCP_CLIENT);
    EXPECT_EQ(ports::parse("dhcpc"), ports::DHCP_CLIENT);
    EXPECT_EQ(ports::parse("DHCPC"), ports::DHCP_CLIENT);
}

TEST_F(PortsTest, ParseNtp) {
    EXPECT_EQ(ports::parse("ntp"), ports::NTP);
    EXPECT_EQ(ports::parse("NTP"), ports::NTP);
}

TEST_F(PortsTest, ParseSnmp) {
    EXPECT_EQ(ports::parse("snmp"), ports::SNMP);
    EXPECT_EQ(ports::parse("SNMP"), ports::SNMP);
}

TEST_F(PortsTest, ParseSnmpTrap) {
    EXPECT_EQ(ports::parse("snmp-trap"), ports::SNMP_TRAP);
    EXPECT_EQ(ports::parse("SNMP-TRAP"), ports::SNMP_TRAP);
    EXPECT_EQ(ports::parse("snmptrap"), ports::SNMP_TRAP);
    EXPECT_EQ(ports::parse("SNMPTRAP"), ports::SNMP_TRAP);
}

TEST_F(PortsTest, ParseSmb) {
    EXPECT_EQ(ports::parse("smb"), ports::SMB);
    EXPECT_EQ(ports::parse("SMB"), ports::SMB);
    EXPECT_EQ(ports::parse("cifs"), ports::SMB);
    EXPECT_EQ(ports::parse("CIFS"), ports::SMB);
}

TEST_F(PortsTest, ParseMysql) {
    EXPECT_EQ(ports::parse("mysql"), ports::MYSQL);
    EXPECT_EQ(ports::parse("MYSQL"), ports::MYSQL);
}

TEST_F(PortsTest, ParsePostgresql) {
    EXPECT_EQ(ports::parse("postgresql"), ports::POSTGRESQL);
    EXPECT_EQ(ports::parse("POSTGRESQL"), ports::POSTGRESQL);
    EXPECT_EQ(ports::parse("postgres"), ports::POSTGRESQL);
    EXPECT_EQ(ports::parse("POSTGRES"), ports::POSTGRESQL);
}

TEST_F(PortsTest, ParseRedis) {
    EXPECT_EQ(ports::parse("redis"), ports::REDIS);
    EXPECT_EQ(ports::parse("REDIS"), ports::REDIS);
}

TEST_F(PortsTest, ParseMongodb) {
    EXPECT_EQ(ports::parse("mongodb"), ports::MONGODB);
    EXPECT_EQ(ports::parse("MONGODB"), ports::MONGODB);
    EXPECT_EQ(ports::parse("mongo"), ports::MONGODB);
    EXPECT_EQ(ports::parse("MONGO"), ports::MONGODB);
}

TEST_F(PortsTest, ParseUnknown) {
    EXPECT_EQ(ports::parse("unknown"), ports::UNDEF);
    EXPECT_EQ(ports::parse(""), ports::UNDEF);
    EXPECT_EQ(ports::parse("xyz"), ports::UNDEF);
    EXPECT_EQ(ports::parse("8080"), ports::UNDEF);
}

TEST_F(PortsTest, ToStringWebSocket) {
    ports http(ports::HTTP);
    EXPECT_EQ(http.to_string(), "http");
    EXPECT_EQ(http.to_string(false), "http");
    EXPECT_EQ(http.to_string(true), "ws");

    ports https(ports::HTTPS);
    EXPECT_EQ(https.to_string(), "https");
    EXPECT_EQ(https.to_string(false), "https");
    EXPECT_EQ(https.to_string(true), "wss");
}

TEST_F(PortsTest, ToStringFtp) {
    ports p(ports::FTP_DATA);
    EXPECT_EQ(p.to_string(), "ftp-data");
    EXPECT_EQ(p.to_string(true), "ftp-data");

    ports ftp(ports::FTP);
    EXPECT_EQ(ftp.to_string(), "ftp");
}

TEST_F(PortsTest, ToStringTftp) {
    ports p(ports::TFTP);
    EXPECT_EQ(p.to_string(), "tftp");
}

TEST_F(PortsTest, ToStringSsh) {
    ports p(ports::SSH);
    EXPECT_EQ(p.to_string(), "ssh");
}

TEST_F(PortsTest, ToStringTelnet) {
    ports p(ports::TELNET);
    EXPECT_EQ(p.to_string(), "telnet");
}

TEST_F(PortsTest, ToStringSmtp) {
    ports p(ports::SMTP);
    EXPECT_EQ(p.to_string(), "smtp");
}

TEST_F(PortsTest, ToStringSmtps) {
    ports p(ports::SMTPS);
    EXPECT_EQ(p.to_string(), "smtps");
}

TEST_F(PortsTest, ToStringSmtpSubmit) {
    ports p(ports::SMTP_SUBMIT);
    EXPECT_EQ(p.to_string(), "smtp-submit");
}

TEST_F(PortsTest, ToStringPop3) {
    ports p(ports::POP3);
    EXPECT_EQ(p.to_string(), "pop3");
}

TEST_F(PortsTest, ToStringPop3s) {
    ports p(ports::POP3S);
    EXPECT_EQ(p.to_string(), "pop3s");
}

TEST_F(PortsTest, ToStringImap) {
    ports p(ports::IMAP);
    EXPECT_EQ(p.to_string(), "imap");
}

TEST_F(PortsTest, ToStringImaps) {
    ports p(ports::IMAPS);
    EXPECT_EQ(p.to_string(), "imaps");
}

TEST_F(PortsTest, ToStringDns) {
    ports p(ports::DNS);
    EXPECT_EQ(p.to_string(), "dns");
}

TEST_F(PortsTest, ToStringLdap) {
    ports p(ports::LDAP);
    EXPECT_EQ(p.to_string(), "ldap");
}

TEST_F(PortsTest, ToStringLdaps) {
    ports p(ports::LDAPS);
    EXPECT_EQ(p.to_string(), "ldaps");
}

TEST_F(PortsTest, ToStringDhcpServer) {
    ports p(ports::DHCP_SERVER);
    EXPECT_EQ(p.to_string(), "dhcp-server");
}

TEST_F(PortsTest, ToStringDhcpClient) {
    ports p(ports::DHCP_CLIENT);
    EXPECT_EQ(p.to_string(), "dhcp-client");
}

TEST_F(PortsTest, ToStringNtp) {
    ports p(ports::NTP);
    EXPECT_EQ(p.to_string(), "ntp");
}

TEST_F(PortsTest, ToStringSnmp) {
    ports p(ports::SNMP);
    EXPECT_EQ(p.to_string(), "snmp");
}

TEST_F(PortsTest, ToStringSnmpTrap) {
    ports p(ports::SNMP_TRAP);
    EXPECT_EQ(p.to_string(), "snmp-trap");
}

TEST_F(PortsTest, ToStringSmb) {
    ports p(ports::SMB);
    EXPECT_EQ(p.to_string(), "smb");
}

TEST_F(PortsTest, ToStringMysql) {
    ports p(ports::MYSQL);
    EXPECT_EQ(p.to_string(), "mysql");
}

TEST_F(PortsTest, ToStringPostgresql) {
    ports p(ports::POSTGRESQL);
    EXPECT_EQ(p.to_string(), "postgresql");
}

TEST_F(PortsTest, ToStringRedis) {
    ports p(ports::REDIS);
    EXPECT_EQ(p.to_string(), "redis");
}

TEST_F(PortsTest, ToStringMongodb) {
    ports p(ports::MONGODB);
    EXPECT_EQ(p.to_string(), "mongodb");
}

TEST_F(PortsTest, ToStringUnknown) {
    ports undef;
    EXPECT_EQ(undef.to_string(), "");

    ports custom(static_cast<uint16_t>(8080));
    EXPECT_EQ(custom.to_string(), "");
}

TEST_F(PortsTest, EqualityOperators) {
    ports lhs(ports::HTTP);
    ports rhs(ports::HTTP);
    ports other(ports::HTTPS);

    EXPECT_TRUE(lhs == rhs);
    EXPECT_FALSE(lhs == other);
    EXPECT_TRUE(lhs != other);
    EXPECT_FALSE(lhs != rhs);

    EXPECT_TRUE(lhs == ports::HTTP);
    EXPECT_FALSE(lhs == ports::HTTPS);
    EXPECT_TRUE(lhs != ports::HTTPS);
    EXPECT_FALSE(lhs != ports::HTTP);

    EXPECT_TRUE(ports::HTTP == lhs);
    EXPECT_FALSE(ports::HTTPS == lhs);
    EXPECT_TRUE(ports::HTTPS != lhs);
    EXPECT_FALSE(ports::HTTP != lhs);

    EXPECT_TRUE(lhs == uint16_t(80));
    EXPECT_FALSE(lhs == uint16_t(443));
    EXPECT_TRUE(lhs != uint16_t(443));
    EXPECT_FALSE(lhs != uint16_t(80));

    EXPECT_TRUE(uint16_t(80) == lhs);
    EXPECT_FALSE(uint16_t(443) == lhs);
    EXPECT_TRUE(uint16_t(443) != lhs);
    EXPECT_FALSE(uint16_t(80) != lhs);
}

TEST_F(PortsTest, EqualityWithUndef) {
    ports undef1;
    ports undef2;

    EXPECT_TRUE(undef1 == undef2);
    EXPECT_TRUE(undef1 == ports::UNDEF);
    EXPECT_TRUE(ports::UNDEF == undef1);
    EXPECT_TRUE(undef1 == uint16_t(0));
    EXPECT_TRUE(uint16_t(0) == undef1);

    EXPECT_FALSE(undef1 != undef2);
}

TEST_F(PortsTest, EqualityWithEnum) {
    ports lhs(ports::SSH);
    EXPECT_TRUE(lhs == ports::raw::SSH);
    EXPECT_FALSE(lhs == ports::raw::TELNET);
    EXPECT_TRUE(lhs != ports::raw::TELNET);
    EXPECT_FALSE(lhs != ports::raw::SSH);

    EXPECT_TRUE(ports::raw::SSH == lhs);
    EXPECT_FALSE(ports::raw::TELNET == lhs);
    EXPECT_TRUE(ports::raw::TELNET != lhs);
    EXPECT_FALSE(ports::raw::SSH != lhs);
}

TEST_F(PortsTest, ConstexprPortValues) {
    constexpr ports http(ports::HTTP);
    constexpr ports undef;
    constexpr ports from_uint(static_cast<uint16_t>(22));

    static_assert(http.port == ports::HTTP);
    static_assert(undef.port == ports::UNDEF);
    static_assert(from_uint.port == ports::SSH);
    static_assert(http.value() == 80u);
    static_assert(undef.value() == 0u);
    static_assert(from_uint.value() == 22u);
}

TEST_F(PortsTest, ConstexprBoolConversion) {
    constexpr ports http(ports::HTTP);
    constexpr ports undef;

    static_assert(static_cast<bool>(http));
    static_assert(!static_cast<bool>(undef));
}

TEST_F(PortsTest, ConstexprRangeChecks) {
    constexpr ports http(ports::HTTP);
    constexpr ports mysql(ports::MYSQL);
    constexpr ports dynamic(static_cast<uint16_t>(60000));

    static_assert(http.is_well_known());
    static_assert(!http.is_registered());
    static_assert(!http.is_dynamic());

    static_assert(!mysql.is_well_known());
    static_assert(mysql.is_registered());
    static_assert(!mysql.is_dynamic());

    static_assert(!dynamic.is_well_known());
    static_assert(!dynamic.is_registered());
    static_assert(dynamic.is_dynamic());
}

TEST_F(PortsTest, ConstexprEquality) {
    constexpr ports lhs(ports::HTTP);
    constexpr ports rhs(ports::HTTP);
    constexpr ports other(ports::HTTPS);

    static_assert(lhs == rhs);
    static_assert(lhs != other);
    static_assert(lhs == uint16_t(80));
    static_assert(lhs != uint16_t(443));
    static_assert(uint16_t(80) == lhs);
}

class UrlTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(UrlTest, DefaultConstructor) {
    url u;
    EXPECT_TRUE(u.scheme.empty());
    EXPECT_TRUE(u.host.empty());
    EXPECT_EQ(u.port, ports::UNDEF);
    EXPECT_TRUE(u.path.empty());
    EXPECT_TRUE(u.query.empty());
    EXPECT_TRUE(u.fragment.empty());
}

TEST_F(UrlTest, MoveConstructor) {
    url u1;
    u1.scheme = "https";
    u1.host = "example.com";
    u1.port = ports::HTTPS;
    u1.path = "/path";
    u1.query = "key=value";
    u1.fragment = "section";

    url u2(std::move(u1));
    EXPECT_EQ(u2.scheme, "https");
    EXPECT_EQ(u2.host, "example.com");
    EXPECT_EQ(u2.port, ports::HTTPS);
    EXPECT_EQ(u2.path, "/path");
    EXPECT_EQ(u2.query, "key=value");
    EXPECT_EQ(u2.fragment, "section");
}

TEST_F(UrlTest, MoveAssignment) {
    url u1;
    u1.scheme = "https";
    u1.host = "example.com";

    url u2;
    u2 = std::move(u1);
    EXPECT_EQ(u2.scheme, "https");
    EXPECT_EQ(u2.host, "example.com");
}

TEST_F(UrlTest, IsValid) {
    url u;
    u.scheme = "https";
    u.host = "example.com";
    u.path = "/";
    EXPECT_TRUE(u.is_valid());

    u.scheme.clear();
    EXPECT_FALSE(u.is_valid());

    u.scheme = "http";
    u.host.clear();
    EXPECT_FALSE(u.is_valid());

    u.host = "example.com";
    u.path.clear();
    EXPECT_FALSE(u.is_valid());

    u.path = "relative/path";
    EXPECT_FALSE(u.is_valid());

    u.path = "/valid/path";
    EXPECT_TRUE(u.is_valid());
}

TEST_F(UrlTest, IsValidEmptyPath) {
    url u;
    u.scheme = "ftp";
    u.host = "ftp.example.com";
    EXPECT_FALSE(u.is_valid());
}

TEST_F(UrlTest, IsValidPathWithoutLeadingSlash) {
    url u;
    u.scheme = "http";
    u.host = "example.com";
    u.path = "relative";
    EXPECT_FALSE(u.is_valid());
}

TEST_F(UrlTest, ParseBasicHttp) {
    url u = url::parse("http://example.com");
    EXPECT_EQ(u.scheme, "http");
    EXPECT_EQ(u.host, "example.com");
    EXPECT_EQ(u.port, ports::HTTP);
    EXPECT_EQ(u.path, "/");
    EXPECT_TRUE(u.query.empty());
    EXPECT_TRUE(u.fragment.empty());
}

TEST_F(UrlTest, ParseBasicHttps) {
    url u = url::parse("https://example.com");
    EXPECT_EQ(u.scheme, "https");
    EXPECT_EQ(u.host, "example.com");
    EXPECT_EQ(u.port, ports::HTTPS);
    EXPECT_EQ(u.path, "/");
}

TEST_F(UrlTest, ParseWithPath) {
    url u = url::parse("http://example.com/path/to/resource");
    EXPECT_EQ(u.scheme, "http");
    EXPECT_EQ(u.host, "example.com");
    EXPECT_EQ(u.port, ports::HTTP);
    EXPECT_EQ(u.path, "/path/to/resource");
    EXPECT_TRUE(u.query.empty());
    EXPECT_TRUE(u.fragment.empty());
}

TEST_F(UrlTest, ParseWithQuery) {
    url u = url::parse("http://example.com/path?key=value&foo=bar");
    EXPECT_EQ(u.scheme, "http");
    EXPECT_EQ(u.host, "example.com");
    EXPECT_EQ(u.path, "/path");
    EXPECT_EQ(u.query, "key=value&foo=bar");
    EXPECT_TRUE(u.fragment.empty());
}

TEST_F(UrlTest, ParseWithFragment) {
    url u = url::parse("http://example.com/path#section");
    EXPECT_EQ(u.scheme, "http");
    EXPECT_EQ(u.host, "example.com");
    EXPECT_EQ(u.path, "/path");
    EXPECT_TRUE(u.query.empty());
    EXPECT_EQ(u.fragment, "section");
}

TEST_F(UrlTest, ParseWithQueryAndFragment) {
    url u = url::parse("http://example.com/path?key=value#section");
    EXPECT_EQ(u.scheme, "http");
    EXPECT_EQ(u.host, "example.com");
    EXPECT_EQ(u.path, "/path");
    EXPECT_EQ(u.query, "key=value");
    EXPECT_EQ(u.fragment, "section");
}

TEST_F(UrlTest, ParseWithExplicitPort) {
    url u = url::parse("http://example.com:8080/path");
    EXPECT_EQ(u.scheme, "http");
    EXPECT_EQ(u.host, "example.com");
    EXPECT_EQ(u.port.value(), 8080u);
    EXPECT_EQ(u.path, "/path");
}

TEST_F(UrlTest, ParseWithStandardPort) {
    url u = url::parse("https://example.com:443/path");
    EXPECT_EQ(u.scheme, "https");
    EXPECT_EQ(u.host, "example.com");
    EXPECT_EQ(u.port, ports::HTTPS);
    EXPECT_EQ(u.path, "/path");
}

TEST_F(UrlTest, ParseIpv4Host) {
    url u = url::parse("http://192.168.1.1:8080/path");
    EXPECT_EQ(u.scheme, "http");
    EXPECT_EQ(u.host, "192.168.1.1");
    EXPECT_EQ(u.port.value(), 8080u);
    EXPECT_EQ(u.path, "/path");
}

TEST_F(UrlTest, ParseIpv6Host) {
    url u = url::parse("http://[::1]:8080/path");
    EXPECT_EQ(u.scheme, "http");
    EXPECT_EQ(u.host, "::1");
    EXPECT_EQ(u.port.value(), 8080u);
    EXPECT_EQ(u.path, "/path");
}

TEST_F(UrlTest, ParseIpv6HostWithoutPort) {
    url u = url::parse("http://[::1]/path");
    EXPECT_EQ(u.scheme, "http");
    EXPECT_EQ(u.host, "::1");
    EXPECT_EQ(u.port, ports::HTTP);
    EXPECT_EQ(u.path, "/path");
}

TEST_F(UrlTest, ParseIpv6HostWithQuery) {
    url u = url::parse("https://[fe80::1%25eth0]:8443/path?q=1");
    EXPECT_EQ(u.scheme, "https");
    EXPECT_EQ(u.host, "fe80::1%25eth0");
    EXPECT_EQ(u.port.value(), 8443u);
    EXPECT_EQ(u.path, "/path");
    EXPECT_EQ(u.query, "q=1");
}

TEST_F(UrlTest, ParseIpv6HostNoPortPathQueryFragment) {
    url u = url::parse("http://[::1]/path?key=val#frag");
    EXPECT_EQ(u.scheme, "http");
    EXPECT_EQ(u.host, "::1");
    EXPECT_EQ(u.port, ports::HTTP);
    EXPECT_EQ(u.path, "/path");
    EXPECT_EQ(u.query, "key=val");
    EXPECT_EQ(u.fragment, "frag");
}

TEST_F(UrlTest, ParseEmptyInput) { EXPECT_THROW(ignore = url::parse(""), network_exception); }

TEST_F(UrlTest, ParseMissingScheme) { EXPECT_THROW(ignore = url::parse("example.com/path"), network_exception); }

TEST_F(UrlTest, ParseMissingHost) { EXPECT_THROW(ignore = url::parse("http://"), network_exception); }

TEST_F(UrlTest, ParseSchemeOnly) { EXPECT_THROW(ignore = url::parse("http://"), network_exception); }

TEST_F(UrlTest, ParseWithColonOnly) { EXPECT_THROW(ignore = url::parse("http:///path"), network_exception); }

TEST_F(UrlTest, ParseUnclosedIpv6Bracket) { EXPECT_THROW(ignore = url::parse("http://[::1/path"), network_exception); }

TEST_F(UrlTest, ParseInvalidPort) {
    EXPECT_THROW(ignore = url::parse("http://example.com:abc/path"), network_exception);
}

TEST_F(UrlTest, ParseFtpScheme) {
    url u = url::parse("ftp://ftp.example.com/files");
    EXPECT_EQ(u.scheme, "ftp");
    EXPECT_EQ(u.host, "ftp.example.com");
    EXPECT_EQ(u.port, ports::FTP);
    EXPECT_EQ(u.path, "/files");
}

TEST_F(UrlTest, ParseWsScheme) {
    url u = url::parse("ws://echo.websocket.org");
    EXPECT_EQ(u.scheme, "ws");
    EXPECT_EQ(u.host, "echo.websocket.org");
    EXPECT_EQ(u.port, ports::WS);
    EXPECT_EQ(u.path, "/");
}

TEST_F(UrlTest, ParseWssScheme) {
    url u = url::parse("wss://echo.websocket.org");
    EXPECT_EQ(u.scheme, "wss");
    EXPECT_EQ(u.host, "echo.websocket.org");
    EXPECT_EQ(u.port, ports::WSS);
    EXPECT_EQ(u.path, "/");
}

TEST_F(UrlTest, ParseSshScheme) {
    url u = url::parse("ssh://user@host.example.com");
    EXPECT_EQ(u.scheme, "ssh");
    EXPECT_EQ(u.host, "user@host.example.com");
    EXPECT_EQ(u.port, ports::SSH);
}

TEST_F(UrlTest, ParseQueryOnly) {
    url u = url::parse("http://example.com?query");
    EXPECT_EQ(u.scheme, "http");
    EXPECT_EQ(u.host, "example.com");
    EXPECT_EQ(u.path, "/");
    EXPECT_EQ(u.query, "query");
    EXPECT_TRUE(u.fragment.empty());
}

TEST_F(UrlTest, ParseFragmentOnly) {
    url u = url::parse("http://example.com#fragment");
    EXPECT_EQ(u.scheme, "http");
    EXPECT_EQ(u.host, "example.com");
    EXPECT_EQ(u.path, "/");
    EXPECT_TRUE(u.query.empty());
    EXPECT_EQ(u.fragment, "fragment");
}

TEST_F(UrlTest, ParsePathEmptyButSlashPresent) {
    url u = url::parse("http://example.com/");
    EXPECT_EQ(u.scheme, "http");
    EXPECT_EQ(u.host, "example.com");
    EXPECT_EQ(u.path, "/");
}

TEST_F(UrlTest, ParseQueryMultipleEquals) {
    url u = url::parse("http://example.com/path?key=val=ue");
    EXPECT_EQ(u.query, "key=val=ue");
}

TEST_F(UrlTest, ParseHostWithUnderscore) {
    url u = url::parse("http://my_host.example.com/path");
    EXPECT_EQ(u.host, "my_host.example.com");
    EXPECT_EQ(u.path, "/path");
}

TEST_F(UrlTest, ParseHostEndingWithPortColon) {
    EXPECT_THROW(ignore = url::parse("http://example.com:/path"), network_exception);
}

TEST_F(UrlTest, ParseQueryAtPosition) {
    url u = url::parse("http://example.com/path?");
    EXPECT_EQ(u.query, "");
}

TEST_F(UrlTest, ParseFragmentAtPosition) {
    url u = url::parse("http://example.com/path#");
    EXPECT_EQ(u.fragment, "");
}

TEST_F(UrlTest, ToStringBasic) {
    url u = url::parse("http://example.com");
    EXPECT_EQ(u.to_string(), "http://example.com/");
}

TEST_F(UrlTest, ToStringWithPath) {
    url u = url::parse("http://example.com/path/to/resource");
    EXPECT_EQ(u.to_string(), "http://example.com/path/to/resource");
}

TEST_F(UrlTest, ToStringOmitDefaultPort) {
    url u = url::parse("http://example.com:80/path");
    EXPECT_EQ(u.to_string(), "http://example.com/path");
}

TEST_F(UrlTest, ToStringOmitHttpsDefaultPort) {
    url u = url::parse("https://example.com:443/path");
    EXPECT_EQ(u.to_string(), "https://example.com/path");
}

TEST_F(UrlTest, ToStringWithNonDefaultPort) {
    url u = url::parse("http://example.com:8080/path");
    EXPECT_EQ(u.to_string(), "http://example.com:8080/path");
}

TEST_F(UrlTest, ToStringWithQuery) {
    url u = url::parse("http://example.com/path?key=value");
    EXPECT_EQ(u.to_string(), "http://example.com/path?key=value");
}

TEST_F(UrlTest, ToStringWithFragment) {
    url u = url::parse("http://example.com/path#section");
    EXPECT_EQ(u.to_string(), "http://example.com/path#section");
}

TEST_F(UrlTest, ToStringWithQueryAndFragment) {
    url u = url::parse("http://example.com/path?key=value#section");
    EXPECT_EQ(u.to_string(), "http://example.com/path?key=value#section");
}

TEST_F(UrlTest, ToStringEmptyPath) {
    url u = url::parse("http://example.com");
    u.path.clear();
    EXPECT_EQ(u.to_string(), "http://example.com/");
}

TEST_F(UrlTest, ToStringUndefPort) {
    url u;
    u.scheme = "custom";
    u.host = "example.com";
    u.port = ports::UNDEF;
    u.path = "/test";
    EXPECT_EQ(u.to_string(), "custom://example.com/test");
}

TEST_F(UrlTest, ToStringNonZeroPortWithUndefDefault) {
    url u = url::parse("http://example.com:8080/path");
    EXPECT_EQ(u.to_string(), "http://example.com:8080/path");
}

TEST_F(UrlTest, EncodeBasic) {
    EXPECT_EQ(url::encode("hello world"), "hello%20world");
    EXPECT_EQ(url::encode("hello%20world"), "hello%2520world");
    EXPECT_EQ(url::encode("test&value"), "test%26value");
    EXPECT_EQ(url::encode("test=value"), "test%3Dvalue");
}

TEST_F(UrlTest, EncodeUnreservedCharacters) {
    EXPECT_EQ(url::encode("ABCDEFGHIJKLMNOPQRSTUVWXYZ"), "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    EXPECT_EQ(url::encode("abcdefghijklmnopqrstuvwxyz"), "abcdefghijklmnopqrstuvwxyz");
    EXPECT_EQ(url::encode("0123456789"), "0123456789");
    EXPECT_EQ(url::encode("-._~"), "-._~");
}

TEST_F(UrlTest, EncodeSlashWithFlag) { EXPECT_EQ(url::encode("/path/to", true), "%2Fpath%2Fto"); }

TEST_F(UrlTest, EncodeSlashWithoutFlag) { EXPECT_EQ(url::encode("/path/to", false), "/path/to"); }

TEST_F(UrlTest, EncodePrintableSpecial) {
    EXPECT_EQ(url::encode("!*'();:@&=+$,?#[]"), "%21%2A%27%28%29%3B%3A%40%26%3D%2B%24%2C%3F%23%5B%5D");
}

TEST_F(UrlTest, EncodeEmptyString) {
    EXPECT_EQ(url::encode(""), "");
    EXPECT_EQ(url::encode("", false), "");
}

TEST_F(UrlTest, EncodeAlreadyEncoded) { EXPECT_EQ(url::encode("%20"), "%2520"); }

TEST_F(UrlTest, DecodeBasic) {
    auto result = url::decode("hello%20world");
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, "hello world");
}

TEST_F(UrlTest, DecodePlusSign) {
    auto result = url::decode("hello+world");
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, "hello world");
}

TEST_F(UrlTest, DecodeMultipleSequences) {
    auto result = url::decode("%48%65%6C%6C%6F");
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, "Hello");
}

TEST_F(UrlTest, DecodeLowercaseHex) {
    auto result = url::decode("%48%65%6c%6c%6f");
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, "Hello");
}

TEST_F(UrlTest, DecodeMixedCaseHex) {
    auto result = url::decode("%4A%6f%68%6E");
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, "John");
}

TEST_F(UrlTest, DecodeEmptyString) {
    auto result = url::decode("");
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, "");
}

TEST_F(UrlTest, DecodePlainString) {
    auto result = url::decode("hello");
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, "hello");
}

TEST_F(UrlTest, DecodeInvalidIncompletePercent) {
    auto result = url::decode("hello%2");
    EXPECT_FALSE(result.has_value());
}

TEST_F(UrlTest, DecodeInvalidTrailingPercent) {
    auto result = url::decode("hello%");
    EXPECT_FALSE(result.has_value());
}

TEST_F(UrlTest, DecodeInvalidHexCharacters) {
    auto result = url::decode("hello%GG");
    EXPECT_FALSE(result.has_value());
}

TEST_F(UrlTest, DecodeInvalidPartialHex) {
    auto result = url::decode("hello%2G");
    EXPECT_FALSE(result.has_value());
}

TEST_F(UrlTest, DecodeNullCharacter) {
    auto result = url::decode("%00");
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ((*result)[0], '\0');
    EXPECT_EQ(result->size(), 1u);
}

TEST_F(UrlTest, EncodeFormBasic) {
    EXPECT_EQ(url::encode_form("hello world"), "hello+world");
    EXPECT_EQ(url::encode_form("test&value"), "test%26value");
    EXPECT_EQ(url::encode_form("test=value"), "test%3Dvalue");
}

TEST_F(UrlTest, EncodeFormUnreservedCharacters) {
    EXPECT_EQ(url::encode_form("ABCDEFGHIJKLMNOPQRSTUVWXYZ"), "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    EXPECT_EQ(url::encode_form("abcdefghijklmnopqrstuvwxyz"), "abcdefghijklmnopqrstuvwxyz");
    EXPECT_EQ(url::encode_form("0123456789"), "0123456789");
    EXPECT_EQ(url::encode_form("-._~"), "-._~");
}

TEST_F(UrlTest, EncodeFormSpecialCharacters) {
    EXPECT_EQ(url::encode_form("!*'();:@&=+$,/?#[]"), "%21%2A%27%28%29%3B%3A%40%26%3D%2B%24%2C%2F%3F%23%5B%5D");
}

TEST_F(UrlTest, EncodeFormEmptyString) { EXPECT_EQ(url::encode_form(""), ""); }

TEST_F(UrlTest, EncodeFormPercentSign) { EXPECT_EQ(url::encode_form("%"), "%25"); }

TEST_F(UrlTest, DecodeTolerantValid) { EXPECT_EQ(url::decode_tolerant("hello%20world"), "hello world"); }

TEST_F(UrlTest, DecodeTolerantPlusSign) { EXPECT_EQ(url::decode_tolerant("hello+world"), "hello world"); }

TEST_F(UrlTest, DecodeTolerantInvalidPercent) { EXPECT_EQ(url::decode_tolerant("hello%2"), "hello%2"); }

TEST_F(UrlTest, DecodeTolerantTrailingPercent) { EXPECT_EQ(url::decode_tolerant("hello%"), "hello%"); }

TEST_F(UrlTest, DecodeTolerantInvalidHex) { EXPECT_EQ(url::decode_tolerant("hello%GG"), "hello%GG"); }

TEST_F(UrlTest, DecodeTolerantMixedValidAndInvalid) {
    EXPECT_EQ(url::decode_tolerant("hello%20world%2"), "hello world%2");
}

TEST_F(UrlTest, DecodeTolerantEmptyString) { EXPECT_EQ(url::decode_tolerant(""), ""); }

TEST_F(UrlTest, DecodeTolerantPlainString) { EXPECT_EQ(url::decode_tolerant("hello"), "hello"); }

TEST_F(UrlTest, ParseQueryBasic) {
    unordered_map<string, string> params;
    url::parse_query("key1=value1&key2=value2", params);
    EXPECT_EQ(params.size(), 2u);
    EXPECT_EQ(params["key1"], "value1");
    EXPECT_EQ(params["key2"], "value2");
}

TEST_F(UrlTest, ParseQuerySinglePair) {
    unordered_map<string, string> params;
    url::parse_query("key=value", params);
    EXPECT_EQ(params.size(), 1u);
    EXPECT_EQ(params["key"], "value");
}

TEST_F(UrlTest, ParseQueryNoEquals) {
    unordered_map<string, string> params;
    url::parse_query("key1&key2=value2", params);
    EXPECT_EQ(params.size(), 1u);
    EXPECT_EQ(params["key2"], "value2");
}

TEST_F(UrlTest, ParseQueryEmptyString) {
    unordered_map<string, string> params;
    url::parse_query("", params);
    EXPECT_TRUE(params.empty());
}

TEST_F(UrlTest, ParseQueryEncodedCharacters) {
    unordered_map<string, string> params;
    url::parse_query("key%20name=value%20data", params);
    EXPECT_EQ(params.size(), 1u);
    EXPECT_EQ(params["key name"], "value data");
}

TEST_F(UrlTest, ParseQueryPlusSignForSpace) {
    unordered_map<string, string> params;
    url::parse_query("key+name=value+data", params);
    EXPECT_EQ(params.size(), 1u);
    EXPECT_EQ(params["key name"], "value data");
}

TEST_F(UrlTest, ParseQueryMultipleSameKey) {
    unordered_map<string, string> params;
    url::parse_query("key=val1&key=val2", params);
    EXPECT_EQ(params.size(), 1u);
}

TEST_F(UrlTest, ParseQueryValueWithEquals) {
    unordered_map<string, string> params;
    url::parse_query("key=val=ue", params);
    EXPECT_EQ(params.size(), 1u);
    EXPECT_EQ(params["key"], "val=ue");
}

TEST_F(UrlTest, ParseQueryEmptyValue) {
    unordered_map<string, string> params;
    url::parse_query("key=", params);
    EXPECT_EQ(params.size(), 1u);
    EXPECT_EQ(params["key"], "");
}

TEST_F(UrlTest, ParseQueryEmptyKey) {
    unordered_map<string, string> params;
    url::parse_query("=value", params);
    EXPECT_EQ(params.size(), 1u);
    EXPECT_EQ(params[""], "value");
}

TEST_F(UrlTest, ParseQueryTrailingSeparator) {
    unordered_map<string, string> params;
    url::parse_query("key=value&", params);
    EXPECT_EQ(params.size(), 1u);
    EXPECT_EQ(params["key"], "value");
}

TEST_F(UrlTest, BuildQueryBasic) {
    unordered_map<string, string> params;
    params["key1"] = "value1";
    params["key2"] = "value2";
    string result = url::build_query(params);
    EXPECT_TRUE(result == "key1=value1&key2=value2" || result == "key2=value2&key1=value1");
}

TEST_F(UrlTest, BuildQuerySinglePair) {
    unordered_map<string, string> params;
    params["key"] = "value";
    EXPECT_EQ(url::build_query(params), "key=value");
}

TEST_F(UrlTest, BuildQueryEmpty) {
    unordered_map<string, string> params;
    EXPECT_EQ(url::build_query(params), "");
}

TEST_F(UrlTest, BuildQueryEncodedCharacters) {
    unordered_map<string, string> params;
    params["key name"] = "value data";
    EXPECT_EQ(url::build_query(params), "key+name=value+data");
}

TEST_F(UrlTest, BuildQuerySpecialCharacters) {
    unordered_map<string, string> params;
    params["key&"] = "val=";
    EXPECT_EQ(url::build_query(params), "key%26=val%3D");
}

TEST_F(UrlTest, RoundtripParseAndToString) {
    string_view original = "https://example.com:8443/path/to/resource?key=value&foo=bar#section";
    url u = url::parse(original);
    string rebuilt = u.to_string();
    EXPECT_EQ(rebuilt, original);
}

TEST_F(UrlTest, RoundtripDefaultPort) {
    string_view original = "http://example.com/path";
    url u = url::parse(original);
    EXPECT_EQ(u.to_string(), original);
}

TEST_F(UrlTest, RoundtripEncodeDecode) {
    string_view original = "hello world!@#";
    string encoded = url::encode(original);
    auto decoded = url::decode(encoded.view());
    EXPECT_TRUE(decoded.has_value());
    EXPECT_EQ(*decoded, original);
}

TEST_F(UrlTest, RoundtripEncodeDecodeSlash) {
    string_view original = "/path/to/resource";
    string encoded = url::encode(original, true);
    auto decoded = url::decode(encoded.view());
    EXPECT_TRUE(decoded.has_value());
    EXPECT_EQ(*decoded, original);
}

TEST_F(UrlTest, RoundtripFormEncodeAndDecode) {
    string_view original = "hello world";
    string encoded = url::encode_form(original);
    string decoded = url::decode_tolerant(encoded.view());
    EXPECT_EQ(decoded, original);
}

TEST_F(UrlTest, RoundtripBuildAndParseQuery) {
    unordered_map<string, string> original;
    original["key1"] = "value1";
    original["key2"] = "value2";
    string query_string = url::build_query(original);

    unordered_map<string, string> parsed;
    url::parse_query(query_string.view(), parsed);
    EXPECT_EQ(parsed.size(), original.size());
    EXPECT_EQ(parsed["key1"], "value1");
    EXPECT_EQ(parsed["key2"], "value2");
}

TEST_F(UrlTest, ParseQueryWithFragmentMarkerNotPresent) {
    unordered_map<string, string> params;
    url::parse_query("key=value", params);
    EXPECT_EQ(params.size(), 1u);
    EXPECT_EQ(params["key"], "value");
}

class IpAddressTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(IpAddressTest, DefaultConstructor) {
    ip_address addr;
    EXPECT_FALSE(addr.is_valid());
    EXPECT_FALSE(addr.is_ipv4());
    EXPECT_FALSE(addr.is_ipv6());
}

TEST_F(IpAddressTest, ConstructorFromSockaddrIn) {
    ::sockaddr_in a4{};
    a4.sin_family = AF_INET;
    a4.sin_port = htons(8080);
    ::inet_pton(AF_INET, "192.168.1.1", &a4.sin_addr);

    ip_address addr(a4);
    EXPECT_TRUE(addr.is_valid());
    EXPECT_TRUE(addr.is_ipv4());
    EXPECT_FALSE(addr.is_ipv6());
    EXPECT_EQ(addr.family(), AF_INET);
    EXPECT_EQ(addr.port(), ports(8080u));
}

TEST_F(IpAddressTest, ConstructorFromSockaddrIn6) {
    ::sockaddr_in6 a6{};
    a6.sin6_family = AF_INET6;
    a6.sin6_port = htons(443);
    ::inet_pton(AF_INET6, "::1", &a6.sin6_addr);

    ip_address addr(a6);
    EXPECT_TRUE(addr.is_valid());
    EXPECT_FALSE(addr.is_ipv4());
    EXPECT_TRUE(addr.is_ipv6());
    EXPECT_EQ(addr.family(), AF_INET6);
    EXPECT_EQ(addr.port(), ports(443u));
}

TEST_F(IpAddressTest, CopyConstructor) {
    ::sockaddr_in a4{};
    a4.sin_family = AF_INET;
    a4.sin_port = htons(80);
    ::inet_pton(AF_INET, "10.0.0.1", &a4.sin_addr);

    ip_address addr1(a4);
    ip_address addr2(addr1);

    EXPECT_TRUE(addr2.is_valid());
    EXPECT_TRUE(addr2.is_ipv4());
    EXPECT_EQ(addr2.family(), AF_INET);
    EXPECT_EQ(addr2.port(), ports(80u));
    EXPECT_EQ(addr1, addr2);
}

TEST_F(IpAddressTest, CopyAssignment) {
    ::sockaddr_in a4{};
    a4.sin_family = AF_INET;
    a4.sin_port = htons(80);
    ::inet_pton(AF_INET, "10.0.0.1", &a4.sin_addr);

    ip_address addr1(a4);
    ip_address addr2;
    addr2 = addr1;

    EXPECT_TRUE(addr2.is_valid());
    EXPECT_TRUE(addr2.is_ipv4());
    EXPECT_EQ(addr1, addr2);
}

TEST_F(IpAddressTest, MoveConstructor) {
    ::sockaddr_in a4{};
    a4.sin_family = AF_INET;
    a4.sin_port = htons(80);
    ::inet_pton(AF_INET, "10.0.0.1", &a4.sin_addr);

    ip_address addr1(a4);
    ip_address addr2(std::move(addr1));

    EXPECT_TRUE(addr2.is_valid());
    EXPECT_TRUE(addr2.is_ipv4());
}

TEST_F(IpAddressTest, MoveAssignment) {
    ::sockaddr_in a4{};
    a4.sin_family = AF_INET;
    a4.sin_port = htons(80);
    ::inet_pton(AF_INET, "10.0.0.1", &a4.sin_addr);

    ip_address addr1(a4);
    ip_address addr2;
    addr2 = std::move(addr1);

    EXPECT_TRUE(addr2.is_valid());
    EXPECT_TRUE(addr2.is_ipv4());
}

TEST_F(IpAddressTest, IsValid) {
    ip_address invalid;
    EXPECT_FALSE(invalid.is_valid());

    auto valid = ip_address::loopback(ports(80u), AF_INET);
    EXPECT_TRUE(valid.is_valid());
}

TEST_F(IpAddressTest, IsIpv4) {
    auto addr = ip_address::loopback(ports(80u), AF_INET);
    EXPECT_TRUE(addr.is_ipv4());
    EXPECT_FALSE(addr.is_ipv6());
}

TEST_F(IpAddressTest, IsIpv6) {
    auto addr = ip_address::loopback(ports(80u), AF_INET6);
    EXPECT_FALSE(addr.is_ipv4());
    EXPECT_TRUE(addr.is_ipv6());
}

TEST_F(IpAddressTest, AnyIpv4) {
    auto addr = ip_address::any(ports(8080u), AF_INET);
    EXPECT_TRUE(addr.is_valid());
    EXPECT_TRUE(addr.is_ipv4());
    EXPECT_EQ(addr.family(), AF_INET);
    EXPECT_EQ(addr.port(), ports(8080u));
    EXPECT_NE(addr.size(), 0);
}

TEST_F(IpAddressTest, AnyIpv6) {
    auto addr = ip_address::any(ports(8443u), AF_INET6);
    EXPECT_TRUE(addr.is_valid());
    EXPECT_TRUE(addr.is_ipv6());
    EXPECT_EQ(addr.family(), AF_INET6);
    EXPECT_EQ(addr.port(), ports(8443u));
    EXPECT_NE(addr.size(), 0);
}

TEST_F(IpAddressTest, AnyDefaultPort) {
    auto addr = ip_address::any(ports::UNDEF, AF_INET);
    EXPECT_TRUE(addr.is_valid());
    EXPECT_EQ(addr.port(), ports::UNDEF);
}

TEST_F(IpAddressTest, AnyDefaultFamily) {
    auto addr = ip_address::any(ports(80u));
    EXPECT_TRUE(addr.is_valid());
    EXPECT_TRUE(addr.is_ipv4());
    EXPECT_EQ(addr.family(), AF_INET);
}

TEST_F(IpAddressTest, LoopbackIpv4) {
    auto addr = ip_address::loopback(ports(22u), AF_INET);
    EXPECT_TRUE(addr.is_valid());
    EXPECT_TRUE(addr.is_ipv4());
    EXPECT_EQ(addr.family(), AF_INET);
    EXPECT_EQ(addr.port(), ports(22u));
    EXPECT_NE(addr.size(), 0);
}

TEST_F(IpAddressTest, LoopbackIpv6) {
    auto addr = ip_address::loopback(ports(443u), AF_INET6);
    EXPECT_TRUE(addr.is_valid());
    EXPECT_TRUE(addr.is_ipv6());
    EXPECT_EQ(addr.family(), AF_INET6);
    EXPECT_EQ(addr.port(), ports(443u));
    EXPECT_NE(addr.size(), 0);
}

TEST_F(IpAddressTest, LoopbackDefaultPort) {
    auto addr = ip_address::loopback(ports::UNDEF, AF_INET);
    EXPECT_TRUE(addr.is_valid());
    EXPECT_EQ(addr.port(), ports::UNDEF);
}

TEST_F(IpAddressTest, LoopbackDefaultFamily) {
    auto addr = ip_address::loopback(ports(53u));
    EXPECT_TRUE(addr.is_valid());
    EXPECT_TRUE(addr.is_ipv4());
    EXPECT_EQ(addr.family(), AF_INET);
}

TEST_F(IpAddressTest, DataConstValidIpv4) {
    auto addr = ip_address::loopback(ports(80u), AF_INET);
    const ::sockaddr* sa = addr.data();
    EXPECT_NE(sa, nullptr);
    EXPECT_EQ(sa->sa_family, AF_INET);
}

TEST_F(IpAddressTest, DataConstValidIpv6) {
    auto addr = ip_address::loopback(ports(80u), AF_INET6);
    const ::sockaddr* sa = addr.data();
    EXPECT_NE(sa, nullptr);
    EXPECT_EQ(sa->sa_family, AF_INET6);
}

TEST_F(IpAddressTest, DataConstInvalid) {
    ip_address addr;
    const ::sockaddr* sa = addr.data();
    EXPECT_EQ(sa, nullptr);
}

TEST_F(IpAddressTest, DataNonConstValid) {
    auto addr = ip_address::loopback(ports(80u), AF_INET);
    ::sockaddr* sa = addr.data();
    EXPECT_NE(sa, nullptr);
    EXPECT_EQ(sa->sa_family, AF_INET);
}

TEST_F(IpAddressTest, DataNonConstInvalid) {
    ip_address addr;
    ::sockaddr* sa = addr.data();
    EXPECT_EQ(sa, nullptr);
}

TEST_F(IpAddressTest, SizeIpv4) {
    auto addr = ip_address::loopback(ports(80u), AF_INET);
    EXPECT_EQ(addr.size(), static_cast<int>(sizeof(::sockaddr_in)));
}

TEST_F(IpAddressTest, SizeIpv6) {
    auto addr = ip_address::loopback(ports(80u), AF_INET6);
    EXPECT_EQ(addr.size(), static_cast<int>(sizeof(::sockaddr_in6)));
}

TEST_F(IpAddressTest, SizeInvalid) {
    ip_address addr;
    EXPECT_EQ(addr.size(), 0);
}

TEST_F(IpAddressTest, AddressAccessorIpv4) {
    auto addr = ip_address::loopback(ports(80u), AF_INET);
    const auto& var = addr.address();
    EXPECT_TRUE(var.holds_alternative<::sockaddr_in>());
    EXPECT_FALSE(var.holds_alternative<::sockaddr_in6>());
    EXPECT_FALSE(var.holds_alternative<none_t>());
}

TEST_F(IpAddressTest, AddressAccessorIpv6) {
    auto addr = ip_address::loopback(ports(80u), AF_INET6);
    const auto& var = addr.address();
    EXPECT_FALSE(var.holds_alternative<::sockaddr_in>());
    EXPECT_TRUE(var.holds_alternative<::sockaddr_in6>());
    EXPECT_FALSE(var.holds_alternative<none_t>());
}

TEST_F(IpAddressTest, AddressAccessorInvalid) {
    ip_address addr;
    const auto& var = addr.address();
    EXPECT_TRUE(var.holds_alternative<none_t>());
}

TEST_F(IpAddressTest, FamilyIpv4) {
    auto addr = ip_address::loopback(ports::UNDEF, AF_INET);
    EXPECT_EQ(addr.family(), AF_INET);
}

TEST_F(IpAddressTest, FamilyIpv6) {
    auto addr = ip_address::loopback(ports::UNDEF, AF_INET6);
    EXPECT_EQ(addr.family(), AF_INET6);
}

TEST_F(IpAddressTest, FamilyInvalid) {
    ip_address addr;
    EXPECT_EQ(addr.family(), AF_UNSPEC);
}

TEST_F(IpAddressTest, PortIpv4) {
    auto addr = ip_address::loopback(ports(25u), AF_INET);
    EXPECT_EQ(addr.port(), ports(25u));
}

TEST_F(IpAddressTest, PortIpv6) {
    auto addr = ip_address::loopback(ports(993u), AF_INET6);
    EXPECT_EQ(addr.port(), ports(993u));
}

TEST_F(IpAddressTest, PortInvalid) {
    ip_address addr;
    EXPECT_EQ(addr.port(), ports::UNDEF);
}

TEST_F(IpAddressTest, ToStringIpv4) {
    auto addr = ip_address::loopback(ports(8080u), AF_INET);
    string str = addr.to_string();
    EXPECT_EQ(str, "127.0.0.1:8080");
}

TEST_F(IpAddressTest, ToStringIpv6) {
    auto addr = ip_address::loopback(ports(443u), AF_INET6);
    string str = addr.to_string();
    EXPECT_EQ(str, "[::1]:443");
}

TEST_F(IpAddressTest, ToStringInvalid) {
    ip_address addr;
    string str = addr.to_string();
    EXPECT_EQ(str, "");
}

TEST_F(IpAddressTest, ToStringAnyIpv4) {
    auto addr = ip_address::any(ports(80u), AF_INET);
    string str = addr.to_string();
    EXPECT_EQ(str, "0.0.0.0:80");
}

TEST_F(IpAddressTest, ToStringAnyIpv6) {
    auto addr = ip_address::any(ports(80u), AF_INET6);
    string str = addr.to_string();
    EXPECT_EQ(str, "[::]:80");
}

TEST_F(IpAddressTest, ParseIpv4) {
    auto result = ip_address::parse("192.168.1.100", ports(9090u));
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(result->is_ipv4());
    EXPECT_EQ(result->port(), ports(9090u));
    EXPECT_EQ(result->to_string(), "192.168.1.100:9090");
}

TEST_F(IpAddressTest, ParseIpv6) {
    auto result = ip_address::parse("::1", ports(22u));
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(result->is_ipv6());
    EXPECT_EQ(result->port(), ports(22u));
    EXPECT_EQ(result->to_string(), "[::1]:22");
}

TEST_F(IpAddressTest, ParseIpv6Full) {
    auto result = ip_address::parse("2001:db8::1", ports(8443u));
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(result->is_ipv6());
    EXPECT_EQ(result->port(), ports(8443u));
}

TEST_F(IpAddressTest, ParseInvalidAddress) {
    auto result = ip_address::parse("invalid_ip", ports(80u));
    EXPECT_FALSE(result.has_value());
}

TEST_F(IpAddressTest, ParseEmptyString) {
    auto result = ip_address::parse("", ports(80u));
    EXPECT_FALSE(result.has_value());
}

TEST_F(IpAddressTest, ParseDomainName) {
    auto result = ip_address::parse("example.com", ports(80u));
    EXPECT_FALSE(result.has_value());
}

TEST_F(IpAddressTest, ParseIpv4DefaultPort) {
    auto result = ip_address::parse("10.0.0.1");
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(result->is_ipv4());
    EXPECT_EQ(result->port(), ports::UNDEF);
}

TEST_F(IpAddressTest, EqualitySameIpv4) {
    auto addr1 = ip_address::parse("192.168.1.1", ports(80u));
    auto addr2 = ip_address::parse("192.168.1.1", ports(80u));
    EXPECT_TRUE(addr1.has_value());
    EXPECT_TRUE(addr2.has_value());
    EXPECT_TRUE(*addr1 == *addr2);
    EXPECT_FALSE(*addr1 != *addr2);
}

TEST_F(IpAddressTest, EqualityDifferentIpv4) {
    auto addr1 = ip_address::parse("192.168.1.1", ports(80u));
    auto addr2 = ip_address::parse("192.168.1.2", ports(80u));
    EXPECT_TRUE(addr1.has_value());
    EXPECT_TRUE(addr2.has_value());
    EXPECT_FALSE(*addr1 == *addr2);
    EXPECT_TRUE(*addr1 != *addr2);
}

TEST_F(IpAddressTest, EqualityDifferentPort) {
    auto addr1 = ip_address::parse("192.168.1.1", ports(80u));
    auto addr2 = ip_address::parse("192.168.1.1", ports(443u));
    EXPECT_TRUE(addr1.has_value());
    EXPECT_TRUE(addr2.has_value());
    EXPECT_FALSE(*addr1 == *addr2);
    EXPECT_TRUE(*addr1 != *addr2);
}

TEST_F(IpAddressTest, EqualityDifferentFamily) {
    auto addr1 = ip_address::parse("127.0.0.1", ports(80u));
    auto addr2 = ip_address::parse("::1", ports(80u));
    EXPECT_TRUE(addr1.has_value());
    EXPECT_TRUE(addr2.has_value());
    EXPECT_FALSE(*addr1 == *addr2);
    EXPECT_TRUE(*addr1 != *addr2);
}

TEST_F(IpAddressTest, EqualitySameIpv6) {
    auto addr1 = ip_address::parse("::1", ports(22u));
    auto addr2 = ip_address::parse("::1", ports(22u));
    EXPECT_TRUE(addr1.has_value());
    EXPECT_TRUE(addr2.has_value());
    EXPECT_TRUE(*addr1 == *addr2);
}

TEST_F(IpAddressTest, EqualityDifferentIpv6) {
    auto addr1 = ip_address::parse("::1", ports(22u));
    auto addr2 = ip_address::parse("::2", ports(22u));
    EXPECT_TRUE(addr1.has_value());
    EXPECT_TRUE(addr2.has_value());
    EXPECT_FALSE(*addr1 == *addr2);
}

TEST_F(IpAddressTest, EqualityBothInvalid) {
    ip_address addr1;
    ip_address addr2;
    EXPECT_TRUE(addr1 == addr2);
    EXPECT_FALSE(addr1 != addr2);
}

TEST_F(IpAddressTest, EqualityOneInvalid) {
    ip_address addr1;
    auto addr2 = ip_address::parse("127.0.0.1", ports(80u));
    EXPECT_TRUE(addr2.has_value());
    EXPECT_FALSE(addr1 == *addr2);
    EXPECT_TRUE(addr1 != *addr2);
    EXPECT_FALSE(*addr2 == addr1);
    EXPECT_TRUE(*addr2 != addr1);
}

TEST_F(IpAddressTest, EqualitySameAddressDifferentObjectOrder) {
    auto addr1 = ip_address::loopback(ports(80u), AF_INET);
    auto addr2 = ip_address::loopback(ports(80u), AF_INET);
    EXPECT_TRUE(addr1 == addr2);
    EXPECT_TRUE(addr2 == addr1);
}

TEST_F(IpAddressTest, AnyInvalidFamily) {
    auto addr = ip_address::any(ports(80u), AF_UNSPEC);
    EXPECT_FALSE(addr.is_valid());
}

TEST_F(IpAddressTest, LoopbackInvalidFamily) {
    auto addr = ip_address::loopback(ports(80u), AF_UNSPEC);
    EXPECT_FALSE(addr.is_valid());
}

TEST_F(IpAddressTest, ToStringIpv4WithoutPort) {
    auto addr = ip_address::parse("192.168.0.1", ports::UNDEF);
    EXPECT_TRUE(addr.has_value());
    EXPECT_EQ(addr->to_string(), "192.168.0.1:0");
}

TEST_F(IpAddressTest, ToStringIpv6WithoutPort) {
    auto addr = ip_address::parse("fe80::1", ports::UNDEF);
    EXPECT_TRUE(addr.has_value());
    EXPECT_EQ(addr->to_string(), "[fe80::1]:0");
}

TEST_F(IpAddressTest, ParseIpv4Loopback) {
    auto result = ip_address::parse("127.0.0.1", ports(5432u));
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(result->is_ipv4());
    EXPECT_EQ(result->to_string(), "127.0.0.1:5432");
}

TEST_F(IpAddressTest, ParseIpv4Broadcast) {
    auto result = ip_address::parse("255.255.255.255", ports(67u));
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(result->is_ipv4());
}

TEST_F(IpAddressTest, EqualityAfterMove) {
    auto addr1 = ip_address::loopback(ports(80u), AF_INET);
    ip_address addr2(std::move(addr1));
    EXPECT_TRUE(addr2.is_valid());
}

TEST_F(IpAddressTest, ParseIpv6LinkLocal) {
    auto result = ip_address::parse("fe80::1", ports(80u));
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(result->is_ipv6());
}

TEST_F(IpAddressTest, ParseIpv6Multicast) {
    auto result = ip_address::parse("ff02::1", ports(5353u));
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(result->is_ipv6());
}

TEST_F(IpAddressTest, EqualityPortComparison) {
    auto addr1 = ip_address::parse("10.0.0.1", ports(80u));
    auto addr2 = ip_address::parse("10.0.0.2", ports(80u));
    EXPECT_TRUE(addr1.has_value());
    EXPECT_TRUE(addr2.has_value());
    EXPECT_FALSE(*addr1 == *addr2);
}

class MacAddressTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(MacAddressTest, DefaultConstructor) {
    mac_address mac;
    const auto& bytes = mac.bytes();
    for (size_t i = 0; i < mac_address::MAC_LEN; ++i) {
        EXPECT_EQ(bytes[i], byte_t(0));
    }
}

TEST_F(MacAddressTest, ConstructorFromPointer) {
    byte_t data[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    mac_address mac(data);
    const auto& bytes = mac.bytes();
    for (size_t i = 0; i < 6; ++i) {
        EXPECT_EQ(bytes[i], data[i]);
    }
}

TEST_F(MacAddressTest, ConstructorFromArray) {
    mac_address::bytes_type data;
    data[0] = byte_t(0xAA);
    data[1] = byte_t(0xBB);
    data[2] = byte_t(0xCC);
    data[3] = byte_t(0xDD);
    data[4] = byte_t(0xEE);
    data[5] = byte_t(0xFF);
    mac_address mac(data);
    const auto& bytes = mac.bytes();
    for (size_t i = 0; i < 6; ++i) {
        EXPECT_EQ(bytes[i], data[i]);
    }
}

TEST_F(MacAddressTest, BytesAccessor) {
    byte_t data[6] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    mac_address mac(data);
    const auto& bytes = mac.bytes();
    EXPECT_EQ(bytes.size(), 6u);
    EXPECT_EQ(bytes[0], byte_t(0x01));
    EXPECT_EQ(bytes[1], byte_t(0x02));
    EXPECT_EQ(bytes[2], byte_t(0x03));
    EXPECT_EQ(bytes[3], byte_t(0x04));
    EXPECT_EQ(bytes[4], byte_t(0x05));
    EXPECT_EQ(bytes[5], byte_t(0x06));
}

TEST_F(MacAddressTest, MacLenConstant) { EXPECT_EQ(mac_address::MAC_LEN, 6u); }

TEST_F(MacAddressTest, ParseValidColonSeparated) {
    auto result = mac_address::parse("00:11:22:33:44:55");
    EXPECT_TRUE(result.has_value());
    const auto& bytes = result->bytes();
    EXPECT_EQ(bytes[0], byte_t(0x00));
    EXPECT_EQ(bytes[1], byte_t(0x11));
    EXPECT_EQ(bytes[2], byte_t(0x22));
    EXPECT_EQ(bytes[3], byte_t(0x33));
    EXPECT_EQ(bytes[4], byte_t(0x44));
    EXPECT_EQ(bytes[5], byte_t(0x55));
}

TEST_F(MacAddressTest, ParseValidDashSeparated) {
    auto result = mac_address::parse("AA-BB-CC-DD-EE-FF");
    EXPECT_TRUE(result.has_value());
    const auto& bytes = result->bytes();
    EXPECT_EQ(bytes[0], byte_t(0xAA));
    EXPECT_EQ(bytes[1], byte_t(0xBB));
    EXPECT_EQ(bytes[2], byte_t(0xCC));
    EXPECT_EQ(bytes[3], byte_t(0xDD));
    EXPECT_EQ(bytes[4], byte_t(0xEE));
    EXPECT_EQ(bytes[5], byte_t(0xFF));
}

TEST_F(MacAddressTest, ParseValidMixedCase) {
    auto result = mac_address::parse("a1:B2:c3:D4:e5:F6");
    EXPECT_TRUE(result.has_value());
    const auto& bytes = result->bytes();
    EXPECT_EQ(bytes[0], byte_t(0xA1));
    EXPECT_EQ(bytes[1], byte_t(0xB2));
    EXPECT_EQ(bytes[2], byte_t(0xC3));
    EXPECT_EQ(bytes[3], byte_t(0xD4));
    EXPECT_EQ(bytes[4], byte_t(0xE5));
    EXPECT_EQ(bytes[5], byte_t(0xF6));
}

TEST_F(MacAddressTest, ParseAllZeros) {
    auto result = mac_address::parse("00:00:00:00:00:00");
    EXPECT_TRUE(result.has_value());
    const auto& bytes = result->bytes();
    for (size_t i = 0; i < 6; ++i) {
        EXPECT_EQ(bytes[i], byte_t(0x00));
    }
}

TEST_F(MacAddressTest, ParseAllOnes) {
    auto result = mac_address::parse("FF:FF:FF:FF:FF:FF");
    EXPECT_TRUE(result.has_value());
    const auto& bytes = result->bytes();
    for (size_t i = 0; i < 6; ++i) {
        EXPECT_EQ(bytes[i], byte_t(0xFF));
    }
}

TEST_F(MacAddressTest, ParseTooShort) {
    auto result = mac_address::parse("00:11:22:33:44");
    EXPECT_FALSE(result.has_value());
}

TEST_F(MacAddressTest, ParseTooLong) {
    auto result = mac_address::parse("00:11:22:33:44:55:66");
    EXPECT_FALSE(result.has_value());
}

TEST_F(MacAddressTest, ParseEmptyString) {
    auto result = mac_address::parse("");
    EXPECT_FALSE(result.has_value());
}

TEST_F(MacAddressTest, ParseInvalidSeparator) {
    auto result = mac_address::parse("00/11/22/33/44/55");
    EXPECT_FALSE(result.has_value());
}

TEST_F(MacAddressTest, ParseMixedSeparators) {
    auto result = mac_address::parse("00:11-22:33-44:55");
    EXPECT_FALSE(result.has_value());
}

TEST_F(MacAddressTest, ParseInvalidHexCharacter) {
    auto result = mac_address::parse("0G:11:22:33:44:55");
    EXPECT_FALSE(result.has_value());
}

TEST_F(MacAddressTest, ParseInvalidLowerHexCharacter) {
    auto result = mac_address::parse("0g:11:22:33:44:55");
    EXPECT_FALSE(result.has_value());
}

TEST_F(MacAddressTest, ParseInvalidSecondDigit) {
    auto result = mac_address::parse("00:1Z:22:33:44:55");
    EXPECT_FALSE(result.has_value());
}

TEST_F(MacAddressTest, ParseInvalidSingleOctet) {
    auto result = mac_address::parse("0:11:22:33:44:55");
    EXPECT_FALSE(result.has_value());
}

TEST_F(MacAddressTest, ParseInvalidThreeOctets) {
    auto result = mac_address::parse("000:11:22:33:44:55");
    EXPECT_FALSE(result.has_value());
}

TEST_F(MacAddressTest, ParseNoSeparators) {
    auto result = mac_address::parse("001122334455");
    EXPECT_FALSE(result.has_value());
}

TEST_F(MacAddressTest, ParseNonHexCharacters) {
    auto result = mac_address::parse("hello world test!!");
    EXPECT_FALSE(result.has_value());
}

TEST_F(MacAddressTest, ToStringLowercaseInput) {
    auto result = mac_address::parse("0a:1b:2c:3d:4e:5f");
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->to_string(), "0A:1B:2C:3D:4E:5F");
}

TEST_F(MacAddressTest, ToStringUppercaseInput) {
    auto result = mac_address::parse("0A:1B:2C:3D:4E:5F");
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->to_string(), "0A:1B:2C:3D:4E:5F");
}

TEST_F(MacAddressTest, ToStringAllZeros) {
    auto result = mac_address::parse("00:00:00:00:00:00");
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->to_string(), "00:00:00:00:00:00");
}

TEST_F(MacAddressTest, ToStringAllOnes) {
    auto result = mac_address::parse("FF:FF:FF:FF:FF:FF");
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->to_string(), "FF:FF:FF:FF:FF:FF");
}

TEST_F(MacAddressTest, ToStringFromDefaultConstructor) {
    mac_address mac;
    EXPECT_EQ(mac.to_string(), "00:00:00:00:00:00");
}

TEST_F(MacAddressTest, ToStringFromPointerConstructor) {
    byte_t data[6] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};
    mac_address mac(data);
    EXPECT_EQ(mac.to_string(), "12:34:56:78:9A:BC");
}

TEST_F(MacAddressTest, EqualitySameAddress) {
    auto mac1 = mac_address::parse("00:11:22:33:44:55");
    auto mac2 = mac_address::parse("00:11:22:33:44:55");
    EXPECT_TRUE(mac1.has_value());
    EXPECT_TRUE(mac2.has_value());
    EXPECT_TRUE(*mac1 == *mac2);
    EXPECT_FALSE(*mac1 != *mac2);
}

TEST_F(MacAddressTest, EqualityDifferentAddress) {
    auto mac1 = mac_address::parse("00:11:22:33:44:55");
    auto mac2 = mac_address::parse("AA:BB:CC:DD:EE:FF");
    EXPECT_TRUE(mac1.has_value());
    EXPECT_TRUE(mac2.has_value());
    EXPECT_FALSE(*mac1 == *mac2);
    EXPECT_TRUE(*mac1 != *mac2);
}

TEST_F(MacAddressTest, EqualityOneByteDifferent) {
    auto mac1 = mac_address::parse("00:11:22:33:44:55");
    auto mac2 = mac_address::parse("00:11:22:33:44:56");
    EXPECT_TRUE(mac1.has_value());
    EXPECT_TRUE(mac2.has_value());
    EXPECT_FALSE(*mac1 == *mac2);
    EXPECT_TRUE(*mac1 != *mac2);
}

TEST_F(MacAddressTest, EqualityDefaultConstructed) {
    mac_address mac1;
    mac_address mac2;
    EXPECT_TRUE(mac1 == mac2);
    EXPECT_FALSE(mac1 != mac2);
}

TEST_F(MacAddressTest, EqualitySamePointerConstructed) {
    byte_t data[6] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    mac_address mac1(data);
    mac_address mac2(data);
    EXPECT_TRUE(mac1 == mac2);
}

TEST_F(MacAddressTest, EqualityDifferentCaseParsed) {
    auto mac1 = mac_address::parse("aa:bb:cc:dd:ee:ff");
    auto mac2 = mac_address::parse("AA:BB:CC:DD:EE:FF");
    EXPECT_TRUE(mac1.has_value());
    EXPECT_TRUE(mac2.has_value());
    EXPECT_TRUE(*mac1 == *mac2);
}

TEST_F(MacAddressTest, InequalityOperator) {
    auto mac1 = mac_address::parse("00:11:22:33:44:55");
    auto mac2 = mac_address::parse("FF:EE:DD:CC:BB:AA");
    EXPECT_TRUE(mac1.has_value());
    EXPECT_TRUE(mac2.has_value());
    EXPECT_TRUE(*mac1 != *mac2);
    EXPECT_FALSE(*mac1 == *mac2);
}

TEST_F(MacAddressTest, RoundtripParseAndToString) {
    string_view original = "01:23:45:67:89:AB";
    auto result = mac_address::parse(original);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->to_string(), original);
}

TEST_F(MacAddressTest, RoundtripDashSeparator) {
    string_view original = "01-23-45-67-89-AB";
    auto result = mac_address::parse(original);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->to_string(), "01:23:45:67:89:AB");
}

TEST_F(MacAddressTest, ParseBroadcastAddress) {
    auto result = mac_address::parse("ff:ff:ff:ff:ff:ff");
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->to_string(), "FF:FF:FF:FF:FF:FF");
}

TEST_F(MacAddressTest, ParseMulticastAddress) {
    auto result = mac_address::parse("01:00:5E:00:00:01");
    EXPECT_TRUE(result.has_value());
    const auto& bytes = result->bytes();
    EXPECT_EQ(bytes[0], byte_t(0x01));
    EXPECT_EQ(bytes[1], byte_t(0x00));
    EXPECT_EQ(bytes[2], byte_t(0x5E));
}

TEST_F(MacAddressTest, ParseLeadingZeros) {
    auto result = mac_address::parse("00:01:02:03:04:05");
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->to_string(), "00:01:02:03:04:05");
}

TEST_F(MacAddressTest, InvalidIpAddressForArpParse) {
    ip_address ip;
    auto result = mac_address::parse(ip);
    EXPECT_FALSE(result.has_value());
}

TEST_F(MacAddressTest, InvalidIpv6AddressForArpParse) {
    auto ip = ip_address::parse("::1", ports::UNDEF);
    EXPECT_TRUE(ip.has_value());
    auto result = mac_address::parse(*ip);
    EXPECT_FALSE(result.has_value());
}

TEST_F(MacAddressTest, CopyAssignment) {
    auto mac1 = mac_address::parse("00:11:22:33:44:55");
    EXPECT_TRUE(mac1.has_value());
    mac_address mac2 = *mac1;
    EXPECT_TRUE(*mac1 == mac2);
}

TEST_F(MacAddressTest, MoveConstructor) {
    auto mac1 = mac_address::parse("00:11:22:33:44:55");
    EXPECT_TRUE(mac1.has_value());
    mac_address mac2(std::move(*mac1));
    EXPECT_EQ(mac2.to_string(), "00:11:22:33:44:55");
}

TEST_F(MacAddressTest, MoveAssignment) {
    auto mac1 = mac_address::parse("AA:BB:CC:DD:EE:FF");
    EXPECT_TRUE(mac1.has_value());
    mac_address mac2;
    mac2 = std::move(*mac1);
    EXPECT_EQ(mac2.to_string(), "AA:BB:CC:DD:EE:FF");
}
