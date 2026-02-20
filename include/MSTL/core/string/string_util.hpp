#ifndef MSTL_CORE_STRING_STRING_UTIL_HPP__
#define MSTL_CORE_STRING_STRING_UTIL_HPP__
#include "../container/vector.hpp"
#include "string.hpp"
MSTL_BEGIN_NAMESPACE__

vector<string_view> MSTL_API split(string_view str, string_view delimiters, bool skip_empty = true);
vector<string> MSTL_API split(const string& str, const string& delimiters, bool skip_empty = true);

string MSTL_API join(const vector<string>& vec, const string& delimiter = "");
string MSTL_API join_fast(const vector<string>& vec, const string& delimiter = "");
string MSTL_API join_accumulate(const vector<string> &vec, const string &delimiter = "");

vector<string> MSTL_API unique(const vector<string>& vec);
vector<string> MSTL_API concatenate(const vector<vector<string>>& vectors);

vector<string> MSTL_API cartesian_product(
    const vector<string>& vec1,
    const vector<string>& vec2, const string& connector = "");

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_STRING_STRING_UTIL_HPP__
