#include <NeForce/network/socket/socket_base.hpp>
#include <NeForce/network/ssl/ssl_stream.hpp>
#include <openssl/err.h>
NEFORCE_BEGIN_NAMESPACE__

void ssl_stream::handle_ssl_error(const int ret, const char* operation) {
    if (!ssl_) {
        last_error_ = string(operation) + " failed: SSL object is null";
        return;
    }

    const int err = ::SSL_get_error(ssl_.get(), ret);

    switch (err) {
        case SSL_ERROR_NONE: {
            last_error_.clear();
            return;
        }
        case SSL_ERROR_WANT_READ: {
            last_error_ = string(operation) + " needs more data to read";
            return;
        }
        case SSL_ERROR_WANT_WRITE: {
            last_error_ = string(operation) + " needs to write data";
            return;
        }
        case SSL_ERROR_ZERO_RETURN: {
            last_error_ = string(operation) + " connection closed";
            NEFORCE_THROW_EXCEPTION(ssl_exception("SSL connection closed cleanly"));
        }
        case SSL_ERROR_SYSCALL: {
            const unsigned long ssl_err = ::ERR_get_error();
            if (ssl_err == 0) {
                if (ret == 0) {
                    last_error_ = string(operation) + " unexpected EOF";
                    NEFORCE_THROW_EXCEPTION(ssl_exception("Unexpected EOF in SSL operation"));
                } else {
                    last_error_ = string(operation) + " system call error";
                    NEFORCE_THROW_EXCEPTION(ssl_exception("System call error in SSL operation"));
                }
            } else {
                char buf[256];
                ::ERR_error_string_n(ssl_err, buf, sizeof(buf));
                last_error_ = string(operation) + " syscall error: " + buf;
                NEFORCE_THROW_EXCEPTION(ssl_exception(static_cast<int>(ssl_err)));
            }
            break;
        }
        case SSL_ERROR_SSL: {
            const unsigned long ssl_err = ::ERR_get_error();
            char buf[256];
            ::ERR_error_string_n(ssl_err, buf, sizeof(buf));
            last_error_ = string(operation) + " SSL protocol error: " + buf;
            NEFORCE_THROW_EXCEPTION(ssl_exception(static_cast<int>(ssl_err)));
            break;
        }
        default: {
            last_error_ = string(operation) + " unknown error";
            NEFORCE_THROW_EXCEPTION(ssl_exception(err));
        }
    }
}

void ssl_stream::reset(const ssl_context& ctx) {
    if (!ctx.is_valid()) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Invalid SSL context"));
    }

    ssl_.reset(::SSL_new(ctx.native_handle()));
    if (!ssl_) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL_new failed"));
    }

    last_error_.clear();
}

void ssl_stream::set_fd(const native_handle_type fd) {
    if (!ssl_) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL object not initialized"));
    }

    if (fd == socket_base::invalid_handle) {
        NEFORCE_THROW_EXCEPTION(value_exception("Invalid file descriptor"));
    }

    if (::SSL_set_fd(ssl_.get(), static_cast<int>(fd)) != 1) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL_set_fd failed"));
    }
}

void ssl_stream::accept() {
    if (!ssl_) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL object not initialized"));
    }

    const int ret = ::SSL_accept(ssl_.get());
    if (ret != 1) {
        handle_ssl_error(ret, "SSL_accept");
    }
}

bool ssl_stream::connect() {
    if (!ssl_) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL not initialized"));
    }
    SSL_set_connect_state(ssl_.get());

    const int ret = SSL_connect(ssl_.get());

    if (ret != 1) {
        const int err = SSL_get_error(ssl_.get(), ret);
        const auto ssl_err = ERR_get_error();
        char err_buf[256];
        ERR_error_string_n(ssl_err, err_buf, sizeof(err_buf));

        string error_msg = "SSL handshake failed: ";
        error_msg += err_buf;

        switch (err) {
            case SSL_ERROR_SYSCALL: {
                error_msg += " (System call error)";
                break;
            }
            case SSL_ERROR_SSL: {
                error_msg += " (SSL protocol error)";
                break;
            }
            case SSL_ERROR_WANT_READ: {
                error_msg += " (Want read)";
                break;
            }
            case SSL_ERROR_WANT_WRITE: {
                error_msg += " (Want write)";
                break;
            }
            case SSL_ERROR_ZERO_RETURN: {
                error_msg += " (Connection closed)";
                break;
            }
            default: unreachable();
        }
        NEFORCE_THROW_EXCEPTION(ssl_exception(error_msg.data()));
    }

    return true;
}

void ssl_stream::close() {
    auto* ssl = ssl_.release();
    if (ssl) {
        ::SSL_shutdown(ssl);
        ::SSL_free(ssl);
    }
    last_error_.clear();
}

ssize_t ssl_stream::read(void* buffer, const size_t size) {
    if (!ssl_) {
        last_error_ = "SSL object not initialized";
        return -1;
    }
    if (!buffer || size == 0) {
        last_error_ = "Invalid buffer or size";
        return -1;
    }

    if (size > numeric_traits<int>::max()) {
        last_error_ = "Size exceeds maximum allowed";
        return -1;
    }

    const int ret = ::SSL_read(ssl_.get(), buffer, static_cast<int>(size));
    if (ret <= 0) {
        const int err = ::SSL_get_error(ssl_.get(), ret);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            last_error_ = "SSL_read would block";
            return 0;
        }
        handle_ssl_error(ret, "SSL_read");
        return -1;
    }

    last_error_.clear();
    return ret;
}

ssize_t ssl_stream::write(const void* buffer, const size_t size) {
    if (!ssl_) {
        last_error_ = "SSL object not initialized";
        return -1;
    }
    if (!buffer || size == 0) {
        return 0;
    }

    if (size > numeric_traits<int>::max()) {
        last_error_ = "Size exceeds maximum allowed";
        return -1;
    }

    const int ret = ::SSL_write(ssl_.get(), buffer, static_cast<int>(size));
    if (ret <= 0) {
        const int err = ::SSL_get_error(ssl_.get(), ret);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            last_error_ = "SSL_write would block";
            return 0;
        }
        handle_ssl_error(ret, "SSL_write");
        return -1;
    }

    last_error_.clear();
    return ret;
}

vector<char> ssl_stream::read_all(const size_t max_size) {
    if (!ssl_) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL object not initialized"));
    }
    if (max_size == 0) {
        return vector<char>();
    }

    vector<char> buffer;
    buffer.reserve(min(max_size, MEMORY_BIG_ALLOC_THRESHHOLD));

    char temp[MEMORY_BIG_ALLOC_THRESHHOLD];

    while (buffer.size() < max_size) {
        const size_t to_read = min(sizeof(temp), max_size - buffer.size());
        const ssize_t ret = read(temp, to_read);

        if (ret < 0) {
            NEFORCE_THROW_EXCEPTION(ssl_exception("Failed to read data"));
        }
        if (ret == 0) {
            break;
        }

        buffer.insert(buffer.end(), temp, temp + ret);

        if (ret < sizeof(temp)) {
            break;
        }
    }

    return buffer;
}

bool ssl_stream::write_all(const void* data, const size_t size) {
    if (!ssl_) {
        last_error_ = "SSL object not initialized";
        return false;
    }
    if (!data) {
        last_error_ = "Invalid data pointer";
        return false;
    }

    if (size == 0) {
        return true;
    }

    auto ptr = static_cast<const char*>(data);
    size_t remaining = size;

    while (remaining > 0) {
        const ssize_t written = write(ptr, remaining);
        if (written < 0) {
            return false;
        }

        if (written == 0) {
            last_error_ = "SSL_write returned 0";
            return false;
        }

        ptr += written;
        remaining -= written;
    }

    return true;
}

int ssl_stream::pending() const {
    if (!ssl_) return 0;
    return ::SSL_pending(ssl_.get());
}

void ssl_stream::set_sni_hostname(const string& hostname) {
    if (!ssl_) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL object not initialized"));
    }

    if (hostname.empty()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Hostname cannot be null"));
    }

    if (::SSL_set_tlsext_host_name(ssl_.get(), hostname.data()) != 1) {
        const auto err = ::ERR_get_error();
        char buf[256];
        ::ERR_error_string_n(err, buf, sizeof(buf));

        string error_msg = "SSL_set_tlsext_host_name failed: ";
        error_msg += buf;
        NEFORCE_THROW_EXCEPTION(ssl_exception(error_msg.data()));
    }

#if OPENSSL_VERSION_NUMBER >= 0x10002000L
    ::X509_VERIFY_PARAM* param = ::SSL_get0_param(ssl_.get());
    if (param) {
        ::X509_VERIFY_PARAM_set1_host(param, hostname.data(), 0);
    }
#endif
}

::X509* ssl_stream::get_peer_certificate() const {
    if (!ssl_) return nullptr;
    return ::SSL_get_peer_certificate(ssl_.get());
}

bool ssl_stream::verify_peer() const {
    if (!ssl_) {
        return false;
    }

    ::X509* cert = ::SSL_get_peer_certificate(ssl_.get());
    if (!cert) {
        return false;
    }

    const long verify_result = ::SSL_get_verify_result(ssl_.get());
    ::X509_free(cert);

    return verify_result == X509_V_OK;
}

string ssl_stream::get_cipher_name() const {
    if (!ssl_) return "";
    return ::SSL_get_cipher_name(ssl_.get());
}

string ssl_stream::get_version() const {
    if (!ssl_) return "";
    return ::SSL_get_version(ssl_.get());
}

NEFORCE_END_NAMESPACE__
