#include <NeForce/network/ssl/sni_manager.hpp>
#include <openssl/ssl.h>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    bool match_wildcard(const string& pattern, const string& hostname) {
        if (pattern.size() < 3 || pattern[0] != '*' || pattern[1] != '.') {
            return false;
        }

        const string_view suffix = pattern.view(1);
        if (hostname.size() < suffix.size()) {
            return false;
        }
        return hostname.view(hostname.size() - suffix.size()) == suffix;
    }
} // namespace


void sni_manager::add_host(const string& hostname, ssl_context ctx) { hosts_[hostname.lowercase()] = move(ctx); }

void sni_manager::remove_host(const string& hostname) { hosts_.erase(hostname.lowercase()); }

void sni_manager::set_default_context(ssl_context ctx) {
    default_context_ = move(ctx);
    has_default_ = true;
}

bool sni_manager::has_host(const string& hostname) const { return hosts_.find(hostname.lowercase()) != hosts_.end(); }

void* sni_manager::select_ssl_ctx(const string& server_name) const {
    if (server_name.empty()) {
        return has_default_ ? default_context_.native_handle() : nullptr;
    }

    auto it = hosts_.find(string(server_name).lowercase());
    if (it != hosts_.end()) {
        return it->second.native_handle();
    }

    for (const auto& pair: hosts_) {
        if (match_wildcard(pair.first, server_name)) {
            return pair.second.native_handle();
        }
    }

    if (has_default_) {
        return default_context_.native_handle();
    }

    return nullptr;
}

int sni_manager::sni_callback(void* ssl, int* /*alert*/, void* arg) {
    auto* manager = static_cast<sni_manager*>(arg);
    if (manager == nullptr) {
        return SSL_TLSEXT_ERR_OK;
    }

    const char* server_name = SSL_get_servername(static_cast<SSL*>(ssl), TLSEXT_NAMETYPE_host_name);
    if (server_name == nullptr) {
        return SSL_TLSEXT_ERR_OK;
    }

    void* ctx = manager->select_ssl_ctx(string(server_name));
    if (ctx != nullptr) {
        SSL_set_SSL_CTX(static_cast<SSL*>(ssl), static_cast<SSL_CTX*>(ctx));
    }
    return SSL_TLSEXT_ERR_OK;
}

NEFORCE_END_NAMESPACE__
