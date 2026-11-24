#include <NeForce/core/system/io_queue.hpp>
#ifdef NEFORCE_PLATFORM_LINUX
#    include <NeForce/core/async/atomic.hpp>
#    include <NeForce/core/async/mutex.hpp>
#    include <NeForce/core/container/flat_unordered_map.hpp>
#    include <NeForce/core/exception/errc.hpp>
#    include <sys/mman.h>
#    include <sys/syscall.h>
#    include <unistd.h>
NEFORCE_BEGIN_NAMESPACE__

namespace {
#    ifdef __NR_io_uring_setup
    constexpr long nr_io_uring_setup = __NR_io_uring_setup;
#    else
    constexpr long nr_io_uring_setup = 425;
#    endif
#    ifdef __NR_io_uring_enter
    constexpr long nr_io_uring_enter = __NR_io_uring_enter;
#    else
    constexpr long nr_io_uring_enter = 426;
#    endif

    constexpr native_handle_type invalid_handle = -1;

    struct ring_sqe {
        uint8_t opcode;
        uint8_t flags;
        uint16_t iopriority;
        native_handle_type fd;
        uint64_t offset;
        uint64_t address;
        uint32_t length;
        uint32_t operation_flags;
        uint64_t user_data;
        uint16_t buffer_index;
        uint16_t personality;
        int32_t splice_fd_in;
        uint64_t address3;
        uint64_t reserved;
    };
    static_assert(sizeof(ring_sqe) == 64, "io_uring submission queue entry must be 64 bytes");

    struct ring_cqe {
        uint64_t user_data;
        int32_t result;
        uint32_t flags;
    };
    static_assert(sizeof(ring_cqe) == 16, "io_uring completion queue entry must be 16 bytes");

    struct ring_sq_offsets {
        uint32_t head;
        uint32_t tail;
        uint32_t ring_mask;
        uint32_t ring_entries;
        uint32_t flags;
        uint32_t dropped;
        uint32_t array;
        uint32_t reserved1;
        uint64_t user_address;
    };

    struct ring_cq_offsets {
        uint32_t head;
        uint32_t tail;
        uint32_t ring_mask;
        uint32_t ring_entries;
        uint32_t overflow;
        uint32_t cqe_array;
        uint32_t flags;
        uint32_t reserved1;
        uint64_t user_address;
    };

    struct ring_params {
        uint32_t sq_entries;
        uint32_t cq_entries;
        uint32_t flags;
        uint32_t sq_thread_cpu;
        uint32_t sq_thread_idle;
        uint32_t features;
        uint32_t wq_fd;
        uint32_t reserved[3];
        ring_sq_offsets sq_off;
        ring_cq_offsets cq_off;
    };
    static_assert(sizeof(ring_params) == 120, "io_uring_setup params must be 120 bytes");

    constexpr uint32_t off_sq_ring = 0;
    constexpr uint32_t off_cq_ring = 0x8000000;
    constexpr uint32_t off_sqes = 0x10000000;

    constexpr uint32_t feature_single_mmap = 1U << 0;

    constexpr uint8_t op_async_cancel = 14;
    constexpr uint8_t op_read = 22;
    constexpr uint8_t op_write = 23;

    /// Store with release semantics into a kernel-shared ring slot.
    void store_release(uint32_t* slot, const uint32_t value) noexcept {
        __atomic_store_n(slot, value, __ATOMIC_RELEASE);
    }

    /// Load with acquire semantics from a kernel-shared ring slot.
    uint32_t load_acquire(const uint32_t* slot) noexcept { return __atomic_load_n(slot, __ATOMIC_ACQUIRE); }

    uint32_t round_up_power_of_two(uint32_t value) noexcept {
        if (value <= 1) {
            return 1;
        }
        --value;
        value |= value >> 1;
        value |= value >> 2;
        value |= value >> 4;
        value |= value >> 8;
        value |= value >> 16;
        return value + 1;
    }
} // namespace


struct io_queue::impl {
    struct op_state {
        completion_handler handler;
        atomic<bool> cancelled{false};
    };

    struct completion_record {
        uint64_t id;
        int32_t result;
    };

    native_handle_type ring_fd{invalid_handle};
    void* sq_ring{nullptr};
    void* cq_ring{nullptr};
    size_t sq_mapping_size{0};
    size_t cq_mapping_size{0};
    void* sqes_mapping{nullptr};
    size_t sqes_mapping_size{0};

    uint32_t* sq_head{nullptr};
    uint32_t* sq_tail{nullptr};
    uint32_t* sq_array{nullptr};
    uint32_t* cq_head{nullptr};
    uint32_t* cq_tail{nullptr};
    ring_cqe* cqes{nullptr};
    uint32_t sq_mask{0};
    uint32_t cq_mask{0};
    uint32_t entries{0};

    uint32_t cached_tail{0};
    uint32_t pending_submit{0};

    mutex submit_mutex;
    mutex ops_mutex;
    mutex drain_mutex;
    flat_unordered_map<uint64_t, unique_ptr<op_state>> ops;
    atomic<size_t> pending{0};

    ~impl() { close(); }

    NEFORCE_NODISCARD bool is_open() const noexcept { return ring_fd >= 0; }

    bool open(const size_t depth) noexcept {
        ring_params params{};
        auto requested = static_cast<uint32_t>(depth);
        if (requested == 0) {
            requested = 1;
        }
        requested = round_up_power_of_two(requested);

        const long fd = ::syscall(nr_io_uring_setup, requested, &params);
        if (fd < 0) {
            return false;
        }
        ring_fd = static_cast<int>(fd);
        entries = params.sq_entries;
        if (entries == 0) {
            close();
            return false;
        }

        const size_t sq_ring_size = params.sq_off.array + params.sq_entries * sizeof(uint32_t);
        const size_t cq_ring_size = params.cq_off.cqe_array + params.cq_entries * sizeof(ring_cqe);

        if ((params.features & feature_single_mmap) != 0) {
            const size_t size = sq_ring_size > cq_ring_size ? sq_ring_size : cq_ring_size;
            void* const mapping =
                    ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, ring_fd, off_sq_ring);
            if (mapping == MAP_FAILED) {
                close();
                return false;
            }
            sq_ring = mapping;
            cq_ring = mapping;
            sq_mapping_size = size;
            cq_mapping_size = 0;
        } else {
            void* const sq_mapping = ::mmap(nullptr, sq_ring_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE,
                                            ring_fd, off_sq_ring);
            if (sq_mapping == MAP_FAILED) {
                close();
                return false;
            }
            sq_ring = sq_mapping;
            sq_mapping_size = sq_ring_size;

            void* const cq_mapping = ::mmap(nullptr, cq_ring_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE,
                                            ring_fd, off_cq_ring);
            if (cq_mapping == MAP_FAILED) {
                close();
                return false;
            }
            cq_ring = cq_mapping;
            cq_mapping_size = cq_ring_size;
        }

        const size_t sqes_size = params.sq_entries * sizeof(ring_sqe);
        void* const sqes =
                ::mmap(nullptr, sqes_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, ring_fd, off_sqes);
        if (sqes == MAP_FAILED) {
            close();
            return false;
        }
        sqes_mapping = sqes;
        sqes_mapping_size = sqes_size;

        auto* const sq_base = static_cast<char*>(sq_ring);
        sq_head = reinterpret_cast<uint32_t*>(sq_base + params.sq_off.head);
        sq_tail = reinterpret_cast<uint32_t*>(sq_base + params.sq_off.tail);
        sq_array = reinterpret_cast<uint32_t*>(sq_base + params.sq_off.array);
        sq_mask = *reinterpret_cast<uint32_t*>(sq_base + params.sq_off.ring_mask);

        auto* const cq_base = static_cast<char*>(cq_ring);
        cq_head = reinterpret_cast<uint32_t*>(cq_base + params.cq_off.head);
        cq_tail = reinterpret_cast<uint32_t*>(cq_base + params.cq_off.tail);
        cq_mask = *reinterpret_cast<uint32_t*>(cq_base + params.cq_off.ring_mask);
        cqes = reinterpret_cast<ring_cqe*>(cq_base + params.cq_off.cqe_array);

        cached_tail = load_acquire(sq_tail);
        pending_submit = 0;
        return true;
    }

    void close() noexcept {
        if (sqes_mapping != nullptr) {
            ::munmap(sqes_mapping, sqes_mapping_size);
            sqes_mapping = nullptr;
            sqes_mapping_size = 0;
        }
        if (sq_ring != nullptr) {
            ::munmap(sq_ring, sq_mapping_size);
            if (cq_ring == sq_ring) {
                cq_ring = nullptr;
                cq_mapping_size = 0;
            }
            sq_ring = nullptr;
            sq_mapping_size = 0;
        }
        if (cq_ring != nullptr) {
            ::munmap(cq_ring, cq_mapping_size);
            cq_ring = nullptr;
            cq_mapping_size = 0;
        }
        if (ring_fd != invalid_handle) {
            ::close(ring_fd);
            ring_fd = invalid_handle;
        }

        sq_head = nullptr;
        sq_tail = nullptr;
        sq_array = nullptr;
        cq_head = nullptr;
        cq_tail = nullptr;
        cqes = nullptr;

        lock<mutex> guard{ops_mutex};
        ops.clear();
        pending.store(0, memory_order_relaxed);
    }

    NEFORCE_NODISCARD bool submit_locked(const int to_submit) noexcept {
        if (to_submit <= 0) {
            return true;
        }
        int remaining = to_submit;
        while (remaining > 0) {
            const long rc =
                    ::syscall(nr_io_uring_enter, ring_fd, static_cast<unsigned>(remaining), 0U, 0U, nullptr, 0U);
            if (rc == -EINTR) {
                continue;
            }
            if (rc <= 0) {
                return false;
            }
            const auto consumed = static_cast<uint32_t>(rc);
            pending_submit -= consumed < pending_submit ? consumed : pending_submit;
            remaining -= static_cast<int>(consumed);
        }
        return true;
    }

    NEFORCE_NODISCARD ring_sqe* reserve_locked() noexcept {
        for (int attempt = 0; attempt < 2; ++attempt) {
            if (cached_tail - load_acquire(sq_head) < entries) {
                return reinterpret_cast<ring_sqe*>(static_cast<char*>(sqes_mapping)) + (cached_tail & sq_mask);
            }
            if (!submit_locked(static_cast<int>(pending_submit))) {
                return nullptr;
            }
            if (pending_submit > 0) {
                return nullptr;
            }
        }
        return nullptr;
    }

    void publish_locked() noexcept {
        const uint32_t index = cached_tail & sq_mask;
        sq_array[index] = index;
        ++cached_tail;
        ++pending_submit;
        store_release(sq_tail, cached_tail);
    }

    token submit(const uint8_t opcode, const native_handle_type fd, const void* const buffer, const size_t size,
                 const uint64_t offset, completion_handler handler) {
        if (!is_open() || buffer == nullptr || size == 0) {
            return 0;
        }

        auto state = make_unique<op_state>();
        state->handler = move(handler);
        const auto id = reinterpret_cast<token>(state.get());

        {
            lock<mutex> guard{ops_mutex};
            ops[id] = move(state);
        }
        pending.fetch_add(1, memory_order_relaxed);

        lock<mutex> guard{submit_mutex};

        ring_sqe* const sqe = reserve_locked();
        if (sqe == nullptr) {
            {
                lock<mutex> ops_guard{ops_mutex};
                ops.erase(id);
            }
            pending.fetch_sub(1, memory_order_relaxed);
            return 0;
        }

        *sqe = ring_sqe{};
        sqe->opcode = opcode;
        sqe->fd = fd;
        sqe->address = reinterpret_cast<uint64_t>(const_cast<void*>(buffer));
        sqe->length = static_cast<uint32_t>(size);
        sqe->offset = offset;
        sqe->user_data = id;
        publish_locked();

        if (!submit_locked(static_cast<int>(pending_submit))) {
            lock<mutex> ops_guard{ops_mutex};
            ops.erase(id);
            pending.fetch_sub(1, memory_order_relaxed);
            return 0;
        }
        return id;
    }

    bool cancel(const token id) noexcept {
        if (!is_open()) {
            return false;
        }
        {
            lock<mutex> guard{ops_mutex};
            const auto it = ops.find(id);
            if (it == ops.end()) {
                return false;
            }
            it->second->cancelled.store(true, memory_order_release);
        }

        lock<mutex> guard{submit_mutex};
        ring_sqe* const sqe = reserve_locked();
        if (sqe == nullptr) {
            return false;
        }
        *sqe = ring_sqe{};
        sqe->opcode = op_async_cancel;
        sqe->fd = invalid_handle;
        sqe->address = id;
        sqe->user_data = 0;
        publish_locked();
        return submit_locked(static_cast<int>(pending_submit));
    }

    size_t drain() {
        if (!is_open()) {
            return 0;
        }

        const uint32_t head = load_acquire(cq_head);
        const uint32_t tail = load_acquire(cq_tail);
        if (head == tail) {
            return 0;
        }

        vector<completion_record> done;
        {
            lock<mutex> guard{drain_mutex};
            const uint32_t current_head = load_acquire(cq_head);
            const uint32_t current_tail = load_acquire(cq_tail);
            if (current_head == current_tail) {
                return 0;
            }
            done.reserve(static_cast<size_t>(current_tail - current_head));
            for (uint32_t index = current_head; index != current_tail; ++index) {
                const ring_cqe* const cqe = &cqes[index & cq_mask];
                done.push_back(completion_record{cqe->user_data, cqe->result});
            }
            store_release(cq_head, current_tail);
        }

        size_t completed = 0;
        for (const auto& record: done) {
            unique_ptr<op_state> state;
            {
                lock<mutex> guard{ops_mutex};
                const auto it = ops.find(record.id);
                if (it == ops.end()) {
                    continue;
                }
                state = move(it->second);
                ops.erase(it);
            }
            pending.fetch_sub(1, memory_order_relaxed);
            ++completed;

            if (state->cancelled.load(memory_order_acquire) || record.result == -ECANCELED) {
                state->handler(error_code(static_cast<int>(errc::operation_canceled), error_category::generic()), 0);
            } else if (record.result < 0) {
                state->handler(error_code(-record.result, error_category::system()), 0);
            } else {
                state->handler(error_code{}, static_cast<size_t>(record.result));
            }
        }
        return completed;
    }
};

io_queue::io_queue(const size_t entries) :
impl_(make_unique<impl>()) {
    if (!impl_->open(entries)) {
        impl_->close();
    }
}

io_queue::~io_queue() = default;

bool io_queue::available() noexcept {
    static const bool supported = [] {
        ring_params params{};
        const long fd = ::syscall(nr_io_uring_setup, 8U, &params);
        if (fd == invalid_handle) {
            return false;
        }
        ::close(static_cast<native_handle_type>(fd));
        return true;
    }();
    return supported;
}

bool io_queue::is_open() const noexcept { return impl_ != nullptr && impl_->is_open(); }

native_handle_type io_queue::native_handle() const noexcept {
    return impl_ != nullptr ? impl_->ring_fd : invalid_handle;
}

size_t io_queue::pending() const noexcept { return impl_ != nullptr ? impl_->pending.load(memory_order_relaxed) : 0; }

io_queue::token io_queue::submit_read(const native_handle_type fd, void* const buffer, const size_t size,
                                      const uint64_t offset, completion_handler handler) {
    return impl_ != nullptr ? impl_->submit(op_read, fd, buffer, size, offset, move(handler)) : 0;
}

io_queue::token io_queue::submit_write(const native_handle_type fd, const void* const buffer, const size_t size,
                                       const uint64_t offset, completion_handler handler) {
    return impl_ != nullptr ? impl_->submit(op_write, fd, buffer, size, offset, move(handler)) : 0;
}

bool io_queue::cancel(const token id) noexcept { return impl_ != nullptr && impl_->cancel(id); }

size_t io_queue::drain() { return impl_ != nullptr ? impl_->drain() : 0; }

void io_queue::close() noexcept {
    if (impl_ != nullptr) {
        impl_->close();
    }
}

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_PLATFORM_LINUX
