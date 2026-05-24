#include <NeForce/network/ssl/ssl_context.hpp>
#include <NeForce/core/system/pipe.hpp>
#include <openssl/err.h>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    struct ssl_module_init {
        ssl_module_init() noexcept { pipe::ignore_sigpipe(); }
    };

    ssl_module_init ssl_init_instance{};

    constexpr auto cipher_list = "ECDHE-ECDSA-AES128-GCM-SHA256:"
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

    const ::SSL_METHOD* convert_method(const ssl_method method) {
        switch (method) {
            case ssl_method::TLS_CLIENT:
                return ::TLS_client_method();
            case ssl_method::TLS_SERVER:
                return ::TLS_server_method();
#ifdef OPENSSL_NO_DTLS
            case ssl_method::TLS_SERVER_DTLS:
            case ssl_method::TLS_CLIENT_DTLS:
                return ::TLS_method();
#else
            case ssl_method::TLS_SERVER_DTLS:
                return ::DTLS_server_method();
            case ssl_method::TLS_CLIENT_DTLS:
                return ::DTLS_client_method();
#endif
            default:
                return ::TLS_method();
        }
    }

    // NOLINTNEXTLINE(cppcoreguidelines-special-member-functions,hicpp-special-member-functions)
    struct bio_guard {
        ::BIO* bio{nullptr};

        explicit bio_guard(::BIO* b) :
        bio(b) {}
        ~bio_guard() {
            if (bio != nullptr) {
                ::BIO_free(bio);
            }
        }

        bio_guard(const bio_guard&) = delete;
        bio_guard& operator=(const bio_guard&) = delete;

        NEFORCE_NODISCARD ::BIO* get() const noexcept { return bio; }
        explicit operator bool() const noexcept { return bio != nullptr; }
    };

    // NOLINTNEXTLINE(cppcoreguidelines-special-member-functions,hicpp-special-member-functions)
    struct x509_guard {
        ::X509* cert{nullptr};

        explicit x509_guard(::X509* c) :
        cert(c) {}
        ~x509_guard() {
            if (cert != nullptr) {
                ::X509_free(cert);
            }
        }

        x509_guard(const x509_guard&) = delete;
        x509_guard& operator=(const x509_guard&) = delete;

        NEFORCE_NODISCARD ::X509* get() const noexcept { return cert; }
        explicit operator bool() const noexcept { return cert != nullptr; }
    };

    // NOLINTNEXTLINE(cppcoreguidelines-special-member-functions,hicpp-special-member-functions)
    struct evp_pkey_guard {
        ::EVP_PKEY* key{nullptr};

        explicit evp_pkey_guard(::EVP_PKEY* k) :
        key(k) {}
        ~evp_pkey_guard() {
            if (key != nullptr) {
                ::EVP_PKEY_free(key);
            }
        }

        evp_pkey_guard(const evp_pkey_guard&) = delete;
        evp_pkey_guard& operator=(const evp_pkey_guard&) = delete;

        NEFORCE_NODISCARD ::EVP_PKEY* get() const noexcept { return key; }
        explicit operator bool() const noexcept { return key != nullptr; }
    };
} // namespace


int ssl_exception::last_error() noexcept { return static_cast<int>(::ERR_peek_last_error()); }

string ssl_exception::last_error_message() {
    char buf[256];
    const auto err = ::ERR_peek_last_error();
    if (err == 0) {
        return "";
    }
    ::ERR_error_string_n(err, static_cast<char*>(buf), sizeof(buf));
    return {static_cast<char*>(buf)};
}

ssl_context::ssl_context(const ssl_method method) :
method_(method) {
    ctx_.reset(::SSL_CTX_new(convert_method(method)));
    if (!ctx_) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Failed to create SSL context"));
    }

    ::SSL_CTX_set_options(ctx_.get(), SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);

    if (::SSL_CTX_set_cipher_list(ctx_.get(), cipher_list) != 1) {
        ::SSL_CTX_set_cipher_list(ctx_.get(), "DEFAULT:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!MD5:!PSK");
    }

#ifdef TLS1_3_VERSION
    ::SSL_CTX_set_ciphersuites(ctx_.get(), "TLS_AES_128_GCM_SHA256:"
                                           "TLS_AES_256_GCM_SHA384:"
                                           "TLS_CHACHA20_POLY1305_SHA256");
#endif

    if (method == ssl_method::TLS_CLIENT || method == ssl_method::TLS_CLIENT_DTLS) {
        const bool ca_loaded = (::SSL_CTX_set_default_verify_paths(ctx_.get()) == 1);
        if (!ca_loaded) {
            static constexpr const char* ca_paths[] = {"/etc/ssl/certs", "/etc/pki/tls/certs", "/usr/local/share/certs",
                                                       "/etc/ssl/cert.pem"};

            for (const auto& path: ca_paths) {
                if (::SSL_CTX_load_verify_locations(ctx_.get(), nullptr, path) == 1) {
                    break;
                }
            }
        }

        ::SSL_CTX_set_verify(ctx_.get(), SSL_VERIFY_PEER, nullptr);
    }
}

ssl_context ssl_context::clone() const {
    if (!ctx_) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Cannot clone an invalid SSL context"));
    }

    if (::SSL_CTX_up_ref(ctx_.get()) != 1) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Failed to increment SSL_CTX reference count"));
    }

    ssl_context cloned(method_, nullptr);
    cloned.ctx_.reset(ctx_.get());
    cloned.cert_loaded_ = cert_loaded_;
    return cloned;
}

shared_ptr<ssl_context> ssl_context::clone_shared() const { return make_shared<ssl_context>(clone()); }

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
    if (::SSL_CTX_check_private_key(ctx_.get()) != 1) {
        return false;
    }
    cert_loaded_ = true;
    return true;
}

void ssl_context::load_certificate_from_memory(const string& cert_pem, const string& key_pem) {
    if (!ctx_) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL context is not initialized"));
    }
    if (cert_pem.empty() || key_pem.empty()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Certificate or key PEM data is empty"));
    }

    bio_guard cert_bio{::BIO_new_mem_buf(cert_pem.data(), static_cast<int>(cert_pem.size()))};
    bio_guard key_bio{::BIO_new_mem_buf(key_pem.data(), static_cast<int>(key_pem.size()))};

    if (!cert_bio || !key_bio) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Failed to create BIO"));
    }

    x509_guard cert{::PEM_read_bio_X509(cert_bio.get(), nullptr, nullptr, nullptr)};
    evp_pkey_guard key{::PEM_read_bio_PrivateKey(key_bio.get(), nullptr, nullptr, nullptr)};

    if (!cert || !key) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Failed to parse PEM data"));
    }

    const int cert_result = ::SSL_CTX_use_certificate(ctx_.get(), cert.get());
    const int key_result = ::SSL_CTX_use_PrivateKey(ctx_.get(), key.get());

    if (cert_result <= 0 || key_result <= 0) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Failed to set certificate or private key"));
    }
    if (::SSL_CTX_check_private_key(ctx_.get()) != 1) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Private key does not match certificate"));
    }
    cert_loaded_ = true;
}

bool ssl_context::load_verify_locations(const string& ca_file, const string& ca_path) {
    if (!ctx_) {
        return false;
    }

    const char* file_ptr = ca_file.empty() ? nullptr : ca_file.data();
    const char* path_ptr = ca_path.empty() ? nullptr : ca_path.data();

    if (file_ptr == nullptr && path_ptr == nullptr) {
        return false;
    }

    return ::SSL_CTX_load_verify_locations(ctx_.get(), file_ptr, path_ptr) == 1;
}

void ssl_context::set_options(const long options) {
    if (!ctx_) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL context is not initialized"));
    }
    ::SSL_CTX_set_options(ctx_.get(), options);
}

void ssl_context::set_verify_mode(const int mode) {
    if (!ctx_) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL context is not initialized"));
    }
    ::SSL_CTX_set_verify(ctx_.get(), mode, nullptr);
}

void ssl_context::require_client_certificate() {
    if (!ctx_) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL context is not initialized"));
    }
    ::SSL_CTX_set_verify(ctx_.get(), SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
}

void ssl_context::set_cipher_list(const string& ciphers) {
    if (!ctx_) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL context is not initialized"));
    }
    if (::SSL_CTX_set_cipher_list(ctx_.get(), ciphers.data()) <= 0) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Failed to set cipher list"));
    }
}

void ssl_context::set_ciphersuites(const string& ciphersuites) {
    if (!ctx_) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL context is not initialized"));
    }
    if (::SSL_CTX_set_ciphersuites(ctx_.get(), ciphersuites.data()) <= 0) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Failed to set ciphersuites"));
    }
}

void ssl_context::set_default_options() {
    if (!ctx_) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL context is not initialized"));
    }

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
    if (!ctx_) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL context is not initialized"));
    }
    ::SSL_CTX_sess_set_cache_size(ctx_.get(), size);
}

void ssl_context::set_timeout(long seconds) {
    if (!ctx_) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL context is not initialized"));
    }
    ::SSL_CTX_set_timeout(ctx_.get(), seconds);
}

void ssl_context::set_alpn_protos(const vector<string>& protocols) {
    if (!ctx_) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL context is not initialized"));
    }
    if (protocols.empty()) {
        return;
    }

    byte_vector alpn_data;
    for (const auto& proto: protocols) {
        if (proto.empty() || proto.size() > 255) {
            NEFORCE_THROW_EXCEPTION(
                    value_exception(("Invalid ALPN protocol name length (must be 1-255): " + proto).data()));
        }
        alpn_data.push_back(static_cast<byte_t>(proto.size()));
        alpn_data.insert(alpn_data.end(), proto.begin(), proto.end());
    }

    if (::SSL_CTX_set_alpn_protos(ctx_.get(), alpn_data.data(), static_cast<uint32_t>(alpn_data.size())) != 0) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Failed to set ALPN protocols"));
    }
}

NEFORCE_END_NAMESPACE__
