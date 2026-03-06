#include <NeForce/network/http/http_server.hpp>
#include <NeForce/core/utility/hexadecimal.hpp>
#include <NeForce/core/encrypt/base64.hpp>
#include <NeForce/core/encrypt/sha1.hpp>
NEFORCE_BEGIN_NAMESPACE__

const string HTTP_KEY::Access_Control_Allow_Credentials = "Access-Control-Allow-Credentials";
const string HTTP_KEY::Access_Control_Allow_Headers = "Access-Control-Allow-Headers";
const string HTTP_KEY::Access_Control_Allow_Methods = "Access-Control-Allow-Methods";
const string HTTP_KEY::Access_Control_Allow_Origin = "Access-Control-Allow-Origin";
const string HTTP_KEY::Access_Control_Max_Age = "Access-Control-Max-Age";
const string HTTP_KEY::Connection = "Connection";
const string HTTP_KEY::Content_Length = "Content-Length";
const string HTTP_KEY::Content_Type = "Content-Type";
const string HTTP_KEY::Lax = "Lax";
const string HTTP_KEY::Strict = "Strict";
const string HTTP_KEY::X_Forwarded_Proto = "X-Forwarded-Proto";

string http_server_base::session_manager::generate_session_id()  {
    string str;
    for (int i = 0; i < 32; ++i) {
        str += format("{x}", rand_.next_int(0, 15));
    }
    return move(str);
}

http_server_base::session_manager::session_manager() {
    cleanup_running_ = true;
    cleanup_thread_ = thread(&session_manager::cleanup_expired_sessions, this);
}

http_server_base::session_manager::~session_manager() {
    cleanup_running_ = false;
    if (cleanup_thread_.joinable()) {
        cleanup_thread_.join();
    }
}

session* http_server_base::session_manager::get_session(const string& session_id, const bool create) {
    lock<mutex> lock(mutex_);
    const auto it = sessions_.find(session_id);
    if (it != sessions_.end()) {
        if (it->second.is_valid()) {
            it->second.is_new = false;
            return &it->second;
        }
        sessions_.erase(it);
    }

    if (create) {
        string new_id = session_id.empty() ? generate_session_id() : session_id;
        const auto pir = sessions_.emplace(new_id, _NEFORCE session(new_id));
        return &pir.first->second;
    }

    return nullptr;
}

void http_server_base::session_manager::remove_session(const string& session_id) noexcept {
    lock<mutex> lock(mutex_);
    sessions_.erase(session_id);
}

void http_server_base::session_manager::cleanup_expired_sessions()  {
    while (cleanup_running_) {
        {
            lock<mutex> lock(mutex_);
            datetime now = datetime::now();
            auto it = sessions_.begin();
            while (it != sessions_.end()) {
                const int64_t diff = now - it->second.last_access;
                if (!it->second.is_valid() || diff > it->second.max_age) {
                    it = sessions_.erase(it);
                } else {
                    ++it;
                }
            }
        }
        this_thread::sleep_for(minutes(5));
    }
}

bool http_server_base::session_manager::session_exists(const string& session_id) const noexcept {
    lock<mutex> lock(mutex_);
    const auto it = sessions_.find(session_id);
    return it != sessions_.end() && it->second.is_valid();
}

string http_server_base::compute_websocket_accept(const string_view key) {
    const string combined = string(key) + websocket_guid;
    const string sha1_result = sha1(combined);
    return base64_encode(cbyte_view{
        reinterpret_cast<const byte_t*>(sha1_result.data()),
        sha1_result.size()
    });
}

void http_server_base::parse_cookies(const string_view cookie_header, http_request& request) {
    if (cookie_header.empty()) return;

    vector<string_view> cookie_pairs;
    size_t start = 0;
    size_t end = cookie_header.find(';');

    while (end != string::npos) {
        cookie_pairs.push_back(cookie_header.substr(start, end - start).trim());
        start = end + 1;
        end = cookie_header.find(';', start);
    }
    cookie_pairs.push_back(cookie_header.substr(start).trim());

    for (const auto& pair : cookie_pairs) {
        const size_t eq_pos = pair.find('=');
        if (eq_pos != string::npos) {
            request.set_cookie(pair.substr(0, eq_pos).trim(), pair.substr(eq_pos + 1).trim());
        }
    }
}

http_request http_server_base::parse_request(tcp_socket* client_socket,
                                             session_manager& manager,
                                             const HTTP_COOKIE_NAME& name)  {
    http_request request;
    string request_data;
    char buffer[8192];

    while (true) {
        ssize_t bytes_read = client_socket->receive(memory_view<char>(buffer));
        if (bytes_read <= 0) {
            throw_exception(http_exception("Connection closed while reading request"));
        }
        request_data.append(buffer, bytes_read);

        const size_t header_end = request_data.find("\r\n\r\n");
        if (header_end != string::npos) {
            size_t content_length = 0;
            const size_t cl_pos = request_data.find("Content-Length:");
            if (cl_pos != string::npos && cl_pos < header_end) {
                const size_t cl_end = request_data.find("\r\n", cl_pos);
                if (cl_end != string::npos) {
                    const string_view cl_str = request_data.view(cl_pos + 15, cl_end - cl_pos - 15).trim();
                    content_length = uinteger32::parse(cl_str);
                }
            }

            const size_t body_start = header_end + 4;
            const size_t body_received = request_data.size() - body_start;
            if (body_received < content_length) {
                size_t remaining = content_length - body_received;
                while (remaining > 0) {
                    bytes_read = client_socket->receive(memory_view<char>(buffer, sizeof(buffer)));
                    if (bytes_read <= 0) break;
                    request_data.append(buffer, bytes_read);
                    remaining -= bytes_read;
                }
            }
            break;
        }

        if (request_data.size() > 16 * 1024) {
            throw_exception(http_exception("Request header too large"));
        }
    }

    // 解析请求行
    string_view line;
    size_t pos = 0;
    if (getline(request_data.view(), pos, line)) {
        line = line.trim();
        const size_t pos1 = line.find(' ');
        if (pos1 != string::npos) {
            request.method = string{line.substr(0, pos1)};
            const size_t pos2 = line.find(' ', pos1 + 1);
            if (pos2 != string::npos) {
                request.path = line.substr(pos1 + 1, pos2 - pos1 - 1);
                request.version = line.substr(pos2 + 1);
            }
        }
    }

    // 解析 URL 参数
    const size_t query_pos = request.path.find('?');
    if (query_pos != string::npos) {
        request.query = request.path.substr(query_pos + 1);
        request.path = request.path.substr(0, query_pos);
    }

    // 解析头部
    while (_NEFORCE getline(request_data.view(), pos, line)) {
        line = line.trim();
        if (line.empty()) break;

        const size_t colon_pos = line.find(':');
        if (colon_pos != string::npos) {
            string key{line.substr(0, colon_pos).trim()};
            string value{line.substr(colon_pos + 1).trim()};
            request.set_header(key, _NEFORCE move(value));
        }
    }

    // 解析 Body
    const size_t body_start = request_data.find("\r\n\r\n");
    if (body_start != string::npos && body_start + 4 < request_data.size()) {
        request.body = request_data.substr(body_start + 4);
    }

    // 解析 Cookie
    const string& cookie_str = request.header("Cookie");
    if (!cookie_str.empty()) {
        parse_cookies(cookie_str.view(), request);
    }

    parse_parameters(request);

    const string& session_id = request.cookie(name.cookie_name());
    if (!session_id.empty() && manager.session_exists(session_id)) {
        request.session = manager.get_session(session_id, false);
    }

    return request;
}

_NEFORCE session* http_server_base::session(
        http_request& request,
        bool create,
        session_manager& manager,
        const HTTP_COOKIE_NAME& name) {
    _NEFORCE session* sess = request.session;
    if (sess) return sess;

    const string& session_id = request.cookie(name.cookie_name());
    if (!session_id.empty()) {
        sess = manager.get_session(session_id, false);
    }

    if (!sess && create) {
        sess = manager.get_session("", true);
    }
    request.session = sess;
    return sess;
}

void http_server_base::send_response(tcp_socket* client_socket, const http_response& response) {
    string res_str = build_response_str(response);
    client_socket->send_all(memory_view<const char>(res_str.begin(), res_str.size()));
}


static NEFORCE_ALWAYS_INLINE_INLINE
string url_decode(const string_view str) {
    string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            try {
                result += static_cast<char>(hexadecimal::parse(str.substr(i + 1, 2)).value());
                i += 2;
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

static NEFORCE_ALWAYS_INLINE_INLINE
void parse_url_encoded(const string_view data, unordered_map<string, string>& params)  {
    if (data.empty()) return;

    vector<string_view> pairs;
    size_t start = 0;
    size_t end = data.find('&');

    while (end != string::npos) {
        pairs.push_back(data.substr(start, end - start));
        start = end + 1;
        end = data.find('&', start);
    }
    pairs.push_back(data.substr(start));

    for (const auto& pair : pairs) {
        const size_t eq_pos = pair.find('=');
        if (eq_pos != string_view::npos) {
            string name = url_decode(pair.substr(0, eq_pos));
            params[name] = url_decode(pair.substr(eq_pos + 1));
        }
    }
}

void http_server_base::parse_parameters(http_request& request) {
    if (!request.query.empty()) {
        parse_url_encoded(request.query.view(), request.parameters);
    }
    if (request.method.is_post() && !request.body.empty()) {
        if (HTTP_CONTENT::is_form_app(request.header(HTTP_KEY::Content_Type))) {
            parse_url_encoded(request.body.view(), request.parameters);
        }
    }
}

string http_server_base::build_response_str(const http_response& response) {
    string result;
    if (!response.redirect_url.empty()) {
        result += response.version + " 302 Found\r\n";
        result += "Location: " + response.redirect_url + "\r\n";
    } else {
        result += response.version + " " +
            _NEFORCE to_string(static_cast<uint16_t>(response.status)) + " " +
            response.status_message + "\r\n";
    }

    for (const auto& cookie : response.cookies) {
        result += "Set-Cookie: " + cookie.to_string() + "\r\n";
    }

    if (response.redirect_url.empty() && response.header(HTTP_KEY::Content_Length).empty()) {
        result += HTTP_KEY::Content_Length + ": " + _NEFORCE to_string(response.body.size()) + "\r\n";
    }

    for (const auto& [key, value] : response.headers) {
        result += key + ": " + value + "\r\n";
    }
    result += "\r\n";

    if (response.redirect_url.empty()) {
        result += response.body;
    }
    return result;
}

void http_server_base::add_session_cookie(const http_request& request, http_response& response,
                                          _NEFORCE session* session, const HTTP_COOKIE_NAME& name) {
    if (session && session->is_new) {
        cookie session_cookie;
        session_cookie.name = name;
        session_cookie.value = session->id;
        session_cookie.path = "/";
        session_cookie.http_only = true;

        const bool is_https = request.header(HTTP_KEY::X_Forwarded_Proto) == "https";
        session_cookie.secure = is_https;
        session_cookie.same_site = is_https ? HTTP_KEY::Strict : HTTP_KEY::Lax;

        response.cookies.emplace_back(move(session_cookie));
        session->is_new = false;
    }
}

NEFORCE_END_NAMESPACE__
