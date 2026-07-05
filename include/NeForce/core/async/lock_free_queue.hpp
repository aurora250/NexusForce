#ifndef NEFORCE_CORE_ASYNC_LOCK_FREE_QUEUE_HPP__
#define NEFORCE_CORE_ASYNC_LOCK_FREE_QUEUE_HPP__

/**
 * @file lock_free_queue.hpp
 * @brief 无锁队列
 *
 * 此文件提供了无锁队列的实现，支持多线程并发访问，
 * 无需使用互斥锁即可实现线程安全的队列操作。
 */

#include "NeForce/core/async/atomic.hpp"
#include "NeForce/core/async/mutex.hpp"
#include "NeForce/core/async/thread.hpp"
#include "NeForce/core/async/thread_exit_notifier.hpp"
#include "NeForce/core/memory/unique_ptr.hpp"
NEFORCE_BEGIN_NAMESPACE__

/// @cond
NEFORCE_BEGIN_INNER__

NEFORCE_INLINE17 constexpr thread::id invalid_thread_id_zero{0};
#ifdef NEFORCE_PLATFORM_WINDOWS
NEFORCE_INLINE17 constexpr thread::id invalid_thread_id_max{static_cast<thread::id::native_id_type>(-1)};
#endif

template <typename T>
constexpr char* align_for(char* ptr) noexcept {
    constexpr size_t alignment = alignment_of_v<T>;
    return ptr + (alignment - (reinterpret_cast<uintptr_t>(ptr) % alignment)) % alignment;
}

template <typename T>
constexpr T ceil_to_pow_2(T x) noexcept {
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    for (size_t i = 1; i < sizeof(T); i <<= 1) {
        x |= x >> (i << 3);
    }
    ++x;
    return x;
}

constexpr bool circular_less_than(size_t a, size_t b) noexcept { return static_cast<ptrdiff_t>(a - b) < 0; }

NEFORCE_END_INNER__
/// @endcond

/**
 * @defgroup AsyncComponents 异步组件
 * @brief 异步编程相关组件
 * @{
 */

/**
 * @defgroup LockFreeQueue 无锁队列
 * @brief 线程安全的无锁队列
 * @{
 */

/**
 * @class lock_free_queue
 * @brief 无锁队列类模板
 * @tparam T 元素类型
 *
 * 基于 per-producer SPSC 子队列架构实现的无锁队列。
 * 每个生产者线程拥有独立的 block-based SPSC 内队列，block 通过无锁 free_list 回收利用。
 * 支持多生产者多消费者并发 push/try_pop 操作。
 */
template <typename T>
class lock_free_queue {
public:
    /// 每个 block 存储的元素数量
    static constexpr size_t BLOCK_SIZE = 32;
    /// 隐式生产者哈希表初始大小
    static constexpr size_t INITIAL_IMPLICIT_PRODUCER_HASH_SIZE = 32;
    /// block 索引初始容量
    static constexpr size_t IMPLICIT_INITIAL_INDEX_SIZE = 32;

private:
    struct block;
    struct implicit_producer;
    struct implicit_producer_kvp;
    struct implicit_producer_hash;

    static constexpr uint32_t REFS_MASK = 0x7FFFFFFF;
    static constexpr uint32_t SHOULD_BE_ON_FREELIST = 0x80000000;

    struct free_list {
        atomic<block*> free_list_head_{nullptr};

        free_list() noexcept = default;

        free_list(free_list&& other) noexcept :
        free_list_head_(other.free_list_head_.load(memory_order_relaxed)) {
            other.free_list_head_.store(nullptr, memory_order_relaxed);
        }

        free_list(const free_list&) = delete;
        free_list& operator=(const free_list&) = delete;

        void swap(free_list& other) noexcept {
            block* tmp = free_list_head_.load(memory_order_relaxed);
            free_list_head_.store(other.free_list_head_.load(memory_order_relaxed), memory_order_relaxed);
            other.free_list_head_.store(tmp, memory_order_relaxed);
        }

        void add(block* node) noexcept {
            const uint32_t prev = node->free_list_refs_.fetch_add(SHOULD_BE_ON_FREELIST, memory_order_acq_rel);
            if (prev == 0) {
                this->add_knowing_refcount_is_zero(node);
            }
        }

        block* try_get() noexcept {
            auto head = free_list_head_.load(memory_order_acquire);
            while (head != nullptr) {
                auto prev_head = head;
                auto refs = head->free_list_refs_.load(memory_order_relaxed);
                if ((refs & REFS_MASK) == 0 ||
                    !head->free_list_refs_.compare_exchange_strong(refs, refs + 1, memory_order_acquire)) {
                    head = free_list_head_.load(memory_order_acquire);
                    continue;
                }
                auto next = head->free_list_next_.load(memory_order_relaxed);
                if (free_list_head_.compare_exchange_strong(head, next, memory_order_acquire, memory_order_relaxed)) {
                    head->free_list_refs_.fetch_sub(2, memory_order_release);
                    return head;
                }
                refs = prev_head->free_list_refs_.fetch_sub(1, memory_order_acq_rel);
                if (refs == SHOULD_BE_ON_FREELIST + 1) {
                    this->add_knowing_refcount_is_zero(prev_head);
                }
            }
            return nullptr;
        }

        block* head_unsafe() const noexcept { return free_list_head_.load(memory_order_relaxed); }

    private:
        void add_knowing_refcount_is_zero(block* node) noexcept {
            auto head = free_list_head_.load(memory_order_relaxed);
            while (true) {
                node->free_list_next_.store(head, memory_order_relaxed);
                node->free_list_refs_.store(1, memory_order_release);
                if (!free_list_head_.compare_exchange_strong(head, node, memory_order_release, memory_order_relaxed)) {
                    if (node->free_list_refs_.fetch_add(SHOULD_BE_ON_FREELIST - 1, memory_order_acq_rel) == 1) {
                        continue;
                    }
                }
                return;
            }
        }
    };

    struct block {
        block* next_{nullptr};                           // 生产者 block 链
        atomic<size_t> elements_completely_dequeued_{0}; // 已出队元素数
        atomic<uint32_t> free_list_refs_{0};             // free_list 引用计数
        atomic<block*> free_list_next_{nullptr};         // free_list 链接
        bool dynamically_allocated_{true};               // 是否堆分配

        aligned_storage_t<sizeof(T), alignof(T)> elements_[BLOCK_SIZE];

        block() noexcept = default;

        NEFORCE_NODISCARD T* at(const size_t idx) noexcept {
            return reinterpret_cast<T*>(&elements_[idx & (BLOCK_SIZE - 1)]);
        }

        bool set_empty(size_t /*i*/) noexcept {
            const size_t prev = elements_completely_dequeued_.fetch_add(1, memory_order_acq_rel);
            return prev == BLOCK_SIZE - 1;
        }

        void reset_empty() noexcept { elements_completely_dequeued_.store(0, memory_order_relaxed); }

        NEFORCE_NODISCARD bool is_empty() const noexcept {
            if (elements_completely_dequeued_.load(memory_order_relaxed) == BLOCK_SIZE) {
                atomic_thread_fence(memory_order_acquire);
                return true;
            }
            return false;
        }
    };

    static constexpr size_t INVALID_BLOCK_BASE = 1;

    struct block_index_entry {
        atomic<size_t> key_;
        atomic<block*> value_;

        block_index_entry() noexcept :
        key_(INVALID_BLOCK_BASE),
        value_(nullptr) {}
    };

    struct block_index_header {
        size_t capacity_;
        atomic<size_t> tail_;
        block_index_entry* entries_;
        block_index_entry** index_;
        block_index_header* prev_;
    };

    struct implicit_producer_kvp {
        atomic<thread::id> key_{inner::invalid_thread_id_zero};
        implicit_producer* value_{nullptr};

        implicit_producer_kvp() noexcept = default;
    };

    struct implicit_producer_hash {
        size_t capacity_;
        implicit_producer_kvp* entries_;
        implicit_producer_hash* prev_;
    };

    struct implicit_producer {
        implicit_producer* next_{nullptr};
        atomic<bool> inactive_{false};
        thread_exit_listener thread_exit_listener_;

        atomic<size_t> tail_index_{0};
        atomic<size_t> head_index_{0};
        atomic<size_t> dequeue_optimistic_count_{0};
        atomic<size_t> dequeue_overcommit_{0};
        block* tail_block_{nullptr};
        atomic<block_index_header*> block_index_{nullptr};
        size_t next_block_index_capacity_{IMPLICIT_INITIAL_INDEX_SIZE};

        lock_free_queue* parent_;

        explicit implicit_producer(lock_free_queue* parent) :
        parent_(parent) {
            new_block_index();
        }

        ~implicit_producer() {
            if (!inactive_.load(memory_order_relaxed)) {
                thread_exit_notifier::unsubscribe(&thread_exit_listener_);
            }

            size_t index = head_index_.load(memory_order_relaxed);
            const size_t tail = tail_index_.load(memory_order_relaxed);
            block* block = nullptr;
            const bool force_free_last = (index != tail);

            while (index != tail) {
                if ((index & (BLOCK_SIZE - 1)) == 0 || block == nullptr) {
                    if (block != nullptr) {
                        parent_->add_block_to_free_list(block);
                    }
                    auto* entry = get_block_index_entry_for_index(index);
                    block = entry->value_.load(memory_order_relaxed);
                }
                block->at(index)->~T();
                ++index;
            }

            if (tail_block_ != nullptr && (force_free_last || (tail & (BLOCK_SIZE - 1)) != 0)) {
                parent_->add_block_to_free_list(tail_block_);
            }

            auto* local_index = block_index_.load(memory_order_relaxed);
            if (local_index != nullptr) {
                for (size_t i = 0; i != local_index->capacity_; ++i) {
                    local_index->entries_[i].~block_index_entry();
                }
                do {
                    auto prev = local_index->prev_;
                    delete[] local_index->index_;
                    local_index->~block_index_header();
                    ::operator delete(local_index);
                    local_index = prev;
                } while (local_index != nullptr);
            }
        }

        implicit_producer(const implicit_producer&) = delete;
        implicit_producer& operator=(const implicit_producer&) = delete;

        void enqueue(T&& element) {
            size_t current_tail = tail_index_.load(memory_order_relaxed);
            const size_t new_tail = current_tail + 1;

            if ((current_tail & (BLOCK_SIZE - 1)) == 0) {
                const size_t head = head_index_.load(memory_order_relaxed);

                if (!inner::circular_less_than(head, current_tail + BLOCK_SIZE)) {
                    return;
                }

                block_index_entry* idx_entry = nullptr;
                if (!insert_block_index_entry(idx_entry, current_tail)) {
                    return;
                }

                auto* new_block = parent_->requisition_block();
                if (new_block == nullptr) {
                    rewind_block_index_tail();
                    idx_entry->value_.store(nullptr, memory_order_relaxed);
                    return;
                }
                new_block->reset_empty();

                new (new_block->at(current_tail)) T(move(element));

                idx_entry->value_.store(new_block, memory_order_relaxed);
                tail_block_ = new_block;
            } else {
                new (tail_block_->at(current_tail)) T(move(element));
            }

            tail_index_.store(new_tail, memory_order_release);
        }

        bool dequeue(T& element) {
            size_t tail = tail_index_.load(memory_order_relaxed);
            const size_t overcommit = dequeue_overcommit_.load(memory_order_relaxed);

            if (!inner::circular_less_than(dequeue_optimistic_count_.load(memory_order_relaxed) - overcommit, tail)) {
                return false;
            }

            atomic_thread_fence(memory_order_acquire);

            const size_t my_count = dequeue_optimistic_count_.fetch_add(1, memory_order_relaxed);
            tail = tail_index_.load(memory_order_acquire);

            if (inner::circular_less_than(my_count - overcommit, tail)) {
                size_t index = head_index_.fetch_add(1, memory_order_acq_rel);

                auto* entry = get_block_index_entry_for_index(index);
                auto* block = entry->value_.load(memory_order_relaxed);
                auto& el = *block->at(index);

                element = move(el);
                el.~T();

                if (block->set_empty(index)) {
                    {
                        entry->value_.store(nullptr, memory_order_relaxed);
                    }
                    parent_->add_block_to_free_list(block);
                }

                return true;
            }

            dequeue_overcommit_.fetch_add(1, memory_order_release);

            return false;
        }

        NEFORCE_NODISCARD size_t size_approx() const {
            const size_t tail = tail_index_.load(memory_order_relaxed);
            const size_t head = head_index_.load(memory_order_relaxed);
            return inner::circular_less_than(head, tail) ? (tail - head) : 0;
        }

    private:
        bool new_block_index() {
            auto prev = block_index_.load(memory_order_relaxed);
            size_t prev_capacity = (prev == nullptr) ? 0 : prev->capacity_;
            size_t entry_count = (prev == nullptr) ? next_block_index_capacity_ : prev_capacity;

            const size_t alloc_size = sizeof(block_index_header) + inner::align_for<block_index_entry>(nullptr) -
                                      static_cast<char*>(nullptr) + sizeof(block_index_entry) * entry_count;
            void* raw = ::operator new(alloc_size);
            if (raw == nullptr) {
                return false;
            }

            auto* header = new (raw) block_index_header;
            auto* entries = reinterpret_cast<block_index_entry*>(
                    inner::align_for<block_index_entry>(static_cast<char*>(raw) + sizeof(block_index_header)));

            auto** index = new block_index_entry*[entry_count];
            if (prev != nullptr) {
                size_t prev_tail = prev->tail_.load(memory_order_relaxed);
                size_t prev_pos = prev_tail;
                size_t i = 0;
                do {
                    prev_pos = (prev_pos + 1) & (prev->capacity_ - 1);
                    index[i++] = prev->index_[prev_pos];
                } while (prev_pos != prev_tail);
                NEFORCE_DEBUG_VERIFY(i == prev_capacity, "i == prev_capacity failed in lock-free queue");
            }

            for (size_t i = prev_capacity; i < entry_count; ++i) {
                new (entries + i) block_index_entry;
                entries[i].key_.store(INVALID_BLOCK_BASE, memory_order_relaxed);
                index[i] = entries + i;
            }

            header->prev_ = prev;
            header->entries_ = entries;
            header->index_ = index;
            header->capacity_ = entry_count;
            header->tail_.store((prev_capacity - 1) & (entry_count - 1), memory_order_relaxed);

            block_index_.store(header, memory_order_release);
            next_block_index_capacity_ <<= 1;
            return true;
        }

        bool insert_block_index_entry(block_index_entry*& idx_entry, size_t block_start_index) {
            auto* local_index = block_index_.load(memory_order_relaxed);
            if (local_index == nullptr) {
                return false;
            }

            size_t new_tail = (local_index->tail_.load(memory_order_relaxed) + 1) & (local_index->capacity_ - 1);
            idx_entry = local_index->index_[new_tail];

            if (idx_entry->key_.load(memory_order_relaxed) == INVALID_BLOCK_BASE ||
                idx_entry->value_.load(memory_order_relaxed) == nullptr) {
                idx_entry->key_.store(block_start_index, memory_order_relaxed);
                local_index->tail_.store(new_tail, memory_order_release);
                return true;
            }

            if (!new_block_index()) {
                return false;
            }

            local_index = block_index_.load(memory_order_relaxed);
            new_tail = (local_index->tail_.load(memory_order_relaxed) + 1) & (local_index->capacity_ - 1);
            idx_entry = local_index->index_[new_tail];
            idx_entry->key_.store(block_start_index, memory_order_relaxed);
            local_index->tail_.store(new_tail, memory_order_release);
            return true;
        }

        block_index_entry* get_block_index_entry_for_index(size_t index) const {
            const size_t block_base = index & ~(BLOCK_SIZE - 1);

            auto* local_index = block_index_.load(memory_order_acquire);
            size_t tail = local_index->tail_.load(memory_order_acquire);
            const size_t tail_base = local_index->index_[tail]->key_.load(memory_order_relaxed);

            const ptrdiff_t offset =
                    static_cast<ptrdiff_t>(block_base - tail_base) / static_cast<ptrdiff_t>(BLOCK_SIZE);
            size_t idx = (tail + offset) & (local_index->capacity_ - 1);
            return local_index->index_[idx];
        }

        void rewind_block_index_tail() {
            auto* local_index = block_index_.load(memory_order_relaxed);
            local_index->tail_.store((local_index->tail_.load(memory_order_relaxed) - 1) & (local_index->capacity_ - 1),
                                     memory_order_relaxed);
        }
    };

    atomic<implicit_producer*> producer_list_tail_{nullptr};
    atomic<uint32_t> producer_count_{0};

    atomic<implicit_producer_hash*> implicit_producer_hash_;
    atomic<size_t> implicit_producer_hash_count_;
    atomic_flag implicit_producer_hash_resize_in_progress_;
    implicit_producer_hash initial_implicit_producer_hash_;
    implicit_producer_kvp initial_implicit_producer_hash_entries_[INITIAL_IMPLICIT_PRODUCER_HASH_SIZE];

    free_list free_list_;
    atomic<size_t> initial_block_pool_index_{0};
    block* initial_block_pool_{nullptr};
    size_t initial_block_pool_size_{0};

    void populate_initial_implicit_producer_hash() {
        implicit_producer_hash_count_.store(0, memory_order_relaxed);
        auto* hash = &initial_implicit_producer_hash_;
        hash->capacity_ = INITIAL_IMPLICIT_PRODUCER_HASH_SIZE;
        hash->entries_ = initial_implicit_producer_hash_entries_;
        for (size_t i = 0; i != INITIAL_IMPLICIT_PRODUCER_HASH_SIZE; ++i) {
            initial_implicit_producer_hash_entries_[i].key_.store(inner::invalid_thread_id_zero, memory_order_relaxed);
        }
        hash->prev_ = nullptr;
        implicit_producer_hash_.store(hash, memory_order_relaxed);
    }

    void populate_initial_block_list(size_t block_count) {
        initial_block_pool_size_ = block_count;
        if (block_count == 0) {
            initial_block_pool_ = nullptr;
            return;
        }
        initial_block_pool_ = new block[block_count];
        for (size_t i = 0; i < block_count; ++i) {
            initial_block_pool_[i].dynamically_allocated_ = false;
        }
    }

    block* try_get_block_from_initial_pool() {
        if (initial_block_pool_index_.load(memory_order_relaxed) >= initial_block_pool_size_) {
            return nullptr;
        }
        auto index = initial_block_pool_index_.fetch_add(1, memory_order_relaxed);
        return index < initial_block_pool_size_ ? initial_block_pool_ + index : nullptr;
    }

    block* try_get_block_from_free_list() { return free_list_.try_get(); }

    block* requisition_block() {
        auto* blk = try_get_block_from_initial_pool();
        if (blk != nullptr) {
            return blk;
        }
        blk = try_get_block_from_free_list();
        if (blk != nullptr) {
            return blk;
        }
        return new block;
    }

    void add_block_to_free_list(block* block) { free_list_.add(block); }

    void add_blocks_to_free_list(block* block) {
        while (block != nullptr) {
            auto* next = block->next_;
            add_block_to_free_list(block);
            block = next;
        }
    }

    implicit_producer* recycle_or_create_producer() {
        for (auto ptr = producer_list_tail_.load(memory_order_acquire); ptr != nullptr; ptr = ptr->next_) {
            if (ptr->inactive_.load(memory_order_relaxed)) {
                bool expected = true;
                if (ptr->inactive_.compare_exchange_strong(expected, false, memory_order_acquire,
                                                           memory_order_relaxed)) {
                    return ptr;
                }
            }
        }
        return add_producer(new implicit_producer(this));
    }

    implicit_producer* add_producer(implicit_producer* producer) {
        if (producer == nullptr) {
            return nullptr;
        }
        producer_count_.fetch_add(1, memory_order_relaxed);

        auto prev_tail = producer_list_tail_.load(memory_order_relaxed);
        do {
            producer->next_ = prev_tail;
        } while (!producer_list_tail_.compare_exchange_weak(prev_tail, producer, memory_order_release,
                                                            memory_order_relaxed));
        return producer;
    }

    implicit_producer* get_or_add_implicit_producer() {
        auto id = this_thread::id();
        const auto hashed_id = id.to_hash();

        auto* main_hash = implicit_producer_hash_.load(memory_order_acquire);
        for (auto* hash = main_hash; hash != nullptr; hash = hash->prev_) {
            auto index = hashed_id;
            while (true) {
                index &= hash->capacity_ - 1;
                auto probed_key = hash->entries_[index].key_.load(memory_order_relaxed);
                if (probed_key == id) {
                    auto* value = hash->entries_[index].value_;
                    if (hash != main_hash) {
                        index = hashed_id;
                        while (true) {
                            index &= main_hash->capacity_ - 1;
                            auto empty = inner::invalid_thread_id_zero;
#ifdef NEFORCE_PLATFORM_WINDOWS
                            auto reusable = inner::invalid_thread_id_max;
                            if (main_hash->entries_[index].key_.compare_exchange_strong(empty, id, memory_order_seq_cst,
                                                                                        memory_order_relaxed) ||
                                main_hash->entries_[index].key_.compare_exchange_strong(
                                        reusable, id, memory_order_seq_cst, memory_order_relaxed)) {
#else
                            if (main_hash->entries_[index].key_.compare_exchange_strong(empty, id, memory_order_seq_cst,
                                                                                        memory_order_relaxed)) {
#endif
                                main_hash->entries_[index].value_ = value;
                                break;
                            }
                            ++index;
                        }
                    }
                    return value;
                }
                if (probed_key == inner::invalid_thread_id_zero) {
                    break;
                }
                ++index;
            }
        }

        auto new_count = 1 + implicit_producer_hash_count_.fetch_add(1, memory_order_relaxed);
        while (true) {
            if (new_count >= (main_hash->capacity_ >> 1) &&
                !implicit_producer_hash_resize_in_progress_.test_and_set(memory_order_acquire)) {
                main_hash = implicit_producer_hash_.load(memory_order_acquire);
                if (new_count >= (main_hash->capacity_ >> 1)) {
                    size_t new_capacity = main_hash->capacity_ << 1;
                    while (new_count >= (new_capacity >> 1)) {
                        new_capacity <<= 1;
                    }

                    const size_t alloc_size =
                            sizeof(implicit_producer_hash) + inner::align_for<implicit_producer_kvp>(nullptr) -
                            static_cast<char*>(nullptr) + sizeof(implicit_producer_kvp) * new_capacity;
                    void* raw = ::operator new(alloc_size);
                    if (raw == nullptr) {
                        implicit_producer_hash_count_.fetch_sub(1, memory_order_relaxed);
                        implicit_producer_hash_resize_in_progress_.clear(memory_order_relaxed);
                        return nullptr;
                    }

                    auto* new_hash = new (raw) implicit_producer_hash;
                    new_hash->capacity_ = new_capacity;
                    new_hash->entries_ =
                            reinterpret_cast<implicit_producer_kvp*>(inner::align_for<implicit_producer_kvp>(
                                    static_cast<char*>(raw) + sizeof(implicit_producer_hash)));
                    for (size_t i = 0; i != new_capacity; ++i) {
                        new (&new_hash->entries_[i]) implicit_producer_kvp;
                        new_hash->entries_[i].key_.store(inner::invalid_thread_id_zero, memory_order_relaxed);
                    }
                    new_hash->prev_ = main_hash;
                    implicit_producer_hash_.store(new_hash, memory_order_release);
                    implicit_producer_hash_resize_in_progress_.clear(memory_order_release);
                    main_hash = new_hash;
                } else {
                    implicit_producer_hash_resize_in_progress_.clear(memory_order_release);
                }
            }

            if (new_count < (main_hash->capacity_ >> 1) + (main_hash->capacity_ >> 2)) {
                auto* producer = recycle_or_create_producer();
                if (producer == nullptr) {
                    implicit_producer_hash_count_.fetch_sub(1, memory_order_relaxed);
                    return nullptr;
                }

                producer->thread_exit_listener_.callback = &lock_free_queue::implicit_producer_thread_exited_callback;
                producer->thread_exit_listener_.user_data = producer;
                thread_exit_notifier::subscribe(&producer->thread_exit_listener_);

                auto index = hashed_id;
                while (true) {
                    index &= main_hash->capacity_ - 1;
                    auto empty = inner::invalid_thread_id_zero;
#ifdef NEFORCE_PLATFORM_WINDOWS
                    auto reusable = inner::invalid_thread_id_max;
                    if (main_hash->entries_[index].key_.compare_exchange_strong(reusable, id, memory_order_seq_cst,
                                                                                memory_order_relaxed)) {
                        implicit_producer_hash_count_.fetch_sub(1, memory_order_relaxed);
                        main_hash->entries_[index].value_ = producer;
                        break;
                    }
#endif
                    if (main_hash->entries_[index].key_.compare_exchange_strong(empty, id, memory_order_seq_cst,
                                                                                memory_order_relaxed)) {
                        main_hash->entries_[index].value_ = producer;
                        break;
                    }
                    ++index;
                }
                return producer;
            }

            main_hash = implicit_producer_hash_.load(memory_order_acquire);
        }
    }

    void implicit_producer_thread_exited(implicit_producer* producer) {
        auto* hash = implicit_producer_hash_.load(memory_order_acquire);
        const auto id = this_thread::id();
        const auto hashed_id = id.to_hash();

        for (; hash != nullptr; hash = hash->prev_) {
            auto index = hashed_id;
            thread::id probed_key;
            do {
                index &= hash->capacity_ - 1;
                probed_key = id;
#ifdef NEFORCE_PLATFORM_WINDOWS
                if (hash->entries_[index].key_.compare_exchange_strong(probed_key, inner::invalid_thread_id_max,
                                                                       memory_order_seq_cst, memory_order_relaxed)) {
                    break;
                }
#else
                if (hash->entries_[index].key_.compare_exchange_strong(probed_key, inner::invalid_thread_id_zero,
                                                                       memory_order_seq_cst, memory_order_relaxed)) {
                    break;
                }
#endif
                ++index;
            } while (probed_key != inner::invalid_thread_id_zero);
        }

        producer->inactive_.store(true, memory_order_release);
    }

    static void implicit_producer_thread_exited_callback(void* user_data) {
        auto* producer = static_cast<implicit_producer*>(user_data);
        producer->parent_->implicit_producer_thread_exited(producer);
    }

    size_t ceil_to_pow_2_size_t(size_t x) { return inner::ceil_to_pow_2(x); }

public:
    /**
     * @brief 构造函数
     * @param capacity 预分配的元素容量
     *
     * 预分配指定容量的 block 池，减少运行时堆分配。
     * 容量按 BLOCK_SIZE 向上取整。
     */
    explicit lock_free_queue(size_t capacity = 32 * BLOCK_SIZE) {
        implicit_producer_hash_resize_in_progress_.clear(memory_order_relaxed);
        populate_initial_implicit_producer_hash();
        populate_initial_block_list(capacity / BLOCK_SIZE + ((capacity & (BLOCK_SIZE - 1)) == 0 ? 0 : 1));
    }

    /**
     * @brief 析构函数
     *
     * 销毁所有生产者、哈希表、回收池中的 block 和初始 block 池。
     *
     * @warning 析构时不能有并发操作
     */
    ~lock_free_queue() {
        auto ptr = producer_list_tail_.load(memory_order_relaxed);
        while (ptr != nullptr) {
            auto next = ptr->next_;
            delete ptr;
            ptr = next;
        }

        auto* hash = implicit_producer_hash_.load(memory_order_relaxed);
        while (hash != nullptr) {
            auto prev = hash->prev_;
            if (prev != nullptr) {
                for (size_t i = 0; i != hash->capacity_; ++i) {
                    hash->entries_[i].~implicit_producer_kvp();
                }
                hash->~implicit_producer_hash();
                ::operator delete(hash);
            }
            hash = prev;
        }

        auto* block = free_list_.head_unsafe();
        while (block != nullptr) {
            auto next = block->free_list_next_.load(memory_order_relaxed);
            if (block->dynamically_allocated_) {
                delete block;
            }
            block = next;
        }

        delete[] initial_block_pool_;
    }

    lock_free_queue(const lock_free_queue&) = delete;
    lock_free_queue& operator=(const lock_free_queue&) = delete;

    /**
     * @brief 入队操作
     * @param new_value 要入队的元素
     *
     * 为当前线程自动创建隐式生产者。
     *
     * @note 如果内存分配失败，入队操作会静默失败
     */
    void push(T new_value) {
        auto* producer = get_or_add_implicit_producer();
        if (producer != nullptr) {
            producer->enqueue(_NEFORCE move(new_value));
        }
    }

    /**
     * @brief 非阻塞出队操作
     * @return 出队元素的 unique_ptr，队列为空则返回空指针
     *
     * 使用启发式扫描生产者链表，选择元素最多的生产者优先出队。
     * 最多扫描 3 个非空生产者。
     */
    unique_ptr<T> try_pop() {
        T element;
        size_t non_empty_count = 0;
        implicit_producer* best = nullptr;
        size_t best_size = 0;

        for (auto ptr = producer_list_tail_.load(memory_order_acquire); ptr != nullptr && non_empty_count < 3;
             ptr = ptr->next_) {
            auto sz = ptr->size_approx();
            if (sz > 0) {
                if (sz > best_size) {
                    best_size = sz;
                    best = ptr;
                }
                ++non_empty_count;
            }
        }

        if (non_empty_count > 0) {
            if (best->dequeue(element)) {
                return unique_ptr<T>(new T(_NEFORCE move(element)));
            }
            for (auto ptr = producer_list_tail_.load(memory_order_acquire); ptr != nullptr; ptr = ptr->next_) {
                if (ptr != best && ptr->dequeue(element)) {
                    return unique_ptr<T>(new T(_NEFORCE move(element)));
                }
            }
        }

        return unique_ptr<T>();
    }

    /**
     * @brief 阻塞出队操作
     * @return 出队元素的 unique_ptr
     */
    unique_ptr<T> pop() {
        for (;;) {
            auto result = try_pop();
            if (result) {
                return result;
            }
            this_thread::relax();
        }
    }

    /**
     * @brief 检查队列是否为空
     * @return 队列为空返回 true
     * @note 由于并发特性，返回值可能瞬间失效
     */
    NEFORCE_NODISCARD bool empty() const {
        for (auto ptr = producer_list_tail_.load(memory_order_acquire); ptr != nullptr; ptr = ptr->next_) {
            if (ptr->size_approx() > 0) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief 获取队列中元素的近似数量
     * @return 元素数量的近似值
     * @note 由于并发特性，返回值可能瞬间失效
     */
    NEFORCE_NODISCARD size_t size() const {
        size_t total = 0;
        for (auto ptr = producer_list_tail_.load(memory_order_acquire); ptr != nullptr; ptr = ptr->next_) {
            total += ptr->size_approx();
        }
        return total;
    }

    /**
     * @brief 清空队列
     * @warning 此方法不是线程安全的，调用者需保证无并发操作
     */
    void clear() {
        while (try_pop()) {
            this_thread::relax();
        }
    }
};

/** @} */ // LockFreeQueue

/** @} */ // AsyncComponents

NEFORCE_END_NAMESPACE__
#endif
