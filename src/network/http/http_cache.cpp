#include <NeForce/network/http/http_cache.hpp>
#include <NeForce/core/encrypt/sha1.hpp>
#include <NeForce/core/numeric/numeric_traits.hpp>
#include <NeForce/core/time/clocks.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

optional<cached_response> response_cache::get(const string& key) {
    shared_lock<shared_mutex> lk(mutex_);
    auto it = entries_.find(key);
    if (it == entries_.end()) {
        return none;
    }

    auto& entry = it->second;
    auto age = seconds{datetime::now() - entry.response.cached_at};

    if (age >= entry.response.max_age) {
        return none;
    }

    entry.last_access_ms = static_cast<uint64_t>(steady_clock::now().since_epoch().count() / 1'000'000);

    return entry.response;
}

void response_cache::put(const string& key, const http_response& response, seconds max_age) {
    if (response.body.size() > max_body_size.bytes()) {
        return;
    }

    cached_response cached;
    cached.status = response.status;
    cached.status_message = response.status_message;
    cached.headers = response.headers;
    cached.body = response.body;
    cached.etag = generate_etag(response.body.view());
    cached.cached_at = datetime::now();
    cached.max_age = max_age.count() > 0 ? max_age : default_max_age;

    lock<shared_mutex> lk(mutex_);

    if (entries_.size() >= max_entries) {
        evict_lru();
    }

    uint64_t now_ms = static_cast<uint64_t>(steady_clock::now().since_epoch().count() / 1'000'000);
    entries_[key] = {move(cached), now_ms};
}

void response_cache::remove(const string& key) {
    lock<shared_mutex> lk(mutex_);
    entries_.erase(key);
}

void response_cache::cleanup() {
    lock<shared_mutex> lk(mutex_);
    auto now = datetime::now();
    for (auto it = entries_.begin(); it != entries_.end();) {
        auto age = seconds{now - it->second.response.cached_at};
        if (age >= it->second.response.max_age) {
            it = entries_.erase(it);
        } else {
            ++it;
        }
    }
}

void response_cache::clear() {
    lock<shared_mutex> lk(mutex_);
    entries_.clear();
}

string response_cache::generate_etag(string_view body) {
    auto hash = SHA1::hash_hex({reinterpret_cast<const byte_t*>(body.data()), body.size()});
    return "\"" + hash + "\"";
}

string response_cache::build_key(const http_method& method, string_view url) { return method.to_string() + ":" + url; }

void response_cache::evict_lru() {
    uint64_t oldest_ms = numeric_traits<uint64_t>::max();
    string oldest_key;

    for (auto& kv: entries_) {
        if (kv.second.last_access_ms < oldest_ms) {
            oldest_ms = kv.second.last_access_ms;
            oldest_key = kv.first;
        }
    }

    if (!oldest_key.empty()) {
        entries_.erase(oldest_key);
    }
}

bool cache_filter::pre_filter(http_request& request, http_response& response) {
    if (!enabled || cache_ == nullptr) {
        return true;
    }
    if (cache_only_get && !request.method.is_get()) {
        return true;
    }

    string key = response_cache::build_key(request.method, request.path.view());
    auto cached = cache_->get(key);

    if (cached) {
        auto if_none_match = request.header("If-None-Match");
        if (!if_none_match.empty() && if_none_match == cached->etag) {
            response.status = http_status::S3_NOT_MODIFIED;
            response.status_message = "Not Modified";
            response.set_header("ETag", cached->etag);
            return false;
        }

        response.status = cached->status;
        response.status_message = cached->status_message;
        response.headers = cached->headers;
        response.body = cached->body;
        response.set_header("ETag", cached->etag);
        response.set_header("X-Cache", "HIT");
        return false;
    }

    response.set_header("X-Cache", "MISS");
    return true;
}

void cache_filter::post_filter(http_request& request, http_response& response) {
    if (!enabled || cache_ == nullptr) {
        return;
    }
    if (cache_only_get && !request.method.is_get()) {
        return;
    }
    if (response.status != http_status::S2_OK) {
        return;
    }

    string cc = response.header("Cache-Control");
    if (cc.contains("no-cache") || cc.contains("no-store") || cc.contains("private")) {
        return;
    }

    string key = response_cache::build_key(request.method, request.path.view());
    cache_->put(key, response);
}

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
