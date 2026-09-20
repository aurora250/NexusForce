#ifndef NEFORCE_CORE_CONTAINER_FLAT_HASHTABLE_HPP__
#define NEFORCE_CORE_CONTAINER_FLAT_HASHTABLE_HPP__

/**
 * @file flat_hashtable.hpp
 * @brief 平坦哈希表容器
 *
 * 此文件提供了基于开放寻址法的平坦哈希表实现。
 * 采用 SwissTable 风格的元数据控制块设计，通过 H2 预过滤和 SIMD 批量探测
 * 提供平均常数时间复杂度的插入、删除和查找操作。
 * 作为平坦无序关联式容器的底层实现。
 */

#include "NeForce/core/algorithm/sort.hpp"
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/interface/icollector.hpp"
#include "NeForce/core/interface/iiterator.hpp"
#include "NeForce/core/memory/allocator_traits.hpp"
#include "NeForce/core/memory/construct.hpp"
#include "NeForce/core/memory/bit.hpp"
#include "NeForce/core/simd/bytes.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup FlatHashTable 平坦哈希表
 * @brief 基于开放寻址法的平坦哈希表实现
 *
 * 平坦哈希表采用开放寻址法（Open Addressing）处理冲突，
 * 使用元数据控制块实现 H2 预过滤，大幅减少对数据数组的内存访问。
 *
 * @section flat_hash_metadata 元数据控制块
 *
 * 每个 slot 对应 1 字节元数据，独立于数据数组存储：
 *
 * | 状态               | 值      | 含义                       |
 * |--------------------|---------|----------------------------|
 * | EMPTY              | 0x80    | 从未使用                    |
 * | DELETED (tombstone)| 0xFE    | 已删除，可复用              |
 * | H2 tag             | [0,127] | 7-bit 哈希标签              |
 *
 * @section flat_hash_probe 探测策略
 *
 * 使用线性探测：idx = (idx + 1) & (capacity - 1)。
 * 通过元数据字节的 H2 标签预过滤，仅当 H2 匹配时才访问数据数组进行完整键比较。
 * 支持 SSE2 批量探测：一次加载 16 字节元数据，单指令比较 16 个 slot。
 *
 * @section flat_hash_complexity 时间复杂度
 *
 * | 操作     | 平均情况         | 最坏情况     |
 * |----------|------------------|--------------|
 * | insert   | O(1)             | O(n)         |
 * | erase    | O(1)             | O(n)         |
 * | find     | O(1)             | O(n)         |
 * | rehash   | O(n)             | O(n)         |
 *
 * @section flat_hash_iterator 迭代器失效规则
 *
 * | 操作     | 失效范围                       |
 * |----------|-------------------------------|
 * | insert   | rehash 时全部失效              |
 * | erase    | 仅失效指向被删除元素的迭代器    |
 * | rehash   | 全部迭代器失效                 |
 * | clear    | 全部迭代器失效                 |
 * @{
 */

template <typename Value, typename Key, typename HashFcn, typename ExtractKey, typename EqualKey, typename Alloc>
class flat_hashtable;

/**
 * @struct flat_hashtable_iterator
 * @brief 平坦哈希表迭代器
 * @tparam IsConst 是否常量迭代器
 * @tparam FlatHT 平坦哈希表类型
 *
 * 提供对平坦哈希表元素的前向迭代访问。
 */
template <bool IsConst, typename FlatHT>
struct flat_hashtable_iterator : iiterator<flat_hashtable_iterator<IsConst, FlatHT>> {
public:
    using container_type = FlatHT;                                    ///< 容器类型
    using value_type = typename container_type::value_type;           ///< 值类型
    using size_type = typename container_type::size_type;             ///< 大小类型
    using difference_type = typename container_type::difference_type; ///< 差值类型
    using iterator_category = forward_iterator_tag;                   ///< 前向迭代器
    using reference = conditional_t<IsConst, typename container_type::const_reference,
                                    typename container_type::reference>; ///< 引用类型
    using pointer = conditional_t<IsConst, typename container_type::const_pointer,
                                  typename container_type::pointer>; ///< 指针类型

private:
    size_type index_ = 0;                       ///< 当前 slot 索引
    const container_type* container_ = nullptr; ///< 关联容器指针
    int mask_ = -1;                             ///< 当前组内 index_ 之后的占用位掩码，-1 表示未缓存

    template <typename, typename, typename, typename, typename, typename>
    friend class flat_hashtable;

public:
    flat_hashtable_iterator() noexcept = default;
    ~flat_hashtable_iterator() = default;

    flat_hashtable_iterator(const flat_hashtable_iterator&) noexcept = default;
    flat_hashtable_iterator& operator=(const flat_hashtable_iterator&) noexcept = default;
    flat_hashtable_iterator(flat_hashtable_iterator&&) noexcept = default;
    flat_hashtable_iterator& operator=(flat_hashtable_iterator&&) noexcept = default;

    /**
     * @brief 构造函数
     * @param index slot 索引
     * @param container 容器指针
     */
    flat_hashtable_iterator(const size_type index, const container_type* container) noexcept :
    index_(index),
    container_(container) {}

    /**
     * @brief 解引用操作
     * @return 当前元素的引用
     */
    NEFORCE_NODISCARD reference dereference() const noexcept {
        NEFORCE_DEBUG_VERIFY(container_ != nullptr, "null container in flat_hashtable_iterator");
        NEFORCE_DEBUG_VERIFY(index_ < container_->capacity_, "index out of range in flat_hashtable_iterator");
        return container_->data_[index_];
    }

    /**
     * @brief 递增操作
     */
    void increment() noexcept {
        NEFORCE_DEBUG_VERIFY(container_ != nullptr, "null container in flat_hashtable_iterator");
        const size_type capacity = container_->capacity_;
        const size_type next = index_ + 1;
        const size_type offset = next & (container_type::FLAT_HT_GROUP - 1);
        const size_type base = next - offset;

        if (mask_ < 0 || offset == 0) {
            if (next >= capacity) {
                index_ = capacity;
                mask_ = -1;
                return;
            }
            mask_ = container_type::occupied_mask(container_->metadata_, base) & ~((1 << offset) - 1);
        }

        if (mask_ != 0) {
            index_ = base + static_cast<size_type>(countr_zero(static_cast<uintptr_t>(static_cast<unsigned>(mask_))));
            mask_ &= mask_ - 1;
            if (index_ >= capacity || !container_type::is_taken(container_->metadata_[index_])) {
                mask_ = -1;
                index_ = container_type::find_occupied(container_->metadata_, capacity, index_);
            }
            return;
        }

        index_ = container_type::find_occupied(container_->metadata_, capacity, base + container_type::FLAT_HT_GROUP);
        mask_ = -1;
    }

    /**
     * @brief 相等比较
     * @param rhs 右侧迭代器
     * @return 是否相等
     */
    NEFORCE_NODISCARD bool equal_to(const flat_hashtable_iterator& rhs) const noexcept {
        NEFORCE_DEBUG_VERIFY(container_ == rhs.container_, "comparing iterators from different containers");
        return index_ == rhs.index_;
    }

    /**
     * @brief 获取 slot 索引
     * @return 当前索引
     */
    NEFORCE_NODISCARD size_type index() const noexcept { return index_; }

    /**
     * @brief 获取关联容器
     * @return 容器指针
     */
    NEFORCE_NODISCARD const container_type* container() const noexcept { return container_; }
};

/**
 * @class flat_hashtable
 * @brief 平坦哈希表容器
 * @tparam Value 值类型
 * @tparam Key 键类型
 * @tparam HashFcn 哈希函数类型
 * @tparam ExtractKey 从值中提取键的函数对象类型
 * @tparam EqualKey 键相等比较函数对象类型
 * @tparam Alloc 分配器类型（用于 Value 分配）
 *
 * 平坦哈希表使用开放寻址法处理冲突，采用元数据控制块实现 H2 预过滤，
 * 提供平均常数时间复杂度的插入、删除和查找操作。
 */
template <typename Value, typename Key, typename HashFcn, typename ExtractKey, typename EqualKey, typename Alloc>
class flat_hashtable : public icollector<flat_hashtable<Value, Key, HashFcn, ExtractKey, EqualKey, Alloc>> {
public:
    using key_type = Key;       ///< 键类型
    using hasher = HashFcn;     ///< 哈希函数类型
    using key_equal = EqualKey; ///< 键相等比较函数类型
    using value_type = Value;   ///< 值类型

    using pointer = Value*;               ///< 指针类型
    using reference = Value&;             ///< 引用类型
    using const_pointer = const Value*;   ///< 常量指针类型
    using const_reference = const Value&; ///< 常量引用类型
    using size_type = size_t;             ///< 大小类型
    using difference_type = ptrdiff_t;    ///< 差值类型

    using iterator = flat_hashtable_iterator<false, flat_hashtable>;      ///< 迭代器类型
    using const_iterator = flat_hashtable_iterator<true, flat_hashtable>; ///< 常量迭代器类型

    using allocator_type = Alloc; ///< 分配器类型

    /// 元数据数组使用的重绑定分配器
    using byte_allocator_type = typename allocator_traits<allocator_type>::template rebind_alloc<byte_t>;

    static constexpr byte_t FLAT_HT_EMPTY = 0x80;   ///< EMPTY 元数据标记
    static constexpr byte_t FLAT_HT_DELETED = 0xFE; ///< DELETED 元数据标记
    static constexpr byte_t FLAT_HT_H2_MASK = 0x7F; ///< H2 标签位掩码
    static constexpr size_t FLAT_HT_GROUP = 16;     ///< SIMD 组扫描宽度
    static constexpr size_t FLAT_HT_PAD = 16;       ///< 元数据尾部哨兵字节数
    static constexpr size_t npos = static_cast<size_t>(-1);

private:
    Value* data_ = nullptr;       ///< 数据数组
    byte_t* metadata_ = nullptr;  ///< 元数据字节数组
    size_t capacity_ = 0;         ///< slot 总数
    size_t size_ = 0;             ///< 存活元素数
    size_t growth_threshold_ = 0; ///< 触发扩容的元素数上限
    size_t deleted_ = 0;          ///< DELETED 槽数量
    hasher hasher_{};             ///< 哈希函数对象
    key_equal equals_{};          ///< 键相等比较对象
    ExtractKey extracter_{};      ///< 值提取键对象

    compressed_pair<allocator_type, float> alloc_lf_{default_construct_tag{}, 0.875F}; ///< 分配器与最大负载因子

    template <bool, typename>
    friend struct flat_hashtable_iterator;

private:
    /**
     * @brief 计算不小于 n 的下一个 2 的幂
     * @param n 参考值
     * @return 2 的幂
     */
    NEFORCE_ALWAYS_INLINE_INLINE static size_t next_power_of_2(const size_t n) noexcept {
        if (n <= 16) {
            return 16;
        }
        return static_cast<size_t>(bit_ceil(static_cast<uintptr_t>(n)));
    }

    /**
     * @brief 由容量与最大负载因子推导扩容阈值
     * @param capacity 容量
     * @param lf 最大负载因子
     * @return 触发扩容的元素数上限
     */
    NEFORCE_ALWAYS_INLINE_INLINE static size_t threshold_of(const size_t capacity, const float lf) noexcept {
        return static_cast<size_t>(static_cast<float>(capacity) * lf);
    }

    /**
     * @brief 判断元数据标记是否指向占用槽
     * @param meta 元数据字节
     * @return 该槽位是否存活
     */
    NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE_INLINE static constexpr bool is_taken(const byte_t meta) noexcept {
        return meta < FLAT_HT_EMPTY;
    }

    /**
     * @brief 读取一个元数据组的占用位掩码
     * @param metadata 元数据数组
     * @param base 组起始索引
     * @return 16 位掩码，bit i 置位表示 base+i 槽位被占用
     */
    NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE_INLINE static int occupied_mask(const byte_t* metadata,
                                                                            const size_type base) noexcept {
#ifdef NEFORCE_SIMD_SSE2
        return (~simd::to_bitmask(simd::load_unaligned(metadata + base))) & 0xFFFF;
#else
        int mask = 0;
        for (size_t i = 0; i < FLAT_HT_GROUP; ++i) {
            const byte_t meta = metadata[base + i];
            if (meta != FLAT_HT_EMPTY && meta != FLAT_HT_DELETED) {
                mask |= (1 << i);
            }
        }
        return mask;
#endif
    }

    /**
     * @brief 从指定位置开始查找首个占用槽
     * @param metadata 元数据数组
     * @param capacity 容量
     * @param from 起始位置
     * @return 首个占用槽的索引，未找到时返回 capacity
     */
    NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE_INLINE static size_type
    find_occupied(const byte_t* metadata, const size_type capacity, size_type from) noexcept {
        while (from < capacity) {
            const size_type offset = from & (FLAT_HT_GROUP - 1);
            const size_type base = from - offset;
            const int mask = occupied_mask(metadata, base) & ~((1 << offset) - 1);
            if (mask != 0) {
                return base + static_cast<size_type>(countr_zero(static_cast<uintptr_t>(static_cast<unsigned>(mask))));
            }
            from = base + FLAT_HT_GROUP;
        }
        return capacity;
    }

    /**
     * @brief 从哈希值计算初始探测索引（H1）
     * @param hash 哈希值
     * @return 初始槽位索引
     */
    NEFORCE_NODISCARD size_t hash_to_index(const size_t hash) const noexcept { return (hash >> 7) & (capacity_ - 1); }

    /**
     * @brief 从哈希值提取 7-bit 标签（H2）
     * @param hash 哈希值
     * @return H2 标签 [0, 127]
     */
    static byte_t hash_to_h2(const size_t hash) noexcept { return static_cast<byte_t>(hash & FLAT_HT_H2_MASK); }

    /**
     * @brief 获取分配器
     * @return 分配器引用
     */
    allocator_type& get_allocator() noexcept { return alloc_lf_.get_base(); }

    /**
     * @brief 分配元数据数组
     * @param capacity 容量
     * @return 元数据数组指针，尾部含 FLAT_HT_GROUP 个 EMPTY 哨兵字节
     * @throws memory_exception 元数据分配失败
     */
    NEFORCE_NODISCARD byte_t* allocate_metadata(const size_t capacity) {
        byte_allocator_type alloc(get_allocator());
        byte_t* mem = nullptr;
        try {
            mem = alloc.allocate(capacity + FLAT_HT_PAD);
        } catch (...) {
            NEFORCE_THROW_EXCEPTION(memory_exception("flat_hashtable metadata allocation failed"));
        }
        if (mem == nullptr) {
            NEFORCE_THROW_EXCEPTION(memory_exception("flat_hashtable metadata allocation failed"));
        }
        for (size_t i = 0; i < capacity + FLAT_HT_PAD; ++i) {
            mem[i] = FLAT_HT_EMPTY;
        }
        return mem;
    }

    /**
     * @brief 释放元数据数组
     * @param mem 元数据数组指针
     * @param capacity 该数组对应的容量
     */
    void free_metadata(byte_t* mem, const size_t capacity) noexcept {
        if (mem == nullptr) {
            return;
        }
        byte_allocator_type alloc(get_allocator());
        alloc.deallocate(mem, capacity + FLAT_HT_PAD);
    }

    /**
     * @brief 分配存储数组
     * @param cap 容量
     */
    void alloc_arrays(const size_t cap) {
        if (cap == 0) {
            return;
        }
        free_arrays();
        capacity_ = 0;
        size_ = 0;
        deleted_ = 0;
        growth_threshold_ = 0;

        allocator_type& alloc = get_allocator();
        data_ = alloc.allocate(cap);
        try {
            metadata_ = allocate_metadata(cap);
        } catch (...) {
            alloc.deallocate(data_, cap);
            data_ = nullptr;
            throw;
        }
        capacity_ = cap;
        growth_threshold_ = threshold_of(capacity_, max_load_factor());
    }

    /**
     * @brief 释放存储数组
     */
    void free_arrays() noexcept {
        if (data_ && capacity_ > 0) {
            for (size_t i = 0; i < capacity_; ++i) {
                if (metadata_[i] != FLAT_HT_EMPTY && metadata_[i] != FLAT_HT_DELETED) {
                    _NEFORCE destroy(&data_[i]);
                }
            }
            allocator_type& alloc = get_allocator();
            alloc.deallocate(data_, capacity_);
            data_ = nullptr;
        }
        if (metadata_ != nullptr) {
            free_metadata(metadata_, capacity_);
            metadata_ = nullptr;
        }
    }

    /**
     * @brief 加载一个元数据组，跨数组尾部时按环状顺序拼装
     * @param idx 组起始索引
     * @param group 输出的组缓冲
     */
    void load_meta_group(const size_t idx, byte_t* group) const noexcept {
        const size_t remaining = capacity_ - idx;
        if (remaining >= FLAT_HT_GROUP) {
            _NEFORCE memory_copy(group, metadata_ + idx, FLAT_HT_GROUP);
            return;
        }
        for (size_t k = 0; k < remaining; ++k) {
            group[k] = metadata_[idx + k];
        }
        for (size_t k = 0; k < FLAT_HT_GROUP - remaining; ++k) {
            group[remaining + k] = metadata_[k];
        }
    }

    /**
     * @brief 标量探测键或插入位置
     * @param hash 键的哈希值
     * @param key 要查找的键
     * @param h2 H2 标签
     * @return {slot_index, found}
     */
    pair<size_t, bool> probe_find_or_insert(const size_t hash, const key_type& key, const byte_t h2) const noexcept {
        size_t idx = hash_to_index(hash);
        size_t first_deleted = npos;

        for (size_t i = 0; i < capacity_; ++i) {
            const byte_t meta = metadata_[idx];
            if (meta == FLAT_HT_EMPTY) {
                return {first_deleted != npos ? first_deleted : idx, false};
            }
            if (meta == FLAT_HT_DELETED) {
                if (first_deleted == npos) {
                    first_deleted = idx;
                }
            } else if (meta == h2 && equals_(extracter_(data_[idx]), key)) {
                return {idx, true};
            }
            idx = (idx + 1) & (capacity_ - 1);
        }
        return {first_deleted, false};
    }

    /**
     * @brief SIMD 探测键或插入位置
     * @param hash 键的哈希值
     * @param key 要查找的键
     * @param h2 H2 标签
     * @return {slot_index, found}
     */
    pair<size_t, bool> probe_find_or_insert_simd(const size_t hash, const key_type& key,
                                                 const byte_t h2) const noexcept {
        const size_t h1 = hash_to_index(hash);
        size_t idx = h1;
        size_t first_deleted = npos;

        const simd::vec128_t h2_vec = simd::fill_i8(h2);
        const simd::vec128_t empty_vec = simd::fill_i8(FLAT_HT_EMPTY);
        const simd::vec128_t deleted_vec = simd::fill_i8(FLAT_HT_DELETED);

        for (size_t round = 0; round < capacity_; round += FLAT_HT_GROUP) {
            // wrap-around safe load: metadata array is exactly capacity_ bytes,
            // a 16-byte load from idx may cross the array boundary
            byte_t buf[FLAT_HT_GROUP];
            simd::vec128_t meta_vec;
            if (capacity_ - idx >= FLAT_HT_GROUP) {
                meta_vec = simd::load_unaligned(metadata_ + idx);
            } else {
                load_meta_group(idx, buf);
                meta_vec = simd::load_unaligned(buf);
            }

            const int h2_mask = simd::to_bitmask(simd::match_bytes(meta_vec, h2_vec));
            const int empty_mask = simd::to_bitmask(simd::match_bytes(meta_vec, empty_vec));
            const int deleted_mask = simd::to_bitmask(simd::match_bytes(meta_vec, deleted_vec));

            int match = h2_mask;
            while (match != 0) {
                const int bit = countr_zero(static_cast<uintptr_t>(match));
                const size_t slot = (idx + bit) & (capacity_ - 1);
                if (equals_(extracter_(data_[slot]), key)) {
                    return {slot, true};
                }
                match &= (match - 1);
            }

            if (empty_mask != 0) {
                const int empty_bit = countr_zero(static_cast<uintptr_t>(empty_mask));
                const size_t empty_slot = (idx + empty_bit) & (capacity_ - 1);
                if (first_deleted != npos) {
                    const size_t probe_dist_empty =
                            (empty_slot >= h1) ? (empty_slot - h1) : (capacity_ - h1 + empty_slot);
                    const size_t probe_dist_del =
                            (first_deleted >= h1) ? (first_deleted - h1) : (capacity_ - h1 + first_deleted);
                    if (probe_dist_del < probe_dist_empty) {
                        return {first_deleted, false};
                    }
                }
                return {empty_slot, false};
            }

            const int del = deleted_mask;
            if ((del != 0) && first_deleted == npos) {
                const int del_bit = countr_zero(static_cast<uintptr_t>(del));
                first_deleted = (idx + del_bit) & (capacity_ - 1);
            }

            idx = (idx + FLAT_HT_GROUP) & (capacity_ - 1);
        }
        return {first_deleted, false};
    }

    /**
     * @brief 查找键所在槽位
     * @param hash 键的哈希值
     * @param key 要查找的键
     * @param h2 H2 标签
     * @return 槽位索引，未找到时返回 npos
     */
    NEFORCE_NODISCARD size_type probe_find(const size_t hash, const key_type& key, const byte_t h2) const noexcept {
        size_t idx = hash_to_index(hash);
#ifdef NEFORCE_SIMD_SSE2
        const simd::vec128_t h2_vec = simd::fill_i8(h2);
        const simd::vec128_t empty_vec = simd::fill_i8(FLAT_HT_EMPTY);

        for (size_t round = 0; round < capacity_; round += FLAT_HT_GROUP) {
            byte_t buf[FLAT_HT_GROUP];
            simd::vec128_t meta_vec;
            if (capacity_ - idx >= FLAT_HT_GROUP) {
                meta_vec = simd::load_unaligned(metadata_ + idx);
            } else {
                load_meta_group(idx, buf);
                meta_vec = simd::load_unaligned(buf);
            }

            int match = simd::to_bitmask(simd::match_bytes(meta_vec, h2_vec));
            const int empty_mask = simd::to_bitmask(simd::match_bytes(meta_vec, empty_vec));

            while (match != 0) {
                const int bit = countr_zero(static_cast<uintptr_t>(match));
                const size_t slot = (idx + bit) & (capacity_ - 1);
                if (equals_(extracter_(data_[slot]), key)) {
                    return slot;
                }
                match &= (match - 1);
            }

            if (empty_mask != 0) {
                return npos;
            }
            idx = (idx + FLAT_HT_GROUP) & (capacity_ - 1);
        }
        return npos;
#else
        for (size_t i = 0; i < capacity_; ++i) {
            const byte_t meta = metadata_[idx];
            if (meta == FLAT_HT_EMPTY) {
                return npos;
            }
            if (meta == h2 && equals_(extracter_(data_[idx]), key)) {
                return idx;
            }
            idx = (idx + 1) & (capacity_ - 1);
        }
        return npos;
#endif
    }

    /**
     * @brief 统计探测链上匹配键的元素数量
     * @param hash 键的哈希值
     * @param key 要统计的键
     * @param h2 H2 标签
     * @return 匹配的元素数量
     */
    NEFORCE_NODISCARD size_type probe_count(const size_t hash, const key_type& key, const byte_t h2) const noexcept {
        size_t idx = hash_to_index(hash);
        size_type result = 0;
#ifdef NEFORCE_SIMD_SSE2
        const simd::vec128_t h2_vec = simd::fill_i8(h2);
        const simd::vec128_t empty_vec = simd::fill_i8(FLAT_HT_EMPTY);

        for (size_t round = 0; round < capacity_; round += FLAT_HT_GROUP) {
            byte_t buf[FLAT_HT_GROUP];
            simd::vec128_t meta_vec;
            if (capacity_ - idx >= FLAT_HT_GROUP) {
                meta_vec = simd::load_unaligned(metadata_ + idx);
            } else {
                load_meta_group(idx, buf);
                meta_vec = simd::load_unaligned(buf);
            }

            int match = simd::to_bitmask(simd::match_bytes(meta_vec, h2_vec));
            const int empty_mask = simd::to_bitmask(simd::match_bytes(meta_vec, empty_vec));
            const int stop =
                    empty_mask != 0 ? countr_zero(static_cast<uintptr_t>(empty_mask)) : static_cast<int>(FLAT_HT_GROUP);
            match &= (stop >= static_cast<int>(FLAT_HT_GROUP)) ? 0xFFFF : ((1 << stop) - 1);

            while (match != 0) {
                const int bit = countr_zero(static_cast<uintptr_t>(match));
                const size_t slot = (idx + bit) & (capacity_ - 1);
                if (equals_(extracter_(data_[slot]), key)) {
                    ++result;
                }
                match &= (match - 1);
            }

            if (empty_mask != 0) {
                return result;
            }
            idx = (idx + FLAT_HT_GROUP) & (capacity_ - 1);
        }
        return result;
#else
        for (size_t i = 0; i < capacity_; ++i) {
            const byte_t meta = metadata_[idx];
            if (meta == FLAT_HT_EMPTY) {
                return result;
            }
            if (meta == h2 && equals_(extracter_(data_[idx]), key)) {
                ++result;
            }
            idx = (idx + 1) & (capacity_ - 1);
        }
        return result;
#endif
    }

    /**
     * @brief 计算是否需要扩容
     * @return 是否需要扩容
     */
    NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE_INLINE bool should_rehash() const noexcept {
        return size_ + 1 > growth_threshold_;
    }

    /**
     * @brief 判断墓碑是否已经多到需要原地重建
     * @return 是否需要清理墓碑
     */
    NEFORCE_NODISCARD bool needs_purge() const noexcept {
        return deleted_ != 0 && size_ + deleted_ >= growth_threshold_;
    }

    /**
     * @brief 重新哈希
     * @param min_capacity 最小目标容量
     * @param purge_only 只清理墓碑，不允许扩容
     */
    void rehash_impl(const size_t min_capacity, const bool purge_only = false) {
        const size_t needed = max(min_capacity, static_cast<size_t>(static_cast<double>(size_) / max_load_factor()));
        const size_t new_capacity = next_power_of_2(needed);
        if (new_capacity < capacity_ || (new_capacity == capacity_ && !purge_only)) {
            return;
        }

        allocator_type& alloc = get_allocator();
        Value* new_data = alloc.allocate(new_capacity);
        byte_t* new_metadata = nullptr;
        try {
            new_metadata = allocate_metadata(new_capacity);
        } catch (...) {
            alloc.deallocate(new_data, new_capacity);
            throw;
        }

        const size_t old_capacity = capacity_;
        Value* const old_data = data_;
        byte_t* const old_metadata = metadata_;

        try {
            for (size_t i = 0; i < old_capacity; ++i) {
                if (old_metadata[i] != FLAT_HT_EMPTY && old_metadata[i] != FLAT_HT_DELETED) {
                    const key_type& key = extracter_(old_data[i]);
                    const size_t hash = hasher_(key);
                    const byte_t h2 = hash_to_h2(hash);
                    const size_t h1 = (hash >> 7) & (new_capacity - 1);

                    size_t new_idx = h1;
                    bool placed = false;
                    for (size_t j = 0; j < new_capacity; ++j) {
                        const byte_t nm = new_metadata[new_idx];
                        if (nm == FLAT_HT_EMPTY) {
                            break;
                        }
                        if (nm == h2 && equals_(extracter_(new_data[new_idx]), key)) {
                            size_t run_end = new_idx;
                            for (size_t k = j + 1; k < new_capacity; ++k) {
                                run_end = (run_end + 1) & (new_capacity - 1);
                                const byte_t rmn = new_metadata[run_end];
                                if (rmn == FLAT_HT_EMPTY) {
                                    new_idx = run_end;
                                    placed = true;
                                    break;
                                }
                                if (rmn == h2 && equals_(extracter_(new_data[run_end]), key)) {
                                    continue;
                                }
                                size_t shift_end = run_end;
                                for (size_t m = 0; m < new_capacity; ++m) {
                                    shift_end = (shift_end + 1) & (new_capacity - 1);
                                    if (new_metadata[shift_end] == FLAT_HT_EMPTY) {
                                        break;
                                    }
                                }
                                while (shift_end != run_end) {
                                    const size_t prev = (shift_end - 1) & (new_capacity - 1);
                                    _NEFORCE construct(&new_data[shift_end], _NEFORCE move(new_data[prev]));
                                    _NEFORCE destroy(&new_data[prev]);
                                    new_metadata[shift_end] = new_metadata[prev];
                                    shift_end = prev;
                                }
                                new_idx = run_end;
                                placed = true;
                                break;
                            }
                            if (!placed) {
                                new_idx = run_end;
                            }
                            break;
                        }
                        new_idx = (new_idx + 1) & (new_capacity - 1);
                    }
                    _NEFORCE construct(&new_data[new_idx], _NEFORCE move(old_data[i]));
                    new_metadata[new_idx] = h2;
                }
            }
        } catch (...) {
            for (size_t i = 0; i < new_capacity; ++i) {
                if (new_metadata[i] != FLAT_HT_EMPTY && new_metadata[i] != FLAT_HT_DELETED) {
                    _NEFORCE destroy(&new_data[i]);
                }
            }
            free_metadata(new_metadata, new_capacity);
            alloc.deallocate(new_data, new_capacity);
            throw;
        }

        free_metadata(old_metadata, old_capacity);
        if (old_data) {
            alloc.deallocate(old_data, old_capacity);
        }

        data_ = new_data;
        metadata_ = new_metadata;
        capacity_ = new_capacity;
        growth_threshold_ = threshold_of(capacity_, max_load_factor());
        deleted_ = 0;
    }

    /**
     * @brief 在指定位置构造元素
     * @tparam Args 构造参数类型
     * @param idx slot 索引
     * @param h2 H2 标签
     * @param args 构造参数
     */
    template <typename... Args>
    void construct_at(const size_t idx, const byte_t h2, Args&&... args) {
        if (metadata_[idx] == FLAT_HT_DELETED) {
            --deleted_;
        }
        _NEFORCE construct(&data_[idx], _NEFORCE forward<Args>(args)...);
        metadata_[idx] = h2;
        ++size_;
    }

    /**
     * @brief 移动元素以在指定位置腾出空槽
     * @param pos 需要腾空的位置索引
     *
     * 从 pos 开始向后找到第一个 EMPTY 或 DELETED 槽，
     * 将 [pos, end) 的元素向后移动一位，使 pos 空闲。
     */
    void shift_make_room(const size_t pos) noexcept {
        size_t end = pos;
        for (size_t i = 0; i < capacity_; ++i) {
            end = (end + 1) & (capacity_ - 1);
            const byte_t m = metadata_[end];
            if (m == FLAT_HT_EMPTY || m == FLAT_HT_DELETED) {
                break;
            }
        }
        while (end != pos) {
            const size_t prev = (end - 1) & (capacity_ - 1);
            _NEFORCE construct(&data_[end], _NEFORCE move(data_[prev]));
            _NEFORCE destroy(&data_[prev]);
            metadata_[end] = metadata_[prev];
            end = prev;
        }
    }

    /**
     * @brief 从另一个哈希表拷贝
     * @param other 源哈希表
     */
    void copy_from(const flat_hashtable& other) {
        if (other.capacity_ == 0) {
            return;
        }
        if (capacity_ != other.capacity_) {
            alloc_arrays(other.capacity_);
        } else {
            for (size_t i = 0; i < capacity_; ++i) {
                metadata_[i] = FLAT_HT_EMPTY;
            }
            size_ = 0;
        }
        deleted_ = 0;
        try {
            for (size_t i = 0; i < other.capacity_; ++i) {
                metadata_[i] = other.metadata_[i];
                if (other.metadata_[i] != FLAT_HT_EMPTY && other.metadata_[i] != FLAT_HT_DELETED) {
                    _NEFORCE construct(&data_[i], other.data_[i]);
                }
            }
            size_ = other.size_;
            deleted_ = other.deleted_;
        } catch (...) {
            clear();
            throw;
        }
    }

    bool equal_small(const flat_hashtable& rhs) const {
        for (const_iterator iter = begin(); iter != end(); ++iter) {
            const key_type& key = extracter_(*iter);
            const size_t count_lhs = _NEFORCE count_if(
                    begin(), end(), [this, &key](const value_type& val) { return equals_(extracter_(val), key); });
            const size_t count_rhs = _NEFORCE count_if(rhs.begin(), rhs.end(), [&rhs, &key](const value_type& val) {
                return rhs.equals_(rhs.extracter_(val), key);
            });
            if (count_lhs != count_rhs) {
                return false;
            }
        }
        return true;
    }

    bool equal_large(const flat_hashtable& rhs) const {
        if (size_ != rhs.size_) {
            return false;
        }
        vector<const value_type*> ptrs_lhs, ptrs_rhs;
        ptrs_lhs.reserve(size_);
        ptrs_rhs.reserve(size_);
        for (const_iterator it = begin(); it != end(); ++it) {
            ptrs_lhs.push_back(&(*it));
        }
        for (const_iterator it = rhs.begin(); it != rhs.end(); ++it) {
            ptrs_rhs.push_back(&(*it));
        }

        auto key_less = [this](const value_type* a, const value_type* b) { return extracter_(*a) < extracter_(*b); };
        auto rhs_key_less = [&rhs](const value_type* a, const value_type* b) {
            return rhs.extracter_(*a) < rhs.extracter_(*b);
        };
        _NEFORCE sort(ptrs_lhs.begin(), ptrs_lhs.end(), key_less);
        _NEFORCE sort(ptrs_rhs.begin(), ptrs_rhs.end(), rhs_key_less);

        size_type i = 0, j = 0;
        const size_type n = ptrs_lhs.size();
        while (i < n && j < n) {
            const key_type& key_l = extracter_(*ptrs_lhs[i]);
            const key_type& key_r = rhs.extracter_(*ptrs_rhs[j]);
            if (!equals_(key_l, key_r)) {
                return false;
            }
            const size_type i_start = i;
            const size_type j_start = j;
            while (i < n && equals_(extracter_(*ptrs_lhs[i]), key_l)) {
                ++i;
            }
            while (j < n && rhs.equals_(rhs.extracter_(*ptrs_rhs[j]), key_l)) {
                ++j;
            }
            const size_type count_l = i - i_start;
            const size_type count_r = j - j_start;
            if (count_l != count_r) {
                return false;
            }
            for (size_type k = i_start; k < i; ++k) {
                const value_type& val = *ptrs_lhs[k];
                bool found = false;
                for (size_type l = j_start; l < j; ++l) {
                    if (ptrs_rhs[l] && *ptrs_rhs[l] == val) {
                        ptrs_rhs[l] = nullptr;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    return false;
                }
            }
            for (size_type l = j_start; l < j; ++l) {
                if (ptrs_rhs[l] != nullptr) {
                    return false;
                }
            }
        }
        return true;
    }

    static iterator to_iterator(const const_iterator& iter) noexcept {
        return iterator(iter.index(), const_cast<flat_hashtable*>(iter.container()));
    }
    static const_iterator to_const_iterator(const iterator& iter) noexcept {
        return const_iterator(iter.index(), iter.container());
    }

public:
    /**
     * @brief 构造函数，指定初始容量
     * @param n 初始容量提示
     */
    explicit flat_hashtable(const size_type n = 0) {
        if (n > 0) {
            const size_t cap = next_power_of_2(static_cast<size_t>(static_cast<double>(n) / max_load_factor()));
            alloc_arrays(cap);
        }
    }

    /**
     * @brief 构造函数，指定初始容量和哈希函数
     * @param n 初始容量提示
     * @param hf 哈希函数
     */
    flat_hashtable(const size_type n, const HashFcn& hf) :
    hasher_(hf) {
        if (n > 0) {
            const size_t cap = next_power_of_2(static_cast<size_t>(static_cast<double>(n) / max_load_factor()));
            alloc_arrays(cap);
        }
    }

    /**
     * @brief 构造函数，指定初始容量、哈希函数和相等比较函数
     * @param n 初始容量提示
     * @param hf 哈希函数
     * @param eql 相等比较函数
     */
    flat_hashtable(const size_type n, const HashFcn& hf, const EqualKey& eql) :
    hasher_(hf),
    equals_(eql) {
        if (n > 0) {
            const size_t cap = next_power_of_2(static_cast<size_t>(static_cast<double>(n) / max_load_factor()));
            alloc_arrays(cap);
        }
    }

    /**
     * @brief 构造函数，指定所有函数对象
     * @param n 初始容量提示
     * @param hf 哈希函数
     * @param eql 相等比较函数
     * @param ext 值提取函数
     */
    flat_hashtable(const size_type n, const HashFcn& hf, const EqualKey& eql, const ExtractKey& ext) :
    hasher_(hf),
    equals_(eql),
    extracter_(ext) {
        if (n > 0) {
            const size_t cap = next_power_of_2(static_cast<size_t>(static_cast<double>(n) / max_load_factor()));
            alloc_arrays(cap);
        }
    }

    /**
     * @brief 拷贝构造函数
     * @param other 源哈希表
     */
    flat_hashtable(const flat_hashtable& other) :
    hasher_(other.hasher_),
    equals_(other.equals_),
    extracter_(other.extracter_),
    alloc_lf_(other.alloc_lf_) {
        copy_from(other);
    }

    /**
     * @brief 拷贝赋值运算符
     * @param other 源哈希表
     * @return 自身引用
     */
    flat_hashtable& operator=(const flat_hashtable& other) {
        if (_NEFORCE addressof(other) == this) {
            return *this;
        }
        clear();
        hasher_ = other.hasher_;
        equals_ = other.equals_;
        extracter_ = other.extracter_;
        alloc_lf_ = other.alloc_lf_;
        copy_from(other);
        return *this;
    }

    /**
     * @brief 移动构造函数
     * @param other 源哈希表
     */
    flat_hashtable(flat_hashtable&& other) noexcept :
    data_(other.data_),
    metadata_(other.metadata_),
    capacity_(other.capacity_),
    size_(other.size_),
    growth_threshold_(other.growth_threshold_),
    deleted_(other.deleted_),
    hasher_(_NEFORCE move(other.hasher_)),
    equals_(_NEFORCE move(other.equals_)),
    extracter_(_NEFORCE move(other.extracter_)),
    alloc_lf_(_NEFORCE move(other.alloc_lf_)) {
        other.data_ = nullptr;
        other.metadata_ = nullptr;
        other.capacity_ = 0;
        other.size_ = 0;
        other.growth_threshold_ = 0;
        other.deleted_ = 0;
    }

    /**
     * @brief 移动赋值运算符
     * @param other 源哈希表
     * @return 自身引用
     */
    flat_hashtable& operator=(flat_hashtable&& other) noexcept {
        if (_NEFORCE addressof(other) == this) {
            return *this;
        }
        clear();
        swap(other);
        return *this;
    }

    /**
     * @brief 析构函数
     */
    ~flat_hashtable() { free_arrays(); }

    /**
     * @brief 获取起始迭代器
     * @return 指向第一个元素的迭代器
     */
    NEFORCE_NODISCARD iterator begin() noexcept {
        for (size_t n = 0; n < capacity_; ++n) {
            const byte_t meta = metadata_[n];
            if (meta != FLAT_HT_EMPTY && meta != FLAT_HT_DELETED) {
                return iterator(n, this);
            }
        }
        return end();
    }

    /**
     * @brief 获取结束迭代器
     * @return 指向末尾的迭代器
     */
    NEFORCE_NODISCARD iterator end() noexcept { return iterator(capacity_, this); }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个元素的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator begin() const noexcept { return cbegin(); }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向末尾的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator end() const noexcept { return cend(); }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个元素的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator cbegin() const noexcept {
        for (size_t n = 0; n < capacity_; ++n) {
            const byte_t meta = metadata_[n];
            if (meta != FLAT_HT_EMPTY && meta != FLAT_HT_DELETED) {
                return const_iterator(n, this);
            }
        }
        return cend();
    }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向末尾的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator cend() const noexcept { return const_iterator(capacity_, this); }

    /**
     * @brief 获取元素数量
     * @return 元素数量
     */
    NEFORCE_NODISCARD size_type size() const noexcept { return size_; }

    /**
     * @brief 获取最大可能大小
     * @return 最大元素数量
     */
    NEFORCE_NODISCARD size_type max_size() const noexcept { return static_cast<size_type>(-1); }

    /**
     * @brief 检查是否为空
     * @return 是否为空
     */
    NEFORCE_NODISCARD bool empty() const noexcept { return size_ == 0; }

    /**
     * @brief 获取容量（slot 总数）
     * @return 容量
     */
    NEFORCE_NODISCARD size_type capacity() const noexcept { return capacity_; }

    /**
     * @brief 获取哈希函数对象
     * @return 哈希函数对象的副本
     */
    NEFORCE_NODISCARD hasher hash_function() const noexcept(is_nothrow_copy_constructible_v<hasher>) { return hasher_; }

    /**
     * @brief 获取键相等比较函数对象
     * @return 键相等比较函数对象的副本
     */
    NEFORCE_NODISCARD key_equal key_eql() const noexcept(is_nothrow_copy_constructible_v<key_equal>) { return equals_; }

    /**
     * @brief 获取当前负载因子
     * @return 负载因子（元素数量/容量）
     */
    NEFORCE_NODISCARD float load_factor() const noexcept {
        return capacity_ == 0 ? 0.0F : static_cast<float>(size_) / static_cast<float>(capacity_);
    }

    /**
     * @brief 获取最大负载因子
     * @return 最大负载因子
     */
    NEFORCE_NODISCARD float max_load_factor() const noexcept { return alloc_lf_.value; }

    /**
     * @brief 设置最大负载因子
     * @param lf 新的最大负载因子
     */
    void max_load_factor(const float lf) noexcept {
        NEFORCE_DEBUG_VERIFY(lf > 0, "flat_hashtable load factor invalid.");
        alloc_lf_.value = lf;
        growth_threshold_ = threshold_of(capacity_, lf);
    }

    /**
     * @brief 重新哈希，调整容量
     * @param new_size 目标容量
     */
    void rehash(const size_type new_size) { rehash_impl(new_size); }

    /**
     * @brief 预留空间
     * @param n 期望的元素数量
     *
     * 确保哈希表至少能容纳 n 个元素而不触发 rehash。
     */
    void reserve(const size_type n) {
        if (n <= size_) {
            return;
        }
        rehash(static_cast<size_t>(static_cast<double>(n) / max_load_factor()));
    }

    /**
     * @brief 为插入一个元素腾出空间
     */
    NEFORCE_NOINLINE NEFORCE_COLD void grow_for_insert() {
        if (should_rehash()) {
            const size_type new_cap = capacity_ == 0 ? 16 : capacity_ * 2;
            rehash(max(new_cap, static_cast<size_t>(static_cast<double>(size_ + 1) / max_load_factor())));
        } else if (needs_purge()) {
            rehash_impl(capacity_, true);
        }
    }

    /**
     * @brief 构造元素（唯一键版本）
     * @tparam Args 构造参数类型
     * @param args 构造参数
     * @return 插入结果（迭代器和是否成功）
     */
    template <typename... Args>
    NEFORCE_ALWAYS_INLINE_INLINE pair<iterator, bool> emplace_unique(Args&&... args) {
        if (should_rehash() || needs_purge()) {
            NEFORCE_UNLIKELY { grow_for_insert(); }
        }

        value_type tmp(_NEFORCE forward<Args>(args)...);
        const key_type& key = extracter_(tmp);
        const size_t hash = hasher_(key);
        const byte_t h2 = hash_to_h2(hash);

#ifdef NEFORCE_SIMD_SSE2
        const pair<size_t, bool> probe_result = flat_hashtable::probe_find_or_insert_simd(hash, key, h2);
#else
        const pair<size_t, bool> probe_result = flat_hashtable::probe_find_or_insert(hash, key, h2);
#endif

        if (probe_result.second) {
            return {iterator(probe_result.first, this), false};
        }

        const size_t insert_idx = probe_result.first;
        if (metadata_[insert_idx] == FLAT_HT_DELETED) {
            --deleted_;
        }
        _NEFORCE construct(&data_[insert_idx], _NEFORCE move(tmp));
        metadata_[insert_idx] = h2;
        ++size_;
        return {iterator(insert_idx, this), true};
    }

    /**
     * @brief 构造元素（允许重复键版本）
     * @tparam Args 构造参数类型
     * @param args 构造参数
     * @return 指向插入元素的迭代器
     */
    template <typename... Args>
    iterator emplace_equal(Args&&... args) {
        if (should_rehash() || needs_purge()) {
            NEFORCE_UNLIKELY { grow_for_insert(); }
        }

        value_type tmp(_NEFORCE forward<Args>(args)...);
        const key_type& key = extracter_(tmp);
        const size_t hash = hasher_(key);
        const byte_t h2 = hash_to_h2(hash);
        const size_t h1 = hash_to_index(hash);

        size_t idx = h1;
        size_t first_deleted = npos;
        for (size_t i = 0; i < capacity_; ++i) {
            const byte_t meta = metadata_[idx];
            if (meta == FLAT_HT_EMPTY) {
                const size_t insert_idx = (first_deleted != npos) ? first_deleted : idx;
                construct_at(insert_idx, h2, _NEFORCE move(tmp));
                return iterator(insert_idx, this);
            }
            if (meta == FLAT_HT_DELETED && first_deleted == npos) {
                first_deleted = idx;
            } else if (meta == h2 && equals_(extracter_(data_[idx]), key)) {
                size_t run_end = idx;
                for (size_t j = i + 1; j < capacity_; ++j) {
                    run_end = (run_end + 1) & (capacity_ - 1);
                    if (run_end == 0) {
                        const size_type new_cap = capacity_ * 2;
                        rehash(max(new_cap, static_cast<size_t>(static_cast<double>(size_ + 1) / max_load_factor())));
                        return emplace_equal(_NEFORCE move(tmp));
                    }
                    const byte_t rm = metadata_[run_end];
                    if (rm == FLAT_HT_EMPTY || rm == FLAT_HT_DELETED) {
                        construct_at(run_end, h2, _NEFORCE move(tmp));
                        return iterator(run_end, this);
                    }
                    if (rm == h2 && equals_(extracter_(data_[run_end]), key)) {
                        continue;
                    }
                    shift_make_room(run_end);
                    construct_at(run_end, h2, _NEFORCE move(tmp));
                    return iterator(run_end, this);
                }
                NEFORCE_THROW_EXCEPTION(value_exception("flat_hashtable: no available slot for insert_equal"));
            }
            idx = (idx + 1) & (capacity_ - 1);
        }

        const size_t insert_idx = first_deleted;
        if (insert_idx == npos) {
            NEFORCE_THROW_EXCEPTION(value_exception("flat_hashtable: no available slot for insert_equal"));
        }
        construct_at(insert_idx, h2, _NEFORCE move(tmp));
        return iterator(insert_idx, this);
    }

    /**
     * @brief 插入元素（唯一键，拷贝版本）
     * @param value 要插入的值
     * @return 插入结果（迭代器和是否成功）
     */
    pair<iterator, bool> insert_unique(const value_type& value) { return emplace_unique(value); }

    /**
     * @brief 插入元素（唯一键，移动版本）
     * @param value 要插入的值
     * @return 插入结果（迭代器和是否成功）
     */
    pair<iterator, bool> insert_unique(value_type&& value) { return emplace_unique(_NEFORCE move(value)); }

    /**
     * @brief 插入元素（允许重复键，拷贝版本）
     * @param value 要插入的值
     * @return 指向插入元素的迭代器
     */
    iterator insert_equal(const value_type& value) { return emplace_equal(value); }

    /**
     * @brief 插入元素（允许重复键，移动版本）
     * @param value 要插入的值
     * @return 指向插入元素的迭代器
     */
    iterator insert_equal(value_type&& value) { return emplace_equal(_NEFORCE move(value)); }

    /**
     * @brief 范围插入元素（唯一键，前向迭代器版本）
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator>
    enable_if_t<is_ranges_fwd_iter_v<Iterator>> insert_unique(Iterator first, Iterator last) {
        size_type n = _NEFORCE distance(first, last);
        if (n > 0) {
            rehash(static_cast<size_t>(static_cast<double>(size_ + n) / max_load_factor()));
        }
        for (; n > 0; --n, ++first) {
            insert_unique(*first);
        }
        return;
    }

    /**
     * @brief 范围插入元素（唯一键，非前向迭代器版本）
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator>
    enable_if_t<!is_ranges_fwd_iter_v<Iterator>> insert_unique(Iterator first, Iterator last) {
        for (; first != last; ++first) {
            insert_unique(*first);
        }
        return;
    }

    /**
     * @brief 初始化列表插入（唯一键）
     * @param ilist 初始化列表
     */
    void insert_unique(std::initializer_list<value_type> ilist) { insert_unique(ilist.begin(), ilist.end()); }

    /**
     * @brief 范围插入元素（允许重复键，前向迭代器版本）
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator>
    enable_if_t<is_ranges_fwd_iter_v<Iterator>> insert_equal(Iterator first, Iterator last) {
        size_type n = _NEFORCE distance(first, last);
        if (n > 0) {
            rehash(static_cast<size_t>(static_cast<double>(size_ + n) / max_load_factor()));
        }
        for (; n > 0; --n, ++first) {
            insert_equal(*first);
        }
        return;
    }

    /**
     * @brief 范围插入元素（允许重复键，非前向迭代器版本）
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator>
    enable_if_t<!is_ranges_fwd_iter_v<Iterator>> insert_equal(Iterator first, Iterator last) {
        for (; first != last; ++first) {
            insert_equal(*first);
        }
        return;
    }

    /**
     * @brief 初始化列表插入（允许重复键）
     * @param ilist 初始化列表
     */
    void insert_equal(std::initializer_list<value_type> ilist) { insert_equal(ilist.begin(), ilist.end()); }

    /**
     * @brief 删除所有具有指定键的元素
     * @param key 要删除的键
     * @return 删除的元素数量
     */
    size_type erase(const key_type& key) noexcept {
        if (capacity_ == 0) {
            return 0;
        }

        const size_t hash = hasher_(key);
        const byte_t h2 = hash_to_h2(hash);
        size_t idx = hash_to_index(hash);
        size_type erased = 0;

#ifdef NEFORCE_SIMD_SSE2
        const simd::vec128_t h2_vec = simd::fill_i8(h2);
        const simd::vec128_t empty_vec = simd::fill_i8(FLAT_HT_EMPTY);

        for (size_t round = 0; round < capacity_; round += FLAT_HT_GROUP) {
            byte_t buf[FLAT_HT_GROUP];
            simd::vec128_t meta_vec;
            if (capacity_ - idx >= FLAT_HT_GROUP) {
                meta_vec = simd::load_unaligned(metadata_ + idx);
            } else {
                load_meta_group(idx, buf);
                meta_vec = simd::load_unaligned(buf);
            }

            int match = simd::to_bitmask(simd::match_bytes(meta_vec, h2_vec));
            const int empty_mask = simd::to_bitmask(simd::match_bytes(meta_vec, empty_vec));
            const int stop =
                    empty_mask != 0 ? countr_zero(static_cast<uintptr_t>(empty_mask)) : static_cast<int>(FLAT_HT_GROUP);
            match &= (stop >= static_cast<int>(FLAT_HT_GROUP)) ? 0xFFFF : ((1 << stop) - 1);

            while (match != 0) {
                const int bit = countr_zero(static_cast<uintptr_t>(match));
                const size_t slot = (idx + bit) & (capacity_ - 1);
                if (equals_(extracter_(data_[slot]), key)) {
                    _NEFORCE destroy(&data_[slot]);
                    metadata_[slot] = FLAT_HT_DELETED;
                    ++deleted_;
                    ++erased;
                    --size_;
                }
                match &= (match - 1);
            }

            if (empty_mask != 0) {
                return erased;
            }
            idx = (idx + FLAT_HT_GROUP) & (capacity_ - 1);
        }
        return erased;
#else
        for (size_t i = 0; i < capacity_; ++i) {
            const byte_t meta = metadata_[idx];
            if (meta == FLAT_HT_EMPTY) {
                break;
            }
            if (meta == h2 && equals_(extracter_(data_[idx]), key)) {
                _NEFORCE destroy(&data_[idx]);
                metadata_[idx] = FLAT_HT_DELETED;
                ++deleted_;
                ++erased;
                --size_;
            }
            idx = (idx + 1) & (capacity_ - 1);
        }
        return erased;
#endif
    }

    /**
     * @brief 删除指定位置的元素
     * @param position 要删除的位置
     * @return 指向被删除元素之后位置的迭代器
     */
    iterator erase(const iterator& position) noexcept {
        if (position.container() != this || position.index() >= capacity_) {
            return end();
        }
        const byte_t meta = metadata_[position.index()];
        if (meta == FLAT_HT_EMPTY || meta == FLAT_HT_DELETED) {
            return end();
        }

        _NEFORCE destroy(&data_[position.index()]);
        metadata_[position.index()] = FLAT_HT_DELETED;
        ++deleted_;
        --size_;

        return iterator(find_occupied(metadata_, capacity_, position.index() + 1), this);
    }

    /**
     * @brief 删除指定范围内的元素
     * @param first 起始迭代器
     * @param last 结束迭代器
     * @return 指向最后一个被删除元素之后位置的迭代器
     */
    iterator erase(iterator first, iterator last) noexcept {
        if (first == last) {
            return last;
        }
        if (first.container() != this || (last.container() != this && last != end())) {
            return end();
        }

        for (size_t idx = first.index(); idx < last.index() && idx < capacity_; ++idx) {
            const byte_t meta = metadata_[idx];
            if (meta != FLAT_HT_EMPTY && meta != FLAT_HT_DELETED) {
                _NEFORCE destroy(&data_[idx]);
                metadata_[idx] = FLAT_HT_DELETED;
                ++deleted_;
                --size_;
            }
        }
        return last;
    }

    /**
     * @brief 删除指定位置的元素（常量迭代器版本）
     * @param position 要删除的位置
     * @return 指向被删除元素之后位置的常量迭代器
     */
    const_iterator erase(const const_iterator& position) noexcept {
        return to_const_iterator(erase(to_iterator(position)));
    }

    /**
     * @brief 删除指定范围内的常量元素
     * @param first 起始迭代器
     * @param last 结束迭代器
     * @return 指向最后一个被删除元素之后位置的常量迭代器
     */
    const_iterator erase(const_iterator first, const_iterator last) noexcept {
        return to_const_iterator(erase(to_iterator(first), to_iterator(last)));
    }

    /**
     * @brief 清空哈希表
     */
    void clear() noexcept {
        for (size_t i = 0; i < capacity_; ++i) {
            if (metadata_[i] != FLAT_HT_EMPTY && metadata_[i] != FLAT_HT_DELETED) {
                _NEFORCE destroy(&data_[i]);
            }
            metadata_[i] = FLAT_HT_EMPTY;
        }
        size_ = 0;
        deleted_ = 0;
    }

    /**
     * @brief 查找具有指定键的元素
     * @param key 要查找的键
     * @return 指向第一个匹配元素的迭代器，未找到则返回 end()
     */
    NEFORCE_NODISCARD iterator find(const key_type& key) noexcept {
        if (capacity_ == 0) {
            return end();
        }
        const size_t hash = hasher_(key);
        const size_type idx = probe_find(hash, key, hash_to_h2(hash));
        return idx == npos ? end() : iterator(idx, this);
    }

    /**
     * @brief 查找具有指定键的元素（常量版本）
     * @param key 要查找的键
     * @return 指向第一个匹配元素的常量迭代器，未找到则返回 cend()
     */
    NEFORCE_NODISCARD const_iterator find(const key_type& key) const noexcept {
        if (capacity_ == 0) {
            return cend();
        }
        const size_t hash = hasher_(key);
        const size_type idx = probe_find(hash, key, hash_to_h2(hash));
        return idx == npos ? cend() : const_iterator(idx, this);
    }

    /**
     * @brief 统计具有指定键的元素数量
     * @param key 要统计的键
     * @return 匹配的元素数量
     */
    NEFORCE_NODISCARD size_type count(const key_type& key) const noexcept {
        if (capacity_ == 0) {
            return 0;
        }
        const size_t hash = hasher_(key);
        return probe_count(hash, key, hash_to_h2(hash));
    }

    /**
     * @brief 检查是否包含指定键
     * @param key 要检查的键
     * @return 是否包含
     */
    NEFORCE_NODISCARD bool contains(const key_type& key) const noexcept {
        if (capacity_ == 0) {
            return false;
        }
        const size_t hash = hasher_(key);
        return probe_find(hash, key, hash_to_h2(hash)) != npos;
    }

    /**
     * @brief 获取等于指定键的元素范围
     * @param key 键值
     * @return 包含范围起始和结束的 pair
     */
    NEFORCE_NODISCARD pair<iterator, iterator> equal_range(const key_type& key) {
        if (capacity_ == 0) {
            return {end(), end()};
        }
        const size_t hash = hasher_(key);
        const byte_t h2 = hash_to_h2(hash);
        size_t idx = hash_to_index(hash);

        size_t first_idx = npos;
        for (size_t i = 0; i < capacity_; ++i) {
            const byte_t meta = metadata_[idx];
            if (meta == FLAT_HT_EMPTY) {
                return {end(), end()};
            }
            if (meta == h2 && equals_(extracter_(data_[idx]), key)) {
                first_idx = idx;
                break;
            }
            idx = (idx + 1) & (capacity_ - 1);
        }
        if (first_idx == npos) {
            return {end(), end()};
        }

        size_t last_idx = first_idx;
        do {
            last_idx = (last_idx + 1) & (capacity_ - 1);
            if (last_idx == first_idx) {
                return {iterator(first_idx, this), end()};
            }
        } while (metadata_[last_idx] != FLAT_HT_EMPTY && equals_(extracter_(data_[last_idx]), key));

        while (last_idx != first_idx &&
               (metadata_[last_idx] == FLAT_HT_EMPTY || metadata_[last_idx] == FLAT_HT_DELETED)) {
            last_idx = (last_idx + 1) & (capacity_ - 1);
        }
        if (last_idx == first_idx || last_idx < first_idx) {
            return {iterator(first_idx, this), end()};
        }
        return {iterator(first_idx, this), iterator(last_idx, this)};
    }

    /**
     * @brief 获取等于指定键的元素范围（常量版本）
     * @param key 键值
     * @return 包含范围起始和结束的 pair
     */
    NEFORCE_NODISCARD pair<const_iterator, const_iterator> equal_range(const key_type& key) const {
        if (capacity_ == 0) {
            return {cend(), cend()};
        }
        const size_t hash = hasher_(key);
        const byte_t h2 = hash_to_h2(hash);
        size_t idx = hash_to_index(hash);

        size_t first_idx = npos;
        for (size_t i = 0; i < capacity_; ++i) {
            const byte_t meta = metadata_[idx];
            if (meta == FLAT_HT_EMPTY) {
                return {cend(), cend()};
            }
            if (meta == h2 && equals_(extracter_(data_[idx]), key)) {
                first_idx = idx;
                break;
            }
            idx = (idx + 1) & (capacity_ - 1);
        }
        if (first_idx == npos) {
            return {cend(), cend()};
        }

        size_t last_idx = first_idx;
        do {
            last_idx = (last_idx + 1) & (capacity_ - 1);
            if (last_idx == first_idx) {
                return {const_iterator(first_idx, this), cend()};
            }
        } while (metadata_[last_idx] != FLAT_HT_EMPTY && equals_(extracter_(data_[last_idx]), key));

        while (last_idx != first_idx &&
               (metadata_[last_idx] == FLAT_HT_EMPTY || metadata_[last_idx] == FLAT_HT_DELETED)) {
            last_idx = (last_idx + 1) & (capacity_ - 1);
        }
        if (last_idx == first_idx || last_idx < first_idx) {
            // wraparound: the range extends past the end of the array
            return {const_iterator(first_idx, this), cend()};
        }
        return {const_iterator(first_idx, this), const_iterator(last_idx, this)};
    }

    /**
     * @brief 交换两个哈希表的内容
     * @param other 要交换的另一个哈希表
     */
    void swap(flat_hashtable& other) noexcept {
        if (_NEFORCE addressof(other) == this) {
            return;
        }
        _NEFORCE swap(data_, other.data_);
        _NEFORCE swap(metadata_, other.metadata_);
        _NEFORCE swap(capacity_, other.capacity_);
        _NEFORCE swap(size_, other.size_);
        _NEFORCE swap(growth_threshold_, other.growth_threshold_);
        _NEFORCE swap(deleted_, other.deleted_);
        _NEFORCE swap(hasher_, other.hasher_);
        _NEFORCE swap(equals_, other.equals_);
        _NEFORCE swap(extracter_, other.extracter_);
        alloc_lf_.swap(other.alloc_lf_);
    }

    /**
     * @brief 相等比较操作符
     * @param rhs 右侧哈希表
     * @return 如果两个哈希表大小相等且对应元素相等返回 true
     */
    NEFORCE_NODISCARD bool equal_to(const flat_hashtable& rhs) const {
        if (size_ != rhs.size_) {
            return false;
        }
        if (size_ == 0) {
            return true;
        }
        if (this == &rhs) {
            return true;
        }
        if (size_ < 100) {
            return equal_small(rhs);
        }
        return equal_large(rhs);
    }

    /**
     * @brief 小于比较操作符
     * @param rhs 右侧哈希表
     * @return 按字典序比较结果
     */
    NEFORCE_NODISCARD bool less_than(const flat_hashtable& rhs) const
            noexcept(noexcept(_NEFORCE lexicographical_compare(cbegin(), cend(), rhs.cbegin(), rhs.cend()))) {
        return _NEFORCE lexicographical_compare(cbegin(), cend(), rhs.cbegin(), rhs.cend());
    }
};

/** @} */ // FlatHashTable

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_CONTAINER_FLAT_HASHTABLE_HPP__
