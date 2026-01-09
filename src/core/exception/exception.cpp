#include <MSTL/core/exception/exception.hpp>
#include <MSTL/core/async/atomic.hpp>
#include <MSTL/core/string/string.hpp>
#include <MSTL/core/system/console.hpp>
#include <MSTL/core/system/stacktrace.hpp>
#include <cstdlib> // std::abort
MSTL_BEGIN_NAMESPACE__

struct exception::impl {
    string info_;
    string type_;

    impl(string info, string type) noexcept
    : info_(move(info)), type_(move(type)) {}

    impl(const impl&) = default;
    impl& operator =(const impl&) = default;
    impl(impl&&) = default;
    impl& operator =(impl&&) = default;

    ~impl() noexcept = default;
};

exception::exception(const char* info, const char* type)
: ptr_(_MSTL make_unique<impl>(info, type)) {}

exception::~exception() = default;

exception::exception(const exception& e)
: ptr_(_MSTL make_unique<impl>(*e.ptr_)) {}

exception& exception::operator =(const exception& e) {
    if (this == addressof(e)) return *this;
    ptr_ = _MSTL make_unique<impl>(*e.ptr_);
    return *this;
}

exception::exception(exception&&) noexcept = default;
exception& exception::operator =(exception&&) noexcept = default;

const char* exception::what() const noexcept {
    return ptr_->info_.c_str();
}

const char* exception::type() const noexcept {
    return ptr_->type_.c_str();
}


void throw_with_stack(const exception& err) {
    printcln(color::red(), "\nException : (", err.type(), ") ", err.what());
    printcln(color::red(), stacktrace());
    throw err;
}

static atomic<terminate_handler>& get_terminate_handler() noexcept {
    static atomic<terminate_handler> handler{nullptr};
    return handler;
}

void set_terminate(terminate_handler handler) noexcept {
    get_terminate_handler().store(handler, memory_order_release);
}

void terminate() noexcept {
    const auto handler = get_terminate_handler().load(memory_order_acquire);
    if (handler) handler();
    std::abort();
}

MSTL_END_NAMESPACE__
