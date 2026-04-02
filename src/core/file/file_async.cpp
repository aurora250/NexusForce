#include <NeForce/core/file/file_async.hpp>
#ifdef NEFORCE_PLATFORM_LINUX
#    include <errno.h>
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


file_async::async_context::async_context(string&& d) :
data(move(d)),
is_write(true) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    cb = new aiocb_type{};
    cb->hEvent = ::CreateEventA(nullptr, TRUE, FALSE, nullptr);
#else
    cb = new aiocb_type{};
#endif
}

file_async::async_context::async_context(string* buf) :
buffer(buf) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    cb = new aiocb_type{};
    cb->hEvent = ::CreateEventA(nullptr, TRUE, FALSE, nullptr);
#else
    cb = new aiocb_type{};
#endif
}

file_async::async_context::~async_context() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (cb) {
        if (cb->hEvent) {
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
    result.error_code = 0;

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
    result.error_code = ::aio_error(result.cb);
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
        if (!cb) {
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
handle_(invalid_handle) {
    lock<mutex> lk(other.mutex_);
    handle_ = other.handle_;
    operations_ = move(other.operations_);
    contexts_ = move(other.contexts_);
    other.handle_ = invalid_handle;
}

file_async& file_async::operator=(file_async&& other) noexcept {
    if (addressof(other) == this) {
        return *this;
    }

    lock<mutex> lk_this(mutex_);
    lock<mutex> lk_other(other.mutex_);

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

    if (buffer.capacity() < size) {
        try {
            buffer.reserve(size);
        } catch (...) {
            result.error_code = ENOMEM;
            return result;
        }
    }
    if (buffer.size() < size) {
        buffer.resize(size);
    }

    auto* ctx = new async_context(&buffer);

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (!ctx->cb->hEvent) {
        delete ctx;
        result.error_code = static_cast<int>(::GetLastError());
        return result;
    }

    if (offset >= 0) {
        const uint64_t off64 = static_cast<uint64_t>(offset);
        ctx->cb->Offset = static_cast<::DWORD>(off64 & 0xFFFFFFFF);
        ctx->cb->OffsetHigh = static_cast<::DWORD>(off64 >> 32);
    }

    size_type bytes_read = 0;
    if (::ReadFile(handle_, buffer.data(), size, &bytes_read, ctx->cb)) {
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
            result.error_code = static_cast<int>(err);
            delete ctx;
        }
    }

#else
    ctx->cb->aio_fildes = handle_;
    ctx->cb->aio_buf = const_cast<volatile void*>(static_cast<const volatile void*>(buffer.data()));
    ctx->cb->aio_nbytes = size;
    ctx->cb->aio_offset = (offset >= 0) ? offset : 0;
    ctx->cb->aio_sigevent.sigev_notify = SIGEV_NONE;

    if (::aio_read(ctx->cb) == 0) {
        result.completed = false;
        result.cb = ctx->cb;
        result.user_context = ctx;

        lock<mutex> lk(mutex_);
        operations_.push_back(ctx->cb);
        contexts_[ctx->cb] = ctx;
    } else {
        result.error_code = errno;
        delete ctx;
    }
#endif

    return result;
}

file_async::async_result file_async::write(string data, const size_type size, const difference_type offset) {
    async_result result;

    const size_type real_size = (size == numeric_traits<size_type>::max())
                                        ? static_cast<size_type>(data.size())
                                        : min(size, static_cast<size_type>(data.size()));

    auto* ctx = new async_context(move(data));

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (!ctx->cb->hEvent) {
        delete ctx;
        result.error_code = static_cast<int>(::GetLastError());
        return result;
    }

    if (offset >= 0) {
        const uint64_t off64 = static_cast<uint64_t>(offset);
        ctx->cb->Offset = static_cast<::DWORD>(off64 & 0xFFFFFFFF);
        ctx->cb->OffsetHigh = static_cast<::DWORD>(off64 >> 32);
    }

    size_type bytes_written = 0;
    if (::WriteFile(handle_, ctx->data.data(), real_size, &bytes_written, ctx->cb)) {
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
            result.error_code = static_cast<int>(err);
            delete ctx;
        }
    }

#else
    ctx->cb->aio_fildes = handle_;
    ctx->cb->aio_buf = const_cast<char*>(ctx->data.data());
    ctx->cb->aio_nbytes = real_size;
    ctx->cb->aio_offset = (offset >= 0) ? offset : 0;
    ctx->cb->aio_sigevent.sigev_notify = SIGEV_NONE;

    if (::aio_write(ctx->cb) == 0) {
        result.completed = false;
        result.cb = ctx->cb;
        result.user_context = ctx;

        lock<mutex> lk(mutex_);
        operations_.push_back(ctx->cb);
        contexts_[ctx->cb] = ctx;
    } else {
        result.error_code = errno;
        delete ctx;
    }
#endif

    return result;
}

bool file_async::wait(async_result& result, const uint32_t timeout_ms) {
    if (result.completed) {
        return true;
    }
    if (!result.cb) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        result.error_code = ERROR_INVALID_PARAMETER;
#else
        result.error_code = EINVAL;
#endif
        return false;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    size_type bytes_transferred = 0;

    if (timeout_ms == numeric_traits<uint32_t>::max()) {
        if (::GetOverlappedResult(handle_, result.cb, &bytes_transferred, TRUE)) {
            return complete_result(result, bytes_transferred);
        }
    } else {
        const ::HANDLE ev = result.cb->hEvent;
        if (ev) {
            const ::DWORD wr = ::WaitForSingleObject(ev, timeout_ms);
            if (wr == WAIT_OBJECT_0) {
                if (::GetOverlappedResult(handle_, result.cb, &bytes_transferred, FALSE)) {
                    return complete_result(result, bytes_transferred);
                }
            } else if (wr == WAIT_TIMEOUT) {
                result.error_code = WAIT_TIMEOUT;
                return false;
            } else {
                result.error_code = static_cast<int>(::GetLastError());
                return false;
            }
        }
    }
    result.error_code = static_cast<int>(::GetLastError());
    return false;

#else
    const ::aiocb* list[1] = {result.cb};

    if (timeout_ms == 0xFFFFFFFFu) {
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
            const int err = errno;
            result.error_code = (err == EAGAIN || err == ETIMEDOUT) ? ETIMEDOUT : err;
            return false;
        }
    }
    result.error_code = errno;
    return false;
#endif
}

void file_async::cancel(async_result& result) noexcept {
    if (result.completed) {
        return;
    }

    lock<mutex> lk(mutex_);
    if (!result.cb) {
        return;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (!::CancelIoEx(handle_, result.cb)) {
        size_type bytes = 0;
        if (::GetOverlappedResult(handle_, result.cb, &bytes, FALSE)) {
            result.bytes_transferred = bytes;
        } else {
            result.error_code = static_cast<int>(::GetLastError());
        }
    } else {
        result.error_code = ERROR_OPERATION_ABORTED;
    }
    result.completed = true;

#else
    const int cr = ::aio_cancel(handle_, result.cb);
    if (cr == AIO_CANCELED) {
        result.error_code = ECANCELED;
    } else {
        const ::aiocb* list[1] = {result.cb};
        ::aio_suspend(list, 1, nullptr);
        const ssize_t ret = ::aio_return(result.cb);
        result.bytes_transferred = (ret > 0) ? static_cast<size_t>(ret) : 0;
        result.error_code = (ret >= 0) ? 0 : errno;
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
        if (result.cb) {
            delete result.cb;
        }
    }
#endif

    result.cb = nullptr;
    result.user_context = nullptr;
}

NEFORCE_END_NAMESPACE__
