#include <NeForce/network/ports.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    string ports_to_string(const uint16_t port, const bool is_ws) {
        switch (port) {
            case ports::http: {
                return is_ws ? "ws" : "http";
            }
            case ports::https: {
                return is_ws ? "wss" : "https";
            }
            case ports::ftp: {
                return "ftp";
            }
            case ports::ssh: {
                return "ssh";
            }
            case ports::telnet: {
                return "telnet";
            }
            case ports::smtp: {
                return "smtp";
            }
            case ports::dns: {
                return "dns";
            }
            case ports::pop3: {
                return "pop3";
            }
            case ports::imap: {
                return "imap";
            }
            default: {
                return "";
            }
        }
    }
} // namespace


ports ports::parse(const string_view scheme) noexcept {
    if (scheme == "http") {
        return ports::http;
    }
    if (scheme == "ws") {
        return ports::ws;
    }
    if (scheme == "https") {
        return ports::https;
    }
    if (scheme == "wss") {
        return ports::wss;
    }
    if (scheme == "ftp") {
        return ports::ftp;
    }
    if (scheme == "ssh") {
        return ports::ssh;
    }
    if (scheme == "telnet") {
        return ports::telnet;
    }
    if (scheme == "smtp") {
        return ports::smtp;
    }
    if (scheme == "dns") {
        return ports::dns;
    }
    if (scheme == "pop3") {
        return ports::pop3;
    }
    if (scheme == "imap") {
        return ports::imap;
    }
    return ports::undef;
}

string ports::to_string() const { return ports_to_string(port, false); }

string ports::to_string(const bool is_ws) const { return ports_to_string(port, is_ws); }


NEFORCE_END_NAMESPACE__
