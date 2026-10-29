#include <NeForce/core/system/system_event.hpp>
#include <NeForce/core/exception/system_exception.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <NeForce/core/config/windef.hpp>
#    include <windef.h>
#    include <WinBase.h>
#    include <handleapi.h>
#else
#    include <NeForce/core/async/condition_variable.hpp>
#    include <NeForce/core/async/mutex.hpp>
#    include <NeForce/core/time/clocks.hpp>
#endif
NEFORCE_BEGIN_NAMESPACE__

struct system_event::impl {
#ifdef NEFORCE_PLATFORM_WINDOWS
    void* handle_ = nullptr;

    impl(const bool manual, const bool initial_state) {
        handle_ = ::CreateEventW(nullptr, manual ? TRUE : FALSE, initial_state ? TRUE : FALSE, nullptr);
        if (handle_ == nullptr) {
            NEFORCE_THROW_EXCEPTION(system_exception("CreateEvent failed"));
        }
    }

    ~impl() {
        if (handle_ != nullptr) {
            ::CloseHandle(handle_);
        }
    }
#else
    mutex mutex_;
    condition_variable condition_;

    impl(const bool manual, const bool initial_state) {
        static_cast<void>(manual);
        static_cast<void>(initial_state);
    }

    ~impl() = default;
#endif
};


system_event::system_event(const bool initial_state, const type t) :
impl_(make_unique<impl>(t == type::manual_reset, initial_state)),
signaled_(initial_state),
type_(t) {}

system_event::~system_event() = default;

system_event::system_event(system_event&& other) noexcept :
impl_(move(other.impl_)),
signaled_(other.signaled_),
type_(other.type_) {}

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
    signaled_ = true;
    ::SetEvent(impl_->handle_);
#else
    lock<mutex> guard(impl_->mutex_);
    signaled_ = true;
    if (type_ == type::auto_reset) {
        impl_->condition_.notify_one();
    } else {
        impl_->condition_.notify_all();
    }
#endif
}

void system_event::reset() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    signaled_ = false;
    ::ResetEvent(impl_->handle_);
#else
    lock<mutex> guard(impl_->mutex_);
    signaled_ = false;
#endif
}

bool system_event::wait(const uint32_t timeout_ms) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const bool result = ::WaitForSingleObject(impl_->handle_, timeout_ms) == WAIT_OBJECT_0;
    if (result && type_ == type::auto_reset) {
        signaled_ = false;
    }
    return result;
#else
    unique_lock<mutex> guard(impl_->mutex_);

    if (signaled_) {
        if (type_ == type::auto_reset) {
            signaled_ = false;
        }
        return true;
    }

    if (timeout_ms == numeric_traits<uint32_t>::max()) {
        impl_->condition_.wait(guard, [this] { return signaled_; });
    } else {
        const auto deadline = system_clock::now() + milliseconds(timeout_ms);
        if (!impl_->condition_.wait_until(guard, deadline, [this] { return signaled_; })) {
            return false;
        }
    }

    if (type_ == type::auto_reset) {
        signaled_ = false;
    }
    return true;
#endif
}

bool system_event::try_wait() noexcept { return wait(0); }

#ifdef NEFORCE_PLATFORM_WINDOWS
void* system_event::native_handle() const noexcept { return impl_->handle_; }
#endif

NEFORCE_END_NAMESPACE__
