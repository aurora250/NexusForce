#include <NeForce/network/http/http_client_message.hpp>
#include <NeForce/network/util/url.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

string http_client_request::build_full_path() const {
    string full_path = path;

    if (!query_params.empty()) {
        full_path += "?";
        bool first = true;
        for (const auto& pair: query_params) {
            const auto& key = pair.first;
            const auto& value = pair.second;
            if (!first) {
                full_path += "&";
            }
            full_path += url::encode(key.view()) + "=" + url::encode(value.view());
            first = false;
        }
    }

    return full_path;
}

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
