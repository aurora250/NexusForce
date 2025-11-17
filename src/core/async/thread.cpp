#include <MSTL/core/async/thread.hpp>
MSTL_BEGIN_NAMESPACE__

thread::thread(thread&& other) noexcept
    : handle_(other.handle_), id_(other.id_), state_(other.state_) {
#ifdef MSTL_PLATFORM_WINDOWS__
    other.handle_ = nullptr;
#else
    other.handle_ = native_handle_type{};
#endif
    other.id_ = id{};
    other.state_ = NOT_A_THREAD;
}

thread& thread::operator =(thread&& other) noexcept {
    if (this != &other) {
        if (joinable()) {
            _MSTL terminate();
        }

        handle_ = other.handle_;
        id_ = other.id_;
        state_ = other.state_;
#ifdef MSTL_PLATFORM_WINDOWS__
        other.handle_ = nullptr;
#else
        other.handle_ = native_handle_type{};
#endif
        other.id_ = id{};
        other.state_ = NOT_A_THREAD;
    }
    return *this;
}

void thread::join() {
    if (!joinable()) {
        Exception(ThreadOperationError("Thread is not joinable"));
    }

#ifdef MSTL_PLATFORM_WINDOWS__
    if (::WaitForSingleObject(handle_, INFINITE) != WAIT_OBJECT_0) {
        Exception(ThreadOperationError("Fail to join thread"));
    }
    ::CloseHandle(handle_);
    handle_ = nullptr;
#else
    if (::pthread_join(handle_, nullptr) != 0) {
        Exception(ThreadOperationError("Thread is not joinable"));
    }
    handle_ = native_handle_type{};
#endif
    state_ = JOINED;
}

void thread::detach() {
    if (!joinable()) {
        Exception(ThreadOperationError("Thread is not detachable"));
    }
#ifdef MSTL_PLATFORM_WINDOWS__
    ::CloseHandle(handle_);
    handle_ = nullptr;
#else
    if (::pthread_detach(handle_) != 0) {
        Exception(ThreadOperationError("Fail to Detach thread"));
    }
    handle_ = native_handle_type{};
#endif
    state_ = DETACHED;
}

MSTL_END_NAMESPACE__
