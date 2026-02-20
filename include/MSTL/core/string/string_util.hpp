#ifndef MSTL_CORE_STRING_STRING_UTIL_HPP__
#define MSTL_CORE_STRING_STRING_UTIL_HPP__

/**
 * @file string_util.hpp
 * @brief 字符串工具函数
 *
 * 此文件提供了字符串处理的常用工具函数，
 * 支持字符串视图和普通字符串的不同处理方式。
 */

#include "MSTL/core/container/vector.hpp"
#include "MSTL/core/string/string.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup String 字符串
 * @brief 动态字符序列容器
 * @{
 */

/**
 * @brief 分割字符串视图
 * @param str 要分割的字符串视图
 * @param delimiters 分隔符集合
 * @param skip_empty 是否跳过空字符串
 * @return 分割后的字符串视图向量
 *
 * 使用指定的分隔符集合分割字符串，返回子串的视图向量。
 * 分隔符可以是多个字符，每个字符都作为独立的分隔符。
 */
vector<string_view> MSTL_API split(string_view str, string_view delimiters, bool skip_empty = true);

/**
 * @brief 分割字符串
 * @param str 要分割的字符串
 * @param delimiters 分隔符集合
 * @param skip_empty 是否跳过空字符串
 * @return 分割后的字符串向量
 *
 * 使用指定的分隔符集合分割字符串，返回子串的字符串向量。
 * 分隔符可以是多个字符，每个字符都作为独立的分隔符。
 */
vector<string> MSTL_API split(const string& str, const string& delimiters, bool skip_empty = true);

/**
 * @brief 连接字符串
 * @param vec 字符串向量
 * @param delimiter 连接符
 * @return 连接后的字符串
 *
 * 将字符串向量中的所有字符串用指定的连接符连接起来。
 * 使用普通的循环连接方式。
 */
string MSTL_API join(const vector<string>& vec, const string& delimiter = "");

/**
 * @brief 快速连接字符串
 * @param vec 字符串向量
 * @param delimiter 连接符
 * @return 连接后的字符串
 *
 * 预先计算总长度并预留空间，提高连接效率。
 * 适合处理大量字符串的连接操作。
 */
string MSTL_API join_fast(const vector<string>& vec, const string& delimiter = "");

/**
 * @brief 使用累加方式连接字符串
 * @param vec 字符串向量
 * @param delimiter 连接符
 * @return 连接后的字符串
 *
 * 使用accumulate算法进行字符串连接，适合函数式编程风格。
 */
string MSTL_API join_accumulate(const vector<string> &vec, const string& delimiter = "");

/**
 * @brief 字符串去重
 * @param vec 字符串向量
 * @return 去重后的字符串向量
 *
 * 移除向量中重复的字符串，保留首次出现的顺序。
 */
vector<string> MSTL_API unique(const vector<string>& vec);

/** @} */ // String

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_STRING_STRING_UTIL_HPP__
