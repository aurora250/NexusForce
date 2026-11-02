#include <NeForce/core/async/waitable_event.hpp>
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

struct waitable_event::impl {
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


waitable_event::waitable_event(const bool initial_state, const type t) :
impl_(make_unique<impl>(t == type::manual_reset, initial_state)),
signaled_(initial_state),
type_(t) {}

waitable_event::~waitable_event() = default;

waitable_event::waitable_event(waitable_event&& other) noexcept :
impl_(move(other.impl_)),
signaled_(other.signaled_.load()),
type_(other.type_) {}

waitable_event& waitable_event::operator=(waitable_event&& other) noexcept {
    if (addressof(other) == this) {
        return *this;
    }
    this->~waitable_event();
    ::new (this) waitable_event(move(other));
    return *this;
}

void waitable_event::set() noexcept {
    if (impl_ == nullptr) {
        return;
    }
#ifdef NEFORCE_PLATFORM_WINDOWS
    signaled_.store(true, memory_order_release);
    ::SetEvent(impl_->handle_);
#else
    lock<mutex> guard(impl_->mutex_);
    signaled_.store(true, memory_order_release);
    if (type_ == type::auto_reset) {
        impl_->condition_.notify_one();
    } else {
        impl_->condition_.notify_all();
    }
#endif
}

void waitable_event::reset() noexcept {
    if (impl_ == nullptr) {
        return;
    }
#ifdef NEFORCE_PLATFORM_WINDOWS
    signaled_.store(false, memory_order_release);
    ::ResetEvent(impl_->handle_);
#else
    lock<mutex> guard(impl_->mutex_);
    signaled_.store(false, memory_order_release);
#endif
}

bool waitable_event::wait(const uint32_t timeout_ms) {
    if (impl_ == nullptr) {
        return false;
    }
#ifdef NEFORCE_PLATFORM_WINDOWS
    const bool result = ::WaitForSingleObject(impl_->handle_, timeout_ms) == WAIT_OBJECT_0;
    if (result && type_ == type::auto_reset) {
        signaled_.store(false, memory_order_release);
    }
    return result;
#else
    unique_lock<mutex> guard(impl_->mutex_);

    if (signaled_.load(memory_order_acquire)) {
        if (type_ == type::auto_reset) {
            signaled_.store(false, memory_order_release);
        }
        return true;
    }

    if (timeout_ms == numeric_traits<uint32_t>::max()) {
        impl_->condition_.wait(guard, [this] { return signaled_.load(memory_order_acquire); });
    } else {
        const auto deadline = steady_clock::now() + milliseconds(timeout_ms);
        if (!impl_->condition_.wait_until(guard, deadline, [this] { return signaled_.load(memory_order_acquire); })) {
            return false;
        }
    }

    if (type_ == type::auto_reset) {
        signaled_.store(false, memory_order_release);
    }
    return true;
#endif
}

bool waitable_event::try_wait() { return wait(0); }

bool waitable_event::is_set() const noexcept {
    if (impl_ == nullptr) {
        return false;
    }
#ifdef NEFORCE_PLATFORM_WINDOWS
    return ::WaitForSingleObject(impl_->handle_, 0) == WAIT_OBJECT_0;
#else
    return signaled_.load(memory_order_acquire);
#endif
}

void* waitable_event::native_handle() const noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return impl_ != nullptr ? impl_->handle_ : nullptr;
#else
    return nullptr;
#endif
}

NEFORCE_END_NAMESPACE__
