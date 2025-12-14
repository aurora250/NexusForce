#ifndef MSTL_NETWORK_SSL_CONTEXT_HPP__
#define MSTL_NETWORK_SSL_CONTEXT_HPP__
#ifdef MSTL_SUPPORT_OPENSSL__
#include "MSTL/core/string/string.hpp"
#include <openssl/ssl.h>
MSTL_BEGIN_NAMESPACE__

class MSTL_API ssl_context {
public:
    ssl_context() = default;
    ~ssl_context();

    bool load_certificate(const string& cert_file, const string& key_file);
    ::SSL_CTX* context() const noexcept { return ctx_; }

private:
    ::SSL_CTX* ctx_ = nullptr;
};

MSTL_END_NAMESPACE__
#endif
#endif // MSTL_NETWORK_SSL_CONTEXT_HPP__
