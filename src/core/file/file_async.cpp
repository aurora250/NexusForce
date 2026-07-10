#include <NeForce/core/file/file_async.hpp>
#ifdef NEFORCE_PLATFORM_LINUX
#    include <cerrno>
#    include <fcntl.h>
#    include <linux/io_uring.h>
#    include <sys/mman.h>
#    include <sys/syscall.h>
#    include <unistd.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
    const file_async::native_handle_type invalid_handle =
#ifdef NEFORCE_PLATFORM_WINDOWS
            INVALID_HANDLE_VALUE;
#else
            -1;
#endif

#ifdef NEFORCE_PLATFORM_WINDOWS
    struct op_state {
        ::OVERLAPPED ov{};
        string write_data;
    };

#else
    struct uring {
        int ring_fd = -1;
        ::io_uring_sqe* sqes = nullptr;
        ::io_uring_cqe* cqes = nullptr;
        unsigned* sq_head = nullptr;
        unsigned* sq_tail = nullptr;
        unsigned* cq_head = nullptr;
        unsigned* cq_tail = nullptr;
        unsigned* sq_mask = nullptr;
        unsigned* cq_mask = nullptr;
        unsigned* sq_entries = nullptr;
        unsigned* cq_entries = nullptr;
        unsigned* flags = nullptr;
        unsigned* sq_dropped = nullptr;
        char* sq_ring = nullptr;
        char* cq_ring = nullptr;
        char* sqes_ptr = nullptr;
        unsigned sq_ring_sz = 0;
        unsigned cq_ring_sz = 0;

        ~uring() {
            if (sqes_ptr) {
                ::munmap(sqes_ptr, sq_ring_sz);
            }
            if (cqes) {
                ::munmap(cqes, cq_ring_sz);
            }
            if (ring_fd >= 0) {
                ::close(ring_fd);
            }
        }

        bool init(unsigned entries) {
            ::io_uring_params p{};
            ring_fd = static_cast<int>(::syscall(__NR_io_uring_setup, entries, &p));
            if (ring_fd < 0) {
                return false;
            }

            sq_ring_sz = p.sq_off.array + p.sq_entries * sizeof(unsigned);
            cq_ring_sz = p.cq_off.cqes + p.cq_entries * sizeof(::io_uring_cqe);

            sq_ring = static_cast<char*>(::mmap(nullptr, sq_ring_sz, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE,
                                                ring_fd, IORING_OFF_SQ_RING));
            cq_ring = static_cast<char*>(::mmap(nullptr, cq_ring_sz, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE,
                                                ring_fd, IORING_OFF_CQ_RING));
            sqes_ptr = static_cast<char*>(::mmap(nullptr, p.sq_entries * sizeof(::io_uring_sqe), PROT_READ | PROT_WRITE,
                                                 MAP_SHARED | MAP_POPULATE, ring_fd, IORING_OFF_SQES));

            if (sq_ring == MAP_FAILED || cq_ring == MAP_FAILED || sqes_ptr == MAP_FAILED) {
                return false;
            }

            sq_head = reinterpret_cast<unsigned*>(sq_ring + p.sq_off.head);
            sq_tail = reinterpret_cast<unsigned*>(sq_ring + p.sq_off.tail);
            sq_mask = reinterpret_cast<unsigned*>(sq_ring + p.sq_off.ring_mask);
            sq_entries = reinterpret_cast<unsigned*>(sq_ring + p.sq_off.ring_entries);
            flags = reinterpret_cast<unsigned*>(sq_ring + p.sq_off.flags);
            sq_dropped = reinterpret_cast<unsigned*>(sq_ring + p.sq_off.dropped);
            sqes = reinterpret_cast<::io_uring_sqe*>(sqes_ptr);

            cq_head = reinterpret_cast<unsigned*>(cq_ring + p.cq_off.head);
            cq_tail = reinterpret_cast<unsigned*>(cq_ring + p.cq_off.tail);
            cq_mask = reinterpret_cast<unsigned*>(cq_ring + p.cq_off.ring_mask);
            cq_entries = reinterpret_cast<unsigned*>(cq_ring + p.cq_off.ring_entries);
            cqes = reinterpret_cast<::io_uring_cqe*>(cq_ring + p.cq_off.cqes);

            return true;
        }

        ::io_uring_sqe* get_sqe() {
            unsigned tail = *sq_tail;
            unsigned head = *sq_head;
            if (tail - head >= *sq_entries) {
                return nullptr;
            }
            return &sqes[tail & (*sq_mask)];
        }

        void submit() {
            unsigned tail = *sq_tail;
            __atomic_store_n(sq_tail, tail + 1, __ATOMIC_RELEASE);
            ::syscall(__NR_io_uring_enter, ring_fd, 1, 0, IORING_ENTER_GETEVENTS, nullptr);
        }

        int peek_cqe(::io_uring_cqe* out) {
            unsigned head = *cq_head;
            if (head == *cq_tail) {
                return -1;
            }
            *out = cqes[head & (*cq_mask)];
            __atomic_store_n(cq_head, head + 1, __ATOMIC_RELEASE);
            return 0;
        }
    };

    unique_ptr<uring> g_uring;
#endif
} // namespace


file_async::file_async(native_handle_type handle) :
handle_(handle) {}

file_async::~file_async() {
    if (ctx_ != nullptr) {
        ctx_->unregister_file_completion(reinterpret_cast<::ULONG_PTR>(this));
    }
}

file_async::file_async(file_async&& other) noexcept :
handle_(other.handle_),
ctx_(other.ctx_) {
    other.handle_ = invalid_handle;
    other.ctx_ = nullptr;
}

file_async& file_async::operator=(file_async&& other) noexcept {
    if (this != &other) {
        handle_ = other.handle_;
        ctx_ = other.ctx_;
        other.handle_ = invalid_handle;
        other.ctx_ = nullptr;
    }
    return *this;
}

void file_async::ensure_iocp(io_context& ctx) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (ctx_ == &ctx) {
        return;
    }
    if (ctx_ != nullptr) {
        ctx_->unregister_file_completion(reinterpret_cast<::ULONG_PTR>(this));
    }
    ctx_ = &ctx;

    const auto key = reinterpret_cast<::ULONG_PTR>(this);
    const ::HANDLE iocp = ctx_->iocp_handle_;
    ::CreateIoCompletionPort(handle_, iocp, key, 0);

    ctx_->register_file_completion(key, [this](error_code ec, size_t bytes, void* /*overlapped*/) {
        if (pending_handler_) {
            const auto h = move(pending_handler_);
            pending_handler_ = nullptr;
            h(ec, static_cast<size_type>(bytes));
        }
    });
#else
    if (ctx_ == &ctx) {
        return;
    }
    ctx_ = &ctx;

    if (!g_uring) {
        g_uring = make_unique<uring>();
        g_uring->init(256);
    }

    int uring_fd = g_uring->ring_fd;
    ctx_->add_fd(uring_fd, epoll_in, [](int /*fd*/, uint32_t /*events*/, error_code /*ec*/) {
        if (!g_uring) {
            return;
        }
        ::io_uring_cqe cqe;
        while (g_uring->peek_cqe(&cqe) == 0) {
            auto* handler_ptr = reinterpret_cast<function<void(error_code, size_type)>*>(cqe.user_data);
            auto h = move(*handler_ptr);
            delete handler_ptr;
            h(error_code(cqe.res < 0 ? -cqe.res : 0, error_category::system()),
              static_cast<size_type>(cqe.res > 0 ? cqe.res : 0));
        }
    });
#endif
}

void file_async::do_async_read(io_context& ctx, string& buffer, size_type size, difference_type offset,
                               cancellation_slot* /*slot*/, function<void(error_code, size_type)> handler) {
    ensure_iocp(ctx);
    buffer.resize(size);

#ifdef NEFORCE_PLATFORM_WINDOWS
    const auto op = make_shared<op_state>();

    {
        difference_type actual_offset = offset;
        if (offset < 0) {
            ::LARGE_INTEGER current{};
            ::LARGE_INTEGER zero{};
            ::SetFilePointerEx(handle_, zero, &current, FILE_CURRENT);
            actual_offset = current.QuadPart;
        }
        ::LARGE_INTEGER li;
        li.QuadPart = actual_offset;
        op->ov.Offset = li.LowPart;
        op->ov.OffsetHigh = li.HighPart;
    }

    pending_handler_ = [handler = move(handler), &buffer](error_code ec, size_type bytes) mutable {
        buffer.resize(bytes);
        handler(ec, bytes);
    };

    const ::BOOL ok = ::ReadFile(handle_, buffer.data(), size, nullptr, &op->ov);
    if ((ok == FALSE) && ::GetLastError() == ERROR_IO_PENDING) {
        return;
    }

    const auto h = move(pending_handler_);
    pending_handler_ = nullptr;
    if (ok == FALSE) {
        h(error_code(static_cast<int>(::GetLastError()), error_category::system()), 0);
    } else {
        ::DWORD bytes = 0;
        ::GetOverlappedResult(handle_, &op->ov, &bytes, FALSE);
        h(error_code{}, static_cast<size_type>(bytes));
    }
#else
    auto* cb = new function<void(error_code, size_type)>(
            [handler = move(handler), &buffer](error_code ec, size_type n) mutable {
                buffer.resize(static_cast<size_t>(n));
                handler(ec, n);
            });

    auto* sqe = g_uring->get_sqe();
    sqe->opcode = IORING_OP_READ;
    sqe->fd = handle_;
    sqe->off = offset >= 0 ? static_cast<unsigned long long>(offset) : static_cast<unsigned long long>(-1);
    sqe->addr = reinterpret_cast<unsigned long long>(buffer.data());
    sqe->len = size;
    sqe->user_data = reinterpret_cast<unsigned long long>(cb);
    sqe->flags = 0;

    g_uring->submit();
#endif
}

void file_async::do_async_write(io_context& ctx, string data, size_type size, difference_type offset,
                                cancellation_slot* /*slot*/, function<void(error_code, size_type)> handler) {
    ensure_iocp(ctx);

#ifdef NEFORCE_PLATFORM_WINDOWS
    auto op = make_shared<op_state>();
    op->write_data = move(data);

    if (size == numeric_traits<size_type>::max()) {
        size = static_cast<size_type>(op->write_data.size());
    }

    {
        difference_type actual_offset = offset;
        if (offset < 0) {
            ::LARGE_INTEGER current{};
            constexpr ::LARGE_INTEGER zero{};
            ::SetFilePointerEx(handle_, zero, &current, FILE_CURRENT);
            actual_offset = current.QuadPart;
        }
        ::LARGE_INTEGER li;
        li.QuadPart = actual_offset;
        op->ov.Offset = li.LowPart;
        op->ov.OffsetHigh = li.HighPart;
    }

    pending_handler_ = [this, handler = move(handler), op](error_code ec, size_type /*bytes*/) mutable {
        ::DWORD written = 0;
        ::GetOverlappedResult(handle_, &op->ov, &written, FALSE);
        handler(ec, written);
    };

    const ::BOOL ok = ::WriteFile(handle_, op->write_data.data(), size, nullptr, &op->ov);
    if ((ok == FALSE) && ::GetLastError() == ERROR_IO_PENDING) {
        return;
    }
    const auto h = move(pending_handler_);
    pending_handler_ = nullptr;
    if (ok == FALSE) {
        h(error_code(static_cast<int>(::GetLastError()), error_category::system()), 0);
    } else {
        ::DWORD written = 0;
        ::GetOverlappedResult(handle_, &op->ov, &written, FALSE);
        h(error_code{}, written);
    }
#else
    if (size == numeric_traits<size_type>::max()) {
        size = data.size();
    }

    auto write_data = make_shared<string>(move(data));
    auto* cb = new function<void(error_code, size_type)>(
            [handler = move(handler), write_data](error_code ec, size_type n) mutable { handler(ec, n); });

    auto* sqe = g_uring->get_sqe();
    sqe->opcode = IORING_OP_WRITE;
    sqe->fd = handle_;
    sqe->off = offset >= 0 ? static_cast<unsigned long long>(offset) : static_cast<unsigned long long>(-1);
    sqe->addr = reinterpret_cast<unsigned long long>(write_data->data());
    sqe->len = size;
    sqe->user_data = reinterpret_cast<unsigned long long>(cb);
    sqe->flags = 0;

    g_uring->submit();
#endif
}

NEFORCE_END_NAMESPACE__
