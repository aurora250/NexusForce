#ifndef MSTL_CORE_STRING_STRING_UTIL_HPP__
#define MSTL_CORE_STRING_STRING_UTIL_HPP__
#include "../container/vector.hpp"
#include "../iterator/insert_iterator.hpp"
#include "string.hpp"
MSTL_BEGIN_NAMESPACE__

vector<string_view> MSTL_API split(string_view str, string_view delimiters, bool skip_empty = true);
vector<string> MSTL_API split(const string& str, const string& delimiters, bool skip_empty = true);

string MSTL_API join(const vector<string>& vec, const string& delimiter = "");
string MSTL_API join_fast(const vector<string>& vec, const string& delimiter = "");
string MSTL_API join_accumulate(const vector<string> &vec, const string &delimiter = "");

template <typename Pred>
vector<string> filter_if(const vector<string>& vec, Pred pred) {
    vector<string> result;
    _MSTL copy_if(vec.begin(), vec.end(), make_back_inserter(result), pred);
    return result;
}

MSTL_ALWAYS_INLINE_INLINE vector<string> filter_empty(const vector<string>& vec) {
    return filter_if(vec, [](const string& s) { return !s.empty(); });
}

MSTL_ALWAYS_INLINE_INLINE vector<string> filter(const vector<string>& vec, const string& value_to_remove) {
    return filter_if(vec, [&value_to_remove](const string& s) {
            return s != value_to_remove;
        });
}

vector<string> MSTL_API unique(const vector<string>& vec);
vector<string> MSTL_API concatenate(const vector<vector<string>>& vectors);

vector<string> MSTL_API cartesian_product(const vector<string>& vec1,
    const vector<string>& vec2, const string& connector = "");

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_STRING_STRING_UTIL_HPP__
