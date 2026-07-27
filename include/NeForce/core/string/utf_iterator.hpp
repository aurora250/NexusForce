#ifndef NEFORCE_CORE_STRING_UTF_ITERATOR_HPP__
#define NEFORCE_CORE_STRING_UTF_ITERATOR_HPP__

/**
 * @file utf_iterator.hpp
 * @brief UTF-8 码点迭代器
 *
 * 此文件提供了UTF-8字符串的码点级迭代器，
 * 支持以 Unicode 码点为单位的字符级遍历。
 */

#include "NeForce/core/string/codepoint.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup UtfIterator UTF-8码点迭代器
 * @brief UTF-8字符串的码点级遍历支持
 * @{
 */

/**
 * @class utf8_iterator
 * @brief UTF-8前向迭代器，遍历字符串中的Unicode码点
 *
 * 将UTF-8字节序列解码为Unicode码点流。
 * 每次递增消耗当前码点的全部字节（1~4字节）。
 * 无效UTF-8序列产生替换符U+FFFD。
 *
 * @note 此迭代器为前向迭代器，不支持随机访问和双向遍历。
 */
class utf8_iterator {
public:
    using value_type = codepoint;                   ///< 值类型
    using reference = const codepoint&;             ///< 引用类型
    using pointer = const codepoint*;               ///< 指针类型
    using difference_type = ptrdiff_t;              ///< 差值类型
    using iterator_category = forward_iterator_tag; ///< 迭代器类别

private:
    const byte_t* data_ = nullptr;
    size_t remaining_ = 0;
    size_t consumed_ = 0;
    codepoint current_;

    void decode_current() noexcept {
        if ((data_ != nullptr) && remaining_ > 0) {
            size_t pos = 0;
            current_ = codepoint::decode_utf8(data_, pos, remaining_);
            consumed_ = pos;
        } else {
            current_ = codepoint::null();
            consumed_ = 0;
        }
    }

public:
    /**
     * @brief 默认构造函数，构造结束迭代器
     */
    utf8_iterator() noexcept = default;

    /**
     * @brief 构造函数
     * @param data UTF-8字节数据指针
     * @param remaining 剩余字节数
     */
    utf8_iterator(const byte_t* data, size_t remaining) noexcept :
    data_(data),
    remaining_(remaining) {
        decode_current();
    }

    /**
     * @brief 解引用操作符
     * @return 当前码点的常量引用
     */
    NEFORCE_NODISCARD reference operator*() const noexcept { return current_; }

    /**
     * @brief 成员访问操作符
     * @return 当前码点的常量指针
     */
    NEFORCE_NODISCARD pointer operator->() const noexcept { return &current_; }

    /**
     * @brief 前置递增操作符
     * @return 递增后的迭代器引用
     */
    utf8_iterator& operator++() noexcept {
        if ((data_ != nullptr) && remaining_ > 0) {
            data_ += consumed_;
            remaining_ -= _NEFORCE min(remaining_, consumed_);
            decode_current();
        }
        return *this;
    }

    /**
     * @brief 后置递增操作符
     * @return 递增前的迭代器副本
     */
    utf8_iterator operator++(int) noexcept {
        const utf8_iterator tmp = *this;
        ++*this;
        return move(tmp);
    }

    /**
     * @brief 相等比较操作符
     * @param other 另一个迭代器
     * @return 是否相等
     */
    NEFORCE_NODISCARD bool operator==(const utf8_iterator& other) const noexcept {
        if (remaining_ == 0 && other.remaining_ == 0) {
            return true;
        }
        return data_ == other.data_ && remaining_ == other.remaining_;
    }

    /**
     * @brief 不等比较操作符
     * @param other 另一个迭代器
     * @return 是否不等
     */
    NEFORCE_NODISCARD bool operator!=(const utf8_iterator& other) const noexcept { return !(*this == other); }
};

/**
 * @class utf8_range
 * @brief UTF-8码点遍历范围
 *
 * 支持范围for循环。
 */
class utf8_range {
    const byte_t* data_; ///< UTF-8字节数据指针
    size_t size_;        ///< 字节数

public:
    /**
     * @brief 构造函数
     * @param sv 字符串视图
     */
    explicit utf8_range(const string_view sv) noexcept :
    data_(reinterpret_cast<const byte_t*>(sv.data())),
    size_(sv.size()) {}

    explicit utf8_range(const byte_t* data, const size_t size) noexcept :
    data_(data),
    size_(size) {}

    /**
     * @brief 获取起始迭代器
     * @return 指向第一个码点的迭代器
     */
    NEFORCE_NODISCARD utf8_iterator begin() const noexcept { return {data_, size_}; }

    /**
     * @brief 获取结束迭代器
     * @return 结束标记迭代器
     */
    NEFORCE_NODISCARD utf8_iterator end() const noexcept { return {nullptr, 0}; }
};

/** @} */ // UtfIterator

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_STRING_UTF_ITERATOR_HPP__
