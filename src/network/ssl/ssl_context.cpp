#include <NeForce/network/ssl/ssl_context.hpp>
#include <NeForce/network/ssl/ssl_exception.hpp>
#include <NeForce/core/memory/memory.hpp>
#include <NeForce/core/system/pipe.hpp>
#include <openssl/ssl.h>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <wincrypt.h>
#endif
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
            case ssl_method::tls_client:
                return ::TLS_client_method();
            case ssl_method::tls_server:
                return ::TLS_server_method();
#ifndef OPENSSL_NO_DTLS
            case ssl_method::dtls_server:
                return ::DTLS_server_method();
            case ssl_method::dtls_client:
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

#ifdef NEFORCE_PLATFORM_WINDOWS
    int import_certs_from_win_store(::X509_STORE* store, const wchar_t* store_name) {
        int count = 0;

        for (const DWORD location: {CERT_SYSTEM_STORE_CURRENT_USER, CERT_SYSTEM_STORE_LOCAL_MACHINE}) {
            const ::HCERTSTORE hStore =
                    ::CertOpenStore(CERT_STORE_PROV_SYSTEM, X509_ASN_ENCODING, 0,
                                    CERT_STORE_OPEN_EXISTING_FLAG | location | CERT_STORE_READONLY_FLAG, store_name);
            if (hStore == nullptr) {
                continue;
            }

            ::PCCERT_CONTEXT pCertCtx = nullptr;
            while ((pCertCtx = ::CertEnumCertificatesInStore(hStore, pCertCtx)) != nullptr) {
                const byte_t* cert_data = pCertCtx->pbCertEncoded;
                ::X509* x509 = ::d2i_X509(nullptr, &cert_data, static_cast<long>(pCertCtx->cbCertEncoded));
                if (x509 != nullptr) {
                    if (::X509_STORE_add_cert(store, x509) == 1) {
                        ++count;
                    }
                    ::X509_free(x509);
                }
            }

            ::CertCloseStore(hStore, 0);
        }
        return count;
    }

    void load_windows_root_certs(::SSL_CTX* ctx) {
        if (ctx == nullptr) {
            return;
        }

        ::X509_STORE* store = ::SSL_CTX_get_cert_store(ctx);
        if (store == nullptr) {
            return;
        }

        const int total = import_certs_from_win_store(store, L"ROOT") + import_certs_from_win_store(store, L"CA");
        if (total > 0) {
            ::X509_VERIFY_PARAM* param = ::X509_STORE_get0_param(store);
            if (param != nullptr) {
                ::X509_VERIFY_PARAM_set_flags(param, X509_V_FLAG_PARTIAL_CHAIN);
            }
        }
    }
#endif

    void free_alpn_prefs(void* /*parent*/, void* ptr, ::CRYPTO_EX_DATA* /*ad*/, int /*idx*/, long /*argl*/,
                         void* /*argp*/) {
        delete static_cast<byte_vector*>(ptr);
    }

    int alpn_prefs_ex_index() {
        static const int index = ::SSL_CTX_get_ex_new_index(0, nullptr, nullptr, nullptr, &free_alpn_prefs);
        return index;
    }

    int select_alpn_protocol(::SSL* /*ssl*/, const unsigned char** out, unsigned char* outlen, const unsigned char* in,
                             unsigned int inlen, void* arg) {
        const auto* prefs = static_cast<const byte_vector*>(arg);
        if (prefs == nullptr || in == nullptr || inlen == 0) {
            return SSL_TLSEXT_ERR_NOACK;
        }

        size_t pref_offset = 0;
        while (pref_offset < prefs->size()) {
            const size_t pref_len = (*prefs)[pref_offset];
            if (pref_len == 0 || pref_offset + 1 + pref_len > prefs->size()) {
                break;
            }
            const unsigned char* pref_name = prefs->data() + pref_offset + 1;

            size_t offered_offset = 0;
            while (offered_offset < inlen) {
                const size_t offered_len = in[offered_offset];
                if (offered_len == 0 || offered_offset + 1 + offered_len > inlen) {
                    break;
                }
                if (offered_len == pref_len && memory_compare(in + offered_offset + 1, pref_name, pref_len) == 0) {
                    *out = pref_name;
                    *outlen = static_cast<unsigned char>(pref_len);
                    return SSL_TLSEXT_ERR_OK;
                }
                offered_offset += 1 + offered_len;
            }
            pref_offset += 1 + pref_len;
        }

        return SSL_TLSEXT_ERR_NOACK;
    }

    void install_alpn_select_callback(::SSL_CTX* ctx, const byte_vector& prefs) {
        const int index = alpn_prefs_ex_index();
        if (index < 0) {
            NEFORCE_THROW_EXCEPTION(ssl_exception("Failed to set ALPN protocols"));
        }

        auto* stored = new byte_vector(prefs);
        void* previous = ::SSL_CTX_get_ex_data(ctx, index);
        if (::SSL_CTX_set_ex_data(ctx, index, stored) != 1) {
            delete stored;
            NEFORCE_THROW_EXCEPTION(ssl_exception("Failed to set ALPN protocols"));
        }
        delete static_cast<byte_vector*>(previous);

        ::SSL_CTX_set_alpn_select_cb(ctx, &select_alpn_protocol, stored);
    }
} // namespace


ssl_context::ssl_context(const ssl_method method) :
method_(method) {
    ctx_ = ::SSL_CTX_new(convert_method(method));
    if (unlikely(ctx_ == nullptr)) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Failed to create SSL context"));
    }

    auto* ctx = static_cast<::SSL_CTX*>(ctx_);
    ::SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);

    if (::SSL_CTX_set_cipher_list(ctx, cipher_list) != 1) {
        ::SSL_CTX_set_cipher_list(ctx, "DEFAULT:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!MD5:!PSK");
    }

#ifdef TLS1_3_VERSION
    ::SSL_CTX_set_ciphersuites(ctx, "TLS_AES_128_GCM_SHA256:"
                                    "TLS_AES_256_GCM_SHA384:"
                                    "TLS_CHACHA20_POLY1305_SHA256");
#endif

    if (method == ssl_method::tls_client || method == ssl_method::dtls_client) {
        const bool ca_loaded = (::SSL_CTX_set_default_verify_paths(ctx) == 1);
        if (!ca_loaded) {
            static constexpr const char* ca_paths[] = {"/etc/ssl/certs", "/etc/pki/tls/certs", "/usr/local/share/certs",
                                                       "/etc/ssl/cert.pem"};

            for (const auto& path: ca_paths) {
                if (::SSL_CTX_load_verify_locations(ctx, nullptr, path) == 1) {
                    break;
                }
            }
        }

#ifdef NEFORCE_PLATFORM_WINDOWS
        load_windows_root_certs(ctx);
#endif

        ::SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
    }
}

ssl_context::~ssl_context() { reset(); }

ssl_context::ssl_context(ssl_context&& other) noexcept :
ctx_(other.ctx_),
method_(other.method_),
cert_loaded_(other.cert_loaded_) {
    other.ctx_ = nullptr;
    other.cert_loaded_ = false;
}

ssl_context& ssl_context::operator=(ssl_context&& other) noexcept {
    if (this != &other) {
        reset();
        ctx_ = other.ctx_;
        method_ = other.method_;
        cert_loaded_ = other.cert_loaded_;
        other.ctx_ = nullptr;
        other.cert_loaded_ = false;
    }
    return *this;
}

ssl_context ssl_context::clone() const {
    if (unlikely(ctx_ == nullptr)) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Cannot clone an invalid SSL context"));
    }

    auto* ctx = static_cast<::SSL_CTX*>(ctx_);
    if (::SSL_CTX_up_ref(ctx) != 1) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Failed to increment SSL_CTX reference count"));
    }

    ssl_context cloned(method_, nullptr);
    cloned.ctx_ = ctx;
    cloned.cert_loaded_ = cert_loaded_;
    return cloned;
}

shared_ptr<ssl_context> ssl_context::clone_shared() const { return make_shared<ssl_context>(clone()); }

bool ssl_context::load_certificate(const string& cert_file, const string& key_file) {
    if (unlikely(ctx_ == nullptr)) {
        return false;
    }
    auto* ctx = static_cast<::SSL_CTX*>(ctx_);
    if (::SSL_CTX_use_certificate_file(ctx, cert_file.data(), SSL_FILETYPE_PEM) <= 0) {
        return false;
    }
    if (::SSL_CTX_use_PrivateKey_file(ctx, key_file.data(), SSL_FILETYPE_PEM) <= 0) {
        return false;
    }
    if (::SSL_CTX_check_private_key(ctx) != 1) {
        return false;
    }
    cert_loaded_ = true;
    return true;
}

void ssl_context::load_certificate_from_memory(const string& cert_pem, const string& key_pem) {
    if (unlikely(ctx_ == nullptr)) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL context is not initialized"));
    }
    if (cert_pem.empty() || key_pem.empty()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Certificate or key PEM data is empty"));
    }

    const bio_guard cert_bio{::BIO_new_mem_buf(cert_pem.data(), static_cast<int>(cert_pem.size()))};
    const bio_guard key_bio{::BIO_new_mem_buf(key_pem.data(), static_cast<int>(key_pem.size()))};

    if (!cert_bio || !key_bio) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Failed to create BIO"));
    }

    const x509_guard cert{::PEM_read_bio_X509(cert_bio.get(), nullptr, nullptr, nullptr)};
    const evp_pkey_guard key{::PEM_read_bio_PrivateKey(key_bio.get(), nullptr, nullptr, nullptr)};

    if (!cert || !key) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Failed to parse PEM data"));
    }

    auto* ctx = static_cast<::SSL_CTX*>(ctx_);
    const int cert_result = ::SSL_CTX_use_certificate(ctx, cert.get());
    const int key_result = ::SSL_CTX_use_PrivateKey(ctx, key.get());

    if (cert_result <= 0 || key_result <= 0) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Failed to set certificate or private key"));
    }
    if (::SSL_CTX_check_private_key(ctx) != 1) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Private key does not match certificate"));
    }
    cert_loaded_ = true;
}

bool ssl_context::load_verify_locations(const string& ca_file, const string& ca_path) {
    if (unlikely(ctx_ == nullptr)) {
        return false;
    }

    const char* file_ptr = ca_file.empty() ? nullptr : ca_file.data();
    const char* path_ptr = ca_path.empty() ? nullptr : ca_path.data();

    if (file_ptr == nullptr && path_ptr == nullptr) {
        return false;
    }

    return ::SSL_CTX_load_verify_locations(static_cast<::SSL_CTX*>(ctx_), file_ptr, path_ptr) == 1;
}

void ssl_context::set_options(const ssl_option options) {
    if (unlikely(ctx_ == nullptr)) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL context is not initialized"));
    }
    ::SSL_CTX_set_options(static_cast<::SSL_CTX*>(ctx_), static_cast<uint64_t>(options));
}

void ssl_context::set_verify_mode(const ssl_verify mode) {
    if (unlikely(ctx_ == nullptr)) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL context is not initialized"));
    }
    ::SSL_CTX_set_verify(static_cast<::SSL_CTX*>(ctx_), static_cast<int>(mode), nullptr);
}

void ssl_context::require_client_certificate() {
    if (unlikely(ctx_ == nullptr)) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL context is not initialized"));
    }
    ::SSL_CTX_set_verify(static_cast<::SSL_CTX*>(ctx_), SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
}

void ssl_context::set_cipher_list(const string& ciphers) {
    if (unlikely(ctx_ == nullptr)) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL context is not initialized"));
    }
    if (::SSL_CTX_set_cipher_list(static_cast<::SSL_CTX*>(ctx_), ciphers.data()) <= 0) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Failed to set cipher list"));
    }
}

void ssl_context::set_ciphersuites(const string& ciphersuites) {
    if (unlikely(ctx_ == nullptr)) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL context is not initialized"));
    }
    if (::SSL_CTX_set_ciphersuites(static_cast<::SSL_CTX*>(ctx_), ciphersuites.data()) <= 0) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Failed to set ciphersuites"));
    }
}

void ssl_context::set_default_options() {
    if (unlikely(ctx_ == nullptr)) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL context is not initialized"));
    }

    long options = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1;
    options |= SSL_OP_SINGLE_DH_USE | SSL_OP_SINGLE_ECDH_USE;
    options |= SSL_OP_CIPHER_SERVER_PREFERENCE;
    options |= SSL_OP_NO_COMPRESSION;

    auto* ctx = static_cast<::SSL_CTX*>(ctx_);
    ::SSL_CTX_set_options(ctx, options);
    ::SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);

    set_cipher_list("HIGH:!aNULL:!eNULL:!EXPORT:!DES:!MD5:!PSK:!RC4");
    set_ciphersuites("TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256");
}

void ssl_context::set_session_cache_size(long size) {
    if (unlikely(ctx_ == nullptr)) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL context is not initialized"));
    }
    ::SSL_CTX_sess_set_cache_size(static_cast<::SSL_CTX*>(ctx_), size);
}

void ssl_context::set_timeout(long seconds) {
    if (unlikely(ctx_ == nullptr)) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL context is not initialized"));
    }
    ::SSL_CTX_set_timeout(static_cast<::SSL_CTX*>(ctx_), seconds);
}

void ssl_context::set_alpn_protos(const vector<string>& protocols) {
    if (unlikely(ctx_ == nullptr)) {
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

    auto* ssl_ctx = static_cast<::SSL_CTX*>(ctx_);
    if (::SSL_CTX_set_alpn_protos(ssl_ctx, alpn_data.data(), static_cast<uint32_t>(alpn_data.size())) != 0) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Failed to set ALPN protocols"));
    }
    install_alpn_select_callback(ssl_ctx, alpn_data);
}

void ssl_context::reset(void* ctx) noexcept {
    if (ctx_ != nullptr) {
        ::SSL_CTX_free(static_cast<::SSL_CTX*>(ctx_));
    }
    ctx_ = ctx;
}

NEFORCE_END_NAMESPACE__
