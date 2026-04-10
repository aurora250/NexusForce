#ifndef NEFORCE_NETWORK_FTP_PROTOCOL_HPP__
#define NEFORCE_NETWORK_FTP_PROTOCOL_HPP__
#include "NeForce/network/ssl/ssl_stream.hpp"
#include "NeForce/network/ip_socket.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @struct ftp_exception
 * @brief FTP操作异常
 */
struct NEFORCE_API ftp_exception final : network_exception {
    explicit ftp_exception(const char* info = "FTP Operation Failed.", const char* type = static_type,
                           const int code = 0) noexcept :
    network_exception(info, type, code) {}

    explicit ftp_exception(const exception& e) :
    network_exception(e) {}

    ~ftp_exception() override = default;

    static constexpr auto static_type = "ftp_exception";
};

class NEFORCE_API ftp_protocol : public ip_socket {
public:
    enum class transfer_mode {
        ascii,
        binary
    };

    enum class passive_mode {
        active,
        passive
    };

    enum class tls_mode {
        none,
        implicit_,
        explicit_
    };

    struct response {
        int code;
        string message;

        NEFORCE_NODISCARD bool is_success() const noexcept { return code >= 200 && code < 400; }
        NEFORCE_NODISCARD bool is_positive_preliminary() const noexcept { return code >= 100 && code < 200; }
    };

    struct tls_info {
        bool active = false;
        string cipher_name;
        string tls_version;
        bool peer_verified = false;
        bool data_channel = false;
    };

    static constexpr size_t kBufferSize = 8192;

protected:
    bool tls_active_ = false;
    ssl_stream ctrl_ssl_;
    ssl_context* ssl_ctx_ = nullptr;
    bool data_tls_ = false;

    char buffer_[kBufferSize]{};
    size_t buffer_pos_ = 0;
    size_t buffer_size_ = 0;

    ftp_protocol() = default;
    ~ftp_protocol() override = default;

    ftp_protocol(ftp_protocol&& other) noexcept;
    ftp_protocol& operator=(ftp_protocol&& other) noexcept;

    explicit ftp_protocol(native_handle_type fd) :
    ip_socket(fd) {}

    ssize_t ctrl_send(const char* data, size_t len);
    ssize_t ctrl_recv(char* buf, size_t len);
    bool ctrl_read_line(string& out);

    response read_response();
    void send_response(int code, const string& msg);
    void send_response(const response& resp);

    void clear_buffer() {
        buffer_pos_ = 0;
        buffer_size_ = 0;
    }
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_FTP_PROTOCOL_HPP__
