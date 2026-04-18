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
    if (scheme.compare_ignore_case("http")) {
        return ports::HTTP;
    }
    if (scheme.compare_ignore_case("ws")) {
        return ports::WS;
    }
    if (scheme.compare_ignore_case("https")) {
        return ports::HTTPS;
    }
    if (scheme.compare_ignore_case("wss")) {
        return ports::WSS;
    }
    if (scheme.compare_ignore_case("ftp")) {
        return ports::FTP;
    }
    if (scheme.compare_ignore_case("ftp-data")) {
        return ports::FTP_DATA;
    }
    if (scheme.compare_ignore_case("tftp")) {
        return ports::TFTP;
    }
    if (scheme.compare_ignore_case("ssh")) {
        return ports::SSH;
    }
    if (scheme.compare_ignore_case("telnet")) {
        return ports::TELNET;
    }
    if (scheme.compare_ignore_case("smtp")) {
        return ports::SMTP;
    }
    if (scheme.compare_ignore_case("smtps")) {
        return ports::SMTPS;
    }
    if (scheme.compare_ignore_case("smtp-submit") || scheme.compare_ignore_case("submission")) {
        return ports::SMTP_SUBMIT;
    }
    if (scheme.compare_ignore_case("pop3")) {
        return ports::POP3;
    }
    if (scheme.compare_ignore_case("pop3s")) {
        return ports::POP3S;
    }
    if (scheme.compare_ignore_case("imap")) {
        return ports::IMAP;
    }
    if (scheme.compare_ignore_case("imaps")) {
        return ports::IMAPS;
    }
    if (scheme.compare_ignore_case("dns")) {
        return ports::DNS;
    }
    if (scheme.compare_ignore_case("ldap")) {
        return ports::LDAP;
    }
    if (scheme.compare_ignore_case("ldaps")) {
        return ports::LDAPS;
    }
    if (scheme.compare_ignore_case("dhcp-server") || scheme.compare_ignore_case("dhcps")) {
        return ports::DHCP_SERVER;
    }
    if (scheme.compare_ignore_case("dhcp-client") || scheme.compare_ignore_case("dhcpc")) {
        return ports::DHCP_CLIENT;
    }
    if (scheme.compare_ignore_case("ntp")) {
        return ports::NTP;
    }
    if (scheme.compare_ignore_case("snmp")) {
        return ports::SNMP;
    }
    if (scheme.compare_ignore_case("snmp-trap") || scheme.compare_ignore_case("snmptrap")) {
        return ports::SNMP_TRAP;
    }
    if (scheme.compare_ignore_case("smb") || scheme.compare_ignore_case("cifs")) {
        return ports::SMB;
    }
    if (scheme.compare_ignore_case("mysql")) {
        return ports::MYSQL;
    }
    if (scheme.compare_ignore_case("postgresql") || scheme.compare_ignore_case("postgres")) {
        return ports::POSTGRESQL;
    }
    if (scheme.compare_ignore_case("redis")) {
        return ports::REDIS;
    }
    if (scheme.compare_ignore_case("mongodb") || scheme.compare_ignore_case("mongo")) {
        return ports::MONGODB;
    }
    return ports::UNDEF;
}

string ports::to_string() const { return ports_to_string(static_cast<uint16_t>(port), false); }

string ports::to_string(const bool is_ws) const { return ports_to_string(static_cast<uint16_t>(port), is_ws); }

NEFORCE_END_NAMESPACE__
