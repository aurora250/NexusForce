#include <NeForce/network/ftp/ftp_server.hpp>
NEFORCE_BEGIN_NAMESPACE__

pair<string, string> ftp_session::read_command() {
    string line;
    if (!ctrl_read_line(line)) {
        NEFORCE_THROW_EXCEPTION(ftp_exception("Connection closed by peer"));
    }

    size_t space_pos = line.find(' ');
    if (space_pos == string::npos) {
        return {line, ""};
    }

    return {line.substr(0, space_pos), line.substr(space_pos + 1)};
}

void ftp_session::accept_tls(ssl_context& ctx) {
    ctrl_ssl_.reset(ctx);
    ctrl_ssl_.set_fd(fd_);
    ctrl_ssl_.accept();
    tls_active_ = true;
}

NEFORCE_END_NAMESPACE__
