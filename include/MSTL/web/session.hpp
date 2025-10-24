#ifndef MSTL_WEB_COOKIE_HPP__
#define MSTL_WEB_COOKIE_HPP__
#include "MSTL/core/unordered_map.hpp"
#include "MSTL/core/datetime.hpp"
#include "MSTL/core/console.hpp"
#include <mutex>
#include <thread>
MSTL_BEGIN_NAMESPACE__

struct MSTL_API cookie {
private:
    string name;
    string value;
    string domain;
    string path = "/";
    int max_age = -1;  // -1 means session cookie
    bool secure = false;
    bool http_only = false;
    string same_site; // "Strict", "Lax", "None"
    datetime expires_;

public:
    cookie() = default;
    cookie(string name, string value)
    : name(_MSTL move(name)), value(_MSTL move(value)) {}

    void set_name(string name) {
        this->name = _MSTL move(name);
    }
    const string& get_name() const {
        return this->name;
    }

    void set_value(string value) {
        this->value = _MSTL move(value);
    }
    const string& get_value() const {
        return this->value;
    }

    void set_domain(string domain) {
        this->domain = _MSTL move(domain);
    }
    const string& get_domain() const {
        return this->domain;
    }

    void set_path(string path) {
        this->path = _MSTL move(path);
    }
    const string& get_path() const {
        return this->path;
    }

    void set_max_age(const int age) {
        this->max_age = age;
    }
    int get_max_age() const {
        return this->max_age;
    }

    void set_secure(const bool secure) {
        this->secure = secure;
    }
    bool get_secure() const {
        return this->secure;
    }

    void set_http_only(const bool http_only) {
        this->http_only = http_only;
    }
    bool get_http_only() const {
        return this->http_only;
    }

    void set_same_site(string s) {
        this->same_site = _MSTL move(s);
    }
    const string& get_same_site() const {
        return this->same_site;
    }

    void set_expires(datetime expires) {
        this->expires_ = _MSTL move(expires);
    }
    datetime get_expires() const {
        return this->expires_;
    }

    string to_string() const {
        string result;
        result += name + "=" + value;

        if (!domain.empty()) result += "; Domain=" + domain;
        if (!path.empty()) result += "; Path=" + path;

        if (max_age >= 0) {
            result += "; Max-Age=" + _MSTL to_string(max_age);
        }

        if (expires_ > datetime::epoch()) {
            result += "; Expires=" + expires_.to_gmt();
        }

        if (secure) result += "; Secure";
        if (http_only) result += "; HttpOnly";
        if (!same_site.empty()) result += "; SameSite=" + same_site;

        return result;
    }
};


struct MSTL_API session {
private:
    string id;
    unordered_map<string, string> data;
    datetime last_access;
    datetime create_time;
    int max_age = 1800;  // 30 minutes default
    bool is_new_ = true;
    bool invalidated = false;

public:
    explicit session(string session_id)
        : id(_MSTL move(session_id)),
          last_access(datetime::now()),
          create_time(last_access) {}

    void set_id(string id) {
        this->id = _MSTL move(id);
    }
    const string& get_id() const {
        return this->id;
    }

    void set_last_access(const datetime& last_access) {
        this->last_access = last_access;
    }
    const datetime& get_last_access() const {
        return this->last_access;
    }

    void set_create_time(const datetime& create_time) {
        this->create_time = create_time;
    }
    const datetime& get_create_time() const {
        return this->create_time;
    }

    void set_max_age(const int seconds) {
        max_age = seconds;
    }
    int get_max_age() const {
        return max_age;
    }

    void set_new(const bool is_new) {
        this->is_new_ = is_new;
    }
    bool is_new() const {
        return this->is_new_;
    }

    void invalidate() {
        invalidated = true;
        data.clear();
    }
    bool is_valid() const {
        return !invalidated && !expired();
    }

    string& operator [](const string& key) {
        last_access = datetime::now();
        return data[key];
    }

    const unordered_map<string, string>& get_data() const {
        return data;
    }

    bool contains(const string& key) const {
        return data.find(key) != data.end();
    }

    void remove_attribute(const string& key) {
        data.erase(key);
        last_access = datetime::now();
    }

    bool expired(int max_inactive = 0) const {
        if (max_inactive <= 0) max_inactive = max_age;
        const datetime now = datetime::now();
        const int64_t diff = now - last_access;
        return diff > max_inactive;
    }
};

template <>
struct io_base<session> {
    static void write(sys_console& console, const session& s) {
        _MSTL print("Session ID: [", s.get_id(), "]", "Data: ");
        io_base<unordered_map<string, string>>::write(console, s.get_data());
    }
};


class MSTL_API servlet;

MSTL_BEGIN_INNER__
class __session_manager {
private:
    unordered_map<string, session> sessions_;
    std::mutex mutex_;
    std::atomic<bool> cleanup_running_{false};
    std::thread cleanup_thread_;

    friend class _MSTL servlet;

    static string generate_session_id() {
        string str;
        for (int i = 0; i < 32; ++i) {
            str += format("{x}", random_mt::next_int(0, 15));
        }
        return _MSTL move(str);
    }

    void cleanup_expired_sessions() {
        while (cleanup_running_) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                datetime now = datetime::now();
                auto it = sessions_.begin();
                while (it != sessions_.end()) {
                    const int64_t diff = now - it->second.get_last_access();
                    if (!it->second.is_valid() ||
                        diff > it->second.get_max_age()) {
                        it = sessions_.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::minutes(5));
        }
    }

    __session_manager() {
        cleanup_running_ = true;
        cleanup_thread_ = std::thread(&__session_manager::cleanup_expired_sessions, this);
    }

    ~__session_manager() {
        cleanup_running_ = false;
        if (cleanup_thread_.joinable()) {
            cleanup_thread_.join();
        }
    }

    session* get_session(const string& session_id, const bool create = true) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sessions_.find(session_id);
        if (it != sessions_.end()) {
            if (it->second.is_valid()) {
                it->second.set_new(false);
                return &it->second;
            }
            sessions_.erase(it);
        }

        if (create) {
            string new_id = session_id.empty() ? generate_session_id() : session_id;
            const auto pir = sessions_.emplace(new_id, session(new_id));
            return &pir.first->second;
        }

        return nullptr;
    }

    session* create_session() {
        return get_session("", true);
    }

    void remove_session(const string& session_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        sessions_.erase(session_id);
    }

    bool session_exists(const string& session_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sessions_.find(session_id);
        return it != sessions_.end() && it->second.is_valid();
    }
};
MSTL_END_INNER__


enum class HTTP_STATUS : uint16_t {
    // 1xx Message Codes

    // Server received the request header and can continue to send the request body
    S1_CONTINUE = 100,
    // Consent to switch protocols
    S1_SWITCH_PROTOCOL = 101,

    // 2xx Success Codes

    // Request succeed
    S2_OK = 200,
    // Request succeed and created new resources
    S2_CREATED = 201,
    // Request succeed but no content is returned
    S2_NO_CONTENT = 204,
    // Partial request succeed
    S2_PARTIAL_CONTENT = 206,

    // 3xx Redirect Codes

    // Resources are permanently migrated to the new URL
    S3_MOVED_PERMANENT = 301,
    // Resources are temporarily migrated to the new URL
    S3_FOUND = 302,
    // Resources are unmodified and use the local cache to save
    S3_NO_MODIFIED = 304,
    // Temporary redirect
    S3_TEMPORARY_REDIRECT = 307,
    // Permanent redirect
    S3_PERMANENT_REDIRECT = 308,

    // 4xx Client Error Codes

    // Request is in the wrong format
    S4_BAD_REQUEST = 400,
    // Identity verification is required
    S4_UNAUTHORIZED = 401,
    // Server rejects request
    S4_FORBIDDEN = 403,
    // The requested resource does not exist
    S4_NOT_FOUNT = 404,
    // The request method is not allowed or not existed
    S4_METHOD_NOT_ALLOWED = 405,
    // Client request timeout
    S4_REQUEST_TIMEOUT = 408,
    // The request is too large
    S4_PAYLOAD_LARGE = 413,
    // The URL of the request is too long
    S4_URL_LONG = 414,
    // Excessive number of requests
    S4_MANY_REQUESTS = 429,

    // 5xx Server Error Codes

    // Internal server error
    S5_INTERNAL_ERROR = 500,
    // Invalid upstream server response
    S5_BAD_GATEWAY = 502,
    // The service is temporarily unavailable
    S5_SERVICE_UNAVAILABLE = 503,
    // The upstream server responds too slowly
    S5_GATEWAY_TIMEOUT = 504,
    // The HTTP version of the request is not supported
    S5_HTTP_VERSION_NOT_SUPPORT = 505
};


struct MSTL_API HTTP_CONTENT {
    static const string HTML_TEXT;
    static const string XML_TEXT;
    static const string CSS_TEXT;
    static const string PLAIN_TEXT;
    static const string JSON_APP;
    static const string FORM_APP;
    static const string JPEG_IMG;
    static const string PNG_IMG;
    static const string BMP_IMG;
    static const string WEBP_IMG;
    static const string HTML_MSG;
};

#ifdef DELETE
#undef DELETE
#endif

struct MSTL_API HTTP_METHOD {
private:
    string method_{"UNKNOWN"};
public:
    explicit MSTL_CONSTEXPR20 HTTP_METHOD() = default;

    explicit MSTL_CONSTEXPR20 HTTP_METHOD(const string& method) : method_(method) {}
    HTTP_METHOD& operator =(const string& method) {
        method_ = method;
        return *this;
    }

    friend MSTL_CONSTEXPR20 HTTP_METHOD operator &(
        const HTTP_METHOD& lh, const HTTP_METHOD& rh) {
        return HTTP_METHOD(lh.method_ + ", " + rh.method_);
    }

    static const HTTP_METHOD GET;
    static const HTTP_METHOD POST;
    static const HTTP_METHOD HEAD;
    static const HTTP_METHOD PUT;
    static const HTTP_METHOD DELETE;
    static const HTTP_METHOD OPTIONS;
    static const HTTP_METHOD TRACE;
    static const HTTP_METHOD CONNECT;

    bool is_get() const {
        return method_ == GET.method_;
    }
    bool is_post() const {
        return method_ == POST.method_;
    }
    bool is_head() const {
        return method_ == HEAD.method_;
    }
    bool is_put() const {
        return method_ == PUT.method_;
    }
    bool is_delete() const {
        return method_ == DELETE.method_;
    }
    bool is_options() const {
        return method_ == OPTIONS.method_;
    }
    bool is_trace() const {
        return method_ == TRACE.method_;
    }
    bool is_connect() const {
        return method_ == CONNECT.method_;
    }

    string to_string() const {
        return method_;
    }
};

inline string to_string(const HTTP_METHOD& method) {
    return method.to_string();
}

template <>
struct io_base<HTTP_METHOD> {
    static void write(sys_console& console, const HTTP_METHOD& hm) {
        console.write_string(to_string(hm));
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_WEB_COOKIE_HPP__
