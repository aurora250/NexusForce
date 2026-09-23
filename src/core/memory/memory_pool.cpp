#include <NeForce/core/memory/memory_pool.hpp>
#include <NeForce/core/algorithm/compare.hpp>
#include <NeForce/core/async/futex.hpp>
#include <NeForce/core/async/this_thread.hpp>
#include <new>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    ifdef NEFORCE_ARCH_X86_64
#        include <immintrin.h>
#    endif
#    include <windows.h>
#else
#    include <sys/mman.h>
#    include <unistd.h>
#endif
NEFORCE_BEGIN_NAMESPACE__
namespace {
    constexpr uint32_t g_region_flag_cached = 0x1U;                        // 大对象区域已进入复用缓存
    constexpr size_t g_max_request = static_cast<size_t>(1) << 46;         // 单次请求的字节数上限
    constexpr size_t g_slot_shift = 16;                                    // 地址映射表槽位位移（64 KiB）
    constexpr size_t g_span_header_size = 64;                              // span 头部占用字节数，块区从此偏移开始
    constexpr uint32_t g_span_flag_linked = 0x1U;                          // span 已挂入尺寸类链
    constexpr size_t g_map_l1_bits = 16;                                   // 地址映射表一级索引位数
    constexpr uintptr_t g_address_limit = static_cast<uintptr_t>(1) << 48; // 受支持的地址空间上界（256 TiB）

    // 各尺寸类的块大小（字节）
    constexpr uint32_t g_block_sizes[memory_pool::class_count] = {
            16,   32,   48,   64,   80,   96,   112,  128,  160,   192,   224,   256,
            320,  384,  448,  512,  640,  768,  896,  1024, 1280,  1536,  1792,  2048,
            2560, 3072, 3584, 4096, 5120, 6144, 7168, 8192, 10240, 12288, 14336, 16384};

    NEFORCE_INLINE17 constexpr uint8_t g_kind_none = 0;  // 槽位不属于任何内存池
    NEFORCE_INLINE17 constexpr uint8_t g_kind_small = 1; // 槽位不属于任何内存池
    NEFORCE_INLINE17 constexpr uint8_t g_kind_large = 2; // 槽位属于大对象区域

    NEFORCE_INLINE17 constexpr uint32_t g_span_magic = 0x4E465350U;   // span 头部魔数
    NEFORCE_INLINE17 constexpr uint32_t g_region_magic = 0x4E465247U; // 大对象区域头部魔数

    /// Both spans and large regions start at a slot aligned address, so a 64 KiB map slot
    /// always describes exactly one pool object and its entry is never ambiguous.
    constexpr size_t g_slot_size = static_cast<size_t>(1) << g_slot_shift;
    constexpr size_t g_map_l1_size = static_cast<size_t>(1) << g_map_l1_bits;
    constexpr size_t g_map_l2_size = static_cast<size_t>(1) << g_map_l1_bits;
    constexpr size_t g_map_l2_bytes = g_map_l2_size * sizeof(memory_pool::map_entry);

    /// Pool instances the registry can track simultaneously.
    constexpr size_t g_registry_slots = 256;

    /// Spin rounds before a contended lock falls back to a kernel wait.
    constexpr int g_lock_spin_rounds = 128;

    /// Target payload bytes per span, used to derive span geometry for every size class.
    constexpr size_t g_span_target_payload = 65536 - g_span_header_size;

    NEFORCE_ALWAYS_INLINE void lock_spin(memory_pool::spinlock& lock) noexcept {
        uint32_t expected = 0;
        for (int round = 0; round < g_lock_spin_rounds; ++round) {
            if (lock.value.compare_exchange_weak(expected, 1, memory_order_acquire, memory_order_relaxed)) {
                return;
            }
            expected = 0;
            this_thread::relax();
        }
        while (lock.value.exchange(2, memory_order_acquire) != 0) {
            _NEFORCE futex_wait(&lock.value, static_cast<platform_wait_t>(2));
        }
    }

    NEFORCE_ALWAYS_INLINE void unlock_spin(memory_pool::spinlock& lock) noexcept {
        if (lock.value.exchange(0, memory_order_release) == 2) {
            _NEFORCE futex_notify(&lock.value, true);
        }
    }

    struct scoped_spinlock {
        explicit scoped_spinlock(memory_pool::spinlock& target) noexcept :
        lock_(&target) {
            lock_spin(target);
        }
        ~scoped_spinlock() { unlock_spin(*lock_); }
        scoped_spinlock(const scoped_spinlock&) = delete;
        scoped_spinlock& operator=(const scoped_spinlock&) = delete;
        memory_pool::spinlock* lock_;
    };

    NEFORCE_ALWAYS_INLINE size_t page_size() noexcept {
        static size_t cached = []() -> size_t {
#ifdef NEFORCE_PLATFORM_WINDOWS
            ::SYSTEM_INFO info;
            ::GetSystemInfo(&info);
            return static_cast<size_t>(info.dwPageSize);
#else
            const long value = ::sysconf(_SC_PAGESIZE);
            return value > 0 ? static_cast<size_t>(value) : static_cast<size_t>(4096);
#endif
        }();
        return cached;
    }

    NEFORCE_ALWAYS_INLINE size_t round_up(const size_t value, const size_t align) noexcept {
        return (value + align - 1) / align * align;
    }

    NEFORCE_ALWAYS_INLINE size_t divide_round_up(const size_t value, const size_t align) noexcept {
        return (value + align - 1) / align;
    }

    NEFORCE_ALWAYS_INLINE void* os_map(const size_t bytes) noexcept {
        if (bytes == 0) {
            return nullptr;
        }
#ifdef NEFORCE_PLATFORM_WINDOWS
        return ::VirtualAlloc(nullptr, bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
        void* ptr = ::mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        return ptr == MAP_FAILED ? nullptr : ptr;
#endif
    }

    NEFORCE_ALWAYS_INLINE void os_unmap(void* ptr, const size_t bytes) noexcept {
        if (ptr == nullptr) {
            return;
        }
#ifdef NEFORCE_PLATFORM_WINDOWS
        static_cast<void>(bytes);
        ::VirtualFree(ptr, 0, MEM_RELEASE);
#else
        ::munmap(ptr, bytes);
#endif
    }

    void* os_map_aligned(const size_t bytes, const size_t align) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
        static_cast<void>(align);
        return os_map(bytes);
#else
        const size_t align_size = align < page_size() ? page_size() : align;
        const size_t total = bytes + align_size;
        auto* raw = static_cast<char*>(os_map(total));
        if (raw == nullptr) {
            return nullptr;
        }
        const auto base = reinterpret_cast<uintptr_t>(raw);
        const uintptr_t aligned = (base + align_size - 1) & ~(static_cast<uintptr_t>(align_size) - 1);
        const auto head = static_cast<size_t>(aligned - base);
        const size_t tail = total - head - bytes;
        if (head != 0) {
            os_unmap(raw, head);
        }
        if (tail != 0) {
            os_unmap(reinterpret_cast<void*>(aligned + bytes), tail);
        }
        return reinterpret_cast<void*>(aligned);
#endif
    }

    struct map_root {
        atomic<memory_pool::map_entry*> l1[g_map_l1_size];
    };

    atomic<map_root*>& map_root_slot() noexcept {
        static atomic<map_root*> slot;
        return slot;
    }

    memory_pool::spinlock& map_lock() noexcept {
        static memory_pool::spinlock lock;
        return lock;
    }

    NEFORCE_ALWAYS_INLINE uint64_t pack_tag(const uint32_t owner_id, const uint16_t class_index,
                                            const uint8_t kind) noexcept {
        return (static_cast<uint64_t>(owner_id) << 32) | (static_cast<uint64_t>(class_index) << 16) |
               static_cast<uint64_t>(kind);
    }

    map_root* map_root_get(const bool create) noexcept {
        map_root* root = map_root_slot().load(memory_order_acquire);
        if (root != nullptr || !create) {
            return root;
        }
        scoped_spinlock guard(map_lock());
        root = map_root_slot().load(memory_order_relaxed);
        if (root == nullptr) {
            auto* fresh = static_cast<map_root*>(os_map(sizeof(map_root)));
            if (fresh == nullptr) {
                return nullptr;
            }
            map_root_slot().store(fresh, memory_order_release);
            root = fresh;
        }
        return root;
    }

    memory_pool::map_entry* map_page_get(map_root& root, const size_t index, const bool create) noexcept {
        memory_pool::map_entry* page = root.l1[index].load(memory_order_acquire);
        if (page != nullptr || !create) {
            return page;
        }
        scoped_spinlock guard(map_lock());
        page = root.l1[index].load(memory_order_relaxed);
        if (page == nullptr) {
            auto* fresh = static_cast<memory_pool::map_entry*>(os_map(g_map_l2_bytes));
            if (fresh == nullptr) {
                return nullptr;
            }
            root.l1[index].store(fresh, memory_order_release);
            page = fresh;
        }
        return page;
    }

    NEFORCE_ALWAYS_INLINE size_t map_l1_index(const uintptr_t addr) noexcept {
        return static_cast<size_t>((addr >> 32) & (g_map_l1_size - 1));
    }

    NEFORCE_ALWAYS_INLINE size_t map_l2_index(const uintptr_t addr) noexcept {
        return static_cast<size_t>((addr >> g_slot_shift) & (g_map_l2_size - 1));
    }

    struct map_view {
        void* base;
        uint32_t owner_id;
        uint16_t class_index;
        uint8_t kind;
    };

    bool map_read(const void* ptr, map_view& out) noexcept {
        const auto addr = reinterpret_cast<uintptr_t>(ptr);
        if (addr >= g_address_limit) {
            return false;
        }
        const map_root* root = map_root_slot().load(memory_order_acquire);
        if (root == nullptr) {
            return false;
        }
        memory_pool::map_entry* page = root->l1[map_l1_index(addr)].load(memory_order_acquire);
        if (page == nullptr) {
            return false;
        }
        const memory_pool::map_entry& entry = page[map_l2_index(addr)];
        const uint64_t tag = entry.tag.load(memory_order_acquire);
        out.kind = static_cast<uint8_t>(tag & 0xFFU);
        if (out.kind == g_kind_none) {
            return false;
        }
        out.base = reinterpret_cast<void*>(entry.desc.load(memory_order_relaxed));
        if (out.base == nullptr) {
            return false;
        }
        out.class_index = static_cast<uint16_t>((tag >> 16) & 0xFFFFU);
        out.owner_id = static_cast<uint32_t>(tag >> 32);
        return true;
    }

    bool map_cover(void* ptr, const size_t bytes, void* base, const uint32_t owner_id, const uint16_t class_index,
                   const uint8_t kind) noexcept {
        map_root* root = map_root_get(true);
        if (root == nullptr) {
            return false;
        }
        const auto first = reinterpret_cast<uintptr_t>(ptr);
        const auto tag = pack_tag(owner_id, class_index, kind);
        const auto desc = reinterpret_cast<uint64_t>(base);
        const size_t slots = divide_round_up(bytes, g_slot_size);
        for (size_t index = 0; index < slots; ++index) {
            const uintptr_t addr = first + index * g_slot_size;
            if (addr >= g_address_limit) {
                return false;
            }
            memory_pool::map_entry* page = map_page_get(*root, map_l1_index(addr), true);
            if (page == nullptr) {
                return false;
            }
            memory_pool::map_entry& entry = page[map_l2_index(addr)];
            entry.desc.store(desc, memory_order_relaxed);
            entry.tag.store(tag, memory_order_release);
        }
        return true;
    }

    bool map_publish(const void* ptr, void* base, const uint32_t owner_id, const uint16_t class_index,
                     const uint8_t kind) noexcept {
        return map_cover(const_cast<void*>(ptr), 1, base, owner_id, class_index, kind);
    }

    void map_retire(const void* ptr) noexcept {
        const auto addr = reinterpret_cast<uintptr_t>(ptr);
        if (addr >= g_address_limit) {
            return;
        }
        const map_root* root = map_root_slot().load(memory_order_acquire);
        if (root == nullptr) {
            return;
        }
        memory_pool::map_entry* page = root->l1[map_l1_index(addr)].load(memory_order_acquire);
        if (page == nullptr) {
            return;
        }
        memory_pool::map_entry& entry = page[map_l2_index(addr)];
        entry.tag.store(pack_tag(0, 0, g_kind_none), memory_order_release);
        entry.desc.store(0, memory_order_relaxed);
    }

    void map_retire_range(void* ptr, const size_t bytes) noexcept {
        const auto first = reinterpret_cast<uintptr_t>(ptr);
        const auto slots = divide_round_up(bytes, g_slot_size);
        for (size_t index = 0; index < slots; ++index) {
            map_retire(reinterpret_cast<void*>(first + index * g_slot_size));
        }
    }

    atomic<uint32_t>* registry_ids() noexcept {
        static atomic<uint32_t> ids[g_registry_slots];
        return ids;
    }

    atomic<memory_pool*>* registry_pools() noexcept {
        static atomic<memory_pool*> pools[g_registry_slots];
        return pools;
    }

    memory_pool::spinlock& registry_lock() noexcept {
        static memory_pool::spinlock lock;
        return lock;
    }

    uint32_t& next_pool_id() noexcept {
        static uint32_t value = 1;
        return value;
    }

    void registry_insert(memory_pool* pool, const uint32_t id) noexcept {
        scoped_spinlock guard(registry_lock());
        for (size_t index = 0; index < g_registry_slots; ++index) {
            if (registry_ids()[index].load(memory_order_relaxed) == 0) {
                registry_pools()[index].store(pool, memory_order_relaxed);
                registry_ids()[index].store(id, memory_order_release);
                return;
            }
        }
    }

    void registry_remove(const uint32_t id) noexcept {
        scoped_spinlock guard(registry_lock());
        for (size_t index = 0; index < g_registry_slots; ++index) {
            if (registry_ids()[index].load(memory_order_relaxed) == id) {
                registry_ids()[index].store(0, memory_order_release);
                registry_pools()[index].store(nullptr, memory_order_relaxed);
                return;
            }
        }
    }

    uint32_t registry_next_id() noexcept {
        scoped_spinlock guard(registry_lock());
        for (;;) {
            const uint32_t id = next_pool_id();
            ++next_pool_id();
            if (next_pool_id() == 0) {
                next_pool_id() = 1;
            }
            if (id != 0) {
                return id;
            }
        }
    }

    memory_pool* registry_lookup(const uint32_t id) noexcept {
        if (id == 0) {
            return nullptr;
        }
        for (size_t index = 0; index < g_registry_slots; ++index) {
            if (registry_ids()[index].load(memory_order_acquire) == id) {
                return registry_pools()[index].load(memory_order_relaxed);
            }
        }
        return nullptr;
    }

    void update_peak(atomic<size_t>& peak, const size_t value) noexcept {
        size_t current = peak.load(memory_order_relaxed);
        while (current < value) {
            if (peak.compare_exchange_weak(current, value, memory_order_relaxed, memory_order_relaxed)) {
                break;
            }
        }
    }

    void list_push_front(memory_pool::class_state& state, memory_pool::span_header* span) noexcept {
        span->prev = nullptr;
        span->next = state.partial;
        if (state.partial != nullptr) {
            state.partial->prev = span;
        } else {
            state.tail = span;
        }
        state.partial = span;
        span->flags |= g_span_flag_linked;
    }

    void list_push_back(memory_pool::class_state& state, memory_pool::span_header* span) noexcept {
        span->next = nullptr;
        span->prev = state.tail;
        if (state.tail != nullptr) {
            state.tail->next = span;
        } else {
            state.partial = span;
        }
        state.tail = span;
        span->flags |= g_span_flag_linked;
    }

    void list_unlink(memory_pool::class_state& state, memory_pool::span_header* span) noexcept {
        if ((span->flags & g_span_flag_linked) == 0) {
            return;
        }
        if (span->prev != nullptr) {
            span->prev->next = span->next;
        } else {
            state.partial = span->next;
        }
        if (span->next != nullptr) {
            span->next->prev = span->prev;
        } else {
            state.tail = span->prev;
        }
        span->next = nullptr;
        span->prev = nullptr;
        span->flags &= ~g_span_flag_linked;
    }

    alignas(memory_pool) byte_t g_system_pool_storage[sizeof(memory_pool)];

    uint8_t g_system_pool_built = 0;

    atomic<memory_pool*>& system_pool_slot() noexcept {
        static atomic<memory_pool*> slot;
        return slot;
    }

    memory_pool::spinlock& system_pool_lock() noexcept {
        static memory_pool::spinlock lock;
        return lock;
    }
} // namespace


memory_pool::memory_pool() noexcept { initialize(options()); }

memory_pool::memory_pool(const options& opts) noexcept { initialize(opts); }

memory_pool::~memory_pool() {
    alive_ = 0;
    registry_remove(id_);
    release_all();
}

void memory_pool::initialize(const options& opts) noexcept {
    id_ = registry_next_id();
    alive_ = 1;
    options_ = opts;
    if (options_.thread_cache_batch == 0) {
        options_.thread_cache_batch = 1;
    }
    options_.thread_cache_max = max(options_.thread_cache_max, options_.thread_cache_batch);
    options_.max_cached_regions = min(options_.max_cached_regions, region_cache_slots - 1);

    const size_t page = page_size();
    for (size_t index = 0; index < class_count; ++index) {
        class_state& state = classes_[index];
        state.lock.value.store(0, memory_order_relaxed);
        state.partial = nullptr;
        state.tail = nullptr;
        state.span_count = 0;
        state.empty_count = 0;
        const size_t block = g_block_sizes[index];
        size_t count = g_span_target_payload / block;
        count = max<size_t>(count, 2);
        const size_t span = round_up(g_span_header_size + count * block, page);
        state.span_size = span;
        state.block_count = (span - g_span_header_size) / block;
        // Small classes refill in larger batches so that a refill, which takes the class lock,
        // is amortized over many operations; the batch is capped so no thread hoards a whole span.
        size_t batch = (options_.thread_cache_bytes + block - 1) / block;
        batch = batch < options_.thread_cache_batch ? options_.thread_cache_batch : batch;
        batch = batch > 256 ? 256 : batch;
        batch = min(batch, state.block_count);
        state.batch = static_cast<uint32_t>(batch);
        size_t cache_max = (options_.thread_cache_max_bytes + block - 1) / block;
        const size_t cache_floor = batch * 2;
        cache_max = cache_max < cache_floor ? cache_floor : cache_max;
        cache_max = cache_max < options_.thread_cache_max ? options_.thread_cache_max : cache_max;
        cache_max = min<size_t>(cache_max, 4096);
        state.cache_max = static_cast<uint32_t>(cache_max);
    }

    all_spans_ = nullptr;
    all_regions_ = nullptr;
    region_cache_count_ = 0;
    registry_lock_.value.store(0, memory_order_relaxed);
    region_lock_.value.store(0, memory_order_relaxed);
    os_mapped_bytes_.store(0, memory_order_relaxed);
    peak_mapped_bytes_.store(0, memory_order_relaxed);
    small_mapped_bytes_.store(0, memory_order_relaxed);
    large_mapped_bytes_.store(0, memory_order_relaxed);
    active_bytes_.store(0, memory_order_relaxed);
    empty_bytes_.store(0, memory_order_relaxed);
    peak_active_bytes_.store(0, memory_order_relaxed);
    os_map_calls_.store(0, memory_order_relaxed);
    os_unmap_calls_.store(0, memory_order_relaxed);
    registry_insert(this, id_);
}

size_t memory_pool::block_size(const size_t class_index) noexcept {
    return class_index < class_count ? g_block_sizes[class_index] : 0;
}


uint32_t memory_pool::id() const noexcept { return id_; }

const memory_pool::options& memory_pool::config() const noexcept { return options_; }

memory_pool::thread_cache& memory_pool::current_cache() noexcept {
    struct holder {
        thread_cache cache{};
        holder() noexcept { cache.owner_id = 0; }
        ~holder() { memory_pool::release_thread_cache(cache); }
    };
    // The default global dynamic TLS model calls __tls_get_addr on every cache access,
    // which is a measurable part of a single allocation.
    // The initial exec model resolves the offset once at load time instead.
    // Define NEFORCE_MEMORY_POOL_TLS_GLOBAL_DYNAMIC to go back to the default model,
    // which is what a build that must survive an arbitrary dlopen() of this library needs.
#if defined(NEFORCE_PLATFORM_LINUX) && defined(NEFORCE_COMPILER_GNUC) && \
        !defined(NEFORCE_MEMORY_POOL_TLS_GLOBAL_DYNAMIC)
    thread_local holder instance __attribute__((tls_model("initial-exec")));
#else
    thread_local holder instance;
#endif
    return instance.cache;
}

memory_pool* memory_pool::pool_for_id(const uint32_t id) noexcept { return registry_lookup(id); }

void memory_pool::release_thread_cache(thread_cache& cache) noexcept {
    memory_pool* owner = pool_for_id(cache.owner_id);
    if (owner != nullptr) {
        for (size_t index = 0; index < class_count; ++index) {
            if (cache.entries[index].count != 0) {
                owner->return_blocks(index, cache.entries[index].list, cache.entries[index].count);
            }
            cache.entries[index].list = nullptr;
            cache.entries[index].count = 0;
        }
        for (uint32_t index = 0; index < cache.region_count; ++index) {
            if (cache.regions[index].region != nullptr) {
                cache.regions[index].region->flags &= ~g_region_flag_cached;
                owner->return_region(cache.regions[index].region);
                cache.regions[index].region = nullptr;
                cache.regions[index].region_size = 0;
            }
        }
    } else {
        for (size_t index = 0; index < class_count; ++index) {
            cache.entries[index].list = nullptr;
            cache.entries[index].count = 0;
        }
    }
    cache.region_count = 0;
    cache.owner_id = 0;
}

bool memory_pool::bind_cache(thread_cache& cache) noexcept {
    if (cache.owner_id == id_) {
        return true;
    }
    release_thread_cache(cache);
    cache.owner_id = id_;
    return true;
}

void* memory_pool::allocate_small(thread_cache& cache, const size_t class_index) noexcept {
    if (cache.owner_id != id_) {
        bind_cache(cache);
    }
    if (cache.entries[class_index].count != 0) {
        void* block = cache.entries[class_index].list;
        cache.entries[class_index].list = *static_cast<void**>(block);
        --cache.entries[class_index].count;
        return block;
    }
    return refill(cache, class_index);
}


memory_pool::span_header* memory_pool::create_span(const size_t class_index) noexcept {
    class_state& state = classes_[class_index];
    const size_t block = g_block_sizes[class_index];
    void* base = os_map_aligned(state.span_size, g_slot_size);
    if (base == nullptr) {
        return nullptr;
    }
    auto* span = static_cast<span_header*>(base);
    span->magic = g_span_magic;
    span->owner_id = id_;
    span->class_index = static_cast<uint32_t>(class_index);
    span->span_size = static_cast<uint32_t>(state.span_size);
    span->block_count = static_cast<uint32_t>(state.block_count);
    span->flags = 0;
    span->next = nullptr;
    span->prev = nullptr;
    span->global_next = nullptr;

    char* area = static_cast<char*>(base) + g_span_header_size;
    void* head = nullptr;
    for (size_t index = state.block_count; index > 0; --index) {
        void* block_ptr = area + (index - 1) * block;
        *static_cast<void**>(block_ptr) = head;
        head = block_ptr;
    }
    span->free_head = head;
    span->free_count.store(static_cast<uint32_t>(state.block_count), memory_order_relaxed);
    span->used_count.store(0, memory_order_relaxed);

    if (!map_cover(base, state.span_size, base, id_, static_cast<uint16_t>(class_index), g_kind_small)) {
        os_unmap(base, state.span_size);
        return nullptr;
    }

    os_map_calls_.fetch_add(1, memory_order_relaxed);
    os_mapped_bytes_.fetch_add(state.span_size, memory_order_relaxed);
    small_mapped_bytes_.fetch_add(state.span_size, memory_order_relaxed);
    update_peak(peak_mapped_bytes_, os_mapped_bytes_.load(memory_order_relaxed));
    {
        scoped_spinlock guard(registry_lock_);
        span->global_next = all_spans_;
        all_spans_ = span;
    }
    ++state.span_count;
    return span;
}

void memory_pool::destroy_span(span_header* span) noexcept {
    if (span == nullptr) {
        return;
    }
    const size_t span_size = span->span_size;
    {
        scoped_spinlock guard(registry_lock_);
        span_header** link = &all_spans_;
        while (*link != nullptr && *link != span) {
            link = &(*link)->global_next;
        }
        if (*link == span) {
            *link = span->global_next;
        }
    }
    map_retire_range(span, span_size);
    span->magic = 0;
    os_unmap(span, span_size);
    os_unmap_calls_.fetch_add(1, memory_order_relaxed);
    os_mapped_bytes_.fetch_sub(span_size, memory_order_relaxed);
    small_mapped_bytes_.fetch_sub(span_size, memory_order_relaxed);
}

void* memory_pool::refill(thread_cache& cache, const size_t class_index) noexcept {
    class_state& state = classes_[class_index];
    void* batch = nullptr;
    uint32_t taken = 0;
    {
        scoped_spinlock guard(state.lock);
        span_header* span = state.partial;
        if (span == nullptr) {
            span = create_span(class_index);
            if (span == nullptr) {
                return nullptr;
            }
            list_push_front(state, span);
        }
        uint32_t available = span->free_count.load(memory_order_relaxed);
        if (available == 0) {
            list_unlink(state, span);
            span = create_span(class_index);
            if (span == nullptr) {
                return nullptr;
            }
            list_push_front(state, span);
            available = span->free_count.load(memory_order_relaxed);
        }
        if (state.empty_count != 0 && span->used_count.load(memory_order_relaxed) == 0) {
            --state.empty_count;
            empty_bytes_.fetch_sub(span->span_size, memory_order_relaxed);
            list_unlink(state, span);
            list_push_front(state, span);
        }
        uint32_t want = options_.thread_cache_enabled ? state.batch : 1U;
        want = min(want, available);
        void* head = span->free_head;
        void* tail = head;
        for (uint32_t index = 1; index < want; ++index) {
            tail = *static_cast<void**>(tail);
        }
        span->free_head = *static_cast<void**>(tail);
        *static_cast<void**>(tail) = nullptr;
        const uint32_t remaining = available - want;
        span->free_count.store(remaining, memory_order_relaxed);
        span->used_count.fetch_add(want, memory_order_relaxed);
        if (remaining == 0) {
            list_unlink(state, span);
        }
        batch = head;
        taken = want;
    }

    const size_t bytes = static_cast<size_t>(taken) * static_cast<size_t>(g_block_sizes[class_index]);
    const size_t active = active_bytes_.fetch_add(bytes, memory_order_relaxed) + bytes;
    update_peak(peak_active_bytes_, active);

    cache.entries[class_index].list = taken > 1 ? *static_cast<void**>(batch) : nullptr;
    cache.entries[class_index].count = taken - 1;
    return batch;
}

void memory_pool::return_blocks(const size_t class_index, void* head, const size_t count) noexcept {
    if (head == nullptr || count == 0) {
        return;
    }
    class_state& state = classes_[class_index];
    scoped_spinlock guard(state.lock);
    void* node = head;
    size_t handled = 0;
    size_t accepted = 0;
    // Blocks of one flush batch usually come from a handful of spans,
    // so the address map is consulted once per 64 KiB slot instead of once per block.
    // The cached span cannot be torn down while the batch is in flight,
    // because every block handed to a thread cache keeps its span accounted as used.
    constexpr uintptr_t slot_mask = ~(static_cast<uintptr_t>(g_slot_size) - 1);
    uintptr_t cached_slot = ~static_cast<uintptr_t>(0);
    span_header* cached_span = nullptr;
    while (node != nullptr && handled < count) {
        const uintptr_t slot = reinterpret_cast<uintptr_t>(node) & slot_mask;
        if (slot != cached_slot) {
            map_view view;
            cached_slot = slot;
            cached_span = nullptr;
            if (map_read(node, view) && view.kind == g_kind_small && view.owner_id == id_ &&
                view.class_index == class_index) {
                cached_span = static_cast<span_header*>(view.base);
            }
        }
        if (cached_span == nullptr) {
            node = *static_cast<void**>(node);
            ++handled;
            continue;
        }
        // Blocks of one span arrive consecutively,
        // so the whole run is spliced onto the span free list and accounted with a single update instead of one update per block.
        // That keeps the critical section short, which is what the many-thread case pays for.
        void* run_head = node;
        void* run_tail = node;
        size_t run_length = 1;
        void* next = *static_cast<void**>(node);
        while (next != nullptr && handled + run_length < count &&
               (reinterpret_cast<uintptr_t>(next) & slot_mask) == slot) {
            run_tail = next;
            ++run_length;
            next = *static_cast<void**>(next);
        }
        auto* span = cached_span;
        const uint32_t free_before = span->free_count.load(memory_order_relaxed);
        const uint32_t used_before = span->used_count.load(memory_order_relaxed);
        NEFORCE_DEBUG_VERIFY(free_before + run_length <= span->block_count, "memory pool detected a duplicated block.");
        if (free_before + run_length <= span->block_count) {
            if (free_before == 0) {
                list_push_front(state, span);
            }
            *static_cast<void**>(run_tail) = span->free_head;
            span->free_head = run_head;
            span->free_count.store(free_before + static_cast<uint32_t>(run_length), memory_order_relaxed);
            const uint32_t used_after = used_before > run_length ? used_before - static_cast<uint32_t>(run_length) : 0;
            span->used_count.store(used_after, memory_order_relaxed);
            if (used_after == 0) {
                if ((span->flags & g_span_flag_linked) != 0) {
                    list_unlink(state, span);
                }
                list_push_back(state, span);
                ++state.empty_count;
                empty_bytes_.fetch_add(span->span_size, memory_order_relaxed);
            }
            accepted += run_length;
        }
        node = next;
        handled += run_length;
    }

    if (accepted != 0) {
        active_bytes_.fetch_sub(accepted * g_block_sizes[class_index], memory_order_relaxed);
    }

    // Bursty workloads hand out many spans, then release them all.
    // Keeping only a single empty span per size class forces those spans to be mapped again for the next burst,
    // which costs a page fault per block. Retaining up to a byte budget keeps the peak working set reusable
    // while the accounting stays bounded and purge() still returns everything.
    // Bursty workloads hand out many spans and then release them all.
    // Keeping a single empty span per size class forces those spans to be mapped again for the next burst,
    // which costs a page fault per block. Spans are therefore retained while the pool wide empty span budget allows it,
    // which keeps a peak working set reusable while both the per class floor and the global budget stay bounded,
    // and purge() still returns everything.
    // Every size class keeps one warm span: dropping the floor for the classes with larger spans
    // made those spans be mapped again for every burst, which costs far more than the retained mapping.
    // The pool wide byte budget bounds the total instead.
    const size_t floor_spans = options_.purge_on_empty ? 0 : options_.max_empty_spans;
    const size_t budget = options_.purge_on_empty ? 0 : options_.max_empty_span_bytes;
    while (state.empty_count > floor_spans && state.tail != nullptr &&
           state.tail->used_count.load(memory_order_relaxed) == 0 &&
           (budget == 0 || empty_bytes_.load(memory_order_relaxed) > budget)) {
        span_header* span = state.tail;
        list_unlink(state, span);
        --state.empty_count;
        --state.span_count;
        empty_bytes_.fetch_sub(span->span_size, memory_order_relaxed);
        destroy_span(span);
    }
}

void memory_pool::deallocate(void* ptr, const size_t bytes) noexcept {
    static_cast<void>(bytes);
    if (ptr == nullptr) {
        return;
    }
    map_view view;
    if (!map_read(ptr, view)) {
        NEFORCE_DEBUG_VERIFY(false, "memory pool received a pointer it does not own.");
        return;
    }

    if (view.kind == g_kind_large) {
        memory_pool* owner = pool_for_id(view.owner_id);
        if (owner != nullptr) {
            owner->deallocate_large(static_cast<region_header*>(view.base));
        }
        return;
    }

    if (view.class_index >= class_count) {
        NEFORCE_DEBUG_VERIFY(false, "memory pool received a pointer with an invalid size class.");
        return;
    }

    if (view.owner_id != id_) {
        memory_pool* owner = pool_for_id(view.owner_id);
        if (owner != nullptr) {
            owner->deallocate(ptr, bytes);
        }
        return;
    }

    const size_t class_index = view.class_index;
    if (!options_.thread_cache_enabled) {
        return_blocks(class_index, ptr, 1);
        return;
    }

    thread_cache& cache = current_cache();
    if (cache.owner_id != id_) {
        bind_cache(cache);
    }
    if (cache.entries[class_index].count >= classes_[class_index].cache_max) {
        const uint32_t total = cache.entries[class_index].count;
        const uint32_t give = total / 2;
        void* head = cache.entries[class_index].list;
        void* tail = head;
        for (uint32_t index = 1; index < give; ++index) {
            tail = *static_cast<void**>(tail);
        }
        cache.entries[class_index].list = *static_cast<void**>(tail);
        *static_cast<void**>(tail) = nullptr;
        cache.entries[class_index].count = total - give;
        return_blocks(class_index, head, give);
    }
    *static_cast<void**>(ptr) = cache.entries[class_index].list;
    cache.entries[class_index].list = ptr;
    ++cache.entries[class_index].count;
}

void* memory_pool::allocate_large(const size_t bytes, const size_t align) noexcept {
    const size_t alignment = align < min_align ? min_align : align;
    if (bytes > g_max_request || alignment > g_max_request) {
        return nullptr;
    }
    const size_t need = round_up(sizeof(region_header) + alignment + bytes, page_size());
    region_header* region = nullptr;

    // A per thread region cache keeps the repeated allocate / release pattern of medium and large buffers
    // off the pool wide lock entirely. It is bounded by both the per entry byte cap and the slot count,
    // and the entries are handed back when the thread exits.
    if (options_.region_cache_enabled && need <= options_.thread_region_bytes) {
        thread_cache& cache = current_cache();
        if (cache.owner_id == id_) {
            auto best = static_cast<size_t>(-1);
            auto slot = static_cast<size_t>(cache.region_count);
            for (uint32_t index = 0; index < cache.region_count; ++index) {
                const size_t size = cache.regions[index].region_size;
                if (size >= need && size < best) {
                    best = size;
                    slot = index;
                }
            }
            if (slot < cache.region_count) {
                region = cache.regions[slot].region;
                cache.regions[slot] = cache.regions[cache.region_count - 1];
                cache.regions[cache.region_count - 1].region = nullptr;
                cache.regions[cache.region_count - 1].region_size = 0;
                --cache.region_count;
                region->flags &= ~g_region_flag_cached;
                region->next = nullptr;
            }
        }
    }

    if (region == nullptr && options_.region_cache_enabled) {
        scoped_spinlock guard(region_lock_);
        auto best = static_cast<size_t>(-1);
        auto slot = region_cache_count_;
        for (size_t index = 0; index < region_cache_count_; ++index) {
            const size_t size = region_cache_[index].region_size;
            if (size >= need && size < best) {
                best = size;
                slot = index;
            }
        }
        if (slot < region_cache_count_) {
            region = region_cache_[slot].region;
            region_cache_[slot] = region_cache_[region_cache_count_ - 1];
            region_cache_[region_cache_count_ - 1].region = nullptr;
            region_cache_[region_cache_count_ - 1].region_size = 0;
            --region_cache_count_;
            region->flags &= ~g_region_flag_cached;
            region->next = nullptr;
        }
    }

    if (region == nullptr) {
        void* base = os_map_aligned(need, g_slot_size);
        if (base == nullptr) {
            return nullptr;
        }
        region = static_cast<region_header*>(base);
        region->magic = g_region_magic;
        region->owner_id = id_;
        region->region_size = need;
        region->flags = 0;
        region->base = base;
        region->next = nullptr;
        region->global_next = nullptr;
        region->global_prev = nullptr;
        os_map_calls_.fetch_add(1, memory_order_relaxed);
        const size_t mapped = os_mapped_bytes_.fetch_add(need, memory_order_relaxed) + need;
        large_mapped_bytes_.fetch_add(need, memory_order_relaxed);
        update_peak(peak_mapped_bytes_, mapped);
        scoped_spinlock guard(region_lock_);
        region->global_next = all_regions_;
        if (all_regions_ != nullptr) {
            all_regions_->global_prev = region;
        }
        all_regions_ = region;
    }

    const auto base_addr = reinterpret_cast<uintptr_t>(region->base);
    const auto payload = round_up(base_addr + sizeof(region_header), alignment);
    region->payload_offset = static_cast<uint32_t>(payload - base_addr);
    if (!map_publish(reinterpret_cast<void*>(payload), region, id_, 0, g_kind_large)) {
        deallocate_large(region);
        return nullptr;
    }
    return reinterpret_cast<void*>(payload);
}

void memory_pool::release_region(region_header* region) noexcept {
    if (region == nullptr) {
        return;
    }
    const auto size = static_cast<size_t>(region->region_size);
    void* base = region->base;
    {
        scoped_spinlock guard(region_lock_);
        if (region->global_prev != nullptr) {
            region->global_prev->global_next = region->global_next;
        } else if (all_regions_ == region) {
            all_regions_ = region->global_next;
        }
        if (region->global_next != nullptr) {
            region->global_next->global_prev = region->global_prev;
        }
        region->global_next = nullptr;
        region->global_prev = nullptr;
        region->magic = 0;
    }
    os_unmap(base, size);
    os_unmap_calls_.fetch_add(1, memory_order_relaxed);
    os_mapped_bytes_.fetch_sub(size, memory_order_relaxed);
    large_mapped_bytes_.fetch_sub(size, memory_order_relaxed);
}

void memory_pool::deallocate_large(region_header* region) noexcept {
    if (region == nullptr || region->magic != g_region_magic || region->owner_id != id_) {
        NEFORCE_DEBUG_VERIFY(false, "memory pool received an invalid region.");
        return;
    }
    map_retire(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(region->base) + region->payload_offset));
    if (options_.region_cache_enabled && region->region_size <= options_.thread_region_bytes) {
        thread_cache& cache = current_cache();
        if (cache.owner_id == id_ && cache.region_count < thread_region_slots) {
            region->flags |= g_region_flag_cached;
            region->next = nullptr;
            cache.regions[cache.region_count].region = region;
            cache.regions[cache.region_count].region_size = static_cast<size_t>(region->region_size);
            ++cache.region_count;
            return;
        }
    }
    return_region(region);
}

void memory_pool::return_region(region_header* region) noexcept {
    bool cached = false;
    if (options_.region_cache_enabled) {
        scoped_spinlock guard(region_lock_);
        if (region_cache_count_ < options_.max_cached_regions) {
            region->flags |= g_region_flag_cached;
            region->next = nullptr;
            region_cache_[region_cache_count_].region = region;
            region_cache_[region_cache_count_].region_size = static_cast<size_t>(region->region_size);
            ++region_cache_count_;
            cached = true;
        }
    }
    if (!cached) {
        release_region(region);
    }
}

size_t memory_pool::usable_size(const void* ptr) const noexcept {
    if (ptr == nullptr) {
        return 0;
    }
    map_view view;
    if (!map_read(ptr, view) || view.owner_id != id_) {
        return 0;
    }
    if (view.kind == g_kind_large) {
        const auto* region = static_cast<region_header*>(view.base);
        return static_cast<size_t>(region->region_size) - region->payload_offset;
    }
    if (view.class_index >= class_count) {
        return 0;
    }
    return g_block_sizes[view.class_index];
}

bool memory_pool::owns(const void* ptr) const noexcept {
    if (ptr == nullptr) {
        return false;
    }
    map_view view;
    return map_read(ptr, view) && view.owner_id == id_;
}

void* memory_pool::reallocate(void* ptr, const size_t bytes, const size_t align) {
    if (ptr == nullptr) {
        return allocate(bytes, align);
    }
    const size_t alignment = align < min_align ? min_align : align;
    const size_t capacity = usable_size(ptr);
    if (capacity >= bytes && (reinterpret_cast<uintptr_t>(ptr) & (alignment - 1)) == 0) {
        return ptr;
    }
    void* fresh = allocate(bytes, align);
    const size_t copy = capacity < bytes ? capacity : bytes;
    if (copy != 0) {
        memory_copy(fresh, ptr, copy);
    }
    deallocate(ptr);
    return fresh;
}

void memory_pool::flush_thread_cache() noexcept { release_thread_cache(current_cache()); }

void memory_pool::purge() noexcept {
    for (size_t index = 0; index < class_count; ++index) {
        class_state& state = classes_[index];
        scoped_spinlock guard(state.lock);
        while (state.tail != nullptr && state.tail->used_count.load(memory_order_relaxed) == 0) {
            span_header* span = state.tail;
            list_unlink(state, span);
            if (state.empty_count != 0) {
                --state.empty_count;
            }
            empty_bytes_.fetch_sub(span->span_size, memory_order_relaxed);
            if (state.span_count != 0) {
                --state.span_count;
            }
            destroy_span(span);
        }
    }

    region_header* cached[region_cache_slots];
    size_t cached_count = 0;
    {
        scoped_spinlock guard(region_lock_);
        for (size_t index = 0; index < region_cache_count_; ++index) {
            cached[cached_count] = region_cache_[index].region;
            ++cached_count;
            region_cache_[index].region = nullptr;
            region_cache_[index].region_size = 0;
        }
        region_cache_count_ = 0;
    }
    for (size_t index = 0; index < cached_count; ++index) {
        cached[index]->flags &= ~g_region_flag_cached;
        release_region(cached[index]);
    }
}

void memory_pool::release_all() noexcept {
    for (size_t index = 0; index < class_count; ++index) {
        classes_[index].partial = nullptr;
        classes_[index].tail = nullptr;
        classes_[index].span_count = 0;
        classes_[index].empty_count = 0;
    }
    span_header* span = all_spans_;
    all_spans_ = nullptr;
    while (span != nullptr) {
        span_header* next = span->global_next;
        const size_t span_size = span->span_size;
        map_retire_range(span, span_size);
        span->magic = 0;
        os_unmap(span, span_size);
        span = next;
    }
    region_header* region = all_regions_;
    all_regions_ = nullptr;
    while (region != nullptr) {
        region_header* next = region->global_next;
        const auto region_size = static_cast<size_t>(region->region_size);
        map_retire(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(region->base) + region->payload_offset));
        region->magic = 0;
        os_unmap(region->base, region_size);
        region = next;
    }
    region_cache_count_ = 0;
}

memory_pool::statistics memory_pool::stats() noexcept {
    statistics result;
    for (size_t index = 0; index < class_count; ++index) {
        result.classes[index].block_size = g_block_sizes[index];
        result.classes[index].span_size = classes_[index].span_size;
    }

    {
        scoped_spinlock guard(registry_lock_);
        for (const span_header* span = all_spans_; span != nullptr; span = span->global_next) {
            const size_t class_index = span->class_index;
            if (class_index >= class_count) {
                continue;
            }
            const size_t used = span->used_count.load(memory_order_relaxed);
            const size_t bytes = used * g_block_sizes[class_index];
            result.classes[class_index].active_blocks += used;
            result.classes[class_index].active_bytes += bytes;
            result.classes[class_index].span_count += 1;
            result.active_blocks += used;
            result.active_bytes += bytes;
            if (used == 0) {
                result.classes[class_index].empty_spans += 1;
                result.cached_empty_bytes += span->span_size;
            }
            result.small_mapped_bytes += span->span_size;
        }
    }

    {
        scoped_spinlock guard(region_lock_);
        for (const region_header* region = all_regions_; region != nullptr; region = region->global_next) {
            const auto size = static_cast<size_t>(region->region_size);
            result.large_mapped_bytes += size;
            if ((region->flags & g_region_flag_cached) != 0) {
                result.cached_region_bytes += size;
            }
        }
    }

    result.mapped_bytes = os_mapped_bytes_.load(memory_order_relaxed);
    result.peak_mapped_bytes = peak_mapped_bytes_.load(memory_order_relaxed);
    result.peak_active_bytes = peak_active_bytes_.load(memory_order_relaxed);
    result.os_map_calls = os_map_calls_.load(memory_order_relaxed);
    result.os_unmap_calls = os_unmap_calls_.load(memory_order_relaxed);
    return result;
}

bool memory_pool::verify() noexcept {
    bool healthy = true;
    size_t counted[class_count];
    for (size_t index = 0; index < class_count; ++index) {
        counted[index] = 0;
    }

    {
        scoped_spinlock guard(registry_lock_);
        for (const span_header* span = all_spans_; span != nullptr; span = span->global_next) {
            const size_t class_index = span->class_index;
            if (class_index >= class_count) {
                healthy = false;
                continue;
            }
            ++counted[class_index];
            if (span->magic != g_span_magic || span->owner_id != id_) {
                healthy = false;
                continue;
            }
            const class_state& state = classes_[class_index];
            if (span->span_size != state.span_size || span->block_count != state.block_count) {
                healthy = false;
                continue;
            }
            const size_t free_count = span->free_count.load(memory_order_relaxed);
            const size_t used_count = span->used_count.load(memory_order_relaxed);
            if (free_count + used_count != span->block_count) {
                healthy = false;
                continue;
            }
            const size_t block = g_block_sizes[class_index];
            const auto* base = reinterpret_cast<const char*>(span);
            size_t walked = 0;
            const void* node = span->free_head;
            while (node != nullptr && walked <= free_count) {
                const auto offset = static_cast<size_t>(static_cast<const char*>(node) - base);
                if (offset < g_span_header_size || offset >= span->span_size ||
                    ((offset - g_span_header_size) % block) != 0) {
                    healthy = false;
                    break;
                }
                node = *static_cast<const void* const*>(node);
                ++walked;
            }
            if (walked != free_count || node != nullptr) {
                healthy = false;
            }
            map_view view;
            if (!map_read(base, view) || view.base != span || view.kind != g_kind_small || view.owner_id != id_ ||
                view.class_index != class_index) {
                healthy = false;
            }
        }
    }

    for (size_t index = 0; index < class_count; ++index) {
        class_state& state = classes_[index];
        scoped_spinlock guard(state.lock);
        if (state.span_count != counted[index]) {
            healthy = false;
        }
        if (state.partial == nullptr && state.tail != nullptr) {
            healthy = false;
        }
        if (state.tail != nullptr && state.tail->next != nullptr) {
            healthy = false;
        }
        size_t empty = 0;
        bool seen_empty = false;
        for (const span_header* span = state.partial; span != nullptr; span = span->next) {
            if (span->class_index != index || (span->flags & g_span_flag_linked) == 0) {
                healthy = false;
            }
            const size_t used = span->used_count.load(memory_order_relaxed);
            if (span->free_count.load(memory_order_relaxed) == 0) {
                healthy = false;
            }
            if (used == 0) {
                ++empty;
                seen_empty = true;
            } else if (seen_empty) {
                healthy = false;
            }
        }
        if (empty != state.empty_count) {
            healthy = false;
        }
        if (state.span_count < empty) {
            healthy = false;
        }
    }

    {
        scoped_spinlock guard(region_lock_);
        for (const region_header* region = all_regions_; region != nullptr; region = region->global_next) {
            if (region->magic != g_region_magic || region->owner_id != id_) {
                healthy = false;
                continue;
            }
            if (region->payload_offset >= region->region_size) {
                healthy = false;
                continue;
            }
            const auto* payload =
                    reinterpret_cast<const void*>(reinterpret_cast<uintptr_t>(region->base) + region->payload_offset);
            map_view view;
            const bool published = map_read(payload, view);
            const bool cached = (region->flags & g_region_flag_cached) != 0;
            if (cached) {
                if (published) {
                    healthy = false;
                }
            } else if (!published || view.base != region || view.kind != g_kind_large || view.owner_id != id_) {
                healthy = false;
            }
        }
    }
    return healthy;
}

memory_pool& system_memory_pool() noexcept {
    memory_pool* pool = system_pool_slot().load(memory_order_acquire);
    if (pool != nullptr) {
        return *pool;
    }
    lock_spin(system_pool_lock());
    pool = system_pool_slot().load(memory_order_relaxed);
    if (pool == nullptr) {
        NEFORCE_DEBUG_VERIFY(g_system_pool_built == 0, "system memory pool state was reset after construction.");
        pool = new (static_cast<void*>(g_system_pool_storage)) memory_pool();
        g_system_pool_built = 1;
        system_pool_slot().store(pool, memory_order_release);
    }
    unlock_spin(system_pool_lock());
    return *pool;
}

#ifndef NEFORCE_STANDARD_17
constexpr size_t memory_pool::class_count;
constexpr size_t memory_pool::small_max;
constexpr size_t memory_pool::min_align;
constexpr size_t memory_pool::region_cache_slots;
#endif

NEFORCE_END_NAMESPACE__
