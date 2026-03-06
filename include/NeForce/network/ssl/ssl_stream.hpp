#ifndef NEFORCE_NETWORK_SSL_SSL_STREAM_HPP__
#define NEFORCE_NETWORK_SSL_SSL_STREAM_HPP__
#ifdef NEFORCE_SUPPORT_OPENSSL
#include "NeForce/network/ssl/ssl_context.hpp"
NEFORCE_BEGIN_NAMESPACE__

class NEFORCE_API ssl_stream {
public:
    using native_handle_type =
    #ifdef NEFORCE_PLATFORM_WINDOWS
        uintptr_t;
#else
        int;
#endif

private:
    struct ssl_deleter {
        void operator()(::SSL* ssl) const noexcept {
            if (ssl) {
                ::SSL_shutdown(ssl);
                ::SSL_free(ssl);
            }
        }
    };
    
    unique_ptr<::SSL, ssl_deleter> ssl_;
    string last_error_;

    void handle_ssl_error(int ret, const char* operation);

public:
    ssl_stream() = default;

    explicit ssl_stream(const ssl_context& ctx) {
        reset(ctx);
    }

    ssl_stream(ssl_stream&& other) noexcept = default;
    ssl_stream& operator=(ssl_stream&& other) noexcept = default;

    void reset(const ssl_context& ctx);

    void set_fd(native_handle_type fd);

    void accept();

    void connect();

    ssize_t read(void* buffer, size_t size);

    ssize_t write(const void* buffer, size_t size);

    vector<char> read_all(size_t max_size = 8192);

    bool write_all(const void* data, size_t size);

    int pending() const;

    ::X509* get_peer_certificate() const;

    bool verify_peer() const;

    string get_cipher_name() const;

    string get_version() const;

    const string& last_error() const {
        return last_error_;
    }

    bool is_valid() const noexcept {
        return ssl_ != nullptr;
    }

    explicit operator bool() const noexcept {
        return is_valid();
    }

    ::SSL* native_handle() const noexcept {
        return ssl_.get();
    }

    ::SSL* release() noexcept {
        return ssl_.release();
    }
};

NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_NETWORK_SSL_SSL_STREAM_HPP__
