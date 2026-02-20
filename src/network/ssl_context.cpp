#include <MSTL/network/ssl_context.hpp>
#ifdef MSTL_SUPPORT_OPENSSL__
#include <MSTL/core/system/console.hpp>
#include <openssl/err.h>
MSTL_BEGIN_NAMESPACE__

ssl_context::~ssl_context() {
    if (ctx_) ::SSL_CTX_free(ctx_);
}

bool ssl_context::load_certificate(const string& cert_file, const string& key_file) {
    ctx_ = ::SSL_CTX_new(::TLS_server_method());
    if (!ctx_) {
        printcln(color::red(), "SSL_CTX_new failed: ", ::ERR_error_string(::ERR_get_error(), nullptr));
        return false;
    }

    if (::SSL_CTX_use_certificate_file(ctx_, cert_file.data(), SSL_FILETYPE_PEM) <= 0) {
        printcln(color::red(), "Certificate load failed: ", ::ERR_error_string(::ERR_get_error(), nullptr));
        ::SSL_CTX_free(ctx_);
        ctx_ = nullptr;
        return false;
    }

    if (::SSL_CTX_use_PrivateKey_file(ctx_, key_file.data(), SSL_FILETYPE_PEM) <= 0) {
        printcln(color::red(), "Private key load failed: ", ::ERR_error_string(::ERR_get_error(), nullptr));
        ::SSL_CTX_free(ctx_);
        ctx_ = nullptr;
        return false;
    }

    if (!::SSL_CTX_check_private_key(ctx_)) {
        printcln(color::red(), "Private key does not match certificate");
        ::SSL_CTX_free(ctx_);
        ctx_ = nullptr;
        return false;
    }

    ::SSL_CTX_set_ciphersuites(ctx_, "TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256");
    ::SSL_CTX_set_options(ctx_, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1);
    return true;
}

MSTL_END_NAMESPACE__
#endif
