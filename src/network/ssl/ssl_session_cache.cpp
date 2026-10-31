#include <NeForce/core/string/format.hpp>
#include <NeForce/network/ssl/ssl_session_cache.hpp>
#include <NeForce/network/ssl/ssl_stream.hpp>
#include <openssl/ssl.h>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    string session_key(const string& host, const ports port) { return host + ":" + format("{}", port.value()); }
} // namespace


ssl_session_cache::~ssl_session_cache() { clear(); }

void* ssl_session_cache::acquire(const string& host, const ports port) {
    lock<mutex> lk(mutex_);
    const auto it = sessions_.find(session_key(host, port));
    if (it == sessions_.end() || it->second.empty()) {
        misses_.fetch_add(1, memory_order_relaxed);
        return nullptr;
    }
    void* session = it->second.back();
    it->second.pop_back();
    hits_.fetch_add(1, memory_order_relaxed);
    if (it->second.empty()) {
        sessions_.erase(it);
    }
    return session;
}

void ssl_session_cache::store(const string& host, const ports port, void* session) {
    if (session == nullptr) {
        return;
    }
    lock<mutex> lk(mutex_);
    auto& slot = sessions_[session_key(host, port)];
    if (slot.size() >= max_sessions_per_endpoint) {
        ssl_stream::free_session(slot.front());
        slot.erase(slot.begin());
        evicted_.fetch_add(1, memory_order_relaxed);
    }
    slot.push_back(session);
    stored_.fetch_add(1, memory_order_relaxed);
}

ssl_session_cache_stats ssl_session_cache::stats() const noexcept {
    ssl_session_cache_stats result;
    result.hits = hits_.load(memory_order_relaxed);
    result.misses = misses_.load(memory_order_relaxed);
    result.stored = stored_.load(memory_order_relaxed);
    result.evicted = evicted_.load(memory_order_relaxed);
    return result;
}

void ssl_session_cache::clear() {
    lock<mutex> lk(mutex_);
    for (auto& entry: sessions_) {
        for (void* session: entry.second) {
            ssl_stream::free_session(session);
        }
    }
    sessions_.clear();
}

NEFORCE_END_NAMESPACE__
