#include <NeForce/core/time/clocks.hpp>
#include <NeForce/network/http/rate_limiter.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

namespace {
    uint64_t current_time_ms() { return static_cast<uint64_t>(steady_clock::now().since_epoch().count() / 1'000'000); }
} // namespace


bool token_bucket::try_consume(const uint64_t now_ms) {
    lock<mutex> lk(mutex_);
    refill(now_ms);

    if (tokens >= 1.0) {
        tokens = tokens - 1.0;
        return true;
    }
    return false;
}

void token_bucket::refill(const uint64_t now_ms) {
    if (now_ms <= last_refill_ms) {
        return;
    }

    const double elapsed = static_cast<double>(now_ms - last_refill_ms) / 1000.0;
    const double new_tokens = elapsed * refill_rate;

    if (new_tokens < 0.001) {
        return;
    }

    tokens = (tokens + new_tokens > capacity) ? capacity : tokens + new_tokens;
    last_refill_ms = now_ms;
}

void token_bucket_limiter::set_default_rate(const double rate) { default_rate_ = rate; }
void token_bucket_limiter::set_default_burst(const double burst) { default_burst_ = burst; }

size_t token_bucket_limiter::size() const {
    lock<mutex> lk(mutex_);
    return buckets_.size();
}

bool token_bucket_limiter::allow(const string& key, double rate, double burst) {
    const uint64_t now = current_time_ms();

    {
        lock<mutex> lk(mutex_);
        auto it = buckets_.find(key);
        if (it != buckets_.end()) {
            it->second.last_access_ms = now;
            return it->second.bucket.try_consume(now);
        }

        if (buckets_.size() >= MAX_BUCKETS_COUNT) {
            auto lru = buckets_.begin();
            for (auto bucket = buckets_.begin(); bucket != buckets_.end(); ++bucket) {
                if (bucket->second.last_access_ms < lru->second.last_access_ms) {
                    lru = bucket;
                }
            }
            buckets_.erase(lru);
        }

        const double r = (rate > 0.0) ? rate : default_rate_;
        const double b = (burst > 0.0) ? burst : default_burst_;
        auto& entry = buckets_[key];
        entry.bucket = token_bucket(r, b);
        entry.last_access_ms = now;
        return entry.bucket.try_consume(now);
    }
}

void token_bucket_limiter::cleanup_expired(const seconds max_age) {
    const uint64_t now = current_time_ms();
    const uint64_t threshold = static_cast<uint64_t>(max_age.count()) * 1000;

    lock<mutex> lk(mutex_);
    for (auto it = buckets_.begin(); it != buckets_.end();) {
        if (now - it->second.last_access_ms >= threshold) {
            it = buckets_.erase(it);
        } else {
            ++it;
        }
    }
}

bool token_bucket_filter::pre_filter(http_request& request, http_response& response) {
    if (!enabled) {
        return true;
    }

    const string client_ip = request.client_ip();
    if (client_ip.empty()) {
        return true;
    }

    string key = client_ip;
    if (per_route) {
        key += ":" + request.path;
    }

    if (!limiter_.allow(key, default_rate, default_burst)) {
        response.status = http_status::S4_TOO_MANY_REQUESTS;
        response.status_message = "Too Many Requests";
        response.set_content_type(http_content::PLAIN_TEXT());
        response.body = "Rate limit exceeded. Please try again later.";
        response.set_header("Retry-After", "1");
        return false;
    }

    return true;
}

void token_bucket_filter::cleanup_expired(const seconds max_age) { limiter_.cleanup_expired(max_age); }

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
