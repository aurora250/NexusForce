#ifndef NEFORCE_NETWORK_SOCKET_SOCKET_BASE_HPP__
#define NEFORCE_NETWORK_SOCKET_SOCKET_BASE_HPP__
#include "NeForce/core/time/duration.hpp"
#include "NeForce/network/ip_address.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Exceptions 异常类集
 * @brief 异常类集
 * @{
 */

/**
 * @struct socket_exception
 * @extends network_exception
 * @brief socket操作异常
 */
struct NEFORCE_API socket_exception final : network_exception {
    static int last_error() noexcept;
    static bool is_would_block(int error) noexcept;

    explicit socket_exception(
        const char* info = "Socket Operation Failed.",
        const char* type = static_type,
        const int code = last_error()) noexcept
    : network_exception(info, type, code) {}

    explicit socket_exception(const exception& e)
    : network_exception(e) {}

    ~socket_exception() override = default;

    static constexpr auto static_type = "socket_exception";
};

/** @} */ // Exceptions


class NEFORCE_API socket_base {
public:
    using native_handle_type =
    #ifdef NEFORCE_PLATFORM_WINDOWS
        uintptr_t;
#else
        int;
#endif

    static constexpr native_handle_type invalid_handle =
#ifdef NEFORCE_PLATFORM_WINDOWS
        static_cast<native_handle_type>(~0);
#else
        -1;
#endif

protected:
    native_handle_type fd_ = invalid_handle;

public:
    socket_base();

    explicit socket_base(const native_handle_type fd) noexcept
    : fd_(fd) {}

    socket_base(const socket_base&) = delete;
    socket_base& operator =(const socket_base&) = delete;

    socket_base(socket_base&& other) noexcept
    : fd_(exchange(other.fd_, invalid_handle)) {}

    socket_base& operator =(socket_base&& other) noexcept;

    virtual ~socket_base() {
        close();
    }

    NEFORCE_NODISCARD native_handle_type native_handle() const noexcept {
        return fd_;
    }

    NEFORCE_NODISCARD bool is_open() const noexcept {
        return fd_ != invalid_handle;
    }

    explicit operator bool() const noexcept {
        return is_open();
    }

    bool close() noexcept;

    bool set_nonblocking(bool enable) noexcept;

    bool shutdown_send() noexcept;
    bool shutdown_receive() noexcept;
    bool shutdown_both() noexcept;

    bool set_option(int level, int optname, const void* value, ::socklen_t len) noexcept;
    bool get_option(int level, int optname, void* optval, ::socklen_t* optlen) const noexcept;

    bool set_reuse_address(bool enable = true) noexcept;
    bool set_reuse_port(bool enable = true) noexcept;

    bool set_keep_alive(bool enable = true) noexcept;

    bool set_tcp_nodelay(bool enable = true) noexcept;

    bool set_receive_buffer_size(int size) noexcept;
    bool set_send_buffer_size(int size) noexcept;

    bool set_send_timeout(milliseconds timeout) noexcept;
    bool set_receive_timeout(milliseconds timeout) noexcept;

    NEFORCE_NODISCARD optional<ip_address> local_endpoint() const;
    NEFORCE_NODISCARD optional<ip_address> remote_endpoint() const;

    void bind(const ip_address& endpoint);

    void listen(int backlog);
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_SOCKET_SOCKET_BASE_HPP__
