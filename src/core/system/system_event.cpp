#include <NeForce/core/system/system_event.hpp>
#include <NeForce/core/exception/error_category.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <Windows.h>
#else
#    include <ctime>
#endif
NEFORCE_BEGIN_NAMESPACE__

system_event::system_event(bool initial_state, const type type) :
type_(type) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::BOOL manual = (type == type::manual_reset) ? TRUE : FALSE;
    handle_ = ::CreateEventA(nullptr, manual, initial_state ? TRUE : FALSE, nullptr);
    if (handle_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(system_exception("CreateEvent failed"));
    }
#else
    auto m = ::new (std::nothrow) pthread_mutex_t;
    auto c = ::new (std::nothrow) pthread_cond_t;
    mutex_.reset(m);
    cond_.reset(c);

    if (!mutex_ || !cond_) {
        mutex_.reset();
        cond_.reset();
        NEFORCE_THROW_EXCEPTION(system_exception("Failed to allocate pthread objects"));
    }

    ::pthread_mutexattr_t mutex_attr;
    ::pthread_mutexattr_init(&mutex_attr);
    ::pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK);
    int ret = ::pthread_mutex_init(mutex_.get(), &mutex_attr);
    ::pthread_mutexattr_destroy(&mutex_attr);
    if (ret != 0) {
        mutex_.reset();
        cond_.reset();
        NEFORCE_THROW_EXCEPTION(system_exception("pthread_mutex_init failed"));
    }

    ::pthread_condattr_t cond_attr;
    ::pthread_condattr_init(&cond_attr);
    ::pthread_condattr_setclock(&cond_attr, CLOCK_REALTIME);
    ret = ::pthread_cond_init(cond_.get(), &cond_attr);
    ::pthread_condattr_destroy(&cond_attr);
    if (ret != 0) {
        mutex_.reset();
        cond_.reset();
        NEFORCE_THROW_EXCEPTION(system_exception("pthread_cond_init failed"));
    }

    signaled_ = initial_state;
#endif
}

system_event::~system_event() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (handle_ != nullptr) {
        ::CloseHandle(handle_);
    }
#endif
}

system_event::system_event(system_event&& other) noexcept :
#ifdef NEFORCE_PLATFORM_WINDOWS
handle_(other.handle_),
type_(other.type_) {
    other.handle_ = nullptr;
}
#else
mutex_(other.mutex_.release()),
cond_(other.cond_.release()),
signaled_(other.signaled_),
type_(other.type_) {
}
#endif

system_event& system_event::operator=(system_event&& other) noexcept {
    if (addressof(other) == this) {
        return *this;
    }
    this->~system_event();
    ::new (this) system_event(move(other));
    return *this;
}

void system_event::set() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::SetEvent(handle_);
#else
    ::pthread_mutex_lock(mutex_.get());
    signaled_ = true;
    if (type_ == type::auto_reset) {
        ::pthread_cond_signal(cond_.get());
    } else {
        ::pthread_cond_broadcast(cond_.get());
    }
    ::pthread_mutex_unlock(mutex_.get());
#endif
}

void system_event::reset() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::ResetEvent(handle_);
#else
    ::pthread_mutex_lock(mutex_.get());
    signaled_ = false;
    ::pthread_mutex_unlock(mutex_.get());
#endif
}

bool system_event::wait(uint32_t timeout_ms) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return ::WaitForSingleObject(handle_, timeout_ms) == WAIT_OBJECT_0;
#else
    ::pthread_mutex_lock(mutex_.get());

    if (signaled_) {
        if (type_ == type::auto_reset) {
            signaled_ = false;
        }
        ::pthread_mutex_unlock(mutex_.get());
        return true;
    }

    if (timeout_ms == numeric_traits<uint32_t>::max()) {
        while (!signaled_) {
            int ret = ::pthread_cond_wait(cond_.get(), mutex_.get());
            (void) ret;
        }
    } else {
        struct ::timespec ts;
        ::clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += timeout_ms / 1000;
        ts.tv_nsec += (timeout_ms % 1000) * 1000000;
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000;
        }

        int ret = 0;
        while (!signaled_ && ret == 0) {
            ret = ::pthread_cond_timedwait(cond_.get(), mutex_.get(), &ts);
            if (ret == EINTR) {
                ret = 0;
            }
        }

        if (ret == ETIMEDOUT && !signaled_) {
            ::pthread_mutex_unlock(mutex_.get());
            return false;
        }
    }

    if (type_ == type::auto_reset) {
        signaled_ = false;
    }
    ::pthread_mutex_unlock(mutex_.get());
    return true;
#endif
}

NEFORCE_END_NAMESPACE__
