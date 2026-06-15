#include <NeForce/core/string/regex.hpp>
#include <NeForce/core/utility/packages.hpp>
NEFORCE_BEGIN_NAMESPACE__

match_result::match_result(string subject, const size_t pos, const size_t len, const vector<string>& groups,
                           vector<pair<size_t, size_t>> group_positions) :
groups_(groups),
group_positions_(move(group_positions)),
position_(pos),
length_(len),
subject_(move(subject)) {}

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
    int error_code = 0;
    ::PCRE2_SIZE error_offset = 0;

    code_.reset(::pcre2_compile(reinterpret_cast<::PCRE2_SPTR>(pattern.data()), pattern.length(), options, &error_code,
                                &error_offset, nullptr));

    if (!code_) {
        char error_message[256];
        ::pcre2_get_error_message(error_code, reinterpret_cast<::PCRE2_UCHAR*>(error_message), sizeof(error_message));
        NEFORCE_THROW_EXCEPTION(regex_exception(error_message));
    }

    ::pcre2_pattern_info(code_.get(), PCRE2_INFO_CAPTURECOUNT, &capture_count_);

    pattern_ = pattern;
    options_ = options;
}

match_result regex::do_match(const ::PCRE2_SPTR subject, const size_t length, const size_t start_offset,
                             const uint32_t options, const string& subject_str) const {
    if (!code_) {
        NEFORCE_THROW_EXCEPTION(regex_exception("Uninitialized regex object"));
    }

    const unique_ptr<::pcre2_match_data, pcre2_match_data_deleter> match_data(
            ::pcre2_match_data_create_from_pattern(code_.get(), nullptr));

    if (!match_data) {
        NEFORCE_THROW_EXCEPTION(regex_exception("Failed to create match data"));
    }

    const int rc = ::pcre2_match(code_.get(), subject, length, start_offset, options, match_data.get(), nullptr);

    if (rc < 0) {
        if (rc == PCRE2_ERROR_NOMATCH) {
            return {};
        }
        char error_message[256];
        ::pcre2_get_error_message(rc, reinterpret_cast<::PCRE2_UCHAR*>(error_message), sizeof(error_message));
        NEFORCE_THROW_EXCEPTION(regex_exception(error_message));
    }

    const ::PCRE2_SIZE* ovector = ::pcre2_get_ovector_pointer(match_data.get());

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

    return {subject_str, ovector[0], ovector[1] - ovector[0], groups, group_positions};
}

regex::regex(const string& pattern, const uint32_t options) { compile(pattern, options); }

regex::regex(regex&& other) noexcept :
code_(move(other.code_)),
pattern_(move(other.pattern_)),
options_(other.options_),
capture_count_(other.capture_count_) {}

regex& regex::operator=(regex&& other) noexcept {
    if (addressof(other) == this) {
        return *this;
    }

    code_ = move(other.code_);
    pattern_ = move(other.pattern_);
    options_ = other.options_;
    capture_count_ = other.capture_count_;

    return *this;
}

regex::regex(const regex& other) {
    if (!other.pattern_.empty()) {
        compile(other.pattern_, other.options_);
    }
}

regex& regex::operator=(const regex& other) {
    if (this != &other) {
        if (!other.pattern_.empty()) {
            compile(other.pattern_, other.options_);
        } else {
            code_.reset();
            pattern_.clear();
            options_ = 0;
            capture_count_ = 0;
        }
    }
    return *this;
}

match_result regex::do_match(const string& str) const {
    return do_match(reinterpret_cast<::PCRE2_SPTR>(str.data()), str.length(), 0, PCRE2_ANCHORED | PCRE2_ENDANCHORED,
                    str);
}

bool regex::match(const string& str) const {
    return do_match(reinterpret_cast<::PCRE2_SPTR>(str.data()), str.length(), 0, PCRE2_ANCHORED | PCRE2_ENDANCHORED,
                    str)
            .matched();
}

match_result regex::search(const string& str, const size_t pos) const {
    return do_match(reinterpret_cast<::PCRE2_SPTR>(str.data()), str.length(), pos, 0, str);
}

vector<match_result> regex::find_all(const string& str) const {
    vector<match_result> results;
    const auto* subject = reinterpret_cast<::PCRE2_SPTR>(str.data());
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

        if (match.size() > 1) {
            for (size_t i = 1; i < match.size(); ++i) {
                parts.emplace_back(match[i].data(), match[i].size());
            }
        }

        last_pos = match.position() + match.length();
        splits++;
    }

    parts.push_back(str.tail(last_pos));
    return parts;
}

void regex_iterator::find_next() {
    if (regex_ == nullptr || !regex_->valid() || next_pos_ > subject_.length()) {
        current_ = match_result{};
        done_ = true;
        return;
    }

    auto result = regex_->search(subject_, next_pos_);
    if (result.matched()) {
        next_pos_ = result.position() + _NEFORCE max(result.length(), size_t(1));
        if (next_pos_ > subject_.length()) {
            next_pos_ = subject_.length() + 1;
        }
        current_ = _NEFORCE move(result);
    } else {
        current_ = match_result{};
        done_ = true;
    }
}

regex_iterator::regex_iterator(const regex* re, string str, const size_t pos) :
regex_(re),
subject_(move(str)),
next_pos_(pos),
done_(false) {
    find_next();
}

regex_iterator& regex_iterator::operator++() {
    if (!done_) {
        find_next();
    }
    return *this;
}

bool regex_iterator::operator==(const regex_iterator& other) const noexcept {
    if (done_ && other.done_) {
        return true;
    }
    if (done_ || other.done_) {
        return false;
    }
    return regex_ == other.regex_ && subject_ == other.subject_ && next_pos_ == other.next_pos_;
}

void regex_token_iterator::find_next() {
    if (state_ == state::END) {
        current_ = "";
        return;
    }
    if (state_ == state::AFTER_LAST) {
        current_ = "";
        state_ = state::END;
        return;
    }

    if (index_ < 0) {
        switch (state_) {
            case state::BEFORE_FIRST: {
                if (match_iterator_ != end_iterator_) {
                    constexpr size_t start = 0;
                    const size_t end = match_iterator_->position();
                    current_ = (start < end) ? subject_.view(start, end - start) : "";
                    last_pos_ = match_iterator_->position() + match_iterator_->length();
                    ++match_iterator_;
                    state_ = state::BETWEEN_MATCHES;
                } else {
                    current_ = subject_.view();
                    state_ = state::AFTER_LAST;
                }
                break;
            }
            case state::BETWEEN_MATCHES: {
                if (match_iterator_ != end_iterator_) {
                    const size_t start = last_pos_;
                    const size_t end = match_iterator_->position();
                    current_ = (start < end) ? subject_.view(start, end - start) : "";
                    last_pos_ = match_iterator_->position() + match_iterator_->length();
                    ++match_iterator_;
                } else {
                    if (last_pos_ < subject_.length()) {
                        current_ = subject_.view(last_pos_);
                    } else {
                        current_ = "";
                    }
                    state_ = state::AFTER_LAST;
                }
                break;
            }
            default: {
                state_ = state::END;
                current_ = "";
                break;
            }
        }
        return;
    }

    switch (state_) {
        case state::BEFORE_FIRST: {
            if (match_iterator_ != end_iterator_) {
                if (static_cast<size_t>(index_) < match_iterator_->size()) {
                    current_ = (*match_iterator_)[index_];
                } else {
                    current_ = "";
                }
                ++match_iterator_;
                state_ = state::BETWEEN_MATCHES;
            } else {
                state_ = state::END;
                current_ = "";
            }
            break;
        }
        case state::BETWEEN_MATCHES: {
            if (match_iterator_ != end_iterator_) {
                if (static_cast<size_t>(index_) < match_iterator_->size()) {
                    current_ = (*match_iterator_)[index_];
                } else {
                    current_ = "";
                }
                ++match_iterator_;
            } else {
                state_ = state::END;
            }
            break;
        }
        default: {
            state_ = state::END;
            current_ = "";
            break;
        }
    }
}

regex_token_iterator::regex_token_iterator(const regex* re, string str, const int index) :
regex_(re),
subject_(move(str)),
index_(index) {
    if (regex_ != nullptr && !subject_.empty()) {
        match_iterator_ = regex_->begin(subject_);
        end_iterator_ = regex_->end(subject_);

        if (index_ >= 0) {
            state_ = state::BEFORE_FIRST;
            find_next();
        } else {
            if (match_iterator_ == end_iterator_) {
                current_ = subject_.view();
                state_ = state::AFTER_LAST;
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

regex_token_iterator& regex_token_iterator::operator++() {
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
