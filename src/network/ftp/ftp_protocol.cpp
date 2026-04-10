#include <NeForce/core/utility/packages.hpp>
#include <NeForce/network/ftp/ftp_protocol.hpp>
#include <NeForce/logging/logger.hpp>
NEFORCE_BEGIN_NAMESPACE__

ftp_protocol::ftp_protocol(ftp_protocol&& other) noexcept :
tls_active_(other.tls_active_),
ctrl_ssl_(move(other.ctrl_ssl_)),
ssl_ctx_(other.ssl_ctx_),
data_tls_(other.data_tls_) {
    other.tls_active_ = false;
    other.ssl_ctx_ = nullptr;
    other.data_tls_ = false;
}

ftp_protocol& ftp_protocol::operator=(ftp_protocol&& other) noexcept {
    if (this != &other) {
        tls_active_ = other.tls_active_;
        ctrl_ssl_ = move(other.ctrl_ssl_);
        ssl_ctx_ = other.ssl_ctx_;
        data_tls_ = other.data_tls_;
        other.tls_active_ = false;
        other.ssl_ctx_ = nullptr;
        other.data_tls_ = false;
        other.tls_active_ = false;
    }
    return *this;
}

ssize_t ftp_protocol::ctrl_send(const char* data, const size_t len) {
    if (tls_active_) {
        return ctrl_ssl_.write(data, len);
    }
    return ::send(fd_, data, static_cast<int>(len), 0);
}

ssize_t ftp_protocol::ctrl_recv(char* buf, const size_t len) {
    if (len == 0) {
        return 0;
    }

    if (buffer_pos_ >= buffer_size_) {
        buffer_pos_ = 0;
        ssize_t n = 0;
        if (tls_active_) {
            n = ctrl_ssl_.read(buffer_, kBufferSize);
        } else {
            if (fd_ == invalid_handle) {
                return -1;
            }
            n = ::recv(fd_, buffer_, static_cast<int>(kBufferSize), 0);
        }

        if (n <= 0) {
            buffer_size_ = 0;
            return n;
        }
        buffer_size_ = static_cast<size_t>(n);
    }

    const size_t available = buffer_size_ - buffer_pos_;
    const size_t to_copy = (len < available) ? len : available;
    memory_copy(buf, buffer_ + buffer_pos_, to_copy);
    buffer_pos_ += to_copy;
    return static_cast<ssize_t>(to_copy);
}

bool ftp_protocol::ctrl_read_line(string& out) {
    out.clear();
    char ch = 0;
    while (true) {
        ssize_t bytes_read = ctrl_recv(&ch, 1);
        if (bytes_read <= 0) {
            NEFORCE_LOGF_DEBUG("ctrl_read_line: End of stream or error, bytes_read={}", bytes_read);
            return false;
        }
        if (ch == '\n') {
            if (!out.empty() && out.back() == '\r') {
                out.pop_back();
            }
            NEFORCE_LOGF_DEBUG("ctrl_read_line: LINE_END, out='{}'", out);
            return true;
        }
        out += ch;
    }
}

ftp_protocol::response ftp_protocol::read_response() {
    response resp{0, {}};
    string line;
    bool in_multiline = false;
    int expected_code = 0;

    NEFORCE_LOG_DEBUG("read_response: START");

    while (true) {
        if (!ctrl_read_line(line)) {
            NEFORCE_LOG_DEBUG("read_response: ctrl_read_line returned false");
            NEFORCE_THROW_EXCEPTION(ftp_exception("Connection closed while reading FTP response"));
        }
        NEFORCE_LOGF_DEBUG("read_response: Read line: '{}'", line);

        if (line.size() < 3) {
            NEFORCE_LOGF_DEBUG("read_response: Line too short: '{}'", line);
            if (in_multiline) {
                resp.message += "\n" + line;
                continue;
            }
            NEFORCE_THROW_EXCEPTION(ftp_exception("Malformed FTP response: line too short"));
        }

        int current_code = 0;
        bool has_code = true;
        for (int i = 0; i < 3; ++i) {
            if (line[i] < '0' || line[i] > '9') {
                has_code = false;
                break;
            }
            current_code = current_code * 10 + (line[i] - '0');
        }

        if (!in_multiline) {
            if (!has_code) {
                NEFORCE_THROW_EXCEPTION(ftp_exception("Invalid FTP response: no status code"));
            }
            resp.code = current_code;

            if (line.size() > 3 && line[3] == '-') {
                in_multiline = true;
                expected_code = current_code;
                resp.message += line.substr(4);
                continue;
            }
            resp.message = (line.size() > 4) ? line.substr(4) : "";
            break;
        }
        if (has_code && current_code == expected_code && line.size() >= 3 && (line.size() == 3 || line[3] == ' ')) {
            if (line.size() > 4) {
                resp.message += "\n" + line.substr(4);
            }
            break;
        }
        resp.message += "\n" + line;
    }

    NEFORCE_LOGF_DEBUG("read_response: END. Code: {}, Message: '{}'", resp.code, resp.message);
    return resp;
}


void ftp_protocol::send_response(const int code, const string& msg) {
    string full = to_string(code) + " " + msg + "\r\n";
    size_t total = 0;
    while (total < full.size()) {
        const ssize_t n = ctrl_send(full.data() + total, full.size() - total);
        if (n <= 0) {
            NEFORCE_THROW_EXCEPTION(ftp_exception("FTP control channel send failed"));
        }
        total += static_cast<size_t>(n);
    }
}

void ftp_protocol::send_response(const response& resp) { send_response(resp.code, resp.message); }

NEFORCE_END_NAMESPACE__
