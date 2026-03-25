#ifndef NEFORCE_CORE_CONTAINER_HASHTABLE_HPP__
#define NEFORCE_CORE_CONTAINER_HASHTABLE_HPP__

/**
 * @file hashtable.hpp
 * @brief 哈希表容器
 *
 * 此文件提供了哈希表容器的实现。
 * 哈希表是一种基于键直接访问数据的数据结构，通过哈希函数将键映射到桶，
 * 提供平均常数时间复杂度的插入、删除和查找操作。
 * 作为无序关联式容器的底层实现。
 */

#include "NeForce/core/algorithm/sort.hpp"
#include "NeForce/core/container/vector.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup HashTable 哈希表
 * @brief 基于哈希表的无序容器实现
 * @{
 */

/**
 * @struct hashtable_node
 * @brief 哈希表节点
 * @tparam T 数据类型
 *
 * 哈希表节点包含数据和指向下一个节点的指针，形成链表。
 */
template <typename T>
struct hashtable_node {
    hashtable_node* next = nullptr;  ///< 指向下一个节点的指针
    T data;  ///< 节点存储的数据

    /**
     * @brief 默认构造函数
     */
    hashtable_node()
    noexcept(is_nothrow_default_constructible_v<T>)
    : data() {}
};


template <typename Value, typename Key, typename HashFcn,
    typename ExtractKey, typename EqualKey, typename Alloc>
class hashtable;


/**
 * @struct hashtable_iterator
 * @brief 哈希表迭代器
 * @tparam IsConst 是否常量迭代器
 * @tparam HashTable 哈希表类型
 *
 * 提供对哈希表元素的前向迭代访问。
 */
template <bool IsConst, typename HashTable>
struct hashtable_iterator : iiterator<hashtable_iterator<IsConst, HashTable>> {
public:
    using container_type	= HashTable;  ///< 容器类型
    using value_type		= typename container_type::value_type;  ///< 值类型
    using size_type			= typename container_type::size_type;  ///< 大小类型
    using difference_type	= typename container_type::difference_type;  ///< 差值类型
    using iterator_category = forward_iterator_tag;  ///< 迭代器类别（前向迭代器）
    using reference = conditional_t<IsConst, typename container_type::const_reference, typename container_type::reference>;  ///< 引用类型
    using pointer	= conditional_t<IsConst, typename container_type::const_pointer, typename container_type::pointer>;  ///< 指针类型

private:
    using node_type = hashtable_node<value_type>;  ///< 节点类型

    node_type* current_ = nullptr;  ///< 当前节点指针
    size_type bucket_ = 0;  ///< 当前桶索引
    const container_type* container_ = nullptr;  ///< 关联容器指针

    template <typename, typename, typename, typename, typename, typename>
    friend class hashtable;

public:
    hashtable_iterator() noexcept = default;
    ~hashtable_iterator() = default;

    hashtable_iterator(const hashtable_iterator&) noexcept = default;
    hashtable_iterator& operator =(const hashtable_iterator&) noexcept = default;
    hashtable_iterator(hashtable_iterator&&) noexcept = default;
    hashtable_iterator& operator =(hashtable_iterator&&) noexcept = default;

    /**
     * @brief 构造函数
     * @param ptr 节点指针
     * @param bucket 桶索引
     * @param ht 哈希表指针
     */
    hashtable_iterator(node_type* ptr, const size_type bucket, const HashTable* ht)
    : current_(ptr), bucket_(bucket), container_(ht) {}

    /**
     * @brief 解引用操作
     * @return 当前元素的引用
     */
    NEFORCE_NODISCARD reference dereference() const noexcept {
        NEFORCE_DEBUG_VERIFY(current_ && container_, "Attempting to dereference on a null pointer");
        NEFORCE_DEBUG_VERIFY(
            bucket_ < container_->buckets_.size() && 0 <= bucket_,
            "Attempting to dereference out of boundary");
        return current_->data;
    }

    /**
     * @brief 递增操作
     *
     * 移动到当前链表的下一个节点，如果当前链表结束，则查找下一个非空桶。
     */
    void increment() noexcept {
        NEFORCE_DEBUG_VERIFY(current_ && container_, "Attempting to increment a null pointer");
        NEFORCE_DEBUG_VERIFY(
            bucket_ < container_->buckets_.size() &&
            !(bucket_ + 1 == container_->buckets_.size() && current_->next != nullptr),
            "Attempting to increment out of boundary");
        current_ = current_->next;
        if (current_ == nullptr) {
            while (current_ == nullptr && ++bucket_ < container_->buckets_.size()) {
                current_ = container_->buckets_[bucket_];
            }
        }
    }

    /**
     * @brief 相等比较
     * @param rhs 右侧迭代器
     * @return 是否相等
     */
    NEFORCE_NODISCARD bool equal(const hashtable_iterator& rhs) const noexcept {
        NEFORCE_DEBUG_VERIFY(container_ == rhs.container_, "Attempting to equal to a different container");
        return current_ == rhs.current_;
    }

    /**
     * @brief 获取底层指针
     * @return 当前节点指针
     */
    NEFORCE_NODISCARD pointer base() const noexcept {
        return current_;
    }

    /**
     * @brief 获取关联容器
     * @return 关联容器指针
     */
    NEFORCE_NODISCARD const container_type* container() const noexcept {
        return container_;
    }
};


NEFORCE_BEGIN_CONSTANTS__
#ifdef NEFORCE_ARCH_BITS_64

/**
 * @brief 哈希表素数列表（64位系统）
 *
 * 用于确定哈希表大小的一系列素数，减少哈希冲突。
 */
NEFORCE_INLINE17 constexpr size_t HASH_PRIME_LIST[] = {
    101,                    173,                        263,                        397,
    599,                    907,                        1361,                       2053,
    3083,                   4637,                       6959,                       10453,
    15683,                  23531,                      35311,                      52967,
    79451,                  119179,                     178781,                     268189,
    402299,                 603457,                     905189,                     1357787,
    2036687,                3055043,                    4582577,                    6873871,
    10310819,               15466229,                   23199347,                   34799021,
    52198537,               78297827,                   117446801,                  176170229,
    264255353,              396383041,                  594574583,                  891861923,
    1337792887,             2006689337,                 3010034021u,                4515051137ull,
    6772576709ull,          10158865069ull,             15238297621ull,             22857446471ull,
    34286169707ull,         51429254599ull,             77143881917ull,             115715822899ull,
    173573734363ull,        260360601547ull,            390540902329ull,            585811353559ull,
    878717030339ull,        1318075545511ull,           1977113318311ull,           2965669977497ull,
    4448504966249ull,       6672757449409ull,           10009136174239ull,          15013704261371ull,
    22520556392057ull,      33780834588157ull,          50671251882247ull,          76006877823377ull,
    114010316735089ull,     171015475102649ull,         256523212653977ull,         384784818980971ull,
    577177228471507ull,     865765842707309ull,         1298648764060979ull,        1947973146091477ull,
    2921959719137273ull,    4382939578705967ull,        6574409368058969ull,        9861614052088471ull,
    14792421078132871ull,   22188631617199337ull,       33282947425799017ull,       49924421138698549ull,
    74886631708047827ull,   112329947562071807ull,      168494921343107851ull,      252742382014661767ull,
    379113573021992729ull,  568670359532989111ull,      853005539299483657ull,      1279508308949225477ull,
    1919262463423838231ull, 2878893695135757317ull,     4318340542703636011ull,     6477510814055453699ull
};

#else

/**
 * @brief 哈希表素数列表（32位系统）
 */
NEFORCE_INLINE17 constexpr size_t HASH_PRIME_LIST[] = {
    53,         97,           193,         389,       769,
    1543,       3079,         6151,        12289,     24593,
    49157,      98317,        196613,      393241,    786433,
    1572869,    3145739,      6291469,     12582917,  25165843,
    50331653,   100663319,    201326611,   402653189, 805306457,
    1610612741
};

#endif

/// 素数列表长度
NEFORCE_INLINE17 constexpr size_t HASH_PRIMER_COUNT = extent_v<decltype(HASH_PRIME_LIST)>;

NEFORCE_END_CONSTANTS__


/**
 * @class hashtable
 * @brief 哈希表容器
 * @tparam Value 值类型
 * @tparam Key 键类型
 * @tparam HashFcn 哈希函数类型
 * @tparam ExtractKey 从值中提取键的函数对象类型
 * @tparam EqualKey 键相等比较函数对象类型
 * @tparam Alloc 分配器类型
 *
 * 哈希表使用链地址法处理冲突，采用素数表确定桶数量，提供平均常数时间复杂度的
 * 插入、删除和查找操作。支持唯一键和允许重复键两种模式。
 */
template <typename Value, typename Key, typename HashFcn,
    typename ExtractKey, typename EqualKey, typename Alloc>
class hashtable : public icollector<hashtable<Value, Key, HashFcn, ExtractKey, EqualKey, Alloc>> {
public:
    using key_type          = Key;       ///< 键类型
    using hasher            = HashFcn;   ///< 哈希函数类型
    using key_equal         = EqualKey;  ///< 键相等比较函数类型

    using value_type        = Value;    ///< 值类型
    using pointer           = Value*;   ///< 指针类型
    using reference         = Value&;   ///< 引用类型
    using const_pointer     = const Value*;  ///< 常量指针类型
    using const_reference   = const Value&;  ///< 常量引用类型
    using size_type         = size_t;        ///< 大小类型
    using difference_type   = ptrdiff_t;     ///< 差值类型
    using iterator          = hashtable_iterator<false, hashtable>;  ///< 迭代器类型
    using const_iterator    = hashtable_iterator<true, hashtable>;   ///< 常量迭代器类型
    using allocator_type    = Alloc;     ///< 分配器类型

private:
    using node_type = hashtable_node<Value>;  ///< 节点类型
    using link_type = node_type*;  ///< 节点指针类型

    vector<link_type> buckets_{};  ///< 桶数组
    size_type size_ = 0;  ///< 元素数量
    hasher hasher_{};  ///< 哈希函数对象
    key_equal equals_{};  ///< 键相等比较对象
    ExtractKey extracter_{};  ///< 值提取键对象
    compressed_pair<allocator_type, float> pair_{ default_construct_tag{}, 1.0f };  ///< 压缩存储分配器和最大负载因子

    template <bool, typename>
    friend struct hashtable_iterator;

private:
    /**
     * @brief 获取不小于n的下一个素数
     * @param n 参考值
     * @return 素数
     */
    NEFORCE_NODISCARD static size_type next_size(const size_type n) noexcept {
        const size_t* first = constants::HASH_PRIME_LIST;
        const size_t* last = constants::HASH_PRIME_LIST + constants::HASH_PRIMER_COUNT;
        const size_t* pos = _NEFORCE lower_bound(first, last, n);
        return pos == last ? *(last - 1) : *pos;
    }

    /**
     * @brief 初始化桶数组
     * @param n 期望的桶数量
     */
    void initialize_buckets(const size_type n) {
        const size_type n_buckets = next_size(n);
        buckets_.assign(n_buckets, nullptr);
    }

    /**
     * @brief 计算键的桶索引
     * @param key 键
     * @param n 桶数量
     * @return 桶索引
     */
    size_type bucket_index_key(const key_type& key, const size_t n) const
    noexcept(is_nothrow_hashable_v<key_type>) {
        if (n == 0) return 0;
        return hasher_(key) % n;
    }

    /**
     * @brief 计算值的桶索引
     * @param value 值
     * @param n 桶数量
     * @return 桶索引
     */
    size_type bucket_index_value(const value_type& value, const size_t n) const
    noexcept(is_nothrow_hashable_v<key_type>) {
        return hashtable::bucket_index_key(extracter_(value), n);
    }

    /**
     * @brief 创建新节点
     * @tparam Args 构造参数类型
     * @param args 构造参数
     * @return 新节点指针
     */
    template <typename... Args>
    link_type new_node(Args&&... args) {
        link_type n = pair_.get_base().allocate();
        n->next = nullptr;
        try {
            _NEFORCE construct(&n->data, _NEFORCE forward<Args>(args)...);
        } catch (...) {
            hashtable::delete_node(n);
            NEFORCE_THROW_EXCEPTION(memory_exception("hashtable construct node failed."));
        }
        return n;
    }

    /**
     * @brief 删除节点
     * @param n 要删除的节点
     */
    void delete_node(link_type n) noexcept {
        _NEFORCE destroy(&n->data);
        pair_.get_base().deallocate(n);
    }

    /**
     * @brief 从另一个哈希表拷贝
     * @param other 源哈希表
     */
    void copy_from(const hashtable& other) {
        buckets_.clear();
        buckets_.reserve(other.buckets_.size());
        buckets_.insert(buckets_.end(), other.buckets_.size(), nullptr);
        try {
            for (size_type i = 0; i < other.buckets_.size(); ++i) {
                if (link_type cur = other.buckets_[i]) {
                    link_type copy = hashtable::new_node(cur->data);
                    buckets_[i] = copy;
                    for (link_type next = cur->next; next != nullptr; cur = next, next = cur->next) {
                        copy->next = hashtable::new_node(next->data);
                        copy = copy->next;
                    }
                }
            }
            size_ = other.size_;
        } catch (...) {
            clear();
            throw;
        }
    }

    /**
     * @brief 插入唯一键节点（不触发rehash）
     * @param ptr 要插入的节点
     * @return 插入结果（迭代器和是否成功）
     */
    pair<iterator, bool> insert_unique_noresize(link_type ptr) {
        const size_type n = hashtable::bucket_index_value(ptr->data, buckets_.size());

        node_type** buckets_ptr = &buckets_[n];
        while (*buckets_ptr != nullptr) {
            if (equals_(extracter_((*buckets_ptr)->data), extracter_(ptr->data))) {
                ptr->next = (*buckets_ptr)->next;
                hashtable::delete_node(*buckets_ptr);
                *buckets_ptr = ptr;
                return {{ptr, n, this}, false};
            }
            buckets_ptr = &(*buckets_ptr)->next;
        }
        ptr->next = nullptr;
        *buckets_ptr = ptr;
        ++size_;
        return {iterator{ptr, n, this}, true};
    }

    /**
     * @brief 插入允许重复键节点（不触发rehash）
     * @param ptr 要插入的节点
     * @return 指向插入位置的迭代器
     */
    iterator insert_equal_noresize(link_type ptr) {
        const size_type n = hashtable::bucket_index_value(ptr->data, buckets_.size());
        link_type first = buckets_[n];

        link_type prev = nullptr;
        link_type cur = first;
        while (cur != nullptr &&
               equals_(extracter_(cur->data), extracter_(ptr->data))) {
            prev = cur;
            cur = cur->next;
        }

        if (prev != nullptr) {
            prev->next = ptr;
            ptr->next = cur;
        } else {
            ptr->next = first;
            buckets_[n] = ptr;
        }

        ++size_;
        return {ptr, n, this};
    }

    /**
     * @brief 范围插入唯一键（非前向迭代器版本）
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator>
    enable_if_t<!is_ranges_fwd_iter_v<Iterator>>
    insert_unique_aux(Iterator first, Iterator last) {
        for (; first != last; ++first) {
            hashtable::insert_unique(*first);
        }
        return;
    }

    /**
     * @brief 范围插入唯一键（前向迭代器版本）
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator>
    enable_if_t<is_ranges_fwd_iter_v<Iterator>>
    insert_unique_aux(Iterator first, Iterator last) {
        size_type n = _NEFORCE distance(first, last);

        const size_type need_buckets = static_cast<size_type>(
            _NEFORCE ceil(static_cast<double>(size_ + n) / max_load_factor()));
        rehash(need_buckets);

        for (; n > 0; --n, ++first) {
            link_type tmp = hashtable::new_node(*first);
            hashtable::insert_unique_noresize(tmp);
        }
        return;
    }

    /**
     * @brief 范围插入允许重复键（非前向迭代器版本）
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator>
    enable_if_t<!is_ranges_fwd_iter_v<Iterator>>
    insert_equal_aux(Iterator first, Iterator last) {
        for (; first != last; ++first) {
            hashtable::insert_equal(*first);
        }
        return;
    }

    /**
     * @brief 范围插入允许重复键（前向迭代器版本）
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator>
    enable_if_t<is_ranges_fwd_iter_v<Iterator>>
    insert_equal_aux(Iterator first, Iterator last) {
        size_type n = _NEFORCE distance(first, last);

        const size_type need_buckets = static_cast<size_type>(
            _NEFORCE ceil(static_cast<double>(size_ + n) / max_load_factor()));
        rehash(need_buckets);

        for (; n > 0; --n, ++first) {
            link_type tmp = hashtable::new_node(*first);
            hashtable::insert_equal_noresize(tmp);
        }
        return;
    }

    /**
     * @brief 删除桶中从开头到指定节点的节点
     * @param bucket 桶索引
     * @param last 结束节点（不删除）
     * @return 删除的节点数量
     */
    size_type erase_bucket_to_node(size_type bucket, link_type last) noexcept {
        size_type count = 0;
        link_type curr = buckets_[bucket];
        while (curr != nullptr && curr != last) {
            link_type next = curr->next;
            hashtable::delete_node(curr);
            curr = next;
            --size_;
            ++count;
        }
        buckets_[bucket] = last;
        return count;
    }

    /**
     * @brief 删除桶中指定范围内的节点
     * @param bucket 桶索引
     * @param first 起始节点
     * @param last 结束节点（不删除）
     * @return 删除的节点数量
     */
    size_type erase_bucket_range(size_type bucket, link_type first, link_type last) noexcept {
        size_type count = 0;
        if (first == nullptr) return 0;

        if (buckets_[bucket] == first) {
            count += hashtable::erase_bucket_to_node(bucket, last);
        } else {
            link_type prev = buckets_[bucket];
            while (prev != nullptr && prev->next != first) {
                prev = prev->next;
            }
            if (prev == nullptr) return 0;

            link_type curr = first;
            while (curr != nullptr && curr != last) {
                link_type next = curr->next;
                prev->next = next;
                hashtable::delete_node(curr);
                curr = next;
                ++count;
            }
        }
        return count;
    }

    /**
     * @brief 删除桶中的所有节点
     * @param bucket 桶索引
     * @return 删除的节点数量
     */
    size_type erase_bucket_completely(size_type bucket) noexcept {
        size_type count = 0;
        link_type curr = buckets_[bucket];
        while (curr != nullptr) {
            link_type next = curr->next;
            hashtable::delete_node(curr);
            curr = next;
            ++count;
        }
        buckets_[bucket] = nullptr;
        return count;
    }

    /**
     * @brief 相等比较辅助函数（小数据量）
     * @param rhs 右侧哈希表
     * @return 是否相等
     */
    bool equal_small(const hashtable& rhs) const {
        for (const_iterator iter = begin(); iter != end(); ++iter) {
            const key_type& key = extracter_(*iter);

            const size_t count_lhs = _NEFORCE count_if(begin(), end(),
                [this, &key](const value_type& val) {
                    return equals_(extracter_(val), key);
                });
            const size_t count_rhs = _NEFORCE count_if(rhs.begin(), rhs.end(),
                [&rhs, &key](const value_type& val) {
                    return rhs.equals_(rhs.extracter_(val), key);
                });

            if (count_lhs != count_rhs) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief 相等比较辅助函数（大数据量）
     * @param rhs 右侧哈希表
     * @return 是否相等
     */
    bool equal_large(const hashtable& rhs) const {
        vector<value_type> elements_lhs, elements_rhs;
        elements_lhs.reserve(size_);
        elements_rhs.reserve(size_);

        for (const_iterator it = begin(); it != end(); ++it) {
            elements_lhs.push_back(*it);
        }
        for (const_iterator it = rhs.begin(); it != rhs.end(); ++it) {
            elements_rhs.push_back(*it);
        }

        _NEFORCE sort(elements_lhs.begin(), elements_lhs.end());
        _NEFORCE sort(elements_rhs.begin(), elements_rhs.end());

        return elements_lhs == elements_rhs;
    }

public:
    /**
     * @brief 构造函数
     * @param n 初始桶数量提示
     * @param max_lf 最大负载因子
     */
    explicit hashtable(const size_type n, float max_lf = 1.0f)
    : buckets_(next_size(n), nullptr),
      pair_(default_construct_tag{}, max_lf) {}

    /**
     * @brief 构造函数，指定哈希函数
     * @param n 初始桶数量提示
     * @param hf 哈希函数
     * @param max_lf 最大负载因子
     */
    hashtable(const size_type n, const HashFcn& hf, float max_lf = 1.0f)
    : buckets_(next_size(n), nullptr), hasher_(hf),
      pair_(default_construct_tag{}, max_lf) {}

    /**
     * @brief 构造函数，指定哈希函数和相等比较函数
     * @param n 初始桶数量提示
     * @param hf 哈希函数
     * @param eql 相等比较函数
     * @param max_lf 最大负载因子
     */
    hashtable(const size_type n, const HashFcn& hf, const EqualKey& eql, float max_lf = 1.0f)
    : buckets_(next_size(n), nullptr), hasher_(hf), equals_(eql),
      pair_(default_construct_tag{}, max_lf) {}

    /**
     * @brief 构造函数，指定所有函数对象
     * @param n 初始桶数量提示
     * @param hf 哈希函数
     * @param eql 相等比较函数
     * @param ext 值提取函数
     * @param max_lf 最大负载因子
     */
    hashtable(const size_type n, const HashFcn& hf, const EqualKey& eql, const ExtractKey& ext, float max_lf = 1.0f)
    : buckets_(next_size(n), nullptr), hasher_(hf), equals_(eql),
      extracter_(ext), pair_(default_construct_tag{}, max_lf) {}

    /**
     * @brief 拷贝构造函数
     * @param other 源哈希表
     */
    hashtable(const hashtable& other)
    : hasher_(other.hasher_), equals_(other.equals_),
      extracter_(other.extracter_), pair_(other.pair_) {
        hashtable::copy_from(other);
    }

    /**
     * @brief 拷贝赋值运算符
     * @param other 源哈希表
     * @return 自身引用
     */
    hashtable& operator =(const hashtable& other) {
        if (_NEFORCE addressof(other) == this) return *this;
        hashtable::clear();
        hasher_ = other.hasher_;
        equals_ = other.equals_;
        extracter_ = other.extracter_;
        hashtable::copy_from(other);
        return *this;
    }

    /**
     * @brief 移动构造函数
     * @param other 源哈希表
     */
    hashtable(hashtable&& other)
    noexcept(noexcept(hashtable::swap(other)))
    : buckets_(_NEFORCE move(other.buckets_)),
      size_(other.size_),
      hasher_(_NEFORCE move(other.hasher_)),
      equals_(_NEFORCE move(other.equals_)),
      extracter_(_NEFORCE move(other.extracter_)),
      pair_(_NEFORCE move(other.pair_)){
        other.size_ = 0;
    }

    /**
     * @brief 移动赋值运算符
     * @param other 源哈希表
     * @return 自身引用
     */
    hashtable& operator =(hashtable&& other)
    noexcept(noexcept(hashtable::swap(other))) {
        if (_NEFORCE addressof(other) == this) return *this;
        clear();
        hashtable::swap(other);
        return *this;
    }

    /**
     * @brief 析构函数
     */
    ~hashtable() {
        clear();
    }

    /**
     * @brief 获取起始迭代器
     * @return 指向第一个元素的迭代器
     */
    NEFORCE_NODISCARD iterator begin() noexcept {
        for (size_type n = 0; n < buckets_.size(); ++n) {
            if (buckets_[n] != nullptr) {
                return iterator(buckets_[n], n, this);
            }
        }
        return end();
    }

    /**
     * @brief 获取结束迭代器
     * @return 指向末尾的迭代器
     */
    NEFORCE_NODISCARD iterator end() noexcept {
        return iterator(nullptr, 0, this);
    }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个元素的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator begin() const noexcept {
        return cbegin();
    }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向末尾的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator end() const noexcept {
        return cend();
    }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个元素的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator cbegin() const noexcept {
        for (size_type n = 0; n < buckets_.size(); ++n) {
            if (buckets_[n] != nullptr) {
                return const_iterator(buckets_[n], n, this);
            }
        }
        return cend();
    }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向末尾的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator cend() const noexcept {
        return const_iterator(nullptr, 0, this);
    }

    /**
     * @brief 获取元素数量
     * @return 元素数量
     */
    NEFORCE_NODISCARD size_type size() const noexcept {
        return size_;
    }

    /**
     * @brief 获取最大可能大小
     * @return 最大元素数量
     */
    NEFORCE_NODISCARD size_type max_size() const noexcept {
        return static_cast<size_type>(-1);
    }

    /**
     * @brief 检查是否为空
     * @return 是否为空
     */
    NEFORCE_NODISCARD bool empty() const noexcept {
        return size_ == 0;
    }

    /**
     * @brief 获取桶数量
     * @return 桶数量
     */
    NEFORCE_NODISCARD size_type buckets_size() const noexcept {
        return buckets_.size();
    }

    /**
     * @brief 获取最大桶数量
     * @return 最大桶数量
     */
    NEFORCE_NODISCARD static size_type buckets_max_size() noexcept {
        return constants::HASH_PRIME_LIST[constants::HASH_PRIMER_COUNT - 1];
    }

    /**
     * @brief 获取键的桶索引
     * @param key 键
     * @return 桶索引
     */
    NEFORCE_NODISCARD size_type bucket_index(const key_type& key) const
    noexcept(is_nothrow_hashable_v<key_type>) {
        return hashtable::bucket_index_key(key);
    }

    /**
     * @brief 获取指定桶的大小
     * @param index 桶索引
     * @return 桶中的元素数量
     */
    NEFORCE_NODISCARD size_type bucket_size(size_type index) const noexcept {
        size_type result = 0;
        for (link_type cur = buckets_[index]; cur != nullptr; cur = cur->next) {
            result++;
        }
        return result;
    }

    /**
     * @brief 获取哈希函数对象
     * @return 哈希函数对象的副本
     */
    NEFORCE_NODISCARD hasher hash_func() const
    noexcept(is_nothrow_copy_constructible_v<hasher>) {
        return hasher_;
    }

    /**
     * @brief 获取键相等比较函数对象
     * @return 键相等比较函数对象的副本
     */
    NEFORCE_NODISCARD key_equal key_eql() const
    noexcept(is_nothrow_copy_constructible_v<key_equal>) {
        return equals_;
    }

    /**
     * @brief 获取当前负载因子
     * @return 负载因子（元素数量/桶数量）
     */
    NEFORCE_NODISCARD float load_factor() const noexcept {
        return buckets_size() == 0 ?
            0.0f :
            static_cast<float>(size()) / static_cast<float>(buckets_size());
    }

    /**
     * @brief 获取最大负载因子
     * @return 最大负载因子
     */
    NEFORCE_NODISCARD float max_load_factor() const noexcept {
        return pair_.value;
    }

    /**
     * @brief 设置最大负载因子
     * @param lf 新的最大负载因子
     */
    void max_load_factor(const float lf) noexcept {
        NEFORCE_DEBUG_VERIFY(lf > 0, "hashtable load factor invalid.");
        pair_.value = lf;
    }

    /**
     * @brief 重新哈希，调整桶数量
     * @param new_size 目标桶数量
     */
    void rehash(const size_type new_size) {
        const auto min_buckets_for_size = static_cast<size_type>(
            _NEFORCE ceil(static_cast<double>(size_) / max_load_factor()));
        const size_type target = _NEFORCE max(new_size, min_buckets_for_size);
        const size_type old_size = buckets_.size();

        if (target <= old_size) return;

        const size_type n = hashtable::next_size(target);
        if (n < target) {
            NEFORCE_THROW_EXCEPTION(value_exception("hashtable size exceeds max count"));
        }

        vector<link_type> new_buckets(n, nullptr);

        for (size_type bucket = 0; bucket < old_size; ++bucket) {
            link_type cur = buckets_[bucket];
            while (cur != nullptr) {
                link_type next = cur->next;
                const size_type new_bucket = hashtable::bucket_index_value(cur->data, n);

                cur->next = new_buckets[new_bucket];
                new_buckets[new_bucket] = cur;

                cur = next;
            }
            buckets_[bucket] = nullptr;
        }

        buckets_.swap(new_buckets);
    }

    /**
     * @brief 预留空间
     * @param n 期望的元素数量
     *
     * 确保哈希表至少能容纳n个元素而不触发rehash。
     */
    void reserve(const size_type n) {
        if (n <= size_) return;

        const size_type needed = static_cast<size_type>(
            _NEFORCE ceil(static_cast<double>(n) / max_load_factor()));

        if (needed > static_cast<float>(buckets_max_size())) {
            NEFORCE_THROW_EXCEPTION(value_exception("hashtable size exceeds max count"));
        }
        rehash(static_cast<size_type>(needed));
    }

    /**
     * @brief 构造元素（唯一键版本）
     * @tparam Args 构造参数类型
     * @param args 构造参数
     * @return 插入结果（迭代器和是否成功）
     */
    template <typename... Args>
    pair<iterator, bool> emplace_unique(Args&&... args) {
        if (size_ + 1 > static_cast<size_type>(buckets_.size() * max_load_factor())) {
            rehash(size_ + 1);
        }
        const link_type node = hashtable::new_node(_NEFORCE forward<Args>(args)...);
        return hashtable::insert_unique_noresize(node);
    }

    /**
     * @brief 构造元素（允许重复键版本）
     * @tparam Args 构造参数类型
     * @param args 构造参数
     * @return 指向插入元素的迭代器
     */
    template <typename... Args>
    iterator emplace_equal(Args&&... args) {
        if (size_ + 1 > static_cast<size_type>(buckets_.size() * max_load_factor())) {
            rehash(size_ + 1);
        }
        const link_type node = hashtable::new_node(_NEFORCE forward<Args>(args)...);
        return hashtable::insert_equal_noresize(node);
    }

    /**
     * @brief 插入元素（唯一键版本）
     * @param value 要插入的值
     * @return 插入结果（迭代器和是否成功）
     */
    pair<iterator, bool> insert_unique(const value_type& value) {
        return hashtable::emplace_unique(value);
    }

    /**
     * @brief 移动插入元素（唯一键版本）
     * @param value 要插入的值
     * @return 插入结果（迭代器和是否成功）
     */
    pair<iterator, bool> insert_unique(value_type&& value) {
        return hashtable::emplace_unique(_NEFORCE move(value));
    }

    /**
     * @brief 插入元素（允许重复键版本）
     * @param value 要插入的值
     * @return 指向插入元素的迭代器
     */
    iterator insert_equal(const value_type& value) {
        return hashtable::emplace_equal(value);
    }

    /**
     * @brief 移动插入元素（允许重复键版本）
     * @param value 要插入的值
     * @return 指向插入元素的迭代器
     */
    iterator insert_equal(value_type&& value) {
        return hashtable::emplace_equal(_NEFORCE move(value));
    }

    /**
     * @brief 范围插入元素（唯一键版本）
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator>
    enable_if_t<is_iter_v<Iterator>>
    insert_unique(Iterator first, Iterator last) {
        hashtable::insert_unique_aux(first, last);
        return;
    }

    /**
     * @brief 初始化列表插入元素（唯一键版本）
     * @param ilist 初始化列表
     */
    void insert_unique(std::initializer_list<value_type> ilist) {
        hashtable::insert_unique(ilist.begin(), ilist.end());
    }

    /**
     * @brief 范围插入元素（允许重复键版本）
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator>
    enable_if_t<is_iter_v<Iterator>>
    insert_equal(Iterator first, Iterator last) {
        hashtable::insert_equal_aux(first, last);
        return;
    }

    /**
     * @brief 初始化列表插入元素（允许重复键版本）
     * @param ilist 初始化列表
     */
    void insert_equal(std::initializer_list<value_type> ilist) {
        hashtable::insert_equal(ilist.begin(), ilist.end());
    }

    /**
     * @brief 删除所有具有指定键的元素
     * @param key 要删除的键
     * @return 删除的元素数量
     */
    size_type erase(const key_type& key)
    noexcept(is_nothrow_hashable_v<key_type>) {
        const size_type n = hashtable::bucket_index_key(key, buckets_.size());
        link_type first = buckets_[n];
        size_type erased = 0;

        if (first != nullptr) {
            link_type cur = first;
            link_type next = cur->next;

            while (next != nullptr) {
                if (equals_(extracter_(next->data), key)) {
                    cur->next = next->next;
                    hashtable::delete_node(next);
                    next = cur->next;
                    ++erased;
                    --size_;
                } else {
                    cur = next;
                    next = cur->next;
                }
            }

            if (equals_(extracter_(first->data), key)) {
                buckets_[n] = first->next;
                hashtable::delete_node(first);
                ++erased;
                --size_;
            }
        }

        return erased;
    }

    /**
     * @brief 删除指定位置的元素
     * @param position 要删除的位置
     * @return 指向被删除元素之后位置的迭代器
     */
    iterator erase(const iterator& position)
    noexcept(is_nothrow_hashable_v<key_type>) {
        if (position.current_ == nullptr || position.container_ != this) {
            return hashtable::end();
        }

        const size_type n = position.bucket_;
        link_type const p = position.current_;
        link_type next_node = p->next;

        link_type prev = nullptr;
        link_type curr = buckets_[n];
        while (curr != nullptr && curr != p) {
            prev = curr;
            curr = curr->next;
        }

        if (curr == nullptr) {
            return hashtable::end();
        }

        if (prev == nullptr) {
            buckets_[n] = next_node;
        } else {
            prev->next = next_node;
        }

        hashtable::delete_node(p);
        --size_;

        if (next_node != nullptr) {
            return iterator(next_node, n, this);
        }

        for (size_type bucket = n + 1; bucket < buckets_.size(); ++bucket) {
            if (buckets_[bucket] != nullptr) {
                return iterator(buckets_[bucket], bucket, this);
            }
        }
        return hashtable::end();
    }

    /**
     * @brief 删除指定范围内的元素
     * @param first 起始迭代器
     * @param last 结束迭代器
     * @return 指向最后一个被删除元素之后位置的迭代器
     */
    iterator erase(iterator first, iterator last)
    noexcept(is_nothrow_hashable_v<key_type>) {
        if (first == last) return last;

        if (first.container_ != this || (last.container_ != this && last != end())) {
            return hashtable::end();
        }

        size_type count_erased = 0;

        if (first.bucket_ == last.bucket_) {
            count_erased = hashtable::erase_bucket_range(
                first.bucket_, first.current_, last.current_);
        } else {
            count_erased += hashtable::erase_bucket_range(
                first.bucket_, first.current_, nullptr);
            for (size_type bucket = first.bucket_ + 1; bucket < last.bucket_; ++bucket) {
                count_erased += hashtable::erase_bucket_completely(bucket);
            }
            if (last.bucket_ < buckets_.size()) {
                count_erased += hashtable::erase_bucket_range(
                    last.bucket_, buckets_[last.bucket_], last.current_);
            }
        }
        size_ -= count_erased;
        return last;
    }

    /**
     * @brief 删除指定位置的元素（常量迭代器版本）
     * @param position 要删除的位置
     * @return 指向被删除元素之后位置的常量迭代器
     */
    const_iterator erase(const const_iterator& position)
    noexcept(is_nothrow_hashable_v<key_type>) {
        return hashtable::erase(iterator(position));
    }

    /**
     * @brief 删除指定范围内的元素（常量迭代器版本）
     * @param first 起始迭代器
     * @param last 结束迭代器
     * @return 指向最后一个被删除元素之后位置的常量迭代器
     */
    const_iterator erase(const_iterator first, const_iterator last)
    noexcept(is_nothrow_hashable_v<key_type>) {
        return hashtable::erase(iterator(first), iterator(last));
    }

    /**
     * @brief 清空哈希表
     */
    void clear() noexcept {
        for (size_type i = 0; i < buckets_.size(); ++i) {
            link_type cur = buckets_[i];
            while (cur != nullptr) {
                link_type next = cur->next;
                hashtable::delete_node(cur);
                cur = next;
            }
            buckets_[i] = nullptr;
        }
        size_ = 0;
    }

    /**
     * @brief 查找具有指定键的元素
     * @param key 要查找的键
     * @return 指向第一个匹配元素的迭代器，未找到则返回end()
     */
    NEFORCE_NODISCARD iterator find(const key_type& key)
    noexcept(is_nothrow_hashable_v<key_type>) {
        if (buckets_.empty()) return hashtable::end();

        size_type n = hashtable::bucket_index_key(key, buckets_.size());
        for (link_type first = buckets_[n]; first != nullptr; first = first->next) {
            if (equals_(extracter_(first->data), key)) {
                return iterator(first, n, this);
            }
        }
        return hashtable::end();
    }

    /**
     * @brief 查找具有指定键的元素（常量版本）
     * @param key 要查找的键
     * @return 指向第一个匹配元素的常量迭代器，未找到则返回cend()
     */
    NEFORCE_NODISCARD const_iterator find(const key_type& key) const
    noexcept(is_nothrow_hashable_v<key_type>) {
        if (buckets_.empty()) return hashtable::cend();

        size_type n = hashtable::bucket_index_key(key, buckets_.size());
        for (link_type first = buckets_[n]; first != nullptr; first = first->next) {
            if (equals_(extracter_(first->data), key)) {
                return const_iterator(first, n, this);
            }
        }
        return hashtable::cend();
    }

    /**
     * @brief 统计具有指定键的元素数量
     * @param key 要统计的键
     * @return 匹配的元素数量
     */
    NEFORCE_NODISCARD size_type count(const key_type& key) const
    noexcept(is_nothrow_hashable_v<key_type>) {
        if (buckets_.empty()) return 0;
        const size_type n = hashtable::bucket_index_key(key, buckets_.size());
        size_type result = 0;
        for (link_type cur = buckets_[n]; cur != nullptr; cur = cur->next) {
            if (equals_(extracter_(cur->data), key)) ++result;
        }
        return result;
    }

    /**
     * @brief 检查是否包含指定键
     * @param key 要检查的键
     * @return 是否包含
     */
    NEFORCE_NODISCARD bool contains(const key_type& key) const
    noexcept(is_nothrow_hashable_v<key_type>) {
        return hashtable::find(key) != cend();
    }

    /**
     * @brief 获取等于指定键的元素范围
     * @param key 键值
     * @return 包含范围起始和结束的pair
     */
    NEFORCE_NODISCARD pair<iterator, iterator> equal_range(const key_type& key) {
        if (buckets_.empty()) {
            return {
                hashtable::end(),
                hashtable::end()
            };
        }

        const size_type n = hashtable::bucket_index_key(key, buckets_.size());
        link_type first_match = nullptr;
        link_type last_match = nullptr;

        for (link_type curr = buckets_[n]; curr != nullptr; curr = curr->next) {
            if (equals_(extracter_(curr->data), key)) {
                if (first_match == nullptr) {
                    first_match = curr;
                }
                last_match = curr;
            } else if (first_match != nullptr) {
                break;
            }
        }

        if (first_match == nullptr) {
            return {
                hashtable::end(),
                hashtable::end()
            };
        }

        link_type range_end = (last_match != nullptr) ? last_match->next : nullptr;
        return {
            iterator(first_match, n, this),
            iterator(range_end, n, this)
        };
    }

    /**
     * @brief 获取等于指定键的元素范围（常量版本）
     * @param key 键值
     * @return 包含范围起始和结束的pair
     */
    NEFORCE_NODISCARD pair<const_iterator, const_iterator> equal_range(const key_type& key) const {
        if (buckets_.empty()) return {cend(), cend()};

        const size_type n = hashtable::bucket_index_key(key, buckets_.size());
        const link_type first_match = nullptr;
        const link_type last_match = nullptr;

        for (const link_type curr = buckets_[n]; curr != nullptr; curr = curr->next) {
            if (equals_(extracter_(curr->data), key)) {
                if (first_match == nullptr) {
                    first_match = curr;
                }
                last_match = curr;
            } else if (first_match != nullptr) {
                break;
            }
        }

        if (first_match == nullptr) {
            return {
                hashtable::cend(),
                hashtable::cend()
            };
        }

        const link_type range_end = (last_match != nullptr) ? last_match->next : nullptr;
        return {
            const_iterator(first_match, n, this),
            const_iterator(range_end, n, this)
        };
    }

    /**
     * @brief 交换两个哈希表的内容
     * @param other 要交换的另一个哈希表
     */
    void swap(hashtable& other)
    noexcept(is_nothrow_swappable_v<HashFcn> &&
             is_nothrow_swappable_v<EqualKey>&&
             is_nothrow_swappable_v<allocator_type>) {
        if (_NEFORCE addressof(other) == this) return;
        _NEFORCE swap(hasher_, other.hasher_);
        _NEFORCE swap(equals_, other.equals_);
        _NEFORCE swap(extracter_, other.extracter_);
        buckets_.swap(other.buckets_);
        _NEFORCE swap(size_, other.size_);
        pair_.swap(other.pair_);
    }

    /**
     * @brief 相等比较操作符
     * @param rhs 右侧哈希表
     * @return 如果两个哈希表大小相等且对应元素相等返回true
     */
    NEFORCE_NODISCARD bool operator ==(const hashtable& rhs) const {
        if (size_ != rhs.size_) return false;
        if (size_ == 0) return true;
        if (this == &rhs) return true;

        if (size_ < 100) return hashtable::equal_small(rhs);
        return hashtable::equal_large(rhs);
    }

    /**
     * @brief 小于比较操作符
     * @param rhs 右侧哈希表
     * @return 按字典序比较结果
     */
    NEFORCE_NODISCARD bool operator <(const hashtable& rhs) const
    noexcept(noexcept(_NEFORCE lexicographical_compare(hashtable::cbegin(), hashtable::cend(), rhs.cbegin(), rhs.cend()))) {
        return _NEFORCE lexicographical_compare(hashtable::cbegin(), hashtable::cend(), rhs.cbegin(), rhs.cend());
    }
};

/** @} */ // HashTable

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_CONTAINER_HASHTABLE_HPP__
