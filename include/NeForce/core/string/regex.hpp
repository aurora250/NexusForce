#ifndef NEFORCE_CORE_STRING_REGEX_HPP__
#define NEFORCE_CORE_STRING_REGEX_HPP__
#ifdef NEFORCE_SUPPORT_PCRE2
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/functional/function.hpp"
#include "NeForce/core/memory/unique_ptr.hpp"
#include "NeForce/core/string/string.hpp"
#include <pcre2.h>
NEFORCE_BEGIN_NAMESPACE__

NEFORCE_ERROR_BUILD_FINAL_CLASS(regex_exception, value_exception, "Regex Operation Failed")


class NEFORCE_API match_result {
public:
    using iterator = vector<string>::const_iterator;

private:
    vector<string> groups_;
    vector<pair<size_t, size_t>> group_positions_;
    size_t position_ = string::npos;
    size_t length_ = 0;
    string subject_;

public:
    match_result() = default;
    
    match_result(
        const string& subject,
        size_t pos, size_t len,
        const vector<string>& groups,
        const vector<pair<size_t, size_t>>& group_positions);
    
    NEFORCE_NODISCARD bool matched() const noexcept {
        return position_ != string::npos;
    }
    NEFORCE_NODISCARD size_t position() const noexcept {
        return position_;
    }
    NEFORCE_NODISCARD size_t length() const noexcept {
        return length_;
    }
    NEFORCE_NODISCARD string_view data() const noexcept {
        return matched() ? groups_[0].view() : ""_sv;
    }
    NEFORCE_NODISCARD size_t size() const noexcept {
        return groups_.size();
    }
    
    NEFORCE_NODISCARD string_view operator [](const size_t idx) const noexcept {
        if (idx >= groups_.size()) return ""_sv;
        return groups_[idx].view();
    }

    NEFORCE_NODISCARD pair<size_t, size_t> position(const size_t idx) const noexcept {
        if (idx >= group_positions_.size()) {
            return {string::npos, 0};
        }
        return group_positions_[idx];
    }

    NEFORCE_NODISCARD string_view prefix() const noexcept {
        if (!matched()) return ""_sv;
        return subject_.view(0, position_);
    }

    NEFORCE_NODISCARD string_view suffix() const noexcept {
        if (!matched()) return ""_sv;
        return subject_.view(position_ + length_);
    }

    string format(string_view fmt) const;

    NEFORCE_NODISCARD iterator begin() const noexcept {
        return groups_.begin();
    }

    NEFORCE_NODISCARD iterator end() const noexcept {
        return groups_.end();
    }
};


class NEFORCE_API regex {
private:
    struct pcre2_code_deleter {
        void operator ()(pcre2_code* code) const noexcept {
            if (code) pcre2_code_free(code);
        }
    };

    struct pcre2_match_data_deleter {
        void operator ()(pcre2_match_data* data) const noexcept {
            if (data) pcre2_match_data_free(data);
        }
    };

    unique_ptr<pcre2_code, pcre2_code_deleter> code_;
    string pattern_;
    uint32_t options_;
    int capture_count_ = 0;

    friend class regex_iterator;
    friend class regex_token_iterator;

private:
    void compile(const string& pattern, uint32_t options = 0);
    
    match_result do_match(PCRE2_SPTR subject, size_t length,
                          size_t start_offset, uint32_t options,
                          const string& subject_str) const;
    
public:
    explicit regex(const string& pattern, uint32_t options = 0);
    
    regex(regex&& other) noexcept;
    regex& operator =(regex&& other) noexcept;
    
    regex(const regex&) = delete;
    regex& operator =(const regex&) = delete;

    NEFORCE_NODISCARD bool match(const string& str) const;
    
    NEFORCE_NODISCARD match_result search(const string& str, size_t pos = 0) const;

    NEFORCE_NODISCARD vector<match_result> find_all(const string& str) const;

    string replace_first(const string& str, string_view fmt) const;
    
    string replace_all(const string& str, string_view fmt) const;

    string replace_all_callback(const string& str, function<string(const match_result&)> callback) const;

    NEFORCE_NODISCARD vector<string> split(const string& str, int max_splits = -1) const;

    NEFORCE_NODISCARD int capture_count() const noexcept {
        return capture_count_;
    }

    NEFORCE_NODISCARD const string& pattern() const noexcept {
        return pattern_;
    }
    
    NEFORCE_NODISCARD bool valid() const noexcept {
        return code_ != nullptr;
    }

    NEFORCE_NODISCARD regex_iterator begin(const string& str) const;

    NEFORCE_NODISCARD regex_iterator end(const string& str) const;
};


class NEFORCE_API regex_iterator {
public:
    using iterator_category = bidirectional_iterator_tag;
    using value_type        = match_result;
    using difference_type   = ptrdiff_t;
    using pointer           = const match_result*;
    using reference         = const match_result&;

private:
    const regex* regex_ = nullptr;
    string subject_;

    mutable vector<match_result> cached_matches_;
    mutable bool cache_built_ = false;

    mutable ptrdiff_t current_index_ = -1;

    void build_cache() const;

    void move_next();
    void move_previous();

    void find_from_position(size_t pos) const;
    void find_last_before_position(size_t pos);

public:
    regex_iterator() = default;

    regex_iterator(const regex* re, const string& str, size_t pos = 0);

    static regex_iterator from_index(const regex* re, const string& str, ptrdiff_t index);

    NEFORCE_NODISCARD reference operator *() const noexcept;

    NEFORCE_NODISCARD pointer operator ->() const noexcept {
        return &(operator*());
    }

    regex_iterator& operator ++() {
        move_next();
        return *this;
    }

    regex_iterator operator ++(int) {
        regex_iterator tmp = *this;
        ++(*this);
        return tmp;
    }

    regex_iterator& operator --() {
        if (current_index_ == -1 && cache_built_) {
            if (!cached_matches_.empty()) {
                current_index_ = static_cast<ptrdiff_t>(cached_matches_.size()) - 1;
            }
        } else {
            move_previous();
        }
        return *this;
    }

    regex_iterator operator --(int) {
        regex_iterator tmp = *this;
        --(*this);
        return tmp;
    }

    NEFORCE_NODISCARD bool operator ==(const regex_iterator& other) const noexcept;

    NEFORCE_NODISCARD bool operator !=(const regex_iterator& other) const noexcept {
        return !(*this == other);
    }

    static regex_iterator begin(const regex* re, const string& str) {
        return regex_iterator(re, str, 0);
    }

    static regex_iterator end(const regex* re, const string& str) {
        regex_iterator it;
        it.regex_ = re;
        it.subject_ = str;
        if (re) {
            it.build_cache();
            it.current_index_ = -1;
        }
        return it;
    }
};

class NEFORCE_API regex_token_iterator {
public:
    enum class State {
        BEFORE_FIRST,    // 第一个匹配之前
        BETWEEN_MATCHES, // 匹配之间
        AFTER_LAST,      // 最后一个匹配之后
        END              // 结束
    };

private:
    const regex* regex_ = nullptr;
    string subject_;
    regex_iterator match_iterator_;
    regex_iterator end_iterator_;
    string_view current_;
    int index_ = 0;
    State state_ = State::END;
    size_t last_pos_ = 0;

private:
    void find_next() noexcept;

public:
    regex_token_iterator() = default;
    
    regex_token_iterator(const regex* re, const string& str, int index = 0);
    
    NEFORCE_NODISCARD string operator *() const noexcept {
        return current_;
    }
    
    regex_token_iterator& operator ++() noexcept;
    
    regex_token_iterator operator ++(int) noexcept {
        regex_token_iterator tmp = *this;
        ++(*this);
        return tmp;
    }
    
    NEFORCE_NODISCARD bool operator ==(const regex_token_iterator& other) const noexcept;
    
    NEFORCE_NODISCARD bool operator !=(const regex_token_iterator& other) const noexcept {
        return !(*this == other);
    }
};


inline regex_iterator regex::begin(const string& str) const {
    return regex_iterator::begin(this, str);
}

inline regex_iterator regex::end(const string& str) const {
    return regex_iterator::end(this, str);
}

NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_CORE_STRING_REGEX_HPP__
