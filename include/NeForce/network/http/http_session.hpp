#ifndef NEFORCE_NETWORK_HTTP_COOKIE_HPP__
#define NEFORCE_NETWORK_HTTP_COOKIE_HPP__
#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/time/datetime.hpp"
#include "NeForce/network/http/http_constants.hpp"
NEFORCE_BEGIN_NAMESPACE__

struct NEFORCE_API http_cookie : istringify<http_cookie> {
    HTTP_COOKIE_NAME name{};
    string value{};
    string domain{};
    string path = "/";
    int max_age = -1;
    bool secure = false;
    bool http_only = false;
    string same_site{HTTP_KEY::Strict};
    datetime expires{};

    NEFORCE_NODISCARD string to_string() const;

    NEFORCE_NODISCARD bool is_valid() const noexcept;
    NEFORCE_NODISCARD bool is_expired() const noexcept;

    void set_expires_from_now(int64_t seconds);
};


struct NEFORCE_API http_session : istringify<http_session> {
    string id;
    unordered_map<string, string> data;
    datetime last_access{datetime::now()};
    datetime create_time{datetime::now()};
    int max_age = 1800; // 30 minutes default
    bool is_new = true;
    bool invalidated = false;

    NEFORCE_NODISCARD string& operator[](const string& key);

    NEFORCE_NODISCARD string_view get(const string& key) const;
    void set(const string& key, string value);

    bool remove(const string& key);
    void clear();

    void invalidate() noexcept;

    void touch() noexcept;

    NEFORCE_NODISCARD bool contains(const string& key) const noexcept;

    NEFORCE_NODISCARD bool is_valid() const noexcept;

    NEFORCE_NODISCARD bool expired(int max_inactive = 0) const noexcept;

    NEFORCE_NODISCARD int64_t age() const noexcept { return datetime::now() - create_time; }

    NEFORCE_NODISCARD int64_t idle_time() const noexcept { return datetime::now() - last_access; }

    NEFORCE_NODISCARD string to_string() const;
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_COOKIE_HPP__
