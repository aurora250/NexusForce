#include <NeForce/network/http/health_check.hpp>
#include <NeForce/core/time/clocks.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

bool health_check_filter::pre_filter(http_request& request, http_response& response) {
    if (!enabled) {
        return true;
    }
    if (!request.method.is_get() || request.path != path) {
        return true;
    }

    bool all_healthy = true;

    if (show_details) {
        auto [body, ok] = build_health_json();
        response.body = move(body);
        all_healthy = ok;
    } else {
        for (auto& kv: checks_) {
            if (kv.second && !kv.second()) {
                all_healthy = false;
                break;
            }
        }
        response.body = all_healthy ? R"({"status":"ok"})" : R"({"status":"unhealthy"})";
    }

    response.set_content_type(http_content::JSON_APP());
    response.status = all_healthy ? http_status::S2_OK : http_status::S5_SERVICE_UNAVAILABLE;
    response.status_message = all_healthy ? "OK" : "Service Unavailable";
    response.headers["Cache-Control"] = "no-cache, no-store, must-revalidate";
    return false;
}

pair<string, bool> health_check_filter::build_health_json() {
    if (start_time_ms_ == 0) {
        start_time_ms_ = static_cast<int64_t>(steady_clock::now().since_epoch().count() / 1'000'000);
    }

    int64_t now_ms = static_cast<int64_t>(steady_clock::now().since_epoch().count() / 1'000'000);
    int64_t uptime_s = (now_ms - start_time_ms_) / 1000;

    string json = R"({"status":"ok","uptime":)" + to_string(uptime_s);
    bool all_ok = true;

    if (!checks_.empty()) {
        json += ",\"checks\":{";
        bool first = true;
        for (auto& kv: checks_) {
            if (!first) {
                json += ",";
            }
            first = false;
            bool ok = kv.second ? kv.second() : true;
            if (!ok) {
                all_ok = false;
            }
            json += "\"" + kv.first + "\":\"" + (ok ? "ok" : "fail") + "\"";
        }
        json += "}";

        if (!all_ok) {
            auto pos = json.find("\"ok\"");
            if (pos != string::npos) {
                json.replace(pos, 4, "\"unhealthy\"");
            }
        }
    }

    json += "}";
    return {json, all_ok};
}

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
