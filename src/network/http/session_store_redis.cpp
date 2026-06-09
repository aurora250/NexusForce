#include <NeForce/network/http/session_store_redis.hpp>
#include <NeForce/network/util/url.hpp>
#ifdef NEFORCE_SUPPORT_HIREDIS
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

namespace {
    string serialize(const http_session& session) {
        string result;
        for (const auto& pair: session.data) {
            if (!result.empty()) {
                result += '&';
            }
            result += url::encode(pair.first.view()) + '=' + url::encode(pair.second.view());
        }
        return result;
    }

    optional<http_session> deserialize(const string& id, const string& data) {
        http_session session;
        session.id = id;
        session.is_new = false;

        size_t pos = 0;
        while (pos < data.length()) {
            const size_t eq = data.find('=', pos);
            if (eq == string::npos) {
                break;
            }
            size_t amp = data.find('&', eq + 1);
            if (amp == string::npos) {
                amp = data.length();
            }
            auto key = url::decode(data.view(pos, eq - pos));
            auto val = url::decode(data.view(eq + 1, amp - eq - 1));
            if (key && val) {
                session.data[string(key->view())] = string(val->view());
            }
            pos = amp + 1;
        }
        return session;
    }
} // namespace


optional<http_session> redis_session_store::load(const string& id) {
    const string key = prefix_ + id;
    auto result = conn_->get(key);
    if (!result || result->empty()) {
        return none;
    }
    return deserialize(id, string(result->value()));
}

void redis_session_store::save(const http_session& session) {
    const string key = prefix_ + session.id;
    const string data = serialize(session);
    conn_->setex(key, data, static_cast<int>(session.max_age.count()));
}

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
#endif
