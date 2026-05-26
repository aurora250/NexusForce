#include <NeForce/core/file/file_async.hpp>
#ifdef NEFORCE_PLATFORM_LINUX
#    include <cerrno>
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
} // namespace


file_async::async_context::async_context(string d) :
data(move(d)),
cb(new aiocb_type{}),
is_write(true) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    cb->hEvent = ::CreateEventA(nullptr, TRUE, FALSE, nullptr);
#endif
}

file_async::async_context::async_context(string* buf) :
buffer(buf),
cb(new aiocb_type{}) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    cb->hEvent = ::CreateEventA(nullptr, TRUE, FALSE, nullptr);
#endif
}

file_async::async_context::~async_context() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (cb != nullptr) {
        if (cb->hEvent != nullptr) {
            ::CloseHandle(cb->hEvent);
        }
        delete cb;
        cb = nullptr;
    }
#else
    delete cb;
    cb = nullptr;
#endif
}

bool file_async::complete_result(async_result& result, const size_type bytes) noexcept {
    result.completed = true;
    result.bytes_transferred = bytes;
    result.error.clear();

    lock<mutex> lk(mutex_);
    const auto it = find(operations_.begin(), operations_.end(), result.cb);
    if (it != operations_.end()) {
        operations_.erase(it);
    }

    const auto ci = contexts_.find(result.cb);
    if (ci != contexts_.end()) {
        delete ci->second;
        contexts_.erase(ci);
    }

    result.cb = nullptr;
    result.user_context = nullptr;
    return true;
}

bool file_async::check_completion(async_result& result) noexcept {
#ifdef NEFORCE_PLATFORM_LINUX
    const ssize_t ret = ::aio_return(result.cb);
    if (ret >= 0) {
        return complete_result(result, static_cast<size_type>(ret));
    }
    result.error = static_cast<errc>(::aio_error(result.cb));
    return false;
#else
    return false;
#endif
}

file_async::file_async(native_handle_type handle) :
handle_(handle) {}

file_async::~file_async() {
    lock<mutex> lk(mutex_);
    for (auto* cb: operations_) {
        if (cb == nullptr) {
            continue;
        }
#ifdef NEFORCE_PLATFORM_WINDOWS
        ::CancelIoEx(handle_, cb);
#else
        ::aio_cancel(handle_, cb);
#endif
    }
    for (auto& context: contexts_) {
        delete context.second;
    }
    contexts_.clear();
    operations_.clear();
}

file_async::file_async(file_async&& other) noexcept :
handle_(other.handle_),
operations_(move(other.operations_)),
contexts_(move(other.contexts_)) {
    other.handle_ = invalid_handle;
}

file_async& file_async::operator=(file_async&& other) noexcept {
    if (addressof(other) == this) {
        return *this;
    }
    handle_ = other.handle_;
    operations_ = move(other.operations_);
    contexts_ = move(other.contexts_);
    other.handle_ = invalid_handle;
    return *this;
}

file_async::async_result file_async::read(string& buffer, const size_type size, const difference_type offset) const {
    async_result result;

    if (size == 0) {
        buffer.clear();
        result.completed = true;
        return result;
    }

    difference_type resolved_offset = offset;
    if (offset < 0) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        ::LARGE_INTEGER current{};
        if (::SetFilePointerEx(handle_, {}, &current, FILE_CURRENT) == FALSE) {
            result.error = last_error();
            return result;
        }
        resolved_offset = current.QuadPart;
#else
        const difference_type pos = ::lseek(handle_, 0, SEEK_CUR);
        if (pos == static_cast<difference_type>(-1)) {
            result.error = last_error();
            return result;
        }
        resolved_offset = pos;
#endif
    }

    if (buffer.capacity() < size) {
        try {
            buffer.reserve(size);
        } catch (...) {
            result.error = errc::not_enough_memory;
            return result;
        }
    }
    if (buffer.size() < size) {
        buffer.resize(size);
    }

    auto* ctx = new async_context(&buffer);

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (ctx->cb->hEvent == nullptr) {
        delete ctx;
        result.error = last_error();
        return result;
    }

    {
        const auto off64 = static_cast<uint64_t>(resolved_offset);
        ctx->cb->Offset = static_cast<::DWORD>(off64 & 0xFFFFFFFF);
        ctx->cb->OffsetHigh = static_cast<::DWORD>(off64 >> 32);
    }

    size_type bytes_read = 0;
    if (::ReadFile(handle_, buffer.data(), size, &bytes_read, ctx->cb) == TRUE) {
        result.completed = true;
        result.bytes_transferred = bytes_read;
        delete ctx;
    } else {
        const ::DWORD err = ::GetLastError();
        if (err == ERROR_IO_PENDING) {
            result.completed = false;
            result.cb = ctx->cb;
            result.user_context = ctx;

            lock<mutex> lk(mutex_);
            operations_.push_back(ctx->cb);
            contexts_[ctx->cb] = ctx;
        } else {
            result.error = static_cast<errc>(err);
            delete ctx;
        }
    }

#else
    ctx->cb->aio_fildes = handle_;
    ctx->cb->aio_buf = const_cast<volatile void*>(static_cast<const volatile void*>(buffer.data()));
    ctx->cb->aio_nbytes = size;
    ctx->cb->aio_offset = resolved_offset;
    ctx->cb->aio_sigevent.sigev_notify = SIGEV_NONE;

    if (::aio_read(ctx->cb) == 0) {
        result.completed = false;
        result.cb = ctx->cb;
        result.user_context = ctx;

        lock<mutex> lk(mutex_);
        operations_.push_back(ctx->cb);
        contexts_[ctx->cb] = ctx;
    } else {
        result.error = last_error();
        delete ctx;
    }
#endif

    return result;
}

file_async::async_result file_async::write(string data, const size_type size, const difference_type offset) {
    async_result result;

    difference_type resolved_offset = offset;
    if (offset < 0) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        ::LARGE_INTEGER current{};
        if (::SetFilePointerEx(handle_, {}, &current, FILE_CURRENT) == FALSE) {
            result.error = last_error();
            return result;
        }
        resolved_offset = current.QuadPart;
#else
        const difference_type pos = ::lseek(handle_, 0, SEEK_CUR);
        if (pos == static_cast<difference_type>(-1)) {
            result.error = last_error();
            return result;
        }
        resolved_offset = pos;
#endif
    }

    const size_type real_size = (size == numeric_traits<size_type>::max())
                                        ? static_cast<size_type>(data.size())
                                        : min(size, static_cast<size_type>(data.size()));

    auto* ctx = new async_context(move(data));

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (ctx->cb->hEvent == nullptr) {
        delete ctx;
        result.error = last_error();
        return result;
    }

    {
        const auto off64 = static_cast<uint64_t>(resolved_offset);
        ctx->cb->Offset = static_cast<::DWORD>(off64 & 0xFFFFFFFF);
        ctx->cb->OffsetHigh = static_cast<::DWORD>(off64 >> 32);
    }

    size_type bytes_written = 0;
    if (::WriteFile(handle_, ctx->data.data(), real_size, &bytes_written, ctx->cb) == TRUE) {
        result.completed = true;
        result.bytes_transferred = bytes_written;
        delete ctx;
    } else {
        const ::DWORD err = ::GetLastError();
        if (err == ERROR_IO_PENDING) {
            result.completed = false;
            result.cb = ctx->cb;
            result.user_context = ctx;

            lock<mutex> lk(mutex_);
            operations_.push_back(ctx->cb);
            contexts_[ctx->cb] = ctx;
        } else {
            result.error = static_cast<errc>(err);
            delete ctx;
        }
    }

#else
    ctx->cb->aio_fildes = handle_;
    ctx->cb->aio_buf = const_cast<char*>(ctx->data.data());
    ctx->cb->aio_nbytes = real_size;
    ctx->cb->aio_offset = resolved_offset;
    ctx->cb->aio_sigevent.sigev_notify = SIGEV_NONE;

    if (::aio_write(ctx->cb) == 0) {
        result.completed = false;
        result.cb = ctx->cb;
        result.user_context = ctx;

        lock<mutex> lk(mutex_);
        operations_.push_back(ctx->cb);
        contexts_[ctx->cb] = ctx;
    } else {
        result.error = last_error();
        delete ctx;
    }
#endif

    return result;
}

bool file_async::wait(async_result& result, const uint32_t timeout_ms) {
    if (result.completed) {
        return true;
    }
    if (result.cb == nullptr) {
        result.error = errc::invalid_argument;
        return false;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    size_type bytes_transferred = 0;

    if (timeout_ms == numeric_traits<uint32_t>::max()) {
        if (::GetOverlappedResult(handle_, result.cb, &bytes_transferred, TRUE) == TRUE) {
            return complete_result(result, bytes_transferred);
        }
    } else {
        const ::HANDLE ev = result.cb->hEvent;
        if (ev != nullptr) {
            const ::DWORD wr = ::WaitForSingleObject(ev, timeout_ms);
            if (wr == WAIT_OBJECT_0) {
                if (::GetOverlappedResult(handle_, result.cb, &bytes_transferred, FALSE) == TRUE) {
                    return complete_result(result, bytes_transferred);
                }
            } else if (wr == WAIT_TIMEOUT) {
                result.error = errc::timed_out;
                return false;
            } else {
                result.error = last_error();
                return false;
            }
        }
    }
    result.error = last_error();
    return false;

#else
    const ::aiocb* list[1] = {result.cb};

    if (timeout_ms == 0xFFFFFFFFU) {
        if (::aio_suspend(list, 1, nullptr) == 0) {
            return check_completion(result);
        }
    } else {
        ::timespec ts{};
        ts.tv_sec = timeout_ms / 1000;
        ts.tv_nsec = static_cast<long>(timeout_ms % 1000) * 1000000L;

        if (::aio_suspend(list, 1, &ts) == 0) {
            return check_completion(result);
        } else {
            result.error = last_error();
            return false;
        }
    }
    result.error = last_error();
    return false;
#endif
}

void file_async::cancel(async_result& result) noexcept {
    if (result.completed) {
        return;
    }

    lock<mutex> lk(mutex_);
    if (result.cb == nullptr) {
        return;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (::CancelIoEx(handle_, result.cb) == FALSE) {
        size_type bytes = 0;
        if (::GetOverlappedResult(handle_, result.cb, &bytes, FALSE) == TRUE) {
            result.bytes_transferred = bytes;
        } else {
            result.error = last_error();
        }
    } else {
        result.error = errc::operation_canceled;
    }
    result.completed = true;

#else
    const int cr = ::aio_cancel(handle_, result.cb);
    if (cr == AIO_CANCELED) {
        result.error = errc::operation_canceled;
    } else {
        const ::aiocb* list[1] = {result.cb};
        ::aio_suspend(list, 1, nullptr);
        const ssize_t ret = ::aio_return(result.cb);
        result.bytes_transferred = (ret > 0) ? static_cast<size_t>(ret) : 0;
        result.error = (ret >= 0) ? errc::success : last_error();
    }
    result.completed = true;

#endif

    const auto it = find(operations_.begin(), operations_.end(), result.cb);
    if (it != operations_.end()) {
        operations_.erase(it);
    }

    const auto ci = contexts_.find(result.cb);
    if (ci != contexts_.end()) {
        delete ci->second;
        contexts_.erase(ci);
    }
#ifdef NEFORCE_PLATFORM_WINDOWS
    else {
        delete result.cb;
    }
#endif

    result.cb = nullptr;
    result.user_context = nullptr;
}

NEFORCE_END_NAMESPACE__
