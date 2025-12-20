#include <MSTL/core/async/thread.hpp>
MSTL_BEGIN_NAMESPACE__

#ifdef MSTL_PLATFORM_WINDOWS__
unsigned int __stdcall thread::thread_entry(void* arg) {
#else
void* thread::thread_entry(void* arg) {
#endif
    const unique_ptr<data_base> data(static_cast<data_base*>(arg));
    try {
        data->run();
    } catch (...) {
        _MSTL terminate();
    }
#ifdef MSTL_PLATFORM_WINDOWS__
    return 0;
#else
    return nullptr;
#endif
}

void thread::start_thread_impl(void* args) {
#ifdef MSTL_PLATFORM_WINDOWS__
    unsigned int thread_id;
    handle_ = reinterpret_cast<native_handle_type>(
        ::_beginthreadex(nullptr, 0, thread_entry, args, 0, &thread_id)
    );
    if (handle_ == nullptr) {
        throw_exception(thread_exception("Failed to create thread"));
    }
    id_ = id(thread_id);
#else
    native_handle_type tid;
    if (::pthread_create(&tid, nullptr, thread_entry, args) != 0) {
        throw_exception(thread_exception("Failed to create thread"));
    }
    handle_ = tid;
    id_ = id(tid);
#endif
}

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

thread::~thread() {
    if (joinable()) {
        _MSTL terminate();
    }
}

void thread::join() {
    if (!joinable()) {
        throw_exception(thread_exception("Thread is not joinable"));
    }

#ifdef MSTL_PLATFORM_WINDOWS__
    if (::WaitForSingleObject(handle_, INFINITE) != WAIT_OBJECT_0) {
        throw_exception(thread_exception("Fail to join thread"));
    }
    ::CloseHandle(handle_);
    handle_ = nullptr;
#else
    if (::pthread_join(handle_, nullptr) != 0) {
        throw_exception(thread_exception("Thread is not joinable"));
    }
    handle_ = native_handle_type{};
#endif
    state_ = JOINED;
}

void thread::detach() {
    if (!joinable()) {
        throw_exception(thread_exception("Thread is not detachable"));
    }
#ifdef MSTL_PLATFORM_WINDOWS__
    ::CloseHandle(handle_);
    handle_ = nullptr;
#else
    if (::pthread_detach(handle_) != 0) {
        throw_exception(thread_exception("Fail to Detach thread"));
    }
    handle_ = native_handle_type{};
#endif
    state_ = DETACHED;
}

void thread::swap(thread& other) noexcept {
    _MSTL swap(handle_, other.handle_);
    _MSTL swap(id_, other.id_);
    _MSTL swap(state_, other.state_);
}

uint32_t thread::hardware_concurrency() noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::SYSTEM_INFO sysinfo;
    ::GetSystemInfo(&sysinfo);
    return static_cast<uint32_t>(sysinfo.dwNumberOfProcessors);
#else
    const long nprocs = ::sysconf(_SC_NPROCESSORS_ONLN);
    return nprocs > 0 ? static_cast<uint32_t>(nprocs) : 0;
#endif
}

MSTL_END_NAMESPACE__
