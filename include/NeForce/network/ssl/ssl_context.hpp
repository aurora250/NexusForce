#ifndef NEFORCE_NETWORK_SSL_SSL_CONTEXT_HPP__
#define NEFORCE_NETWORK_SSL_SSL_CONTEXT_HPP__
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/memory/unique_ptr.hpp"
#include "NeForce/network/ssl/ssl_exception.hpp"
#include <openssl/ssl.h>
NEFORCE_BEGIN_NAMESPACE__

enum class ssl_method {
    TLS_SERVER,
    TLS_CLIENT,
    TLS_SERVER_DTLS,
    TLS_CLIENT_DTLS
};


class NEFORCE_API ssl_context {
private:
    struct ctx_deleter {
        void operator()(::SSL_CTX* ctx) const noexcept {
            if (ctx) {
                ::SSL_CTX_free(ctx);
            }
        }
    };

    unique_ptr<::SSL_CTX, ctx_deleter> ctx_;

public:
    explicit ssl_context(ssl_method method = ssl_method::TLS_SERVER);
    ~ssl_context() = default;

    ssl_context(const ssl_context&) = delete;
    ssl_context& operator=(const ssl_context&) = delete;

    ssl_context(ssl_context&& other) noexcept = default;
    ssl_context& operator=(ssl_context&& other) noexcept = default;

    bool load_certificate(const string& cert_file, const string& key_file);
    void load_certificate_from_memory(const string& cert_pem, const string& key_pem);
    bool load_verify_locations(const string& ca_file, const string& ca_path = "");

    void set_options(long options);
    void set_verify_mode(int mode);

    void require_client_certificate();

    void set_cipher_list(const string& ciphers);
    void set_ciphersuites(const string& ciphersuites);

    void set_default_options();
    void set_session_cache_size(long size);
    void set_timeout(long seconds);
    void set_alpn_protos(const vector<string>& protocols);

    NEFORCE_NODISCARD ::SSL_CTX* native_handle() const noexcept { return ctx_.get(); }

    explicit operator bool() const noexcept { return ctx_ != nullptr; }

    NEFORCE_NODISCARD bool is_valid() const noexcept { return ctx_ != nullptr; }
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_SSL_SSL_CONTEXT_HPP__
