#ifndef NEFORCE_CORE_STRING_BASIC_STRING_HPP__
#define NEFORCE_CORE_STRING_BASIC_STRING_HPP__

/**
 * @file basic_string.hpp
 * @brief 动态字符串容器
 *
 * 此文件提供了动态字符串容器的实现。
 */

#include "NeForce/core/memory/allocator_traits.hpp"
#include "NeForce/core/memory/standard_allocator.hpp"
#include "NeForce/core/memory/uninitialized.hpp"
#include "NeForce/core/string/string_view.hpp"
#include "NeForce/core/utility/compressed_pair.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup String 字符串
 * @brief 动态字符序列容器
 * @{
 */

/**
 * @struct basic_string_iterator
 * @brief 字符串迭代器
 * @tparam IsConst 是否常量迭代器
 * @tparam String 字符串类型
 *
 * 为basic_string提供随机访问迭代器支持。
 */
template <bool IsConst, typename String>
struct basic_string_iterator : iiterator<basic_string_iterator<IsConst, String>> {
public:
    using container_type = String;                                    ///< 容器类型
    using value_type = typename container_type::value_type;           ///< 值类型
    using size_type = typename container_type::size_type;             ///< 大小类型
    using difference_type = typename container_type::difference_type; ///< 差值类型
    using iterator_category = contiguous_iterator_tag;                ///< 迭代器类别
    using reference = conditional_t<IsConst, typename container_type::const_reference,
                                    typename container_type::reference>; ///< 引用类型
    using pointer = conditional_t<IsConst, typename container_type::const_pointer,
                                  typename container_type::pointer>; ///< 指针类型

private:
    pointer current_ = nullptr;           ///< 当前指针位置
    const container_type* str_ = nullptr; ///< 关联字符串指针

public:
    NEFORCE_CONSTEXPR20 basic_string_iterator() noexcept = default;
    NEFORCE_CONSTEXPR20 ~basic_string_iterator() = default;

    NEFORCE_CONSTEXPR20 basic_string_iterator(const basic_string_iterator&) noexcept = default;
    NEFORCE_CONSTEXPR20 basic_string_iterator& operator=(const basic_string_iterator&) noexcept = default;
    NEFORCE_CONSTEXPR20 basic_string_iterator(basic_string_iterator&&) noexcept = default;
    NEFORCE_CONSTEXPR20 basic_string_iterator& operator=(basic_string_iterator&&) noexcept = default;

    /**
     * @brief 构造函数
     * @param ptr 初始指针位置
     * @param str 关联字符串指针
     */
    NEFORCE_CONSTEXPR20 basic_string_iterator(pointer ptr, const container_type* str) noexcept :
    current_(ptr),
    str_(str) {}

    /**
     * @brief 解引用操作
     * @return 当前元素的引用
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 reference dereference() const noexcept {
        NEFORCE_DEBUG_VERIFY(current_ && str_, "Attempting to dereference on a null pointer");
        NEFORCE_DEBUG_VERIFY(str_->data() <= current_ && current_ <= str_->data() + str_->size(),
                             "Attempting to dereference out of boundary");
        return *current_;
    }

    /**
     * @brief 递增操作
     */
    NEFORCE_CONSTEXPR20 void increment() noexcept {
        NEFORCE_DEBUG_VERIFY(current_ && str_, "Attempting to increment a null pointer");
        NEFORCE_DEBUG_VERIFY(current_ < str_->data() + str_->size(), "Attempting to increment out of boundary");
        ++current_;
    }

    /**
     * @brief 递减操作
     */
    NEFORCE_CONSTEXPR20 void decrement() noexcept {
        NEFORCE_DEBUG_VERIFY(current_ && str_, "Attempting to decrement a null pointer");
        NEFORCE_DEBUG_VERIFY(str_->data() < current_, "Attempting to decrement out of boundary");
        --current_;
    }

    /**
     * @brief 前进操作
     * @param off 前进距离
     */
    NEFORCE_CONSTEXPR20 void advance(difference_type off) noexcept {
        NEFORCE_DEBUG_VERIFY((current_ && str_) || off == 0, "Attempting to advance a null pointer");
        NEFORCE_DEBUG_VERIFY((off < 0 ? off >= str_->data() - current_ : off <= str_->data() + str_->size() - current_),
                             "Attempting to advance out of boundary");
        current_ += off;
    }

    /**
     * @brief 计算距离操作
     * @param other 另一个迭代器
     * @return 两个迭代器之间的距离
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 difference_type
    distance_to(const basic_string_iterator& other) const noexcept {
        NEFORCE_DEBUG_VERIFY(str_ == other.str_, "Attempting to distance to a different container");
        return static_cast<difference_type>(current_ - other.current_);
    }

    /**
     * @brief 下标访问操作符
     * @param n 偏移量
     * @return 偏移位置元素的引用
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 reference operator[](difference_type n) const noexcept {
        return *(*this + n);
    }

    /**
     * @brief 相等比较
     * @param rhs 右侧迭代器
     * @return 是否相等
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool equal(const basic_string_iterator& rhs) const noexcept {
        NEFORCE_DEBUG_VERIFY(str_ == rhs.str_, "Attempting to equal to a different container");
        return current_ == rhs.current_;
    }

    /**
     * @brief 小于比较
     * @param rhs 右侧迭代器
     * @return 当前迭代器是否在rhs之前
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool less_than(const basic_string_iterator& rhs) const noexcept {
        NEFORCE_DEBUG_VERIFY(str_ == rhs.str_, "Attempting to less than a different container");
        return current_ < rhs.current_;
    }

    /**
     * @brief 获取底层指针
     * @return 当前指针
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 pointer base() const noexcept { return current_; }

    /**
     * @brief 获取关联容器
     * @return 关联字符串指针
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 const container_type* container() const noexcept { return str_; }
};


/**
 * @class basic_string
 * @brief 基础字符串模板
 * @tparam CharT 字符类型
 * @tparam Traits 字符特征类型，默认为char_traits<CharT>
 * @tparam Alloc 分配器类型，默认为allocator<CharT>
 *
 * basic_string是一个动态字符序列容器，针对字符串操作进行了优化。
 * 支持小字符串优化（SSO）减少动态分配。
 */
template <typename CharT, typename Traits = char_traits<CharT>, typename Alloc = allocator<CharT>>
class basic_string : public icommon<basic_string<CharT, Traits, Alloc>> {
    static_assert(is_allocator_v<Alloc>, "Alloc type is not a standard allocator type.");
    static_assert(is_same_v<CharT, typename Alloc::value_type>, "allocator type mismatch.");
    static_assert(is_same_v<CharT, typename Traits::char_type>, "trait type mismatch.");
    static_assert(!is_array_v<CharT> && is_trivial_v<CharT> && is_standard_layout_v<CharT>,
                  "basic string only contains non-array trivial standard-layout types.");

public:
    using value_type = CharT;                                                 ///< 值类型
    using pointer = CharT*;                                                   ///< 指针类型
    using reference = CharT&;                                                 ///< 引用类型
    using const_pointer = const CharT*;                                       ///< 常量指针类型
    using const_reference = const CharT&;                                     ///< 常量引用类型
    using size_type = size_t;                                                 ///< 大小类型
    using difference_type = ptrdiff_t;                                        ///< 差值类型
    using iterator = basic_string_iterator<false, basic_string>;              ///< 迭代器类型
    using const_iterator = basic_string_iterator<true, basic_string>;         ///< 常量迭代器类型
    using reverse_iterator = _NEFORCE reverse_iterator<iterator>;             ///< 反向迭代器类型
    using const_reverse_iterator = _NEFORCE reverse_iterator<const_iterator>; ///< 常量反向迭代器类型

    using traits_type = Traits;                         ///< 字符特征类型
    using view_type = basic_string_view<CharT, Traits>; ///< 字符串视图类型
    using allocator_type = Alloc;                       ///< 分配器类型

    /// 特殊值，表示未找到或"直到末尾"
    static constexpr size_type npos = string_view::npos;

private:
#ifdef NEFORCE_USING_SSO
    /// SSO缓冲区字节数
    static constexpr size_type sso_buffer_bytes = MEMORY_ALIGN_THRESHHOLD;
    /// SSO缓冲区可容纳的字符数
    static constexpr size_type sso_buffer_size = (sso_buffer_bytes + sizeof(CharT) - 1) / sizeof(CharT);
    /// SSO最大容量
    static constexpr size_type sso_capacity = sso_buffer_size - 1;
    /// 长字符串标志（最高位）
    static constexpr size_type long_flag = static_cast<size_type>(1) << (sizeof(size_type) * 8 - 1);

    /// 压缩存储：分配器和大小
    compressed_pair<allocator_type, size_type> size_pair_{default_construct_tag{}, 0};
    /// 联合存储：长字符串指针/容量 或 短字符串缓冲区
    union storage {
        struct long_pointer {
            pointer ptr;   ///< 长字符串指针
            size_type cap; ///< 容量
        } long_;
        CharT short_[sso_buffer_size]; ///< 短字符串缓冲区
    } storage_;
#else
    pointer data_ = nullptr; ///< 数据指针
    size_type size_ = 0;     ///< 当前大小
    /// 压缩存储：分配器和容量
    compressed_pair<allocator_type, size_type> capacity_pair_{default_construct_tag{}, 0};
#endif

private:
#ifdef NEFORCE_USING_SSO
    /**
     * @brief 判断是否为长字符串模式
     * @return 是否为长字符串
     */
    NEFORCE_CONSTEXPR20 bool is_long() const noexcept { return (size_pair_.value & long_flag) != 0; }

    /**
     * @brief 设置大小
     * @param new_size 新大小
     */
    NEFORCE_CONSTEXPR20 void set_size(size_type new_size) noexcept {
        size_pair_.value = (is_long() ? (new_size | long_flag) : new_size);
    }

    /**
     * @brief 切换到长字符串模式
     * @param new_cap 新容量
     */
    NEFORCE_CONSTEXPR20 void switch_to_long(size_type new_cap) {
        NEFORCE_DEBUG_VERIFY(new_cap >= sso_buffer_size, "switch_to_long: new_cap too small");
        pointer new_ptr = size_pair_.get_base().allocate(new_cap);
        const size_type old_size = size();
        traits_type::copy(new_ptr, storage_.short_, old_size);
        traits_type::assign(new_ptr + old_size, 1, value_type());

        storage_.long_.ptr = new_ptr;
        storage_.long_.cap = new_cap;
        size_pair_.value = old_size | long_flag;
    }

    /**
     * @brief 销毁长字符串
     */
    NEFORCE_CONSTEXPR20 void destroy_long() noexcept {
        if (storage_.long_.ptr) {
            size_pair_.get_base().deallocate(storage_.long_.ptr, storage_.long_.cap);
            storage_.long_.ptr = nullptr;
            storage_.long_.cap = 0;
        }
    }
#endif

    /**
     * @brief 从迭代器范围构造
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator>
    NEFORCE_CONSTEXPR20 void construct_from_iter(Iterator first, Iterator last) {
        const size_type n = _NEFORCE distance(first, last);

#ifdef NEFORCE_USING_SSO
        if (n < sso_capacity) {
            pointer dest = storage_.short_;
            for (size_type i = 0; i < n; ++i) {
                dest[i] = *first++;
            }
            traits_type::assign(dest + n, 1, value_type());
            size_pair_.value = n;
        } else {
            const size_type init_cap = _NEFORCE max(sso_buffer_size, n + 1);
            pointer new_ptr = size_pair_.get_base().allocate(init_cap);
            pointer dest = new_ptr;
            for (size_type i = 0; i < n; ++i) {
                *dest++ = *first++;
            }
            traits_type::assign(new_ptr + n, 1, value_type());

            storage_.long_.ptr = new_ptr;
            storage_.long_.cap = init_cap;
            size_pair_.value = n | long_flag;
        }
#else
        const size_type init_size = _NEFORCE max(MEMORY_ALIGN_THRESHHOLD, n + 1);
        pointer temp_data = nullptr;
        try {
            temp_data = capacity_pair_.get_base().allocate(init_size);
            size_ = n;
            capacity_pair_.value = init_size;
            _NEFORCE uninitialized_copy(first, last, temp_data);

            data_ = temp_data;
            size_ = n;
            capacity_pair_.value = init_size;
            traits_type::assign(data_ + size_, 1, value_type());
        } catch (...) {
            if (temp_data) {
                _NEFORCE destroy(temp_data, temp_data + n);
                capacity_pair_.get_base().deallocate(temp_data, capacity_pair_.value);
            }
            destroy_buffer();
            throw;
        }
#endif
    }

    /**
     * @brief 从字符指针构造
     * @param str 源指针
     * @param position 起始位置
     * @param n 字符数
     */
    NEFORCE_CONSTEXPR20 void construct_from_ptr(const_pointer str, size_type position, size_type n) {
#ifdef NEFORCE_USING_SSO
        if (n < sso_capacity) {
            traits_type::copy(storage_.short_, str + position, n);
            traits_type::assign(storage_.short_ + n, 1, value_type());
            size_pair_.value = n;
        } else {
            const size_type init_cap = _NEFORCE max(sso_buffer_size, n + 1);
            pointer new_ptr = size_pair_.get_base().allocate(init_cap);
            traits_type::copy(new_ptr, str + position, n);
            traits_type::assign(new_ptr + n, 1, value_type());

            storage_.long_.ptr = new_ptr;
            storage_.long_.cap = init_cap;
            size_pair_.value = n | long_flag;
        }
#else
        pointer temp_data = nullptr;
        size_type temp_capacity = 0;
        try {
            temp_capacity = _NEFORCE max(MEMORY_ALIGN_THRESHHOLD, n + 1);
            temp_data = capacity_pair_.get_base().allocate(temp_capacity);
            traits_type::copy(temp_data, str + position, n);

            data_ = temp_data;
            size_ = n;
            capacity_pair_.value = temp_capacity;
            traits_type::assign(data_ + size_, 1, value_type());
        } catch (...) {
            if (temp_data) {
                capacity_pair_.get_base().deallocate(temp_data, capacity_pair_.value);
            }
            data_ = nullptr;
            size_ = 0;
            capacity_pair_.value = 0;
            throw;
        }
#endif
    }

    /**
     * @brief 销毁缓冲区
     */
    NEFORCE_CONSTEXPR20 void destroy_buffer() noexcept {
#ifdef NEFORCE_USING_SSO
        if (is_long()) {
            destroy_long();
        }
        size_pair_.value = 0;
        traits_type::assign(storage_.short_, 1, value_type());
#else
        if (data_) {
            if (capacity_pair_.value > 0) {
                capacity_pair_.get_base().deallocate(data_, capacity_pair_.value);
            }
            data_ = nullptr;
        }
        capacity_pair_.value = 0;
        size_ = 0;
#endif
    }

    /**
     * @brief 替换填充
     * @param first 起始位置
     * @param n1 原长度
     * @param n2 新长度
     * @param value 填充值
     * @return 自身引用
     */
    NEFORCE_CONSTEXPR20 basic_string& replace_fill(iterator first, size_type n1, const size_type n2,
                                                   const value_type value) {
#ifdef NEFORCE_USING_SSO
        const difference_type offset = first - begin();
        const size_type old_size = size();
        const size_type actual_n1 = _NEFORCE min(n1, old_size - offset);
        if (actual_n1 == 0 && n2 == 0) {
            return *this;
        }

        const size_type new_size = old_size - actual_n1 + n2;

        if (!is_long() && new_size < sso_capacity) {
            pointer p = storage_.short_ + offset;
            if (static_cast<difference_type>(old_size - offset - actual_n1) > 0) {
                traits_type::move(p + n2, p + actual_n1, old_size - offset - actual_n1);
            }
            traits_type::assign(p, n2, value);
            size_pair_.value = new_size;
            traits_type::assign(storage_.short_ + new_size, 1, value_type());
            return *this;
        }

        size_type new_cap = is_long() ? storage_.long_.cap : sso_buffer_size;
        if (new_cap < new_size + 1) {
            new_cap = _NEFORCE max(new_size + 1, new_cap + (new_cap >> 1));
        }

        pointer new_ptr = size_pair_.get_base().allocate(new_cap);
        pointer dest = new_ptr;

        dest = traits_type::copy(dest, data(), offset) + offset;
        dest = traits_type::assign(dest, n2, value) + n2;
        traits_type::copy(dest, data() + offset + actual_n1, old_size - offset - actual_n1);

        if (is_long()) {
            destroy_long();
        }
        storage_.long_.ptr = new_ptr;
        storage_.long_.cap = new_cap;
        size_pair_.value = new_size | long_flag;
        traits_type::assign(storage_.long_.ptr + new_size, 1, value_type());

        return *this;
#else
        if (static_cast<size_type>(end() - first) < n1) {
            n1 = cend() - first;
        }

        if (n1 < n2) {
            const size_type diff = n2 - n1;
            NEFORCE_DEBUG_VERIFY(size_ + diff < max_size(), "basic_string index out of range.");
            if (size_ > capacity_pair_.value - diff) {
                reallocate(diff);
            }

            pointer raw_ptr = &*first;
            traits_type::move(raw_ptr + n2, raw_ptr + n1, end() - (first + n1));
            traits_type::assign(raw_ptr, n2, value);
            size_ += diff;
        } else {
            pointer raw_ptr = &*first;
            traits_type::move(raw_ptr + n2, raw_ptr + n1, end() - (first + n1));
            traits_type::assign(raw_ptr, n2, value);
            size_ -= n1 - n2;
        }

        traits_type::assign(data_ + size_, 1, value_type());
        return *this;
#endif
    }

    /**
     * @brief 替换复制
     * @tparam Iterator 迭代器类型
     * @param first1 起始位置
     * @param last1 结束位置
     * @param first2 源起始
     * @param last2 源结束
     * @return 自身引用
     */
    template <typename Iterator, enable_if_t<is_iter_v<Iterator>, int> = 0>
    NEFORCE_CONSTEXPR20 basic_string& replace_copy(iterator first1, iterator last1, Iterator first2, Iterator last2) {
        static_assert(is_iter_v<Iterator> && is_same_v<iter_value_t<Iterator>, value_type>, "Iterator type mismatch.");

        size_type len1 = _NEFORCE distance(first1, last1);
        size_type len2 = _NEFORCE distance(first2, last2);

#ifdef NEFORCE_USING_SSO
        const difference_type offset = first1 - begin();
        const size_type old_size = size();
        const size_type new_size = old_size - len1 + len2;

        if (!is_long() && new_size < sso_capacity) {
            pointer p = storage_.short_ + offset;
            if (static_cast<difference_type>(old_size - offset - len1) > 0) {
                traits_type::move(p + len2, p + len1, old_size - offset - len1);
            }
            for (size_type i = 0; i < len2; ++i) {
                p[i] = *first2++;
            }
            size_pair_.value = new_size;
            traits_type::assign(storage_.short_ + new_size, 1, value_type());
            return *this;
        }

        size_type new_cap = is_long() ? storage_.long_.cap : sso_buffer_size;
        if (new_cap < new_size + 1) {
            new_cap = _NEFORCE max(new_size + 1, new_cap + (new_cap >> 1));
        }

        pointer new_ptr = size_pair_.get_base().allocate(new_cap);
        pointer dest = new_ptr;

        dest = traits_type::copy(dest, data(), offset) + offset;
        dest = _NEFORCE uninitialized_copy(first2, last2, dest);
        traits_type::copy(dest, data() + offset + len1, old_size - offset - len1);

        if (is_long()) {
            destroy_long();
        }
        storage_.long_.ptr = new_ptr;
        storage_.long_.cap = new_cap;
        size_pair_.value = new_size | long_flag;
        traits_type::assign(storage_.long_.ptr + new_size, 1, value_type());

        return *this;
#else
        if (len1 < len2) {
            const size_type diff = len2 - len1;
            NEFORCE_DEBUG_VERIFY(size_ + diff < max_size(), "basic_string replace_copy index out of range.");
            if (size_ > capacity_pair_.value - diff) {
                reallocate(diff);
            }

            pointer raw_ptr = &*first1;
            traits_type::move(raw_ptr + len2, raw_ptr + len1, end() - (first1 + len1));
            traits_type::copy(raw_ptr, &*first2, len2);
            size_ += diff;
        } else {
            pointer raw_ptr = &*first1;
            traits_type::move(raw_ptr + len2, raw_ptr + len1, end() - (first1 + len1));
            traits_type::copy(raw_ptr, &*first2, len2);
            size_ -= len1 - len2;
        }

        traits_type::assign(data_ + size_, 1, value_type());
        return *this;
#endif
    }

    /**
     * @brief 替换复制
     * @tparam Iterator 迭代器类型
     * @param first1 起始位置
     * @param n1 原长度
     * @param first2 源起始
     * @param n2 源长度
     * @return 自身引用
     */
    template <typename Iterator, enable_if_t<is_iter_v<Iterator>, int> = 0>
    NEFORCE_CONSTEXPR20 basic_string& replace_copy(iterator first1, const size_type n1, Iterator first2,
                                                   const size_type n2) {
        return replace_copy(first1, first1 + n1, first2, _NEFORCE next(first2, n2));
    }

    /**
     * @brief 重新分配内存
     * @param n 需要增加的空间
     */
    NEFORCE_CONSTEXPR20 void reallocate(size_type n) {
#ifdef NEFORCE_USING_SSO
        if (!is_long()) {
            const size_type new_cap = _NEFORCE max(sso_buffer_size, size() + n + 1);
            switch_to_long(new_cap);
            return;
        }

        const size_type old_cap = storage_.long_.cap;
        const size_type min_new_cap = size() + n + 1;
        const size_type new_cap = _NEFORCE max(min_new_cap, old_cap + (old_cap >> 1));

        pointer new_ptr = size_pair_.get_base().allocate(new_cap);
        traits_type::move(new_ptr, storage_.long_.ptr, size());
        traits_type::assign(new_ptr + size(), 1, value_type());

        destroy_long();
        storage_.long_.ptr = new_ptr;
        storage_.long_.cap = new_cap;
#else
        pointer new_buffer = nullptr;
        try {
            const size_t new_cap =
                    _NEFORCE max(capacity_pair_.value + n, capacity_pair_.value + (capacity_pair_.value >> 1)) + 1;
            new_buffer = capacity_pair_.get_base().allocate(new_cap);
            traits_type::move(new_buffer, data_, size_);

            capacity_pair_.get_base().deallocate(data_, capacity_pair_.value);
            data_ = new_buffer;
            capacity_pair_.value = new_cap;
            traits_type::assign(data_ + size_, 1, value_type());
        } catch (...) {
            if (new_buffer) {
                capacity_pair_.get_base().deallocate(new_buffer, capacity_pair_.value);
            }
            throw;
        }
#endif
    }

    /**
     * @brief 重新分配并填充插入
     * @param position 插入位置
     * @param n 插入数量
     * @param value 插入值
     * @return 指向插入起始的迭代器
     */
    NEFORCE_CONSTEXPR20 iterator reallocate_fill(iterator position, size_type n, value_type value) {
#ifdef NEFORCE_USING_SSO
        const size_type offset = position - begin();
        if (!is_long() && size() + n < sso_buffer_size) {
            pointer p = storage_.short_ + offset;
            traits_type::move(p + n, p, size() - offset);
            traits_type::assign(p, n, value);
            size_pair_.value = (size() + n);
            traits_type::assign(storage_.short_ + size(), 1, value_type());
            return iterator(storage_.short_ + offset, this);
        }

        const size_type old_size = size();
        const size_type new_cap = _NEFORCE max((is_long() ? storage_.long_.cap : sso_buffer_size) + n,
                                               (is_long() ? storage_.long_.cap : sso_buffer_size) +
                                                       ((is_long() ? storage_.long_.cap : sso_buffer_size) >> 1)) +
                                  1;

        pointer new_ptr = size_pair_.get_base().allocate(new_cap);
        pointer dest = new_ptr;

        dest = traits_type::copy(dest, data(), offset) + offset;
        dest = traits_type::assign(dest, n, value) + n;
        traits_type::copy(dest, data() + offset, old_size - offset);

        if (is_long()) {
            destroy_long();
        }
        storage_.long_.ptr = new_ptr;
        storage_.long_.cap = new_cap;
        size_pair_.value = (old_size + n) | long_flag;
        traits_type::assign(storage_.long_.ptr + size(), 1, value_type());

        return iterator(storage_.long_.ptr + offset, this);
#else
        const difference_type diff = (&*position) - data_;
        const size_t old_cap = capacity_pair_.value;
        const size_t new_cap = _NEFORCE max(old_cap + n, old_cap + (old_cap >> 1));
        pointer new_buffer = capacity_pair_.get_base().allocate(new_cap);
        pointer end1 = traits_type::move(new_buffer, data_, diff) + diff;
        pointer end2 = traits_type::assign(end1, n, value) + n;
        traits_type::move(end2, data_ + diff, size_ - diff);
        capacity_pair_.get_base().deallocate(data_, old_cap);
        data_ = new_buffer;
        size_ += n;
        capacity_pair_.value = new_cap;
        traits_type::assign(data_ + size_, 1, value_type());
        return iterator(data_ + diff, this);
#endif
    }

    /**
     * @brief 重新分配并复制插入
     * @tparam Iterator 迭代器类型
     * @param position 插入位置
     * @param first 源起始
     * @param last 源结束
     * @return 指向插入起始的迭代器
     */
    template <typename Iterator>
    NEFORCE_CONSTEXPR20 iterator reallocate_copy(iterator position, Iterator first, Iterator last) {
#ifdef NEFORCE_USING_SSO
        const size_type offset = position - begin();
        const size_type n = _NEFORCE distance(first, last);
        const size_type old_size = size();

        if (!is_long() && old_size + n < sso_buffer_size) {
            pointer p = storage_.short_ + offset;
            traits_type::move(p + n, p, old_size - offset);
            for (size_type i = 0; i < n; ++i) {
                p[i] = *first++;
            }
            size_pair_.value = old_size + n;
            traits_type::assign(storage_.short_ + size(), 1, value_type());
            return iterator(storage_.short_ + offset, this);
        }

        const size_type new_cap = _NEFORCE max((is_long() ? storage_.long_.cap : sso_buffer_size) + n,
                                               (is_long() ? storage_.long_.cap : sso_buffer_size) +
                                                       ((is_long() ? storage_.long_.cap : sso_buffer_size) >> 1)) +
                                  1;

        pointer new_ptr = size_pair_.get_base().allocate(new_cap);
        pointer dest = new_ptr;

        dest = traits_type::copy(dest, data(), offset) + offset;
        dest = _NEFORCE uninitialized_copy(first, last, dest);
        traits_type::copy(dest, data() + offset, old_size - offset);

        if (is_long()) {
            destroy_long();
        }
        storage_.long_.ptr = new_ptr;
        storage_.long_.cap = new_cap;
        size_pair_.value = (old_size + n) | long_flag;
        traits_type::assign(storage_.long_.ptr + size(), 1, value_type());

        return iterator(storage_.long_.ptr + offset, this);
#else
        const difference_type diff = position - begin();
        const size_type old_cap = capacity_pair_.value;
        const size_type n = _NEFORCE distance(first, last);
        const size_t new_cap = _NEFORCE max(old_cap + n, old_cap + (old_cap >> 1));
        pointer new_buffer = capacity_pair_.get_base().allocate(new_cap);
        pointer end1 = traits_type::move(new_buffer, data_, diff) + diff;
        pointer end2 = _NEFORCE uninitialized_copy_n(first, n, end1).second + n;
        traits_type::move(end2, data_ + diff, size_ - diff);
        capacity_pair_.get_base().deallocate(data_, old_cap);
        data_ = new_buffer;
        size_ += n;
        capacity_pair_.value = new_cap;
        traits_type::assign(data_ + size_, 1, value_type());
        return iterator(data_ + diff, this);
#endif
    }

public:
    /**
     * @brief 默认构造函数
     *
     * 构造一个空字符串。
     */
    NEFORCE_CONSTEXPR20 basic_string() {
#ifdef NEFORCE_USING_SSO
        traits_type::assign(storage_.short_, 1, value_type());
        size_pair_.value = 0;
#else
        basic_string::reserve(MEMORY_ALIGN_THRESHHOLD);
#endif
    }

    /**
     * @brief 构造函数，指定大小
     * @param n 字符数
     */
    NEFORCE_CONSTEXPR20 explicit basic_string(size_type n) :
    basic_string(n, static_cast<value_type>(0)) {}

    /**
     * @brief 构造函数，指定大小和32位整数值
     * @param n 字符数
     * @param value 整数值
     */
    NEFORCE_CONSTEXPR20 explicit basic_string(size_type n, int32_t value) :
    basic_string(n, static_cast<value_type>(value)) {}

    /**
     * @brief 构造函数，指定大小和64位整数值
     * @param n 字符数
     * @param value 整数值
     */
    NEFORCE_CONSTEXPR20 explicit basic_string(size_type n, int64_t value) :
    basic_string(n, static_cast<value_type>(value)) {}

    /**
     * @brief 构造函数，指定大小和填充字符
     * @param n 字符数
     * @param value 填充字符
     */
    NEFORCE_CONSTEXPR20 explicit basic_string(size_type n, value_type value) {
#ifdef NEFORCE_USING_SSO
        if (n < sso_capacity) {
            traits_type::assign(storage_.short_, n, value);
            traits_type::assign(storage_.short_ + n, 1, value_type());
            size_pair_.value = n;
        } else {
            const size_type init_cap = _NEFORCE max(sso_buffer_size, n + 1);
            pointer new_ptr = size_pair_.get_base().allocate(init_cap);
            traits_type::assign(new_ptr, n, value);
            traits_type::assign(new_ptr + n, 1, value_type());

            storage_.long_.ptr = new_ptr;
            storage_.long_.cap = init_cap;
            size_pair_.value = n | long_flag;
        }
#else
        const size_type init_size = _NEFORCE max(MEMORY_ALIGN_THRESHHOLD, n + 1);
        data_ = capacity_pair_.get_base().allocate(init_size);
        traits_type::assign(data_, n, value);
        size_ = n;
        capacity_pair_.value = init_size;
        traits_type::assign(data_ + size_, 1, value_type());
#endif
    }

    /**
     * @brief 拷贝构造函数
     * @param other 源字符串
     */
    NEFORCE_CONSTEXPR20 basic_string(const basic_string& other) {
#ifdef NEFORCE_USING_SSO
        const size_type len = other.size();
        if (len < sso_capacity) {
            traits_type::copy(storage_.short_, other.data(), len);
            traits_type::assign(storage_.short_ + len, 1, value_type());
            size_pair_.value = len;
        } else {
            const size_type cap = other.is_long() ? other.storage_.long_.cap : (len + 1);
            pointer new_ptr = size_pair_.get_base().allocate(cap);
            traits_type::copy(new_ptr, other.data(), len);
            traits_type::assign(new_ptr + len, 1, value_type());

            storage_.long_.ptr = new_ptr;
            storage_.long_.cap = cap;
            size_pair_.value = len | long_flag;
        }
#else
        construct_from_ptr(other.data(), 0, other.size());
#endif
    }

    /**
     * @brief 拷贝赋值运算符
     * @param other 源字符串
     * @return 自身引用
     */
    NEFORCE_CONSTEXPR20 basic_string& operator=(const basic_string& other) {
        if (_NEFORCE addressof(other) == this) {
            return *this;
        }

#ifdef NEFORCE_USING_SSO
        const size_type len = other.size();

        if (len < sso_capacity) {
            if (is_long()) {
                destroy_long();
            }
            traits_type::copy(storage_.short_, other.data(), len);
            traits_type::assign(storage_.short_ + len, 1, value_type());
            size_pair_.value = len;
        } else {
            if (is_long()) {
                if (storage_.long_.cap >= len + 1) {
                    traits_type::copy(storage_.long_.ptr, other.data(), len);
                    traits_type::assign(storage_.long_.ptr + len, 1, value_type());
                    size_pair_.value = len | long_flag;
                    return *this;
                }
                destroy_long();
            }

            const size_type cap = other.is_long() ? other.storage_.long_.cap : (len + 1);
            pointer new_ptr = size_pair_.get_base().allocate(cap);
            traits_type::copy(new_ptr, other.data(), len);
            traits_type::assign(new_ptr + len, 1, value_type());

            storage_.long_.ptr = new_ptr;
            storage_.long_.cap = cap;
            size_pair_.value = len | long_flag;
        }
#else
        destroy_buffer();
        construct_from_ptr(other.data(), 0, other.size());
#endif
        return *this;
    }

    /**
     * @brief 移动构造函数
     * @param other 源字符串
     */
    NEFORCE_CONSTEXPR20 basic_string(basic_string&& other) noexcept
#ifdef NEFORCE_USING_SSO
    :
    size_pair_(_NEFORCE move(other.size_pair_)) {
        if (other.is_long()) {
            storage_.long_.ptr = other.storage_.long_.ptr;
            storage_.long_.cap = other.storage_.long_.cap;
            size_pair_.value = other.size_pair_.value;

            other.storage_.long_.ptr = nullptr;
            other.storage_.long_.cap = 0;
            other.size_pair_.value = 0;
        } else {
            traits_type::copy(storage_.short_, other.storage_.short_, other.size() + 1);
            size_pair_.value = other.size();
            traits_type::assign(other.storage_.short_, 1, value_type());
            other.size_pair_.value = 0;
        }
#else
    :
    data_(other.data_),
    size_(other.size_),
    capacity_pair_(_NEFORCE move(other.capacity_pair_)) {
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_pair_.value = 0;
#endif
    }

    /**
     * @brief 移动赋值运算符
     * @param other 源字符串
     * @return 自身引用
     */
    NEFORCE_CONSTEXPR20 basic_string& operator=(basic_string&& other) noexcept {
        if (_NEFORCE addressof(other) == this) {
            return *this;
        }

#ifdef NEFORCE_USING_SSO
        destroy_buffer();

        size_pair_ = _NEFORCE move(other.size_pair_);

        if (other.is_long()) {
            storage_.long_.ptr = other.storage_.long_.ptr;
            storage_.long_.cap = other.storage_.long_.cap;
            size_pair_.value = other.size_pair_.value;

            other.storage_.long_.ptr = nullptr;
            other.storage_.long_.cap = 0;
            other.size_pair_.value = 0;
        } else {
            traits_type::copy(storage_.short_, other.storage_.short_, other.size() + 1);
            size_pair_.value = other.size();
            traits_type::assign(other.storage_.short_, 1, value_type());
            other.size_pair_.value = 0;
        }
#else
        pointer new_data = other.data_;
        const size_type new_size = other.size_;
        auto new_capacity_pair = _NEFORCE move(other.capacity_pair_);

        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_pair_.value = 0;

        destroy_buffer();
        data_ = new_data;
        size_ = new_size;
        capacity_pair_ = _NEFORCE move(new_capacity_pair);
#endif

        return *this;
    }

    /**
     * @brief 从字符串视图构造
     * @param view 字符串视图
     */
    NEFORCE_CONSTEXPR20 basic_string(view_type view) { construct_from_ptr(view.data(), 0, view.size()); }

    /**
     * @brief 从字符串视图构造（指定长度）
     * @param view 字符串视图
     * @param n 字符数
     */
    NEFORCE_CONSTEXPR20 basic_string(view_type view, const size_type n) { construct_from_ptr(view.data(), 0, n); }

    /**
     * @brief 字符串视图赋值运算符
     * @param view 字符串视图
     * @return 自身引用
     */
    NEFORCE_CONSTEXPR20 basic_string& operator=(view_type view) {
        const size_type len = view.size();

#ifdef NEFORCE_USING_SSO
        if (len < sso_capacity) {
            if (is_long()) {
                destroy_long();
            }
            traits_type::copy(storage_.short_, view.data(), len);
            traits_type::assign(storage_.short_ + len, 1, value_type());
            size_pair_.value = len;
        } else {
            if (is_long() && storage_.long_.cap >= len + 1) {
                traits_type::copy(storage_.long_.ptr, view.data(), len);
                traits_type::assign(storage_.long_.ptr + len, 1, value_type());
                size_pair_.value = len | long_flag;
                return *this;
            }

            if (is_long()) {
                destroy_long();
            }
            const size_type new_cap = len + 1;
            pointer new_ptr = size_pair_.get_base().allocate(new_cap);
            traits_type::copy(new_ptr, view.data(), len);
            traits_type::assign(new_ptr + len, 1, value_type());

            storage_.long_.ptr = new_ptr;
            storage_.long_.cap = new_cap;
            size_pair_.value = len | long_flag;
        }
#else
        if (capacity_pair_.value < len) {
            pointer new_buffer = capacity_pair_.get_base().allocate(len + 1);
            capacity_pair_.get_base().deallocate(data_);
            data_ = new_buffer;
            capacity_pair_.value = len + 1;
        }

        traits_type::copy(data_, view.data(), len);
        size_ = len;
        traits_type::assign(data_ + size_, 1, value_type());
#endif
        return *this;
    }

    /**
     * @brief 从子串构造
     * @param other 源字符串
     * @param position 起始位置
     */
    NEFORCE_CONSTEXPR20 basic_string(const basic_string& other, size_type position) {
        NEFORCE_DEBUG_VERIFY(position <= other.size(), "basic_string index out of range");
        construct_from_ptr(other.data(), position, other.size() - position);
    }

    /**
     * @brief 从子串构造（指定长度）
     * @param other 源字符串
     * @param position 起始位置
     * @param n 字符数
     */
    NEFORCE_CONSTEXPR20 basic_string(const basic_string& other, size_type position, size_type n) {
        NEFORCE_DEBUG_VERIFY(position <= other.size(), "basic_string index out of range");
        n = _NEFORCE min(n, other.size() - position);
        construct_from_ptr(other.data(), position, n);
    }

    /**
     * @brief 从C风格字符串构造
     * @param str C风格字符串
     */
    NEFORCE_CONSTEXPR20 basic_string(const_pointer str) { construct_from_ptr(str, 0, traits_type::length(str)); }

    /**
     * @brief 从字符数组构造（指定长度）
     * @param str 字符指针
     * @param n 字符数
     */
    NEFORCE_CONSTEXPR20 basic_string(const_pointer str, const size_type n) { construct_from_ptr(str, 0, n); }

    /**
     * @brief C风格字符串赋值运算符
     * @param str C风格字符串
     * @return 自身引用
     */
    NEFORCE_CONSTEXPR20 basic_string& operator=(const_pointer str) {
        const size_type len = traits_type::length(str);
#ifdef NEFORCE_USING_SSO
        if (len < sso_capacity) {
            if (is_long()) {
                destroy_long();
            }
            traits_type::copy(storage_.short_, str, len);
            traits_type::assign(storage_.short_ + len, 1, value_type());
            size_pair_.value = len;
        } else {
            if (is_long() && storage_.long_.cap >= len + 1) {
                traits_type::copy(storage_.long_.ptr, str, len);
                traits_type::assign(storage_.long_.ptr + len, 1, value_type());
                size_pair_.value = len | long_flag;
                return *this;
            }

            if (is_long()) {
                destroy_long();
            }
            const size_type new_cap = len + 1;
            pointer new_ptr = size_pair_.get_base().allocate(new_cap);
            traits_type::copy(new_ptr, str, len);
            traits_type::assign(new_ptr + len, 1, value_type());

            storage_.long_.ptr = new_ptr;
            storage_.long_.cap = new_cap;
            size_pair_.value = len | long_flag;
        }
#else
        if (capacity_pair_.value < len) {
            pointer new_buffer = capacity_pair_.get_base().allocate(len + 1);
            capacity_pair_.get_base().deallocate(data_);
            data_ = new_buffer;
            capacity_pair_.value = len + 1;
        }
        traits_type::copy(data_, str, len);
        size_ = len;
        traits_type::assign(data_ + size_, 1, value_type());
#endif
        return *this;
    }

    /**
     * @brief 从迭代器范围构造
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator, enable_if_t<!is_convertible_v<Iterator, value_type>, int> = 0>
    NEFORCE_CONSTEXPR20 basic_string(Iterator first, Iterator last) {
        construct_from_iter(first, last);
    }

    /**
     * @brief 从初始化列表构造
     * @param ilist 初始化列表
     */
    NEFORCE_CONSTEXPR20 basic_string(std::initializer_list<value_type> ilist) :
    basic_string(ilist.begin(), ilist.end()) {}

    /**
     * @brief 初始化列表赋值运算符
     * @param ilist 初始化列表
     * @return 自身引用
     */
    NEFORCE_CONSTEXPR20 basic_string& operator=(std::initializer_list<value_type> ilist) {
        clear();
        insert(begin(), ilist.begin(), ilist.end());
        return *this;
    }

    /**
     * @brief 析构函数
     */
    NEFORCE_CONSTEXPR20 ~basic_string() { destroy_buffer(); }

    /**
     * @brief 获取起始迭代器
     * @return 指向第一个字符的迭代器
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 iterator begin() noexcept { return {data(), this}; }

    /**
     * @brief 获取结束迭代器
     * @return 指向最后一个字符之后位置的迭代器
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 iterator end() noexcept { return {data() + size(), this}; }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个字符的常量迭代器
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 const_iterator begin() const noexcept { return cbegin(); }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向最后一个字符之后位置的常量迭代器
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 const_iterator end() const noexcept { return cend(); }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个字符的常量迭代器
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 const_iterator cbegin() const noexcept { return {data(), this}; }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向最后一个字符之后位置的常量迭代器
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 const_iterator cend() const noexcept { return {data() + size(), this}; }

    /**
     * @brief 获取反向起始迭代器
     * @return 指向最后一个字符的反向迭代器
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }

    /**
     * @brief 获取反向结束迭代器
     * @return 指向第一个字符之前位置的反向迭代器
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 reverse_iterator rend() noexcept { return reverse_iterator(begin()); }

    /**
     * @brief 获取常量反向起始迭代器
     * @return 指向最后一个字符的常量反向迭代器
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 const_reverse_iterator rbegin() const noexcept { return crbegin(); }

    /**
     * @brief 获取常量反向结束迭代器
     * @return 指向第一个字符之前位置的常量反向迭代器
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 const_reverse_iterator rend() const noexcept { return crend(); }

    /**
     * @brief 获取常量反向起始迭代器
     * @return 指向最后一个字符的常量反向迭代器
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 const_reverse_iterator crbegin() const noexcept {
        return const_reverse_iterator(cend());
    }

    /**
     * @brief 获取常量反向结束迭代器
     * @return 指向第一个字符之前位置的常量反向迭代器
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 const_reverse_iterator crend() const noexcept {
        return const_reverse_iterator(cbegin());
    }

    /**
     * @brief 获取字符数
     * @return 字符串长度
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type size() const noexcept {
#ifdef NEFORCE_USING_SSO
        return size_pair_.value & ~long_flag;
#else
        return size_;
#endif
    }

    /**
     * @brief 获取最大可能大小
     * @return 最大长度
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type max_size() const noexcept { return npos; }

    /**
     * @brief 获取容量
     * @return 当前分配的存储可容纳的字符数
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type capacity() const noexcept {
#ifdef NEFORCE_USING_SSO
        return is_long() ? storage_.long_.cap : sso_buffer_size;
#else
        return capacity_pair_.value;
#endif
    }

    /**
     * @brief 获取字符串长度
     * @return 字符串长度
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type length() const noexcept { return size(); }

    /**
     * @brief 检查是否为空
     * @return 是否为空
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool empty() const noexcept { return size() == 0; }

    /**
     * @brief 预留容量
     * @param n 要预留的字符数
     */
    NEFORCE_CONSTEXPR20 void reserve(const size_type n) {
        NEFORCE_DEBUG_VERIFY(n < max_size(), "basic_string reserve index out of range.");
        const size_type new_cap = n + 1;
        if (new_cap <= capacity()) {
            return;
        }

#ifdef NEFORCE_USING_SSO
        if (!is_long()) {
            switch_to_long(new_cap);
        } else {
            pointer new_ptr = size_pair_.get_base().allocate(new_cap);
            traits_type::move(new_ptr, storage_.long_.ptr, size());
            traits_type::assign(new_ptr + size(), 1, value_type());
            destroy_long();
            storage_.long_.ptr = new_ptr;
            storage_.long_.cap = new_cap;
        }
#else
        pointer new_buffer = capacity_pair_.get_base().allocate(new_cap);
        traits_type::move(new_buffer, data_, size_);
        capacity_pair_.get_base().deallocate(data_, capacity_pair_.value);

        data_ = new_buffer;
        capacity_pair_.value = new_cap;
        traits_type::assign(data_ + size_, 1, value_type());
#endif
    }

    /**
     * @brief 下标访问操作符
     * @param n 索引
     * @return 指定位置的字符引用
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 reference operator[](const size_type n) noexcept {
        NEFORCE_DEBUG_VERIFY(n <= size(), "basic_string [] index out of range.");
        return *(data() + n);
    }

    /**
     * @brief 常量下标访问操作符
     * @param n 索引
     * @return 指定位置的字符常量引用
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 const_reference operator[](const size_type n) const noexcept {
        NEFORCE_DEBUG_VERIFY(n <= size(), "basic_string [] index out of range.");
        return *(data() + n);
    }

    /**
     * @brief 带边界检查的访问
     * @param n 索引
     * @return 指定位置的字符引用
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 reference at(const size_type n) noexcept { return (*this)[n]; }

    /**
     * @brief 带边界检查的常量访问
     * @param n 索引
     * @return 指定位置的字符常量引用
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 const_reference at(const size_type n) const noexcept { return (*this)[n]; }

    /**
     * @brief 访问第一个字符
     * @return 第一个字符的引用
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 reference front() noexcept {
        NEFORCE_DEBUG_VERIFY(!empty(), "front called on empty basic_string");
        return *data();
    }

    /**
     * @brief 常量访问第一个字符
     * @return 第一个字符的常量引用
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 const_reference front() const noexcept {
        NEFORCE_DEBUG_VERIFY(!empty(), "front called on empty basic_string");
        return *data();
    }

    /**
     * @brief 访问最后一个字符
     * @return 最后一个字符的引用
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 reference back() noexcept {
        NEFORCE_DEBUG_VERIFY(!empty(), "back called on empty basic_string");
        return *(data() + size() - 1);
    }

    /**
     * @brief 常量访问最后一个字符
     * @return 最后一个字符的常量引用
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 const_reference back() const noexcept {
        NEFORCE_DEBUG_VERIFY(!empty(), "back called on empty basic_string");
        return *(data() + size() - 1);
    }

    /**
     * @brief 获取数据指针
     * @return 指向底层字符数组的指针
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 pointer data() noexcept {
#ifdef NEFORCE_USING_SSO
        if (!is_long()) {
            return storage_.short_;
        }
        return storage_.long_.ptr;
#else
        return data_;
#endif
    }

    /**
     * @brief 获取常量数据指针
     * @return 指向底层字符数组的常量指针
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 const_pointer data() const noexcept {
#ifdef NEFORCE_USING_SSO
        if (!is_long()) {
            return storage_.short_;
        }
        return storage_.long_.ptr;
#else
        return data_;
#endif
    }

    /**
     * @brief 插入单个字符
     * @param position 插入位置
     * @param value 要插入的字符
     * @return 指向插入字符的迭代器
     */
    NEFORCE_CONSTEXPR20 iterator insert(iterator position, value_type value) {
#ifdef NEFORCE_USING_SSO
        const size_type offset = position - begin();
        if (!is_long() && size() + 1 < sso_buffer_size) {
            pointer p = storage_.short_ + offset;
            traits_type::move(p + 1, p, size() - offset);
            *p = value;
            ++size_pair_.value;
            traits_type::assign(storage_.short_ + size(), 1, value_type());
            return iterator(p, this);
        }
        return basic_string::reallocate_fill(position, 1, value);
#else
        if (size_ == capacity_pair_.value) {
            return basic_string::reallocate_fill(position, 1, value);
        }

        size_type offset = position - begin();
        pointer p = data_ + offset;
        size_type chars_after = size_ - offset;
        if (chars_after > 0) {
            traits_type::move(p + 1, p, chars_after);
        }

        *p = value;
        ++size_;
        traits_type::assign(data_ + size_, 1, value_type());
        return iterator(p, this);
#endif
    }

    /**
     * @brief 在指定位置插入多个相同字符
     * @param position 插入位置
     * @param n 插入数量
     * @param value 插入字符
     * @return 自身引用
     */
    NEFORCE_CONSTEXPR20 basic_string& insert(size_type position, size_type n, value_type value) {
        insert(begin() + position, n, value);
        return *this;
    }

    NEFORCE_CONSTEXPR20 iterator insert(iterator position, size_type n, value_type value) {
        if (n == 0) {
            return position;
        }

#ifdef NEFORCE_USING_SSO
        if (!is_long() && size() + n < sso_buffer_size) {
            const size_type offset = position - begin();
            pointer p = storage_.short_ + offset;
            traits_type::move(p + n, p, size() - offset);
            traits_type::assign(p, n, value);
            size_pair_.value = size() + n;
            traits_type::assign(storage_.short_ + size(), 1, value_type());
            return iterator(p, this);
        }

        return basic_string::reallocate_fill(position, n, value);
#else
        if (capacity_pair_.value - size_ < n) {
            return basic_string::reallocate_fill(position, n, value);
        }

        const size_type offset = position - begin();
        pointer p = data_ + offset;
        const size_type chars_after = size_ - offset;

        if (chars_after > 0) {
            traits_type::move(p + n, p, chars_after);
        }
        traits_type::assign(p, n, value);

        size_ += n;
        traits_type::assign(data_ + size_, 1, value_type());
        return iterator(p, this);
#endif
    }

    /**
     * @brief 插入迭代器范围
     * @tparam Iterator 迭代器类型
     * @param position 插入位置
     * @param first 源起始
     * @param last 源结束
     * @return 指向插入起始的迭代器
     */
    template <typename Iterator>
    NEFORCE_CONSTEXPR20 iterator insert(iterator position, Iterator first, Iterator last) {
        const size_type len = _NEFORCE distance(first, last);
        if (len == 0) {
            return position;
        }

#ifdef NEFORCE_USING_SSO
        if (!is_long() && size() + len < sso_buffer_size) {
            const size_type offset = position - begin();
            pointer p = storage_.short_ + offset;
            traits_type::move(p + len, p, size() - offset);
            for (size_type i = 0; i < len; ++i) {
                p[i] = *first++;
            }
            size_pair_.value = size() + len;
            traits_type::assign(storage_.short_ + size(), 1, value_type());
            return iterator(p, this);
        }
        return basic_string::reallocate_copy(position, first, last);
#else
        if (capacity_pair_.value - size_ < len) {
            return basic_string::reallocate_copy(position, first, last);
        }

        const size_type offset = position - begin();
        pointer p = data_ + offset;
        const size_type chars_after = size_ - offset;

        if (chars_after > 0) {
            traits_type::move(p + len, p, chars_after);
        }
        pointer curr = p;
        for (Iterator it = first; it != last; ++it, ++curr) {
            *curr = *it;
        }

        size_ += len;
        traits_type::assign(data_ + size_, 1, value_type());
        return position;
#endif
    }

    /**
     * @brief 在末尾插入字符
     * @param value 要插入的字符
     */
    NEFORCE_CONSTEXPR20 void push_back(value_type value) { append(1, value); }

    /**
     * @brief 删除末尾字符
     */
    NEFORCE_CONSTEXPR20 void pop_back() noexcept {
        NEFORCE_DEBUG_VERIFY(!empty(), "pop_back called on empty basic_string");
#ifdef NEFORCE_USING_SSO
        const size_type new_size = size() - 1;
        if (is_long()) {
            size_pair_.value = new_size | long_flag;
            traits_type::assign(storage_.long_.ptr + new_size, 1, value_type());
        } else {
            size_pair_.value = new_size;
            traits_type::assign(storage_.short_ + new_size, 1, value_type());
        }
#else
        --size_;
        traits_type::assign(data_ + size_, 1, value_type());
#endif
    }

    /**
     * @brief 追加多个相同字符
     * @param n 字符数量
     * @param value 要追加的字符
     * @return 自身引用
     */
    NEFORCE_CONSTEXPR20 basic_string& append(size_type n, value_type value) {
        NEFORCE_DEBUG_VERIFY(size() + n < max_size(), "basic_string append iterator out of ranges.");
        if (n == 0) {
            return *this;
        }

#ifdef NEFORCE_USING_SSO
        if (!is_long() && size() + n < sso_buffer_size) {
            pointer p = storage_.short_ + size();
            traits_type::assign(p, n, value);
            size_pair_.value = size() + n;
            traits_type::assign(storage_.short_ + size(), 1, value_type());
            return *this;
        }

        const size_type old_size = size();
        if (is_long() && storage_.long_.cap >= old_size + n + 1) {
            pointer p = storage_.long_.ptr + old_size;
            traits_type::assign(p, n, value);
            size_pair_.value = (old_size + n) | long_flag;
            traits_type::assign(storage_.long_.ptr + size(), 1, value_type());
            return *this;
        }

        reallocate(n);
        pointer p = data() + old_size;
        traits_type::assign(p, n, value);
        size_pair_.value = (old_size + n) | (is_long() ? long_flag : 0);
        traits_type::assign(data() + size(), 1, value_type());
#else
        if (capacity_pair_.value - size_ <= n) {
            reallocate(n);
        }
        traits_type::assign(data_ + size_, n, value);
        size_ += n;
        traits_type::assign(data_ + size_, 1, value_type());
#endif
        return *this;
    }

    /**
     * @brief 追加单个字符
     * @param value 要追加的字符
     * @return 自身引用
     */
    NEFORCE_CONSTEXPR20 basic_string& append(value_type value) { return append(1, value); }

    /**
     * @brief 追加另一个字符串的子串
     * @param other 源字符串
     * @param position 起始位置
     * @param n 字符数
     * @return 自身引用
     */
    NEFORCE_CONSTEXPR20 basic_string& append(const basic_string& other, size_type position, size_type n) {
        NEFORCE_DEBUG_VERIFY(size() + n < max_size(), "basic_string append iterator out of ranges.");
        if (n == 0) {
            return *this;
        }
        n = _NEFORCE min(n, other.size() - position);
        return basic_string::append(other.data() + position, n);
    }

    /**
     * @brief 追加另一个字符串
     * @param other 源字符串
     * @return 自身引用
     */
    NEFORCE_CONSTEXPR20 basic_string& append(const basic_string& other) { return append(other, 0, other.size()); }

    /**
     * @brief 追加另一个字符串的子串
     * @param other 源字符串
     * @param position 起始位置
     * @return 自身引用
     */
    NEFORCE_CONSTEXPR20 basic_string& append(const basic_string& other, size_type position) {
        return append(other, position, other.size() - position);
    }

    /**
     * @brief 追加移动字符串的子串
     * @param other 源字符串
     * @param position 起始位置
     * @param n 字符数
     * @return 自身引用
     */
    NEFORCE_CONSTEXPR20 basic_string& append(basic_string&& other, size_type position, size_type n) {
        NEFORCE_DEBUG_VERIFY(size() + n < max_size(), "basic_string append iterator out of ranges.");
        if (n == 0) {
            return *this;
        }
        n = _NEFORCE min(n, other.size() - position);
        basic_string::append(other.data() + position, n);
        other.clear();
        return *this;
    }

    /**
     * @brief 追加移动字符串
     * @param other 源字符串
     * @return 自身引用
     */
    NEFORCE_CONSTEXPR20 basic_string& append(basic_string&& other) {
        const size_type len = other.size();
        return basic_string::append(_NEFORCE move(other), 0, len);
    }

    /**
     * @brief 追加移动字符串的子串
     * @param other 源字符串
     * @param position 起始位置
     * @return 自身引用
     */
    NEFORCE_CONSTEXPR20 basic_string& append(basic_string&& other, size_type position) {
        const size_type len = other.size();
        return basic_string::append(_NEFORCE move(other), position, len - position);
    }

    /**
     * @brief 追加字符串视图的指定长度
     * @param view 字符串视图
     * @param n 字符数
     * @return 自身引用
     */
    NEFORCE_CONSTEXPR20 basic_string& append(view_type view, size_type n) { return append(view.data(), n); }

    /**
     * @brief 追加字符串视图
     * @param view 字符串视图
     * @return 自身引用
     */
    NEFORCE_CONSTEXPR20 basic_string& append(view_type view) { return append(view.data(), view.size()); }

    /**
     * @brief 追加字符数组的指定长度
     * @param str 字符指针
     * @param n 字符数
     * @return 自身引用
     */
    NEFORCE_CONSTEXPR20 basic_string& append(const_pointer str, size_type n) {
        NEFORCE_DEBUG_VERIFY(size() + n < max_size(), "basic_string append iterator out of ranges.");
        if (n == 0) {
            return *this;
        }

#ifdef NEFORCE_USING_SSO
        const size_type old_size = size();
        if (!is_long() && old_size + n < sso_buffer_size) {
            traits_type::copy(storage_.short_ + old_size, str, n);
            size_pair_.value = old_size + n;
            traits_type::assign(storage_.short_ + size(), 1, value_type());
            return *this;
        }

        if (is_long() && storage_.long_.cap >= old_size + n + 1) {
            traits_type::copy(storage_.long_.ptr + old_size, str, n);
            size_pair_.value = (old_size + n) | long_flag;
            traits_type::assign(storage_.long_.ptr + size(), 1, value_type());
            return *this;
        }

        reallocate(n);
        traits_type::copy(data() + old_size, str, n);
        size_pair_.value = (old_size + n) | long_flag;
        traits_type::assign(data() + size(), 1, value_type());
#else
        if (capacity_pair_.value - size_ <= n) {
            reallocate(n);
        }
        traits_type::copy(data_ + size_, str, n);
        size_ += n;
        traits_type::assign(data_ + size_, 1, value_type());
#endif
        return *this;
    }

    /**
     * @brief 追加C风格字符串
     * @param str C风格字符串
     * @return 自身引用
     */
    NEFORCE_CONSTEXPR20 basic_string& append(const_pointer str) { return append(str, traits_type::length(str)); }

    /**
     * @brief 追加迭代器范围
     * @tparam Iterator 迭代器类型
     * @param first 源起始
     * @param last 源结束
     * @return 自身引用
     */
    template <typename Iterator, enable_if_t<is_iter_v<Iterator>, int> = 0>
    NEFORCE_CONSTEXPR20 basic_string& append(Iterator first, Iterator last) {
        const size_type n = _NEFORCE distance(first, last);
        NEFORCE_DEBUG_VERIFY(size() + n < max_size(), "basic_string append iterator out of ranges.");
        if (n == 0) {
            return *this;
        }

#ifdef NEFORCE_USING_SSO
        const size_type old_size = size();
        if (!is_long() && old_size + n < sso_buffer_size) {
            pointer p = storage_.short_ + old_size;
            for (size_type i = 0; i < n; ++i) {
                p[i] = *first++;
            }
            size_pair_.value = old_size + n;
            traits_type::assign(storage_.short_ + size(), 1, value_type());
            return *this;
        }

        if (is_long() && storage_.long_.cap >= old_size + n + 1) {
            pointer p = storage_.long_.ptr + old_size;
            for (size_type i = 0; i < n; ++i) {
                p[i] = *first++;
            }
            size_pair_.value = (old_size + n) | long_flag;
            traits_type::assign(storage_.long_.ptr + size(), 1, value_type());
            return *this;
        }

        reallocate(n);
        pointer p = data() + old_size;
        for (size_type i = 0; i < n; ++i) {
            p[i] = *first++;
        }
        size_pair_.value = (old_size + n) | long_flag;
        traits_type::assign(data() + size(), 1, value_type());
#else
        if (capacity_pair_.value - size_ <= n) {
            reallocate(n);
        }
        _NEFORCE uninitialized_copy_n(first, n, data_ + size_);
        size_ += n;
        traits_type::assign(data_ + size_, 1, value_type());
#endif
        return *this;
    }

    /**
     * @brief 追加初始化列表
     * @param ilist 初始化列表
     * @return 自身引用
     */
    NEFORCE_CONSTEXPR20 basic_string& append(std::initializer_list<value_type> ilist) {
        return append(ilist.begin(), ilist.end());
    }

    /// 追加另一个字符串
    NEFORCE_CONSTEXPR20 basic_string& operator+=(const basic_string& other) { return basic_string::append(other); }

    /// 追加移动字符串
    NEFORCE_CONSTEXPR20 basic_string& operator+=(basic_string&& other) {
        return basic_string::append(_NEFORCE move(other));
    }

    /// 追加单个字符
    NEFORCE_CONSTEXPR20 basic_string& operator+=(const value_type value) { return basic_string::append(value); }

    /// 追加C风格字符串
    NEFORCE_CONSTEXPR20 basic_string& operator+=(const_pointer str) { return basic_string::append(str); }

    /// 追加初始化列表
    NEFORCE_CONSTEXPR20 basic_string& operator+=(std::initializer_list<value_type> ilist) {
        return basic_string::append(ilist);
    }

    /// 追加字符串视图
    NEFORCE_CONSTEXPR20 basic_string& operator+=(view_type view) { return basic_string::append(view); }

    /**
     * @brief 赋值另一个字符串
     * @param other 源字符串
     * @return 自身引用
     */
    NEFORCE_CONSTEXPR20 basic_string& assign(const basic_string& other) { return *this = other; }

    /**
     * @brief 赋值移动字符串
     * @param other 源字符串
     * @return 自身引用
     */
    NEFORCE_CONSTEXPR20 basic_string& assign(basic_string&& other) { return *this = _NEFORCE move(other); }

    /**
     * @brief 赋值C风格字符串
     * @param str C风格字符串
     * @return 自身引用
     */
    NEFORCE_CONSTEXPR20 basic_string& assign(const_pointer str) { return *this = str; }

    /**
     * @brief 赋值字符数组的指定长度
     * @param str 字符指针
     * @param n 字符数
     * @return 自身引用
     */
    NEFORCE_CONSTEXPR20 basic_string& assign(const_pointer str, const size_type n) {
        clear();
        return append(str, n);
    }

    /**
     * @brief 赋值多个相同字符
     * @param n 字符数
     * @param value 填充字符
     * @return 自身引用
     */
    NEFORCE_CONSTEXPR20 basic_string& assign(const size_type n, value_type value) {
        clear();
        return append(n, value);
    }

    /**
     * @brief 赋值迭代器范围
     * @tparam Iterator 迭代器类型
     * @param first 源起始
     * @param last 源结束
     * @return 自身引用
     */
    template <typename Iterator>
    NEFORCE_CONSTEXPR20 basic_string& assign(Iterator first, Iterator last) {
        clear();
        return append(first, last);
    }

    /**
     * @brief 赋值初始化列表
     * @param ilist 初始化列表
     * @return 自身引用
     */
    NEFORCE_CONSTEXPR20 basic_string& assign(std::initializer_list<value_type> ilist) { return *this = ilist; }

    /**
     * @brief 赋值字符串视图
     * @param view 字符串视图
     * @return 自身引用
     */
    NEFORCE_CONSTEXPR20 basic_string& assign(const view_type& view) { return *this = view; }

    /**
     * @brief 删除指定位置的字符
     * @param position 要删除的位置
     * @return 指向被删除字符之后位置的迭代器
     */
    NEFORCE_CONSTEXPR20 iterator erase(iterator position) noexcept {
        NEFORCE_DEBUG_VERIFY(position != end(), "erase: cannot erase end() iterator");

#ifdef NEFORCE_USING_SSO
        const size_type offset = position - begin();
        pointer p = data() + offset;
        const size_type chars_after = size() - offset - 1;
        if (chars_after > 0) {
            traits_type::move(p, p + 1, chars_after);
        }
        if (is_long()) {
            size_pair_.value = (size() - 1) | long_flag;
            traits_type::assign(storage_.long_.ptr + size(), 1, value_type());
        } else {
            size_pair_.value = size() - 1;
            traits_type::assign(storage_.short_ + size(), 1, value_type());
        }
#else
        pointer ptr = &*position;
        const size_type chars_after = end() - position - 1;
        if (chars_after > 0) {
            traits_type::move(ptr, ptr + 1, chars_after);
        }

        --size_;
        traits_type::assign(data_ + size_, 1, value_type());
#endif
        return position;
    }

    /**
     * @brief 删除指定范围内的字符
     * @param position 起始位置
     * @param n 字符数
     * @return 自身引用
     */
    NEFORCE_CONSTEXPR20 basic_string& erase(size_type position = 0, size_type n = npos) noexcept {
        if (position >= size()) {
            return *this;
        }
        n = _NEFORCE min(n, size() - position);
        basic_string::erase(begin() + position, n);
        return *this;
    }

    /**
     * @brief 删除指定数量的字符
     * @param first 起始位置
     * @param n 字符数
     * @return 指向被删除区域之后位置的迭代器
     */
    NEFORCE_CONSTEXPR20 iterator erase(iterator first, const size_type n) noexcept {
        if (n == 0) {
            return first;
        }
        iterator last = first + _NEFORCE min(n, static_cast<size_type>(end() - first));
        return erase(first, last);
    }

    /**
     * @brief 删除迭代器范围
     * @param first 起始位置
     * @param last 结束位置
     * @return 指向被删除区域之后位置的迭代器
     */
    NEFORCE_CONSTEXPR20 iterator erase(iterator first, iterator last) noexcept {
        if (first == last) {
            return first;
        }

        const size_type erase_count = last - first;

#ifdef NEFORCE_USING_SSO
        const size_type offset = first - begin();
        pointer p = data() + offset;
        const size_type chars_after = end() - last;
        if (chars_after > 0) {
            traits_type::move(p, p + erase_count, chars_after);
        }
        if (is_long()) {
            size_pair_.value = (size() - erase_count) | long_flag;
            traits_type::assign(storage_.long_.ptr + size(), 1, value_type());
        } else {
            size_pair_.value = size() - erase_count;
            traits_type::assign(storage_.short_ + size(), 1, value_type());
        }
#else
        const size_type chars_after = end() - last;

        if (chars_after > 0) {
            pointer p_first = data_ + (first - begin());
            pointer p_last = data_ + (last - begin());
            traits_type::move(p_first, p_last, chars_after);
        }

        size_ -= erase_count;
        traits_type::assign(data_ + size_, 1, value_type());
#endif
        return first;
    }


    /**
     * @brief 调整大小
     * @param n 新大小
     * @param value 填充值
     */
    NEFORCE_CONSTEXPR20 void resize(size_type n, value_type value) {
        if (n < size()) {
            basic_string::erase(begin() + n, end());
        } else {
            basic_string::append(n - size(), value);
        }
    }

    /**
     * @brief 调整大小（默认填充0）
     * @param n 新大小
     */
    NEFORCE_CONSTEXPR20 void resize(const size_type n) { basic_string::resize(n, value_type()); }

    /**
     * @brief 清空字符串
     */
    NEFORCE_CONSTEXPR20 void clear() noexcept {
#ifdef NEFORCE_USING_SSO
        if (is_long()) {
            destroy_long();
            traits_type::assign(storage_.short_, 1, value_type());
            size_pair_.value = 0;
        } else {
            traits_type::assign(storage_.short_, 1, value_type());
            size_pair_.value = 0;
        }
#else
        size_ = 0;
        traits_type::assign(data_ + size_, 1, value_type());
#endif
    }

    /**
     * @brief 收缩容量以适应当前大小
     */
    NEFORCE_CONSTEXPR20 void shrink_to_fit() {
#ifdef NEFORCE_USING_SSO
        if (!is_long()) {
            return;
        }
        const size_type len = size();
        if (len < sso_capacity) {
            CharT tmp[sso_buffer_size];
            traits_type::copy(tmp, storage_.long_.ptr, len);
            traits_type::assign(tmp + len, 1, value_type());
            destroy_long();
            traits_type::copy(storage_.short_, tmp, len + 1);
            size_pair_.value = len;
        } else {
            if (storage_.long_.cap > len + 1) {
                pointer new_ptr = size_pair_.get_base().allocate(len + 1);
                traits_type::move(new_ptr, storage_.long_.ptr, len);
                traits_type::assign(new_ptr + len, 1, value_type());
                destroy_long();
                storage_.long_.ptr = new_ptr;
                storage_.long_.cap = len + 1;
                size_pair_.value = len | long_flag;
            }
        }
#else
        const size_type new_cap = size_ + 1;
        if (new_cap >= capacity_pair_.value) {
            return;
        }

        basic_string temp;
        temp.reserve(new_cap);
        temp.append(*this);
        basic_string::swap(temp);
#endif
    }

    /**
     * @brief 重复当前字符串n次
     * @param n 重复次数
     * @return 新字符串
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 basic_string repeat(size_type n) const noexcept {
        basic_string result;
        result.reserve(size() * n);
        while (n--) {
            result += *this;
        }
        return _NEFORCE move(result);
    }

    /**
     * @brief 获取子串
     * @param off 起始偏移
     * @param count 长度
     * @return 子串
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 basic_string substr(const size_type off = 0, size_type count = npos) const {
        NEFORCE_DEBUG_VERIFY(off <= size(), "basic_string index out of ranges.");
        count = _NEFORCE min(count, size() - off);
        return basic_string(data() + off, count);
    }

    /**
     * @brief 获取字符串视图
     * @return 字符串视图
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 view_type view() const noexcept { return view_type(data(), size()); }

    /**
     * @brief 获取子串视图
     * @param off 起始偏移
     * @param count 长度
     * @return 字符串视图
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 view_type view(const size_type off, size_type count = npos) const noexcept {
        NEFORCE_DEBUG_VERIFY(off <= size(), "basic_string index out of ranges.");
        count = _NEFORCE min(count, size() - off);
        return view_type(data() + off, count);
    }

    /**
     * @brief 复制字符到目标缓冲区
     * @param dest 目标缓冲区
     * @param count 要复制的字符数
     * @param position 起始位置
     * @return 实际复制的字符数
     */
    NEFORCE_CONSTEXPR20 size_type copy(pointer dest, const size_type count, size_type position = 0) const {
        NEFORCE_DEBUG_VERIFY(position <= size(), "basic_string copy position out of range");
        const size_type len = _NEFORCE min(count, size() - position);
        traits_type::copy(dest, data() + position, len);
        return len;
    }

    /**
     * @brief 比较另一个字符串
     * @param other 另一个字符串
     * @return 比较结果
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 int compare(const basic_string& other) const noexcept {
        return compare(other.view());
    }

    /**
     * @brief 比较子串与另一个字符串
     * @param off 起始偏移
     * @param n 长度
     * @param other 另一个字符串
     * @return 比较结果
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 int compare(const size_type off, const size_type n,
                                                      const basic_string& other) const {
        return view(off, n).compare(other.view());
    }

    /**
     * @brief 比较子串与另一个字符串的子串
     * @param off 起始偏移
     * @param n 长度
     * @param other 另一个字符串
     * @param roff 目标起始偏移
     * @param count 目标长度
     * @return 比较结果
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 int compare(const size_type off, const size_type n, const basic_string& other,
                                                      const size_type roff, const size_type count) const {
        return view(off, n).compare(other.view(roff, count));
    }

    /**
     * @brief 比较C风格字符串
     * @param str C风格字符串
     * @return 比较结果
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 int compare(const CharT* str) const noexcept {
        return compare(view_type(str));
    }

    /**
     * @brief 比较字符串视图
     * @param view 字符串视图
     * @return 比较结果
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 int compare(const view_type& view) const noexcept {
        return char_traits_compare<traits_type>(data(), size(), view.data(), view.size());
    }

    /**
     * @brief 比较子串与C风格字符串
     * @param off 起始偏移
     * @param n 长度
     * @param str C风格字符串
     * @return 比较结果
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 int compare(const size_type off, const size_type n, const CharT* str) const {
        return view(off, n).compare(view_type(str));
    }

    /**
     * @brief 比较子串与指定长度的字符数组
     * @param off 起始偏移
     * @param n 长度
     * @param str 字符指针
     * @param count 字符数组长度
     * @return 比较结果
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 int compare(const size_type off, const size_type n, const CharT* str,
                                                      size_type count) const {
        return view(off, n).compare(view_type(str, count));
    }

    /// 替换子串为另一个字符串
    NEFORCE_CONSTEXPR20 basic_string& replace(const size_type position, const size_type n, const basic_string& other) {
        NEFORCE_DEBUG_VERIFY(position < size(), "basic_string index out of ranges.");
        return replace_copy(begin() + position, n, other.data(), other.size());
    }

    /// 替换迭代器范围为另一个字符串
    NEFORCE_CONSTEXPR20 basic_string& replace(iterator first, iterator last, const basic_string& other) {
        NEFORCE_DEBUG_VERIFY(begin() <= first && last <= end() && first <= last,
                             "basic_string replace iterator out of ranges.");
        return replace_copy(first, last - first, other.data(), other.size());
    }

    /// 替换子串为C风格字符串
    NEFORCE_CONSTEXPR20 basic_string& replace(const size_type position, const size_type n, const_pointer str) {
        NEFORCE_DEBUG_VERIFY(position < size(), "basic_string index out of ranges.");
        return replace_copy({data() + position, this}, n, str, traits_type::length(str));
    }

    /// 替换迭代器范围为C风格字符串
    NEFORCE_CONSTEXPR20 basic_string& replace(iterator first, iterator last, const_pointer str) {
        NEFORCE_DEBUG_VERIFY(begin() <= first && last <= end() && first <= last,
                             "basic_string replace iterator out of ranges.");
        return replace_copy(first, last - first, str, traits_type::length(str));
    }

    /// 替换子串为指定长度的字符数组
    NEFORCE_CONSTEXPR20 basic_string& replace(const size_type position, const size_type n1, const_pointer str,
                                              const size_type n2) {
        NEFORCE_DEBUG_VERIFY(position < size(), "basic_string index out of ranges.");
        return replace_copy({data() + position, this}, n1, str, n2);
    }

    /// 替换迭代器范围为指定长度的字符数组
    NEFORCE_CONSTEXPR20 basic_string& replace(iterator first, iterator last, const_pointer str, const size_type n) {
        NEFORCE_DEBUG_VERIFY(begin() <= first && last <= end() && first <= last,
                             "basic_string replace iterator out of ranges.");
        return replace_copy(first, last - first, str, n);
    }

    /// 替换子串为多个相同字符
    NEFORCE_CONSTEXPR20 basic_string& replace(const size_type position, const size_type n1, const size_type n2,
                                              const value_type value) {
        NEFORCE_DEBUG_VERIFY(position < size(), "basic_string index out of ranges.");
        return replace_fill({data() + position, this}, n1, n2, value);
    }

    /// 替换迭代器范围为多个相同字符
    NEFORCE_CONSTEXPR20 basic_string& replace(iterator first, iterator last, const size_type n,
                                              const value_type value) {
        NEFORCE_DEBUG_VERIFY(begin() <= first && last <= end() && first <= last,
                             "basic_string replace iterator out of ranges.");
        return replace_fill(first, static_cast<size_type>(last - first), n, value);
    }

    /// 替换子串为另一个字符串的子串
    NEFORCE_CONSTEXPR20 basic_string& replace(const size_type position1, const size_type n1, const basic_string& str,
                                              const size_type position2, const size_type n2 = npos) {
        NEFORCE_DEBUG_VERIFY(position1 < size(), "basic_string index out of ranges.");
        NEFORCE_DEBUG_VERIFY(position2 < size(), "basic_string index out of ranges.");
        return replace_copy({data() + position1, this}, n1, str.data() + position2, n2);
    }

    /// 替换迭代器范围为另一个迭代器范围
    template <typename Iterator>
    NEFORCE_CONSTEXPR20 basic_string& replace(iterator first, iterator last, Iterator first2, Iterator last2) {
        NEFORCE_DEBUG_VERIFY(begin() <= first && last <= end() && first <= last,
                             "basic_string replace iterator out of ranges.");
        return replace_copy(first, last, first2, last2);
    }

    /**
     * @brief 反转字符串
     */
    NEFORCE_CONSTEXPR20 void reverse() noexcept {
        if (size() < 2) {
            return;
        }

        for (iterator first = begin(), last = end(); first < last;) {
            _NEFORCE iter_swap(first++, --last);
        }
    }

    /// 查找子串
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type find(const basic_string& other,
                                                         const size_type n = 0) const noexcept {
        return (char_traits_find<Traits>) (data(), size(), n, other.data(), other.size());
    }

    /// 查找字符
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type find(const CharT value, const size_type n = 0) const noexcept {
        return (char_traits_find_char<Traits>) (data(), size(), n, value);
    }

    /// 查找指定长度的子串
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type find(const CharT* str, const size_type off,
                                                         const size_type count) const noexcept {
        return (char_traits_find<Traits>) (data(), size(), off, str, count);
    }

    /// 查找C风格字符串
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type find(const CharT* str, const size_type off = 0) const noexcept {
        return (char_traits_find<Traits>) (data(), size(), off, str, Traits::length(str));
    }

    /// 查找指定长度的字符串视图
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type find(const view_type& view, const size_type off,
                                                         const size_type count) const noexcept {
        return (char_traits_find<Traits>) (data(), size(), off, view.data(), count);
    }

    /// 查找字符串视图
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type find(const view_type& view,
                                                         const size_type off = 0) const noexcept {
        return _NEFORCE char_traits_find<Traits>(data(), size(), off, view.data(), view.size());
    }

    /// 从后向前查找子串
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type rfind(const basic_string& other,
                                                          const size_type off = npos) const noexcept {
        return (char_traits_rfind<Traits>) (data(), size(), off, other.data(), other.size());
    }

    /// 从后向前查找字符
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type rfind(const CharT value, const size_type n = npos) const noexcept {
        return (char_traits_rfind_char<Traits>) (data(), size(), n, value);
    }

    /// 从后向前查找指定长度的子串
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type rfind(const CharT* str, const size_type off,
                                                          const size_type n) const noexcept {
        return (char_traits_rfind<Traits>) (data(), size(), off, str, n);
    }

    /// 从后向前查找C风格字符串
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type rfind(const CharT* str, const size_type off = npos) const noexcept {
        return (char_traits_rfind<Traits>) (data(), size(), off, str, Traits::length(str));
    }

    /// 从后向前查找指定长度的字符串视图
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type rfind(const view_type& view, const size_type off,
                                                          const size_type count) const noexcept {
        return (char_traits_rfind<Traits>) (data(), size(), off, view.data(), count);
    }

    /// 从后向前查找字符串视图
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type rfind(const view_type& view,
                                                          const size_type off = 0) const noexcept {
        return (char_traits_rfind<Traits>) (data(), size(), off, view.data(), view.size());
    }

    /// 查找第一个出现在字符集合中的字符
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type find_first_of(const basic_string& other,
                                                                  const size_type off = 0) const noexcept {
        return (char_traits_find_first_of<Traits>) (data(), size(), off, other.data(), other.size());
    }

    /// 查找第一个等于指定字符的位置
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type find_first_of(const CharT value,
                                                                  const size_type off = 0) const noexcept {
        return (char_traits_find_char<Traits>) (data(), size(), off, value);
    }

    /// 查找第一个出现在指定字符数组中的字符
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type find_first_of(const CharT* str, const size_type off,
                                                                  const size_type n) const noexcept {
        return (char_traits_find_first_of<Traits>) (data(), size(), off, str, n);
    }

    /// 查找第一个出现在C风格字符串中的字符
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type find_first_of(const CharT* str,
                                                                  const size_type off = 0) const noexcept {
        return (char_traits_find_first_of<Traits>) (data(), size(), off, str, Traits::length(str));
    }

    /// 查找第一个出现在字符串视图中的字符
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type find_first_of(const view_type& view, const size_type off,
                                                                  const size_type n) const noexcept {
        return (char_traits_find_first_of<Traits>) (data(), size(), off, view.data(), n);
    }

    /// 查找第一个出现在字符串视图中的字符
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type find_first_of(const view_type& view,
                                                                  const size_type off = 0) const noexcept {
        return (char_traits_find_first_of<Traits>) (data(), size(), off, view.data(), view.size());
    }

    /// 查找最后一个出现在字符集合中的字符
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type find_last_of(const basic_string& other,
                                                                 const size_type off = npos) const noexcept {
        return (char_traits_find_last_of<Traits>) (data(), size(), off, other.data(), other.size());
    }

    /// 查找最后一个等于指定字符的位置
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type find_last_of(const CharT value,
                                                                 const size_type off = npos) const noexcept {
        return (char_traits_rfind_char<Traits>) (data(), size(), off, value);
    }

    /// 查找最后一个出现在指定字符数组中的字符
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type find_last_of(const CharT* str, const size_type off,
                                                                 const size_type n) const noexcept {
        return (char_traits_find_last_of<Traits>) (data(), size(), off, str, n);
    }

    /// 查找最后一个出现在C风格字符串中的字符
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type find_last_of(const CharT* str,
                                                                 const size_type off = npos) const noexcept {
        return (char_traits_find_last_of<Traits>) (data(), size(), off, str, Traits::length(str));
    }

    /// 查找最后一个出现在字符串视图中的字符
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type find_last_of(const view_type& view, const size_type off,
                                                                 const size_type n) const noexcept {
        return (char_traits_find_last_of<Traits>) (data(), size(), off, view.data(), n);
    }

    /// 查找最后一个出现在字符串视图中的字符
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type find_last_of(const view_type& view,
                                                                 const size_type off = npos) const noexcept {
        return (char_traits_find_last_of<Traits>) (data(), size(), off, view.data(), view.size());
    }

    /// 查找第一个不在字符集合中的字符
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type find_first_not_of(const basic_string& other,
                                                                      const size_type off = 0) const noexcept {
        return (char_traits_find_first_not_of<Traits>) (data(), size(), off, other.data(), other.size());
    }

    /// 查找第一个不等于指定字符的位置
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type find_first_not_of(const CharT value,
                                                                      const size_type off = 0) const noexcept {
        return (char_traits_find_not_char<Traits>) (data(), size(), off, value);
    }

    /// 查找第一个不在指定字符数组中的字符
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type find_first_not_of(const CharT* str, const size_type off,
                                                                      const size_type n) const noexcept {
        return (char_traits_find_first_not_of<Traits>) (data(), size(), off, str, n);
    }

    /// 查找第一个不在C风格字符串中的字符
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type find_first_not_of(const CharT* str,
                                                                      const size_type off = 0) const noexcept {
        return (char_traits_find_first_not_of<Traits>) (data(), size(), off, str, Traits::length(str));
    }

    /// 查找第一个不在字符串视图中的字符
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type find_first_not_of(const view_type& view, const size_type off,
                                                                      const size_type n) const noexcept {
        return (char_traits_find_first_not_of<Traits>) (data(), size(), off, view.data(), n);
    }

    /// 查找第一个不在字符串视图中的字符
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type find_first_not_of(const view_type& view,
                                                                      const size_type off = 0) const noexcept {
        return (char_traits_find_first_not_of<Traits>) (data(), size(), off, view.data(), view.size());
    }

    /// 查找最后一个不在字符集合中的字符
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type find_last_not_of(const basic_string& other,
                                                                     const size_type off = npos) const noexcept {
        return (char_traits_find_last_not_of<Traits>) (data(), size(), off, other.data(), other.size());
    }

    /// 查找最后一个不等于指定字符的位置
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type find_last_not_of(const CharT value,
                                                                     const size_type off = npos) const noexcept {
        return (char_traits_rfind_not_char<Traits>) (data(), size(), off, value);
    }

    /// 查找最后一个不在指定字符数组中的字符
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type find_last_not_of(const CharT* str, const size_type off,
                                                                     const size_type n) const noexcept {
        return (char_traits_find_last_not_of<Traits>) (data(), size(), off, str, n);
    }

    /// 查找最后一个不在C风格字符串中的字符
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type find_last_not_of(const CharT* str,
                                                                     const size_type off = npos) const noexcept {
        return (char_traits_find_last_not_of<Traits>) (data(), size(), off, str, Traits::length(str));
    }

    /// 查找最后一个不在字符串视图中的字符
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type find_last_not_of(const view_type& view, const size_type off,
                                                                     const size_type n) const noexcept {
        return (char_traits_find_last_not_of<Traits>) (data(), size(), off, view.data(), n);
    }

    /// 查找最后一个不在字符串视图中的字符
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type find_last_not_of(const view_type& view,
                                                                     const size_type off = npos) const noexcept {
        return (char_traits_find_last_not_of<Traits>) (data(), size(), off, view.data(), view.size());
    }

    /**
     * @brief 统计指定字符出现的次数
     * @param value 要统计的字符
     * @param position 起始位置
     * @return 字符出现的次数
     */
    NEFORCE_CONSTEXPR20 size_type count(value_type value, const size_type position = 0) const noexcept {
        size_type n = 0;
        for (size_type idx = position; idx < size(); ++idx) {
            if (*(data() + idx) == value) {
                ++n;
            }
        }
        return n;
    }

    /// 检查是否以另一个字符串开头
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool starts_with(const basic_string& other) const noexcept {
        return other.size() <= size() && traits_type::compare(data(), other.data(), other.size()) == 0;
    }

    /// 检查是否以字符串视图开头
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool starts_with(view_type view) const noexcept {
        return view.size() <= size() && traits_type::compare(data(), view.data(), view.size()) == 0;
    }

    /// 检查是否以指定字符开头
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool starts_with(const value_type value) const noexcept {
        return !empty() && traits_type::eq(front(), value);
    }

    /// 检查是否以C风格字符串开头
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool starts_with(const_pointer str) const noexcept {
        return starts_with(view_type(str));
    }

    /// 检查是否以另一个字符串结尾
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool ends_with(const basic_string& other) const noexcept {
        const size_type other_size = other.size();
        return other_size <= size() &&
               traits_type::compare(data() + size() - other_size, other.data(), other_size) == 0;
    }

    /// 检查是否以字符串视图结尾
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool ends_with(view_type view) const noexcept {
        const size_type view_size = view.size();
        return view_size <= size() && traits_type::compare(data() + size() - view_size, view.data(), view_size) == 0;
    }

    /// 检查是否以指定字符结尾
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool ends_with(value_type value) const noexcept {
        return !empty() && traits_type::eq(back(), value);
    }

    /// 检查是否以C风格字符串结尾
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool ends_with(const_pointer str) const noexcept {
        return ends_with(view_type(str));
    }

    /// 检查是否包含另一个字符串
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool contains(const basic_string& other) const noexcept {
        return find(other) != npos;
    }

    /// 检查是否包含字符串视图
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool contains(view_type view) const noexcept { return find(view) != npos; }

    /// 检查是否包含指定字符
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool contains(value_type value) const noexcept { return find(value) != npos; }

    /// 检查是否包含C风格字符串
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool contains(const_pointer str) const noexcept { return find(str) != npos; }

    /**
     * @brief 去除左侧空白字符
     * @return 自身引用
     */
    NEFORCE_CONSTEXPR20 basic_string& trim_left() noexcept {
        return trim_left_if([](value_type value) { return _NEFORCE is_space(value); });
    }

    /**
     * @brief 去除右侧空白字符
     * @return 自身引用
     */
    NEFORCE_CONSTEXPR20 basic_string& trim_right() noexcept {
        return trim_right_if([](value_type value) { return _NEFORCE is_space(value); });
    }

    /**
     * @brief 去除两侧空白字符
     * @return 自身引用
     */
    NEFORCE_CONSTEXPR20 basic_string& trim() noexcept { return trim_left().trim_right(); }

    /**
     * @brief 根据谓词去除左侧字符
     * @tparam Pred 谓词类型
     * @param pred 谓词函数
     * @return 自身引用
     */
    template <typename Pred>
    NEFORCE_CONSTEXPR20 basic_string& trim_left_if(Pred pred) {
        if (empty()) {
            return *this;
        }

        iterator it = begin();
        while (it != end() && pred(*it)) {
            ++it;
        }
        if (it != begin()) {
            basic_string::erase(begin(), it - begin());
        }

        return *this;
    }

    /**
     * @brief 根据谓词去除右侧字符
     * @tparam Pred 谓词类型
     * @param pred 谓词函数
     * @return 自身引用
     */
    template <typename Pred>
    NEFORCE_CONSTEXPR20 basic_string& trim_right_if(Pred pred) {
        if (empty()) {
            return *this;
        }

        reverse_iterator rit = rbegin();
        while (rit != rend() && pred(*rit)) {
            ++rit;
        }
        if (rit != rbegin()) {
            basic_string::erase(end() - (rit - rbegin()), end());
        }

        return *this;
    }

    /**
     * @brief 根据谓词去除两侧字符
     * @tparam Predicate 谓词类型
     * @param pred 谓词函数
     * @return 自身引用
     */
    template <typename Predicate>
    NEFORCE_CONSTEXPR20 basic_string& trim_if(Predicate pred) {
        return trim_left_if(pred).trim_right_if(pred);
    }

    /**
     * @brief 相等比较
     * @param other 另一个字符串
     * @return 是否相等
     */
    NEFORCE_CONSTEXPR20 bool equal_to(const basic_string& other) const noexcept { return equal_to(other.view()); }

    /**
     * @brief 与字符串视图相等比较
     * @param view 字符串视图
     * @return 是否相等
     */
    NEFORCE_CONSTEXPR20 bool equal_to(const view_type view) const noexcept {
        return _NEFORCE char_traits_equal<Traits>(data(), size(), view.data(), view.size());
    }

    /**
     * @brief 与C风格字符串相等比较
     * @param str C风格字符串
     * @return 是否相等
     */
    NEFORCE_CONSTEXPR20 bool equal_to(const CharT* str) const noexcept { return equal_to(view_type(str)); }

    /**
     * @brief 转换为小写
     */
    NEFORCE_CONSTEXPR20 basic_string&
    lowercase() noexcept(noexcept(_NEFORCE transform(begin(), end(), begin(), _NEFORCE to_lowercase<CharT>))) {
        _NEFORCE transform(begin(), end(), begin(), _NEFORCE to_lowercase<CharT>);
        return *this;
    }

    /**
     * @brief 转换为大写
     */
    NEFORCE_CONSTEXPR20 basic_string&
    uppercase() noexcept(noexcept(_NEFORCE transform(begin(), end(), begin(), _NEFORCE to_uppercase<CharT>))) {
        _NEFORCE transform(begin(), end(), begin(), _NEFORCE to_uppercase<CharT>);
        return *this;
    }

    /**
     * @brief 交换两个字符串
     * @param other 另一个字符串
     */
    NEFORCE_CONSTEXPR20 void swap(basic_string& other) noexcept {
        if (_NEFORCE addressof(other) == this) {
            return;
        }
#ifdef NEFORCE_USING_SSO
        _NEFORCE swap(storage_, other.storage_);
        _NEFORCE swap(size_pair_, other.size_pair_);
#else
        _NEFORCE swap(data_, other.data_);
        _NEFORCE swap(size_, other.size_);
        _NEFORCE swap(capacity_pair_, other.capacity_pair_);
#endif
    }

    /// 相等比较操作符
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool operator==(const basic_string& rhs) const noexcept {
        return equal_to(rhs);
    }

    /// 小于比较操作符
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool operator<(const basic_string& rhs) const noexcept {
        return compare(rhs) < 0;
    }

    /// 计算哈希值
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_t to_hash() const noexcept {
        return _NEFORCE FNV_hash_string(data(), size());
    }
};

#ifdef NEFORCE_STANDARD_17
template <typename Iterator, typename Alloc = allocator<iter_value_t<Iterator>>>
basic_string(Iterator, Iterator, Alloc = Alloc())
        -> basic_string<iter_value_t<Iterator>, char_traits<iter_value_t<Iterator>>, Alloc>;

template <typename CharT, typename Traits, typename Alloc = allocator<CharT>>
explicit basic_string(basic_string_view<CharT, Traits>, const Alloc& = Alloc()) -> basic_string<CharT, Traits, Alloc>;

template <typename CharT, typename Traits, typename Alloc = allocator<CharT>>
basic_string(basic_string_view<CharT, Traits>, typename allocator_traits<Alloc>::size_type,
             typename allocator_traits<Alloc>::size_type, const Alloc& = Alloc()) -> basic_string<CharT, Traits, Alloc>;
#endif

template <typename CharT, typename Traits, typename Alloc>
NEFORCE_CONSTEXPR20 basic_string<CharT, Traits, Alloc> operator+(const basic_string<CharT, Traits, Alloc>& lhs,
                                                                 const basic_string<CharT, Traits, Alloc>& rhs) {
    basic_string<CharT, Traits, Alloc> tmp(lhs);
    tmp.append(rhs);
    return _NEFORCE move(tmp);
}

template <typename CharT, typename Traits, typename Alloc>
NEFORCE_CONSTEXPR20 basic_string<CharT, Traits, Alloc> operator+(const CharT* lhs,
                                                                 const basic_string<CharT, Traits, Alloc>& rhs) {
    basic_string<CharT, Traits, Alloc> tmp(lhs);
    tmp.append(rhs);
    return _NEFORCE move(tmp);
}
template <typename CharT, typename Traits, typename Alloc>
NEFORCE_CONSTEXPR20 basic_string<CharT, Traits, Alloc> operator+(const basic_string<CharT, Traits, Alloc>& lhs,
                                                                 const CharT* rhs) {
    basic_string<CharT, Traits, Alloc> tmp(lhs);
    tmp.append(rhs);
    return _NEFORCE move(tmp);
}

template <typename CharT, typename Traits, typename Alloc>
NEFORCE_CONSTEXPR20 basic_string<CharT, Traits, Alloc> operator+(const basic_string_view<CharT, Traits>& lhs,
                                                                 const basic_string<CharT, Traits, Alloc>& rhs) {
    basic_string<CharT, Traits, Alloc> tmp(lhs);
    tmp.append(rhs);
    return _NEFORCE move(tmp);
}
template <typename CharT, typename Traits, typename Alloc>
NEFORCE_CONSTEXPR20 basic_string<CharT, Traits, Alloc> operator+(const basic_string<CharT, Traits, Alloc>& lhs,
                                                                 const basic_string_view<CharT, Traits>& rhs) {
    basic_string<CharT, Traits, Alloc> tmp(lhs);
    tmp.append(rhs);
    return _NEFORCE move(tmp);
}

template <typename CharT, typename Traits, typename Alloc>
NEFORCE_CONSTEXPR20 basic_string<CharT, Traits, Alloc> operator+(CharT lhs,
                                                                 const basic_string<CharT, Traits, Alloc>& rhs) {
    basic_string<CharT, Traits, Alloc> tmp(1, lhs);
    tmp.append(rhs);
    return _NEFORCE move(tmp);
}
template <typename CharT, typename Traits, typename Alloc>
NEFORCE_CONSTEXPR20 basic_string<CharT, Traits, Alloc> operator+(const basic_string<CharT, Traits, Alloc>& lhs,
                                                                 CharT rhs) {
    basic_string<CharT, Traits, Alloc> tmp(lhs);
    tmp.append(1, rhs);
    return _NEFORCE move(tmp);
}

template <typename CharT, typename Traits, typename Alloc>
NEFORCE_CONSTEXPR20 basic_string<CharT, Traits, Alloc> operator+(basic_string<CharT, Traits, Alloc>&& lhs,
                                                                 const basic_string<CharT, Traits, Alloc>& rhs) {
    return _NEFORCE move(lhs.append(rhs));
}
template <typename CharT, typename Traits, typename Alloc>
NEFORCE_CONSTEXPR20 basic_string<CharT, Traits, Alloc> operator+(const basic_string<CharT, Traits, Alloc>& lhs,
                                                                 basic_string<CharT, Traits, Alloc>&& rhs) {
    basic_string<CharT, Traits, Alloc> tmp(lhs);
    tmp.append(_NEFORCE move(rhs));
    return _NEFORCE move(tmp);
}

template <typename CharT, typename Traits, typename Alloc>
NEFORCE_CONSTEXPR20 basic_string<CharT, Traits, Alloc> operator+(basic_string<CharT, Traits, Alloc>&& lhs,
                                                                 basic_string<CharT, Traits, Alloc>&& rhs) {
    basic_string<CharT, Traits, Alloc> tmp(_NEFORCE move(lhs));
    if (_NEFORCE addressof(lhs) != _NEFORCE addressof(rhs)) {
        tmp.append(_NEFORCE move(rhs));
    } else {
        tmp.append(lhs);
    }
    return _NEFORCE move(tmp);
}

template <typename CharT, typename Traits, typename Alloc>
NEFORCE_CONSTEXPR20 basic_string<CharT, Traits, Alloc> operator+(const CharT* lhs,
                                                                 basic_string<CharT, Traits, Alloc>&& rhs) {
    basic_string<CharT, Traits, Alloc> tmp(lhs);
    tmp.append(_NEFORCE move(rhs));
    return _NEFORCE move(tmp);
}
template <typename CharT, typename Traits, typename Alloc>
NEFORCE_CONSTEXPR20 basic_string<CharT, Traits, Alloc> operator+(basic_string<CharT, Traits, Alloc>&& lhs,
                                                                 const CharT* rhs) {
    return _NEFORCE move(lhs.append(rhs));
}

template <typename CharT, typename Traits, typename Alloc>
NEFORCE_CONSTEXPR20 basic_string<CharT, Traits, Alloc> operator+(CharT lhs, basic_string<CharT, Traits, Alloc>&& rhs) {
    basic_string<CharT, Traits, Alloc> tmp(1, lhs);
    tmp.append(_NEFORCE move(rhs));
    return _NEFORCE move(tmp);
}
template <typename CharT, typename Traits, typename Alloc>
NEFORCE_CONSTEXPR20 basic_string<CharT, Traits, Alloc> operator+(basic_string<CharT, Traits, Alloc>&& lhs, CharT rhs) {
    return _NEFORCE move(lhs.append(rhs));
}

template <typename CharT, typename Traits, typename Alloc>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool operator==(const CharT* const lhs,
                                                      const basic_string<CharT, Traits, Alloc>& rhs) noexcept {
    return rhs.equal_to(lhs);
}
template <typename CharT, typename Traits, typename Alloc>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool operator==(const basic_string<CharT, Traits, Alloc>& lhs,
                                                      const CharT* const rhs) noexcept {
    return lhs.equal_to(rhs);
}
template <typename CharT, typename Traits, typename Alloc>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool operator==(const basic_string_view<CharT, Traits>& lhs,
                                                      const basic_string<CharT, Traits, Alloc>& rhs) noexcept {
    return rhs.equal_to(lhs);
}
template <typename CharT, typename Traits, typename Alloc>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool operator==(const basic_string<CharT, Traits, Alloc>& lhs,
                                                      const basic_string_view<CharT, Traits>& rhs) noexcept {
    return lhs.equal_to(rhs);
}

template <typename CharT, typename Traits, typename Alloc>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool operator!=(const CharT* const lhs,
                                                      const basic_string<CharT, Traits, Alloc>& rhs) noexcept {
    return !(lhs == rhs);
}
template <typename CharT, typename Traits, typename Alloc>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool operator!=(const basic_string<CharT, Traits, Alloc>& lhs,
                                                      const CharT* const rhs) noexcept {
    return !(lhs == rhs);
}
template <typename CharT, typename Traits, typename Alloc>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool operator!=(const basic_string_view<CharT, Traits>& lhs,
                                                      const basic_string<CharT, Traits, Alloc>& rhs) noexcept {
    return !(lhs == rhs);
}
template <typename CharT, typename Traits, typename Alloc>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool operator!=(const basic_string<CharT, Traits, Alloc>& lhs,
                                                      const basic_string_view<CharT, Traits>& rhs) noexcept {
    return !(lhs == rhs);
}

template <typename CharT, typename Traits, typename Alloc>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool operator<(const CharT* const lhs,
                                                     const basic_string<CharT, Traits, Alloc>& rhs) noexcept {
    return 0 < rhs.compare(lhs);
}
template <typename CharT, typename Traits, typename Alloc>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool operator<(const basic_string<CharT, Traits, Alloc>& lhs,
                                                     const CharT* const rhs) noexcept {
    return lhs.compare(rhs) < 0;
}
template <typename CharT, typename Traits, typename Alloc>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool operator<(const basic_string_view<CharT, Traits>& lhs,
                                                     const basic_string<CharT, Traits, Alloc>& rhs) noexcept {
    return 0 < rhs.compare(lhs);
}
template <typename CharT, typename Traits, typename Alloc>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool operator<(const basic_string<CharT, Traits, Alloc>& lhs,
                                                     const basic_string_view<CharT, Traits>& rhs) noexcept {
    return lhs.compare(rhs) < 0;
}

template <typename CharT, typename Traits, typename Alloc>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool operator>(const CharT* const lhs,
                                                     const basic_string<CharT, Traits, Alloc>& rhs) noexcept {
    return rhs < lhs;
}
template <typename CharT, typename Traits, typename Alloc>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool operator>(const basic_string<CharT, Traits, Alloc>& lhs,
                                                     const CharT* const rhs) noexcept {
    return rhs < lhs;
}
template <typename CharT, typename Traits, typename Alloc>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool operator>(const basic_string_view<CharT, Traits>& lhs,
                                                     const basic_string<CharT, Traits, Alloc>& rhs) noexcept {
    return rhs < lhs;
}
template <typename CharT, typename Traits, typename Alloc>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool operator>(const basic_string<CharT, Traits, Alloc>& lhs,
                                                     const basic_string_view<CharT, Traits>& rhs) noexcept {
    return rhs < lhs;
}

template <typename CharT, typename Traits, typename Alloc>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool operator<=(const CharT* const lhs,
                                                      const basic_string<CharT, Traits, Alloc>& rhs) noexcept {
    return !(lhs > rhs);
}
template <typename CharT, typename Traits, typename Alloc>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool operator<=(const basic_string<CharT, Traits, Alloc>& lhs,
                                                      const CharT* const rhs) noexcept {
    return !(lhs > rhs);
}
template <typename CharT, typename Traits, typename Alloc>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool operator<=(const basic_string_view<CharT, Traits>& lhs,
                                                      const basic_string<CharT, Traits, Alloc>& rhs) noexcept {
    return !(lhs > rhs);
}
template <typename CharT, typename Traits, typename Alloc>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool operator<=(const basic_string<CharT, Traits, Alloc>& lhs,
                                                      const basic_string_view<CharT, Traits>& rhs) noexcept {
    return !(lhs > rhs);
}

template <typename CharT, typename Traits, typename Alloc>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool operator>=(const CharT* const lhs,
                                                      const basic_string<CharT, Traits, Alloc>& rhs) noexcept {
    return !(rhs < lhs);
}
template <typename CharT, typename Traits, typename Alloc>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool operator>=(const basic_string<CharT, Traits, Alloc>& lhs,
                                                      const CharT* const rhs) noexcept {
    return !(rhs < lhs);
}
template <typename CharT, typename Traits, typename Alloc>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool operator>=(const basic_string_view<CharT, Traits>& lhs,
                                                      const basic_string<CharT, Traits, Alloc>& rhs) noexcept {
    return !(rhs < lhs);
}
template <typename CharT, typename Traits, typename Alloc>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool operator>=(const basic_string<CharT, Traits, Alloc>& lhs,
                                                      const basic_string_view<CharT, Traits>& rhs) noexcept {
    return !(rhs < lhs);
}


extern template class basic_string<char>;
extern template class basic_string<wchar_t>;
#ifdef NEFORCE_STANDARD_20
extern template class basic_string<char8_t>;
#endif
extern template class basic_string<char16_t>;
extern template class basic_string<char32_t>;

/** @} */ // String

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_STRING_BASIC_STRING_HPP__
