#include <NeForce/core/file/file_async.hpp>
#ifdef NEFORCE_PLATFORM_LINUX
#    include <liburing.h>
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
    struct io_op {
        static constexpr uint64_t MAGIC = 0x4E464F504F5F4F50ULL;
        function<void(error_code, file_async::size_type)> handler;
        string* buffer = nullptr;
        shared_ptr<string> write_data;
        uint64_t magic = MAGIC;
    };
#endif
} // namespace


#ifdef NEFORCE_PLATFORM_LINUX
struct file_async::uring {
    ::io_uring ring;
    bool initialized = false;

    ~uring() {
        if (initialized) {
            ::io_uring_queue_exit(&ring);
        }
    }

    bool init(unsigned entries) {
        if (::io_uring_queue_init(entries, &ring, 0) != 0) {
            return false;
        }
        initialized = true;
        return true;
    }

    NEFORCE_NODISCARD int ring_fd() const { return ring.ring_fd; }
};
#endif


file_async::file_async(native_handle_type handle) :
handle_(handle) {}

file_async::~file_async() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (ctx_ != nullptr) {
        ctx_->unregister_file_completion(reinterpret_cast<uintptr_t>(this));
    }
#else
    if (ctx_ != nullptr && uring_) {
        ctx_->remove_fd(uring_->ring_fd());
    }
#endif
}

file_async::file_async(file_async&& other) noexcept :
handle_(other.handle_),
ctx_(other.ctx_)
#ifdef NEFORCE_PLATFORM_LINUX
,
uring_(move(other.uring_))
#endif
{
    other.handle_ = invalid_handle;
    other.ctx_ = nullptr;
}

file_async& file_async::operator=(file_async&& other) noexcept {
    if (this != &other) {
        handle_ = other.handle_;
        ctx_ = other.ctx_;
#ifdef NEFORCE_PLATFORM_LINUX
        uring_ = move(other.uring_);
#endif
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
        ctx_->unregister_file_completion(reinterpret_cast<uintptr_t>(this));
    }
    ctx_ = &ctx;

    const auto key = reinterpret_cast<uintptr_t>(this);
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

    if (!uring_) {
        uring_ = make_unique<uring>();
        if (!uring_->init(256)) {
            uring_.reset();
            return;
        }
    }

    const int uring_fd = uring_->ring_fd();
    ctx_->add_fd(uring_fd, epoll_in, [this](int /*fd*/, uint32_t /*events*/, error_code /*ec*/) {
        if (!uring_) {
            return;
        }
        ::io_uring_cqe* cqe = nullptr;
        while (::io_uring_peek_cqe(&uring_->ring, &cqe) == 0) {
            auto* raw_op = reinterpret_cast<io_op*>(cqe->user_data);
            if (raw_op == nullptr || raw_op->magic != io_op::MAGIC) {
                ::io_uring_cqe_seen(&uring_->ring, cqe);
                continue;
            }
            unique_ptr<io_op> op(raw_op);
            op->magic = 0;

            const auto ec = error_code(cqe->res < 0 ? -cqe->res : 0, error_category::system());
            const auto n = static_cast<size_type>(cqe->res > 0 ? cqe->res : 0);
            if (op->buffer != nullptr) {
                op->buffer->resize(static_cast<size_t>(n));
            }
            op->handler(ec, n);
            ::io_uring_cqe_seen(&uring_->ring, cqe);
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
    auto* op = new io_op{move(handler), &buffer, nullptr};

    auto* sqe = ::io_uring_get_sqe(&uring_->ring);
    if (sqe == nullptr) {
        delete op;
        return;
    }
    ::io_uring_prep_read(sqe, handle_, buffer.data(), static_cast<unsigned>(size),
                         offset >= 0 ? static_cast<uint64_t>(offset) : numeric_traits<uint64_t>::max());
    sqe->user_data = reinterpret_cast<uint64_t>(op);
    ::io_uring_submit(&uring_->ring);
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

    const auto write_data = make_shared<string>(move(data));
    auto* op = new io_op{move(handler), nullptr, write_data};

    auto* sqe = ::io_uring_get_sqe(&uring_->ring);
    if (sqe == nullptr) {
        delete op;
        return;
    }
    ::io_uring_prep_write(sqe, handle_, write_data->data(), static_cast<unsigned>(size),
                          offset >= 0 ? static_cast<uint64_t>(offset) : numeric_traits<uint64_t>::max());
    sqe->user_data = reinterpret_cast<uint64_t>(op);
    ::io_uring_submit(&uring_->ring);
#endif
}

NEFORCE_END_NAMESPACE__
