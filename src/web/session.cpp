#include <MSTL/web/session.hpp>
MSTL_BEGIN_NAMESPACE__

void cookie::swap(cookie& other) noexcept {
    if (_MSTL addressof(other) == this) return;
    name_ = _MSTL move(other.name_);
    value_ = _MSTL move(other.value_);
    domain_ = _MSTL move(other.domain_);
    path_ = _MSTL move(other.path_);
    max_age_ = other.max_age_;
    secure_ = other.secure_;
    http_only_ = other.http_only_;
    same_site_ = _MSTL move(other.same_site_);
    expires_ = _MSTL move(other.expires_);
    other.max_age_ = -1;
    other.secure_ = false;
    other.http_only_  = false;
}

MSTL_NODISCARD string cookie::to_string() const {
    string result;
    result += name_.cookie_name() + "=" + value_;

    if (!domain_.empty()) result += "; Domain=" + domain_;
    if (!path_.empty()) result += "; Path=" + path_;

    if (max_age_ >= 0) {
        result += "; Max-Age=" + _MSTL to_string(max_age_);
    }
    if (expires_ > datetime::epoch()) {
        result += "; Expires=" + expires_.to_GMT();
    }

    if (secure_) result += "; Secure";
    if (http_only_) result += "; HttpOnly";
    if (!same_site_.empty()) result += "; SameSite=" + same_site_;

    return result;
}

MSTL_NODISCARD bool session::expired(int max_inactive) const noexcept {
    if (max_inactive <= 0) max_inactive = max_age_;
    const datetime now = datetime::now();
    const int64_t diff = now - last_access_;
    return diff > max_inactive;
}

MSTL_NODISCARD string session::to_string() const {
    string result;
    result += "Session ID: ["_s + _MSTL to_string(id_) + "]" + "Data: ";
    result += data_.to_string();
    return result;
}


MSTL_BEGIN_INNER__

MSTL_NODISCARD string __session_manager::generate_session_id() {
    string str;
    for (int i = 0; i < 32; ++i) {
        str += format("{x}", random_mt::next_int(0, 15));
    }
    return _MSTL move(str);
}

__session_manager::__session_manager() {
    cleanup_running_ = true;
    cleanup_thread_ = std::thread(&__session_manager::cleanup_expired_sessions, this);
}

__session_manager::~__session_manager() {
    cleanup_running_ = false;
    if (cleanup_thread_.joinable()) {
        cleanup_thread_.join();
    }
}

MSTL_NODISCARD session* __session_manager::get_session(const string& session_id, const bool create) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = sessions_.find(session_id);
    if (it != sessions_.end()) {
        if (it->second.is_valid()) {
            it->second.set_is_new(false);
            return &it->second;
        }
        sessions_.erase(it);
    }

    if (create) {
        string new_id = session_id.empty() ? generate_session_id() : session_id;
        const auto pir = sessions_.emplace(new_id, session(new_id));
        return &pir.first->second;
    }

    return nullptr;
}

void __session_manager::cleanup_expired_sessions() {
    while (cleanup_running_) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            datetime now = datetime::now();
            auto it = sessions_.begin();
            while (it != sessions_.end()) {
                const int64_t diff = now - it->second.last_access();
                if (!it->second.is_valid() ||
                    diff > it->second.max_age()) {
                    it = sessions_.erase(it);
                    } else {
                        ++it;
                    }
            }
        }
        std::this_thread::sleep_for(std::chrono::minutes(5));
    }
}

void __session_manager::remove_session(const string& session_id) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.erase(session_id);
}

MSTL_NODISCARD bool __session_manager::session_exists(const string& session_id) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = sessions_.find(session_id);
    return it != sessions_.end() && it->second.is_valid();
}

MSTL_END_INNER__

MSTL_END_NAMESPACE__
