#include <NeForce/core/async/lazy_thread.hpp>
NEFORCE_BEGIN_NAMESPACE__

lazy_thread::lazy_thread(lazy_thread&& other) noexcept :
func_(move(other.func_)),
thread_(move(other.thread_)) {}

lazy_thread& lazy_thread::operator=(lazy_thread&& other) noexcept {
    if (addressof(other) == this) {
        return *this;
    }

    if (joinable()) {
        try {
            thread_.join();
        } catch (...) {
            terminate();
        }
    }
    func_ = move(other.func_);
    thread_ = move(other.thread_);

    return *this;
}

lazy_thread::~lazy_thread() {
    if (joinable()) {
        thread_.join();
    }
}

void lazy_thread::start() {
    if (!func_) {
        NEFORCE_THROW_EXCEPTION(thread_exception("No callable stored in lazy_thread"));
    }
    if (joinable()) {
        NEFORCE_THROW_EXCEPTION(thread_exception("Thread already started"));
    }
    thread_.start(_NEFORCE move(func_));
    func_ = nullptr;
}

NEFORCE_END_NAMESPACE__
