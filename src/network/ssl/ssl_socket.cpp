#include <NeForce/network/ssl/ssl_socket.hpp>
NEFORCE_BEGIN_NAMESPACE__

bool ssl_socket::close() noexcept {
    if (ssl_.has_value()) {
        ssl_->close();
        ssl_.reset();
    }
    return tcp_socket::close();
}

void ssl_socket::init_server_ssl(const ssl_context& ctx) {
    if (!is_open()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Socket is not open"));
    }
    if (!ctx.is_valid()) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Invalid SSL context"));
    }

    try {
        ssl_.emplace(ctx);
        ssl_->set_fd(native_handle());
        ssl_->accept();
    } catch (...) {
        ssl_.reset();
        throw;
    }
}

void ssl_socket::init_client_ssl(const ssl_context& ctx, const string& hostname) {
    if (!is_open()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Socket is not open"));
    }
    if (!ctx.is_valid()) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Invalid SSL context"));
    }

    try {
        ssl_.emplace(ctx);
        ssl_->set_fd(native_handle());
        if (!hostname.empty()) {
            ssl_->set_sni_hostname(hostname);
        }
        ssl_->connect();
    } catch (...) {
        ssl_.reset();
        throw;
    }
}

string ssl_socket::peer_certificate_info() const {
    if (!ssl_ || !ssl_->is_valid()) {
        return "";
    }

    const auto cert = ssl_->get_peer_certificate();
    if (!cert) {
        return "";
    }

    string result;

    char* subj = ::X509_NAME_oneline(::X509_get_subject_name(cert.get()), nullptr, 0);
    char* issuer = ::X509_NAME_oneline(::X509_get_issuer_name(cert.get()), nullptr, 0);

    if (subj != nullptr) {
        result = "Subject: " + string(subj) + "\n";
        ::OPENSSL_free(subj);
    }
    if (issuer != nullptr) {
        result += "Issuer: " + string(issuer);
        ::OPENSSL_free(issuer);
    }

    return result;
}

string ssl_socket::get_alpn_negotiated() const {
    if (!ssl_ || !ssl_->is_valid()) {
        return "";
    }
    return ssl_->get_alpn_negotiated();
}

ssize_t ssl_socket::send(memory_view<const char> data, const int flags) {
    if (ssl_ && ssl_->is_valid()) {
        if (data.empty()) {
            return 0;
        }
        return ssl_->write(data.data(), data.size());
    }
    return tcp_socket::send(data, flags);
}

ssize_t ssl_socket::receive(memory_view<char> buffer, const int flags) {
    if (ssl_ && ssl_->is_valid()) {
        if (buffer.empty()) {
            return 0;
        }
        return ssl_->read(buffer.data(), buffer.size());
    }
    return tcp_socket::receive(buffer, flags);
}

NEFORCE_END_NAMESPACE__
