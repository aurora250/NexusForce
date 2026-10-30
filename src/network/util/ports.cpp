#include <NeForce/network/util/ports.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    string ports_to_string(const uint16_t port, const bool is_ws) {
        switch (port) {
            case ports::http:
                return is_ws ? "ws" : "http";
            case ports::https:
                return is_ws ? "wss" : "https";
            case ports::ftp_data:
                return "ftp-data";
            case ports::ftp:
                return "ftp";
            case ports::tftp:
                return "tftp";
            case ports::ssh:
                return "ssh";
            case ports::telnet:
                return "telnet";
            case ports::smtp:
                return "smtp";
            case ports::smtps:
                return "smtps";
            case ports::smtp_submit:
                return "smtp-submit";
            case ports::pop3:
                return "pop3";
            case ports::pop3s:
                return "pop3s";
            case ports::imap:
                return "imap";
            case ports::imaps:
                return "imaps";
            case ports::dns:
                return "dns";
            case ports::ldap:
                return "ldap";
            case ports::ldaps:
                return "ldaps";
            case ports::dhcp_server:
                return "dhcp-server";
            case ports::dhcp_client:
                return "dhcp-client";
            case ports::ntp:
                return "ntp";
            case ports::snmp:
                return "snmp";
            case ports::snmp_trap:
                return "snmp-trap";
            case ports::smb:
                return "smb";
            case ports::mysql:
                return "mysql";
            case ports::postgresql:
                return "postgresql";
            case ports::redis:
                return "redis";
            case ports::mongodb:
                return "mongodb";
            default:
                return "";
        }
    }
} // namespace


ports ports::parse(const string_view scheme) noexcept {
    if (scheme.compare_ignore_case("http") == 0) {
        return ports::http;
    }
    if (scheme.compare_ignore_case("ws") == 0) {
        return ports::ws;
    }
    if (scheme.compare_ignore_case("https") == 0) {
        return ports::https;
    }
    if (scheme.compare_ignore_case("wss") == 0) {
        return ports::wss;
    }
    if (scheme.compare_ignore_case("ftp") == 0) {
        return ports::ftp;
    }
    if (scheme.compare_ignore_case("ftp-data") == 0) {
        return ports::ftp_data;
    }
    if (scheme.compare_ignore_case("tftp") == 0) {
        return ports::tftp;
    }
    if (scheme.compare_ignore_case("ssh") == 0) {
        return ports::ssh;
    }
    if (scheme.compare_ignore_case("telnet") == 0) {
        return ports::telnet;
    }
    if (scheme.compare_ignore_case("smtp") == 0) {
        return ports::smtp;
    }
    if (scheme.compare_ignore_case("smtps") == 0) {
        return ports::smtps;
    }
    if (scheme.compare_ignore_case("smtp-submit") == 0 || scheme.compare_ignore_case("submission") == 0) {
        return ports::smtp_submit;
    }
    if (scheme.compare_ignore_case("pop3") == 0) {
        return ports::pop3;
    }
    if (scheme.compare_ignore_case("pop3s") == 0) {
        return ports::pop3s;
    }
    if (scheme.compare_ignore_case("imap") == 0) {
        return ports::imap;
    }
    if (scheme.compare_ignore_case("imaps") == 0) {
        return ports::imaps;
    }
    if (scheme.compare_ignore_case("dns") == 0) {
        return ports::dns;
    }
    if (scheme.compare_ignore_case("ldap") == 0) {
        return ports::ldap;
    }
    if (scheme.compare_ignore_case("ldaps") == 0) {
        return ports::ldaps;
    }
    if (scheme.compare_ignore_case("dhcp-server") == 0 || scheme.compare_ignore_case("dhcps") == 0) {
        return ports::dhcp_server;
    }
    if (scheme.compare_ignore_case("dhcp-client") == 0 || scheme.compare_ignore_case("dhcpc") == 0) {
        return ports::dhcp_client;
    }
    if (scheme.compare_ignore_case("ntp") == 0) {
        return ports::ntp;
    }
    if (scheme.compare_ignore_case("snmp") == 0) {
        return ports::snmp;
    }
    if (scheme.compare_ignore_case("snmp-trap") == 0 || scheme.compare_ignore_case("snmptrap") == 0) {
        return ports::snmp_trap;
    }
    if (scheme.compare_ignore_case("smb") == 0 || scheme.compare_ignore_case("cifs") == 0) {
        return ports::smb;
    }
    if (scheme.compare_ignore_case("mysql") == 0) {
        return ports::mysql;
    }
    if (scheme.compare_ignore_case("postgresql") == 0 || scheme.compare_ignore_case("postgres") == 0) {
        return ports::postgresql;
    }
    if (scheme.compare_ignore_case("redis") == 0) {
        return ports::redis;
    }
    if (scheme.compare_ignore_case("mongodb") == 0 || scheme.compare_ignore_case("mongo") == 0) {
        return ports::mongodb;
    }
    return ports::undef;
}

string ports::to_string() const { return ports_to_string(static_cast<uint16_t>(port), false); }

string ports::to_string(const bool is_ws) const { return ports_to_string(static_cast<uint16_t>(port), is_ws); }

NEFORCE_END_NAMESPACE__
