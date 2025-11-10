#ifndef MSTL_WEB_COOKIE_HPP__
#define MSTL_WEB_COOKIE_HPP__
#include "MSTL/core/unordered_map.hpp"
#include "MSTL/core/datetime.hpp"
#include "http_constants.hpp"
#include <mutex>
#include <atomic>
#include <thread>
MSTL_BEGIN_NAMESPACE__


struct MSTL_API cookie : istringify<cookie> {
private:
    HTTP_COOKIE_NAME name_{};
    string value_{};
    string domain_{};
    string path_ = "/";
    int max_age_ = -1;  // -1 means session cookie
    bool secure_ = false;
    bool http_only_ = false;
    string same_site_{"Strict"}; // "Strict", "Lax", "None"
    datetime expires_{};

public:
    cookie() = default;
    cookie(const cookie&) = default;
    cookie& operator =(const cookie&) = default;

    cookie(cookie&& other) noexcept {
        swap(other);
    }
    cookie& operator =(cookie&& other) noexcept {
        if (_MSTL addressof(other) == this) return *this;
        swap(other);
        return *this;
    }

    cookie(HTTP_COOKIE_NAME name, string value) : name_(_MSTL move(name)), value_(_MSTL move(value)) {}
    cookie(const string_view name, const string_view value) : cookie(HTTP_COOKIE_NAME(string{name}), string(value)) {}
    cookie(const char* name, const char* value) : cookie(HTTP_COOKIE_NAME(name), string(value)) {}

    ~cookie() = default;


    void set_name(HTTP_COOKIE_NAME name) noexcept { this->name_ = _MSTL move(name); }
    MSTL_NODISCARD const HTTP_COOKIE_NAME& name() const noexcept { return this->name_; }

    void set_value(string value) noexcept { this->value_ = _MSTL move(value); }
    MSTL_NODISCARD const string& value() const noexcept { return this->value_; }

    void set_domain(string domain) noexcept { this->domain_ = _MSTL move(domain); }
    MSTL_NODISCARD const string& domain() const noexcept { return this->domain_; }

    void set_path(string path) noexcept { this->path_ = _MSTL move(path); }
    MSTL_NODISCARD const string& path() const noexcept { return this->path_; }

    void set_max_age(const int age) noexcept { this->max_age_ = age; }
    MSTL_NODISCARD int max_age() const noexcept { return this->max_age_; }

    void set_secure(const bool secure) noexcept { this->secure_ = secure; }
    MSTL_NODISCARD bool secure() const noexcept { return this->secure_; }

    void set_http_only(const bool http_only) noexcept { this->http_only_ = http_only; }
    MSTL_NODISCARD bool http_only() const noexcept { return this->http_only_; }

    void set_same_site(string s) noexcept { this->same_site_ = _MSTL move(s); }
    MSTL_NODISCARD const string& same_site() const noexcept { return this->same_site_; }

    void set_expires(datetime expires) noexcept { this->expires_ = _MSTL move(expires); }
    MSTL_NODISCARD datetime expires() const noexcept { return this->expires_; }


    void invalidate() noexcept {
        set_max_age(0);
        set_expires(datetime::epoch());
    }

    void swap(cookie& other) noexcept;

    MSTL_NODISCARD string to_string() const;
};


struct MSTL_API session : istringify<session> {
private:
    string id_;
    unordered_map<string, string> data_;
    datetime last_access_;
    datetime create_time_;
    int max_age_ = 1800;  // 30 minutes default
    bool is_new_ = true;
    bool invalidated_ = false;

public:
    explicit session(string session_id)
    : id_(_MSTL move(session_id)),
    last_access_(datetime::now()),
    create_time_(last_access_) {}

    ~session() = default;


    void set_id(string id) noexcept { this->id_ = _MSTL move(id); }
    MSTL_NODISCARD const string& id() const noexcept { return this->id_; }

    void set_last_access(datetime last_access) noexcept { this->last_access_ = _MSTL move(last_access); }
    MSTL_NODISCARD datetime last_access() const noexcept { return this->last_access_; }

    void set_create_time(datetime create_time) noexcept { this->create_time_ = _MSTL move(create_time); }
    MSTL_NODISCARD datetime create_time() const noexcept { return this->create_time_; }

    void set_max_age(const int seconds) noexcept { max_age_ = seconds; }
    MSTL_NODISCARD int max_age() const noexcept { return max_age_; }

    void set_is_new(const bool is_new) noexcept { this->is_new_ = is_new; }
    MSTL_NODISCARD bool is_new() const noexcept { return this->is_new_; }

    MSTL_NODISCARD const unordered_map<string, string>& get_data() const noexcept { return data_; }


    void invalidate() noexcept { invalidated_ = true; data_.clear(); }

    MSTL_NODISCARD bool is_valid() const noexcept { return !invalidated_ && !expired(); }


    MSTL_NODISCARD string& operator [](const string& key) {
        last_access_ = datetime::now();
        return data_[key];
    }

    MSTL_NODISCARD bool contains(const string& key) const {
        return data_.find(key) != data_.end();
    }


    void remove_attribute(const string& key) noexcept {
        data_.erase(key);
        last_access_ = datetime::now();
    }

    MSTL_NODISCARD bool expired(int max_inactive = 0) const noexcept;

    MSTL_NODISCARD string to_string() const;
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

    MSTL_NODISCARD static string generate_session_id();

    __session_manager();
    ~__session_manager();

    MSTL_NODISCARD session* get_session(const string& session_id, bool create = true);
    MSTL_NODISCARD session* create_session() { return get_session("", true); }

    void remove_session(const string& session_id) noexcept;
    void cleanup_expired_sessions();

    MSTL_NODISCARD bool session_exists(const string& session_id) noexcept;
};
MSTL_END_INNER__

MSTL_END_NAMESPACE__
#endif // MSTL_WEB_COOKIE_HPP__
