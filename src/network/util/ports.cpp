#include <NeForce/network/util/ports.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    string ports_to_string(const uint16_t port, const bool is_ws) {
        switch (port) {
            case ports::HTTP:
                return is_ws ? "ws" : "http";
            case ports::HTTPS:
                return is_ws ? "wss" : "https";
            case ports::FTP_DATA:
                return "ftp-data";
            case ports::FTP:
                return "ftp";
            case ports::TFTP:
                return "tftp";
            case ports::SSH:
                return "ssh";
            case ports::TELNET:
                return "telnet";
            case ports::SMTP:
                return "smtp";
            case ports::SMTPS:
                return "smtps";
            case ports::SMTP_SUBMIT:
                return "smtp-submit";
            case ports::POP3:
                return "pop3";
            case ports::POP3S:
                return "pop3s";
            case ports::IMAP:
                return "imap";
            case ports::IMAPS:
                return "imaps";
            case ports::DNS:
                return "dns";
            case ports::LDAP:
                return "ldap";
            case ports::LDAPS:
                return "ldaps";
            case ports::DHCP_SERVER:
                return "dhcp-server";
            case ports::DHCP_CLIENT:
                return "dhcp-client";
            case ports::NTP:
                return "ntp";
            case ports::SNMP:
                return "snmp";
            case ports::SNMP_TRAP:
                return "snmp-trap";
            case ports::SMB:
                return "smb";
            case ports::MYSQL:
                return "mysql";
            case ports::POSTGRESQL:
                return "postgresql";
            case ports::REDIS:
                return "redis";
            case ports::MONGODB:
                return "mongodb";
            default:
                return "";
        }
    }
} // namespace


ports ports::parse(const string_view scheme) noexcept {
    if (scheme.compare_ignore_case("http") == 0) {
        return ports::HTTP;
    }
    if (scheme.compare_ignore_case("ws") == 0) {
        return ports::WS;
    }
    if (scheme.compare_ignore_case("https") == 0) {
        return ports::HTTPS;
    }
    if (scheme.compare_ignore_case("wss") == 0) {
        return ports::WSS;
    }
    if (scheme.compare_ignore_case("ftp") == 0) {
        return ports::FTP;
    }
    if (scheme.compare_ignore_case("ftp-data") == 0) {
        return ports::FTP_DATA;
    }
    if (scheme.compare_ignore_case("tftp") == 0) {
        return ports::TFTP;
    }
    if (scheme.compare_ignore_case("ssh") == 0) {
        return ports::SSH;
    }
    if (scheme.compare_ignore_case("telnet") == 0) {
        return ports::TELNET;
    }
    if (scheme.compare_ignore_case("smtp") == 0) {
        return ports::SMTP;
    }
    if (scheme.compare_ignore_case("smtps") == 0) {
        return ports::SMTPS;
    }
    if (scheme.compare_ignore_case("smtp-submit") == 0 || scheme.compare_ignore_case("submission") == 0) {
        return ports::SMTP_SUBMIT;
    }
    if (scheme.compare_ignore_case("pop3") == 0) {
        return ports::POP3;
    }
    if (scheme.compare_ignore_case("pop3s") == 0) {
        return ports::POP3S;
    }
    if (scheme.compare_ignore_case("imap") == 0) {
        return ports::IMAP;
    }
    if (scheme.compare_ignore_case("imaps") == 0) {
        return ports::IMAPS;
    }
    if (scheme.compare_ignore_case("dns") == 0) {
        return ports::DNS;
    }
    if (scheme.compare_ignore_case("ldap") == 0) {
        return ports::LDAP;
    }
    if (scheme.compare_ignore_case("ldaps") == 0) {
        return ports::LDAPS;
    }
    if (scheme.compare_ignore_case("dhcp-server") == 0 || scheme.compare_ignore_case("dhcps") == 0) {
        return ports::DHCP_SERVER;
    }
    if (scheme.compare_ignore_case("dhcp-client") == 0 || scheme.compare_ignore_case("dhcpc") == 0) {
        return ports::DHCP_CLIENT;
    }
    if (scheme.compare_ignore_case("ntp") == 0) {
        return ports::NTP;
    }
    if (scheme.compare_ignore_case("snmp") == 0) {
        return ports::SNMP;
    }
    if (scheme.compare_ignore_case("snmp-trap") == 0 || scheme.compare_ignore_case("snmptrap") == 0) {
        return ports::SNMP_TRAP;
    }
    if (scheme.compare_ignore_case("smb") == 0 || scheme.compare_ignore_case("cifs") == 0) {
        return ports::SMB;
    }
    if (scheme.compare_ignore_case("mysql") == 0) {
        return ports::MYSQL;
    }
    if (scheme.compare_ignore_case("postgresql") == 0 || scheme.compare_ignore_case("postgres") == 0) {
        return ports::POSTGRESQL;
    }
    if (scheme.compare_ignore_case("redis") == 0) {
        return ports::REDIS;
    }
    if (scheme.compare_ignore_case("mongodb") == 0 || scheme.compare_ignore_case("mongo") == 0) {
        return ports::MONGODB;
    }
    return ports::UNDEF;
}

string ports::to_string() const { return ports_to_string(static_cast<uint16_t>(port), false); }

string ports::to_string(const bool is_ws) const { return ports_to_string(static_cast<uint16_t>(port), is_ws); }

NEFORCE_END_NAMESPACE__
