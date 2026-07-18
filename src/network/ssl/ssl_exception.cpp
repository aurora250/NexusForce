#include <NeForce/network/ssl/ssl_exception.hpp>
#include <openssl/err.h>
NEFORCE_BEGIN_NAMESPACE__

string ssl_error_category::message(const int32_t ev) const {
    if (ev == 0) {
        return "";
    }
    char buf[256];
    ::ERR_error_string_n(static_cast<unsigned long>(ev), static_cast<char*>(buf), sizeof(buf));
    return {static_cast<char*>(buf)};
}

const error_category& ssl_category() noexcept {
    static ssl_error_category instance;
    return instance;
}

error_code ssl_exception::last_error() noexcept { return {static_cast<int>(::ERR_peek_last_error()), ssl_category()}; }

NEFORCE_END_NAMESPACE__
