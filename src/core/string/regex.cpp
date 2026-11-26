#include <NeForce/core/string/regex.hpp>
#include <NeForce/core/utility/packages.hpp>
#include <NeForce/core/utility/scope.hpp>
#include "regex_builtin.hpp"
NEFORCE_BEGIN_NAMESPACE__

match_result::match_result(const string& subject, const size_t pos, const size_t len, vector<string> groups,
                           vector<pair<size_t, size_t>> group_positions) :
groups_(move(groups)),
group_positions_(move(group_positions)),
position_(pos),
length_(len),
owner_(make_shared<string>(subject)),
subject_(owner_->view()) {}

match_result::match_result(const string_view subject, const size_t pos, const size_t len, vector<string> groups,
                           vector<pair<size_t, size_t>> group_positions) :
groups_(move(groups)),
group_positions_(move(group_positions)),
position_(pos),
length_(len),
subject_(subject) {}

void match_result::attach_owner(const string& subject) {
    owner_ = make_shared<string>(subject);
    subject_ = owner_->view();
}

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

void regex::compile(const string& pattern, const regex_option options) {
    reset();

    int error_code = 0;
    size_t error_offset = 0;
    char error_message[256] = {};

    code_ = regex_compile(pattern.data(), pattern.length(), static_cast<uint32_t>(options), &error_code, &error_offset,
                          error_message, sizeof(error_message));

    if (code_ == nullptr) {
        string message(error_message);
        message += " at offset ";
        message += to_string(error_offset);
        NEFORCE_THROW_EXCEPTION(regex_exception(message.data()));
    }

    capture_count_ = regex_capture_count(static_cast<const regex_program*>(code_));
    regex_analysis_of(static_cast<const regex_program*>(code_), &analysis_.has_nested_quantifier,
                      &analysis_.may_backtrack_exponentially);

    pattern_ = pattern;
    options_ = options;
}

match_result regex::do_match(const void* subject, const size_t length, const size_t start_offset,
                             const uint32_t options, const string& subject_str) const {
    match_result result = do_match_borrowed(subject, length, start_offset, options, subject_str.view());

    if (result.matched()) {
        result.attach_owner(subject_str);
    }

    return result;
}

match_result regex::do_match_owned(const shared_ptr<string>& owner, const size_t length, const size_t start_offset,
                                   const uint32_t options) const {
    match_result result = do_match_borrowed(owner->data(), length, start_offset, options, owner->view());

    if (result.matched()) {
        result.owner_ = owner;
    }

    return result;
}

match_result regex::do_match_borrowed(const void* subject, const size_t length, const size_t start_offset,
                                      const uint32_t options, const string_view subject_view) const {
    constexpr auto unset_offset = numeric_traits<size_t>::max();

    if (code_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(regex_exception("Uninitialized regex object"));
    }

    const auto* const subject_bytes = static_cast<const char*>(subject);

    auto* match_data = regex_match_create(static_cast<const regex_program*>(code_));

    if (match_data == nullptr) {
        NEFORCE_THROW_EXCEPTION(regex_exception("Failed to create match data"));
    }
    const scope_exit match_data_guard([match_data]() noexcept { regex_match_free(match_data); });

    const int rc = regex_exec(static_cast<const regex_program*>(code_), subject_bytes, length, start_offset, options,
                              match_data);

    if (rc < 0) {
        if (rc == match_result_nomatch) {
            return {};
        }

        NEFORCE_THROW_EXCEPTION(regex_exception(rc == match_result_bad_utf
                                                        ? "Invalid UTF-8 sequence in subject"
                                                        : "Regular expression match limit exceeded"));
    }

    const size_t* const ovector = regex_ovector(match_data);

    vector<string> groups;
    vector<pair<size_t, size_t>> group_positions;
    groups.reserve(rc);
    group_positions.reserve(rc);

    for (ptrdiff_t i = 0; i < rc; ++i) {
        if (ovector[i * 2] != unset_offset) {
            const size_t start = ovector[i * 2];
            const size_t end = ovector[i * 2 + 1];
            groups.emplace_back(subject_bytes + start, end - start);
            group_positions.emplace_back(start, end - start);
        } else {
            groups.emplace_back();
            group_positions.emplace_back(string::npos, 0);
        }
    }

    const match_result res{subject_view, ovector[0], ovector[1] - ovector[0], move(groups), move(group_positions)};
    return move(res);
}

int regex::group_index(const string_view name) const noexcept {
    return regex_group_index(static_cast<const regex_program*>(code_), name.data(), name.size());
}

regex::regex(const string& pattern, const regex_option options) { compile(pattern, options); }

regex::regex(regex&& other) noexcept :
code_(other.code_),
pattern_(move(other.pattern_)),
options_(other.options_),
capture_count_(other.capture_count_) {
    other.code_ = nullptr;
    other.options_ = regex_option::none;
    other.capture_count_ = 0;
}

regex& regex::operator=(regex&& other) noexcept {
    if (addressof(other) == this) {
        return *this;
    }

    reset();

    code_ = other.code_;
    pattern_ = move(other.pattern_);
    options_ = other.options_;
    capture_count_ = other.capture_count_;

    other.code_ = nullptr;
    other.options_ = regex_option::none;
    other.capture_count_ = 0;

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
            reset();
        }
    }
    return *this;
}

regex::~regex() { reset(); }

match_result regex::do_match(const string& str) const {
    return do_match(str.data(), str.length(), 0, match_anchored | match_end_anchored, str);
}

bool regex::match(const string& str) const {
    return do_match(str.data(), str.length(), 0, match_anchored | match_end_anchored, str).matched();
}

match_result regex::search(const string& str, const size_t pos) const {
    return do_match(str.data(), str.length(), pos, 0, str);
}

vector<match_result> regex::find_all(const string& str) const {
    vector<match_result> results;
    const auto owner = make_shared<string>(str);
    const size_t length = owner->size();
    size_t start_offset = 0;

    while (start_offset <= length) {
        auto result = do_match_owned(owner, length, start_offset, 0);

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
    const auto* const subject = str.data();
    const size_t length = str.length();
    size_t last_pos = 0;
    size_t start_offset = 0;

    while (start_offset <= length) {
        auto match = do_match_borrowed(subject, length, start_offset, 0, str.view());

        if (!match.matched()) {
            break;
        }

        result.append(str, last_pos, match.position() - last_pos);
        result.append(match.format(fmt));

        last_pos = match.position() + match.length();
        start_offset = last_pos + (match.length() == 0 ? 1 : 0);
    }

    result.append(str, last_pos);
    return result;
}

string regex::replace_all_callback(const string& str, function<string(const match_result&)> callback) const {
    string result;
    const auto* const subject = str.data();
    const size_t length = str.length();
    size_t last_pos = 0;
    size_t start_offset = 0;

    while (start_offset <= length) {
        auto match = do_match_borrowed(subject, length, start_offset, 0, str.view());

        if (!match.matched()) {
            break;
        }

        result.append(str, last_pos, match.position() - last_pos);
        result.append(callback(match));

        last_pos = match.position() + match.length();
        start_offset = last_pos + (match.length() == 0 ? 1 : 0);
    }

    result.append(str, last_pos);
    return result;
}

vector<string> regex::split(const string& str, const int max_splits) const {
    vector<string> parts;
    const auto* const subject = str.data();
    const size_t length = str.length();
    size_t last_pos = 0;
    size_t start_offset = 0;
    int splits = 0;

    while (start_offset <= length) {
        if (max_splits >= 0 && splits >= max_splits) {
            break;
        }

        auto match = do_match_borrowed(subject, length, start_offset, 0, str.view());

        if (!match.matched()) {
            break;
        }

        parts.push_back(str.substr(last_pos, match.position() - last_pos));

        if (match.size() > 1) {
            for (size_t index = 1; index < match.size(); ++index) {
                parts.emplace_back(match[index].data(), match[index].size());
            }
        }

        last_pos = match.position() + match.length();
        start_offset = last_pos + (match.length() == 0 ? 1 : 0);
        ++splits;
    }

    parts.push_back(str.tail(last_pos));
    return parts;
}

void regex::reset() noexcept {
    if (code_ != nullptr) {
        regex_free(static_cast<regex_program*>(code_));
    }
    code_ = nullptr;
    pattern_.clear();
    options_ = regex_option::none;
    capture_count_ = 0;
}

void regex_iterator::find_next() {
    if (regex_ == nullptr || !regex_->valid() || subject_ == nullptr || next_pos_ > subject_->size()) {
        current_ = match_result{};
        done_ = true;
        return;
    }

    auto result = regex_->do_match_owned(subject_, subject_->size(), next_pos_, 0);
    if (result.matched()) {
        next_pos_ = result.position() + max(result.length(), static_cast<size_t>(1));
        if (next_pos_ > subject_->size()) {
            next_pos_ = subject_->size() + 1;
        }
        current_ = move(result);
    } else {
        current_ = match_result{};
        done_ = true;
    }
}

regex_iterator::regex_iterator(const regex* re, string str, const size_t pos) :
regex_(re),
subject_(make_shared<string>(move(str))),
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
    return regex_ == other.regex_ && subject_.get() == other.subject_.get() && next_pos_ == other.next_pos_;
}

void regex_token_iterator::find_next() {
    if (state_ == state::end) {
        current_ = "";
        return;
    }
    if (state_ == state::after_last) {
        current_ = "";
        state_ = state::end;
        return;
    }

    if (index_ < 0) {
        switch (state_) {
            case state::before_first: {
                if (match_iterator_ != end_iterator_) {
                    constexpr size_t start = 0;
                    const size_t end = match_iterator_->position();
                    current_ = (start < end) ? subject_.view(start, end - start) : "";
                    last_pos_ = match_iterator_->position() + match_iterator_->length();
                    ++match_iterator_;
                    state_ = state::between_matches;
                } else {
                    current_ = subject_.view();
                    state_ = state::after_last;
                }
                break;
            }
            case state::between_matches: {
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
                    state_ = state::after_last;
                }
                break;
            }
            default: {
                state_ = state::end;
                current_ = "";
                break;
            }
        }
        return;
    }

    switch (state_) {
        case state::before_first: {
            if (match_iterator_ != end_iterator_) {
                if (static_cast<size_t>(index_) < match_iterator_->size()) {
                    current_ = (*match_iterator_)[index_];
                } else {
                    current_ = "";
                }
                ++match_iterator_;
                state_ = state::between_matches;
            } else {
                state_ = state::end;
                current_ = "";
            }
            break;
        }
        case state::between_matches: {
            if (match_iterator_ != end_iterator_) {
                if (static_cast<size_t>(index_) < match_iterator_->size()) {
                    current_ = (*match_iterator_)[index_];
                } else {
                    current_ = "";
                }
                ++match_iterator_;
            } else {
                state_ = state::end;
            }
            break;
        }
        default: {
            state_ = state::end;
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
            state_ = state::before_first;
            find_next();
        } else {
            if (match_iterator_ == end_iterator_) {
                current_ = subject_.view();
                state_ = state::after_last;
            } else {
                state_ = state::before_first;
                find_next();
            }
        }
    } else {
        state_ = state::end;
        current_ = "";
    }
}

regex_token_iterator& regex_token_iterator::operator++() {
    if (state_ != state::end) {
        find_next();
    }
    return *this;
}

bool regex_token_iterator::operator==(const regex_token_iterator& other) const noexcept {
    if (state_ == state::end && other.state_ == state::end) {
        return true;
    }
    if (state_ != other.state_) {
        return false;
    }

    return regex_ == other.regex_ && subject_ == other.subject_ && match_iterator_ == other.match_iterator_ &&
           index_ == other.index_ && last_pos_ == other.last_pos_;
}

NEFORCE_END_NAMESPACE__
