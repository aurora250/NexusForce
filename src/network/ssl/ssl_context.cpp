#include <NeForce/network/ssl/ssl_context.hpp>
#ifdef NEFORCE_SUPPORT_OPENSSL
#include <openssl/err.h>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    constexpr auto cipher_list =
            "ECDHE-ECDSA-AES128-GCM-SHA256:"
            "ECDHE-RSA-AES128-GCM-SHA256:"
            "ECDHE-ECDSA-AES256-GCM-SHA384:"
            "ECDHE-RSA-AES256-GCM-SHA384:"
            "ECDHE-ECDSA-CHACHA20-POLY1305:"
            "ECDHE-RSA-CHACHA20-POLY1305:"
            "DHE-RSA-AES128-GCM-SHA256:"
            "DHE-RSA-AES256-GCM-SHA384:"
            "ECDHE-ECDSA-AES128-SHA256:"
            "ECDHE-RSA-AES128-SHA256:"
            "ECDHE-ECDSA-AES256-SHA384:"
            "ECDHE-RSA-AES256-SHA384:"
            "DHE-RSA-AES128-SHA256:"
            "DHE-RSA-AES256-SHA256:"
            "AES128-GCM-SHA256:"
            "AES256-GCM-SHA384:"
            "AES128-SHA256:"
            "AES256-SHA256";

    SSL_METHOD* convert_method(const ssl_method method) {
        switch (method) {
            case ssl_method::TLS_CLIENT: {
                return const_cast<SSL_METHOD*>(TLS_client_method());
            }
            case ssl_method::TLS_SERVER: {
                return const_cast<SSL_METHOD*>(TLS_server_method());
            }
            default: {
                return const_cast<SSL_METHOD*>(TLS_method());
            }
        }
    }
}


int ssl_exception::last_error() noexcept {
    return static_cast<int>(::ERR_get_error());
}

string ssl_exception::last_error_message() {
    char buf[256];
    const auto err = ::ERR_get_error();
    if (err == 0) return "";
    ::ERR_error_string_n(err, buf, sizeof(buf));
    return {buf};
}

ssl_context::ssl_context(const ssl_method method) {
    ctx_ = ::SSL_CTX_new(convert_method(method));
    if (!ctx_) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Failed to create SSL context"));
    }

    ::SSL_CTX_set_options(ctx_.get(),
            SSL_OP_NO_SSLv2 |
            SSL_OP_NO_SSLv3
        );

    if (::SSL_CTX_set_cipher_list(ctx_.get(), cipher_list) != 1) {
        ::SSL_CTX_set_cipher_list(ctx_.get(), "DEFAULT:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!MD5:!PSK");
    }

#ifdef TLS1_3_VERSION
    ::SSL_CTX_set_ciphersuites(ctx_.get(),
        "TLS_AES_128_GCM_SHA256:"
        "TLS_AES_256_GCM_SHA384:"
        "TLS_CHACHA20_POLY1305_SHA256");
#endif

    if (::SSL_CTX_set_default_verify_paths(ctx_.get()) != 1) {
        const char* ca_paths[] = {
            "/etc/ssl/certs",
            "/etc/pki/tls/certs",
            "/usr/local/share/certs",
            "/etc/ssl/cert.pem",
            nullptr
        };

        for (int i = 0; ca_paths[i] != nullptr; ++i) {
            ::SSL_CTX_load_verify_locations(ctx_.get(), nullptr, ca_paths[i]);
        }
    }

    ::SSL_CTX_set_verify(ctx_.get(), SSL_VERIFY_PEER, nullptr);
}

void ssl_context::set_options(const long options) {
    if (ctx_) {
        SSL_CTX_set_options(ctx_.get(), options);
    }
}

bool ssl_context::load_certificate(const string& cert_file, const string& key_file) {
    if (!ctx_) {
        return false;
    }
    if (::SSL_CTX_use_certificate_file(ctx_.get(), cert_file.data(), SSL_FILETYPE_PEM) <= 0) {
        return false;
    }
    if (::SSL_CTX_use_PrivateKey_file(ctx_.get(), key_file.data(), SSL_FILETYPE_PEM) <= 0) {
        return false;
    }
    return SSL_CTX_check_private_key(ctx_.get()) == 1;
}

void ssl_context::load_certificate_from_memory(const string& cert_pem, const string& key_pem) {
    if (!ctx_) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL context is not initialized"));
    }
    if (cert_pem.empty() || key_pem.empty()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Certificate or key PEM data is empty"));
    }

    ::BIO *cert_bio = ::BIO_new_mem_buf(cert_pem.data(), cert_pem.size());
    ::BIO *key_bio = ::BIO_new_mem_buf(key_pem.data(), key_pem.size());

    if (!cert_bio || !key_bio) {
        if (cert_bio) ::BIO_free(cert_bio);
        if (key_bio) ::BIO_free(key_bio);
        NEFORCE_THROW_EXCEPTION(ssl_exception("Failed to create BIO"));
    }

    ::X509* cert = ::PEM_read_bio_X509(cert_bio, nullptr, nullptr, nullptr);
    ::EVP_PKEY* key = ::PEM_read_bio_PrivateKey(key_bio, nullptr, nullptr, nullptr);

    ::BIO_free(cert_bio);
    ::BIO_free(key_bio);

    if (!cert || !key) {
        if (cert) ::X509_free(cert);
        if (key) ::EVP_PKEY_free(key);
        NEFORCE_THROW_EXCEPTION(ssl_exception("Failed to parse PEM data"));
    }

    const int cert_result = ::SSL_CTX_use_certificate(ctx_.get(), cert);
    const int key_result = ::SSL_CTX_use_PrivateKey(ctx_.get(), key);

    ::X509_free(cert);
    ::EVP_PKEY_free(key);

    if (cert_result <= 0 || key_result <= 0) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Failed to set certificate or private key"));
    }
    if (::SSL_CTX_check_private_key(ctx_.get()) != 1) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Private key does not match certificate"));
    }
}

bool ssl_context::load_verify_locations(const string& ca_file, const string& ca_path) {
    if (!ctx_) {
        return false;
    }

    const char* file_ptr = ca_file.empty() ? nullptr : ca_file.data();
    const char* path_ptr = ca_path.empty() ? nullptr : ca_path.data();

    return SSL_CTX_load_verify_locations(ctx_.get(), file_ptr, path_ptr) == 1;
}

void ssl_context::set_verify_mode(const int mode) {
    if (ctx_) {
        SSL_CTX_set_verify(ctx_.get(), mode, nullptr);
    }
}

void ssl_context::require_client_certificate() {
    ::SSL_CTX_set_verify(ctx_.get(), SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
}

void ssl_context::set_cipher_list(const string& ciphers) {
    if (::SSL_CTX_set_cipher_list(ctx_.get(), ciphers.data()) <= 0) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Failed to set cipher list"));
    }
}

void ssl_context::set_ciphersuites(const string& ciphersuites) {
    if (::SSL_CTX_set_ciphersuites(ctx_.get(), ciphersuites.data()) <= 0) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Failed to set ciphersuites"));
    }
}

void ssl_context::set_default_options() {
    if (!ctx_) return;

    long options = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1;
    options |= SSL_OP_SINGLE_DH_USE | SSL_OP_SINGLE_ECDH_USE;
    options |= SSL_OP_CIPHER_SERVER_PREFERENCE;
    options |= SSL_OP_NO_COMPRESSION;

    ::SSL_CTX_set_options(ctx_.get(), options);
    ::SSL_CTX_set_min_proto_version(ctx_.get(), TLS1_2_VERSION);

    set_cipher_list("HIGH:!aNULL:!eNULL:!EXPORT:!DES:!MD5:!PSK:!RC4");
    set_ciphersuites("TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256");
}

void ssl_context::set_session_cache_size(long size) {
    ::SSL_CTX_sess_set_cache_size(ctx_.get(), size);
}

void ssl_context::set_timeout(long seconds) {
    ::SSL_CTX_set_timeout(ctx_.get(), seconds);
}

void ssl_context::set_alpn_protos(const vector<string>& protocols) {
    if (!ctx_) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL context is not initialized"));
    }
    if (protocols.empty()) return;

    byte_vector alpn_data;
    for (const auto& proto : protocols) {
        if (proto.empty() || proto.size() > 255) {
            NEFORCE_THROW_EXCEPTION(value_exception("Invalid ALPN protocol length"));
        }
        alpn_data.push_back(static_cast<byte_t>(proto.size()));
        alpn_data.insert(alpn_data.end(), proto.begin(), proto.end());
    }

    if (::SSL_CTX_set_alpn_protos(ctx_.get(), alpn_data.data(), static_cast<unsigned int>(alpn_data.size())) != 0) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Failed to set ALPN protocols"));
    }
}

NEFORCE_END_NAMESPACE__
#endif
