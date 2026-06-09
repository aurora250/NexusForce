#include <NeForce/core/numeric/random.hpp>
#include <NeForce/network/http/load_balancer.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

lb_backend* load_balancer::select_backend() {
    lock<mutex> lk(mutex_);

    if (backends_.empty()) {
        return nullptr;
    }

    switch (strategy_) {
        case lb_strategy::ROUND_ROBIN:
            return select_round_robin();
        case lb_strategy::LEAST_CONNECTIONS:
            return select_least_connections();
        case lb_strategy::WEIGHTED:
            return select_weighted();
        case lb_strategy::RANDOM:
            return select_random();
    }
    return nullptr;
}

void load_balancer::release_backend(lb_backend* backend) {
    if (backend != nullptr) {
        --backend->active_connections;
    }
}

void load_balancer::mark_failure(lb_backend* backend) {
    if (backend == nullptr) {
        return;
    }
    backend->consecutive_failures++;
    if (backend->consecutive_failures >= max_failures_) {
        backend->healthy = false;
    }
}

void load_balancer::run_health_checks() {
    auto now_ms = static_cast<uint64_t>(steady_clock::now().since_epoch().count() / 1'000'000);

    lock<mutex> lk(mutex_);
    for (auto& backend: backends_) {
        if (!backend.healthy &&
            now_ms - backend.last_health_check_ms >= static_cast<uint64_t>(health_check_interval_.count())) {
            backend.last_health_check_ms = now_ms;

            if (health_checker_) {
                if (health_checker_(backend)) {
                    backend.healthy = true;
                    backend.consecutive_failures = 0;
                }
            } else {
                backend.healthy = true;
                backend.consecutive_failures = 0;
            }
        }
    }
}

size_t load_balancer::healthy_count() const {
    lock<mutex> lk(mutex_);
    size_t count = 0;
    for (const auto& b: backends_) {
        if (b.healthy) {
            count++;
        }
    }
    return count;
}

lb_backend* load_balancer::select_round_robin() {
    size_t attempts = 0;
    while (attempts < backends_.size()) {
        size_t idx = rr_counter_++ % backends_.size();
        if (backends_[idx].healthy) {
            ++backends_[idx].active_connections;
            return &backends_[idx];
        }
        attempts++;
    }
    return nullptr;
}

lb_backend* load_balancer::select_least_connections() {
    lb_backend* best = nullptr;
    size_t min_conn = numeric_traits<size_t>::max();
    for (auto& b: backends_) {
        if (b.healthy) {
            size_t conn = b.active_connections;
            if (conn < min_conn) {
                min_conn = conn;
                best = &b;
            }
        }
    }
    if (best != nullptr) {
        ++best->active_connections;
    }
    return best;
}

lb_backend* load_balancer::select_weighted() {
    size_t total_weight = 0;
    for (auto& b: backends_) {
        if (b.healthy) {
            total_weight += b.weight;
        }
    }
    if (total_weight == 0) {
        return nullptr;
    }

    auto target = secret::next_int<size_t>(total_weight);
    size_t cumulative = 0;
    for (auto& b: backends_) {
        if (b.healthy) {
            cumulative += b.weight;
            if (target < cumulative) {
                ++b.active_connections;
                return &b;
            }
        }
    }
    return nullptr;
}

lb_backend* load_balancer::select_random() {
    vector<lb_backend*> healthy;
    for (auto& b: backends_) {
        if (b.healthy) {
            healthy.push_back(&b);
        }
    }
    if (healthy.empty()) {
        return nullptr;
    }

    auto idx = secret::next_int<size_t>(healthy.size());
    ++healthy[idx]->active_connections;
    return healthy[idx];
}

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
