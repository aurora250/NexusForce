#ifndef NEFORCE_NETWORK_HTTP_COOKIE_HPP__
#define NEFORCE_NETWORK_HTTP_COOKIE_HPP__
#include "NeForce/core/async/thread.hpp"
#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/time/datetime.hpp"
#include "NeForce/network/http/http_constants.hpp"
NEFORCE_BEGIN_NAMESPACE__

struct NEFORCE_API HTTP_KEY {
    static const string Access_Control_Allow_Credentials;
    static const string Access_Control_Allow_Headers;
    static const string Access_Control_Allow_Methods;
    static const string Access_Control_Allow_Origin;
    static const string Access_Control_Max_Age;
    static const string Connection;
    static const string Content_Length;
    static const string Content_Type;
    static const string Lax;
    static const string Strict;
    static const string X_Forwarded_Proto;
};


struct NEFORCE_API cookie : istringify<cookie> {
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
};


struct NEFORCE_API session : istringify<session> {
    string id;
    unordered_map<string, string> data;
    datetime last_access{datetime::now()};
    datetime create_time{datetime::now()};
    int max_age = 1800;  // 30 minutes default
    bool is_new = true;
    bool invalidated = false;

    explicit session(string session_id)
    : id(_NEFORCE move(session_id)) {}

    NEFORCE_NODISCARD string& operator [](const string& key) {
        last_access = datetime::now();
        return data[key];
    }

    NEFORCE_NODISCARD bool is_valid() const noexcept {
        return !invalidated && !expired();
    }

    NEFORCE_NODISCARD bool expired(int max_inactive = 0) const noexcept;

    NEFORCE_NODISCARD string to_string() const;
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_COOKIE_HPP__
