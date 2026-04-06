#include <NeForce/core/string/regex.hpp>
#include <NeForce/core/utility/packages.hpp>
NEFORCE_BEGIN_NAMESPACE__

match_result::match_result(const string& subject, const size_t pos, const size_t len, const vector<string>& groups,
                           const vector<pair<size_t, size_t>>& group_positions) :
groups_(groups),
group_positions_(group_positions),
position_(pos),
length_(len),
subject_(subject) {}

string match_result::format(const string_view fmt) const {
    if (!matched()) {
        return "";
    }

    string result;
    size_t i = 0;
    while (i < fmt.length()) {
        if (fmt[i] == '$' && i + 1 < fmt.length()) {
            const char next = fmt[i + 1];
            if (next == '$') {
                result += '$';
                i += 2;
            } else if (next == '&') {
                result += groups_[0];
                i += 2;
            } else if (next == '`') {
                result += prefix();
                i += 2;
            } else if (next == '\'') {
                result += suffix();
                i += 2;
            } else if (next >= '0' && next <= '9') {
                const int group = next - '0';
                if (group < static_cast<int>(groups_.size())) {
                    result += groups_[group];
                }
                i += 2;
            } else if (next == '{') {
                size_t j = i + 2;
                while (j < fmt.length() && fmt[j] != '}') {
                    j++;
                }
                if (j < fmt.length()) {
                    const string_view group_ref = fmt.view(i + 2, j - (i + 2));
                    try {
                        const int group = integer32::parse(group_ref).value();
                        if (group >= 0 && group < static_cast<int>(groups_.size())) {
                            result += groups_[group];
                        }
                        // NOLINTNEXTLINE(bugprone-empty-catch)
                    } catch (...) {
                        // ignore
                    }
                    i = j + 1;
                } else {
                    result += '$';
                    i++;
                }
            } else {
                result += '$';
                i++;
            }
        } else {
            result += fmt[i];
            i++;
        }
    }
    return result;
}

void regex::compile(const string& pattern, const uint32_t options) {
    int errorcode;
    PCRE2_SIZE erroroffset;

    code_.reset(pcre2_compile(reinterpret_cast<PCRE2_SPTR>(pattern.data()), pattern.length(), options, &errorcode,
                              &erroroffset, nullptr));

    if (!code_) {
        char error_message[256];
        pcre2_get_error_message(errorcode, reinterpret_cast<PCRE2_UCHAR*>(error_message), sizeof(error_message));
        NEFORCE_THROW_EXCEPTION(regex_exception(error_message));
    }

    pcre2_pattern_info(code_.get(), PCRE2_INFO_CAPTURECOUNT, &capture_count_);

    pattern_ = pattern;
    options_ = options;
}

void regex::compile(string&& pattern, const uint32_t options) {
    int errorcode;
    PCRE2_SIZE erroroffset;

    code_.reset(pcre2_compile(reinterpret_cast<PCRE2_SPTR>(pattern.data()), pattern.length(), options, &errorcode,
                              &erroroffset, nullptr));

    if (!code_) {
        char error_message[256];
        pcre2_get_error_message(errorcode, reinterpret_cast<PCRE2_UCHAR*>(error_message), sizeof(error_message));
        NEFORCE_THROW_EXCEPTION(regex_exception(error_message));
    }

    pcre2_pattern_info(code_.get(), PCRE2_INFO_CAPTURECOUNT, &capture_count_);

    pattern_ = move(pattern);
    options_ = options;
}

match_result regex::do_match(const PCRE2_SPTR subject, const size_t length, const size_t start_offset,
                             const uint32_t options, const string& subject_str) const {
    if (!code_) {
        NEFORCE_THROW_EXCEPTION(regex_exception("Uninitialized regex object"));
    }

    const unique_ptr<pcre2_match_data, pcre2_match_data_deleter> match_data(
            pcre2_match_data_create_from_pattern(code_.get(), nullptr));

    if (!match_data) {
        NEFORCE_THROW_EXCEPTION(regex_exception("Failed to create match data"));
    }

    const int rc = pcre2_match(code_.get(), subject, length, start_offset, options, match_data.get(), nullptr);

    if (rc < 0) {
        if (rc == PCRE2_ERROR_NOMATCH) {
            return match_result();
        }
        char error_message[256];
        pcre2_get_error_message(rc, reinterpret_cast<PCRE2_UCHAR*>(error_message), sizeof(error_message));
        NEFORCE_THROW_EXCEPTION(regex_exception(error_message));
    }

    const PCRE2_SIZE* ovector = pcre2_get_ovector_pointer(match_data.get());

    vector<string> groups;
    vector<pair<size_t, size_t>> group_positions;
    groups.reserve(rc);
    group_positions.reserve(rc);

    for (ptrdiff_t i = 0; i < rc; ++i) {
        if (ovector[i * 2] != PCRE2_UNSET) {
            const size_t start = ovector[i * 2];
            const size_t end = ovector[i * 2 + 1];
            groups.emplace_back(reinterpret_cast<const char*>(subject + start), end - start);
            group_positions.emplace_back(start, end - start);
        } else {
            groups.emplace_back();
            group_positions.emplace_back(string::npos, 0);
        }
    }

    return match_result(subject_str, ovector[0], ovector[1] - ovector[0], groups, group_positions);
}

regex::regex(const string& pattern, const uint32_t options) { compile(pattern, options); }

regex::regex(string&& pattern, const uint32_t options) { compile(move(pattern), options); }

regex::regex(regex&& other) noexcept :
code_(move(other.code_)),
pattern_(move(other.pattern_)),
options_(other.options_),
capture_count_(other.capture_count_) {}

regex& regex::operator=(regex&& other) noexcept {
    if (this != &other) {
        code_ = move(other.code_);
        pattern_ = move(other.pattern_);
        options_ = other.options_;
        capture_count_ = other.capture_count_;
    }
    return *this;
}

match_result regex::do_match(const string& str) const {
    return do_match(reinterpret_cast<PCRE2_SPTR>(str.data()), str.length(), 0, PCRE2_ANCHORED | PCRE2_ENDANCHORED, str);
}

bool regex::match(const string& str) const {
    return do_match(reinterpret_cast<PCRE2_SPTR>(str.data()), str.length(), 0, PCRE2_ANCHORED | PCRE2_ENDANCHORED, str)
            .matched();
}

match_result regex::search(const string& str, const size_t pos) const {
    return do_match(reinterpret_cast<PCRE2_SPTR>(str.data()), str.length(), pos, 0, str);
}

vector<match_result> regex::find_all(const string& str) const {
    vector<match_result> results;
    const auto subject = reinterpret_cast<PCRE2_SPTR>(str.data());
    const size_t length = str.length();
    size_t start_offset = 0;

    while (start_offset <= length) {
        auto result = do_match(subject, length, start_offset, 0, str);

        if (!result.matched()) {
            break;
        }

        results.push_back(result);

        if (result.length() == 0) {
            start_offset++;
        } else {
            start_offset = result.position() + result.length();
        }

        if (start_offset > length) {
            break;
        }
    }

    return results;
}

string regex::replace_first(const string& str, const string_view fmt) const {
    const auto result = search(str);
    if (!result.matched()) {
        return str;
    }

    string output;
    output.reserve(str.length() + fmt.length());

    output.append(str, 0, result.position());
    output.append(result.format(fmt));
    output.append(str, result.position() + result.length());

    return output;
}

string regex::replace_all(const string& str, const string_view fmt) const {
    string result;
    size_t last_pos = 0;
    auto matches = find_all(str);

    for (const auto& match: matches) {
        result.append(str, last_pos, match.position() - last_pos);
        result.append(match.format(fmt));
        last_pos = match.position() + match.length();
    }

    result.append(str, last_pos);
    return result;
}

string regex::replace_all_callback(const string& str, function<string(const match_result&)> callback) const {
    string result;
    size_t last_pos = 0;
    auto matches = find_all(str);

    for (const auto& match: matches) {
        result.append(str, last_pos, match.position() - last_pos);
        result.append(callback(match));
        last_pos = match.position() + match.length();
    }

    result.append(str, last_pos);
    return result;
}

vector<string> regex::split(const string& str, const int max_splits) const {
    vector<string> parts;
    size_t last_pos = 0;
    int splits = 0;
    auto matches = find_all(str);

    for (const auto& match: matches) {
        if (max_splits >= 0 && splits >= max_splits) {
            break;
        }

        parts.push_back(str.substr(last_pos, match.position() - last_pos));
        last_pos = match.position() + match.length();
        splits++;
    }

    parts.push_back(str.substr(last_pos));
    return parts;
}

void regex_iterator::build_cache() const {
    if (!cache_built_ && regex_) {
        cached_matches_ = regex_->find_all(subject_);
        cache_built_ = true;

        if (!cached_matches_.empty()) {
            current_index_ = 0;
        }
    }
}

void regex_iterator::move_next() {
    build_cache();
    if (current_index_ >= 0 && current_index_ + 1 < static_cast<ptrdiff_t>(cached_matches_.size())) {
        ++current_index_;
    } else {
        current_index_ = -1;
    }
}

void regex_iterator::move_previous() {
    build_cache();
    if (current_index_ > 0) {
        --current_index_;
    } else {
        current_index_ = -1;
    }
}

void regex_iterator::find_from_position(size_t pos) const {
    build_cache();

    auto it = find_if(cached_matches_.begin(), cached_matches_.end(),
                      [pos](const match_result& m) { return m.position() >= pos; });

    if (it != cached_matches_.end()) {
        current_index_ = distance(cached_matches_.begin(), it);
    } else {
        current_index_ = -1;
    }
}

void regex_iterator::find_last_before_position(const size_t pos) {
    build_cache();

    if (cached_matches_.empty()) {
        current_index_ = -1;
        return;
    }

    for (ptrdiff_t i = static_cast<ptrdiff_t>(cached_matches_.size()) - 1; i >= 0; --i) {
        if (cached_matches_[i].position() < pos) {
            current_index_ = i;
            return;
        }
    }

    current_index_ = -1;
}

regex_iterator::regex_iterator(const regex* re, const string& str, const size_t pos) :
regex_(re),
subject_(str) {
    if (regex_) {
        if (pos == 0) {
            build_cache();
        } else {
            find_from_position(pos);
        }
    }
}

regex_iterator regex_iterator::from_index(const regex* re, const string& str, ptrdiff_t index) {
    regex_iterator it;
    it.regex_ = re;
    it.subject_ = str;
    if (re) {
        it.build_cache();
        if (index >= 0 && index < static_cast<ptrdiff_t>(it.cached_matches_.size())) {
            it.current_index_ = index;
        } else if (index == static_cast<ptrdiff_t>(it.cached_matches_.size())) {
            it.current_index_ = -1;
        }
    }
    return it;
}

regex_iterator::reference regex_iterator::operator*() const {
    thread_local const match_result empty_result{};
    build_cache();
    if (current_index_ >= 0 && current_index_ < static_cast<ptrdiff_t>(cached_matches_.size())) {
        return cached_matches_[current_index_];
    }
    return empty_result;
}

bool regex_iterator::operator==(const regex_iterator& other) const noexcept {
    if (current_index_ == -1 && other.current_index_ == -1) {
        return true;
    }

    if (!cache_built_ || !other.cache_built_) {
        return regex_ == other.regex_ && subject_ == other.subject_ && current_index_ == other.current_index_;
    }

    return regex_ == other.regex_ && subject_ == other.subject_ && current_index_ == other.current_index_;
}

void regex_token_iterator::find_next() noexcept {
    if (state_ == state::END) {
        current_ = "";
        return;
    }

    if (index_ >= 0) {
        while (match_iterator_ != end_iterator_) {
            if (index_ < static_cast<int>(match_iterator_->size())) {
                current_ = (*match_iterator_)[index_];
                ++match_iterator_;
                return;
            }
            ++match_iterator_;
        }
        state_ = state::END;
        current_ = "";
    } else {
        switch (state_) {
            case state::BEFORE_FIRST: {
                if (match_iterator_ != end_iterator_) {
                    size_t start = 0;
                    size_t end = match_iterator_->position();
                    if (start < end) {
                        current_ = subject_.view(start, end - start);
                        last_pos_ = end;
                        state_ = state::BETWEEN_MATCHES;
                    } else {
                        last_pos_ = match_iterator_->position() + match_iterator_->length();
                        ++match_iterator_;
                        find_next();
                    }
                } else {
                    current_ = subject_.view();
                    state_ = state::END;
                }
                break;
            }
            case state::BETWEEN_MATCHES: {
                if (match_iterator_ != end_iterator_) {
                    size_t start = last_pos_;
                    size_t end = match_iterator_->position();
                    if (start < end) {
                        current_ = subject_.view(start, end - start);
                        last_pos_ = end;
                    } else {
                        current_ = "";
                        last_pos_ = match_iterator_->position() + match_iterator_->length();
                        ++match_iterator_;
                    }

                    if (current_.empty()) {
                        find_next();
                    }
                } else {
                    state_ = state::AFTER_LAST;
                    find_next();
                }
                break;
            }
            case state::AFTER_LAST: {
                if (last_pos_ < subject_.length()) {
                    current_ = subject_.view(last_pos_);
                } else {
                    current_ = "";
                }
                state_ = state::END;
                break;
            }
            default: {
                state_ = state::END;
                current_ = "";
                break;
            }
        }
    }
}

regex_token_iterator::regex_token_iterator(const regex* re, const string& str, const int index) :
regex_(re),
subject_(str),
index_(index) {
    if (regex_ && !subject_.empty()) {
        match_iterator_ = regex_->begin(subject_);
        end_iterator_ = regex_->end(subject_);

        if (index_ >= 0) {
            state_ = state::BEFORE_FIRST;
            find_next();
        } else {
            if (match_iterator_ == end_iterator_) {
                current_ = subject_.view();
                state_ = state::END;
            } else {
                state_ = state::BEFORE_FIRST;
                find_next();
            }
        }
    } else {
        state_ = state::END;
        current_ = "";
    }
}

regex_token_iterator& regex_token_iterator::operator++() noexcept {
    if (state_ != state::END) {
        find_next();
    }
    return *this;
}

bool regex_token_iterator::operator==(const regex_token_iterator& other) const noexcept {
    if (state_ == state::END && other.state_ == state::END) {
        return true;
    }
    if (state_ != other.state_) {
        return false;
    }

    return regex_ == other.regex_ && subject_ == other.subject_ && match_iterator_ == other.match_iterator_ &&
           index_ == other.index_ && last_pos_ == other.last_pos_;
}

NEFORCE_END_NAMESPACE__
