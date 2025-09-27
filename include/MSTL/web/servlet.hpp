#ifndef MSTL_SERVLET_HPP__
#define MSTL_SERVLET_HPP__
#include "socket.hpp"
#include "session.hpp"
#include "MSTL/core/print.hpp"
#ifdef MSTL_PLATFORM_LINUX__
#include <netinet/in.h>
#include <unistd.h>
#endif
MSTL_BEGIN_NAMESPACE__

MSTL_ERROR_BUILD_FINAL_CLASS(HttpError, LinkError, "Http Actions Failed");

struct http_request {
private:
    HTTP_METHOD method = HTTP_METHOD::GET;
    string path = "/";
    string version = "HTTP/1.1";
    unordered_map<string, string> headers;
    unordered_map<string, string> cookies;
    unordered_map<string, string> parameters; // query + body parameters
    string query{};
    string body{};
    session* session = nullptr;

    friend class _INNER servlet;

    static MSTL_API const string EMPTY_MARK;

public:
    http_request() = default;
    http_request(const http_request&) = delete;
    http_request& operator=(const http_request&) = delete;
    http_request(http_request&& req) noexcept = default;
    http_request& operator=(http_request&& req) noexcept = default;
    ~http_request() = default;

    void set_version(string version) {
        this->version = _MSTL move(version);
    }
    const string& get_version() const {
        return version;
    }

    void set_parameter(const string& name, const string& value) {
        parameters[name] = value;
    }
    const string& get_parameter(const string& name) const {
        auto it = parameters.find(name);
        return it != parameters.end() ? it->second : EMPTY_MARK;
    }

    void set_cookie(const string& name, const string& value) {
        cookies[name] = value;
    }
    const string& get_cookie(const string& name) const {
        auto it = cookies.find(name);
        return it != cookies.end() ? it->second : EMPTY_MARK;
    }

    void set_session(class session* session) {
        this->session = session;
    }
    class session* get_session() const {
        return session;
    }

    void set_header(const string& name, const string& value) {
        headers[name] = value;
    }
    const string& get_header(const string& name) const {
        auto it = headers.find(name);
        return it != headers.end() ? it->second : EMPTY_MARK;
    }

    void set_content_type(const string& value) {
        cookies["Content-Type"] = value;
    }
    const string& get_content_type() const {
        return get_header("Content-Type");
    }
    const string& get_cookie() const {
        return get_header("Cookie");
    }

    void set_method(const HTTP_METHOD& method) {
        this->method = method;
    }
    void set_method(const string& method) {
        this->method = method;
    }
    const HTTP_METHOD& get_method() const {
        return method;
    }

    void set_path(const string& path) {
        this->path = path;
    }
    const string& get_path() const {
        return path;
    }

    void set_query(string query) {
        this->query = _MSTL move(query);
    }
    const string& get_query() const {
        return query;
    }

    const string& get_body() const {
        return body;
    }
    void set_body(string body) {
        this->body = _MSTL move(body);
    }

    bool is_https() const {
        auto proto_header = get_header("X-Forwarded-Proto");
        return proto_header == "https";
    }
};


struct http_response {
private:
    string version = "HTTP/1.1";
    HTTP_STATUS status_code = HTTP_STATUS::S4_NOT_FOUNT;
    string status_msg = "";
    unordered_map<string, string> headers;
    vector<cookie> cookies;
    string body{};
    string redirect_url{};
    string forward_path{};

    static MSTL_API const string EMPTY_MARK;

    friend class _INNER servlet;

public:
    http_response() {
        set_content_type(HTTP_CONTENT::HTML_TEXT);
        set_content_encode("utf-8");
        set_header("Connection", "close");
    }

    http_response(const http_response&) = delete;
    http_response& operator=(const http_response&) = delete;
    http_response(http_response&& res) noexcept = default;
    http_response& operator=(http_response&& res) noexcept = default;

    const string& get_version() const {
        return version;
    }
    void set_version(string version) {
        this->version = _MSTL move(version);
    }

    void add_cookie(const cookie& cookie) {
        cookies.push_back(cookie);
    }
    void add_cookie(const string& name, const string& value) {
        cookies.emplace_back(name, value);
    }

    void set_header(const string& name, const string& value) {
        headers[name] = value;
    }
    const string& get_header(const string& name) const {
        auto it = headers.find(name);
        return it != headers.end() ? it->second : EMPTY_MARK;
    }

    const string& get_content_length() const {
        return get_header("Content-Length");
    }

    void set_content_type(const string& value) {
        headers["Content-Type"] = value;
    }
    void set_content_type(const char* value) {
        headers["Content-Type"] = value;
    }
    void set_content_encode(const string& value) {
        headers["Content-Type"] += "; charset=" + value;
    }

    void set_allow_method(const HTTP_METHOD& method) {
        headers["Access-Control-Allow-Methods"] = method.to_string();
    }
    void set_max_age(const size_t age) {
        headers["Access-Control-Max-Age"] = to_string(age);
    }
    void set_allow_credentials(const bool allow) {
        if (allow) headers["Access-Control-Allow-Credentials"] = "true";
        else headers["Access-Control-Allow-Credentials"] = "false";
    }
    void set_allow_origin(string origin) {
        headers["Access-Control-Allow-Origin"] = _MSTL move(origin);
    }
    void set_allow_headers(string header) {
        headers["Access-Control-Allow-Headers"] = _MSTL move(header);
    }

    void set_status(const HTTP_STATUS status) {
        status_code = status;
    }
    void set_ok() {
        status_code = HTTP_STATUS::S2_OK;
    }
    void set_not_found() {
        status_code = HTTP_STATUS::S4_NOT_FOUNT;
    }
    void set_bad_request() {
        status_code = HTTP_STATUS::S4_BAD_REQUEST;
    }
    HTTP_STATUS get_status() const {
        return status_code;
    }

    void set_status_msg(string status_msg) {
        this->status_msg = _MSTL move(status_msg);
    }
    const string& get_status_msg() const {
        return status_msg;
    }

    void set_body(string body) {
        this->body = _MSTL move(body);
    }
    const string& get_body() const {
        return body;
    }


    void redirect(string url) {
        redirect_url = _MSTL move(url);
    }
    const string& get_redirect() const {
        return redirect_url;
    }

    void forward(string url) {
        forward_path = _MSTL move(url);
    }
    const string& get_forward() const {
        return forward_path;
    }
};


class filter {
public:
    virtual ~filter() = default;
    virtual void do_filter(http_request& request, http_response& response) = 0;
    virtual bool pre_filter(http_request& request, http_response& response) { return true; }
    virtual void post_filter(http_request& request, http_response& response) {}
};

class filter_chain {
private:
    vector<filter*> filters_;
    size_t current_index_ = 0;

public:
    void add_filter(filter* filter) {
        filters_.push_back(filter);
    }

    bool execute_pre_filters(http_request& request, http_response& response) {
        for (auto* filter : filters_) {
            if (!filter->pre_filter(request, response)) {
                return false;
            }
        }
        return true;
    }

    void execute_post_filters(http_request& request, http_response& response) {
        for (auto it = filters_.rbegin(); it != filters_.rend(); ++it) {
            (*it)->post_filter(request, response);
        }
    }

    void execute_filters(http_request& request, http_response& response) {
        for (auto* filter : filters_) {
            filter->do_filter(request, response);
        }
    }
};


#ifdef DELETE
#undef DELETE
#endif

class cors_filter final : public filter {
private:
    string allowed_origins_ = "*";
    HTTP_METHOD allowed_methods_ = HTTP_METHOD::GET & HTTP_METHOD::POST &
        HTTP_METHOD::DELETE & HTTP_METHOD::PUT & HTTP_METHOD::OPTIONS;
    string allowed_headers_ = "Content-Type, Cookie, Accept, X-Requested-With";

public:
    void set_allowed_origins(const string& origins) { allowed_origins_ = origins; }
    void set_allowed_methods(const HTTP_METHOD& methods) { allowed_methods_ = methods; }
    void set_allowed_headers(const string& headers) { allowed_headers_ = headers; }

    void do_filter(http_request& request, http_response& response) override {
        response.set_allow_origin(allowed_origins_);
        response.set_allow_method(allowed_methods_);
        response.set_allow_headers(allowed_headers_);
        response.set_allow_credentials(true);

        if (request.get_method().is_options()) {
            response.set_ok();
            response.set_status_msg("OK");
            response.set_body("");
        }
    }
};

class logging_filter final : public filter {
public:
    bool pre_filter(http_request& request, http_response& response) override {
        print();
        println("[", datetime::now(), "] Request: ", request.get_method(), " ", request.get_path());
        print();
        return true;
    }

    void post_filter(http_request& request, http_response& response) override {
        print();
        println("[", datetime::now(), "] Response: ", response.get_status(), " ", response.get_status_msg());
        print();
    }

    void do_filter(http_request& request, http_response& response) override {}
};


struct HTTP_COOKIE {
    static constexpr auto JSESSIONID = "JSESSIONID";
    static constexpr auto SESSIONID = "SESSIONID";
    static constexpr auto PHPSESSID = "PHPSESSID";
    static constexpr auto ASPSESSIONID = "ASP.NET_SessionId";
};


MSTL_BEGIN_INNER__

class servlet {
private:
    socket server_socket_{};
    uint16_t port_;
    int backlog_;
    std::atomic<bool> running_{false};
#ifdef MSTL_PLATFORM_WINDOWS__
    ::WSADATA wsa_data_{};
#endif
    ::sockaddr_in server_addr_{};
    vector<std::thread> worker_threads_;
    _INNER __session_manager session_manager_;
    filter_chain filter_chain_;
    string session_cookie_name_ = HTTP_COOKIE::JSESSIONID;

private:
    void start_workers(const int thread_count) {
        for (int i = 0; i < thread_count; ++i) {
            worker_threads_.emplace_back(&servlet::accept_conns, this);
        }
    }

    void accept_conns() {
        while (running_) {
            ::sockaddr_in client_addr{};
            ::socklen_t client_len = sizeof(client_addr);

            const socket::socket_t client_socket = ::accept(server_socket_.get(),
                reinterpret_cast<::sockaddr *>(&client_addr), &client_len);
            if (client_socket == socket::INVALID_MARK) {
                if (running_) {
                    println("accept failed");
                }
                continue;
            }

            try {
                handle_client(client_socket);
            } catch (const Error& e) {
                println(e);
            }
#ifdef MSTL_PLATFORM_WINDOWS__
            ::closesocket(client_socket);
#elif defined(MSTL_PLATFORM_LINUX__)
            ::close(client_socket);
#endif
        }
    }

    void handle_client(const socket::socket_t client_socket) {
        http_request request = parse_request(client_socket);
        int forward_count = 0;
        do {
            constexpr int MAX_FORWARD = 5;
            http_response response = handle_request(request);
            if (!response.get_forward().empty() && forward_count < MAX_FORWARD) {
                request.set_path(response.get_forward());
                forward_count++;
                continue;
            }
            send_response(client_socket, response);
            return;
        } while (true);
    }

    static void parse_cookies(const string &cookie_header, http_request &request) {
        if (cookie_header.empty())
            return;

        vector<string> cookie_pairs;
        size_t start = 0;
        size_t end = cookie_header.find(';');

        while (end != string::npos) {
            cookie_pairs.push_back(cookie_header.substr(start, end - start).trim());
            start = end + 1;
            end = cookie_header.find(';', start);
        }
        cookie_pairs.push_back(cookie_header.substr(start).trim());

        for (const auto &pair: cookie_pairs) {
            const size_t eq_pos = pair.find('=');
            if (eq_pos != string::npos) {
                string name = pair.substr(0, eq_pos).trim();
                string value = pair.substr(eq_pos + 1).trim();
                request.set_cookie(name, value);
            }
        }
    }

    static void parse_parameters(http_request &request) {
        if (!request.get_query().empty()) {
            parse_url_encoded(request.get_query(), request.parameters);
        }
        if (request.get_method().is_post() && !request.get_body().empty()) {
            string content_type = request.get_content_type();
            if (content_type.find(HTTP_CONTENT::FORM_APP) == 0) {
                parse_url_encoded(request.get_body(), request.parameters);
            }
        }
    }

    static void parse_url_encoded(const string &data, unordered_map<string, string> &params) {
        if (data.empty())
            return;

        vector<string> pairs;
        size_t start = 0;
        size_t end = data.find('&');

        while (end != string::npos) {
            pairs.push_back(data.substr(start, end - start));
            start = end + 1;
            end = data.find('&', start);
        }
        pairs.push_back(data.substr(start));

        for (const auto &pair: pairs) {
            const size_t eq_pos = pair.find('=');
            if (eq_pos != string::npos) {
                string name = url_decode(pair.substr(0, eq_pos));
                string value = url_decode(pair.substr(eq_pos + 1));
                params[name] = value;
            }
        }
    }

    static string url_decode(const string& str) {
        string result;
        for (size_t i = 0; i < str.length(); ++i) {
            if (str[i] == '%' && i + 2 < str.length()) {
                try {
                    hexadecimal hex_val(str.substr(i + 1, 2));
                    result += static_cast<char>(hex_val.to_decimal());
                } catch (...) {
                    result += str[i];
                }
            } else if (str[i] == '+') {
                result += ' ';
            } else {
                result += str[i];
            }
        }
        return result;
    }

protected:
    virtual http_request parse_request(const socket::socket_t client_socket) {
        http_request req;
        char buffer[4096];
        string request_data;

        ssize_t total_read = 0;
        while (true) {
#ifdef MSTL_PLATFORM_WINDOWS__
            int bytes_read = ::recv(client_socket, buffer, sizeof(buffer) - 1, 0);
#elif defined(MSTL_PLATFORM_LINUX__)
            ssize_t bytes_read = ::read(client_socket, buffer, sizeof(buffer) - 1);
#endif
            if (bytes_read <= 0) break;

            buffer[bytes_read] = '\0';
            request_data.append(buffer, bytes_read);
            total_read += bytes_read;

            const size_t header_end = request_data.find("\r\n\r\n");
            if (header_end != string::npos) {
                size_t content_length = 0;
                const size_t content_start = header_end + 4;

                const size_t cl_pos = request_data.find("Content-Length:");
                if (cl_pos != string::npos) {
                    const size_t cl_end = request_data.find("\r\n", cl_pos);
                    if (cl_end != string::npos) {
                        string cl_str = request_data.substr(
                            cl_pos + 15, cl_end - cl_pos - 15
                            ).trim();
                        content_length = _MSTL to_uint32(cl_str.c_str());
                    }
                }

                const auto body_read = static_cast<ssize_t>(total_read - content_start);
                if (body_read < static_cast<ssize_t>(content_length)) {
                    ssize_t remaining = static_cast<ssize_t>(content_length - body_read);
                    while (remaining > 0) {
                        bytes_read = ::read(client_socket, buffer,
                            _MSTL min(static_cast<ssize_t>(sizeof(buffer)), remaining));
                        if (bytes_read <= 0) break;
                        request_data.append(buffer, bytes_read);
                        remaining -= bytes_read;
                    }
                }
                break;
            }

            if (total_read > 1024 * 16) break;
        }

        istringstream iss(request_data);
        string line;
        if (iss.getline(line)) {
            line.trim();
            const size_t pos1 = line.find(' ');
            if (pos1 != string::npos) {
                req.set_method(line.substr(0, pos1));
                const size_t pos2 = line.find(' ', pos1 + 1);
                if (pos2 != string::npos) {
                    req.set_path(line.substr(pos1 + 1, pos2 - pos1 - 1));
                    req.set_version(line.substr(pos2 + 1));
                }
            }
        }

        // url query
        const size_t query_pos = req.get_path().find('?');
        if (query_pos != string::npos) {
            req.set_query(req.get_path().substr(query_pos+1));
            req.set_path(req.get_path().substr(0, query_pos));
        }

        // request header
        while (iss.getline(line)) {
            line.trim();
            if (line.empty()) break;

            const size_t colon_pos = line.find(':');
            if (colon_pos != string::npos) {
                string key = line.substr(0, colon_pos).trim();
                string value = line.substr(colon_pos+1).trim();
                req.set_header(key, _MSTL move(value));
            }
        }

        // request body
        const size_t body_start = request_data.find("\r\n\r\n");
        if (body_start != string::npos && body_start + 4 < request_data.size()) {
            req.set_body(request_data.substr(body_start + 4));
        }

        // Parse cookies
        auto cookie_str = req.get_cookie();
        if (!cookie_str.empty()) {
            parse_cookies(cookie_str, req);
        }

        // Parse parameters
        parse_parameters(req);

        // Handle session
        const string session_id = req.get_cookie(session_cookie_name_);
        if (!session_id.empty() && session_manager_.session_exists(session_id)) {
            req.set_session(session_manager_.get_session(session_id, false));
        }

        return req;
    }

    virtual string build_response_str(const http_response& response) {
        ostringstream ss;

        if (!response.get_redirect().empty()) {
            ss << response.get_version() << " 302 Found\r\n";
            ss << "Location: " << response.get_redirect() << "\r\n";
        } else {
            ss << response.get_version() << " "
               << static_cast<uint16_t>(response.get_status()) << " "
               << response.get_status_msg() << "\r\n";
        }

        for (const auto& cookie : response.cookies) {
            ss << "Set-Cookie: " << cookie.to_string() << "\r\n";
        }
        if (response.get_redirect().empty() &&
            response.get_content_length().empty()) {
            ss << "Content-Length: " << response.get_body().size() << "\r\n";
        }
        for (auto iter = response.headers.begin(); iter != response.headers.end(); ++iter) {
            ss << iter->first << ": " << iter->second << "\r\n";
        }
        ss << "\r\n";

        if (!response.get_redirect().empty()) {
        } else {
            ss << response.get_body();
        }
        return ss.str();
    }

    void send_response(const socket::socket_t client_socket, const http_response& response) {
        string response_str = build_response_str(response);
        const char *data = response_str.data();
        const size_t total = response_str.size();
        size_t sent = 0;

        while (sent < total) {
#ifdef MSTL_PLATFORM_WINDOWS__
            int bytes_sent = ::send(client_socket, data + sent, static_cast<int>(total - sent), 0);
#elif defined(MSTL_PLATFORM_LINUX__)
            ssize_t bytes_sent = ::send(client_socket, data + sent, total - sent, 0);
#endif
            if (bytes_sent <= 0) {
                println("send failed");
                break;
            }
            sent += bytes_sent;
        }
    }

protected:
    void add_filter(filter* filter) {
        filter_chain_.add_filter(filter);
    }

    session* get_session(http_request& request, const bool create) {
        session* session = request.get_session();
        if (session) return session;

        string session_id = request.get_cookie(session_cookie_name_);
        if (!session_id.empty()) {
            session = session_manager_.get_session(session_id, false);
        }

        if (!session && create) {
            session = session_manager_.create_session();
        }
        request.set_session(session);
        return session;
    }

    session* get_session(http_request& request) {
        return get_session(request, false);
    }

    void add_session_cookie(http_request& request, http_response& response, session* session) const {
        if (session && session->is_new()) {
            cookie session_cookie(session_cookie_name_, session->get_id());
            session_cookie.set_path("/");
            session_cookie.set_http_only(true);

            const bool is_https = request.is_https();
            session_cookie.set_http_only(is_https);
            session_cookie.set_same_site(is_https ? "Strict" : "Lax");

            response.add_cookie(session_cookie);
            session->set_new(false);
        }
    }

    virtual bool init() { return true; }
    virtual void destroy() {}

    virtual http_response handle_request(http_request& request) {
        http_response response;

        if (!filter_chain_.execute_pre_filters(request, response)) {
            return response;
        }

        session* session = get_session(request, true);
        if (session) {
            add_session_cookie(request, response, session);
        }
        filter_chain_.execute_filters(request, response);

        print();
        println(
            "[", request.get_method(), "]", request.get_path(),
            "Query:", request.get_query(),
            "Body size:", request.get_body().size(),
            *request.get_session());
        print();

        const HTTP_METHOD& method = request.get_method();
        if (method.is_get()) {
            do_get(request, response);
        } else if (method.is_post()) {
            do_post(request, response);
        } else if (method.is_put()) {
            do_put(request, response);
        } else if (method.is_delete()) {
            do_delete(request, response);
        } else if (method.is_head()) {
            do_head(request, response);
        } else if (method.is_options()) {
            do_options(request, response);
        } else if (method.is_trace()) {
            do_trace(request, response);
        } else if (method.is_connect()) {
            do_connect(request, response);
        } else {
            response.set_status(HTTP_STATUS::S4_METHOD_NOT_ALLOWED);
            response.set_status_msg("Method Not Allowed");
            response.set_body("Method Not Allowed");
        }

        filter_chain_.execute_post_filters(request, response);
        return response;
    }

    virtual void do_get(http_request& request, http_response& response) = 0;

    virtual void do_post(http_request& request, http_response& response) = 0;

    virtual void do_put(http_request& request, http_response& response) {
        do_post(request, response);
    }

    virtual void do_delete(http_request& request, http_response& response) {
        do_post(request, response);
    }

    virtual void do_head(http_request& request, http_response& response) {
        do_get(request, response);
        response.set_body(""); // HEAD should not return body
    }

    virtual void do_options(http_request& request, http_response& response) {
        response.set_status(HTTP_STATUS::S2_NO_CONTENT);
        response.set_status_msg("No Content");
        response.set_allow_method(
            HTTP_METHOD::GET & HTTP_METHOD::POST & HTTP_METHOD::PUT
            & HTTP_METHOD::DELETE & HTTP_METHOD::OPTIONS
            );
        response.set_allow_headers("Content-Type, Cookie, Accept, X-Requested-With");
        response.set_max_age(86400);
    }

    virtual void do_trace(http_request& request, http_response& response) {
        response.set_ok();
        response.set_status_msg("OK");
        response.set_content_type(HTTP_CONTENT::HTML_MSG);

        ostringstream ss;
        ss << request.get_method().to_string() << " " << request.get_path();
        if (!request.get_query().empty()) ss << "?" << request.get_query();
        ss << " " << request.get_version() << "\r";
        for (auto iter = request.headers.begin(); iter != request.headers.end(); ++iter) {
            ss << iter->first << ": " << iter->second << "\r";
        }
        ss << "\r" << request.get_body();
        response.set_body(ss.str());
    }

    virtual void do_connect(http_request& request, http_response& response) {
        response.set_status(HTTP_STATUS::S4_METHOD_NOT_ALLOWED);
        response.set_status_msg("Method Not Allowed");
        response.set_body("CONNECT method not supported");
    }

public:
    explicit servlet(const uint16_t port = 8080, const int backlog = 128)
        : port_(port), backlog_(backlog) {
        _MSTL memory_set(&server_addr_, 0, sizeof(server_addr_));
    }

    virtual ~servlet() {
        this->stop();
    }

    bool start(const SOCKET_DOMAIN domain = SOCKET_DOMAIN::IPV4,
        const SOCKET_TYPE type = SOCKET_TYPE::STREAM,
        const SOCKET_PROTOCOL protocol = SOCKET_PROTOCOL::AUTO,
        const uint32_t thread_count = 5) {
        if (running_) return true;
        if (!init()) return false;

#ifdef MSTL_PLATFORM_WINDOWS__
        if (::WSAStartup(MAKEWORD(2, 2), &wsa_data_) != 0) {
            println("WSAStartup failed");
            return false;
        }
#endif

        server_socket_ = _MSTL move(socket(domain, type, protocol));
        if (!server_socket_.is_valid()) {
            println("socket creation failed");
            return false;
        }

        constexpr int opt = 1;
        if (::setsockopt(server_socket_.get(), SOL_SOCKET, SO_REUSEADDR,
#ifdef MSTL_PLATFORM_WINDOWS__
            reinterpret_cast<const char*>(&opt)
#elif defined(MSTL_PLATFORM_LINUX__)
            &opt
#endif
            , sizeof(opt))) {
            println("setsockopt failed");
            return false;
        }

        server_addr_.sin_family = AF_INET;
        server_addr_.sin_addr.s_addr = INADDR_ANY;
        server_addr_.sin_port = htons(port_);

        if (::bind(server_socket_.get(),
            reinterpret_cast<::sockaddr*>(&server_addr_),
            sizeof(server_addr_))) {
            println("bind failed");
            return false;
        }

        if (::listen(server_socket_.get(), backlog_) < 0) {
            println("listen failed");
            return false;
        }

        running_ = true;
        start_workers(static_cast<int32_t>(thread_count));
        return true;
    }

    void stop() {
        if (!running_) return;

        running_ = false;
        server_socket_.close();
#ifdef MSTL_PLATFORM_WINDOWS__
        WSACleanup();
#endif

        for (auto& t : worker_threads_) {
            if (t.joinable()) t.join();
        }
        worker_threads_.clear();
        destroy();
    }

    void set_session_cookie_name(const char* name) {
        session_cookie_name_ = name;
    }
    const string& get_session_cookie_name() const {
        return session_cookie_name_;
    }
};

MSTL_END_INNER__

using servlet = _INNER servlet;


MSTL_END_NAMESPACE__
#endif // MSTL_SERVLET_HPP__
