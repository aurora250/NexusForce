#include <MSTL/core/string/string_util.hpp>
#include <MSTL/core/algorithm/numeric.hpp>
#include <MSTL/core/container/unordered_set.hpp>
MSTL_BEGIN_NAMESPACE__

vector<string_view> split(const string_view str,
    const string_view delimiters, const bool skipEmpty) {
    vector<string_view> tokens;
    size_t start = 0;
    size_t end = str.find_first_of(delimiters);

    while (end != string_view::npos) {
        string_view token = str.substr(start, end - start);
        if (!skipEmpty || !token.empty()) {
            tokens.push_back(token);
        }
        start = end + 1;
        end = str.find_first_of(delimiters, start);
    }

    const string_view lastToken = str.substr(start);
    if (!skipEmpty || !lastToken.empty()) {
        tokens.push_back(lastToken);
    }

    return tokens;
}

vector<string> split(const string& str,
    const string& delimiters, const bool skipEmpty) {
    vector<string> tokens;
    size_t start = 0;
    size_t end = str.find_first_of(delimiters);

    while (end != string::npos) {
        string token = str.substr(start, end - start);
        if (!skipEmpty || !token.empty()) {
            tokens.push_back(token);
        }
        start = end + 1;
        end = str.find_first_of(delimiters, start);
    }

    const string lastToken = str.substr(start);
    if (!skipEmpty || !lastToken.empty()) {
        tokens.push_back(lastToken);
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

string join_accumulate(const vector<string> &vec, const string &delimiter) {
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

vector<string> concatenate(const vector<vector<string>>& vectors) {
    vector<string> result;

    size_t total_size = 0;
    for (const auto& vec : vectors) {
        total_size += vec.size();
    }
    result.reserve(total_size);

    for (const auto& vec : vectors) {
        result.insert(result.end(), vec.begin(), vec.end());
    }

    return result;
}

vector<string> cartesian_product(const vector<string>& vec1,
    const vector<string>& vec2, const string& connector) {
    vector<string> result;
    result.reserve(vec1.size() * vec2.size());

    for (const auto& s1 : vec1) {
        for (const auto& s2 : vec2) {
            result.push_back(s1 + connector + s2);
        }
    }

    return result;
}

MSTL_END_NAMESPACE__
