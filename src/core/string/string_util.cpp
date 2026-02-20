#include <MSTL/core/string/string_util.hpp>
#include <MSTL/core/algorithm/numeric.hpp>
#include <MSTL/core/container/unordered_set.hpp>
MSTL_BEGIN_NAMESPACE__

vector<string_view> split(const string_view str, const string_view delimiters, const bool skip_empty) {
    vector<string_view> tokens;
    size_t start = 0;
    size_t end = str.find_first_of(delimiters);

    while (end != string_view::npos) {
        string_view token = str.substr(start, end - start);
        if (!skip_empty || !token.empty()) {
            tokens.push_back(token);
        }
        start = end + 1;
        end = str.find_first_of(delimiters, start);
    }

    const string_view last_token = str.substr(start);
    if (!skip_empty || !last_token.empty()) {
        tokens.push_back(last_token);
    }

    return tokens;
}

vector<string> split(const string& str, const string& delimiters, const bool skip_empty) {
    vector<string> tokens;
    size_t start = 0;
    size_t end = str.find_first_of(delimiters);

    while (end != string::npos) {
        string token = str.substr(start, end - start);
        if (!skip_empty || !token.empty()) {
            tokens.push_back(token);
        }
        start = end + 1;
        end = str.find_first_of(delimiters, start);
    }

    const string last_token = str.substr(start);
    if (!skip_empty || !last_token.empty()) {
        tokens.push_back(last_token);
    }

    return tokens;
}

string join(const vector<string>& vec, const string& delimiter) {
    string result;
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i != 0) result += delimiter;
        result += vec[i];
    }
    return result;
}

string join_fast(const vector<string>& vec, const string& delimiter) {
    if (vec.empty()) return "";

    size_t total_length = 0;
    for (const auto& s : vec) {
        total_length += s.length();
    }
    total_length += delimiter.length() * (vec.size() - 1);

    string result;
    result.reserve(total_length);

    for (size_t i = 0; i < vec.size(); ++i) {
        if (i != 0) result.append(delimiter);
        result.append(vec[i]);
    }

    return result;
}

string join_accumulate(const vector<string>& vec, const string& delimiter) {
    if (vec.empty()) return "";
    return _MSTL accumulate(next(vec.begin()), vec.end(), vec[0],
         [&delimiter](const string& a, const string& b) {
             return a + delimiter + b;
         });
}


vector<string> unique(const vector<string>& vec) {
    vector<string> result;
    unordered_set<string> seen;
    for (const auto& s : vec) {
        if (seen.insert(s).second) {
            result.push_back(s);
        }
    }
    return result;
}

MSTL_END_NAMESPACE__
