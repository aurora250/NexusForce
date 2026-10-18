#include <NeForce/core/file/file_async.hpp>
#include <NeForce/core/container/flat_unordered_map.hpp>
#ifdef NEFORCE_PLATFORM_LINUX
#    include <NeForce/core/async/thread.hpp>
#    ifdef NEFORCE_USING_IO_URING
#        include <liburing.h>
#    else
#        include <unistd.h>
#    endif
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
    const file_async::native_handle_type invalid_handle =
#ifdef NEFORCE_PLATFORM_WINDOWS
            INVALID_HANDLE_VALUE;
#else
            -1;
#endif

    constexpr size_t g_ring_entries = 512; ///< io_uring 环容量

#ifdef NEFORCE_PLATFORM_WINDOWS
    struct win_io_op {
        ::OVERLAPPED ov{};
        string write_data;                                         ///< 写载荷
        string* read_buffer = nullptr;                             ///< 调用方读缓冲
        bool abort_requested = false;                              ///< 已请求取消
        function<void(error_code, file_async::size_type)> handler; ///< 完成回调
    };
#endif

#ifdef NEFORCE_PLATFORM_LINUX
    struct sync_io_op {
        io_context* ctx = nullptr;
        file_async::native_handle_type fd = -1;
        off_t offset = 0;
        size_t size = 0;
        string* buffer = nullptr;
        shared_ptr<string> write_data;
        function<void(error_code, file_async::size_type)> handler;
    };

    void run_sync_io_op(const shared_ptr<sync_io_op>& op) {
        // Count the operation as outstanding work from the moment it is issued until its
        // completion handler runs, so run()/run_one() wait for the worker instead of
        // returning as if the context were idle.
        op->ctx->work_started();
        try {
            thread worker([op]() mutable {
                if (op->buffer != nullptr) {
                    const auto n = ::pread64(op->fd, op->buffer->data(), op->size, op->offset);
                    const error_code ec = (n < 0) ? last_error() : error_code{};
                    op->ctx->post([op, n, ec]() {
                        if (ec) {
                            op->buffer->resize(0);
                            op->handler(ec, 0);
                        } else {
                            op->buffer->resize(static_cast<size_t>(n));
                            op->handler(error_code{}, static_cast<file_async::size_type>(n));
                        }
                        op->ctx->work_finished();
                    });
                } else {
                    const auto n = ::pwrite64(op->fd, op->write_data->data(), op->size, op->offset);
                    const error_code ec = (n < 0) ? last_error() : error_code{};
                    op->ctx->post([op, n, ec]() {
                        op->handler(ec, n < 0 ? 0 : static_cast<file_async::size_type>(n));
                        op->ctx->work_finished();
                    });
                }
            });
            worker.detach();
        } catch (...) {
            op->ctx->post([op]() {
                if (op->buffer != nullptr) {
                    op->buffer->resize(0);
                }
                op->handler(
                        error_code(static_cast<int>(errc::resource_unavailable_try_again), error_category::system()),
                        0);
                op->ctx->work_finished();
            });
        }
    }
#endif

#ifdef NEFORCE_USING_IO_URING
    struct io_uring_op {
        static constexpr uint64_t MAGIC = 0x4E464F504F5F4F50ULL;
        function<void(error_code, file_async::size_type)> handler;
        string* buffer = nullptr;
        shared_ptr<string> write_data;
        bool cancelled = false;
        uint64_t magic = MAGIC;
    };

    ::io_uring_sqe* get_sqe_or_submit(::io_uring& ring) {
        for (int attempt = 0; attempt < 2; ++attempt) {
            if (::io_uring_sqe* const sqe = ::io_uring_get_sqe(&ring)) {
                return sqe;
            }
            if (::io_uring_submit(&ring) < 0) {
                return nullptr;
            }
        }
        return nullptr;
    }
#endif

#ifdef __NEFORCE_ASYNC_STATE_DEFINED
#    undef __NEFORCE_ASYNC_STATE_DEFINED
#endif
#define __NEFORCE_ASYNC_STATE_DEFINED
} // namespace


struct file_async::state {
#ifdef NEFORCE_PLATFORM_WINDOWS
    mutex ops_mutex;
    flat_unordered_map<::OVERLAPPED*, shared_ptr<win_io_op>> ops;
    bool overlapped_capable = false;
    io_context* ctx = nullptr;
#elif defined(NEFORCE_USING_IO_URING)
    ::io_uring ring;
    bool initialized = false;
    mutex ops_mutex;
    flat_unordered_map<void*, shared_ptr<io_uring_op>> ops;

    ~state() {
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
#else
#    undef __NEFORCE_ASYNC_STATE_DEFINED
#endif
};

bool file_async::complete_state(state& state, void* ov, const error_code ec, const size_type bytes) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    shared_ptr<win_io_op> op;
    bool abort = false;
    {
        lock<mutex> lk(state.ops_mutex);
        const auto it = state.ops.find(static_cast<::OVERLAPPED*>(ov));
        if (it == state.ops.end()) {
            return false;
        }
        op = it->second;
        state.ops.erase(it);
        abort = op->abort_requested;
    }

    // Balance the work_started() issued when the operation was submitted. Called before
    // the handler so that a handler observing an idle context is already accurate.
    if (state.ctx != nullptr) {
        state.ctx->work_finished();
    }

    if (abort) {
        if (op->read_buffer != nullptr) {
            op->read_buffer->resize(0);
        }
        op->handler(make_operation_aborted(), 0);
    } else {
        if (op->read_buffer != nullptr) {
            op->read_buffer->resize(bytes);
        }
        op->handler(ec, bytes);
    }
#else
    (void) state;
    (void) ov;
    (void) ec;
    (void) bytes;
#endif

    return true;
}


file_async::file_async(const native_handle_type handle) :
handle_(handle) {}

file_async::~file_async() { release_binding(); }

file_async::file_async(file_async&& other) noexcept {
    other.release_binding();

    handle_ = other.handle_;
    other.handle_ = invalid_handle;
    ctx_ = other.ctx_;
    other.ctx_ = nullptr;
#ifdef __NEFORCE_ASYNC_STATE_DEFINED
    state_ = move(other.state_);
#endif
}

file_async& file_async::operator=(file_async&& other) noexcept {
    if (this == addressof(other)) {
        return *this;
    }

    release_binding();
    other.release_binding();

    handle_ = other.handle_;
    other.handle_ = invalid_handle;
    ctx_ = other.ctx_;
    other.ctx_ = nullptr;
#ifdef __NEFORCE_ASYNC_STATE_DEFINED
    state_ = move(other.state_);
#endif
    return *this;
}

void file_async::release_binding() noexcept {
    if (ctx_ == nullptr) {
        return;
    }
#ifdef NEFORCE_PLATFORM_WINDOWS
    ctx_->unregister_file_completion(reinterpret_cast<uintptr_t>(this));
#else
#    ifdef NEFORCE_USING_IO_URING
    if (state_ != nullptr) {
        ctx_->remove_fd(state_->ring_fd());
    }
#    endif
#endif
    ctx_ = nullptr;
}

void file_async::ensure_state(io_context& ctx) {
    if (ctx_ == &ctx) {
        return;
    }
    if (ctx_ != nullptr) {
        release_binding();
    }
    ctx_ = &ctx;

#ifdef NEFORCE_PLATFORM_WINDOWS
    state_ = make_unique<state>();

    const auto key = reinterpret_cast<uintptr_t>(this);
    const ::HANDLE iocp = ctx_->iocp_handle_;
    const ::HANDLE associated = ::CreateIoCompletionPort(handle_, iocp, key, 0);
    state_->overlapped_capable = (associated != nullptr && associated == iocp);
    state_->ctx = ctx_;

    ctx_->register_file_completion(key, [this](const error_code ec, const size_t bytes, void* const overlapped) {
        if (state_ == nullptr || overlapped == nullptr) {
            return;
        }
        complete_state(*state_, static_cast<::OVERLAPPED*>(overlapped), ec, static_cast<file_async::size_type>(bytes));
    });
#else
#    ifdef NEFORCE_USING_IO_URING
    if (state_ == nullptr) {
        auto ring_holder = make_unique<state>();
        if (ring_holder->init(g_ring_entries)) {
            state_ = move(ring_holder);
        }
    }

    if (state_ != nullptr) {
        const int uring_fd = state_->ring_fd();
        ctx_->add_fd(uring_fd, epoll_in, [this](int /*fd*/, uint32_t /*events*/, error_code /*ec*/) {
            if (state_ == nullptr) {
                return;
            }

            struct reaped_op {
                shared_ptr<io_uring_op> op;
                int res;
            };
            vector<reaped_op> done;
            {
                lock<mutex> lk(state_->ops_mutex);
                ::io_uring_cqe* cqe = nullptr;
                while (::io_uring_peek_cqe(&state_->ring, &cqe) == 0) {
                    auto* const raw_op = reinterpret_cast<io_uring_op*>(cqe->user_data);
                    if (raw_op == nullptr || raw_op->magic != io_uring_op::MAGIC) {
                        ::io_uring_cqe_seen(&state_->ring, cqe);
                        continue;
                    }
                    const auto it = state_->ops.find(raw_op);
                    if (it == state_->ops.end()) {
                        ::io_uring_cqe_seen(&state_->ring, cqe);
                        continue;
                    }
                    done.push_back({it->second, cqe->res});
                    state_->ops.erase(it);
                    ::io_uring_cqe_seen(&state_->ring, cqe);
                }
            }

            for (auto& entry: done) {
                const auto& op = entry.op;
                op->magic = 0;
                // Balance the work_started() issued at submission.
                if (ctx_ != nullptr) {
                    ctx_->work_finished();
                }
                if (op->cancelled || entry.res == -ECANCELED) {
                    if (op->buffer != nullptr) {
                        op->buffer->resize(0);
                    }
                    op->handler(make_operation_aborted(), 0);
                } else if (entry.res < 0) {
                    if (op->buffer != nullptr) {
                        op->buffer->resize(0);
                    }
                    op->handler(error_code(-entry.res, error_category::system()), 0);
                } else {
                    const auto n = static_cast<file_async::size_type>(entry.res);
                    if (op->buffer != nullptr) {
                        op->buffer->resize(n);
                    }
                    op->handler(error_code{}, n);
                }
            }
        });
    }
#    endif
#endif
}

void file_async::do_async_read(io_context& ctx, string& buffer, const size_type size, const difference_type offset,
                               cancellation_slot* slot, function<void(error_code, size_type)> handler) {
    ensure_state(ctx);

    if (slot != nullptr && slot->is_cancelled()) {
        buffer.resize(0);
        handler(make_operation_aborted(), 0);
        return;
    }
    if (size == 0) {
        buffer.resize(0);
        handler(error_code{}, 0);
        return;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (state_ == nullptr) {
        buffer.resize(0);
        handler(make_error_code(errc::io_error), 0);
        return;
    }

    difference_type actual_offset = offset;
    if (offset < 0) {
        ::LARGE_INTEGER current{};
        constexpr ::LARGE_INTEGER zero{};
        ::SetFilePointerEx(handle_, zero, &current, FILE_CURRENT);
        actual_offset = current.QuadPart;
    }
    ::LARGE_INTEGER li{};
    li.QuadPart = actual_offset;

    auto op = make_shared<win_io_op>();
    op->handler = move(handler);
    op->read_buffer = &buffer;
    op->ov.Offset = li.LowPart;
    op->ov.OffsetHigh = li.HighPart;

    buffer.resize(size);
    {
        lock<mutex> lk(state_->ops_mutex);
        state_->ops[&op->ov] = op;
    }

    // Register the operation as outstanding work so run()/run_one() wait for it
    // (complete_win_op() balances this call).
    ctx.work_started();

    const ::BOOL ok = ::ReadFile(handle_, buffer.data(), size, nullptr, &op->ov);
    if (ok == FALSE) {
        const ::DWORD err = ::GetLastError();
        if (err == ERROR_IO_PENDING) {
            if (!state_->overlapped_capable) {
                ::DWORD bytes = 0;
                error_code ec;
                if (::GetOverlappedResult(handle_, &op->ov, &bytes, TRUE) == FALSE) {
                    ec = error_code(static_cast<int>(::GetLastError()), error_category::system());
                    bytes = 0;
                }
                complete_state(*state_, &op->ov, ec, static_cast<size_type>(bytes));
            }
        } else {
            complete_state(*state_, &op->ov, error_code(static_cast<int>(err), error_category::system()), 0);
        }
    } else {
        ::DWORD bytes = 0;
        error_code ec;
        if (::GetOverlappedResult(handle_, &op->ov, &bytes, FALSE) == FALSE) {
            ec = error_code(static_cast<int>(::GetLastError()), error_category::system());
            bytes = 0;
        }
        complete_state(*state_, &op->ov, ec, static_cast<size_type>(bytes));
    }

    if (slot != nullptr) {
        slot->assign([this, op]() {
            if (state_ == nullptr) {
                return;
            }
            lock<mutex> lk(state_->ops_mutex);
            const auto it = state_->ops.find(&op->ov);
            if (it == state_->ops.end()) {
                return;
            }
            it->second->abort_requested = true;
            ::CancelIoEx(handle_, &op->ov);
        });
    }
#else
#    ifdef NEFORCE_USING_IO_URING
    if (state_ != nullptr) {
        auto op = make_shared<io_uring_op>();
        op->handler = move(handler);
        op->buffer = &buffer;
        buffer.resize(size);
        const uint64_t resolved_offset =
                offset >= 0 ? static_cast<uint64_t>(offset) : static_cast<uint64_t>(::lseek64(handle_, 0, SEEK_CUR));

        {
            lock<mutex> lk(state_->ops_mutex);
            state_->ops[op.get()] = op;
        }

        ::io_uring_sqe* sqe = nullptr;
        {
            lock<mutex> lk(state_->ops_mutex);
            sqe = get_sqe_or_submit(state_->ring);
            if (sqe != nullptr) {
                ::io_uring_prep_read(sqe, handle_, buffer.data(), static_cast<unsigned>(size), resolved_offset);
                sqe->user_data = reinterpret_cast<uint64_t>(op.get());
                if (::io_uring_submit(&state_->ring) < 0) {
                    sqe = nullptr;
                }
            }
        }
        if (sqe == nullptr) {
            {
                lock<mutex> lk(state_->ops_mutex);
                state_->ops.erase(op.get());
            }
            buffer.resize(0);
            op->handler(error_code(static_cast<int>(errc::resource_unavailable_try_again), error_category::system()),
                        0);
            return;
        }

        // Submitted: count as outstanding work; the reaper balances it.
        ctx.work_started();

        if (slot != nullptr) {
            slot->assign([this, op]() {
                if (state_ == nullptr) {
                    return;
                }
                lock<mutex> lk(state_->ops_mutex);
                const auto it = state_->ops.find(op.get());
                if (it == state_->ops.end()) {
                    return;
                }
                it->second->cancelled = true;
                if (::io_uring_sqe* const cancel_sqe = get_sqe_or_submit(state_->ring)) {
                    ::io_uring_prep_cancel(cancel_sqe, op.get(), 0);
                    cancel_sqe->user_data = 0;
                    ::io_uring_submit(&state_->ring);
                }
            });
        }
        return;
    }
#    endif
    // offload the blocking positioned read onto a worker thread.
    {
        const auto op = make_shared<sync_io_op>();
        op->ctx = ctx_;
        op->fd = handle_;
        op->offset = static_cast<off_t>(offset >= 0 ? offset : ::lseek64(handle_, 0, SEEK_CUR));
        op->size = static_cast<size_t>(size);
        op->buffer = &buffer;
        buffer.resize(size);
        op->handler = move(handler);
        run_sync_io_op(op);
    }
#endif
}

void file_async::do_async_write(io_context& ctx, string data, size_type size, const difference_type offset,
                                cancellation_slot* slot, function<void(error_code, size_type)> handler) {
    ensure_state(ctx);

    if (size == numeric_traits<size_type>::max()) {
        size = static_cast<size_type>(data.size());
    }

    if (slot != nullptr && slot->is_cancelled()) {
        handler(make_operation_aborted(), 0);
        return;
    }
    if (size == 0) {
        handler(error_code{}, 0);
        return;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (state_ == nullptr) {
        handler(make_error_code(errc::io_error), 0);
        return;
    }

    difference_type actual_offset = offset;
    if (offset < 0) {
        ::LARGE_INTEGER current{};
        constexpr ::LARGE_INTEGER zero{};
        ::SetFilePointerEx(handle_, zero, &current, FILE_CURRENT);
        actual_offset = current.QuadPart;
    }
    ::LARGE_INTEGER li{};
    li.QuadPart = actual_offset;

    auto op = make_shared<win_io_op>();
    op->handler = move(handler);
    op->write_data = move(data);
    op->ov.Offset = li.LowPart;
    op->ov.OffsetHigh = li.HighPart;

    {
        lock<mutex> lk(state_->ops_mutex);
        state_->ops[&op->ov] = op;
    }

    // Register the operation as outstanding work so run()/run_one() wait for it
    // (complete_win_op() balances this call).
    ctx.work_started();

    const ::BOOL ok = ::WriteFile(handle_, op->write_data.data(), size, nullptr, &op->ov);
    if (ok == FALSE) {
        const ::DWORD err = ::GetLastError();
        if (err == ERROR_IO_PENDING) {
            if (!state_->overlapped_capable) {
                ::DWORD bytes = 0;
                error_code ec;
                if (::GetOverlappedResult(handle_, &op->ov, &bytes, TRUE) == FALSE) {
                    ec = error_code(static_cast<int>(::GetLastError()), error_category::system());
                    bytes = 0;
                }
                complete_state(*state_, &op->ov, ec, static_cast<size_type>(bytes));
            }
        } else {
            complete_state(*state_, &op->ov, error_code(static_cast<int>(err), error_category::system()), 0);
        }
    } else {
        ::DWORD bytes = 0;
        error_code ec;
        if (::GetOverlappedResult(handle_, &op->ov, &bytes, FALSE) == FALSE) {
            ec = error_code(static_cast<int>(::GetLastError()), error_category::system());
            bytes = 0;
        }
        complete_state(*state_, &op->ov, ec, static_cast<size_type>(bytes));
    }

    if (slot != nullptr) {
        slot->assign([this, op]() {
            if (state_ == nullptr) {
                return;
            }
            lock<mutex> lk(state_->ops_mutex);
            const auto it = state_->ops.find(&op->ov);
            if (it == state_->ops.end()) {
                return;
            }
            it->second->abort_requested = true;
            ::CancelIoEx(handle_, &op->ov);
        });
    }
#else
#    ifdef NEFORCE_USING_IO_URING
    if (state_ != nullptr) {
        const auto write_data = make_shared<string>(move(data));
        auto op = make_shared<io_uring_op>();
        op->handler = move(handler);
        op->write_data = write_data;
        const uint64_t resolved_offset =
                offset >= 0 ? static_cast<uint64_t>(offset) : static_cast<uint64_t>(::lseek64(handle_, 0, SEEK_CUR));

        {
            lock<mutex> lk(state_->ops_mutex);
            state_->ops[op.get()] = op;
        }

        ::io_uring_sqe* sqe = nullptr;
        {
            lock<mutex> lk(state_->ops_mutex);
            sqe = get_sqe_or_submit(state_->ring);
            if (sqe != nullptr) {
                ::io_uring_prep_write(sqe, handle_, write_data->data(), static_cast<unsigned>(size), resolved_offset);
                sqe->user_data = reinterpret_cast<uint64_t>(op.get());
                if (::io_uring_submit(&state_->ring) < 0) {
                    sqe = nullptr;
                }
            }
        }
        if (sqe == nullptr) {
            {
                lock<mutex> lk(state_->ops_mutex);
                state_->ops.erase(op.get());
            }
            op->handler(error_code(static_cast<int>(errc::resource_unavailable_try_again), error_category::system()),
                        0);
            return;
        }

        // Submitted: count as outstanding work; the reaper balances it.
        ctx.work_started();

        if (slot != nullptr) {
            slot->assign([this, op]() {
                if (state_ == nullptr) {
                    return;
                }
                lock<mutex> lk(state_->ops_mutex);
                const auto it = state_->ops.find(op.get());
                if (it == state_->ops.end()) {
                    return;
                }
                it->second->cancelled = true;
                if (::io_uring_sqe* const cancel_sqe = get_sqe_or_submit(state_->ring)) {
                    ::io_uring_prep_cancel(cancel_sqe, op.get(), 0);
                    cancel_sqe->user_data = 0;
                    ::io_uring_submit(&state_->ring);
                }
            });
        }
        return;
    }
#    endif
    {
        const auto op = make_shared<sync_io_op>();
        op->ctx = ctx_;
        op->fd = handle_;
        op->offset = static_cast<off_t>(offset >= 0 ? offset : ::lseek64(handle_, 0, SEEK_CUR));
        op->size = static_cast<size_t>(size);
        op->write_data = make_shared<string>(move(data));
        op->handler = move(handler);
        run_sync_io_op(op);
    }
#endif
}

NEFORCE_END_NAMESPACE__
