#ifndef MSTL_CORE_CONTAINER_BITMAP_HPP__
#define MSTL_CORE_CONTAINER_BITMAP_HPP__

/**
 * @file bitmap.hpp
 * @brief MSTL位图容器
 *
 * 此文件提供了位图容器的实现。
 * 位图是一种高效的紧凑型数据结构，使用单个位来存储布尔值，
 * 大幅节省内存空间。支持随机访问和标准容器操作。
 */

#include "MSTL/core/interface/iiterator.hpp"
#include "MSTL/core/interface/istringify.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup Bitmap 位图
 * @brief 基于位的紧凑型布尔容器
 * @{
 */

/// 每个字的位数
MSTL_INLINE17 constexpr uint32_t BITMAP_WORD_SIZE = 8 * sizeof(uint32_t);

/**
 * @struct bit_reference
 * @brief 位引用类
 *
 * 提供对位图中单个位的引用语义，支持赋值、转换和翻转操作。
 * 模拟布尔引用的行为，允许直接操作位图中的位。
 */
struct bit_reference : icommon<bit_reference>, istringify<bit_reference> {
private:
    uint32_t* ptr_ = nullptr;  ///< 指向包含该位的字
    uint32_t mask_ = 0;  ///< 掩码，用于定位特定位

public:
    /**
     * @brief 默认构造函数
     */
    MSTL_CONSTEXPR20 bit_reference() = default;

    /**
     * @brief 构造函数
     * @param ptr 指向字的指针
     * @param mask 位掩码
     */
    MSTL_CONSTEXPR20 bit_reference(uint32_t* ptr, const uint32_t mask) noexcept
    : ptr_(ptr), mask_(mask) {}

    /**
     * @brief 拷贝构造函数
     * @param other 源位引用
     */
    MSTL_CONSTEXPR20 bit_reference(const bit_reference& other) noexcept
    : ptr_(other.ptr_), mask_(other.mask_) {}

    /**
     * @brief 拷贝赋值运算符
     * @param other 源位引用
     * @return 自身引用
     */
    MSTL_CONSTEXPR20 bit_reference& operator =(const bit_reference& other) noexcept {
        return *this = static_cast<bool>(other);
    }

    /**
     * @brief 移动构造函数
     * @param other 源位引用
     */
    MSTL_CONSTEXPR20 bit_reference(bit_reference&& other) noexcept
    : ptr_(other.ptr_), mask_(other.mask_) {
        other.ptr_ = nullptr;
        other.mask_ = 0;
    }

    /**
     * @brief 移动赋值运算符
     * @param other 源位引用
     * @return 自身引用
     */
    MSTL_CONSTEXPR20 bit_reference& operator =(bit_reference&& other) noexcept {
        *this = static_cast<bool>(other);
        other.ptr_ = nullptr;
        other.mask_ = 0;
        return *this;
    }

    /**
     * @brief 赋值布尔值
     * @param value 要赋的值
     * @return 自身引用
     */
    MSTL_CONSTEXPR20 bit_reference& operator =(const bool value) noexcept {
        if (value) {
            *ptr_ |= mask_;
        } else {
            *ptr_ &= ~mask_;
        }
        return *this;
    }

    /**
     * @brief 转换为布尔值
     * @return 位的布尔值
     */
    MSTL_CONSTEXPR20 explicit operator bool() const noexcept {
        return *ptr_ & mask_;
    }

    /**
     * @brief 翻转位值
     */
    MSTL_CONSTEXPR20 void flip() const noexcept {
        *ptr_ ^= mask_;
    }

    /**
     * @brief 交换两个位引用
     * @param other 要交换的另一个位引用
     */
    MSTL_CONSTEXPR20 void swap(bit_reference& other) noexcept {
        if (_MSTL addressof(other) == this) return;
        const bool tmp = static_cast<bool>(other);
        other = *this;
        *this = tmp;
    }

    /**
     * @brief 相等比较操作符
     * @param rhs 右侧位引用
     * @return 两个位是否相等
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator ==(const bit_reference& rhs) const noexcept {
        return static_cast<bool>(*this) == static_cast<bool>(rhs);
    }

    /**
     * @brief 小于比较操作符
     * @param rhs 右侧位引用
     * @return 比较结果（false < true）
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator <(const bit_reference& rhs) const noexcept {
        return static_cast<bool>(*this) < static_cast<bool>(rhs);
    }

    /**
     * @brief 计算哈希值
     * @return 哈希值
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_t to_hash() const noexcept {
        return hash<bool>()(static_cast<bool>(*this));
    }

    /**
     * @brief 转换为字符串
     * @return "1"或"0"的字符串表示
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string() const {
        return static_cast<bool>(*this) ? "1"_s : "0"_s;
    }
};


/**
 * @struct bitmap_iterator
 * @brief 位图迭代器
 * @tparam IsConst 是否常量迭代器
 * @tparam BitMap 位图类型
 *
 * 提供对位图元素的随机访问迭代器支持。
 */
template <bool IsConst, typename BitMap>
struct bitmap_iterator : iiterator<bitmap_iterator<IsConst, BitMap>> {
public:
    using container_type	= BitMap;  ///< 容器类型
    using value_type		= typename container_type::value_type;  ///< 值类型
    using size_type			= typename container_type::size_type;  ///< 大小类型
    using difference_type	= typename container_type::difference_type;  ///< 差值类型
    using iterator_category = random_access_iterator_tag;  ///< 迭代器类别
    using reference = conditional_t<IsConst, typename container_type::const_reference, typename container_type::reference>;  ///< 引用类型
    using pointer	= conditional_t<IsConst, typename container_type::const_pointer, typename container_type::pointer>;  ///< 指针类型

private:
    uint32_t* ptr_ = nullptr;  ///< 指向当前字的指针
    uint32_t off_ = 0;  ///< 在当前字中的位偏移
    const container_type* container_ = nullptr;  ///< 关联容器指针

    friend class bitmap;
    template <bool, typename> friend struct bitmap_iterator;

private:
    template <typename Ref>
    MSTL_ALWAYS_INLINE MSTL_CONSTEXPR20
    enable_if_t<is_boolean_v<Ref>, Ref>
    reference_dispatch() const noexcept {
        return (*ptr_ & (1U << off_)) != 0;
    }

    template <typename Ref>
    MSTL_ALWAYS_INLINE MSTL_CONSTEXPR20
    enable_if_t<!is_boolean_v<Ref>, Ref>
    reference_dispatch() const noexcept {
        return Ref(ptr_, 1U << off_);
    }

public:
    MSTL_CONSTEXPR20 bitmap_iterator() noexcept = default;
    MSTL_CONSTEXPR20 ~bitmap_iterator() = default;

    MSTL_CONSTEXPR20 bitmap_iterator(const bitmap_iterator&) noexcept = default;
    MSTL_CONSTEXPR20 bitmap_iterator& operator =(const bitmap_iterator&) noexcept = default;
    MSTL_CONSTEXPR20 bitmap_iterator(bitmap_iterator&&) noexcept = default;
    MSTL_CONSTEXPR20 bitmap_iterator& operator =(bitmap_iterator&&) noexcept = default;

    /**
     * @brief 构造函数
     * @param ptr 指向字的指针
     * @param offset 位偏移
     * @param bm 位图指针
     */
    MSTL_CONSTEXPR20 bitmap_iterator(uint32_t* ptr, const uint32_t offset, const container_type* bm) noexcept
    : ptr_(ptr), off_(offset), container_(bm) {}

    /**
     * @brief 从另一个迭代器转换构造
     * @tparam IsConst2 源迭代器的常量性
     * @param other 源迭代器
     */
    template <bool IsConst2>
    MSTL_CONSTEXPR20 explicit bitmap_iterator(const bitmap_iterator<IsConst2, BitMap>& other) noexcept
    : ptr_(other.ptr_), off_(other.off_), container_(other.container_) {}

    /**
     * @brief 解引用操作
     * @return 当前位的引用
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 reference dereference() const noexcept {
        MSTL_DEBUG_VERIFY(ptr_ && container_, "Attempting to dereference on a null pointer");
        return reference_dispatch<reference>();
    }

    /**
     * @brief 递增操作
     */
    MSTL_CONSTEXPR20 void increment() noexcept {
        MSTL_DEBUG_VERIFY(ptr_ && container_, "Attempting to increment a null pointer");
        if (off_++ == BITMAP_WORD_SIZE - 1) {
            off_ = 0;
            ++ptr_;
        }
    }

    /**
     * @brief 递减操作
     */
    MSTL_CONSTEXPR20 void decrement() noexcept {
        MSTL_DEBUG_VERIFY(ptr_ && container_, "Attempting to increment a null pointer");
        if (off_-- == 0) {
            off_ = BITMAP_WORD_SIZE - 1;
            --ptr_;
        }
    }

    /**
     * @brief 前进操作
     * @param off 前进距离
     */
    MSTL_CONSTEXPR20 void advance(difference_type off) noexcept {
        MSTL_DEBUG_VERIFY((ptr_ && container_) || off == 0, "Attempting to advance a null pointer");
        difference_type n = off + off_;
        ptr_ += n / BITMAP_WORD_SIZE;
        n = n % BITMAP_WORD_SIZE;
        if (n < 0) {
            off_ = static_cast<uint32_t>(n) + BITMAP_WORD_SIZE;
            --ptr_;
        } else {
            off_ = static_cast<uint32_t>(n);
        }
    }

    /**
     * @brief 计算距离操作
     * @param other 另一个迭代器
     * @return 两个迭代器之间的距离
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 difference_type distance_to(const bitmap_iterator& other) const noexcept {
        MSTL_DEBUG_VERIFY(container_ == other.container_, "Attempting to distance to a different container");
        return BITMAP_WORD_SIZE * (ptr_ - other.ptr_) + off_ - other.off_;
    }

    /**
     * @brief 下标访问操作符
     * @param n 偏移量
     * @return 偏移位置元素的引用
     */
    MSTL_CONSTEXPR20 reference operator [](const difference_type n) const noexcept {
        return *(*this + n);
    }

    /**
     * @brief 相等比较
     * @param rhs 右侧迭代器
     * @return 是否相等
     */
    MSTL_CONSTEXPR20 bool equal(const bitmap_iterator& rhs) const noexcept {
        MSTL_DEBUG_VERIFY(container_ == rhs.container_, "Attempting to equal to a different container");
        return ptr_ == rhs.ptr_ && off_ == rhs.off_;
    }

    /**
     * @brief 小于比较
     * @param rhs 右侧迭代器
     * @return 当前迭代器是否在rhs之前
     */
    MSTL_CONSTEXPR20 bool less_than(const bitmap_iterator& rhs) const noexcept {
        MSTL_DEBUG_VERIFY(container_ == rhs.container_, "Attempting to equal to a different container");
        return ptr_ < rhs.ptr_ || (ptr_ == rhs.ptr_ && off_ < rhs.off_);
    }

    /**
     * @brief 获取底层指针
     * @return 当前字的指针
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 pointer base() const noexcept {
        return ptr_;
    }

    /**
     * @brief 获取关联容器
     * @return 关联容器指针
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 const container_type* container() const noexcept {
        return container_;
    }
};


/**
 * @class bitmap
 * @brief 位图容器
 *
 * 位图是一种内存高效的布尔值容器，每个元素只占用一个位。
 * 支持随机访问、动态增长和标准容器操作。
 */
class bitmap : public icollector<bitmap> {
public:
    using value_type         = bool;  ///< 值类型
    using pointer            = bit_reference*;  ///< 指针类型
    using reference          = bit_reference;  ///< 引用类型
    using const_pointer      = const bool*;  ///< 常量指针类型
    using const_reference    = const bool;  ///< 常量引用类型
    using size_type          = size_t;  ///< 大小类型
    using difference_type    = ptrdiff_t;  ///< 差值类型
    using iterator                  = bitmap_iterator<false, bitmap>;  ///< 迭代器类型
    using const_iterator            = bitmap_iterator<true, bitmap>;   ///< 常量迭代器类型
    using reverse_iterator          = _MSTL reverse_iterator<iterator>;  ///< 反向迭代器类型
    using const_reverse_iterator    = _MSTL reverse_iterator<const_iterator>;  ///< 常量反向迭代器类型
    using allocator_type            = allocator<uint32_t>;  ///< 分配器类型

private:
    /**
     * @struct bit_storage
     * @brief 位存储管理类
     *
     * 管理底层数组的分配和释放。
     */
    struct bit_storage {
        uint32_t* ptr = nullptr;  ///< 指向存储数组的指针
        compressed_pair<allocator_type, size_t> cpair{default_construct_tag{}, 0};  ///< 压缩存储分配器和容量

        /**
         * @brief 默认构造函数
         */
        MSTL_CONSTEXPR20 bit_storage() noexcept = default;

        /**
         * @brief 构造函数，分配指定数量的字
         * @param word 字数
         */
        explicit MSTL_CONSTEXPR20 bit_storage(const size_t word) {
            if (word == 0) return;
            ptr = cpair.get_base().allocate(word);
            cpair.value = word;
        }

        /**
         * @brief 析构函数
         */
        MSTL_CONSTEXPR20 ~bit_storage() {
            if (ptr) {
                cpair.get_base().deallocate(ptr, cpair.value);
            }
        }

        bit_storage(const bit_storage&) = delete;
        bit_storage& operator =(const bit_storage&) = delete;

        /**
         * @brief 移动构造函数
         * @param other 源存储对象
         */
        MSTL_CONSTEXPR20 bit_storage(bit_storage&& other) noexcept
        : ptr(other.ptr), cpair(_MSTL move(other.cpair)) {
            other.ptr = nullptr;
            other.cpair.value = 0;
        }

        /**
         * @brief 移动赋值运算符
         * @param other 源存储对象
         * @return 自身引用
         */
        MSTL_CONSTEXPR20 bit_storage& operator =(bit_storage&& other) noexcept {
            if (this != &other) {
                reset();
                cpair = _MSTL move(other.cpair);
                other.ptr = nullptr;
                other.cpair.value = 0;
            }
            return *this;
        }

        /**
         * @brief 重置存储
         * @param new_ptr 新指针
         * @param cap 新容量
         */
        MSTL_CONSTEXPR20 void reset(uint32_t* new_ptr = nullptr, const size_t cap = 0) {
            if (ptr) {
                cpair.get_base().deallocate(ptr, cpair.value);
            }
            ptr = new_ptr;
            cpair.value = cap;
        }

        /**
         * @brief 分配指定数量的字
         * @param word 字数
         */
        MSTL_CONSTEXPR20 void allocate(const size_t word) {
            reset(cpair.get_base().allocate(word), word);
        }

        /**
         * @brief 获取存储指针
         * @return 存储指针
         */
        MSTL_CONSTEXPR20 uint32_t* get() const noexcept {
            return ptr;
        }

        /**
         * @brief 获取容量（字数）
         * @return 容量
         */
        MSTL_CONSTEXPR20 size_t capacity() const noexcept {
            return cpair.value;
        }
    };

    iterator start_{};  ///< 起始迭代器
    iterator finish_{};  ///< 结束迭代器
    bit_storage storage_{};  ///< 底层存储

private:
    /**
     * @brief 计算存储n个位所需的字数
     * @param word 位数
     * @return 字数
     */
    static constexpr size_t word_count(const size_type word) noexcept {
        return (word + BITMAP_WORD_SIZE - 1) / BITMAP_WORD_SIZE;
    }

    /**
     * @brief 分配存储空间
     * @param word 位数
     */
    MSTL_CONSTEXPR20 void allocate_storage(const size_type word) {
        if (word == 0) return;
        storage_.allocate(word_count(word));
    }

    /**
     * @brief 设置迭代器
     * @param word 位数
     */
    MSTL_CONSTEXPR20 void set_iterators(const size_type word) noexcept {
        start_ = iterator(storage_.get(), 0, this);
        finish_ = start_ + static_cast<difference_type>(word);
    }

    /**
     * @brief 复制位范围
     * @tparam Iterator1 源迭代器类型
     * @tparam Iterator2 目标迭代器类型
     * @param first 源起始
     * @param last 源结束
     * @param result 目标起始
     * @return 目标结束位置
     */
    template <typename Iterator1, typename Iterator2>
    MSTL_CONSTEXPR20 Iterator2 bit_copy(Iterator1 first, Iterator1 last, Iterator2 result) {
        iter_difference_t<Iterator1> n = _MSTL distance(first, last);
        for (; n > 0; --n, ++first, ++result) {
            *result = *first;
        }
        return result;
    }

    /**
     * @brief 反向复制位范围
     * @tparam Iterator1 源迭代器类型
     * @tparam Iterator2 目标迭代器类型
     * @param first 源起始
     * @param last 源结束
     * @param result 目标结束
     * @return 目标起始位置
     */
    template <typename Iterator1, typename Iterator2>
    MSTL_CONSTEXPR20 Iterator2 bit_copy_backward(Iterator1 first, Iterator1 last, Iterator2 result) {
        iter_difference_t<Iterator1> n = _MSTL distance(first, last);
        for (; n > 0; --n) {
            *--result = *--last;
        }
        return result;
    }

    /**
     * @brief 重新分配并插入
     * @param position 插入位置
     * @param extra_len 插入长度
     * @param value 插入值
     */
    void reallocate_insert(const iterator& position, const size_type extra_len, const bool value) {
        const size_type old_size = size();
        const size_type new_size = old_size + extra_len;
        const size_type new_words = word_count(new_size);
        bit_storage new_storage(new_words);

        const iterator new_start(new_storage.get(), 0, this);
        auto new_finish = bitmap::bit_copy(begin(), position, new_start);
        _MSTL fill_n(new_finish, extra_len, value);
        new_finish += static_cast<difference_type>(extra_len);
        bitmap::bit_copy(position, end(), new_finish);
        new_finish += (end() - position);

        storage_ = _MSTL move(new_storage);
        start_ = new_start;
        finish_ = new_finish;
    }

    /**
     * @brief 重新分配并插入范围
     * @tparam Iterator 迭代器类型
     * @param position 插入位置
     * @param first 源起始
     * @param last 源结束
     * @param extra_len 插入长度
     */
    template <typename Iterator>
    void reallocate_insert_range(const iterator position, Iterator first,
                                 Iterator last, const size_type extra_len) {
        const size_type old_size = size();
        const size_type new_size = old_size + extra_len;
        const size_type new_words = word_count(new_size);
        bit_storage new_storage(new_words);

        const iterator new_start(new_storage.get(), 0, this);
        auto new_finish = bitmap::bit_copy(begin(), position, new_start);
        new_finish = bitmap::bit_copy(first, last, new_finish);
        new_finish = bitmap::bit_copy(position, end(), new_finish);

        storage_ = _MSTL move(new_storage);
        start_ = new_start;
        finish_ = new_finish;
    }

    /**
     * @brief 插入辅助函数
     * @param position 插入位置
     * @param value 插入值
     */
    void insert_aux(const iterator& position, const bool value) {
        if (finish_.ptr_ != storage_.get() + storage_.capacity()) {
            bit_copy_backward(position, finish_, finish_ + 1);
            *position = value;
            ++finish_;
        } else {
            const size_type len = size() ? 2 * size() : BITMAP_WORD_SIZE;
            reallocate_insert(position, 1, value);
        }
    }

    /**
     * @brief 范围初始化（非前向迭代器版本）
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator>
    enable_if_t<!is_ranges_fwd_iter_v<Iterator>>
    range_init(Iterator first, Iterator last) {
        if (first == last) return;

        bitmap tmp{};
        while (first != last) {
            tmp.push_back(*first);
            ++first;
        }

        bitmap::swap(tmp);
        return;
    }

    /**
     * @brief 范围初始化（前向迭代器版本）
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator>
    enable_if_t<is_ranges_fwd_iter_v<Iterator>>
    range_init(Iterator first, Iterator last) {
        if (first == last) return;

        const size_type n = _MSTL distance(first, last);
        bit_storage tmp_storage(word_count(n));
        iterator tmp_start(tmp_storage.get(), 0, this);
        const iterator tmp_finish = bitmap::bit_copy(first, last, tmp_start);

        storage_ = _MSTL move(tmp_storage);
        start_ = tmp_start;
        finish_ = tmp_finish;
        return;
    }

    /**
     * @brief 范围插入（非前向迭代器版本）
     * @tparam Iterator 迭代器类型
     * @param position 插入位置
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator>
    enable_if_t<!is_ranges_fwd_iter_v<Iterator>>
    insert_range(iterator position, Iterator first, Iterator last) {
        if (first == last) return;
        bitmap tmp(first, last);
        insert_range(position, tmp.begin(), tmp.end());
        return;
    }

    /**
     * @brief 范围插入（前向迭代器版本）
     * @tparam Iterator 迭代器类型
     * @param position 插入位置
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator>
    enable_if_t<is_ranges_fwd_iter_v<Iterator>>
    insert_range(iterator position, Iterator first, Iterator last) {
        if (first == last) return;
        const size_type n = _MSTL distance(first, last);
        if (capacity() - size() >= n) {
            bitmap::bit_copy_backward(position, end(), finish_ + static_cast<difference_type>(n));
            bitmap::bit_copy(first, last, position);
            finish_ += static_cast<difference_type>(n);
        } else {
            bitmap::reallocate_insert_range(position, first, last, n);
        }
        return;
    }

public:
    /**
     * @brief 默认构造函数
     *
     * 构造一个空位图。
     */
    MSTL_CONSTEXPR20 bitmap() noexcept = default;

    /**
     * @brief 构造函数，指定大小
     * @param word 位数
     *
     * 构造包含n个位的位置，所有位初始化为false。
     */
    MSTL_CONSTEXPR20 explicit bitmap(const size_type word) {
        if (word == 0) return;
        allocate_storage(word);
        set_iterators(word);
        _MSTL fill(storage_.get(), storage_.get() + storage_.capacity(), 0);
    }

    /**
     * @brief 构造函数，指定大小和初始值
     * @param word 位数
     * @param value 初始值
     */
    MSTL_CONSTEXPR20 explicit bitmap(const size_type word, const bool value) {
        if (word == 0) return;
        allocate_storage(word);
        set_iterators(word);
        _MSTL fill(storage_.get(), storage_.get() + storage_.capacity(), value ? ~0U : 0U);
    }

    /**
     * @brief 32位整数构造函数
     * @param word 位数
     * @param value 初始值
     */
    MSTL_CONSTEXPR20 explicit bitmap(const int32_t word, const bool value)
    : bitmap(static_cast<size_type>(word), value) {}

    /**
     * @brief 64位整数构造函数
     * @param word 位数
     * @param value 初始值
     */
    MSTL_CONSTEXPR20 explicit bitmap(const int64_t word, const bool value)
    : bitmap(static_cast<size_type>(word), value) {}

    /**
     * @brief 拷贝构造函数
     * @param other 源位图
     */
    MSTL_CONSTEXPR20 bitmap(const bitmap& other) {
        if (other.empty()) return;

        const size_type n = other.size();
        bit_storage tmp(word_count(n));
        const iterator tmp_start(tmp.get(), 0, this);
        const auto tmp_finish = bit_copy(other.cbegin(), other.cend(), tmp_start);

        storage_ = _MSTL move(tmp);
        start_ = tmp_start;
        finish_ = tmp_finish;
    }

    /**
     * @brief 拷贝赋值运算符
     * @param other 源位图
     * @return 自身引用
     */
    MSTL_CONSTEXPR20 bitmap& operator =(const bitmap& other) {
        if (_MSTL addressof(other) == this) return *this;
        bitmap tmp(other);
        bitmap::swap(tmp);
        return *this;
    }

    /**
     * @brief 移动构造函数
     * @param other 源位图
     */
    MSTL_CONSTEXPR20 bitmap(bitmap&& other) noexcept
    : start_(other.start_.ptr_, other.start_.off_, this),
      finish_(other.finish_.ptr_, other.finish_.off_, this),
      storage_(_MSTL move(other.storage_)) {
        other.start_ = iterator();
        other.finish_ = iterator();
    }

    /**
     * @brief 移动赋值运算符
     * @param other 源位图
     * @return 自身引用
     */
    MSTL_CONSTEXPR20 bitmap& operator =(bitmap&& other) noexcept {
        if (_MSTL addressof(other) == this) return *this;
        bitmap::swap(other);
        return *this;
    }

    /**
     * @brief 范围构造函数
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator, enable_if_t<is_iter_v<Iterator>, int> = 0>
    MSTL_CONSTEXPR20 bitmap(Iterator first, Iterator last) {
        bitmap::range_init(first, last);
    }

    /**
     * @brief 析构函数
     */
    MSTL_CONSTEXPR20 ~bitmap() = default;

     /**
     * @brief 获取起始迭代器
     * @return 指向第一个位的迭代器
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 iterator begin() noexcept {
        return start_;
    }

    /**
     * @brief 获取结束迭代器
     * @return 指向最后一个位之后位置的迭代器
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 iterator end() noexcept {
        return finish_;
    }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个位的常量迭代器
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_iterator begin() const noexcept {
        return cbegin();
    }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向最后一个位之后位置的常量迭代器
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_iterator end() const noexcept {
        return cend();
    }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个位的常量迭代器
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_iterator cbegin() const noexcept {
        return const_iterator{start_};
    }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向最后一个位之后位置的常量迭代器
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_iterator cend() const noexcept {
        return const_iterator{finish_};
    }

    /**
     * @brief 获取反向起始迭代器
     * @return 指向最后一个位的反向迭代器
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 reverse_iterator rbegin() noexcept {
        return reverse_iterator(end());
    }

    /**
     * @brief 获取反向结束迭代器
     * @return 指向第一个位之前位置的反向迭代器
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 reverse_iterator rend() noexcept {
        return reverse_iterator(begin());
    }

    /**
     * @brief 获取常量反向起始迭代器
     * @return 指向最后一个位的常量反向迭代器
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_reverse_iterator rbegin() const noexcept {
        return crbegin();
    }

    /**
     * @brief 获取常量反向结束迭代器
     * @return 指向第一个位之前位置的常量反向迭代器
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_reverse_iterator rend() const noexcept {
        return crend();
    }

    /**
     * @brief 获取常量反向起始迭代器
     * @return 指向最后一个位的常量反向迭代器
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_reverse_iterator crbegin() const noexcept {
        return const_reverse_iterator(cend());
    }

    /**
     * @brief 获取常量反向结束迭代器
     * @return 指向第一个位之前位置的常量反向迭代器
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_reverse_iterator crend() const noexcept {
        return const_reverse_iterator(cbegin());
    }

    /**
     * @brief 获取位数
     * @return 位图中的位数
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type size() const noexcept {
        return cend() - cbegin();
    }

    /**
     * @brief 获取最大可能位数
     * @return 最大位数
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type max_size() const noexcept {
        return static_cast<size_type>(-1);
    }

    /**
     * @brief 检查是否为空
     * @return 位图是否为空
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool empty() const noexcept {
        return start_ == finish_;
    }

    /**
     * @brief 获取容量（位数）
     * @return 当前分配的存储可容纳的位数
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type capacity() const noexcept {
        return storage_.capacity() * BITMAP_WORD_SIZE;
    }

    /**
     * @brief 下标访问操作符
     * @param n 索引
     * @return 指定位置的位的引用
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 reference operator [](const size_type n) {
        return *(begin() + static_cast<difference_type>(n));
    }

    /**
     * @brief 常量下标访问操作符
     * @param n 索引
     * @return 指定位置的位的常量值
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_reference operator [](const size_type n) const {
        return *(cbegin() + static_cast<difference_type>(n));
    }

    /**
     * @brief 访问第一个位
     * @return 第一个位的引用
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 reference front() {
        return *begin();
    }

    /**
     * @brief 常量版本，访问第一个位
     * @return 第一个位的常量值
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_reference front() const {
        return *cbegin();
    }

    /**
     * @brief 访问最后一个位
     * @return 最后一个位的引用
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 reference back() {
        return *(end() - 1);
    }

    /**
     * @brief 常量版本，访问最后一个位
     * @return 最后一个位的常量值
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_reference back() const {
        return *(cend() - 1);
    }

    /**
     * @brief 预留容量
     * @param n 要预留的位数
     *
     * 确保位图至少有n位的容量。
     */
    MSTL_CONSTEXPR20 void reserve(const size_type n) {
        if (capacity() < n) {
            const size_type new_words = word_count(n);
            bit_storage new_storage(new_words);
            const iterator new_start(new_storage.get(), 0, this);
            const auto new_finish = bit_copy(begin(), end(), new_start);

            storage_ = _MSTL move(new_storage);
            start_ = new_start;
            finish_ = new_finish;
        }
    }

    /**
     * @brief 在末尾插入位
     * @param value 要插入的值
     */
    MSTL_CONSTEXPR20 void push_back(const bool value) {
        if (finish_.ptr_ != storage_.get() + storage_.capacity()) {
            *finish_++ = value;
        } else {
            insert_aux(end(), value);
        }
    }

    /**
     * @brief 在指定位置插入位
     * @param position 插入位置
     * @param value 要插入的值
     * @return 指向插入位置的迭代器
     */
    MSTL_CONSTEXPR20 iterator insert(const iterator& position, const bool value) {
        const difference_type n = position - begin();
        if (finish_.ptr_ != storage_.get() + storage_.capacity() && position == end()) {
            *finish_++ = value;
        } else {
            insert_aux(position, value);
        }
        return begin() + n;
    }

    /**
     * @brief 范围插入
     * @tparam Iterator 迭代器类型
     * @param position 插入位置
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator>
    MSTL_CONSTEXPR20 void insert(iterator position, Iterator first, Iterator last) {
        bitmap::insert_range(position, first, last);
    }

    /**
     * @brief 插入布尔数组范围
     * @param position 插入位置
     * @param first 数组起始指针
     * @param last 数组结束指针
     */
    MSTL_CONSTEXPR20 void insert(const iterator& position, const bool* first, const bool* last) {
        if (first == last) return;
        const size_type n = _MSTL distance(first, last);
        if (capacity() - size() >= n) {
            bitmap::bit_copy_backward(position, end(), finish_ + static_cast<difference_type>(n));
            bitmap::bit_copy(first, last, position);
            finish_ += static_cast<difference_type>(n);
        } else {
            bitmap::reallocate_insert_range(position, first, last, n);
        }
    }

    /**
     * @brief 插入n个相同的位
     * @param position 插入位置
     * @param n 插入数量
     * @param value 插入值
     */
    MSTL_CONSTEXPR20 void insert(const iterator& position, const size_type n, const bool value) {
        if (n == 0) return;
        if (capacity() - size() >= n) {
            bitmap::bit_copy_backward(position, end(), finish_ + static_cast<difference_type>(n));
            _MSTL fill(position, position + static_cast<difference_type>(n), value);
            finish_ += static_cast<difference_type>(n);
        } else {
            bitmap::reallocate_insert(position, n, value);
        }
    }

    /**
     * @brief 插入n个相同的位
     * @param pos 插入位置
     * @param n 插入数量
     * @param value 插入值
     */
    MSTL_CONSTEXPR20 void insert(const iterator& pos, const int32_t n, const bool value) {
        bitmap::insert(pos, static_cast<size_type>(n), value);
    }

    /**
     * @brief 插入n个相同的位
     * @param pos 插入位置
     * @param n 插入数量
     * @param value 插入值
     */
    MSTL_CONSTEXPR20 void insert(const iterator& pos, const int64_t n, const bool value) {
        bitmap::insert(pos, static_cast<size_type>(n), value);
    }

    /**
     * @brief 删除末尾位
     */
    MSTL_CONSTEXPR20 void pop_back() {
        --finish_;
    }

    /**
     * @brief 删除指定位置的位
     * @param position 要删除的位置
     * @return 指向被删除元素之后位置的迭代器
     */
    MSTL_CONSTEXPR20 iterator erase(const iterator& position) {
      if (position + 1 != end()) {
          bitmap::bit_copy(position + 1, end(), position);
      }
      --finish_;
      return position;
    }

    /**
     * @brief 删除指定范围内的位
     * @param first 起始位置
     * @param last 结束位置
     * @return 指向最后一个被删除元素之后位置的迭代器
     */
    MSTL_CONSTEXPR20 iterator erase(const iterator& first, const iterator& last) {
        finish_ = bitmap::bit_copy(last, end(), first);
        return first;
    }

    /**
     * @brief 调整大小
     * @param n 新的大小
     * @param value 用于填充新元素的默认值
     */
    MSTL_CONSTEXPR20 void resize(const size_type n, const bool value = bool()) {
        if (n < size()) {
            bitmap::erase(begin() + static_cast<difference_type>(n), end());
        } else {
            bitmap::insert(end(), n - size(), value);
        }
    }

    /**
     * @brief 清空位图
     */
    MSTL_CONSTEXPR20 void clear() {
        bitmap::erase(begin(), end());
    }

    /**
     * @brief 交换两个位图的内容
     * @param other 要交换的另一个位图
     */
    MSTL_CONSTEXPR20 void swap(bitmap& other) noexcept {
        if (_MSTL addressof(other) == this) return;
        _MSTL swap(start_, other.start_);
        _MSTL swap(finish_, other.finish_);
        _MSTL swap(storage_, other.storage_);
        start_.container_ = this;
        finish_.container_ = this;
        other.start_.container_ = &other;
        other.finish_.container_ = &other;
    }

    /**
     * @brief 相等比较操作符
     * @param rhs 右侧位图
     * @return 如果两个位图大小相等且对应位相等返回true
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator ==(const bitmap& rhs) const
    noexcept(noexcept(_MSTL equal(this->cbegin(), this->cend(), rhs.cbegin()))) {
        return this->size() == rhs.size() && _MSTL equal(this->cbegin(), this->cend(), rhs.cbegin());
    }

    /**
     * @brief 小于比较操作符
     * @param rhs 右侧位图
     * @return 按字典序比较结果
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator <(const bitmap& rhs) const
    noexcept(noexcept(_MSTL lexicographical_compare(this->cbegin(), this->cend(), rhs.cbegin(), rhs.cend()))) {
        return _MSTL lexicographical_compare(this->cbegin(), this->cend(), rhs.cbegin(), rhs.cend());
    }
};

/** @} */ // Bitmap

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_CONTAINER_BITMAP_HPP__
