#include <NeForce/core/async/thread.hpp>
#include <NeForce/core/async/thread_tracker.hpp>
#include <NeForce/core/time/clocks.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#include <windef.h>
#include <process.h>
#include <WinBase.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

atomic<int> thread_tracker::count_{0};


thread::thread_monitor::thread_monitor() noexcept {
    thread_tracker::instance().on_thread_create();
}

thread::thread_monitor::~thread_monitor() {
    thread_tracker::instance().on_thread_destroy();
}

void thread::start_thread_impl(void* args) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    unsigned int thread_id;
    handle_ = reinterpret_cast<native_handle_type>(
        ::_beginthreadex(nullptr, 0, thread_entry, args, 0, &thread_id)
    );
    if (handle_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(thread_exception("Failed to create thread"));
    }
    id_ = id(thread_id);
#else
    native_handle_type tid;
    if (::pthread_create(&tid, nullptr, thread_entry, args) != 0) {
        NEFORCE_THROW_EXCEPTION(thread_exception("Failed to create thread"));
    }
    handle_ = tid;
    id_ = id(tid);
#endif
}

thread::thread(thread&& other) noexcept
: handle_(other.handle_), id_(other.id_), state_(other.state_) {
#ifdef NEFORCE_PLATFORM_WINDOWS
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
            terminate();
        }

        handle_ = other.handle_;
        id_ = other.id_;
        state_ = other.state_;
#ifdef NEFORCE_PLATFORM_WINDOWS
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
        terminate();
    }
}

void thread::join() {
    if (!joinable()) {
        NEFORCE_THROW_EXCEPTION(thread_exception("Thread is not joinable"));
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (::WaitForSingleObject(handle_, numeric_traits<::DWORD>::max()) != WAIT_OBJECT_0) {
        NEFORCE_THROW_EXCEPTION(thread_exception("Fail to join thread"));
    }
    ::CloseHandle(handle_);
    handle_ = nullptr;
#else
    if (::pthread_join(handle_, nullptr) != 0) {
        NEFORCE_THROW_EXCEPTION(thread_exception("Thread is not joinable"));
    }
    handle_ = native_handle_type{};
#endif
    state_ = JOINED;
}

void thread::detach() {
    if (!joinable()) {
        NEFORCE_THROW_EXCEPTION(thread_exception("Thread is not detachable"));
    }
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (::CloseHandle(handle_) == FALSE) {
        NEFORCE_THROW_EXCEPTION(thread_exception("Fail to detach thread"));
    }
    handle_ = nullptr;
#else
    if (::pthread_detach(handle_) != 0) {
        NEFORCE_THROW_EXCEPTION(thread_exception("Fail to detach thread"));
    }
    handle_ = native_handle_type{};
#endif
    state_ = DETACHED;
}

NEFORCE_END_NAMESPACE__
